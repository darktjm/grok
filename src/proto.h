/*
 * grok prototypes
 */

typedef enum {	SM_SEARCH, SM_INQUERY, SM_NARROW, SM_WIDEN,
		SM_WIDEN_INQUERY, SM_FIND, SM_NMODES } Searchmode;

/*---------------------------------------- canvdraw.c ------------*/

void destroy_canvas_window(void);
void create_canvas_window(
	FORM		*f);
void redraw_canvas(void);
void redraw_canvas_item(
	register ITEM	*item);		/* item to redraw */

/*---------------------------------------- cardwin.c ------------*/

void destroy_card_menu(
	register CARD	*card);		/* card to destroy */
CARD *create_card_menu(
	FORM		*form,		/* form that controls layout */
	DBASE		*dbase,		/* database for callbacks, or 0 */
	QWidget		*wform);		/* form widget to install into, or 0 */
void card_readback_texts(
	CARD		*card,		/* card that is displayed in window */
	int		which);		/* all -f < 0, one item only if >= 0 */
const char *format_time_data(
	const char	*data,		/* string from database */
	TIMEFMT		timefmt);	/* new format, one of T_* */
void fillout_card(
	CARD		*card,		/* card to draw into menu */
	BOOL		deps);		/* if TRUE, dependencies only */
void fillout_item(
	CARD		*card,		/* card to draw into menu */
	int		i,		/* item index */
	BOOL		deps);		/* if TRUE, dependencies only */

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
char *mktimestring(
	time_t		time,		/* date in seconds */
	BOOL		dur);		/* duration, not time-of-day */
time_t parse_datestring(
	char		*text);		/* input string */
time_t parse_timestring(
	char		*text,		/* input string */
	BOOL		dur);		/* duration, not time-of-day */
time_t parse_datetimestring(
	char		*text);		/* input string */

/*---------------------------------------- dbase.c ------------*/

DBASE *dbase_create(void);
void dbase_delete(
	DBASE		*dbase);	/* dbase to delete */
BOOL dbase_addrow(
	int		*rowp,		/* ptr to returned row number */
	register DBASE	*dbase);	/* database to add row to */
void dbase_delrow(
	int		nrow,		/* row to delete */
	register DBASE	*dbase);	/* database to delete row in */
char *dbase_get(
	register DBASE	*dbase,		/* database to get string from */
	register int	nrow,		/* row to get */
	register int	ncolumn);	/* column to get */
BOOL dbase_put(
	register DBASE	*dbase,		/* database to put into */
	register int	nrow,		/* row to put into */
	register int	ncolumn,	/* column to put into */
	const char	*data);		/* string to store */
void dbase_sort(
	CARD		*card,		/* database and form to sort */
	int		col,		/* column to sort by */
	int		rev);		/* reverse if nonzero */

extern int	col_sorted_by;		/* dbase is sorted by this column */

/*---------------------------------------- dbfile.c ------------*/

BOOL write_dbase(
	DBASE		*dbase,		/* form and items to write */
	FORM		*form,		/* contains column delimiter */
	BOOL		force);		/* write even if not modified*/
BOOL read_dbase(
	DBASE		*dbase,		/* form and items to write */
	FORM		*form,		/* contains column delimiter */
	char		*path);		/* file to read list from */

/*---------------------------------------- editwin.c ------------*/

void destroy_edit_popup(void);
void create_edit_popup(
	const char	*title,		/* menu title string */
	char		**initial,	/* initial default text */
	BOOL		readonly,	/* not modifiable if TRUE */
	const char	*helptag);	/* help tag */
void edit_file(
	const char	*name,		/* file name to read */
	BOOL		readonly,	/* not modifiable if TRUE */
	BOOL		create,		/* create if nonexistent if TRUE */
	const char	*title,		/* if nonzero, window title */
	const char	*helptag);	/* help tag */

/*---------------------------------------- eval.c ------------*/

const char *evaluate(
	CARD		*card,
	const char	*exp);
bool eval_error(void); // true if last evaluate() encountered an error
BOOL evalbool(
	CARD		*card,
	const char	*exp);
const char *subeval(
	const char	*exp);
