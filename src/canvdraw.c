/*
 * draw card canvas. This canvas is used by the form editor to allow the
 * user to position items in a form and to set their size. The canvas is
 * drawn using Xlib calls; items are represented by colored rectangles.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <QtWidgets>
#include "canv-widget.h"
#include "grok.h"
#include "form.h"
#include "proto.h"

#define XSNAP(x)	((x)-(x)%form->xg)
#define YSNAP(y)	((y)-(y)%form->yg)
#define DIV_WIDTH	2
#define DIV_GRIPSZ	8
#define DIV_GRIPOFF	12

static bool		have_shell = false;	/* week window is being displayed */
static FORM		*form;		/* current form, registered by create*/
static GrokCanvas	*shell;		/* entire window */
#define canvas shell	/* drawing area -- not distinct from shell any more */


/*
 * destroy the canvas window. Remove it from the screen, and destroy its
 * widgets.
 */

void destroy_canvas_window(void)
{
	if (have_shell) {
		have_shell = false;
		shell->close();
		delete shell;
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
GrokCanvas::GrokCanvas() : moving(false) {
	// setProperty("colStd", true);
	setProperty("colCanv", true);
	// there is no way to obtain fonts via QSS directly, so
	// create dummy widgets to receive fonts.
	for(int i = 0; i < F_NFONTS; i++) {
		font[i] = new QWidget(this);
		font[i]->setProperty(font_prop[i], true);
		font[i]->ensurePolished();
		font[i]->hide();
	}
	setMouseTracking(true);
}

GrokCanvas *create_canvas_window(
	FORM		*f)
{
	destroy_canvas_window();
	form = f;

	// Close event is captured by overriding closeEvent().
	shell = new GrokCanvas;
	shell->setWindowTitle("Form Editor Canvas");
	set_icon(shell, 1);

	shell->resize(form->xs, form->ys);
	shell->setObjectName("canvas"); // there is no separate canvas widget

	// resize callback is captured by overriding resizeEvent()

	popup_nonmodal(shell);
	// As mentioned above, window close callback is now closeEvent().
	set_cursor(canvas, Qt::ArrowCursor);
	have_shell = true;
	return shell;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

// formerly quit_callback
void GrokCanvas::closeEvent(QCloseEvent *e)
{
	create_error_popup(shell, 0, "%s%s",
		"Please press the Done button in the main\n",
		"Form Editor window to remove this window.");
	e->ignore();
}


// formerly resize_callback
void GrokCanvas::resizeEvent(QResizeEvent *)
{
	form->xs = size().width();
	form->ys = size().height();
	update();
}


/*-------------------------------------------------- dragging ---------------*/

static MOUSE locate_item(int *, int, int);
static Qt::CursorShape cursorglyph[] = {
	/* no X, so maybe something different? */
	Qt::CrossCursor,
	Qt::SizeAllCursor,
	Qt::SplitVCursor,
	Qt::SizeVerCursor, /* no side-specific resize cursors, I guess */
	Qt::SizeVerCursor, /* ditto */
	Qt::SizeHorCursor, /* ditto */
	Qt::SizeHorCursor, /* ditto */
	Qt::SplitHCursor,
	Qt::SplitVCursor
};


/*
 * this routine is called directly from the drawing area's translation
 * table, with mouse up/down/motion events.
 */

void GrokCanvas::canvas_callback(
	QMouseEvent	*event,		/* X event, contains position */
	int		press)		/* what happened, up/down/motion */
{
	ITEM		*item;		/* item being selected or moved */
	int		x = 0, y = 0, xs = 0, ys = 0;	/* new item start and size */
	int		xm = 0, ym = 0;		/* new item midpoint division */
	int		dx, dy;		/* movement since initial press */
	int		i, nsel;

	if(event->button() != Qt::NoButton && event->button() != Qt::LeftButton) {
		event->ignore();
		return;
	}
	if (press > 0) { // button down
		down_x = event->x();
		down_y = event->y();
		state  = event->modifiers();
		moving = false;
		mode   = locate_item(&nitem, event->x(), event->y());
		set_cursor(canvas, cursorglyph[mode]);
		event->accept();
		return;
	}
	if (!press && (event->buttons() & Qt::LeftButton)
				       && mode != M_OUTSIDE) { // move
		x = abs(event->x() - down_x);
		y = abs(event->y() - down_y);
		moving |= x > 3 || y > 3;
	}
	if (moving) {
		if (mode == M_OUTSIDE) {
			event->ignore();
			return;
		}
		if (mode == M_DIVIDER) {
			x  = 0;
			y  = event->y();
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
			dx = event->x() - down_x;
			dy = event->y() - down_y;
			switch(mode) {
			  case M_INSIDE:  x  += dx; y  += dy;	break;
			  case M_TOP:	  y  += dy; ys -= dy;	break;
			  case M_BOTTOM:  ys += dy;		break;
			  case M_LEFT:	  x  += dx; xs -= dx;	break;
			  case M_RIGHT:	  xs += dx;		break;
			  case M_XMID:	  xm  = item->xm + dx;	break;
			  case M_YMID:	  ym  = item->ym + dy;	break;
			  default: ;
			}
			if (x  < 0)	x  = 0;
			x  = XSNAP(x);
			if (y  < 0)	y  = 0;
			y  = XSNAP(y);
			if (xs < 0)	xs = 0;
			xs = XSNAP(xs);
			if (ys < 0)	ys = 0;
			ys = XSNAP(ys);
			if (xm > xs)	xm = xs;
			xm = XSNAP(xm);
			if (ym > ys)	ym = ys;
			ym = XSNAP(ym);
		}
	}
	event->accept();
	if (!press) { // move
	    if (moving && (event->buttons() & Qt::LeftButton))
		switch(mode) {
		  case M_XMID: draw_rubberband(true, xm+x, y, 1, ys, false); break;
		  case M_YMID: draw_rubberband(true, x, ym+y, xs, 1, false); break;
		  default:     draw_rubberband(true, x, y, xs, ys);
	    }
	    if (!(event->buttons() & Qt::LeftButton))
		set_cursor(canvas, cursorglyph[locate_item(0, event->x(), event->y())]);
	} else if (press < 0) { // button up
		draw_rubberband(false, 0, 0, 0, 0);
		if (mode == M_OUTSIDE) {
			set_cursor(canvas, Qt::ArrowCursor);
			return;
		}
		if (mode == M_DIVIDER) {
			int dy = form->ydiv;
			form->ydiv = event->y() <  0        ? 0 :
				     event->y() >= form->ys ? form->ys-1
							  : event->y();
			form->ydiv = YSNAP(form->ydiv);
			dy = form->ydiv - dy;
			for (i=0; i < form->nitems; i++)
				form->items[i]->y += dy;
			redraw_canvas();
			set_cursor(canvas, Qt::ArrowCursor);
			return;
		}
		item = form->items[nitem];
		for (nsel=i=0; i < form->nitems; i++)
			nsel += IFL(form->items[i]->,SELECTED);
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
			if (state & Qt::ShiftModifier) {
				IFT(item->,SELECTED);
				curr_item = nitem;
			} else {
				if (!IFL(item->,SELECTED) || nsel > 1) {
					item_deselect(form);	/*... sel */
					IFS(item->,SELECTED);
					curr_item = nitem;
				} else {
					item_deselect(form);	/*... unsel */
					curr_item = form->nitems;
				}
			}
			::redraw_canvas_item(item);
			fillout_formedit();
			sensitize_formedit();
		}
		set_cursor(canvas, Qt::ArrowCursor);
	}
}


/*
 * find the item the mouse is in or nearby, and return exactly in which way
 * it is close (left, right, top, bottom). Return the item if found, and the
 * edge found as one of M_*. This is done when the mouse is pressed down, to
 * find out what should be done, and to set the cursor shape.
 */

#define LEFT_LABELED ( \
	item->type == IT_INPUT || \
	item->type == IT_NUMBER || \
	item->type == IT_PRINT || \
	item->type == IT_TIME || \
	item->type == IT_MENU || \
	(item->type == IT_FKEY && !IFL(item->,FKEY_MULTI)))

#define TOP_LABELED ( \
	item->type == IT_CHART || \
	item->type == IT_NOTE || \
	item->type == IT_MULTI || \
	item->type == IT_RADIO || \
	item->type == IT_FLAGS || \
	(item->type == IT_FKEY && IFL(item->,FKEY_MULTI)) || \
	item->type == IT_INV_FKEY)

