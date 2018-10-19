/*
 * draw card canvas. This canvas is used by the form editor to allow the
 * user to position items in a form and to set their size. The canvas is
 * drawn using Xlib calls; items are represented by colored rectangles.
 *
 *	destroy_card_menu(card)
 *	create_card_menu(form, dbase, wform)
 *	card_readback_texts(card, nitem)
 *	format_time_data(data, timefmt)
 *	fillout_card(card, deps)
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <assert.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelP.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include <X11/StringDefs.h>
#include <time.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void create_item_widgets(CARD *, int);
static void mwm_quit_callback
			(Widget, XtPointer, XmToggleButtonCallbackStruct *);
static void chart_expose_callback
			(Widget, XtPointer, XmDrawingAreaCallbackStruct *);
static void card_callback(Widget, XtPointer, XmToggleButtonCallbackStruct *);
void card_readback_texts(CARD *, int);
static BOOL store(CARD *, int, char *);


extern Display		*display;	/* everybody uses the same server */
extern GC		gc;		/* everybody uses this context */
extern XtAppContext	app;		/* application handle, for actions */
extern struct pref	pref;		/* global preferences */
extern CARD 		*curr_card;	/* card being displayed in main win */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern Widget		mainwindow;	/* popup menus hang off main window */
extern Widget		w_summary;	/* form for summary table */
extern char		*switch_name;	/* if switch stmt was found, name to */
extern char		*switch_expr;	/* .. switch to and search expression*/


/*
 * Read back any unread text widgets. Next, destroy the card widgets, and
 * the window if there is one (there is one if create_card_menu() was called
 * with wform==0). All the associated data structures are cleared, except
 * the referenced database, if any. Do not use the CARD struct after calling
 * destroy_card_menu().
 */

void destroy_card_menu(
	register CARD	*card)		/* card to destroy */
{
	int		i;		/* item counter */

	if (!card)
		return;
	card_readback_texts((CARD *)card, -1);
	XtUnmanageChild(XtParent(card->wform));
	XtDestroyWidget(card->wform);
	if (card->shell)
		XtPopdown(card->shell);
	for (i=0; i < card->nitems; i++) {
		card->items[i].w0 = 0;
		card->items[i].w1 = 0;
	}
	card->shell = card->wform = 0;
}


/*
 * create a card, based on the form struct (which describes structure and
 * layout) and a database struct (which describes the contents). The database
 * is not accessed, it is stored to permit later entry through the form's
 * callbacks into the database. In addition, a form widget to install into
 * must be given. If a null form widget is passed, this routine creates a
 * new window with a form widget in it. The new card is not filled out.
 * A card data structure is returned that allows printing into the card,
 * entering data into the database through the card, and destroying the card.
 */

