/*
 * add and remove rows (cards) from the database
 *
 *	dbase_create()			create a new empty DBASE struct
 *	dbase_delete(dbase)		delete contents of a DBASE struct
 *	dbase_addrow(rp,dbase)		add a row to a database
 *	dbase_delrow(r,dbase)		delete a row from a database
 *	dbase_get(dbase,r,c)		get a string from the database
 *	dbase_put(dbase,r,c,str)	put a string into the database
 *	dbase_sort(dbase,c,rev)		sort database by column c
 */

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <QtWidgets>
#include "config.h"
#include "grok.h"
#include "form.h"
#include "proto.h"

#define CHUNK	32			/* alloc 32 new rows at a time */


DBASE *dbase_list;


/*
 * allocate a new empty dbase struct with no data elements. Use dbase_addrow
 * to add a row (card), and use free/mystrdup to fill individual data items
 * in that row.
 */

DBASE *dbase_create(
	const FORM	*form)
{
	DBASE		*dbase;		/* new dbase */

	/* since even this code assumes success, errors are fatal */
	dbase = zalloc(0, "database", DBASE, 1);
	dbase->form = form;
	dbase->path = mystrdup(db_path(form));
	return(dbase);
}


/*
 * destroy a dbase struct and all its data.
 */

void dbase_clear(
	DBASE		*dbase)		/* dbase to delete */
{
	int		r, c;		/* data counter */
	ROW		*row;		/* row to delete */
	const FORM	*form = dbase->form;

	dbase->form = NULL;
	for(FORM *f = form_list; f; f = f->next)
		if(f == form) {
			form_delete(f);
			break;
		}
	for (r=0; r < dbase->nsects; r++)
		zfree(dbase->sect[r].path);
	zfree(dbase->sect);

	for (r=0; r < dbase->nrows; r++) {
		row = dbase->row[r];
		for (c=row->ncolumns-1; c >= 0; c--)
			zfree(row->data[c]);
		free(row);
	}
	zfree(dbase->row);
	zfree(dbase->path);
	for(r=0; r < 26; r++)
		zfree(dbase->var[r].string);
}


/* The public interface only deletes unreferenced databases */
/* It might even delete more than one, not necessarily passed-in one */
void dbase_delete(
	DBASE		*dbase)		/* dbase to delete */
{
	CARD		*card;
	DBASE		**prev;
	int		n;

	/* If the database has no path, it's not in the list, so delete */
	if(dbase && !dbase->path) {
		dbase_clear(dbase);
		free(dbase);
	}
	/* but prune databases anyway */
	for(n = 0, prev = &dbase_list; *prev; prev = &(*prev)->next)
		if(!(*prev)->modified) {
			for (card = card_list; card; card = card->next)
				if (card->dbase == *prev)
					break;
			if(!card && ++n > pref.db_keep) {
				DBASE *dbase = *prev;
				*prev = dbase->next;
				dbase_clear(dbase);
				free(dbase);
				if(!*prev)
					return;
			}
		}
}


/* Whenever a dbase is referenced, move it to the top of the list */
static void dbase_to_top(DBASE *dbase)
{
	DBASE **prev;
	if (dbase && dbase->path && dbase != dbase_list)
		for(prev = &dbase_list; *prev; prev = &(*prev)->next)
			if(*prev == dbase) {
				*prev = dbase->next;
				dbase->next = dbase_list;
				dbase_list = dbase;
				return;
			}
}


/*
 * append a new row (card) to the database, and return the number of the
 * appended row so the caller can store it in the card struct. Don't fill
 * in any data yet, just add null pointers. Return false if allocation
 * failed. The caller must redraw the current card. Always put new cards
 * into section dbase->currsect. Set the ctime (create time) and ctimex
 * (a disambiguating counter) that will be used as a lifetime identifier
 * for the new card, used by synchronization. (fixme: search is O(n^2))
 */

