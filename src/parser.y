%{
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

static struct var {		/* variables 0..25 are a..z, cleared when */
	char	*string;	/* switching databases. 26..51 are A..Z, */
	double	value;		/* which are never cleared. */
	BOOL	numeric;
} var[52];

static char *yyzstrdup(const char *s)
{
	char *ret = BLANK(s) ? NULL : strdup(s);
	if(!BLANK(s) && !ret)
		parsererror("NO memory");
	return ret;
}
#define check_error do { \
	if(eval_error()) \
		YYERROR; \
} while(0)
#define yystrdup(d, s) do { \
	d = yyzstrdup(s); \
	check_error; \
} while(0)

static int	f_len	(char  *s)  { return(s ? strlen(s) : 0); }
static char    *f_str	(double d)  { char buf[100]; sprintf(buf,"%.*lg",DBL_DIG + 1, d);
					return(yyzstrdup(buf)); }
static int	f_cmp	(char  *s,
			 char  *t)  { int r = strcmp(STR(s), STR(t));
				      zfree(s); zfree(t); return r;}

static char    *getsvar	(int    v)  { return var[v].numeric ? f_str(var[v].value) :
						yyzstrdup(var[v].string); }
static double   getnvar	(int    v)  { return(var[v].numeric ? var[v].value :
					var[v].string?atof(var[v].string):0);}
static void	setsvari(int	v,
			 char  *s)  { zfree(var[v].string);
					var[v].numeric = FALSE;
					var[v].string = s; }
static char    *setsvar	(int    v,
			 char  *s)  {   setsvari(v, s);
					return(yyzstrdup(var[v].string)); }
static double   setnvar	(int    v,
			 double d)  {   setsvari(v, 0);
					var[v].numeric = TRUE;
					return(var[v].value = d); }
void set_var(int v, char *s)
{
    zfree(var[v].string);
    var[v].string = s;
    var[v].numeric = FALSE;
}

static char *f_concat(char *a, char *b)
{
	int len = f_len(a) + f_len(b);
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
		parsererror("No memory");
		return NULL;
	}
	if (a)
		strcpy(r, a);
	zfree(a);
	if (b)
		strcat(r, b);
	zfree(b);
	return r;
}

void init_variables(void) { int i; for (i=0; i < 26; i++) setsvari(i, 0); }

%}

%union { int ival; double dval; char *sval; struct arg *aval; }
/* THese destructors should wipe out any memory leaks on error, but they
 * only work in bison. */
/* The only portable way would be to keep track of all allocations, and
 * free them when parsing is complete.  For example, add another link to
 * the "arg" structure (say, "alloc") and link all allocated args this way.
 * Never free args, but instead zero out their string and add to a "free"
 * list of args (linked via next).  There would no longer be a distinction
 * between strings and args in the %union, and all former string uses would
 * have to use the arg's string, and then move the arg to the free list.
 * When parsing is complete, just follow the alloc link and free any
 * non-NULL strings as well as the arg itself. */
%destructor { zfree($$); } <sval>
%destructor { free_args($$); } <aval>
%type	<dval>	number numarg
%type	<sval>	string
%type	<aval>	args
%type	<ival>	plus
%token	<dval>	NUMBER
%token	<sval>	STRING SYMBOL
%token	<ival>	FIELD VAR
%token		EQ NEQ LE GE SHR SHL AND OR IN UNION INTERSECT DIFF REQ RNEQ
%token		PLA MIA MUA MOA DVA ANA ORA INC DEC_ APP AAS ALEN_ DOTDOT
%token		AVG DEV AMIN AMAX SUM
%token		QAVG QDEV QMIN QMAX QSUM
%token		SAVG SDEV SMIN SMAX SSUM
%token		ABS INT BOUND LEN CHOP TR SUBSTR SQRT EXP LOG LN POW RANDOM
%token		SIN COS TAN ASIN ACOS ATAN ATAN2
%token		DATE TIME DURATION EXPAND
%token		YEAR MONTH DAY HOUR MINUTE SECOND LEAP JULIAN
%token		SECTION_ DBASE_ FORM_ PREVFORM SWITCH THIS LAST DISP FOREACH
%token		HOST USER UID GID SYSTEM ACCESS BEEP ERROR PRINTF MATCH SUB
%token		GSUB BSUB ESC TOSET DETAB ALIGN

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
%left '['

%start stmt

%%
stmt	: string			{ yyret = $1; }
	;

