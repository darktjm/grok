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
#include "parser_yacc.h"


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
	if (s) free(s);
	return(d);
}


/*
 * All these functions run over the entire database and calculate something
 * from a single column.
 */

double f_sum(				/* sum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	sum;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		sum += fnum(dbase_get(dbase, row, column));
	return(sum);
}


double f_avg(				/* average */
	register int	column)		/* number of column to average */
{
	register double	sum = f_sum(column);
	return(yycard->dbase->nrows ? sum / yycard->dbase->nrows : 0);
}


double f_dev(				/* standard deviation */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	sum, avg, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	avg = f_avg(column);
	for (sum=0, row=dbase->nrows-1; row >= 0; row--) {
		val = fnum(dbase_get(dbase, row, column)) - avg;
		sum += val * val;
	}
	return(sqrt(sum / dbase->nrows));
}


double f_min(				/* minimum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	min = 1e100, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	for (row=dbase->nrows-1; row >= 0; row--) {
		val = fnum(dbase_get(dbase, row, column));
		if (val < min)
			min = val;
	}
	return(min == 1e100 ? 0 : min);
}


double f_max(				/* maximum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	max = -1e100, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	for (row=dbase->nrows-1; row >= 0; row--) {
		val = fnum(dbase_get(dbase, row, column));
		if (val > max)
			max = val;
	}
	return(max == -1e100 ? 0 : max);
}


/*
 * All these functions run over a query or the entire database and calculate
 * something from a single column.
 */

double f_qsum(				/* sum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	sum = 0;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	if (!yycard->query)
		return(f_sum(column));
	for (row=0; row < yycard->nquery; row++)
		sum += fnum(dbase_get(dbase, yycard->query[row], column));
	return(sum);
}


double f_qavg(				/* average */
	register int	column)		/* number of column to average */
{
	register double	sum = f_qsum(column);
	register int	count;

	count = yycard->query ? yycard->nquery : yycard->dbase->nrows;
	return(count ? sum / count : 0);
}


double f_qdev(				/* standard deviation */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	sum = 0, avg, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	if (!yycard->query)
		return(f_dev(column));
	if (!yycard->nquery)
		return(0);
	avg = f_qavg(column);
	for (row=0; row < yycard->nquery; row++) {
		val = fnum(dbase_get(dbase, yycard->query[row], column)) - avg;
		sum += val * val;
	}
	return(sqrt(sum / yycard->nquery));
}


double f_qmin(				/* minimum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	min = 1e100, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	if (!yycard->query)
		return(f_min(column));
	for (row=0; row < yycard->nquery; row++) {
		val = fnum(dbase_get(dbase, yycard->query[row], column));
		if (val < min)
			min = val;
	}
	return(min == 1e100 ? 0 : min);
}


double f_qmax(				/* maximum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register double	max = -1e100, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	if (!yycard->query)
		return(f_max(column));
	for (row=0; row < yycard->nquery; row++) {
		val = fnum(dbase_get(dbase, yycard->query[row], column));
		if (val > max)
			max = val;
	}
	return(max == -1e100 ? 0 : max);
}


/*
 * All these functions run over the entire database and calculate something
 * from a single section.
 */

double f_ssum(				/* sum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register int	sect;
	register double	sum;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_sum(column));
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect)
			sum += fnum(dbase_get(dbase, row, column));
	return(sum);
}


double f_savg(				/* average */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register int	sect;
	register double	sum;
	register int	row, num=0;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_avg(column));
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			sum += fnum(dbase_get(dbase, row, column));
			num++;
		}
	return(num ? sum / sum : 0);
}


double f_sdev(				/* standard deviation */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register int	sect;
	register double	sum, avg, val;
	register int	row, num=0;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_dev(column));
	avg = f_savg(column);
	for (sum=0, row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			val = fnum(dbase_get(dbase, row, column)) - avg;
			sum += val * val;
			num++;
		}
	return(num ? sqrt(sum / num) : 0);
}


