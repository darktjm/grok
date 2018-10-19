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
void undraw_canvas_item(
	register ITEM	*item);		/* item to redraw */
void redraw_canvas_item(
	register ITEM	*item);		/* item to redraw */

/*---------------------------------------- cardwin.c ------------*/

void destroy_card_menu(
	register CARD	*card);		/* card to destroy */
CARD *create_card_menu(
	FORM		*form,		/* form that controls layout */
	DBASE		*dbase,		/* database for callbacks, or 0 */
	Widget		wform);		/* form widget to install into, or 0 */
void card_readback_texts(
	CARD		*card,		/* card that is displayed in window */
	int		which);		/* all -f < 0, one item only if >= 0 */
char *format_time_data(
	char		*data,		/* string from database */
	TIMEFMT		timefmt);	/* new format, one of T_* */
void fillout_card(
	CARD		*card,		/* card to draw into menu */
	BOOL		deps);		/* if TRUE, dependencies only */
void fillout_item(
	CARD		*card,		/* card to draw into menu */
	int		i,		/* item index */
	BOOL		deps);		/* if TRUE, dependencies only */

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
void chart_action_callback(
	Widget		widget,		/* drawing area */
	XButtonEvent	*event,		/* X event, contains position */
	String		*args,		/* what happened, up/down/motion */
	int		nargs);		/* # of args, must be 1 */

/*---------------------------------------- convert.c ------------*/

char *mkdatestring(
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
	char		*data);		/* string to store */
void dbase_sort(
	CARD		*card,		/* database and form to sort */
	int		col,		/* column to sort by */
	int		rev);		/* reverse if nonzero */

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
	char		*title,		/* menu title string */
	char		**initial,	/* initial default text */
	BOOL		readonly,	/* not modifiable if TRUE */
	char		*helptag);	/* help tag */
void edit_file(
	char		*name,		/* file name to read */
	BOOL		readonly,	/* not modifiable if TRUE */
	BOOL		create,		/* create if nonexistent if TRUE */
	char		*title,		/* if nonzero, window title */
	char		*helptag);	/* help tag */

/*---------------------------------------- eval.c ------------*/

char *evaluate(
	CARD		*card,
	char		*exp);
BOOL evalbool(
	CARD		*card,
	char		*exp);
void f_foreach(
	char		*cond,
	char		*expr);
int yylex(void);

/*---------------------------------------- evalfunc.c ------------*/

void init_variables(void);
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
	Widget		shell);		/* error popup parent */
void form_edit_script(
	FORM		*form,		/* form to edit */
	Widget		shell,		/* error popup parent */
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
	BOOL		new);		/* ok to change form name */
void sensitize_formedit(void);
void fillout_formedit(void);
void fillout_formedit_widget_by_code(
	int		code);
void readback_formedit(void);

/*---------------------------------------- help.c ------------*/

void destroy_help_popup(void);
void help_callback(
	Widget		parent,
	char		*topic);

/*---------------------------------------- main.c ------------*/

void get_rsrc(
	void		*ret,
	char		*res_name,
	char		*res_class_name,
	char		*res_type);
void set_color(
	int		col);

/*---------------------------------------- mainwin.c ------------*/

void create_mainwindow(
	Widget		toplvl);
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

/*---------------------------------------- popup.c ------------*/

void create_about_popup(void);
void create_error_popup(Widget widget, int error, char *fmt, ...);
void create_query_popup(
	Widget		widget,		/* window that caused this */
	void		(*callback)(),	/* OK callback */
	char		*help,		/* help text tag for popup */
	char		*fmt, ...);	/* message */
void create_dbase_info_popup(
	CARD		*card);

/*---------------------------------------- prefwin.c ------------*/

void destroy_preference_popup(void);
void create_preference_popup(void);
void write_preferences(void);
void read_preferences(void);

/*---------------------------------------- print.c ------------*/

void print(void);

/*---------------------------------------- template.c ------------*/

int get_template_nbuiltins(void);
char *get_template_path(
	char		*name,		/* template name or 0 */
	int		seq,		/* if name==0, sequential number */
	CARD		*card);		/* need this for form name */
