/*
 * Create and destroy default query menu and all widgets in them. All widgets
 * are labels or pushbuttons; they are faster than text buttons. Whenever
 * the user presses a button with text in it, it is overlaid with a Text
 * button. The query menu is started from the form editor, default queries
 * are expressions attached to a form. They appear in the Query pulldown.
 *
 *	add_dquery(form)		apend a query entry to the form's list
 *	destroy_query_window()		remove query popup
 *	create_query_window()		create query popup
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <pwd.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define NCOLUMNS	4		/* # of widget columns in query list */

static void create_query_rows(void);
static void edit_query_button(BOOL, int, int);
static void got_query_text(int, int, char *);
static void draw_row(int);
static void delete_callback(Widget, int, XmToggleButtonCallbackStruct *);
static void dupl_callback  (Widget, int, XmToggleButtonCallbackStruct *);
static void done_callback  (Widget, int, XmToggleButtonCallbackStruct *);
static void list_callback  (Widget, int, XmToggleButtonCallbackStruct *);
static void got_text       (Widget, int, XmToggleButtonCallbackStruct *);

static FORM		*form;		/* form whose queries are edited */
static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static Widget		delete, dupl;	/* buttons, for desensitizing */
static Widget		info;		/* info line for error messages */
static Widget		textwidget;	/* if editing, text widget; else 0 */
static int		have_nrows;	/* # of table widget rows allocated */
static int		xedit, yedit;	/* if editing, column/row */
static int		ycurr;		/* current row, 0=none, 1=1st query..*/
static Widget		qlist;		/* query list RowColumn widget */
static Widget	(*qtable)[NCOLUMNS];	/* all widgets in query list table */
					/* [0][*] is title row */



/*
 * add a query entry. This is used when a new entry is started by pressing on
 * an empty row, or when duplicating an entry, or when a form file is read.
 */

DQUERY *add_dquery(
	FORM		*fp)		/* form to add blank entry to */
{
	int n = ++fp->nqueries * sizeof(DQUERY);
	if (!(fp->query = (DQUERY *)(fp->query ? realloc((char *)fp->query, n)
					       : malloc(n))))
		fatal("no memory for query");
	mybzero((void *)&fp->query[fp->nqueries-1], sizeof(DQUERY));
	return(&fp->query[fp->nqueries-1]);
}


/*
 * destroy the popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_query_window(void)
{
	if (have_shell) {
		XtPopdown(shell);
		XtDestroyWidget(shell);
		have_shell = FALSE;
	}
}


/*
 * create a query popup as a separate application shell.
 */

void create_query_window(
	FORM			*newform)	/*form whose queries are chgd*/
{
	Widget			wform, scroll, w;
	Arg			args[15];
	int			n;
	Atom			closewindow;

	destroy_query_window();
	if (have_shell)
		return;
	form = newform;
	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmUNMAP);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("Default Queries", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);
	wform = XtCreateWidget("queryform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(wform, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"queries");

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	delete = w = XtCreateManagedWidget("Delete", xmPushButtonWidgetClass,
			wform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)delete_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"dq_delete");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	dupl = w = XtCreateManagedWidget("Duplicate", xmPushButtonWidgetClass,
			wform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)dupl_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"dq_dupl");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Done", xmPushButtonWidgetClass,
			wform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)done_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"dq_done");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Help", xmPushButtonWidgetClass,
			wform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)help_callback, (XtPointer)"queries");
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"queries");

							/*-- infotext -- */
	n = 0;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNeditMode,		XmMULTI_LINE_EDIT);	n++;
	XtSetArg(args[n], XmNalignment,		XmALIGNMENT_BEGINNING);	n++;
	XtSetArg(args[n], XmNeditable,		FALSE);			n++;
	XtSetArg(args[n], XmNrows,		2);			n++;
	info = XmCreateScrolledText(wform, "info", args, n);

							/*-- scroll --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		info);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w);			n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		580);			n++;
	XtSetArg(args[n], XmNheight,		300);			n++;
	XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);		n++;
	scroll = XtCreateWidget("qscroll", xmScrolledWindowWidgetClass,
			wform, args, n);
	XtAddCallback(scroll, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"queries");

	n = 0;
	qlist = XtCreateManagedWidget("qlist", xmBulletinBoardWidgetClass,
			scroll, args, n);

	create_query_rows();	/* have_shell must be FALSE here */
	print_query_info();

	XtManageChild(info);
	XtManageChild(scroll);
	XtManageChild(wform);
	XtPopup(shell, XtGrabNone);

	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)done_callback, (XtPointer)shell);
	have_shell = TRUE;
}


/*
 * makes sure there are enough widget rows for query entries. Also makes
 * sure that there aren't too many, for speed reasons. Allocate one extra
 * widget row for the title at the top. All the text buttons are
 * label widgets. For performance reasons, they are overlaid by a text
 * widget when pressed.
 * No text is printed into the buttons, this is done later by draw_row().
 */