bool dbase_addrow(
	int		 *rowp,		/* ptr to returned row number */
	DBASE		 *dbase)	/* database to add row to */
{
	int		 i, n;		/* size of data ptr array in bytes */
	ROW		 *row;		/* new database row */
	SECTION		 *sect;			/* current insert section */
	int		 newsect = 0;

	dbase_to_top(dbase);
	newsect = dbase->nsects<2 || dbase->currsect<0 ? 0 : dbase->currsect;
	if (dbase->nrows+1 >= (int)dbase->size) {
		i = (dbase->size + CHUNK) * sizeof(ROW *);
		if (!(dbase->row = (ROW **)(dbase->row ? realloc(dbase->row, i)
						       : malloc(i))))
			return(false);
		dbase->size += CHUNK;
	}
	n = dbase->maxcolumns ? dbase->maxcolumns : 1;
	i = sizeof(ROW) + (n-1) * sizeof(char *);
	if (!(row = dbase->row[dbase->nrows] = (ROW *)malloc(i)))
		return(false);
	memset(row, 0, i);
	row->ncolumns   = n;
	row->section    = newsect;
	row->ctime	= time(0);
	row->ctimex = dbase->ctimex_next++;
	sect = &dbase->sect[newsect];
	dbase->modified = true;
	sect->modified  = true;
	sect->nrows++;
	*rowp = dbase->nrows++;
	print_info_line();
	return(true);
}


/*
 * delete a row (card) in the database. Don't update any menus. This cannot
 * fail. The caller must make sure to set card->row to a new value afterwards,
 * and redraw the current card.
 * <<< this messes up the query array in the card data structure.
 */

void dbase_delrow(
	int		 nrow,		/* row to delete */
	DBASE		 *dbase)	/* database to delete row in */
{
	int		 i;		/* copy counter */
	ROW		 *row;		/* new database row */

	if (!dbase || nrow >= dbase->nrows)
		return;
	dbase_to_top(dbase);
	row = dbase->row[nrow];
	dbase->sect[row->section].nrows--;
	dbase->sect[row->section].modified = true;
	dbase->nrows--;
	dbase->modified = true;

	for (i=0; i < row->ncolumns; i++)
		if (row->data[i])
			free(row->data[i]);
	free(row);
	for (i=nrow; i < dbase->nrows; i++)
		dbase->row[i] = dbase->row[i+1];
	print_info_line();
}


/*
 * retrieve a data string from the database, by row and column number. If
 * the string does not exist for any reason, return 0.
 */

char *dbase_get(
	const DBASE	*dbase,		/* database to get string from */
	int		nrow,		/* row to get */
	int		ncolumn)	/* column to get */
{
	ROW		*row;		/* row to get from */

	dbase_to_top((DBASE *)dbase);
	if (dbase && nrow >= 0
		  && nrow < dbase->nrows
		  && (row = dbase->row[nrow])
		  && ncolumn < row->ncolumns)
		return(row->data[ncolumn]);
	return(0);
}


/*
 * store a string in the database. Add columns if necessary, but don't
 * add rows at the end. It should never be necessary to add a row in
 * the middle, this must have been done with dbase_addrow earlier. Don't
 * write into read-only sections. Store empty strings as null pointers.
 * return false if the string is unchanged.
 *
 * Since there is no way to distinguish allocation errors from unchanged
 * database, what do I do on memory errors?
 *
 * Since there's no periodic autosave, dying seems harsh, but I don't
 * think trying to pop up an error message will work anyway, and the
 * user has no idea what data write failed, if it was done in an
 * expression or read_file().  Best to just be harsh.  FIXME: one day
 * fix this to actually return an error code on errors instead of dying.
 */

bool dbase_put(
	DBASE		*dbase,		/* database to put into */
	int		nrow,		/* row to put into */
	int		ncolumn,	/* column to put into */
	const char	*data,		/* string to store */
	bool		force)		/* true if dbase has been modified */
{
	ROW		*row;		/* row to put into */
	char		*p;

	if (!dbase || nrow < 0
		   || nrow >= dbase->nrows
		   || dbase->rdonly
		   || !(row = dbase->row[nrow])
		   || dbase->sect[row->section].rdonly)
		return(false);
	dbase_to_top(dbase);
	if (ncolumn >= dbase->maxcolumns)
		dbase->maxcolumns = ncolumn + 1;
	if (ncolumn >= row->ncolumns) {
		bzgrow(0, "update database", ROW, row, data, row->ncolumns,
		       dbase->maxcolumns, 0);
		dbase->row[nrow] = row;
		row->ncolumns = dbase->maxcolumns;
	}
	if (data && !*data)
		data = 0;
	p = row->data[ncolumn];
	if ((!data && !p) || (data && p && !strcmp(data, p))) {
		if (!force)
			return(false);
	} else {
		if (p)
			free(p);
		row->data[ncolumn] = mystrdup(data);
	}
	row->mtime = time(0);
	dbase->modified = dbase->sect[row->section].modified = true;
	return(true);
}


