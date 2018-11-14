#ifndef _FORM_H_
#define _FORM_H_
/*
 * form and database definitions
 */

/*
 * the CARD data structure describes a card window on the screen. A card can
 * be installed in any form widget. A card has two main parts: a form that
 * describes the data types and visual layout, and a database that contains
 * raw data as a matrix of strings. A card is the primary data structure of
 * grok. It is not an individual card, but all cards plus an index which one
 * is current. In addition to all that, there is some info about the widgets
 * that were used to display the card and its items. The form and the dbase
 * are the secondary data structures contained in the card. This split
 * between form and database allows multiple representations of the same
 * data, and also allows procedural databases that are not tied to a form.
 *
 * Note that if card->nquery > 0, query[card->qcurr] == card->row must be
 * true; ie. the line highlighted in the summary list must be the card
 * displayed in the card menu. This makes deleting cards a bit difficult.
 */

struct carditem {
   QWidget   *w0;		/* primary input widget, 0 if invisible_if */
   QWidget   *w1;		/* secondary widget (card form, label etc) */
};
typedef struct card {
	struct form *form;	/* form struct that controls this card */
	struct dbase*dbase;	/* database that callbacks use to store data */
				/****** summary window ***********************/
	int	    nquery;	/* # of valid row indices in query[] */
	int	    qcurr;	/* index into query[] for displayed card */
	int	    *query;	/* row numbers of cards that satisfy query */
	QTreeWidget *wsummary;	/* summary list widget, for destroying */
				/****** card window **************************/
	QDialog	    *shell;	/* if nonzero, card has its own window */
	QWidget	    *wform;	/* form widget card is drawn into */
	QWidget	    *wcard;	/* child of wform card is drawn into */
	QWidget	    *wstat;	/* child of wform static part is drawn into */
	int	    row;	/* database row shown in card, -1=none */
	int	    disprow;	/* row being displayed in main window */
	int	    nitems;	/* # of items, also size of following array */
	struct carditem items[1];
} CARD;


/*
 * database header struct. A database is basically a big 2-D array of data
 * pointers. A card is stored in a row; each item in the row is a column.
 * Each row can have a different number of columns; to avoid frequent
 * reallocs dbase->nmaxolumns is used as a hint for how many columns to alloc
 * initially when a new row is created.
 */

typedef struct section {
	char	*path;		/* path name of file section was loaded from */
	BOOL	rdonly;		/* no write permission for db file */
	BOOL	modified;	/* TRUE if modified */
	int	nrows;		/* # of cards in this section */
	time_t	mtime;		/* modification time of file when read */
} SECTION;

typedef struct row {
	short	ncolumns;	/* # of columns allocated */
	short	section;	/* section this row belongs to */
	BOOL	selected;	/* for dbase_sort: restore query after sort */
	int	seq;		/* used during sort, preserves order */
	time_t	mtime;		/* last modification time */
	time_t	ctime;		/* creation time, identifies card uniquely */
	int	ctimex;		/* just in case two cards have same ctime */
	char	*data[1];	/* <ncolumns> strings, allocated larger */
} ROW;

typedef struct dbase {
	BOOL	rdonly;		/* no write permission for any section */
	BOOL	modified;	/* TRUE if any section was modified */
	int	maxcolumns;	/* # of columns in widest row */
	int	nrows;		/* # of valid rows in database */
	int	size;		/* # of rows allocated */
	short	nsects;		/* # of files loaded */
	short	currsect;	/* current section, -1=all, 0..nsects-1=one */
	SECTION	*sect;		/* describes all section files 0..nsects-1 */
	BOOL	havesects;	/* db is a directory, >1 sections possible */
	ROW	**row;		/* array of <nrows> rows */
} DBASE;


/*
 * Form describe data types and graphical layout of a card. There is a
 * header struct that describes global info such as the size of the card,
 * and a list of item structs, each of which describes one item (one
 * field, such as input areas, labels, pushbuttons etc).
 *
 * An item is a data entry in a form. It specifies what the entry does,
 * what it looks like, when and how it can be edited by the user etc.
 * When adding a field, provide defaults in item_add(), release malloced
 * data in item_delete(), and provide a user interface in formwin.c.
 */

