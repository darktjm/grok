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

#include <X11/Xos.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <Xm/Xm.h>
#include "config.h"

#if defined(GROK) || defined(PLANGROK)

#include "grok.h"
#include "form.h"
#include "proto.h"

#define CHUNK	32			/* alloc 32 new rows at a time */

int		col_sorted_by;		/* dbase is sorted by this column */



/*
 * allocate a new empty dbase struct with no data elements. Use dbase_addrow
 * to add a row (card), and use free/mystrdup to fill individual data items
 * in that row.
 */

DBASE *dbase_create(void)
{
	DBASE		*dbase;		/* new dbase */

	dbase = (DBASE *)malloc(sizeof(DBASE));
	mybzero((void *)dbase, sizeof(DBASE));
	if (!(dbase->sect = malloc(sizeof(SECTION)))) {
		fprintf(stderr,
			"grok: no memory for section, cannot continue.");
		exit(1);
	}
	mybzero(dbase->sect, sizeof(SECTION));
	dbase->nsects	= 1;
	dbase->currsect	= -1;
	return(dbase);
}


/*
 * destroy a dbase struct and all its data. The pointer passed is not freed.
 */

void dbase_delete(
	DBASE		*dbase)		/* dbase to delete */
{
	register int	r, c;		/* data counter */
	register ROW	*row;		/* row to delete */

	if (!dbase)
		return;
	for (r=0; r < dbase->nsects; r++)
		if (dbase->sect[r].path)
			free(dbase->sect[r].path);
	if (dbase->sect)
		free(dbase->sect);

	for (r=0; r < dbase->nrows; r++) {
		row = dbase->row[r];
		for (c=row->ncolumns-1; c >= 0; c--)
			if (row->data[c])
				free(row->data[c]);
	}
	if (dbase->row)
		free(dbase->row);
	mybzero((void *)dbase, sizeof(DBASE));
}


/*
 * append a new row (card) to the database, and return the number of the
 * appended row so the caller can store it in the card struct. Don't fill
 * in any data yet, just add null pointers. Return FALSE if allocation
 * failed. The caller must redraw the current card. Always put new cards
 * into section dbase->currsect. Set the ctime (create time) and ctimex
 * (a disambiguating counter) that will be used as a lifetime identifier
 * for the new card, used by synchronization. (fixme: search is O(n^2))
 */

BOOL dbase_addrow(
	int		 *rowp,		/* ptr to returned row number */
	register DBASE	 *dbase)	/* database to add row to */
{
	register int	 i, n, r;	/* size of data ptr array in bytes */
	register ROW	 *row;		/* new database row */
	register SECTION *sect;		/* current insert section */
	register int	 newsect = 0;

	newsect = dbase->nsects<2 || dbase->currsect<0 ? 0 : dbase->currsect;
	if (dbase->nrows+1 >= dbase->size) {
		i = (dbase->size + CHUNK) * sizeof(ROW);
		if (!(dbase->row = (ROW **)(dbase->row ? realloc(dbase->row, i)
						       : malloc(i))))
			return(FALSE);
		dbase->size += CHUNK;
	}
	n = dbase->maxcolumns ? dbase->maxcolumns : 1;
	i = sizeof(ROW) + (n-1) * sizeof(char *);
	if (i == 0) i = 1;
	if (!(row = dbase->row[dbase->nrows] = malloc(i)))
		return(FALSE);
	mybzero(row, i);
	row->ncolumns   = n;
	row->section    = newsect;
	row->ctime	= time(0);
	for (r=0; r < dbase->nrows; r++) {
		ROW *other = dbase->row[r];
		if (row->ctime == other->ctime && row->ctimex <= other->ctimex)
			row->ctimex = other->ctimex + 1;
	}
	sect = &dbase->sect[newsect];
	dbase->modified = TRUE;
	sect->modified  = TRUE;
	sect->nrows++;
	*rowp = dbase->nrows++;
	print_info_line();
	return(TRUE);
}


/*
 * delete a row (card) in the database. Don't update any menus. This cannot
 * fail. The caller must make sure to set card->row to a new value afterwards,
 * and redraw the current card.
 * <<< this messes up the query array in the card data structure.
 */

void dbase_delrow(
	int		 nrow,		/* row to delete */
	register DBASE	 *dbase)	/* database to delete row in */
{
	register int	 i;		/* copy counter */
	register ROW	 *row;		/* new database row */

	if (!dbase || nrow >= dbase->nrows)
		return;
	row = dbase->row[nrow];
	dbase->sect[row->section].nrows--;
	dbase->sect[row->section].modified = TRUE;
	dbase->nrows--;
	dbase->modified = TRUE;

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
	register DBASE	*dbase,		/* database to get string from */
	register int	nrow,		/* row to get */
	register int	ncolumn)	/* column to get */
{
	register ROW	*row;		/* row to get from */

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
 * return FALSE if the string is unchanged.
 */

BOOL dbase_put(
	register DBASE	*dbase,		/* database to put into */
	register int	nrow,		/* row to put into */
	register int	ncolumn,	/* column to put into */
	char		*data)		/* string to store */
{
	register ROW	*row;		/* row to put into */
	register char	*p;

	if (!dbase || nrow < 0
		   || nrow >= dbase->nrows
		   || dbase->rdonly
		   || !(row = dbase->row[nrow])
		   || dbase->sect[row->section].rdonly)
		return(FALSE);
	if (ncolumn >= dbase->maxcolumns)
		dbase->maxcolumns = ncolumn + 1;
	if (ncolumn >= row->ncolumns) {
		if (!(row = realloc(row, sizeof(ROW) +
				(dbase->maxcolumns-1) * sizeof(char *))))
			return(FALSE);
		mybzero(&row->data[row->ncolumns], sizeof(char *) *
					(dbase->maxcolumns - row->ncolumns));
		dbase->row[nrow] = row;
		row->ncolumns = dbase->maxcolumns;
	}
	if (data && !*data)
		data = 0;
	p = row->data[ncolumn];
	if (!data && !p || data && p && !strcmp(data, row->data[ncolumn]))
	    	return(FALSE);
	if (row->data[ncolumn])
		free(row->data[ncolumn]);
	row->data[ncolumn] = mystrdup(data);
	row->mtime = time(0);
	dbase->modified = dbase->sect[row->section].modified = TRUE;
	return(TRUE);
}


#ifdef GROK
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
	MYCONST void	*u,
	MYCONST void	*v,
	register int	col)
{
	register ROW	*ru = *(ROW **)u;
	register ROW	*rv = *(ROW **)v;
	register char	*cu;
	register char	*cv;
	register double	du, dv;

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
	return(mystrcasecmp(cu, cv));
}


static int reverse;
static int compare(
	register MYCONST void	*u,
	register MYCONST void	*v)
{
	register int diff = compare_one(u, v, col_sorted_by);
	if (!diff)
		diff = (*(ROW **)v)->seq - (*(ROW **)u)->seq;
	return(reverse ? -diff : diff);
}


void dbase_sort(
	CARD		*card,		/* database and form to sort */
	int		col,		/* column to sort by */
	int		rev)		/* reverse if nonzero */
{
	register FORM	*form;		/* card form */
	register DBASE	*dbase;		/* card data */
	register ROW	**row;		/* list of row struct pointers */
	register int	i, j;

	if (!card)
		return;
	reverse = rev;
	dbase = card->dbase;
	form  = card->form;
	if (!dbase || !form || dbase->nrows < 2)
		return;

	col_sorted_by = col;
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

#endif /* GROK */
#endif /* GROK || PLANGROK */