BOOL subevalbool(
	const char	*exp);
void f_foreach(
	char		*cond,
	char		*expr);
int parserlex(void);
void parsererror(
	const char *msg);
int yywrap(void);


extern int	yylineno;		/* current line # being parsed */
extern char	*yyret;			/* returned string (see parser.y) */
extern CARD	*yycard;		/* database for which to evaluate */
extern char	*switch_name;		/* if switch statement was found, */
extern char	*switch_expr;		/* .. name to switch to and expr */
extern BOOL	assigned;		/* did a field assignment */

/*---------------------------------------- parser.y ------------*/
extern int parserparse(void);
void init_variables(void);
void set_var(int v, const char *s);

/*---------------------------------------- evalfunc.c ------------*/

double f_num(
	char		*s);
double f_sum(				/* sum */
	register int	column);	/* number of column to average */
double f_avg(				/* average */
	register int	column);	/* number of column to average */
double f_dev(				/* standard deviation */
	register int	column);	/* number of column to average */
double f_min(				/* minimum */
	register int	column);	/* number of column to average */
double f_max(				/* maximum */
	register int	column);	/* number of column to average */
double f_qsum(				/* sum */
	register int	column);	/* number of column to average */
double f_qavg(				/* average */
	register int	column);	/* number of column to average */
double f_qdev(				/* standard deviation */
	register int	column);	/* number of column to average */
double f_qmin(				/* minimum */
	register int	column);	/* number of column to average */
double f_qmax(				/* maximum */
	register int	column);	/* number of column to average */
double f_ssum(				/* sum */
	register int	column);	/* number of column to average */
double f_savg(				/* average */
	register int	column);	/* number of column to average */
double f_sdev(				/* standard deviation */
	register int	column);	/* number of column to average */
double f_smin(				/* minimum */
	register int	column);	/* number of column to average */
double f_smax(				/* maximum */
	register int	column);	/* number of column to average */
char *f_field(
	int		column,
	int		row);
char *f_expand(
	int		column,
	int		row);
char *f_assign(
	int		column,
	int		row,
	char		*data);
int f_section(
	int		nrow);
char *f_system(
	char		*cmd);
char *f_tr(
	char		*string,
	char		*rules);
char *f_substr(
	char		*string,
	int		pos,
	int		num);
BOOL f_instr(
	char		*match,
	char		*string);
struct arg *f_addarg(
	struct arg	*list,		/* easier to keep struct arg local */
	char		*value);	/* argument to append to list */
char *f_printf(
	struct arg	*arg);
int re_match(char *s, char *e);
char *re_sub(char *s, char *e, char *r, bool all);
// count aoccurrences of c-chars in s
int countchars(const char *s, char *c);
// remove esc from s, keeping following char.  Returns end of d.
// negative len -> strlen(s)
char *unescape(char *d, const char *s, int len, char esc);
// Add esc in front of esc and toesc-chars in s.  Returns end of d.
// negative len -> strlen(s)
// Don't forget that s will grow (use countchars() to see how much)
char *escape(char *d, const char *s, int len, char esc, char *toesc);
char *f_esc(char *s, char *e);
int f_alen(char *array);
char *f_elt(char *array, int n);
char *f_setelt(char *array, int n, char *val);
void f_foreachelt(
	int		var,
	char		*array,
	char		*cond,
	char		*expr,
	bool		nonblank);
char *f_toset(char *a);
char *f_union(char *a, char *b);
char *f_intersect(char *a, char *b);
char *f_setdiff(char *a, char *b);

/* get loaded form's separator information */
void get_cur_arraysep(char *sep, char *esc);
/* assumes end is no a separator; start search at -1.
   Returns -1 for begin @ end */
void next_aelt(char *array, int *begin, int *after, char sep, char esc);
int stralen(char *array, char sep, char esc);
void find_elt(char *array, int n, int *begin, int *after, char sep, char esc);
char *set_elt(char *array, int n, char *val);
// convert a to a set in-place
void toset(char *a, char sep, char esc);
// find escaped elt e of length len in non-empty set a
bool findelt(char *a, char *s, int len, int *begin, int *after, char sep, char esc);


