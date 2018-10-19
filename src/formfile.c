/*
 * read and write form files.
 *
 *	write_form(form)		write form
 *	read_form(form, path)		read form into empty form struct
 */

#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <Xm/Xm.h>
#include "config.h"

#if defined(GROK) || defined(PLANGROK)

#include "grok.h"
#include "form.h"
#include "proto.h"

#ifdef PLANGROK
static DQUERY *add_dquery(FORM *fp) {return(0);}
static void remake_dbase_pulldown(void) {}
#endif

#define STR(s) (s ? s : "")
#define STORE(t,s) do{if(t)free((void*)t);t=(s)&&*(s)?mystrdup(s):0;}while(0)

char *itemname[NITEMS] = {
	"None", "Label", "Print", "Input", "Time", "Note", "Choice", "Flag",
	"Button", "Summary", "Chart" };


#ifdef GROK
/*
 * Write the form and all the items in it. Also create the database named
 * in the form if it doesn't exist.
 * Returns FALSE if the file could not be written.
 */

BOOL write_form(
	FORM		*form)		/* form and items to write */
{
	char		*path;		/* file name to write to */
	FILE		*fp;		/* open file */
	DQUERY		*dq;		/* default query pointer */
	ITEM		*item;		/* item pointer */
	int		i;		/* item counter */
	CHART		*chart;		/* chart pointer */
	int		c;		/* chart component counter */
	int		v;		/* chart component value counter */
	char		*p;		/* for printing help text */

	path = form->path ? form->path : form->name;
	if (!path || !*path) {
		create_error_popup(toplevel, 0,
			"Form has no name, cannot save to disk");
		return(FALSE);
	}
	if (!form->path)
		path = resolve_tilde(path, "gf");
	if (!(fp = fopen(path, "w"))) {
		create_error_popup(toplevel, errno,
			"Failed to create form file %s", path);
		return(FALSE);
	}
	fprintf(fp, "grok\n");
	fprintf(fp, "name       %s\n",		STR(form->name));
	fprintf(fp, "dbase      %s\n",		STR(form->dbase));
	fprintf(fp, "comment    %s\n",		STR(form->comment));
	fprintf(fp, "cdelim     %s\n",		to_octal(form->cdelim));
	fprintf(fp, "rdonly     %d\n",		form->rdonly);
	fprintf(fp, "proc       %d\n",		form->proc);
	fprintf(fp, "syncable   %d\n",		form->syncable);
	fprintf(fp, "grid       %d %d\n",	form->xg, form->yg);
	fprintf(fp, "size       %d %d\n",	form->xs, form->ys);
	fprintf(fp, "divider    %d\n",		form->ydiv);
	fprintf(fp, "autoq      %d\n",		form->autoquery);
	fprintf(fp, "planquery  %s\n",		STR(form->planquery));

	for (p=form->help; p && *p; ) {
		fprintf(fp, "help\t'");
		while (*p && *p != '\n')
			fputc(*p++, fp);
		fputc('\n', fp);
		if (*p) p++;
	}

	for (dq=form->query, i=0; i < form->nqueries; i++, dq++) {
		if (!dq->name && !dq->query)
			continue;
		fprintf(fp, "query_s    %d\n",		dq->suspended);
		fprintf(fp, "query_n    %s\n",		STR(dq->name));
		fprintf(fp, "query_q    %s\n",		STR(dq->query));
	}
	for (i=0; i < form->nitems; i++) {
		item = form->items[i];
		fprintf(fp, "\nitem\n");
		fprintf(fp, "type       %s\n",		itemname[item->type]);
		fprintf(fp, "name       %s\n",		STR(item->name));
		fprintf(fp, "pos        %d %d\n",	item->x,  item->y);
		fprintf(fp, "size       %d %d\n",	item->xs, item->ys);
		fprintf(fp, "mid        %d %d\n",	item->xm, item->ym);
		fprintf(fp, "sumwid     %d\n",		item->sumwidth);
		fprintf(fp, "sumcol     %d\n",		item->sumcol);
		fprintf(fp, "column     %ld\n",		item->column);
		fprintf(fp, "search     %d\n",		item->search);
		fprintf(fp, "rdonly     %d\n",		item->rdonly);
		fprintf(fp, "nosort     %d\n",		item->nosort);
		fprintf(fp, "defsort    %d\n",		item->defsort);
		fprintf(fp, "timefmt    %d\n",		item->timefmt);
		fprintf(fp, "code       %s\n",		STR(item->flagcode));
		fprintf(fp, "codetxt    %s\n",		STR(item->flagtext));
		fprintf(fp, "label      %s\n",		STR(item->label));
		fprintf(fp, "ljust      %d\n",		item->labeljust);
		fprintf(fp, "lfont      %d\n",		item->labelfont);
		fprintf(fp, "gray       %s\n",		STR(item->gray_if));
		fprintf(fp, "freeze     %s\n",		STR(item->freeze_if));
		fprintf(fp, "invis      %s\n",	      STR(item->invisible_if));
		fprintf(fp, "skip       %s\n",		STR(item->skip_if));
		fprintf(fp, "default    %s\n",		STR(item->idefault));
		fprintf(fp, "pattern    %s\n",		STR(item->pattern));
		fprintf(fp, "minlen     %d\n",		item->minlen);
		fprintf(fp, "maxlen     %d\n",		item->maxlen);
		fprintf(fp, "ijust      %d\n",		item->inputjust);
		fprintf(fp, "ifont      %d\n",		item->inputfont);
		fprintf(fp, "p_act      %s\n",		STR(item->pressed));
		fprintf(fp, "a_act      %s\n",		STR(item->added));
		fprintf(fp, "q_dbase    %s\n",		STR(item->database));
		fprintf(fp, "query      %s\n",		STR(item->query));
		fprintf(fp, "q_summ     %d\n",		item->qsummary);
		fprintf(fp, "q_first    %d\n",		item->qfirst);
		fprintf(fp, "q_last     %d\n",		item->qlast);
		if (item->plan_if)
		    fprintf(fp, "plan_if    %c\n",	item->plan_if);

		fprintf(fp, "ch_xrange  %g %g\n",	item->ch_xmin,
							item->ch_xmax);
		fprintf(fp, "ch_yrange  %g %g\n",	item->ch_ymin,
							item->ch_ymax);
		fprintf(fp, "ch_auto    %d %d\n",	item->ch_xauto,
							item->ch_yauto);
		fprintf(fp, "ch_grid    %g %g\n",	item->ch_xgrid,
							item->ch_ygrid);
		fprintf(fp, "ch_snap    %g %g\n",	item->ch_xsnap,
							item->ch_ysnap);
		fprintf(fp, "ch_label   %g %g\n",	item->ch_xlabel,
							item->ch_ylabel);
		fprintf(fp, "ch_xexpr   %s\n",		STR(item->ch_xexpr));
		fprintf(fp, "ch_yexpr   %s\n",		STR(item->ch_yexpr));
		fprintf(fp, "ch_ncomp   %d\n",		item->ch_ncomp);

		for (c=0; c < item->ch_ncomp; c++) {
		    chart = &item->ch_comp[c];
		    fprintf(fp, "chart\n");
		    fprintf(fp, "_ch_type   %d\n",	chart->line);
		    fprintf(fp, "_ch_fat    %d %d\n",	chart->xfat,
							chart->yfat);
		    fprintf(fp, "_ch_excl   %s\n",	STR(chart->excl_if));
		    fprintf(fp, "_ch_color  %s\n",	STR(chart->color));
		    fprintf(fp, "_ch_label  %s\n",	STR(chart->label));
		    for (v=0; v < 4; v++) {
			struct value *val = &chart->value[v];
			fprintf(fp, "_ch_mode%d  %d\n",	v, val->mode);
			fprintf(fp, "_ch_expr%d  %s\n",	v, STR(val->expr));
			fprintf(fp, "_ch_field%d %d\n",	v, val->field);
			fprintf(fp, "_ch_mul%d   %g\n",	v, val->mul);
			fprintf(fp, "_ch_add%d   %g\n",	v, val->add);
		    }
		}
	}
	fclose(fp);
	remake_dbase_pulldown();

	path = resolve_tilde(form->dbase, "db");
	if (access(path, F_OK) && errno == ENOENT) {
		if (!(fp = fopen(path, "w"))) {
			create_error_popup(toplevel, errno,
"The form was created successfully, but the\n\
database file %s cannot be created.\n\
No cards can be entered into the new Form.\n\nProblem: ", path);
			return(FALSE);
		}
		fclose(fp);
	}
	if (access(path, R_OK)) {
		create_error_popup(toplevel, errno,
"The form was created successfully, but the\n\
database file %s exists but is not readable.\n\
No cards can be entered into the new Form.\n\nProblem: ", path);
		return(FALSE);
	}
	return(TRUE);
}
#endif /* GROK */