double f_smin(				/* minimum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register int	sect;
	register double	min = 1e100, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_min(column));
	for (row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			val = fnum(dbase_get(dbase, row, column));
			if (val < min)
				min = val;
		}
	return(min == 1e100 ? 0 : min);
}


double f_smax(				/* maximum */
	register int	column)		/* number of column to average */
{
	register DBASE	*dbase = yycard->dbase;
	register int	sect;
	register double	max = -1e100, val;
	register int	row;

	if (!dbase || column < 0)
		return(0);
	if ((sect = dbase->currsect) < 0)
		return(f_max(column));
	for (row=dbase->nrows-1; row >= 0; row--)
		if (dbase->row[row]->section == sect) {
			val = fnum(dbase_get(dbase, row, column));
			if (val > max)
				max = val;
		}
	return(max == -1e100 ? 0 : max);
}


/*
 * return the value of a database field
 */

char *f_field(
	int		column,
	int		row)
{
	return(mystrdup(dbase_get(yycard->dbase, row, column)));
}


/*
 * convert choice code to string, and flag to yes/no
 */

char *f_expand(
	int		column,
	int		row)
{
	char		*value = dbase_get(yycard->dbase, row, column);
	int		i;

	if (!value)
		return(0);
	for (i=0; i < yycard->form->nitems; i++) {
		ITEM *item = yycard->form->items[i];
		if(item->multicol) {
			int m;
			for(m = 0; m < item->nmenu; m++)
				if(item->menu[m].column == column) {
					if(item->menu[m].flagtext &&
					   !strcmp(value, item->menu[m].flagcode))
						return mystrdup(item->menu[m].flagtext);
					break;
				}
			if(m < item->nmenu)
				break;
		}
		if (item->column != column)
			continue;
		if ((item->type == IT_CHOICE || item->type == IT_FLAG) &&
		    item->flagcode &&
		    item->flagtext &&
		    !strcmp(value, item->flagcode))
			return(mystrdup(item->flagtext));
		if (item->type == IT_FLAGS || item->type == IT_MULTI) {
			char sep, esc;
			int m;
			int qbegin, qafter = -1;
			char *v = NULL;
			int nv = 0;
			get_form_arraysep(yycard->form, &sep, &esc);
			for(m = 0; m < item->nmenu; m++) {
				if(find_unesc_elt(value, item->menu[m].flagcode,
						  &qbegin, &qafter, sep, esc)) {
					char *e = BLANK(item->menu[m].flagtext) ?
						item->menu[m].flagcode :
						item->menu[m].flagtext;
					v = set_elt(v, nv++, e);
					if(v == e)
						v = strdup(v);
				}
			}
			return v;
		}
		if (item->type == IT_MENU || item->type == IT_RADIO) {
			int m;
			for(m = 0; m < item->nmenu; m++)
				if(!strcmp(value, item->menu[m].flagcode))
					break;
			if(m < item->nmenu && item->menu[m].flagtext)
				return mystrdup(item->menu[m].flagtext);
			break;
		}
	}
	return(mystrdup(value));
}


/*
 * store a string in the database
 */

char *f_assign(
	int		column,
	int		row,
	char		*data)
{
	dbase_put(yycard->dbase, row, column, data);
	return(data);
}


/*
 * return the section of a card in the database
 */

int f_section(
	int		nrow)
{
	register ROW	*row;		/* row to get from */

	if (yycard && yycard->dbase
		   && nrow >= 0
		   && nrow < yycard->dbase->nrows
		   && (row = yycard->dbase->row[nrow]))
		return(row->section);
	return(0);
}


/*
 * run a system command, and return stdout of the command (backquotes)
 */

