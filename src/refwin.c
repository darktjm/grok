/*
 * Create and destroy referer pop; modeled after query popup, but much
 * simpler
 *
 *	destroy_ref_window()		remove referer popup
 *	create_ref_window()		create referer popup
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void create_refby_rows(void);
static void delete_callback(void);
static void done_callback  (void);
static void list_callback  (int y, const QString &sel);

static FORM		*form;		/* form whose refbys are edited */
static bool		have_shell = false;	/* message popup exists if true */
static QDialog		*shell;		/* popup menu shell */
static QPushButton	*del;		/* buttons, for desensitizing */
static QListWidget	*rlist;		/* refby list table widget */


/*
 * destroy the popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_refby_window(void)
{
	if (have_shell) {
		have_shell = false;
		shell->close();
		delete shell;
	}
}


/*
 * create a refby popup as a separate application shell.
 */

void create_refby_window(
	FORM			*newform)	/*form whose refbys are chgd*/
{
	QVBoxLayout		*wform;
	QWidget			*w;

	destroy_refby_window();
	form = newform;
	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway
	shell = new QDialog;
	shell->setWindowTitle("Databases Referring To This");
	set_icon(shell, 1);
	wform = new QVBoxLayout(shell);
	add_layout_qss(wform, "refbyform");
	bind_help(shell, "refby");

							/*-- buttons --*/
	QDialogButtonBox *bb = new QDialogButtonBox;

	w = del = mk_button(bb, "Delete", dbbr(Action));
	w->setEnabled(false);
	set_button_cb(w, delete_callback());
	bind_help(w, "dq_delete");

	w = mk_button(bb, "Done", dbbr(Accept));
	reinterpret_cast<QPushButton *>(w)->setDefault(true);
	set_button_cb(w, done_callback());
	bind_help(w, "dq_done");
	
	w = mk_button(bb, 0, dbbb(Help));
	set_button_cb(w, help_callback(w, "refby"));
	// bind_help(w, "refby");  // same as main window

							/*-- scroll --*/

	w = rlist = new QListWidget();
	rlist->setSelectionMode(QAbstractItemView::SingleSelection);
	// rlist->setDragDropMode(QAbstractItemView::InternalMove);
	// rlist->setDropIndicatorShown(true);
	w->setMinimumSize(580, 300);
	wform->addWidget(w);
	wform->addWidget(bb);
	// bind_help(w, "refby");  // same as main window

	create_refby_rows();	/* have_shell must be false here */

	popup_nonmodal(shell);
	set_dialog_cancel_cb(shell, done_callback());
	have_shell = true;
}


/*
 * Initializes the list widget.  The old code just set up the widgets,
 * and had a separate function for filling in data.  I do it all here.
 * Also, I only have one spare row.  I also auto-delete blank rows.
 */

static void set_row_widgets(int y)
{
	QComboBox *cb = new QComboBox;
	QStringList sl;
	add_dbase_list(sl);
	cb->addItem("");
	cb->addItems(sl);
	cb->setCurrentText(y >= form->nchild ? "" : form->children[y]);
	set_combo_cb(cb, list_callback(y, sel), const QString &sel);
	rlist->setItemWidget(rlist->item(y), cb);
}

static void del_refby(int y)
{
	int n;

	form->nchild--;
	zfree(form->children[y]);
	for (n=y; n < form->nchild; n++)
		form->children[n] = form->children[n+1];
}

static bool remove_if_blank(int y)
{
	if(form->children[y] && *form->children[y])
		return false;
	del_refby(y);
	delete rlist->item(y);
	return true;
}

static void create_refby_rows(void)
{
	int			y;

	rlist->clear();
	for (y=0; y < form->nchild; y++) {
		if(remove_if_blank(y)) {
			y--;
			continue;
		}
		rlist->addItem("");
		set_row_widgets(y);
	}
	rlist->addItem("");
	set_row_widgets(form->nchild);

	rlist->update();
}

/*-------------------------------------------------- callbacks --------------*/
/*
 * Delete and Done buttons.
 */

static void delete_callback(void)
{
	int				ycurr = rlist->currentRow();

	if (ycurr >= 0 && ycurr < form->nchild) {
		del_refby(ycurr);
		QSignalBlocker sb(rlist);
		rlist->removeItemWidget(rlist->item(ycurr));
	}
	if (!form->nchild)
		del->setEnabled(false);
}


static void done_callback(void)
{
	destroy_refby_window();
}


/*
 * one of the buttons in the list was pressed
 */

static void list_callback(
	int				y,
	const QString			&sel)
{
	char **ch = &form->children[y];
	bool onblank = y >= form->nchild, wasblank = onblank;

	if (onblank && !sel.isEmpty()) {
		zgrow(0, "children", char *, form->children, form->nchild,
		      form->nchild + 1, 0);
		form->nchild++;
		ch = &form->children[y];
		onblank = false;
	}
	if (!onblank) {
		zfree(*ch);
		*ch = qstrdup(sel);
	}
	if (!onblank && !(onblank = remove_if_blank(y)) && wasblank) {
		QSignalBlocker sb(rlist);
		rlist->addItem("");
		set_row_widgets(form->nchild);
	}
	del->setEnabled(!onblank);
}
