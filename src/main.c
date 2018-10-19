/*
 * Initializes everything and starts a calendar window for the current
 * month. The interval timer (for autosave and entry recycling) and a
 * few routines used by everyone are also here.
 *
 *	main(argc, argv)		Guess.
 *
 * Author: thomas@bitrot.de (Thomas Driemeyer)
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <Xm/Xm.h>
#include <X11/StringDefs.h>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "version.h"

static void usage(void), mkdir_callback(void), make_grokdir(void);
static void init_colors(void), init_fonts(void), init_pixmaps(void);

Display			*display;	/* everybody uses the same server */
GC			gc;		/* everybody uses this context */
GC			xor_gc;		/* XOR gc for rubberbanding */
XtAppContext		app;		/* application handle */
Widget			toplevel;	/* top-level shell for icon name */
char			*progname;	/* argv[0] */
XFontStruct		*font[NFONTS];	/* fonts: FONT_* */
XmFontList		fontlist[NFONTS];
Pixel			color[NCOLS];	/* colors: COL_* */
Pixmap			pixmap[NPICS];	/* common symbols */
BOOL			restricted;	/* restricted mode, no form editor */


static String fallbacks[] = {
#include "resource.h"
	NULL
};


/*
 * initialize everything and create the main calendar window
 */

int main(
	int		argc,
	char		*argv[])
{
	int		n, i;
	char		*formname = 0;
	char		*template = 0;
	char		*query	  = 0;
	BOOL		nofork	  = FALSE;
	BOOL		ttymode	  = FALSE;
	BOOL		planmode  = FALSE;
	BOOL		noheader  = FALSE;
	BOOL		export	  = FALSE;
	XGCValues	gcval;

	if ((progname = strrchr(argv[0], '/')) && progname[1])
		progname++;
	else
		progname = argv[0];
	restricted = !strcmp(progname, "rgrok");

	for (n=1; n < argc; n++)			/* options */
		if (*argv[n] != '-')
			if (!formname)
				formname = argv[n];
			else if (export && !template)
				template = argv[n];
			else if (!query)
				query = argv[n];
			else
				usage();

		else if (argv[n][2] < 'a' || argv[n][2] > 'z')
			switch(argv[n][1]) {
			  case 'd':
				for (i=0; fallbacks[i]; i++)
					printf("%s%s\n", progname,
							 fallbacks[i]);
				fflush(stdout);
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
				export   = TRUE;
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
	if (export) {
		char *p;
		if (!formname || !template)
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
		if (p = exec_template(0, template, 0, curr_card))
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
	toplevel = XtAppInitialize(&app, "Grok", NULL, 0,
#		ifndef XlibSpecificationRelease
				(Cardinal *)&argc, argv,
#		else
				&argc, argv,
#		endif
				fallbacks, NULL, 0);
	display = XtDisplay(toplevel);
	set_icon(toplevel, 0);
	gc     = XCreateGC(display, DefaultRootWindow(display), 0, 0);
	xor_gc = XCreateGC(display, DefaultRootWindow(display), 0, 0);
	gcval.function = GXxor;
	(void)XChangeGC(display, xor_gc, GCFunction, &gcval);
	XSetForeground(display, xor_gc,
			WhitePixelOfScreen(DefaultScreenOfDisplay(display)));

	init_colors();
	init_fonts();
	init_pixmaps();
	read_preferences();
	XSetForeground(display, xor_gc, color[COL_CANVBACK]);

	create_mainwindow();
	XtRealizeWidget(toplevel);

	make_grokdir();

	if (formname)
		switch_form(formname);
	if (query) {
		query_any(SM_SEARCH, curr_card, query);
		create_summary_menu(curr_card, w_summary, mainwindow);
		curr_card->row = curr_card->query ? curr_card->query[0]
						  : curr_card->dbase->nrows;
		fillout_card(curr_card, FALSE);
	}
	XtAppMainLoop(app);
	return(0);
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
	char *path = resolve_tilde(GROKDIR, 0);
	(void)chmod(path, 0700);
	(void)mkdir(path, 0700);
	if (access(path, X_OK))
		create_error_popup(toplevel, errno,
					"Cannot create directory %s", path);
}

static void make_grokdir(void)
{
	char *path = resolve_tilde(GROKDIR, 0);
	if (access(path, X_OK))
		create_query_popup(toplevel, mkdir_callback, 0,
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
 * read resources and put them into the config struct. This routine is used
 * for getting three types of resources: res_type={XtRInt,XtRBool,XtRString}.
 */

void get_rsrc(
	void		*ret,
	char		*res_name,
	char		*res_class_name,
	char		*res_type)
{
	XtResource	res_list[1];

	res_list->resource_name	  = res_name;
	res_list->resource_class  = res_class_name;
	res_list->resource_type	  = res_type;
	res_list->resource_size	  = sizeof(res_type);
	res_list->resource_offset = 0;
	res_list->default_type	  = res_type;
	res_list->default_addr	  = 0;

	XtGetApplicationResources(toplevel, ret, res_list, 1, NULL, 0);
}


/*
 * determine all colors, and allocate them. They can then be used by a call
 * to set_color(COL_XXX).
 */

static void init_colors(void)
{
	Screen			*screen = DefaultScreenOfDisplay(display);
	Colormap		cmap;
	XColor			rgb;
	int			i, d;
	char			*c, *n, class_name[256];

	cmap = DefaultColormap(display, DefaultScreen(display));
	for (i=0; i < NCOLS; i++) {
		switch (i) {
		  default:
		  case COL_STD:		n = "colStd";		d=1;	break;
		  case COL_BACK:	n = "colBack";		d=0;	break;
		  case COL_SHEET:	n = "colSheet";		d=0;	break;
		  case COL_TEXTBACK:	n = "colTextBack";	d=0;	break;
		  case COL_TOGGLE:	n = "colToggle";	d=1;	break;
		  case COL_CANVBACK:	n = "colCanvBack";	d=0;	break;
		  case COL_CANVFRAME:	n = "colCanvFrame";	d=1;	break;
		  case COL_CANVBOX:	n = "colCanvBox";	d=0;	break;
		  case COL_CANVSEL:	n = "colCanvSel";	d=1;	break;
		  case COL_CANVTEXT:	n = "colCanvText";	d=1;	break;
		  case COL_CHART_AXIS:	n = "colChartAxis";	d=1;	break;
		  case COL_CHART_GRID:	n = "colChartGrid";	d=1;	break;
		  case COL_CHART_BOX:	n = "colChartBox";	d=1;	break;
		  case COL_CHART_0:	n = "colChart0";	d=0;	break;
		  case COL_CHART_1:	n = "colChart1";	d=0;	break;
		  case COL_CHART_2:	n = "colChart2";	d=0;	break;
		  case COL_CHART_3:	n = "colChart3";	d=0;	break;
		  case COL_CHART_4:	n = "colChart4";	d=0;	break;
		  case COL_CHART_5:	n = "colChart5";	d=0;	break;
		  case COL_CHART_6:	n = "colChart6";	d=0;	break;
		  case COL_CHART_7:	n = "colChart7";	d=0;	break;
		}
		strcpy(class_name, n);
		class_name[0] &= ~('a'^'A');
		get_rsrc(&c, n, class_name, XtRString);
		if (!XParseColor(display, cmap, c, &rgb))
			fprintf(stderr, "%s: unknown color \"%s\" (%s)\n",
							progname, c, n);
		else if (!XAllocColor(display, cmap, &rgb))
			fprintf(stderr, "%s: can't alloc color \"%s\" (%s)\n",
							progname, c, n);
		else {
			color[i] = rgb.pixel;
			continue;
		}
		color[i] = d ? BlackPixelOfScreen(screen)
			     : WhitePixelOfScreen(screen);
	}
}


void set_color(
	int		col)
{
	XSetForeground(display, gc, color[col]);
}


/*
 * load all fonts and make them available in the "fonts" struct. They are
 * loaded into the GC as necessary.
 */

static void init_fonts(void)
{
	int		i;
	char		*f, class_name[256];

	for (i=0; i < NFONTS; i++) {
		switch (i) {
		  default:
		  case FONT_STD:	f = "fontList";			break;
		  case FONT_HELP:	f = "helpFont";			break;
		  case FONT_HELV:	f = "helvFont";			break;
		  case FONT_HELV_O:	f = "helvObliqueFont";		break;
		  case FONT_HELV_S:	f = "helvSmallFont";		break;
		  case FONT_HELV_L:	f = "helvLargeFont";		break;
		  case FONT_COURIER:	f = "courierFont";		break;
		  case FONT_LABEL:	f = "labelFont";		break;
		}
		strcpy(class_name, f);
		class_name[0] &= ~('a'^'A');
		get_rsrc(&f, f, class_name, XtRString);
		if (!(font[i] = XLoadQueryFont(display, f)))
			fatal("can't load font \"%s\"\n", f);
		if (!(fontlist[i] = XmFontListCreate(font[i], "cset")))
			fatal("can't create fontlist \"%s\"\n", f);
	}
}


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
	Colormap		cmap;
	XColor			rgb;
	char			*c;

	cmap = DefaultColormap(display, DefaultScreen(display));
	get_rsrc(&c, "background", "Background", XtRString);
	if (!XParseColor(display, cmap, c, &rgb))
		fprintf(stderr, "pixmap error: unknown bkg color \"%s\"\n", c);
	if (!XAllocColor(display, cmap, &rgb))
		fprintf(stderr, "pixmap error: can't alloc bkg color \"%s\"\n",
									c);
	for (p=0; p < NPICS; p++)
		if (!(pixmap[p] = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				(char *)pics[p], pix_width[p], pix_height[p],
				color[COL_STD], rgb.pixel,
				DefaultDepth(display,DefaultScreen(display)))))
			fatal("no memory for pixmaps");
}
