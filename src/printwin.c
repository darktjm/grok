/*
 * Create and destroy the print popup, and call the printing functions.
 * The configuration is stored in the pref structure, but it can be changed
 * only here.
 *
 *	destroy_print_popup()
 *	create_print_popup()
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void cancel_callback	(void);
static void print_callback	(void);
static void config_callback	(char *, char,bool);
static void file_print_callback	(QDialog *,const QString &);

static BOOL	have_shell = FALSE;	/* message popup exists if TRUE */
static QDialog	*shell;		/* popup menu shell */
static BOOL	modified;	/* preferences have changed */


/*
 * destroy popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_print_popup(void)
{
	if (have_shell) {
		have_shell = FALSE;
		if (modified)
			write_preferences();
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
	char	type;		/* L=label, R=RowColumn, C=choice, S=Spring */
	long	code;		/* unique identifier, 0=none */
	char	*ptr;		/* location in pref where value is stored */
	char	value;		/* value stored in pref for each mode */
	const char *text;
	QWidget	*widget;
} menu[] = {
	{ 'L',	0,	0,		  0,	"Cards to print:"	   },
	{ 'R',	0,	0,		  0,	"rc1",			   },
	{ 'C',	0x10,	&pref.pselect,	 'C',	"Current only"		   },
	{ 'C',	0x11,	&pref.pselect,	 'S',	"Search or query"	   },
	{ 'C',	0x12,	&pref.pselect,	 'e',	"All cards in section"	   },
	{ 'C',	0x13,	&pref.pselect,	 'A',	"All cards"		   },
	{ 'L',	0,	0,		  0,	"Output format:"	   },
	{ 'R',	0,	0,		  0,	"rc2",			   },
	{ 'C',	0x20,	&pref.pformat,	 'S',	"Summary"		   },
	{ 'C',	0x21,	&pref.pformat,	 'N',	"Summary with notes"	   },
	{ 'C',	0x22,	&pref.pformat,	 'C',	"Cards"			   },
	{ 'L',	0,	0,		  0,	"Output quality:"	   },
	{ 'R',	0,	0,		  0,	"rc3",			   },
	{ 'C',	0x30,	&pref.pquality,  'A',	"Low, ASCII only"	   },
	{ 'C',	0x31,	&pref.pquality,  'O',	"Medium, overstrike ASCII" },
/*	{ 'C',	0x32,	&pref.pquality,  'P',	"High, PostScript"	   },*/
	{ 'L',	0,	0,		  0,	"Output device:"	   },
	{ 'R',	0,	0,		  0,	"rc4",			   },
	{ 'C',	0x40,	&pref.pdevice,	 'P',	"Printer"		   },
	{ 'C',	0x41,	&pref.pdevice,	 'F',	"File"			   },
	{ 'C',	0x42,	&pref.pdevice,	 'W',	"Window"		   },
	{  0,   0,	0,		  0,	0			   }
};

void create_print_popup(void)
{
	struct menu	*mp;			/* current menu[] entry */
	QVBoxLayout	*form;
	QGridLayout	*g;
	QButtonGroup	*bg=0;
	QWidget		*w=0;
	QRadioButton	*b;
	int		span=1, row = 0;

	destroy_print_popup();

	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new QDialog;
	shell->setWindowTitle("Grok Print");
	set_icon(shell, 1);
	form = new QVBoxLayout(shell);
	add_layout_qss(form, "prefform");
	g = new QGridLayout;
	form->addLayout(g);
	bind_help(shell, "print");

	for (mp=menu; mp->type; mp++) {
		switch(mp->type) {
		    case 'L':
			w = new QLabel(mp->text);
			span = 2;
			break;
		    case 'R':
			bg = new QButtonGroup(shell);
			w = 0;
			break;
		    case 'C':
			w = b = new QRadioButton(mp->text);
			bg->addButton(b);
			b->setChecked(*mp->ptr==mp->value);
			span = 1;
			set_button_cb(b, config_callback(mp->ptr, mp->value, c), bool c);
			break;
		}
		mp->widget = w;
		if(w)
			g->addWidget(w, row++, span == 1, 1, span);
	}
							/*-- buttons --*/
	form->addWidget(mk_separator());

	QDialogButtonBox *bb = new QDialogButtonBox;
	form->addWidget(bb);

	w = mk_button(bb, "Print", dbbr(Accept));
	set_button_cb(w, print_callback());

	w = mk_button(bb, 0, dbbb(Cancel));
	set_button_cb(w, cancel_callback());

	w = mk_button(bb, 0, dbbb(Help));
	set_button_cb(w, help_callback(shell, "print"));

	set_dialog_cancel_cb(shell, cancel_callback());

	popup_nonmodal(shell);

	have_shell = TRUE;
	modified = FALSE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

static void cancel_callback(void)
{
	destroy_print_popup();
}


static void print_callback(void)
{
	if (pref.pdevice == 'F') {
		int ret;
		QFileDialog *d = new QFileDialog(shell, "pfile");
		d->setAcceptMode(QFileDialog::AcceptSave);
		set_file_dialog_cb(d, file_print_callback(d, fn), fn);
		ret = d->exec();
		delete d;
		if(ret == QDialog::Rejected)
			return;
	}
	print();
	destroy_print_popup();
}


static void file_print_callback(
	QDialog		*d,
	const QString	&filename)
{
	if (!filename.size()) {
		create_error_popup(shell, 0, "No file name, aborted.");
		return;
	}
	if (pref.pfile)
		free(pref.pfile);
	pref.pfile = qstrdup(filename);
}


static void config_callback(
	char				*ptr,
	char				value,
	bool				set)
{
	if (!set || !ptr)
		return;
	*ptr = value;
	modified = TRUE;
}
