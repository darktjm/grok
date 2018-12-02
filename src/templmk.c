/*
 * create a default template file
 *
 *	mktemplate_html()	mode 0=both, 1=summary, 2=data
 *	mktemplate_tex()
 *	mktemplate_ps()
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"


/*
 * given an item, print an expression that will represent the data, using a
 * combination of date, time, and other commands.
 */

static void print_esc_q(FILE *fp, const char *s, char sep, char esc)
{
	putc('"', fp);
	for(; *s; s++) {
		if(*s == '"')
			putc('\\', fp);
		/* there is no way this will work right if sep is " */
		if(*s == esc || *s == sep) {
			putc(esc, fp);
			if(esc == '"') {
				putc('\\', fp);
				putc(esc, fp);
			}
		}
		putc(*s, fp);
	}
	putc('"', fp);
}

static void print_data_expr(
	FILE		*fp,		/* file to print to */
	const ITEM	*item,		/* item to print */
	const MENU	*menu,		/* menu item to print */
	bool		restrict = false)
{
	int sumwidth = menu ? menu->sumwidth : item->sumwidth;
	switch(item->type) {
	  case IT_CHOICE:
	  case IT_FLAG:
	  case IT_MENU:
	  case IT_RADIO:
#define prefix do { \
	fputs("\\{", fp); \
	if(restrict) \
		fputs("substr(", fp); \
} while(0)
		prefix;
		fprintf(fp, "expand(_%s)", item->name);
#define suffix do { \
	if(restrict) \
		fprintf(fp, ",0,%d)", sumwidth); \
	putc('}', fp); \
} while(0)
		suffix;
		break;

	  case IT_MULTI:
	  case IT_FLAGS:
		prefix;
		fprintf(fp, "expand(_%s)", item->multicol ? menu->name : item->name);
		suffix;
		break;

	  case IT_TIME:
		prefix;
		switch(item->timefmt) {
		  case T_DATE:
			fprintf(fp, "date(_%s)", item->name);
			break;
		  case T_TIME:
			fprintf(fp, "time(_%s)", item->name);
			break;
		  case T_DURATION:
			fprintf(fp, "duration(_%s)", item->name);
			break;
		  case T_DATETIME:
			fprintf(fp, "date(_%s).\" \".time(_%s)",
						item->name, item->name);
		}
		suffix;
		break;

	  default:
		prefix;
		fprintf(fp, "_%s", item->name);
		suffix;
	}
}


/*------------------------------------ HTML ---------------------------------*/
/*
 * create a HTML template for the current database. The template consists
 * of two parts, summary and data list. Summary entries are hotlinked to
 * the data entries in the data list.
 */

static void *allocate(int n)
	{ void *p=malloc(n); if (!p) fatal("no memory"); return(p); }

const char *mktemplate_html(
	char		*oname,		/* default output filename, 0=stdout */
	int		mode)		/* 0=both, 1=summary, 2=data list */
{
	register CARD	*card = curr_card;
	FILE		*fp;		/* output HTML file */
	struct menu_item *itemorder;	/* order of items in summary */
	int		nitems, nalloc;	/* number of items in summary */
	int		i, j, m;	/* item counter */
	const ITEM	*item;		/* current item */
	const MENU	*menu;		/* current menu selection */
	char		*name;		/* current field name */
	const ITEM	*primary_i = NULL;	/* item that is hyperlinked */
	char		sep, esc;

	if (!card || !card->dbase || !card->form)
		return("no database");
	if (!(fp = fopen(oname, "w")))
		return("failed to create HTML file");
	get_form_arraysep(card->form, &sep, &esc);
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
		nalloc = card->form->nitems;
		itemorder = (struct menu_item *)allocate(nalloc * sizeof(*itemorder));
		nitems = get_summary_cols(&itemorder, &nalloc, card->form);
		for (i=0; i < nitems; i++) {
			item = itemorder[i].item;
			menu = itemorder[i].menu;
			if (item->type == IT_CHOICE && i &&
			    itemorder[i-1].item->type == IT_CHOICE &&
			    itemorder[i-1].item->column == item->column)
				continue;
			fprintf(fp,"<TH ALIGN=LEFT BGCOLOR=#a0a0c0>%.*s",
				menu ? menu->sumwidth : item->sumwidth,
				menu ? menu->label :
					BLANK(item->label) ? item->name
							   : item->label);
		}
		fprintf(fp, "\n\\{FOREACH}\n<TR>");
		for (i=0; i < nitems; i++) {
			item = itemorder[i].item;
			menu = itemorder[i].menu;
			if (item->type == IT_CHOICE && i &&
			    itemorder[i-1].item->type == IT_CHOICE &&
			    itemorder[i-1].item->column == item->column)
				continue;
			fprintf(fp, "<TD VALIGN=TOP>");
			if (!i) {  // FIXME:  pick a better candidate
				primary_i = item;
				fprintf(fp, "<A HREF=#\\{(this)}>");
			}
			print_data_expr(fp, item, menu, true);
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
		for (i=0,m=-1; i < card->form->nitems; i++) {
			item = card->form->items[i];
			if (!IN_DBASE(item->type)) // FIXME: allow Print
				continue;
			name = item->name;
			if (item->type == IT_CHOICE) {
				for (j=0; j < i; j++)
					if (!strcmp(card->form->items[j]->name, name))
						break;
				if (j < i)
					continue;
			}
			if (item->type == IT_MULTI || item->type == IT_FLAGS) {
				if(++m == item->nmenu) {
					m = -1;
					continue;
				}
				menu = &item->menu[m];
				if(item->multicol)
					name = menu->name;
				--i;
			} else
				menu = NULL;
			fprintf(fp, "\\{IF {_%s", name);
			if(menu && !item->multicol) {
				fputs("|*", fp);
				print_esc_q(fp, menu->flagcode, sep, esc);
			}
			fputs(" != \"\"}}\n"
			      "<TR><TD ALIGN=RIGHT VALIGN=TOP><B>", fp);
			fprintf(fp, item==primary_i ? "<A NAME=\\{(this)}>%s:</A>"
						 : "%s:", name);
			fprintf(fp, "</B><TD>");
			if (!menu || item->multicol)
				print_data_expr(fp, item, menu);
			else
				fputs(BLANK(menu->flagtext) ? menu->flagcode
				      			    : menu->flagtext, fp);
			fprintf(fp, "\n\\{ENDIF}\n");
		}
		fprintf(fp, "<TR><TD COLSPAN=2><HR>\n\\{END}\n</TABLE>\n");
	}
	fprintf(fp, "</BODY></HTML>\n");
	fclose(fp);
	return(0);
}