char *f_system(
	char		*cmd)
{
	char		outpath[80];
	char		errpath[80];
	int		fd0_save = dup(0);
	int		fd1_save = dup(1);
	int		fd2_save = dup(2);
	FILE		*fp;
	char		data[32768];
	int		size, i;

	if (!cmd || !*cmd)
		return(0);
	close(1);					/* run cmd */
	sprintf(outpath, "/tmp/grok%05dout", getpid());
	(void)open(outpath, O_WRONLY | O_CREAT, 0600);
	close(2);
	sprintf(errpath, "/tmp/grok%05derr", getpid());
	(void)open(errpath, O_WRONLY | O_CREAT, 0600);
	close(0);
	(void)open("/dev/null", O_RDONLY);
	int UNUSED ret = system(cmd);
	dup2(fd0_save, 0);
	dup2(fd1_save, 1);
	dup2(fd2_save, 2);
	close(fd0_save); /* added Paul van Slobbe bug fix */
	close(fd1_save);
	close(fd2_save);
							/* error messages */
	if ((fp = fopen(errpath, "r"))) {
		(void)fseek(fp, 0, 2);
		if ((size = ftell(fp))) {
			rewind(fp);
			sprintf(data, "Command failed: %s\n", cmd);
			i = strlen(data);
			if (i + size > (int)sizeof(data)-1)
				size = (int)sizeof(data)-1 - i;
			size = fread(data+i, 1, size, fp);
			data[i + size] = 0;
			create_error_popup(mainwindow, 0, data);
		}
		fclose(fp);
	}
							/* result */
	*data = 0;
	if ((fp = fopen(outpath, "r"))) {
		(void)fseek(fp, 0, 2);
		if ((size = ftell(fp))) {
			rewind(fp);
			if (size > (int)sizeof(data)-1)
				size = sizeof(data)-1;
			size = fread(data, 1, size, fp);
			data[size] = 0;
		}
		fclose(fp);
	}
	unlink(outpath);
	unlink(errpath);
	free((void *)cmd);
	return(*data ? mystrdup(data) : 0);
}
/*
 * transliterate characters in a string according to the rules in the
 * translation string, which contains x=y pairs like SUBST strings.
 */

#define FREE(p) if(p) free(p)

char *f_tr(
	char		*string,
	char		*rules)
{
	int		i, len = 0, max = 1024;
	char		*ret = (char *)malloc(max);
	char		**array = (char **)malloc(256 * sizeof(char *));
	const char	*err;

	if (!ret || !array) {
		FREE(ret);
		FREE(array);
		FREE(rules);
		create_error_popup(mainwindow, errno, "tr");
		return(string);
	}
	memset(array, 0, 256 * sizeof(char *));
	*ret = 0;
	if (!(err = substitute_setup(array, rules))) {
		FREE(ret);
		FREE(array);
		FREE(rules);
		create_error_popup(mainwindow, 0, err);
		return(string);
	}
	while (*string) {
		i = array[(unsigned char)*string] ? strlen(array[(unsigned char)*string]) : 1;
		if (len+i >= max && !(ret = (char *)realloc(ret, max += max/2))) {
			create_error_popup(mainwindow, errno, "tr");
			break;
		}
		if (array[(unsigned char)*string]) {
			strcpy(ret, array[(unsigned char)*string++]);
			ret += i;
		} else
			*ret++ = *string++;
	}
	*ret = 0;
	for (i=0; i < 256; i++)
		FREE(array[i]);
	FREE(array);
	FREE(string);
	FREE(rules);
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
	char		*news = 0;

	if (!string)
		return(0);
	len = strlen(string);
	if (pos < 0)
		pos = len + pos;
	if (pos < 0)
		pos = 0;
	if (pos < len) {
		if (num > len - pos)
			num = len - pos;
		if (num > 0 && (news = (char *)malloc(num + 1))) {
			strncpy(news, string+pos, num);
			news[num] = 0;
		}
	}
	free((void *)string);
	return(news);
}


/*
 * return true if <match> is contained in <string>
 */