/*
 * sort database by a column. Leading blanks are stripped before the
 * comparison. If a string begins with a number, compare numerically.
 * If not, or if the numbers are equal, compare lexicographically,
 * case-insensitive. The caller must redraw the current card.
 *
 * Sorting is done on a copy of the real 2D data pointer array. The copy has
 * one extra column that is non-null if the row is in the query array. This
 * way, the cards selected of the last search or query can be re-selected
 * after the sort, despite them having changed their card numbers.
 */

static int compare_one(
	const ROW	*ru,
	const ROW	*rv,
	int		col,
	int		date_type)
{
	char		*cu;
	char		*cv;
	double		du, dv;

	cu = col < ru->ncolumns ? ru->data[col] : 0;
	cv = col < rv->ncolumns ? rv->data[col] : 0;
	if (!cu || !cv)
		return(!cv - !cu);
	if(date_type >= 0) {
	    du = parse_time_data(cu, (TIMEFMT)date_type);
	    dv = parse_time_data(cv, (TIMEFMT)date_type);
	    if (du != dv)
		return(du < dv ? -1 : du == dv ? 0 : 1);
	}
	while (*cu == ' ' || *cu == '\t') cu++;
	while (*cv == ' ' || *cv == '\t') cv++;
	if ((isdigit(*cu) || *cu == '.') &&
	    (isdigit(*cv) || *cv == '.')) {
		du = atof(cu);
		dv = atof(cv);
		if (du != dv)
			return(du < dv ? -1 : du == dv ? 0 : 1);
	}
	return(strcasecmp(cu, cv));
}


/* If there's one thing I hate most about C, it's the fact that the most
 * useful library function has no global user data pointer */
/* I guess this means I'm going to have to use C++, which at least passes
 * "this" to objects */
#include <algorithm>

struct db_comp {
	const ROW * const *rows;
	int col_sorted_by;
	int date_type;
	bool reverse;
	bool operator() (int u, int v)
	{
		int diff = compare_one(rows[u], rows[v], col_sorted_by, date_type);
		return(reverse ? diff > 0 : diff < 0);
	}
};


void dbase_sort(
	CARD		*card,		/* database and form to sort */
	int		col,		/* column to sort by */
	bool		rev,		/* reverse if nonzero */
	bool		noinit)		/* multi-field sort */
{
	FORM		*form;		/* card form */
	DBASE		*dbase;		/* card data */
	int		i, j;
	db_comp		cmp;
#define sort_it(a, n) std::stable_sort<int *>(&a[0], &a[n], cmp)

	if (!card)
		return;
	dbase = card->dbase;
	form  = card->form;
	if (!dbase || !form || dbase->nrows < 2)
		return;

	cmp.reverse = rev;
	card->col_sorted_by = cmp.col_sorted_by = col;
	/* <sigh> don't know which item to use */
	cmp.date_type = -1; 
	for (i = 0; i < form->nitems; i++)
		if (form->items[i]->column == col) {
			if (form->items[i]->type == IT_TIME)
				cmp.date_type = form->items[i]->timefmt;
			break;
		}
	cmp.rows = dbase->row;
	if (!noinit) {
		grow(0, "dbase sort order", int, card->sorted, dbase->nrows, 0);
		for (i = 0; i < dbase->nrows; i++)
			card->sorted[i] = i;
	}
	sort_it(card->sorted, dbase->nrows);
	if(card->nquery) {
		if(card->qcurr < card->nquery)
			i = card->query[card->qcurr];
		else
			i = -1;
		sort_it(card->query, card->nquery);
		if(i >= 0)
			for(j = 0; j < card->nquery; j++)
				if(card->query[j] == i) {
					card->qcurr = j;
					break;
				}
	}
}

