/*
 * print all currently selected cards.
 *
 *	print()
 */

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define INDENT	8		/* note indentation in N format */
#define TAB	8		/* tab stops every 8 characters */

#define SECT_OK(db,r) ((db)->currsect < 0 ||\
		       (db)->currsect == (db)->row[r]->section)

static void print_head_a(FILE *);
static void print_tail_a(FILE *);
static void print_card_a(FILE *, int);
static void print_head_p(FILE *);
static void print_tail_p(FILE *);
static void print_card_p(FILE *, int);


/*
 * If the print spooler dies for some reason, print a message. Don't let
 * grok die because of the broken pipe signal.
 */

static void broken_pipe_handler(int sig)
{
	create_error_popup(mainwindow, 0,
			"Print failed, spooler aborted with signal %d\n", sig);
	signal(SIGPIPE, SIG_IGN);
}


/*
 * print mainline. Gets all the parameters from pref.
 */

void print(void)
{
	FILE		*fp;		/* file or spooler to print to */
	char		*file;		/* name of file or spooler */
	char		path[128];	/* temp file name if to window */
	int		n = 0;		/* misc */

	if (curr_card && curr_card->dbase && curr_card->form)
		switch(pref.pselect) {
		  case 'C': n = curr_card->row;			break;
		  case 'S': n = curr_card->nquery;		break;
		  case 'A': n = curr_card->dbase->nrows;	break;
		}
	if (n == 0) {
		create_error_popup(mainwindow, 0, "No cards to print.");
		return;
	}
	if (pref.pquality == 'P') {
		create_error_popup(mainwindow, 0, "PostScript not supported");
		return;
	}
	signal(SIGPIPE, broken_pipe_handler);
	file = pref.pfile;
	switch(pref.pdevice) {
	  case 'P':
		file = pref.pquality == 'P' ? pref.pspooler_a : pref.pspooler_a;
		if (!(fp = popen(file, "w"))) {
			create_error_popup(mainwindow, errno,
			    "Cannot open pipe to print spooler\n\"%s\"", file);
			return;
		}
		break;

	  case 'W':
		sprintf(file = path, "/usr/tmp/grok%d", getpid());
		FALLTHROUGH

	  case 'F':
		if (!(fp = fopen(file, "w"))) {
			create_error_popup(mainwindow, errno,
				"Cannot open print output file\n\"%s\"", file);
			return;
		}
	}
	if (pref.pquality == 'P')
		print_head_p(fp);
	else
		print_head_a(fp);

	switch(pref.pselect) {
	  case 'C':
		if (pref.pquality == 'P')
			print_card_p(fp, n);
		else
			print_card_a(fp, n);
		break;

	  case 'S':
		for (n=0; n < curr_card->nquery; n++)
			if (pref.pquality == 'P')
				print_card_p(fp, curr_card->query[n]);
			else
				print_card_a(fp, curr_card->query[n]);
		break;

	  case 'e':
	  	for (n=0; n < curr_card->dbase->nrows; n++)
			if (SECT_OK(curr_card->dbase, n)) {
				if (pref.pquality == 'P')
					print_card_p(fp, n);
				else
					print_card_a(fp, n);
			}
		break;

	  case 'A':
	  	for (n=0; n < curr_card->dbase->nrows; n++)
			if (pref.pquality == 'P')
				print_card_p(fp, n);
			else
				print_card_a(fp, n);
	}
	if (pref.pquality == 'P')
		print_tail_p(fp);
	else
		print_tail_a(fp);

	switch(pref.pdevice) {
	  case 'P':
		pclose(fp);
		break;

	  case 'F':
		fclose(fp);
		break;

	  case 'W':
		fclose(fp);
		edit_file(file, TRUE, FALSE, "Grok Print", "editprint");
		unlink(file);
	}
}


/*----------------------------------------------------- ascii ---------------*/
/*
 * convert a line with max pref.linelen characters to bold or underline using
 * ascii overstrike effects. This will not work well on magic-cookie terminals.
 * As a side effect, truncates to pref.linelen characters. Does nothing if
 * overstrike mode is off, or if the output goes to a window.
 */

