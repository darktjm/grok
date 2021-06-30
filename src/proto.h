/*
 * grok prototypes
 */

typedef enum {	SM_SEARCH, SM_INQUERY, SM_NARROW, SM_WIDEN,
		SM_WIDEN_INQUERY, SM_FIND, SM_NMODES } Searchmode;

/*---------------------------------------- canvdraw.c ------------*/

void destroy_canvas_window(void);
class GrokCanvas;
GrokCanvas *create_canvas_window(
	FORM		*f);
void redraw_canvas(void);
void redraw_canvas_item(
	ITEM		*item);		/* item to redraw */

/*---------------------------------------- cardwin.c ------------*/

void destroy_card_menu(
	CARD		*card);		/* card to destroy */
void free_card(
	CARD		*card);		/* card to destroy */
void free_fkey_card(
	CARD		*card);		/* card to destroy */
CARD *create_card_menu(
	FORM		*form,		/* form that controls layout */
	DBASE		*dbase,		/* database for callbacks, or 0 */
	QWidget		*wform,		/* form widget to install into, or 0 */
	bool		no_gui);	/* true to just init card */
void build_card_menu(
	CARD		*card,		/* initialized non-GUI card */
	QWidget		*wform);	/* form widget to install into, or 0 */
void card_readback_texts(
	CARD		*card,		/* card that is displayed in window */
	int		which);		/* all -f < 0, one item only if >= 0 */
const char *format_time_data(
	time_t		time,
	TIMEFMT		timefmt);	/* new format, one of T_* */
time_t parse_time_data(
	const char	*data,
	TIMEFMT		timefmt);	/* new format, one of T_* */
void fillout_card(
	CARD		*card,		/* card to draw into menu */
	bool		deps);		/* if true, dependencies only */
void fillout_item(
	CARD		*card,		/* card to draw into menu */
	int		i,		/* item index */
	bool		deps);		/* if true, dependencies only */

/* property names for widget fonts */
extern const char * const font_prop[F_NFONTS];

/*---------------------------------------- chart.c ------------*/

void add_chart_component(
	ITEM		*item);
void del_chart_component(
	ITEM		*item);
void clone_chart_component(
	CHART		*to,
	CHART		*from);

/*---------------------------------------- chartdrw.c ------------*/

void draw_chart(
	CARD		*card,		/* life, the universe, and everything*/
	int		nitem);		/* # of item in form */

/*---------------------------------------- convert.c ------------*/

const char *mkdatestring(
	time_t		time);		/* date in seconds */
const char *mktimestring(
	time_t		time,		/* date in seconds */
	bool		dur);		/* duration, not time-of-day */
const char *mkdatetimestring(
	time_t		time);		/* date in seconds */
time_t parse_datestring(
	const char	*text);		/* input string */
time_t parse_timestring(
	const char	*text,		/* input string */
	bool		dur);		/* duration, not time-of-day */
time_t parse_datetimestring(
	const char	*text);		/* input string */

/*---------------------------------------- dbase.c ------------*/

DBASE *dbase_create(
	const FORM	*form);		/* how to read/write dbase */
bool check_dbase_form(
	const FORM	*form);
void dbase_delete(
	DBASE		*dbase);	/* dbase to maybe delete */
void dbase_clear(
	DBASE		*dbase);	/* dbase to clear data from */
bool dbase_addrow(
	int		*rowp,		/* ptr to returned row number */
	DBASE		*dbase);	/* database to add row to */
void dbase_delrow(
	int		nrow,		/* row to delete */
	DBASE		*dbase);	/* database to delete row in */
char *dbase_get(
	const DBASE	*dbase,		/* database to get string from */
	int		nrow,		/* row to get */
	int		ncolumn);	/* column to get */
bool dbase_put(
	DBASE		*dbase,		/* database to put into */
	int		nrow,		/* row to put into */
	int		ncolumn,	/* column to put into */
	const char	*data);		/* string to store */
void dbase_sort(
	CARD		*card,		/* database and form to sort */
	int		col,		/* column to sort by */
	bool		rev,		/* reverse if nonzero */
	bool		noinit = false);/* multi-field sort */
