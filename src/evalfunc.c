/*
 * all the functions that implement eval language features that require more
 * than a line or two of code. All these functions are called from parser.y.
 *
 *	f_num
 *	f_sum, f_qsum, f_ssum
 *	f_avg, f_qavg, f_savg
 *	f_dev, f_qdev, f_sdev
 *	f_min, f_qmin, f_smin
 *	f_max, f_qmax, f_smax
 *	f_field
 *	f_assign
 *	f_section
 *	f_system
 *	f_substr
 *	f_instr
 *	f_addarg
 *	f_printf
 */

#include "config.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "parser.h"


/*
 * convert a string to a number. There is an internal version that doesn't
 * free the string, and another version for the parser that does.
 */

static double fnum(
	char		*s)
{
	return(s ? atof(s) : 0);
}

double f_num(
	char		*s)
{
	double d = fnum(s);
	zfree(s);
	return(d);
}

/*
 * All these functions run over the entire database and calculate something
 * from a single column.
 */

double f_sum(				/* sum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		sum;
	int		row;

	if (!dbase || column < 0)
		return(0);
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		sum += fnum(dbase_get(dbase, row, column));
	return(sum);
}


double f_avg(				/* average */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		sum = f_sum(g, column);
	return(dbase->nrows ? sum / dbase->nrows : 0);
}


double f_dev(				/* standard deviation */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		sum, avg, val;
	int		row;

	if (!dbase || column < 0)
		return(0);
	avg = f_avg(g, column);
	for (sum=0, row=dbase->nrows-1; row >= 0; row--) {
		val = fnum(dbase_get(dbase, row, column)) - avg;
		sum += val * val;
	}
	return(sqrt(sum / dbase->nrows));
}


double f_min(				/* minimum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		min, val;
	int		row;

	if (!dbase || column < 0 || !dbase->nrows)
		return(0);
	min = fnum(dbase_get(dbase, dbase->nrows-1, column));
	for (row=dbase->nrows-2; row >= 0; row--) {
		val = fnum(dbase_get(dbase, row, column));
		if (val < min)
			min = val;
	}
	return(min);
}


double f_max(				/* maximum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		max, val;
	int		row;

	if (!dbase || column < 0 || !dbase->nrows)
		return(0);
	max = fnum(dbase_get(dbase, dbase->nrows-1, column));
	for (row=dbase->nrows-2; row >= 0; row--) {
		val = fnum(dbase_get(dbase, row, column));
		if (val > max)
			max = val;
	}
	return(max);
}


/*
 * All these functions run over a query or the entire database and calculate
 * something from a single column.
 */

double f_qsum(				/* sum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		sum = 0;
	int		row;

	if (!dbase || column < 0)
		return(0);
	if (!g->card->query)
		return(f_sum(g, column));
	for (row=0; row < g->card->nquery; row++)
		sum += fnum(dbase_get(dbase, g->card->query[row], column));
	return(sum);
}


double f_qavg(				/* average */
	PG,
	int		column)		/* number of column to average */
{
	double		sum = f_qsum(g, column);
	int		count;

	count = g->card->query ? g->card->nquery : g->card->dbase->nrows;
	return(count ? sum / count : 0);
}


double f_qdev(				/* standard deviation */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase;
	double		sum = 0, avg, val;
	int		row;

	if (!g->card->query)
		return(f_dev(g, column));
	dbase = g->card->dbase;
	if (!dbase || column < 0)
		return(0);
	if (!g->card->nquery)
		return(0);
	avg = f_qavg(g, column);
	for (row=0; row < g->card->nquery; row++) {
		val = fnum(dbase_get(dbase, g->card->query[row], column)) - avg;
		sum += val * val;
	}
	return(sqrt(sum / g->card->nquery));
}


double f_qmin(				/* minimum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		min, val;
	int		row;

	if (!dbase || column < 0)
		return(0);
	if (!g->card->query)
		return(f_min(g, column));
	if (!g->card->nquery)
		return(0);
	min = fnum(dbase_get(dbase, g->card->query[0], column));
	for (row=1; row < g->card->nquery; row++) {
		val = fnum(dbase_get(dbase, g->card->query[row], column));
		if (val < min)
			min = val;
	}
	return(min);
}


double f_qmax(				/* maximum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	double		max, val;
	int		row;

	if (!dbase || column < 0)
		return(0);
	if (!g->card->query)
		return(f_max(g, column));
	if (!g->card->nquery)
		return(0);
	max = fnum(dbase_get(dbase, g->card->query[0], column));
	for (row=0; row < g->card->nquery; row++) {
		val = fnum(dbase_get(dbase, g->card->query[row], column));
		if (val > max)
			max = val;
	}
	return(max);
}


/*
 * All these functions run over the entire database and calculate something
 * from a single section.
 */

double f_ssum(				/* sum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	int		sect;
	double		sum;
	int		row;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_sum(g, column));
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect)
			sum += fnum(dbase_get(dbase, row, column));
	return(sum);
}


double f_savg(				/* average */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	int		sect;
	double		sum;
	int		row, num=0;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_avg(g, column));
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			sum += fnum(dbase_get(dbase, row, column));
			num++;
		}
	return(num ? sum / sum : 0);
}


double f_sdev(				/* standard deviation */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	int		sect;
	double		sum, avg, val;
	int		row, num=0;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_dev(g, column));
	avg = f_savg(g, column);
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			val = fnum(dbase_get(dbase, row, column)) - avg;
			sum += val * val;
			num++;
		}
	return(num ? sqrt(sum / num) : 0);
}


