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
static int readback_item(int);

static bool		have_shell = false;	/* message popup exists if true */
static QDialog		*shell;		/* popup menu shell */
static FORM		*form;		/* current form definition */
static QWidget		*scroll_w;	/* the widget inside the scroll area */
static QFrame		*chart;		/* the widget for chart options */
static QTableWidget	*menu_w;	/* the menu editor widget */
static GrokCanvas	*canvas;

const char		plan_code[] = "tlwWrecnmsSTA";	/* code 0x260..0x26c */


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
#define BAS ITX | FLG | SNG | MUL
#define PLN ITX | FLG | SNG
#define IDF TXT | FLG | SNG | MUL
#define BUT 1U<<IT_BUTTON
#define CHA 1U<<IT_CHART

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
	{ IT_CHART,  "Chart"		}
	
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

#define MENU_COMBO  (1<<0)			/* only label */
#define MENU_SINGLE (1<<0 | 1<<3 | 1<<6)	/* label, flagcode, flagtext */
#define MENU_MULTI  ~0				/* everything */

static int menu_col_mask(const ITEM *item)
{
	if(item->type == IT_INPUT)
		return MENU_COMBO;
	else if(item->type == IT_MENU || item->type == IT_RADIO || !item->multicol)
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

/*
 * next free: 10a,114 (global), 240 (field), 30b (chart component)
 */

static struct _template {
	unsigned
	int	sensitive;	/* when type is n, make sensitive if & 1<<n */
	int	type;		/* Text, Label, Rradio, Fflag, -line */
	int	code;		/* code given to (global) callback routine */
	const char *text;	/* label string */
	const char *help;	/* label string */
	QWidget	*widget;	/* allocated widget */
	int	role;		/* for buttons using standard labels */
				/* also, offset of text field for combos */
} tmpl[] = {
	{ ALL, 'L',	 0,	"Form name:",		"fe_form",	},
	{ ALL, 'T',	0x101,	" ",			"fe_form",	},
	{ ALL, 'L',	 0,	"Referenced database:",	"fe_form",	},
	{ ALL, 't',	0x102,	" ",			"fe_form",	},
	{ ALL, 'f',	0x104,	"Read only",		"fe_rdonly",	},
	{ ALL, 'f',	0x114,	"Timestamps/Synchronizable","fe_sync",	},
	{ ALL, 'f',	0x105,	"Procedural",		"fe_proc",	},
	{ ALL, 'B',	0x106,	"Edit",			"fe_proc",	},
	{ ALL, 'L',	 0,	"dbase field delim:",	"fe_delim",	},
	{ ALL, 't',	0x103,	" ",			"fe_delim",	},
	{ ALL, 'l',	 0,	"Array elt delimiter:", "fe_adelim",	},
	{ ALL, 't',     0x108,  " ",			"fe_adelim",	},
	{ ALL, 'l',	 0,	"Array elt escape:",	"fe_adelim",	},
	{ ALL, 't',     0x109,  " ",			"fe_adelim",	},
	{   0, 'F',	 0,	" ",			0,		},
	{ ALL, 'L',	 0,	"Comment:",		"fe_ref",	},
	{ ALL, 'T',	0x107,	" ",			"fe_ref",	},

	{ ALL, 'L',	 0,	"Form",			0,		},
	{ ALL, 'B',	0x10c,	"Queries",		"fe_query",	},
	{ ALL, 'B',	0x10b,	"Def Help",		"fe_defhelp",	},
	{ ALL, 'B',	0x10d,	"Debug",		"fe_debug",	},
	{ ALL, 'B',	0x10e,	"Preview",		"fe_preview",	},
	{ ALL, 'B',	0x10f,	NULL,			"edit",		0, dbbb(Help)},
	{ ALL, 'B',	0x110,	NULL,			"fe_cancel",	0, dbbb(Cancel)},
	{ ALL, 'B',	0x111,	"Done",			"fe_done",	},
	{ ALL, '-',	 0,	" ",			0,		},
	{ ALL, 'L',	 0,	"Field",		"fe_buts",	},
	{ ALL, 'B',	0x112,	"Add",			"fe_add",	},
	{ ANY, 'B',	0x113,	"Delete",		"fe_delete",	},
	{   0, '[',	 0,	"attrs",		0,		},

	{ ANY, 'L',	 0,	   "Field type:",	"fe_type",	},
	{ ANY, 'I',  IT_LABEL,	   " ",			"fe_type",	},
	{ ANY, 'L',	 0,	"Flags:",		"fe_flags",	},
	{   0, 'F',	 0,	" ",			0,		},
	{ BAS, 'f',	0x200,	"Searchable",		"fe_flags",	},
	{ BAS, 'f',	0x201,	"Read only",		"fe_flags",	},
	{ BAS, 'f',	0x202,	"Not sortable",		"fe_flags",	},
	{ BAS, 'f',	0x203,	"Default sort",		"fe_flags",	},
	{ MUL, 'f',	0x23e,	"Multi-field",		"fe_menu",	},
	{ ANY, 'L',	 0,	"Internal field name:",	"fe_int",	},
	{ ANY, 't',	0x204,	" ",			"fe_int",	},
	{ BAS, 'L',	 0,	"Database column:",	"fe_column",	},
	{ BAS, 'i',	0x205,	" ",			"fe_column",	0, 999 },
	{ BAS, 'l',	 0,	"Summary column:",	"fe_sum",	},
	{ BAS, 'i',	0x206,	" ",			"fe_sum",	0, 999 },
	{ BAS, 'l',	 0,	"Width in summary:",	"fe_sum",	},
	{ BAS, 'i',	0x207,	" ",			"fe_sum",	0, 100 },
	{ BAS, 'L',	 0,	"Show in summary:",	"fe_sump",	},
	{ BAS, 'T',	0x229,	" ",			"fe_sump",	},
	{ FLG, 'L',	 0,	"Choice/flag code:",	"fe_flag",	},
	{ FLG, 't',	0x208,	" ",			"fe_flag",	},
	{ FLG, 'l',	 0,	"shown in summary as",	"fe_flag",	},
	{ FLG, 't',	0x236,	" ",			"fe_flag",	},
	{ TIM, 'L',	 0,	"Time format:",		"fe_time",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ TIM, 'r',	0x209,	"Date",			"fe_time",	},
	{ TIM, 'r',	0x20a,	"Time",			"fe_time",	},
	{ TIM, 'r',	0x20b,	"Date+time",		"fe_time",	},
	{ TIM, 'r',	0x20c,	"Duration",		"fe_time",	},
	{ TIM, 'f',	0x23f,	"Widget",		"fe_time",	},
	{ ANY, '-',	 0,	" ",			0,		},

	{ ANY, 'L',	 0,	"Label Justification:",	"fe_ljust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ ANY, 'r',	0x20d,	"left",			"fe_ljust",	},
	{ ANY, 'r',	0x20e,	"center",		"fe_ljust",	},
	{ ANY, 'r',	0x20f,	"right",		"fe_ljust",	},
	{ ANY, 'L',	 0,	"Label font:",		"fe_ljust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ ANY, 'r',	0x210,	"Helv",			"fe_lfont",	},
	{ ANY, 'r',	0x211,	"HelvO",		"fe_lfont",	},
	{ ANY, 'r',	0x212,	"HelvN",		"fe_lfont",	},
	{ ANY, 'r',	0x213,	"HelvB",		"fe_lfont",	},
	{ ANY, 'r',	0x214,	"Courier",		"fe_lfont",	},
	{ ANY, 'L',	 0,	"Label text",		"fe_ltxt",	},
	{ ANY, 'T',	0x21a,	" ",			"fe_ltxt",	},
	{ ANY, '-',	 0,	" ",			0,		},

	{ TXT, 'L',	 0,	"Input Justification:",	"fe_ijust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ TXT, 'r',	0x21b,	"left",			"fe_ijust",	},
	{ TXT, 'r',	0x21c,	"center",		"fe_ijust",	},
	{ TXT, 'r',	0x21d,	"right",		"fe_ijust",	},
	{ FT2, 'L',	 0,	"Input font:",		"fe_ijust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ FT2, 'r',	0x215,	"Helv",			"fe_ifont",	},
	{ FT2, 'r',	0x216,	"HelvO",		"fe_ifont",	},
	{ FT2, 'r',	0x217,	"HelvN",		"fe_ifont",	},
	{ FT2, 'r',	0x218,	"HelvB",		"fe_ifont",	},
	{ FT2, 'r',	0x219,	"Courier",		"fe_ifont",	},
	{ ITP, 'L',	 0,	"Max input length:",	"fe_range",	},
	{ ITP, 'i',	0x21f,	" ",			"fe_range",	0, 999999},
	{ NUM, 'L',	 0,	"Min Value",		"fe_range",	},
	{ NUM, 'd',	0x237,	" ",			"fe_range",	},
	{ NUM, 'l',	 0,	"Max Value",		"fe_range",	},
	{ NUM, 'd',	0x238,	" ",			"fe_range",	},
	{ NUM, 'l',	 0,	"Digits",		"fe_range",	},
	{ NUM, 'i',	0x239,	" ",			"fe_range",	0, 20},
	{ IDF, 'L',	 0,	"Input default:",	"fe_def",	},
	{ IDF, 'T',	0x220,	" ",			"fe_def",	},
	{ MNU, 'L',	 0,	"Menu",			"fe_menu",	},
	{ MNU, 'M',	 0,	" ",			"fe_menu",	},
	{   0, 'R',	 0,	" ",			"fe_menu",	},
	{ INP, 'r',	0x23b,	"Static",		"fe_menu",	},
	{ INP, 'r',	0x23c,	"Dynamic",		"fe_menu",	},
	{ INP, 'r',	0x23d,	"All",			"fe_menu",	},
	{ TXT, '-',	 0,	" ",			0,		},

	{ PLN, 'L',	 0,	"Calendar interface:",	"fe_plan",	},
	{   0, 'R',	 0,	" ",			0,		},
    /* FIXME:  make this a choice menu (with None/blank as a choice) */
	{ TIM, 'r',	0x260,  "Date+time",		"fe_plan",	},
	{ PLN, 'r',	0x261,  "Length",		"fe_plan",	},
	{ PLN, 'r',	0x262,  "Early warn",		"fe_plan",	},
	{ PLN, 'r',	0x263,  "Late warn",		"fe_plan",	},
	{ PLN, 'r',	0x264,  "Day repeat",		"fe_plan",	},
	{ TIM, 'r',	0x265,  "End date",		"fe_plan",	},
	{ PLN, 'L',	 0,	"",			"fe_plan",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ PLN, 'r',	0x266,  "Color",		"fe_plan",	},
	{ PLN, 'r',	0x267,  "Note",			"fe_plan",	},
	{ PLN, 'r',	0x268,  "Message",		"fe_plan",	},
	{ PLN, 'r',	0x269,  "Script",		"fe_plan",	},
	{ PLN, 'r',	0x26a,  "Suspended",		"fe_plan",	},
	{ PLN, 'r',	0x26b,  "No time",		"fe_plan",	},
	{ PLN, 'r',	0x26c,  "No alarm",		"fe_plan",	},
    /* FIXME:  planquery is form-wide, so it should be in top area */
    /*         I guess it's down here to keep the plan-related stuff together */
	{ PLN, 'L',	 0,	"Shown in calendar if:","fe_plan",	},
	{ PLN, 'T',	0x228,	" ",			"fe_plan",	},
	{ PLN, '-',	 0,	" ",			0,		},

	{ ANY, 'L',	 0,	"Grayed out if:",	"fe_gray",	},
	{ ANY, 'C',	0x222,	" ",			"fe_gray",	0, offsetof(ITEM, gray_if) },
	{ ANY, 'L',	 0,	"Invisible if:",	"fe_inv",	},
	{ ANY, 'C',	0x223,	" ",			"fe_inv",	0, offsetof(ITEM, invisible_if) },
	{ ANY, 'L',	 0,	"Read-only if:",	"fe_ro",	},
	{ ANY, 'C',	0x224,	" ",			"fe_ro",	0, offsetof(ITEM, freeze_if) },
	{ ANY, 'L',	 0,	"Skip if:",		"fe_skip",	},
	{ ANY, 'C',	0x225,	" ",			"fe_skip",	0, offsetof(ITEM, skip_if) },
	{ BUT, '-',	 0,	" ",			0,		},

	{ BUT, 'L',	 0,	"Action when pressed:",	"fe_press",	},
	{ BUT, 'T',	0x226,	" ",			"fe_press",	},
	{ CHA, '-',	 0,	" ",			0,		},

	{ CHA, 'L',	 0,	"Chart X range:",	"fe_chart",	},
	{ CHA, 'd',	0x280,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"to",			"fe_chart",	},
	{ CHA, 'd',	0x281,	" ",			"fe_chart",	},
	{   0, 'F',	 0, 	" ",			0,		},
	{ CHA, 'f',	0x28c,	"automatic",		"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart Y range:",	"fe_chart",	},
	{ CHA, 'd',	0x282,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"to",			"fe_chart",	},
	{ CHA, 'd',	0x283,	" ",			"fe_chart",	},
	{   0, 'F',	 0, 	" ",			0,		},
	{ CHA, 'f',	0x28d,	"automatic",		"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart XY grid every:",	"fe_chart",	},
	{ CHA, 'd',	0x284,	" ",			"fe_chart",	},
	{ CHA, 'd',	0x285,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart XY snap every:",	"fe_chart",	},
	{ CHA, 'd',	0x286,	" ",			"fe_chart",	},
	{ CHA, 'd',	0x287,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart component:",	0,		},
	{ CHA, 'B',	0x290,	"Add",			"fe_chart",	},
	{ CHA, 'B',	0x291,	"Delete",		"fe_chart",	},
	{ CHA, 'l',	0x294,	"none",			"fe_chart",	},
	{ CHA, 'B',	0x293,	"Next",			"fe_chart",	},
	{ CHA, 'B',	0x292,	"Previous",		"fe_chart",	},

	{ CHA, '{',	 0,	"comps",		0,		},
	{ CHA, 'L',	 0, 	"Component flags",	"fe_chart",	},
	{   0, 'F',	 0, 	" ",			0,		},
	{ CHA, 'f',	0x301,	"Line",			"fe_chart",	},
	{ CHA, 'f',	0x302,	"X fat",		"fe_chart",	},
	{ CHA, 'f',	0x303,	"Y fat",		"fe_chart",	},
	{ CHA, 'L',	 0,	"Exclude if:",		"fe_chart",	},
	{ CHA, 'T',	0x304,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Color 0..7:",		"fe_chart",	},
	{ CHA, 'T',	0x305,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Label:",		"fe_chart",	},
	{ CHA, 'T',	0x306,	" ",			"fe_chart",	},

	{ CHA, 'L',	 0,	"X position:",		"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x310,	"next free",		"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x311,	"same as previous",	"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x312,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x314,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x313,	"drag field",		"fe_chart",	},
        // FIXME:  this should probably be a combo box with field names
	{ CHA, 'i',	0x315,	" ",			"fe_chart",	0, 999},
	{ CHA, 'l',	 0,	"multiplied by",	"fe_chart",	},
	{ CHA, 'd',	0x316,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"plus",			"fe_chart",	},
	{ CHA, 'd',	0x317,	" ",			"fe_chart",	},

	{ CHA, 'L',	 0,	"Y position:",		"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x320,	"next free",		"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x321,	"same as previous",	"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x322,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x324,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x323,	"drag field",		"fe_chart",	},
        // FIXME:  this should probably be a combo box with field names
	{ CHA, 'i',	0x325,	" ",			"fe_chart",	0, 999},
	{ CHA, 'l',	 0,	"multiplied by",	"fe_chart",	},
	{ CHA, 'd',	0x326,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"plus",			"fe_chart",	},
	{ CHA, 'd',	0x327,	" ",			"fe_chart",	},

	{ CHA, 'L',	 0,	"X size:",		"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x332,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x334,	" ",			"fe_chart",	},

	{ CHA, 'L',	 0,	"Y size:",		"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x342,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x344,	" ",			"fe_chart",	},

	{   0, '}',	 0,	" ",			0,		},

	{   0, ']',	 0,	" ",			0,		},
	{   0,  0,	 0,	0,			0		},
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
		    case 'M': {
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
		else if(tp->type == 'C')
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
				tp->code == 0x106 ? form->proc :
				tp->code == 0x113 ? item != 0 : true);
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
		if ((mask & (MUL)) &&
		    ((tp->code >= 0x204 && tp->code <= 0x207) || tp->code == 0x236)) {
			tp->widget->setEnabled(!item->multicol);
			tp[-1].widget->setEnabled(!item->multicol);
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
}


void fillout_formedit_widget_by_code(
	int			code)
{
	struct _template	*tp;

	for (tp=tmpl; tp->type; tp++)
		if (tp->code == code) {
			fillout_formedit_widget(tp);
			break;
		}
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
		if (tp->code == 0x237 || tp->code == 0x238)
			reinterpret_cast<QDoubleSpinBox *>(tp->widget)->setDecimals(dig);
}

static void fillout_formedit_widget(
	struct _template	*tp)
{
	ITEM			*item = NULL;
	CHART			*chart = NULL;
	CHART			nullchart;
	QWidget			*w = tp->widget;

	if (tp->code < 0x100 || tp->code > 0x107) {
		if (!form->items || canvas->curr_item >= form->nitems ||
		    !(tp->sensitive & (1 << form->items[canvas->curr_item]->type))) {
			if (tp->type == 'T' || tp->type == 't' ||
			    tp->type == 'i' || tp->type == 'd' ||
			    tp->type == 'C')
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
	  case 0x101: print_text_button_s(w, form->name);		break;
	  case 0x102: print_text_button_s(w, form->dbase);		break;
	  case 0x107: print_text_button_s(w, form->comment);		break;
	  case 0x103: print_text_button_s(w, to_octal(form->cdelim));	break;
	  case 0x104: set_toggle(w, form->rdonly);			break;
	  case 0x114: set_toggle(w, form->syncable);			break;
	  case 0x105: set_toggle(w, form->proc);
		      fillout_formedit_widget_by_code(0x106);		break;
	  case 0x106: w->setEnabled(form->proc);			break;
	  case 0x108: print_text_button_s(w, to_octal(form->asep ? form->asep : '|'));	break;
	  case 0x109: print_text_button_s(w, to_octal(form->aesc ? form->aesc : '\\'));	break;

	  case IT_LABEL:
		  for(int n = 0; n < ALEN(item_types); n++)
			  if(item_types[n].type == item->type) {
				  reinterpret_cast<QComboBox *>(w)->setCurrentIndex(n);
				  break;
			  }
		  break;

	  case 0x210:
	  case 0x211:
	  case 0x212:
	  case 0x213:
	  case 0x214: set_toggle(w, item->labelfont == tp->code-0x210);	break;

	  case 0x215:
	  case 0x216:
	  case 0x217:
	  case 0x218:
	  case 0x219: set_toggle(w, item->inputfont == tp->code-0x215);	break;

	  case 0x20d: set_toggle(w, item->labeljust == J_LEFT);		break;
	  case 0x20e: set_toggle(w, item->labeljust == J_CENTER);	break;
	  case 0x20f: set_toggle(w, item->labeljust == J_RIGHT);	break;

	  case 0x21b: set_toggle(w, item->inputjust == J_LEFT);		break;
	  case 0x21c: set_toggle(w, item->inputjust == J_CENTER);	break;
	  case 0x21d: set_toggle(w, item->inputjust == J_RIGHT);	break;

	  case 0x200: set_toggle(w, item->search);			break;
	  case 0x201: set_toggle(w, item->rdonly);			break;
	  case 0x202: set_toggle(w, item->nosort);			break;
	  case 0x203: set_toggle(w, item->defsort);			break;
	  case 0x209: set_toggle(w, item->timefmt == T_DATE);		break;
	  case 0x20a: set_toggle(w, item->timefmt == T_TIME);		break;
	  case 0x20b: set_toggle(w, item->timefmt == T_DATETIME);	break;
	  case 0x20c: set_toggle(w, item->timefmt == T_DURATION);	break;
	  case 0x23f: set_toggle(w, item->timewidget);			break;

	  case 0x260:
	  case 0x261:
	  case 0x262:
	  case 0x263:
	  case 0x264:
	  case 0x265:
	  case 0x266:
	  case 0x267:
	  case 0x268:
	  case 0x269:
	  case 0x26a:
	  case 0x26b:
	  case 0x26c: set_toggle(w, item->plan_if == plan_code[tp->code & 31]);
		      sensitize_formedit();
		      break;

	  case 0x237: set_dsb_value(w, item->min);			break;
	  case 0x238: set_dsb_value(w, item->max);			break;
	  case 0x230: set_sb_value(w, item->digits); set_digits(item->digits);			break;
	  case 0x23b:
	  case 0x23c:
	  case 0x23d: set_toggle(w, item->dcombo == tp->code - 0x23b);	break;
	  case 0x23e: set_toggle(w, item->multicol);			break;
	  case 0x21f: set_sb_value(w, item->maxlen);			break;
	  case 0x206: set_sb_value(w, item->sumcol);			break;
	  case 0x207: set_sb_value(w, item->sumwidth);			break;
	  case 0x205: set_sb_value(w, item->column);			break;
	  case 0x204: print_text_button_s(w, item->name);		break;
	  case 0x208: print_text_button_s(w, item->flagcode);		break;
	  case 0x236: print_text_button_s(w, item->flagtext);		break;
	  case 0x21a: print_text_button_s(w, item->label);		break;
	  case 0x220: print_text_button_s(w, item->idefault);		break;

	  case 0x222: print_text_button_s(w, item->gray_if);		break;
	  case 0x223: print_text_button_s(w, item->invisible_if);	break;
	  case 0x224: print_text_button_s(w, item->freeze_if);		break;
	  case 0x225: print_text_button_s(w, item->skip_if);		break;

	  case 0x226: print_text_button_s(w, item->pressed);		break;
	  case 0x228: print_text_button_s(w, form->planquery);		break;
	  case 0x229: print_text_button_s(w, item->sumprint);		break;

	  case 0x280: set_dsb_value(w, item->ch_xmin);			break;
	  case 0x281: set_dsb_value(w, item->ch_xmax);			break;
	  case 0x282: set_dsb_value(w, item->ch_ymin);			break;
	  case 0x283: set_dsb_value(w, item->ch_ymax);			break;
	  case 0x284: set_dsb_value(w, item->ch_xgrid);			break;
	  case 0x285: set_dsb_value(w, item->ch_ygrid);			break;
	  case 0x286: set_dsb_value(w, item->ch_xsnap);			break;
	  case 0x287: set_dsb_value(w, item->ch_ysnap);			break;
	  case 0x28c: set_toggle(w, item->ch_xauto);			break;
	  case 0x28d: set_toggle(w, item->ch_yauto);			break;
	  case 0x294: print_button(w, item->ch_ncomp ? "%d of %d" : "none",
				    item->ch_curr+1, item->ch_ncomp);	break;

	  case 0x301: set_toggle(w, chart->line);			break;
	  case 0x302: set_toggle(w, chart->xfat);			break;
	  case 0x303: set_toggle(w, chart->yfat);			break;
	  case 0x304: print_text_button_s(w, chart->excl_if);		break;
	  case 0x305: print_text_button_s(w, chart->color);		break;
	  case 0x306: print_text_button_s(w, chart->label);		break;

	  case 0x310: set_toggle(w, chart->value[0].mode == CC_NEXT);	break;
	  case 0x311: set_toggle(w, chart->value[0].mode == CC_SAME);	break;
	  case 0x312: set_toggle(w, chart->value[0].mode == CC_EXPR);	break;
	  case 0x313: set_toggle(w, chart->value[0].mode == CC_DRAG);	break;
	  case 0x314: print_text_button_s(w, chart->value[0].expr);	break;
	  case 0x315: set_sb_value(w, chart->value[0].field);		break;
	  case 0x316: set_dsb_value(w, chart->value[0].mul);		break;
	  case 0x317: set_dsb_value(w, chart->value[0].add);		break;

	  case 0x320: set_toggle(w, chart->value[1].mode == CC_NEXT);	break;
	  case 0x321: set_toggle(w, chart->value[1].mode == CC_SAME);	break;
	  case 0x322: set_toggle(w, chart->value[1].mode == CC_EXPR);	break;
	  case 0x323: set_toggle(w, chart->value[1].mode == CC_DRAG);	break;
	  case 0x324: print_text_button_s(w, chart->value[1].expr);	break;
	  case 0x325: set_sb_value(w, chart->value[1].field);		break;
	  case 0x326: set_dsb_value(w, chart->value[1].mul);		break;
	  case 0x327: set_dsb_value(w, chart->value[1].add);		break;

	  case 0x332: set_toggle(w, chart->value[2].mode == CC_EXPR);	break;
	  case 0x334: print_text_button_s(w, chart->value[2].expr);	break;

	  case 0x342: set_toggle(w, chart->value[3].mode == CC_EXPR);	break;
	  case 0x344: print_text_button_s(w, chart->value[3].expr);	break;
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
			if(!--item->nmenu)
				free(item->menu);
			else
				memmove(item->menu + row, item->menu + row + 1, (item->nmenu - row) * sizeof(MENU));
			w->removeRow(row);
			do_resize = true;
		} else { // dup
			grow(0, "new menu item", MENU, item->menu, ++item->nmenu, NULL);
			memmove(item->menu + row + 1, item->menu + row, (item->nmenu - row - 1) * sizeof(MENU));
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
			    tp->type == 'C')
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
	if (!chart && (tp->code & 0x300) == 0x300)
		return(0);
	if (item && item == filling_item)
		return(0);

	switch(tp->code) {
	  case 0x101: (void)read_text_button_noblanks(w, &form->name);
		      if (form->name && !form->dbase) {
				form->dbase = mystrdup(form->name);
				fillout_formedit_widget_by_code(0x102);
		      }
		      break;

	  case 0x102: (void)read_text_button_noblanks(w, &form->dbase);	break;
	  case 0x107: (void)read_text_button(w, &form->comment);	break;
	  case 0x103: form->cdelim=to_ascii(read_text_button(w,0),':');	break;
	  case 0x104: form->rdonly     ^= true;				break;
	  case 0x114: form->syncable   ^= true;				break;
	  case 0x105: form->proc       ^= true; sensitize_formedit();	break;
	  case 0x106: form_edit_script(form, w, form->dbase);		break;

	  case 0x108: form->asep=to_ascii(read_text_button(w,0),'|');	break;
	  case 0x109: form->aesc=to_ascii(read_text_button(w,0),'\\');	break;
	  case 0x112: readback_formedit();
		      item_deselect(form);
		      (void)item_create(form, canvas->curr_item);
		      form->items[canvas->curr_item]->selected = true;
		      item = 0; // skip item adjustment below
		      all = true;
		      break;

	  case 0x113: item_deselect(form);
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

	  case 0x10b: create_edit_popup("Card Help Editor",
					 &form->help, false, "fe_help");
		      break;

	  case 0x10c: create_query_window(form);
		      break;

	  case 0x10d: if (!verify_form(form, &i, shell) && i < form->nitems) {
				item_deselect(form);
				canvas->curr_item = i;
				form->items[i]->selected = true;
				redraw_canvas_item(form->items[i]);
				fillout_formedit();
				sensitize_formedit();
		      } else {
				fillout_formedit();
				sensitize_formedit();
		      }
		      break;

	  case 0x10e: create_card_menu(form, 0, 0, false);
		      break;

	  case 0x10f: help_callback(shell, "edit");
	 	      return(0);

	  case 0x111: readback_formedit();
		      if (!verify_form(form, &i, shell)) {
				if (i < form->nitems) {
					item_deselect(form);
					canvas->curr_item = i;
					form->items[i]->selected = true;
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
				      for(DBASE **prev = &dbase_list; *prev; prev = &(*prev)->next)
					      if(!strcmp((*prev)->form->path, formpath)) {
						      DBASE *dbase = *prev;
						      if(dbase->modified &&
							 !create_save_popup(mainwindow,
									    dbase,
									    "Form configuration changes require reloading database %s\n"
									    "Discard changes and reload?",
									    formpath))
							      return(0);
						      if(mainwindow->card &&
							 mainwindow->card->dbase == dbase) {
							      switch_form(mainwindow->card, 0);
							      /* switch_form may free dbase */
							      for(prev = &dbase_list; *prev; prev = &(*prev)->next)
								      if(*prev == dbase)
									      break;
							      if(!*prev)
								      break;
						      }
						      *prev = dbase->next;
						      dbase_clear(dbase);
						      free(dbase);
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
		      /* FIXME: not form->path, but form->dir/<name> */
		      /* On the other hand, I don't save name at all */
		      /* but using form->name as previously is right out */
		      switch_form(mainwindow->card, form->path);
		      form_delete(form);
		      form = 0;
	 	      return(0);

	  case 0x110: if(create_query_popup(shell,
				"form_cancel", "Ok to discard changes?")) {
			destroy_formedit_window();
			destroy_canvas_window();
			destroy_query_window();
			form_delete(form);
			form = 0;
		      }
	 	      return(0);

	  case IT_LABEL:
	 	      item->type = (ITYPE)item_types[reinterpret_cast<QComboBox *>(tp->widget)->currentIndex()].type;
		      all = true;
		      break;

	  case 0x202: item->nosort ^= true;
		      if (item->name)
				for (i=0; i < form->nitems; i++) {
					ip = form->items[i];
					if (ip->name &&
					    !strcmp(item->name, ip->name))
						ip->nosort = item->nosort;
				}
		      break;

	  case 0x203: item->defsort ^= true;
		      if (item->name)
				for (i=0; i < form->nitems; i++)
				    if (i != canvas->curr_item) {
					ip = form->items[i];
					ip->defsort = !strcmp(item->name,
					     ip->name) ? item->defsort : false;
				}
		      break;

	  case 0x237: item->min = get_dsb_value(w);			break;
	  case 0x238: item->max = get_dsb_value(w);			break;
	  case 0x239: item->digits = get_sb_value(w); set_digits(item->digits);	break;
	  case 0x23b:
	  case 0x23c:
	  case 0x23d:  item->dcombo = (DCOMBO)(tp->code - 0x23b);
		      for (code=0x23b; code <= 0x23d; code++)
				fillout_formedit_widget_by_code(code);
		       break;
	  case 0x23e:  item->multicol ^= true;	sensitize_formedit(); fill_menu_table(menu_w);	break;

	  case 0x210:
	  case 0x211:
	  case 0x212:
	  case 0x213:
	  case 0x214: item->labelfont = tp->code - 0x210;
		      for (code=0x210; code <= 0x214; code++)
				fillout_formedit_widget_by_code(code);
		      all = true;
		      break;

	  case 0x215:
	  case 0x216:
	  case 0x217:
	  case 0x218:
	  case 0x219: item->inputfont = tp->code - 0x215;
		      for (code=0x215; code <= 0x219; code++)
				fillout_formedit_widget_by_code(code);
		      all = true;
		      break;

	  case 0x260:
	  case 0x261:
	  case 0x262:
	  case 0x263:
	  case 0x264:
	  case 0x265:
	  case 0x266:
	  case 0x267:
	  case 0x268:
	  case 0x269:
	  case 0x26a:
	  case 0x26b:
	  case 0x26c: item->plan_if = plan_code[tp->code & 31];
		      for (code=0x260; code <= 0x26c; code++)
				fillout_formedit_widget_by_code(code);
		      break;

	  case 0x20d: item->labeljust = J_LEFT;
		      fillout_formedit_widget_by_code(0x20e);
		      fillout_formedit_widget_by_code(0x20f);		break;
	  case 0x20e: item->labeljust = J_CENTER;
		      fillout_formedit_widget_by_code(0x20d);
		      fillout_formedit_widget_by_code(0x20f);		break;
	  case 0x20f: item->labeljust = J_RIGHT;
		      fillout_formedit_widget_by_code(0x20d);
		      fillout_formedit_widget_by_code(0x20e);		break;

	  case 0x21b: item->inputjust = J_LEFT;
		      fillout_formedit_widget_by_code(0x21c);
		      fillout_formedit_widget_by_code(0x21d);		break;
	  case 0x21c: item->inputjust = J_CENTER;
		      fillout_formedit_widget_by_code(0x21b);
		      fillout_formedit_widget_by_code(0x21d);		break;
	  case 0x21d: item->inputjust = J_RIGHT;
		      fillout_formedit_widget_by_code(0x21b);
		      fillout_formedit_widget_by_code(0x21c);		break;

	  case 0x200: item->search ^= true;				break;
	  case 0x201: item->rdonly ^= true;				break;
	  case 0x209: item->timefmt = T_DATE;		all = true;	break;
	  case 0x20a: item->timefmt = T_TIME;		all = true;	break;
	  case 0x20b: item->timefmt = T_DATETIME;	all = true;	break;
	  case 0x20c: item->timefmt = T_DURATION;	all = true;	break;
	  case 0x23f: item->timewidget ^= true;				break;

	  case 0x21f: item->maxlen   = get_sb_value(w);			break;
	  case 0x206: item->sumcol   = get_sb_value(w);			break;
	  case 0x207: item->sumwidth = get_sb_value(w);			break;
	  case 0x205: item->column   = get_sb_value(w);			break;
	  case 0x204: (void)read_text_button(w, &item->name);		break;
	  case 0x208: (void)read_text_button(w, &item->flagcode);	break;
	  case 0x236: (void)read_text_button(w, &item->flagtext);	break;
	  case 0x21a: (void)read_text_button(w, &item->label);		break;
	  case 0x220: (void)read_text_button(w, &item->idefault);	break;

	  case 0x222: (void)read_text_button(w, &item->gray_if);	break;
	  case 0x223: (void)read_text_button(w, &item->invisible_if);	break;
	  case 0x224: (void)read_text_button(w, &item->freeze_if);	break;
	  case 0x225: (void)read_text_button(w, &item->skip_if);	break;

	  case 0x226: (void)read_text_button(w, &item->pressed);	break;
	  case 0x228: (void)read_text_button(w, &form->planquery);	break;
	  case 0x229: (void)read_text_button(w, &item->sumprint);	break;

	  case 0x280: item->ch_xmin   = get_dsb_value(w);			break;
	  case 0x281: item->ch_xmax   = get_dsb_value(w);			break;
	  case 0x282: item->ch_ymin   = get_dsb_value(w);			break;
	  case 0x283: item->ch_ymax   = get_dsb_value(w);			break;
	  case 0x284: item->ch_xgrid  = get_dsb_value(w);			break;
	  case 0x285: item->ch_ygrid  = get_dsb_value(w);			break;
	  case 0x286: item->ch_xsnap  = get_dsb_value(w);			break;
	  case 0x287: item->ch_ysnap  = get_dsb_value(w);			break;
	  case 0x28c: item->ch_xauto ^= true;				break;
	  case 0x28d: item->ch_yauto ^= true;				break;

	  case 0x290: add_chart_component(item); all = true;		break;
	  case 0x291: del_chart_component(item); all = true;		break;
	  case 0x292: if (item->ch_curr) item->ch_curr--; all = true;	break;
	  case 0x293: if (item->ch_curr < item->ch_ncomp-1) item->ch_curr++;
		      all = true;					break;

	  case 0x301: chart->line ^= true;				break;
	  case 0x302: chart->xfat ^= true;				break;
	  case 0x303: chart->yfat ^= true;				break;
	  case 0x304: (void)read_text_button(w, &chart->excl_if);	break;
	  case 0x305: (void)read_text_button(w, &chart->color);		break;
	  case 0x306: (void)read_text_button(w, &chart->label);		break;

	  case 0x310: chart->value[0].mode = CC_NEXT;			break;
	  case 0x311: chart->value[0].mode = CC_SAME;			break;
	  case 0x312: chart->value[0].mode = CC_EXPR;			break;
	  case 0x313: chart->value[0].mode = CC_DRAG;			break;
	  case 0x314: (void)read_text_button(w, &chart->value[0].expr);	break;
	  case 0x315: chart->value[0].field=get_sb_value(w);		break;
	  case 0x316: chart->value[0].mul= get_dsb_value(w);		break;
	  case 0x317: chart->value[0].add= get_dsb_value(w);		break;

	  case 0x320: chart->value[1].mode = CC_NEXT;			break;
	  case 0x321: chart->value[1].mode = CC_SAME;			break;
	  case 0x322: chart->value[1].mode = CC_EXPR;			break;
	  case 0x323: chart->value[1].mode = CC_DRAG;			break;
	  case 0x324: (void)read_text_button(w, &chart->value[1].expr);	break;
	  case 0x325: chart->value[1].field=get_sb_value(w);		break;
	  case 0x326: chart->value[1].mul= get_dsb_value(w);		break;
	  case 0x327: chart->value[1].add= get_dsb_value(w);		break;

	  case 0x332: chart->value[2].mode = CC_EXPR;			break;
	  case 0x334: (void)read_text_button(w, &chart->value[2].expr);	break;

	  case 0x342: chart->value[3].mode = CC_EXPR;			break;
	  case 0x343: chart->value[3].mode = CC_DRAG;			break;
	  case 0x344: (void)read_text_button(w, &chart->value[3].expr);	break;
	  case 0x345: chart->value[3].field=get_sb_value(w);		break;
	  case 0x346: chart->value[3].mul= get_dsb_value(w);		break;
	  case 0x347: chart->value[3].add= get_dsb_value(w);		break;
	}

	/*
	 * the chart choice buttons are widely separated and must be handled
	 * by hand here, to make sure only one of each group is enabled.
	 */
	i = tp->code & 0x00f;
	if (tp->code >= 0x310 && tp->code <= 0x3ff && i < 4) {
		i = tp->code & 0x3f0;
		fillout_formedit_widget_by_code(i + 0);
		fillout_formedit_widget_by_code(i + 1);
		fillout_formedit_widget_by_code(i + 2);
		fillout_formedit_widget_by_code(i + 3);
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
				ip->search   = item->search;
				ip->rdonly   = item->rdonly;
				ip->nosort   = item->nosort;
				ip->defsort  = item->defsort;
	  			ip->sumcol   = item->sumcol;
				ip->sumwidth = item->sumwidth;
				ip->column   = item->column;
				if (item->idefault) {
					zfree(ip->idefault);
	  				ip->idefault =mystrdup(item->idefault);
				}
				redraw_canvas_item(ip);
			}
		}
	return(all ? 2 : 1);
}
