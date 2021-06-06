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

/* FIXME:  none of these routines check results of writes for errors */


/*
 * given an item, print an expression that will represent the data, using a
 * combination of date, time, and other commands.
 */

static void print_data_expr(
	FILE		*fp,		/* file to print to */
	const ITEM	*item,		/* item to print */
	const MENU	*menu)		/* menu item to print */
{
	switch(item->type) {
	  case IT_CHOICE:
	  case IT_FLAG:
	  case IT_MENU:
	  case IT_RADIO:
		fprintf(fp, "expand(_%s)", item->name);
		break;

	  case IT_MULTI:
	  case IT_FLAGS:
		fprintf(fp, "expand(_%s)", IFL(item->,MULTICOL) ? menu->name : item->name);
		break;

	  case IT_TIME:
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
		break;

	  case IT_PRINT:
		fputs(item->idefault, fp);
		break;

	  case IT_FKEY:
	  case IT_INV_FKEY:
		/* FIXME: INV_FKEY has no name */
		/* FIXME: option to do / or </TD><TD> as field sep */
		/* FIXME: option to do </TR><TR> as multi-item sep */
		fprintf(fp, "deref(_%s)", item->name);
		break;

	  default:
		fprintf(fp, "_%s", item->name);
	}
}

/* template text must escape backslashes */
/* in some contexts, quotes or HTML need to be escaped, as well */
/* in one context, array-escaping is useful, but too much trouble for now */
/* instead, just call esc() at run-time */

#define ESC_HTML	1
#define ESC_QUOTE	2

static void pr_escaped(FILE *fp, const char *s, int len, int esc)
{
	if(!len || !s || !*s)
		return;
	while(len-- && *s) {
		if(esc & ESC_HTML)
			switch(*s++) {
			    case '<':
				fputs("&lt;", fp);
				continue;
			    case '>':
				fputs("&gt;", fp);
				continue;
			    case '&':
				fputs("&amp;", fp);
				continue;
			    case '"':
				fputs("&quot;", fp);
				continue;
			    case '\n':
				fputs("<BR>\n", fp);
				continue;
			    default:
				s--;
			}
		if(*s == '\\' || (*s == '"' && (esc & ESC_QUOTE)))
			putc('\\', fp);
		putc(*s++, fp);
	}
}


/*------------------------------------ HTML ---------------------------------*/
/*
 * create a HTML template for the current database. The template consists
 * of two parts, summary and data list. Summary entries are hotlinked to
 * the data entries in the data list.
 */

