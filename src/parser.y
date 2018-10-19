%{
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <Xm/Xm.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

extern char	*yyret;
extern CARD	*yycard;	/* current card to evaluate for */
extern Display	*display;	/* current X screen */
extern Widget	toplevel;	/* for error popups */
extern char	*prev_form;	/* previous form loaded, curr is in <card> */
extern char	*switch_name;	/* if switch statement was found, name to */
extern char	*switch_expr;	/* .. switch to and search expression */
extern BOOL	assigned;	/* did a field assignment */

static struct var {		/* variables 0..25 are a..z, cleared when */
	char	*string;	/* switching databases. 26..51 are A..Z, */
	double	value;		/* which are never cleared. */
	BOOL	numeric;
} var[52];

static int	f_len	(char  *s)  { return(s ? strlen(s) : 0); }
static void	f_free	(char  *s)  { if (s) free((void *)s); }
static char    *f_str	(double d)  { char buf[100]; sprintf(buf,"%.12lg",d);
					return(mystrdup(buf)); }
static int	f_cmp	(char  *s,
			 char  *t)  { return(strcmp(s?s:"", t?t:"")); }

static char    *getsvar	(int    v)  { char buf[100], *r = var[v].string;
					if (var[v].numeric) { sprintf(r = buf,
							"%g", var[v].value); }
					return(mystrdup(r)); }
static double   getnvar	(int    v)  { return(var[v].numeric ? var[v].value :
					var[v].string?atof(var[v].string):0);}
static char    *setsvar	(int    v,
			 char  *s)  { f_free(var[v].string);
					var[v].numeric = FALSE;
					return(mystrdup(var[v].string = s)); }
static double   setnvar	(int    v,
			 double d)  { (void)setsvar(v, 0);
					var[v].numeric = TRUE;
					return(var[v].value = d); }

void init_variables(void) { int i; for (i=0; i < 26; i++) setsvar(i, 0); }
%}

%union { int ival; double dval; char *sval; struct arg *aval; }
%type	<dval>	number
%type	<sval>	string
%type	<aval>	args
%token	<dval>	NUMBER
%token	<sval>	STRING SYMBOL
%token	<ival>	FIELD VAR
%token		EQ NEQ LE GE SHR SHL AND OR IN
%token		PLA MIA MUA MOA DVA ANA ORA INC DEC_ APP
%token		AVG DEV AMIN AMAX SUM
%token		QAVG QDEV QMIN QMAX QSUM
%token		SAVG SDEV SMIN SMAX SSUM
%token		ABS INT BOUND LEN CHOP TR SUBSTR SQRT EXP LOG LN POW RANDOM
%token		SIN COS TAN ASIN ACOS ATAN ATAN2
%token		DATE TIME DURATION EXPAND
%token		YEAR MONTH DAY HOUR MINUTE SECOND LEAP JULIAN
%token		SECTION_ DBASE_ FORM_ PREVFORM SWITCH THIS LAST DISP FOREACH
%token		HOST USER UID GID SYSTEM ACCESS BEEP ERROR PRINTF

%left ',' ';'
%right '?' ':'
%right PLA MIA MUA MOA DVA ANA ORA APP '='
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left IN
%left EQ NEQ
%left '<' '>' LE GE
%left SHL SHR
%left '-' '+'
%left '*' '/' '%'
%nonassoc '!' '~' UMINUS
%left '.'

%start stmt

%%
stmt	: string			{ yyret = $1; }
	;

