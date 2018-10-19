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

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <errno.h>
#include <Xm/Xm.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define SECT_OK(db,r) ((db)->currsect < 0 ||\
		       (db)->currsect == (db)->row[r]->section)

static BOOL alloc_query(CARD *, char **);
static BOOL search_matches_card(CARD *, char *);
static int expr_matches_card(CARD *, char *);


/*
 * make sense out of the string, and return TRUE if the given card matches
 * the given search or expression string.
 * returns -1 if expr had an error; otherwise 0/1 (FALSE/TRUE)
 */

int match_card(
	CARD		*card,		/* database and form */
	char		*string)	/* query string */
{
	char		search[1024];	/* lower-case search string */
	register char	*p, *q;		/* copy and comparison pointers */
	int		i;		/* search string index */

	if (!string || *string == '*' && !string[1])
		return(TRUE);
	else if (*string == '(' || *string == '{')
		return(expr_matches_card(card, string));
	else {
		if (*string == '/')
			string++;
		for (p=string, q=search, i=0; *p && i < sizeof(search)-1; i++)
			*q++ = *p++ | 0x20;
		*q = 0;
		return(search_matches_card(card, search));
	}
}


/*
 * make sense out of the string, and do the appropriate search
 */

void query_any(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	char		*string)	/* query string */
{
	if (!string || *string == '*' && !string[1])
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
	if (card->query)				/* clear old query */
		free((void *)card->query);
	card->query  = 0;
	card->nquery = 0;
	card->qcurr  = 0;
}


/*
 * put all cards into the summary
 */

void query_all(
	CARD		*card)		/* database and form */
{
	int		r, n;

	if (!alloc_query(card, 0))
		return;
	for (r=n=0; r < card->dbase->nrows; r++)
		if (SECT_OK(card->dbase, r))
			card->query[n++] = r;
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
	char		*string)	/* string to search for */
{
	int		i;		/* item counter */
	char		*mask;		/* for skipping unselected cards */
	char		search[1024];	/* lower-case search string */
	register char	*p, *q;		/* copy and comparison pointers */

	if (!alloc_query(card, &mask))
		return;
	for (p=string, q=search, i=0; *p && i < sizeof(search)-1; i++)
		*q++ = *p++ | 0x20;
	*q = 0;
	if (mode == SM_SEARCH && pref.incremental)
		mode = SM_NARROW;
	for (card->row=0; card->row < card->dbase->nrows; card->row++) {
		if (!SECT_OK(card->dbase, card->row))
			continue;
		switch(mode) {
		  case SM_INQUERY:
		  case SM_WIDEN_INQUERY:
			if (last_query >= 0) {
				int res = expr_matches_card(card,
					card->form->query[last_query].query);
				if(res < 0)
					break;
				else if(!res)
					continue;
			}
			if (mode == SM_INQUERY)
				break;
		  case SM_WIDEN:
			if (mask && mask[card->row]) {
				card->query[card->nquery++] = card->row;
				continue;
			}
		  case SM_NARROW:
			if (!mask || !mask[card->row])
				continue;
			break;
		}
		if (search_matches_card(card, search))
			card->query[card->nquery++] = card->row;
	}
	if (mask)
		free(mask);
}


/*
 * put all cards into the summary whose last-sorted-by column begins with
 * <letter>: 0..25=letter, 26=misc, 27=all. The search is case-insensitive.
 */

#define BLANK(c) ((c) && strchr("\t\n \"!#$%&'()*+,-./:;<=>?@[\\]^`{|}~", (c)))

