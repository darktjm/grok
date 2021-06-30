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

typedef struct form FORM;
typedef struct dbase DBASE;
struct carditem {
   QWidget   *w0;		/* primary input widget, 0 if invisible_if */
   QWidget   *w1;		/* secondary widget (card form, label etc) */
};
typedef struct card CARD;
struct card {
	CARD	     *next;	/* link for all reusable cards */
	FORM	    *form;	/* form struct that controls this card */
	char	    *prev_form;	/* previous form name */
	DBASE	    *dbase;	/* database that callbacks use to store data */
				/****** summary window ***********************/
	int	    nquery;	/* # of valid row indices in query[] */
	int	    qcurr;	/* index into query[] for displayed card */
	int	col_sorted_by;	/* dbase is sorted by this column */
	int	    *sorted;	/* all database rows in sort order */
	int	    *query;	/* row numbers of cards that satisfy query */
	char	    *letter_mask; /* letters search within last results */
	int	    lm_size;	/* size of letter_mask */
	QTreeWidget *wsummary;	/* summary list widget, for destroying */
	CARD	    *fkey_next;	/* foreign key related links */
				/****** card window **************************/
	QDialog	    *shell;	/* if nonzero, card has its own window */
	QWidget	    *wform;	/* form widget card is drawn into */
	QWidget	    *wcard;	/* child of wform card is drawn into */
	QWidget	    *wstat;	/* child of wform static part is drawn into */
	int	    row;	/* database row shown in card, -1=none */
	int	    disprow;	/* row being displayed in main window */
	int	    nitems;	/* # of items, also size of following array */
	int	    last_query;	/* last query pd index, for ReQuery */
	struct carditem items[1];
};

extern CARD *card_list;

/*
 * database header struct. A database is basically a big 2-D array of data
 * pointers. A card is stored in a row; each item in the row is a column.
 * Each row can have a different number of columns; to avoid frequent
 * reallocs dbase->nmaxolumns is used as a hint for how many columns to alloc
 * initially when a new row is created.
 */

typedef struct section {
	char	*path;		/* path name of file section was loaded from */
	bool	rdonly;		/* no write permission for db file */
	bool	modified;	/* true if modified */
	int	nrows;		/* # of cards in this section */
	time_t	mtime;		/* modification time of file when read */
} SECTION;

typedef struct row {
	short	ncolumns;	/* # of columns allocated */
	short	section;	/* section this row belongs to */
	time_t	mtime;		/* last modification time */
	time_t	ctime;		/* creation time, identifies card uniquely */
	int	ctimex;		/* just in case two cards have same ctime */
	char	*data[1];	/* <ncolumns> strings, allocated larger */
} ROW;

/* An expression variable */
typedef struct evar {
	char	*string;
	double	value;
	bool	numeric;
} EVAR;

struct dbase {
	DBASE	    *next;	/* link for all loaded databases */
	char	    *path;	/* File/dir; also unique key in dbase_list */
	const FORM  *form;	/* form this was loaded with */
	bool	    rdonly;	/* no write permission for any section */
	bool	    modified;	/* true if any section was modified */
	int	    maxcolumns;	/* # of columns in widest row */
	int	    nrows;	/* # of valid rows in database */
	size_t	    size;	/* # of rows allocated */
	short	    nsects;	/* # of files loaded */
	/* FIXME: move currsect to card */
	/* it's basically a per-window GUI thing */
	short	    currsect;	/* current section, -1=all, 0..nsects-1=one */
	SECTION	    *sect;	/* describes all section files 0..nsects-1 */
	bool	    havesects;	/* db is a directory, >1 sections possible */
	ROW	    **row;	/* array of <nrows> rows */
	int	    ctimex_next;/* just in case two cards have same ctime */
	EVAR	    var[26];	/* dbase-local expression variables */
};

extern DBASE *dbase_list;

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
	IT_NUMBER,		/* "spin" box for entering numbers */
	IT_MENU,		/* popup menu with fixed values */
	IT_RADIO,		/* radio groups as a single widget */
	IT_MULTI,		/* flags as a multiselect list */
	IT_FLAGS,		/* flags as a group of checkboxes */
	IT_FKEY,		/* reference to other database */
	IT_INV_FKEY,		/* references from other database */
	NITEMS
} ITYPE;

/* Warning: these enums are stored as raw numbers in the form file. */
/* This does not apply to IT_* above; just keep itemname[] in sync. */
/* Do not change the ordering or make gaps */

