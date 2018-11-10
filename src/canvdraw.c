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

static BOOL		have_shell = FALSE;	/* week window is being displayed */
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
		have_shell = FALSE;
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
GrokCanvas::GrokCanvas() : moving(FALSE) {
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
}

void create_canvas_window(
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
	have_shell = TRUE;
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
	int		x, y, xs, ys;	/* new item start and size */
	int		xm, ym;		/* new item midpoint division */
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
		moving = FALSE;
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
			}
			if (x  < 0)	x  = 0;		x  = XSNAP(x);
			if (y  < 0)	y  = 0;		y  = XSNAP(y);
			if (xs < 0)	xs = 0;		xs = XSNAP(xs);
			if (ys < 0)	ys = 0;		ys = XSNAP(ys);
			if (xm > xs)	xm = xs;	xm = XSNAP(xm);
			if (ym > ys)	ym = ys;	ym = XSNAP(ym);
		}
	}
	event->accept();
	if (!press) { // move
	    if (moving && (event->buttons() & Qt::LeftButton))
		switch(mode) {
		  case M_XMID: draw_rubberband(TRUE, xm+x, y, 1, ys); break;
		  case M_YMID: draw_rubberband(TRUE, x, ym+y, xs, 1); break;
		  default:     draw_rubberband(TRUE, x, y, xs, ys);
	    }
	} else if (press < 0) { // button up
		draw_rubberband(FALSE, 0, 0, 0, 0);
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
			if (state & Qt::ShiftModifier) {
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

		if ((item->type == IT_CHART ||
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

void GrokCanvas::draw_rubberband(
	BOOL		draw,		/* draw or undraw */
	int		x,		/* position of box */
	int		y,
	int		xs,		/* size of box */
	int		ys)
{
	draw_rb = draw;
	--x; --y; ++xs; ++ys;
	// only erase if it's not where it will be
	if (rb_is_drawn && (!draw || x != drb_x || y != drb_y || xs != drb_xs || ys != drb_ys))
		update(drb_x, drb_y, drb_xs, drb_ys);
	if (!draw)
		return;

	// only draw if it wasn't there or was erased
	if (!rb_is_drawn || x != drb_x || y != drb_y || xs != drb_xs || ys != drb_ys)
		update(rb_x = x, rb_y = y, rb_xs = xs, rb_ys = ys);
}

// This has to be the last thing in the repaint, as it changes to xor mode
// FIXME:  This doesn't work right.  Eresure should happen automatically
//         due to paintEvent(), but it doesn't.
void GrokCanvas::draw_rubberband(
	QPainter	&painter,
	const QRect	&clip)
{
	// printf("redraw rubberband: %d %d %d %d\n", clip.x(),
	//       clip.y(), clip.width(), clip.height());
#if 0 // since everything is redrawn, no point in erasing old rectangle
	if (!rb_is_drawn && !draw_rb)
#else
	if (!draw_rb)
#endif
		return;

	QPen pen(QColor("#ffffff"));
	//pen.setWidth(2);
	painter.setPen(pen);
	painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
#if 0
	if (rb_is_drawn && clip.intersects(QRect(drb_x, drb_y, drb_xs, drb_ys))) {
		painter.drawRect(drb_x, drb_y, drb_xs, drb_ys);
	}
#endif
	if (!(rb_is_drawn = draw_rb)) {
		//painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		return;
	}

	// printf("draw rubberband: %d %d %d %d\n", rb_x, rb_y, rb_xs, rb_ys);
	painter.drawRect(drb_x=rb_x, drb_y=rb_y, drb_xs=rb_xs, drb_ys=rb_ys);
	//painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
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
	draw_rubberband(painter, e->rect());
}


static const char * const datatext[NITEMS] = {
	"None", "", "Print", "Input", "Time", "Note", "", "", "", "" };

void GrokCanvas:: redraw_canvas_item(
	QPainter	&painter,	/* widget into which to draw */
	const QRect	&clip,		/* redraw clipping region */
	register ITEM	*item)		/* item to redraw */
{
	char		buf[1024];	/* truncated texts */
	int		xm=-1, ym=-1;	/* middle division pos */
	int		nfont;		/* font number as F_FONT_* */
	char		sumcol[20];	/* summary column indicator msg */
	int		n, i;		/* for chart grid lines */

	if (!clip.intersects(QRect(item->x, item->y, item->xs, item->ys)))
		return;
	painter.setPen(item->selected ? selcolor : boxcolor);
	fillrect(item->x, item->y, item->xs, item->ys);

	painter.setPen(fgcolor());

	painter.drawRect(item->x, item->y, item->xs-1, item->ys-1);
	if (item->type == IT_INPUT || item->type == IT_PRINT
				   || item->type == IT_TIME)
		fillrect(item->x + (xm=item->xm), item->y, 1, item->ys);

	if (item->type == IT_CHART || item->type == IT_NOTE)
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
	*buf = 0;
	sprintf(sumcol, item->sumwidth ? ",%d" : "", item->sumcol);
	if (item->type == IT_CHOICE || item->type == IT_FLAG)
		sprintf(buf, "[%ld=%s%s] ", item->column,
				item->flagcode ? item->flagcode : "?", sumcol);
	strcat(buf, item->label ? item->label : "label");
	nfont = item->labelfont;
	truncate_string(font[nfont], buf, xm<0 ? item->xs-4 : xm-4);
	painter.setFont(font[nfont]->font());
	painter.drawText(item->x + 3,
			 item->y + ((ym>=0?ym:item->ys)+font[nfont]->fontMetrics().ascent())/2,
			 buf);

	*buf = 0;
	if (IN_DBASE(item->type))
		sprintf(buf, "[%ld%s] ", item->column, sumcol);
	strcat(buf, datatext[item->type]);
	if (xm > 0 && xm < item->xs) {
		nfont = item->inputfont;
		truncate_string(font[nfont], buf, item->xs-xm-4);
		painter.setFont(font[nfont]->font());
		painter.drawText(item->x + xm + 3,
				 item->y + (item->ys + font[nfont]->fontMetrics().ascent())/2,
				 buf);

	} else if (ym > 0 && ym < item->ys) {
		nfont = item->type == IT_NOTE ? item->inputfont
					      : item->labelfont;
		truncate_string(font[nfont], buf, item->xs-4);
		painter.setFont(font[nfont]->font());
		painter.drawText(item->x + 3,
				 item->y + ym + (item->ys - ym + font[nfont]->fontMetrics().ascent())/2,
				 buf);
	}
	// this used to set the color to COL_STD, but why?
}

void redraw_canvas_item(
	register ITEM	*item)		/* item to redraw */
{
	if (!have_shell)
		return;
	shell->draw_rubberband(FALSE, 0, 0, 0, 0);
	shell->update(item->x, item->y, item->xs, item->ys);
}