string	: STRING			{ $$ = $1; }
	| '{' string '}'		{ $$ = $2; }
	| string ';' string		{ $$ = $3; f_free($1); }
	| string '.' string		{ char *s=$1, *t=$3, *r=
					     malloc(f_len(s)+f_len(t)+1); *r=0;
					  if (s) strcpy(r, s); f_free(s);
					  if (t) strcat(r, t); f_free(t);
					  $$ = r; }
	| VAR				{ $$ = getsvar($1); }
	| VAR APP string		{ int v=$1;
					  char *s=getsvar(v), *t=$3, *r=
					     malloc(f_len(s)+f_len(t)+1); *r=0;
					  if (s) strcpy(r, s); f_free(s);
					  if (t) strcat(r, t); f_free(t);
					  $$ = setsvar(v, r); }
	| VAR '=' string		{ $$ = setsvar($1, $3);}
	| '(' number ')'		{ $$ = f_str($2); }
	| string '?' string ':' string	{ $$ = f_num($1) ? $3 : $5; }
	| string '<' string		{ $$ = f_str((double)
							(f_cmp($1, $3) <  0));}
	| string '>' string		{ $$ = f_str((double)
							(f_cmp($1, $3) >  0));}
	| string EQ  string		{ $$ = f_str((double)
							(f_cmp($1, $3) == 0));}
	| string NEQ string		{ $$ = f_str((double)
							(f_cmp($1, $3) != 0));}
	| string LE  string		{ $$ = f_str((double)
							(f_cmp($1, $3) <= 0));}
	| string GE  string		{ $$ = f_str((double)
							(f_cmp($1, $3) >= 0));}
	| string IN  string		{ $$ = f_str((double)f_instr($1, $3));}
	| FIELD				{ $$ = f_field($1, yycard->row); }
	| FIELD '[' number ']'		{ $$ = f_field($1, $3); }
	| FIELD '=' string		{ $$ = f_assign($1, yycard->row, $3);
					  assigned = 1; }
	| FIELD '[' number ']' '=' string
					{ $$ = f_assign($1, $3, $6);
					  assigned = 1; }
	| SYSTEM '(' string ')'		{ $$ = f_system($3); }
	| '$' SYMBOL			{ $$ = mystrdup(getenv($2)); }
	| CHOP '(' string ')'		{ char *s=$3; if (s) { int n=strlen(s);
					  if (n && s[n-1]=='\n') s[n-1] = 0; }
					  $$ = s; }
	| TR '(' string ',' string ')'	{ $$ = f_tr($3, $5); }
	| SUBSTR '(' string ',' number ',' number ')'
					{ $$ = f_substr($3, (int)$5, (int)$7);}
	| HOST				{ char s[80]; if (gethostname(s, 80))
					  *s=0; s[80-1]=0; $$ = mystrdup(s); }
	| USER				{ $$ = mystrdup(getenv("USER")); }
	| PREVFORM			{ $$ = mystrdup(prev_form); }
	| SECTION_			{ $$ = !yycard || !yycard->dbase ? 0 :
						mystrdup(section_name(
						    yycard->dbase,
						    yycard->dbase->currsect));}
	| SECTION_ '[' number ']'	{ $$ = mystrdup(section_name(
						    yycard->dbase,
						    f_section($3))); }
	| FORM_				{ $$ = yycard && yycard->form
						      && yycard->form->name ?
						mystrdup(resolve_tilde
						(yycard->form->name, "gf")):0;}
	| DBASE_			{ $$ = yycard && yycard->form
						      && yycard->form->dbase ?
						mystrdup(resolve_tilde
							(yycard->form->dbase,
							 yycard->form->proc ?
								0 : "db")) :0;}
	| SWITCH '(' string ',' string ')'
					{ char *name = $3, *expr = $5;
					  f_free(switch_name);
					  f_free(switch_expr);
					  switch_name = mystrdup(name);
					  switch_expr = mystrdup(expr);
					  f_free(name); f_free(expr); $$ = 0; }
	| FOREACH '(' string ')'	{ f_foreach(0, $3); $$ = 0; }
	| FOREACH '(' string ',' string ')'
					{ f_foreach($3, $5); $$ = 0; }
	| TIME				{ $$ = mystrdup(mktimestring
						(time(0), FALSE)); }
	| DATE				{ $$ = mystrdup(mkdatestring
						(time(0))); }
	| TIME '(' number ')'		{ $$ = mystrdup(mktimestring
						((time_t)$3, FALSE)); }
	| DATE '(' number ')'		{ $$ = mystrdup(mkdatestring
						((time_t)$3)); }
	| DURATION '(' number ')'	{ $$ = mystrdup(mktimestring
						((time_t)$3, TRUE)); }
	| EXPAND '(' FIELD ')'		{ $$ = f_expand($3, yycard->row); }
	| EXPAND '(' FIELD '[' number ']' ')'
					{ $$ = f_expand($3, $5); }
	| PRINTF '(' args ')'		{ $$ = f_printf($3); }
	| BEEP				{ XBell(display, 0); $$ = 0; }
	| ERROR '(' args ')'		{ char *s = f_printf($3);
					  create_error_popup(toplevel, 0, s);
					  f_free(s); $$ = 0; }
	;

args	: string			{ $$ = f_addarg(0, $1); }
	| args ',' string		{ $$ = f_addarg($1, $3); }
	;

