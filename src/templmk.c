/*
 * create a default template file
 *
 *	mktemplate_html()	mode 0=both, 1=summary, 2=data
 *	mktemplate_tex()
 *	mktemplate_ps()
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include "grok.h"
#include "form.h"
#include "proto.h"


/*
 * given an item, print an expression that will represent the data, using a
 * combination of date, time, and other commands.
 */

static void print_data_expr(
	FILE		*fp,		/* file to print to */
	ITEM		*item)		/* item to print */
{
	switch(item->type) {
	  case IT_CHOICE:
	  case IT_FLAG:
		fprintf(fp, "\\{expand(_%s)}", item->name);
		break;

	  case IT_TIME:
		switch(item->timefmt) {
		  case T_DATE:
			fprintf(fp, "\\{date(_%s)}", item->name);
			break;
		  case T_TIME:
			fprintf(fp, "\\{time(_%s)}", item->name);
			break;
		  case T_DURATION:
			fprintf(fp, "\\{duration(_%s)}", item->name);
			break;
		  case T_DATETIME:
			fprintf(fp, "\\{date(_%s)} \\{time(_%s)",
						item->name, item->name);
		}
		break;

	  default:
		fprintf(fp, "\\{_%s}", item->name);
	}
}


/*------------------------------------ HTML ---------------------------------*/
/*
 * create a HTML template for the current database. The template consists
 * of two parts, summary and data list. Summary entries are hotlinked to
 * the data entries in the data list.
 */

static int compare(register MYCONST void *u, register MYCONST void *v)
	{ return(*(int *)u - *(int *)v); }
static void *allocate(int n)
	{ void *p=malloc(n); if (!p) fatal("no memory"); return(p); }

char *mktemplate_html(
	char		*oname,		/* default output filename, 0=stdout */
	int		mode)		/* 0=both, 1=summary, 2=data list */
{
	register CARD	*card = curr_card;
	FILE		*fp;		/* output HTML file */
	int		*itemorder;	/* order of items in summary */
	int		nitems;		/* number of items in summary */
	int		i, j, io;	/* item counter */
	ITEM		**ip;		/* current item */
	char		*name;		/* current field name */
	int		primary_i = -1;	/* item that is hyperlinked */

	if (!card || !card->dbase || !card->form)
		return("no database");
	if (!(fp = fopen(oname, "w")))
		return("failed to create HTML file");
							/*--- header ---*/
	fprintf(fp,
		"\\{SUBST HTML}\n"
		"<HTML><HEAD>\n<TITLE>%s</TITLE>\n</HEAD><BODY "
		"BGCOLOR=#ffffff>\n<H1>%s</H1>\n"
		"This page was last updated \\{chop(system(\"date\"))}.\n",
			card->form->name, card->form->name);

							/*--- summary ---*/
	if (mode != 2) {
		fprintf(fp, "<H2>Summary:</H2>\n<TABLE BORDER=0 CELLSPACING=3 "
				"CELLPADDING=4 BGCOLOR=#e0e0e0>\n<TR>");
		itemorder = allocate(2 * card->form->nitems * sizeof(int));
		ip = card->form->items;
		for (nitems=i=0; i < card->form->nitems; i++)
			if (ip[i]->sumwidth) {
				itemorder[nitems++] = ip[i]->sumcol;
				itemorder[nitems++] = i;
			}
		nitems /= 2;
		qsort((void *)itemorder, nitems, 2*sizeof(int), compare);
		for (i=0; i < nitems; i++) {
			io = itemorder[i*2+1];
			if (!IN_DBASE(ip[io]->type))
				continue;
			if (!io || strcmp(ip[io]->name, ip[io-1]->name))
				fprintf(fp,"<TH ALIGN=LEFT BGCOLOR=#a0a0c0>%s",
								ip[io]->name);
		}
		fprintf(fp, "\n\\{FOREACH}\n<TR>");
		for (i=0; i < nitems; i++) {
			io = itemorder[i*2+1];
			if (!IN_DBASE(ip[io]->type))
				continue;
			if (io && !strcmp(ip[io]->name, ip[io-1]->name))
				continue;
			fprintf(fp, "<TD VALIGN=TOP>");
			if (!i) {
				primary_i = io;
				fprintf(fp, "<A HREF=#\\{(this)}>");
			}
			print_data_expr(fp, ip[io]);
			if (!i) fprintf(fp, "</A>");
			fprintf(fp, "&nbsp;");
		}
		fprintf(fp, "\n\\{END}\n</TABLE>\n");
		free(itemorder);
	}
							/*--- data list ---*/
	if (mode != 1) {
		fprintf(fp,
			"<HR><H1>Data</H1>\n"
			"<TABLE BORDER=0>\n"
			"\\{FOREACH}\n");
		for (i=0, ip=card->form->items; i < card->form->nitems; i++) {
			if (!IN_DBASE(ip[i]->type))
				continue;
			name = ip[i]->name;
			if (ip[i]->type == IT_CHOICE) {
				for (j=0; j < i; j++)
					if (!strcmp(ip[j]->name, name))
						break;
				if (j < i)
					continue;
			}
			fprintf(fp,
				"\\{IF {_%s != \"\"}}\n"
				"<TR><TD ALIGN=RIGHT VALIGN=TOP><B>", name);
			fprintf(fp, i==primary_i ? "<A NAME=\\{(this)}>%s:</A>"
						 : "%s:", name);
			fprintf(fp, "</B><TD>");
			print_data_expr(fp, ip[i]);
			fprintf(fp, "\n\\{ENDIF}\n");
		}
		fprintf(fp, "<TR><TD COLSPAN=2><HR>\n\\{END}\n</TABLE>\n");
	}
	fprintf(fp, "</BODY></HTML>\n");
	fclose(fp);
	return(0);
}
