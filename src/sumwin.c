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

static      QTreeWidget	*w_summary = NULL;
static void sum_callback(CARD *card, int r);

QWidget *create_summary_widget()
{
	w_summary  = new QTreeWidget(mainwindow);
	bind_help(w_summary, "summary"); // formerly set in mainwin.c
	w_summary->setItemsExpandable(false);
	// no decorations should follow from not expandable, but set to be sure
	w_summary->setRootIsDecorated(false);
	w_summary->setUniformRowHeights(true);
	w_summary->setSelectionMode(QAbstractItemView::SingleSelection);
	w_summary->setSelectionBehavior(QAbstractItemView::SelectRows);

	w_summary->header()->setSectionsMovable(false);
	w_summary->header()->setSectionResizeMode(QHeaderView::Fixed);
	w_summary->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	w_summary->setHeaderHidden(true);
	w_summary->setMinimumSize(400, 20);

	return w_summary;
}

/*
 * create a summary in a list widget, based on the card struct. Some query
 * must have been done earlier, so that card->query is defined. Install the
 * summary in form widget wform.
 */

void create_summary_menu(
	CARD		*card)		/* card with query results */
{
	CARD		dummy;		/* if no card yet, use empty card */
	char		*buf = NULL;		/* summary line buffer */
	size_t		buf_len;
	int		n, w, h;

	if (!w_summary)
		return;
	QSignalBlocker sb(w_summary);
	w_summary->setUpdatesEnabled(false);
	/* undo previous cb, if present */
	w_summary->disconnect(SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
	/* connect to new card */
	if (card)
		set_qt_cb(QTreeWidget, currentItemChanged, w_summary,
			  sum_callback(card, w_summary->indexOfTopLevelItem(c)),
			  QTreeWidgetItem *c);
	print_info_line();
	if (!card) {
		tzero(CARD, &dummy, 1);
		card = &dummy;
	}

	w_summary->clear();
	for (n=0; n < card->nquery; n++)
		make_summary_line(&buf, &buf_len, card, card->query[n], w_summary);
	make_summary_header(&buf, &buf_len, card, w_summary);
	if(card->nquery)
		w_summary->setCurrentItem(w_summary->topLevelItem(0));

	// Fit view to data.  FIXME:  How do I add line-separators?  Or is
	// increasing paddding all I can do?
	if (!card->nquery && !*buf) {
		w_summary->setHeaderHidden(true);
		w = 400;
	} else {
		w_summary->setHeaderHidden(false);
		w_summary->header()->setStretchLastSection(false);
		w_summary->resize(10, 10);
		for(n = 0; n < w_summary->columnCount(); n++)
			w_summary->resizeColumnToContents(n);
		// tjm - FIXME:  How do I get the max line width?
		w = w_summary->verticalScrollBar()->width() +
			w_summary->header()->length();
		w_summary->header()->setStretchLastSection(true);
	}
	w_summary->setMinimumWidth(w);

	free(buf);

	// not sure how to set # of visible lines to pref.sumlines
	// I'll just measure a row and add the header height.
	if(!n) {
		QTreeWidgetItem *twi = new QTreeWidgetItem;
		twi->setText(0, "QT is a pain in the ass");
		w_summary->addTopLevelItem(twi);
	}
	h = w_summary->sizeHintForRow(0) * pref.sumlines +
		w_summary->header()->height();
	if(!n)
		w_summary->clear();
	w_summary->setMinimumHeight(h);
	w_summary->updateGeometry();

	if(n) {
		scroll_summary(card);
		if(card->qcurr >= 0 && card->qcurr < card->nquery) {
			card->row = card->query[card->qcurr];
			fillout_card(card, false);
		}
	}
	card->wsummary = w_summary;
	w_summary->setUpdatesEnabled(true);
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

static bool in_sum_callback = false;
static void sum_callback(CARD *card, int item_position)
{
	if (!card || !card->wsummary)
		return;
	in_sum_callback = true;
	card_readback_texts(card, -1);
	in_sum_callback = false;
	if (item_position < card->nquery && item_position >= 0 &&
	    item_position != card->qcurr) {
		card->qcurr = item_position;
		card->row   = card->query[card->qcurr];
		fillout_card(card, false);
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
	const void *u,
	const void *v)
{
	const struct sum_item *mi0 = (const struct sum_item *)u;
	const struct sum_item *mi1 = (const struct sum_item *)v;
	if (mi0->sumcol != mi1->sumcol)
		return mi0->sumcol - mi1->sumcol;
	return mi0->sumoff - mi1->sumoff;
}

static void add_fkey_summary(struct sum_item **res, size_t *nres,
			     int itemno, int sumcol, int &ncol,
			     CARD *rcard)
{
	ITEM *item = rcard->form->items[itemno];
	resolve_fkey_fields(item);
	FORM *fform = item->fkey_form;
	if(!fform)
		return;
	CARD *card = create_card_menu(fform, read_dbase(fform));
	card->fkey_next = rcard;
	card->qcurr = itemno;
	for (int i = 0; i < item->nfkey; i++) {
		if (!item->fkey[i].display)
			continue;
		const FKEY &fk = item->fkey[i];
		if(!fk.item)
			break;
		int fitno = fk.index % fform->nitems;
		ITEM *fit = fk.item;
		if (fit->type == IT_FKEY) {
			add_fkey_summary(res, nres, fitno, sumcol, ncol, card);
			continue;
		}
		grow(0, "summary", struct sum_item, *res, ncol + 1, nres);
		(*res)[ncol].fcard = card;
		
		(*res)[ncol].sumcol = sumcol;
#if 0
		(*res)[ncol].sumoff = IFL(fit->,MULTICOL) ? fk.menu->sumcol :
							    fit->sumcol;
#else
		(*res)[ncol].sumoff = ncol;
#endif
		(*res)[ncol].item = fit;
		(*res)[ncol++].menu = fk.menu;
	}
}

int get_summary_cols(struct sum_item **res, size_t *nres, const CARD *card)
{
	int i, ncol, m;
	const FORM *form = card->form;

	for(i=ncol=0,m=-1; i < form->nitems; i++) {
		ITEM *item = form->items[i];
		if (!IN_DBASE(item->type)) // FIXME: allow Print in summary
			continue;
		if (!IFL(item->,MULTICOL) && item->sumwidth <= 0)
			continue;
		if (IFL(item->,MULTICOL)) {
			for(m++; m < item->nmenu; m++)
				if(item->menu[m].sumwidth > 0)
					break;
			if(m == item->nmenu) {
				m = -1;
				continue;
			}
			i--; // keep processing same item
		}
		if (item->type == IT_FKEY) {
			add_fkey_summary(res, nres, i, item->sumcol, ncol,
					 const_cast<CARD *>(card));
			continue;
		}
		grow(0, "summary", struct sum_item, *res, ncol + 1, nres);
		(*res)[ncol].fcard = const_cast<CARD *>(card);
		(*res)[ncol].sumcol = IFL(item->,MULTICOL) ? item->menu[m].sumcol :
							     item->sumcol;
		(*res)[ncol].sumoff = -1;
		(*res)[ncol].item = item;
		(*res)[ncol++].menu = IFL(item->,MULTICOL) ? &item->menu[m] : NULL;
	}
	qsort(*res, ncol, sizeof(struct sum_item), compare);
	return ncol;
}

void free_summary_cols(
	struct sum_item	*cols,
	size_t		ncols)
{
	for (size_t i = 0; i < ncols; i++)
		if(cols[i].fcard->fkey_next) {
			CARD *p = cols[i].fcard;
			do {
				for (size_t j = i + 1; j < ncols; j++)
					if (cols[j].fcard->fkey_next) {
						CARD **q = &cols[j].fcard;
						while (*q != p && (*q)->fkey_next)
							q = &(*q)->fkey_next;
						if ((*q)->fkey_next) {
							CARD *p2;
							for (p2 = *q; p2->fkey_next; p2 = p2->fkey_next);
							*q = p2;
						}
					}
			} while ((p = p->fkey_next) && p->fkey_next);
			free_fkey_card(cols[i].fcard);
		}
	free(cols);
}

static int get_fkey_field(const CARD *fcard, int r)
{
	if (!fcard->fkey_next)
		return r;
	if (r < 0)
		return -1;
	r = get_fkey_field(fcard->fkey_next, r);
	ITEM *fitem = fcard->fkey_next->form->items[fcard->qcurr];
	const char *key = dbase_get(fcard->fkey_next->dbase, r, fitem->column);
	/* FIXME:  Somehow generate multiple rows if keyno can be > 0 */
	resolve_fkey_fields(fitem);
	return fkey_lookup(fcard->dbase, fcard->fkey_next->form, fitem, key);
}

char *summary_display(const char *&data, CARD *card, int row, const ITEM *item,
		      const MENU *menu)
{
	int j;
	const FORM *form = card->form;

	if (item->sumprint) {
		int saved_row = card->row;
		card->row = row;
		data = evaluate(card, item->sumprint);
		card->row = saved_row;
		return 0;
	}
	if (!data)
		return 0;
	if ((item->type == IT_CHOICE || item->type == IT_FLAG)
	    && item->flagtext && item->flagcode
	    && !strcmp(data, item->flagcode))
		data = item->flagtext;
	if ((item->type == IT_MENU || item->type == IT_RADIO)) {
		for(j = 0; j < item->nmenu; j++)
			if(!strcmp(item->menu[j].flagcode, data))
				break;
		if(j < item->nmenu && /* else invalid code! */
		   !BLANK(item->menu[j].flagtext))
			data = item->menu[j].flagtext;
	}
	if (menu && !BLANK(menu->flagtext) &&
	    !strcmp(data, menu->flagcode))
		data = menu->flagtext;
	if (!IFL(item->,MULTICOL) && (item->type == IT_FLAGS || item->type == IT_MULTI)) {
		int qbegin, qafter;
		char sep, esc;
		char *v = NULL;
		int nv = 0;
		get_form_arraysep(form, &sep, &esc);
		for(j = 0; j < item->nmenu; j++) {
			MENU *menu = &item->menu[j];
			if(find_unesc_elt(data, menu->flagcode,
					  &qafter, &qbegin,
					  sep, esc)) {
				char *e = BLANK(menu->flagtext) ?
					menu->flagcode : menu->flagtext;
				if(!set_elt(&v, nv++, e, form))
					fatal("No memory");
				if(v == e)
					v = mystrdup(v);
			}
		}
		data = v;
		return v;
	}
	if (item->type == IT_NUMBER && item->digits >= 0) {
		char *v = qstrdup(qsprintf("%.*f", item->digits, atof(data)));
		data = v;
		return v;
	}
	return 0;
}

void make_summary_line(
	char		**buf,		/* text buffer for result line */
	size_t		*buf_len,	/* allocated length of *buf */
	const CARD	*card,		/* card with query results */
	int		row,		/* database row */
	QTreeWidget	*w,		/* non-0: add line to table widget */
	int		lrow)		/* >=0: replace row #lrow */
{
	static size_t	nsorted = 0;	/* size of <sorted> array */
	static struct sum_item	*sorted = 0;	/* sorted item pointer list */
	const char	*data;		/* data string from the database */
	int		data_len;
	const ITEM	*item;		/* contains info about formatting */
	const MENU	*menu;
	int		x = 0;		/* index to next free char in buf */
	int		i, j, l;
	QTreeWidgetItem *twi = 0;	
	int		lcol = 0;
	int		ncol;
	char		*allocdata = NULL;
	int		sumwidth, sumheight = 0;

	if(!*buf)
		*buf = alloc(0, "summary line", char, (*buf_len = 80));
	**buf = 0;
	if (!card || !card->dbase || !card->form || row >= card->dbase->nrows)
		return;
	/* FIXME:  only reuse sorted if it's the same */
	/*         otherwise call free_summary_cols() */
	ncol = get_summary_cols(&sorted, &nsorted, card);

	for (i=0; i < ncol; i++) {
		item = sorted[i].item;
		menu = sorted[i].menu;
		int r = row;
		if (r >= 0 && sorted[i].fcard->fkey_next)
			r = get_fkey_field(sorted[i].fcard, r);
		if(r < 0 && row >= 0) {
			data = 0;
			data_len = 0;
		} else if(r < 0) {
			data = item->type==IT_CHOICE ? item->name : item->label;
			if(menu)
				data = menu->label;
			data_len = BLANK(data) ? 0 : strlen(data);
			if (data_len && data[data_len-1] == ':')
				--data_len;
			// skip over rest of IT_CHOICE group
			// only save the matching choice, if any
			if (item->type == IT_CHOICE) {
				for(i++; i < ncol; i++)
					if(sorted[i].item->type != IT_CHOICE ||
					   sorted[i].item->sumcol != item->sumcol ||
					   sorted[i].fcard != sorted[i - 1].fcard)
						break;
				i--;
			}
		} else {
			data = dbase_get(sorted[i].fcard->dbase, r, menu?
					 menu->column :
					 item->column);
			// skip over rest of IT_CHOICE group
			// only save the matching choice, if any
			if (item->type == IT_CHOICE) {
				for(i++; i < ncol; i++) {
					if(sorted[i].item->type != IT_CHOICE ||
					   sorted[i].item->sumcol != item->sumcol ||
					   sorted[i].fcard != sorted[i - 1].fcard)
						break;
					if(!BLANK(data) &&
					   !strcmp(data, STR(sorted[i].item->flagcode)))
						item = sorted[i].item;
				}
				i--;
			}
			allocdata = summary_display(data, sorted[i].fcard, r,
						    item, menu);
			data_len = BLANK(data) ? 0 : strlen(data);
		}
		sumwidth = menu ? menu->sumwidth : item->sumwidth;
#if 0 /* I don't think arbitrary cutoffs are good */
		if (sumwidth > 80)
			sumwidth = 80;
#endif
		/* pass 1:  count # of lines (up to form->sumheight + 1) */
		/* trim whitespace off end of data first */
		while(data_len > 0 && isspace(data[data_len - 1]))
			data_len--;
		for (j = 0, l = card->form->sumheight; j < data_len; j++)
			if (data[j] == '\n' && !l--) {
				l++;
				data_len = j;
				break;
			}
		l = card->form->sumheight - l + 1;
		if (l < sumheight)
			l = sumheight;

		grow(0, "summary line", char, *buf, (x + sumwidth + 3) * l, buf_len);

		/* pass 1: shorten lines of data and set widget text */
		const char *nl = data, *lp = data;
		char *dp = *buf + x;
		while (1) {
			while(nl < data + data_len && *nl != '\n') {
				if(nl < lp + sumwidth)
					*dp++ = *nl;
				nl++;
			}
			if(nl == data + data_len)
				break;
			*dp++ = *nl++;
			lp = nl;
		}
		*dp = 0;
		if(w) {
			if(!twi) {
				if(row >= 0 && lrow >= 0) {
					twi = w->topLevelItem(lrow);
					// FIXME:  this should never happen
					if(!twi)
						w->addTopLevelItem((twi = new QTreeWidgetItem));
				} else
					twi = new QTreeWidgetItem;
			}
			twi->setText(lcol++, *buf + x);
		}
		/* pass 2: insert each line into multi-line result */
		if (l == 1) {
			memset(dp, ' ', sumwidth - (dp - (*buf + x)) + 2);
			x += sumwidth + 2;
			(*buf)[x] = 0;
			zfree(allocdata);
			allocdata = NULL;
			continue;
		}
		int rl = l;
		char *ip = *buf + x;
		while(sumheight-- > 1) {
			dp = ip;
			memmove(ip + sumwidth + 2, ip, x * sumheight);
			int p;
			for(p = 0; p < data_len && p < sumwidth &&
				       data[p] != '\n'; p++)
				*dp++ = data[p];
			data += p;
			data_len -= p;
			memset(dp, ' ', (p < sumwidth ? sumwidth - p : 0) + 2);
			while(data_len && data[p] != '\n') {
				data_len--;
				data++;
				p++;
			}
			if(data_len) {
				data_len--;
				data++;
			}
			ip += x + sumwidth + 3;
			rl--;
		}
		while(data_len) {
			int p;
			for (p = 0; data_len && p < sumwidth; p++, data_len--) {
				if(*data == '\n')
					break;
				*ip++ = *data++;
			}
			memset(ip, ' ', sumwidth - p + 2);
			ip += sumwidth - p + 2;
			while(data_len && *data != '\n') {
				data_len--;
				data++;
			}
			if(data_len) {
				data_len--;
				data++;
				*ip++ = '\n';
				memset(ip, ' ', x);
				ip += x;
			} else
				*ip = 0;
		}
		sumheight = l;
		zfree(allocdata);
		allocdata = NULL;
		x += sumwidth + 2;
	}
	if (x == 0 && row >= 0)
		sprintf(*buf, "card %d", row);
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
	if (in_sum_callback)
		return;
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
	char		*buf = 0;	/* summary line buffer */
	size_t		buf_len;

	if (!card || !card->dbase || !card->form || row >= card->dbase->nrows)
		return;
	make_summary_line(&buf, &buf_len, card, row, card->wsummary, card->qcurr);
	free(buf);
	scroll_summary(card);
}