CARD *create_card_menu(
	FORM		*form,		/* form that controls layout */
	DBASE		*dbase,		/* database for callbacks, or 0 */
	Widget		wform)		/* form widget to install into, or 0 */
{
	CARD		*card;		/* new card being allocated */
	int		xs, ys, ydiv;	/* card size and divider */
	Arg		args[15];
	int		n;
	Atom		closewindow;

							/*-- alloc card --*/
	n = sizeof(CARD) + sizeof(struct carditem) * form->nitems;
	if (!(card = (CARD *)malloc(n)))
		return((CARD *)0);
	mybzero((void *)card, n);
	card->form   = form;
	card->dbase  = dbase;
	card->row    = -1;
	card->nitems = form->nitems;
	if (!mainwindow)
		return((CARD *)card);
	xs   = pref.scale * form->xs;
	ys   = pref.scale * form->ys;
	ydiv = pref.scale * form->ydiv;
							/*-- make form --*/
	if (wform) {
		XtUnmanageChild(wform);
		n = 0;
		XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNwidth,		xs+6);		n++;
		XtSetArg(args[n], XmNheight,		ys+6);		n++;
		XtSetArg(args[n], XmNscrollingPolicy,	XmAUTOMATIC);	n++;
		XtSetArg(args[n], XmNresizable,		False);		n++;
		card->wform = XtCreateManagedWidget("wform",
				xmFormWidgetClass, wform, args, 0);
		XtManageChild(wform);
	} else {
		n = 0;
		XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);	n++;
		XtSetArg(args[n], XmNiconic,		False);		n++;
		card->shell = XtAppCreateShell("Card", "Grok",
				applicationShellWidgetClass, display, args, n);
		set_icon(card->shell, 1);
		n = 0;
		XtSetArg(args[n], XmNwidth,		xs+6);		n++;
		XtSetArg(args[n], XmNheight,		ys+6);		n++;
		XtSetArg(args[n], XmNresizable,		False);		n++;
		card->wform = XtCreateManagedWidget("wform",
				xmFormWidgetClass, card->shell, args, n);
		XtPopup(card->shell, XtGrabNone);
		closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
		XmAddWMProtocolCallback(card->shell, closewindow,
				(XtCallbackProc)mwm_quit_callback, NULL);
	}
	XtManageChild(card->wform);
	card->wstat = card->wcard = 0;
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	  XmATTACH_FORM);	n++;
	if (ydiv) {
	    if (ydiv >= ys) {
	     XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM);	n++;
	    }
	    XtSetArg(args[n], XmNleftAttachment,  XmATTACH_FORM);	n++;
	    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);	n++;
	    XtSetArg(args[n], XmNwidth,		  xs+6);		n++;
	    XtSetArg(args[n], XmNheight,	  ydiv);		n++;
	    XtSetArg(args[n], XmNresizable,	  FALSE);		n++;
	    card->wstat = XtCreateManagedWidget("staticform",
			xmFormWidgetClass, card->wform, args, n);
	    n = 0;
	    XtSetArg(args[n], XmNtopAttachment,	  XmATTACH_WIDGET);	n++;
	    XtSetArg(args[n], XmNtopWidget,	  card->wstat);		n++;
	    XtSetArg(args[n], XmNtopOffset,	  8);			n++;
	}
	if (ydiv < ys) {
	    XtSetArg(args[n], XmNleftAttachment,  XmATTACH_FORM);	n++;
	    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);	n++;
	    XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);	n++;
	    XtSetArg(args[n], XmNwidth,		  xs+6);		n++;
	    XtSetArg(args[n], XmNheight,	  ys-ydiv+6);		n++;
	    XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC);		n++;
	    XtSetArg(args[n], XmNshadowType,	  XmSHADOW_IN);		n++;
	    XtSetArg(args[n], XmNresizable,	  FALSE);		n++;
	    card->wcard = XtCreateManagedWidget("cardframe",
	   		xmFrameWidgetClass, card->wform, args, n);
	    card->wcard = XtCreateManagedWidget("cardform",
			xmFormWidgetClass, card->wcard, NULL, 0);
	    n = 0;
	    XtSetArg(args[n], XmNwidth,		  xs);			n++;
	    XtSetArg(args[n], XmNheight,	  ys-ydiv);		n++;
	    XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	    (void)XtCreateManagedWidget("",
			xmLabelWidgetClass, card->wcard, args, n);
	}
							/*-- create items --*/
	for (n=0; n < card->nitems; n++)
		create_item_widgets(card, n);
	return(card);
}


/*
 * create the widgets in the card menu, at the location specified by the
 * item in the form. The resulting widgets are stored in the card item.
 */

#define JUST(j) (j==J_LEFT   ? XmALIGNMENT_BEGINNING :	\
		 j==J_RIGHT  ? XmALIGNMENT_END		\
			     : XmALIGNMENT_CENTER)

