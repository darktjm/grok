/*
 * Create and destroy the print popup, and call the printing functions.
 * The configuration is stored in the pref structure, but it can be changed
 * only here.
 *
 *	destroy_print_popup()
 *	create_print_popup()
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include <Xm/FileSB.h>
#include <Xm/Protocols.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void cancel_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void print_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void config_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void file_print_callback	(Widget,int,XmFileSelectionBoxCallbackStruct*);
static void file_cancel_callback(Widget,int,XmFileSelectionBoxCallbackStruct*);

extern Display	*display;	/* everybody uses the same server */
extern Pixel	color[NCOLS];	/* colors: COL_* */
extern int	errno;
extern Widget	toplevel;	/* top-level shell for error popup */
extern struct	pref pref;	/* global preferences */

static BOOL	have_shell;	/* message popup exists if TRUE */
static Widget	shell;		/* popup menu shell */
static BOOL	modified;	/* preferences have changed */


/*
 * destroy popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_print_popup(void)
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
	char	type;		/* L=label, R=RowColumn, C=choice, S=Spring */
	long	code;		/* unique identifier, 0=none */
	char	*ptr;		/* location in pref where value is stored */
	char	value;		/* value stored in pref for each mode */
	char	*text;
	Widget	widget;
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
	WidgetClass	class;			/* label, radio, or button */
	String		cback;			/* activale or valueChanged */
	Widget		form, w=0, rowcol=0, sep;
	Arg		args[20];
	int		n;
	Atom		closewindow;

	destroy_print_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("Grok Print", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);
	form = XtCreateManagedWidget("prefform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"print");

	for (mp=menu; mp->type; mp++) {
	    class = xmLabelWidgetClass;
	    cback = 0;
	    n = 0;
	    if (mp->type == 'L')
		w = rowcol;
	    if (w == 0) {
		XtSetArg(args[n], XmNtopAttachment,  XmATTACH_FORM);	n++;
	    } else {
		XtSetArg(args[n], XmNtopAttachment,  XmATTACH_WIDGET);	n++;
		XtSetArg(args[n], XmNtopWidget,      w);		n++;
	    }
	    if (mp->type == 'R') {
		XtSetArg(args[n], XmNradioBehavior,  True);		n++;
		XtSetArg(args[n], XmNradioAlwaysOne, True);		n++;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNleftOffset,     32);		n++;
		class = xmRowColumnWidgetClass;
	    } else {
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNtopOffset,	     8);		n++;
		XtSetArg(args[n], XmNleftOffset,     16);		n++;
	    }
	    if (mp->type == 'C') {
		XtSetArg(args[n], XmNset,	  *mp->ptr==mp->value);	n++;
		XtSetArg(args[n], XmNfillOnSelect,   True);		n++;
		XtSetArg(args[n], XmNselectColor,    color[COL_TOGGLE]);n++;
		class = xmToggleButtonWidgetClass;
		cback = XmNvalueChangedCallback;
	    }
	    if (mp->code == 0x32) { /*<<<*/
		XtSetArg(args[n], XmNsensitive,	     FALSE);		n++;
	    }
	    XtSetArg(args[n], XmNhighlightThickness, 0);		n++;
	    w = mp->widget = XtCreateManagedWidget(mp->text, class,
	   			 mp->type == 'C' ? rowcol : form, args, n);
	    if (cback)
		XtAddCallback(w, cback,
			(XtCallbackProc)config_callback, (XtPointer)mp->code);
	    if (mp->type == 'R') {
		rowcol = w;
		w = 0;
	    }
	}
							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	sep = XtCreateManagedWidget("ps",xmSeparatorWidgetClass,form,args,n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Print",xmPushButtonWidgetClass,form,args,n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)print_callback, NULL);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Cancel",xmPushButtonWidgetClass,form,args,n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)cancel_callback, NULL);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		sep);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Help",xmPushButtonWidgetClass,form,args,n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)help_callback, (XtPointer)"print");

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)cancel_callback, NULL);
	have_shell = TRUE;
	modified = FALSE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void cancel_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	destroy_print_popup();
}


/*ARGSUSED*/
static void print_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	Arg		args[20];
	int		n = 0;
	Widget		w;

	if (pref.pdevice == 'F') {
		/*
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
		*/
		w = XmCreateFileSelectionDialog(shell, "pfile", args, n);
		XtAddCallback(w, XmNokCallback,
				(XtCallbackProc)file_print_callback, 0);
		XtAddCallback(w, XmNcancelCallback,
				(XtCallbackProc)file_cancel_callback, 0);
		XtManageChild(w);
		return;
	}
	print();
	destroy_print_popup();
}


/*ARGSUSED*/
static void file_print_callback(
	Widget				widget,
	int				item,
	XmFileSelectionBoxCallbackStruct*data)
{
	char	*p = 0;

	if (!XmStringGetLtoR(data->value,XmSTRING_DEFAULT_CHARSET,&p) || !*p) {
		create_error_popup(shell, 0, "No file name, aborted.");
		if (p) XtFree(p);
		return;
	}
	if (pref.pfile)
		free(pref.pfile);
	pref.pfile = mystrdup(p);
	print();
	XtFree(p);
	destroy_print_popup();
}


/*ARGSUSED*/
static void file_cancel_callback(
	Widget				widget,
	int				item,
	XmFileSelectionBoxCallbackStruct*data)
{
	XtDestroyWidget(widget);
}


/*ARGSUSED*/
static void config_callback(
	Widget				widget,
	int				code,
	XmToggleButtonCallbackStruct	*data)
{
	struct menu			*mp;

	if (!data->set)
		return;
	for (mp=menu; mp->type; mp++)
		if (code == mp->code)
			break;
	if (!mp->type)
		return;
	*mp->ptr = mp->value;
	modified = TRUE;
}
