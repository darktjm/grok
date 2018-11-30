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

static struct var {		/* variables 0..25 are a..z, cleared when */
	char	*string;	/* switching databases. 26..51 are A..Z, */
	double	value;		/* which are never cleared. */
	BOOL	numeric;
} var[52];

static int	f_len	(char  *s)  { return(s ? strlen(s) : 0); }
static char    *f_str	(double d)  { char buf[100]; sprintf(buf,"%.12lg",d);
					return(mystrdup(buf)); }
static int	f_cmp	(char  *s,
			 char  *t)  { int r = strcmp(STR(s), STR(t));
				      zfree(s); zfree(t); return r;}

static char    *getsvar	(int    v)  { char buf[100], *r = var[v].string;
					if (var[v].numeric) { sprintf(r = buf,
							"%.*lg", DBL_DIG + 1,
							    var[v].value); }
					return(mystrdup(r)); }
static double   getnvar	(int    v)  { return(var[v].numeric ? var[v].value :
					var[v].string?atof(var[v].string):0);}
static char    *setsvar	(int    v,
			 char  *s)  { zfree(var[v].string);
					var[v].numeric = FALSE;
					return(mystrdup(var[v].string = s)); }
static double   setnvar	(int    v,
			 double d)  { (void)setsvar(v, 0);
					var[v].numeric = TRUE;
					return(var[v].value = d); }
void set_var(int v, const char *s)
{
    zfree(var[v].string);
    if(!BLANK(s))
	var[v].string = strdup(s);
    else
	var[v].string = NULL;
    var[v].numeric = FALSE;
}

void init_variables(void) { int i; for (i=0; i < 26; i++) setsvar(i, 0); }

%}

%union { int ival; double dval; char *sval; struct arg *aval; }
%type	<dval>	number numarg
%type	<sval>	string
%type	<aval>	args
%type	<ival>	plus
%token	<dval>	NUMBER
%token	<sval>	STRING SYMBOL
%token	<ival>	FIELD VAR
%token		EQ NEQ LE GE SHR SHL AND OR IN UNION INTERSECT DIFF
%token		PLA MIA MUA MOA DVA ANA ORA INC DEC_ APP AAS ALEN_
%token		AVG DEV AMIN AMAX SUM
%token		QAVG QDEV QMIN QMAX QSUM
%token		SAVG SDEV SMIN SMAX SSUM
%token		ABS INT BOUND LEN CHOP TR SUBSTR SQRT EXP LOG LN POW RANDOM
%token		SIN COS TAN ASIN ACOS ATAN ATAN2
%token		DATE TIME DURATION EXPAND
%token		YEAR MONTH DAY HOUR MINUTE SECOND LEAP JULIAN
%token		SECTION_ DBASE_ FORM_ PREVFORM SWITCH THIS LAST DISP FOREACH
%token		HOST USER UID GID SYSTEM ACCESS BEEP ERROR PRINTF MATCH SUB
%token		GSUB BSUB ESC TOSET

