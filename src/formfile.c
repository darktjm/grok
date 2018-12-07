/*
 * read and write form files.
 *
 *	write_form(form)		write form
 *	read_form(form, path)		read form into empty form struct
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <float.h>
#include <time.h>
#include <QtWidgets>
#include "config.h"

#if defined(GROK) || defined(PLANGROK)

#include "grok.h"
#include "form.h"
#include "proto.h"

#ifdef PLANGROK
static DQUERY *add_dquery(FORM *fp) {return(0);}
static void remake_dbase_pulldown(void) {}
#endif

#define STORE(t,s) do{zfree(t);t=mystrdup(s);}while(0)

static const char * const itemname[NITEMS] = {
	"None", "Label", "Print", "Input", "Time", "Note", "Choice", "Flag",
	"Button", "Chart", "Number", "Menu", "Radio", "Multi", "Flags" };


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
		create_error_popup(mainwindow, 0,
			"Form has no name, cannot save to disk");
		return(FALSE);
	}
	if (!form->path)
		path = resolve_tilde(path, "gf");
	if (!(fp = fopen(path, "w"))) {
		create_error_popup(mainwindow, errno,
			"Failed to create form file %s", path);
		return(FALSE);
	}
	/* don't write values that are at the default (see set_form_defaults()) */
	/* if default is not 0, add arg to macro to check default */
#define write_str(prop, val) do { if(val) fprintf(fp, prop "%s\n", val); } while(0)
#define write_int(prop, val, ...) do { if(val __VA_ARGS__) fprintf(fp, prop "%ld\n", (long)(val)); } while(0)
#define write_2int(prop, val1, val2) \
    do { if(val1 || val2) fprintf(fp, prop "%ld %ld\n", (long)(val1), (long)(val2)); } while(0)
#define write_2intc(prop, val1, chk1, val2, chk2) \
    do { if(val1 chk1 || val2 chk2) fprintf(fp, prop "%ld %ld\n", (long)(val1), (long)(val2)); } while(0)
#define write_2float(prop, val1, val2) \
    do { if(val1 || val2) fprintf(fp, prop "%.*lg %.*lg\n", DBL_DIG + 1, val1, DBL_DIG + 1, val2); } while(0)