int row_with_ctime(
	const DBASE	*dbase,		/* database to search */
	time_t		ctime,		/* ctime to find */
	long		ctimex)		/* ctime uniquifier */
{
	if (!dbase)
		return -1;
	for (int r = 0; r < dbase->nrows; r++)
		if (dbase->row[r]->ctime == ctime &&
		    dbase->row[r]->ctimex == ctimex)
			return r;
	return -1;
}

void elt_at(const char *array, unsigned int n, int *begin, int *after, char sep, char esc)
{
    *after = -1;
    do {
	next_aelt(array, begin, after, sep, esc);
	if(!n--)
	    return;
    } while(*after >= 0);
}

char *unescape(char *d, const char *s, int len, char esc)
{
    if(len < 0)
	len = strlen(s);
    while(len--) {
	if(*s == esc && len) {
	    s++;
	    len--;
	}
	*d++ = *s++;
    }
    return d;
}

char *unesc_elt_at(const FORM *form, const char *array, int n)
{
    char sep, esc;
    int b, a;
    get_form_arraysep(form, &sep, &esc);
    elt_at(array, n, &b, &a, sep, esc);
    if (b == a)
	return 0;
    char *ret = alloc(0, "aelt", char, a - b + 1);
    *unescape(ret, array + b, a - b, esc) = 0;
    return ret;
}

char **split_array(const FORM *form, const char *array, int *len)
{
    *len = 0;
    if(!array || !*array)
	return 0;
    char sep, esc;
    get_form_arraysep(form, &sep, &esc);
    int begin, after = -1;
    size_t rsz;
    char **ret = alloc(0, "split array", char *, rsz = 10);
    while(1) {
	next_aelt(array, &begin, &after, sep, esc);
	if(after < 0)
	    break;
	zgrow(0, "split array", char *, ret, *len, *len + 1, &rsz);
	if(after == begin)
	    ret[(*len)++] = 0;
	else {
	    ret[*len] = alloc(0, "split array", char, after - begin + 1);
	    *unescape(ret[(*len)++], array + begin, after - begin, esc) = 0;
	}
    }
    return ret;
}

void get_form_arraysep(const FORM *form, char *sep, char *esc)
{
    *sep = '|';
    *esc = '\\';
    if(!form)
	return;
    if(form->asep)
	*sep = form->asep;
    if(form->aesc)
	*esc = form->aesc;
}

/* Starts search at after + 1;  Returns -1 for begin & after @ end */
void next_aelt(const char *array, int *begin, int *after, char sep, char esc)
{
    if(!array || !*array) {
	if(*after == -1)
	    *begin = *after = 0;
	else
	    *begin = *after = -1;
	return;
    }
    if(*after >= 0 && !array[*after]) {
	*begin = *after = -1;
	return;
    }
    *begin = ++*after;
    while(array[*after] && array[*after] != sep) {
	if(array[*after] == esc && array[*after + 1])
	    ++*after;
	++*after;
    }
}

int stralen(const char *array, char sep, char esc)
{
    int ret = 0, b, a = -1;
    do {
	next_aelt(array, &b, &a, sep, esc);
	++ret;
    } while(a >= 0);
    return ret - 1;
}

int countchars(const char *s, const char *c)
{
    int len = 0;
    if(!s)
	return 0;
    while(*s)
	if(strchr(c, *s++))
	    len++;
    return len;
}

char *escape(char *d, const char *s, int len, char esc, const char *toesc)
{
    if(len < 0)
	len = strlen(s);
    while(len--) {
	if(*s == esc || strchr(toesc, *s))
	    *d++ = esc;
	*d++ = *s++;
    }
    return d;
}