typedef enum {			/*----- dynamic combo box mode */
	C_NONE, C_QUERY, C_ALL
} DCOMBO;

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
	double	mul;	/* CC_DRAG: field * mul + add */
	double	add;	/* CC_DRAG: field * mul + add */
};
typedef struct {		/*----- IT_CHART component */
	bool	line;		/* replace bars with lines */
	bool	xfat, yfat;	/* make bigger to touch neighbor */
	char	*excl_if;	/* don't draw if this expr is true */
	char	*color;		/* color 0..7 */
	char	*label;		/* numeric label */
	struct value value[4];		/* x, y, xs, ys */
} CHART;

typedef struct {		/*----- storage for visible chart bars */
	double	value[4];	/* evaluated bar position and size */
	int	color;		/* bar color 0..7 */
} BAR;

typedef struct {
	/* These fields override the equivalent ITEM fields */
	char	*label;		/* Must be non-blank and unique within menu */
	/* flag info for all but IT_INPUT */
	char	*flagcode;	/* Must be unique within menu, and non-blank if not IT_MENU or IT_RADIO */
	char	*flagtext;
	/* multicol info for IT_MULTI, IT_FLAGS */
	char	*name;		/* Must be non-blank and unique within form */
	int	column;		/* must be unique within form */
	int	sumcol;		/* must be unique within form if sumwidth > 0 */
	int	sumwidth;
} MENU;

typedef struct item ITEM;
typedef struct {
	char	*name;		/* foreign variable name */
	ITEM	*item;	/* resolved field pointer */
	const MENU *menu;	/* resolved field pointer */
	int	index;		/* resolved field pointer */
	bool	key;		/* part of key, or just display? */
	bool	display;	/* if key, display value? */
} FKEY;

enum {
	/* General */
	IF_SEARCH,		/* queries search this item */
	IF_RDONLY,		/* user cannot change this field */
	IF_NOSORT,		/* user can sort by this field */
	IF_DEFSORT,		/* sort by this field when loading file */

	/* Menu */
	IF_MULTICOL,		/* true if menu[] has multiple column defs */

	/* Chart */
	IF_CH_XAUTO,		/* automatic xmin/xmax */
	IF_CH_YAUTO,		/* automatic ymin/ymax */

	/* Reference */
	IF_FKEY_HEADER,		/* display headers? */
	IF_FKEY_SEARCH,		/* parent restriction search field? */
	IF_FKEY_MULTI,		/* key field is array of parents */

	/* not part of form definition */
	IF_SELECTED,		/* box is selected in form editor */

};
/* IFL should return same as bool did */
#define IFL(i,x) ((i flags & (1U<<IF_##x)) ? true : false)
#define IFT(i,x) i flags ^= 1U<<IF_##x
#define IFS(i,x) i flags |= 1U<<IF_##x
#define IFC(i,x) i flags &= ~(1U<<IF_##x)
#define IFV(i,x,v) do { \
	if(v) \
		IFS(i,x); \
	else \
		IFC(i,x); \
} while(0)
#define IFX(i,j,x) i flags = ((i flags) & ~(1U<<IF_##x)) | (j flags & (1U<<IF_##x))

