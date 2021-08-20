/* I want parser-related stuff to go into parser-generated header */
/* This requires use of bison-specific %code directives */
%code top {
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <time.h>
#include <math.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define TAB	8		/* tab stops every 8 characters */

}

/* Yet more bison-specific code */
/* This makes yyparse more reentrant */
%code requires {
struct fitem { CARD *card; ITEM *item; };
typedef std::vector<fitem> fitem_stack;
typedef struct {
	char	*ret;		/* returned string */
	CARD	*card;		/* database for which to evaluate */
	char	*switch_name;	/* if switch statement was found, */
	char	*switch_expr;	/* .. name to switch to and expr */
	/* FIXME: either remove assigned or find a use for it */
	bool	assigned;	/* did a field assignment */
	char	*expr;		/* first char read by lexer */
	char	*text;		/* next char to be read by lexer */
	fitem_stack fitem;		/* current fkey context */
	char	errormsg[1000];	/* error message if any, or "" */
	char	*errormsg_full;	/* full message if too big for above */
} PARSE_GLOBALS;
#define PG PARSE_GLOBALS *g
#define eval_error (!!g->errormsg[0])
void parsererror(
	PG,
	const char	*msg, ...);
}
%param {PARSE_GLOBALS *g}
%define api.pure full

/* This has to come after YYSTYPE is declared */
%code provides {
int parserlex(
	YYSTYPE *lvalp,
	PG);
}

%code {
static char *yyzstrdup(PG, const char *s)
{
	char *ret = BLANK(s) ? NULL : strdup(s);
	if(!BLANK(s) && !ret)
		parsererror(g, "No memory");
	return ret;
}
#define check_error do { \
	if(eval_error) \
		YYERROR; \
} while(0)
#define yystrdup(d, s) do { \
	d = yyzstrdup(g, s); \
	check_error; \
} while(0)

static int	f_len	(char  *s)  { return(s ? strlen(s) : 0); }
static char    *f_str	(PG, double d)  { char buf[100]; sprintf(buf,"%.*lg",DBL_DIG + 1, d);
					return(yyzstrdup(g, buf)); }
static char    *f_str2	(PG, double d1, double d2)  { char buf[100]; sprintf(buf,"%.*lg %.*lg",DBL_DIG + 1, d1, DBL_DIG + 1, d2);
					return(yyzstrdup(g, buf)); }
static int	f_cmp	(char  *s,
			 char  *t)  { int r = strcmp(STR(s), STR(t));
				      zfree(s); zfree(t); return r;}

static EVAR *var_ptr(CARD *card, int v) {
    /* variables 0..25 are a..z, cleared when */
    /* switching databases (i.e., card-local). */
    /* 26..51 are A..Z, which are never cleared after init */
    /* FIXME: to be reentrant, this needs to lock global_vars or card */
    static EVAR global_vars[26] = {};
    if(v < 26) {
	static EVAR dummy = {};
	if(!card || !card->dbase) /* should never happen */
	    return &dummy;
	return &card->dbase->var[v];
    }
    return &global_vars[v-26];
}
#define VP EVAR *vp = var_ptr(g->card, v)
static char    *getsvar	(PG, int    v)  { VP; return vp->numeric ? f_str(g, vp->value) :
						yyzstrdup(g, vp->string); }
static double   getnvar	(PG, int    v)  { VP; return(vp->numeric ? vp->value :
					vp->string?atof(vp->string):0);}
static void	setsvari(PG, int	v,
			 char  *s)  { VP; zfree(vp->string);
					vp->numeric = false;
					vp->string = s; }
static char    *setsvar	(PG, int    v,
			 char  *s)  {   VP; setsvari(g, v, s);
					return(yyzstrdup(g, vp->string)); }
static double   setnvar	(PG, int    v,
			 double d)  {   VP; setsvari(g, v, 0);
					vp->numeric = true;
					return(vp->value = d); }
void set_var(CARD *card, int v, char *s)
{
    EVAR *vp = var_ptr(card, v);
    zfree(vp->string);
    vp->string = s;
    vp->numeric = false;
}

static char *f_concat(PG, char *a, char *b)
{
	int alen = f_len(a);
	int len = alen + f_len(b);
	char *r;
	if(!len) {
		zfree(a);
		zfree(b);
		return NULL;
	}
	r = (char *)malloc(len + 1);
	if(!r) {
		zfree(a);
		zfree(b);
		parsererror(g, "No memory");
		return NULL;
	}
	if (a)
		strcpy(r, a);
	zfree(a);
	if (b)
		strcpy(r + alen, b);
	zfree(b);
	return r;
}