static short cell_x    [NCOLUMNS] = {  4, 44,  84,  284 };
static short cell_xs   [NCOLUMNS] = { 30, 30, 200, 1000 };
static char *cell_name [NCOLUMNS] = { "on", "def", "Name", "Query Expression"};

static void create_query_rows(void)
{
	int			nrows = form->nqueries+3 - form->nqueries%3;
	int			x, y;
	Arg			args[15];
	int			n;
	char			*name;
	WidgetClass		class;

	if (!have_shell)				/* check # of rows: */
		have_nrows = 0;
	if (nrows <= have_nrows)
		return;

	n = (nrows+1) * NCOLUMNS * sizeof(Widget *);
	if (qtable && !(qtable = (Widget (*)[])realloc(qtable, n)) ||
	   !qtable && !(qtable = (Widget (*)[])malloc(n)))
		fatal("no memory");

	for (x=0; x < NCOLUMNS; x++) {
	    for (y=have_nrows; y <= nrows; y++) {
		XtUnmanageChild(qlist);
		name  = cell_name[x];
		class = xmPushButtonWidgetClass;
		n = 0;
		if (y) {
			if (x < 2) {
				class = xmToggleButtonWidgetClass;
				XtSetArg(args[n], XmNselectColor,
						color[COL_TOGGLE]);	n++;
			}
			if (x == 1) {
				XtSetArg(args[n], XmNindicatorType,
						XmONE_OF_MANY);		n++;
			}
			name  = " ";
		} else
			class = xmLabelWidgetClass;

		XtSetArg(args[n], XmNx,			cell_x[x]);	n++;
		XtSetArg(args[n], XmNy,			10 + 30*y);	n++;
		XtSetArg(args[n], XmNwidth,		cell_xs[x]);	n++;
		XtSetArg(args[n], XmNheight,		30);		n++;
		XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING);	n++;
		XtSetArg(args[n], XmNrecomputeSize,	False);		n++;
		XtSetArg(args[n], XmNtraversalOn,	True);		n++;
		XtSetArg(args[n], XmNhighlightThickness,0);		n++;
		XtSetArg(args[n], XmNshadowThickness,	x > 1 && y);	n++;
		qtable[y][x] = XtCreateManagedWidget(name, class,
				qlist, args, n);
		if (y)
			XtAddCallback(qtable[y][x],
				x > 1 ? XmNactivateCallback
				      : XmNvalueChangedCallback,
				(XtCallbackProc)list_callback,
				(XtPointer)((long)(x + y * NCOLUMNS)));
		XtAddCallback(qtable[y][x], XmNhelpCallback,
				(XtCallbackProc)help_callback,
				(XtPointer)"queries");
	    }
	}
	for (y=have_nrows; y <= nrows; y++)
		draw_row(y);
	have_nrows = nrows;

	XtManageChild(qlist);
}


/*-------------------------------------------------- editing ----------------*/
/*
 * turn a text label into a Text button, to allow user input. This is done
 * by simply installing a text widget on top of the label widget. The proper
 * query name or expression is put into the text widget. The previously edited
 * button is un-edited.
 */

static void edit_query_button(
	BOOL			doedit,		/* TRUE=edit, FALSE=unedit */
	int			x,		/* column, 0..NCOLUMNS-1* */
	int			y)		/* row, y=0: title */
{
	Arg			args[15];
	int			n;
	char			*text;

	if (textwidget) {
		char *string = XmTextGetString(textwidget);
		got_query_text(xedit, yedit, string);
		XtFree(string);
		XtDestroyWidget(textwidget);
		draw_row(yedit);
		create_query_rows();
	}
	textwidget = 0;
	if (!doedit)
		return;

	if (y > form->nqueries+1)
		y = form->nqueries+1;
	n = 0;
	XtSetArg(args[n], XmNx,			cell_x[x]);		n++;
	XtSetArg(args[n], XmNy,			10 + 30*y);		n++;
	XtSetArg(args[n], XmNwidth,		cell_xs[x]);		n++;
	XtSetArg(args[n], XmNheight,		30);			n++;
	XtSetArg(args[n], XmNrecomputeSize,	False);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNshadowThickness,	1);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	textwidget = XtCreateManagedWidget("text", xmTextWidgetClass,
			qlist, args, n);
	XtAddCallback(textwidget, XmNactivateCallback,
			(XtCallbackProc)got_text, (XtPointer)0);
	XtAddCallback(textwidget, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"queries");
	XmProcessTraversal(textwidget, XmTRAVERSE_CURRENT);

	text = y > form->nqueries ? "" : x == 2 ? form->query[y-1].name
						: form->query[y-1].query;
	print_text_button_s(textwidget, text);
	xedit = x;
	yedit = y;
}

static void got_query_text(
	int		x,		/* column, 0..NCOLUMNS-1* */
	int		y,		/* row, y=0: title */
	char		*string)	/* text entered by user */
{
	register DQUERY	*dq;		/* query entry */

	if (!y--)
		return;
	if (y >= form->nqueries) {
		(void)add_dquery(form);
		y = form->nqueries - 1;
	}
	dq = &form->query[y];
	if (x == 2) {					/* name */
		if (dq->name)
			free(dq->name);
		dq->name = mystrdup(string);
	} else {					/* query expr */
		if (dq->query)
			free(dq->query);
		dq->query = mystrdup(string);
	}
}


