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
#include "parser.h"

/*
 * callbacks from the yacc parser
 */

void parsererror(
	PARSE_GLOBALS	*g,
	const char	*msg, ...)
{
	va_list ap;
	if (!*g->errormsg) {
		int sl, l = sprintf(g->errormsg,
				"Problem with search expression\n"
				"%.80s in column %d near '%.4s':\n",
				g->expr, (int)(g->text - g->expr), g->text);
		fprintf(stderr, "%s: %s\n", progname, g->errormsg);
		va_start(ap, msg);
		vfprintf(stderr, msg, ap);
		va_end(ap);
		va_start(ap, msg);
		sl = vsnprintf(g->errormsg + l, sizeof(g->errormsg) - l, msg, ap);
		va_end(ap);
		if(sl > (int)sizeof(g->errormsg) - l - 1) {
			g->errormsg_full = (char *)malloc(sl + 1);
			if(g->errormsg_full) {
				va_start(ap, msg);
				vsprintf(g->errormsg_full, msg, ap);
				va_end(ap);
			}
		}
	}
}

/*
 * evaluate an expression and return a static buffer with the result string.
 * Actually the buffer isn't static, it's allocated memory that gets freed
 * when the next expression is evaluated. It's up to 10 times faster this way.
 */

const char *evaluate(
	CARD		*card,
	const char	*exp,
	CARD		**switch_card)		/* If non-0, allow switch() */
{
	static PARSE_GLOBALS pg = {};
	if(switch_card) {
		zfree(pg.switch_name);
		zfree(pg.switch_expr);
		pg.switch_name = pg.switch_expr = 0;
	}
	if (!exp)
		return(0);
	if (!*exp || (*exp != '(' && *exp != '{' && *exp != '$'))
		return(exp);
	zfree(pg.ret);
	pg.ret  = 0;
	*pg.errormsg = 0;
	zfree(pg.errormsg_full);
	pg.assigned  = 0;
	pg.text = pg.expr = strdup(exp);
	if(!pg.text) {
		parsererror(&pg, "No memory for expression");
		create_error_popup(mainwindow, 0, pg.errormsg);
		return 0;
	}
	pg.card = card;
	pg.fitem.push_back({});

	(void)parserparse(&pg);

	free(pg.expr);
	while (pg.fitem.size() > 0) {
		free_fkey_card(pg.fitem.back().card);
		pg.fitem.pop_back();
	}
	if (*pg.errormsg) {
		create_error_popup(mainwindow, 0,
				   pg.errormsg_full ? pg.errormsg_full
						    : pg.errormsg);
		return(0);
	}
	if (switch_card) {
		if (pg.switch_name)
			switch_form(*switch_card, pg.switch_name);
		if (pg.switch_expr)
			search_cards(SM_SEARCH, *switch_card, pg.switch_expr);
		zfree(pg.switch_name);
		zfree(pg.switch_expr);
		pg.switch_name = pg.switch_expr = 0;
	}
	return(STR(pg.ret));
}

/* For foreach(): evaluate the string and restore parser state */
/* Unlike evaluate(), this must be completely reentrant */
const char *subeval(
	PG,
	const char	*exp)
{
	char		*saved_text = g->text;
	char		*saved_expr = g->expr;
	fitem_stack	fist(g->fitem);

	if (!exp || *g->errormsg) // refuse to continue on errors
		return(0);
	zfree(g->ret);
	g->ret  = 0;
	/* old code saved switch, but that ignores switches in subeval */
	/* this controdicts the documentation */
	/* old code also saved assigned, but then ored in new value anyway */
	/* that only makes sesne if it's determine assignment in subexprs */
	/* but in fact, assigned isn't used anywhere for anything */
	/* the best use I can see for it is redoing the card window if assigned */

	g->text = g->expr = strdup(exp);
	if(!g->expr) {
		parsererror(g, "No memory for expression");
		return 0;
	}
	g->fitem.clear();
	g->fitem.push_back({});

	(void)parserparse(g);

	free(g->expr);
	g->expr = saved_expr;
	g->text = saved_text;
	g->fitem.clear();
	g->fitem = fist;
	return(STR(g->ret));
}