%left 's' /* Force a shift; i.e., prefer longer versions */
%left ',' ';'
%right '?' ':'
%right PLA MIA MUA MOA DVA ANA ORA APP '=' AAS
%left OR
%left AND
%left '|'
%left '^'
%left '&'
%left IN
%left EQ NEQ
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
	| string '.' string		{ char *s=$1, *t=$3, *r=
					     (char *)malloc(f_len(s)+f_len(t)+1); *r=0;
					  if (s) strcpy(r, s); zfree(s);
					  if (t) strcat(r, t); zfree(t);
					  $$ = r; }
	| VAR %prec 's'			{ $$ = getsvar($1); }
	| VAR APP string		{ int v=$1;
					  char *s=getsvar(v), *t=$3, *r=
					     (char *)malloc(f_len(s)+f_len(t)+1); *r=0;
					  if (s) strcpy(r, s); zfree(s);
					  if (t) strcat(r, t); zfree(t);
					  $$ = setsvar(v, r); }
	| VAR '=' string		{ $$ = setsvar($1, $3);}
	| '(' number ')'		{ $$ = f_str($2); }
	| string '?' string ':' string	{ bool c = f_num($1); $$ = c ? $3 : $5;
					  if (c) zfree($5); else zfree($3);}
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
	| ESC '(' string ',' string ')'	{ if($5) $$ = f_esc($3, $5); else $$ = $3; }
	| ESC '(' string ')'		{ $$ = f_esc($3, NULL); }
	| string '[' number ']' 		{ $$ = f_elt($1, $3); }
	| string '[' number ']' AAS string  { $$ = f_setelt($1, $3, $6); }
	| string UNION  string		{ $$ = f_union($1, $3); }
	| string INTERSECT  string	{ $$ = f_intersect($1, $3);}
	| string DIFF  string		{ $$ = f_setdiff($1, $3);}
	| TOSET '(' string ')'		{ $$ = f_toset($3); }
	| FIELD %prec 's'			{ $$ = f_field($1, yycard->row); }
	| FIELD '[' number ']' 		{ $$ = f_field($1, $3); }
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
	| SUBSTR '(' string ',' numarg ',' number ')'
					{ $$ = f_substr($3, (int)$5, (int)$7);}
	| HOST				{ char s[80]; if (gethostname(s, 80))
					  *s=0; s[80-1]=0; $$ = mystrdup(s); }
	| USER				{ $$ = mystrdup(getenv("USER")); }
	| PREVFORM			{ $$ = mystrdup(prev_form); }
	| SECTION_ %prec 's'		{ $$ = !yycard || !yycard->dbase ? 0 :
						mystrdup(section_name(
						    yycard->dbase,
						    yycard->dbase->currsect));}
	| SECTION_ '[' number ']' 	{ $$ = !yycard || !yycard->dbase ? 0 :
						mystrdup(section_name(
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
					  zfree(switch_name);
					  zfree(switch_expr);
					  switch_name = name;
					  switch_expr = expr; 
					  $$ = 0; }
	| FOREACH '(' string ')'	{ f_foreach(0, $3); $$ = 0; }
	| FOREACH '(' string ',' string ')'
					{ f_foreach($3, $5); $$ = 0; }
	| FOREACH '(' VAR ',' plus string ',' string ')'
					{ f_foreachelt($3, $6, 0, $8, $5); $$ = 0; }
	| FOREACH '(' VAR ',' plus string ',' string ',' string ')'
					{ f_foreachelt($3, $6, $8, $10, $5); $$ = 0; }
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
	| MATCH '(' string ',' string ')' { $$ = f_str(re_match($3, $5)); }
	| SUB '(' string ',' string ',' string ')' { $$ = re_sub($3, $5, $7, false); }
	| GSUB '(' string ',' string ',' string ')' { $$ = re_sub($3, $5, $7, true); }
	| BSUB '(' string ')'		{ $$ = $3; if($3) backslash_subst($3); }
	| BEEP				{ app->beep(); $$ = 0; }
	| ERROR '(' args ')'		{ char *s = f_printf($3);
					  create_error_popup(mainwindow, 0, s);
					  zfree(s); $$ = 0; }
	;

args	: string			{ $$ = f_addarg(0, $1); }
	| args ',' string		{ $$ = f_addarg($1, $3); }
	;

number	: numarg			{ $$ = $1; }
        | number ',' numarg		{ $$ = $3; }

numarg	: NUMBER			{ $$ = $1; }
	| '{' string '}'		{ $$ = f_num($2); }
	| '(' number ')'		{ $$ = $2; }
	| FIELD				{ $$ = f_num(f_field($1,yycard->row));}
	| FIELD '[' number ']'		{ $$ = f_num(f_field($1, $3)); }
	| FIELD '=' numarg		{ zfree(f_assign($1, yycard->row,
					  f_str($$ = $3))); assigned = 1; }
	| FIELD '[' number ']' '=' numarg
					{ zfree(f_assign($1, $3,
					  f_str($$ = $6))); assigned = 1; }
	| VAR				{ $$ = getnvar($1); }
	| VAR '=' numarg		{ $$ = setnvar($1, $3); }
	| VAR PLA numarg		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) + $3); }
	| VAR MIA numarg		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) - $3); }
	| VAR MUA numarg		{ int v = $1;
					  $$ = setnvar(v, getnvar(v) * $3); }
	| VAR DVA numarg		{ int v = $1; double d=$3; if(d==0)d=1;
					  $$ = setnvar(v, getnvar(v) / d); }
	| VAR MOA numarg		{ int v = $1; double d=$3; if(d==0)d=1;
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
	| numarg '%' numarg		{ int i=$3; if (i==0) i=1;
					  $$ = (int)$1 % i; }
	| numarg '+' numarg		{ $$ = $1 +  $3; }
	| numarg '-' numarg		{ $$ = $1 -  $3; }
	| numarg '*' numarg		{ $$ = $1 *  $3; }
	| numarg '/' numarg		{ double d=$3; if (d==0) d=1;
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
	| INT   '(' number ')'		{ $$ = (int)($3); }
	| BOUND '(' numarg ',' numarg ',' number ')'
					{ register double a=$3, b=$5, c=$7;
					  $$ = a < b ? b : a > c ? c : a; }
	| LEN   '(' string ')'		{ char *a=$3; $$ = a ? f_len(a) : 0;
								zfree(a); }
	| '#' string			{ $$ = $2 ? f_len($2) : 0; zfree($2); }
	| ALEN_ string			{ $$ = f_alen($2); }
	| MATCH '(' string ',' string ')' { $$ = re_match($3, $5); }
	| SQRT  '(' number ')'		{ $$ = sqrt(abs($3));  }
	| EXP   '(' number ')'		{ $$ = exp($3); }
	| LOG   '(' number ')'		{ double a=$3; $$ = a<=0 ? 0:log10(a);}
	| LN    '(' number ')'		{ double a=$3; $$ = a<=0 ? 0:log(a); }
	| POW   '(' numarg ',' number ')'
					{ $$ = pow($3, $5); }
	| RANDOM			{ $$ = drand48(); }
	| SIN   '(' number ')'		{ $$ = sin($3); }
	| COS   '(' number ')'		{ $$ = cos($3); }
	| TAN   '(' number ')'		{ $$ = tan($3); }
	| ASIN  '(' number ')'		{ $$ = asin($3); }
	| ACOS  '(' number ')'		{ $$ = acos($3); }
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