const char *mktemplate_html(
	const CARD	*card,
	FILE		*fp)		/* output file */
{
	struct menu_item *itemorder;	/* order of items in summary */
	int		nitems;		/* number of items in summary */
	size_t		nalloc;		/* number of allocated items */
	int		i, j, m;	/* item counter */
	const ITEM	*item;		/* current item */
	const MENU	*menu;		/* current menu selection */
	char		*name;		/* current field name */
	const ITEM	*primary_i = NULL;	/* item that is hyperlinked */

	if (!card || !card->dbase || !card->form)
		return("no database");
							/*--- header ---*/
	fprintf(fp,
		"\\{SUBST HTML}\n"
		"<HTML><HEAD>\n<TITLE>%s</TITLE>\n</HEAD><BODY "
		"BGCOLOR=#ffffff>\n<H1>%s</H1>\n"
		"This page was last updated \\{chop(system(\"date\"))}.\n",
			card->form->name, card->form->name);

							/*--- summary ---*/
	fputs("\\{IF +d}\n", fp);
	fprintf(fp, "<H2>Summary:</H2>\n<TABLE BORDER=0 CELLSPACING=3 "
		"CELLPADDING=4 BGCOLOR=#e0e0e0>\n<TR>");
	nalloc = card->form->nitems;
	itemorder = alloc(0, "summary", struct menu_item, nalloc);
	nitems = get_summary_cols(&itemorder, &nalloc, card->form);
	for (i=0; i < nitems; i++) {
		item = itemorder[i].item;
		menu = itemorder[i].menu;
		if (item->type == IT_CHOICE && i &&
		    itemorder[i-1].item->type == IT_CHOICE &&
		    itemorder[i-1].item->column == item->column)
			continue;
		fputs("<TH ALIGN=LEFT BGCOLOR=#a0a0c0>", fp);
		pr_escaped(fp, menu ? menu->label :
				BLANK(item->label) ? item->name
						   : item->label,
			   menu ? menu->sumwidth : item->sumwidth,
			   ESC_HTML | 0);
	}
	fprintf(fp, "\n\\{FOREACH}\n<TR>");
	/* old code always picked 1st column, but that doesn't work well for
	 * my games db, which lists flags to the left */
	/* instead, pick primary sort key if present */
	/* otherwise, at least skip to the first plain input field */
	/* it would be better to pick the first such field that is the least
	 * empty, but that's too much work. */
	for (i=0; i < nitems; i++) {
		item = itemorder[i].item;
		if(!primary_i && (item->type == IT_INPUT ||
				  item->type == IT_TIME ||
				  item->type == IT_NUMBER)) /* maybe add PRINT? */
			primary_i = item;
		if(IFL(item->,DEFSORT)) {
			primary_i = item;
			break;
		}
	}
	/* at the very least, don't support multicol fields */
	/* if all are multicol, then there won't be a primary_i.  Tough. */
	if(!primary_i) {
		for(i=0; i < nitems; i++)
			if(!IFL(itemorder[i].item->,MULTICOL)) {
				primary_i = itemorder[i].item;
				break;
			}
	}
	for (i=0; i < nitems; i++) {
		item = itemorder[i].item;
		menu = itemorder[i].menu;
		if (item->type == IT_CHOICE && i &&
		    itemorder[i-1].item->type == IT_CHOICE &&
		    itemorder[i-1].item->column == item->column)
			continue;
		fprintf(fp, "<TD VALIGN=TOP>");
		if (item == primary_i)
			fprintf(fp, "\\{IF +s}\n<A HREF=#\\{(this)}>\\{ENDIF}\n");
		fputs("\\{IF -n}\n<B>\\{ENDIF}\n", fp);
		fputs("\\{substr(", fp);
		print_data_expr(fp, item, menu);
		fprintf(fp, ",0,%d)}", menu ? menu->sumwidth : item->sumwidth);
		fputs("\\{IF -n}\n</B>\\{ENDIF}\n", fp);
		if (item == primary_i) fprintf(fp, "\\{IF +s}\n</A>\\{ENDIF}\n");
		fprintf(fp, "&nbsp;");
	}
	fputs("\n\\{IF -n}\n", fp);
	for (i=0; i < card->form->nitems; i++) {
		item = card->form->items[i];
		if(item->type != IT_NOTE)
			continue;
		fprintf(fp, "\\{IF {_%s!=\"\"}}\n"
			    "<TR><TD VALIGN=TOP COLSPAN=%d>"
			    "<BLOCKQUOTE>\\{_%s}</BLOCKQUOTE>"
			    "\n\\{ENDIF}\n",
			item->name, nitems, item->name);
	}
	fputs("\\{ENDIF}\n", fp);
	fprintf(fp, "\\{END}\n</TABLE>\n");
	free(itemorder);
	fputs("\\{ENDIF}\n", fp);
							/*--- data list ---*/
	fputs("\\{IF +s}\n", fp);
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
				if (card->form->items[j]->type == IT_CHOICE &&
				    card->form->items[j]->column == item->column)
					break;
			if (j < i)
				continue;
		}
		if (IFL(item->,MULTICOL)) {
			if(++m == item->nmenu) {
				m = -1;
				continue;
			}
			menu = &item->menu[m];
			name = menu->name;
			--i;
		} else
			menu = NULL;
		bool is_set = !menu &&
			(item->type == IT_MULTI || item->type == IT_FLAGS);
		if(is_set) {
			fprintf(fp, "\\{FOREACH [+e {expand(_%s)", name);
		} else
			fprintf(fp, "\\{IF {_%s!=\"\"", name);
		fputs("}}\n"
		      "<TR><TD ALIGN=RIGHT VALIGN=TOP><B>", fp);
		if(item==primary_i)
			fputs("<A NAME=\\{(this)}>", fp);
		pr_escaped(fp, name, -1, ESC_HTML | 0);
		putc(':', fp);
		if(item==primary_i)
			fputs("</A>", fp);
		fprintf(fp, "</B><TD>\\{");
		if (is_set)
			putc('e', fp);
		else
			print_data_expr(fp, item, menu);
		fprintf(fp, "}\n\\{END%s}\n", is_set ? "" : "IF");
	}
	fprintf(fp, "<TR><TD COLSPAN=2><HR>\n\\{END}\n</TABLE>\n");
	fputs("\\{ENDIF}\n", fp);
	fprintf(fp, "</BODY></HTML>\n");
	fflush(fp);
	return(0);
}

/* This is print_*_a() converted to templates */

#define INDENT	8		/* note indentation in N format */

/* print.c doesn't wrap summary lines; it just clips them */
/* maybe I should make this a template opption (-w = wrap) */
#define CLIP_SUMMARY 1

/* print.c also stripped trailing spaces from summary lines and wrapped
 * text, but that's more trouble than it's worth.  You can't see whitespace
 * at the end of line anyway, unless it wraps, which linelen ensures it
 * doesn't */

