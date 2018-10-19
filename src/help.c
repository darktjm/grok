/*
 * print help popup on a particular topic. Help topics are identified by
 * a short string that is looked up in the help file. The help file contains
 * a list of button help entries; each consisting of a line "%% topic"
 * followed by help text lines. Leading blanks in text lines are ignored.
 * Lines beginning with '#' are ignored.
 *
 * To add a help text to a widget, call
 *	XtAddCallback(widget, XmNhelpCallback, help_callback, "topic")
 * and add "%%n topic" and the help text to the help file (n is a number
 * used for extracting a user's manual, it is not used by the grok program).
 *
 * help_callback(parent, topic)		Print help for a topic, usually for
 *					button <parent>.
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include <X11/cursorfont.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void done_callback    (Widget, int, XmToggleButtonCallbackStruct *);
static void context_callback (Widget, int, XmToggleButtonCallbackStruct *);
static char *get_text(char *);

extern Display		*display;	/* everybody uses the same server */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern XmFontList	fontlist[NFONTS];
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern CARD 		*curr_card;	/* card being displayed in main win */

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */


/*
 * destroy the help popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_help_popup(void)
{
	if (have_shell) {
		XtPopdown(shell);
		XtDestroyWidget(shell);
		have_shell = FALSE;
	}
}



/*
 * look up the help text for <topic> and create a window containing the text.
 */

/*ARGSUSED*/
void help_callback(
	Widget			parent,
	char			*topic)
{
	static BOOL		have_fontlist;
	static XmFontList	fontlist;
	static Widget		text_w;
	Widget			form, w;
	Atom			closewindow;
	char			*message;
	Arg			args[20];
	int			n;
	int			nlines = 1;
	char			*p;

	if (!(message = get_text(topic)))
		return;
	if (!strcmp(topic, "card") && curr_card && curr_card->form
						&& curr_card->form->help) {
		if (!(message = realloc(message, strlen(message) +
				 	strlen(curr_card->form->help) + 2)))
			return;
		strcat(message, "\n");
		strcat(message, curr_card->form->help);
	}
	if (have_shell) {
		XmTextSetString(text_w, message);
		XtPopup(shell, XtGrabNone);
		free(message);
		return;
	}
	if (!have_fontlist++)
		fontlist = XmFontListCreate(font[FONT_HELP], "cset");
	for (nlines=0, p=message; *p; p++)
		nlines += *p == '\n';
	if (nlines > 30)
		nlines = 30;

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("Help", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);
	form = XtCreateManagedWidget("helpform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"help");

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Dismiss", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)done_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"help_done");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Context", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)context_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"help_ctx");

							/*-- text --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w);			n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNeditable,		False);			n++;
	XtSetArg(args[n], XmNeditMode,		XmMULTI_LINE_EDIT);	n++;
	XtSetArg(args[n], XmNcolumns,		60);			n++;
	XtSetArg(args[n], XmNrows,		nlines+1);		n++;
	XtSetArg(args[n], XmNfontList,		fontlist);		n++;
	XtSetArg(args[n], XmNscrollVertical,	True);			n++;
	XtSetArg(args[n], XmNpendingDelete,	False);			n++;
	text_w = w = XmCreateScrolledText(form, "text", args, n);
	XmTextSetString(w, message);
	XtVaSetValues(w, XmNbackground, color[COL_SHEET],
			 XmNforeground, color[COL_STD], NULL);
	XtManageChild(w);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"help");
	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)done_callback, (XtPointer)shell);
	have_shell = TRUE;
	free(message);
}


/*ARGSUSED*/
static void done_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	destroy_help_popup();
}


/*ARGSUSED*/
static void context_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	Widget w;
	Cursor cursor = XCreateFontCursor(display, XC_question_arrow);
	if (w = XmTrackingLocate(shell, cursor, False)) {
		data->reason = XmCR_HELP;
		XtCallCallbacks(w, XmNhelpCallback, &data);
	}
	XFreeCursor(display, cursor);
}


/*ARGSUSED*/
static char *get_text(
	char			*topic)
{
	FILE			*fp;		/* help file */
	char			line[1024];	/* line buffer (and filename)*/
	char			*text;		/* text buffer */
	int			textsize;	/* size of text buffer */
	int			textlen = 0;	/* # of chars in text buffer */
	int			n;		/* for stripping trailing \n */
	register char		*p;

	if (!(text = (char *)malloc(textsize = 4096)))
		return(0);
	*text = 0;
	if (!find_file(line, HELP_FN, FALSE) || !(fp = fopen(line, "r"))) {
		sprintf(text, "Sorry, no help available,\n%s not found",
								HELP_FN);
		return(text);
	}
	for (;;) {					/* find topic */
		if (!fgets(line, 1024, fp)) {
			strcpy(text, "Sorry, no help available on this topic");
			return(text);
		}
		if (line[0] != '%' || line[1] != '%')
			continue;
		line[strlen(line)-1] = 0; /* strip \n */
		for (p=line+2; *p >= '0' && *p <= '9'; p++);
		for (; *p == ' ' || *p == '\t'; p++);
		if (*p && *p != '\n' && !strcmp(p, topic))
			break;
	}
	for (;;) {					/* read text */
		if (!fgets(line, 1024, fp))
			break;
		if (line[0] == '#')
			continue;
		if (line[0] == '%' && line[1] == '%')
			return(text);
		p = line[0] == '\t' ? line+1 : line;
		if (textlen + strlen(p) + 1 > textsize)
			if (!(text = (char *)realloc(text, textsize += 4096)))
				break;
		strcat(text, p);
		textlen += strlen(p);
	}
	for (n=strlen(text); n && text[n-1] == '\n'; n--)
		text[n-1] = 0;
	return(text);
}
