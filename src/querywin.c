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
static void delete_callback(void);
static void dupl_callback  (void);
static void done_callback  (void);
static void list_callback  (QWidget *w, int x, int y, bool checked = false);

static FORM		*form;		/* form whose queries are edited */
static BOOL		have_shell = FALSE;	/* message popup exists if TRUE */
static QDialog		*shell;		/* popup menu shell */
static QWidget		*del, *dupl;	/* buttons, for desensitizing */
static QTextEdit	*info;		/* info line for error messages */
static QTableWidget	*qlist;		/* query list table widget */



/*
 * add a query entry. This is used when a new entry is started by pressing on
 * an empty row, or when duplicating an entry, or when a form file is read.
 */

DQUERY *add_dquery(
	FORM		*fp)		/* form to add blank entry to */
{
	int n = ++fp->nqueries * sizeof(DQUERY);
	if (!(fp->query = (DQUERY *)(fp->query ? realloc((char *)fp->query, n)
					       : malloc(n))))
		fatal("no memory for query");
	memset((void *)&fp->query[fp->nqueries-1], 0, sizeof(DQUERY));
	return(&fp->query[fp->nqueries-1]);
}


/*
 * destroy the popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_query_window(void)
{
	if (have_shell) {
		have_shell = FALSE;
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
	if (have_shell)
		return;
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

	w = del = mk_button(bb, "Delete", dbbr(Action));
	w->setEnabled(false);
	set_button_cb(w, delete_callback());
	bind_help(w, "dq_delete");

	w = dupl = mk_button(bb, "Duplicate", dbbr(Action));
	w->setEnabled(false);
	set_button_cb(w, dupl_callback());
	bind_help(w, "dq_dupl");

	w = mk_button(bb, "Done", dbbr(Accept));
	dynamic_cast<QPushButton *>(w)->setDefault(true);
	set_button_cb(w, done_callback());
	bind_help(w, "dq_done");
	
	w = mk_button(bb, 0, dbbb(Help));
	set_button_cb(w, help_callback(w, "queries"));
	// bind_help(w, "queries");  // same as main window

							/*-- infotext -- */
	info = new QTextEdit();
	info->setLineWrapMode(QTextEdit::NoWrap);
	info->setReadOnly(true);
	// QSS doesn't support :read-only for QTextEdit
	info->setProperty("readOnly", true);
	info->ensurePolished();
	info->setMinimumHeight(info->fontMetrics().height() * 2);
	wform->addWidget(info);

							/*-- scroll --*/

	w = qlist = new QTableWidget(3, NCOLUMNS);
	qlist->setShowGrid(false);
	qlist->setWordWrap(false);
	qlist->horizontalHeader()->setStretchLastSection(true);
	qlist->verticalHeader()->hide();
	w->setMinimumSize(580, 300);
	wform->addWidget(w);
	wform->addWidget(bb);
	// bind_help(w, "queries");  // same as main window

	create_query_rows();	/* have_shell must be FALSE here */
	print_query_info();

	popup_nonmodal(shell);
	set_dialog_cancel_cb(shell, done_callback());
	have_shell = TRUE;
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
	int x;
	register DQUERY	*dq = &form->query[y];
	bool blank = y >= form->nqueries;

	QCheckBox *cb = new QCheckBox;
	if (!blank)
		cb->setCheckState(dq->suspended ? Qt::Unchecked : Qt::Checked);
	qlist->setCellWidget(y, 0, cb);
	QRadioButton *rb = new QRadioButton;
	if (!blank)
		rb->setChecked(y == form->autoquery);
	qlist->setCellWidget(y, 1, rb);
	QLineEdit *t = new QLineEdit;
	if (!blank && dq->name)
		t->setText(dq->name);
	qlist->setCellWidget(y, 2, t);
	t = new QLineEdit;
	if (!blank && dq->query)
		t->setText(dq->query);
	qlist->setCellWidget(y, 3, t);

	for(x = 0; x < NCOLUMNS; x++) {
		QWidget *w = qlist->cellWidget(y, x);
		if(x > 1)
			set_text_cb(w, list_callback(w, x, y));
		else
			set_button_cb(w, list_callback(w, x, y, c), bool c);
		// bind_help(w, "queries"); // same as main window
	}
}