double f_smin(				/* minimum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	int		sect;
	double		min = 0, val;
	int		row;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_min(g, column));
	for (row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			min = fnum(dbase_get(dbase, row, column));
			break;
		}
	for (row-- ; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			val = fnum(dbase_get(dbase, row, column));
			if (val < min)
				min = val;
		}
	return(min);
}


double f_smax(				/* maximum */
	PG,
	int		column)		/* number of column to average */
{
	DBASE		*dbase = g->card->dbase;
	int		sect;
	double		max = 0, val;
	int		row;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_max(g, column));
	for (row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			max = fnum(dbase_get(dbase, row, column));
			break;
		}
	for (row--; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			val = fnum(dbase_get(dbase, row, column));
			if (val > max)
				max = val;
		}
	return(max);
}


/*
 * return the value of a database field
 */

char *f_field(
	PG,
	fkey_field	field)
{
	const char *v = dbase_get(field.card->dbase, field.row, field.column);
	char *res;
	char buf[20];
	if(BLANK(v))
		return NULL;
	/* following used to be handled in read_dbase() */
	/* but I don't want internal rep different from external */
	for(int i = g->card->form->nitems - 1; i >= 0; i--) {
		const ITEM *item = g->card->form->items[i];
		if(item->type == IT_TIME && item->column == field.column) {
			const char *p;

			for (p=v; *p; p++)
				if (*p=='.' || *p==':' || *p=='/')
					break;
			if(*p) {
				sprintf(buf, "%ld",
					(long)parse_time_data(v, item->timefmt));
				v = buf;
			}
		}
	}
	res = strdup(v);
	if(!res)
		parsererror(g, "No memory");
	return res;
}


/*
 * convert choice code to string, and flag to yes/no
 */

char *f_expand(
	PG,
	fkey_field	field)
{
	char		*value = dbase_get(field.card->dbase, field.row, field.column), *ret;
	int		i;

	if (!value)
		return(0);
	for (i=0; i < g->card->form->nitems; i++) {
		ITEM *item = g->card->form->items[i];
		if(IFL(item->,MULTICOL)) {
			int m;
			for(m = 0; m < item->nmenu; m++)
				if(item->menu[m].column == field.column) {
					if(item->menu[m].flagtext &&
					   !strcmp(value, item->menu[m].flagcode)) {
						ret = strdup(item->menu[m].flagtext);
						if(!ret)
							parsererror(g, "No memory for expand");
						return ret;
					}
					break;
				}
			if(m < item->nmenu)
				break;
		}
		if (item->column != field.column)
			continue;
		if ((item->type == IT_CHOICE || item->type == IT_FLAG) &&
		    item->flagcode &&
		    item->flagtext &&
		    !strcmp(value, item->flagcode)) {
			ret = strdup(item->flagtext);
			if(!ret)
				parsererror(g, "No memory for expand");
			return ret;
		}
		if (item->type == IT_FLAGS || item->type == IT_MULTI) {
			char sep, esc;
			int m;
			int qbegin, qafter = -1;
			char *v = NULL;
			int nv = 0;
			get_form_arraysep(g->card->form, &sep, &esc);
			for(m = 0; m < item->nmenu; m++) {
				if(find_unesc_elt(value, item->menu[m].flagcode,
						  &qbegin, &qafter, sep, esc)) {
					char *e = BLANK(item->menu[m].flagtext) ?
						item->menu[m].flagcode :
						item->menu[m].flagtext;
					if(!set_elt(&v, nv++, e, g->card->form)) {
						parsererror(g, "No memory for expand");
						return NULL;
					}
					if(v == e) {
						v = strdup(v);
						if(!v) {
							parsererror(g, "No memory for expand");
							return NULL;
						}
					}
				}
			}
			return v;
		}
		if (item->type == IT_MENU || item->type == IT_RADIO) {
			int m;
			for(m = 0; m < item->nmenu; m++)
				if(!strcmp(value, item->menu[m].flagcode))
					break;
			if(m < item->nmenu && item->menu[m].flagtext) {
				ret = strdup(item->menu[m].flagtext);
				if(!ret)
					parsererror(g, "No memory for expand");
				return ret;
			}
			break;
		}
	}
	ret = strdup(value);
	if(!ret)
		parsererror(g, "No memory for expand");
	return ret;
}


/*
 * store a string in the database
 */

char *f_assign(
	UNUSED PG,
	fkey_field	field,
	char		*data)
{
	const char *v = data;

	/* following used to be handled in write_dbase() */
	/* but I don't want internal rep different from external */
	for (int i = field.card->form->nitems - 1; i >= 0; i--) {
		const ITEM *item = field.card->form->items[i];
		if (item->type == IT_TIME && item->column == field.column)
			v = format_time_data(atol(data), item->timefmt);
	}
	dbase_put(field.card->dbase, field.row, field.column, v);
	return(data);
}


/*
 * return the section of a card in the database
 */

int f_section(
	PG,
	int		nrow)
{
	ROW		*row;		/* row to get from */

	if (g->card && g->card->dbase
		   && nrow >= 0
		   && nrow < g->card->dbase->nrows
		   && (row = g->card->dbase->row[nrow]))
		return(row->section);
	else if (g->card && g->card->dbase && !g->card->dbase->nrows)
		return(g->card->dbase->currsect);
	return(0);
}


/*
 * run a system command, and return stdout of the command (backquotes)
 */

