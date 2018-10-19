/*
 * Various popups, such as the About... dialog.
 *
 *	create_about_popup()
 *					Create About info popup
 *	create_error_popup(widget, error, fmt, ...)
 *					Create error popup with Unix error
 */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <X11/Xos.h>
#include <errno.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/Protocols.h>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "version.h"

#ifndef __DATE__
#define __DATE__ ""
#endif

extern char		*progname;	/* argv[0] */
extern Display		*display;	/* everybody uses the same server */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern struct		pref pref;	/* global preferences */


/*---------------------------------------------------------- about ----------*/
/*
 * install the About popup. Nothing can be done with it except clicking it
 * away. This is called from the File pulldown in the main menu.
 */

static char about_message[] = "\
\n\
Graphical Resource Organizer Kit\n\
Version %s\n\
Compiled %s\n\n\
Author: Thomas Driemeyer <thomas@bitrot.de>\n\n\
Homepage: http://www.bitrot.de/grok.html\n";

void create_about_popup(void)
{
	char			msg[512];
	Widget			dialog;
	XmString		s;
	Arg			args[10];
	int			n;

	sprintf(msg, about_message, VERSION, __DATE__);
	s = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);
	n = 0;
	XtSetArg(args[n], XmNmessageString, s); n++;
	dialog = XmCreateInformationDialog(mainwindow, "About", args, n);
	XmStringFree(s);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}


/*---------------------------------------------------------- errors ---------*/
/*
 * install an error popup with a message. If a nonzero error is given,
 * insert the appropriate Unix error message. The user must click this
 * away. The widget is some window that the popup goes on top of.
 */

/*VARARGS*/
void create_error_popup(Widget widget, int error, char *fmt, ...)
{
	va_list			parm;
	char			msg[17108];
	Widget			dialog;
	XmString		string;
	Arg			args;

	strcpy(msg, "ERROR:\n\n");
	va_start(parm, fmt);
	vsprintf(msg + strlen(msg), fmt, parm);
	va_end(parm);
	if (error) {
		strcat(msg, "\n");
		strcat(msg, strerror(error));
	}
	if (!widget) {
		fprintf(stderr, "%s: %s\n", progname, msg);
		return;
	}
	string = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);
	XtSetArg(args, XmNmessageString, string);
	dialog = XmCreateWarningDialog(widget, "Grok Error", &args, 1);
	XmStringFree(string);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}


/*---------------------------------------------------------- question -------*/
/*
 * put up a dialog with a message, and wait for the user to press either
 * the OK or the Cancel button. When the OK button is pressed, call the
 * callback. If Cancel is pressed, the callback is not called. The widget
 * is some window that the popup goes on top of. The help message is some
 * string that appears in grok.hlp after %%.
 */

/*VARARGS*/
void create_query_popup(
	Widget		widget,			/* window that caused this */
	void		(*callback)(),		/* OK callback */
	char		*help,			/* help text tag for popup */
	char		*fmt, ...)		/* message */
{
	va_list		parm;
	char		msg[1024];
	Widget		dialog;
	XmString	string;
	Arg		args[2];

	va_start(parm, fmt);
	vsprintf(msg, fmt, parm);
	va_end(parm);
	string = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);

	XtSetArg(args[0], XmNmessageString,     string);
	XtSetArg(args[1], XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON);
	dialog = XmCreateQuestionDialog(widget, "Grok Dialog", args, 2);
	XmStringFree(string);

	XtAddCallback(dialog, XmNokCallback,
			(XtCallbackProc)callback, NULL);
	XtAddCallback(dialog, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)help);
	XtSetSensitive(XmMessageBoxGetChild(dialog,
					    XmDIALOG_HELP_BUTTON), !!help);
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}


/*---------------------------------------------------------- dbase info -----*/
/*
 * install the Database info popup. It displays useful information about
 * the named card. This is called from the Help pulldown in the main menu.
 */

static char info_message[] = "\n\
  Form name:  %s\n\
  Form path:  %s\n\
  Form comment:  %s\n\n\
  Default query:  %s\n\n\
  Database name:  %s%s%s\n\
  Database size:  %d cards,  %d columns\n\n\
  Help information:\n%s\n\n\
  Sections:\
";

void create_dbase_info_popup(
	CARD		*card)
{
	FORM		*form;
	DBASE		*dbase;
	char		msg[8192], date[20];
	struct tm	*tm;
	Widget		dialog;
	XmString	s;
	Arg		args[10];
	int		n;

	if (!card)
		return;
	form  = card->form;
	dbase = card->dbase;
	sprintf(msg, info_message,
		form && form->name	? form->name	      : "(none)",
		form && form->path	? form->path	      : "(none)",
		form && form->comment	? form->comment	      : "(none)",
		form && form->autoquery >= 0
					? form->query[form->autoquery].name
					: "(none)",
		form  && form->dbase	? form->dbase	      : "(none)",
		dbase && dbase->rdonly ||
		form  && form->rdonly	? " (read-only)"      : "",
		form  && form->proc	? " (procedural)"     : "",
		dbase			? dbase->nrows	      : 0,
		dbase			? dbase->maxcolumns-1 : 0,
		form  && form->help	? form->help	      : "     (none)");
	if (!dbase || !dbase->nsects)
		strcat(msg, " (none)");
	else {
		register SECTION *sect = dbase->sect;
		for (n=0; n < dbase->nsects; n++, sect++) {
			tm = localtime(&sect->mtime);
			if (pref.mmddyy)
				sprintf(date, "%2d/%02d/", tm->tm_mon+1,
							   tm->tm_mday);
			else
				sprintf(date, "%2d.%02d.", tm->tm_mday,
							   tm->tm_mon+1);
			sprintf(date+6, "%2d %2d:%02d",    tm->tm_year % 100,
							   tm->tm_hour,
							   tm->tm_min);
			sprintf(msg + strlen(msg),
				"\n      %s:  %d cards, %s, \"%s\" %s%s",
				section_name(dbase, n),
				sect->nrows == -1 ? 0 : sect->nrows,
				date,
				sect->path,
				sect->rdonly   ? " (read only)" : "",
				sect->modified ? " (modified)"  : "");
		}
	}
	s = XmStringCreateLtoR(msg, XmSTRING_DEFAULT_CHARSET);
	n = 0;
	XtSetArg(args[n], XmNmessageString, s); n++;
	dialog = XmCreateInformationDialog(mainwindow, "Database Info", args,n);
	XmStringFree(s);
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	(void)XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XtManageChild(dialog);
}