struct item {
	ITYPE	type;		/* one of IT_* */
	char	*name;		/* field name, used in expressions */
	int	x, y;		/* position in form */
	int	xs, ys;		/* total width and height */
	int	xm, ym;		/* if multipart item, relative pos of split */
	int	sumwidth;	/* width in summary listing if IN_DBASE */
	int	sumcol;		/* column # in summary listing if IN_DBASE */
	char	*sumprint;	/* if nz, show this in summary if IN_DBASE */
	long	column;		/* database column #, 0 is first */
	int	flags;		/* misc flags; see IF_* above */
	char	plan_if;	/* plan interface field type (t=time, ...) */
				/*----- common */
	char	*label;		/* label string */
	JUST	labeljust;	/* label justification */
	int	labelfont;	/* label font, F_* */
				/*----- TIME */
	TIMEFMT	timefmt;	/* one of T_DATE..T_DURATION */
	int	timewidget;	/* use special widget? */
				/*----- CHOICE, FLAG */
	char	*flagcode;	/* dbase column value if on */
	char	*flagtext;	/* text shown in summary if on */
				/*----- conditionals */
	char	*gray_if;	/* if expr is true, turn gray */
	char	*freeze_if;	/* if expr is true, don't permit changes */
	char	*invisible_if;	/* if expr is true, make invisible */
	char	*skip_if;	/* if expr is true, cursor skips field */
				/*----- for INPUT, DATE, TIME, NOTE, NUMBER */
	char	*idefault;	/* default input string */
	int	maxlen;		/* max length of input field */
	double	min, max;	/* NUMBER range */
	int	digits;		/* NUMBER digits past decimal */
	int	nmenu;		/* number of items in menu array */
	MENU	*menu;		/* combo box static entries & multi-item config */
	DCOMBO	dcombo;		/* INPUT combo box dynamic entry type */
	JUST	inputjust;	/* input field justification */
	int	inputfont;	/* input font, F_* */
				/*----- for BUTTON */
	char	*pressed;	/* command that button execs when pressed */
				/*----- for CHART */
	double	ch_xmin;	/* coord of left edge */
	double	ch_xmax;	/* coord of right edge */
	double	ch_ymin;	/* coord of bottom edge */
	double	ch_ymax;	/* coord of top edge */
	double	ch_xgrid;	/* vert grid lines every xgrid units */
	double	ch_ygrid;	/* horz grid lines every ygrid units */
	double	ch_xsnap;	/* snap X to nearest xsnap */
	double	ch_ysnap;	/* snap Y to nearest ysnap */
	int	ch_ncomp;	/* # of components in ch_comp */
	int	ch_curr;	/* current component index */
	CHART	*ch_comp;	/* component array */
	BAR	*ch_bar;	/* nrows * ncomp bars, ncomp-major order */
	int	ch_nbars;	/* number of bars in ch_bar array */
				/*----- for FKEY, INV_FKEY */
	char	*fkey_form_name;	/* foreign database */
	FORM	*fkey_form;	/* resolved foreign database */
	int	nfkey;		/* number of items in fkey field array */
	FKEY	*fkey;		/* key/display field[s] */
};


/* true if the item type uses multicol */

#define IS_MULTI(t) (t == IT_MULTI || t == IT_FLAGS)

/* true if the item type uses the menu */

#define IS_MENU(t) (t == IT_INPUT || t == IT_MENU || t == IT_RADIO || IS_MULTI(t))

/* true if the item type accesses some database field */

#define IN_DBASE(t) (t==IT_TIME  || t==IT_NUMBER || IS_MENU(t) ||\
		     t==IT_NOTE  || t==IT_CHOICE || t==IT_FLAG || t == IT_FKEY)


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

/* I didn't want to use C++ except where needed for Qt, but this isn't
 * worth using C alteratives. */
#include <unordered_map>
#include <functional> // for hash
#include <string> // yes, converting to string every time is stupid

// using namespace std;  // conflicts too much; just live with std:: prefix

typedef std::unordered_map<char *, int, std::hash<std::string>,
					std::equal_to<std::string>> FIELDS;

typedef struct dquery {
	bool	suspended;	/* if true, remove from pulldown */
	char	*name;		/* name of query, for query pulldown */
	char	*query;		/* query expression for evaluate() */
} DQUERY;
	
struct form {
	FORM	*next;	/* link for all loaded forms */
	char	*path;		/* complete path name form was read from */
	char	*dir;		/* directory form was in, fully resolved */
	char	*name;		/* filename of form (usually) */
	char	*dbase;		/* referenced database filename */
	const char  *dbpath;	/* database path if known/loaded */
	char	*comment;	/* user-defined comment */
	char	*help;		/* help text */
	unsigned char cdelim;	/* column delimiter in database file */
	unsigned char asep, aesc;  /* string array delimiter and how to escape it */
	bool	rdonly;		/* don't allow writing to database */
	bool	proc;		/* procedural */
	bool	syncable;	/* keep timestamp files */
	int	xg, yg;		/* grid size in pixels */
	int	xs, ys;		/* total size of form in pixels */
	int	ydiv;		/* Y of divider between static part and card */
	size_t	size;		/* # of items the item array has space for*/
	int	nitems;		/* # of items in this form */
	ITEM	**items;	/* array of item definitions */
	int	nqueries;	/* # of queries in query array */
	int	autoquery;	/* query to do when loading, -1=none */
	DQUERY	*query;		/* default queries for query pulldown */
	char	*planquery;	/* default query for -p option */
	FIELDS	*fields;	/* map fields to item#/menu# */
	int	nchild;		/* number of databases referencing this */
	char	**children;	/* databases probably referencing this */
};

extern FORM *form_list;
#endif