static int row_from_id(char *id, const DBASE *db)
{
	double ct, ctx;
	int ret;

	if(sscanf(STR(id), "%lg %lg", &ct, &ctx) == 2)
		ret = row_with_ctime(db, ct, ctx);
	else
		ret = -1;
	zfree(id);
	return ret;
}

}

%code requires { struct fkey_field { CARD *card; int column; int row; }; }

%code requires {
/*---------------------------------------- eval.c ------------*/

const char *subeval(
	PG,
	const char	*exp);
bool subevalbool(
	PG,
	const char	*exp);
void f_foreach(
	PG,
	char		*cond,
	char		*expr);

/*---------------------------------------- evalfunc.c ------------*/

double f_num(
	char		*s);
double f_sum(				/* sum */
	PG,
	int		field);	/* number of column to average */
double f_avg(				/* average */
	PG,
	int		field);	/* number of column to average */
double f_dev(				/* standard deviation */
	PG,
	int		field);	/* number of column to average */
double f_min(				/* minimum */
	PG,
	int		field);	/* number of column to average */
double f_max(				/* maximum */
	PG,
	int		field);	/* number of column to average */
double f_qsum(				/* sum */
	PG,
	int		field);	/* number of column to average */
double f_qavg(				/* average */
	PG,
	int		field);	/* number of column to average */
double f_qdev(				/* standard deviation */
	PG,
	int		field);	/* number of column to average */
double f_qmin(				/* minimum */
	PG,
	int		field);	/* number of column to average */
double f_qmax(				/* maximum */
	PG,
	int		field);	/* number of column to average */
double f_ssum(				/* sum */
	PG,
	int		field);	/* number of column to average */
double f_savg(				/* average */
	PG,
	int		field);	/* number of column to average */
double f_sdev(				/* standard deviation */
	PG,
	int		field);	/* number of column to average */
double f_smin(				/* minimum */
	PG,
	int		field);	/* number of column to average */
double f_smax(				/* maximum */
	PG,
	int		field);	/* number of column to average */
char *f_field(
	PG,
	fkey_field	field);
char *f_expand(
	PG,
	fkey_field	field);
char *f_assign(
	PG,
	fkey_field	field,
	char		*data);
int f_section(
	PG,
	int		nrow);
char *f_system(
	PG,
	char		*cmd);
char *f_tr(
	PG,
	char		*string,
	char		*rules);
char *f_substr(
	char		*string,
	int		pos,
	int		num);
char *f_trunc2d(
	char *s,
	int w,
	int h,
	char *lsep);
bool f_instr(
	char		*match,
	char		*string);
struct arg *f_addarg(
	PG,
	struct arg	*list,		/* easier to keep struct arg local */
	char		*value);	/* argument to append to list */
void free_args(
	struct arg	*list);		/* argument list to free */
char *f_printf(
	PG,
	struct arg	*arg);
int f_re_match(
	PG,
	char		*s,		/* string to search */
	char		*e);		/* regular expression pattern */
char *f_re_sub(
	PG,
	char		*s,		/* string to search/replace on */
	char		*e,		/* regular expression pattern */
	char		*r,		/* replacement string */
	bool		all);		/* if false, only replace once */
char *f_esc(
	PG,
	char		*s,
	char		*e);
int f_alen(
	PG,
	char		*array);
char *f_elt(
	PG,
	char		*array,
	int		n);
char *f_slice(
	PG,
	char		*array,
	int		start,
	int		end);
char *f_astrip(
	PG,
	char		*array);
char *f_setelt(
	PG,
	char		*array,
	int		n,
	char		*val);
void f_foreachelt(
	PG,
	int		var,
	char		*array,
	char		*cond,
	char		*expr,
	bool		nonblank);
char *f_toset(
	PG,
	char		*a);
char *f_union(
	PG,
	char		*a,
	char		*b);
char *f_intersect(
	PG,
	char		*a,
	char		*b);
char *f_setdiff(
	PG,
	char		*a,
	char		*b);
char *f_detab(
	PG,
	char		*s,
	int		start,
	int		tabstop);
char *f_align(
	PG,
	char		*s,
	char		*pad,
	int		len,
	int		where);
CARD *f_db_start(
	PG,
	char		*formname,
	char		*search,
	struct db_sort	*sort);
void f_db_end(
	PG,
	CARD		*card);
struct db_sort *new_db_sort(
	PG,
	struct db_sort	*prev,
	int		dir,
	char		*field);
void free_db_sort(
	struct db_sort	*sort);
char *f_deref(
	PG,
	fkey_field field,
	char *fsep,
	char *rsep);
#define nzs(s) ((s) ? (s) : strdup(""))
char *f_dereff(
	PG,
	fkey_field field);
bool f_referenced(
	PG,
	int row);
}

