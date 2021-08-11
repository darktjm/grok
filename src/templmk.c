/*
 * create a default template file
 *
 *	mktemplate_html()
 *	mktemplate_tex()
 *	mktemplate_sql()
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


static const char *label_of(const ITEM *item, const MENU *menu)
{
	const char *label;
	if(menu)
		label = BLANK(menu->label) ? menu->name : menu->label;
	else
		label = BLANK(item->label) ? item->name : item->label;
	if(!label)
		label = "--";
	return label;
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
	struct sum_item *itemorder;	/* order of items in summary */
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
	itemorder = alloc(0, "summary", struct sum_item, nalloc);
	nitems = get_summary_cols(&itemorder, &nalloc, const_cast<CARD *>(card));
	for (i=0; i < nitems; i++) {
		item = itemorder[i].item;
		menu = itemorder[i].menu;
		if (item->type == IT_CHOICE && i &&
		    itemorder[i-1].item->type == IT_CHOICE &&
		    itemorder[i-1].item->column == item->column)
			continue;
		fputs("<TH ALIGN=LEFT BGCOLOR=#a0a0c0>", fp);
		pr_escaped(fp, label_of(item, menu),
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
		fputs("\\{IF -n}\n<B>\\{ENDIF}\n\\{", fp);
		if (card->form->sumheight > 0)
			fputs("trunc2d(", fp);
		else
			fputs("substr(gsub(", fp);
		print_data_expr(fp, item, menu);
		if (card->form->sumheight > 0)
			fprintf(fp, ",%d,%d",
				menu ? menu->sumwidth : item->sumwidth,
				card->form->sumheight + 1);
		else
			fprintf(fp, ",\"\n.*\",\"\"),0,%d", menu ? menu->sumwidth : item->sumwidth);
		fputs(")}\\{IF -n}\n</B>\\{ENDIF}\n", fp);
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
	free_summary_cols(itemorder, nitems);
	fputs("\\{ENDIF}\n", fp);
							/*--- data list ---*/
	fputs("\\{IF +s}\n", fp);
	fprintf(fp,
		"<HR><H1>Data</H1>\n"
		"<TABLE BORDER=0>\n"
		"\\{FOREACH}\n");
	for (i=0,m=-1; i < card->form->nitems; i++) {
		item = card->form->items[i];
		if (!IN_DBASE(item->type) && item->type != IT_PRINT)
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
		pr_escaped(fp, label_of(item, menu), -1, ESC_HTML | 0);
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
	struct sum_item *itemorder;	/* order of items in summary */
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
	itemorder = alloc(0, "summary", struct sum_item, nalloc);
	nitems = get_summary_cols(&itemorder, &nalloc, const_cast<CARD *>(card));
	fputs("\n\\{FOREACH}\n", fp);
	if (card->form->sumheight > 0) {
		char asep, aesc;
		get_form_arraysep(card->form, &asep, &aesc);
		fputs("\\{(x=0);t=\"\";\n  foreach(v,\"", fp);
		for (i=0; i < nitems; i++) {
			item = itemorder[i].item;
			menu = itemorder[i].menu;
			if (item->type == IT_CHOICE && i &&
			    itemorder[i-1].item->type == IT_CHOICE &&
			    itemorder[i-1].item->column == item->column)
				continue;
			if(i)
				putc(asep, fp);
			fputs(menu ? menu->name : item->name, fp);
		}
		fprintf(fp,
			"\",\n\t\"foreach(w,'',"
			"bsub('(y=count(_'.v.',\\\"\\n\\\"))'));\n\t\t"
			"(x=(y>x?y>%d?%d:y:x))\")\n}"
			"\\{FOREACH [v {align(\"\",x,\"%c\")}}\n",
			card->form->sumheight, card->form->sumheight, asep);
	}
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
			if(sumwidth + len > pref.linelen || i == nitems - 1)
				fputs("substr(", fp);
			else
				fputs("align(", fp);
			fputs("gsub(", fp);
			if(card->form->sumheight > 0)
				fputs("sub(", fp);
			print_data_expr(fp, item, menu);
			if(card->form->sumheight > 0)
				fputs(",t,\"\")", fp);
			fputs(",\"\n.*\",\"\")", fp);
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
			else
				fprintf(fp, ",0,%d)", sumwidth);
			if(o)
				fputs(",\"[^ \n]\",\"\\0\b\\0\")", fp);
			fputs("\n}", fp);
			if(overstrike)
				fputs("\\{ENDIF}\n", fp);
			len += sumwidth;
		}
	}
	free_summary_cols(itemorder, nitems);
	putc('\n', fp);
	if (card->form->sumheight > 0)
		fputs("\\{t=bsub(\"[^\\n]*($|\\n\").t.\")\";\"\"}"
		         "\\{END}\n", fp);
	fputs("\\{IF -n}\n", fp);
	for (i=0; i < card->form->nitems; i++) {
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
	fputs("\\{ENDIF}\n"
	      "\\{END}\n", fp);
	fputs("\\{ENDIF}\n\\{IF +s}\n", fp);
	fputs("\\{FOREACH}\n", fp);
	for (i=0; i < card->form->nitems; i++) {
		item = card->form->items[i];
		if(item->type == IT_MULTI || item->type == IT_FLAGS) {
			for(m = 0; m < item->nmenu; m++) {
				len = strlen(label_of(item, &item->menu[m]));
				if(len > label_len)
					label_len = len;
			}
		} else {
			len = strlen(label_of(item, NULL));
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
		if(item->type == IT_MULTI || item->type == IT_FLAGS ||
		   item->type == IT_MENU  || item->type == IT_RADIO) {
			if(++m == item->nmenu) {
				m = -1;
				continue;
			}
			menu = &item->menu[m];
			if(IFL(item->,MULTICOL))
				name = menu->name;
			--i;
		} else
			menu = NULL;
		const char *label;
		label = label_of(item, menu);
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

static void pr_schar(FILE *fp, char c, char doub = '\'');
static void pr_dq(FILE *fp, const char *s, char q);
static void pr_sql_type(FILE *fp, const FORM *form, int i, bool null = false);
static void pr_sql_fkey_fields(FILE *fp, const char *db, ITEM *item,
			       const int *seq, int nseq, bool has_groupby,
			       bool is_agg = false);
static void pr_sql_fkey_tables(FILE *fp, const char *db, ITEM *item,
			       const int *seq, int nseq);
static void pr_sql_fkey_ref(FILE *fp, ITEM *item);
static void pr_sql_tq(FILE *fp, const char *db, const int *seq, int nseq);
static void pr_sql_item(FILE *fp, const char *db, const FORM *form, int itno,
			const MENU *menu, const char *&label,
			const int *seq, int nseq, bool has_groupby);
static void add_ref_keys(QStringList &sl, const char *cform, const FORM *form);
static bool any_fkey_multi(const FORM *form, const struct sum_item *itemorder = 0,
			   int nitems = 0, const ITEM *item = 0);

const char *mktemplate_sql(const CARD *card, FILE *fp)
{
	struct sum_item *itemorder;	/* order of items in summary */
	int		nitems;		/* number of items in summary */
	size_t		nalloc;		/* number of allocated items */
	int		i, j;		/* item counter */
	ITEM		*item;		/* current item */
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
		"\\{IF +s}\n"
		"DROP VIEW\\{IF +f} IF EXISTS\\{ENDIF} %s_view;\n"
		"\\{ENDIF}\n"
		"\\{IF +d}\n"
		"DROP VIEW\\{IF +f} IF EXISTS\\{ENDIF} %s_sum;\n"
		"\\{ENDIF}\n"
		"DROP TABLE\\{IF +f} IF EXISTS\\{ENDIF} %s\\{IF -p} CASCADE\\{ENDIF};\n"
		"\\{IF +f}BEGIN;\n\\{ENDIF}"
		"CREATE TABLE %s (\n", form->name, form->name, form->name, form->name);
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
		pr_sql_type(fp, form, i);
	}
	if(!didcol) {
		fputs(");\n", fp);
		return(0);
	}
	bool has_groupby = any_fkey_multi(form);
	if(has_groupby)
		fputs(",\n  fk_rowid INTEGER", fp);
	for(i = 0; i < form->nitems; i++) {
		int m;
		item = form->items[i];
		if(item->type != IT_FKEY)
			continue;
		for(j = m = 0; j < item->nfkey; j++) {
			if(!item->fkey[j].key)
				continue;
			if(m++)
				break;
		}
		if(j < item->nfkey) {
			fputs(",\n    FOREIGN KEY (", fp);
			for(j = m = 0; j < item->nfkey; j++)
				if(item->fkey[j].key) {
					if(m++)
						fprintf(fp, ",%s_%d", item->name, m);
					else
						fputs(item->name, fp);
				}
			putc(')', fp);
			pr_sql_fkey_ref(fp, item);
		}
	}
	/* make referred-to fields UNIQUE */
	QStringList sl;
	for(i = 0; i < card->form->nchild; i++)
		add_ref_keys(sl, card->form->children[i], card->form);
	for(i = 0; i < card->form->nitems; i++)
		if(card->form->items[i]->type == IT_INV_FKEY) {
			resolve_fkey_fields(card->form->items[i]);
			add_ref_keys(sl, card->form->items[i]->fkey_form->name,
				     card->form);
		}
	sl.removeDuplicates();
	for(i = 0; i < sl.length(); i++) {
		char *s = qstrdup(sl[i]);
		fprintf(fp, ",\n  UNIQUE(%s)", s);
		free(s);
	}
	fputs(");\n", fp);
	/* Note: postgres creates these automatically */
	/*       so does sqlite3, sort of, but not permanently */
	for(i = 0; i < sl.length(); i++) {
		char *s = qstrdup(sl[i]);
		fprintf(fp, "DROP INDEX\\{IF +f} IF EXISTS\\{ENDIF} %s_%d;\n"
			"CREATE INDEX %s_%d ON %s (%s);\n", card->form->name, i,
			card->form->name, i, card->form->name, s);
		free(s);
	}
	fprintf(fp,
		"\\{IF +d}\n"
		"CREATE VIEW %s_sum AS SELECT\n", form->name);
	nalloc = card->form->nitems;
	itemorder = alloc(0, "summary", struct sum_item, nalloc);
	nitems = get_summary_cols(&itemorder, &nalloc, card);
	has_groupby = any_fkey_multi(form, itemorder, nitems);
	for(i = 0; i < nitems; i++) {
		item = itemorder[i].item;
		menu = itemorder[i].menu;
		if (item->type == IT_CHOICE && i &&
		    itemorder[i-1].item->type == IT_CHOICE &&
		    itemorder[i-1].item->column == item->column)
			continue;
		if(i)
			fputs(",\n", fp);
		int nseq = 0;
		const CARD *c;
		for(c = itemorder[i].fcard; c->fkey_next; c = c->fkey_next)
			nseq++;
		int seq[nseq];
		for(c = itemorder[i].fcard, j = nseq - 1; c->fkey_next; c = c->fkey_next, j--) {
			if(!j)
				seq[j] = c->qcurr;
			else
				seq[j] = c->row;
		}
		const char *label = label_of(item, menu);
		/* LEFT(x,y) -> SUBSTR(x,1,y) (sqlite3 doesn't support LEFT) */
		/* and firebird doesn't support SUBSTR, so use LEFT there */
		fputs("  \\{IF -f}LEFT\\{ELSE}SUBSTR\\{ENDIF}(", fp);
		bool is_agg = false;
		if(has_groupby) {
			int it = itemorder[i].fcard->qcurr;
			for(c = itemorder[i].fcard->fkey_next; c && c->fkey_next; c = c->fkey_next) {
				const ITEM *fitem = c->form->items[it];
				if((fitem->type == IT_FKEY && IFL(fitem->, FKEY_MULTI)) ||
				   fitem->type == IT_INV_FKEY) {
					is_agg = true;
					break;
				}
			}
			if(is_agg)
				fputs("\\{IF -p}string_agg\\{ELSEIF -f}list\\{ELSE}group_concat\\{END}(coalesce(", fp);
			else
				fputs("max(", fp);
		}
		if(item->type == IT_NUMBER)
			/* postgres requires explicit num->str conversion */
			fputs("\\{IF -p}''||\\{ENDIF}", fp);
		int itno;
		const FORM *fform = itemorder[i].fcard->form;
		for(itno = 0; itno < fform->nitems; itno++)
			if(fform->items[itno] == item)
				break;
		pr_sql_item(fp, form->name, fform, itno, menu, label, seq, nseq, has_groupby);
		fprintf(fp, "\\{IF +f},1\\{ENDIF},%d)", menu ? menu->sumwidth : item->sumwidth);
		if(has_groupby) {
			if(is_agg)
				/* FIXME:  should this be arraysep? */
				/*         if so, array-esc items? */
				fputs(",''), CH\\{IF +p}A\\{ENDIF}R(10))", fp);
			else
				putc(')', fp);
		}
		fputs(" \"", fp);
		pr_dq(fp, label, '"');
		/* alias to avoid name conflicts w/ other tables */
		/* FIXME: use field labels as prefix instead of this suffix */
		for(j=0; j < nseq; j++)
			fprintf(fp, "_%d", seq[j]);
		putc('"', fp);
	}
	fprintf(fp, "\n  FROM %s", form->name);
	for(i=0; i < form->nitems; i++) {
		item = form->items[i];
		if(item->type == IT_FKEY && item->sumwidth)
			pr_sql_fkey_tables(fp, form->name, item, &i, 1);
	}
	if(has_groupby)
		fprintf(fp, "\n    GROUP BY %s.fk_rowid", form->name);
	free_summary_cols(itemorder, nitems);
	/* FIXME:  ORDER BY sort column */
	fprintf(fp,
		";\n"
		"\\{ENDIF}\n"
		"\\{IF +s}\n"
		"CREATE VIEW %s_view AS SELECT\n", form->name);
	has_groupby = any_fkey_multi(form);
	for(didcol = false, i = 0; i < form->nitems; i++) {
		item = form->items[i];
		/* labels are pointless, and print is impossible to translate */
		/* charts are right out */
		/* inv_fkey requires multiple rows */
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
				fputs("  ", fp);
				if(has_groupby)
					fputs("max(", fp);
				pr_sql_tq(fp, form->name, 0, 0);
				fputs(item->menu[j].name, fp);
				if(has_groupby)
					putc(')', fp);
				fputs(" \"", fp);
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
		fputs("  ", fp);
		const char *label = item->label;
		char esc, sep;
		get_form_arraysep(form, &sep, &esc);
		switch(item->type) {
		    case IT_INPUT:
		    case IT_NOTE:
		    case IT_NUMBER:
		    case IT_TIME:
		    case IT_FLAG:
		    case IT_CHOICE:
			if(has_groupby)
				fputs("max(", fp);
			pr_sql_item(fp, form->name, form, i, 0, label, 0, 0, has_groupby);
			if(has_groupby)
				putc(')', fp);
			fputs(" \"", fp);
			pr_dq(fp, label, '"');
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
			if(has_groupby)
				fputs("max(", fp);
			if(j == item->nmenu) {
				fputs(item->name, fp);
				if(has_groupby)
					putc(')', fp);
				fputs(" \"", fp);
				pr_dq(fp, item->label, '"');
				putc('"', fp);
				break;
			}
			fprintf(fp, "CASE %s", item->name);
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
			if(has_groupby)
				putc(')', fp);
			fputs(" \"", fp);
			pr_dq(fp, label ? label : item->name, '"');
			putc('"', fp);
			break;
		    case IT_MULTI:
		    case IT_FLAGS:
			/* this is bound to fail in some cases */
			/* that's at least one reason why I suggest
			 * manual editing in the header comment */
			if(has_groupby)
				fputs("max(", fp);
			fputs("REPLACE(REPLACE(TRIM(", fp);
			for(j = 0; j < item->nmenu; j++)
				if(strcmp(item->menu[j].label,
					  item->menu[j].flagcode) ||
				   strchr(item->menu[j].flagcode, esc) ||
				   strchr(item->menu[j].flagcode, sep) ||
				   (item->menu[j].flagtext &&
					   strcmp(item->menu[j].flagtext,
						  item->menu[j].label)))
					fputs("REPLACE(", fp);
			putc('\'', fp);
			pr_schar(fp, sep);
			fprintf(fp, "'||%s||'", item->name);
			pr_schar(fp, sep);
			fputs("',", fp);
			for(j = 0; j < item->nmenu; j++)
				if(strcmp(item->menu[j].label,
					  item->menu[j].flagcode) ||
				   strchr(item->menu[j].flagcode, esc) ||
				   strchr(item->menu[j].flagcode, sep) ||
				   (item->menu[j].flagtext &&
					   strcmp(item->menu[j].flagtext,
						  item->menu[j].label))) {
					putc('\'', fp);
					pr_schar(fp, sep);
					for(s = item->menu[j].flagcode; *s; s++) {
						if(*s == sep || *s == esc)
							pr_schar(fp, esc);
						pr_schar(fp, *s);
					}
					pr_schar(fp, sep);
					fputs("','", fp);
					pr_schar(fp, sep);
					pr_dq(fp, item->menu[j].label, '\'');
					if(item->menu[j].flagtext &&
					   strcmp(item->menu[j].flagtext,
						  item->menu[j].label)) {
						fputs(" (", fp);
						pr_dq(fp, item->menu[j].flagtext, '\'');
						putc(')', fp);
					}
					pr_schar(fp, sep);
					fputs("'),", fp);
				}
			putc('\'', fp);
			pr_schar(fp, sep);
			fputs("'),'", fp);
			pr_schar(fp, sep);
			fputs("',', '),'", fp);
			pr_schar(fp, esc);
			fputs(", ','", fp);
			pr_schar(fp, sep);
			fputs("')", fp);
			if(has_groupby)
				putc(')', fp);
			fputs(" \"", fp);
			pr_dq(fp, label ? label : item->name, '"');
			putc('"', fp);
			break;
		    case IT_FKEY:
			pr_sql_fkey_fields(fp, form->name, item, &i, 1, has_groupby);
			break;
		    default: ; /* shut gcc up */
		}
	}
	fprintf(fp, "\n  FROM %s", form->name);
	for(i=0; i < form->nitems; i++) {
		item = form->items[i];
		if(item->type == IT_FKEY)
			pr_sql_fkey_tables(fp, form->name, item, &i, 1);
	}
	if(has_groupby)
		fprintf(fp, "\n    GROUP BY %s.fk_rowid", form->name);
	/* FIXME:  ORDER BY sort column */
	fprintf(fp,
		";\n"
		"\\{ENDIF}\n"
		"\\{SUBST '=''}\\{FOREACH}\n"
		"INSERT INTO %s (\n", form->name);
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
	if(has_groupby)
		fputs(",\n  fk_rowid", fp);
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
			else if(item->type == IT_TIME)
				fprintf(fp, "   \\{IF -f}DATEADD\\{ENDIF}"
					        "\\{IF -p}to_timestamp\\{ENDIF}"
					"(\\{_%s?_%s:\"0\"}"
					"\\{IF -f} SECOND TO DATE '1/1/1970'\\{ENDIF}"
					")",
					item->name, item->name);
			else if(item->type == IT_FKEY)
				fprintf(fp, "   \\{IF (!#_%s)}NULL\\{ELSE}'\\{_%s}'\\{ENDIF}",
					item->name, item->name);
			else
				fprintf(fp, "   '\\{_%s}'", item->name);
		}
	}
	if(has_groupby)
		fputs(",\n  \\{this}", fp);
	fputs("\n  );\n\\{END}\nCOMMIT;\n", fp);
	fflush(fp);
	return(0);
}