BOOL f_instr(
	register char	*match,
	char		*src)
{
	register int	i;
	register char	*string = src;

	if (!match || !*match) {
		if (match)
			free((void *)match);
		if (string)
			free((void *)string);
		return(TRUE);
	}
	if (!string) {
		free((void *)match);
		return(FALSE);
	}
	for (; *string; string++)
		for (i=0; ; i++) {
			if (!match[i]) {
				free((void *)match);
				free((void *)src);
				return(TRUE);
			}
			if (match[i] != string[i])
				break;
		}
	free((void *)match);
	free((void *)src);
	return(FALSE);
}


/*
 * compose an argument list, by adding a new string argument to an existing
 * list of string arguments. The result is used and freed by f_printf().
 */

struct arg *f_addarg(
	struct arg	*list,		/* easier to keep struct arg local */
	char		*value)		/* argument to append to list */
{
	struct arg	*newa = (struct arg *)malloc(sizeof(struct arg));
	struct arg	*tail;

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

static void free_args(
	struct arg	*arg)
{
	struct arg	*t;

	while(arg) {
		t = arg->next;
		if(arg->value)
			free(arg->value);
		free(arg);
		arg = t;
	}
}

char *f_printf(
	struct arg	*arg)
{
	struct arg	*argp;		/* next argument */
	const char	*value;		/* value string from next argument */
	char		buf[10240];	/* result buffer */
	char		*bp = buf;	/* next free char in result buffer */
	char		*fmt;		/* next char from format string */
	char		*ctl;		/* for scanning % controls */
	char		cbuf[100];	/* control substring, [0] is '%' */
	int		i;

	if (!arg)
		return(0);
	argp = arg->next;
	fmt  = arg->value;
	if (!fmt || !*fmt) {
		free_args(arg);
		return(0);
	}
	while (*fmt) {
		if (*fmt == '\\' && fmt[1]) {
			fmt++;
			*bp++ = *fmt++;
		} else if (*fmt != '%') {
			*bp++ = *fmt++;
		} else {
			for (ctl=fmt+1; strrchr("0123456789.-", *ctl); ctl++);
			if (*ctl == 'l')
				ctl++;
			i = ctl - fmt + 1;
			if (i > (int)sizeof(cbuf)-1)
				i = sizeof(cbuf)-1;
			strncpy(cbuf, fmt, i);
			cbuf[i] = 0;
			value = argp && argp->value ? argp->value : "";
			switch(*ctl) {
			  case 'c':
			  case 'd':
			  case 'x':
			  case 'X':
			  case 'o':
			  case 'i':
			  case 'u':
				sprintf(bp, cbuf, atoi(value));
				break;
			  case 'e':
			  case 'E':
			  case 'f':
			  case 'F':
			  case 'g':
			  case 'G':
				sprintf(bp, cbuf, atof(value));
				break;
			  case 's':
				sprintf(bp, cbuf, value);
				break;
			  default:
				strcat(bp, cbuf);
			}
			if (argp)
				argp = argp->next;
			bp += strlen(bp);
			fmt = ctl+1;
		}
	}
	*bp = 0;
	return(mystrdup(buf));
}

// The following use QRegularExpression rather than POSIX EXTENDED as I
// usually prefer for portability.  QRegularExpression is "perl-compatible".

bool re_check(QRegularExpression &re, char *e)
{
    bool ret = re.isValid();
    if(!ret) {
	char *msg = qstrdup(QString("Error in regular expression '") +
			    QString(STR(e)) + QString("': ") +
			    re.errorString());
	parsererror(msg);
	free(msg);
    }
    if(e)
	free(e);
    return ret;
}

// Find e in s, returning offset in s + 1 (0 == no match).
// Both e and s are freed.
int re_match(char *s, char *e)
{
    if(!e || !*e) {
	if(e)
	    free(e);
	if(s)
	    free(s);
	return 1;
    }
    QRegularExpression re(STR(e));
    if(!re_check(re, e)) {
	if(s)
	    free(s);
	return 0;
    }
    QRegularExpressionMatch m = re.match(STR(s));
    if(s)
	free(s);
    return m.capturedStart() + 1;
}

// Replace e in s with r.  If all is true, advance and repeat while possible
// r can contain \0 .. \9 and \{n} for subexpression replacmeents
char *re_sub(char *s, char *e, char *r, bool all)
{
    QRegularExpression re(STR(e));
    if(!re_check(re, e)) {
	if(r)
	    free(r);
	return s;
    }
    QString res;
    QString str(STR(s));
    if(s)
	free(s);
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
	off = m.capturedEnd() + 1;
	if(!all)
	    break;
    }
    res.append(str.midRef(off));
    if(r)
	free(r);
    return qstrdup(res);
}

