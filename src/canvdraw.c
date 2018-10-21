/*
 * draw card canvas. This canvas is used by the form editor to allow the
 * user to position items in a form and to set their size. The canvas is
 * drawn using Xlib calls; items are represented by colored rectangles.
 */

#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Protocols.h>
#include <X11/cursorfont.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define XSNAP(x)	((x)-(x)%form->xg)
#define YSNAP(y)	((y)-(y)%form->yg)
#define DIV_WIDTH	2
#define DIV_GRIPSZ	8
#define DIV_GRIPOFF	12

static void quit_callback  (Widget, XtPointer, XmToggleButtonCallbackStruct *);
static void expose_callback(Widget, XtPointer, XmDrawingAreaCallbackStruct *);
static void resize_callback(Widget, XtPointer, XmDrawingAreaCallbackStruct *);
static void canvas_callback(Widget, XButtonEvent *, String *, int);
static void draw_rubberband(BOOL, int, int, int, int);
static int  ifont(FONTN);

static BOOL		have_shell = FALSE;	/* week window is being displayed */
static FORM		*form;		/* current form, registered by create*/
static Widget		shell;		/* entire window */
static Widget		canvas;		/* drawing area */


/*
 * destroy the canvas window. Remove it from the screen, and destroy its
 * widgets.
 */

void destroy_canvas_window(void)
{
	if (have_shell) {
		XtPopdown(shell);
		XtDestroyWidget(shell);
		have_shell = FALSE;
	}
}


/*
 * create the canvas window with a single DrawingArea in it. Find out how
 * many pixels the window manager added for the window decoration around
 * the drawing area, so we can figure out the new drawing area size from
 * the new window size if the user resizes the window.
 * The form pointer is saved in a static variable because the callbacks
 * also need it, and I think it's easier to understand this way.
 */

void create_canvas_window(
	FORM		*f)
{
	Arg		args[15];
	int		n;
	Atom		closewindow;
	XtActionsRec	action;
	String		translations =
		"<Btn1Down>:	canvas(down)	ManagerGadgetArm()	\n\
		 <Btn1Up>:	canvas(up)	ManagerGadgetActivate()	\n\
		 <Btn1Motion>:	canvas(motion)	ManagerGadgetButtonMotion()";

	destroy_canvas_window();
	form = f;

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("Form Editor Canvas", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);

	action.string = "canvas";
	action.proc   = (XtActionProc)canvas_callback;
	XtAppAddActions(app, &action, 1);

	n = 0;
	XtSetArg(args[n], XmNwidth,		form->xs);		n++;
	XtSetArg(args[n], XmNheight,		form->ys);		n++;
	XtSetArg(args[n], XmNresizePolicy,	XmRESIZE_ANY);		n++;
	XtSetArg(args[n], XmNtranslations,
			XtParseTranslationTable(translations));		n++;

	canvas = XtCreateManagedWidget("canvas", xmDrawingAreaWidgetClass,
			shell, args, n);
	XtAddCallback(canvas, XmNexposeCallback,
			(XtCallbackProc)expose_callback, (XtPointer)0);
	XtAddCallback(canvas, XmNresizeCallback,
			(XtCallbackProc)resize_callback, (XtPointer)0);

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)quit_callback, (XtPointer)0);
	set_cursor(canvas, XC_arrow);
	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void quit_callback(
	Widget				widget,
	XtPointer			item,
	XmToggleButtonCallbackStruct	*data)
{
	create_error_popup(shell, 0, "%s%s",
		"Please press the Done button in the main\n",
		"Form Editor window to remove this window.");
}


/*ARGSUSED*/
static void expose_callback(
	Widget				w,
	XtPointer			data,
	XmDrawingAreaCallbackStruct	*info)
{
	XEvent				dummy;
	while (XCheckWindowEvent(display, info->window, ExposureMask, &dummy));
	redraw_canvas();
}