static void create_item_widgets(
	CARD		*card,		/* card the item is added to */
	int		nitem)		/* number of item being added */
{
	ITEM		item;		/* describes type and geometry */
	struct carditem	*carditem;	/* widget pointers stored here */
	Widget		wform;		/* static part form, or card form */
	static int	did_register;	/* drawing area action registered? */
	static BOOL	have_fonts;
	static XmFontList ftlist[F_NFONTS];
	Arg		args[15];
	int		n, i;
	XmString	label, blank;
	BOOL		editable;
	XtActionsRec	action;
	String		translations =
		"<Btn1Down>:	chart(down)	ManagerGadgetArm()	\n\
		 <Btn1Up>:	chart(up)	ManagerGadgetActivate()	\n\
		 <Btn1Motion>:	chart(motion)	ManagerGadgetButtonMotion()";

	item = *card->form->items[nitem];
	if (item.y < card->form->ydiv) {		/* static or card? */
		wform  = card->wstat;
	} else {
		wform  = card->wcard;
		item.y -= card->form->ydiv;
	}
	item.x  *= pref.scale;
	item.y  *= pref.scale;
	item.xs *= pref.scale;
	item.ys *= pref.scale;
	item.xm *= pref.scale;
	item.ym *= pref.scale;

	if (!wform)
		return;

	carditem = &card->items[nitem];
	carditem->w0 = carditem->w1 = 0;
	if (evalbool(card, item.invisible_if))
		return;
	label = item.label ? XmStringCreateSimple(item.label) : 0;
	blank = XmStringCreateSimple(" ");
	editable = item.type != IT_PRINT
			&& (!card->dbase || !card->dbase->rdonly)
			&& !card->form->rdonly
			&& !item.rdonly
			&& !evalbool(card, item.freeze_if);

	if (!have_fonts++)
		for (n=0; n < F_NFONTS; n++) {
			switch(n) {
			  case F_HELV:	   i = FONT_HELV;	break;
			  case F_HELV_O:   i = FONT_HELV_O;	break;
			  case F_HELV_S:   i = FONT_HELV_S;	break;
			  case F_HELV_L:   i = FONT_HELV_L;	break;
			  case F_COURIER:  i = FONT_COURIER;	break;
			}
			ftlist[n] = XmFontListCreate(font[i], "cset");
		}
	switch(item.type) {
	  case IT_LABEL:			/* a text without function */
		n = 0;
		XtSetArg(args[n], XmNx,		 item.x);		   n++;
		XtSetArg(args[n], XmNy,		 item.y);		   n++;
		XtSetArg(args[n], XmNwidth,	 item.xs);		   n++;
		XtSetArg(args[n], XmNheight,	 item.ys);		   n++;
		XtSetArg(args[n], XmNalignment,	 JUST(item.labeljust));    n++;
		XtSetArg(args[n], XmNlabelString,label);		   n++;
		XtSetArg(args[n], XmNfontList,	 ftlist[item.labelfont]);  n++;
		XtSetArg(args[n], XmNhighlightThickness, 0);		   n++;
		carditem->w0 = XtCreateManagedWidget("label",
					xmLabelWidgetClass, wform, args, n);
		break;

	  case IT_INPUT:			/* arbitrary line of text */
	  case IT_PRINT:			/* non-editable text */
	  case IT_TIME:				/* date and/or time */
		if (item.xm > 6) {
		  n = 0;
		  XtSetArg(args[n], XmNx,	  item.x);		   n++;
		  XtSetArg(args[n], XmNy,	  item.y);		   n++;
		  XtSetArg(args[n], XmNwidth,	  item.xm - 6);		   n++;
		  XtSetArg(args[n], XmNheight,	  item.ys);		   n++;
		  XtSetArg(args[n], XmNalignment, JUST(item.labeljust));   n++;
		  XtSetArg(args[n], XmNlabelString,label);		   n++;
		  XtSetArg(args[n], XmNfontList,  ftlist[item.labelfont]); n++;
		  XtSetArg(args[n], XmNhighlightThickness, 0);		   n++;
		  carditem->w1 = XtCreateManagedWidget("label",
				xmLabelWidgetClass, wform, args, n);
		}
		n = 0;
		XtSetArg(args[n], XmNx,		 item.x  + item.xm);	   n++;
		XtSetArg(args[n], XmNy,		 item.y);		   n++;
		XtSetArg(args[n], XmNwidth,	 item.xs - item.xm);	   n++;
		XtSetArg(args[n], XmNheight,	 item.ys);		   n++;
		XtSetArg(args[n], XmNalignment,	 JUST(item.inputjust));    n++;
		XtSetArg(args[n], XmNlabelString,blank);	 	   n++;
		XtSetArg(args[n], XmNfontList,	 ftlist[item.inputfont]); n++;
		XtSetArg(args[n], XmNmaxLength,	 item.maxlen?item.maxlen:10);
									   n++;
		XtSetArg(args[n], XmNmarginHeight, 2);			   n++;
		XtSetArg(args[n], XmNeditable,   editable);		   n++;
		XtSetArg(args[n], XmNpendingDelete, True);		   n++;
		XtSetArg(args[n], XmNbackground, color[editable ?
						 COL_TEXTBACK:COL_BACK]);  n++;
		carditem->w0 = XtCreateManagedWidget("input",
					xmTextWidgetClass, wform, args, n);
		if (editable)
			XtAddCallback(carditem->w0, XmNactivateCallback,
				(XtCallbackProc)card_callback,(XtPointer)card);
		break;

	  case IT_NOTE:				/* multi-line text */
		n = 0;
		XtSetArg(args[n], XmNx,		 item.x);		   n++;
		XtSetArg(args[n], XmNy,		 item.y);		   n++;
		XtSetArg(args[n], XmNwidth,	 item.xs);		   n++;
		XtSetArg(args[n], XmNheight,	 item.ym);		   n++;
		XtSetArg(args[n], XmNalignment,	 JUST(item.labeljust));    n++;
		XtSetArg(args[n], XmNlabelString,label);		   n++;
		XtSetArg(args[n], XmNfontList,	 ftlist[item.labelfont]);  n++;
		XtSetArg(args[n], XmNhighlightThickness, 0);		   n++;
		carditem->w1 = XtCreateManagedWidget("label",
					xmLabelWidgetClass, wform, args, n);
		n = 0;
		XtSetArg(args[n], XmNx,		 item.x);		   n++;
		XtSetArg(args[n], XmNy,		 item.y + item.ym);	   n++;
		XtSetArg(args[n], XmNwidth,	 item.xs);		   n++;
		XtSetArg(args[n], XmNheight,	 item.ys - item.ym);	   n++;
		XtSetArg(args[n], XmNhighlightThickness, 1);		   n++;
		XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC);	   n++;
		carditem->w0 = XtCreateManagedWidget("noteSW",
					xmScrolledWindowWidgetClass, wform,
					args, n);
		n = 0;
		XtSetArg(args[n], XmNfontList,	 ftlist[item.inputfont]);  n++;
		XtSetArg(args[n], XmNeditMode,	 XmMULTI_LINE_EDIT);	   n++;
		XtSetArg(args[n], XmNmaxLength,	 item.maxlen);		   n++;
		XtSetArg(args[n], XmNalignment,	 JUST(item.inputjust));    n++;
		XtSetArg(args[n], XmNhighlightThickness, 0);		   n++;
		XtSetArg(args[n], XmNshadowThickness, 0);		   n++;
		carditem->w0 = XtCreateWidget("note",
				xmTextWidgetClass, carditem->w0, args, n);
		if (editable)
			XtAddCallback(carditem->w0, XmNactivateCallback,
				(XtCallbackProc)card_callback,(XtPointer)card);
		break;

	  case IT_CHOICE:			/* diamond on/off switch */
	  case IT_FLAG:				/* square on/off switch */
		n = 0;
		XtSetArg(args[n], XmNx,		 item.x);		   n++;
		XtSetArg(args[n], XmNy,		 item.y);		   n++;
		XtSetArg(args[n], XmNwidth,	 item.xs);		   n++;
		XtSetArg(args[n], XmNheight,	 item.ys);		   n++;
		XtSetArg(args[n], XmNalignment,	 JUST(item.labeljust));    n++;
		XtSetArg(args[n], XmNselectColor,color[COL_TOGGLE]);	   n++;
		XtSetArg(args[n], XmNlabelString,label);		   n++;
		XtSetArg(args[n], XmNfontList,	 ftlist[item.labelfont]);  n++;
		XtSetArg(args[n], XmNhighlightThickness, 1);		   n++;
		XtSetArg(args[n], XmNindicatorType, item.type == IT_CHOICE ?
					XmONE_OF_MANY : XmN_OF_MANY);	   n++;
		carditem->w0 = XtCreateManagedWidget("label",
				xmToggleButtonWidgetClass, wform,
				args, n);
		if (card->dbase && !card->dbase->rdonly
				&& !card->form->rdonly
				&& !item.rdonly)
			XtAddCallback(carditem->w0, XmNvalueChangedCallback,
				(XtCallbackProc)card_callback,(XtPointer)card);
		break;

	  case IT_BUTTON:			/* pressable button */
		n = 0;
		XtSetArg(args[n], XmNx,		 item.x);		   n++;
		XtSetArg(args[n], XmNy,		 item.y);		   n++;
		XtSetArg(args[n], XmNwidth,	 item.xs);		   n++;
		XtSetArg(args[n], XmNheight,	 item.ys);		   n++;
		XtSetArg(args[n], XmNfontList,	 ftlist[item.labelfont]);  n++;
		XtSetArg(args[n], XmNhighlightThickness, 1);		   n++;
		XtSetArg(args[n], XmNlabelString,label);		   n++;
		carditem->w0 = XtCreateManagedWidget("button",
				xmPushButtonWidgetClass, wform, args, n);
		XtAddCallback(carditem->w0, XmNactivateCallback,
				(XtCallbackProc)card_callback,(XtPointer)card);
		break;

	  case IT_CHART:			/* chart display */
		if (!did_register++) {
			action.string = "chart";
			action.proc   = (XtActionProc)chart_action_callback;
			XtAppAddActions(app, &action, 1);
		}
		n = 0;
		XtSetArg(args[n], XmNx,		 item.x);		   n++;
		XtSetArg(args[n], XmNy,		 item.y);		   n++;
		XtSetArg(args[n], XmNwidth,	 item.xs);		   n++;
		XtSetArg(args[n], XmNheight,	 item.ys);		   n++;
		XtSetArg(args[n], XmNhighlightThickness, 1);		   n++;
		XtSetArg(args[n], XmNlabelString,label);		   n++;
		XtSetArg(args[n], XmNtranslations,
				XtParseTranslationTable(translations));	   n++;
		carditem->w0 = XtCreateManagedWidget("chart",
				xmDrawingAreaWidgetClass, wform, args,n);
		XtAddCallback(carditem->w0, XmNinputCallback,
				(XtCallbackProc)card_callback,(XtPointer)card);
		XtAddCallback(carditem->w0, XmNexposeCallback,
				(XtCallbackProc)chart_expose_callback,
							(XtPointer)card);
		break;

	  case IT_VIEW:				/* database summary & card */
		break;
	}
	if (label)
		XmStringFree(label);
	XmStringFree(blank);
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void mwm_quit_callback(
	Widget				widget,
	XtPointer			card,
	XmToggleButtonCallbackStruct	*data)
{
	XtPopdown(widget);
	XtDestroyWidget(widget);
	free((void *)card);
}