string	: STRING			{ $$ = $1; }
	| '{' string '}'		{ $$ = $2; }
	| string ';' string		{ $$ = $3; zfree($1); }
	| string '.' string		{ $$ = f_concat($1, $3); check_error; }
	| VAR %prec 's'			{ $$ = getsvar($1); check_error; }
	| VAR APP string		{ int v=$1;
					  char *s=getsvar(v);
					  if(eval_error()) { zfree(s); zfree($3); $$ = NULL; YYERROR; }
					  $$ = setsvar(v, f_concat(s, $3));
					  check_error; }
	| VAR '=' string		{ $$ = setsvar($1, $3); check_error; }
	| '(' number ')'		{ $$ = f_str($2); check_error; }
	| string '?' string ':' string	{ bool c = f_num($1); $$ = c ? $3 : $5;
					  if (c) zfree($5); else zfree($3);}
	| string '<' string		{ $$ = f_str((double)
							(f_cmp($1, $3) <  0)); check_error;}
	| string '>' string		{ $$ = f_str((double)
							(f_cmp($1, $3) >  0)); check_error;}
	| string EQ  string		{ $$ = f_str((double)
							(f_cmp($1, $3) == 0)); check_error;}
	| string NEQ string		{ $$ = f_str((double)
							(f_cmp($1, $3) != 0)); check_error;}
	| string REQ  string		{ $$ = f_str((double)
							(f_re_match($1, $3) > 0)); check_error;}
	| string RNEQ string		{ $$ = f_str((double)
							(f_re_match($1, $3) == 0)); check_error;}
	| string LE  string		{ $$ = f_str((double)
							(f_cmp($1, $3) <= 0)); check_error;}
	| string GE  string		{ $$ = f_str((double)
							(f_cmp($1, $3) >= 0)); check_error;}
	| string IN  string		{ $$ = f_str((double)f_instr($1, $3)); check_error;}
	| ESC '(' string ',' string ')'	{ if($5) $$ = f_esc($3, $5); else $$ = $3; check_error; }
	| ESC '(' string ')'		{ $$ = f_esc($3, NULL); check_error; }
	| string '[' DOTDOT number ']'	{ $$ = f_slice($1, 0, $4); } /* can't fail */
	| string '[' number DOTDOT number ']'	{ $$ = f_slice($1, $3, $5); }
	| string '[' number DOTDOT ']'	{ $$ = f_slice($1, $3, -1); }
	| string '[' DOTDOT ']'		{ $$ = f_astrip($1); }
	| string '[' number ']' 		{ $$ = f_elt($1, $3); } /* can't fail */
	| string '[' number ']' AAS string  { $$ = f_setelt($1, $3, $6); check_error; }
	| string UNION  string		{ $$ = f_union($1, $3); check_error; }
	| string INTERSECT  string	{ $$ = f_intersect($1, $3);} /* can't fail */
	| string DIFF  string		{ $$ = f_setdiff($1, $3);} /* can't fail */
	| TOSET '(' string ')'		{ $$ = f_toset($3); check_error; }
	| DETAB '(' string ')'		{ $$ = f_detab($3, 0, TAB); check_error; }
	| DETAB '(' string ',' numarg ')'	{ $$ = f_detab($3, $5, TAB); check_error; }
	| DETAB '(' string ',' numarg ',' number ')'	{ $$ = f_detab($3, $5, $7); check_error; }
	| ALIGN '(' string ',' numarg ')'		{ $$ = f_align($3, NULL, $5, -1); check_error; }
	| ALIGN '(' string ',' numarg ',' string ')'	{ $$ = f_align($3, $7, $5, -1); check_error; }
	| ALIGN '(' string ',' numarg ',' string ',' number ')'	{ $$ = f_align($3, $7, $5, $9); check_error; }
	| FIELD %prec 's'			{ $$ = f_field($1, yycard->row); check_error; }
	| FIELD '[' number ']' 		{ $$ = f_field($1, $3); check_error; }
	| FIELD '=' string		{ $$ = f_assign($1, yycard->row, $3);
					  assigned = 1; } /* if this fails, it's fatal */
	| FIELD '[' number ']' '=' string 
					{ $$ = f_assign($1, $3, $6);
					  assigned = 1; } /* if this fails, it's fatal */
	| SYSTEM '(' string ')'		{ $$ = f_system($3); check_error; }
	| '$' SYMBOL			{ yystrdup($$, getenv($2)); }
	| CHOP '(' string ')'		{ char *s=$3; if (s) { int n=strlen(s);
					  if (n && s[n-1]=='\n') s[n-1] = 0; }
					  $$ = s; }
	| TR '(' string ',' string ')'	{ $$ = f_tr($3, $5); check_error; }
	| SUBSTR '(' string ',' numarg ',' number ')'
					{ $$ = f_substr($3, (int)$5, (int)$7);} /* can't fail */
	| HOST				{ char *s; size_t len = 20; int e;
					  s = (char *)malloc(len);
					  while(s && !(e = gethostname(s, len)) &&
						errno == ENAMETOOLONG) {
						  len *= 2;
						  free(s);
						  s = (char *)malloc(len);
					  }
					  if(!s || !e) {
						  yyerror("Error getting host name");
						  zfree(s);
						  s = 0;
					  }
					  $$ = s; check_error; }
	| USER				{ yystrdup($$, getenv("USER")); }
	| PREVFORM			{ yystrdup($$, yycard->prev_form); }
	| SECTION_ %prec 's'		{ if(!yycard || !yycard->dbase)
						$$ = 0;
					  else
						yystrdup($$, section_name(
						    yycard->dbase,
						    yycard->dbase->currsect));}
	| SECTION_ '[' number ']' 	{ if(!yycard || !yycard->dbase)
						$$ = 0;
					  else
						yystrdup($$, section_name(
						    yycard->dbase,
						    f_section($3))); }
	| FORM_				{ if(!yycard || !yycard->form
						     || !yycard->form->name)
						$$ = 0;
					  else
						yystrdup($$, resolve_tilde
						(yycard->form->name, "gf"));}
	| DBASE_			{ if(!yycard || !yycard->form
						     || !yycard->form->dbase)
						$$ = 0;
					  else
						yystrdup($$, resolve_tilde
							(yycard->form->dbase,
							 yycard->form->proc ?
								0 : "db"));}
	| SWITCH '(' string ',' string ')'
					{ char *name = $3, *expr = $5;
					  zfree(switch_name);
					  zfree(switch_expr);
					  switch_name = name;
					  switch_expr = expr; 
					  $$ = 0; }
	| FOREACH '(' string ')'	{ f_foreach(0, $3); $$ = 0; check_error; }
	| FOREACH '(' string ',' string ')'
					{ f_foreach($3, $5); $$ = 0; check_error; }
	| FOREACH '(' VAR ',' plus string ',' string ')'
					{ f_foreachelt($3, $6, 0, $8, $5); $$ = 0; check_error; }
	| FOREACH '(' VAR ',' plus string ',' string ',' string ')'
					{ f_foreachelt($3, $6, $8, $10, $5); $$ = 0; check_error; }
	| TIME				{ yystrdup($$, mktimestring
						(time(0), FALSE)); }
	| DATE				{ yystrdup($$, mkdatestring
						(time(0))); }
	| TIME '(' number ')'		{ yystrdup($$, mktimestring
						((time_t)$3, FALSE)); }
	| DATE '(' number ')'		{ yystrdup($$, mkdatestring
						((time_t)$3)); }
	| DURATION '(' number ')'	{ yystrdup($$, mktimestring
						((time_t)$3, TRUE)); }
	| EXPAND '(' FIELD ')'		{ $$ = f_expand($3, yycard->row); check_error; }
	| EXPAND '(' FIELD '[' number ']' ')'
					{ $$ = f_expand($3, $5); check_error; }
	| PRINTF '(' args ')'		{ $$ = f_printf($3); check_error; }
	| MATCH '(' string ',' string ')' { $$ = f_str(f_re_match($3, $5)); check_error; }
	| SUB '(' string ',' string ',' string ')' { $$ = f_re_sub($3, $5, $7, false); check_error; }
	| GSUB '(' string ',' string ',' string ')' { $$ = f_re_sub($3, $5, $7, true); check_error; }
	| BSUB '(' string ')'		{ $$ = $3; if($3) backslash_subst($3); }
	| BEEP				{ app->beep(); $$ = 0; }
	| ERROR '(' args ')'		{ char *s = f_printf($3); check_error;
					  create_error_popup(mainwindow, 0, s);
					  zfree(s); $$ = 0; }
	;