char *f_system(
	PG,
	char		*cmd)
{
	char		*tpath;
	FILE		*fp;
	char		*data;
	char		*p;
	int		size, i;
	QProcess	proc;

	if (!cmd || !*cmd) {
		zfree(cmd);
		return(0);
	}
	/* FIXME: use something other than getpid() */
	/* can't use tempnam() because headless security chickens think that */
	/* it's a bad idea, even when security is not an issue */
	tpath = qstrdup(qsprintf(P_tmpdir "/grok%05dxxx", getpid()));
	p = tpath + strlen(tpath) - 3;
	strcpy(p, "out");
	proc.setStandardOutputFile(tpath);
	strcpy(p, "err");
	proc.setStandardErrorFile(tpath);
	proc.setStandardInputFile(QProcess::nullDevice());
#ifdef Q_OS_UNIX
	/* QProcess doesn't understand UNIX shell syntax */
	/* Not even a basic parser like what glib has */
	const char *shell = getenv("SHELL");
	if(!shell || !*shell)
		shell = "/bin/sh";
	proc.start(shell, { "-c", cmd });
#else
	proc.start(cmd);
#endif
	proc.waitForFinished(-1);
							/* error messages */
	if ((fp = fopen(tpath, "r"))) {
		(void)fseek(fp, 0, 2);
		if ((size = ftell(fp))) {
			const char msg[] = "Command failed: %s\n";
			rewind(fp);
			/* no need for error message to be unlimited */
			if(size > 4096)
				size = 4096;
			data = (char *)malloc(sizeof(msg) - 2 + strlen(cmd) + size);
			if(!data) {
				parsererror(g, "No memory for command error message");
				free(cmd);
				unlink(tpath);
				strcpy(p, "out");
				unlink(tpath);
				free(tpath);
				return 0;
			}
			i = sprintf(data, msg, cmd);
			size = fread(data+i, 1, size, fp);
			data[i + size] = 0;
			create_error_popup(mainwindow, 0, data);
			free(data);
		}
		fclose(fp);
	}
	unlink(tpath);
	free(cmd);
							/* result */
	data = NULL;
	strcpy(p, "out");
	if ((fp = fopen(tpath, "r"))) {
		(void)fseek(fp, 0, 2);
		if ((size = ftell(fp))) {
			rewind(fp);
			data = (char *)malloc(size + 1);
			if(!data) {
				parsererror(g, "No memory for command output");
				unlink(tpath);
				free(tpath);
				return 0;
			}
			size = fread(data, 1, size, fp);
			if(size)
				data[size] = 0;
			else {
				free(data);
				data = NULL;
			}
		}
		fclose(fp);
	}
	unlink(tpath);
	free(tpath);
	return(data);
}
/*
 * transliterate characters in a string according to the rules in the
 * translation string, which contains x=y pairs like SUBST strings.
 */

char *f_tr(
	PG,
	char		*string,
	char		*rules)
{
	if(!string) {
		zfree(rules);
		return string;
	}
	int		i, len = 0, max = 1024;
	char		*ret = (char *)malloc(max);
	char		**array = (char **)malloc(256 * sizeof(char *));
	const char	*err, *str = string;

	if (!ret || !array) {
		zfree(ret);
		zfree(array);
		zfree(rules);
		parsererror(g, "No memory for tr table");
		return(string);
	}
	memset(array, 0, 256 * sizeof(char *));
	*ret = 0;
	backslash_subst(rules);
	if (!(err = substitute_setup(array, rules))) {
		for (i=0; i < 256; i++)
			zfree(array[i]);
		free(ret);
		free(array);
		zfree(rules);
		parsererror(g, err);
		return(string);
	}
	while (*str) {
		char *pret = ret;
		i = array[(unsigned char)*str] ? strlen(array[(unsigned char)*str]) : 1;
		if (len+i >= max && !(ret = (char *)realloc(ret, max += max/2))) {
			free(pret);
			parsererror(g, "No memory for tr result");
			break;
		}
		if (array[(unsigned char)*str]) {
			strcpy(ret, array[(unsigned char)*str++]);
			ret += i;
		} else
			*ret++ = *str++;
	}
	if(ret)
		*ret = 0;
	for (i=0; i < 256; i++)
		zfree(array[i]);
	free(array);
	free(string);
	zfree(rules);
	return(ret);
}


/*
 * cut num chars at pos from string
 */

char *f_substr(
	char		*string,
	int		pos,
	int		num)
{
	int		len;

	if (!string)
		return(0);
	if (num <= 0) {
		free(string);
		return NULL;
	}
	len = strlen(string);
	if (pos < 0)
		pos = len + pos;
	if (pos < 0)
		pos = 0;
	if (pos < len) {
		if(num > len - pos)
			num = len - pos;
		memmove(string, string + pos, num);
		string[num] = 0;
		return string;
	}
	free(string);
	return NULL;
}


/*
 * s is a set of lines separated by lsep (just \n if NULL)
 * strip away characters beyond w in each line
 * strip away lines > h
 */
char *f_trunc2d(char *s, int w, int h, char *lsep)
{
	char *ret = s;
	if(h <= 0 || w <= 0 || BLANK(s)) {
		zfree(s);
		zfree(lsep);
		return 0;
	}
	const char *sep = lsep ? lsep : "\n";
	while(1) {
		char *nl = strstr(s, sep);
		if (!nl)
			nl = s + strlen(s);
		if (!--h)
			*nl = 0;
		if(nl - s > w) {
			memmove(s + w, nl, strlen(nl) + 1);
			nl = s + w;
		}
		if(!*nl) {
			zfree(lsep);
			return ret;
		}
		s = nl + strlen(sep);
	}
}


/*
 * return true if <match> is contained in <string>
 */

bool f_instr(
	char		*match,
	char		*src)
{
	int		i;
	char		*string = src;

	if (!match || !*match) {
		zfree(match);
		zfree(string);
		return(true);
	}
	if (!string) {
		free(match);
		return(false);
	}
	for (; *string; string++)
		for (i=0; ; i++) {
			if (!match[i]) {
				free(match);
				free(src);
				return(true);
			}
			if (match[i] != string[i])
				break;
		}
	free(match);
	free(src);
	return(false);
}