typedef enum {			/*----- item type */
	IT_NULL = 0,		/* terminates the list, null item */
	IT_LABEL,		/* a text without function, must be 1st */
	IT_PRINT,		/* inset text without function */
	IT_INPUT,		/* arbitrary line of text */
	IT_TIME,		/* date and/or time */
	IT_NOTE,		/* multi-line text */
	IT_CHOICE,		/* diamond on/off switch, one of many */
	IT_FLAG,		/* square on/off switch, no restriction */
	IT_BUTTON,		/* pressable button with script */
	IT_CHART,		/* graphical chart */
	NITEMS
} ITYPE;

/* Warning: these enums are stored as raw numbers in the form file. */
/* This does not apply to IT_* above; just keep itemname[] in sync. */
/* Do not change the ordering or make gaps */

typedef enum {			/*----- justification */
	J_LEFT = 0,		/* default, left-justified */
	J_RIGHT,		/* right-justified */
	J_CENTER		/* centered */
} JUST;

typedef enum {			/*----- standard font */
	F_HELV = 0,		/* standard helvetica */
	F_HELV_O,		/* standard helvetica oblique */
	F_HELV_S,		/* small helvetica */
	F_HELV_L,		/* large helvetica */
	F_COURIER,		/* standard courier */
	F_NFONTS
} FONTN;

typedef enum {			/*----- IT_TIME formats */
	T_DATE = 0,		/* date */
	T_TIME,			/* time */
	T_DATETIME,		/* date and time */
	T_DURATION		/* duration */
} TIMEFMT;

				/* chart.*mode flags: */
#define CC_NEXT		0	/* pos is next free to right/above */
#define CC_SAME		1	/* pos is same as previous */
#define CC_EXPR		2	/* pos/size is computed */
#define CC_DRAG		3	/* pos/size is draggable */
				/* index into chart.value[] */
#define	CC_X		0	/* X position */
#define	CC_Y		1	/* Y position */
#define	CC_XS		2	/* X size */
#define	CC_YS		3	/* Y size */

struct value {
	int	mode;	/* one of CC_* */
	char	*expr;	/* CC_EXPR: expression to eval */
	int	field;	/* CC_DRAG: field number */
	float	mul;	/* CC_DRAG: field * mul + add */
	float	add;	/* CC_DRAG: field * mul + add */
};
typedef struct {		/*----- IT_CHART component */
	BOOL	line;		/* replace bars with lines */
	BOOL	xfat, yfat;	/* make bigger to touch neighbor */
	char	*excl_if;	/* don't draw if this expr is true */
	char	*color;		/* color 0..7 */
	char	*label;		/* numeric label */
	struct value value[4];		/* x, y, xs, ys */
} CHART;

typedef struct {		/*----- storage for visible chart bars */
	float	value[4];	/* evaluated bar position and size */
	int	color;		/* bar color 0..7 */
} BAR;