/*
 * variable argument list element
 */

%code requires { struct arg { struct arg *next; char *value; }; }

/* database sort order specifier linked list */
%code requires { struct db_sort { struct db_sort *next; int dir; char *field; }; }

%union {
	int ival;
	double dval;
	char *sval;
	struct arg *aval;
	CARD *cval;
	struct fkey_field fval;
	struct db_sort *Sval;
}
/* These destructors should wipe out any memory leaks on error, but they
 * only work in bison. */
%destructor { zfree($$); } <sval>
%destructor { free_args($$); } <aval>
%destructor { free_db_sort($$); } <Sval>
%destructor { f_db_end(g, $$); } <cval>
%destructor { free_fkey_card($$.card); } <fval>

%type	<dval>	number numarg timestamp card
%type	<sval>	string
%type	<aval>	args
%type	<cval>	db_prefix
%type	<Sval>	db_sort
%type	<fval>	field
%token	<dval>	NUMBER
%token	<sval>	STRING SYMBOL
%token	<ival>	VAR
%token	<fval>	FIELD
%token		EQ NEQ LE GE SHR SHL AND OR IN UNION INTERSECT DIFF REQ RNEQ
%token		PLA MIA MUA MOA DVA ANA ORA INC DEC_ APP AAS ALEN_ DOTDOT
%token		LBRLBR RBRBR
%token		AVG DEV AMIN AMAX SUM
%token		QAVG QDEV QMIN QMAX QSUM
%token		SAVG SDEV SMIN SMAX SSUM
%token		ABS INT BOUND LEN CHOP TR SUBSTR SQRT EXP LOG LN POW RANDOM
%token		SIN COS TAN ASIN ACOS ATAN ATAN2
%token		DATE TIME DURATION EXPAND
%token		YEAR MONTH DAY HOUR MINUTE SECOND LEAP JULIAN
%token		SECTION_ DBASE_ FORM_ PREVFORM SWITCH THIS LAST DISP FOREACH
%token		HOST USER UID GID SYSTEM ACCESS BEEP ERROR PRINTF MATCH SUB
%token		GSUB BSUB ESC TOSET DETAB ALIGN DEREF DEREFF ID MTIME CTIME
%token		TRUNC2D COUNT REFERENCED

%left 's' /* Force a shift; i.e., prefer longer versions */
%left ',' ';'
%nonassoc DOTDOT
%right '?' ':'
%right PLA MIA MUA MOA DVA ANA ORA APP '=' AAS
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left IN
%left EQ NEQ REQ RNEQ
%left '<' '>' LE GE
%left SHL SHR
%left '-' '+' UNION DIFF INTERSECT
%left '*' '/' '%'
%nonassoc '!' '~' UMINUS '#' ALEN_
%left '.'
%left '[' LBRBR

%start stmt

%%
stmt	: string			{ zfree(g->ret); g->ret = $1; }
	;

