/*
 * Create and destroy the export popup, and call the template functions.
 * The configuration is stored in the pref structure, but it can be changed
 * only here.
 *
 *	destroy_templ_popup()
 *	create_templ_popup()
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define NLINES	15		/* number of lines in list widget */

static void button_callback(int code);
static void file_export_callback(const QString &file);

static void mklist(void);
static void editfile(char *);
static void askname(BOOL);

static BOOL	have_shell = FALSE;	/* message popup exists if TRUE */
static QDialog	*shell;		/* popup menu shell */
static QListWidget	*list;		/* template list widget */
static int	list_nlines;	/* # of lines displayed in scroll list */
static BOOL	modified;	/* preferences have changed */


/*
 * destroy popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_templ_popup(void)
{
	if (have_shell) {
		if (modified)
			write_preferences();
		have_shell = FALSE;
		shell->close();
		delete shell;
	}
}


/*
 * create a print popup as a separate application shell. The popup is
 * initialized with data from pref. The menu array is the template for
 * generating the upper part of the popup window. The code is used to
 * identify buttons in callbacks. Code bits 4..7 are used to group
 * one-of-many choices.
 */

static struct menu {
	char	type;		/* Label, Scroll, b/q/Button, Text, -line */
	long	code;		/* unique identifier, 0=none */
	int 	role;
	const char *text;
	QWidget	*widget;
} menu[] = {
	{ 'L',	0,	dbbr(Invalid), "Template:"		},
	{ 'S',	0x10,	dbbr(Invalid), "scroll",		},
	{ 'B',	0x20,	dbbr(Action),  "Create"		},
	{ 'b',	0x21,	dbbr(Action),  "Dup"		},
	{ 'b',	0x22,	dbbr(Action),  "Edit"		},
	{ 'q',	0x23,	dbbr(Action),  "Delete"		},
	{ '-',	0,	dbbr(Invalid), "div"		},
	{ 'L',	0,	dbbr(Invalid), "Output file:"	},
	{ 'T',	0x30,	dbbr(Invalid), "text",		},
	{ 'B',	0x40,	dbbr(Action),  "Browse"		},
	{ 'b',	0x41,	dbbr(Accept),  "Export"		},
	{ 'b',	0x42,	dbbb(Cancel),  0		},
	{ 'q',	0x43,	dbbb(Help),    0		},
	{  0,   0,	dbbr(Invalid), 0			}
};

void create_templ_popup(void)
{
	struct menu	*mp;			/* current menu[] entry */
	QWidget		*w=0;
	QBoxLayout	*form;
	QDialogButtonBox *hb = 0;

	destroy_templ_popup();

	if (!curr_card) {
		create_error_popup(mainwindow, 0, "Select a database to export");
		return;
	}

	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new QDialog;
	shell->setWindowTitle("Grok Export");
	set_icon(shell, 1);
	form = new QVBoxLayout(shell);
	add_layout_qss(form, "exportform");
    	/* I could use from->setContentsMargins() to 16 but I'll leave at default */
	/* Same applies to setSpacing() */
	bind_help(shell, "tempname");

	for (mp=menu; mp->type; mp++) {
	    switch(mp->type) {
	      case 'B': hb = new QDialogButtonBox; FALLTHROUGH
	      case 'b':
	      case 'q': {
		      w = mk_button(hb, mp->text, mp->role);
		      if(mp->role == dbbr(Accept))
			      dynamic_cast<QPushButton *>(w)->setDefault(true);
		      break;
	      }
	      case 'T': w = new QLineEdit; w->setObjectName(mp->text);	break;
	      case '-': w = mk_separator();		break;
	      case 'L':	w = new QLabel(mp->text);	break;
	      case 'S': {
		  w = list = new QListWidget;
		  list->setSelectionMode(QAbstractItemView::SingleSelection);
		  // list->setAttribute("courierFont", true); // This isn't necessary
		  list->addItem("QT is a pain in the ass");
		  list->setMinimumHeight(list->sizeHintForRow(0) * NLINES);
		  list->clear();
		  list_nlines = 0;
		  mklist();
		  break;
	      }
	    }
	    if (!hb && w)
		form->addWidget(w);
	    if (mp->type == 'b' || mp->type == 'q')
		form->addWidget(hb);
	    if (mp->type == 'q')
		hb = 0;
	    if (mp->type == 'T' && pref.xfile)
		print_text_button(w, pref.xfile);

	    if (mp->type == 'b' || mp->type == 'q' || mp->type == 'B')
		set_button_cb(w, button_callback(mp->code));
	    if (mp->type == 'T')
		set_text_cb(w, button_callback(mp->code));
	    mp->widget = w;
	}
	// close does a reject by default, so no extra callback needed
	set_dialog_cancel_cb(shell, button_callback(0x42));

	popup_nonmodal(shell);
	have_shell = TRUE;
	modified = FALSE;
}


/*
 * put the current list of templates into the template list widget
 */

static void save_cb(UNUSED int seq, char *name)
{
	list->addItem(name);
	list_nlines++;
}

static void mklist(void)
{
	list->clear();
	list_nlines = 0;
	list_templates(save_cb, curr_card);
	if (pref.xlistpos >= list_nlines)
		pref.xlistpos = list_nlines-1;
	list->setCurrentRow(pref.xlistpos);
}


/*-------------------------------------------------- callbacks --------------*/


static int get_list_seq(void)
{
	int i = list->currentRow();
	if(i >= 0) {
		pref.xlistpos = i;
		return(TRUE);
	} else {
		create_error_popup(shell, 0, "Please choose a template name");
		return(FALSE);
	}
}