#define write_2floatc(prop, val1, chk1, val2, chk2) \
    do { if(val1 chk1 || val2 chk2) fprintf(fp, prop "%.*lg %.*lg\n", DBL_DIG + 1, val1, DBL_DIG + 1, val2); } while(0)
	fputs("grok\n", fp);
	write_str("name       ", form->name);
	write_str("dbase      ", form->dbase);
	write_str("comment    ", form->comment);
	write_str("cdelim     ", to_octal(form->cdelim)); /* always written */
	if ((form->asep && form->asep != '|') ||
	    (form->aesc && form->aesc != '\\')) {
	    /* to_octal isn't re-entrant, so one at a time */
	    fprintf(fp, "adelim     %s ", to_octal(form->asep ? form->asep : '|'));
	    fprintf(fp, "%s\n", to_octal(form->aesc ? form->aesc : '\\'));
	}
	write_int("rdonly     ", form->rdonly);
	write_int("proc       ", form->proc);
	write_int("syncable   ", form->syncable, != TRUE);
	write_2intc("grid       ", form->xg, != 4, form->yg, != 4);
	write_2intc("size       ", form->xs, != 400, form->ys, != 200);
	write_int("divider    ", form->ydiv);
	write_int("autoq      ", form->autoquery, != -1);
	write_str("planquery  ", form->planquery);

	for (p=form->help; p && *p; ) {
		fputs("help\t'", fp);
		while (*p && *p != '\n')
			fputc(*p++, fp);
		fputc('\n', fp);
		if (*p) p++;
	}

	for (dq=form->query, i=0; i < form->nqueries; i++, dq++) {
		if (!dq->name && !dq->query)
			continue;
		/* reader relies on query_s being present, so always write */
		write_int("query_s    ", dq->suspended, || true);
		write_str("query_n    ", dq->name);
		write_str("query_q    ", dq->query);
	}
	for (i=0; i < form->nitems; i++) {
		item = form->items[i];
		fputs("\nitem\n", fp);
		/* see item_create() for defaults */
		write_str("type       ", itemname[item->type]); /* always written */
		/* following 4 have dynamic defaults, so they're always written */
		write_str("name       ", STR(item->name));
		write_2intc("pos        ", item->x,  || true, item->y, || true);
		write_2intc("size       ", item->xs, || true, item->ys, || true);
		write_2intc("mid        ", item->xm, || true, item->ym, || true);
		write_int("sumwid     ", item->sumwidth);
		write_int("sumcol     ", item->sumcol);
		write_str("sumprint   ", item->sumprint);
		write_int("column     ", item->column, || true); /* dynamic default */
		write_int("search     ", item->search);
		write_int("rdonly     ", item->rdonly);
		write_int("nosort     ", item->nosort);
		write_int("defsort    ", item->defsort);
		write_int("timefmt    ", item->timefmt);
		write_int("timewidget ", item->timewidget);
		write_str("code       ", item->flagcode);
		write_str("codetxt    ", item->flagtext);
		write_str("label      ", item->label);
		write_int("ljust      ", item->labeljust, != J_LEFT);
		write_int("lfont      ", item->labelfont, || true); /* dynamic default */
		write_str("gray       ", item->gray_if);
		write_str("freeze     ", item->freeze_if);
		write_str("invis      ", item->invisible_if);
		write_str("skip       ", item->skip_if);
		if(IS_MENU(item->type)) {
			write_int("mcol       ", item->multicol);
			write_int("nmenu      ", item->nmenu);
			for(int n = 0; n < item->nmenu; n++) {
				MENU &m = item->menu[n];
				write_str("menu       ", STR(m.label)); // should never by 0
				write_str("_m_code    ", m.flagcode);
				write_str("_m_codetxt ", m.flagtext);
				write_str("_m_name    ", m.name);
				write_int("_m_column  ", m.column);
				write_int("_m_sumcol  ", m.sumcol);
				write_int("_m_sumwid  ", m.sumwidth);
			}
			if(item->type == IT_INPUT)
				write_int("dcombo     ", item->dcombo);
		}
		write_str("default    ", item->idefault);
		write_int("maxlen     ", item->maxlen, != 100);
		if(item->type == IT_NUMBER) {
			write_2float("range      ", item->min, item->max);
			write_int("digits     ", item->digits);
		}
		write_int("ijust      ", item->inputjust, != J_LEFT);
		write_int("ifont      ", item->inputfont, || true); /* dynamic default */
		write_str("p_act      ", item->pressed);
		if (item->plan_if)
		    fprintf(fp, "plan_if    %c\n",	item->plan_if);

		if(item->type == IT_CHART) {
			write_2floatc("ch_xrange  ", item->ch_xmin,,
						     item->ch_xmax, != 1.0);
			write_2floatc("ch_yrange  ", item->ch_ymin,,
						     item->ch_ymax, != 1.0);
			write_2int("ch_auto    ", item->ch_xauto,
						  item->ch_yauto);
			write_2float("ch_grid    ",	item->ch_xgrid,
							item->ch_ygrid);
			write_2float("ch_snap    ",	item->ch_xsnap,
							item->ch_ysnap);
			write_int("ch_ncomp   ", item->ch_ncomp);

			for (c=0; c < item->ch_ncomp; c++) {
			    chart = &item->ch_comp[c];
			    fputs("chart\n", fp);
			    write_int("_ch_type   ", chart->line);
			    write_2int("_ch_fat    ", chart->xfat,
							chart->yfat);
			    write_str("_ch_excl   ", chart->excl_if);
			    write_str("_ch_color  ", chart->color);
			    write_str("_ch_label  ", chart->label);
			    for (v=0; v < 4; v++) {
				struct value *val = &chart->value[v];
#define write_chval(f, t, ...) do { \
    if(val->f) \
	fprintf(fp, "_ch_" #f "%d  " t "\n", v, __VA_ARGS__ val->f); \
} while(0)
#define write_chflt(f) write_chval(f, "%.*lg", DBL_DIG + 1,)
				write_chval(mode, "%d");
				write_chval(expr, "%s");
				write_chval(field, "%d");
				write_chflt(mul);
				write_chflt(add);
			    }
			}
		}
	}
	fclose(fp);
	remake_dbase_pulldown();

	/* This used to only support relative .db files in GROKDIR */
	/* The loading routines only check the same dir as the .gf, though */
	path = strdup(path);
	i = strlen(path);
	if(i < 3 || strcmp(path + i - 3, ".gf"))
		return(TRUE); /* invalid, really, but leae it alone */
	/* The loader checks .db first, but it doesn't matter what order */
	/* it's checked here, other than creation wanting the .db extencsion */
	path[i] = 0;
	/* On the other hand, having a non-readable non-db file is useless */
	if (access(path, form->proc ? X_OK : R_OK))
		strcpy(path + i, ".db");
	/* auto-creation of procedurals won't work */
	/* best to just force user to edit */
	/* if they want to do it later, they can set it to /bintrue */
	if (!form->proc && access(path, F_OK) && errno == ENOENT) {
		if (!(fp = fopen(path, "w"))) {
			create_error_popup(mainwindow, errno,
"The form was created successfully, but the\n"
"database file %s cannot be created.\n"
"No cards can be entered into the new Form.\n\nProblem: ", path);
			return(FALSE);
		}
		fclose(fp);
	}
	if (access(path, form->proc ? X_OK : R_OK)) {
		create_error_popup(mainwindow, errno,
"The form was created successfully, but the\n"
"database file %s exists but is not readable.\n"
"No cards can be entered into the new Form.\n\nProblem: ", path);
		free(path);
		return(FALSE);
	}
	free(path);
	return(TRUE);
}
#endif /* GROK */


