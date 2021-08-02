/*
 * Create and destroy default query menu and all widgets in them. All widgets
 * are labels or pushbuttons; they are faster than text buttons. Whenever
 * the user presses a button with text in it, it is overlaid with a Text
 * button. The query menu is started from the form editor, default queries
 * are expressions attached to a form. They appear in the Query pulldown.
 *
 * tjm - As part of the Qt port, I have changed the behavior entirely.
 * The main area is a standard table widget with editable cells, which sort
 * of works the same way, but is managed by Qt instead of this code.  Besides,
 * any statement about speed is moot on modern systems, especially considering
 * that Qt is probably slower than Motif overall.  I also immediately prune
 * empty lines, and only provide one line for entry of new data.
 *
 *	add_dquery(form)		apend a query entry to the form's list
 *	destroy_query_window()		remove query popup
 *	create_query_window()		create query popup
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define NCOLUMNS	4		/* # of widget columns in query list */

static void create_query_rows(void);
static void move_callback  (int dir);
static void delete_callback(void);
static void dupl_callback  (void);
static void done_callback  (void);
static void list_callback  (int x, int y, bool checked = false);

static FORM		*form;		/* form whose queries are edited */
static bool		have_shell = false;	/* message popup exists if true */
static QDialog		*shell;		/* popup menu shell */
static QPushButton	*del, *dupl, *up, *down;	/* buttons, for desensitizing */
static QTextEdit	*info;		/* info line for error messages */
static QTableWidget	*qlist;		/* query list table widget */


/*
 * add a query entry. This is used when a new entry is started by pressing on
 * an empty row, or when duplicating an entry, or when a form file is read.
 */

DQUERY *add_dquery(
	FORM		*fp)		/* form to add blank entry to */
{
	zgrow(0, "query", DQUERY, fp->query, fp->nqueries, fp->nqueries + 1, 0);
	return(&fp->query[fp->nqueries++]);
}


/*
 * destroy the popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_query_window(void)
{
	if (have_shell) {
		have_shell = false;
		shell->close();
		delete shell;
	}
}


/*
 * create a query popup as a separate application shell.
 */

void create_query_window(
	FORM			*newform)	/*form whose queries are chgd*/
{
	QVBoxLayout		*wform;
	QWidget			*w;

	destroy_query_window();
	form = newform;
	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new QDialog;
	shell->setWindowTitle("Default Queries");
	set_icon(shell, 1);
	wform = new QVBoxLayout(shell);
	add_layout_qss(wform, "queryform");
	bind_help(shell, "queries");

							/*-- buttons --*/
	QDialogButtonBox *bb = new QDialogButtonBox;

	up = new QPushButton(QIcon::fromTheme("go-up"), "");
	bb->addButton(up, dbbr(Action));
	up->setEnabled(false);
	set_button_cb(up, move_callback(-1));
	bind_help(up, "dq_move");

	down = new QPushButton(QIcon::fromTheme("go-down"), "");
	bb->addButton(down, dbbr(Action));
	down->setEnabled(false);
	set_button_cb(down, move_callback(1));
	bind_help(down, "dq_move");

	w = del = mk_button(bb, "Delete", dbbr(Action));
	w->setEnabled(false);
	set_button_cb(w, delete_callback());
	bind_help(w, "dq_delete");

	w = dupl = mk_button(bb, "Duplicate", dbbr(Action));
	w->setEnabled(false);
	set_button_cb(w, dupl_callback());
	bind_help(w, "dq_dupl");

	w = mk_button(bb, "Done", dbbr(Accept));
	reinterpret_cast<QPushButton *>(w)->setDefault(true);
	set_button_cb(w, done_callback());
	bind_help(w, "dq_done");
	
	w = mk_button(bb, 0, dbbb(Help));
	set_button_cb(w, help_callback(w, "queries"));
	// bind_help(w, "queries");  // same as main window

							/*-- infotext -- */
	info = new QTextEdit();
	info->setLineWrapMode(QTextEdit::WidgetWidth);
	info->setReadOnly(true);
	// QSS doesn't support :read-only for QTextEdit
	info->setProperty("readOnly", true);
	info->ensurePolished();
	info->setMinimumHeight(info->fontMetrics().height() * 2);
	// Don't make the help window the initial kb focus
	info->setFocusPolicy(Qt::ClickFocus);
	wform->addWidget(info);

							/*-- scroll --*/

	w = qlist = new QTableWidget(3, NCOLUMNS);
	qlist->setShowGrid(false);
	qlist->setWordWrap(false);
#if 0  // this doesn't always activate when I expect
	qlist->setEditTriggers(QAbstractItemView::CurrentChanged|
			       QAbstractItemView::SelectedClicked);
#else // this doesn't, either, but it might get better
	qlist->setEditTriggers(QAbstractItemView::AllEditTriggers);
#endif
	qlist->setSelectionBehavior(QAbstractItemView::SelectRows);
	qlist->horizontalHeader()->setStretchLastSection(true);
	qlist->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
	qlist->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
	// qlist->setDragDropMode(QAbstractItemView::InternalMove);
	// qlist->setDropIndicatorShown(true);
	qlist->verticalHeader()->hide();
	w->setMinimumSize(580, 300);
	wform->addWidget(w);
	wform->addWidget(bb);
	// bind_help(w, "queries");  // same as main window

	create_query_rows();	/* have_shell must be false here */
	info->setPlainText(print_query_info(form));

	// This is only called after a cell has been edited
	// It takes the place of QLineEdit's set_text_cb().
	set_qt_cb(QTableWidget, cellChanged, qlist,
		  list_callback(c, r), int r, int c);
	// This is called on cursor/tab movement and clicking.
	// It is only used to update the buttons, so column is ignored.
	// That is, unless it is a blank row, where 1st 2 are skipped
	// The old GUI always skipped the 1st 2, but that disallows
	// keyboard toggling.
	set_qt_cb(QTableWidget, currentCellChanged, qlist,
		  list_callback(-1, r, c > 1), int r, int c);

	popup_nonmodal(shell);
	set_dialog_cancel_cb(shell, done_callback());
	have_shell = true;
}