string	: STRING			{ $$ = $1; }
	| '{' string '}'		{ $$ = $2; }
	| string ';' string		{ $$ = $3; zfree($1); }
	| string '.' string		{ $$ = f_concat(g, $1, $3); check_error; }
	| VAR %prec 's'			{ $$ = getsvar(g, $1); check_error; }
	| VAR APP string		{ int v=$1;
					  char *s=getsvar(g, v);
					  if(eval_error) { zfree(s); zfree($3); $$ = NULL; YYERROR; }
					  $$ = setsvar(g, v, f_concat(g, s, $3));
					  check_error; }
	| VAR '=' string		{ $$ = setsvar(g, $1, $3); check_error; }
	| '(' number ')'		{ $$ = f_str(g, $2); check_error; }
	| string '?' string ':' string	{ bool c = f_num($1); $$ = c ? $3 : $5;
					  if (c) zfree($5); else zfree($3);}
	| string '<' string		{ $$ = f_str(g, (double)
							(f_cmp($1, $3) <  0)); check_error;}
	| string '>' string		{ $$ = f_str(g, (double)
							(f_cmp($1, $3) >  0)); check_error;}
	| string EQ  string		{ $$ = f_str(g, (double)
							(f_cmp($1, $3) == 0)); check_error;}
	| string NEQ string		{ $$ = f_str(g, (double)
							(f_cmp($1, $3) != 0)); check_error;}
	| string REQ  string		{ $$ = f_str(g, (double)
							(f_re_match(g, $1, $3) > 0)); check_error;}
	| string RNEQ string		{ $$ = f_str(g, (double)
							(f_re_match(g, $1, $3) == 0)); check_error;}
	| string LE  string		{ $$ = f_str(g, (double)
							(f_cmp($1, $3) <= 0)); check_error;}
	| string GE  string		{ $$ = f_str(g, (double)
							(f_cmp($1, $3) >= 0)); check_error;}
	| string IN  string		{ $$ = f_str(g, (double)f_instr($1, $3)); check_error;}
	| ESC '(' string ',' string ')'	{ if($5) $$ = f_esc(g, $3, $5); else $$ = $3; check_error; }
	| ESC '(' string ')'		{ $$ = f_esc(g, $3, NULL); check_error; }
	| string '[' DOTDOT number ']'	{ $$ = f_slice(g, $1, 0, $4); } /* can't fail */
	| string '[' number DOTDOT number ']'	{ $$ = f_slice(g, $1, $3, $5); }
	| string '[' number DOTDOT ']'	{ $$ = f_slice(g, $1, $3, -1); }
	| string '[' DOTDOT ']'		{ $$ = f_astrip(g, $1); }
	| string '[' number ']' 		{ $$ = f_elt(g, $1, $3); } /* can't fail */
	| string '[' number ']' AAS string  { $$ = f_setelt(g, $1, $3, $6); check_error; }
	| string UNION  string		{ $$ = f_union(g, $1, $3); check_error; }
	| string INTERSECT  string	{ $$ = f_intersect(g, $1, $3);} /* can't fail */
	| string DIFF  string		{ $$ = f_setdiff(g, $1, $3);} /* can't fail */
	| TOSET '(' string ')'		{ $$ = f_toset(g, $3); check_error; }
	| DETAB '(' string ')'		{ $$ = f_detab(g, $3, 0, TAB); check_error; }
	| DETAB '(' string ',' numarg ')'	{ $$ = f_detab(g, $3, $5, TAB); check_error; }
	| DETAB '(' string ',' numarg ',' number ')'	{ $$ = f_detab(g, $3, $5, $7); check_error; }
	| ALIGN '(' string ',' numarg ')'		{ $$ = f_align(g, $3, NULL, $5, -1); check_error; }
	| ALIGN '(' string ',' numarg ',' string ')'	{ $$ = f_align(g, $3, $7, $5, -1); check_error; }
	| ALIGN '(' string ',' numarg ',' string ',' number ')'	{ $$ = f_align(g, $3, $7, $5, $9); check_error; }
	| field				{ $$ = f_field(g, $1); check_error; }
	| field '=' string		{ $$ = f_assign(g, $1, $3);
					  g->assigned = 1; } /* if this fails, it's fatal */
	| SYSTEM '(' string ')'		{ $$ = f_system(g, $3); check_error; }
	| '$' SYMBOL			{ yystrdup($$, getenv($2)); }
	| CHOP '(' string ')'		{ char *s=$3; if (s) { int n=strlen(s);
					  if (n && s[n-1]=='\n') s[n-1] = 0; }
					  $$ = s; }
	| TR '(' string ',' string ')'	{ $$ = f_tr(g, $3, $5); check_error; }
	| SUBSTR '(' string ',' numarg ',' number ')'
					{ $$ = f_substr($3, (int)$5, (int)$7);} /* can't fail */
	| TRUNC2D '(' string ',' numarg ',' numarg ')'
					{ $$ = f_trunc2d($3, (int)$5, (int)$7, NULL); }
	| TRUNC2D '(' string ',' numarg ',' numarg ',' string ')'
					{ $$ = f_trunc2d($3, (int)$5, (int)$7, $9); }
	| HOST				{ char *s; size_t len = 20; int e;
					  s = (char *)malloc(len);
					  while(s && !(e = gethostname(s, len)) &&
						errno == ENAMETOOLONG) {
						  len *= 2;
						  free(s);
						  s = (char *)malloc(len);
					  }
					  if(!s || !e) {
						  parsererror(g, "Error getting host name");
						  zfree(s);
						  s = 0;
					  }
					  $$ = s; check_error; }
	| USER				{ yystrdup($$, getenv("USER")); }
	| PREVFORM			{ yystrdup($$, g->card->prev_form); }
	/* sections are deprecated, so don't support fkey prefix */
	/* @db can always be used to get it anyway */
	| SECTION_ card			{ if(!g->card || !g->card->dbase)
						$$ = 0;
					  else
						yystrdup($$, section_name(
						    g->card->dbase,
						    f_section(g, $2)));}
	| FORM_				{ if(!g->card || !g->card->form
						     || !g->card->form->name)
						$$ = 0;
					  else
						yystrdup($$, resolve_tilde
						(g->card->form->name, "gf"));}
	| DBASE_			{ if(!g->card || !g->card->form
						     || !g->card->form->dbase)
						$$ = 0;
					  else
						yystrdup($$, resolve_tilde
							(g->card->form->dbase,
							 g->card->form->proc ?
								0 : "db"));}
	| ID %prec 's'			{ if(!g->card || !g->card->dbase || g->card->row < 0)
						$$ = 0;
					  else {
						ROW *r = g->card->dbase->row[g->card->row];
						$$ = f_str2(g, r->ctime, r->ctimex); }}
	| field ID			{ if(!$1.card || !$1.card->dbase || $1.row < 0)
						$$ = 0;
					  else {
						ROW *r = $1.card->dbase->row[$1.row];
						$$ = f_str2(g, r->ctime, r->ctimex); }}
	| ID '[' number ']'		{ if(!g->card || !g->card->dbase || $3 < 0 || $3 >= g->card->dbase->nrows)
						$$ = 0;
					  else {
						ROW *r = g->card->dbase->row[(int)$3];
						$$ = f_str2(g, r->ctime, r->ctimex); }}
	| timestamp card		{ if(!g->card || !g->card->dbase || $2 < 0)
						$$ = 0;
					  else {
						ROW *r = g->card->dbase->row[(int)$2];
						yystrdup($$, mkdatetimestring($1 ? r->ctime : r->mtime)); }}
	| field timestamp		{ if(!$1.card || !$1.card->dbase || $1.row < 0)
						$$ = 0;
					  else {
						ROW *r = $1.card->dbase->row[$1.row];
						yystrdup($$, mkdatetimestring($2 ? r->ctime : r->mtime));}}
	| SWITCH '(' string ',' string ')'
					{ char *name = $3, *expr = $5;
					  zfree(g->switch_name);
					  zfree(g->switch_expr);
					  g->switch_name = name;
					  g->switch_expr = expr; 
					  $$ = 0; }
	| db_prefix string %prec DOTDOT	{ $$ = $2; f_db_end(g, $1); }
	| FOREACH '(' string ')'	{ f_foreach(g, 0, $3); $$ = 0; check_error; }
	| FOREACH '(' string ',' string ')'
					{ f_foreach(g, $3, $5); $$ = 0; check_error; }
	| FOREACH '(' VAR ',' string ')'
					{ f_foreach(g, getsvar(g, $3), $5); $$ = 0; check_error; }
	| FOREACH '(' VAR ',' string ',' string ')'
					{ f_foreachelt(g, $3, $5, 0, $7, 0); $$ = 0; check_error; }
	| FOREACH '(' VAR ',' string ',' string ',' string ')'
					{ f_foreachelt(g, $3, $5, $7, $9, 0); $$ = 0; check_error; }
	| FOREACH '(' VAR ',' '+' string ',' string ')'
					{ f_foreachelt(g, $3, $6, 0, $8, 1); $$ = 0; check_error; }
	| FOREACH '(' VAR ',' '+' string ',' string ',' string ')'
					{ f_foreachelt(g, $3, $6, $8, $10, 1); $$ = 0; check_error; }
	| TIME				{ yystrdup($$, mktimestring
						(time(0), false)); }
	| DATE				{ yystrdup($$, mkdatestring
						(time(0))); }
	| TIME '(' number ')'		{ yystrdup($$, mktimestring
						((time_t)$3, false)); }
	| DATE '(' number ')'		{ yystrdup($$, mkdatestring
						((time_t)$3)); }
	| DURATION '(' number ')'	{ yystrdup($$, mktimestring
						((time_t)$3, true)); }
	| EXPAND '(' field ')'		{ $$ = f_expand(g, $3); check_error; }
	| PRINTF '(' args ')'		{ $$ = f_printf(g, $3); check_error; }
	| MATCH '(' string ',' string ')' { $$ = f_str(g, f_re_match(g, $3, $5)); check_error; }
	| SUB '(' string ',' string ',' string ')' { $$ = f_re_sub(g, $3, $5, $7, false); check_error; }
	| GSUB '(' string ',' string ',' string ')' { $$ = f_re_sub(g, $3, $5, $7, true); check_error; }
	| BSUB '(' string ')'		{ $$ = $3; if($3) backslash_subst($3); }
	| BEEP				{ app->beep(); $$ = 0; }
	| ERROR '(' args ')'		{ char *s = f_printf(g, $3); check_error;
					  create_error_popup(mainwindow, 0, s);
					  zfree(s); $$ = 0; }
	| DEREF '(' field ')'		{ $$ = f_deref(g, $3, 0, 0); check_error; }
	| DEREF '(' field ',' ')'	{ $$ = f_deref(g, $3, 0, 0); check_error; }
	| DEREF '(' field ',' string ')'	{ $$ = f_deref(g, $3, nzs($5), 0); check_error; }
	| DEREF '(' field ',' ',' string ')'	{ $$ = f_deref(g, $3, 0, nzs($6)); check_error; }
	| DEREF '(' field ',' string ',' string')'		{ $$ = f_deref(g, $3, nzs($5), nzs($7)); check_error; }
	| DEREFF '(' field ')'		{ $$ = f_dereff(g, $3); check_error; }
	;