args	: string			{ $$ = f_addarg(0, $1); check_error; }
	| args ',' string		{ $$ = f_addarg($1, $3); check_error; }
	;

number	: numarg			{ $$ = $1; }
        | number ',' numarg		{ $$ = $3; }

numarg	: NUMBER			{ $$ = $1; }
	| '{' string '}'		{ $$ = f_num($2); }
	| '(' number ')'		{ $$ = $2; }
	| FIELD				{ $$ = f_num(f_field($1,yycard->row));}
	| FIELD '[' number ']'		{ $$ = f_num(f_field($1, $3)); }
	| FIELD '=' numarg		{ zfree(f_assign($1, yycard->row,
					  f_str($$ = $3))); assigned = 1; check_error; }
	| FIELD '[' number ']' '=' numarg
					{ zfree(f_assign($1, $3,
					  f_str($$ = $6))); assigned = 1; check_error; }
	| VAR				{ $$ = getnvar($1); }
	| VAR '=' numarg		{ $$ = setnvar($1, $3); }
	| VAR PLA numarg		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) + $3); }
	| VAR MIA numarg		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) - $3); }
	| VAR MUA numarg		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) * $3); }
	| VAR DVA numarg		{ int v = $1; double d=$3; if(d==0)d=1; /* cheater */
					  $$ = setnvar(v, getnvar(v) / d); }
	| VAR MOA numarg		{ int v = $1; double d=$3; if(d==0)d=1; /* cheater */
					  $$ = setnvar(v, (double)((int)
							  getnvar(v)%(int)d));}
	| VAR ANA numarg		{ int v = $1;
					  $$ = setnvar(v, (double)((int)$3 &
							  (int)getnvar(v))); }
	| VAR ORA numarg		{ int v = $1;
					  $$ = setnvar(v, (double)((int)$3 |
							  (int)getnvar(v))); }
	| VAR INC			{ int v = $1;
					  $$ = setnvar(v, getnvar(v) + 1) - 1;}
	| VAR DEC_			{ int v = $1;
					  $$ = setnvar(v, getnvar(v) - 1) + 1;}
	| INC VAR			{ int v = $2;
					  $$ = setnvar(v, getnvar(v) + 1); }
	| DEC_ VAR			{ int v = $2;
					  $$ = setnvar(v, getnvar(v) - 1); }
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
	| THIS				{ $$ = yycard && yycard->row > 0 ?
					       yycard->row : 0; }
	| LAST				{ $$ = yycard && yycard->dbase ?
					       yycard->dbase->nrows - 1 : -1; }
	| DISP				{ $$ = yycard && yycard->dbase
						      && yycard->disprow >= 0
						      && yycard->disprow <
							 yycard->dbase->nrows ?
					       yycard->disprow : -1; }
	| AVG   '(' FIELD ')'		{ $$ = f_avg($3); }
	| DEV   '(' FIELD ')'		{ $$ = f_dev($3); }
	| AMIN  '(' FIELD ')'		{ $$ = f_min($3); }
	| AMAX  '(' FIELD ')'		{ $$ = f_max($3); }
	| SUM   '(' FIELD ')'		{ $$ = f_sum($3); }
	| QAVG  '(' FIELD ')'		{ $$ = f_qavg($3); }
	| QDEV  '(' FIELD ')'		{ $$ = f_qdev($3); }
	| QMIN  '(' FIELD ')'		{ $$ = f_qmin($3); }
	| QMAX  '(' FIELD ')'		{ $$ = f_qmax($3); }
	| QSUM  '(' FIELD ')'		{ $$ = f_qsum($3); }
	| SAVG  '(' FIELD ')'		{ $$ = f_savg($3); }
	| SDEV  '(' FIELD ')'		{ $$ = f_sdev($3); }
	| SMIN  '(' FIELD ')'		{ $$ = f_smin($3); }
	| SMAX  '(' FIELD ')'		{ $$ = f_smax($3); }
	| SSUM  '(' FIELD ')'		{ $$ = f_ssum($3); }
	| ABS   '(' number ')'		{ $$ = abs($3); }
	| INT   '(' number ')'		{ $$ = (long)($3); }
	| BOUND '(' numarg ',' numarg ',' number ')'
					{ double a=$3, b=$5, c=$7;
					  $$ = a < b ? b : a > c ? c : a; }
	| LEN   '(' string ')'		{ char *a=$3; $$ = a ? f_len(a) : 0;
								zfree(a); }
	| '#' string			{ $$ = $2 ? f_len($2) : 0; zfree($2); }
	| ALEN_ string			{ $$ = f_alen($2); }
	| MATCH '(' string ',' string ')' { $$ = f_re_match($3, $5); check_error; }
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
	| SECTION_			{ $$ = yycard && yycard->dbase ?
						yycard->dbase->currsect : 0; }
	| SECTION_ '[' number ']'	{ $$ = f_section($3); }
	| DATE				{ $$ = time(0); }
	| DATE  '(' string ')'		{ $$ = $3 ? parse_datetimestring($3) : 0; zfree($3);}
	| TIME  '(' string ')'		{ $$ = $3 ? parse_timestring($3, FALSE) : 0; zfree($3);}
	| DURATION '(' string ')'	{ $$ = $3 ? parse_timestring($3, TRUE) : 0; zfree($3);}
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

plus	: { $$ = 0; }
	| '+' { $$ = 1; }
%%