// modifies array in-place; assumes array has been malloc'd
bool set_elt(char **array, int n, const char *val, const FORM *form)
{
    int b, a, vlen = val ? strlen(val) : 0, alen = *array ? strlen(*array) : 0;
    char toesc[3];
    char &sep = toesc[1], &esc = toesc[0];
    int vesc;
    get_form_arraysep(form, &sep, &esc);
    toesc[2] = 0;
    if(n < 0)
	n = stralen(*array, sep, esc);
    vesc = countchars(val, toesc);
    elt_at(*array, n, &b, &a, sep, esc);
    if(a >= 0) { // replace
	if(vlen + vesc == a - b) // exact fit
	    escape(*array + b, val, vlen, esc, toesc + 1);
	else if(vlen + vesc < a - b) { // smaller
	    memmove(*array + b + vlen + vesc, *array + a, alen - a + 1);
	    escape(*array + b, val, vlen, esc, toesc + 1);
	} else if(!alen && !vesc) { // bigger, but completely replace array
	    zfree(*array);
	    *array = zstrdup(val);
	    return true;
	} else { // bigger
	    char *oarray = *array;
	    int nlen = alen + vlen + vesc - (a - b) + 1;
	    /* C supports NULL realloc, but some memory debuggers don't */
	    if(*array)
		*array = (char *)realloc(*array, nlen);
	    else
		*array = (char *)malloc(nlen);
	    if(!*array) {
		zfree(oarray);
		return false;
	    }
	    memmove(*array + b + vlen + vesc, *array + a, alen - a);
	    (*array)[nlen - 1] = 0;
	    escape(*array + b, val, vlen, esc, toesc + 1);
	}
	alen += vlen + vesc - (a - b);
    } else { // append
	int l = stralen(*array, sep, esc);
	char *oarray = *array;
	int nlen = alen + (n - l + 1) + vlen + vesc + 1;
	/* C supports NULL realloc, but some memory debuggers don't */
	if(*array)
	    *array = (char *)realloc(*array, nlen);
	else
	    *array = (char *)malloc(nlen);
	if(!*array) {
	    zfree(oarray);
	    return false;
	}
	memset(*array + alen, sep, n - l + 1);
	escape(*array + alen + n - l + 1, val, vlen, esc, toesc + 1);
	(*array)[nlen - 1] = 0;
    }
    return array;
}

/* array should be a global, but C's qsort doesn't support globals */
/* I'm tring to eliminate statics, so I won't pass it that way */
/* another fix would be to use std::sort, but this is OK, too. */
struct aelt_loc {
    const char *array;
    int b, a;
};

static int cmp_aelt(const void *_a, const void *_b)
{
    const struct aelt_loc *a = (struct aelt_loc *)_a;
    const struct aelt_loc *b = (struct aelt_loc *)_b;
    int alen = a->a - a->b;
    int blen = b->a - b->b;
    int c;
    if(!alen && !blen)
	return 0;
    c = memcmp(a->array + a->b, b->array + b->b, alen > blen ? blen : alen);
    if(c || alen == blen)
	return c;
    if(alen > blen)
	return 1;
    else
	return -1;
}

/* This scans the string twice, and allocates a sort array every run */
/* A possibly better way would be to keep a static growable array and
 * build it during the first scan, avoiding the need for a second scan */
bool toset(char *a, char sep, char esc)
{
    if(!a || !*a)
	return true;
    int alen = stralen(a, sep, esc);
    if(alen == 1)
	return true;
    struct aelt_loc *elts = (struct aelt_loc *)malloc(alen * sizeof(*elts)), *e;
    int begin, after = -1, i;
    if(!elts)
	return false;
    for(e = elts, i = 0; i < alen; e++, i++) {
	next_aelt(a, &begin, &after, sep, esc);
	e->array = a;
	e->b = begin;
	e->a = after;
    }
    qsort(elts, alen, sizeof(*elts), cmp_aelt);
    // ah, screw it.  Just make it anew, always
    // doing it in-place would just be too much processing for little benefit
    char *set = (char *)malloc(strlen(a) + 1), *p = set;
    if(!set) {
	free(elts);
	return false;
    }
    for(e = elts, i = 0; i < alen; e++, i++) {
	// remove blanks
	if(e->a == e->b)
	    continue;
	// remove dups
	if(i && e->a - e->b == e[-1].a - e[-1].b &&
	   !memcmp(a + e->b, a + e[-1].b, e->a - e->b))
	    continue;
	memcpy(p, a + e->b, e->a - e->b);
	p += e->a - e->b;
	*p++ = sep;
    }
    if(p == set)
	*p++ = 0;
    else
	p[-1] = 0;
    memcpy(a, set, p - set);
    free(elts);
    free(set);
    return true;
}