/*
 * a widget with a chart is exposed or otherwise redrawn. Motif can't do
 * this automatically like with all other types of widgets because charts
 * are drawn with raw Xlib. Do it here.
 */

/*ARGSUSED*/
static void chart_expose_callback(
	Widget				widget,
	XtPointer			icard,
	XmDrawingAreaCallbackStruct	*data)
{
	XEvent		dummy;
	CARD		*card = (CARD *)icard;
	int		nitem;

	while (XCheckWindowEvent(display, data->window, ExposureMask, &dummy));
	for (nitem=0; nitem < card->nitems; nitem++)
		if (widget == card->items[nitem].w0 ||
		    widget == card->items[nitem].w1)
			break;
	if (nitem >= card->nitems ||			/* illegal */
	    card->dbase == 0	  ||			/* preview dummy card*/
	    card->row < 0)				/* card still empty */
		return;

	draw_chart(card, nitem);
}


/*
 * the user pressed the mouse or the return key on an item in the card.
 * If the card references a row in a database, use store() to change the
 * column of that row that is referenced by the item. Redraw the entire
 * card (because a Choice item turns off other Choice items). This is
 * used for all item types except charts, which are more complicated
 * because of drawing and get their own callback, chart_action_callback.
 */

/*ARGSUSED*/
static void card_callback(
	Widget				widget,
	XtPointer			icard,
	XmToggleButtonCallbackStruct	*data)
{
	CARD		*card = (CARD *)icard;
	ITEM		*item;
	int		nitem, i;
	char		*n, *o;

	for (nitem=0; nitem < card->nitems; nitem++)
		if (widget == card->items[nitem].w0 ||
		    widget == card->items[nitem].w1)
			break;
	if (nitem >= card->nitems ||			/* illegal */
	    card->dbase == 0	  ||			/* preview dummy card*/
	    card->row < 0)				/* card still empty */
		return;

	item = card->form->items[nitem];
	switch(item->type) {
	  case IT_INPUT:				/* arbitrary input */
	  case IT_TIME:					/* date and/or time */
	  case IT_NOTE:					/* multi-line text */
		i = (nitem+1) % card->form->nitems;
		for (; i != nitem; i=(i+1)%card->form->nitems) {
			int t = card->form->items[i]->type;
			if ((t == IT_INPUT || t == IT_TIME || t == IT_NOTE) &&
			    !evalbool(card, card->form->items[i]->skip_if))
				break;
		}
		XmProcessTraversal(card->items[i].w0, XmTRAVERSE_CURRENT);
		card_readback_texts(card, nitem);
		break;

	  case IT_CHOICE:				/* diamond on/off */
		if (!store(card, nitem, item->flagcode))
			return;
		break;

	  case IT_FLAG:					/* square on/off */
		if (!(n = item->flagcode))
			return;
		o = dbase_get(card->dbase, card->row,
					   card->form->items[nitem]->column);
		if (!store(card, nitem, !o || strcmp(n, o) ? n : 0))
			return;
		break;

	  case IT_BUTTON:				/* pressable button */
		if (item->pressed) {
			if (switch_name) free(switch_name); switch_name = 0;
			if (switch_expr) free(switch_expr); switch_expr = 0;
			n = evaluate(card, item->pressed);
			if (switch_name) {
				switch_form(switch_name);
				card = curr_card;
			}
			if (switch_expr)
				search_cards(SM_SEARCH, card, switch_expr);
			if (n && *n)
				system(n);
			if (switch_name) free(switch_name); switch_name = 0;
			if (switch_expr) free(switch_expr); switch_expr = 0;
		}
		break;

	  case IT_VIEW:					/* database summary */
		break;
	}
	fillout_card(card, TRUE);
}