/* get parser's current form's separator information */
#define get_cur_arraysep(sep, esc) \
	get_form_arraysep(yycard ? yycard->form : NULL, sep, esc);

void get_form_arraysep(FORM *form, char *sep, char *esc)
{
    *sep = '|';
    *esc = '\\';
    if(!form)
	return;
    if(form->asep)
	*sep = form->asep;
    if(form->aesc)
	*esc = form->aesc;
}

/* Starts search at after + 1;  Returns -1 for begin & after @ end */
void next_aelt(char *array, int *begin, int *after, char sep, char esc)
{
    if(!array || !*array) {
	if(*after == -1)
	    *begin = *after = 0;
	else
	    *begin = *after = -1;
	return;
    }
    if(*after >= 0 && !array[*after]) {
	*begin = *after = -1;
	return;
    }
    *begin = ++*after;
    while(array[*after] && array[*after] != sep) {
	if(array[*after] == esc && array[*after + 1])
	    ++*after;
	++*after;
    }
}

int stralen(char *array, char sep, char esc)
{
    int ret = 0, b, a = -1;
    do {
	next_aelt(array, &b, &a, sep, esc);
	++ret;
    } while(a >= 0);
    return ret - 1;
}

int f_alen(char *array)
{
    char sep, esc;
    int ret;
    get_cur_arraysep(&sep, &esc);
    ret = stralen(array, sep, esc);
    if(array)
	free(array);
    return ret;
}

void elt_at(char *array, int n, int *begin, int *after, char sep, char esc)
{
    *after = -1;
    do {
	next_aelt(array, begin, after, sep, esc);
	if(!n--)
	    return;
    } while(*after >= 0);
}

char *unescape(char *d, const char *s, int len, char esc)
{
    if(len < 0)
	len = strlen(s);
    while(len--) {
	if(*s == esc && len) {
	    s++;
	    len--;
	}
	*d++ = *s++;
    }
    return d;
}

char *f_elt(char *array, int n)
{
    char sep, esc;
    int b, a;
    get_cur_arraysep(&sep, &esc);
    elt_at(array, n, &b, &a, sep, esc);
    if(b == a) {
	if(array)
	    free(array);
	return NULL;
    }
    *unescape(array, array + b, a - b, esc) = 0;
    return array;
}

int countchars(const char *s, const char *c)
{
    int len = 0;
    if(!s)
	return 0;
    while(*s)
	if(strchr(c, *s++))
	    len++;
    return len;
}

char *escape(char *d, const char *s, int len, char esc, const char *toesc)
{
    if(len < 0)
	len = strlen(s);
    while(len--) {
	if(*s == esc || strchr(toesc, *s))
	    *d++ = esc;
	*d++ = *s++;
    }
    return d;
}