/* find element containing array[n] */
/* if n is on a separator, find element before separator */
static void elt_at_off(const char *array, int o, int *begin, int *after, char sep, char esc)
{
    int b = o, a = o;
    while(1) { // find start
	// Find prev separator
	while(b > 0 && array[b - 1] != sep)
	    --b;
	// Is it escaped?  If not, we're done
	if(!b || b == 1 || array[b - 2] != esc)
	    break;
	// If so, count escapes
	o = b - 2;
	while(o > 0 && array[o - 1] == esc)
	    --o;
	// Even #?  then it's not really escaped, so done
	if((b - o) % 2)
	    break;
	// Otherwise, continue search
	b = o;
    }
    // find end
    for(; array[a] && array[a] != sep; a++)
	if(array[a] == esc && array[a + 1])
	    ++a;
    *begin = b;
    *after = a;
}

bool find_unesc_elt(const char *a, const char *s, int *begin, int *after,
		    char sep, char esc)
{
    bool ret;
    char toesc[3];
    int len = strlen(s);
    int nesc;
    char *tmp = NULL;

    toesc[0] = esc;
    toesc[1] = sep;
    toesc[2] = 0;
    nesc = countchars(s, toesc);
    if(nesc) {
	s = tmp = alloc(0, "escaping", char, len + nesc + 1);
	*escape(tmp, s, len, sep, toesc) = 0;
	len += nesc;
    }
    ret = find_elt(a, s, len, begin, after, sep, esc);
    zfree(tmp);
    return ret;
}

bool find_elt(const char *a, const char *s, int len, int *begin, int *after,
	      char sep, char esc)
{
    int alen = strlen(a);
    int l = 0, h = alen - 1, m;
    int mb, ma;

    while(l <= h) {
	m = (l + h) / 2;
	elt_at_off(a, m, &mb, &ma, sep, esc);
	int c = memcmp(s, a + mb, len > ma - mb ? ma - mb : len);
	if(c > 0)
	    l = ma + 1;
	else if(c < 0)
	    h = mb - 2;
	else if(len > ma - mb)
	    l = ma + 1;
	else if(len < ma - mb)
	    h = mb - 2;
	else {
	    *begin = mb;
	    *after = ma;
	    return true;
	}
    }
    *begin = l > alen ? alen : l;
    return false;
}

int keylen_of(const ITEM *item)
{
	int n, keylen = 0;
	for (n = 0; n < item->nfkey; n++)
		if (item->fkey[n].key)
			++keylen;
	return keylen;
}

int copy_fkey(const ITEM *item, int *keys)
{
	int keylen;
	int ret = 0;
	for (int n = keylen = 0; n < item->nfkey; n++)
		if (item->fkey[n].key) {
			keys[keylen++] = n;
			if(!item->fkey[n].item)
				ret = -1;
		}
	return ret;
}

int fkey_lookup( /* ret -2 for oob, -1 for not found */
	const DBASE	*dbase,		/* database to search */
	const FORM	*form,		/* fkey origin */
	const ITEM	*item,		/* fkey definition */
	const char	*val,		/* reference value */
	int		keyno,		/* multi array elt (<0 = already extracted) */
	int		start)		/* row to start looking */
{
	/* expects resolve_fkey_fields(item); to have been called first */
	const FORM *fform = item->fkey_form;
	if(!fform)
		return -2;

	/* no blank keys */
	if (!val || !*val)
		return -1;
	char *vbuf = 0;

	if (IFL(item->,FKEY_MULTI) && keyno >= 0) {
		val = vbuf = unesc_elt_at(form, val, keyno);
		/* no blank keys */
		if (!vbuf)
			return -2;
	} else if(keyno > 0)
		return -2;
	int n;
	QString keyf;
	/* each key may be an array */
	int keylen = keylen_of(item);
	/* ??? invalid form */
	if (!keylen) {
		zfree(vbuf);
		return -1;
	}
	int keys[keylen];
	copy_fkey(item, keys);
	char **vals;
	if(keylen == 1)
		vals = (char **)&val;
	else {
		int vlen;
		vals = split_array(form, val, &vlen);
		if(vlen != keylen) {
			/* invalid data */
			for(n = 0; n < vlen; n++)
				zfree(vals[n]);
			zfree(vals);
			zfree(vbuf);
			return -1;
		}
	}
	int r;
	for (r = start; r < dbase->nrows; r++) {
		for (n = 0; n < keylen; n++) {
			const FKEY &k = item->fkey[keys[n]];
			if(!k.item)
				return -2;
			const ITEM *i = k.item;
			const MENU *m = k.menu;
			int col = m ? m->column : i->column;
			const char *fval = dbase_get(dbase, r, col);
			if (strcmp(STR(fval), vals[n]))
				break;
		}
		if (n == keylen)
			break;
	}
	if (keylen > 1) {
		for(n = 0; n < keylen; n++)
			zfree(vals[n]);
		zfree(vals);
	}
	zfree(vbuf);
	return r == dbase->nrows ? -1 : r;
}