/*
 * compose an argument list, by adding a new string argument to an existing
 * list of string arguments. The result is used and freed by f_printf().
 */

struct arg *f_addarg(
	PG,
	struct arg	*list,		/* easier to keep struct arg local */
	char		*value)		/* argument to append to list */
{
	struct arg	*newa = (struct arg *)malloc(sizeof(struct arg));
	struct arg	*tail;

	if(!newa) {
		parsererror(g, "No memory for arg");
		free_args(list);
		zfree(value);
		return NULL;
	}
	for (tail=list; tail && tail->next; tail=tail->next);
	if (tail)
		tail->next = newa;
	if (newa) {
		newa->next  = 0;
		newa->value = value;
	}
	return(list ? list : newa);
}


/*
 * sprintf to allocated memory, and return a pointer to the resulting string.
 * Requires an argument list, which is freed when finished. What makes this a
 * bit difficult is that sprintf expects different arg data types for different
 * % controls. Only the final % control char is used for determining the type,
 * the actual printing is still done by printf. This will blow up if someone
 * says "%100000s".
 */

void free_args(
	struct arg	*arg)
{
	struct arg	*t;

	while(arg) {
		t = arg->next;
		zfree(arg->value);
		free(arg);
		arg = t;
	}
}

char *f_printf(
	PG,
	struct arg	*arg)
{
	struct arg	*argp;		/* next argument */
	const char	*value;		/* value string from next argument */
	char		*buf;		/* result buffer */
	size_t		buflen;
	int		bp = 0;		/* next free char in result buffer */
	char		*fmt;		/* next char from format string */
	char		*ctl;		/* for scanning % controls */
	long		lval;
	double		dval;
	bool		islong;

	if (!arg)
		return(0);
	argp = arg->next;
	fmt  = arg->value;
	if (!fmt || !*fmt) {
		free_args(arg);
		return(0);
	}
	buf = (char *)malloc((buflen = 32));
	if(!buf) {
		free_args(arg);
		parsererror(g, "No memory for printf result");
		return(0);
	}
#define check_buf(n) do { \
	int l = n; \
	if(bp + l + 1 >= (int)buflen) { \
		char *obuf = buf; \
		while((int)(buflen *= 2) <= bp + l + 1); \
		if(!(buf = (char *)realloc(buf, buflen))) { \
			parsererror(g, "No memory for printf result"); \
			free_args(arg); \
			free(obuf); \
			return(0); \
		} \
	} \
} while(0)
	while (*fmt) {
		if (*fmt == '\\' && fmt[1]) {
			fmt++;
			check_buf(1);
			buf[bp++] = *fmt++;
		} else if (*fmt != '%') {
			check_buf(1);
			buf[bp++] = *fmt++;
		} else {
			char ac = 0; /* char after c */
			int plen; /* print result length */
			/* I guess the lack of * is made up for the ease
			 * of just tacking it into the format string directly */
			/* I'll probably have to add %n$ support if I want
			 * to support translation in expressions, though */
			for (ctl=fmt+1; *ctl && strchr("0123456789.-", *ctl); ctl++);
			if ((islong = *ctl == 'l'))
				ctl++;
			if(*ctl) {
				ac = ctl[1];
				ctl[1] = 0;
			}
			value = argp && argp->value ? argp->value : "";
			switch(*ctl) {
			  /* C/lc are not really supported right now */
			  case 'c':
			  case 'd':
			  case 'x':
			  case 'X':
			  case 'o':
			  case 'i':
			  case 'u':
				lval = atol(value);
				plen = snprintf(0, 0, fmt, islong ? lval : (int)lval);
				check_buf(plen);
				sprintf(buf + bp, fmt, islong ? lval : (int)lval);
				break;
			  case 'a':
			  case 'A':
			  case 'e':
			  case 'E':
			  case 'f':
			  case 'F':
			  case 'g':
			  case 'G':
				dval = atof(value);
				plen = snprintf(0, 0, fmt, dval);
				check_buf(plen);
				sprintf(buf + bp, fmt, dval);
				break;
			  /* S/ls are not really supported right now */
			  case 's':
				plen = snprintf(0, 0, fmt, value);
				check_buf(plen);
				sprintf(buf + bp, fmt, value);
				break;
			  default:
				plen = strlen(fmt);
				check_buf(plen);
				strcat(buf + bp, fmt);
			}
			if (argp)
				argp = argp->next;
			bp += plen;
			if(*ctl) {
				fmt = ctl+1;
				*fmt = ac;
			} else
				fmt = ctl;
		}
	}
	free_args(arg);
	buf[bp] = 0;
	return buf;
}

// The following use QRegularExpression rather than POSIX EXTENDED as I
// usually prefer for portability.  QRegularExpression is "perl-compatible".

static bool re_check(PG, QRegularExpression &re, char *e)
{
    bool ret = re.isValid();
    if(!ret) {
	char *msg = qstrdup(re.errorString());
	parsererror(g, "Error in regular expression '%s': %s", e, STR(msg));
	zfree(msg);
    }
    zfree(e);
    return ret;
}

// Find e in s, returning offset in s + 1 (0 == no match).
// Both e and s are freed.
int f_re_match(PG, char *s, char *e)
{
    if(!e || !*e) {
	zfree(e);
	zfree(s);
	return 1;
    }
    QRegularExpression re(STR(e));
    if(!re_check(g, re, e)) {
	zfree(s);
	return 0;
    }
    QRegularExpressionMatch m = re.match(STR(s));
    zfree(s);
    return m.capturedStart() + 1;
}

