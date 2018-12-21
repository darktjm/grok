#ifndef _GROK_H_
#define _GROK_H_
/*
 * common definitions for grok. The actual data structures the database is
 * stored in are in form.h, everything else is here.
 */

/*
 * global preferences
 */

class QPrinter;
struct pref {
	bool	modified;	/* prefs need saving */
	bool	ampm;		/* US time format if true, sane if false */
	bool	mmddyy;		/* US date format if true, European if false */
	bool	query2search;	/* copy query pulldn string to search string */
	bool	letters;	/* show letter search row below summary */
	bool	allwords;	/* letter search looks at all word beginnings*/
	bool	incremental;	/* searches and queries are incremental */
	bool	uniquedb;	/* only unique names in dbase pulldown */
	char	pselect;	/* print: C=curr, S=search, A=all */
	char	*xfile;		/* export file */
	int	xlistpos;	/* last chosen line in export list, 0=top */
	int	xflags;		/* last set of export flags */
	int	linelen;	/* truncate printer line after n chars (79) */
	double	scale;		/* scale factor for cards */
	int	sumlines;	/* number of summary lines */
	int	sortcol;	/* initially form->defsort, then sort pulldn */
	bool	revsort;	/* initially false, then line1 of sort pulldn*/
	bool	autoquery;	/* run form.autoquery after every change? */
	int	db_keep;	/* keep at most this many unrefed databases */
	QPrinter *printer;	/* printer settings */
};

#define COL_CHART_N	8		/* number of chart colors */

				/* pixmaps for labels and buttons */
#define PIC_LEFT	0		/* previous card */
#define PIC_RIGHT	1		/* next card */
#define NPICS		2
#endif