void query_letter(
	CARD		*card,		/* database and form */
	int		letter)		/* 0=0..9, 1..26=a..z, 27=all */
{
	int		r;		/* row counter */
	char		*data;		/* database string to test */
	char		*mask;		/* for skipping unselected cards */

	if (letter == 27) {
		query_all(card);
		return;
	}
	if (!alloc_query(card, &mask))
		return;
	letter = letter == 26 ? 0 : letter+'A';
	if (col_sorted_by >= card->dbase->maxcolumns-1)
		col_sorted_by = 0;
	for (r=0; r < card->dbase->nrows; r++)
		if (SECT_OK(card->dbase, r) && (!mask || mask[r])) {
			data = dbase_get(card->dbase, r, col_sorted_by);
			while (data && BLANK(*data))
				data++;
			if ((!data || !*data || strchr("0123456789_", *data))
								&& !letter)
				card->query[card->nquery++] = r;
			else if (data && letter)
				while (*data) {
					if ((*data & ~0x20) == letter) {
						card->query[card->nquery++] = r;
						break;
					}
					if (!pref.allwords)
						break;
					while (*data && !BLANK(*data))
						data++;
					while (BLANK(*data))
						data++;
				}
		}
	if (mask)
		free(mask);
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
	char		*expr)		/* expression to apply to dbase */
{
	char		*mask;		/* for skipping unselected cards */
	int		match;		/* to detect/skip errors */

	if (!alloc_query(card, &mask))
		return;
	if (mode == SM_SEARCH && pref.incremental)
		mode = SM_NARROW;
	for (card->row=0; card->row < card->dbase->nrows; card->row++) {
		if (!SECT_OK(card->dbase, card->row))
			continue;
		switch(mode) {
		  case SM_INQUERY:
			if (last_query >= 0) {
				match = expr_matches_card(card,
					card->form->query[last_query].query);
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
		}
		match = expr_matches_card(card, expr);
		if (match < 0)
			break;
		if (match)
			card->query[card->nquery++] = card->row;
	}
	if (mask)
		free(mask);
}


/*
 * prepare for a query: destroy the previous query list, and allocate a new
 * one with enough entries even if all cards match the query. The query list
 * is a list of row numbers in the dbase. Return TRUE if everything went well.
 * If <mask> is nonzero and incremental searches are enabled, create a string
 * with one byte per card, which is 1 if that card was in the previous query.
 * This makes it easy for the search routines to skip cards that have not been
 * in the old query.
 */

static BOOL alloc_query(
	CARD		*card,		/* database and form */
	char		**mask)		/* where to store pointer to string */
{
	if (mask) {
		register int r;
		*mask = 0;
		if (card->query && (*mask = malloc(card->dbase->nrows))) {
			(void)memset(*mask, 0, card->dbase->nrows);
			for (r=0; r < card->nquery; r++)
				(*mask)[card->query[r]] = 1;
		}
	}
	query_none(card);
	if (!card->dbase->nrows)
		return(FALSE);
	if (!(card->query = (int*)malloc(card->dbase->nrows * sizeof(int*)))) {
		create_error_popup(toplevel, errno,
					"No memory for query result summary");
		return(FALSE);
	}
	*card->query = 0;
	return(TRUE);
}


/*
 * return TRUE if the given card contains the given search string. This is
 * used by eval_search above, and by the find-and-select function in the File
 * pulldown (also Ctrl-F and a search mode).
 */

static BOOL search_matches_card(
	CARD		*card,		/* database and form */
	char		*search)	/* lowercased string to search for */
{
	int		i;		/* item counter */
	ITEM		*item;		/* item to check */
	char		*data;		/* database string to test */
	register char	*p, *q;		/* copy and comparison pointers */

	for (i=0; i < card->form->nitems; i++) {
		item = card->form->items[i];
		if (!IN_DBASE(item->type) || !item->search)
			continue;
		if (!(data = dbase_get(card->dbase, card->row, item->column)))
			continue;
		for (p=data; *p; p++) {
			if ((*p | 0x20) != *search)
				continue;
			for (q=search; *q; q++, p++)
				if ((*p | 0x20) != *q)
					break;
			if (!*q)
				return(TRUE);
			p -= q - search;
		}
	}
	return(FALSE);
}


/*
 * return TRUE if the given card matches the given expression. This is also
 * used by eval_search above, and by the find-and-select function in the File
 * pulldown (also Ctrl-F and a search mode).
 * returns -1 if expr had an error; otherwise 0/1 (FALSE/TRUE)
 */

static int expr_matches_card(
	CARD		*card,		/* database and form */
	char		*expr)		/* expression to test */
{
	char		*val;		/* return from expression eval */

	if (!(val = evaluate(card, expr)))
		return(-1);
	while (*val == ' ' || *val == '\t') val++; \
	return(*val && *val != '0' && *val != 'f' && *val != 'F');
}