// Replace e in s with r.  If all is true, advance and repeat while possible
// r can contain \0 .. \9 and \{n} for subexpression replacmeents
char *f_re_sub(PG, char *s, char *e, char *r, bool all)
{
    QRegularExpression re(STR(e));
    if(!re_check(g, re, e)) {
	zfree(r);
	return s;
    }
    QString res;
    QString str(STR(s));
    zfree(s);
    int off = 0;
    QRegularExpressionMatchIterator iter = re.globalMatch(str);
    while(iter.hasNext()) {
	const QRegularExpressionMatch &m = iter.next();
	if(off > m.capturedStart())
	    continue;
	if(off != m.capturedStart())
	    res.append(str.midRef(off, m.capturedStart() - off));
	QString rstr(STR(r));
	int roff = 0, bsloc;
	while((bsloc = rstr.indexOf('\\', roff)) >= 0) {
	    int expr = -1;
	    if(bsloc == rstr.size() - 1)
		break;
	    if(rstr[bsloc + 1].isDigit()) {
		expr = rstr[bsloc + 1].toLatin1() - '0';
		rstr.remove(bsloc, 2);
	    } else if(rstr[bsloc + 1] == '{') {
		int eloc = rstr.indexOf('}', bsloc + 2);
		if(eloc >= 0) {
		    expr = 0;
		    int i;
		    for(i = bsloc + 2; i < eloc; i++) {
			if(!rstr[i].isDigit()) {
			    expr = -1;
			    break;
			} else
			    expr = expr * 10 + rstr[i].toLatin1() - '0';
		    }
		}
		if(expr >= 0)
		    rstr.remove(bsloc, eloc - bsloc + 1);
	    }
	    if(expr < 0) {
		rstr.remove(bsloc, 1);
		roff = bsloc + 1;
	    } else {
		rstr.insert(bsloc, m.capturedRef(expr));
		roff = bsloc + m.capturedLength(expr);
	    }
	}
	res.append(rstr);
	off = m.capturedEnd();
	if(!all)
	    break;
    }
    res.append(str.midRef(off));
    zfree(r);
    return qstrdup(res);
}

/* get parser's current form's separator information */
#define get_cur_arraysep(sep, esc) \
	get_form_arraysep(g->card ? g->card->form : NULL, sep, esc);

int f_alen(PG, char *array)
{
    char sep, esc;
    int ret;
    get_cur_arraysep(&sep, &esc);
    ret = stralen(array, sep, esc);
    zfree(array);
    return ret;
}

char *f_elt(PG, char *array, int n)
{
    char sep, esc;
    int b, a;
    get_cur_arraysep(&sep, &esc);
    if(n < 0) {
	n += stralen(array, sep, esc);
	if(n < 0) {
	    zfree(array);
	    return NULL;
	}
    }
    elt_at(array, n, &b, &a, sep, esc);
    if(b == a) {
	zfree(array);
	return NULL;
    }
    *unescape(array, array + b, a - b, esc) = 0;
    return array;
}

char *f_slice(
	PG,
	char		*array,
	int		start,
	int		end)
{
    char sep, esc;
    int bs, t, ae;
    if(!array)
	return array;
    get_cur_arraysep(&sep, &esc);
    int alen = stralen(array, sep, esc);
    if(start < 0)
	start += alen;
    if(start < 0)
	start = 0;
    if(end < 0)
	end += alen;
    if(end >= alen)
	end = alen - 1;
    if(start > end) {
	zfree(array);
	return NULL;
    }
    elt_at(array, start, &bs, &t, sep, esc);
    if(start == end)
	ae = t;
    else
	elt_at(array + t + 1, end - start - 1, &t, &ae, sep, esc);
    memmove(array, array + bs, ae - bs);
    array[ae - bs] = 0;
    return array;
}

char *f_astrip(
	PG,
	char		*array)
{
    if(!array)
	return array;
    int alen = strlen(array);
    char sep, esc;
    get_cur_arraysep(&sep, &esc);
    // strip off trailing empties except last if esc seen
    while(alen && array[alen - 1] == sep &&
	  (alen < 2 || array[alen - 2] != esc))
	array[--alen] = 0;
    /* if the last one was maybe escaped, see if it was */
    if(alen > 2 && array[alen - 1] == sep) {
	int nesc;
	for(nesc = 1; nesc < alen - 1; nesc++)
	    if(array[alen - nesc - 2] != esc)
		break;
	/* odd is escaped, even is not */
	if(!(nesc % 2))
	    array[--alen] = 0;
    }
    if(!alen) {
	free(array);
	return NULL;
    }
    return array;
}

char *f_setelt(PG, char *array, int n, char *val)
{
    if(!set_elt(&array, n, val, g->card->form))
	parsererror(g, "No memory");
    if(val && val != array)
	free(val);
    return array;
}

void f_foreachelt(
	PG,
	int		var,
	char		*array,
	char		*cond,
	char		*expr,
	bool		nonblank)
{
    if(expr) {
	char toesc[3];
	char &sep = toesc[1], &esc = toesc[0];
	int begin, after;

	get_cur_arraysep(&sep, &esc);
	toesc[2] = 0;
	after = -1;
	while(1) {
	    next_aelt(array, &begin, &after, sep, esc);
	    if(after < 0)
		break;
	    if(begin == after && nonblank)
		continue;
	    if(begin == after)
		set_var(g->card, var, NULL);
	    else {
		char c = array[after], *s;
		*unescape(array + begin, array + begin, after - begin, esc) = 0;
		s = strdup(array + begin);
		if(!s) {
			parsererror(g, "No memory for loop variable");
			break;
		}
		set_var(g->card, var, s);
		// no need to re-escape and re-store value
		array[after] = c;
	    }
	    if (!cond || subevalbool(g, cond))
		subeval(g, expr); /* no-op if evalbool failed */
	    if (eval_error)
		break;
	}
    }
    zfree(array);
    zfree(cond);
    zfree(expr);
}

