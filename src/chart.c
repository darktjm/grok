/*
 * functions relating to charts, except drawing functions which are in
 * chartdraw.c.
 *
 * add_chart_component		implements ADD button in the form editor
 * del_chart_component		implements DELETE button in the form editor
 * clone_chart_component	copies a chart component and its contents
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"


/*
 * add a chart component to an item
 */

void add_chart_component(
	ITEM		*item)
{
	CHART		*chart;

	grow(0, "chart", CHART, item->ch_comp, item->ch_ncomp + 1, NULL);
	if (item->ch_curr < item->ch_ncomp)
		item->ch_curr++;
	else
		tmemmove(CHART, item->ch_comp + item->ch_curr + 1,
			 item->ch_comp + item->ch_curr,
			 item->ch_ncomp - item->ch_curr);
	item->ch_ncomp++;
	chart = &item->ch_comp[item->ch_curr];
	tzero(CHART, chart, 1);
	chart->xfat =
	chart->yfat = TRUE;
	chart->value[0].mul = 1;
	chart->value[1].mul = 1;
	chart->value[2].mul = 1;
	chart->value[3].mul = 1;
}


/*
 * delete a chart component from an item
 */

void del_chart_component(
	ITEM		*item)
{
	CHART		*chart = &item->ch_comp[item->ch_curr];
	int		i;

	if (--item->ch_ncomp < 1) {
		if (item->ch_comp)
			free(item->ch_comp);
		item->ch_comp  = 0;
		item->ch_ncomp = 0;
		return;
	}
	zfree(chart->excl_if);
	zfree(chart->color);
	zfree(chart->label);
	zfree(chart->value[0].expr);
	zfree(chart->value[1].expr);
	zfree(chart->value[2].expr);
	zfree(chart->value[3].expr);

	for (i=item->ch_curr; i < item->ch_ncomp; i++)
		item->ch_comp[i] = item->ch_comp[i+1];
	if (item->ch_curr == item->ch_ncomp)
		item->ch_curr--;
}


/*
 * copy a chart component to an empty component buffer
 */

void clone_chart_component(
	CHART		*to,
	CHART		*from)
{
	*to = *from;
	to->excl_if	  = mystrdup(from->excl_if);
	to->color	  = mystrdup(from->color);
	to->label	  = mystrdup(from->label);
	to->value[0].expr = mystrdup(from->value[0].expr);
	to->value[1].expr = mystrdup(from->value[1].expr);
	to->value[2].expr = mystrdup(from->value[2].expr);
	to->value[3].expr = mystrdup(from->value[3].expr);
}
