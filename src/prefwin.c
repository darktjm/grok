/*
 * Create and destroy the preferences popup, and read and write the
 * preferences file. The printing configuration is also stored in the
 * pref structure, but the user interface for it is in printwin.c.
 *
 *	read_preferences()
 *	write_preferences()
 *	destroy_preference_popup()
 *	create_preference_popup()
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <float.h>
#include <unistd.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void done_callback (void);
static void spool_callback(void);

struct pref	pref;		/* global preferences */

static BOOL		have_shell = FALSE;	/* message popup exists if TRUE */
static QDialog		*shell;		/* popup menu shell */
static QLineEdit	*w_spoola;	/* ascii spool text */
static QLineEdit	*w_spoolp;	/* ascii spool text */
static QSpinBox		*w_linelen;	/* truncate printer lines */
static QSpinBox		*w_lines;	/* summary lines text */
static QDoubleSpinBox	*w_scale;	/* card scale text */
static BOOL		modified;	/* preferences have changed */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_preference_popup(void)
{
	if (have_shell) {
		int	i;
		double	d;
		char	*p;

		p = read_text_button(w_spoola, 0);
		if (*p && strcmp(p, pref.pspooler_a)) {
			free(pref.pspooler_a);
			pref.pspooler_a = mystrdup(p);
			modified = TRUE;
		}
		p = read_text_button(w_spoolp, 0);
		if (*p && strcmp(p, pref.pspooler_p)) {
			free(pref.pspooler_p);
			pref.pspooler_p = mystrdup(p);
			modified = TRUE;
		}
		i = w_linelen->value();
		if (i > 39 && i <= 250 && pref.linelen != i) {
			pref.linelen = i;
			modified = TRUE;
		}
		i = w_lines->value();
		if (i > 0 && i < 80 && pref.sumlines != i) {
			pref.sumlines = i;
			modified = TRUE;
		}
		// FIXME: since it's fp, it might alwasy seem modified
		d = w_scale->value();
		if (d >= 0.1 && d <= 10.0 && d != pref.scale) {
			pref.scale = d;
			modified = TRUE;
		}
		have_shell = FALSE;
		shell->close();
		delete shell;
		if (modified)
			write_preferences();
		modified = FALSE;
	}
}


/*
 * create a preference popup as a separate application shell. The popup is
 * initialized with data from pref.
 */

static const struct flag { BOOL *value; const char *text; } flags[] = {
	{ &pref.ampm,		"12 hour mode (am/pm)"			},
	{ &pref.mmddyy,		"Month/day/year mode"			},
	{ &pref.query2search,	"Show query search expressions"		},
	{ &pref.letters,	"Enable search by initial letter"	},
	{ &pref.allwords,	"Letter search checks all words"	},
	{ &pref.incremental,	"Incremental searches and queries"	},
	{ &pref.uniquedb,	"Don't show duplicate databases"	},
	{ 0,			0					}
};

static void flag_callback(const struct flag*, bool);

