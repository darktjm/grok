/*
 * evaluate an expression string, return result string or 0 if error
 */

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "parser_yacc.h"

#if !(defined(bsdi) || defined(linux))
int		yylineno;		/* current line # being parsed */
#endif
static char	*yyexpr;		/* first char read by lexer */
static char	*yytext;		/* next char to be read by lexer */
char		*yyret;			/* returned string (see parser.y) */
static char	errormsg[2000];		/* error message if any, or "" */
CARD		*yycard;		/* database for which to evaluate */
char		*switch_name;		/* if switch statement was found, */
char		*switch_expr;		/* .. name to switch to and expr */
BOOL		assigned;		/* did a field assignment */


/*
 * callbacks from the yacc parser
 */

int yywrap(void) { return(1); }

void parsererror(
	const char *msg)
{
	if (!*errormsg) {
		sprintf(errormsg,
"Problem with search expression\n%s:\n%.80s in column %d near '%.4s'\n",
				yyexpr, msg, (int)(yytext - yyexpr), yytext);
		fprintf(stderr, "%s: %s\n", progname, errormsg);
	}
}

bool eval_error(void)
{
    return !!*errormsg;
}


/*
 * evaluate an expression and return a static buffer with the result string.
 * Actually the buffer isn't static, it's allocated memory that gets freed
 * when the next expression is evaluated. It's up to 10 times faster this way.
 */

const char *evaluate(
	CARD		*card,
	const char	*exp)
{
	if (!exp)
		return(0);
	if (!*exp || (*exp != '(' && *exp != '{' && *exp != '$'))
		return(exp);
	if (yyret)
		free((void *)yyret);
	*errormsg = 0;
	assigned  = 0;
	yytext =
	yyexpr = mystrdup(exp);
	yycard = card;
	yyret  = 0;

	(void)parserparse();

	free((void *)yyexpr);
	if (*errormsg) {
		create_error_popup(mainwindow, 0, errormsg);
		return(0);
	}
	return(STR(yyret));
}

/* For foreach(): evaluate the string and restore parser state */
/* This relies on parserparse() being reentrant, which may not be true */
const char *subeval(
	const char	*exp)
{
	char		*saved_text = yytext;
	char		*saved_expr = yyexpr;
	BOOL		saved_assigned = assigned;
	if (!exp || *errormsg) // refuse to continue on errors
		return(0);
	if (!*exp || (*exp != '(' && *exp != '{' && *exp != '$'))
		return(exp);
	if (yyret)
		free((void *)yyret);

	yytext = yyexpr = strdup(exp);
	assigned  = 0;
	yyret  = 0;

	(void)parserparse();

	free((void *)yyexpr);
	assigned   |= saved_assigned;
	yytext	    = saved_text;
	yyexpr	    = saved_expr;
	return(STR(yyret));
}

BOOL evalbool(
	CARD		*card,
	const char	*exp)
{
	if (!(exp = evaluate(card, exp)))
		return(FALSE);
	while (*exp == ' ' || *exp == '\t' || *exp == '\n')
		exp++;
	return(*exp && *exp != '0' && *exp != 'f' && *exp != 'F');
}

BOOL subevalbool(
	const char	*exp)
{
	if (!(exp = subeval(exp)))
		return(FALSE);
	while (*exp == ' ' || *exp == '\t' || *exp == '\n')
		exp++;
	return(*exp && *exp != '0' && *exp != 'f' && *exp != 'F');
}


/*
 * the following function evaluates a sub-expression from within another
 * expression. For each card that matches the first <cond> expression,
 * execute the <expr> expression.
 */

void f_foreach(
	char	*cond,
	char	*expr)
{
	register DBASE	*dbase	    = yycard->dbase;
	int		saved_row   = yycard->row;

	if (expr && !eval_error())
		for (yycard->row=0; yycard->row < dbase->nrows; yycard->row++){
			if (!cond || subevalbool(cond))
				subeval(expr);
			if (eval_error())
				break;
	}
	if (cond)
		free(cond);
	if (expr)
		free(expr);
	yycard->row = saved_row;
}


#define ISDIGIT(c) ((c)>='0' && (c)<='9')
#define ISSYM_B(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))
#define ISSYM(c)   (ISDIGIT(c) || (c)=='_' || ISSYM_B(c))