static void overstrike(
	char		*text,		/* text to convert */
	BOOL		underline)	/* TRUE=underline, FALSE=bold */
{
	char		buf[1024];	/* original text buffer */
	char		*p, *q;		/* source and target copy pointers */

	text[pref.linelen] = 0;
	if (pref.pquality != 'O' || pref.pdevice == 'W')
		return;
	strcpy(buf, text);
	for (p=buf, q=text; *p; p++) {
		*q++ = underline ? '_' : *p;
		/* this used to bold spaces, but why? */
		if (*p != ' ') {
			*q++ = '\b';
			*q++ = *p;
		}
	}
	*q = 0;
}


/*
 * print header (title line), for summary and summary+note formats
 */

static void print_head_a(
	FILE		*fp)		/* file or spooler to print to */
{
	char		buf[1024], *p;	/* summary line buffer */

	if (pref.pformat != 'C') {
		make_summary_line(buf, curr_card, -1);
		overstrike(buf, TRUE);
		fprintf(fp, "%s\n", buf);
		if (pref.pquality != 'O' || pref.pdevice == 'W') {
			for (p=buf; *p; p++)
				*p = '-';
			fprintf(fp, "%s\n", buf);
		}
	}
}


/*
 * no trailer for ascii
 */

/*ARGSUSED*/
static void print_tail_a(
UNUSED	FILE		*fp)		/* file or spooler to print to */
{
}


/*
 * print one card to file fp. The first routine prints a string with a certain
 * indentation and maximum length, specified separately for the first and
 * continuation lines. The indentation counts towards the maximum length.
 */

static void print_formatted(
	FILE		*fp,		/* file or spooler to print to */
	const char	*text,		/* text to print */
	int		in0,		/* indent of first line */
	int		len0,		/* length of first line */
	int		in1,		/* indent of other lines */
	int		len1,		/* length of other lines */
	BOOL		nl)		/* append trailing newline if TRUE */
{
	char		out[1024];	/* output line buffer */
	register const char *p = text;	/* next source character from text */
	register char	*q = out;	/* next target character in out[] */
	int		len;		/* current output line length */

	while (p && *p) {
		for (len=0; len < in0; len++)
			*q++ = ' ';
		while (*p && *p != '\n' && len < len0) {
			if (*p == '\t') {
				while (++len % TAB)
					*q++ = ' ';
				p++;
			} else {
				len += *p == '\b' ? -1 : 1;
				*q++ = *p++;
			}
		}
		if(*p == '\b' && p[1]) {
			*q++ = *p++;
			*q++ = *p++;
		}
		while (nl && q != out && q[-1] == ' ')
			q--;
		*q = 0;
		q    = out;
		p   += *p == '\n';
		in0  = in1;
		len0 = len1;
		fprintf(fp, "%s", out);
		if (nl || *p)
			fprintf(fp, "\n");
	}
}