char *f_esc(PG, char *s, char *e)
{
    if(!s || !*s) {
	zfree(e);
	return s;
    }
    // NULL e is signal to use the defaults
    char defesc[3];
    if(!e) {
	get_cur_arraysep(defesc + 1, defesc);
	defesc[2] = 0;
    }
    int exp = countchars(s, e ? e : defesc);
    if(!exp) {
	zfree(e);
	return s;
    }
    char *ret = (char *)malloc(strlen(s) + exp + 1);
    if(!ret) {
	parsererror(g, "No memory");
	zfree(e);
	return s;
    }
    *escape(ret, s, -1, e ? e[0] : defesc[0], e ? e + 1 : defesc + 1) = 0;
    free(s);
    zfree(e);
    return ret;
}

char *f_toset(PG, char *a)
{
    char sep, esc;
    get_cur_arraysep(&sep, &esc);
    if(!toset(a, sep, esc))
	parsererror(g, "No memory for set conversion");
    return a;
}

char *f_union(PG, char *a, char *b)
{
    if(!a)
	return b;
    if(!b)
	return a;
    // OK, I'm being lazy/expensive for this one.  Just append and toset()
    // Inserting b an element at a time might be faster.
    char sep, esc;
    get_cur_arraysep(&sep, &esc);
    int alen = strlen(a), blen = strlen(b);
    char *oa = a;
    a = (char *)realloc(a, alen + blen + 2);
    if(!a) {
	parsererror(g, "No memory");
	free(oa);
	free(b);
	return a;
    }
    memcpy(a + alen + 1, b, blen + 1);
    free(b);
    a[alen] = sep;
    toset(a, sep, esc);
    return a;
}

static int del_elt(char *a, int len, int begin, int after)
{
    if(a[after])
	memmove(a + begin, a + after + 1, len - after);
    len -= after - begin;
    if(len) // only if there were >1 items in a
	--len; // removed separator
    a[len] = 0;
    return len;
}

char *f_intersect(PG, char *a, char *b)
{
    if(!a || !*a || !b || !*b) {
	zfree(a);
	zfree(b);
	return NULL;
    }
    char sep, esc;
    get_cur_arraysep(&sep, &esc);
    int begin, after = -1, blen = strlen(b);
    while(1) {
	int fbegin, fafter;
	next_aelt(b, &begin, &after, sep, esc);
	if(after < 0)
	    break;
	if(begin == after)
	    continue;
	if(!find_elt(a, b + begin, after - begin, &fbegin, &fafter, sep, esc)) {
	    blen = del_elt(b, blen, begin, after);
	    after = begin - 1; // note this may make it -1, so can't use do-while
	}
    }
    free(a);
    return b;
}

char *f_setdiff(PG, char *a, char *b)
{
    if(!b || !*b)
	return a;
    if(!a || !*a) {
	zfree(b);
	return a;
    }
    char sep, esc;
    get_cur_arraysep(&sep, &esc);
    int begin, after = -1, alen = strlen(a);
    do {
	int fbegin, fafter;
	next_aelt(b, &begin, &after, sep, esc);
	if(begin == after)
	    continue;
	if(find_elt(a, b + begin, after - begin, &fbegin, &fafter, sep, esc)) {
	    alen = del_elt(a, alen, fbegin, fafter);
	    if(!alen)
		break;
	}
    } while(after >= 0);
    free(b);
    if(!alen) {
	free(a);
	return NULL;
    }
    return a;
}

char *f_detab(PG, char *s, int start, int tabstop)
{
	char *d, *p, *q;
	int ntab = countchars(s, "\t");
	if(!ntab)
		return s;
	if(tabstop < 1)
		tabstop = 1;
	if(start < 0)
		start = tabstop + start % tabstop;
	d = (char *)malloc(strlen(s) + ntab * tabstop + 1);
	if(!d) {
		free(s);
		parsererror(g, "No memory");
		return d;
	}
	for(p = s, q = d; *p; p++, start++) {
		if(*p != '\t')
			*q++ = *p;
		else {
			int i = tabstop - (start % tabstop);
			while(i--) {
				*q++ = ' ';
				start++;
			}
			start--;
		}
	}
	*q = 0;
	free(s);
	return d;
}

char *f_align(PG, char *s, char *pad, int len, int where)
{
	int slen = s ? strlen(s) : 0;
	char *ns;
	char padc = BLANK(pad) ? ' ' : *pad;
	zfree(pad);
	if(slen == len)
		return s;
	if(slen > len) {
		if(len <= 0) {
			zfree(s);
			return NULL;
		}
		if(where > 0)
			memmove(s, s + slen - len, len);
		else if(!where)
			memmove(s, s + (slen - len) / 2, len);
		s[len] = 0;
		return s;
	}
	ns = (char *)malloc(len + 1);
	if(!ns) {
		zfree(s);
		parsererror(g, "No memory");
		return NULL;
	}
	if(where > 0) {
		memset(ns, padc, len - slen);
		memcpy(ns + len - slen, s, slen);
	} else if(where < 0) {
		memset(ns + slen, padc, len - slen);
		memcpy(ns, s, slen);
	} else {
		memset(ns, padc, (len - slen) / 2);
		memcpy(ns + (len - slen) / 2, s, slen);
		memset(ns + slen + (len - slen) / 2, padc, (len - slen + 1) / 2);
	}
	ns[len] = 0;
	zfree(s);
	return ns;
}

