/*
 * Initializes everything and starts a calendar window for the current
 * month. The interval timer (for autosave and entry recycling) and a
 * few routines used by everyone are also here.
 *
 *	main(argc, argv)		Guess.
 *
 * Author: thomas@bitrot.de (Thomas Driemeyer)
 * Major changes by Thomas J. Moore; see HISTORY
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

static void usage(void), mkdir_callback(void), make_grokdir(void);
static void init_pixmaps(void);

QApplication		*app;		/* application handle */
char			*progname;	/* argv[0] */
QIcon			pixmap[NPICS];	/* common symbols */
BOOL			restricted;	/* restricted mode, no form editor */


static const char * const default_qss =
#include "resource.h"
;


static char rname[1024]; // ugh -- fixed size

/*
 * initialize everything and create the main calendar window
 */

int main(
	int		argc,
	char		*argv[])
{
	int		n;
	char		*formname = 0;
	char		*tmpl = 0;
	char		*query	  = 0;
	BOOL		nofork	  = FALSE;
	BOOL		ttymode	  = FALSE;
	BOOL		planmode  = FALSE;
	BOOL		noheader  = FALSE;
	BOOL		do_export = FALSE;

	setlocale(LC_ALL, "");
	if ((progname = strrchr(argv[0], '/')) && progname[1])
		progname++;
	else
		progname = argv[0];
	restricted = !strcmp(progname, "rgrok");
	nofork = TRUE; // tjm - debugging for now

	for (n=1; n < argc; n++)			/* options */
		if (*argv[n] != '-')
			if (!formname)
				formname = argv[n];
			else if (do_export && !tmpl)
				tmpl = argv[n];
			else if (!query)
				query = argv[n];
			else
				usage();

		else if (argv[n][2] < 'a' || argv[n][2] > 'z')
			switch(argv[n][1]) {
			  case 'd':
				puts(default_qss);
				return(0);
			  case 'v':
				fprintf(stderr, "%s: %s\n", progname, VERSION);
				return(0);
			  case 'f':
				nofork   = TRUE;
				break;
			  case 'T':
				noheader = TRUE;
			  case 't':
				ttymode  = TRUE;
				break;
			  case 'p':
				planmode = TRUE;
				noheader = TRUE;
				break;
			  case 'x':
				do_export   = TRUE;
				break;
			  case 'r':
				restricted = TRUE;
				break;
			  default:
				usage();
			}

	(void)umask(0077);
	tzset();
	if (ttymode) {
		char buf[1024];
		if (!formname)
			usage();
		read_preferences();
		switch_form(formname);
		if (!curr_card ||!curr_card->dbase ||!curr_card->dbase->nrows){
			fprintf(stderr, "%s: %s: no data\n",progname,formname);
			_exit(0);
		}
		query_any(SM_SEARCH, curr_card, query);
		if (!curr_card->nquery) {
			fprintf(stderr,"%s: %s: no match\n",progname,formname);
			_exit(0);
		}
		if (!noheader) {
			char *p;
			make_summary_line(buf, curr_card, -1);
			puts(buf);
			for (p=buf; *p; p++)
				*p = '-';
			puts(buf);
		}
		for (n=0; n < curr_card->nquery; n++) {
			make_summary_line(buf, curr_card, curr_card->query[n]);
			puts(buf);
		}
		fflush(stdout);
		_exit(0);
	}
	if (planmode) {
		if (!formname)
			usage();
		read_preferences();
		switch_form(formname);
		if (!curr_card ||!curr_card->dbase ||!curr_card->dbase->nrows)
			_exit(0);
		query_any(SM_SEARCH, curr_card, query ?
					query : curr_card->form->planquery);
		for (n=0; n < curr_card->nquery; n++)
			make_plan_line(curr_card, curr_card->query[n]);
		fflush(stdout);
		_exit(0);
	}
	if (do_export) {
		const char *p;
		if (!formname || !tmpl)
			usage();
		read_preferences();
		switch_form(formname);
		if (!curr_card ||!curr_card->dbase ||!curr_card->dbase->nrows){
			fprintf(stderr, "%s: %s: no data\n",progname,formname);
			_exit(0);
		}
		query_any(SM_SEARCH, curr_card, query);
		if (!curr_card->nquery) {
			fprintf(stderr,"%s: %s: no match\n",progname,formname);
			_exit(0);
		}
		if (p = exec_template(0, tmpl, 0, curr_card))
			fprintf(stderr, "%s %s: %s\n", progname, formname, p);
		fflush(stdout);
		_exit(0);
	}
	if (!nofork) {					/* background */
		long pid = fork();
		if (pid < 0)
			perror("can't fork");
		else if (pid > 0)
			_exit(0);
	}
	app = new QApplication(argc, argv);
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
	strcpy(rname, resolve_tilde((char *)QSS_FN, NULL));
	if(!access(rname, R_OK) || find_file(rname, QSS_FN, FALSE)) {
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

	// FIXME: setfgcolor(white)
	init_pixmaps();
	read_preferences();
	// FIXME: setfgcolor(color[COL_CANVBACK])

	create_mainwindow();
	mainwindow->show();

	make_grokdir();

	if (formname)
		switch_form(formname);
	if (query) {
		query_any(SM_SEARCH, curr_card, query);
		create_summary_menu(curr_card);
		curr_card->row = curr_card->query ? curr_card->query[0]
						  : curr_card->dbase->nrows;
		fillout_card(curr_card, FALSE);
	}
	return app->exec();
}


/*
 * usage information
 */

static void usage(void)
{
	fprintf(stderr, "Usage: %s [options] [form ['query']]\n", progname);
	fprintf(stderr, "       %s -x form template ['query']\n", progname);
	fprintf(stderr, "    Options:\n%s%s%s%s%s%s%s%s%s\n",
			"\t-h\tprint this help text\n",
			"\t-d\tdump fallback app-defaults and exit\n",
			"\t-v\tprint version string\n",
			"\t-t\tprint cards matching query to stdout\n",
			"\t-T\tsame as -t without header line\n",
			"\t-p\tprint cards matching query in `plan' format\n",
			"\t-x\tevaluate and print template file to stdout\n",
			"\t-f\tdon't fork on startup\n",
			"\t-r\trestricted, disable form editor (rgrok)\n");
	fprintf(stderr, "    the form argument is required for -t and -T.\n");
	_exit(1);
}


/*
 * create the directory that holds all forms and databases. This is a
 * separate routine to avoid having a big buffer on the stack in main().
 */

static void mkdir_callback(void)
{
	char *path = resolve_tilde((char *)GROKDIR, 0); /* GROKDIR has no trailing / */
	(void)chmod(path, 0700);
	(void)mkdir(path, 0700);
	if (access(path, X_OK))
		create_error_popup(mainwindow, errno,
					"Cannot create directory %s", path);
}

static void make_grokdir(void)
{
	char *path = resolve_tilde((char *)GROKDIR, 0); /* GROKDIR has no trailing / */
	if (access(path, X_OK))
		create_query_popup(mainwindow, mkdir_callback, 0,
"Cannot access directory %s\n\n\
This directory is required to store the grok configuration\n\
file, and it is the default location for forms and databases.\n\
If you are running grok for the first time and intend to use it\n\
regularly, press OK now and copy all files from the grokdir\n\
demo directory in the grok distribution into %s .\n\
\n\
If you want to experiment with grok first, press Cancel. If\n\
you have a grokdir directory in the directory you started\n\
grok from, it will be used in place of %s, but you may\n\
not be able to create new forms and databases, and con-\n\
figuration changes will not be saved.",
						path, GROKDIR, GROKDIR);
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

	for (p=0; p < NPICS; p++)
		pixmap[p].addPixmap(QBitmap::fromData(QSize(pix_width[p], pix_height[p]), pics[p]));
}
