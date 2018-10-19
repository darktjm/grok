/*
 * create and display a summary list.
 *
 *	destroy_summary_menu(card)	destroy summary list in form widget
 *	create_summary_menu(card,wf,sh)	create summary list in form widget
 *	scroll_summary(card)		scroll summary such that current row
 *					is highlighted and visible
 *	make_summary_line(buf,card,row)	make string describing one card
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <assert.h>
#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/LabelP.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void sum_callback(Widget, XtPointer, XmListCallbackStruct *);

extern Display		*display;	/* everybody uses the same server */
extern GC		gc;		/* everybody uses this context */
extern struct pref	pref;		/* global preferences */
extern CARD 		*curr_card;	/* card being displayed in main win, */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern XmFontList	fontlist[NFONTS];


/*
 */

void destroy_summary_menu(
	register CARD	*card)		/* card to destroy */
{
	if (card->wsummary) {
		XtDestroyWidget(card->wsummary);
		card->wsummary = 0;
	}
}


/*
 * create a summary in a list widget, based on the card struct. Some query
 * must have been done earlier, so that card->query is defined. Install the
 * summary in form widget wform.
 */

void create_summary_menu(
	CARD		*card,		/* card with query results */
	Widget		wform,		/* form widget to install into */
	Widget		shell)		/* enclosing shell */
{
	CARD		dummy;		/* if no card yet, use empty card */
	char		buf[1024];	/* summary line buffer */
	XmStringTable	list;		/* string list for summary widget */
	int		nlines;		/* # of lines, at least pref.sumlines*/
	Arg		args[15];
	int		n;

	if (!display)
		return;
	print_info_line();
	if (!card)
		mybzero((void *)(card = &dummy), sizeof(dummy));
	if (card->wheader)
		XtDestroyWidget(card->wheader);
	if (card->wsummary)
		XtDestroyWidget(card->wsummary);
	card->wsummary  = 0;
	card->wsumshell = shell;

	nlines = card->nquery < pref.sumlines ? pref.sumlines : card->nquery;
	list = (XmStringTable)XtMalloc(nlines * sizeof(XmString *));
	for (n=0; n < card->nquery; n++) {
		make_summary_line(buf, card, card->query[n]);
		list[n] = XmStringCreateSimple(buf);
	}
	while (n < nlines)
		list[n++] = XmStringCreateSimple(" ");
	nlines = n;
	make_summary_line(buf, card, -1);
	strcat(buf, "                                                       ");
	XtUnmanageChild(wform);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	3);			n++;
	XtSetArg(args[n], XmNfontList,		fontlist[FONT_COURIER]);n++;
	card->wheader = XtCreateWidget(buf, xmLabelWidgetClass,
			wform, args, n);
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		card->wheader);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNselectionPolicy,	XmBROWSE_SELECT);	n++;
	XtSetArg(args[n], XmNvisibleItemCount,	pref.sumlines);		n++;
	XtSetArg(args[n], XmNitemCount,		nlines);		n++;
	XtSetArg(args[n], XmNitems,		list);			n++;
	XtSetArg(args[n], XmNfontList,		fontlist[FONT_COURIER]);n++;
	XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmSTATIC);		n++;
	card->wsummary = XmCreateScrolledList(wform, "sumlist",
			args, n);
	XtAddCallback(card->wsummary, XmNbrowseSelectionCallback,
				(XtCallbackProc)sum_callback, (XtPointer)0);
	XmListSelectPos(card->wsummary, 1, False);
	XtManageChild(card->wheader);
	XtManageChild(card->wsummary);
	XtManageChild(wform);

	for (n=0; n < nlines; n++)
		XmStringFree(list[n]);
	XtFree((char *)list);
}


/*
 * user pressed on a line. Read back changes to the old card displayed, if
 * any, and display the new one.
 */

