/*
 * functions relating to charts, except drawing functions which are in
 * chartdraw.c.
 *
 * add_chart_component		implements ADD button in the form editor
 * del_chart_component		implements DELETE button in the form editor
 * clone_chart_component	copies a chart component and its contents
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include "grok.h"
#include "form.h"
#include "proto.h"


/*
 * add a chart component to an item
 */

void add_chart_component(
	ITEM		*item)
{
	CHART		*array;
	CHART		*chart;
	int		i;

	array = (CHART *)(item->ch_ncomp++
		? realloc(item->ch_comp, item->ch_ncomp * sizeof(CHART))
		: calloc(item->ch_ncomp, sizeof(CHART)));
	if (!array)
		fatal("no memory");
	for (i=item->ch_ncomp-1; i > item->ch_curr; i--)
		array[i] = array[i-1];
	if (item->ch_curr < item->ch_ncomp-1)
		item->ch_curr++;
	item->ch_comp = array;
	chart = &array[item->ch_curr];
	memset((void *)chart, 0, sizeof(CHART));
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
			free((void *)item->ch_comp);
		item->ch_comp  = 0;
		item->ch_ncomp = 0;
		return;
	}
	if (chart->excl_if)		free((void *)chart->excl_if);
	if (chart->color)		free((void *)chart->color);
	if (chart->label)		free((void *)chart->label);
	if (chart->value[0].expr)	free((void *)chart->value[0].expr);
	if (chart->value[1].expr)	free((void *)chart->value[1].expr);
	if (chart->value[2].expr)	free((void *)chart->value[2].expr);
	if (chart->value[3].expr)	free((void *)chart->value[3].expr);

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
