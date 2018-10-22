/*
 * draw a chart in the card form
 *
 *	draw_chart(card,nitem)
 *	chart_action_callback(widget,event,args,nargs)
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <Xm/Xm.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void drawrect(Window, ITEM *, int, int, int, int);
static void drawbox (Window, ITEM *, int, int, int, int);


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

static float		xmin, xmax;	/* bounds of all bars in chart */
static float		ymin, ymax;	/* bounds of all bars in chart */

void draw_chart(
	CARD		*card,		/* life, the universe, and everything*/
	int		nitem)		/* # of item in form */
{
	int		save_row;	/* save card->row (current card) */
	Window		win;		/* window on screen to draw into */
	register ITEM	*item;		/* chart item to draw */
	register CHART	*chart;		/* describes current component */
	const char	*res;		/* result of expr evaluation */
	register BAR	*bar;		/* current bar */
	int		r, c;		/* row (card) and comp (bar) counters*/
	int		i;		/* index values */
	float		f;		/* coordinate in value space */

	if (!card || !card->form || !card->dbase || !card->dbase->nrows)
		return;
	item = card->form->items[nitem];
	if (!item->ch_ncomp)
		return;
	win = XtWindow(card->items[nitem].w0);
	save_row = card->row;

	/*
	 * step 1: calculate positions and colors of all bars
	 */
	item->ch_nbars = item->ch_ncomp * card->dbase->nrows;
	if (item->ch_bar)
		free((void *)item->ch_bar);
	if (!(item->ch_bar = (BAR *)malloc(item->ch_nbars * sizeof(BAR))))
		fatal("no memory");

	xmin = ymin =  1e30;
	xmax = ymax = -1e30;
	bar = item->ch_bar;
	for (r=0; r < card->dbase->nrows; r++) {
		card->row = r;
		for (c=0; c < item->ch_ncomp; c++, bar++) {
			chart = &item->ch_comp[c];
			if ((res=evaluate(card,chart->excl_if)) && atoi(res)) {
				memset((void *)bar, 0, sizeof(BAR));
				continue;
			}
			if (res = evaluate(card, chart->color))
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
					res = dbase_get(card->dbase, r,
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
	set_color(COL_BACK);				/* draw background */
	drawrect(win, item, 0, 0, item->xs, item->ys);

	set_color(COL_CHART_GRID);			/* draw grid lines */
	if (item->ch_xgrid)
		for (f=snap(xmin, item->ch_xgrid); f < xmax; f+=item->ch_xgrid)
			drawrect(win, item, XPIX(f), 0, 1, item->ys);
	if (item->ch_ygrid)
		for (f=snap(ymin, item->ch_ygrid); f < ymax; f+=item->ch_ygrid)
			drawrect(win, item, 0, YPIX(f), item->xs, 1);

	set_color(COL_CHART_AXIS);			/* draw axes */
	drawrect(win, item, 0, YPIX(0), item->xs, 1);
	drawrect(win, item, XPIX(0), 0, 1, item->ys);

	/*
	 * step 3: draw bars
	 */
	bar = item->ch_bar;
	for (r=0; r < card->dbase->nrows; r++) {
		card->row = r;
		for (c=0; c < item->ch_ncomp; c++, bar++) {
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
			set_color(COL_CHART_0 + bar->color % COL_CHART_N);
			drawrect(win, item, x, y, xs, ys);
			if (r == save_row || !chart->xfat && !chart->yfat) {
				set_color(COL_STD);
				drawbox(win, item, x, y, xs, ys);
			}
			if (r == save_row) {
				set_color(COL_SHEET);
				drawbox(win, item, x+1, y+1, xs-2, ys-2);
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
	Window		win,
	ITEM		*item,
	int		x,
	int		y,
	int		xs,
	int		ys)
{
	CLIP;
	XFillRectangle(display, win, gc, x, item->ys-ys-y, xs, ys);
}

static void drawbox(
	Window		win,
	ITEM		*item,
	int		x,
	int		y,
	int		xs,
	int		ys)
{
	CLIP;
	XDrawRectangle(display, win, gc, x, item->ys-ys-y, xs-1, ys-1);
}


/*
 * given a x/y pixel coordinate in the drawing area, return the row number
 * of the card that produced the bar, or -1 if no bar was hit
 */

static int pick_chart(
	CARD		*card,		/* life, the universe, and everything*/
	int		nitem,		/* # of item in form */
	int		*comp,		/* returned component # */
	int		xpick,		/* mouxe x/y pixel coordinate */
	int		ypick)
{
	register ITEM	*item;		/* chart item to draw */
	register CHART	*chart;		/* describes current component */
	register BAR	*bar;		/* current bar */
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
			    	return(r);
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

void chart_action_callback(
	Widget		widget,		/* drawing area */
	XButtonEvent	*event,		/* X event, contains position */
	String		*args,		/* what happened, up/down/motion */
	int		nargs)		/* # of args, must be 1 */
{
	static int	nitem;		/* item on which pen was pressed */
	static int	row, comp;	/* row and column of dragged bar */
	static int	down_x, down_y;	/* pos where pen was pressed down */
	static double	x_val, y_val;	/* previous values of fields */
	static BOOL	moving = FALSE;		/* this is not a selection, move box */
	ITEM		*item;		/* item being selected or moved */
	CHART		*chart;		/* chart component being dragged */
	struct value	*xval, *yval;	/* chart component value (x,y,xs,ys) */
	int		x, y;		/* pixel coordinate of mouse */
	double		f;		/* new db value */
	char		buf[40];	/* new db value as numeric string */
	char		*p;
	BOOL		redraw = FALSE;
	int		i;

	for (nitem=0; nitem < curr_card->nitems; nitem++)
		if (widget == curr_card->items[nitem].w0 ||
		    widget == curr_card->items[nitem].w1)
			break;
	if (nitem >= curr_card->nitems	||		/* illegal */
	    curr_card->dbase == 0	||		/* preview dummy card*/
	    curr_card->row < 0)				/* card still empty */
		return;
	item = curr_card->form->items[nitem];

	if (!strcmp(args[0], "down")) {
		moving = FALSE;
		down_x = event->x;
		down_y = event->y;
		row    = pick_chart(curr_card, nitem, &comp, down_x, down_y);
		if (row >= 0 && row < curr_card->dbase->nrows) {
			card_readback_texts(curr_card, -1);
			curr_card->row = row;
			for (i=0; i < curr_card->nquery; i++)
				if (curr_card->query[i] == row) {
					curr_card->qcurr = i;
					break;
				}
			fillout_card(curr_card, FALSE);
			scroll_summary(curr_card);
			chart = &item->ch_comp[comp];
			p = dbase_get(curr_card->dbase, row,
						chart->value[CC_X].field);
			x_val = p ? atof(p) : 0;
			p = dbase_get(curr_card->dbase, row,
						chart->value[CC_Y].field);
			y_val = p ? atof(p) : 0;
		}
		return;
	}

	chart = &item->ch_comp[comp];
	xval  = &chart->value[CC_X];
	yval  = &chart->value[CC_Y];
	if (xval->mode != CC_DRAG && yval->mode != CC_DRAG ||
						xmax <= xmin || ymax <= ymin)
		return;

	if (!strcmp(args[0], "up")) {
		if (moving) {
			print_info_line();
			fillout_card(curr_card, FALSE);
		}
		return;
	}
	x = abs(event->x - down_x);
	y = abs(event->y - down_y);
	moving |= x > 3 || y > 3;
	if (!moving)
		return;

	if (xval->mode == CC_DRAG && xval->mul != 0) {
		x = event->x - down_x;
		f = x_val + x * (xmax-xmin) / xval->mul / item->xs;
		f = snap(f, item->ch_xsnap);
		sprintf(buf, "%.12lg", f);
		dbase_put(curr_card->dbase, row, xval->field, buf);
		redraw = TRUE;
	}
	if (yval->mode == CC_DRAG && yval->mul != 0) {
		y = event->y - down_y;
		f = y_val + y * (ymax-ymin) / yval->mul / item->ys;
		f = snap(f, item->ch_ysnap);
		sprintf(buf, "%.12lg", f);
		dbase_put(curr_card->dbase, row, yval->field, buf);
		redraw = TRUE;
	}
	if (redraw)
		draw_chart(curr_card, nitem);
}