CARD *f_db_start(
	PG,
	char		*formname,
	char		*search,
	struct db_sort	*sort)
{
	CARD  *ocard = g->card;
	FORM  *form = NULL;
	DBASE *dbase = NULL;

	if (BLANK(formname))
		form = g->card->form;
	else {
		/* make relative to current form first if relative */
		const char *lasts, *opath = g->card->form->path;
		if(!strchr(formname, '/') && (lasts = strrchr(opath, '/'))) {
			char *fname = (char *)malloc((lasts - opath) + strlen(formname) + 5);
			if(!fname) {
				parsererror(g, "No memory for form name");
				zfree(formname);
				zfree(search);
				free_db_sort(sort);
				return NULL;
			}
			memcpy(fname, opath, lasts - opath + 1);
			strcpy(fname + (lasts - opath + 1), formname);
			if(!(lasts = strrchr(formname, '.')) || strcmp(lasts, ".gf"))
				strcat(fname, ".gf");
			if(!access(fname, R_OK))
				form = read_form(fname);
			free(fname);
		}
		if(!form)
			form = read_form(formname, false, 0);
	}
	if (form)
		dbase = read_dbase(form);
	else {
		parsererror(g, "Can't find form %s", STR(formname));
		zfree(formname);
		zfree(search);
		free_db_sort(sort);
		return NULL;
	}
	zfree(formname);
	if(!dbase)
		dbase = dbase_create(form);
	form->dbpath = dbase->path;
	g->card = create_card_menu(form, dbase);
	g->card->prev_form = zstrdup(ocard->form->name);
	g->card->last_query = -1;
	/* searching may be by expr, so save state */
	PARSE_GLOBALS pgs = *g;
	g->ret = 0;
	/* FIXME:  cache this somehow, so it doesn't get run too often */
	if(search)
		query_any(SM_SEARCH, g->card, search);
	else if (form->autoquery >= 0 && form->autoquery < form->nqueries)
		query_any(SM_SEARCH, g->card, form->query[form->autoquery].query);
	else
		query_all(g->card);
	zfree(g->ret);
	*g = pgs;
	zfree(search);
	if(!sort) {
		for (int i=0; i < form->nitems; i++)
			if (IFL(form->items[i]->,DEFSORT)) {
				dbase_sort(g->card, form->items[i]->column, 0);
				break;
			}
	} else {
		/* since sorts are stable, sort from lowest to highest */
		/* list was created in reverse order already */
		bool did_sort = false;
		while(sort) {
			struct db_sort *ds = sort;
			sort = sort->next;
			const char *f = ds->field;
			int fn = -1;
			if(f && *f == '_')
				f++;
			if(f && isdigit(*f))
				fn = atoi(f);
			else if(f) {
				FIELDS *s = form->fields;
				auto it = s->find((char *)f);
				if(it != s->end()) {
					int i = it->second % form->nitems;
					int m = it->second / form->nitems;
					fn = IFL(form->items[i]->,MULTICOL) ?
						form->items[i]->menu[m].column :
						form->items[i]->column;
				}
			}
			if(fn == -1) {
				parsererror(g, "Invalid sort field '%s'", STR(ds->field));
				free_db_sort(sort);
				sort = NULL;
			} else {
				dbase_sort(g->card, fn, ds->dir < 0, did_sort);
				did_sort = true;
			}
			zfree(ds->field);
			free(ds);
		}
	}
	return ocard;
}

void f_db_end(
	PG,
	CARD		*card)
{
	FORM *form = g->card->form;
	DBASE *dbase = g->card->dbase;

	free_card(g->card);
	form_delete(form);
	dbase_delete(dbase);
	g->card = card;
}

struct db_sort *new_db_sort(
	PG,
	struct db_sort	*prev,
	int		dir,
	char		*field)
{
	struct db_sort *sort = (struct db_sort *)malloc(sizeof(*sort));

	if(!sort) {
		free_db_sort(prev);
		zfree(field);
		parsererror(g, "No memory for db switch");
		return NULL;
	}
	/* I actually want these in reverse order, since that's how I sort */
	sort->dir = dir;
	sort->field = field;
	sort->next = prev;
	return sort;
}

void free_db_sort(
	struct db_sort	*sort)
{
	while(sort) {
		struct db_sort *next = sort->next;
		zfree(sort->field);
		free(sort);
		sort = next;
	}
}