// modifies array in-place; assumes array has been malloc'd
char *set_elt(char *array, int n, char *val)
{
    int b, a, vlen = val ? strlen(val) : 0, alen = array ? strlen(array) : 0;
    char toesc[3];
    char &sep = toesc[1], &esc = toesc[0];
    int vesc;
    get_cur_arraysep(&sep, &esc);
    toesc[2] = 0;
    if(n < 0)
	n = stralen(array, sep, esc);
    vesc = countchars(val, toesc);
    elt_at(array, n, &b, &a, sep, esc);
    if(a >= 0) { // replace
	if(vlen + vesc == a - b)
	    escape(array + b, val, vlen, esc, toesc + 1);
	else if(vlen + vesc < a - b) {
	    memmove(array + b + vlen + vesc, array + a, alen - a + 1);
	    escape(array + b, val, vlen, esc, toesc + 1);
	} else if(!alen) {
	    if(array)
		free(array);
	    array = val;
	    val = NULL;
	} else {
	    array = (char *)realloc(array, alen + vlen + vesc - (a - b) + 1);
	    memmove(array + b + vlen + vesc, array + a, alen - a + 1);
	    escape(array + b, val, vlen, esc, toesc + 1);
	}
	alen += vlen + vesc - (a - b);
	// strip off trailing empties
	while(alen && array[alen - 1] == sep)
	    array[--alen] = 0;
    } else if(vlen) { // add non-blank
	int l = stralen(array, sep, esc);
	array = (char *)realloc(array, alen + (n - l + 1) + vlen + vesc + 1);
	memset(array + alen, sep, n - l + 1);
	escape(array + alen + n - l + 1, val, vlen, esc, toesc + 1);
	array[alen + n - l + 1 + vlen + vesc] = 0;
    } else { // add blank
	// just in case this was built manually, strip off trailing empties
	while(alen && array[alen - 1] == sep)
	    array[--alen] = 0;
    }
    return array;
}

char *f_setelt(char *array, int n, char *val)
{
    array = set_elt(array, n, val);
    if(val && val != array)
	free(val);
    return array;
}

void f_foreachelt(
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
		set_var(var, NULL);
	    else {
		char c = array[after];
		*unescape(array + begin, array + begin, after - begin, esc) = 0;
		set_var(var, strdup(array + begin));
		// no need to re-escape and re-store value
		array[after] = c;
	    }
	    if (!cond || subevalbool(cond))
		subeval(expr);
	    if (eval_error())
		break;
	}
    }
    if(array)
	free(array);
    if(cond)
	free(cond);
    if(expr)
	free(expr);
}

char *f_esc(char *s, char *e)
{
    if(!s || !*s) {
	if(e)
	    free(e);
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
	if(e)
	    free(e);
	return s;
    }
    char *ret = (char *)malloc(strlen(s) + exp + 1);
    *escape(ret, s, -1, e ? e[0] : defesc[0], e ? e + 1 : defesc + 1) = 0;
    free(s);
    if(e)
	free(e);
    return ret;
}

struct aelt_loc {
    int b, a;
};

static const char *sort_array;

static int cmp_aelt(const void *_a, const void *_b)
{
    const struct aelt_loc *a = (struct aelt_loc *)_a;
    const struct aelt_loc *b = (struct aelt_loc *)_b;
    int alen = a->a - a->b;
    int blen = b->a - b->b;
    int c;
    if(!alen && !blen)
	return 0;
    c = memcmp(sort_array + a->b, sort_array + b->b, alen > blen ? blen : alen);
    if(c || alen == blen)
	return c;
    if(alen > blen)
	return 1;
    else
	return -1;
}

