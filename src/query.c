/*
 * Do a query into the database, and store a query list into the card. A
 * query list is an 1D array of row numbers in the database. Also set the
 * number of cards from the query in nquery, and pick the card to be
 * displayed first by setting qcurr.
 *
 *	match_card(card, string)
 *	query_any(mode, card, string)
 *	query_none(card)
 *	query_all(card)
 *	query_search(mode, card, string)
 *	query_letter(card, letter)
 *	query_eval(mode, card, expr)
 */

/* FIXME: change the use of a single mask to "store" previous queries when
 *        narrowing to the use of a linked list of searches; that way
 *        the search stack can survive database changes */
/*        This would also fix the letter buttons, as they can just pop the
 *        last letter query off the stack */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define SECT_OK(db,r) ((db)->currsect < 0 ||\
		       (db)->currsect == (db)->row[r]->section)

static bool alloc_query(CARD *, char **, bool);
static bool search_matches_card(CARD *, const char *);
static int expr_matches_card(CARD *, const char *);

/* FIXME:  this only works for US ASCII.  No Latin-1, no UTF-8, etc.  */
static const char *strlower(const char *string)
{
	static char	*search = 0;	/* lower-case search string */
	static size_t	search_len;
	const char	*p;
	char		*q;		/* copy and comparison pointers */
	grow(0, "search string", char, search, strlen(string) + 1, &search_len);
	for (p=string, q=search; *p; p++)
		*q++ = *p | 0x20;
	*q = 0;
	return search;
}


/* true if q in s, a la strcasestr */
/* unlike strcasestr, this requires q to be "lower-case" as per strlower above */
/* and compares s as if it were converted as per strlower above */
static bool lc_in(const char *s, const char *q)
{
	if(!s || !*s || !q || !*q)
		return(false);
	while(1) {
		const char *c, *p;
		for (p = s, c = q; *c && *p; c++, p++)
			/* FIXME:  this only works for US ASCII.  No Latin-1, no UTF-8, etc.  */
			if ((*p | 0x20) != *c)
				break;
		if (!*c)
			return(true);
		if (!*p)
			return(false);
		s++;
	}
}


/*
 * make sense out of the string, and return true if the given card matches
 * the given search or expression string.
 * returns -1 if expr had an error; otherwise 0/1 (false/true)
 */

int match_card(
	CARD		*card,		/* database and form */
	char		*string)	/* query string */
{

	if (!string || (*string == '*' && !string[1]))
		return(true);
	else if (*string == '(' || *string == '{')
		return(expr_matches_card(card, string));
	else {
		if (*string == '/')
			string++;
		return(search_matches_card(card, strlower(string)));
	}
}


/*
 * make sense out of the string, and do the appropriate search
 */

void query_any(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	const char	*string)	/* query string */
{
	if (!string || (*string == '*' && !string[1]))
		query_all(card);
	else if (*string == '(' || *string == '{')
		query_eval(mode, card, string);
	else if (*string == '/')
		query_search(mode, card, string+1);
	else
		query_search(mode, card, string);
}


/*
 * delete query, summary becomes empty
 */

void query_none(
	CARD		*card)		/* database and form */
{
	zfree(card->query);
	card->query  = 0;
	card->nquery = 0;
	card->qcurr  = 0;
}


/*
 * true if card restriction matches
 */

static bool fkey_restrict(const CARD *card, int row)
{
	if(card->rest_item < 0)
		return true;
	const ITEM *item = card->form->items[card->rest_item];
	const char *val = dbase_get(card->form->dbase, row, item->column);
	if(BLANK(val))
		return false;
	if(!IFL(item->,FKEY_MULTI))
		return !strcmp(val, card->rest_val);
	char sep, esc;
	get_form_arraysep(card->form, &sep, &esc);
	int b, a;
	/* rest_val is already escaped */
	return find_elt(val, card->rest_val, strlen(card->rest_val), &b, &a,
			sep, esc);
}

/*
 * put all cards into the summary
 */

void query_all(
	CARD		*card)		/* database and form */
{
	int		r, n;

	if (!alloc_query(card, 0, false))
		return;
	for (r=n=0; r < card->form->dbase->nrows; r++) {
		int sr = card->sorted ? card->sorted[r] : r;
		if (SECT_OK(card->form->dbase, sr) && fkey_restrict(card, sr))
			card->query[n++] = sr;
	}
	card->nquery = n;
}


/*
 * put all cards into the summary that contain a search string in any of
 * the searchable items. The search is case-insensitive. This code will
 * gleefully fail if run on a non-ascii system.
 */