static void print_card_a(
	FILE		*fp,		/* file or spooler to print to */
	int		num)		/* card number to print */
{
	ITEM		*item;		/* contains info about formatting */
	char		buf[1024];	/* summary line buffer */
	const char	*label;		/* if C format, label in left column */
	const char	*data;		/* data string from the database */
	int		i, mi;		/* item counter */
	int		len;		/* output line length */
	int		label_len = 0;	/* width of label column if C format */
	char		sep, esc;	/* array/set separator/esc */

	switch(pref.pformat) {
	  case 'S':
		make_summary_line(buf, curr_card, num);
		buf[pref.linelen] = 0;
		print_formatted(fp, buf, 0, pref.linelen,
					 0, pref.linelen, TRUE);
		break;

	  case 'N':
		make_summary_line(buf, curr_card, num);
		overstrike(buf, FALSE);
		print_formatted(fp, buf, 0, pref.linelen,
					 0, pref.linelen, TRUE);
		for (i=0; i < curr_card->nitems; i++) {
			item = curr_card->form->items[i];
			if (item->type != IT_NOTE)
				continue;
			print_formatted(fp, dbase_get(curr_card->dbase,
							num, item->column),
					    INDENT, pref.linelen,
					    INDENT, pref.linelen, TRUE);
			fprintf(fp, "\n");
		}
		break;

	  case 'C':
		/* FIXME: maybe instead of 1 line per flag, show ,-separated
		 * list of set flags (or just print set flags, period) */
		get_form_arraysep(curr_card->form, &sep, &esc);
		for (i=0; i < curr_card->nitems; i++) {
			item = curr_card->form->items[i];
			if(item->type == IT_MULTI || item->type == IT_FLAGS) {
				for(int m = 0; m < item->nmenu; m++) {
					len = strlen(item->menu[m].label);
					if(len > label_len)
						label_len = len;
				}
			} else {
				len = strlen(item->label ? item->label :
					     item->name  ? item->name  : "--");
				if (len > label_len)
					label_len = len;
			}
		}
		label_len += 2;
		for (i=mi=0; i < curr_card->nitems; i++) {
			item  = curr_card->form->items[i];
			if(IN_DBASE(item->type) && !item->multicol)
				data  = dbase_get(curr_card->dbase, num, item->column);
			else
				data = NULL;
			if(item->type == IT_MULTI || item->type == IT_FLAGS) {
				if(mi == item->nmenu) {
					mi = 0;
					continue;
				}
				if(item->multicol) {
					data = dbase_get(curr_card->dbase, num, item->menu[mi].column);
					label = item->menu[mi].label;
				}
				mi++;
				i--;
			} else
				label = item->label ? item->label :
					item->name  ? item->name  : "--";
			switch(item->type) {
			  case IT_INPUT:
			  case IT_NOTE:
			  case IT_NUMBER:
				break;
			  case IT_TIME:
				data = format_time_data(data, item->timefmt);
				break;
			  case IT_CHOICE: {
				char *c = item->flagcode;
				if (!data || !c || strcmp(data, c))
					continue;
				label = item->name  ? item->name  : "--";
				data  = item->label ? item->label : c?c : "--";
				break; }
			  case IT_MENU:
			  case IT_RADIO: {
				int m;
				for(m = 0; m < item->nmenu; m++)
					if(!strcmp(STR(data), STR(item->menu[m].flagcode)))
						  break;
				data = m < item->nmenu ? item->menu[m].label : "--";
				break;
			  }
			  case IT_MULTI:
			  case IT_FLAGS: {
				bool fl;
				if (item->multicol)
					fl = !strcmp(STR(data), item->menu[mi-1].flagcode);
				else {
					  int qbegin, qafter;
					  fl = find_unesc_elt(data, item->menu[mi-1].flagcode,
							      &qbegin, &qafter, sep, esc);
				}
				data = fl ? "yes" : "no";
				break;
			  }
			  case IT_FLAG:
				data = !data || !item->flagcode || strcmp(data,
						item->flagcode) ? "no" : "yes";
				break;
			  case IT_PRINT:
				data = evaluate(curr_card, item->idefault);
				break;
			  default:
				continue;
			}
			if (!data || !*data)
				continue;
			strcpy(buf, label);
			for (len=strlen(buf); len < label_len; len++)
				strcat(buf, " ");
			overstrike(buf, FALSE);
			print_formatted(fp, buf,  0, pref.linelen,
						  0, pref.linelen, FALSE);
			print_formatted(fp, data, 0, pref.linelen-label_len,
						label_len, pref.linelen, TRUE);
		}
		fprintf(fp, "\n");
		break;
	}
}


/*----------------------------------------------------- PostScript ----------*/
/*ARGSUSED*/
static void print_head_p(
UNUSED	FILE		*fp)		/* file or spooler to print to */
{
}

/*ARGSUSED*/
static void print_tail_p(
UNUSED	FILE		*fp)		/* file or spooler to print to */
{
}

/*ARGSUSED*/
static void print_card_p(
UNUSED	FILE		*fp,		/* file or spooler to print to */
UNUSED	int		num)		/* card number to print */
{
}
