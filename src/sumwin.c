/*
 * create and display a summary list.
 *
 *	destroy_summary_menu(card)	destroy summary list in form widget
 *	create_summary_menu(card,wf,sh)	create summary list in form widget
 *	scroll_summary(card)		scroll summary such that current row
 *					is highlighted and visible
 *	make_summary_line(buf,card,row)	make string describing one card
 */

// tjm - instead of making COURIER text lines that line up due to fixed spacing
//       this now uses a QTreeWidget with auto-adjusted lines between columns
//       and a "real" header line.  The old make_summary_line() is still here
//       for printing, mainly

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void sum_callback(void);


/*
 * create a summary in a list widget, based on the card struct. Some query
 * must have been done earlier, so that card->query is defined. Install the
 * summary in form widget wform.
 */

void create_summary_menu(
	CARD		*card)		/* card with query results */
{
	CARD		dummy;		/* if no card yet, use empty card */
	char		buf[1024];	/* summary line buffer */
	int		n, w, h;
	QTreeWidget	*newsum;

	if (!mainwindow)
		return;
	print_info_line();
	if (!card)
		memset((void *)(card = &dummy), 0, sizeof(dummy));
	card->wsummary = NULL;
	newsum  = new QTreeWidget(mainwindow);
	bind_help(newsum, "summary"); // formerly set in mainwin.c
	newsum->setItemsExpandable(false);
	// no decorations should follow from not expandable, but set to be sure
	newsum->setRootIsDecorated(false);
	newsum->setUniformRowHeights(true);
	newsum->setSelectionMode(QAbstractItemView::SingleSelection);
	newsum->setSelectionBehavior(QAbstractItemView::SelectRows);

	for (n=0; n < card->nquery; n++)
		make_summary_line(buf, card, card->query[n], newsum);
	make_summary_line(buf, card, -1, newsum);
	newsum->header()->setSectionsMovable(false);
	newsum->header()->setSectionResizeMode(QHeaderView::Fixed);
	newsum->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	// Fit view to data.  FIXME:  How do I add line-separators?  Or is
	// increasing paddding all I can do?
	if (!card->nquery && !*buf) {
		newsum->header()->hide();
		w = 400;
	} else {
		newsum->resize(10, 10);
		for(n = 0; n < newsum->columnCount(); n++)
			newsum->resizeColumnToContents(n);
		// tjm - FIXME:  How do I get the max line width?
		w = newsum->verticalScrollBar()->width() +
			newsum->header()->length();
	}
	newsum->setMinimumWidth(w);

	// not sure how to set # of visible lines to pref.sumlines
	// I'll just measure a row and add the header height.
	if(!n) {
		QTreeWidgetItem *twi = new QTreeWidgetItem;
		twi->setText(0, "QT is a pain in the ass");
		newsum->addTopLevelItem(twi);
	}
	h = newsum->sizeHintForRow(0) * pref.sumlines +
		newsum->header()->height();
	if(!n)
		newsum->clear();
	newsum->setMinimumHeight(h);
	newsum->resize(w, h);
	set_qt_cb(QTreeWidget, itemSelectionChanged, newsum,
		  sum_callback());
	if(n) {
		scroll_summary(card);
		if(card->qcurr >= 0 && card->qcurr < card->nquery) {
			card->row = card->query[card->qcurr];
			fillout_card(card, FALSE);
		}
	}
	delete mainform->replaceWidget(w_summary, newsum);
	delete w_summary;
	w_summary = card->wsummary = newsum;
}


/*
 * user pressed on a line. Read back changes to the old card displayed, if
 * any, and display the new one.
 */

static int selected_item(CARD *card)
{
	QList<QTreeWidgetItem *> sel = card->wsummary->selectedItems();
	if (sel.empty())
		return -1;
	return card->wsummary->indexOfTopLevelItem(sel.first());
}

static void sum_callback(void)
{
	int			item_position;
	register CARD		*card = curr_card;

	if (!card || !card->wsummary)
		return;
	card_readback_texts(card, -1);
	item_position = selected_item(card);
	if (item_position < card->nquery && item_position >= 0 &&
	    item_position != card->qcurr) {
		card->qcurr = item_position;
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
	register const void *u,
	register const void *v)
{
	return((*(ITEM **)u)->sumcol - (*(ITEM **)v)->sumcol);
}


void make_summary_line(
	char		*buf,		/* text buffer for result line */
	CARD		*card,		/* card with query results */
	int		row,		/* database row */
	QTreeWidget	*w,		/* non-0: add line to table widget */
	int		lrow)		/* >=0: replace row #lrow */
{
	static int	nsorted;	/* size of <sorted> array */
	static ITEM	**sorted;	/* sorted item pointer list */
	const char	*data;		/* data string from the database */
	char		databuf[200];	/* buffer for data with ':' stripped */
	ITEM		*item;		/* contains info about formatting */
	int		x = 0;		/* index to next free char in buf */
	int		i;		/* item counter */
	int		j;		/* char copy counter */
	QTreeWidgetItem *twi = 0;	
	int		lcol = 0;

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
		if(w) {
			if(!twi) {
				if(row >= 0 && lrow >= 0)
					twi = w->topLevelItem(lrow);
				else
					twi = new QTreeWidgetItem;
			}
			twi->setText(lcol++, buf + x);
		}
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
	if(w && twi) {
		if(row < 0)
			w->setHeaderItem(twi);
		else if(lrow < 0)
			w->addTopLevelItem(twi);
	}
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

void make_plan_line(
	CARD		*card,		/* card with query results */
	int		row)		/* database row */
{
	static int	*sorted;	/* maps plan columns to grok columns */
	const char	*data;		/* data string from the database */
	char		sep = 0;	/* separator */
	ITEM		*item;		/* contains info about formatting */
	int		i, j;		/* grok item counter */
	int		c;		/* plan column counter */

	if (!card || !card->dbase || !card->form || row < 0
						 || row >= card->dbase->nrows)
		return;
	if (!sorted) {
		sorted = (int *)malloc(strlen(plan_code) * sizeof(int));
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
	if (card->wsummary && card->qcurr < card->nquery) {
		QTreeWidgetItem *twi = card->wsummary->topLevelItem(card->qcurr);
		if (selected_item(card) != card->qcurr)
			card->wsummary->setCurrentItem(twi);
		card->wsummary->scrollToItem(twi);
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

	if (!card || !card->dbase || !card->form || row >= card->dbase->nrows)
		return;
	make_summary_line(buf, card, row, card->wsummary, card->qcurr);
	scroll_summary(card);
}