static void del_query(int y)
{
	int n;

	form->nqueries--;
	if(form->query[y].name)
		free(form->query[y].name);
	if(form->query[y].query)
		free(form->query[y].query);
	for (n=y; n < form->nqueries; n++)
		form->query[n] = form->query[n+1];
	if (y == form->autoquery)
		form->autoquery = -1;
	else if (y < form->autoquery)
		--form->autoquery;
}

static bool remove_if_blank(int y)
{
	if(y == form->autoquery ||
	   (form->query[y].name && form->query[y].name[0]) ||
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

void print_query_info(void)
{
	char		msg[4096];	/* message buffer */
	int		i, j, n;	/* index to next free char in msg */
	char		comma = ' ';	/* delimiter between message items */
	int		item;		/* item (field) counter */
	ITEM		*ip;		/* current item (field) */

	strcpy(msg, "Fields:");
	i = j = strlen(msg);
	for (item=0; item < form->nitems; item++) {
		ip = form->items[item];
		switch(ip->type) {
		  case IT_INPUT:
		  case IT_TIME:
		  case IT_NOTE:
			sprintf(msg+i, "%c %s", comma, ip->name);
			break;

		  case IT_FLAG:
		  case IT_CHOICE:
			sprintf(msg+i, "%c %s=%s", comma, ip->name,
							  ip->flagcode);
			break;

		  default:
			continue;
		}
		comma = ',';
		n = strlen(msg+i);
		i += n;
		j += n;
		if (j > 60) {
			comma = '\n';
			j = 0;
		}
		if (i > (int)sizeof(msg)-100)
			break;
	}
	if (comma == ' ')
		strcpy(msg+i, "none");
	print_text_button(info, msg);
}



/*-------------------------------------------------- callbacks --------------*/
/*
 * Delete, Duplicate, and Done buttons.
 * All of these routines are direct X callbacks.
 */

static void delete_callback(void)
{
	int				ycurr = qlist->currentRow();

	if (ycurr >= 0 && ycurr < form->nqueries) {
		qlist->removeRow(ycurr);
		del_query(ycurr);
	}
	if (!form->nqueries) {
		del->setEnabled(false);
		dupl->setEnabled(false);
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
		qlist->insertRow(ycurr);
		set_row_widgets(ycurr);
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
	QWidget				*w,
	int				x,
	int				y,
	bool				checked)
{
	int				i;
	register DQUERY	*dq = &form->query[y];
	char *string = 0;
	bool onblank = y >= form->nqueries, wasblank = onblank;

	if(x > 1) {
		const QString &qs = dynamic_cast<QLineEdit *>(w)->text();
		if(qs.size())
			string = qstrdup(qs);
	}
	if (onblank && (string || (x == 1 && checked))) {
		(void)add_dquery(form);
		dq = &form->query[y];
		onblank = false;
	}
	switch(x) {
	    case 0:
		if(onblank)
			dynamic_cast<QCheckBox *>(w)->setCheckState(Qt::Unchecked);
		else
			dq->suspended = !checked;
		break;
	    case 1:
		if(checked && y != (i = form->autoquery)) {
			form->autoquery = y;
			if(remove_if_blank(i)) {
				if(i < y)
					y--;
			} else
				dynamic_cast<QRadioButton *>(qlist->cellWidget(i, x))->setChecked(false);
		}
		break;
	    case 2:					/* name */
		if (!onblank) {
			if (dq->name)
				free(dq->name);
			dq->name = string;
		}
		break;
	    case 3:					/* query expr */
		if (!onblank) {
			if (dq->query)
				free(dq->query);
			dq->query = string;
		}
		break;
	}
	if (!onblank && !(onblank = remove_if_blank(y)) && wasblank) {
		qlist->setRowCount(form->nqueries + 1);
		set_row_widgets(form->nqueries);
	}

	// FIXME:  this needs to be in a click-callback as well
	del->setEnabled(!onblank);
	dupl->setEnabled(!onblank);
}