/*
 * Initializes the table widget.  The old code just set up the widgets,
 * and had a separate function for filling in data.  I do it all here.
 * Also, I only have one spare row.  I also auto-delete blank rows, unless
 * they are set to being the default.
 */

// 1000 for the last widget was way too much, and scrolled to the right
static short cell_xs   [NCOLUMNS] = { 30, 30, 200, 200 };
static QStringList cell_name = { "on", "def", "Name", "Query Expression"};

static void set_row_widgets(int y)
{
	DQUERY	*dq = &form->query[y];
	bool blank = y >= form->nqueries;
	QTableWidgetItem *twi;

#if 0  /* this isn't just a checkbox -- it has an empty but visible string */
       /* Not only that, but tabbing into the table widget doesn't set the */
       /* selected/current row to 0 (but having a widget does... weird) */
	twi = new QTableWidgetItem;
	twi->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	twi->setCheckState(blank || dq->suspended ? Qt::Unchecked : Qt::Checked);
	qlist->setItem(y, 0, twi);
#else
	QCheckBox *cb = new QCheckBox;
	cb->setCheckState(blank || dq->suspended ? Qt::Unchecked : Qt::Checked);
	cb->setEnabled(!blank);
	qlist->setCellWidget(y, 0, cb);
	set_button_cb(cb, list_callback(0, y, c), bool c);
#endif
	/* Apparently radio selection is not possible */
	QRadioButton *rb = new QRadioButton;
	rb->setAutoExclusive(false);
	if (!blank)
		rb->setChecked(y == form->autoquery);
	rb->setEnabled(!blank);
	qlist->setCellWidget(y, 1, rb);
	set_button_cb(rb, list_callback(1, y, c), bool c);
	// The callback for twi is set in the list widget itself, above.
	twi = new QTableWidgetItem(!blank && dq->name ? dq->name : "");
	qlist->setItem(y, 2, twi);
	twi = new QTableWidgetItem(!blank && dq->query ? dq->query : "");
	qlist->setItem(y, 3, twi);
}

/*
 * <sigh> then again, moving queries calls for a function to just fill in.
 */

static void fill_row_widgets(int y)
{
	DQUERY	*dq = &form->query[y];
	bool blank = y >= form->nqueries;
	QAbstractButton *b;

#if 0
	qlist->item(y, 0)->setCheckState(blank || dq->suspended ? Qt::Unchecked : Qt::Checked);
#else
	b = reinterpret_cast<QAbstractButton *>(qlist->cellWidget(y, 0));
	b->setChecked(!blank && !dq->suspended);
	b->setEnabled(!blank);
#endif
	b = reinterpret_cast<QAbstractButton *>(qlist->cellWidget(y, 1));
	b->setChecked(!blank && y == form->autoquery);
	b->setEnabled(!blank);
	qlist->item(y, 2)->setText(!blank && dq->name ? dq->name : "");
	qlist->item(y, 3)->setText(!blank && dq->query ? dq->query : "");
}