static void pr_schar(FILE *fp, char c, char doub)
{
	if(c == '\\') /* grok itself eats unescaped backslashes */
		fputs("\\\\", fp);
	else if(!isprint(c))
		/* FIXME: should just forget this so Unicode works right */
		fprintf(fp, "'||\\{IF -f}ASCII_\\{ENDIF}CH\\{IF +p}A\\{ENDIF}R(%d)||'", (int)(unsigned char)c);
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

static const char *sqlite_date_fmt(const ITEM *item)
{
	/* Note:  2-year dates not supported in sqlite3 */
	/* Note:  12-hour time not supported in sqlite3 */
	switch(item->timefmt) {
	    case T_DATE:
		return pref.mmddyy ? "%m/%d/%Y" : "%d.%m.%Y"; /* %y */
	    case T_TIME:
		return /* pref.ampm ? "%I:%M%p" : */ "%H:%M"; /* %p is > 1 char */
	    case T_DURATION:
		return "%H:%M"; /* %H is limited to 24 hrs */
	    case T_DATETIME:
	    default:
		/* pref.ampm should be used here as well */
		return pref.mmddyy ? "%m/%d/%Y %H:%M" : "%d.%m.%Y %H:%M";
	}
}

static const char *postgres_date_fmt(const ITEM *item)
{
	switch(item->timefmt) {
	    case T_DATE:
		return pref.mmddyy ? "MM/DD/DD" : "DD.MM.YY";
	    case T_TIME:
		return pref.ampm ? "HH12:MIam" : "HH24:MI"; /* am is > 1 char */
	    case T_DURATION:
		return "HH24:MI"; /* HH24 is limited to 24 hrs */
	    case T_DATETIME:
	    default:
		return pref.mmddyy ?
			(pref.ampm ? "MM/DD/YY HH12:MIam" : "MM/DD/YY HH24:MI") :
			(pref.ampm ? "DD.MM.YY HH12:MIam" : "DD.MM.YY HH24:MI");
	}
}

static void pr_sql_item(FILE *fp, const char *db, const FORM *form, int itno,
			const MENU *menu, const char *&label,
			const int *seq, int nseq, bool has_groupby)
{
	ITEM *item = form->items[itno];
	int j;
	char esc, sep;
	get_form_arraysep(form, &sep, &esc);
	switch(item->type) {
	    case IT_INPUT:
	    case IT_NOTE:
	    case IT_NUMBER:
		pr_sql_tq(fp, db, seq, nseq);
		fputs(item->name, fp);
		break;
	    case IT_TIME: {
		    /* there is no standard way of doing this */
		    /* sqlite3 by default; postgres with -p, firebird -f */
		    /* note that firebird has no control of output format */
		    fprintf(fp, "\\{IF -p}to_char(\\{ELSEIF -f}CAST(\\{ELSE}"
			        "strftime('%s',\\{ENDIF}", sqlite_date_fmt(item));
		    pr_sql_tq(fp, db, seq, nseq);
		    fprintf(fp, "%s\\{IF +f},\\{ENDIF}\\{IF -p}'%s'\\{ELSEIF -f} AS VARCHAR(20)\\{ELSE}"
			        "'unixepoch'\\{ENDIF})",
			    item->name, postgres_date_fmt(item));
		    break;
	    }
	    case IT_CHOICE:
		/* FIXME: if label blank, use flagtext */
		/*        if both non-blank, append flagtext */
		/*   label (flagtext) */
		if(itno > 0 && form->items[itno - 1]->type == IT_LABEL)
			label = form->items[itno - 1]->label;
		else
			label = item->name;
		for(j = itno; j < form->nitems; j++)
			if(form->items[j]->type == IT_CHOICE &&
			   form->items[j]->column == item->column &&
			   form->items[j]->flagtext)
				break;
		if(j == form->nitems) {
			pr_sql_tq(fp, db, seq, nseq);
			fputs(item->name, fp);
			break;
		}
		fputs("CASE ", fp);
		pr_sql_tq(fp, db, seq, nseq);
		fputs(item->name, fp);
		for(j = itno; j < form->nitems; j++)
			if(form->items[j]->type == IT_CHOICE &&
			   form->items[j]->column == item->column &&
			   form->items[j]->flagtext) {
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
				putc('\'', fp);
			}
		fputs(" ELSE ", fp);
		pr_sql_tq(fp, db, seq, nseq);
		fprintf(fp, "%s END", item->name);
		break;
	    case IT_FLAG:
		if(item->flagtext) {
			fputs("CASE WHEN ", fp);
			pr_sql_tq(fp, db, seq, nseq);
			fprintf(fp, "%s THEN '", item->name);
			pr_dq(fp, item->flagtext, '\'');
			fputs("' ELSE '' END", fp);
		} else {
			pr_sql_tq(fp, db, seq, nseq);
			fputs(item->name, fp);
		}
		break;
	    case IT_MENU:
	    case IT_RADIO:
		for(j = 0; j < item->nmenu; j++)
			if(item->menu[j].flagtext)
				break;
		if(j == item->nmenu) {
			pr_sql_tq(fp, db, seq, nseq);
			fputs(item->name, fp);
			break;
		}
		fputs("CASE ", fp);
		pr_sql_tq(fp, db, seq, nseq);
		fputs(item->name, fp);
		for(j = 0; j < item->nmenu; j++) {
			if(!item->menu[j].flagtext)
				continue;
			fputs(" WHEN '", fp);
			pr_dq(fp, item->menu[j].flagcode, '\'');
			fputs("' THEN '", fp);
			pr_dq(fp, item->menu[j].flagtext, '\'');
			putc('\'', fp);
		}
		fputs(" ELSE ", fp);
		pr_sql_tq(fp, db, seq, nseq);
		fprintf(fp, "%s END", item->name);
		break;
	    case IT_MULTI:
	    case IT_FLAGS:
		if(menu) {
			label = menu->label;
			if(menu->flagtext) {
				fputs("CASE WHEN ", fp);
				pr_sql_tq(fp, db, seq, nseq);
				fprintf(fp, "%s THEN '", menu->name);
				pr_dq(fp, menu->flagtext, '\'');
				fputs("' ELSE '' END", fp);
			} else {
				pr_sql_tq(fp, db, seq, nseq);
				fputs(menu->name, fp);
			}
			break;
		}
		for(j = 0; j < item->nmenu; j++)
			if(item->menu[j].flagtext)
				break;
		if(j == item->nmenu) {
			pr_sql_tq(fp, db, seq, nseq);
			fputs(item->name, fp);
			break;
		}
		fputs("TRIM(", fp);
		for(j = 0; j < item->nmenu; j++)
			if(item->menu[j].flagtext)
				fputs("REPLACE(", fp);
		putc('\'', fp);
		pr_schar(fp, sep);
		fputs("'||", fp);
		pr_sql_tq(fp, db, seq, nseq);
		fprintf(fp, "%s||'", item->name);
		pr_schar(fp, sep);
		putc('\'', fp);
		for(j = 0; j < item->nmenu; j++)
			if(item->menu[j].flagtext) {
				fputs(",'", fp);
				pr_schar(fp, sep);
				for(const char *s = item->menu[j].flagcode; *s; s++) {
					if(*s == sep || *s == esc)
						pr_schar(fp, esc);
					pr_schar(fp, *s);
				}
				pr_schar(fp, sep);
				fputs("','", fp);
				pr_schar(fp, sep);
				pr_dq(fp, item->menu[j].flagtext, '\'');
				pr_schar(fp, sep);
				fputs("')", fp);
			}
		fputs(",'", fp);
		pr_schar(fp, sep);
		fputs("')", fp);
		break;
	    case IT_FKEY:
		pr_sql_fkey_fields(fp, db, item, &itno, 1, has_groupby);
		break;
	    default: ; /* shut gcc up */
	}
}

static void pr_sql_fkey_ref(FILE *fp, ITEM *item)
{
	int j, m;
	resolve_fkey_fields(item);
	fprintf(fp, " REFERENCES %s (", item->fkey_form->name);
	for(j = m = 0; j < item->nfkey; j++)
		if(item->fkey[j].key) {
			if(m++)
				putc(',', fp);
			fputs(item->fkey[j].name, fp);
		}
	putc(')', fp);
}

static void pr_sql_type(FILE *fp, const FORM *form, int i, bool null)
{
	int j, m;
	ITEM *item = form->items[i];
	switch(item->type) {
	    case IT_INPUT:
	    case IT_NOTE:
		fprintf(fp, "VARCHAR(%d)", item->maxlen);
		if(!null)
			fputs(" NOT NULL", fp);
		break;
	    case IT_NUMBER:
		if(item->digits)
			fprintf(fp, "DECIMAL(15,%d)", item->digits);
			/* fputs("DOUBLE PRECISION", fp); */
		else
			fputs("BIGINT", fp);
		if(!null)
			fputs(" NOT NULL", fp);
		break;
	    case IT_TIME:
		switch(item->timefmt) {
		    case T_DATE:
			fputs("DATE", fp);
			break;
		    case T_TIME:
			fputs("TIME", fp);
			break;
		    case T_DATETIME:
			fputs("TIMESTAMP", fp);
			break;
		    case T_DURATION:
			fputs("TIME", fp); /* probably too limiting */
			break;
		}
		if(!null)
			fputs(" NOT NULL", fp);
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
		    fprintf(fp, "CHAR(%d)", maxlen);
		    if(!null)
			    fputs(" NOT NULL", fp);
		    fprintf(fp, "\n    (CHECK (%s in(", item->name);
		    for(j = i; j < form->nitems; j++)
			    if(form->items[j]->type == IT_CHOICE &&
			       form->items[j]->column == item->column) {
				    if(j != i)
					    putc(',', fp);
				    putc('\'', fp);
				    pr_dq(fp, form->items[j]->flagcode, '\'');
				    putc('\'', fp);
			    }
		    fputs("))", fp);
		    break;
	    }
	    case IT_FLAG:
		fputs("BOOLEAN", fp);
		if(!null)
			fputs(" NOT NULL", fp);
		break;
	    case IT_MENU:
	    case IT_RADIO: {
		    int minlen = strlen(item->menu->flagcode), maxlen = minlen;
		    for(j = 0; j < item->nmenu; j++) {
			    int k = strlen(item->menu[j].flagcode);
			    if(k < minlen)
				    minlen = k;
			    if(k > maxlen)
				    maxlen = k;
		    }
		    if(minlen != maxlen)
			    fputs("VAR", fp);
		    fprintf(fp, "CHAR(%d)", maxlen);
		    if(!null)
			    fputs(" NOT NULL", fp);
		    fprintf(fp, "\n    CHECK (%s in(", item->name);
		    for(j = 0; j < item->nmenu; j++) {
			    if(j)
				    putc(',', fp);
			    putc('\'', fp);
			    pr_dq(fp, item->menu[j].flagcode, '\'');
			    putc('\'', fp);
		    }
		    fputs("))", fp);
		    break;
	    }
	    case IT_MULTI:
	    case IT_FLAGS:
		for(j = m = 0; m< item->nmenu; m++)
			j += strlen(item->menu[j].flagcode);
		fprintf(fp, "VARCHAR(%d)", j ? j + item->nmenu - 1 : 0);
		if(!null)
			fputs(" NOT NULL", fp);
		/* a check here would be too complicated */
		/* making the db slower if it actually enforces */
		break;
	    case IT_FKEY: {
		    resolve_fkey_fields(item);
		    const FORM *fform = item->fkey_form;
		    if(!fform)
			    break;
		    /* FIXME:  create support table for FKEY_MULTI */
		    /*    <dbname>_<itemname>_ref */
		    /*      <ref_field>.... <itemname>_id */
		    /*    <itemname> becomes integer sequence # */
		    for(j = m = 0; j < item->nfkey; j++)
			    if(item->fkey[j].key) {
				    if(!item->fkey[j].item)
					    break;
				    if(m++)
					    fprintf(fp, ", %s_k%d ", item->name, m);
				    pr_sql_type(fp, fform, item->fkey[j].index % fform->nitems, true);
			    }
		    if(m == 1) {
			    fputs("\n   ", fp);
			    pr_sql_fkey_ref(fp, item);
		    }
		    break;
	    }
	    default: ; /* shut gcc up */
	}
}