/*ARGSUSED*/
static void resize_callback(
	Widget				w,
	XtPointer			data,
	XmDrawingAreaCallbackStruct	*info)
{
	XEvent				dummy;
	Arg				args[2];
	Dimension			xs=0, ys=0;

	while (XCheckWindowEvent(display, info->window,
			ResizeRedirectMask, &dummy));
	XtSetArg(args[0], XmNwidth,  &xs);
	XtSetArg(args[1], XmNheight, &ys);
	XtGetValues(canvas, args, 2);
	form->xs = xs;
	form->ys = ys;
	redraw_canvas();
}


/*-------------------------------------------------- dragging ---------------*/
/*
 * mouse position classifications
 */

typedef enum {
	M_OUTSIDE = 0,		/* not near any item */
	M_INSIDE,		/* inside an item, but not near an edge */
	M_DIVIDER,		/* on grip between static part and card */
	M_TOP,			/* near top edge */
	M_BOTTOM,		/* near bottom edge */
	M_LEFT,			/* near left edge */
	M_RIGHT,		/* near right edge */
	M_XMID,			/* near X divider in an input/date/time item */
	M_YMID			/* near Y divider in a note/view item */
} MOUSE;

static MOUSE locate_item(int *, int, int);
static int cursorglyph[] = {
	XC_X_cursor,
	XC_fleur,
	XC_sb_v_double_arrow,
	XC_top_side,
	XC_bottom_side,
	XC_left_side,
	XC_right_side,
	XC_sb_h_double_arrow,
	XC_sb_v_double_arrow
};


/*
 * this routine is called directly from the drawing area's translation
 * table, with mouse up/down/motion events.
 */

/*ARGSUSED*/
static void canvas_callback(
	Widget		w,		/* widget, == canvas */
	XButtonEvent	*event,		/* X event, contains position */
	String		*args,		/* what happened, up/down/motion */
	int		nargs)		/* # of args, must be 1 */
{
	static int	nitem;		/* item on which pen was pressed */
	static MOUSE	mode;		/* what's being moved: M_* */
	static int	down_x, down_y;	/* pos where pen was pressed down */
	static int	state;		/* button/modkey mask when pressed */
	static BOOL	moving = FALSE;		/* this is not a selection, move box */
	ITEM		*item;		/* item being selected or moved */
	int		x, y, xs, ys;	/* new item start and size */
	int		xm, ym;		/* new item midpoint division */
	int		dx, dy;		/* movement since initial press */
	int		i, nsel;

	if (!strcmp(args[0], "down")) {
		down_x = event->x;
		down_y = event->y;
		state  = event->state;
		moving = FALSE;
		mode   = locate_item(&nitem, event->x, event->y);
		set_cursor(canvas, cursorglyph[mode]);
		return;
	}
	if (!strcmp(args[0], "motion") && (event->state & Button1Mask)
				       && mode != M_OUTSIDE) {
		x = abs(event->x - down_x);
		y = abs(event->y - down_y);
		moving |= x > 3 || y > 3;
	}
	if (moving) {
		if (mode == M_OUTSIDE)
			return;
		if (mode == M_DIVIDER) {
			x  = 0;
			y  = event->y;
			xs = form->xs;
			ys = 1;
		} else {
			item = form->items[nitem];
			x  = item->x;
			y  = item->y;
			xs = item->xs;
			ys = item->ys;
			xm = item->xm;
			ym = item->ym;
			dx = event->x - down_x;
			dy = event->y - down_y;
			switch(mode) {
			  case M_INSIDE:  x  += dx; y  += dy;	break;
			  case M_TOP:	  y  += dy; ys -= dy;	break;
			  case M_BOTTOM:  ys += dy;		break;
			  case M_LEFT:	  x  += dx; xs -= dx;	break;
			  case M_RIGHT:	  xs += dx;		break;
			  case M_XMID:	  xm  = item->xm + dx;	break;
			  case M_YMID:	  ym  = item->ym + dy;	break;
			}
			if (x  < 0)	x  = 0;		x  = XSNAP(x);
			if (y  < 0)	y  = 0;		y  = XSNAP(y);
			if (xs < 0)	xs = 0;		xs = XSNAP(xs);
			if (ys < 0)	ys = 0;		ys = XSNAP(ys);
			if (xm > xs)	xm = xs;	xm = XSNAP(xm);
			if (ym > ys)	ym = ys;	ym = XSNAP(ym);
		}
	}
	if (!strcmp(args[0], "motion")) {
	    if (moving && (event->state & Button1Mask))
		switch(mode) {
		  case M_XMID: draw_rubberband(TRUE, xm+x, y, 1, ys); break;
		  case M_YMID: draw_rubberband(TRUE, x, ym+y, xs, 1); break;
		  default:     draw_rubberband(TRUE, x, y, xs, ys);
	    }
	} else if (!strcmp(args[0], "up")) {
		draw_rubberband(FALSE, 0, 0, 0, 0);
		if (mode == M_OUTSIDE) {
			set_cursor(canvas, XC_arrow);
			return;
		}
		if (mode == M_DIVIDER) {
			int dy = form->ydiv;
			form->ydiv = event->y <  0        ? 0 :
				     event->y >= form->ys ? form->ys-1
							  : event->y;
			form->ydiv = YSNAP(form->ydiv);
			dy = form->ydiv - dy;
			for (i=0; i < form->nitems; i++)
				form->items[i]->y += dy;
			redraw_canvas();
			set_cursor(canvas, XC_arrow);
			return;
		}
		item = form->items[nitem];
		for (nsel=i=0; i < form->nitems; i++)
			nsel += form->items[i]->selected;
		if (moving) {					/* moved */
			item->x  = x;
			item->y  = y;
			item->xs = xs;
			item->ys = ys;
			item->xm = xm;
			item->ym = ym;
			redraw_canvas();
		} else {					/* selected */
			readback_formedit();
			if (state & ShiftMask) {
				item->selected ^= TRUE;		/*... multi */
				curr_item = nitem;
			} else {
				if (!item->selected || nsel > 1) {
					item_deselect(form);	/*... sel */
					item->selected = TRUE;
					curr_item = nitem;
				} else {
					item_deselect(form);	/*... unsel */
					curr_item = form->nitems;
				}
			}
			redraw_canvas_item(item);
			fillout_formedit();
			sensitize_formedit();
		}
		set_cursor(canvas, XC_arrow);
	}
}