/*ARGSUSED*/
static void sum_callback(
	Widget			widget,
	XtPointer		item,
	XmListCallbackStruct	*data)
{
	register CARD		*card = curr_card;

	card_readback_texts(card, -1);
	if (card && data->item_position-1 < card->nquery) {
		card->qcurr = data->item_position-1;
		card->row   = card->query[card->qcurr];
		fillout_card(card, FALSE);
		print_info_line();
	}
}


/*
 * compose one line for the summary from a data row. Before starting, the
 * items must be sorted by database column. Choice items are a special case
 * because all choice items with identical names share one column. Only if
 * the last choice item with a certain name doesn't match, print blanks.
 * if row < 0, create a header line.
 */

static int compare(
	register MYCONST void *u,
	register MYCONST void *v)
{
	return((*(ITEM **)u)->sumcol - (*(ITEM **)v)->sumcol);
}


void make_summary_line(
	char		*buf,		/* text buffer for result line */
	CARD		*card,		/* card with query results */
	int		row)		/* database row */
{
	static int	nsorted;	/* size of <sorted> array */
	static ITEM	**sorted;	/* sorted item pointer list */
	char		*data;		/* data string from the database */
	char		databuf[200];	/* buffer for data with ':' stripped */
	ITEM		*item;		/* contains info about formatting */
	int		x = 0;		/* index to next free char in buf */
	int		i;		/* item counter */
	int		j;		/* char copy counter */

	*buf = 0;
	if (!card || !card->dbase || !card->form || row >= card->dbase->nrows)
		return;
	if (card->form->nitems > nsorted) {
		nsorted = card->form->nitems + 3;
		if (sorted)
			free((void *)sorted);
		if (!(sorted = (ITEM **)malloc(nsorted * sizeof(ITEM *)))) {
			nsorted = 0;
			return;
		}
	}
	for (i=0; i < card->form->nitems; i++)
		sorted[i] = card->form->items[i];
	qsort((void *)sorted, card->form->nitems, sizeof(ITEM *), compare);

	for (i=0; i < card->form->nitems; i++) {
		item = sorted[i];
		if (!IN_DBASE(item->type) || item->sumwidth < 1)
			continue;
		data = row<0 ? item->type==IT_CHOICE ? item->name : item->label
			     : dbase_get(card->dbase, row, item->column);

		if (row < 0 && data && data[j = strlen(data)-1] == ':') {
			strncpy(databuf, data, sizeof(databuf));
			databuf[j] = 0;
			databuf[sizeof(databuf)-1] = 0;
			data = databuf;
		}
		if (item->type == IT_CHOICE && row < 0) {
			for (j=0; j < i; j++)
				if (item->sumcol == sorted[j]->sumcol)
					break;
			if (j < i)
				continue;
		}
		if (item->type == IT_CHOICE && row >= 0 &&
		    (!data || !item->flagcode || strcmp(data,item->flagcode))){
			for (j=i+1; j < card->form->nitems; j++)
				if (!strcmp(item->name, sorted[j]->name))
					break;
			if (j < card->form->nitems)
				continue;
			data = 0;
		}
		if ((item->type == IT_CHOICE || item->type == IT_FLAG)
			    && item->flagtext && item->flagcode && data
			    && !strcmp(data, item->flagcode))
			data = item->flagtext;
		if (item->type == IT_TIME && row >= 0)
			data = format_time_data(data, item->timefmt);
		if (data)
			strncpy(buf+x, data, 80);
		buf[x + item->sumwidth] = 0;
		for (j=x; buf[j]; j++)
			if (buf[j] == '\n')
				buf[j--] = 0;
		x += j = strlen(buf+x);
		for (; j < item->sumwidth && j < 80; j++)
			buf[x++] = ' ';
		buf[x++] = ' ';
		buf[x++] = ' ';
		buf[x]   = 0;
		while (++i < card->form->nitems-1 &&
					item->sumcol == sorted[i]->sumcol);
		i--;
	}
	if (x == 0)
		sprintf(buf, row < 0 ? "" : "card %d", row);
}