int fkey_lookup(
	const DBASE	*dbase,		/* database to search */
	const FORM	*form,		/* fkey origin */
	const ITEM	*item,		/* fkey definition */
	const char	*val,		/* reference value */
	int		keyno = 0,	/* multi: array elt (ret. -2 if oob */
	int		start = 0);	/* row to start looking */
char *fkey_of(
	const DBASE	*dbase,		/* database to search */
	int		row,		/* row */
	const FORM	*form,		/* fkey origin */
	const ITEM	*item);		/* fkey definition */
int keylen_of(
	const ITEM	*item);		/* fkey definition; must be IT_*FKEY */
int copy_fkey(				/* returns <0 on lookup failure */
	const ITEM	*item,		/* fkey definition; must be IT_*FKEY */
	int		*keys);		/* array of length keylen_of; filled in
					   with item->fkey indices */
enum badref_reason {
    BR_MISSING, BR_DUP, BR_INV_MISSING, BR_INV_DUP, BR_NO_INVREF, BR_NO_FORM
    /*, BR_NO_FREF */ /* verify_form ensures this can't hpppen */
};
struct badref {
    const FORM *form, *fform;
    const DBASE *dbase, *fdbase;
    int item, row, keyno;
    enum badref_reason reason;
};
void check_db_references(
	const FORM	*form,
	const DBASE	*db,
	badref		**badrefs,
	int		*nbadref);

/*---------------------------------------- dbfile.c ------------*/

const char *db_path(
	const FORM	*form);
bool write_dbase(
	DBASE		*dbase,		/* form and items to write */
	bool		force);		/* write even if not modified*/
DBASE *read_dbase(
	const FORM	*form,		/* col delim, proc info, etc. */
	bool		force = false);	/* revert if already loaded */

/*---------------------------------------- editwin.c ------------*/

void destroy_edit_popup(void);
void create_edit_popup(
	const char	*title,		/* menu title string */
	char		**initial,	/* initial default text */
	bool		readonly,	/* not modifiable if true */
	const char	*helptag,	/* help tag */
	QTextDocument	*initdoc = 0);	/* full initial document */
void edit_file(
	const char	*name,		/* file name to read */
	bool		readonly,	/* not modifiable if true */
	bool		create,		/* create if nonexistent if true */
	const char	*title,		/* if nonzero, window title */
	const char	*helptag);	/* help tag */

/*---------------------------------------- eval.c ------------*/

const char *evaluate(
	CARD		*card,
	const char	*exp,
	CARD		**switch_card = 0);	/* If non-0, allow switch() */
bool evalbool(
	CARD		*card,
	const char	*exp);

/*---------------------------------------- evalfunc.c ------------*/

/* count aoccurrences of c-chars in s */
int countchars(
	const char	*s,		/* string to search */
	const char	*c);		/* characters to count */
/* remove esc from s, keeping following char.  Returns end of d. */
/* negative len -> strlen(s) */
char *unescape(
	char		*d,		/* destination buffer */
	const char	*s,		/* source; may be same as d */
	int		len,		/* # of chars in s to unescape */
	char esc);			/* escape character */
/* Add esc in front of esc and toesc-chars in s.  Returns end of d. */
/* negative len -> strlen(s) */
/* Don't forget that s will grow (use countchars() to see how much) */
char *escape(
	char		*d,		/* destination buffer */
	const char	*s,		/* source; may not be same as d */
	int		len,		/* # of chars in s to escape */
	char		esc,		/* escape character */
	const char	*toesc);	/* characters to escape; at least include esc! */

/* get a form's separator information */
void get_form_arraysep(
	const FORM	*form,
	char		*sep,
	char		*esc);
/* Main function for iterating over arrays. */
/* assumes end is on a separator; start search at -1.
   Returns -1 for begin @ end */
void next_aelt(
	const char	*array,		/* array to iterate over */
	int		*begin,		/* return: 1st char of next element */
	int		*after,		/* in/out: 1st char after element */
	char		sep,		/* array separator */
	char		esc);		/* array escape char */
int stralen(
	const char	*array,		/* array whose length is to be counted */
	char		sep,		/* array separator */
	char		esc);		/* array escape char */
