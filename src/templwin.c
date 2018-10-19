/*
 * Create and destroy the export popup, and call the template functions.
 * The configuration is stored in the pref structure, but it can be changed
 * only here.
 *
 *	destroy_templ_popup()
 *	create_templ_popup()
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/Separator.h>
#include <Xm/FileSB.h>
#include <Xm/Protocols.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define NLINES	15		/* number of lines in list widget */

static void mklist(void);
static void button_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void file_export_callback(Widget,int,XmFileSelectionBoxCallbackStruct*);
static void file_cancel_callback(Widget,int,XmFileSelectionBoxCallbackStruct*);
static void editfile(char *);
static void askname(BOOL);

extern Display	*display;	/* everybody uses the same server */
extern int	errno;
extern XmFontList fontlist[NFONTS];
extern Pixel	color[NCOLS];	/* colors: COL_* */
extern Widget	toplevel;	/* top-level shell for error popup */
extern struct	pref pref;	/* global preferences */
extern CARD 	*curr_card;	/* card being displayed in main win */

static BOOL	have_shell;	/* message popup exists if TRUE */
static Widget	shell;		/* popup menu shell */
static Widget	list;		/* template list widget */
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
		XtPopdown(shell);
		XtDestroyWidget(shell);
		have_shell = FALSE;
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
	char	*text;
	Widget	widget;
} menu[] = {
	{ 'L',	0,	"Template:"		},
	{ 'S',	0x10,	"scroll",		},
	{ 'B',	0x20,	"Create"		},
	{ 'b',	0x21,	"Dup"			},
	{ 'b',	0x22,	"Edit"			},
	{ 'q',	0x23,	"Delete"		},
	{ '-',	0,	"div"			},
	{ 'L',	0,	"Output file:"		},
	{ 'T',	0x30,	"text",			},
	{ 'B',	0x40,	"Browse"		},
	{ 'b',	0x41,	"Export"		},
	{ 'b',	0x42,	"Cancel"		},
	{ 'q',	0x43,	"Help"			},
	{  0,   0,	0			}
};

void create_templ_popup(void)
{
	struct menu	*mp;			/* current menu[] entry */
	WidgetClass	class;			/* label, radio, or button */
	Widget		form, w=0, top=0;
	Arg		args[20];
	int		n;
	Atom		closewindow;

	destroy_templ_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("Grok Export", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);
	form = XtCreateManagedWidget("exportform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"tempname");

	for (mp=menu; mp->type; mp++) {
	    n = 0;
	    if (w == 0) {
		XtSetArg(args[n], XmNtopAttachment,  XmATTACH_FORM);	n++;
	    } else {
		XtSetArg(args[n], XmNtopAttachment,  XmATTACH_WIDGET);	n++;
		XtSetArg(args[n], XmNtopWidget,      top);		n++;
	    }
	    if ((mp->code & 0x70) == 0x40) {
		XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNbottomOffset,   16);		n++;
	    }
	    if (mp->type == 'b' || mp->type == 'q') {
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);	n++;
		XtSetArg(args[n], XmNleftWidget,     w);		n++;
		XtSetArg(args[n], XmNleftOffset,     8);		n++;
	    } else {
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNleftOffset,     16);		n++;
	    }
	    if (mp->type != 'L' && mp->type != 'b' && mp->type != 'B') {
		XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM);	n++;
	        XtSetArg(args[n], XmNrightOffset,    16);		n++;
	    }
	    if (mp->type == 'b' || mp->type == 'q' || mp->type == 'B') {
		XtSetArg(args[n], XmNwidth,	     80);		n++;
		XtSetArg(args[n], XmNtopOffset,	     16);		n++;
	    } else if (mp->type == 'L') {
		XtSetArg(args[n], XmNtopOffset,	     16);		n++;
	    } else {
		XtSetArg(args[n], XmNtopOffset,	     8);		n++;
	    }
	    if (mp->type == 'T') {
		XtSetArg(args[n], XmNbackground,  color[COL_TEXTBACK]);	n++;
	    }
	    XtSetArg(args[n], XmNhighlightThickness, 0);		n++;
	    switch(mp->type) {
	      case 'B':
	      case 'b':
	      case 'q': class = xmPushButtonWidgetClass;	break;
	      case 'T': class = xmTextWidgetClass;		break;
	      case '-': class = xmSeparatorWidgetClass;		break;
	      case 'L':	class = xmLabelWidgetClass;		break;
	      case 'S': class = 0;				break;
	    }
	    if (class) {
		w = mp->widget = XtCreateManagedWidget(mp->text, class,
							form, args, n);
		if (mp->type == 'T' && pref.xfile)
			print_text_button(w, pref.xfile);
	    } else {
		XtSetArg(args[n], XmNselectionPolicy,	XmBROWSE_SELECT);  n++;
		XtSetArg(args[n], XmNvisibleItemCount,	NLINES);	   n++;
		XtSetArg(args[n], XmNitemCount,		0);		   n++;
		XtSetArg(args[n], XmNfontList,	fontlist[FONT_COURIER]);   n++;
		XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC);	   n++;
		w = list = mp->widget =
				XmCreateScrolledList(form, mp->text, args, n);
		list_nlines = 0;
		mklist();
		XtManageChild(list);
	    }
	    if (mp->type != 'B' && mp->type != 'b')
		top = w;

	    if (mp->type == 'b' || mp->type == 'q' || mp->type == 'B'
	    					   || mp->type == 'T')
		XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)button_callback, (XtPointer)mp->code);
	}
	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)button_callback, (XtPointer)0x42);
	have_shell = TRUE;
	modified = FALSE;
}


