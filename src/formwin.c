/*
 * Create the form edit window and the form edit canvas window.
 *
 *	create_formedit_window		Create form edit window and realize
 *	fillout_formedit_widget_by_code	Redraw single item
 */

#include "config.h"
#include <unistd.h>
#include <float.h>
#include <stdlib.h>
#include <stddef.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "canv-widget.h"

// This window can't be closed.  Probably ought to pop up a dialog
// like the canvas does.
class QDialogNoClose : public QDialog {
    void closeEvent(QCloseEvent *event) {
	    event->ignore();
    }
};

static void formedit_callback(int);
static void menu_callback(QTableWidget *w, int x, int y);
static void key_callback(int x, int y, int c = 0);
static int readback_item(int);

static bool		have_shell = false;	/* message popup exists if true */
static QDialog		*shell;		/* popup menu shell */
static FORM		*form;		/* current form definition */
static QWidget		*scroll_w;	/* the widget inside the scroll area */
static QFrame		*chart;		/* the widget for chart options */
static QTableWidget	*menu_w;	/* the menu editor widget */
static QTableWidget	*key_w;		/* the reference field editor widget */
static GrokCanvas	*canvas;

const char		plan_code[] = "tlwWrecnmsSTA";	/* code EA_PLDT..EA_PLNAL */


#define ALL ~0U
#define ANY ~1U  // all except IT_NULL
#define INP 1U<<IT_INPUT
#define TIM 1U<<IT_TIME
#define ITP 1U<<IT_NOTE | INP | TIM
#define NUM 1U<<IT_NUMBER
#define ITX ITP | NUM
#define TXT 1U<<IT_PRINT  | ITX
#define FT2 TXT | 1U<<IT_RADIO | 1U<<IT_FLAGS
#define FLG 1U<<IT_CHOICE | 1U<<IT_FLAG
#define SNG 1U<<IT_MENU | 1U<<IT_RADIO
#define MUL 1U<<IT_MULTI | 1U<<IT_FLAGS
#define MNU INP  | SNG | MUL
#define BAS ITX | FLG | SNG | MUL | FKY
#define RON BAS | 1U<<IT_INV_FKEY
#define PLN ITX | FLG | SNG
#define IDF TXT | FLG | SNG | MUL
#define BUT 1U<<IT_BUTTON
#define CHA 1U<<IT_CHART
#define FKY 1U<<IT_FKEY
#define AFK FKY | 1U<<IT_INV_FKEY

static const struct {
    int type;
    const char *name;
} item_types[] = {
	{ IT_INPUT,  "Input"		},
	{ IT_NUMBER, "Number"		},
	{ IT_TIME,   "Time"		},
	{ IT_NOTE,   "Note"		},
	{ IT_LABEL,  "Label"		},
	{ IT_PRINT,  "Print"		},
	{ IT_CHOICE, "Choice"		},
	{ IT_RADIO,  "Choice Group"	},
	{ IT_MENU,   "Choice Menu"	},
	{ IT_FLAG,   "Flag"		},
	{ IT_FLAGS,  "Flag Group"	},
	{ IT_MULTI,  "Flag List"	},
	{ IT_BUTTON, "Button"		},
	{ IT_CHART,  "Chart"		},
	{ IT_FKEY,   "Reference"	},
	{ IT_INV_FKEY, "Referers"	},
	
};

static QStringList menu_col_labels = {
	"Label", "Field Name", "DB Col", "Code", "Sum Col", "Sum Width",
	"Sum Code" };

static const struct {
    int  fieldoff;
    bool isint;
} menu_cols[] = {
    { offsetof(MENU, label) },
    { offsetof(MENU, name) },
    { offsetof(MENU, column),   true },
    { offsetof(MENU, flagcode) },
    { offsetof(MENU, sumcol),   true },
    { offsetof(MENU, sumwidth), true },
    { offsetof(MENU, flagtext) }
};
#define MENU_FIELD_COLUMN 2

#define MENU_COMBO  (1<<0)			/* only label */
#define MENU_SINGLE (1<<0 | 1<<3 | 1<<6)	/* label, flagcode, flagtext */
#define MENU_MULTI  ~0				/* everything */

static int menu_col_mask(const ITEM *item)
{
	if(item->type == IT_INPUT)
		return MENU_COMBO;
	else if(item->type == IT_MENU || item->type == IT_RADIO || !IFL(item->,MULTICOL))
		return MENU_SINGLE;
	else
		return MENU_MULTI;
}

static void fill_menu_row(QTableWidget *tw, const ITEM *item, int row)
{
	for(int j = 0; j < ALEN(menu_cols); j++) {
		void *p = (char *)&item->menu[row] + menu_cols[j].fieldoff;
		QTableWidgetItem *twi = tw->item(row, j);
		if(!twi)
			tw->setItem(row, j, (twi = new QTableWidgetItem));
		if(row == item->nmenu)
			twi->setText("");
		else if(menu_cols[j].isint)
			twi->setText(qsprintf("%d", *(int *)p));
		else
			twi->setText(STR(*(char **)p));
	}
}

static void resize_menu_table(QTableWidget *tw);
static void fill_menu_table(QTableWidget *tw)
{
	QSignalBlocker sb(tw);
	if(!canvas || canvas->curr_item >= form->nitems) {
		tw->setRowCount(0);
		return;
	}
	const ITEM *item = form->items[canvas->curr_item];
	if(!((1U<<item->type) & (MNU))) {
		tw->setRowCount(8);
		return;
	}
	int row, col;
	if(tw->rowCount()) {
		row = tw->currentRow();
		col = tw->currentColumn();
	} else
		row = col = -1;
	int mask = menu_col_mask(item);
	for(int i = 0; i < ALEN(menu_cols); i++)
		tw->setColumnHidden(i, !(mask & (1<<i)));
	tw->horizontalHeader()->setVisible(mask != MENU_COMBO);
	tw->setRowCount(item->nmenu + 1);
	for(int i = 0; i <= item->nmenu; i++)
	    fill_menu_row(tw, item, i);
	resize_menu_table(tw);
	if(row >= 0)
		tw->setCurrentCell(row, col);
}

static void resize_menu_table(QTableWidget *tw)
{
	// FIXME:  How do I determine the grid line width?  It's there
	// whether the grid is drawn or not.
	if(!canvas || canvas->curr_item >= form->nitems)
		return;
	const ITEM *item = form->items[canvas->curr_item];
	int mask = menu_col_mask(item);
	tw->ensurePolished();
	int w = 0;
	tw->horizontalHeader()->setStretchLastSection(false);
	for(int i = 0; i < ALEN(menu_cols); i++)
		if(mask & (1<<i)) {
			int min = menu_cols[i].isint ? 40 : 100;
			tw->resizeColumnToContents(i);
			if(tw->columnWidth(i) < min)
				tw->setColumnWidth(i, min);
			w += tw->columnWidth(i);
			// not sure if grid is included above; probably not
			w++;
		}
	// 1 too many grid lines, in any case
	w--;
	tw->horizontalHeader()->setStretchLastSection(true);
	// this resize probably isn't necessary, but better to be safe
	tw->resizeRowsToContents();
	int h = tw->rowHeight(0) * (item->nmenu + 1);
	// actually, I guess the grid isn't an issue
	// h += item->nmenu; // assume 1-pixel grid
	// however, I do observe a 2-pixel shortage.  I have no idea why.
	h+=2;
	if(mask != MENU_COMBO)
		h += tw->horizontalHeader()->sizeHint().height();
	if(w > tw->width())
		h += tw->horizontalScrollBar()->height();
	tw->setMaximumHeight(h);
	tw->setMinimumHeight(h);
	tw->updateGeometry();
}

void resolve_fkey_fieldsel(const FORM *f, int idx, FKEY &fk)
{
	int item = -1, menu;
	fk.item = 0;
	fk.menu = 0;
	fk.index = -1;
	if (idx < 0) {
		zfree(fk.name);
		fk.name = 0;
		return;
	}
	for (int i=0; i < f->nitems; i++) {
		ITEM *it = f->items[i];
		if (!IN_DBASE(it->type))
			continue;
		if (IFL(it->,MULTICOL)) {
			if (it->nmenu)
				item++;
			for (menu=0; menu < it->nmenu; menu++)
				if (!idx--) {
					fk.item = it;
					fk.menu = &it->menu[menu];
					fk.index = item + menu * f->nitems;
					if(!fk.name || strcmp(fk.name, fk.menu->name)) {
						zfree(fk.name);
						fk.name = strdup(fk.menu->name);
					}
					return;
				}
		} else {
			item++;
			if (!idx--) {
				fk.item = it;
				fk.menu = 0;
				fk.index = item;
				if(!fk.name || strcmp(fk.name, it->name)) {
					zfree(fk.name);
					fk.name = strdup(it->name);
				}
				return;
			}
		}
	}
}

static QString fkey_name(const FORM *f, int item, int menu) {
	const ITEM *fitem = f->items[item];
	QString s = fitem->label ? fitem->label : fitem->name ? fitem->name : "";
	if (IFL(fitem->,MULTICOL)) {
		const MENU *m = &fitem->menu[menu];
		if (!s.isEmpty())
			s += '/';
		s += m->label ? m->label : m->name ? m->name : QString::number(m->column);
	} else if (s.isEmpty())
		s = QString::number(fitem->column);
	return s;
}

static QStringList key_col_labels = {
	"Key", "Display", "Field" };

enum {
	KEY_FIELD_KEY, KEY_FIELD_DISPLAY, KEY_FIELD_FIELD };

QComboBox *make_fkey_field_select(const FORM *fform)
{
	QComboBox *cb = new QComboBox;
	cb->addItem("");
	if (fform) {
		for (int i=0; i < fform->nitems; i++) {
			const ITEM *it = fform->items[i];
			if (!IN_DBASE(it->type))
				continue;
			if (IFL(it->,MULTICOL)) {
				for (int m=0; m < it->nmenu; m++)
					cb->addItem(fkey_name(fform,
							      i, m));
			} else
				cb->addItem(fkey_name(fform, i, 0));
		}
	}
	return cb;
}

static void fill_key_row(QTableWidget *tw, ITEM *item, int row)
{
	bool blank = row >= item->nfkey;
	FKEY *fk = &item->fkey[row];
	resolve_fkey_fields(item);
	const FORM *fform = item->fkey_form;
	QComboBox *cb = make_fkey_field_select(fform);
	if (blank)
		cb->setCurrentIndex(0);
	else if (!fk->item) {
		cb->addItem(fk->name);
		cb->setCurrentText(fk->name);
	} else
		cb->setCurrentText(fkey_name(item->fkey_form,
					     fk->index % item->fkey_form->nitems,
					     fk->index / item->fkey_form->nitems));
	tw->setCellWidget(row, KEY_FIELD_FIELD, cb);
	set_popup_cb(cb, key_callback(KEY_FIELD_FIELD, row, c), int, c);
	QCheckBox *chk = new QCheckBox;
	chk->setCheckState(blank || !fk->key ? Qt::Unchecked : Qt::Checked);
	bool ken = !blank;
	if (ken && item->type == IT_FKEY)
		ken = !fk->item || (fk->item->type != IT_NOTE &&
				    !IFL(item->,FKEY_MULTI));
	else if(ken && item->type == IT_INV_FKEY)
		ken = !fk->item || fk->item->type == IT_FKEY;
	chk->setEnabled(ken);
	tw->setCellWidget(row, KEY_FIELD_KEY, chk);
	set_button_cb(chk, key_callback(KEY_FIELD_KEY, row, c), bool c);
	chk = new QCheckBox;
	chk->setCheckState(blank || !fk->display ? Qt::Unchecked : Qt::Checked);
	chk->setEnabled(!blank);
	tw->setCellWidget(row, KEY_FIELD_DISPLAY, chk);
	set_button_cb(chk, key_callback(KEY_FIELD_DISPLAY, row, c), bool c);
}