/*
 * find the item the mouse is in or nearby, and return exactly in which way
 * it is close (left, right, top, bottom). Return the item if found, and the
 * edge found as one of M_*. This is done when the mouse is pressed down, to
 * find out what should be done, and to set the cursor shape.
 */

static MOUSE locate_item(
	int		*nitem_p,	/* set to the located item */
	int		x,		/* position in drawing area */
	int		y)
{
	int		nitem;		/* current item number */
	register ITEM	*item;		/* current item, form->items[nitem] */
	int		xc, yc;		/* current item's center pos */
	int		dx, dy;		/* current is this far away from x/y */
	int		cdx=8, cdy=8;	/* closest is this far away from x/y */
	int		closest = -1;	/* closest item so far */

							/* divider? */
	if (y > form->ydiv - DIV_GRIPSZ/2 &&
	    y < form->ydiv + DIV_GRIPSZ/2 &&
	    x > form->xs - DIV_GRIPOFF - DIV_GRIPSZ/2 &&
	    x < form->xs - DIV_GRIPOFF + DIV_GRIPSZ/2)
		return(M_DIVIDER);
							/* find closest */
	for (nitem=0; nitem < form->nitems; nitem++) {
		item = form->items[nitem];
								/* inside? */
		if (item->x+3 <= x && item->x + item->xs-3 > x &&
		    item->y+3 <= y && item->y + item->ys-3 > y) {
			closest = nitem;
			break;
		}
		dx = dy = 999;					/* close? */
		if (y >= item->y && y < item->y+item->ys) {
			dy = cdy;
			if (x <  item->x+3)	     dx = item->x+3-x;
			if (x >= item->x+item->xs-3) dx = x-item->x-item->xs+3;
		}
		if (x >= item->x && x < item->x+item->xs) {
			dx = cdx;
			if (y <  item->y+3)	     dy = item->y+3-y;
			if (y >= item->y+item->ys-3) dy = y-item->y-item->ys+3;
		}
		if (dx <= cdx && dy <= cdy) {
			cdx = dx;
			cdy = dy;
			closest = nitem;
		}
	}
	if (closest < 0)
		return(M_OUTSIDE);
	if (nitem_p)
		*nitem_p = closest;
	item = form->items[closest];
	if (nitem == closest) {					/* inside */
		if ((item->type == IT_INPUT ||
		     item->type == IT_PRINT ||
		     item->type == IT_TIME) && abs(x - item->x - item->xm) < 6)
			return(M_XMID);

		if ((item->type == IT_VIEW  ||
		     item->type == IT_CHART ||
		     item->type == IT_NOTE) && abs(y - item->y - item->ym) < 6)
			return(M_YMID);

		return(M_INSIDE);
	}
	xc = item->x + item->xs/2;				/* near edge */
	yc = item->y + item->ys/2;
	dx = x < xc ? abs(x - item->x) : abs(x - item->x-item->xs);
	dy = y < yc ? abs(y - item->y) : abs(y - item->y-item->ys);

	if (dx < dy && x <  xc)		return(M_LEFT);
	if (dx < dy && x >= xc)		return(M_RIGHT);
	if (dy < dx && y <  yc)		return(M_TOP);
	if (dy < dx && y >= yc)		return(M_BOTTOM);
	return(M_OUTSIDE); /* paranoid */
}


