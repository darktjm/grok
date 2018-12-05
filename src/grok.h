#ifndef _GROK_H_
#define _GROK_H_
/*
 * common definitions for grok. The actual data structures the database is
 * stored in are in form.h, everything else is here.
 */

#define FALSE	0
#define TRUE	1
#define BOOL	int


/*
 * global preferences
 */

struct pref {
	BOOL	ampm;		/* US time format if TRUE, sane if FALSE */
	BOOL	mmddyy;		/* US date format if TRUE, European if FALSE */
	BOOL	query2search;	/* copy query pulldn string to search string */
	BOOL	letters;	/* show letter search row below summary */
	BOOL	allwords;	/* letter search looks at all word beginnings*/
	BOOL	incremental;	/* searches and queries are incremental */
	BOOL	uniquedb;	/* only unique names in dbase pulldown */
	char	pselect;	/* print: C=curr, S=search, A=all */
	char	pformat;	/* print: S=summary, N=summary+note, C=cards */
	char	pquality;	/* print: A=ascii, O=overstrike, P=PostScript*/
	char	pdevice;	/* print: P=printer, F=file, W=window */
	char	*pspooler_a;	/* print spool command, ascii */
	char	*pspooler_p;	/* print spool command, PostScript */
	char	*pfile;		/* print file (if pdevice==F) */
	char	*xfile;		/* export file */
	int	xlistpos;	/* last chosen line in export list, 0=top */
	int	xflags;		/* last set of export flags */
	int	linelen;	/* truncate printer line after n chars (79) */
	double	scale;		/* scale factor for cards */
	int	sumlines;	/* number of summary lines */
	int	sortcol;	/* initially form->defsort, then sort pulldn */
	BOOL	revsort;	/* initially FALSE, then line1 of sort pulldn*/
	BOOL	autoquery;	/* run form.autoquery after every change? */
};

#define COL_CHART_N	8		/* number of chart colors */

				/* pixmaps for labels and buttons */
#define PIC_LEFT	0		/* previous card */
#define PIC_RIGHT	1		/* next card */
#define NPICS		2
#endif