static void add_deref(const char *key, const FORM *form, ITEM *item,
		      char *&ret, size_t *retalloc, int &retlen,
		      const char *fsep, const char *rsep,
		      int name_prefix_off, int nplen)
{
	const FORM *fform = 0;
	DBASE *dbase = 0;
	if (key) {
		resolve_fkey_fields(item);
		if (!(fform = item->fkey_form))
			return;
		dbase = read_dbase(fform);
	}
	for (int keyno = 0; ; keyno++) {
		int r = 0;
		if (key) {
			r = fkey_lookup(dbase, form, item, key, keyno);
			if (r == -2)
				break;
			if (keyno > 0 && *rsep) {
				int rslen = strlen(rsep);
				grow(0, "deref", char, ret, retlen + rslen + 1, retalloc);
				memcpy(ret + retlen, rsep, rslen + 1);
				retlen += rslen;
			}
		} else {
			/* FIXME: esc(item->name, *fsep, *rsep) */
			/* also escape ' ' and '_' if needed */
			int nlen = strlen(item->name);
			grow(0, "dereff", char, ret, retlen + nlen + 3, retalloc);
			memcpy(ret + retlen, ret + name_prefix_off, nplen);
			if(nplen) {
				ret[retlen++] = ' ';
				nplen++;
			}
			ret[retlen++] = '_';
			memcpy(ret + retlen, item->name, nlen + 1);
			retlen += nlen;
			nplen += nlen + 1;
		}
		bool first = true;
		for (int i = 0; i < item->nfkey; i++) {
			const FKEY &fk = item->fkey[i];
			if (!fk.display)
				continue;
			ITEM *fit = fk.item;
			const char *value = fk.name;
			if(fit)
				value = dbase_get(dbase, r, fit->column);
			if (fit && fit->type == IT_FKEY)
				add_deref(value, fform, fit,
					  ret, retalloc, retlen,
					  fsep, rsep, name_prefix_off, nplen);
			else if (key) {
				if (!first && *fsep) {
					int fslen = strlen(fsep);
					grow(0, "deref", char, ret,
					     retlen + fslen + 1, retalloc);
					memcpy(ret + retlen, fsep, fslen + 1);
					retlen += fslen;
				}
				if (!BLANK(value)) {
					int vlen = strlen(value);
					grow(0, "deref", char, ret,
					     retlen + vlen + 1, retalloc);
					memcpy(ret + retlen, value, vlen + 1);
					retlen += vlen;
				}
			} else {
				/* FIXME: esc(fk.name, *fsep, *rsep) */
				/* also escape ' ' and '_' if needed */
				int nlen = strlen(fk.name);
				if (!first) {
					grow(0, "dreff", char, ret,
					     retlen + nplen + nlen + 3, retalloc);
					ret[retlen++] = *fsep;
					memcpy(ret + retlen, ret + name_prefix_off, nplen);
					retlen += nplen;
					ret[retlen++] = ' ';
				} else
					grow(0, "dreff", char, ret,
					     retlen + nlen + 2, retalloc);
				ret[retlen++] = '_';
				memcpy(ret + retlen, fk.name, nlen + 1);
			}
			first = false;
		}
	}
}

/* expand fkey reference to visible fields */
char *f_deref(
	UNUSED PG,
	fkey_field field,
	char *fsep,
	char *rsep)
{
	const char	*value = dbase_get(field.card->dbase, field.row, field.column);
	char		*ret = 0;
	int		retlen = 0;
	size_t		retalloc = 0;
	int		i;
	const FORM	*form = field.card->form;

	if (!value)
		return(0);
	for (i=0; i < form->nitems; i++) {
		ITEM *item = form->items[i];
		if (item->column != field.column || item->type != IT_FKEY)
			continue;
		add_deref(value, form, item, ret, &retalloc, retlen,
			  fsep ? fsep : " ", rsep ? rsep : "\n", 0, 0);
		zfree(fsep);
		zfree(rsep);
		return ret;
	}
	zfree(fsep);
	zfree(rsep);
	return zstrdup(value);
}

/* expand fkey reference to visible field names */
char *f_dereff(
	UNUSED PG,
	fkey_field field)
{
	char		*ret = 0;
	int		retlen = 0;
	size_t		retalloc = 0;
	int		i;
	const FORM	*form = field.card->form;
	char		sep, esc;

	get_form_arraysep(form, &sep, &esc);
	for (i=0; i < form->nitems; i++) {
		ITEM *item = form->items[i];
		if (!IN_DBASE(item->type))
			continue;
		if (item->type == IT_FKEY && item->column == field.column) {
			add_deref(0, form, item, ret, &retalloc, retlen,
				  &sep, &esc, 0, 0);
			return ret;
		}
		const MENU *m = 0;
		if (IFL(item->,MULTICOL)) {
			m = item->menu;
			for (int n = 0; n < item->nmenu; n++, m++) {
				if (m->column == field.column)
					break;
			}
			if (m == item->menu + item->nmenu)
				m = 0;
		}
		if (m || item->column == field.column) {
			int nlen = strlen(m ? m->name : item->name);
			grow(0, "dereff", char, ret, retlen + nlen + 3, &retalloc);
			if (retlen)
				ret[retlen++] = ' ';
			memcpy(ret + retlen, m ? m->name : item->name, nlen + 1);
			retlen += nlen;
			return ret;
		}
	}
	/* should never get here */
	return 0;
}

/* true if a known referrer refers to this row */
static bool check_referenced(const FORM *fform, const FORM *form, const DBASE *dbase, int row)
{
	DBASE *fdbase = read_dbase(fform);
	if(!fdbase)
		return false;
	bool ret = false;
	for(int i = 0; i < fform->nitems; i++) {
		if(fform->items[i]->type != IT_FKEY)
			continue;
		resolve_fkey_fields(fform->items[i]);
		if(fform->items[i]->fkey_form != form)
			continue;
		char *key = fkey_of(dbase, row, fform, fform->items[i]);
		if(!key)
			continue;
		ret = find_referrer(fform, fdbase, fform->items[i], key) >= 0;
		free(key);
		if(ret)
			break;
	}
	dbase_delete(fdbase);
	return ret;
}

bool f_referenced(
	PG,
	int row)
{
	if (!g->card || !g->card->form || !g->card->dbase || row < 0 ||
	    row >= g->card->dbase->nrows)
		return false;
	const FORM *form = g->card->form;
	for(int i = 0; i < form->nchild; i++) {
		FORM *fform = form->childform[i];
		if(!fform)
			fform = form->childform[i] = read_form(form->childname[i]);
		if(!fform)
			continue;
		bool ret = check_referenced(fform, form, g->card->dbase, row);
		if(ret)
			return true;
	}
	return false;
}