static void del_query(int y)
{
	int n;

	form->nqueries--;
	zfree(form->query[y].name);
	zfree(form->query[y].query);
	for (n=y; n < form->nqueries; n++)
		form->query[n] = form->query[n+1];
	if (y == form->autoquery)
		form->autoquery = -1;
	else if (y < form->autoquery)
		--form->autoquery;
}

static bool remove_if_blank(int y)
{
	if((form->query[y].name && form->query[y].name[0]) ||
	   (form->query[y].query && form->query[y].query[0]))
		return false;
	del_query(y);
	qlist->removeRow(y);
	return true;
}

static void create_query_rows(void)
{
	int			x, y;

	qlist->setRowCount(form->nqueries + 1);
	// setting width here probably doesn't work right
	for (x=0; x < NCOLUMNS; x++)
		qlist->setColumnWidth(x, (int)cell_xs[x]);
	qlist->setHorizontalHeaderLabels(cell_name);
	for (y=0; y < form->nqueries; y++) {
		if(remove_if_blank(y)) {
			y--;
			continue;
		}
		set_row_widgets(y);
	}
	set_row_widgets(form->nqueries);

	qlist->update();
}

/*
 * print interesting info into the message line. This is done only once, when
 * the menu is created.
 * FIXME: make this a separate popup that can be called from help menu
 */

QString print_query_info(const FORM *form)
{
	QString		msg("Fields:");	/* message buffer */
	char		comma = ' ';	/* delimiter between message items */
	int		item;		/* item (field) counter */
	int		mi;		/* menu counter */
	ITEM		*ip;		/* current item (field) */
	char		sep, esc;

#define append_comma append(comma); msg.append(' ')

	get_form_arraysep(form, &sep, &esc);
	for (item=0, mi = -1; item < form->nitems; item++) {
		ip = form->items[item];
		switch(ip->type) {
		  case IT_INPUT:
		  case IT_TIME:
		  case IT_NOTE:
		  case IT_NUMBER:
		  case IT_FKEY:
			msg.append_comma;
			msg.append(ip->name);
			break;

		  case IT_FLAG:
		  case IT_CHOICE:
			msg.append_comma;
			msg.append(ip->name);
			msg.append('=');
			msg.append(ip->flagcode);
			break;

		  case IT_FLAGS:
		  case IT_MULTI:
			if(!IFL(ip->,MULTICOL)) {
				if(++mi == ip->nmenu) {
					mi = -1;
					continue;
				}
				msg.append_comma;
				msg.append(ip->name);
				msg.append('=');
				msg.append(ip->menu[mi].flagcode);
				--item;
				break;
			}
			FALLTHROUGH
		  case IT_MENU:
		  case IT_RADIO:
			if(++mi == ip->nmenu) {
				mi = -1;
				continue;
			}
			msg.append_comma;
			msg.append(IFL(ip->,MULTICOL) ? ip->menu[mi].name : ip->name);
			msg.append('=');
			msg.append(ip->menu[mi].flagcode);
			--item;
			break;

		  default:
			continue;
		}
		comma = ',';
	}
	if (comma == ' ')
		msg.append("none");
	return msg;
}



/*-------------------------------------------------- callbacks --------------*/
/*
 * Move, Delete, Duplicate, and Done buttons.
 * All of these routines are direct X callbacks.
 */

static void move_callback  (int dir)
{
	int	ycurr = qlist->currentRow(), xcurr = qlist->currentColumn();
	DQUERY	tmp; 

	if (ycurr < 0 || ycurr + dir < 0 ||
	    ycurr >= form->nqueries || ycurr + dir >= form->nqueries)
		return;
	tmp = form->query[ycurr];
	form->query[ycurr] = form->query[ycurr + dir];
	form->query[ycurr + dir] = tmp;
	if(form->autoquery == ycurr)
		form->autoquery += dir;
	else if(form->autoquery == ycurr + dir)
		form->autoquery -= dir;
	QSignalBlocker sb(qlist);
	fill_row_widgets(ycurr);
	fill_row_widgets(ycurr += dir);
	qlist->setCurrentCell(ycurr, xcurr);
	up->setEnabled(ycurr > 0);
	down->setEnabled(ycurr < form->nqueries - 1);
}