/*
 * draw or undraw a rubber band box into the canvas, using XOR drawing. This
 * must be called in draw-undraw pairs.
 */

static void draw_rubberband(
	BOOL		draw,		/* draw or undraw */
	int		x,		/* position of box */
	int		y,
	int		xs,		/* size of box */
	int		ys)
{
	static BOOL	is_drawn = FALSE;	/* TRUE if rubber band exists */
	static int	lx,ly,lxs,lys;	/* drawn rubberband */

	if (is_drawn) {
		is_drawn = FALSE;
		XDrawRectangle(display, XtWindow(canvas), xor_gc,
						lx, ly, lxs, lys);
	}
	if (!draw)
		return;

	XDrawRectangle(display, XtWindow(canvas), xor_gc,
					lx=x-1, ly=y-1, lxs=xs+1, lys=ys+1);
	is_drawn = TRUE;
}


/*-------------------------------------------------- drawing ----------------*/
/*
 * draw all items in the current form
 */

void redraw_canvas(void)
{
	int 		i;

	if (!have_shell)
		return;
	set_color(COL_CANVBACK);
	XFillRectangle(display, XtWindow(canvas), gc, 0,0, form->xs, form->ys);

	for (i=0; i < form->nitems; i++)
		redraw_canvas_item(form->items[i]);

	set_color(COL_CANVFRAME);
	XFillRectangle(display, XtWindow(canvas), gc, 0,
				form->ydiv - DIV_WIDTH/2, form->xs, DIV_WIDTH);
	XFillRectangle(display, XtWindow(canvas), gc,
				form->xs - DIV_GRIPOFF - DIV_GRIPSZ/2,
				form->ydiv - DIV_GRIPSZ/2,
				DIV_GRIPSZ, DIV_GRIPSZ);
	set_color(COL_CANVBACK);
	XFillRectangle(display, XtWindow(canvas), gc,
				form->xs - DIV_GRIPOFF - DIV_GRIPSZ/2 + 2,
				form->ydiv - DIV_GRIPSZ/2 + 2,
				DIV_GRIPSZ - 4, DIV_GRIPSZ - 4);
}


void undraw_canvas_item(
	register ITEM	*item)		/* item to redraw */
{
	set_color(COL_CANVBACK);
	XFillRectangle(display, XtWindow(canvas), gc,
					item->x, item->y, item->xs, item->ys);
}


static char *datatext[NITEMS] = {
	"None", "", "Print", "Input", "Time", "Note", "", "", "", "Card", "" };

