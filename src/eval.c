/*
 * evaluate an expression string, return result string or 0 if error
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "y.tab.h"

extern int	yyparse(void);
extern char	*progname;		/* argv[0] */
extern Widget	toplevel;		/* top-level shell for error msg */

#if defined(bsdi) || defined(linux)
extern
#endif
int		yylineno;		/* current line # being parsed */
static char	*yyexpr;		/* first char read by lexer */
static char	*yytext;		/* next char to be read by lexer */
char		*yyret;			/* returned string (see parser.y) */
static char	errormsg[2000];		/* error message if any, or "" */
static char	extramsg[1000];		/* extra error message details */
CARD		*yycard;		/* database for which to evaluate */
char		*switch_name;		/* if switch statement was found, */
char		*switch_expr;		/* .. name to switch to and expr */
BOOL		assigned;		/* did a field assignment */


/*
 * callbacks from the yacc parser
 */

int yywrap(void) { return(1); }

void yyerror(
	char *msg)
{
	if (!*errormsg) {
		sprintf(errormsg,
"Problem with search expression\n%s:\n%.80s in column %d near '%.4s'\n%s",
				yyexpr, msg, yytext - yyexpr, yytext, extramsg);
		fprintf(stderr, "%s: %s\n", progname, errormsg);
	}
}


/*
 * evaluate an expression and return a static buffer with the result string.
 * Actually the buffer isn't static, it's allocated memory that gets freed
 * when the next expression is evaluated. It's up to 10 times faster this way.
 */

char *evaluate(
	CARD		*card,
	char		*exp)
{
	if (!exp)
		return(0);
	if (!*exp || *exp != '(' && *exp != '{' && *exp != '$')
		return(exp);
	if (yyret)
		free((void *)yyret);
	*errormsg = 0;
	*extramsg = 0;
	assigned  = 0;
	yytext =
	yyexpr = mystrdup(exp);
	yycard = card;
	yyret  = 0;

	(void)yyparse();

	free((void *)yyexpr);
	if (*errormsg) {
		create_error_popup(toplevel, 0, errormsg);
		return(0);
	}
	return(yyret ? yyret : "");
}

BOOL evalbool(
	CARD		*card,
	char		*exp)
{
	if (!(exp = evaluate(card, exp)))
		return(FALSE);
	while (*exp == ' ' || *exp == '\t')
		exp++;
	return(*exp && *exp != '0' && *exp != 'f' && *exp != 'F');
}


/*
 * the following function evaluates a sub-expression from within another
 * expression. For each card that matches the first <cond> expression,
 * execute the <expr> expression.
 */

void f_foreach(
	char		*cond,
	char		*expr)
{
	register DBASE	*dbase	    = yycard->dbase;
	int		saved_row   = yycard->row;
	char		*saved_text = yytext;
	char		*saved_expr = yyexpr;
	char		*saved_ret  = yyret;
	BOOL		saved_assigned = assigned;

	saved_text = saved_expr = saved_ret = 0;
	if (expr)
		for (yycard->row=0; yycard->row < dbase->nrows; yycard->row++){
			if (!cond || evalbool(yycard, cond))
				evaluate(yycard, expr);
			if (*errormsg)
				break;
	}
	if (cond)
		free(cond);
	if (expr)
		free(expr);
	yycard->row = saved_row;
	assigned   |= saved_assigned;
	yytext	    = saved_text;
	yyexpr	    = saved_expr;
	yyret	    = saved_ret;
}


#define ISDIGIT(c) ((c)>='0' && (c)<='9')
#define ISSYM_B(c) ((c)>='a' && (c)<='z' || (c)>='A' && (c)<='Z')
#define ISSYM(c)   ((c)>='0' && (c)<='9' || (c)=='_' || ISSYM_B(c))

static char *pair_l  = "=!<><>&|+/-*%|&.+-";
static char *pair_r  = "====<>&|========+-";
static short value[] = { EQ, NEQ, LE, GE, SHL, SHR, AND, OR,
			 PLA, DVA, MIA, MUA, MOA, ORA, ANA, APP, INC, DEC_ };

