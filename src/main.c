/*
 * Initializes everything and starts a calendar window for the current
 * month. The interval timer (for autosave and entry recycling) and a
 * few routines used by everyone are also here.
 *
 *	main(argc, argv)		Guess.
 *
 * Author: thomas@bitrot.de (Thomas Driemeyer)
 * Major changes by Thomas J. Moore <darktjm@gmail.com>; see HISTORY
 */

#include "config.h"
#include <unistd.h>
#include <locale.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "version.h"

static void usage(void), make_grokdir(void);
static void init_pixmaps(void);

QApplication		*app;		/* application handle */
const char		*progname;	/* argv[0] */
QIcon			pixmap[NPICS];	/* common symbols */
bool			restricted;	/* restricted mode, no form editor */


static const char * const default_qss =
#include "resource.h"
;


/*
 * initialize everything and create the main calendar window
 */

int main(
	int		argc,
	char		*argv[])
{
	int		n;
	CARD		*card = 0;
	char		*formname = 0;
	char		*tmpl = 0;
	char		*query	  = 0;
	bool		nofork	  = false;
	bool		ttymode	  = false;
	bool		planmode  = false;
	bool		noheader  = false;
	bool		do_export = false;
	bool		do_pr_tmpl = false;
	bool		bad_args = false;

	setlocale(LC_ALL, "");
	if ((progname = strrchr(argv[0], '/')) && progname[1])
		progname++;
	else
		progname = argv[0];
	restricted = !strcmp(progname, "rgrok");

	for (n=1; n < argc; n++) {			/* options */
		if (*argv[n] != '-') {
			if (!formname)
				formname = argv[n];
			else if ((do_export || do_pr_tmpl) && !tmpl)
				tmpl = argv[n];
			else if (!query)
				query = argv[n];
			else
				bad_args = true;
		} else if (!argv[n][2])
			switch(argv[n][1]) {
			  case 'd':
				fputs(default_qss, stdout);
				return(0);
			  case 'v':
				fprintf(stderr, "%s: " GROK_VERSION "\n", progname);
				return(0);
			  case 'f':
				nofork   = true;
				break;
			  case 'T':
				noheader = true;
				FALLTHROUGH
			  case 't':
				ttymode  = true;
				break;
			  case 'p':
				planmode = true;
				noheader = true;
				break;
			  case 'x':
				if(do_pr_tmpl)
					usage();
				do_export   = true;
				break;
			  case 'X':
				if(do_export)
					usage();
				do_pr_tmpl = true;
				break;
			  case 'r':
				restricted = true;
				break;
			  default:
				usage();
			}
		else
			bad_args = true;
	}

	/* only allow qt args in interactive mode */
	if(bad_args && (ttymode || planmode || do_export || do_pr_tmpl))
		usage();

	(void)umask(0077);
	tzset();
	if (ttymode) {
		char *buf = NULL;
		size_t buf_len;
		if (!formname)
			usage();
		read_preferences();
		switch_form(card, formname);
		if (!card || !card->form || !card->form->dbase ||!card->form->dbase->nrows){
			fprintf(stderr, "%s: %s: no data\n",progname,formname);
			exit(0);
		}
		query_any(SM_SEARCH, card, query);
		if (!card->nquery) {
			fprintf(stderr,"%s: %s: no match\n",progname,formname);
			exit(0);
		}
		if (!noheader) {
			char *p;
			make_summary_line(&buf, &buf_len, card, -1);
			puts(buf);
			for (p=buf; *p; p++)
				*p = '-';
			puts(buf);
		}
		for (n=0; n < card->nquery; n++) {
			make_summary_line(&buf, &buf_len, card, card->query[n]);
			puts(buf);
		}
		zfree(buf);
		fflush(stdout);
		exit(0);
	}
	if (planmode) {
		if (!formname)
			usage();
		read_preferences();
		switch_form(card, formname);
		if (!card || !card->form || !card->form->dbase ||!card->form->dbase->nrows)
			exit(0);
		query_any(SM_SEARCH, card, query ?
					query : card->form->planquery);
		for (n=0; n < card->nquery; n++)
			make_plan_line(card, card->query[n]);
		fflush(stdout);
		exit(0);
	}
	if (do_export || do_pr_tmpl) {
		const char *p;
		if (!formname || !tmpl)
			usage();
		read_preferences();
		switch_form(card, formname);
		if (!card || !card->form || !card->form->dbase ||!card->form->dbase->nrows){
			fprintf(stderr, "%s: %s: no data\n",progname,formname);
			exit(0);
		}
		query_any(SM_SEARCH, card, query);
		if (!card->nquery) {
			fprintf(stderr,"%s: %s: no match\n",progname,formname);
			exit(0);
		}
		if ((p = exec_template(0, 0, tmpl, 0, 0, card, do_pr_tmpl)))
				fprintf(stderr, "%s %s: %s\n", progname, formname, p);
		fflush(stdout);
		exit(0);
	}
	if (!nofork) {				/* background */
		long pid = fork();
		if (pid < 0)
			perror("can't fork");
		else if (pid > 0)
			exit(0);
	}
	app = new QApplication(argc, argv);
	/* qt has removed args it understands, so recheck args */
	formname = tmpl = query = 0;
	for (n=1; n < argc; n++) {
		if (*argv[n] != '-') {
			if (!formname)
				formname = argv[n];
			else if (!query)
				query = argv[n];
			else
				usage();
		} else if (argv[n][2])
			usage();
	}
	// propagate style sheet prefs down the widget tree
	QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, true);
	// Qt Style Sheets are a poor substitute for X resources
	// But, to make this as much like the old app as possible, I'll
	// at least make them a little more usable:
	// There is only one storage location for application-wide defaults,
	// so calling app->setStyleSheet() overrides the command-line setting
	// Instead, I append the command-line setting, so it overrides
	// defaults.

	// First, the default qss is loaded.
	QString qss(default_qss);
	// Then, the config path is searched for a resource file
	// This replaces the defaults, so the user can run without any styling
	const char *rname = resolve_tilde(QSS_FN, NULL);
	if(!access(rname, R_OK) || (rname = find_file(QSS_FN, false))) {
		QFile file(rname);
		if (file.open(QFile::ReadOnly)) {
			QTextStream stream(&file);
			// qss.append(stream.readAll());
			qss = stream.readAll();
		} else
			qWarning() << "Could not load style sheet " << rname;
	}
	// Then, the user-supplied style sheet is read.
	// Since it comes after the defaults/global file, it overrides them
	if (app->styleSheet().size()) {
		QString fn(app->styleSheet());
		// strip off file:///; it's always there
		// Note that this may change in the future, so I should
		// probably be more careful.  Problem is, if it isn't
		// there, I have no idea *what* to expect.
		fn.remove(0, 8);
		QFile file(fn);
		if (file.open(QFile::ReadOnly)) {
			QTextStream stream(&file);
			qss.append(stream.readAll());
		} else
			qWarning() << "Could not load style sheet " << fn;
	}
	app->setStyleSheet(qss);

	init_pixmaps();
	read_preferences();

	create_mainwindow();

	make_grokdir();

	// always do this, even if there is no form, so icon name and window
	// size are set correctly
	switch_form(mainwindow->card, formname);
	card = mainwindow->card;
		
	if (query) {
		query_any(SM_SEARCH, card, query);
		create_summary_menu(card);
		card->row = card->query ? card->query[0]
						  : card->form->dbase->nrows;
		fillout_card(card, false);
	}
	mainwindow->show();
	return app->exec();
}


