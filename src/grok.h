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
	int	linelen;	/* truncate printer line after n chars (79) */
	double	scale;		/* scale factor for cards */
	int	sumlines;	/* number of summary lines */
	int	sortcol;	/* initially form->defsort, then sort pulldn */
	BOOL	revsort;	/* initially FALSE, then line1 of sort pulldn*/
	BOOL	autoquery;	/* run form.autoquery after every change? */
};


/*
 * X stuff
 */
				/* fonts */
#define FONT_STD	0		/* standard font: menus, text */
#define FONT_HELP	1		/* pretty font for help popups */
#define FONT_HELV	2		/* fonts available in forms */
#define FONT_HELV_O	3
#define FONT_HELV_S	4
#define FONT_HELV_L	5
#define FONT_COURIER	6
#define FONT_LABEL	7		/* chart axis labels */
#define NFONTS		8

				/* colors */
#define COL_BACK	0		/* standard background */
#define COL_SHEET	1		/* paper-like text scroll areas */
#define COL_STD		2		/* standard foreground */
#define COL_TEXTBACK	3		/* standard bkground of text widgets */
#define COL_TOGGLE	4		/* toggle button diamond color */
#define COL_CANVBACK	5		/* canvas background */
#define COL_CANVFRAME	6		/* canvas frame around boxes */
#define COL_CANVBOX	7		/* canvas box representing item */
#define COL_CANVSEL	8		/* canvas selected box */
#define COL_CANVTEXT	9		/* canvas text inside box */
#define COL_CHART_AXIS	10		/* chart axis color */
#define COL_CHART_GRID	11		/* chart grid color */
#define COL_CHART_BOX	12		/* border of each bar in the chart */
#define COL_CHART_0	13		/* eight colors for bars in chart */
#define COL_CHART_1	14
#define COL_CHART_2	15
#define COL_CHART_3	16
#define COL_CHART_4	17
#define COL_CHART_5	18
#define COL_CHART_6	19
#define COL_CHART_7	20
#define COL_CHART_N	8		/* number of chart colors */
#define NCOLS		21

				/* pixmaps for labels and buttons */
#define PIC_LEFT	0		/* previous card */
#define PIC_RIGHT	1		/* next card */
#define NPICS		2