typedef struct item {
	ITYPE	type;		/* one of IT_* */
	char	*name;		/* field name, used in expressions */
	int	x, y;		/* position in form */
	int	xs, ys;		/* total width and height */
	int	xm, ym;		/* if multipart item, relative pos of split */
	int	sumwidth;	/* width in summary listing if IN_DBASE */
	int	sumcol;		/* column # in summary listing if IN_DBASE */
	char	*sumprint;	/* if nz, show this in summary if IN_DBASE */
	long	column;		/* database column #, 0 is first */
	BOOL	search;		/* queries search this item */
	BOOL	rdonly;		/* user cannot change this field */
	BOOL	nosort;		/* user can sort by this field */
	BOOL	defsort;	/* sort by this field when loading file */
	BOOL	selected;	/* box is selected in form editor */
	char	plan_if;	/* plan interface field type (t=time, ...) */
				/*----- common */
	char	*label;		/* label string */
	JUST	labeljust;	/* label justification */
	int	labelfont;	/* label font, F_* */
				/*----- TIME */
	TIMEFMT	timefmt;	/* one of T_DATE..T_DURATION */
				/*----- CHOICE, FLAG */
	char	*flagcode;	/* dbase column value if on */
	char	*flagtext;	/* text shown in summary if on */
				/*----- conditionals */
	char	*gray_if;	/* if expr is true, turn gray */
	char	*freeze_if;	/* if expr is true, don't permit changes */
	char	*invisible_if;	/* if expr is true, make invisible */
	char	*skip_if;	/* if expr is true, cursor skips field */
				/*----- for INPUT, DATE, TIME, NOTE */
	char	*idefault;	/* default input string */
	int	maxlen;		/* max length of input field */
	JUST	inputjust;	/* input field justification */
	int	inputfont;	/* input font, F_* */
				/*----- for BUTTON */
	char	*pressed;	/* command that button execs when pressed */
	char	*added;		/* command that button execs when added */
				/*----- for CHART */
	float	ch_xmin;	/* coord of left edge */
	float	ch_xmax;	/* coord of right edge */
	float	ch_ymin;	/* coord of bottom edge */
	float	ch_ymax;	/* coord of top edge */
	BOOL	ch_xauto;	/* automatic xmin/xmax */
	BOOL	ch_yauto;	/* automatic ymin/ymax */
	float	ch_xgrid;	/* vert grid lines every xgrid units */
	float	ch_ygrid;	/* horz grid lines every ygrid units */
	float	ch_xsnap;	/* snap X to nearest xsnap */
	float	ch_ysnap;	/* snap Y to nearest ysnap */
	int	ch_ncomp;	/* # of components in ch_comp */
	int	ch_curr;	/* current component index */
	CHART	*ch_comp;	/* component array */
	BAR	*ch_bar;	/* nrows * ncomp bars, ncomp-major order */
	int	ch_nbars;	/* number of bars in ch_bar array */
} ITEM;


/* TRUE if the item type accesses some database field */

#define IN_DBASE(t) (t==IT_INPUT || t==IT_TIME ||\
		     t==IT_NOTE  || t==IT_CHOICE || t==IT_FLAG)


/*
 * form data structure. The form defines which items are stored in the
 * database, and how the data is being displayed. This is the database
 * header describing one card. The actual database information will be
 * used to fill in data into the card, according to the formatting info
 * in the item structures. The order of items in items[] is the order
 * on screen, top->bottom then left->right. Each item has a data index
 * that specifies the data order in the database, which can be different
 * if the card was re-arranged or items were inserted.
 * If you change something here, also change form_create(), form_delete(),
 * and the user interafe in formwin.c.
 * (In the menus, "item" has been renamed to "field".)
 */

typedef struct dquery {
	BOOL	suspended;	/* if TRUE, remove from pulldown */
	char	*name;		/* name of query, for query pulldown */
	char	*query;		/* query expression for evaluate() */
} DQUERY;
	
typedef struct form {
	char	*path;		/* complete path name form was read from */
	char	*name;		/* filename of form */
	char	*dbase;		/* referenced database filename */
	char	*comment;	/* user-defined comment */
	char	*help;		/* help text */
	unsigned char cdelim;	/* column delimiter in database file */
	BOOL	rdonly;		/* don't allow writing to database */
	BOOL	proc;		/* procedural */
	BOOL	syncable;	/* keep timestamp files */
	int	xg, yg;		/* grid size in pixels */
	int	xs, ys;		/* total size of form in pixels */
	int	ydiv;		/* Y of divider between static part and card */
	int	size;		/* # of items the item array has space for*/
	int	nitems;		/* # of items in this form */
	ITEM	**items;	/* array of item definitions */
	int	nqueries;	/* # of queries in query array */
	int	autoquery;	/* query to do when loading, -1=none */
	DQUERY	*query;		/* default queries for query pulldown */
	char	*planquery;	/* default query for -p option */
} FORM;


/*
 * for the parser, variable argument list element
 */

struct arg { struct arg *next; char *value; };
#endif