/* Find array element n */
/* If n > stralen(array), -1 is returned in begin and after */
void elt_at(
	const char	*array,		/* array to traverse */
	unsigned int	n,		/* element # to extract */
	int		*begin,		/* return: begin of extracted element */
	int		*after,		/* return: char after extracted element */
	char		sep,
	char		esc);
/* get and unescape array element n, allocating result if non-empty */
char *unesc_elt_at(
	const FORM	*form,		/* form context (sep, esc) */
	const char	*array,		/* array to traverse */
	int		n);		/* element # to extract */
/* split array into allocated array of allocated unescaped elements */
/* returns NULL on completely empty value */
char **split_array(
	const FORM	*form,		/* form context (sep, esc) */
	const char	*array,		/* array to traverse */
	int		*len);		/* return: length of array */
/* set element n to val.  */
/* array and val can be NULL */
/* non-NULL array assumed to be malloced, and will be freed or reused */
/* return value is NULL or malloc'ed result */
/* array will auto-expand by adding empties if n > array len */
/* returns true if successful, false on memory allocation errors */
bool set_elt(
	char		**array,	/* array to modify */
	int		n,		/* element to set */
	const char	*val,		/* value to set */
	const FORM	*form);		/* where to get sep/esc */
// convert a to a set in-place (false == memory alloc failure)
bool toset(char *a, char sep, char esc);
/* find escaped elt s of length len in non-empty set a */
/* sets begin & end if true; sets begin to insertion point if false */
/* sep/esc of a & s are the same */
bool find_elt(
	const char	*a,		/* set to search */
	const char	*s,		/* element to search for */
	int		len,		/* length of element */
	int		*begin,		/* out: start of found or insert pt */
	int		*after,		/* out: end of found or invalid */
	char		sep,		/* array/element separator */
	char		esc);		/* array/element esc char */
/* find unescaped 0-terminated elt s in non-empty set a, as above. */
bool find_unesc_elt(
	const char	*a,		/* set to search */
	const char	*s,		/* element to search for */
	int		*begin,		/* out: start of found or insert pt */
	int		*after,		/* out: end of found or invalid */
	char		sep,		/* array/element separator */
	char		esc);		/* array/element esc char */


/*---------------------------------------- parser.y ------------*/

/* Set a variable outside of evaluate(); used by templates */
void set_var(CARD *card, int v, char *s);

/*---------------------------------------- formfile.c ------------*/

bool write_form(
	FORM		*form);		/* form and items to write */
FORM *read_form(
	const char	*path,		/* file to read list from */
	bool		force = false);	/* overrwrite loaded forms */

/*---------------------------------------- formop.c ------------*/

FORM *form_create(void);
FORM *form_clone(
	FORM		*parent);	/* old form */
void form_delete(
	FORM		*form);		/* form to delete */
bool verify_form(
	FORM		*form,		/* form to verify */
	int		*bug,		/* retuirned buggy item # */
	QWidget		*shell);	/* error popup parent */
bool check_loaded_forms(
	QString		&msg,
	FORM		*form);
void form_edit_script(
	FORM		*form,		/* form to edit */
	QWidget		*shell,		/* error popup parent */
	const char	*fname);	/* file name of script (dbase name) */
void form_sort(
	FORM		*form);		/* form to sort */
void item_deselect(
	FORM		*form);		/* describes form and all items in it*/
bool item_create(
	FORM		*form,		/* describes form and all items in it*/
	int		nitem);		/* the current item, insert point */
int avail_column(
	const FORM *form,
	const ITEM *item);		/* item to skip in search, or NULL */
void item_delete(
	FORM		*form,		/* describes form and all items in it*/
	int		nitem);		/* the current item, insert point */
ITEM *item_clone(
	ITEM		*parent);	/* item to clone */
void menu_delete(
	MENU		*m);
void menu_clone(
	MENU		*m);
void resolve_fkey_fields(ITEM *item);

/*---------------------------------------- formwin.c ------------*/

void destroy_formedit_window(void);
void create_formedit_window(
	FORM		*def,		/* new form to edit */
	bool		copy);		/* use a copy of <def> */