/*
 * draw all buttons of row y. y must be > 0 because 0 is the title row.
 * If y is > form->nqueries, the row is blanked.
 */

static void draw_row(
	int		y)
{
	Arg		arg;
	register DQUERY	*dq = &form->query[y-1];

	if (y < 1)
		return;
	if (y <= form->nqueries) {				/* valid row */
		XtSetArg(arg, XmNset, !dq->suspended);
		XtSetValues (qtable[y][0], &arg, 1);
		XtSetArg(arg, XmNset, form->autoquery == y-1);
		XtSetValues (qtable[y][1], &arg, 1);
		print_button(qtable[y][2], dq->name  ? dq->name  : " ");
		print_button(qtable[y][3], dq->query ? dq->query : " ");
	} else {						/* blank row */
		XtSetArg(arg, XmNset, 0);
		XtSetValues (qtable[y][0], &arg, 1);
		XtSetValues (qtable[y][1], &arg, 1);
		print_button(qtable[y][2], " ");
		print_button(qtable[y][3], " ");
	}
}


/*
 * print interesting info into the message line. This is done only once, when
 * the menu is created.
 */

void print_query_info(void)
{
	char		msg[4096];	/* message buffer */
	int		i, j, n;	/* index to next free char in msg */
	char		comma = ' ';	/* delimiter between message items */
	int		item;		/* item (field) counter */
	ITEM		*ip;		/* current item (field) */

	strcpy(msg, "Fields:");
	i = j = strlen(msg);
	for (item=0; item < form->nitems; item++) {
		ip = form->items[item];
		switch(ip->type) {
		  case IT_INPUT:
		  case IT_TIME:
		  case IT_NOTE:
			sprintf(msg+i, "%c %s", comma, ip->name);
			break;

		  case IT_FLAG:
		  case IT_CHOICE:
			sprintf(msg+i, "%c %s=%s", comma, ip->name,
							  ip->flagcode);
			break;

		  default:
			continue;
		}
		comma = ',';
		n = strlen(msg+i);
		i += n;
		j += n;
		if (j > 60) {
			comma = '\n';
			j = 0;
		}
		if (i > sizeof(msg)-100)
			break;
	}
	if (comma == ' ')
		strcpy(msg+i, "none");
	print_text_button(info, msg);
}



/*-------------------------------------------------- callbacks --------------*/
/*
 * Delete, Duplicate, and Done buttons.
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void delete_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	int				n;
	Arg				args;

	if (ycurr && ycurr <= form->nqueries) {
		edit_query_button(FALSE, 0, 0);
		for (n=ycurr-1; n < form->nqueries; n++)
			form->query[n] = form->query[n+1];
		form->nqueries--;
		for (n=ycurr; n <= have_nrows; n++)
			draw_row(n);
	}
	if (!ycurr) {
		XtSetArg(args, XmNsensitive, 0);
		XtSetValues(delete, &args, 1);
		XtSetValues(dupl,   &args, 1);
	}
}


/*ARGSUSED*/
static void dupl_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	int				n;

	if (form->nqueries) {
		edit_query_button(FALSE, 0, 0);
		(void)add_dquery(form);
		create_query_rows();
		for (n=form->nqueries-1; n > ycurr-1; n--)
			form->query[n] = form->query[n-1];
		form->query[n].name  = mystrdup(form->query[n].name);
		form->query[n].query = mystrdup(form->query[n].query);
		for (n=ycurr; n <= form->nqueries; n++)
			draw_row(n);
	}
}

/*ARGSUSED*/
static void done_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	edit_query_button(FALSE, 0, 0);
	destroy_query_window();
	remake_query_pulldown();
}


/*
 * one of the buttons in the list was pressed
 */

/*ARGSUSED*/
static void list_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	int				x = item % NCOLUMNS;
	int				y = item / NCOLUMNS;
	Arg				arg;
	int				i;

	if (y > form->nqueries) {				/* new entry */
		x = 2;
		edit_query_button(TRUE, x, ycurr = form->nqueries+1);
	} else {						/* old entry */
		ycurr = y;
		switch(x) {
		  case 0:
			form->query[y-1].suspended = !data->set;
			break;
		  case 1:
			i = form->autoquery;
			form->autoquery = y-1;
			draw_row(i+1);
			draw_row(y);
			break;
		  default:
			edit_query_button(TRUE, x, y);
		}
	}
	XtSetArg(arg, XmNsensitive, ycurr > 0);
	XtSetValues(delete, &arg, 1);
	XtSetValues(dupl,   &arg, 1);
}


/*
 * the user pressed Return in a text entry button
 */

/*ARGSUSED*/
static void got_text(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	edit_query_button(FALSE, 0, 0);
}
