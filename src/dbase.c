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



/*
 * allocate a new empty dbase struct with no data elements. Use dbase_addrow
 * to add a row (card), and use free/mystrdup to fill individual data items
 * in that row.
 */

DBASE *dbase_create(void)
{
	DBASE		*dbase;		/* new dbase */

	/* since even this code assumes success, errors are fatal */
	dbase = zalloc(0, "database", DBASE, 1);
	/* this used to pointlessly create a section, but it was
	 * always followed by a call to dbase_delete(), which freed it */
	return(dbase);
}


/*
 * destroy a dbase struct and all its data.
 */

void dbase_delete(
	DBASE		*dbase)		/* dbase to delete */
{
	int		r, c;		/* data counter */
	ROW		*row;		/* row to delete */

	if (!dbase)
		return;
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
	for(r=0; r < 26; r++)
		zfree(dbase->var[r].string);
	free(dbase);
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
	int		 i, n, r;	/* size of data ptr array in bytes */
	ROW		 *row;		/* new database row */
	SECTION		 *sect;			/* current insert section */
	int		 newsect = 0;

	newsect = dbase->nsects<2 || dbase->currsect<0 ? 0 : dbase->currsect;
	if (dbase->nrows+1 >= (int)dbase->size) {
		i = (dbase->size + CHUNK) * sizeof(ROW);
		if (!(dbase->row = (ROW **)(dbase->row ? realloc(dbase->row, i)
						       : malloc(i))))
			return(false);
		dbase->size += CHUNK;
	}
	n = dbase->maxcolumns ? dbase->maxcolumns : 1;
	i = sizeof(ROW) + (n-1) * sizeof(char *);
	if (i == 0) i = 1;
	if (!(row = dbase->row[dbase->nrows] = (ROW *)malloc(i)))
		return(false);
	memset(row, 0, i);
	row->ncolumns   = n;
	row->section    = newsect;
	row->ctime	= time(0);
	for (r=0; r < dbase->nrows; r++) {
		ROW *other = dbase->row[r];
		if (row->ctime == other->ctime && row->ctimex <= other->ctimex)
			row->ctimex = other->ctimex + 1;
	}
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
	const char	*data)		/* string to store */
{
	ROW		*row;		/* row to put into */
	char		*p;

	if (!dbase || nrow < 0
		   || nrow >= dbase->nrows
		   || dbase->rdonly
		   || !(row = dbase->row[nrow])
		   || dbase->sect[row->section].rdonly)
		return(false);
	if (ncolumn >= dbase->maxcolumns)
		dbase->maxcolumns = ncolumn + 1;
	if (ncolumn >= row->ncolumns) {
		/* old code used maxolumns-1.  Why did that not die? */
		/* I'm guessing this code never gets called */
		zgrow(0, "update database", ROW, row, row->ncolumns,
		      dbase->maxcolumns, 0);
		dbase->row[nrow] = row;
		row->ncolumns = dbase->maxcolumns;
	}
	if (data && !*data)
		data = 0;
	p = row->data[ncolumn];
	if ((!data && !p) || (data && p && !strcmp(data, row->data[ncolumn])))
	    	return(false);
	if (row->data[ncolumn])
		free(row->data[ncolumn]);
	row->data[ncolumn] = mystrdup(data);
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
	const void	*u,
	const void	*v,
	int		col)
{
	ROW		*ru = *(ROW **)u;
	ROW		*rv = *(ROW **)v;
	char		*cu;
	char		*cv;
	double		du, dv;

	cu = col < ru->ncolumns ? ru->data[col] : 0;
	cv = col < rv->ncolumns ? rv->data[col] : 0;
	if (!cu || !cv)
		return(!cv - !cu);
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


static int col_sorted_by;
static int reverse;
static int compare(
	const void		*u,
	const void		*v)
{
	int diff = compare_one(u, v, col_sorted_by);
	if (!diff)
		diff = (*(ROW **)v)->seq - (*(ROW **)u)->seq;
	return(reverse ? -diff : diff);
}


void dbase_sort(
	CARD		*card,		/* database and form to sort */
	int		col,		/* column to sort by */
	int		rev)		/* reverse if nonzero */
{
	FORM		*form;		/* card form */
	DBASE		*dbase;		/* card data */
	ROW		**row;		/* list of row struct pointers */
	int		i, j;

	if (!card)
		return;
	reverse = rev;
	dbase = card->dbase;
	form  = card->form;
	if (!dbase || !form || dbase->nrows < 2)
		return;

	card->dbase->col_sorted_by = col_sorted_by = col;
	for (row=dbase->row, i=dbase->nrows; i; i--, row++) {
		(*row)->selected = 0;
		(*row)->seq      = i;
	}
	for (i=card->nquery-1; i >= 0; i--)
		dbase->row[card->query[i]]->selected = 1;
	if (card->row < dbase->nrows)
		dbase->row[card->row]->selected |= 4;
	if (card->nquery)
		dbase->row[card->query[card->qcurr]]->selected |= 2;

	qsort(card->dbase->row, dbase->nrows, sizeof(ROW *), compare);

	card->qcurr = 0;
	for (row=dbase->row, i=j=0; i < dbase->nrows; i++, row++) {
		if ((*row)->selected & 2)
			card->qcurr = j;
		if ((*row)->selected & 1)
			card->query[j++] = i;
		if ((*row)->selected & 4)
			card->row = i;
	}
	assert(j == card->nquery);
}