void sensitize_formedit(void);
void fillout_formedit(void);
void fillout_formedit_widget_by_code(
	int		code);
void readback_formedit(void);

extern const char	plan_code[];	/* code 0x260..0x26c */

/*---------------------------------------- help.c ------------*/

void destroy_help_popup(void);
void bind_help(
	QWidget		*parent,
	const char	*topic);
void help_callback(
	QWidget		*parent,
	const char	*topic);

/*---------------------------------------- main.c ------------*/

extern QApplication	*app;		/* application handle */
extern char		*progname;	/* argv[0] */
extern QIcon		pixmap[NPICS];	/* common symbols */
extern bool		restricted;	/* restricted mode, no form editor */

/*---------------------------------------- mainwin.c ------------*/

void create_mainwindow(void);
void resize_mainwindow(void);
void print_info_line(void);
void find_and_select(char *string);
void remake_dbase_pulldown(void);
void add_dbase_list(QStringList &l);
void remake_section_pulldown(void);
void remake_section_popup(
	bool);
void remake_query_pulldown(void);
void remake_sort_pulldown(void);
void switch_form(
	CARD		*&card,		/* card to switch */
	char		*formname);	/* new form name */
void search_cards(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,
	char		*string);
void append_search_string(
	char		*text);
void do_query(
	int		qmode);		/* -1=all, or query number */

class GrokMainWindow : public QMainWindow {
  public:
    CARD *card = 0;
};

extern GrokMainWindow	*mainwindow;	/* popup menus hang off main window */

/*---------------------------------------- popup.c ------------*/

void create_about_popup(void);
void create_error_popup(
	QWidget		*widget,
	int		error,
	const char	*fmt, ...);
bool create_save_popup(
	QWidget		*widget,	/* window that caused this */
	DBASE		*dbase,		/* database to save */
	const char	*help,		/* help text tag for popup */
	const char	*fmt, ...);	/* message */
#define create_query_popup(w,...) create_save_popup(w, NULL, __VA_ARGS__)
void create_dbase_info_popup(
	CARD		*card);

/*---------------------------------------- prefwin.c ------------*/

void destroy_preference_popup(void);
void create_preference_popup(void);
void write_preferences(void);
void read_preferences(void);

extern struct pref	pref;		/* global preferences */

/*---------------------------------------- template.c ------------*/

int get_template_nbuiltins(void);
char *get_template_path(
	const char	*name,		/* template name or 0 */
	int		seq,		/* if name==0, sequential number */
	CARD		*card);		/* need this for form name */
void list_templates(
	void	(*func)(int, char *),	/* callback with seq# and name */
	CARD		*card);		/* need this for form name */
const char *exec_template(
	char		*oname,		/* output file name, 0=stdout */
	FILE		*ofp,		/* output file descriptor if already open */
	const char	*name,		/* template name to execute */
	int		seq,		/* if name is 0, execute by seq num */
	int		flags,		/* template flags a..z */
	CARD		*card,		/* need this for form name */
	bool		pr_template = false);	/* output template itself to ofp */
char *copy_template(
	QWidget		*shell,		/* export window widget */
	char		*tar,		/* target template name */
	int		seq,		/* source template number */
	CARD		*card);		/* need this for form name */
bool delete_template(
	QWidget		*shell,		/* export window widget */
	int		seq,		/* template to delete, >= NBUILTINS */
	CARD		*card);		/* need this for form name */
const char *substitute_setup(
	char		**array,	/* where to store substitutions */
	const char	*instr);	/* x=y x=y ... command string */
void backslash_subst(char *);

/*---------------------------------------- saveprint.c ------------*/

class QPrinter;
QDataStream& operator<<(QDataStream &os, const QPrinter &printer);
QDataStream& operator>>(QDataStream &is,  QPrinter &printer);

/*---------------------------------------- templwin.c ------------*/

void create_templ_popup(CARD *card);
void create_print_popup(CARD *card);

/*---------------------------------------- templmk.c ------------*/

/* These write a template to fp and return 0 on success, message on error */
const char *mktemplate_html(
	const CARD	*card,
	FILE		*fp);
const char *mktemplate_plain(
	const CARD	*card,
	FILE		*fp);