void toset(char *a, char sep, char esc)
{
    if(!a || !*a)
	return;
    int alen = stralen(a, sep, esc);
    if(alen == 1)
	return;
    struct aelt_loc *elts = (struct aelt_loc *)malloc(alen * sizeof(*elts)), *e;
    int begin, after = -1, i;
    for(e = elts, i = 0; i < alen; e++, i++) {
	next_aelt(a, &begin, &after, sep, esc);
	e->b = begin;
	e->a = after;
    }
    sort_array = a;
    qsort(elts, alen, sizeof(*elts), cmp_aelt);
    // ah, screw it.  Just make it anew, always
    char *set = (char *)malloc(strlen(a) + 1), *p = set;
    for(e = elts, i = 0; i < alen; e++, i++) {
	// remove blanks
	if(e->a == e->b)
	    continue;
	// remove dups
	if(i && e->a - e->b == e[-1].a - e[-1].b &&
	   !memcmp(a + e->b, a + e[-1].b, e->a - e->b))
	    continue;
	memcpy(p, a + e->b, e->a - e->b);
	p += e->a - e->b;
	*p++ = sep;
    }
    if(p == set)
	*p++ = 0;
    else
	p[-1] = 0;
    memcpy(a, set, p - set);
    free(set);
}

char *f_toset(char *a)
{
    char sep, esc;
    get_cur_arraysep(&sep, &esc);
    toset(a, sep, esc);
    return a;
}

char *f_union(char *a, char *b)
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
    a = (char *)realloc(a, alen + blen + 2);
    memcpy(a + alen + 1, b, blen + 1);
    a[alen] = sep;
    toset(a, sep, esc);
    return a;
}

static void elt_at(const char *array, int o, int *begin, int *after, char sep, char esc)
{
    int b = o, a = o;
    while(1) { // find start
	// Find prev separator
	while(b > 0 && array[b - 1] != sep)
	    --b;
	// Is it escaped?  If not, we're done
	if(!b || b == 1 || array[b - 2] != esc)
	    break;
	// If so, count escapes
	o = b - 2;
	while(o > 0 && array[o - 1] == esc)
	    --o;
	// Even #?  then it's not really escaped, so done
	if((b - o) % 2)
	    break;
	// Otherwise, continue search
	b = o;
    }
    // find end
    for(; array[a] && array[a] != sep; a++)
	if(array[a] == esc && array[a + 1])
	    ++a;
    *begin = b;
    *after = a;
}

bool find_unesc_elt(const char *a, const char *s, int *begin, int *after,
		    char sep, char esc)
{
    bool ret;
    char toesc[3];
    int len = strlen(s);
    int nesc;
    char *tmp = NULL;

    toesc[0] = esc;
    toesc[1] = sep;
    toesc[2] = 0;
    nesc = countchars(s, toesc);
    if(nesc) {
	s = tmp = (char *)malloc(len + nesc + 1);
	*escape(tmp, s, len, sep, toesc) = 0;
	len += nesc;
    }
    ret = find_elt(a, s, len, begin, after, sep, esc);
    if(tmp)
	free(tmp);
    return ret;
}

bool find_elt(const char *a, const char *s, int len, int *begin, int *after,
	      char sep, char esc)
{
    int l = 0, h = strlen(a) - 1, m;
    int mb, ma;

    while(l <= h) {
	m = (l + h) / 2;
	elt_at(a, m, &mb, &ma, sep, esc);
	int c = memcmp(s, a + mb, len > ma - mb ? ma - mb : len);
	if(c > 0)
	    l = ma + 1;
	else if(c < 0)
	    h = mb - 2;
	else if(len > ma - mb)
	    l = ma + 1;
	else if(len < ma - mb)
	    h = mb - 2;
	else {
	    *begin = mb;
	    *after = ma;
	    return true;
	}
    }
    *begin = l > len ? len : l;
    return false;
}

int del_elt(char *a, int len, int begin, int after)
{
    if(a[after])
	memmove(a + begin, a + after + 1, len - after);
    len -= after - begin;
    if(len) // only if there were >1 items in a
	--len; // removed separator
    a[len] = 0;
    return len;
}

char *f_intersect(char *a, char *b)
{
    if(!a || !*a || !b || !*b) {
	if(a)
	    free(a);
	if(b)
	    free(b);
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

char *f_setdiff(char *a, char *b)
{
    if(!b || !*b)
	return a;
    if(!a || !*a) {
	if(b)
	    free(b);
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