/*
 * usage information
 */

static void usage(void)
{
	fprintf(stderr, "Usage: %s [options] [form ['query']]\n", progname);
	fprintf(stderr, "       %s -x|-X form template[flags] ['query']\n", progname);
	fputs("    Options:\n"
	      "\t-h\tprint this help text\n"
	      "\t-d\tdump fallback Qt Style Sheet and exit\n"
	      "\t-v\tprint version string\n"
	      "\t-t\tprint cards matching query to stdout\n"
	      "\t-T\tsame as -t without header line\n"
	      "\t-p\tprint cards matching query in `plan' format\n"
	      "\t-x\tevaluate and print template file to stdout\n"
	      "\t-X\tprint raw template file to stdout\n"
	      "\t-f\tdon't fork on startup\n"
	      "\t-r\trestricted, disable form editor (rgrok)\n\n"
	      "    the form argument is required for -t and -T.\n", stderr);
	exit(1);
}


/*
 * create the directory that holds all forms and databases. This is a
 * separate routine to avoid having a big buffer on the stack in main().
 */

static void make_grokdir(void)
{
	const char *path = resolve_tilde(GROKDIR, 0);
	if (access(path, X_OK) &&
	    create_query_popup(mainwindow, 0,
"Cannot access directory %s\n\n"
"This directory is required to store the grok configuration\n"
"file, and it is the default location for forms and databases.\n"
"If you are running grok for the first time and intend to use it\n"
"regularly, press OK now and copy all files from the grokdir\n"
"demo directory in the grok distribution into " GROKDIR " .\n"
"\n"
"If you want to experiment with grok first, press Cancel. If\n"
"you have a grokdir directory in the directory you started\n"
"grok from, it will be used in place of " GROKDIR ", but you may\n"
"not be able to create new forms and databases, and con-\n"
"figuration changes will not be saved.",
						path)) {
	    (void)chmod(path, 0700);
	    (void)mkdir(path, 0700);
	    if (access(path, X_OK))
		create_error_popup(mainwindow, errno,
					"Cannot create directory %s", path);
	}
}


/*------------------------------------ init ---------------------------------*/
/*
 * allocate pixmaps
 */

#include "bm_left.h"
#include "bm_right.h"

static unsigned char *pics[NPICS] = { bm_left_bits,   bm_right_bits };
static int pix_width[NPICS]	  = { bm_left_width,  bm_right_width };
static int pix_height[NPICS]	  = { bm_left_height, bm_right_height };

static void init_pixmaps(void)
{
	int			p;

	for (p=0; p < NPICS; p++) {
		QBitmap m(QBitmap(QBitmap::fromData(
				QSize(pix_width[p], pix_height[p]), pics[p])));
		// it's not possible for a QBitmap to have a mask, so
		// a pixmap must be used instead
		QPixmap pm(m.size());
		// FIXME: should be alterable via QSS
		//        motif version used black, too, though
		pm.fill(Qt::black);
		pm.setMask(m);
		pixmap[p] = QIcon(pm);
	}
}