static const char *pair_l  = "=!<><>&|+/-*%|&.+--#|||=!";
static const char *pair_r  = "====<>&|========+->#+*-~~";
static const short value[] = { EQ, NEQ, LE, GE, SHL, SHR, AND, OR,
			 PLA, DVA, MIA, MUA, MOA, ORA, ANA, APP, INC, DEC_,
			 AAS, ALEN_, UNION, INTERSECT, DIFF, REQ, RNEQ};

static const struct symtab { const char *name; int token; } symtab[] = {
			{ "this",	THIS	},
			{ "last",	LAST	},
			{ "avg",	AVG	},
			{ "dev",	DEV	},
			/* tjm - I'm guessing this was supposed to be variance */
			/*       but it conflicts with variables */
			/* { "var",	VAR	}, */
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
			{ "match",	MATCH	},
			{ "sub",	SUB	},
			{ "gsub",	GSUB	},
			{ "bsub",	BSUB	},
			{ "esc",	ESC	},
			{ "toset",	TOSET	},
			{ "detab",	DETAB	},
			{ "align",	ALIGN	},
			{ 0,		0	}
};


/*
 * return next token and its value, called by parser
 */

int parserlex(void)
#if 0 /* debug lexer */
{
	int Xparserlex(void);
	int t = Xparserlex();
	if (t > 31 && t < 127)
		printf("== '%c'\n", t);
	else switch(t) {
	  case 0:	printf("== EOF\n");				break;
	  case NUMBER:	printf("== number %.*lg\n", DBL_DIG + 1, yylval.dval);		break;
	  case STRING:	printf("== string \"%s\"\n", yylval.sval);	break;
	  case SYMBOL:	printf("== symbol \"%s\"\n", yylval.sval);	break;
	  case VAR:	printf("== var %c\n", yylval.ival +
				(yylval.ival < 26 ? 'a' : 'A'-26));	break;
	  case FIELD:	printf("== field %d\n", yylval.ival);		break;
	  default:	printf("== token %d\n", t);			break;
	}
	return(t);
}
int Xparserlex(void)
#endif
{
	int		i;
	register const struct symtab *sym;
	register ITEM	**item;
	char		msg[100], token[100], *tp=token;

	while (*yytext == ' ' || *yytext == '\t' || *yytext == '\n')		/* blanks */
		yytext++;
	if (!*yytext)						/* eof */
		return(0);
								/* number */
	if (ISDIGIT(*yytext) || (*yytext == '.' && ISDIGIT(yytext[1]))) {
		char *ptr;
		parserlval.dval = strtod(yytext, &ptr);
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
		parserlval.sval = mystrdup(begin);
		*yytext++ = quote;
		return(STRING);
	}
	if (*yytext>='a'&&*yytext<='z'&&!ISSYM(yytext[1])) {	/* variable */
		parserlval.ival = *yytext++ - 'a';
		return(VAR);
	}
	if (*yytext>='A'&&*yytext<='Z'&&!ISSYM(yytext[1])) {
		parserlval.ival = *yytext++ - 'A' + 26;
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
				parserlval.ival = atoi(token);
				return(FIELD);
			} else if(yycard->form->fields) {	/* ...name */
				FIELDS *s = yycard->form->fields;
				int n = yycard->form->nitems;
				auto it = s->find(token);
				if(it != s->end()) {
					i = it->second % n;
					parserlval.ival = item[i]->multicol ?
						item[i]->menu[it->second / n].column :
						item[i]->column;
					return(FIELD);
				}
			}
		}
		sprintf(msg, "Illegal field \"_%.80s\"", token);
		parsererror(msg);
		return(EOF);
	}
	if (ISSYM_B(*yytext)) {					/* symbol */
		while (ISSYM(*yytext))
			*tp++ = *yytext++;
		*tp = 0;
		for (sym=symtab; sym->name; sym++)		/* ...token? */
			if (!strcmp(sym->name, token))
				return(sym->token);
		parserlval.sval = mystrdup(token);			/* ...symbol */
		return(SYMBOL);
	}
	for (i=0; pair_l[i]; i++)				/* char pair */
		if (*yytext == pair_l[i] && yytext[1] == pair_r[i]) {
			yytext += 2;
			return(value[i]);
		}

	return(*yytext++);					/* char */
}