void list_templates(
	void	(*func)(int, char *),	/* callback with seq# and name */
	CARD		*card);		/* need this for form name */
char *exec_template(
	char		*oname,		/* output file name, 0=stdout */
	char		*name,		/* template name to execute */
	int		seq,		/* if name is 0, execute by seq num */
	CARD		*card);		/* need this for form name */
char *copy_template(
	Widget		shell,		/* export window widget */
	char		*tar,		/* target template name */
	int		seq,		/* source template number */
	CARD		*card);		/* need this for form name */
BOOL delete_template(
	Widget		shell,		/* export window widget */
	int		seq,		/* template to delete, >= NBUILTINS */
	CARD		*card);		/* need this for form name */
char *eval_template(
	char		*iname,		/* template filename */
	char		*oname);	/* default output filename, 0=stdout */
char *substitute_setup(
	char		**array,	/* where to store substitutions */
	char		*instr);	/* x=y x=y ... command string */

/*---------------------------------------- templwin.c ------------*/

void destroy_templ_popup(void);
void create_templ_popup(void);

/*---------------------------------------- templmk.c ------------*/

char *mktemplate_html(
	char		*oname,		/* default output filename, 0=stdout */
	int		mode);		/* 0=both, 1=summary, 2=data list */

/*---------------------------------------- printwin.c ------------*/

void destroy_print_popup(void);
void create_print_popup(void);

/*---------------------------------------- query.c ------------*/

BOOL match_card(
	CARD		*card,		/* database and form */
	char		*string);	/* query string */
void query_any(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	char		*string);	/* query string */
void query_none(
	CARD		*card);		/* database and form */
void query_all(
	CARD		*card);		/* database and form */
void query_search(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	char		*string);	/* string to search for */
void query_letter(
	CARD		*card,		/* database and form */
	int		letter);	/* 0=0..9, 1..26=a..z, 27=all */
void query_eval(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,		/* database and form */
	char		*expr);		/* expression to apply to dbase */

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
	CARD		*card,		/* card with query results */
	Widget		wform,		/* form widget to install into */
	Widget		shell);		/* enclosing shell */
void make_summary_line(
	char		*buf,		/* text buffer for result line */
	CARD		*card,		/* card with query results */
	int		row);		/* database row */
void make_plan_line(
	CARD		*card,		/* card with query results */
	int		row);		/* database row */
void scroll_summary(
	CARD		*card);		/* which card's summary */
void replace_summary_line(
	CARD		*card,		/* card with query results */
	int		row);		/* database row that has changed */

/*---------------------------------------- util.c ------------*/

char *section_name(
	register DBASE	*dbase,		/* contains section array */
	int		n);		/* 0 .. dbase->nsects-1 */
char *resolve_tilde(
	char		*path,		/* path with ~ */
	char		*ext);		/* append extension unless 0 */
BOOL find_file(
	char		*buf,		/* buffer for returned path */
	char		*name,		/* file name to locate */
	BOOL		exec);		/* must be executable? */
void fatal(char *fmt, ...);
char *mystrdup(
	register char	*s);
void mybzero(
	void		*p,
	register int	n);
int mystrcasecmp(
	register char	*a,
	register char	*b);
void print_button(Widget w, char *fmt, ...);
void print_text_button(Widget w, char *fmt, ...);
void print_text_button_s(Widget w, char *str);
char *read_text_button_noskipblank(Widget, char **);
char *read_text_button(Widget, char **);
char *read_text_button_noblanks(Widget, char **);
void set_toggle(
	Widget		w,
	BOOL		set);
void set_icon(
	Widget		shell,
	int		sub);		/* 0=main, 1=submenu */
void set_cursor(
	Widget		w,		/* in which widget */
	int		n);		/* which cursor, one of XC_* */
void truncate_string(
	register char	*string,	/* string to truncate */
	register int	len,		/* max len in pixels */
	int		sfont);		/* font of string */
int strlen_in_pixels(
	register char	*string,	/* string to truncate */
	int		sfont);		/* font of string */
char *to_octal(
	int		n);		/* ascii to convert to string */
char to_ascii(
	char		*str,		/* string to convert to ascii */
	int		def);		/* default if string is empty */