const char *mktemplate_fancy(
	const CARD	*card,
	FILE		*fp);
const char *mktemplate_sql(
	const CARD	*card,
	FILE		*fp);

/*---------------------------------------- query.c ------------*/

int match_card(
	CARD		*card,		/* database and form */
	char		*string);	/* query string */
void query_any(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	const char	*string);	/* query string */
void query_none(
	CARD		*card);		/* database and form */
void query_all(
	CARD		*card);		/* database and form */
void query_search(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	const char	*string);	/* string to search for */
void query_letter(
	CARD		*card,		/* database and form */
	int		letter);	/* 0=0..9, 1..26=a..z, 27=all */
void query_eval(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	const char	*expr);		/* expression to apply to dbase */

/*---------------------------------------- querywin.c ------------*/

DQUERY *add_dquery(
	FORM		*fp);		/* form to add blank entry to */
void destroy_query_window(void);
void create_query_window(
	FORM		*newform);	/*form whose queries are chgd*/
void print_query_info(void);

/*---------------------------------------- refwin.c ------------*/

void destroy_refby_window(void);
void create_refby_window(
	FORM		*newform);	/*form whose referers are chgd*/

/*---------------------------------------- sectwin.c ------------*/

void destroy_newsect_popup(void);
void create_newsect_popup(CARD *card);

/*---------------------------------------- sumwin.c ------------*/

void destroy_summary_menu(
	CARD		*card);		/* card to destroy */
QWidget *create_summary_widget(void);	/* just create the list widget */
void create_summary_menu(
	CARD		*card);		/* card with query results */
/* get_summary_cols() fills (*res)[] with sorted list of summary columns */
/* nres is alloced size of res, and return value is # of filled entries */
/* menu is non-NULL for multi-column fields */
struct sum_item {
	ITEM		*item;
	const MENU	*menu;
	CARD		*fcard;
	int		sumcol, sumoff;
};
int get_summary_cols(
	struct sum_item	**res,
	size_t		*nres,
	const CARD	*card);
void free_summary_cols(
	struct sum_item	*cols,
	size_t		ncols);
void make_summary_line(
	char		**buf,		/* text buffer for result line */
	size_t		*buf_len,	/* allocated length of *buf */
	const CARD	*card,		/* card with query results */
	int		row,		/* database row */
	QTreeWidget	*w = 0,		/* non-0: add line to table widget */
	int		lrow = -1);	/* >=0: replace row #lrow */
/* setting the header will never modify card */
#define make_summary_header(b, l, c, w) \
	make_summary_line(b, l, c, -1, w)
void make_plan_line(
	CARD		*card,		/* card with query results */
	int		row);		/* database row */
void scroll_summary(
	CARD		*card);		/* which card's summary */
void replace_summary_line(
	CARD		*card,		/* card with query results */
	int		row);		/* database row that has changed */

/*---------------------------------------- util.c ------------*/

const char *section_name(
	const DBASE	*dbase,		/* contains section array */
	int		n);		/* 0 .. dbase->nsects-1 */
const char *resolve_tilde(
	const char	*path,		/* path with ~ */
	const char	*ext);		/* append extension unless 0 */
const char *find_file(
	const char	*name,		/* file name to locate */
	bool		exec);		/* must be executable? */
void fatal(
	const char	*fmt, ...);
char *mystrdup(
	const char	*s);
char *zstrdup(
	const char	*s);
/* I'm tired of double-checking every string */
/* making allocations always return non-0 would only partly fix, as initial */
/* pointers are still NULL. */
#define STR(s) ((s) ? (s) : "") // sort of from former formfile.c
#define BLANK(s) (!(s) || !*(s))
/* C allows NULL frees, but some memory trackers that replace free() don't */
#define zfree(p) do { \
	if(p) \
		free(p); \
	/* p = NULL; */ \
} while(0)
/* safely malloc/realloc, reporting errors and fatally aborting if no parent */
/* note that a 0-length allocation will return NULL unlesss shrinking to 0 */
/* note that AM_ZERO only applies to mallocs or the mlen growth area. */
/* That's why I added azero and zgrow below. */
void *abort_malloc(
	QWidget		*parent,	/* for message popups; 0 = fatal */
	const char	*purpose,	/* for messages; may be 0 */
	void		*old,		/* for realloc; may be 0 */
	size_t		len,		/* how much, minimum */
	size_t		*mlen,		/* if non-NULL, track size of buffer */
	int		flags);		/* additional flags; see below */