void create_preference_popup(void)
{
	QGridLayout		*form;
	const struct flag	*flag;
	int			row = 0;

	destroy_preference_popup();

	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new QDialog;
	shell->setWindowTitle("Grok Preferences");
	set_icon(shell, 1);
	form = new QGridLayout(shell);
	bind_help(shell, "pref");
							/*-- flags --*/
	for (flag=flags; flag->value; flag++) {
		QCheckBox *cb = new QCheckBox(flag->text);
		form->addWidget(cb, row++, 0, 1, 2);
		cb->setCheckState(*flag->value ? Qt::Checked : Qt::Unchecked);
		set_button_cb(cb, flag_callback(flag, c), bool c);
		// bind_help(cb, "pref"); // same as shell
	}


							/*-- print spooler --*/
	form->addWidget(mk_separator(), row++, 0, 1, 2);

	form->addWidget(new QLabel("PostScript print spooler:"), row, 0);
	form->addWidget(w_spoolp = new QLineEdit, row++, 1);
	w_spoolp->setMinimumWidth(200); // should be enough to set it once
	set_text_cb(w_spoolp, spool_callback());

	form->addWidget(new QLabel("ASCII print spooler:"), row, 0);
	form->addWidget(w_spoola = new QLineEdit, row++, 1);
	set_text_cb(w_spoola, spool_callback());

	form->addWidget(new QLabel("Printer line length:"), row, 0);
	form->addWidget(w_linelen = new QSpinBox, row++, 1);
	w_linelen->setRange(40, 250);
	set_spin_cb(w_linelen, spool_callback());

							/*-- nlines, scale --*/
	form->addWidget(mk_separator(), row++, 0, 1, 2);

	form->addWidget(new QLabel("Summary lines:"), row, 0);
	form->addWidget(w_lines = new QSpinBox, row++, 1);
	w_lines->setRange(1, 80);

	form->addWidget(new QLabel("Card scaling factor:"), row, 0);
	form->addWidget(w_scale = new QDoubleSpinBox, row++, 1);
	w_scale->setRange(0.1, 10.0);

							/*-- buttons --*/
	form->addWidget(mk_separator(), row++, 0, 1, 2);

	QDialogButtonBox *bb = new QDialogButtonBox;
	form->addWidget(bb, row++, 0, 1, 2);

	QPushButton *b = mk_button(bb, "Done", dbbr(Accept));
	set_button_cb(b, done_callback());
	bind_help(b, "pref_done");

	b = mk_button(bb, 0, dbbb(Help));
	set_button_cb(b, help_callback(shell, "pref"));
	// bind_help(b, "pref"); // same as shell

	set_dialog_cancel_cb(shell, done_callback());
	popup_nonmodal(shell);

	print_text_button_s(w_spoola,        pref.pspooler_a);
	print_text_button_s(w_spoolp,        pref.pspooler_p);
	w_linelen->setValue(pref.linelen);
	w_lines->setValue(pref.sumlines);
	w_scale->setValue(pref.scale);
	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

static void done_callback(void)
{
	destroy_preference_popup();
}


static void flag_callback(
	const struct flag		*flag,
	bool				set)
{
	*flag->value = set;
	if (flag->value == &pref.uniquedb)
		remake_dbase_pulldown();
	modified = TRUE;
}


static void spool_callback(void)
{
	int				i;

	(void)read_text_button(w_spoola, &pref.pspooler_a);
	(void)read_text_button(w_spoolp, &pref.pspooler_p);
	i = w_linelen->value();
	if (i > 39 && i <= 250)
		pref.linelen = i;
	modified = TRUE;
}


/*-------------------------------------------------- file i/o ---------------*/
/*
 * write pref struct to preferences file, used whenever popup is destroyed
 * if some button in it had been pressed
 */

void write_preferences(void)
{
	char		*path;		/* path of preferences file */
	FILE		*fp;		/* preferences file */

	path = resolve_tilde((char *)PREFFILE, 0); /* PREFFILE has no final / */
	if (!(fp = fopen(path, "w"))) {
		create_error_popup(mainwindow, errno,
			"Failed to write to preferences file\n%s", path);
		return;
	}
	fprintf(fp, "ampm	%s\n",	pref.ampm	  ? "yes" : "no");
	fprintf(fp, "mmddyy	%s\n",	pref.mmddyy	  ? "yes" : "no");
	fprintf(fp, "q2s	%s\n",	pref.query2search ? "yes" : "no");
	fprintf(fp, "letter	%s\n",	pref.letters	  ? "yes" : "no");
	fprintf(fp, "allword	%s\n",	pref.allwords	  ? "yes" : "no");
	fprintf(fp, "incr	%s\n",	pref.incremental  ? "yes" : "no");
	fprintf(fp, "unique	%s\n",	pref.uniquedb     ? "yes" : "no");
	fprintf(fp, "scale	%.*lg\n",	DBL_DIG + 1, pref.scale);
	fprintf(fp, "lines	%d\n",	pref.sumlines);
	fprintf(fp, "pselect	%c\n",	pref.pselect);
	fprintf(fp, "pformat	%c\n",	pref.pformat);
	fprintf(fp, "pqual	%c\n",	pref.pquality);
	fprintf(fp, "pdevice	%c\n",	pref.pdevice);
	fprintf(fp, "pspoola	%s\n",	pref.pspooler_a);
	fprintf(fp, "pspoolp	%s\n",	pref.pspooler_p);
	fprintf(fp, "pfile	%s\n",	STR(pref.pfile));
	fprintf(fp, "linelen	%d\n",	pref.linelen);
	fclose(fp);
}


/*
 * read preferences file into pref struct, done by main() when starting up
 */

void read_preferences(void)
{
	char		*path;		/* path of preferences file */
	FILE		*fp;		/* preferences file */
	char		line[1024];	/* line from file */
	char		*p;		/* for scanning line */
	char		*key;		/* first char of first word in line */
	int		value;		/* value of second word in line */

	pref.letters	= TRUE;
	pref.scale	= 1.0;
	pref.sumlines	= 8;
	pref.pselect	= 'S';
	pref.pformat	= 'S';
	pref.pquality	= 'A';
	pref.pdevice	= 'P';
	pref.pspooler_a	= mystrdup(PSPOOL_A);
	pref.pspooler_p	= mystrdup(PSPOOL_P);
	pref.linelen	= 79;

	path = resolve_tilde((char *)PREFFILE, 0); /* PREFFILE has no final / */
	if (!(fp = fopen(path, "r")))
		return;
	for (;;) {
		if (!fgets(line, 1023, fp))
			break;
		for (p = line; *p; ++p);
		if (*--p == '\n') *p = '\0';
		for (p=line; *p == ' ' || *p == '\t'; p++);
		if (*p == '#' || !*p)
			continue;
		for (key=p; *p && *p != ' ' && *p != '\t'; p++);
		if (*p) *p++ = 0;
		for (; *p == ' ' || *p == '\t'; p++);
		value = (*p&~32) == 'T' || (*p&~32) == 'Y' ? 1 : atoi(p);

		if (!strcmp(key, "ampm"))	pref.ampm	 = value;
		if (!strcmp(key, "mmddyy"))	pref.mmddyy	 = value;
		if (!strcmp(key, "q2s"))	pref.query2search= value;
		if (!strcmp(key, "letter"))	pref.letters	 = value;
		if (!strcmp(key, "allword"))	pref.allwords	 = value;
		if (!strcmp(key, "incr"))	pref.incremental = value;
		if (!strcmp(key, "unique"))	pref.uniquedb	 = value;
		if (!strcmp(key, "scale"))	pref.scale	 = atof(p);
		if (!strcmp(key, "lines"))	pref.sumlines	 = value;
		if (!strcmp(key, "pselect"))	pref.pselect	 = *p;
		if (!strcmp(key, "pformat"))	pref.pformat	 = *p;
		if (!strcmp(key, "pqual"))	pref.pquality	 = *p;
		if (!strcmp(key, "pdevice"))	pref.pdevice	 = *p;
		if (!strcmp(key, "pspoola"))	{ free(pref.pspooler_a); pref.pspooler_a	 = mystrdup(p); }
		if (!strcmp(key, "pspoolp"))	{ free(pref.pspooler_p); pref.pspooler_p	 = mystrdup(p); }
		if (!strcmp(key, "pfile"))	pref.pfile	 = mystrdup(p);
		if (!strcmp(key, "linelen"))	pref.linelen	 = atoi(p);
	}
	fclose(fp);
}