static void pr_sql_tq(FILE *fp, const char *db, const int *seq, int nseq)
{
	if(!nseq) {
		fprintf(fp, "%s.", db);
		return;
	}
	fputs("fk", fp);
	for(int m=0; m < nseq; m++)
		fprintf(fp, "_%d", seq[m]);
	putc('.', fp);
}

static void pr_sql_fkey_fields(FILE *fp, const char *db, ITEM *item,
			       const int *seq, int nseq, bool has_groupby,
			       bool is_agg)
{
	int n, m;
	bool didone = false;
	is_agg |= item->type == IT_INV_FKEY || IFL(item->,FKEY_MULTI);
	resolve_fkey_fields(item);
	const FORM *fform = item->fkey_form;
	for(n = 0; n < item->nfkey; n++) {
		if(!item->fkey[n].display)
			continue;
		if(didone)
			fputs(", ", fp);
		didone = true;
		if(!item->fkey[n].item)
			continue;
		ITEM *fitem = item->fkey[n].item;
		if(fitem->type == IT_FKEY) {
			int fseq[nseq + 1];
			memcpy(fseq, seq, nseq * sizeof(int));
			fseq[nseq] = n;
			pr_sql_fkey_fields(fp, db, item, fseq, nseq + 1, has_groupby, is_agg);
			continue;
		}
		const char *label = IFL(fitem->,MULTICOL) ?
				  item->fkey[n].menu->label : fitem->label;
		bool is_agg = false;
		if(has_groupby) {
			if(is_agg)
				fputs("\\{IF -p}string_agg\\{ELSE}group_concat\\{END}(coalesce(", fp);
			else
				fputs("max(", fp);
		}
		if(fitem->type == IT_TIME)
			pr_sql_item(fp, db, fform, item->fkey[n].index % fform->nitems,
				    0, label, seq, nseq, has_groupby);
		else {
			pr_sql_tq(fp, db, seq, nseq);
			fputs(IFL(fitem->,MULTICOL) ? item->fkey[n].menu->name
						    : fitem->name, fp);
		}
		if(has_groupby) {
			if(is_agg)
				/* FIXME:  should this be arraysep? */
				/*         if so, array-esc items? */
				fputs(",''), CH\\{IF +p}A\\{ENDIF}R(10))", fp);
			else
				putc(')', fp);
		}
		fputs(" \"", fp);
		pr_dq(fp, label, '"');
		/* alias to avoid name conflicts w/ other tables */
		/* FIXME: use field labels as prefix instead of this suffix */
		for(m=0; m < nseq; m++)
			fprintf(fp, "_%d", seq[m]);
		putc('"', fp);
	}
}