void redraw_canvas_item(
	register ITEM	*item)		/* item to redraw */
{
	Window		window = XtWindow(canvas);
	char		buf[1024];	/* truncated texts */
	int		xm=-1, ym=-1;	/* middle division pos */
	int		nfont;		/* font number as FONT_* */
	char		sumcol[20];	/* summary column indicator msg */
	int		n, i;		/* for chart grid lines */

	if (!have_shell)
		return;

	draw_rubberband(FALSE, 0, 0, 0, 0);
	set_color(item->selected ? COL_CANVSEL : COL_CANVBOX);
	XFillRectangle(display, window, gc,
					item->x, item->y, item->xs, item->ys);

	set_color(COL_CANVFRAME);

	XDrawRectangle(display, window, gc,
				item->x, item->y, item->xs-1, item->ys-1);
	if (item->type == IT_INPUT || item->type == IT_PRINT
				   || item->type == IT_TIME)
		XFillRectangle(display, window, gc,
				item->x + (xm=item->xm), item->y, 1, item->ys);

	if (item->type == IT_CHART || item->type == IT_VIEW
				   || item->type == IT_NOTE)
		XFillRectangle(display, window, gc,
				item->x, item->y + (ym=item->ym), item->xs, 1);

	if (item->type == IT_CHART) {
		int ny = 10;
		int nx = 15 - ny;
		set_color(COL_CANVBACK);
		for (n=1; n < ny; n++) {
			i = item->y + ym + n * (item->ys - ym) / ny,
			XDrawLine(display, window, gc,
						item->x + 1,            i,
						item->x + item->xs - 2, i);
		}
		for (n=1; n < nx; n++) {
			i = item->x + n * item->xs / nx;
			XDrawLine(display, window, gc,
						i, item->y + ym + 1,
						i, item->y + item->ys - 3);
		}
	}

	set_color(COL_CANVTEXT);
	*buf = 0;
	sprintf(sumcol, item->sumwidth ? ",%d" : "", item->sumcol);
	if (item->type == IT_CHOICE || item->type == IT_FLAG)
		sprintf(buf, "[%ld=%s%s] ", item->column,
				item->flagcode ? item->flagcode : "?", sumcol);
	strcat(buf, item->label ? item->label : "label");
	nfont = ifont(item->labelfont);
	truncate_string(buf, xm<0 ? item->xs-4 : xm-4, nfont);
	XSetFont(display, gc, font[nfont]->fid);
	XDrawString(display, window, gc,
			item->x + 3,
			item->y + ((ym>=0?ym:item->ys)+font[nfont]->ascent)/2,
			buf, strlen(buf));

	*buf = 0;
	if (IN_DBASE(item->type))
		sprintf(buf, "[%ld%s] ", item->column, sumcol);
	strcat(buf, datatext[item->type]);
	if (xm > 0 && xm < item->xs) {
		nfont = ifont(item->inputfont);
		truncate_string(buf, item->xs-xm-4, nfont);
		XSetFont(display, gc, font[nfont]->fid);
		XDrawString(display, window, gc,
			item->x + xm + 3,
			item->y + (item->ys + font[nfont]->ascent)/2,
			buf, strlen(buf));

	} else if (ym > 0 && ym < item->ys) {
		nfont = ifont(item->type == IT_NOTE ? item->inputfont
						    : item->labelfont);
		truncate_string(buf, item->xs-4, nfont);
		XSetFont(display, gc, font[nfont]->fid);
		XDrawString(display, window, gc,
			item->x + 3,
			item->y + ym + (item->ys - ym + font[nfont]->ascent)/2,
			buf, strlen(buf));
	}
	set_color(COL_STD);
}


static int ifont(
	FONTN		fontn)
{
	switch(fontn) {
	  default:
	  case F_HELV:		return(FONT_HELV);
	  case F_HELV_O:	return(FONT_HELV_O);
	  case F_HELV_S:	return(FONT_HELV_S);
	  case F_HELV_L:	return(FONT_HELV_L);
	  case F_COURIER:	return(FONT_COURIER);
	}
}
