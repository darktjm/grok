/*
 * draw a chart in the card form
 *
 *	draw_chart(card,nitem)
 *	chart_action_callback(widget,event,args,nargs)
 */

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <QtWidgets>
#include "chart-widget.h"
#include "grok.h"
#include "form.h"
#include "proto.h"

static void drawrect(QPainter &, ITEM *, int, int, int, int);
static void drawbox (QPainter &, ITEM *, int, int, int, int);
#define setcolor(c) do { \
    painter.setBrush()); \
} while(0)


/*
 * adjust x such that it is a multiple of s
 */

static double snap(
	double x,
	double s)
{
	long i;
	if (s < 1e-6) return(x);
	i = x / s;
	return(i * s);
#if 0
	long tmp = x * 1024;
	if (s < 0) s = -s;
	if (s < 1e-6) return(x);
	tmp -= tmp % (int)(s * 1024);
	return(tmp / 1024.0);
#endif
}


/*
 * redraw a single chart defined by an item into a drawing area widget.
 */

#define XPIX(x)   ((int)(((x) - xmin) / (xmax - xmin) * item->xs))
#define YPIX(y)   ((int)(((y) - ymin) / (ymax - ymin) * item->ys))


void draw_chart(
	CARD		*card,		/* life, the universe, and everything*/
	int		nitem)		/* # of item in form */
{
	ITEM		*item;		/* chart item to draw */
	GrokChart	*gc;

	if (!card || !card->form || !card->dbase || !card->dbase->nrows)
		return;
	item = card->form->items[nitem];
	if (!item->ch_ncomp)
		return;
	gc = dynamic_cast<GrokChart *>(card->items[nitem].w0);
	gc->card = card;
	gc->item = item;
	gc->update();
}

void GrokChart::paintEvent(QPaintEvent *)
{
	int		save_row;	/* save card->row (current card) */
	CHART		*chart;		/* describes current component */
	const char	*res;		/* result of expr evaluation */
	BAR		*bar;		/* current bar */
	int		r, c, sr;	/* row (card) and comp (bar) counters*/
	int		i;		/* index values */
	double		f;		/* coordinate in value space */

	if(!item)
		return;
	save_row = card->row;

	/*
	 * step 1: calculate positions and colors of all bars
	 */
	item->ch_nbars = item->ch_ncomp * card->dbase->nrows;
	zfree(item->ch_bar);
	item->ch_bar = alloc(0, "bar chart", BAR, item->ch_nbars);

	xmin = ymin =  1e30;
	xmax = ymax = -1e30;
	bar = item->ch_bar;
	for (r=0; r < card->dbase->nrows; r++) {
		sr = card->sorted ? card->sorted[r] : r;
		card->row = sr;
		for (c=0; c < item->ch_ncomp; c++, bar++) {
			chart = &item->ch_comp[c];
			if ((res=evaluate(card,chart->excl_if)) && atoi(res)) {
				memset(bar, 0, sizeof(BAR));
				continue;
			}
			if ((res = evaluate(card, chart->color)))
				bar->color = atoi(res) & 7;
			for (i=0; i < 4; i++)
				switch(chart->value[i].mode) {
				  default:
				  case CC_NEXT:
					bar->value[i] =
						i ? c ? bar[-1].value[CC_Y] +
							bar[-1].value[CC_YS]
						      : 0
						  : xmax < xmin ? 0 : xmax;
					break;
				  case CC_SAME:
					bar->value[i] =
						c ? bar[-1].value[i] : 0;
					break;
				  case CC_EXPR:
					bar->value[i] =
						(res = evaluate(card,
							chart->value[i].expr))
						  ? atof(res)
						  : 0;
					break;
				  case CC_DRAG:
					res = dbase_get(card->dbase, sr,
							chart->value[i].field);
					bar->value[i] =
						res ? atof(res)
							* chart->value[i].mul
							+ chart->value[i].add
						    : 0;
				}
			if (bar->value[2] < 0) {
				bar->value[0] += bar->value[2];
				bar->value[2] = -bar->value[2];
			}
			if (bar->value[3] < 0) {
				bar->value[1] += bar->value[3];
				bar->value[3] = -bar->value[3];
			}
			if (bar->value[0] < xmin)
				xmin = bar->value[0];
			if (bar->value[1] < ymin)
				ymin = bar->value[1];
			if (bar->value[0] + bar->value[2] > xmax)
				xmax = bar->value[0] + bar->value[2];
			if (bar->value[1] + bar->value[3] > ymax)
				ymax = bar->value[1] + bar->value[3];
		}
	}
	if (!item->ch_xauto) {
		xmin = item->ch_xmin;
		xmax = item->ch_xmax;
	} else {
		if (xmin < 0)
			xmin -= (xmax - xmin) * .05;
		if (xmax > 0)
			xmax += (xmax - xmin) * .05;
	}
	if (!item->ch_yauto) {
		ymin = item->ch_ymin;
		ymax = item->ch_ymax;
	} else {
		if (ymin < 0)
			ymin -= (ymax - ymin) * .05;
		if (ymax > 0)
			ymax += (ymax - ymin) * .05;
	}
	if (xmin >= xmax || ymin >= ymax) {
		card->row = save_row;
		return;
	}

	/*
	 * step 2: prepare background
	 */
	// background already painted upon entry

	QPainter painter(this);
	painter.setPen(gridcolor);			/* draw grid lines */
	if (item->ch_xgrid)
		for (f=snap(xmin, item->ch_xgrid); f < xmax; f+=item->ch_xgrid)
			drawrect(painter, item, XPIX(f), 0, 1, item->ys);
	if (item->ch_ygrid)
		for (f=snap(ymin, item->ch_ygrid); f < ymax; f+=item->ch_ygrid)
			drawrect(painter, item, 0, YPIX(f), item->xs, 1);

	painter.setPen(axiscolor);			/* draw axes */
	drawrect(painter, item, 0, YPIX(0), item->xs, 1);
	drawrect(painter, item, XPIX(0), 0, 1, item->ys);

	/*
	 * step 3: draw bars
	 */
	bar = item->ch_bar;
	for (r=0; r < card->dbase->nrows; r++) {
		sr = card->sorted ? card->sorted[r] : r;
		card->row = sr;
		for (c=0; c < item->ch_ncomp; c++, bar++) {
			int x  = XPIX(snap(bar->value[0], item->ch_xsnap));
			int xs = XPIX(snap(bar->value[0] +
					   bar->value[2], item->ch_xsnap)) - x;
			int y  = YPIX(snap(bar->value[1], item->ch_ysnap));
			int ys = YPIX(snap(bar->value[1] +
					   bar->value[3], item->ch_ysnap)) - y;
			chart = &item->ch_comp[c];
			/* "Fat" means "no border and no gap", I guess */
			/* except that border is present on selected item */
			if (!chart->xfat)
				x++, xs-=2;
			if (!chart->yfat)
				y++, ys-=2;
			painter.setPen(fillcolor[bar->color % COL_CHART_N]);
			drawrect(painter, item, x, y, xs, ys);
			if (sr == save_row || (!chart->xfat && !chart->yfat)) {
				painter.setPen(boxcolor); // used to be COL_STD
				drawbox(painter, item, x, y, xs, ys);
			}
			if (sr == save_row) {
				painter.setPen(hlcolor); // used to be COL_SHEET
				drawbox(painter, item, x+1, y+1, xs-2, ys-2);
			}
		}
	}
	card->row = save_row;
}