/*
 * Read the form and all the items in it from a file.
 * Returns FALSE and leave *form untouched if the file could not be read.
 */

static FILE *try_path(const char *path, char **fname)
{
	static char buf[1024]; /* ugh! */
	int len;
	FILE *fp;
	len = sprintf(buf, "%s/%s", path, *fname);
	if(len > 3 && strcmp(buf + len - 3, ".gf"))
		strcpy(buf + len, ".gf");
	if((fp = fopen(buf, "r"))) {
		fprintf(stderr, "found %s\n", buf);
		*fname = buf;
		return fp;
	}
	return fp;
}

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
	CHART			*chart = 0;	/* next chart component */
	MENU			*menu = 0;	/* next menu item */

	/* Use same path as the GUI for relative names */
	if(!strchr(path, '/')) {
		do {
			char *env;
			env = getcwd(line, sizeof(line));
			env = getenv("GROK_FORM");
			if((fp = try_path(env ? env : line, &path)))
				break;
			strcat(line, "/grokdir");
			if((fp = try_path(line, &path)))
				break;
			if((fp = try_path(resolve_tilde((char *)GROKDIR, NULL), &path)))
				break;
			fp = try_path(LIB "/grokdir", &path);
		} while(0);
			
	} else {
		path = resolve_tilde(path, "gf");
		fp = fopen(path, "r");
	}
	if (!fp) {
		create_error_popup(mainwindow, errno,
			"Failed to open form file %s", path);
		return(FALSE);
	}
	form_delete(form);
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
			chart = NULL;
			menu = NULL;
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
			else if (!strcmp(key, "adelim")) {
					form->asep = to_ascii(p, '|');
					while(*p && !isspace(*p))
						p++;
					form->aesc = to_ascii(p, '\\');
			} else if (!strcmp(key, "grid"))
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
				if ((dq = add_dquery(form)))
					dq->suspended = *p != '0';
			} else if (!strcmp(key, "query_n")) {
				if (dq)
					STORE(dq->name, p);
			} else if (!strcmp(key, "query_q")) {
				if (dq)
					STORE(dq->query, p);
			} else if (!strcmp(key, "help") && (*p++ == '\'' ||
							    p[-1] == '`')) {
				if (form->help) {
					if ((form->help = (char *)realloc(form->help,
							   strlen(form->help) +
							   strlen(p) + 2)))
						strcat(form->help, p);
				} else {
					if ((form->help = (char *)malloc(strlen(p) + 2)))
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
				item->type = (ITYPE)i;
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
			else if (!strcmp(key, "sumprint"))
					STORE(item->sumprint, p);
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
					item->timefmt = (TIMEFMT)atoi(p);
			else if (!strcmp(key, "timewidget"))
					item->timewidget = atoi(p);
			else if (!strcmp(key, "code"))
					STORE(item->flagcode, p);
			else if (!strcmp(key, "codetxt"))
					STORE(item->flagtext, p);
			else if (!strcmp(key, "label"))
					STORE(item->label, p);
			else if (!strcmp(key, "ljust"))
					item->labeljust = (JUST)atoi(p);
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
			else if (!strcmp(key, "mcol"))
				 	item->multicol = atoi(p);
			else if (!strcmp(key, "nmenu")) {
					if ((item->nmenu = atoi(p)))
						item->menu =
							(MENU *)calloc(item->nmenu,
								sizeof(MENU));
			}
			else if (!strcmp(key, "menu")) {
					menu = !menu ?
						item->menu : menu+1;
					if(menu) STORE(menu->label, p);
			}
			else if (menu && !strcmp(key, "_m_code"))
					STORE(menu->flagcode, p);
			else if (menu && !strcmp(key, "_m_codetxt"))
					STORE(menu->flagtext, p);
			else if (menu && !strcmp(key, "_m_name"))
					STORE(menu->name, p);
			else if (menu && !strcmp(key, "_m_column"))
					menu->column = atoi(p);
			else if (menu && !strcmp(key, "_m_sumcol"))
					menu->sumcol = atoi(p);
			else if (menu && !strcmp(key, "_m_sumwid"))
					menu->sumwidth = atoi(p);
			else if (!strcmp(key, "dcombo"))
					item->dcombo = (DCOMBO)atoi(p);
			else if (!strcmp(key, "default"))
					STORE(item->idefault, p);
			else if (!strcmp(key, "maxlen"))
					item->maxlen = atoi(p);
			else if (!strcmp(key, "range"))
					sscanf(p, "%lg %lg", &item->min, &item->max);
			else if (!strcmp(key, "digits"))
					item->digits = atoi(p);
			else if (!strcmp(key, "ijust"))
					item->inputjust = (JUST)atoi(p);
			else if (!strcmp(key, "ifont"))
					item->inputfont = atoi(p);
			else if (!strcmp(key, "p_act"))
					STORE(item->pressed, p);
			else if (!strcmp(key, "showplan"))
					;	/* 1.5: now called planquery */
			else if (!strcmp(key, "plan_if"))
					item->plan_if = *p;

			else if (!strcmp(key, "ch_xrange"))
					sscanf(p, "%lg %lg", &item->ch_xmin,
							   &item->ch_xmax);
			else if (!strcmp(key, "ch_yrange"))
					sscanf(p, "%lg %lg", &item->ch_ymin,
							   &item->ch_ymax);
			else if (!strcmp(key, "ch_auto"))
					sscanf(p, "%d %d", &item->ch_xauto,
							   &item->ch_yauto);
			else if (!strcmp(key, "ch_grid"))
					sscanf(p, "%lg %lg", &item->ch_xgrid,
							   &item->ch_ygrid);
			else if (!strcmp(key, "ch_snap"))
					sscanf(p, "%lg %lg", &item->ch_xsnap,
							   &item->ch_ysnap);
			else if (!strcmp(key, "ch_ncomp")) {
					if ((item->ch_ncomp = atoi(p)))
						item->ch_comp =
							(CHART *)calloc(item->ch_ncomp,
								sizeof(CHART));
			}
			else if (!strcmp(key, "chart"))
					chart = !chart ?
						item->ch_comp : chart+1;
			else if (chart && !strcmp(key, "_ch_type"))
					chart->line = atoi(p);
			else if (chart && !strcmp(key, "_ch_fat"))
					sscanf(p, "%d %d", &chart->xfat,
							   &chart->yfat);
			else if (chart && !strcmp(key, "_ch_excl"))
					STORE(chart->excl_if, p);
			else if (chart && !strcmp(key, "_ch_color"))
					STORE(chart->color, p);
			else if (chart && !strcmp(key, "_ch_label"))
					STORE(chart->label, p);
			else if (chart && !strncmp(key, "_ch_mode", 8))
					chart->value[key[8]-'0'].mode =atoi(p);
			else if (chart && !strncmp(key, "_ch_field", 9))
					chart->value[key[9]-'0'].field=atoi(p);
			else if (chart && !strncmp(key, "_ch_mul", 7))
					chart->value[key[7]-'0'].mul = atof(p);
			else if (chart && !strncmp(key, "_ch_add", 7))
					chart->value[key[7]-'0'].add = atof(p);
			else if (chart && !strncmp(key, "_ch_expr", 8))
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
	/* verify_form() will also create the symtab */
	if(!verify_form(form, NULL, mainwindow)) {
		form_delete(form);
		return(FALSE);
	}
	return(TRUE);
}

#endif /* GROK || PLANGROK */