/*
 * read back all text fields. This is necessary whenever a card is removed
 * or redrawn because we might not have received a callback for a changed
 * text input field. We get callbacks only if the user presses Return, and
 * not even that on multi-line IT_NOTE fields.
 * The <which> parameter, if >= 0, narrows the readback to a particular item.
 */

void card_readback_texts(
	CARD		*card,		/* card that is displayed in window */
	int		which)		/* all if < 0, one item only if >= 0 */
{
	int		nitem;		/* counter for searching text items */
	int		start, end;	/* first and last item to read back */
	char		*data;		/* data string to store */
	time_t		time;		/* parsed time */
	char		buf[40];	/* if numeric time, temp string */

	if (!card || !card->form || !card->form->items || !card->dbase
		  ||  card->form->rdonly
		  ||  card->dbase->rdonly
		  ||  card->row >= card->dbase->nrows)
		return;
	start = which >= 0 ? which : 0;
	end   = which >= 0 ? which : card->nitems-1;
	for (nitem=start; nitem <= end; nitem++) {
		if (!card->items[nitem].w0		||
		    card->form->items[nitem]->rdonly)
			continue;
		switch(card->form->items[nitem]->type) {
		  case IT_INPUT:
			data = read_text_button(card->items[nitem].w0, 0);
			(void)store(card, nitem, data);
			break;

		  case IT_NOTE:
			data = read_text_button_noskipblank(
						card->items[nitem].w0, 0);
			(void)store(card, nitem, data);
			break;

		  case IT_TIME:
			data = read_text_button(card->items[nitem].w0, 0);
			while (*data == ' ' || *data == '\t' || *data == '\n')
				data++;
			if (!*data)
				*buf = 0;
			else {
				switch(card->form->items[nitem]->timefmt) {
				  case T_DATE:
					time = parse_datestring(data);
					break;
				  case T_TIME:
					time = parse_timestring(data, FALSE);
					break;
				  case T_DURATION:
					time = parse_timestring(data, TRUE);
					break;
				  case T_DATETIME:
					time = parse_datetimestring(data);
				}
				sprintf(buf, "%d", time);
			}
			(void)store(card, nitem, buf);
			if (which >= 0)
				fillout_item(card, nitem, FALSE);
		}
	}
}