/*
 * draw lines and boxes. All coordinates are doubles in the range 0..1,
 * and assume 0/0 in the lower left corner, not in the upper right as X
 * normally does.
 */

#define CLIP \
	if (x + xs > item->xs)	xs = item->xs - x;	\
	if (x      < 0)		xs += x, x = 0;		\
	if (y + ys > item->ys)	ys = item->ys - y;	\
	if (y      < 0)		ys += y, y = 0;		\
	if (xs <= 0 || ys <= 0)	return

static void drawrect(
	QPainter	&painter,
	ITEM		*item,
	int		x,
	int		y,
	int		xs,
	int		ys)
{
	CLIP;
	painter.fillRect(x, item->ys-ys-y, xs, ys, QBrush(painter.pen().color(), Qt::SolidPattern));
}

static void drawbox(
	QPainter	&painter,
	ITEM		*item,
	int		x,
	int		y,
	int		xs,
	int		ys)
{
	CLIP;
	painter.drawRect(x, item->ys-ys-y, xs-1, ys-1);
}


/*
 * given a x/y pixel coordinate in the drawing area, return the row number
 * of the card that produced the bar, or -1 if no bar was hit
 */

int GrokChart::pick_chart(
	CARD		*card,		/* life, the universe, and everything*/
	int		nitem,		/* # of item in form */
	int		*comp,		/* returned component # */
	int		xpick,		/* mouxe x/y pixel coordinate */
	int		ypick)
{
	ITEM		*item;		/* chart item to draw */
	CHART		*chart;		/* describes current component */
	BAR		*bar;		/* current bar */
	int		r, c;		/* row (card) and comp (bar) counters*/

	if (!card || !card->form || !card->dbase || !card->dbase->nrows)
		return(-1);
	item = card->form->items[nitem];
	if (!item->ch_ncomp)
		return(-1);

	ypick = item->ys - ypick;
	bar = item->ch_bar + card->dbase->nrows * item->ch_ncomp -1;
	for (r=card->dbase->nrows-1; r >= 0; r--) {
		for (c=item->ch_ncomp-1; c >= 0; c--, bar--) {
			int x  = XPIX(snap(bar->value[0], item->ch_xsnap));
			int xs = XPIX(snap(bar->value[0] +
					   bar->value[2], item->ch_xsnap)) - x;
			int y  = YPIX(snap(bar->value[1], item->ch_ysnap));
			int ys = YPIX(snap(bar->value[1] +
					   bar->value[3], item->ch_ysnap)) - y;
			chart = &item->ch_comp[c];
			if (!chart->xfat)
				x++, xs-=2;
			if (!chart->yfat)
				y++, ys-=2;
			if (xpick >= x && xpick < x + xs &&
			    ypick >= y && ypick < y + ys) {
				*comp = c;
				return(card->sorted ? card->sorted[r] : r);
			}
		}
	}
	return(-1);
}