char *fkey_of(
	const DBASE	*dbase,		/* database to search */
	int		row,		/* row */
	const FORM	*form,		/* fkey origin */
	const ITEM	*item)		/* fkey definition */
{
	int n, keylen = keylen_of(item);
	/* ??? invalid form */
	if (!keylen)
		return 0;
	int keys[keylen];
	copy_fkey(item, keys);
	char *ret = 0;
	for (n = 0; n < keylen; n++) {
		const FKEY *k = &item->fkey[keys[n]];
		const ITEM *i = k->item;
		const MENU *m = k->menu;
		int col = m ? m->column : i->column;
		const char *fval = dbase_get(dbase, row, col);
		if (keylen == 1)
			return zstrdup(fval);
		set_elt(&ret, n, fval, form);
	}
	return ret;
}

/* check form/db for invalid foreign key defs or data */
/* fills in badrefs/nbadref with problem descriptors */
/* inv/invdb are for recursion, to check that foreign db's refs to this db
 * are valid as well */
void check_db_references(FORM *form, DBASE *db, badref **badrefs,
			 int *nbadref, const FORM *inv, DBASE *invdb)
{
	int i, j, r, k;

	for(i = 0; i < form->nitems; i++) {
		ITEM *item = form->items[i];
		FORM *fform = NULL;
		if(item->type == IT_FKEY ||
		   (!inv && item->type == IT_INV_FKEY)) {
			resolve_fkey_fields(item);
			fform = item->fkey_form;
			if(inv && fform != inv)
				continue;
			/* by now, form and fields should be resolved */
			if(!fform) {
				badref br;
				br.form = form;
				br.item = i;
				br.fform = fform;
				br.dbase = db;
				br.fdbase = 0;
				br.row = br.keyno = -1;
				br.reason = BR_NO_FORM;
				grow(0, "bad refs", badref,
				     *badrefs, *nbadref + 1, 0);
				(*badrefs)[(*nbadref)++] = br;
				continue; /* fatal error */
			}
			for(j = 0; j < item->nfkey; j++)
				if(!item->fkey[j].item)
					break;
			if(j < item->nfkey) {
				badref br;
				br.form = form;
				br.item = i;
				br.fform = fform;
				br.dbase = db;
				br.fdbase = 0;
				br.row = -1;
				br.keyno = j;
				br.reason = BR_NO_REFITEM;
				grow(0, "bad refs", badref,
				     *badrefs, *nbadref + 1, 0);
				(*badrefs)[(*nbadref)++] = br;
				continue; /* fatal error */
			}
		}
		if(item->type == IT_FKEY) {
			DBASE *fdb = invdb ? invdb : read_dbase(fform);
			if(!inv) {
				/* foreign form should declare this as referrer */
				/* either directly ... */
				for(j = 0; j < fform->nchild; j++)
					if(!strcmp(fform->children[j], form->name))
						break;
				if(j == fform->nchild) {
					/* or by an INV_FKEY */
					for(j = 0; j < fform->nitems; j++) {
						ITEM *fitem = fform->items[j];
						if(fitem->type != IT_INV_FKEY)
							continue;
						resolve_fkey_fields(fitem);
						if(fitem->fkey_form &&
						   !strcmp(fitem->fkey_form->name, form->name))
							break;
					}
					if(j == fform->nitems) {
						badref br;
						br.form = form;
						br.item = i;
						br.fform = fform;
						br.dbase = db;
						br.fdbase = fdb;
						br.row = br.keyno = -1;
						br.reason = BR_NO_INVREF;
						grow(0, "bad refs", badref,
						     *badrefs, *nbadref + 1, 0);
						(*badrefs)[(*nbadref)++] = br;
					}
				}
			}
			for(r = 0; r < db->nrows; r++) {
				const char *data = dbase_get(db, r, item->column);
				if(BLANK(data))
					continue;
				/* multi-key values must be a set */
				char sep, esc;
				get_form_arraysep(form, &sep, &esc);
				if(IFL(item->,FKEY_MULTI)) {
					char *sv = strdup(data);
					if(!sv || !toset(sv, sep, esc)) {
						perror("fkey check");
						exit(1);
					}
					if(strcmp(sv, data)) {
						badref br;
						br.form = form;
						br.item = i;
						br.fform = fform;
						br.dbase = db;
						br.fdbase = fdb;
						br.row = r;
						br.keyno = -1;
						br.reason = BR_BADKEYS;
						grow(0, "bad refs", badref,
						     *badrefs, *nbadref + 1, 0);
						(*badrefs)[(*nbadref)++] = br;
						/* fix right now */
						dbase_put(db, r, item->column, sv);
						data = dbase_get(db, r, item->column);
					}
					free(sv);
				}
				for(k = 0; ; k++) {
					/* all values should be in fdb */
					/* this implicitly checks array len
					 * for multi-field keys, so no
					 * separate error for that */
					int fr = fkey_lookup(fdb, form, item, data, k);
					if(fr == -2)
						break;
					if(fr == -1 ||
					   /* and only once */
					   fkey_lookup(fdb, form, item, data, k, fr + 1) >= 0) {
						badref br;
						br.form = form;
						br.item = i;
						br.fform = fform;
						br.dbase = db;
						br.fdbase = fdb;
						br.row = r;
						br.keyno = k;
						br.reason = fr < 0 ? BR_MISSING : BR_DUP;
						grow(0, "bad refs", badref,
						     *badrefs, *nbadref + 1, 0);
						(*badrefs)[(*nbadref)++] = br;
					}
				}
			}
		} else if(!inv && item->type == IT_INV_FKEY)
			/* verify_form ensures foreign key field is FKEY */
			/* otherwise, I'd have to check for BR_NO_FREF here */
			check_db_references(fform, read_dbase(fform), badrefs,
					    nbadref, form, db);
	}
	if(inv)
		return;
	for(i = 0; i < form->nchild; i++) {
		/* foreign form should resolve by now */
		FORM *fform = read_form(form->children[i], false, 0);
		if(!fform) {
			badref br;
			br.form = form;
			br.item = i;
			br.fform = fform;
			br.dbase = 0;
			br.fdbase = 0;
			br.row = br.keyno = -1;
			br.reason = BR_NO_CFORM;
			grow(0, "bad refs", badref,
			     *badrefs, *nbadref + 1, 0);
			(*badrefs)[(*nbadref)++] = br;
			continue;
		}
		/* explicit referrer should actually refer to this */
		for(j = 0; j < fform->nitems; j++) {
			ITEM *item = form->items[j];
			if(item->type == IT_FKEY) {
				resolve_fkey_fields(item);
				if(item->fkey_form == form)
					break;
			}
		}
		if(j >= fform->nitems) {
			badref br;
			br.form = form;
			br.item = i;
			br.fform = fform;
			br.dbase = 0;
			br.fdbase = 0;
			br.row = br.keyno = -1;
			br.reason = BR_NO_FREF;
			grow(0, "bad refs", badref,
			     *badrefs, *nbadref + 1, 0);
			(*badrefs)[(*nbadref)++] = br;
		} else
			check_db_references(fform, read_dbase(fform), badrefs,
					    nbadref, form, db);
	}
}