#define AM_REALLOC	(1<<0)		/* if unset, grow by free/malloc */
#define AM_ZERO		(1<<1)		/* like calloc: zero out new result */
#define AM_SHRINK	(1<<2)		/* shrink if tracking and len<mlen/2 */

/* initial allocation size when *mlen or old is 0 and mlen is non-NULL */
#define MIN_AM_SIZE 16

/* The maximum size beyond which doubling is no longer done. */
/* doubling is fast, but at large sizes, it can be wasteful */
#define MAX_AM_DOUBLE 65536

/* abort_malloc, but cast to t* (C++ disallows void * auto-convert, anyway) */
/* len is actually # of t-elements */
#define talloc(w, p, t, o, len, mlen, flags) \
    (t *)abort_malloc(w, p, o, (len)*sizeof(t), mlen, flags)
/* shorter versions of talloc for common tasks: */
/* malloc */
#define alloc(w, p, t, len) talloc(w, p, t, NULL, len, NULL, 0)
/* calloc */
#define zalloc(w, p, t, len) talloc(w, p, t, NULL, len, NULL, AM_ZERO)
/* realloc */
#define grow(w, p, t, a, len, track) \
	a = talloc(w, p, t, a, len, track, AM_REALLOC)
/* grow and discard old */
#define fgrow(w, p, t, a, len, track) \
	a = talloc(w, p, t, a, len, track, 0)
/* since mlen only tracks malloc len, AM_ZERO can't work with it */
/* instead, zero out with: */
#define azero(t, a, l, cnt) memset((a)+(l), 0, (cnt)*sizeof(t))
/* note that t may equal *a for same effect */
/* realloc while clearing new data */
#define zgrow(w, p, t, a, olen, len, track) do { \
	size_t zg_len_ = len, zg_olen_ = olen; \
	a = talloc(w, p, t, a, zg_len_, track, AM_REALLOC); \
	azero(t, a, zg_olen_, zg_len_ - zg_olen_); \
} while(0)
/* same as above but for "built-in" array-at-end-of-struct "e": */
#define btalloc(w, p, t, o, e, len, mlen, flags) \
    (t *)abort_malloc(w, p, o, offsetof(t,e)+(len)*sizeof(((t*)0)->e[0]), mlen, flags)
#define balloc(w, p, t, e, len) btalloc(w, p, t, NULL, e, len, NULL, 0)
#define bzalloc(w, p, t, e, len) btalloc(w, p, t, NULL, e, len, NULL, AM_ZERO)
#define bgrow(w, p, t, a, e, len, track) \
	a = btalloc(w, p, t, a, e, len, track, AM_REALLOC)
#define bfgrow(w, p, t, a, e, len, track) \
	a = btalloc(w, p, t, a, e, len, track, 0)
#define bazero(t, a, e, l, cnt) memset((a)->e+(l), 0, (cnt)*sizeof(((t*)0)->e[0]))
#define bzgrow(w, p, t, a, e, olen, len, track) do { \
	size_t zg_len_ = len, zg_olen_ = olen; \
	a = btalloc(w, p, t, a, e, zg_len_, track, AM_REALLOC); \
	bazero(t, a, e, zg_olen_, zg_len_ - zg_olen_); \
} while(0)

/* type-safe memcpy/memmove/memset */
#define tmemcpy(t, d, s, n) do { \
	t *tmc_pd_ = d; \
	const t *tmc_ps_ = s; \
	memcpy(tmc_pd_, tmc_ps_, (n)*sizeof(t)); \
} while(0)
#define tmemmove(t, d, s, n) do { \
	t *tmm_pd_ = d; /* type check */ \
	const t *tmm_ps_ = s; /* type check */ \
	memmove(tmm_pd_, tmm_ps_, (n)*sizeof(t)); \
} while(0)
#define tzero(t, p, n) do { \
	t *tz_pp_ = p; /* type check */ \
	memset(tz_pp_, 0, (n)*sizeof(t)); \
} while(0)