static const char *mktemplate_text(
	const CARD	*card,
	FILE		*fp,		/* output file */
	bool		overstrike)	/* fancy mode? */
{
	struct menu_item *itemorder;	/* order of items in summary */
	int		nitems;		/* number of items in summary */
	size_t		nalloc;		/* number of allocated items */
	int		i, j, m;	/* item counter */
	const ITEM	*item;		/* current item */
	const MENU	*menu;		/* current menu selection */
	char		*name;		/* current field name */
	char		*buf = NULL;	/* summary line buffer */
	size_t		buf_len;
	char		*s;
	int		len, label_len = 0;
	int		sumwidth;

	if (!card || !card->dbase || !card->form)
		return("no database");
							/*--- header ---*/
	fputs("\\{IF +d}\n", fp);
	/* just let sumwin.c build the header line */
	make_summary_header(&buf, &buf_len, card, 0);
	if(overstrike) {
		for(s = buf, i = len = 0; *s; s++, i++, len++) {
#if CLIP_SUMMARY
			if(len == pref.linelen)
				break;
#else
			if(len > pref.linelen) {
				putc('\n', fp);
				len = 0;
			}
#endif
			if(*s == '\\')
				putc('\\', fp);
			if(*s == ' ')
				putc('_', fp);
			else
				fprintf(fp, "_\b%c", *s);
		}
	} else {
		len = strlen(buf);
#if CLIP_SUMMARY
		if(len > pref.linelen)
			len = pref.linelen;
#endif
		s = buf;
#if !CLIP_SUMMARY
		while(len > pref.linelen) {
			pr_escaped(fp, s, pref.linelen, 0);
			putc('\n', fp);
			s += pref.linelen;
			len -= pref.linelen;
			for(i = 0; i < pref.linelen; i++)
				putc('-', fp);
			putc('\n', fp);
		}
#endif
		pr_escaped(fp, s, len, 0);
		putc('\n', fp);
		while(len--)
			putc('-', fp);
	}
	free(buf);

	len = 0;
	/* but recompute everything sumwin did to display the data */
	nalloc = card->form->nitems;
	itemorder = alloc(0, "summary", struct menu_item, nalloc);
	nitems = get_summary_cols(&itemorder, &nalloc, card->form);
	fputs("\n\\{FOREACH}\n", fp);
	for (i=0; i < nitems; i++) {
		item = itemorder[i].item;
		menu = itemorder[i].menu;
		if (item->type == IT_CHOICE && i &&
		    itemorder[i-1].item->type == IT_CHOICE &&
		    itemorder[i-1].item->column == item->column)
			continue;
		if(i) {
			len += 2;
			if(len < pref.linelen)
				fputs("  ", fp);
			else {
				len = 0;
#if CLIP_SUMMARY
				break;
#else
				putc('\n', fp);
#endif
			}
		}
		int olen = len;
		/* only bold summary line if -n flag enabled */
		for(int o = overstrike ? 2 : 1; o--; ) {
			len = olen;
			sumwidth = menu ? menu->sumwidth : item->sumwidth;
			if(overstrike)
				fprintf(fp, "\\{IF %cn}\n", o ? '-' : '+');
			fputs("\\{", fp);
			if(o)
				fputs("gsub(", fp);
			/* substr() only chops; align also fills */
			if(sumwidth + len > pref.linelen)
				fputs("substr(", fp);
			else if(i < nitems - 1)
				fputs("align(", fp);
			print_data_expr(fp, item, menu);
			if(sumwidth + len > pref.linelen) {
				fprintf(fp, ",0,%d)", pref.linelen - len);
#if !CLIP_SUMMARY
				j = pref.linelen - len;
				sumwidth -= j;
				while(1) {
					fputs(".\"\n\".", fp);
					if(sumwidth < pref.linelen - 1 && i < nitems - 1)
						fputs("align(", fp);
					fputs("substr(", fp);
					print_data_expr(fp, item, menu);
					fprintf(fp, ",%d,%d)", j,
						sumwidth > pref.linelen ? pref.linelen : sumwidth);
					if(sumwidth < pref.linelen - 1 && i < nitems - 1)
						fprintf(fp, ",%d)", sumwidth);
					if(sumwidth <= pref.linelen)
						break;
					sumwidth -= pref.linelen;
				}
				len = 0;
#endif
			} else if(i < nitems - 1)
				fprintf(fp, ",%d)", sumwidth);
			if(o)
				fputs(",\"[^ \n]\",\"\\0\b\\0\")", fp);
			fputs("\n}", fp);
			if(overstrike)
				fputs("\\{ENDIF}\n", fp);
			len += sumwidth;
		}
	}
	free(itemorder);
	fputs("\n\\{IF -n}\n", fp);
	for (i=0; i < card->nitems; i++) {
		item = card->form->items[i];
		if (item->type != IT_NOTE)
			continue;
		fprintf(fp, "\\{IF {_%s!=\"\"}}\n", item->name);
		/* outer gsub() prepends INDENT spaces to every line */
		/* inner gsub() wraps lines to pref.linelen */
		/* detab() doesn't take wrapping into account & can't */
		fprintf(fp, "\\{chop(gsub(gsub(detab(_%s,%d),"
			"\"[^\n]{%d}(?!\n)\",\"\\0\n\")"
			",\"(?m)^\",\"",
			item->name, INDENT, pref.linelen - INDENT);
		for(j = 0; j < INDENT; j++)
			putc(' ', fp);
		fputs("\"))}\n\\{ENDIF}\n\n", fp);
	}
	fputs("\\{ENDIF}\n",fp);
	fprintf(fp, "\\{END}\n");
	fputs("\\{ENDIF}\n\\{IF +s}\n", fp);
	const char *label;
	fputs("\\{FOREACH}\n", fp);
	for (i=0; i < card->nitems; i++) {
		item = card->form->items[i];
		if(item->type == IT_MULTI || item->type == IT_FLAGS) {
			for(m = 0; m < item->nmenu; m++) {
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
	/* don't let labels take over half the line (unlike print.c) */
	/* this means that labels may be truncated, since I won't wrap them */
	if(label_len > (pref.linelen - 1) / 2)
		label_len = (pref.linelen - 1) / 2;
	for (i=0,m=-1; i < card->form->nitems; i++) {
		item = card->form->items[i];
		if (!IN_DBASE(item->type) && item->type != IT_PRINT)
			continue;
		name = item->name;
		label = item->label ? item->label : item->name;
		if(item->type == IT_MULTI || item->type == IT_FLAGS ||
		   item->type == IT_MENU  || item->type == IT_RADIO) {
			if(++m == item->nmenu) {
				m = -1;
				continue;
			}
			menu = &item->menu[m];
			if(IFL(item->,MULTICOL))
				name = menu->name;
			if(item->type == IT_FLAGS || item->type == IT_MULTI)
				label = menu->label;
			--i;
		} else
			menu = NULL;
		if(!label)
			label = "--";
		/* choice fields are only printed if code matches */
		/* FIXME: print() displayed -- if MENU/RADIO invalid */
		/*        to do that, either implement \{ELSE}/\{ELSEIF} */
		/*        or have one final massive check for all values */
		if (item->type == IT_CHOICE || item->type == IT_MENU ||
		    item->type == IT_RADIO) {
			fprintf(fp, "\\{IF {_%s == \"", name);
			pr_escaped(fp, menu ? menu->flagcode : item->flagcode,
				   -1, ESC_QUOTE);
			fputs("\"}}\n", fp);
			if(item->type == IT_CHOICE)
				label = name /* ? name : "--" */; /* empty name is illegal */
			/* numeric types always have values */
			/* but print() didn't force Numbers */
			/* flag fields also always have values */
		} else if(item->type != IT_NUMBER && item->type != IT_TIME &&
			  item->type != IT_FLAG && item->type != IT_FLAGS &&
			  item->type != IT_MULTI)
			fprintf(fp, "\\{IF {_%s != \"\"}}\n", name);
		int len = label ? strlen(label) : 0;
		/* truncate if > 1/2 line len */
		if(len > label_len - 2)
			len = label_len - 2;
		for(j = 0; j < len; j++) {
			if(label[j] == '\\')
				putc('\\', fp);
			if(!overstrike || label[j] == ' ') // or \t or \n, probably too
				putc(label[j], fp);
			else {
				fprintf(fp, "%c\b%c", label[j], label[j]);
				if(label[j] == '\\')
					putc('\\', fp);
			}
		}
		for(; j < label_len; j++)
			putc(' ', fp);
		fputs("\\{", fp);
		/* outer gsub indents after line breaks */
		/* inner gsub wraps to linelen-label_len */
		/* detab() doesn't take wrapping into account & can't */
		fputs("gsub(chop(gsub(detab(", fp);
		switch(item->type) {
		    case IT_NOTE:
		    case IT_INPUT:
		    case IT_NUMBER:
		    case IT_PRINT:
		    case IT_TIME:
			print_data_expr(fp, item, menu);
			break;
		    case IT_CHOICE:
			if(item->label) {
				/* FIXME: wrap this in C, rather than tmpl */
				putc('"', fp);
				pr_escaped(fp, item->label, -1, ESC_QUOTE);
				putc('"', fp);
			} else
				/* print translated empty code to -- */
				/* but empty codes are forbidden */
				printf("_%s", item->name);
			break;
		    case IT_MENU:
		    case IT_RADIO:
			/* FIXME: wrap this in C, rather than tmpl */
			putc('"', fp);
			pr_escaped(fp, menu->label, pref.linelen - label_len, ESC_QUOTE);
			putc('"', fp);
			break;
		    case IT_MULTI:
		    case IT_FLAGS:
			/* FIXME: maybe instead of 1 line per flag, show ,-separated
			 * list of set flags (or just print set flags, period)
			 * [this applies to IT_FLAG as well] */
			/* double-escaping is too much of a pain, even though */
			/* it executes more efficiently than calling esc() */
			fprintf(fp, "_%s%s\"",
				name, IFL(item->,MULTICOL) ? "|*esc(" : "==");
			pr_escaped(fp, menu->flagcode, -1, ESC_QUOTE);
			fprintf(fp, "\"%s?\"%.*s\":\"%.*s\"",
				IFL(item->,MULTICOL) ? ")!=\"\"" : "",
				pref.linelen - label_len, "yes",
				pref.linelen - label_len, "no");
			break;
		    case IT_FLAG:
			fprintf(fp, "_%s==\"", name);
			pr_escaped(fp, item->flagcode, -1, ESC_QUOTE);
			fprintf(fp, "\"?\"%.*s\":\"%.*s\"",
				pref.linelen - label_len, "yes",
				pref.linelen - label_len, "no");
			break;
		    default:
			exit(1); /* should never get here */
		}
		/* outer gsub indents after line breaks */
		/* inner gsub wraps to linelen-label_len */
		/* gsub(chop(gsub(detab( */
		fprintf(fp, ",%d)"
			",\"([^\n]{%d})(?!\n)\",\"\\0\n\"))"
			",\"\n\",\"\n",
			label_len, pref.linelen - label_len);
		for(j = 0; j < label_len; j++)
			putc(' ', fp);
		fputs("\")}\n", fp);
		if(item->type != IT_NUMBER && item->type != IT_TIME &&
		   item->type != IT_FLAG && item->type != IT_FLAGS &&
		   item->type != IT_MULTI)
			fputs("\\{ENDIF}\n", fp);
	}
	fputs("\n\\{END}\n", fp);
	fputs("\\{ENDIF}\n", fp);
	fflush(fp);
	return(0);
}

const char *mktemplate_plain(const CARD *card, FILE *fp)
{
	return mktemplate_text(card, fp, false);
}
const char *mktemplate_fancy(const CARD *card, FILE *fp)
{
	return mktemplate_text(card, fp, true);
}

static void pr_schar(FILE *fp, char c, char doub = '\'')
{
	if(!isprint(c))
		fprintf(fp, "'||CHAR(%d)||'", (int)c);
	else
		putc(c, fp);
	if(c == doub)
		putc(doub, fp);
}

static void pr_dq(FILE *fp, const char *s, char q)
{
	/* note: mysql's broken parser needs \ escaped as well */
	/* but I don't care about mysql */
	if(!s)
		return;
	while(*s)
		pr_schar(fp, *s++, q);
}

/*
 * Unlike the others, this exports part of the schema as well
 * -d -> exclude summary view
 * -n -> N/A
 * -s -> exclude descriptive view
 * Except for flag fields, data is exported verbatim into database.  Flag
 * fields are converted to TRUE/FALSE first.  Maybe a future enhancement
 * would be to add a flag to export non-multicol flags as multicol flags
 * Table is created and data is exported regardless.
 * Form name and field names are assumed to be valid SQL
 */
const char *mktemplate_sql(const CARD *card, FILE *fp)
{
	struct menu_item *itemorder;	/* order of items in summary */
	int		nitems;		/* number of items in summary */
	size_t		nalloc;		/* number of allocated items */
	int		i, j, m;	/* item counter */
	const ITEM	*item;		/* current item */
	const MENU	*menu;		/* current menu selection */
	char		*s;
	const FORM	*form;
	bool		didcol;

	if (!card || !card->dbase || !card->form)
		return("no database");
	form = card->form;
	fprintf(fp, "-- grok form/database %s exported \\{date}\n", form->name);
	fputs("-- note that some databases may need additional editing\n", fp);
	fprintf(fp,
		"DROP TABLE IF EXISTS %s;\n"
		"BEGIN;\n"
		"CREATE TABLE %s (\n", form->name, form->name);
	if(form->help) {
		/* should be COMMENT ('....') but sqlite3 doesn't support it */
		/* maybe I should toggle support with -c */
		char *h = form->help;
		while(1) {
			s = strchr(h, '\n');
			if(!s)
				s = h + strlen(h);
			printf("  -- %.*s\n", (int)(s - h), h);
			if(!*s || !s[1])
				break;
			h = s + 1;
		}
	}
	for(didcol = false, i = 0; i < form->nitems; i++) {
		item = form->items[i];
		if(!IN_DBASE(item->type))
			continue;
		if(item->type == IT_CHOICE) {
			for(j = 0; j < i; j++)
				if(form->items[j]->type == IT_CHOICE &&
				   form->items[j]->column == item->column)
					break;
			if(j < i)
				continue;
		}
		if(IFL(item->,MULTICOL)) {
			for(j = 0; j < item->nmenu; j++) {
				if(didcol)
					fputs(",\n", fp);
				didcol = true;
				fprintf(fp, "  %s BOOLEAN NOT NULL", item->menu[j].name);
			}
			continue;
		}
		if(didcol)
			fputs(",\n", fp);
		didcol = true;
		fprintf(fp, "  %s ", item->name);
		switch(item->type) {
		    case IT_INPUT:
		    case IT_NOTE:
			fprintf(fp, "VARCHAR(%d) NOT NULL", item->maxlen);
			break;
		    case IT_NUMBER:
			if(item->digits)
				/* fprintf(fp, "DECIMAL(15,%d) NOT NULL", item->digits): */
				/* fputs("REAL NOT NULL", fp); */
				fputs("FLOAT(15) NOT NULL", fp);
			else
				fputs("BIGINT NOT NULL", fp);
			break;
		    case IT_TIME:
			switch(item->timefmt) {
			    case T_DATE:
				fputs("DATE NOT NULL", fp);
				break;
			    case T_TIME:
				fputs("TIME NOT NULL", fp);
				break;
			    case T_DATETIME:
				fputs("TIMESTAMP NOT NULL", fp);
				break;
			    case T_DURATION:
				fputs("TIME NOT NULL", fp); /* probably too limiting */
				break;
			}
			break;
		    case IT_CHOICE: {
			    int minlen = strlen(item->flagcode), maxlen = minlen;
			    for(j = i + 1; j < form->nitems; j++)
				    if(form->items[j]->type == IT_CHOICE &&
				       form->items[j]->column == item->column) {
					    int k = strlen(form->items[j]->flagcode);
					    if(k < minlen)
						    minlen = k;
					    if(k > maxlen)
						    maxlen = k;
				    }
			    if(minlen != maxlen)
				    fputs("VAR", fp);
			    fprintf(fp, "CHAR(%d) NOT NULL\n", maxlen);
			    fprintf(fp, "    (CHECK (%s in(", item->name);
			    for(j = i; j < form->nitems; j++)
				    if(form->items[j]->type == IT_CHOICE &&
				       form->items[j]->column == item->column) {
					    if(j != i)
						    putc(',', fp);
					    putc('\'', fp);
					    pr_dq(fp, form->items[j]->flagcode, '\'');
					    fputc('\'', fp);
				    }
			    fputs("))", fp);
			    break;
		    }
		    case IT_FLAG:
			fputs("BOOLEAN NOT NULL", fp);
			break;
		    case IT_MENU:
		    case IT_RADIO: {
			    int minlen = strlen(item->menu->flagcode), maxlen = minlen;
			    for(j = i; j < item->nmenu; j++) {
				    int k = strlen(item->menu[j].flagcode);
				    if(k < minlen)
					    minlen = k;
				    if(k > maxlen)
					    maxlen = k;
			    }
			    if(minlen != maxlen)
				    fputs("VAR", fp);
			    fprintf(fp, "CHAR(%d) NOT NULL\n", maxlen);
			    fprintf(fp, "    CHECK (%s in(", item->name);
			    for(j = 0; j < item->nmenu; j++) {
				    if(j)
					    putc(',', fp);
				    putc('\'', fp);
				    pr_dq(fp, item->menu[j].flagcode, '\'');
				    fputc('\'', fp);
			    }
			    fputs("))", fp);
			    break;
		    }
		    case IT_MULTI:
		    case IT_FLAGS:
			for(j = m = 0; m< item->nmenu; m++)
				j += strlen(item->menu[j].flagcode);
			fprintf(fp, "VARCHAR(%d) NOT NULL", j ? j + item->nmenu - 1 : 0);
			/* a check here would be too complicated */
			/* making the db slower if it actually enforces */
		    default: ; /* shut gcc up */
		}
	}
	if(!didcol) {
		fputs(");\n", fp);
		return(0);
	}
	fprintf(fp,
		");\n"
		"\\{IF +d}\n"
		"DROP VIEW IF EXISTS %s_sum;\n"
		"CREATE VIEW %s_sum AS SELECT\n", form->name, form->name);
	nalloc = card->form->nitems;
	itemorder = alloc(0, "summary", struct menu_item, nalloc);
	nitems = get_summary_cols(&itemorder, &nalloc, card->form);
	for(i = 0; i < nitems; i++) {
		item = itemorder[i].item;
		menu = itemorder[i].menu;
		const char *label = item->label;
		if (item->type == IT_CHOICE && i &&
		    itemorder[i-1].item->type == IT_CHOICE &&
		    itemorder[i-1].item->column == item->column)
			continue;
		if(i)
			fputs(",\n", fp);
		/* LEFT(x,y) -> SUBSTR(x,1,y) */
		fputs("  SUBSTR(", fp);
		switch(item->type) {
		    case IT_INPUT:
		    case IT_NOTE:
		    case IT_NUMBER:
		    case IT_TIME:
			fputs(item->name, fp);
			break;
		    case IT_CHOICE:
			if(i > 0 && form->items[i - 1]->type == IT_LABEL)
				label = form->items[i - 1]->label;
			else
				label = item->name;
			for(j = i; j < form->nitems; j++)
				if(form->items[j]->type == IT_CHOICE &&
				   form->items[j]->column == item->column &&
				   form->items[j]->flagtext)
					break;
			if(j == form->nitems) {
				fputs(item->name, fp);
				break;
			}
			fprintf(fp, "CASE %s", item->name);
			for(j = i; j < form->nitems; j++)
				if(form->items[j]->type == IT_CHOICE &&
				   form->items[j]->column == item->column &&
				   form->items[j]->flagtext) {
					fputs(" WHEN '", fp);
					pr_dq(fp, form->items[j]->flagcode, '\'');
					fputs("' THEN '", fp);
					pr_dq(fp, form->items[j]->flagtext, '\'');
					fputc('\'', fp);
				}
			fprintf(fp, " ELSE %s END", item->name);
			break;
		    case IT_FLAG:
			if(item->flagtext) {
				fprintf(fp, "CASE WHEN %s THEN '", item->name);
				pr_dq(fp, item->flagtext, '\'');
				fputs("' ELSE '' END", fp);
			} else
				fputs(item->name, fp);
			break;
		    case IT_MENU:
		    case IT_RADIO:
			for(j = 0; j < item->nmenu; j++)
				if(item->menu[j].flagtext)
					break;
			if(j == item->nmenu) {
				fputs(item->name, fp);
				break;
			}
			fprintf(fp, "CASE %s", item->name);
			for(j = 0; j < item->nmenu; j++) {
				if(!item->menu[j].flagtext)
					continue;
				fputs(" WHEN '", fp);
				pr_dq(fp, item->menu[j].flagcode, '\'');
				fputs("' THEN '", fp);
				pr_dq(fp, item->menu[j].flagtext, '\'');
				putc('\'', fp);
			}
			fprintf(fp, " ELSE %s END", item->name);
			break;
		    case IT_MULTI:
		    case IT_FLAGS:
			if(menu) {
				label = menu->label;
				if(menu->flagtext) {
					fprintf(fp, "CASE WHEN %s THEN '", menu->name);
					pr_dq(fp, menu->flagtext, '\'');
					fputs("' ELSE '' END", fp);
				} else
					fputs(menu->name, fp);
				break;
			}
			for(j = 0; j < item->nmenu; j++)
				if(item->menu[j].flagtext)
					break;
			if(j == item->nmenu) {
				fputs(item->name, fp);
				break;
			}
			fputs("TRIM(", fp);
			for(j = 0; j < item->nmenu; j++)
				if(item->menu[j].flagtext)
					fputs("REPLACE(", fp);
			putc('\'', fp);
			pr_schar(fp, form->aesc);
			fprintf(fp, "'||%s||'", item->name);
			pr_schar(fp, form->asep);
			putc('\'', fp);
			for(j = 0; j < item->nmenu; j++)
				if(item->menu[j].flagtext) {
					putc('\'', fp);
					pr_schar(fp, form->asep);
					for(s = item->menu[j].flagcode; *s; s++) {
						if(*s == form->asep ||
						   *s == form->aesc)
							pr_schar(fp, form->aesc);
						pr_schar(fp, *s++);
					}
					pr_schar(fp, form->asep);
					fputs("','", fp);
					pr_schar(fp, form->asep);
					pr_dq(fp, item->menu[j].flagtext, '\'');
					pr_schar(fp, form->asep);
					fputs("')", fp);
				}
			fputs(",'", fp);
			pr_schar(fp, form->asep);
			fprintf(fp, "'),1,%d)", item->sumwidth);
		    default: ; /* shut gcc up */
		}
		fprintf(fp, ",1,%d) \"", menu ? menu->sumwidth : item->sumwidth);
		pr_dq(fp, label, '"');
		putc('"', fp);
	}
	free(itemorder);
	fprintf(fp,
		"\n  FROM %s;\n"
		"\\{ENDIF}\n"
		"\\{IF +s}\n"
		"DROP VIEW IF EXISTS %s_view;\n"
		"CREATE VIEW %s_view AS SELECT\n", form->name, form->name, form->name);
	for(didcol = false, i = 0; i < form->nitems; i++) {
		item = form->items[i];
		/* labels are pointless, and print is impossible to translate */
		/* charts are right out */
		if(!IN_DBASE(item->type))
			continue;
		if(item->type == IT_CHOICE) {
			for(j = 0; j < i; j++)
				if(form->items[j]->type == IT_CHOICE &&
				   form->items[j]->column == item->column)
					break;
			if(j < i)
				continue;
		}
		if(IFL(item->,MULTICOL)) {
			for(j = 0; j < item->nmenu; j++) {
				if(didcol)
					fputs(",\n", fp);
				didcol = true;
				fprintf(fp, "  %s \"", item->menu[j].name);
				if(item->label) {
					pr_dq(fp, item->label, '"');
					fputs(": ", fp);
				}
				pr_dq(fp, item->menu[j].label, '"');
				putc('"', fp);
			}
			continue;
		}
		if(didcol)
			fputs(",\n", fp);
		didcol = true;
		switch(item->type) {
		    case IT_INPUT:
		    case IT_NOTE:
		    case IT_NUMBER:
		    case IT_TIME:
			fprintf(fp, "  %s \"", item->name);
			pr_dq(fp, item->label, '"');
			putc('"', fp);
			break;
		    case IT_CHOICE:
			/* FIXME: if label blank, use flagtext */
			/*        if both non-blank, append flagtext */
			/*   label (flagtext) */
			fprintf(fp, "  CASE %s", item->name);
			for(j = i; j < form->nitems; j++)
				if(form->items[j]->type == IT_CHOICE &&
				   form->items[j]->column == item->column) {
					fputs(" WHEN '", fp);
					pr_dq(fp, form->items[j]->flagcode, '\'');
					fputs("' THEN '", fp);
					if(form->items[j]->label) {
						pr_dq(fp, form->items[j]->label, '\'');
						if(form->items[j]->flagtext &&
						   strcmp(form->items[j]->flagtext, form->items[j]->label)) {
							fputs(" (", fp);
							pr_dq(fp, form->items[j]->flagtext, '\'');
							putc(')', fp);
						}
					} else
						pr_dq(fp, form->items[j]->flagtext, '\'');
					fputc('\'', fp);
				}
			fprintf(fp, " ELSE %s END \"", item->name);
			if(i > 0 && form->items[i - 1]->type == IT_LABEL)
				pr_dq(fp, form->items[i - 1]->label, '"');
			else
				pr_dq(fp, item->name, '"');
			putc('"', fp);
			break;
		    case IT_FLAG:
			if(item->flagtext) {
				fprintf(fp, "  CASE WHEN %s THEN '", item->name);
				pr_dq(fp, item->flagtext, '\'');
				fputs("' ELSE '' END \"", fp);
				putc('\'', fp);
			} else
				fprintf(fp, "  %s \"", item->name);
			pr_dq(fp, item->label, '"');
			putc('"', fp);
			break;
		    case IT_MENU:
		    case IT_RADIO:
			for(j = 0; j < item->nmenu; j++)
				if(strcmp(item->menu[j].label,
					  item->menu[j].flagcode) ||
				   (item->menu[j].flagtext &&
					   strcmp(item->menu[j].flagtext,
						  item->menu[j].label)))
					break;
			if(j == item->nmenu) {
				fprintf(fp, "  %s \"", item->name);
				pr_dq(fp, item->label, '"');
				putc('"', fp);
				break;
			}
			fprintf(fp, "  CASE %s", item->name);
			for(j = 0; j < item->nmenu; j++) {
				if(!strcmp(item->menu[j].label,
					   item->menu[j].flagcode) &&
				   (!item->menu[j].flagtext ||
					   !strcmp(item->menu[j].flagtext,
						   item->menu[j].label)))
					continue;
				fputs(" WHEN '", fp);
				pr_dq(fp, item->menu[j].flagcode, '\'');
				fputs("' THEN '", fp);
				pr_dq(fp, item->menu[j].label, '\'');
				if(item->menu[j].flagtext &&
				   strcmp(item->menu[j].flagtext, item->menu[j].label)) {
					fputs(" (", fp);
					pr_dq(fp, item->menu[j].flagtext, '\'');
					putc(')', fp);
				}
				putc('\'', fp);
			}
			fprintf(fp, " ELSE %s END", item->name);
			break;
		    case IT_MULTI:
		    case IT_FLAGS:
			/* this is bound to fail in some cases */
			/* that's at least one reason why I suggest
			 * manual editing in the header comment */
			fputs("  REPLACE(REPLACE(TRIM(", fp);
			for(j = 0; j < item->nmenu; j++)
				if(strcmp(item->menu[j].label,
					  item->menu[j].flagcode) ||
				   strchr(item->menu[j].flagcode, form->aesc) ||
				   strchr(item->menu[j].flagcode, form->asep) ||
				   (item->menu[j].flagtext &&
					   strcmp(item->menu[j].flagtext,
						  item->menu[j].label)))
					fputs("REPLACE(", fp);
			putc('\'', fp);
			pr_schar(fp, form->asep);
			fprintf(fp, "'||%s||'", item->name);
			pr_schar(fp, form->asep);
			putc('\'', fp);
			for(j = 0; j < item->nmenu; j++)
				if(strcmp(item->menu[j].label,
					  item->menu[j].flagcode) ||
				   strchr(item->menu[j].flagcode, form->aesc) ||
				   strchr(item->menu[j].flagcode, form->asep) ||
				   (item->menu[j].flagtext &&
					   strcmp(item->menu[j].flagtext,
						  item->menu[j].label))) {
					putc('\'', fp);
					pr_schar(fp, form->asep);
					for(s = item->menu[j].flagcode; *s; s++) {
						if(*s == form->asep ||
						   *s == form->aesc)
							pr_schar(fp, form->aesc);
						pr_schar(fp, *s++);
					}
					pr_schar(fp, form->asep);
					fputs("','", fp);
					pr_schar(fp, form->asep);
					pr_dq(fp, item->menu[j].label, '\'');
					if(item->menu[j].flagtext &&
					   strcmp(item->menu[j].flagtext,
						  item->menu[j].label)) {
						fputs(" (", fp);
						pr_dq(fp, item->menu[j].flagtext, '\'');
						putc(')', fp);
					}
					pr_schar(fp, form->asep);
					fputs("')", fp);
				}
			fputs(",'", fp);
			pr_schar(fp, form->asep);
			fputs("'),'", fp);
			pr_schar(fp, form->asep);
			fputs("',', '),'", fp);
			pr_schar(fp, form->aesc);
			fputs(", ','", fp);
			pr_schar(fp, form->asep);
			fputs("')", fp);
		    default: ; /* shut gcc up */
		}
	}
	fprintf(fp,
		"\n  FROM %s;\n"
		"\\{ENDIF}\n"
		"\\{SUBST '=''}\\{FOREACH}\n"
		"INSERT INTO %s (\n", form->name, form->name);
	for(didcol = false, i = 0; i < form->nitems; i++) {
		item = form->items[i];
		if(!IN_DBASE(item->type))
			continue;
		if(item->type == IT_CHOICE) {
			for(j = 0; j < i; j++)
				if(form->items[j]->type == IT_CHOICE &&
				   form->items[j]->column == item->column)
					break;
			if(j < i)
				continue;
		}
		if(IFL(item->,MULTICOL)) {
			for(j = 0; j < item->nmenu; j++) {
				if(didcol)
					fputs(",\n", fp);
				didcol = true;
				fprintf(fp, "   %s", item->menu[j].name);
			}
		} else {
			if(didcol)
				fputs(",\n", fp);
			didcol = true;
			fprintf(fp, "   %s", item->name);
		}
	}
	fputs("\n  ) VALUES (\n", fp);
	for(didcol = false, i = 0; i < form->nitems; i++) {
		item = form->items[i];
		if(!IN_DBASE(item->type))
			continue;
		if(item->type == IT_CHOICE) {
			for(j = 0; j < i; j++)
				if(form->items[j]->type == IT_CHOICE &&
				   form->items[j]->column == item->column)
					break;
			if(j < i)
				continue;
		}
		if(IFL(item->,MULTICOL)) {
			for(j = 0; j < item->nmenu; j++) {
				if(didcol)
					fputs(",\n", fp);
				didcol = true;
				fprintf(fp, "   \\{_%s?\"TRUE\":\"FALSE\"}", item->menu[j].name);
			}
		} else {
			if(didcol)
				fputs(",\n", fp);
			didcol = true;
			if(item->type == IT_NUMBER)
				fprintf(fp, "   \\{(_%s?_%s:0)}", item->name, item->name);
			else if(item->type == IT_FLAG)
				fprintf(fp, "   \\{_%s?\"TRUE\":\"FALSE\"}", item->name);
			else
				fprintf(fp, "   '\\{_%s}'", item->name);
		}
	}
	fputs("\n  );\n\\{END}\nCOMMIT;\n", fp);
	fflush(fp);
	return(0);
}