static void delete_callback(void)
{
	int				ycurr = qlist->currentRow();

	if (ycurr >= 0 && ycurr < form->nqueries) {
		del_query(ycurr);
		QSignalBlocker sb(qlist);
		qlist->removeRow(ycurr);
	}
	if (!form->nqueries) {
		del->setEnabled(false);
		dupl->setEnabled(false);
		up->setEnabled(false);
		down->setEnabled(false);
	} else {
		up->setEnabled(ycurr > 0);
		down->setEnabled(ycurr < form->nqueries - 1);
	}
}


static void dupl_callback(void)
{
	int				ycurr = qlist->currentRow(), n;

	if (form->nqueries) {
		(void)add_dquery(form);
		for (n=form->nqueries-1; n > ycurr; n--)
			form->query[n] = form->query[n-1];
		form->query[n].name  = mystrdup(form->query[n].name);
		form->query[n].query = mystrdup(form->query[n].query);
		QSignalBlocker sb(qlist);
		qlist->insertRow(ycurr);
		set_row_widgets(ycurr);
		up->setEnabled(true);
	}
}

static void done_callback(void)
{
	destroy_query_window();
	remake_query_pulldown();
}


/*
 * one of the buttons in the list was pressed
 */

static void list_callback(
	int				x,
	int				y,
	bool				checked)
{
	DQUERY	*dq = &form->query[y];
	char *string = 0;
	bool onblank = y >= form->nqueries, wasblank = onblank, oy;
	QAbstractButton *b;

	if(x > 1) {
		QTableWidgetItem *twi = qlist->item(y, x);
		const QString &qs = twi->text();
		if(qs.size())
			string = qstrdup(qs);
	}
	if (onblank && string) {
		b = reinterpret_cast<QAbstractButton *>(qlist->cellWidget(y, 0));
		QSignalBlocker sb(b);
		b->setChecked(true);
		b->setEnabled(true);
		qlist->cellWidget(y, 1)->setEnabled(true);
		(void)add_dquery(form);
		dq = &form->query[y];
		dq->suspended = false;
		onblank = false;
	} else 	if(x >= 0 && x < 2)
		b = reinterpret_cast<QAbstractButton *>(qlist->cellWidget(y, x));
	switch(x) {
	    case 0:
		if(onblank && checked) {
			QSignalBlocker sb(b);
			b->setChecked(false);
		} else if(!onblank)
			dq->suspended = !checked;
		break;
	    case 1:
		if(!checked)
			break;
		if(onblank) {
			QSignalBlocker sb(b);
			b->setChecked(false);
			break;
		}
		if(y == form->autoquery) {
			form->autoquery = -1;
			QSignalBlocker sb(b);
			b->setChecked(false);
		} else {
			if(form->autoquery >= 0) {
				b = reinterpret_cast<QAbstractButton *>(
					qlist->cellWidget(form->autoquery, x));
				QSignalBlocker sb(b);
				b->setChecked(false);
			}
			form->autoquery = y;
		}
		break;
	    case 2:					/* name */
		if (!onblank) {
			zfree(dq->name);
			dq->name = string;
		}
		break;
	    case 3:					/* query expr */
		if (!onblank) {
			zfree(dq->query);
			dq->query = string;
		}
		break;
	}
	oy = qlist->currentRow();
	if (!onblank && !(onblank = remove_if_blank(y)) && wasblank) {
		QSignalBlocker sb(qlist);
		qlist->setRowCount(form->nqueries + 1);
		set_row_widgets(form->nqueries);
	}
	// skip 1st 2 fields when tabbing into blank line
	// also, if removing a newly blank line, select the 1st text field.
	if((onblank && ((x < 0 && !checked) || !x || x == 1)) ||
	   (onblank && !wasblank && oy == y)) {
		QSignalBlocker sb(qlist);
		qlist->setCurrentCell(y, 2);
		qlist->editItem(qlist->item(y, 2));
	}
	del->setEnabled(!onblank);
	dupl->setEnabled(!onblank);
	up->setEnabled(!onblank && y > 0);
	down->setEnabled(!onblank && y < form->nqueries - 1);
}