/*
 * Read the form and all the items in it from a file.
 * Returns FALSE and leave *form untouched if the file could not be read.
 */

BOOL read_form(
	FORM			*form,		/* form and items to write */
	char			*path)		/* file to read list from */
{
	FILE			*fp;		/* open file */
	char			line[1024];	/* line buffer */
	char			*p, *key;	/* next char in line buf */
	DQUERY			*dq = 0;	/* default query pointer */
	ITEM			*item = 0;	/* item pointer, 0=form hdr */
	int			i;		/* item counter */
	CHART			*chart;		/* next chart component */
	CHART			nullchart;	/* if chart keyword missing */

	path = resolve_tilde(path, "gf");
	if (!(fp = fopen(path, "r"))) {
		create_error_popup(toplevel, errno,
			"Failed to open form file %s", path);
		return(FALSE);
	}
	form_delete(form);
	if (form->path)
		free(form->path);
	form->path = mystrdup(path);
	for (;;) {
		if (!fgets(line, 1023, fp))
			break;
		if ((i = strlen(line)) && line[i-1] == '\n')
			line[i-1] = 0;
		for (p=line; *p == ' ' || *p == '\t'; p++);
		if (*p == '#' || !*p)
			continue;
		for (key=p; *p && *p != ' ' && *p != '\t'; p++);
		if (*p)
			*p++ = 0;
		else
			*++p = 0;
		for (; *p == ' ' || *p == '\t'; p++);

		if (!strcmp(key, "item")) {
			if (!item_create(form, form->nitems)) {
				form_delete(form);
				return(FALSE);
			}
			chart = &nullchart;
			item  = form->items[form->nitems-1];
			item->selected = FALSE;
			item->ch_curr  = 0;
								/* header */
		} else if (!item) {
			if (!strcmp(key, "grok"))
					;
			else if (!strcmp(key, "name"))
					STORE(form->name, p);
			else if (!strcmp(key, "dbase"))
					STORE(form->dbase, p);
			else if (!strcmp(key, "comment"))
					STORE(form->comment, p);
			else if (!strcmp(key, "rdonly"))
					{sscanf(p, "%d", &i); form->rdonly=i;}
			else if (!strcmp(key, "proc"))
					{sscanf(p, "%d", &i); form->proc=i;}
			else if (!strcmp(key, "syncable"))
					{sscanf(p, "%d", &i);form->syncable=i;}
			else if (!strcmp(key, "cdelim"))
					form->cdelim = to_ascii(p, ':');
			else if (!strcmp(key, "grid"))
					sscanf(p, "%d %d",&form->xg,&form->yg);
			else if (!strcmp(key, "size"))
					sscanf(p, "%d %d",&form->xs,&form->ys);
			else if (!strcmp(key, "divider"))
					sscanf(p, "%d", &form->ydiv);
			else if (!strcmp(key, "autoq"))
					sscanf(p, "%d", &form->autoquery);
			else if (!strcmp(key, "planquery"))
					STORE(form->planquery, p);
			else if (!strcmp(key, "query_s")) {
				if (dq = add_dquery(form))
					dq->suspended = *p != '0';
			} else if (!strcmp(key, "query_n")) {
				if (dq)
					STORE(dq->name, p);
			} else if (!strcmp(key, "query_q")) {
				if (dq)
					STORE(dq->query, p);
			} else if (!strcmp(key, "help") && *p++ == '\'') {
				if (form->help) {
					if (form->help = realloc(form->help,
							   strlen(form->help) +
							   strlen(p) + 2))
						strcat(form->help, p);
				} else {
					if (form->help = malloc(strlen(p) + 2))
						strcpy(form->help, p);
				}
				if (form->help)
					strcat(form->help, "\n");
			} else
				fprintf(stderr,
					"%s: %s: ignored headers keyword %s\n",
							progname, path, key);

		} else {					/* item */
			if (!strcmp(key, "type")) {
				for (i=0; i < NITEMS; i++)
					if (!strcmp(itemname[i], p))
						break;
				if (i == NITEMS)
					fprintf(stderr,
						"%s: %s: bad item type %s\n",
						progname, path, p);
				item->type = i;
			}
			else if (!strcmp(key, "name"))
					STORE(item->name, p);
			else if (!strcmp(key, "pos"))
					sscanf(p, "%d %d",&item->x, &item->y);
			else if (!strcmp(key, "size"))
					sscanf(p,"%d %d", &item->xs,&item->ys);
			else if (!strcmp(key, "mid"))
					sscanf(p,"%d %d", &item->xm,&item->ym);
			else if (!strcmp(key, "sumwid"))
					item->sumwidth = atoi(p);
			else if (!strcmp(key, "sumcol"))
					item->sumcol = atoi(p);
			else if (!strcmp(key, "column"))
					item->column = atoi(p);
			else if (!strcmp(key, "search"))
					item->search = atoi(p) ? TRUE : FALSE;
			else if (!strcmp(key, "rdonly"))
					item->rdonly = atoi(p) ? TRUE : FALSE;
			else if (!strcmp(key, "nosort"))
					item->nosort = atoi(p) ? TRUE : FALSE;
			else if (!strcmp(key, "defsort"))
					item->defsort = atoi(p) ? TRUE : FALSE;
			else if (!strcmp(key, "timefmt"))
					item->timefmt = atoi(p);
			else if (!strcmp(key, "code"))
					STORE(item->flagcode, p);
			else if (!strcmp(key, "codetxt"))
					STORE(item->flagtext, p);
			else if (!strcmp(key, "label"))
					STORE(item->label, p);
			else if (!strcmp(key, "ljust"))
					item->labeljust = atoi(p);
			else if (!strcmp(key, "lfont"))
					item->labelfont = atoi(p);
			else if (!strcmp(key, "gray"))
					STORE(item->gray_if, p);
			else if (!strcmp(key, "freeze"))
					STORE(item->freeze_if, p);
			else if (!strcmp(key, "invis"))
					STORE(item->invisible_if, p);
			else if (!strcmp(key, "skip"))
					STORE(item->skip_if, p);
			else if (!strcmp(key, "default"))
					STORE(item->idefault, p);
			else if (!strcmp(key, "pattern"))
					STORE(item->pattern, p);
			else if (!strcmp(key, "minlen"))
					item->minlen = atoi(p);
			else if (!strcmp(key, "maxlen"))
					item->maxlen = atoi(p);
			else if (!strcmp(key, "ijust"))
					item->inputjust = atoi(p);
			else if (!strcmp(key, "ifont"))
					item->inputfont = atoi(p);
			else if (!strcmp(key, "p_act"))
					STORE(item->pressed, p);
			else if (!strcmp(key, "a_act"))
					STORE(item->added, p);
			else if (!strcmp(key, "q_dbase"))
					STORE(item->database, p);
			else if (!strcmp(key, "query"))
					STORE(item->query, p);
			else if (!strcmp(key, "q_summ"))
					item->qsummary = atoi(p);
			else if (!strcmp(key, "q_first"))
					item->qfirst = atoi(p);
			else if (!strcmp(key, "q_last"))
					item->qlast = atoi(p);
			else if (!strcmp(key, "showplan"))
					;	/* 1.5: now called planquery */
			else if (!strcmp(key, "plan_if"))
					item->plan_if = *p;

			else if (!strcmp(key, "ch_xrange"))
					sscanf(p, "%g %g", &item->ch_xmin,
							   &item->ch_xmax);
			else if (!strcmp(key, "ch_yrange"))
					sscanf(p, "%g %g", &item->ch_ymin,
							   &item->ch_ymax);
			else if (!strcmp(key, "ch_auto"))
					sscanf(p, "%d %d", &item->ch_xauto,
							   &item->ch_yauto);
			else if (!strcmp(key, "ch_grid"))
					sscanf(p, "%g %g", &item->ch_xgrid,
							   &item->ch_ygrid);
			else if (!strcmp(key, "ch_snap"))
					sscanf(p, "%g %g", &item->ch_xsnap,
							   &item->ch_ysnap);
			else if (!strcmp(key, "ch_label"))
					sscanf(p, "%g %g", &item->ch_xlabel,
							   &item->ch_ylabel);
			else if (!strcmp(key, "ch_xexpr"))
					STORE(item->ch_xexpr, p);
			else if (!strcmp(key, "ch_yexpr"))
					STORE(item->ch_yexpr, p);
			else if (!strcmp(key, "ch_ncomp")) {
					if (item->ch_ncomp = atoi(p))
						item->ch_comp =
							calloc(item->ch_ncomp,
								sizeof(CHART));
			}
			else if (!strcmp(key, "chart"))
					chart = chart == &nullchart ?
						item->ch_comp : chart+1;
			else if (!strcmp(key, "_ch_type"))
					chart->line = atoi(p);
			else if (!strcmp(key, "_ch_fat"))
					sscanf(p, "%d %d", &chart->xfat,
							   &chart->yfat);
			else if (!strcmp(key, "_ch_excl"))
					STORE(chart->excl_if, p);
			else if (!strcmp(key, "_ch_color"))
					STORE(chart->color, p);
			else if (!strcmp(key, "_ch_label"))
					STORE(chart->label, p);
			else if (!strncmp(key, "_ch_mode", 8))
					chart->value[key[8]-'0'].mode =atoi(p);
			else if (!strncmp(key, "_ch_field", 9))
					chart->value[key[9]-'0'].field=atoi(p);
			else if (!strncmp(key, "_ch_mul", 7))
					chart->value[key[7]-'0'].mul = atof(p);
			else if (!strncmp(key, "_ch_add", 7))
					chart->value[key[7]-'0'].add = atof(p);
			else if (!strncmp(key, "_ch_expr", 8))
					STORE(chart->value[key[8]-'0'].expr,p);
#if 0
			else
				fprintf(stderr,
					"%s: %s: ignored item keyword %s\n",
							progname, path, key);
#endif
		}
	}
	fclose(fp);
	return(TRUE);
}

#endif /* GROK || PLANGROK */