/*---------------------------------------- formfile.c ------------*/

BOOL write_form(
	FORM		*form);		/* form and items to write */
BOOL read_form(
	FORM		*form,		/* form and items to write */
	char		*path);		/* file to read list from */

/*---------------------------------------- formop.c ------------*/

FORM *form_create(void);
FORM *form_clone(
	FORM		*parent);	/* old form */
void form_delete(
	FORM		*form);		/* form to delete */
BOOL verify_form(
	FORM		*form,		/* form to verify */
	int		*bug,		/* retuirned buggy item # */
	QWidget		*shell);		/* error popup parent */
void form_edit_script(
	FORM		*form,		/* form to edit */
	QWidget		*shell,		/* error popup parent */
	char		*fname);	/* file name of script (dbase name) */
void form_sort(
	register FORM	*form);		/* form to sort */
void item_deselect(
	register FORM	*form);		/* describes form and all items in it*/
BOOL item_create(
	register FORM	*form,		/* describes form and all items in it*/
	int		nitem);		/* the current item, insert point */
void item_delete(
	FORM		*form,		/* describes form and all items in it*/
	int		nitem);		/* the current item, insert point */
ITEM *item_clone(
	register ITEM	*parent);	/* item to clone */

/*---------------------------------------- formwin.c ------------*/

void destroy_formedit_window(void);
void create_formedit_window(
	FORM		*def,		/* new form to edit */
	BOOL		copy,		/* use a copy of <def> */
	BOOL		isnew);		/* ok to change form name */
void sensitize_formedit(void);
void fillout_formedit(void);
void fillout_formedit_widget_by_code(
	int		code);
void readback_formedit(void);

extern int		curr_item;	/* current item, 0..form.nitems-1 */
extern const char	plan_code[];	/* code 0x260..0x26c */

/*---------------------------------------- help.c ------------*/

void destroy_help_popup(void);
/* Qt doesn't support a "help callback" */
/* closest would be binding the Help key, I guess */
void bind_help(
	QWidget		*parent,
	const char	*topic);
/* or maybe tooltip or "what's this?"? for the whole dialog? */
/* tooltip/whatsThis requires loading text in advance, though */
/* maybe later */
void help_callback(
	QWidget		*parent,
	const char	*topic);

/*---------------------------------------- main.c ------------*/

extern QApplication	*app;		/* application handle */
extern char		*progname;	/* argv[0] */
extern QIcon		pixmap[NPICS];	/* common symbols */
extern BOOL		restricted;	/* restricted mode, no form editor */

/*---------------------------------------- mainwin.c ------------*/

void create_mainwindow(void);
void resize_mainwindow(void);
void print_info_line(void);
void remake_dbase_pulldown(void);
void remake_section_pulldown(void);
void remake_section_popup(BOOL);
void remake_query_pulldown(void);
void remake_sort_pulldown(void);
void switch_form(
	char		*formname);	/* new form name */
void search_cards(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,
	char		*string);
void do_query(
	int		qmode);		/* -1=all, or query number */

extern CARD 		*curr_card;	/* card being displayed in main win, */
extern char		*prev_form;	/* previous form name */
extern QMainWindow	*mainwindow;	/* popup menus hang off main window */
extern int		last_query;	/* last query pd index, for ReQuery */
extern QVBoxLayout	*mainform;	/* form for summary table */
extern QWidget		*w_summary;	/* the widget to replace in form */

/*---------------------------------------- popup.c ------------*/

void create_about_popup(void);
void create_error_popup(QWidget *widget, int error, const char *fmt, ...);
void create_query_popup(
	QWidget		*widget,		/* window that caused this */
	void		(*callback)(void),	/* OK callback */
	const char	*help,		/* help text tag for popup */
	const char	*fmt, ...);	/* message */
void create_dbase_info_popup(
	CARD		*card);

/*---------------------------------------- prefwin.c ------------*/

void destroy_preference_popup(void);
void create_preference_popup(void);
void write_preferences(void);
void read_preferences(void);

extern struct pref	pref;		/* global preferences */

/*---------------------------------------- print.c ------------*/