void query_search(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	const char	*string)	/* string to search for */
{
	char		*mask;		/* for skipping unselected cards */
	const char	*search;	/* lower-case search string */
	DBASE		*dbase = card->form->dbase;

	if (!alloc_query(card, &mask, false))
		return;
	search = strlower(string);
	if (mode == SM_SEARCH && pref.incremental)
		mode = SM_NARROW;
	for (int row=0; row < dbase->nrows; row++) {
		card->row = card->sorted ? card->sorted[row] : row;
		if (!SECT_OK(dbase, card->row) ||
		    !fkey_restrict(card, card->row))
			continue;
		switch(mode) {
		  case SM_INQUERY:
		  case SM_WIDEN_INQUERY:
			if (card->last_query >= 0) {
				int res = expr_matches_card(card,
					card->form->query[card->last_query].query);
				if(res < 0)
					break;
				else if(!res)
					continue;
			}
			if (mode == SM_INQUERY)
				break;
			FALLTHROUGH
		  case SM_WIDEN:
			if (mask && mask[card->row]) {
				card->query[card->nquery++] = card->row;
				continue;
			}
			break;
		  case SM_NARROW:
			if (!mask || !mask[card->row])
				continue;
			break;
		  default: /* SEARCH FIND NMODES */ ;
		}
		if (search_matches_card(card, search))
			card->query[card->nquery++] = card->row;
	}
	zfree(mask);
}


/*
 * put all cards into the summary whose last-sorted-by column begins with
 * <letter>: 0..25=letter, 26=misc, 27=all. The search is case-insensitive.
 */

#define SKIP(c) ((c) && strchr("\t\n \"!#$%&'()*+,-./:;<=>?@[\\]^`{|}~", (c)))

void query_letter(
	CARD		*card,		/* database and form */
	int		letter)		/* 0..25=a..z, 26=misc, 27=all */
{
	int		r;		/* row counter */
	char		*data;		/* database string to test */
	char		*mask;		/* for skipping unselected cards */
	DBASE		*dbase = card->form->dbase;

	if (letter == 27) {
		query_all(card);
		return;
	}
	if (!alloc_query(card, &mask, true))
		return;
	letter = letter == 26 ? 0 : letter+'A';
	if (card->col_sorted_by >= dbase->maxcolumns-1)
		card->col_sorted_by = 0;
	for (r=0; r < dbase->nrows; r++) {
		int sr = card->sorted ? card->sorted[r] : r;
		if (SECT_OK(dbase, sr) && fkey_restrict(card, sr) &&
		    (!mask || mask[sr])) {
			data = dbase_get(dbase, sr, card->col_sorted_by);
			while (data && SKIP(*data))
				data++;
			if ((!data || !*data || strchr("0123456789_", *data))
								&& !letter)
				card->query[card->nquery++] = sr;
			else if (data && letter)
				while (*data) {
					if ((*data & ~0x20) == letter) {
						card->query[card->nquery++] = sr;
						break;
					}
					if (!pref.allwords)
						break;
					while (*data && !SKIP(*data))
						data++;
					while (SKIP(*data))
						data++;
				}
		}
	}
	zfree(mask);
}


/*
 * collect all cards that match the query expression. The expression is
 * said to match if it does not return an empty string, or a string that,
 * after stripping leading white space, begins with '0', 'f' or 'F'. The
 * F's are supposed to mean False.
 */

void query_eval(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	const char	*expr)		/* expression to apply to dbase */
{
	char		*mask;		/* for skipping unselected cards */
	int		match;		/* to detect/skip errors */
	DBASE		*dbase = card->form->dbase;

	if (!alloc_query(card, &mask, false))
		return;
	if (mode == SM_SEARCH && pref.incremental)
		mode = SM_NARROW;
	for (int row=0; row < dbase->nrows; row++) {
		card->row = card->sorted ? card->sorted[row] : row;
		if (!SECT_OK(dbase, card->row) ||
		    !fkey_restrict(card, card->row))
			continue;
		switch(mode) {
		  case SM_INQUERY:
			if (card->last_query >= 0) {
				match = expr_matches_card(card,
					card->form->query[card->last_query].query);
				if(match < 0)
					break;
				else if(!match)
					continue;
			}
			break;
		  case SM_NARROW:
			if (!mask || !mask[card->row])
				continue;
			break;
		  case SM_WIDEN:
			if (mask && mask[card->row]) {
				card->query[card->nquery++] = card->row;
				continue;
			}
		  default: /* SEARCH WINDEN_INQUERY FIND NMODES */ ;
		}
		match = expr_matches_card(card, expr);
		if (match < 0)
			break;
		if (match)
			card->query[card->nquery++] = card->row;
	}
	zfree(mask);
}