bool evalbool(
	CARD		*card,
	const char	*exp)
{
	if (!(exp = evaluate(card, exp)))
		return(false);
	while (*exp == ' ' || *exp == '\t' || *exp == '\n')
		exp++;
	return(*exp && *exp != '0' && *exp != 'f' && *exp != 'F');
}

bool subevalbool(
	PG,
	const char	*exp)
{
	if (!(exp = subeval(g, exp)))
		return(false);
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
	PG,
	char	*cond,
	char	*expr)
{
	DBASE		*dbase	    = g->card->form->dbase;
	int		saved_row   = g->card->row;
	int		row;

	if (expr && !eval_error)
		for (row=0; row < dbase->nrows; row++){
			g->card->row = g->card->sorted ? g->card->sorted[row] : row;
			if (!cond || subevalbool(g, cond))
				subeval(g, expr); /* no-op if evalbool failed */
			if (eval_error)
				break;
	}
	zfree(cond);
	zfree(expr);
	g->card->row = saved_row;
}


#define ISDIGIT(c) ((c)>='0' && (c)<='9')
#define ISSYM_B(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z'))
#define ISSYM(c)   (ISDIGIT(c) || (c)=='_' || ISSYM_B(c))

static const char *pair_l  = "=!<><>&|+/-*%|&.+--#|||=!.[]";
static const char *pair_r  = "====<>&|========+->#+*-~~.[]";
static const short value[] = { EQ, NEQ, LE, GE, SHL, SHR, AND, OR,
			 PLA, DVA, MIA, MUA, MOA, ORA, ANA, APP, INC, DEC_,
			 AAS, ALEN_, UNION, INTERSECT, DIFF, REQ, RNEQ,
			 DOTDOT, LBRBR, RBRBR};

static const struct symtab { const char *name; int token; } symtab[] = {
			{ "this",	THIS	},
			{ "last",	LAST	},
			{ "id",		ID	},
			{ "ctime",	CTIME	},
			{ "mtime",	MTIME	},
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
			{ "count",	COUNT	},
			{ "chop",	CHOP	},
			{ "tr",		TR	},
			{ "substr",	SUBSTR	},
			{ "trunc2d",	TRUNC2D	},
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
			{ "deref",	DEREF	},
			{ "dereff",	DEREFF	},
			{ "referenced",	REFERENCED },
			{ 0,		0	}
};


/*
 * return next token and its value, called by parser
 */