static void pr_sql_fkey_tables(FILE *fp, const char *db, ITEM *item,
			       const int *seq, int nseq)
{
	int n, m;
	if(IFL(item->,FKEY_MULTI)) {
		/* note: only one ID field needs to be made per table */
		/* but queries make more sense if there is one per FKEY_MULTI */
		fprintf(fp, "\n    LEFT OUTER JOIN %s_fk%d ON %s_fk%d.%s_id = %s.%s",
			db, *seq, db, *seq, item->name, db, item->name);
	}
	resolve_fkey_fields(item);
	fprintf(fp, "\n    LEFT OUTER JOIN %s fk", item->fkey_form->name);
	for(n = 0; n < nseq; n++)
		fprintf(fp, "_%d", seq[n]);
	fputs(" ON ", fp);
	for(n = m = 0; n < item->nfkey; n++) {
		if(!item->fkey[n].key)
			continue;
		if(m)
			fputs(" AND ", fp);
		if(nseq == 1) {
			fputs(db, fp);
			if(IFL(item->,FKEY_MULTI))
				fprintf(fp, "_fk%d", *seq);
		} else {
			fputs("fk", fp);
			for(int i=0; i < nseq - 1; i++)
				fprintf(fp, "_%d", seq[i]);
		}
		fprintf(fp, ".%s", item->name);
		if(m++)
			fprintf(fp, "_k%d", m);
		fputs(" = fk", fp);
		for(int i=0; i < nseq; i++)
			fprintf(fp, "_%d", seq[i]);
		const ITEM *fitem = item->fkey[n].item;
		fprintf(fp, ".%s", IFL(fitem->,MULTICOL) ?
				item->fkey[n].menu->name :
				fitem->name);
	}
	for(n = 0; n < item->nfkey; n++) {
		ITEM *fitem = item->fkey[n].item;
		if(fitem->type == IT_FKEY) {
			int fseq[nseq + 1];
			memcpy(fseq, seq, nseq * sizeof(int));
			fseq[nseq] = n;
			pr_sql_fkey_tables(fp, db, fitem, fseq, nseq + 1);
		}
	}
}