/*
 * similar to make_summary_line but sorts and formats the line for output
 * in a format that the plan program (www.bitrot.de/plan.html) can read.
 * While make_summary_line is used for the GUI and -t/-T, make_plan_line
 * is used for -p only.
 * The code assumes that it is only called from -p because it first builds
 * a mapping table that converts the columns expected by plan to the item
 * (column) index of grok, but only once, so the form editor can't be used
 * to change the column order.
 */

extern char plan_code[];		/* from formwin.c */

void make_plan_line(
	CARD		*card,		/* card with query results */
	int		row)		/* database row */
{
	static int	*sorted;	/* maps plan columns to grok columns */
	char		*data;		/* data string from the database */
	char		sep = 0;	/* separator */
	ITEM		*item;		/* contains info about formatting */
	int		i, j;		/* grok item counter */
	int		c;		/* plan column counter */

	if (!card || !card->dbase || !card->form || row < 0
						 || row >= card->dbase->nrows)
		return;
	if (!sorted) {
		sorted = malloc(strlen(plan_code) * sizeof(int));
		for (c=0; plan_code[c]; c++) {
			for (i=card->form->nitems-1; i >= 0; i--) {
				item = card->form->items[i];
				if (item->plan_if == plan_code[c])
					break;
			}
			sorted[c] = i;
		}
	}
	for (c=0; plan_code[c]; c++) {
		if (sep)
			putchar(sep);
		sep = ':';
		if (sorted[c] < 0 || !(item = card->form->items[sorted[c]]))
			continue;
		assert(item->plan_if && IN_DBASE(item->type));
		data = dbase_get(card->dbase, row, item->column);
		if (item->type == IT_CHOICE &&
		    (!data || !item->flagcode || strcmp(data,item->flagcode))){
			for (j=i+1; j < card->form->nitems; j++)
				if (!strcmp(item->name,
					   card->form->items[sorted[j]]->name))
					break;
			if (j < card->form->nitems)
				continue;
			data = 0;
		}
		if ((item->type == IT_CHOICE || item->type == IT_FLAG)
			    && item->flagtext && item->flagcode && data
			    && !strcmp(data, item->flagcode))
			data = item->flagtext;
		if (item->type == IT_TIME && row >= 0)
			data = format_time_data(data, item->timefmt);
		if (data)
			while (*data) {
				if (*data=='\\' || *data==':' || *data=='\n')
					putchar('\\');
				putchar(*data++);
			}
	}
	putchar('\n');
}


/*
 * scroll summary such that the current row is visible. This is used to jump
 * to the first or last row, and to make sure the current card is highlighted
 * after the user pressed the next or prev arrows below the card.
 */

void scroll_summary(
	CARD		*card)		/* which card's summary */
{
	Arg		args[2];
	int		top=0, nvis=0;

	if (card->wsummary && card->qcurr < card->nquery) {
		XtSetArg(args[0], XmNtopItemPosition,  &top);
		XtSetArg(args[1], XmNvisibleItemCount, &nvis);
		XtGetValues(card->wsummary, args, 2);
		if (card->qcurr+1 < top)
			XmListSetPos(card->wsummary, card->qcurr+1);
		else if (card->qcurr+1 >= top + nvis)
			XmListSetBottomPos(card->wsummary, card->qcurr+1);
		XmListSelectPos(card->wsummary, card->qcurr+1, False);
	}
}


/*
 * replace a line in the list. This is done whenever card_readback_texts()
 * reads text strings from the card menu back into the database. This
 * routine updates the summary window accordingly.
 */

void replace_summary_line(
	CARD		*card,		/* card with query results */
	int		row)		/* database row that has changed */
{
	char		buf[1024];	/* summary line buffer */
	XmString	list;		/* string that replaces summary line */

	if (!card || !card->dbase || !card->form || row >= card->dbase->nrows)
		return;
	make_summary_line(buf, card, row);
	list = XmStringCreateSimple(buf);
	XmListReplaceItemsPos(card->wsummary, &list, 1, card->qcurr+1);
	XmListSelectPos(card->wsummary, card->qcurr+1, False);
	XmStringFree(list);
}