const char *canonicalize(
	const char	*path,
	bool		dir_only);

void print_button(
	QWidget		*w,
	const char	*fmt, ...);
#define print_text_button print_button
void print_text_button_s(
	QWidget		*w,
	const char	*str);
char *read_text_button_noskipblank(
	QWidget		*,
	char		**);
char *read_text_button(
	QWidget		*,
	char		**);
char *read_text_button_noblanks(
	QWidget		*,
	char		**);
void set_toggle(
	QWidget		*w,
	bool		set);
void set_icon(
	QWidget		*shell,
	int		sub);		/* 0=main, 1=submenu */
void set_cursor(
	QWidget		*w,		/* in which widget */
	Qt::CursorShape	n);		/* which cursor, one of XC_* */
void truncate_string(
	QWidget		*w,		/* widget string will show in */
	QString		&string,	/* string to truncate */
	int		len);		/* max len in pixels */
int strlen_in_pixels(
	QWidget		*w,		/* widget string will show in */
	const char	*string);	/* string to truncate */
const char *to_octal(
	int		n);		/* ascii to convert to string */
char to_ascii(
	char		*str,		/* string to convert to ascii */
	int		def);		/* default if string is empty */
// Convenience function to make a horizontal separator
QWidget *mk_separator(void);
/* 80-pixel-wide button in a DialogButtonBox */
// Why does C++ always have to be so goddamn verbose?
#define dbbb(x) QDialogButtonBox::x
#define dbbr(x) dbbb(x##Role)
QPushButton *mk_button(
	QDialogButtonBox*bb,		/* target button box; may be 0 */
	const char	*label,		/* label, or NULL if standard */
	int		role = dbbr(Action));	/* role, or ID if label is NULL */
void add_layout_qss(
	QLayout		*l,		/* layout to get QSS support */
	const char	*name);		/* object name, or NULL if none */

// These macros allow me to connect Qt signals to C callbacks
#define set_qt_cb(_t, _sig, _w, _f, ...) \
    QObject::connect(dynamic_cast<_t *>(_w), &_t::_sig, [=](__VA_ARGS__){ _f; })
#define set_qt_cb_ov1(_t, _sig, _p, _w, _f, ...) \
    QObject::connect(dynamic_cast<_t *>(_w), QOverload<_p>::of(&_t::_sig), [=](__VA_ARGS__){ _f; })
#define set_button_cb(_w, _f, ...) \
    set_qt_cb(QAbstractButton, clicked, _w, _f, __VA_ARGS__)
/* for some reason, editingFinished is generated way too often sometimes */
#define set_text_cb(_w, _f) set_qt_cb(QLineEdit, editingFinished, _w, _f)
/* use this one if it's important to generate less often */
#define set_textr_cb(_w, _f) set_qt_cb(QLineEdit, returnPressed, _w, _f)
#define set_spin_cb(_w, _f) set_qt_cb(QAbstractSpinBox, editingFinished, _w, _f)
#define set_dialog_cancel_cb(_w, _f) set_qt_cb(QDialog, rejected, _w, _f)
#define set_file_dialog_cb(_w, _f, _v) set_qt_cb(QFileDialog, fileSelected, \
	                                         _w, _f, const QString &_v)
#define set_popup_cb(_w, _f, _t, _v) \
    set_qt_cb_ov1(QComboBox, currentIndexChanged, _t, _w, _f, UNUSED _t _v)
#define set_combo_cb(_w, _f, ...) set_qt_cb(QComboBox, currentTextChanged, _w, _f, __VA_ARGS__)
// Make the calls needed to pop up a non-modal dialog (use exec for modal)
void popup_nonmodal(
	QDialog		*d);
// Convert QString to char * by allocating enough memory
char *qstrdup(
	const QString	&str);
/* too much typing */
#define qsprintf QString::asprintf

#define ALEN(a) (int)(sizeof(a)/sizeof((a)[0]))
#define APTR_OK(p, a) ((p)-(a) < ALEN(a))