static BOOL do_export(void)
{
	struct menu	*mp;		/* for finding text widget */
	const char	*err;

	for (mp=menu; mp->code != 0x30; mp++);
	read_text_button_noblanks(mp->widget, &pref.xfile);
	if (!pref.xfile) {
		create_error_popup(shell,0,"Please enter an output file name");
		return(FALSE);
	}
	if (!get_list_seq())
		return(FALSE);

	if ((err = exec_template(pref.xfile, 0, pref.xlistpos, curr_card))) {
		create_error_popup(shell, 0, "Export failed:\n%s", err);
		return(FALSE);
	}
	modified = TRUE;
	return(TRUE);
}


static void button_callback(
	int				code)
{
	switch(code) {
	  case 0x20:						/* Create */
		askname(FALSE);
		break;
	  case 0x21:						/* Dup */
		if (!get_list_seq())
			return;
		askname(TRUE);
		break;
	  case 0x22:						/* Edit */
		if (!get_list_seq())
			break;
		if (pref.xlistpos >= get_template_nbuiltins()) {
			char *path = get_template_path(0,
					pref.xlistpos, curr_card);
			editfile(path);
			free(path);
		} else
			create_error_popup(shell, 0,
				"Cannot edit a builtin template, use Dup");
		break;
	  case 0x23:						/* Delete */
		if (!get_list_seq())
			return;
		(void)delete_template(shell, pref.xlistpos, curr_card);
		mklist();
		break;
	  case 0x40:						/* Browse */
	{
		QFileDialog *d = new QFileDialog(shell, "xfile");
		d->setAcceptMode(QFileDialog::AcceptSave);
		set_file_dialog_cb(d, file_export_callback(fn), fn);
		// close does a reject by default, so no extra callback needed
		d->exec();
		delete d;
		break;
	}
	  case 0x30:						/* text */
	  case 0x41:						/* Export */
		if (do_export())
	  case 0x42:						/* Cancel */
		destroy_templ_popup();
		break;
	  case 0x43:						/* Help */
		help_callback(shell, "export");
		break;
	}
}


/*-------------------------------- browse callbacks -------------------------*/

static void file_export_callback(
	const QString	&filename)
{
	struct menu			*mp;

	if (filename.size()) {
		if (pref.xfile)
			free(pref.xfile);
		pref.xfile = qstrdup(filename);
		for (mp=menu; mp->code != 0x30; mp++);
		print_text_button(mp->widget, pref.xfile);
	}
}


/*-------------------------------- ask for name -----------------------------*/
/*
 * user pressed Dup or Create. Ask for a new file name.
 */

static void text_callback	(void);
static void textcancel_callback	(void);

static BOOL		have_askshell = FALSE;	/* text popup exists if TRUE */
static QDialog		*askshell;	/* popup menu shell */
static QLineEdit	*text;		/* template name string */
static BOOL		duplicate;	/* dup file before editing */

static void askname(
	BOOL		dup)		/* duplicate file before editing */
{
	QWidget		*w;
	QBoxLayout	*form;
	QDialogButtonBox *b;

	duplicate = dup;
	if (have_askshell) {
		popup_nonmodal(askshell);
 		return;
	}
	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	askshell = new QDialog(shell);
	askshell->setWindowTitle("Template name");
	askshell->setObjectName("tempform"); // for style sheets

	set_icon(askshell, 1);
	bind_help(askshell, "tempname");

	form = new QVBoxLayout(askshell);
	add_layout_qss(form, "tempform");

	w = new QLabel("Name for new template:");
	form->addWidget(w);

	text = new QLineEdit;
	form->addWidget(text);
	set_text_cb(text, text_callback());

	b = new QDialogButtonBox;
	form->addWidget(b);

	w = mk_button(b, 0, dbbb(Cancel));
	set_button_cb(w, textcancel_callback());

	w = mk_button(b, 0, dbbb(Help));
	set_button_cb(w, help_callback(askshell, "tempname"));

	// tjm - added this button mostly for consistency
	w = mk_button(b, "Create", dbbr(Accept));
	dynamic_cast<QPushButton *>(w)->setDefault(true);
	set_button_cb(w, text_callback());

	// close does a reject by default, so no extra callback needed
	set_dialog_cancel_cb(shell, textcancel_callback());

	popup_nonmodal(askshell);
	have_askshell = TRUE;
}


static void textcancel_callback(void)
{
	if (have_askshell)
		delete askshell;
	have_askshell = FALSE;
}


static void text_callback(void)
{
	char				*name, *p;
	char				*string;

	string = qstrdup(text->text());
	for (name=string; *name == ' ' || *name == '\n'; name++);
	if (*name) {
		for (p=name; *p; p++)
			if (*p == '/' || *p == ' ' || *p == '\t')
				*p = '_';
		if (duplicate) {
			if (!(name = copy_template(askshell, name,
						   pref.xlistpos, curr_card)))
				return;
			mklist();
		} else
			name = get_template_path(name, 0, curr_card);

		editfile(name);
		free(name);
	}
	free(string);
	textcancel_callback();
}


/*-------------------------------- edit template file -----------------------*/
/*
 * user pressed Dup or Create. Ask for a new file name.
 */

static void editfile(
	char		*path)		/* path to edit */
{
	if (access(path, F_OK)) {
		FILE *fp = fopen(path, "w");
		fclose(fp);
		mklist();
	}
	edit_file(path, FALSE, TRUE, path, "tempedit");
}
