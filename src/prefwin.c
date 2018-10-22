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
#include <X11/Xos.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include <Xm/Protocols.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void done_callback (Widget, int, XmToggleButtonCallbackStruct *);
static void spool_callback(Widget, int, XmToggleButtonCallbackStruct *);

struct pref	pref;		/* global preferences */

static BOOL	have_shell = FALSE;	/* message popup exists if TRUE */
static Widget	shell;		/* popup menu shell */
static Widget	w_spoola;	/* ascii spool text */
static Widget	w_spoolp;	/* ascii spool text */
static Widget	w_linelen;	/* truncate printer lines */
static Widget	w_lines;	/* summary lines text */
static Widget	w_scale;	/* card scale text */
static BOOL	modified;	/* preferences have changed */


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
			free(pref.pspooler_a);
			pref.pspooler_p = mystrdup(p);
			modified = TRUE;
		}
		p = read_text_button(w_linelen,  0);
		if ((i = atoi(p)) > 39 && i <= 250) {
			pref.linelen = i;
			modified = TRUE;
		}
		p = read_text_button(w_lines,  0);
		if ((i = atoi(p)) > 0 && i < 80) {
			pref.sumlines = i;
			modified = TRUE;
		}
		p = read_text_button(w_scale,  0);
		if ((d = atof(p)) > 0.1 && d < 10.0) {
			pref.scale = d;
			modified = TRUE;
		}
		XtPopdown(shell);
		XtDestroyWidget(shell);
		have_shell = FALSE;
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

static void flag_callback(Widget, struct flag*, XmToggleButtonCallbackStruct*);

void create_preference_popup(void)
{
	Widget			form, w, sep;
	Arg			args[20];
	int			n;
	Atom			closewindow;
	const struct flag	*flag;

	destroy_preference_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("Grok Preferences", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);
	form = XtCreateManagedWidget("prefform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pref");

							/*-- flags --*/
	for (w=0, flag=flags; flag->value; flag++) {
	   n = 0;
	   XtSetArg(args[n], XmNtopAttachment,	w ? XmATTACH_WIDGET
						  : XmATTACH_FORM);	n++;
	   XtSetArg(args[n], XmNtopWidget,	w);			n++;
	   XtSetArg(args[n], XmNtopOffset,	8);			n++;
	   XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	   XtSetArg(args[n], XmNleftOffset,	16);			n++;
	   XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);	n++;
	   XtSetArg(args[n], XmNset,		*flag->value);		n++;
	   XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	   w = XtCreateManagedWidget(flag->text,
			xmToggleButtonWidgetClass, form, args, n);
	   XtAddCallback(w, XmNvalueChangedCallback,
			(XtCallbackProc)flag_callback, (XtPointer)flag);
	   XtAddCallback(w, XmNhelpCallback,
	  		(XtCallbackProc)help_callback, (XtPointer)"pref");
	}

							/*-- print spooler --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	sep = XtCreateManagedWidget("ps1",
				xmSeparatorWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget("PostScript print spooler:",
				xmLabelWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		200);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	w_spoolp = XtCreateManagedWidget("spoolp",
				xmTextWidgetClass, form, args, n);
	XtAddCallback(w_spoolp, XmNactivateCallback,
				(XtCallbackProc)spool_callback, NULL);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_spoolp);		n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget("ASCII print spooler:",
				xmLabelWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_spoolp);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_spoolp);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		200);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	w_spoola = XtCreateManagedWidget("spoola",
				xmTextWidgetClass, form, args, n);
	XtAddCallback(w_spoola, XmNactivateCallback,
				(XtCallbackProc)spool_callback, NULL);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_spoola);		n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget("Printer line length:",
				xmLabelWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_spoola);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_spoola);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		200);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	w_linelen = XtCreateManagedWidget("linelen",
				xmTextWidgetClass, form, args, n);
	XtAddCallback(w_linelen, XmNactivateCallback,
				(XtCallbackProc)spool_callback, NULL);

							/*-- nlines, scale --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_linelen);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	sep = XtCreateManagedWidget("ps2",
				xmSeparatorWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget("Summary lines:",
				xmLabelWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_spoolp);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		200);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	w_lines = XtCreateManagedWidget("lines",
				xmTextWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_lines);		n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget("Card scaling factor:",
				xmLabelWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_lines);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_spoolp);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		200);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	w_scale = XtCreateManagedWidget("scale",
				xmTextWidgetClass, form, args, n);

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_scale);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	sep = XtCreateManagedWidget("ps2",
				xmSeparatorWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Done",
				xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)done_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pref_done");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Help",
				xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pref");
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pref");

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, (char *)"WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)done_callback, (XtPointer)0);
	print_text_button_s(w_spoola,        pref.pspooler_a);
	print_text_button_s(w_spoolp,        pref.pspooler_p);
	print_text_button  (w_linelen, "%d", pref.linelen);
	print_text_button  (w_lines,   "%d", pref.sumlines);
	print_text_button  (w_scale,   "%g", pref.scale);
	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void done_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	destroy_preference_popup();
}


/*ARGSUSED*/
static void flag_callback(
	Widget				widget,
	struct flag			*flag,
	XmToggleButtonCallbackStruct	*data)
{
	*flag->value = data->set;
	if (flag->value == &pref.uniquedb)
		remake_dbase_pulldown();
	modified = TRUE;
}


/*ARGSUSED*/
static void spool_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	int				i;

	if (pref.pspooler_a)
		free(pref.pspooler_a);
	if (pref.pspooler_p)
		free(pref.pspooler_p);
	(void)read_text_button(w_spoola, &pref.pspooler_a);
	(void)read_text_button(w_spoolp, &pref.pspooler_p);
	i = atoi(read_text_button(w_linelen, 0));
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
		create_error_popup(toplevel, errno,
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
	fprintf(fp, "scale	%g\n",	pref.scale);
	fprintf(fp, "lines	%d\n",	pref.sumlines);
	fprintf(fp, "pselect	%c\n",	pref.pselect);
	fprintf(fp, "pformat	%c\n",	pref.pformat);
	fprintf(fp, "pqual	%c\n",	pref.pquality);
	fprintf(fp, "pdevice	%c\n",	pref.pdevice);
	fprintf(fp, "pspoola	%s\n",	pref.pspooler_a);
	fprintf(fp, "pspoolp	%s\n",	pref.pspooler_p);
	fprintf(fp, "pfile	%s\n",	pref.pfile ? pref.pfile : "");
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
		if (!strcmp(key, "pspoola"))	pref.pspooler_a	 = mystrdup(p);
		if (!strcmp(key, "pspoolp"))	pref.pspooler_p	 = mystrdup(p);
		if (!strcmp(key, "pfile"))	pref.pfile	 = mystrdup(p);
		if (!strcmp(key, "linelen"))	pref.linelen	 = atoi(p);
	}
	fclose(fp);
}