static void resize_key_table(QTableWidget *tw);
static void fill_key_table(QTableWidget *tw)
{
	QSignalBlocker sb(tw);
	if(!canvas || canvas->curr_item >= form->nitems) {
		tw->setRowCount(0);
		return;
	}
	ITEM *item = form->items[canvas->curr_item];
	if(!((1U<<item->type) & (AFK))) {
		tw->setRowCount(0);
		return;
	}
	int row, col;
	if(tw->rowCount()) {
		row = tw->currentRow();
		col = tw->currentColumn();
	} else
		row = col = -1;
	tw->horizontalHeader()->setVisible(true);
	tw->setRowCount(item->nfkey + 1);
	for(int i = 0; i <= item->nfkey; i++)
	    fill_key_row(tw, item, i);
	resize_key_table(tw);
	if(row >= 0)
		tw->setCurrentCell(row, col);
}

static void resize_key_table(QTableWidget *tw)
{
	// FIXME:  How do I determine the grid line width?  It's there
	// whether the grid is drawn or not.
	if(!canvas || canvas->curr_item >= form->nitems)
		return;
	const ITEM *item = form->items[canvas->curr_item];
	tw->ensurePolished();
	int w = 0;
	tw->horizontalHeader()->setStretchLastSection(false);
	for(int i = 0; i < key_col_labels.length(); i++) {
		int min = i > 0 ? 40 : 100;
		tw->resizeColumnToContents(i);
		if(tw->columnWidth(i) < min)
			tw->setColumnWidth(i, min);
		w += tw->columnWidth(i);
		// not sure if grid is included above; probably not
		w++;
	}
	// 1 too many grid lines, in any case
	w--;
	tw->horizontalHeader()->setStretchLastSection(true);
	// this resize probably isn't necessary, but better to be safe
	tw->resizeRowsToContents();
	int h = tw->rowHeight(0) * (item->nfkey + 1);
	// actually, I guess the grid isn't an issue
	// h += item->nfkey; // assume 1-pixel grid
	// however, I do observe a 2-pixel shortage.  I have no idea why.
	h+=2;
	h += tw->horizontalHeader()->sizeHint().height();
	if(w > tw->width())
		h += tw->horizontalScrollBar()->height();
	tw->setMaximumHeight(h);
	tw->setMinimumHeight(h);
	tw->updateGeometry();
}

enum edact {
	EA_FORMNM = 1, EA_DBNAME, EA_FRO, EA_HASTS, EA_ISPROC, EA_EDPROC,
	EA_DBDELIM, EA_ARDELIM, EA_ARESC, EA_SUMHT, EA_FCMT,
	EA_QUERY, EA_REFBY, EA_FHELP, EA_CHECK, EA_PREVW, EA_HELP, EA_CANCEL,
	EA_DONE, EA_ADDWID, EA_LAST_FWIDE = EA_ADDWID, EA_DELWID,
	EA_TYPE, EA_FLSRCH, EA_FLRO, EA_FLNSRT, EA_DEFSRT, EA_FLMUL,
	EA_FKHDR, /* EA_FKSRCH, */ EA_FKMUL,
	EA_FLDNM, EA_FIRST_MULTI_OVR = EA_FLDNM, EA_COL, EA_SUMCOL, EA_SUMWID,
	EA_LAST_MULTI_OVR = EA_SUMWID, EA_SUMPR, EA_FLCODE, EA_FLSUM,
	EA_ISDATE, EA_ISTIME, EA_ISDTTM, EA_ISDUR, EA_DTWID, EA_DTCAL,
	EA_LABJL, EA_LABJC, EA_LABJR, EA_LABFH, EA_LABFO, EA_LABFN, EA_LABFB,
	EA_LABFC, EA_LABEL, EA_INJL, EA_INJC, EA_INJR, EA_INFH, EA_INFO,
	EA_INFN, EA_INFB, EA_INFC, EA_INLEN, EA_INMIN, EA_INMAX, EA_INDIG,
	EA_INDEF, EA_MENUST, EA_MENUDY, EA_MENUSD, EA_FKDB, EA_PLDT, EA_PLLEN,
	EA_PLEW, EA_PLLW, EA_PLDAYR, EA_PLEND, EA_PLCOL, EA_PLNOTE, EA_PLMSG,
	EA_PLSCR, EA_PLSUSP, EA_PLNTM, EA_PLNAL, EA_PLIF, EA_GREYIF, EA_HIDEIF,
	EA_ROIF, EA_SKIPIF, EA_BUTACT,
	EA_CHXMIN, EA_CHXMAX, EA_CHXAUT, EA_CHYMIN, EA_CHYMAX, EA_CHYAUT,
	EA_CHXGRD, EA_CHYGRD, EA_CHXSNP, EA_CHYSNP,
	EA_CHADD, EA_CHDEL, EA_CHCUR, EA_CHNEXT, EA_CHPREV,
	EA_CHLINE, EA_FIRST_CHART = EA_CHLINE, EA_CHXFAT, EA_CHYFAT,
	EA_CHNOTIF, EA_CHCOL, EA_CHLAB,
	EA_CHXNXT, EA_CHXPREV, EA_CHXEXPM, EA_CHXDRGM,
	EA_CHYNXT, EA_CHYPREV, EA_CHYEXPM, EA_CHYDRGM,
	EA_CHXEXPR, EA_CHXDRAG, EA_CHXDMUL, EA_CHXDPL, EA_CHYEXPR, EA_CHYDRAG,
	EA_CHYDMUL, EA_CHYDPL, EA_CHXSZEX, EA_CHXSZ, EA_CHYSZEX, EA_CHYSZ,
	EA_LAST_CHART = EA_CHYSZ
};
#define EA_NONE ((enum edact)0)
/*
 * next free: 10a,115 (global), 245 (field), 30b (chart component)
 */