number	: NUMBER			{ $$ = $1; }
	| '{' string '}'		{ $$ = f_num($2); }
	| '(' number ')'		{ $$ = $2; }
	| FIELD				{ $$ = f_num(f_field($1,yycard->row));}
	| FIELD '[' number ']'		{ $$ = f_num(f_field($1, $3)); }
	| FIELD '=' number		{ f_free(f_assign($1, yycard->row,
					  f_str($$ = $3))); assigned = 1; }
	| FIELD '[' number ']' '=' number
					{ f_free(f_assign($1, $3,
					  f_str($$ = $6))); assigned = 1; }
	| VAR				{ $$ = getnvar($1); }
	| VAR '=' number		{ $$ = setnvar($1, $3); }
	| VAR PLA number		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) + $3); }
	| VAR MIA number		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) - $3); }
	| VAR MUA number		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) * $3); }
	| VAR DVA number		{ int v = $1; double d=$3; if(d==0)d=1;
					  $$ = setnvar(v, getnvar(v) / d); }
	| VAR MOA number		{ int v = $1; double d=$3; if(d==0)d=1;
					  $$ = setnvar(v, (double)((int)
							  getnvar(v)%(int)d));}
	| VAR ANA number		{ int v = $1;
					  $$ = setnvar(v, (double)((int)$3 &
							  (int)getnvar(v))); }
	| VAR ORA number		{ int v = $1;
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
	| '-' number %prec UMINUS	{ $$ = - $2; }
	| '!' number			{ $$ = ! $2; }
	| '~' number			{ $$ = ~ (int)$2; }
	| number '&' number		{ $$ = (int)$1 &  (int)$3; }
	| number '^' number		{ $$ = (int)$1 ^  (int)$3; }
	| number '|' number		{ $$ = (int)$1 |  (int)$3; }
	| number SHL number		{ $$ = (int)$1 << (int)$3; }
	| number SHR number		{ $$ = (int)$1 >> (int)$3; }
	| number '%' number		{ int i=$3; if (i==0) i=1;
					  $$ = (int)$1 % i; }
	| number '+' number		{ $$ = $1 +  $3; }
	| number '-' number		{ $$ = $1 -  $3; }
	| number '*' number		{ $$ = $1 *  $3; }
	| number '/' number		{ double d=$3; if (d==0) d=1;
					  $$ = $1 /  d; }
	| number '<' number		{ $$ = $1 <  $3; }
	| number '>' number		{ $$ = $1 >  $3; }
	| number EQ  number		{ $$ = $1 == $3; }
	| number NEQ number		{ $$ = $1 != $3; }
	| number LE  number		{ $$ = $1 <= $3; }
	| number GE  number		{ $$ = $1 >= $3; }
	| number AND number		{ $$ = $1 && $3; }
	| number OR  number		{ $$ = $1 || $3; }
	| number '?' number ':' number	{ $$ = $1 ?  $3 : $5; }
	| number ',' number		{ $$ = $3; }
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
	| INT   '(' number ')'		{ $$ = (int)($3); }
	| BOUND '(' number ',' number ',' number ')'
					{ register double a=$3, b=$5, c=$7;
					  $$ = a < b ? b : a > c ? c : a; }
	| LEN   '(' string ')'		{ char *a=$3; $$ = a ? f_len(a) : 0;
								f_free(a); }
	| SQRT  '(' number ')'		{ $$ = sqrt(abs($3));  }
	| EXP   '(' number ')'		{ $$ = exp($3); }
	| LOG   '(' number ')'		{ double a=$3; $$ = a<=0 ? 0:log10(a);}
	| LN    '(' number ')'		{ double a=$3; $$ = a<=0 ? 0:log(a); }
	| POW   '(' number ',' number ')'
					{ $$ = pow($3, $5); }
	| RANDOM			{ $$ = drand48(); }
	| SIN   '(' number ')'		{ $$ = sin($3); }
	| COS   '(' number ')'		{ $$ = cos($3); }
	| TAN   '(' number ')'		{ $$ = tan($3); }
	| ASIN  '(' number ')'		{ $$ = asin($3); }
	| ACOS  '(' number ')'		{ $$ = acos($3); }
	| ATAN  '(' number ')'		{ $$ = atan($3); }
	| ATAN2 '(' number ',' number ')'
					{ $$ = atan2($3, $5); }
	| SECTION_			{ $$ = yycard && yycard->dbase ?
						yycard->dbase->currsect : 0; }
	| SECTION_ '[' number ']'	{ $$ = f_section($3); }
	| DATE				{ $$ = time(0); }
	| DATE  '(' string ')'		{ $$ = parse_datetimestring($3); }
	| TIME  '(' string ')'		{ $$ = parse_timestring($3, FALSE); }
	| DURATION '(' string ')'	{ $$ = parse_timestring($3, TRUE); }
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
					  f_free(a); }
	;
%%