static MOUSE locate_item(
	int		*nitem_p,	/* set to the located item */
	int		x,		/* position in drawing area */
	int		y)
{
	int		nitem;		/* current item number */
	ITEM		*item;		/* current item, form->items[nitem] */
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
		if (LEFT_LABELED && abs(x - item->x - item->xm) < 6)
			return(M_XMID);

		if (TOP_LABELED && abs(y - item->y - item->ym) < 6)
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

void GrokCanvas::draw_rubberband(
	bool		draw,		/* draw or undraw */
	int		x,		/* position of box */
	int		y,
	int		xs,		/* size of box */
	int		ys,
	bool		isrect)		/* is it a rectangle or line? */
{
	if (!draw) {
		if (rb) {
			delete rb;
			rb = NULL;
		}
		return;
	}

	if (!rb)
		rb = new QRubberBand(isrect ? QRubberBand::Rectangle : QRubberBand::Line, this);

	rb->show();
	rb->move(x, y);
	rb->resize(xs, ys);
}

/*-------------------------------------------------- drawing ----------------*/
/*
 * draw all items in the current form
 */

void redraw_canvas(void)
{
	if (!have_shell)
		return;

	shell->update();
}

#define fillrect(x, y, w, h) painter.fillRect(x, y, w, h, QBrush(painter.pen().color(), Qt::SolidPattern))

void GrokCanvas::paintEvent(QPaintEvent *e)
{
	int 		i;

	QPainter painter(this);
	// bg color is already filled by default
	//painter.setPen(bgcolor());
	//fillrect(0, 0, form->xs, form->ys);
	for (i=0; i < form->nitems; i++)
		redraw_canvas_item(painter, e->rect(), form->items[i]);

	painter.setPen(fgcolor());
	fillrect(0, form->ydiv - DIV_WIDTH/2, form->xs, DIV_WIDTH);
	fillrect(form->xs - DIV_GRIPOFF - DIV_GRIPSZ/2,
		 form->ydiv - DIV_GRIPSZ/2,
		 DIV_GRIPSZ, DIV_GRIPSZ);
	painter.setPen(bgcolor());
	fillrect(form->xs - DIV_GRIPOFF - DIV_GRIPSZ/2 + 2,
		 form->ydiv - DIV_GRIPSZ/2 + 2,
		 DIV_GRIPSZ - 4, DIV_GRIPSZ - 4);
}


static const char * const datatext[NITEMS] = {
	"None", "", "Print", "Input", "Time", "Note", "", "", "", "",
	"Number", "Choice Menu", "Choice Group", "Flag List", "Flag Group",
	"Reference", "Referer" };

void GrokCanvas:: redraw_canvas_item(
	QPainter	&painter,	/* widget into which to draw */
	const QRect	&clip,		/* redraw clipping region */
	ITEM		*item)		/* item to redraw */
{
	QString		buf;		/* truncated texts */
	int		xm=-1, ym=-1;	/* middle division pos */
	int		nfont;		/* font number as F_FONT_* */
	char		sumcol[20];	/* summary column indicator msg */
	int		n, i;		/* for chart grid lines */

	if (!clip.intersects(QRect(item->x, item->y, item->xs, item->ys)))
		return;
	painter.setPen(IFL(item->,SELECTED) ? selcolor : boxcolor);
	fillrect(item->x, item->y, item->xs, item->ys);

	painter.setPen(fgcolor());

	painter.drawRect(item->x, item->y, item->xs-1, item->ys-1);
	if (LEFT_LABELED)
		fillrect(item->x + (xm=item->xm), item->y, 1, item->ys);

	if (TOP_LABELED)
		fillrect(item->x, item->y + (ym=item->ym), item->xs, 1);

	if (item->type == IT_CHART) {
		int ny = 10;
		int nx = 15 - ny;
		painter.setPen(bgcolor());
		for (n=1; n < ny; n++) {
			i = item->y + ym + n * (item->ys - ym) / ny,
			painter.drawLine(item->x + 1,            i,
					 item->x + item->xs - 2, i);
		}
		for (n=1; n < nx; n++) {
			i = item->x + n * item->xs / nx;
			painter.drawLine(i, item->y + ym + 1,
					 i, item->y + item->ys - 3);
		}
	}

	painter.setPen(textcolor);
	if(item->sumwidth)
		sprintf(sumcol, ",%d", item->sumcol);
	else
		*sumcol = 0;
	if (item->type == IT_CHOICE || item->type == IT_FLAG)
		buf = qsprintf("[%d=%s%s] ", item->column,
			       item->flagcode ? item->flagcode : "?", sumcol);
	buf += item->label ? item->label : "label";
	nfont = item->labelfont;
	truncate_string(font[nfont], buf, xm<0 ? item->xs-4 : xm-4);
	painter.setFont(font[nfont]->font());
	painter.drawText(item->x + 3,
			 item->y + ((ym>=0?ym:item->ys)+font[nfont]->fontMetrics().ascent())/2,
			 buf);

	buf.clear();
	if (IN_DBASE(item->type))
		buf = qsprintf("[%d%s] ", item->column, sumcol);
	buf += datatext[item->type];
	if (xm > 0 && xm < item->xs) {
		nfont = item->inputfont;
		truncate_string(font[nfont], buf, item->xs-xm-4);
		painter.setFont(font[nfont]->font());
		painter.drawText(item->x + xm + 3,
				 item->y + (item->ys + font[nfont]->fontMetrics().ascent())/2,
				 buf);

	} else if (ym > 0 && ym < item->ys) {
		nfont = TOP_LABELED ? item->inputfont : item->labelfont;
		truncate_string(font[nfont], buf, item->xs-4);
		painter.setFont(font[nfont]->font());
		painter.drawText(item->x + 3,
				 item->y + ym + (item->ys - ym + font[nfont]->fontMetrics().ascent())/2,
				 buf);
	}
	// this used to set the color to COL_STD, but why?
}

void redraw_canvas_item(
	ITEM		*item)		/* item to redraw */
{
	if (!have_shell)
		return;
	shell->draw_rubberband(false, 0, 0, 0, 0);
	shell->update(item->x, item->y, item->xs, item->ys);
}