static struct _template {
	unsigned
	int	sensitive;	/* when type is n, make sensitive if & 1<<n */
	int	type;		/* Text, Label, Rradio, Fflag, -line */
	enum edact code;	/* code given to (global) callback routine */
	const char *text;	/* label string */
	const char *help;	/* label string */
	QWidget	*widget;	/* allocated widget */
	int	role;		/* for buttons using standard labels */
				/* also, offset of text field for combos */
} tmpl[] = {
	{ ALL, 'L', EA_NONE,	"Form name:",		"fe_form",	},
	{ ALL, 'T', EA_FORMNM,	" ",			"fe_form",	},
	{ ALL, 'L', EA_NONE,	"Referenced database:",	"fe_form",	},
	{ ALL, 't', EA_DBNAME,	" ",			"fe_form",	},
	{ ALL, 'f', EA_FRO,	"Read only",		"fe_rdonly",	},
	{ ALL, 'f', EA_HASTS,	"Timestamps/Synchronizable","fe_sync",	},
	{ ALL, 'f', EA_ISPROC,	"Procedural",		"fe_proc",	},
	{ ALL, 'B', EA_EDPROC,	"Edit",			"fe_proc",	},
	{ ALL, 'L', EA_NONE,	"dbase field delim:",	"fe_delim",	},
	{ ALL, 't', EA_DBDELIM,	" ",			"fe_delim",	},
	{ ALL, 'l', EA_NONE,	"Array elt delim:",	"fe_adelim",	},
	{ ALL, 't', EA_ARDELIM,	" ",			"fe_adelim",	},
	{ ALL, 'l', EA_NONE,	"Array elt esc:",	"fe_adelim",	},
	{ ALL, 't', EA_ARESC,	" ",			"fe_adelim",	},
	{ ALL, 'l', EA_NONE,	"Max Sum row ht:",	"fe_form",	},
	{ ALL, 'd', EA_SUMHT,	" ",			"fe_form",	},
	{   0, 'F', EA_NONE,	" ",			0,		},
	{ ALL, 'L', EA_NONE,	"Comment:",		"fe_cmt",	},
	{ ALL, 'T', EA_FCMT,	" ",			"fe_cmt",	},

	{ ALL, 'L', EA_NONE,	"Form",			0,		},
	{ ALL, 'B', EA_QUERY,	"Queries",		"fe_query",	},
	{ ALL, 'B', EA_REFBY,	"Referers",		"fe_ref",	},
	{ ALL, 'B', EA_FHELP,	"Def Help",		"fe_defhelp",	},
	{ ALL, 'B', EA_CHECK,	"Check",		"fe_check",	},
	{ ALL, 'B', EA_PREVW,	"Preview",		"fe_preview",	},
	{ ALL, 'B', EA_HELP,	NULL,			"edit",		0, dbbb(Help)},
	{ ALL, 'B', EA_CANCEL,	NULL,			"fe_cancel",	0, dbbb(Cancel)},
	{ ALL, 'B', EA_DONE,	"Done",			"fe_done",	},
	{ ALL, '-', EA_NONE,	" ",			0,		},
	{ ALL, 'L', EA_NONE,	"Field",		"fe_buts",	},
	{ ALL, 'B', EA_ADDWID,	"Add",			"fe_add",	},
	{ ANY, 'B', EA_DELWID,	"Delete",		"fe_delete",	},
	{   0, '[', EA_NONE,	"attrs",		0,		},

	{ ANY, 'L', EA_NONE,	"Field type:",		"fe_type",	},
	{ ANY, 'I', EA_TYPE,	" ",			"fe_type",	},
	{ ANY, 'L', EA_NONE,	"Flags:",		"fe_flags",	},
	{   0, 'F', EA_NONE,	" ",			0,		},
	{ BAS, 'f', EA_FLSRCH,	"Searchable",		"fe_flags",	},
	{ RON, 'f', EA_FLRO,	"Read only",		"fe_flags",	},
	{ BAS, 'f', EA_FLNSRT,	"Not sortable",		"fe_flags",	},
	{ BAS, 'f', EA_DEFSRT,	"Default sort",		"fe_flags",	},
	{ MUL, 'f', EA_FLMUL,	"Multi-field",		"fe_menu",	},
	{   0, 'F', EA_NONE,	" ",			0,		},
	{ FKY, 'L', EA_NONE,	" ",			0,		},
	{ AFK, 'f', EA_FKHDR,	"Headers",		"fe_ref",	},
#if 0
	{ AFK, 'f', EA_FKSRCH,	"Subsearch",		"fe_ref",	},
#endif
	{ FKY, 'f', EA_FKMUL,	"Multi-Ref",		"fe_ref",	},
	{   0, 'F', EA_NONE,	" ",			0,		},
	{ ANY, 'L', EA_NONE,	"Internal field name:",	"fe_int",	},
	{ ANY, 't', EA_FLDNM,	" ",			"fe_int",	},
	{ BAS, 'L', EA_NONE,	"Database column:",	"fe_column",	},
	{ BAS, 'i', EA_COL,	" ",			"fe_column",	0, 999 },
	{ BAS, 'l', EA_NONE,	"Summary column:",	"fe_sum",	},
	{ BAS, 'i', EA_SUMCOL,	" ",			"fe_sum",	0, 999 },
	{ BAS, 'l', EA_NONE,	"Width in summary:",	"fe_sum",	},
	{ BAS, 'i', EA_SUMWID,	" ",			"fe_sum",	0, 100 },
	{ BAS, 'L', EA_NONE,	"Show in summary:",	"fe_sump",	},
	{ BAS, 'T', EA_SUMPR,	" ",			"fe_sump",	},
	{ FLG, 'L', EA_NONE,	"Choice/flag code:",	"fe_flag",	},
	{ FLG, 't', EA_FLCODE,	" ",			"fe_flag",	},
	{ FLG, 'l', EA_NONE,	"shown in summary as",	"fe_flag",	},
	{ FLG, 't', EA_FLSUM,	" ",			"fe_flag",	},
	{ TIM, 'L', EA_NONE,	"Time format:",		"fe_time",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ TIM, 'r', EA_ISDATE,	"Date",			"fe_time",	},
	{ TIM, 'r', EA_ISTIME,	"Time",			"fe_time",	},
	{ TIM, 'r', EA_ISDTTM,	"Date+time",		"fe_time",	},
	{ TIM, 'r', EA_ISDUR,	"Duration",		"fe_time",	},
	{ TIM, 'f', EA_DTWID,	"Widget",		"fe_time",	},
	{ TIM, 'f', EA_DTCAL,	"Calendar",		"fe_time",	},
	{ ANY, '-', EA_NONE,	" ",			0,		},

	{ ANY, 'L', EA_NONE,	"Label Justification:",	"fe_ljust",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ ANY, 'r', EA_LABJL,	"left",			"fe_ljust",	},
	{ ANY, 'r', EA_LABJC,	"center",		"fe_ljust",	},
	{ ANY, 'r', EA_LABJR,	"right",		"fe_ljust",	},
	{ ANY, 'L', EA_NONE,	"Label font:",		"fe_ljust",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ ANY, 'r', EA_LABFH,	"Helv",			"fe_lfont",	},
	{ ANY, 'r', EA_LABFO,	"HelvO",		"fe_lfont",	},
	{ ANY, 'r', EA_LABFN,	"HelvN",		"fe_lfont",	},
	{ ANY, 'r', EA_LABFB,	"HelvB",		"fe_lfont",	},
	{ ANY, 'r', EA_LABFC,	"Courier",		"fe_lfont",	},
	{ ANY, 'L', EA_NONE,	"Label text",		"fe_ltxt",	},
	{ ANY, 'T', EA_LABEL,	" ",			"fe_ltxt",	},
	{ ANY, '-', EA_NONE,	" ",			0,		},

	{ TXT, 'L', EA_NONE,	"Input Justification:",	"fe_ijust",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ TXT, 'r', EA_INJL,	"left",			"fe_ijust",	},
	{ TXT, 'r', EA_INJC,	"center",		"fe_ijust",	},
	{ TXT, 'r', EA_INJR,	"right",		"fe_ijust",	},
	{ FT2, 'L', EA_NONE,	"Input font:",		"fe_ijust",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ FT2, 'r', EA_INFH,	"Helv",			"fe_ifont",	},
	{ FT2, 'r', EA_INFO,	"HelvO",		"fe_ifont",	},
	{ FT2, 'r', EA_INFN,	"HelvN",		"fe_ifont",	},
	{ FT2, 'r', EA_INFB,	"HelvB",		"fe_ifont",	},
	{ FT2, 'r', EA_INFC,	"Courier",		"fe_ifont",	},
	{ ITP, 'L', EA_NONE,	"Max input length:",	"fe_range",	},
	{ ITP, 'i', EA_INLEN,	" ",			"fe_range",	0, 999999},
	{ NUM, 'L', EA_NONE,	"Min Value",		"fe_range",	},
	{ NUM, 'd', EA_INMIN,	" ",			"fe_range",	},
	{ NUM, 'l', EA_NONE,	"Max Value",		"fe_range",	},
	{ NUM, 'd', EA_INMAX,	" ",			"fe_range",	},
	{ NUM, 'l', EA_NONE,	"Digits",		"fe_range",	},
	{ NUM, 'i', EA_INDIG,	" ",			"fe_range",	0, 20},
	{ IDF, 'L', EA_NONE,	"Input default:",	"fe_def",	},
	{ IDF, 'T', EA_INDEF,	" ",			"fe_def",	},
	{ MNU, 'L', EA_NONE,	"Menu",			"fe_menu",	},
	{ MNU, 'M', EA_NONE,	" ",			"fe_menu",	},
	{   0, 'R', EA_NONE,	" ",			"fe_menu",	},
	{ INP, 'r', EA_MENUST,	"Static",		"fe_menu",	},
	{ INP, 'r', EA_MENUDY,	"Dynamic",		"fe_menu",	},
	{ INP, 'r', EA_MENUSD,	"All",			"fe_menu",	},
	{ AFK, 'L', EA_NONE,	"Foreign DB:",		"fe_ref",	},
	{ AFK, 'D', EA_FKDB,	" ",			"fe_ref",	},
	{ AFK, 'L', EA_NONE,	"Foreign Fields",	"fe_ref",	},
	{ AFK, 'K', EA_NONE,	" ",			"fe_ref",	},
	{ TXT, '-', EA_NONE,	" ",			0,		},

	{ PLN, 'L', EA_NONE,	"Calendar interface:",	"fe_plan",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
    /* FIXME:  make this a choice menu (with None/blank as a choice) */
	{ TIM, 'r', EA_PLDT,	"Date+time",		"fe_plan",	},
	{ PLN, 'r', EA_PLLEN,	"Length",		"fe_plan",	},
	{ PLN, 'r', EA_PLEW,	"Early warn",		"fe_plan",	},
	{ PLN, 'r', EA_PLLW,	"Late warn",		"fe_plan",	},
	{ PLN, 'r', EA_PLDAYR,	"Day repeat",		"fe_plan",	},
	{ TIM, 'r', EA_PLEND,	"End date",		"fe_plan",	},
	{ PLN, 'L', EA_NONE,	"",			"fe_plan",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ PLN, 'r', EA_PLCOL,	"Color",		"fe_plan",	},
	{ PLN, 'r', EA_PLNOTE,	"Note",			"fe_plan",	},
	{ PLN, 'r', EA_PLMSG,	"Message",		"fe_plan",	},
	{ PLN, 'r', EA_PLSCR,	"Script",		"fe_plan",	},
	{ PLN, 'r', EA_PLSUSP,	"Suspended",		"fe_plan",	},
	{ PLN, 'r', EA_PLNTM,	"No time",		"fe_plan",	},
	{ PLN, 'r', EA_PLNAL,	"No alarm",		"fe_plan",	},
    /* FIXME:  planquery is form-wide, so it should be in top area */
    /*         I guess it's down here to keep the plan-related stuff together */
	{ PLN, 'L', EA_NONE,	"Shown in calendar if:","fe_plan",	},
	{ PLN, 'T', EA_PLIF,	" ",			"fe_plan",	},
	{ PLN, '-', EA_NONE,	" ",			0,		},

	{ ANY, 'L', EA_NONE,	"Grayed out if:",	"fe_gray",	},
	{ ANY, 'C', EA_GREYIF,	" ",			"fe_gray",	0, offsetof(ITEM, gray_if) },
	{ ANY, 'L', EA_NONE,	"Invisible if:",	"fe_inv",	},
	{ ANY, 'C', EA_HIDEIF,	" ",			"fe_inv",	0, offsetof(ITEM, invisible_if) },
	{ ANY, 'L', EA_NONE,	"Read-only if:",	"fe_ro",	},
	{ ANY, 'C', EA_ROIF,	" ",			"fe_ro",	0, offsetof(ITEM, freeze_if) },
	{ ANY, 'L', EA_NONE,	"Skip if:",		"fe_skip",	},
	{ ANY, 'C', EA_SKIPIF,	" ",			"fe_skip",	0, offsetof(ITEM, skip_if) },
	{ BUT, '-', EA_NONE,	" ",			0,		},

	{ BUT, 'L', EA_NONE,	"Action when pressed:",	"fe_press",	},
	{ BUT, 'T', EA_BUTACT,	" ",			"fe_press",	},
	{ CHA, '-', EA_NONE,	" ",			0,		},

	{ CHA, 'L', EA_NONE,	"Chart X range:",	"fe_chart",	},
	{ CHA, 'd', EA_CHXMIN,	" ",			"fe_chart",	},
	{ CHA, 'l', EA_NONE,	"to",			"fe_chart",	},
	{ CHA, 'd', EA_CHXMAX,	" ",			"fe_chart",	},
	{   0, 'F', EA_NONE, 	" ",			0,		},
	{ CHA, 'f', EA_CHXAUT,	"automatic",		"fe_chart",	},
	{ CHA, 'L', EA_NONE,	"Chart Y range:",	"fe_chart",	},
	{ CHA, 'd', EA_CHYMIN,	" ",			"fe_chart",	},
	{ CHA, 'l', EA_NONE,	"to",			"fe_chart",	},
	{ CHA, 'd', EA_CHYMAX,	" ",			"fe_chart",	},
	{   0, 'F', EA_NONE, 	" ",			0,		},
	{ CHA, 'f', EA_CHYAUT,	"automatic",		"fe_chart",	},
	{ CHA, 'L', EA_NONE,	"Chart XY grid every:",	"fe_chart",	},
	{ CHA, 'd', EA_CHXGRD,	" ",			"fe_chart",	},
	{ CHA, 'd', EA_CHYGRD,	" ",			"fe_chart",	},
	{ CHA, 'L', EA_NONE,	"Chart XY snap every:",	"fe_chart",	},
	{ CHA, 'd', EA_CHXSNP,	" ",			"fe_chart",	},
	{ CHA, 'd', EA_CHYSNP,	" ",			"fe_chart",	},
	{ CHA, 'L', EA_NONE,	"Chart component:",	0,		},
	{ CHA, 'B', EA_CHADD,	"Add",			"fe_chart",	},
	{ CHA, 'B', EA_CHDEL,	"Delete",		"fe_chart",	},
	{ CHA, 'l', EA_CHCUR,	"none",			"fe_chart",	},
	{ CHA, 'B', EA_CHNEXT,	"Next",			"fe_chart",	},
	{ CHA, 'B', EA_CHPREV,	"Previous",		"fe_chart",	},

	{ CHA, '{', EA_NONE,	"comps",		0,		},
	{ CHA, 'L', EA_NONE, 	"Component flags",	"fe_chart",	},
	{   0, 'F', EA_NONE, 	" ",			0,		},
	{ CHA, 'f', EA_CHLINE,	"Line",			"fe_chart",	},
	{ CHA, 'f', EA_CHXFAT,	"X fat",		"fe_chart",	},
	{ CHA, 'f', EA_CHYFAT,	"Y fat",		"fe_chart",	},
	{ CHA, 'L', EA_NONE,	"Exclude if:",		"fe_chart",	},
	{ CHA, 'T', EA_CHNOTIF,	" ",			"fe_chart",	},
	{ CHA, 'L', EA_NONE,	"Color 0..7:",		"fe_chart",	},
	{ CHA, 'T', EA_CHCOL,	" ",			"fe_chart",	},
	{ CHA, 'L', EA_NONE,	"Label:",		"fe_chart",	},
	{ CHA, 'T', EA_CHLAB,	" ",			"fe_chart",	},

	{ CHA, 'L', EA_NONE,	"X position:",		"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHXNXT,	"next free",		"fe_chart",	},
	{ CHA, 'L', EA_NONE,	" ",			"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHXPREV,	"same as previous",	"fe_chart",	},
	{ CHA, 'L', EA_NONE,	" ",			"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHXEXPM,	"expression",		"fe_chart",	},
	{ CHA, 'T', EA_CHXEXPR,	" ",			"fe_chart",	},
	{ CHA, 'L', EA_NONE,	" ",			"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHXDRGM,	"drag field",		"fe_chart",	},
        // FIXME:  this should probably be a combo box with field names
	{ CHA, 'i', EA_CHXDRAG,	" ",			"fe_chart",	0, 999},
	{ CHA, 'l', EA_NONE,	"multiplied by",	"fe_chart",	},
	{ CHA, 'd', EA_CHXDMUL,	" ",			"fe_chart",	},
	{ CHA, 'l', EA_NONE,	"plus",			"fe_chart",	},
	{ CHA, 'd', EA_CHXDPL,	" ",			"fe_chart",	},

	{ CHA, 'L', EA_NONE,	"Y position:",		"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHYNXT,	"next free",		"fe_chart",	},
	{ CHA, 'L', EA_NONE,	" ",			"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHYPREV,	"same as previous",	"fe_chart",	},
	{ CHA, 'L', EA_NONE,	" ",			"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHYEXPM,	"expression",		"fe_chart",	},
	{ CHA, 'T', EA_CHYEXPR,	" ",			"fe_chart",	},
	{ CHA, 'L', EA_NONE,	" ",			"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHYDRGM,	"drag field",		"fe_chart",	},
        // FIXME:  this should probably be a combo box with field names
	{ CHA, 'i', EA_CHYDRAG,	" ",			"fe_chart",	0, 999},
	{ CHA, 'l', EA_NONE,	"multiplied by",	"fe_chart",	},
	{ CHA, 'd', EA_CHYDMUL,	" ",			"fe_chart",	},
	{ CHA, 'l', EA_NONE,	"plus",			"fe_chart",	},
	{ CHA, 'd', EA_CHYDPL,	" ",			"fe_chart",	},

	{ CHA, 'L', EA_NONE,	"X size:",		"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHXSZEX,	"expression",		"fe_chart",	},
	{ CHA, 'T', EA_CHXSZ,	" ",			"fe_chart",	},

	{ CHA, 'L', EA_NONE,	"Y size:",		"fe_chart",	},
	{   0, 'R', EA_NONE,	" ",			0,		},
	{ CHA, 'r', EA_CHYSZEX,	"expression",		"fe_chart",	},
	{ CHA, 'T', EA_CHYSZ,	" ",			"fe_chart",	},

	{   0, '}', EA_NONE,	" ",			0,		},

	{   0, ']', EA_NONE,	" ",			0,		},
	{   0,  0,  EA_NONE,	0,			0		},
};

/*-------------------------------------------------- create window ----------*/
/*
 * destroy form edit shell and remove it from the screen. If the form being
 * edited was created by create_formedit_window(), destroy it; otherwise
 * leave it intact and display it in the main window (using switch_form()).
 */

void destroy_formedit_window(void)
{
	if (have_shell) {
		have_shell = false;
		shell->close();
		delete shell;
	}
}


/*
 * create form edit shell and all the buttons in it, but don't fill in
 * any data yet. If <def> is nonzero, edit the form <def> (the one currently
 * being displayed in the main window); if <def> is zero, start a new form.
 * If <copy> is true, don't use <def> directly, create a copy by changing
 * form name to something that won't overwrite the original.
 */

void create_formedit_window(
	FORM			*def,		/* new form to edit */
	bool			copy)		/* use a copy of <def> */
{
	struct _template	*tp;
	int			n, len, off;	/* width of first column */
	QBoxLayout		*vform, *hform = 0;
	QScrollArea		*scroll=0;
	QWidget			*w=0;
	QVBoxLayout		*scroll_l, *chart_l;
	long			t;

	if (def && have_shell && def == form)		/* same as before */
		return;
	if (!def)
		form = form_create();
	else {
		form = form_clone(def);
		if (copy) {
			zfree(form->name);
			form->name = 0;
		}
	}
	canvas = create_canvas_window(form);		/* canvas window */
	if (have_shell) {
		sensitize_formedit();			/* re-use window */
		return;
	}
	// Non-closable dialog
	// Probably ought to make this cancel, like other dialogs
	// or at least pop up a message, like canvas
	shell = new QDialogNoClose;
	shell->setWindowTitle("Form Editor");
	set_icon(shell, 1);

	// FIXME:  this needs a custom layout, because subwidgets within the
	//         form disrupt the layout process.  Even Xm-grok got it wrong,
	//         by indenting by the frame border width
	// For now, I'm going to just repeat what grok did, as I don't want
	// to waste time figuring out how layouts work at such a deep level.
	vform = new QVBoxLayout(shell);
	add_layout_qss(vform, "editform");
	bind_help(shell, "edit");
	w = shell;
	scroll = new QScrollArea;
        chart = new QFrame;
	chart_l = new QVBoxLayout(chart);
	add_layout_qss(chart_l, "chartform");
	int chart_margin = chart_l->contentsMargins().left();
	chart_margin += chart->contentsMargins().left();

	scroll_w = w = new QWidget;
	scroll_l = new QVBoxLayout(w);
	add_layout_qss(scroll_l, "escrform");
	int scroll_margin = scroll_l->contentsMargins().left();
	scroll_margin += scroll->contentsMargins().left();

	for (off=len=0, tp=tmpl; tp->type; tp++) {
		if (tp->type == 'L') {
			// tjm - FIXME: make sure these are actually styled
			// they are attached to their parent here in the
			// hopes of improved styling support, such as
			// inheriting from their parent
			tp->widget = new QLabel(tp->text, w);
			n = strlen_in_pixels(tp->widget, tp->text) + off;
			if (n > len)
				len = n;
		} else if(tp->type == '[') {
			off += scroll_margin;
			w = scroll;
		} else if(tp->type == '{') {
			off += chart_margin;
			w = chart;
		}
	}
	off=0;
	for (t=0, tp=tmpl; tp->type; tp++, t++) {
		switch(tp->type) {
		    case 'L':
			// FIXME:  Use QStyle::SH_FormLayout* style hints
			// In particular, SH_FormLayoutLabelAlignment
			if(hform)
				hform->addStretch(0);
			vform->addLayout((hform = new QHBoxLayout));
			add_layout_qss(hform, NULL);
			hform->addWidget((w = tp->widget));
			// this should be enough to force alignment
			// if not, there's something wrong with len computation
			w->setMinimumWidth(len-off+4);
			// Then again, I don't want it stretching at all.
			// Hopefully this works as intended.
			w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			break;
		    case 'l':
			hform->addWidget((w = new QLabel(tp->text)));
			break;
		    case 'T':
		    case 't':
			hform->addWidget((w = new QLineEdit()), 1);
			if (tp->type == 't')
				w->setMinimumWidth(100);
			break;
		    case 'M': {  /* editable table of menu entries */
			/* modeled after query window */
			w = new QWidget;
			hform->addWidget(w, 2);
			QVBoxLayout *v = new QVBoxLayout(w);
			v->setContentsMargins(0,0,0,0);
			menu_w = new QTableWidget;
			// copied from querywin.c
			menu_w->setShowGrid(false);
			menu_w->setWordWrap(false);
			menu_w->setEditTriggers(QAbstractItemView::AllEditTriggers);
			menu_w->setSelectionBehavior(QAbstractItemView::SelectRows);
			menu_w->setColumnCount(ALEN(menu_cols));
			for(int i = 0; i < ALEN(menu_cols); i++)
				    menu_w->setColumnWidth(i, menu_cols[i].isint ? 40 : 100);
			menu_w->setHorizontalHeaderLabels(menu_col_labels);
			menu_w->horizontalHeader()->setStretchLastSection(true);
			// menu_w->setDragDropMode(QAbstractItemView::InternalMove);
			// menu_w->setDropIndicatorShown(true);
			menu_w->verticalHeader()->hide();
			menu_w->setMinimumWidth(150);
			v->addWidget(menu_w);
			// This needs a more specific callback
			set_qt_cb(QTableWidget, cellChanged, menu_w,
				  menu_callback(menu_w, c, r), int r, int c);
			set_qt_cb(QTableWidget, currentCellChanged, menu_w,
				  menu_callback(menu_w, c, r), int r, int c);
			QDialogButtonBox *bb = new QDialogButtonBox;
			v->addWidget(bb);
			QPushButton *b;
			b = new QPushButton(QIcon::fromTheme("go-up"), "");
			bb->addButton(b, dbbr(Action));
			b->setObjectName("up");
			b->setEnabled(false);
			set_button_cb(b, menu_callback(menu_w, -1, -1));
			bind_help(b, tp->help);
			b = new QPushButton(QIcon::fromTheme("go-down"), "");
			bb->addButton(b, dbbr(Action));
			b->setObjectName("down");
			b->setEnabled(false);
			set_button_cb(b, menu_callback(menu_w, 1, -1));
			bind_help(b, tp->help);
			b = mk_button(bb, "Delete", dbbr(Action));
			b->setObjectName("del");
			b->setEnabled(false);
			set_button_cb(b, menu_callback(menu_w, -1, -2));
			bind_help(b, tp->help);
			b = mk_button(bb, "Duplicate", dbbr(Action));
			b->setObjectName("dup");
			b->setEnabled(false);
			set_button_cb(b, menu_callback(menu_w, 1, -2));
			bind_help(b, tp->help);
			break;
		    }
		    case 'K': { /* Editable key field table; like 'M' */
			w = new QWidget;
			hform->addWidget(w, 2);
			QVBoxLayout *v = new QVBoxLayout(w);
			v->setContentsMargins(0,0,0,0);
			key_w = new QTableWidget;
			// copied from querywin.c
			key_w->setShowGrid(false);
			key_w->setWordWrap(false);
			key_w->setEditTriggers(QAbstractItemView::NoEditTriggers);
			key_w->setSelectionBehavior(QAbstractItemView::SelectRows);
			key_w->setColumnCount(key_col_labels.length());
			for(int i = 0; i < key_col_labels.length(); i++)
				    key_w->setColumnWidth(i, i > 0 ? 40 : 100);
			key_w->setHorizontalHeaderLabels(key_col_labels);
			key_w->horizontalHeader()->setStretchLastSection(true);
			// key_w->setDragDropMode(QAbstractItemView::InternalMove);
			// key_w->setDropIndicatorShown(true);
			key_w->verticalHeader()->hide();
			key_w->setMinimumWidth(150);
			v->addWidget(key_w);
			// This needs a more specific callback
			//set_qt_cb(QTableWidget, cellChanged, key_w,
			//	  key_callback(c, r), int r, int c);
			//set_qt_cb(QTableWidget, currentCellChanged, key_w,
			//	  key_callback(c, r), int r, int c);
			QDialogButtonBox *bb = new QDialogButtonBox;
			v->addWidget(bb);
			QPushButton *b;
			b = new QPushButton(QIcon::fromTheme("go-up"), "");
			bb->addButton(b, dbbr(Action));
			b->setObjectName("up");
			b->setEnabled(false);
			set_button_cb(b, key_callback(-1, -1));
			bind_help(b, tp->help);
			b = new QPushButton(QIcon::fromTheme("go-down"), "");
			bb->addButton(b, dbbr(Action));
			b->setObjectName("down");
			b->setEnabled(false);
			set_button_cb(b, key_callback(1, -1));
			bind_help(b, tp->help);
			b = mk_button(bb, "Delete", dbbr(Action));
			b->setObjectName("del");
			b->setEnabled(false);
			set_button_cb(b, key_callback(-1, -2));
			bind_help(b, tp->help);
			b = mk_button(bb, "Duplicate", dbbr(Action));
			b->setObjectName("dup");
			b->setEnabled(false);
			set_button_cb(b, key_callback(1, -2));
			bind_help(b, tp->help);
			break;
		    }
		    case 'D':
		    case 'C': {
			QComboBox *cb = new QComboBox;
			cb->setEditable(true);
			hform->addWidget((w = cb), 1);
			break;
		    }
		    case 'i': {
			QSpinBox *sb = new QSpinBox;
			hform->addWidget((w = sb));
			sb->setMinimumWidth(100);
			// FIXME:  What should the range be?
			sb->setRange(0, tp->role);
			break;
		    }
		    case 'd': {
			QDoubleSpinBox *sb = new QDoubleSpinBox;
			hform->addWidget((w = sb));
			sb->setMinimumWidth(100);
			// FIXME:  What should the range be?
			sb->setRange(-DBL_MAX, DBL_MAX);
			// FIXME:  Do I need to set decimals?  What's the default?
			// sb->setDecimal(10);
			break;
		    }
		    case 'I': {
			    QComboBox *cb = new QComboBox;
			    for(n = 0; n < ALEN(item_types); n++)
				    cb->addItem(item_types[n].name);
			    hform->addWidget((w = cb));
			    break;
		    }
		    case 'F':
		    case 'R':
			{
				QHBoxLayout *l = new QHBoxLayout;
				hform->addLayout(l);
				add_layout_qss(l, NULL);
				l->setSpacing(4);
				hform = l;
				w = 0;
			}
			break;
		    case 'f':
			hform->addWidget((w = new QCheckBox(tp->text)));
			break;
		    case 'r':
			{
				QRadioButton *r = new QRadioButton(tp->text);
				hform->addWidget((w = r));
				r->setAutoExclusive(false);
			}
			break;
		    case 'B':
			// Xm-grok used 90 here, but I'll use 80 everywhere
			hform->addWidget((w = mk_button(NULL, tp->text,
							tp->role)));
			break;
		    case '-':
			vform->addWidget((w = mk_separator()));
			break;
		    case '[':
			off += scroll_margin;
			vform->addWidget(scroll);
			vform = scroll_l;
			// In Xmgrok, the viewport itself has a sunken border
			// FIXME:  This is a guess at doing it in Qt.
			// If that doesn't work, try area->viewport()->...
			// If that doesn't work, try area->setViewport(QFrame)
#if 0
			{
				QFrame *f = new QFrame;
				scroll->setViewport(f);
				f->setLineWidth(4);
				f->setFrameStyle(QFrame::Panel | QFrame::Sunken);
				// Can't do the following; protected
				scroll->setViewportMargins(QMargins(4, 4, 4, 4));
			}
#endif
			scroll->setMinimumHeight(480);
			scroll->setObjectName(tp->text);
			break;
		    case ']':
			scroll->setWidget(scroll_w);
			scroll->setWidgetResizable(true);
			scroll_w->show();
			break;
		    case '{':
			off += chart_margin;
			w = chart;
			vform->addWidget(chart);
			vform->addStretch(0); // don't spread form out
			vform = chart_l;
			chart->setLineWidth(4);
			chart->setFrameStyle(QFrame::Panel | QFrame::Sunken);
			chart->setObjectName(tp->text);
			break;
		    case '}':
			vform->addStretch(0); // don't spread form out
			break;
		}

		if(w && tp->type != '-')
			w->setProperty("helvSmallFont", true);

		tp->widget = w;

		if(tp->type == 't' || tp->type == 'T')
			set_text_cb(w, formedit_callback(t));
		else if(tp->type == 'i' || tp->type == 'd')
			set_spin_cb(w, formedit_callback(t));
		else if(tp->type == 'r' || tp->type == 'f' || tp->type == 'B')
			set_button_cb(w, formedit_callback(t));
		else if(tp->type == 'I')
			set_popup_cb(w, formedit_callback(t), int, i);
		else if(tp->type == 'C' || tp->type == 'D')
			set_combo_cb(w, formedit_callback(t));

		if(w && tp->help)
			bind_help(w, tp->help);
	}
	sensitize_formedit();
	fillout_formedit();
	popup_nonmodal(shell);
	set_dialog_cancel_cb(shell, formedit_callback(0));
	have_shell = true;
}


/*-------------------------------------------------- menu printing ----------*/
/*
 * sensitize only those items in the form window that are needed by the
 * current type. The procedural edit button is an exception, its
 * sensitivity is determined as a special case because it doesn't
 * depend on the item type.
 *
 * tjm - now I just hide stuff instead of desensitizing
 *       also, refill combo boxes
 */

void sensitize_formedit(void)
{
	struct _template	*tp;
	int			mask;
	ITEM			*item;

	item = canvas->curr_item >= form->nitems ? 0 : form->items[canvas->curr_item];
	mask = 1 << (item ? item->type : IT_NULL);
	for (tp=tmpl; tp->type; tp++) {
		if (tp->sensitive) {
			tp->widget->setEnabled(
				tp->code == EA_EDPROC ? form->proc :
				tp->code == EA_DELWID ? item != 0 : true);
			tp->widget->setVisible(tp->sensitive & mask);
		}
		if (tp->type == 'M' && (tp->sensitive & mask))
			// Qt sometimes narrows the menu table when
			// visibility is toggled.  I don't know how
			// to fix it.
			// Calling resize_table_widget() makes it
			// worse.
			// This seems to help, though.  On the other
			// hand, setting it for all newly visible widgets
			// does nothing but revert this fix.
			menu_w->updateGeometry();
		if (tp->type == 'K')
			key_w->updateGeometry();
		if ((mask & (MUL)) &&
		    (tp->code >= EA_FIRST_MULTI_OVR && tp->code <= EA_LAST_MULTI_OVR)) {
			tp->widget->setEnabled(!IFL(item->,MULTICOL));
			tp[-1].widget->setEnabled(!IFL(item->,MULTICOL));
		}
		if (tp->type == 'C') {
			QStringList sl;
			int n;
			QComboBox *cb = reinterpret_cast<QComboBox *>(tp->widget);
			for(n = 0; n < form->nitems; n++) {
				char *s = *(char **)((char *)form->items[n] + tp->role);
				if(!BLANK(s) && !sl.contains(s))
					sl.append(s);
			}
			QString s = cb->currentText();
			cb->clear();
			cb->addItems(sl);
			cb->setEditText(s);
		}
		if (tp->type == 'D') {
			QStringList sl;
			QComboBox *cb = reinterpret_cast<QComboBox *>(tp->widget);
			add_dbase_list(sl);
			QString s = cb->currentText();
			cb->clear();
			cb->addItem("");
			cb->addItems(sl);
			cb->setEditText(s);
		}
	}
	// Without explicit adjustSize(), there may be huge gaps in form
	if(chart->isVisible())
		chart->adjustSize();
	scroll_w->adjustSize();
}


/*
 * fillout_formedit draws all the current item's values into the form
 * definition window.
 * fillout_formedit_widget_by_code draws a single value into the form.
 * The code is searched for in the field list; see struct _template.
 * This is somewhat inefficient, an index into tmpl[] would be
 * faster but would be guaranteed to get out of sync if tmpl[] is
 * changed.
 * fillout_formedit_widget also draws a single field, but gets a pointer
 * to the right tmpl[] line. The previous two routines use this.
 */

static void fillout_formedit_widget(struct _template *);

void fillout_formedit(void)
{
	struct _template	*tp;

	for (tp=tmpl; tp->type; tp++)
		fillout_formedit_widget(tp);
	fill_menu_table(menu_w);
	fill_key_table(key_w);
}


static void fillout_formedit_widget_by_code(
	enum edact		code)
{
	struct _template	*tp;

	for (tp=tmpl; tp->type; tp++)
		if (tp->code == code) {
			fillout_formedit_widget(tp);
			break;
		}
}

void fillout_formedit_widget_proc()
{
	fillout_formedit_widget_by_code(EA_ISPROC);
}

#define set_sb_value(w, v) reinterpret_cast<QSpinBox *>(w)->setValue(v)
#define set_dsb_value(w, v) reinterpret_cast<QDoubleSpinBox *>(w)->setValue(v)

// Prevent events triggered by filling out to cause a readback
// FIXME: I should do this a better way.
static ITEM *filling_item = 0;

static void set_digits(int dig)
{
	struct _template	*tp;

	for (tp=tmpl; tp->type; tp++)
		if (tp->code == EA_INMIN || tp->code == EA_INMAX)
			reinterpret_cast<QDoubleSpinBox *>(tp->widget)->setDecimals(dig);
}

static void fillout_formedit_widget(
	struct _template	*tp)
{
	ITEM			*item = NULL;
	CHART			*chart = NULL;
	CHART			nullchart;
	QWidget			*w = tp->widget;

	if (!tp->code || tp->code > EA_LAST_FWIDE) {
		if (!form->items || canvas->curr_item >= form->nitems ||
		    !(tp->sensitive & (1 << form->items[canvas->curr_item]->type))) {
			if (tp->type == 'T' || tp->type == 't' ||
			    tp->type == 'i' || tp->type == 'd' ||
			    tp->type == 'C' || tp->type == 'D')
				print_text_button_s(w, "");
			return;
		}
		item  = form->items[canvas->curr_item];
		chart = &item->ch_comp[item->ch_curr];
		if (!chart)
			memset((chart = &nullchart), 0, sizeof(CHART));
	}
	filling_item = item;
	switch(tp->code) {
	  case EA_FORMNM: print_text_button_s(w, form->name);		break;
	  case EA_DBNAME: print_text_button_s(w, form->dbname);		break;
	  case EA_FCMT: print_text_button_s(w, form->comment);		break;
	  case EA_DBDELIM: print_text_button_s(w, to_octal(form->cdelim));	break;
	  case EA_FRO: set_toggle(w, form->rdonly);			break;
	  case EA_HASTS: set_toggle(w, form->syncable);			break;
	  case EA_ISPROC: set_toggle(w, form->proc);
		      fillout_formedit_widget_by_code(EA_EDPROC);	break;
	  case EA_EDPROC: w->setEnabled(form->proc);			break;
	  case EA_ARDELIM: print_text_button_s(w, to_octal(form->asep ? form->asep : '|'));	break;
	  case EA_ARESC: print_text_button_s(w, to_octal(form->aesc ? form->aesc : '\\'));	break;
	  case EA_SUMHT: set_dsb_value(w, form->sumheight + 1);	break;

	  case EA_TYPE:
		  for(int n = 0; n < ALEN(item_types); n++)
			  if(item_types[n].type == item->type) {
				  reinterpret_cast<QComboBox *>(w)->setCurrentIndex(n);
				  break;
			  }
		  break;

	  case EA_LABFH:
	  case EA_LABFO:
	  case EA_LABFN:
	  case EA_LABFB:
	  case EA_LABFC: set_toggle(w, item->labelfont == tp->code-EA_LABFH);	break;

	  case EA_INFH:
	  case EA_INFO:
	  case EA_INFN:
	  case EA_INFB:
	  case EA_INFC: set_toggle(w, item->inputfont == tp->code-EA_INFH);	break;

	  case EA_LABJL: set_toggle(w, item->labeljust == J_LEFT);	break;
	  case EA_LABJC: set_toggle(w, item->labeljust == J_CENTER);	break;
	  case EA_LABJR: set_toggle(w, item->labeljust == J_RIGHT);	break;

	  case EA_INJL: set_toggle(w, item->inputjust == J_LEFT);	break;
	  case EA_INJC: set_toggle(w, item->inputjust == J_CENTER);	break;
	  case EA_INJR: set_toggle(w, item->inputjust == J_RIGHT);	break;

	  case EA_FLSRCH: set_toggle(w, IFL(item->,SEARCH));		break;
	  case EA_FLRO: set_toggle(w, IFL(item->,RDONLY));		break;
	  case EA_FLNSRT: set_toggle(w, IFL(item->,NOSORT));		break;
	  case EA_DEFSRT: set_toggle(w, IFL(item->,DEFSORT));		break;
	  case EA_ISDATE: set_toggle(w, item->timefmt == T_DATE);		break;
	  case EA_ISTIME: set_toggle(w, item->timefmt == T_TIME);		break;
	  case EA_ISDTTM: set_toggle(w, item->timefmt == T_DATETIME);	break;
	  case EA_ISDUR: set_toggle(w, item->timefmt == T_DURATION);	break;
	  case EA_DTWID: set_toggle(w, item->timewidget & 1);		break;
	  case EA_DTCAL: set_toggle(w, item->timewidget & 2);			break;

	  case EA_PLDT:
	  case EA_PLLEN:
	  case EA_PLEW:
	  case EA_PLLW:
	  case EA_PLDAYR:
	  case EA_PLEND:
	  case EA_PLCOL:
	  case EA_PLNOTE:
	  case EA_PLMSG:
	  case EA_PLSCR:
	  case EA_PLSUSP:
	  case EA_PLNTM:
	  case EA_PLNAL: set_toggle(w, item->plan_if == plan_code[tp->code - EA_PLDT]);
		      sensitize_formedit();
		      break;

	  case EA_INMIN: set_dsb_value(w, item->min);			break;
	  case EA_INMAX: set_dsb_value(w, item->max);			break;
	  case EA_INDIG: set_sb_value(w, item->digits); set_digits(item->digits);			break;
	  case EA_MENUST:
	  case EA_MENUDY:
	  case EA_MENUSD: set_toggle(w, item->dcombo == tp->code - EA_MENUST);	break;
	  case EA_FKDB:
		  if (BLANK(item->fkey_form_name))
			  print_text_button_s(w, "");
		  else {
			  /* FIXME: this allows fkey_db_name to be path */
			  /* but nowhere else does */
			  const char *p = canonicalize(item->fkey_form_name, false);
			  if (!strcmp(p, canonicalize(resolve_tilde(GROKDIR, 0), 0)) ||
			      !strcmp(p, canonicalize(resolve_tilde(LIB "/grokdir", 0), 0)))
				  print_text_button_s(w, item->fkey_form_name);
			  else {
				  char *lab = qstrdup(QString(p) + '/' + item->fkey_form_name);
				  print_text_button_s(w, lab);
				  free(lab);
			  }
		  }
		  fill_key_table(key_w);
		  break;
	  case EA_FLMUL: set_toggle(w, IFL(item->,MULTICOL));		break;
	  case EA_FKHDR: set_toggle(w, IFL(item->,FKEY_HEADER));		break;
#if 0
	  case EA_FKSRCH: set_toggle(w, IFL(item->,FKEY_SEARCH));		break;
#endif
	  case EA_FKMUL: set_toggle(w, IFL(item->,FKEY_MULTI));		break;
	  case EA_INLEN: set_sb_value(w, item->maxlen);			break;
	  case EA_SUMCOL: set_sb_value(w, item->sumcol);			break;
	  case EA_SUMWID: set_sb_value(w, item->sumwidth);			break;
	  case EA_COL: set_sb_value(w, item->column);			break;
	  case EA_FLDNM: print_text_button_s(w, item->name);		break;
	  case EA_FLCODE: print_text_button_s(w, item->flagcode);		break;
	  case EA_FLSUM: print_text_button_s(w, item->flagtext);		break;
	  case EA_LABEL: print_text_button_s(w, item->label);		break;
	  case EA_INDEF: print_text_button_s(w, item->idefault);		break;

	  case EA_GREYIF: print_text_button_s(w, item->gray_if);		break;
	  case EA_HIDEIF: print_text_button_s(w, item->invisible_if);	break;
	  case EA_ROIF: print_text_button_s(w, item->freeze_if);		break;
	  case EA_SKIPIF: print_text_button_s(w, item->skip_if);		break;

	  case EA_BUTACT: print_text_button_s(w, item->pressed);		break;
	  case EA_PLIF: print_text_button_s(w, form->planquery);		break;
	  case EA_SUMPR: print_text_button_s(w, item->sumprint);		break;

	  case EA_CHXMIN: set_dsb_value(w, item->ch_xmin);			break;
	  case EA_CHXMAX: set_dsb_value(w, item->ch_xmax);			break;
	  case EA_CHYMIN: set_dsb_value(w, item->ch_ymin);			break;
	  case EA_CHYMAX: set_dsb_value(w, item->ch_ymax);			break;
	  case EA_CHXGRD: set_dsb_value(w, item->ch_xgrid);			break;
	  case EA_CHYGRD: set_dsb_value(w, item->ch_ygrid);			break;
	  case EA_CHXSNP: set_dsb_value(w, item->ch_xsnap);			break;
	  case EA_CHYSNP: set_dsb_value(w, item->ch_ysnap);			break;
	  case EA_CHXAUT: set_toggle(w, IFL(item->,CH_XAUTO));		break;
	  case EA_CHYAUT: set_toggle(w, IFL(item->,CH_YAUTO));		break;
	  case EA_CHCUR: print_button(w, item->ch_ncomp ? "%d of %d" : "none",
				    item->ch_curr+1, item->ch_ncomp);	break;

	  case EA_CHLINE: set_toggle(w, chart->line);			break;
	  case EA_CHXFAT: set_toggle(w, chart->xfat);			break;
	  case EA_CHYFAT: set_toggle(w, chart->yfat);			break;
	  case EA_CHNOTIF: print_text_button_s(w, chart->excl_if);		break;
	  case EA_CHCOL: print_text_button_s(w, chart->color);		break;
	  case EA_CHLAB: print_text_button_s(w, chart->label);		break;

	  case EA_CHXNXT: set_toggle(w, chart->value[0].mode == CC_NEXT);	break;
	  case EA_CHXPREV: set_toggle(w, chart->value[0].mode == CC_SAME);	break;
	  case EA_CHXEXPM: set_toggle(w, chart->value[0].mode == CC_EXPR);	break;
	  case EA_CHXDRGM: set_toggle(w, chart->value[0].mode == CC_DRAG);	break;
	  case EA_CHXEXPR: print_text_button_s(w, chart->value[0].expr);	break;
	  case EA_CHXDRAG: set_sb_value(w, chart->value[0].field);		break;
	  case EA_CHXDMUL: set_dsb_value(w, chart->value[0].mul);		break;
	  case EA_CHXDPL: set_dsb_value(w, chart->value[0].add);		break;

	  case EA_CHYNXT: set_toggle(w, chart->value[1].mode == CC_NEXT);	break;
	  case EA_CHYPREV: set_toggle(w, chart->value[1].mode == CC_SAME);	break;
	  case EA_CHYEXPM: set_toggle(w, chart->value[1].mode == CC_EXPR);	break;
	  case EA_CHYDRGM: set_toggle(w, chart->value[1].mode == CC_DRAG);	break;
	  case EA_CHYEXPR: print_text_button_s(w, chart->value[1].expr);	break;
	  case EA_CHYDRAG: set_sb_value(w, chart->value[1].field);		break;
	  case EA_CHYDMUL: set_dsb_value(w, chart->value[1].mul);		break;
	  case EA_CHYDPL: set_dsb_value(w, chart->value[1].add);		break;

	  case EA_CHXSZEX: set_toggle(w, chart->value[2].mode == CC_EXPR);	break;
	  case EA_CHXSZ: print_text_button_s(w, chart->value[2].expr);	break;

	  case EA_CHYSZEX: set_toggle(w, chart->value[3].mode == CC_EXPR);	break;
	  case EA_CHYSZ: print_text_button_s(w, chart->value[3].expr);	break;

	  case EA_QUERY:
	  case EA_REFBY:
	  case EA_FHELP:
	  case EA_CHECK:
	  case EA_PREVW:
	  case EA_HELP:
	  case EA_CANCEL:
	  case EA_DONE:
	  case EA_ADDWID:
	  case EA_DELWID:
	  case EA_CHADD:
	  case EA_CHDEL:
	  case EA_CHNEXT:
	  case EA_CHPREV:
		break;
	}
	filling_item = NULL;
}


/*-------------------------------------------------- button callbacks -------*/
static void menu_callback(QTableWidget *w, int x, int y)
{
	int row = y < 0 ? w->currentRow() : y;
	bool do_resize = false;

	if (canvas->curr_item >= form->nitems)
		return;
	QSignalBlocker sb(w);
	ITEM *item = form->items[canvas->curr_item];
	if(y < 0 && row == item->nmenu)
		return;
	if(y == -2) {
		if(x < 0) { // del
			menu_delete(&item->menu[row]);
			if(!--item->nmenu) {
				free(item->menu);
				item->menu = 0;
			} else
				memmove(item->menu + row, item->menu + row + 1, (item->nmenu - row) * sizeof(MENU));
			w->removeRow(row);
			do_resize = true;
		} else { // dup
			grow(0, "new menu item", MENU, item->menu, ++item->nmenu, NULL);
			memmove(item->menu + row + 1, item->menu + row, (item->nmenu - row - 1) * sizeof(MENU));
			if (IFL(item->,MULTICOL))
				item->menu[row + 1].column = avail_column(form, NULL);
			menu_clone(&item->menu[row]);
			w->insertRow(row + 1);
			w->setCurrentCell(row + 1, w->currentColumn());
			fill_menu_row(w, item, row + 1);
			do_resize = true;
		}
	} else if(y == -1) { // up/down
		if(x + row < 0 || x + row >= item->nmenu)
			return;
		MENU t = item->menu[row];
		item->menu[row] = item->menu[row + x];
		item->menu[row + x] = t;
		fill_menu_row(w, item, row);
		fill_menu_row(w, item, row + x);
		w->setCurrentCell(row + x, w->currentColumn());
		row += x;
	} else { // cell widget
		QTableWidgetItem *twi = w->item(row, x);
		const QString &qs = twi->text();
		char *string = NULL;
		if(qs.size())
			string = qstrdup(qs);
		if(string && row == item->nmenu) { // unblank a blank
			// yeah, maybe one day I'll alloc in chunks
			zgrow(0, "new menu item", MENU, item->menu, item->nmenu,
			      item->nmenu + 1, NULL);
			if(IFL(item->,MULTICOL) && x != MENU_FIELD_COLUMN) {
				item->menu[item->nmenu].column = avail_column(form, NULL);
				w->item(row, MENU_FIELD_COLUMN)->setText(qsprintf("%d", item->menu[item->nmenu].column));
			}
			item->nmenu++;
			w->insertRow(item->nmenu);
			fill_menu_row(w, item, item->nmenu);
			do_resize = true;
		}
		if(row < item->nmenu) {
			void *p = (char *)&item->menu[row] + menu_cols[x].fieldoff;
			if(menu_cols[x].isint)
				*(int *)p = atoi(STR(string));
			else {
				zfree(*(char **)p);
				*(char **)p = string;
			}
			// delete if newly blank
			// 0s are considered blanks
			int mask = menu_col_mask(item);
			for(x = 0; x < ALEN(menu_cols); x++) {
				if(!(mask & (1 << x)))
					continue;
				void *p = (char *)&item->menu[row] + menu_cols[x].fieldoff;
				if(menu_cols[x].isint) {
					if(*(int *)p)
						break;
				} else
					if(*(char **)p)
						break;
			}
			if(x == ALEN(menu_cols)) { // newly blank; auto-del
				menu_callback(w, -1, -2);
				return;
			}
		}
	}
	if(do_resize)
		resize_menu_table(w);
	QPushButton *b;
	b = w->parent()->findChild<QPushButton *>("up");
	b->setEnabled(row);
	b = w->parent()->findChild<QPushButton *>("down");
	b->setEnabled(row < item->nmenu - 1);
	b = w->parent()->findChild<QPushButton *>("dup");
	b->setEnabled(row < item->nmenu);
	b = w->parent()->findChild<QPushButton *>("del");
	b->setEnabled(row < item->nmenu);
}


static void key_callback(int x, int y, int c)
{
	int row = y < 0 ? key_w->currentRow() : y;
	bool do_resize = false;

	if (canvas->curr_item >= form->nitems)
		return;
	QSignalBlocker sb(key_w);
	ITEM *item = form->items[canvas->curr_item];
	if(y < 0 && row == item->nfkey)
		return;
	if(y == -2) {
		if(x < 0) { // del
			if(!--item->nfkey) {
				free(item->fkey);
				item->fkey = 0;
			} else
				memmove(item->fkey + row, item->fkey + row + 1, (item->nfkey - row) * sizeof(FKEY));
			key_w->removeRow(row);
			do_resize = true;
		} else { // dup
			grow(0, "new fkey item", FKEY, item->fkey, ++item->nfkey, NULL);
			memmove(item->fkey + row + 1, item->fkey + row, (item->nfkey - row - 1) * sizeof(FKEY));
			key_w->insertRow(row + 1);
			key_w->setCurrentCell(row + 1, key_w->currentColumn());
			fill_key_row(key_w, item, row + 1);
			do_resize = true;
		}
	} else if(y == -1) { // up/down
		if(x + row < 0 || x + row >= item->nfkey)
			return;
		FKEY t = item->fkey[row];
		item->fkey[row] = item->fkey[row + x];		item->fkey[row + x] = t;
		fill_key_row(key_w, item, row);
		fill_key_row(key_w, item, row + x);
		key_w->setCurrentCell(row + x, key_w->currentColumn());
		row += x;
	} else { // cell widget
		if(c > 0 && x == KEY_FIELD_FIELD && row == item->nfkey) { // unblank a blank
			// yeah, maybe one day I'll alloc in chunks
			zgrow(0, "new key item", FKEY, item->fkey, item->nfkey,
			      item->nfkey + 1, NULL);
			item->nfkey++;
			key_w->insertRow(item->nfkey);
			key_w->cellWidget(y, KEY_FIELD_KEY)->setEnabled(true);
			key_w->cellWidget(y, KEY_FIELD_DISPLAY)->setEnabled(true);
			fill_key_row(key_w, item, item->nfkey);
			do_resize = true;
		}
		if(row < item->nfkey) {
			switch (x) {
			    case KEY_FIELD_FIELD:
				resolve_fkey_fieldsel(item->fkey_form, c - 1, item->fkey[y]);
				break;
			    case KEY_FIELD_KEY:
				item->fkey[y].key = c;
				if (c && item->type == IT_INV_FKEY)
					for (int n=0; n < item->nfkey; n++)
						if (n != y && item->fkey[n].key) {
							item->fkey[n].key = 0;
							fill_key_row(key_w, item, n);
							break;
						}
				break;
			    case KEY_FIELD_DISPLAY:
				item->fkey[y].display = c;
				break;
			}
			key_w->setCurrentCell(y, x);
			if (x == KEY_FIELD_FIELD && !c) { // newly blank; auto-del
				key_callback(-1, -2);
				return;
			}
		}
	}
	if(do_resize)
		resize_key_table(key_w);
	QPushButton *b;
	b = key_w->parent()->findChild<QPushButton *>("up");
	b->setEnabled(row);
	b = key_w->parent()->findChild<QPushButton *>("down");
	b->setEnabled(row < item->nfkey - 1);
	b = key_w->parent()->findChild<QPushButton *>("dup");
	b->setEnabled(row < item->nfkey);
	b = key_w->parent()->findChild<QPushButton *>("del");
	b->setEnabled(row < item->nfkey);
}

/*
 * some item in one of the menu bar pulldowns was pressed. All of these
 * routines are direct X callbacks.
 */

static void formedit_callback(
	int				indx)
{
	switch(readback_item(indx)) {
	  case 1:
		fillout_formedit_widget(&tmpl[indx]);
		if (form->items && canvas->curr_item < form->nitems)
			redraw_canvas_item(form->items[canvas->curr_item]);
		break;
	  case 2:
		sensitize_formedit();
		fillout_formedit();
		redraw_canvas();
	}
}


/*
 * read back all text widgets. This must be done before a new item is
 * selected or added, because we'll never see new text in text buttons
 * unless the user explicitly pressed Return on them.
 */

void readback_formedit(void)
{
	struct _template	*tp;
	int			t;

	if (canvas->curr_item < form->nitems)
		for (t=0, tp=tmpl; tp->type; tp++, t++)
			if (tp->type == 'T' || tp->type == 't' ||
			    tp->type == 'i' || tp->type == 'd' ||
			    tp->type == 'C' || tp->type == 'D')
				(void)readback_item(t);
}


/*
 * do the operation requested by the widget: execute a function, or read
 * back a value into the form or current item. Return 0 if no canvas redraw
 * is required, 1 if the current item on the canvas must be redrawn, and 2
 * if the entire canvas must be redrawn.
 */

#define get_sb_value(w) reinterpret_cast<QSpinBox *>(w)->value()
#define get_dsb_value(w) reinterpret_cast<QDoubleSpinBox *>(w)->value()

static int readback_item(
	int			indx)
{
	struct _template	*tp = &tmpl[indx];
	ITEM			*item = 0, *ip;
	CHART			*chart = 0;
	QWidget			*w = tp->widget;
	int			code, i;
	bool			all = false; /* redraw oll or one? */

	if (canvas->curr_item < form->nitems) {
		item  = form->items[canvas->curr_item];
		chart = &item->ch_comp[item->ch_curr];
	}
	if (tp->code > EA_LAST_FWIDE && !item)
		return(0);
	if (!chart && tp->code >= EA_FIRST_CHART && tp->code <= EA_LAST_CHART)
		return(0);
	if (item && item == filling_item)
		return(0);

	switch(tp->code) {
	  case EA_FORMNM: (void)read_text_button_noblanks(w, &form->name);
		      if (form->name && !form->dbname) {
				form->dbname = mystrdup(form->name);
				fillout_formedit_widget_by_code(EA_DBNAME);
		      }
		      break;

	  case EA_DBNAME: (void)read_text_button_noblanks(w, &form->dbname);	break;
	  case EA_FCMT: (void)read_text_button(w, &form->comment);	break;
	  case EA_DBDELIM: form->cdelim=to_ascii(read_text_button(w,0),':');	break;
	  case EA_FRO: form->rdonly     ^= true;				break;
	  case EA_HASTS: form->syncable   ^= true;				break;
	  case EA_ISPROC: form->proc       ^= true; sensitize_formedit();	break;
	  case EA_EDPROC: form_edit_script(form, w, form->dbname);		break;

	  case EA_ARDELIM: form->asep=to_ascii(read_text_button(w,0),'|');	break;
	  case EA_ARESC: form->aesc=to_ascii(read_text_button(w,0),'\\');	break;
	  case EA_SUMHT: form->sumheight = get_dsb_value(w) - 1;	break;
	  case EA_ADDWID: readback_formedit();
		      item_deselect(form);
		      (void)item_create(form, canvas->curr_item);
		      IFS(form->items[canvas->curr_item]->,SELECTED);
		      item = 0; // skip item adjustment below
		      all = true;
		      break;

	  case EA_DELWID: item_deselect(form);
		      item_delete(form, canvas->curr_item);
		      if (canvas->curr_item >= form->nitems) {
				if (canvas->curr_item)
				    canvas->curr_item--;
				else
				    item_deselect(form);
		      }
		      item = 0; // skip item adjustment below
	 	      all = true;
		      break;

	  case EA_FHELP: create_edit_popup("Card Help Editor",
					 &form->help, false, "fe_help");
		      break;

	  case EA_QUERY: create_query_window(form);
		      break;

	  case EA_REFBY: create_refby_window(form);
		      break;

	  case EA_CHECK: if (!verify_form(form, &i, shell) && i < form->nitems) {
				item_deselect(form);
				canvas->curr_item = i;
				IFS(form->items[i]->,SELECTED);
				redraw_canvas_item(form->items[i]);
				fillout_formedit();
				sensitize_formedit();
		      } else {
				fillout_formedit();
				sensitize_formedit();
		      }
		      break;

	  case EA_PREVW: create_card_menu(form, 0, 0, false);
		      break;

	  case EA_HELP: help_callback(shell, "edit");
	 	      return(0);

	  case EA_DONE: readback_formedit();
		      if (!verify_form(form, &i, shell)) {
				if (i < form->nitems) {
					item_deselect(form);
					canvas->curr_item = i;
					IFS(form->items[i]->,SELECTED);
					redraw_canvas_item(form->items[i]);
					fillout_formedit();
					sensitize_formedit();
				} else {
					item_deselect(form);
					fillout_formedit();
				}
				return(0);
		      }
		      {
			      QString msg;
			      if(check_loaded_forms(msg, form)) {
				      const char *formpath = resolve_tilde(form->path, "gf");
				      for(FORM *f = form_list; f; f = f->next)
					      if(f->dbase && !strcmp(f->path, formpath)) {
						      if(f->dbase->modified &&
							 !create_save_popup(mainwindow, f,
									    "Form configuration changes require reloading database %s\n"
									    "Discard changes and reload?",
									    formpath))
							      return(0);
						      if(mainwindow->card &&
							 mainwindow->card->form->dbase == f->dbase)
							      switch_form(mainwindow->card, 0);
						      dbase_free(f->dbase);
						      break;
					      }
			      }
		      }
	  	      destroy_formedit_window();
		      destroy_canvas_window();
		      destroy_query_window();
		      form_sort(form);
		      if (!write_form(form))
				return(0);
		      /* now force a reload of all loaded forms with same path */
		      /* "delete" so it's only retained if referenced */
		      form_delete(read_form(form->path, true));
		      dbase_prune();
		      /* FIXME: not form->path, but form->dir/<name> */
		      /* On the other hand, I don't save name at all */
		      /* but using form->name as previously is right out */
		      switch_form(mainwindow->card, form->path);
		      form_delete(form);
		      form = 0;
	 	      return(0);

	  case EA_CANCEL: if(create_query_popup(shell,
				"form_cancel", "Ok to discard changes?")) {
			destroy_formedit_window();
			destroy_canvas_window();
			destroy_query_window();
			form_delete(form);
			form = 0;
		      }
	 	      return(0);

	  case EA_TYPE:
	 	      item->type = (ITYPE)item_types[reinterpret_cast<QComboBox *>(tp->widget)->currentIndex()].type;
		      all = true;
		      break;

	  case EA_FLNSRT: IFS(item->,NOSORT);
		      if (item->name)
				for (i=0; i < form->nitems; i++) {
					ip = form->items[i];
					if (ip->name &&
					    !strcmp(item->name, ip->name))
						IFX(ip->,item->,NOSORT);
				}
		      break;

	  case EA_DEFSRT: IFT(item->,DEFSORT);
		      if (item->name)
				for (i=0; i < form->nitems; i++)
				    if (i != canvas->curr_item) {
					ip = form->items[i];
					if (ip->name && !strcmp(item->name, ip->name))
					    IFX(ip->,item->,DEFSORT);
					else
					    IFC(ip->,DEFSORT);
				}
		      break;

	  case EA_INMIN: item->min = get_dsb_value(w);			break;
	  case EA_INMAX: item->max = get_dsb_value(w);			break;
	  case EA_INDIG: item->digits = get_sb_value(w); set_digits(item->digits);	break;
	  case EA_MENUST:
	  case EA_MENUDY:
	  case EA_MENUSD:  item->dcombo = (DCOMBO)(tp->code - EA_MENUST);
		      for (code=EA_MENUST; code <= EA_MENUSD; code++)
				fillout_formedit_widget_by_code((enum edact)code);
		       break;
	  case EA_FKDB: {
		  const char *s = read_text_button(w, 0);
	  	  zfree(item->fkey_form_name);
		  item->fkey_form_name = zstrdup(s);
		  item->fkey_form = NULL;
		  fill_key_table(key_w);
		  break;
	  }
	  case EA_FLMUL:  IFT(item->,MULTICOL);
		       if (IFL(item->,MULTICOL) && item->nmenu) {
			       int had_0 = avail_column(form, item);
			       for (i=0; i < item->nmenu; i++)
				       if (!item->menu[i].column) {
					       if(!had_0)
						       had_0 = true;
					       else
						       item->menu[i].column = avail_column(form, NULL);
				       }
		       }
		       sensitize_formedit(); fill_menu_table(menu_w);
	               break;

	  case EA_FKHDR: IFT(item->,FKEY_HEADER);		break;
#if 0
	  case EA_FKSRCH: IFT(item->,FKEY_SEARCH);		break;
#endif
	  case EA_FKMUL: IFT(item->,FKEY_MULTI);		break;

	  case EA_LABFH:
	  case EA_LABFO:
	  case EA_LABFN:
	  case EA_LABFB:
	  case EA_LABFC: item->labelfont = tp->code - EA_LABFH;
		      for (code=EA_LABFH; code <= EA_LABFC; code++)
				fillout_formedit_widget_by_code((enum edact)code);
		      all = true;
		      break;

	  case EA_INFH:
	  case EA_INFO:
	  case EA_INFN:
	  case EA_INFB:
	  case EA_INFC: item->inputfont = tp->code - EA_INFH;
		      for (code=EA_INFH; code <= EA_INFC; code++)
				fillout_formedit_widget_by_code((enum edact)code);
		      all = true;
		      break;

	  case EA_PLDT:
	  case EA_PLLEN:
	  case EA_PLEW:
	  case EA_PLLW:
	  case EA_PLDAYR:
	  case EA_PLEND:
	  case EA_PLCOL:
	  case EA_PLNOTE:
	  case EA_PLMSG:
	  case EA_PLSCR:
	  case EA_PLSUSP:
	  case EA_PLNTM:
	  case EA_PLNAL: item->plan_if = plan_code[tp->code - EA_PLDT];
		      for (code=EA_PLDT; code <= EA_PLNAL; code++)
				fillout_formedit_widget_by_code((enum edact)code);
		      break;

	  case EA_LABJL: item->labeljust = J_LEFT;
		      fillout_formedit_widget_by_code(EA_LABJC);
		      fillout_formedit_widget_by_code(EA_LABJR);	break;
	  case EA_LABJC: item->labeljust = J_CENTER;
		      fillout_formedit_widget_by_code(EA_LABJL);
		      fillout_formedit_widget_by_code(EA_LABJR);	break;
	  case EA_LABJR: item->labeljust = J_RIGHT;
		      fillout_formedit_widget_by_code(EA_LABJL);
		      fillout_formedit_widget_by_code(EA_LABJC);	break;

	  case EA_INJL: item->inputjust = J_LEFT;
		      fillout_formedit_widget_by_code(EA_INJC);
		      fillout_formedit_widget_by_code(EA_INJR);		break;
	  case EA_INJC: item->inputjust = J_CENTER;
		      fillout_formedit_widget_by_code(EA_INJL);
		      fillout_formedit_widget_by_code(EA_INJR);		break;
	  case EA_INJR: item->inputjust = J_RIGHT;
		      fillout_formedit_widget_by_code(EA_INJL);
		      fillout_formedit_widget_by_code(EA_INJC);		break;

	  case EA_FLSRCH: IFT(item->,SEARCH);				break;
	  case EA_FLRO: IFT(item->,RDONLY);				break;
	  case EA_ISDATE: item->timefmt = T_DATE;		all = true;	break;
	  case EA_ISTIME: item->timefmt = T_TIME;		all = true;	break;
	  case EA_ISDTTM: item->timefmt = T_DATETIME;	all = true;	break;
	  case EA_ISDUR: item->timefmt = T_DURATION;	all = true;	break;
	  case EA_DTWID: item->timewidget ^= 1;				break;
	  case EA_DTCAL: item->timewidget ^= 2;				break;

	  case EA_INLEN: item->maxlen   = get_sb_value(w);			break;
	  case EA_SUMCOL: item->sumcol   = get_sb_value(w);			break;
	  case EA_SUMWID: item->sumwidth = get_sb_value(w);			break;
	  case EA_COL: item->column   = get_sb_value(w);			break;
	  case EA_FLDNM: (void)read_text_button(w, &item->name);		break;
	  case EA_FLCODE: (void)read_text_button(w, &item->flagcode);	break;
	  case EA_FLSUM: (void)read_text_button(w, &item->flagtext);	break;
	  case EA_LABEL: (void)read_text_button(w, &item->label);		break;
	  case EA_INDEF: (void)read_text_button(w, &item->idefault);	break;

	  case EA_GREYIF: (void)read_text_button(w, &item->gray_if);	break;
	  case EA_HIDEIF: (void)read_text_button(w, &item->invisible_if);	break;
	  case EA_ROIF: (void)read_text_button(w, &item->freeze_if);	break;
	  case EA_SKIPIF: (void)read_text_button(w, &item->skip_if);	break;

	  case EA_BUTACT: (void)read_text_button(w, &item->pressed);	break;
	  case EA_PLIF: (void)read_text_button(w, &form->planquery);	break;
	  case EA_SUMPR: (void)read_text_button(w, &item->sumprint);	break;

	  case EA_CHXMIN: item->ch_xmin   = get_dsb_value(w);			break;
	  case EA_CHXMAX: item->ch_xmax   = get_dsb_value(w);			break;
	  case EA_CHYMIN: item->ch_ymin   = get_dsb_value(w);			break;
	  case EA_CHYMAX: item->ch_ymax   = get_dsb_value(w);			break;
	  case EA_CHXGRD: item->ch_xgrid  = get_dsb_value(w);			break;
	  case EA_CHYGRD: item->ch_ygrid  = get_dsb_value(w);			break;
	  case EA_CHXSNP: item->ch_xsnap  = get_dsb_value(w);			break;
	  case EA_CHYSNP: item->ch_ysnap  = get_dsb_value(w);			break;
	  case EA_CHXAUT: IFT(item->,CH_XAUTO);				break;
	  case EA_CHYAUT: IFT(item->,CH_YAUTO);				break;

	  case EA_CHADD: add_chart_component(item); all = true;		break;
	  case EA_CHDEL: del_chart_component(item); all = true;		break;
	  case EA_CHPREV: if (item->ch_curr) item->ch_curr--; all = true;	break;
	  case EA_CHNEXT: if (item->ch_curr < item->ch_ncomp-1) item->ch_curr++;
		      all = true;					break;

	  case EA_CHLINE: chart->line ^= true;				break;
	  case EA_CHXFAT: chart->xfat ^= true;				break;
	  case EA_CHYFAT: chart->yfat ^= true;				break;
	  case EA_CHNOTIF: (void)read_text_button(w, &chart->excl_if);	break;
	  case EA_CHCOL: (void)read_text_button(w, &chart->color);		break;
	  case EA_CHLAB: (void)read_text_button(w, &chart->label);		break;

	  case EA_CHXNXT: chart->value[0].mode = CC_NEXT;			break;
	  case EA_CHXPREV: chart->value[0].mode = CC_SAME;			break;
	  case EA_CHXEXPM: chart->value[0].mode = CC_EXPR;			break;
	  case EA_CHXDRGM: chart->value[0].mode = CC_DRAG;			break;
	  case EA_CHXEXPR: (void)read_text_button(w, &chart->value[0].expr);	break;
	  case EA_CHXDRAG: chart->value[0].field=get_sb_value(w);		break;
	  case EA_CHXDMUL: chart->value[0].mul= get_dsb_value(w);		break;
	  case EA_CHXDPL: chart->value[0].add= get_dsb_value(w);		break;

	  case EA_CHYNXT: chart->value[1].mode = CC_NEXT;			break;
	  case EA_CHYPREV: chart->value[1].mode = CC_SAME;			break;
	  case EA_CHYEXPM: chart->value[1].mode = CC_EXPR;			break;
	  case EA_CHYDRGM: chart->value[1].mode = CC_DRAG;			break;
	  case EA_CHYEXPR: (void)read_text_button(w, &chart->value[1].expr);	break;
	  case EA_CHYDRAG: chart->value[1].field=get_sb_value(w);		break;
	  case EA_CHYDMUL: chart->value[1].mul= get_dsb_value(w);		break;
	  case EA_CHYDPL: chart->value[1].add= get_dsb_value(w);		break;

	  case EA_CHXSZEX: chart->value[2].mode = CC_EXPR;			break;
	  case EA_CHXSZ: (void)read_text_button(w, &chart->value[2].expr);	break;

	  case EA_CHYSZEX: chart->value[3].mode = CC_EXPR;			break;
	  case EA_CHYSZ: (void)read_text_button(w, &chart->value[3].expr);	break;
	  case EA_CHCUR: break;
	}

	/*
	 * the chart choice buttons are widely separated and must be handled
	 * by hand here, to make sure only one of each group is enabled.
	 */
	if (tp->code >= EA_CHXNXT && tp->code <= EA_CHYDRGM) {
		i = ((tp->code - EA_CHXNXT) & ~3U) + EA_CHXNXT;
		fillout_formedit_widget_by_code((enum edact)(i + 0));
		fillout_formedit_widget_by_code((enum edact)(i + 1));
		fillout_formedit_widget_by_code((enum edact)(i + 2));
		fillout_formedit_widget_by_code((enum edact)(i + 3));
	}

	/*
	 * if a choice item is modified, modify all other choice items with
	 * the same name too.
	 */
	if (item && item->name && item->type == IT_CHOICE)
		for (i=0; i < form->nitems; i++) {
			ip = form->items[i];
			if (i != canvas->curr_item && ip->type == IT_CHOICE
					   && ip->name
					   && !strcmp(ip->name, item->name)) {
				ip->column   = item->column;
				IFX(ip->,item->,SEARCH);
				IFX(ip->,item->,RDONLY);
				IFX(ip->,item->,NOSORT);
				IFX(ip->,item->,DEFSORT);
	  			ip->sumcol   = item->sumcol;
				ip->sumwidth = item->sumwidth;
				if (item->idefault) {
					zfree(ip->idefault);
	  				ip->idefault =mystrdup(item->idefault);
				}
				redraw_canvas_item(ip);
			}
		}
	return(all ? 2 : 1);
}