static void add_ref_keys(QStringList &sl, const char *cform, const FORM *form)
{
	const FORM *fform = read_form(cform, false, 0);
	if(!fform)
		return;
	for(int i = 0; i < fform->nitems; i++) {
		ITEM *fitem = fform->items[i];
		resolve_fkey_fields(fitem);
		/* FIXME:  verify this works; may need strcmp(name) */
		if(fitem->type == IT_FKEY && fitem->fkey_form == form) {
			QString ks, ds;
			for(int j = 0; j < fitem->nfkey; j++) {
				const ITEM *kitem = fitem->fkey[j].item;
				const char *name;
				if(IFL(kitem->,MULTICOL))
					name = fitem->fkey[j].menu->name;
				else
					name = kitem->name;
				if(fitem->fkey[j].key) {
					if(ks.length())
						ks += ',';
					ks += name;
				}
				if(fitem->fkey[j].display) {
					if(ds.length())
						ds += ',';
					ds += name;
				}
			}
			sl.append(ks);
			if(!IFL(fitem->,RDONLY))
				sl.append(ds);
		}
	}
}

/* stupid 3 functions in one */
static bool any_fkey_multi(const FORM *form, const struct sum_item *itemorder,
			   int nitems, const ITEM *item)
{
	/* per item */
	if(item) {
#if 0
		if(item->type == IT_INV_FKEY)
			return true;
#endif
		if(item->type != IT_FKEY)
			return false;
		if(IFL(item->,FKEY_MULTI))
			return true;
		for(int j = 0; j < item->nfkey; j++)
			if(any_fkey_multi(0, 0, 0, item->fkey[j].item))
				return true;
		return false;
	}
	/* summary */
	if(itemorder) {
		for(int j = 0; j < nitems; j++) {
			const CARD *c;
			for(c = itemorder[j].fcard; c->fkey_next; c = c->fkey_next) {
				const ITEM *fitem = c->fkey_next->form->items[c->qcurr];
				if((fitem->type == IT_FKEY && IFL(fitem->,FKEY_MULTI)))
					return true;
#if 0
				if(fitem->type == IT_INV_FKEY)
					return true;
#endif
			}
		}
		return false;
	}
	/* full form */
	for(int i = 0; i < form->nitems; i++)
		if(any_fkey_multi(0, 0, 0, form->items[i]))
			return true;
	return false;
}
