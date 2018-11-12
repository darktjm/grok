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
		if (item->column == column &&
		   (item->type == IT_CHOICE || item->type == IT_FLAG) &&
		    item->flagcode &&
		    item->flagtext &&
		    !strcmp(value, item->flagcode))
			return(mystrdup(item->flagtext));
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
	(void)system(cmd);
	dup2(fd0_save, 0);
	dup2(fd1_save, 1);
	dup2(fd2_save, 2);
	close(fd0_save); /* added Paul van Slobbe bug fix */
	close(fd1_save);
	close(fd2_save);
							/* error messages */
	if (fp = fopen(errpath, "r")) {
		(void)fseek(fp, 0, 2);
		if (size = ftell(fp)) {
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
	if (fp = fopen(outpath, "r")) {
		(void)fseek(fp, 0, 2);
		if (size = ftell(fp)) {
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