/*
 * prepare for a query: destroy the previous query list, and allocate a new
 * one with enough entries even if all cards match the query. The query list
 * is a list of row numbers in the dbase. Return true if everything went well.
 * If <mask> is nonzero and incremental searches are enabled, create a string
 * with one byte per card, which is 1 if that card was in the previous query.
 * This makes it easy for the search routines to skip cards that have not been
 * in the old query.
 */

static bool alloc_query(
	CARD		*card,		/* database and form */
	char		**mask,		/* where to store pointer to string */
	bool		is_letter)	/* if true, mask is static */
{
	DBASE		*dbase = card->form->dbase;
	if (mask) {
		int r;
		*mask = 0;
		/* FIXME: this should do no more if !dbase->nrows */
		/* FIXME: should delete any time database has been modified */
		if (!is_letter || card->lm_size != dbase->nrows) {
			zfree(card->letter_mask);
			card->letter_mask = 0;
		}
		/* what's the point of using abort_malloc here? */
		if (card->query && (!is_letter || !card->letter_mask) &&
		    !(*mask = (char *)malloc(dbase->nrows))) {
			create_error_popup(mainwindow, errno,
					   "No memory for query result summary");
			return(false);
		}
		if (is_letter && card->letter_mask) {
			if (*mask)
				memcpy(*mask, card->letter_mask, dbase->nrows);
		} else if (*mask) {
			(void)memset(*mask, 0, dbase->nrows);
			for (r=0; r < card->nquery; r++)
				(*mask)[card->query[r]] = 1;
			if (is_letter) {
				card->letter_mask = alloc(0, "result summary", char, dbase->nrows);
				memcpy(card->letter_mask, *mask, dbase->nrows);
				card->lm_size = dbase->nrows;
			}
		}
	}
	query_none(card);
	if (!dbase->nrows)
		return(false);
	if (!(card->query = (int*)malloc(dbase->nrows * sizeof(int*)))) {
		create_error_popup(mainwindow, errno,
					"No memory for query result summary");
		if(mask && *mask) {
			free(*mask);
			*mask = 0;
		}
		return(false);
	}
	*card->query = 0;
	return(true);
}


/*
 * return true if the given item contains the given search string.  This is
 * called recursively for foreign key items.
 */
static bool search_matches_item(
	FORM		*form,		/* form item is from */
	const DBASE	*dbase,		/* database item is from */
	int		row,		/* row of database */
	ITEM		*item,		/* item to search; writable for fkey */
	const char	*search)	/* lowercased string to search for */
{
	char		*data;		/* database string to test */

	/* FIXME: support PRINT, INV_FKEY */
	if (!IN_DBASE(item->type) || !IFL(item->,SEARCH))
		return(false);
	if (!(data = dbase_get(dbase, row, item->column)))
		return(false);
	if (item->type != IT_FKEY)
		return lc_in(data, search);
	resolve_fkey_fields(item);
	FORM *fform = item->fkey_form;
	if(!fform)
		return lc_in(data, search);
	int nvis = 0;
	for (int i = 0; i < item->nfkey; i++) {
		if(item->fkey[i].key && !item->fkey[i].item)
			return  lc_in(data, search);
		nvis += item->fkey[i].item && item->fkey[i].display;
	}
	if(!nvis)
		return(false);
	DBASE *fdb = read_dbase(fform);
	for (int k = 0; ; k++) {
		row = fkey_lookup(fdb, form, item, data, k);
		if(row == -2)
			return(false);
		if(row < 0)
			continue;
		for (int i = 0; i < item->nfkey; i++)
			if(item->fkey[i].display &&
			   search_matches_item(fform, fdb, row,
					       item->fkey[i].item,
					       search))
				return(true);
	}
}

/*
 * return true if the given card contains the given search string. This is
 * used by eval_search above, and by the find-and-select function in the File
 * pulldown (also Ctrl-F and a search mode).
 */

static bool search_matches_card(
	CARD		*card,		/* database and form */
	const char	*search)	/* lowercased string to search for */
{
	int		i;		/* item counter */

	for (i=0; i < card->form->nitems; i++)
		if(search_matches_item(card->form, card->form->dbase, card->row,
				       card->form->items[i], search))
			return(true);
	return(false);
}


/*
 * return true if the given card matches the given expression. This is also
 * used by eval_search above, and by the find-and-select function in the File
 * pulldown (also Ctrl-F and a search mode).
 * returns -1 if expr had an error; otherwise 0/1 (false/true)
 */

static int expr_matches_card(
	CARD		*card,		/* database and form */
	const char	*expr)		/* expression to test */
{
	const char	*val;		/* return from expression eval */

	if (!(val = evaluate(card, expr)))
		return(-1);
	while (*val == ' ' || *val == '\t') val++;
	return(*val && *val != '0' && *val != 'f' && *val != 'F');
}