/*
 * put the current list of templates into the template list widget
 */

static void save_cb(int seq, char *name)
{
	XmString string = XmStringCreateSimple(name);
	XmListAddItemUnselected(list, string, 0);
	XmStringFree(string);
	list_nlines++;
}

static void mklist(void)
{
	while (list_nlines)
		XmListDeletePos(list, list_nlines--);
	list_templates(save_cb, curr_card);
	if (pref.xlistpos >= list_nlines)
		pref.xlistpos = list_nlines-1;
	XmListSelectPos(list, pref.xlistpos+1, False);
}


/*-------------------------------------------------- callbacks --------------*/


static int get_list_seq(void)
{
	int		*sel, num;	/* list of selected lines */

	if (XmListGetSelectedPos(list, &sel, &num) && num == 1
						   && *sel <= list_nlines) {
		pref.xlistpos = *sel - 1;
		free(sel);
		return(TRUE);
	} else {
		create_error_popup(shell, 0, "Please choose a template name");
		return(FALSE);
	}
}


static BOOL export(void)
{
	struct menu	*mp;		/* for finding text widget */
	char		*err;

	for (mp=menu; mp->code != 0x30; mp++);
	read_text_button_noblanks(mp->widget, &pref.xfile);
	if (!pref.xfile) {
		create_error_popup(shell,0,"Please enter an output file name");
		return(FALSE);
	}
	if (!get_list_seq())
		return(FALSE);

	if (err = exec_template(pref.xfile, 0, pref.xlistpos, curr_card)) {
		create_error_popup(shell, 0, "Export failed:\n%s", err);
		return(FALSE);
	}
	modified = TRUE;
	return(TRUE);
}


static void button_callback(
	Widget				widget,
	int				code,
	XmToggleButtonCallbackStruct	*data)
{
	Widget				w;

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
		w = XmCreateFileSelectionDialog(shell, "xfile", NULL, 0);
		XtAddCallback(w, XmNokCallback,
				(XtCallbackProc)file_export_callback, 0);
		XtAddCallback(w, XmNcancelCallback,
				(XtCallbackProc)file_cancel_callback, 0);
		XtManageChild(w);
		break;
	  case 0x30:						/* text */
	  case 0x41:						/* Export */
		if (export())
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
	Widget				widget,
	int				item,
	XmFileSelectionBoxCallbackStruct*data)
{
	char				*p = 0;
	struct menu			*mp;

	if (!XmStringGetLtoR(data->value,XmSTRING_DEFAULT_CHARSET, &p) || !p)
		return;
	if (*p) {
		if (pref.xfile)
			free(pref.xfile);
		pref.xfile = mystrdup(p);
		for (mp=menu; mp->code != 0x30; mp++);
		print_text_button(mp->widget, pref.xfile);
	}
	XtFree(p);
	XtDestroyWidget(widget);
}


static void file_cancel_callback(
	Widget				widget,
	int				item,
	XmFileSelectionBoxCallbackStruct*data)
{
	XtDestroyWidget(widget);
}


/*-------------------------------- ask for name -----------------------------*/
/*
 * user pressed Dup or Create. Ask for a new file name.
 */

static void text_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void textcancel_callback	(Widget, int, XmToggleButtonCallbackStruct *);

static BOOL		have_askshell;	/* text popup exists if TRUE */
static Widget		askshell;	/* popup menu shell */
static Widget		text;		/* template name string */
static BOOL		duplicate;	/* dup file before editing */

static void askname(
	BOOL		dup)		/* duplicate file before editing */
{
	Widget		form, w;
	Arg		args[20];
	int		n;
	Atom		closewindow;

	duplicate = dup;
	if (have_askshell) {
		XtPopup(askshell, XtGrabNone);
		return;
	}
	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	askshell = XtAppCreateShell("Template name", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(askshell, 1);
	form = XtCreateManagedWidget("tempform", xmFormWidgetClass,
			askshell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback, (XtCallbackProc)help_callback,
						(XtPointer)"tempname");
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	w = XtCreateManagedWidget("Name for new template:", xmLabelWidgetClass,
			form, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	text = XtCreateManagedWidget(" ", xmTextFieldWidgetClass,
			form, args, n);
	XtAddCallback(text, XmNactivateCallback, (XtCallbackProc)text_callback,
						(XtPointer)NULL);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		text);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Cancel", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
					textcancel_callback, (XtPointer)0);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		text);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Help", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)help_callback,
						(XtPointer)"tempname");

	XtPopup(askshell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(askshell, closewindow, (XtCallbackProc)
				textcancel_callback, (XtPointer)askshell);
	have_askshell = TRUE;
}


/*ARGSUSED*/
static void textcancel_callback(widget, item, data)
	Widget				widget;
	int				item;
	XmToggleButtonCallbackStruct	*data;
{
	if (have_askshell)
		XtPopdown(askshell);
	have_askshell = FALSE;
}


/*ARGSUSED*/
static void text_callback(widget, item, data)
	Widget				widget;
	int				item;
	XmToggleButtonCallbackStruct	*data;
{
	char				*name, *p;
	char				*string;

	string = XmTextFieldGetString(text);
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
	XtFree(string);
	textcancel_callback(widget, item, data);
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