static struct symtab { char *name; int token; } symtab[] = {
			{ "this",	THIS	},
			{ "last",	LAST	},
			{ "avg",	AVG	},
			{ "dev",	DEV	},
			{ "var",	VAR	},
			{ "min",	AMIN	},
			{ "max",	AMAX	},
			{ "sum",	SUM	},
			{ "abs",	ABS	},
			{ "qavg",	QAVG	},
			{ "qdev",	QDEV	},
			{ "qmin",	QMIN	},
			{ "qmax",	QMAX	},
			{ "qsum",	QSUM	},
			{ "savg",	SAVG	},
			{ "sdev",	SDEV	},
			{ "smin",	SMIN	},
			{ "smax",	SMAX	},
			{ "ssum",	SSUM	},
			{ "int",	INT	},
			{ "bound",	BOUND	},
			{ "len",	LEN	},
			{ "chop",	CHOP	},
			{ "tr",		TR	},
			{ "substr",	SUBSTR	},
			{ "in",		IN	},
			{ "sqrt",	SQRT	},
			{ "exp",	EXP	},
			{ "log",	LOG	},
			{ "ln",		LN	},
			{ "pow",	POW	},
			{ "random",	RANDOM	},
			{ "sin",	SIN	},
			{ "cos",	COS	},
			{ "tan",	TAN	},
			{ "asin",	ASIN	},
			{ "acos",	ACOS	},
			{ "atan",	ATAN	},
			{ "atan2",	ATAN2	},
			{ "date",	DATE	},
			{ "time",	TIME	},
			{ "duration",	DURATION},
			{ "year",	YEAR	},
			{ "month",	MONTH	},
			{ "day",	DAY	},
			{ "hour",	HOUR	},
			{ "minute",	MINUTE	},
			{ "second",	SECOND	},
			{ "leap",	LEAP	},
			{ "julian",	JULIAN	},
			{ "expand",	EXPAND	},
			{ "host",	HOST	},
			{ "user",	USER	},
			{ "uid",	UID	},
			{ "gid",	GID	},
			{ "access",	ACCESS	},
			{ "section",	SECTION_},
			{ "dbase",	DBASE_	},
			{ "form",	FORM_	},
			{ "prevform",	PREVFORM},
			{ "switch",	SWITCH	},
			{ "system",	SYSTEM	},
			{ "printf",	PRINTF	},
			{ "beep",	BEEP	},
			{ "error",	ERROR	},
			{ "foreach",	FOREACH	},
			{ 0,		0	}
};


/*
 * return next token and its value, called by parser
 */

int yylex(void)
#if 0 /* debug lexer */
{
	int Xyylex(void);
	int t = Xyylex();
	if (t > 31 && t < 127)
		printf("== '%c'\n", t);
	else switch(t) {
	  case 0:	printf("== EOF\n");				break;
	  case NUMBER:	printf("== number %g\n", yylval.dval);		break;
	  case STRING:	printf("== string \"%s\"\n", yylval.sval);	break;
	  case SYMBOL:	printf("== symbol \"%s\"\n", yylval.sval);	break;
	  case VAR:	printf("== var %c\n", yylval.ival +
				(yylval.ival < 26 ? 'a' : 'A'-26);	break;
	  case FIELD:	printf("== field %d\n", yylval.ival);		break;
	  default:	printf("== token %d\n", t);			break;
	}
	return(t);
}
int Xyylex(void)
#endif
{
	int		i;
	register struct symtab *sym;
	register ITEM	**item;
	char		msg[100], token[100], *tp=token;

	while (*yytext == ' ' || *yytext == '\t')		/* blanks */
		yytext++;
	if (!*yytext)						/* eof */
		return(0);
								/* number */
	if (ISDIGIT(*yytext) || *yytext == '.' && ISDIGIT(yytext[1])) {
		char *ptr;
		yylval.dval = strtod(yytext, &ptr);
		yytext = ptr;
		return(NUMBER);
	}
								/* string */
	if (*yytext == '\'' || *yytext == '"') {
		char quote = *yytext++;
		char *begin = yytext;
		while (*yytext && *yytext != quote)
			if (*yytext++ == '\\' && *yytext)
				yytext++;
		if (!*yytext)
			return(0); /* unterminated */
		*yytext = 0;
		yylval.sval = mystrdup(begin);
		*yytext++ = quote;
		return(STRING);
	}
	if (*yytext>='a'&&*yytext<='z'&&!ISSYM(yytext[1])) {	/* variable */
		yylval.ival = *yytext++ - 'a';
		return(VAR);
	}
	if (*yytext>='A'&&*yytext<='Z'&&!ISSYM(yytext[1])) {
		yylval.ival = *yytext++ - 'A' + 26;
		return(VAR);
	}
	if (*yytext == '_') {					/* field */
		yytext++;
		while (ISSYM(*yytext))
			*tp++ = *yytext++;
		*tp = 0;
		if (yycard && yycard->form) {
			item = yycard->form->items;
			if (ISDIGIT(*token)) {			/* ...numeric*/
				yylval.ival = atoi(token);
				return(FIELD);
			} else					/* ...name */
				for (i=yycard->form->nitems-1; i >= 0; i--)
					if (!strcmp(item[i]->name, token)) {
						yylval.ival = yycard->form->
							      items[i]->column;
						return(FIELD);
					}
		}
		sprintf(msg, "Illegal field \"_%s\"", token);
		yyerror(msg);
		return(EOF);
	}
	if (ISSYM_B(*yytext)) {					/* symbol */
		while (ISSYM(*yytext))
			*tp++ = *yytext++;
		*tp = 0;
		for (sym=symtab; sym->name; sym++)		/* ...token? */
			if (!strcmp(sym->name, token))
				return(sym->token);
		yylval.sval = mystrdup(token);			/* ...symbol */
		return(SYMBOL);
	}
	for (i=0; pair_l[i]; i++)				/* char pair */
		if (*yytext == pair_l[i] && yytext[1] == pair_r[i]) {
			yytext += 2;
			return(value[i]);
		}

	return(*yytext++);					/* char */
}