/*
 * store a data item in the database. The database is just a big array of
 * rows and columns of char strings indexed by the card's row and the item
 * in the card to be set. Items reference a column. Some items, like labels,
 * do not reference any column at all, do not call store() with one of these.
 * This function may modify card->row because of resorting.
 */

static BOOL store(
	register CARD	*card,		/* card the item is added to */
	int		nitem,		/* number of item being added */
	char		*string)	/* string to store in dbase */
{
	BOOL		newsum = FALSE;	/* must redraw summary? */

	if (nitem >= card->nitems		||
	    card->dbase == 0			||
	    card->dbase->rdonly			||
	    card->form->items[nitem]->rdonly || card->form->rdonly)
		return(FALSE);

	if (!dbase_put(card->dbase, card->row,
				card->form->items[nitem]->column, string))
		return(TRUE);

	print_info_line();
	if (pref.sortcol == card->form->items[nitem]->column) {
		dbase_sort(card, pref.sortcol, pref.revsort);
		newsum = TRUE;
	} else
		replace_summary_line(card, card->row);

	if (pref.autoquery) {
		do_query(card->form->autoquery);
		newsum = TRUE;
	}
	if (newsum) {
		create_summary_menu(card, w_summary, mainwindow);
		scroll_summary(card);
	}
	return(TRUE);
}