/*
 * some button action (up, down, motion) has occured in a chart drawing area.
 * The value written back is the field value -> pixel coordinate equation:
 *    pix = ((fieldval * mul + add) - xmin) / (xmax - xmin) * item->xs
 * solved for fieldval:
 *    fieldval = (pix * (xmax - xmin) / item->xs + xmin - add) / mul
 */

void GrokChart::chart_action_callback(QMouseEvent *event, int press)
{
	ITEM		*item;		/* item being selected or moved */
	CHART		*chart;		/* chart component being dragged */
	struct value	*xval, *yval;	/* chart component value (x,y,xs,ys) */
	int		x, y;		/* pixel coordinate of mouse */
	double		f;		/* new db value */
	char		buf[40];	/* new db value as numeric string */
	char		*p;
	bool		redraw = false;
	int		i;

	if(event->button() != Qt::NoButton && event->button() != Qt::LeftButton) {
		event->ignore();
		return;
	}
	for (nitem=0; nitem < mainwindow->card->nitems; nitem++)
		if (this == mainwindow->card->items[nitem].w0 ||
		    this == mainwindow->card->items[nitem].w1)
			break;
	if (nitem >= mainwindow->card->nitems	||		/* illegal */
	    mainwindow->card->dbase == 0	||		/* preview dummy card*/
	    mainwindow->card->row < 0) {			/* card still empty */
		event->ignore();
		return;
	}
	item = mainwindow->card->form->items[nitem];

	if (press > 0) { // button down
		moving = false;
		down_x = event->x();
		down_y = event->y();
		row    = pick_chart(mainwindow->card, nitem, &comp, down_x, down_y);
		if (row >= 0 && row < mainwindow->card->dbase->nrows) {
			card_readback_texts(mainwindow->card, -1);
			mainwindow->card->row = row;
			for (i=0; i < mainwindow->card->nquery; i++)
				if (mainwindow->card->query[i] == row) {
					mainwindow->card->qcurr = i;
					break;
				}
			fillout_card(mainwindow->card, false);
			scroll_summary(mainwindow->card);
			chart = &item->ch_comp[comp];
			p = dbase_get(mainwindow->card->dbase, row,
						chart->value[CC_X].field);
			x_val = p ? atof(p) : 0;
			p = dbase_get(mainwindow->card->dbase, row,
						chart->value[CC_Y].field);
			y_val = p ? atof(p) : 0;
		}
		event->accept();
		return;
	}

	// At this point, it's either a relase or a move event
	// Assume that the press event occured, so comp is valid.
	// FIXME:  this may not be true, so best to track it better
	chart = &item->ch_comp[comp];
	xval  = &chart->value[CC_X];
	yval  = &chart->value[CC_Y];
	if ((xval->mode != CC_DRAG && yval->mode != CC_DRAG) ||
						xmax <= xmin || ymax <= ymin) {
		event->ignore();
		return;
	}

	event->accept();
	if (press < 0) { // button up
		if (moving) {
			print_info_line();
			fillout_card(mainwindow->card, false);
		}
		return;
	}
	x = abs(event->x() - down_x);
	y = abs(event->y() - down_y);
	moving |= x > 3 || y > 3;
	if (!moving)
		return;

	if (xval->mode == CC_DRAG && xval->mul != 0) {
		x = event->x() - down_x;
		f = x_val + x * (xmax-xmin) / xval->mul / item->xs;
		f = snap(f, item->ch_xsnap);
		sprintf(buf, "%.12lg", f);
		dbase_put(mainwindow->card->dbase, row, xval->field, buf);
		redraw = true;
	}
	if (yval->mode == CC_DRAG && yval->mul != 0) {
		y = event->y() - down_y;
		f = y_val + y * (ymax-ymin) / yval->mul / item->ys;
		f = snap(f, item->ch_ysnap);
		sprintf(buf, "%.12lg", f);
		dbase_put(mainwindow->card->dbase, row, yval->field, buf);
		redraw = true;
	}
	if (redraw)
		draw_chart(mainwindow->card, nitem);
}