timestamp : MTIME {$$ = 0;} | CTIME {$$ = 1;} ;

card	: %prec 's'			{ $$ = g->card ? g->card->row : -1; }
	| '[' number ']'		{ $$ = $2; }
	| LBRBR string RBRBR		{ $$ = row_from_id($2, g->card->dbase); }
	;

field	: FIELD card			{ $$ = $1; $$.row = $2; }
	| field FIELD %prec 's'		{ $1.card = 0; $$ = $2; }
	/* better to use @dp: to access any row in other db */
/*	| field FIELD '[' number ']'	{ $$ = $2; $$.row = $4; } */
	;

args	: string			{ $$ = f_addarg(g, 0, $1); check_error; }
	| args ',' string		{ $$ = f_addarg(g, $1, $3); check_error; }
	;

db_prefix
	: '@' string ':'		{ $$ = f_db_start(g, $2, NULL, NULL); check_error; }
	| '@' string db_sort ':'	{ $$ = f_db_start(g, $2, NULL, $3); check_error; }
	| '@' string '/' string ':'	{ $$ = f_db_start(g, $2, $4, NULL); check_error; }
	| '@' string '/' string db_sort ':'	{ $$ = f_db_start(g, $2, $4, $5); check_error; }
	;

db_sort	: '+' string			{ $$ = new_db_sort(g, 0, 1, $2); check_error; }
	| '-' string			{ $$ = new_db_sort(g, 0, -1, $2); check_error; }
	| db_sort '+' string		{ $$ = new_db_sort(g, $1, 1, $3); check_error; }
	| db_sort '-' string		{ $$ = new_db_sort(g, $1, -1, $3); check_error; }
	;