/*-------------------------------------------------- drawing ----------------*/
/*
 * return the text representation of the data string of an IT_TIME database
 * item. This is used by fillout_item and make_summary_line.
 */

char *format_time_data(
	char		*data,		/* string from database */
	TIMEFMT		timefmt)	/* new format, one of T_* */
{
	static char	buf[40];	/* for date/time conversion */
	int		value = data ? atoi(data) : 0;

	if (!data)
		return("");
	value = atoi(data);
	switch(timefmt) {
	  case T_DATE:
		data = mkdatestring(value);
		break;
	  case T_TIME:
		data = mktimestring(value, FALSE);
		break;
	  case T_DURATION:
		data = mktimestring(value, TRUE);
		break;
	  case T_DATETIME:
		sprintf(buf, "%s  %s", mkdatestring(value),
				       mktimestring(value, FALSE));
		data = buf;
	}
	return(data);
}


/*
 * fill out a card, using the data from the row in the database referenced
 * in the card. If there is no database, print empty strings. If the deps
 * argument is true, a field has changed (and got reprinted separately), so
 * only PRINT and CHART items that may need updating are reprinted. This is
 * done to speed things up. It is called by routines that call store().
 */

void fillout_card(
	CARD		*card,		/* card to draw into menu */
	BOOL		deps)		/* if TRUE, dependencies only */
{
	int		i;		/* item counter */

	if (card) {
		card->disprow = card->row;
		for (i=0; i < card->nitems; i++)
			fillout_item(card, i, deps);
	}
	remake_section_popup(FALSE);
}


void fillout_item(
	CARD		*card,		/* card to draw into menu */
	int		i,		/* item index */
	BOOL		deps)		/* if TRUE, dependencies only */
{
	BOOL		sens;		/* (de-)sensitize item */
	register ITEM	*item;		/* describes type and geometry */
	Widget		w0, w1;		/* input widget(s) in card */
	char		*data;		/* value string in database */
	char		*eval;		/* evaluated expression, 0=error */
	Arg		arg;		/* for (de-) sensitizing */

	w0   = card->items[i].w0;
	w1   = card->items[i].w1;
	item = card->form->items[i];
	sens = item->type == IT_BUTTON && !item->gray_if ||
	       item->y < card->form->ydiv ||
	       card->dbase && card->row >= 0
			   && card->row < card->dbase->nrows
			   && !evalbool(card, item->gray_if);
	data = !sens || item->type == IT_BUTTON ? 0 :
	       dbase_get(card->dbase, card->row, card->form->items[i]->column);

	XtSetArg(arg, XmNsensitive, sens);
	if (w0) XtSetValues(w0, &arg, 1);
	if (w1) XtSetValues(w1, &arg, 1);

	switch(item->type) {
	  case IT_TIME:
		if (sens)
			data = format_time_data(data, item->timefmt);

	  case IT_INPUT:
		if (!deps) {
			print_text_button_s(w0, !sens ? " " : data ? data :
					item->idefault &&
					(eval = evaluate(card, item->idefault))
							? eval : "");
			if (w0) XmTextSetInsertionPosition(w0, 0);
		}
		break;

	  case IT_NOTE:
		if (!deps && w0) {
			XtUnmanageChild(w0);
			print_text_button_s(w0, !sens ? " " : data ? data :
					item->idefault &&
					(eval = evaluate(card, item->idefault))
							? eval : "");
			XmTextSetInsertionPosition(w0, 0);
			XtManageChild(w0);
		}
		break;

	  case IT_PRINT:
		print_text_button_s(w0, evaluate(card, item->idefault));
		break;

	  case IT_CHOICE:
	  case IT_FLAG:
		set_toggle(w0, data && item->flagcode
				    && !strcmp(data, item->flagcode));
		break;

	  case IT_CHART:
		draw_chart(card, i);
		break;

	  case IT_VIEW:
		break;
	}
}