int parserlex(YYSTYPE *lvalp, PG)
#if 0 /* debug lexer */
{
	int Xparserlex(YYSTYPE *, PG);
	int t = Xparserlex(lvalp, g);
	if (t > 31 && t < 127)
		printf("== '%c'\n", t);
	else switch(t) {
	  case 0:	printf("== EOF\n");				break;
	  case NUMBER:	printf("== number %.*lg\n", DBL_DIG + 1, lvalp->dval);		break;
	  case STRING:	printf("== string \"%s\"\n", lvalp->sval);	break;
	  case SYMBOL:	printf("== symbol \"%s\"\n", lvalp->sval);	break;
	  case VAR:	printf("== var %c\n", lvalp->ival +
				(lvalp->ival < 26 ? 'a' : 'A'-26));	break;
	  case FIELD:	printf("== field %d\n", lvalp->ival);		break;
	  default:	printf("== token %d\n", t);			break;
	}
	return(t);
}
int Xparserlex(YYSTYPE *, PG); /* kill -Wmissing-declaration */
int Xparserlex(YYSTYPE *lvalp, PG)
#endif
{
	int			i;
	const struct symtab	*sym;
	char			msg[100];
	static char *		token = 0;
	static size_t		tokenlen;
	int			tp = 0;

	while (*g->text == ' ' || *g->text == '\t' || *g->text == '\n')		/* blanks */
		g->text++;
	/* fkey context only valid for next token iff field */
	if (*g->text == '[')
		g->fitem.push_back({});
	else if (*g->text == ']') {
		/* card will be freed when field is used */
		g->fitem.pop_back();
	} else if (*g->text != '_')
		g->fitem.back() = {};
	if (!*g->text)						/* eof */
		return(0);
	
								/* number */
	if (ISDIGIT(*g->text) || (*g->text == '.' && ISDIGIT(g->text[1]))) {
		char *ptr;
		lvalp->dval = strtod(g->text, &ptr);
		g->text = ptr;
		return(NUMBER);
	}
								/* string */
	if (*g->text == '\'' || *g->text == '"') {
		char quote = *g->text++;
		char *begin = g->text;
		while (*g->text && *g->text != quote)
			if (*g->text++ == '\\' && *g->text)
				g->text++;
		if (!*g->text)
			return(0); /* unterminated */
		*g->text = 0;
		lvalp->sval = mystrdup(begin);
		*g->text++ = quote;
		return(STRING);
	}
	if (*g->text>='a'&&*g->text<='z'&&!ISSYM(g->text[1])) {	/* variable */
		lvalp->ival = *g->text++ - 'a';
		return(VAR);
	}
	if (*g->text>='A'&&*g->text<='Z'&&!ISSYM(g->text[1])) {
		lvalp->ival = *g->text++ - 'A' + 26;
		return(VAR);
	}
	if (*g->text == '_') {					/* field */
		g->text++;
		while (ISSYM(*g->text)) {
			/* this is fatal because it affects all expressions */
			grow(0, "field name", char, token, tp + 2, &tokenlen);
			token[tp++] = *g->text++;
		}
		token[tp] = 0;
		if (g->card && g->card->form) {
			CARD *c = g->card;
			FORM *form = g->card->form;
			fitem &fit = g->fitem.back();
			int row = -1;
			if (!fit.card)
				row = g->card->row;
			if (fit.card) {
				resolve_fkey_fields(fit.item);
				read_dbase(form = fit.item->fkey_form);
				c = create_card_menu(form);
				c->fkey_next = fit.card;
				row = fit.card->row;
			}
			ITEM **item = form->items;
			lvalp->fval.card = c;
			if (fit.card) {
				const FORM *f = fit.card->form;
				DBASE *db = f->dbase;
				if (row < 0 || row >= db->nrows)
					lvalp->fval.row = fit.card->qcurr = fit.card->row = -1;
				else {
					DBASE *fdb = c->form->dbase;
					g->card->qcurr = lvalp->fval.row =
						fkey_lookup(fdb, fit.item,
							    dbase_get(db, row,
								      fit.item->column));
				}
			} else
				lvalp->fval.row = g->card->row;
			fit = {};
			if (ISDIGIT(*token)) {			/* ...numeric*/
				lvalp->fval.column = atoi(token);
				for (i = 0; i < form->nitems; i++)
					if (item[i]->type == IT_FKEY &&
					    item[i]->column == lvalp->ival) {
						fit.item = item[i];
						fit.card = c;
						c->row = lvalp->fval.row;
						break;
					}
				return(FIELD);
			} else if(form->fields) {	/* ...name */
				FIELDS *s = form->fields;
				int n = form->nitems;
				auto it = s->find(token);
				if(it != s->end()) {
					i = it->second % n;
					if (item[i]->type == IT_FKEY) {
						fit.item = item[i];
						fit.card = c;
						c->row = lvalp->fval.row;
					}
					lvalp->fval.column = IFL(item[i]->,MULTICOL) ?
						item[i]->menu[it->second / n].column :
						item[i]->column;
					return(FIELD);
				}
			}
		}
		sprintf(msg, "Illegal field \"_%.80s\"", token);
		parsererror(g, msg);
		return(EOF);
	}
	if (ISSYM_B(*g->text)) {					/* symbol */
		while (ISSYM(*g->text)) {
			/* this is fatal because it affects all expressions */
			grow(0, "field name", char, token, tp + 2, &tokenlen);
			token[tp++] = *g->text++;
		}
		token[tp] = 0;
		for (sym=symtab; sym->name; sym++)		/* ...token? */
			if (!strcmp(sym->name, token))
				return(sym->token);
		lvalp->sval = mystrdup(token);			/* ...symbol */
		return(SYMBOL);
	}
	for (i=0; pair_l[i]; i++)				/* char pair */
		if (*g->text == pair_l[i] && g->text[1] == pair_r[i]) {
			g->text += 2;
			return(value[i]);
		}

	return(*g->text++);					/* char */
}