void print(void);

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
	const char	*name,		/* template name to execute */
	int		seq,		/* if name is 0, execute by seq num */
	CARD		*card);		/* need this for form name */
char *copy_template(
	QWidget		*shell,		/* export window widget */
	char		*tar,		/* target template name */
	int		seq,		/* source template number */
	CARD		*card);		/* need this for form name */
BOOL delete_template(
	QWidget		*shell,		/* export window widget */
	int		seq,		/* template to delete, >= NBUILTINS */
	CARD		*card);		/* need this for form name */
const char *eval_template(
	const char	*iname,		/* template filename */
	char		*oname);	/* default output filename, 0=stdout */
const char *substitute_setup(
	char		**array,	/* where to store substitutions */
	char		*instr);	/* x=y x=y ... command string */
void backslash_subst(char *);

/*---------------------------------------- templwin.c ------------*/

void destroy_templ_popup(void);
void create_templ_popup(void);

/*---------------------------------------- templmk.c ------------*/

const char *mktemplate_html(
	char		*oname,		/* default output filename, 0=stdout */
	int		mode);		/* 0=both, 1=summary, 2=data list */

/*---------------------------------------- printwin.c ------------*/

void destroy_print_popup(void);
void create_print_popup(void);

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

/*---------------------------------------- sectwin.c ------------*/

void destroy_newsect_popup(void);
void create_newsect_popup(void);

/*---------------------------------------- sumwin.c ------------*/

void destroy_summary_menu(
	register CARD	*card);		/* card to destroy */
void create_summary_menu(
	CARD		*card);		/* card with query results */
void make_summary_line(
	char		*buf,		/* text buffer for result line */
	CARD		*card,		/* card with query results */
	int		row,		/* database row */
	QTreeWidget	*w = 0,		/* non-0: add line to table widget */
	int		lrow = -1);	/* >=0: replace row #lrow */
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
	register DBASE	*dbase,		/* contains section array */
	int		n);		/* 0 .. dbase->nsects-1 */
char *resolve_tilde(
	char		*path,		/* path with ~ */
	const char	*ext);		/* append extension unless 0 */
BOOL find_file(
	char		*buf,		/* buffer for returned path */
	const char	*name,		/* file name to locate */
	BOOL		exec);		/* must be executable? */
void fatal(const char *fmt, ...);
char *mystrdup(
	const char	*s);
void print_button(QWidget *w, const char *fmt, ...);
#define print_text_button print_button
void print_text_button_s(QWidget *w, const char *str);
char *read_text_button_noskipblank(QWidget*, char **);
char *read_text_button(QWidget*, char **);
char *read_text_button_noblanks(QWidget*, char **);
void set_toggle(
	QWidget		*w,
	BOOL		set);
void set_icon(
	QWidget		*shell,
	int		sub);		/* 0=main, 1=submenu */
void set_cursor(
	QWidget		*w,		/* in which widget */
	Qt::CursorShape	n);		/* which cursor, one of XC_* */
void truncate_string(
	QWidget		*w,		/* widget string will show in */
	register char	*string,	/* string to truncate */
	register int	len);		/* max len in pixels */
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
	QDialogButtonBox *bb,		/* target button box; may be 0 */
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
#define set_text_cb(_w, _f) set_qt_cb(QLineEdit, editingFinished, _w, _f)
#define set_spin_cb(_w, _f) set_qt_cb(QAbstractSpinBox, editingFinished, _w, _f)
#define set_dialog_cancel_cb(_w, _f) set_qt_cb(QDialog, rejected, _w, _f)
#define set_file_dialog_cb(_w, _f, _v) set_qt_cb(QFileDialog, fileSelected, \
	                                         _w, _f, const QString &_v)
#define set_popup_cb(_w, _f, _t, _v) \
    set_qt_cb_ov1(QComboBox, currentIndexChanged, _t, _w, _f, _t _v)
#define set_combo_cb(_w, _f) set_qt_cb(QComboBox, currentTextChanged, _w, _f)
// Make the calls needed to pop up a non-modal dialog (use exec for modal)
void popup_nonmodal(QDialog *d);
// Convert QString to char * by allocating enough memory
char *qstrdup(const QString &str);