number	: numarg			{ $$ = $1; }
        | number ',' numarg		{ $$ = $3; }
	;

numarg	: NUMBER			{ $$ = $1; }
	| '{' string '}'		{ $$ = f_num($2); }
	| '(' number ')'		{ $$ = $2; }
	| field				{ $$ = f_num(f_field(g, $1));}
	| field '=' numarg		{ zfree(f_assign(g, $1,
					  f_str(g, $$ = $3))); g->assigned = 1; check_error; }
	| VAR				{ $$ = getnvar(g, $1); }
	| VAR '=' numarg		{ $$ = setnvar(g, $1, $3); }
	| VAR PLA numarg		{ int v = $1;
					  $$ = setnvar(g, v, getnvar(g, v) + $3); }
	| VAR MIA numarg		{ int v = $1;
					  $$ = setnvar(g, v, getnvar(g, v) - $3); }
	| VAR MUA numarg		{ int v = $1;
					  $$ = setnvar(g, v, getnvar(g, v) * $3); }
	| VAR DVA numarg		{ int v = $1; double d=$3; if(d==0)d=1; /* cheater */
					  $$ = setnvar(g, v, getnvar(g, v) / d); }
	| VAR MOA numarg		{ int v = $1; double d=$3; if(d==0)d=1; /* cheater */
					  $$ = setnvar(g, v, (double)((int)
							  getnvar(g, v)%(int)d));}
	| VAR ANA numarg		{ int v = $1;
					  $$ = setnvar(g, v, (double)((int)$3 &
							  (int)getnvar(g, v))); }
	| VAR ORA numarg		{ int v = $1;
					  $$ = setnvar(g, v, (double)((int)$3 |
							  (int)getnvar(g, v))); }
	| VAR INC			{ int v = $1;
					  $$ = setnvar(g, v, getnvar(g, v) + 1) - 1;}
	| VAR DEC_			{ int v = $1;
					  $$ = setnvar(g, v, getnvar(g, v) - 1) + 1;}
	| INC VAR			{ int v = $2;
					  $$ = setnvar(g, v, getnvar(g, v) + 1); }
	| DEC_ VAR			{ int v = $2;
					  $$ = setnvar(g, v, getnvar(g, v) - 1); }
	| '-' numarg %prec UMINUS	{ $$ = - $2; }
	| '!' numarg			{ $$ = ! $2; }
	| '~' numarg			{ $$ = ~ (int)$2; }
	| numarg '&' numarg		{ $$ = (int)$1 &  (int)$3; }
	| numarg '^' numarg		{ $$ = (int)$1 ^  (int)$3; }
	| numarg '|' numarg		{ $$ = (int)$1 |  (int)$3; }
	| numarg SHL numarg		{ $$ = (int)$1 << (int)$3; }
	| numarg SHR numarg		{ $$ = (int)$1 >> (int)$3; }
	| numarg '%' numarg		{ long i=$3; if (i==0) i=1; /* cheater */
					  $$ = (long)$1 % i; }
	| numarg '+' numarg		{ $$ = $1 +  $3; }
	| numarg '-' numarg		{ $$ = $1 -  $3; }
	| numarg '*' numarg		{ $$ = $1 *  $3; }
	| numarg '/' numarg		{ double d=$3; if (d==0) d=1; /* cheater */
					  $$ = $1 /  d; }
	| numarg '<' numarg		{ $$ = $1 <  $3; }
	| numarg '>' numarg		{ $$ = $1 >  $3; }
	| numarg EQ  numarg		{ $$ = $1 == $3; }
	| numarg NEQ numarg		{ $$ = $1 != $3; }
	| numarg LE  numarg		{ $$ = $1 <= $3; }
	| numarg GE  numarg		{ $$ = $1 >= $3; }
	| numarg AND numarg		{ $$ = $1 && $3; }
	| numarg OR  numarg		{ $$ = $1 || $3; }
	| numarg '?' number ':' numarg	{ $$ = $1 ?  $3 : $5; }
	| THIS				{ $$ = g->card && g->card->row > 0 ?
					       g->card->row : 0; }
	| LAST				{ $$ = g->card && g->card->dbase ?
					       g->card->dbase->nrows - 1 : -1; }
	| field THIS			{ $$ = $1.row; }
	| field LAST			{ DBASE *db = $1.card->dbase; $$ = db ?
						db->nrows - 1 : -1; }
	| THIS LBRBR string RBRBR	{ $$ = row_from_id($3, g->card->dbase); }
	| timestamp card		{ if(!g->card || !g->card->dbase || g->card->row < 0)
						$$ = 0;
					  else {
						ROW *r = g->card->dbase->row[(int)$2];
						$$ = $1 ? r->ctime : r->mtime; }}
	| field timestamp			{ if(!$1.card || !$1.card->dbase || $1.row < 0)
						$$ = 0;
					  else {
						ROW *r = $1.card->dbase->row[$1.row];
						$$ = $2 ? r->ctime : r->mtime;}}
	| DISP				{ $$ = g->card && g->card->dbase
						      && g->card->disprow >= 0
						      && g->card->disprow <
							 g->card->dbase->nrows ?
					       g->card->disprow : -1; }
	| REFERENCED			{ $$ = f_referenced(g, g->card ?
							    g->card->row : 0); }
	| REFERENCED '(' number ')'	{ $$ = f_referenced(g, $3); }
	| AVG   '(' FIELD ')'		{ $$ = f_avg(g, $3.column); }
	| DEV   '(' FIELD ')'		{ $$ = f_dev(g, $3.column); }
	| AMIN  '(' FIELD ')'		{ $$ = f_min(g, $3.column); }
	| AMAX  '(' FIELD ')'		{ $$ = f_max(g, $3.column); }
	| SUM   '(' FIELD ')'		{ $$ = f_sum(g, $3.column); }
	| QAVG  '(' FIELD ')'		{ $$ = f_qavg(g, $3.column); }
	| QDEV  '(' FIELD ')'		{ $$ = f_qdev(g, $3.column); }
	| QMIN  '(' FIELD ')'		{ $$ = f_qmin(g, $3.column); }
	| QMAX  '(' FIELD ')'		{ $$ = f_qmax(g, $3.column); }
	| QSUM  '(' FIELD ')'		{ $$ = f_qsum(g, $3.column); }
	| SAVG  '(' FIELD ')'		{ $$ = f_savg(g, $3.column); }
	| SDEV  '(' FIELD ')'		{ $$ = f_sdev(g, $3.column); }
	| SMIN  '(' FIELD ')'		{ $$ = f_smin(g, $3.column); }
	| SMAX  '(' FIELD ')'		{ $$ = f_smax(g, $3.column); }
	| SSUM  '(' FIELD ')'		{ $$ = f_ssum(g, $3.column); }
	| ABS   '(' number ')'		{ $$ = abs($3); }
	| INT   '(' number ')'		{ $$ = (long long)($3); }
	| BOUND '(' numarg ',' numarg ',' number ')'
					{ double a=$3, b=$5, c=$7;
					  $$ = a < b ? b : a > c ? c : a; }
	| LEN   '(' string ')'		{ char *a=$3; $$ = a ? f_len(a) : 0;
								zfree(a); }
	| '#' string			{ $$ = $2 ? f_len($2) : 0; zfree($2); }
	| COUNT '(' string ',' string ')' { $$ = countchars($3, STR($5));
					    zfree($3); zfree($5); }
	| ALEN_ string			{ $$ = f_alen(g, $2); }
	| MATCH '(' string ',' string ')' { $$ = f_re_match(g, $3, $5); check_error; }
	| SQRT  '(' number ')'		{ $$ = sqrt(abs($3));  } /* cheater */
	| EXP   '(' number ')'		{ $$ = exp($3); }
	| LOG   '(' number ')'		{ double a=$3; $$ = a<=0 ? 0:log10(a);} /* cheater */
	| LN    '(' number ')'		{ double a=$3; $$ = a<=0 ? 0:log(a); } /* cheater */
	| POW   '(' numarg ',' number ')'
					{ $$ = pow($3, $5); } /* check for nan? */
	| RANDOM			{ $$ = drand48(); }
	| SIN   '(' number ')'		{ $$ = sin($3); }
	| COS   '(' number ')'		{ $$ = cos($3); }
	| TAN   '(' number ')'		{ $$ = tan($3); } /* check for nan? */
	| ASIN  '(' number ')'		{ $$ = asin($3); } /* check for nan? */
	| ACOS  '(' number ')'		{ $$ = acos($3); } /* check for nan? */
	| ATAN  '(' number ')'		{ $$ = atan($3); }
	| ATAN2 '(' numarg ',' number ')'
					{ $$ = atan2($3, $5); }
	| SECTION_ card			{ $$ = f_section(g, $2); }
	| DATE				{ $$ = time(0); }
	| DATE  '(' string ')'		{ $$ = $3 ? parse_datetimestring($3) : 0; zfree($3);}
	| TIME  '(' string ')'		{ $$ = $3 ? parse_timestring($3, false) : 0; zfree($3);}
	| DURATION '(' string ')'	{ $$ = $3 ? parse_timestring($3, true) : 0; zfree($3);}
	| YEAR  '(' number ')'		{ const time_t t = $3;
					  $$ = localtime(&t)->tm_year; }
	| MONTH '(' number ')'		{ const time_t t = $3;
					  $$ = localtime(&t)->tm_mon+1; }
	| DAY   '(' number ')'		{ const time_t t = $3;
					  $$ = localtime(&t)->tm_mday; }
	| HOUR  '(' number ')'		{ const time_t t = $3;
					  $$ = localtime(&t)->tm_hour; }
	| MINUTE '(' number ')'		{ const time_t t = $3;
					  $$ = localtime(&t)->tm_min; }
	| SECOND '(' number ')'		{ const time_t t = $3;
					  $$ = localtime(&t)->tm_sec; }
	| JULIAN '(' number ')'		{ const time_t t = $3;
					  $$ = localtime(&t)->tm_yday; }
	| LEAP   '(' number ')'		{ const time_t t = $3;
					  int y=localtime(&t)->tm_year;
					  $$ = !(y%4) ^ !(y%100) ^ !(y%400); }
	| UID				{ $$ = getuid(); }
	| GID				{ $$ = getgid(); }
	| ACCESS '(' string ',' number ')'
					{ char *a = $3;
					  $$ = a ? access(a, (int)$5) : 0;
					  zfree(a); }
	;
%%
