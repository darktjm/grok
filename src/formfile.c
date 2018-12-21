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
#include "grok.h"
#include "form.h"
#include "proto.h"

static const char * const itemname[NITEMS] = {
	"None", "Label", "Print", "Input", "Time", "Note", "Choice", "Flag",
	"Button", "Chart", "Number", "Menu", "Radio", "Multi", "Flags" };


static bool create_form_db(const char *path, bool proc);


/*
 * Write the form and all the items in it. Also create the database named
 * in the form if it doesn't exist.  Also link into main list, replacing
 * form with same path if present (FIXME: canonicalize path first)
 * Returns false if the file could not be written.
 */

bool write_form(
	FORM		*form)		/* form and items to write */
{
	const char	*path;		/* file name to write to */
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
		return(false);
	}
	if (!form->path) {
		path = resolve_tilde(path, "gf");
		if (!(form->path = strdup(path))) {
			create_error_popup(mainwindow, errno,
				"No memory for form path");
			return(false);
		}
	}
	if (!(fp = fopen(path, "w"))) {
		create_error_popup(mainwindow, errno,
			"Failed to create form file %s", path);
		return(false);
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
	write_int("syncable   ", form->syncable, != true);
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
	fclose(fp); /* FIXME: report errors */
	form->next = form_list;
	form_list = form;
	/* FIXME: this isn't an exact match for how it's done in mainwin.c */
	const char *ndb = form_list->dbase ? form_list->dbase : form_list->name;
	DBASE *new_db = NULL;
	for (form = form_list->next; form; form = form->next)
		if(!strcmp(form->path, path))
			break;
	if(form) {
		CARD *card;
		DBASE *del_db = NULL;
		for (card = card_list; card; )
			if(card->form == form) {
				QWidget *w = card->wform;
				QWidget *sh = card->shell;
				if (w || sh) {
					card_readback_texts(card, -1);
					destroy_card_menu(card);
				} if (sh) { /* card was deleted; back to top */
					card = card_list;
					continue;
				}
				card->form = form_list;
				if(new_db != card->dbase) {
					/* FIXME: this isn't an exact match for how it's done in mainwin.c */
					const char *odb = form->dbase ? form->dbase : form->name;
					if(!new_db && !strcmp(STR(odb), STR(ndb))) {
						new_db = card->dbase;
						/* ensure all prior ones were set */
						/* this should actually just be a waste of time */
						card = card_list;
						continue;
					} else {
						del_db = card->dbase;
						card->dbase = new_db;
					}
				}
				if (w)
					build_card_menu(card, w);
				card = card->next;
			}
		if(del_db) {
			/* if form's db changed, old db will likely never be used again */
			for(card = card_list; card; card = card->next)
				if(card->dbase == del_db)
					break;
			if(!card)
				dbase_delete(del_db);
		}
		form_delete(form);
	}
	remake_dbase_pulldown();

	bool created_db = create_form_db(path, form->proc);
	if(!new_db) {
		/* FIXME: code below to create db hasn't been run yet */
		new_db = read_dbase(form_list, ndb);
		if (!new_db)
			new_db = dbase_create();
		for(CARD *card = card_list; card; card = card->next)
			if(card->form == form_list)
				card->dbase = new_db;
	}
	return created_db;
}

static bool create_form_db(const char *path, bool proc)
{
	FILE		*fp;		/* open file */
	int		i;		/* item counter */
	/* This used to only support relative .db files in GROKDIR */
	/* The loading routines only check the same dir as the .gf, though */
	char *dbpath = mystrdup(path);

	i = strlen(dbpath);
	if(i < 3 || strcmp(dbpath + i - 3, ".gf")) {
		free(dbpath);
		return(true); /* invalid, really, but leave it alone */
	}
	/* The loader checks .db first, but it doesn't matter what order */
	/* it's checked here, other than creation wanting the .db extencsion */
	dbpath[i] = 0;
	/* On the other hand, having a non-readable non-db file is useless */
	if (access(dbpath, proc ? X_OK : R_OK))
		strcpy(dbpath + i, ".db");
	/* auto-creation of procedurals won't work */
	/* best to just force user to edit */
	/* if they want to do it later, they can set it to /bintrue */
	if (!proc && access(dbpath, F_OK) && errno == ENOENT) {
		if (!(fp = fopen(dbpath, "w"))) {
			create_error_popup(mainwindow, errno,
"The form was created successfully, but the\n"
"database file %s cannot be created.\n"
"No cards can be entered into the new Form.\n\nProblem: ", path);
			free(dbpath);
			return(false);
		}
		fclose(fp);
	}
	if (access(dbpath, proc ? X_OK : R_OK)) {
		create_error_popup(mainwindow, errno,
"The form was created successfully, but the\n"
"database file %s exists but is not readable.\n"
"No cards can be entered into the new Form.\n\nProblem: ", path);
		free(dbpath);
		return(false);
	}
	free(dbpath);
	return(true);
}


/*
 * Read the form and all the items in it from a file.
 * Returns false and leave *form untouched if the file could not be read.
 */

static FILE *try_path(const char *path, const char **fname)
{
	static char *buf = 0;
	static size_t buflen;
	int len;
	FILE *fp;
	len = strlen(path) + strlen(*fname) + 1;
	grow(0, "form path", char, buf, len + 4, &buflen);
	sprintf(buf, "%s/%s", path, *fname);
	if(len > 3 && strcmp(buf + len - 3, ".gf"))
		strcpy(buf + len, ".gf");
	if((fp = fopen(buf, "r"))) {
		*fname = buf;
		return fp;
	}
	return fp;
}

#define STORE(t,s) do{zfree(t);t=get_string(fp, s, line, sizeof(line));}while(0)

/* This re-runs fgets until EOF or newline */
/* read_form() only reads buflen-1 chars at most */
/* non-EOL is only detected if buflen-1 chars were read, but no \n was stripped */
/* FIXME: this only works if the initial read didn't get EINTR/EAGAIN */
/*        to fix, use a loop to ignore these errors in read_form() */
/* I'm going to ignore EINTR/EAGAIN here as well, for consistency */
/* and because I"m lazy, and because the old code didn't account for */
/* it either, even though it could affect even shorter lines */
static char *get_string(FILE *fp, char *val, char *buf, size_t buflen,
			bool keep_nl = false)
{
	int len = 0, plen = strlen(val);
	if(!plen && !keep_nl)
		return NULL;
	char *out = alloc(0, "form file", char, plen + 2);
	while(1) {
		memcpy(out + len, val, plen);
		if(plen + (int)(val - buf) < (int)buflen - 1 ||
		   !fgets(buf, buflen, fp)) {
			if(keep_nl)
				out[len + plen++] = '\n';
			out[len + plen] = 0;
			return out;
		}
		len += plen;
		plen = strlen(buf);
		if(plen && buf[plen - 1] == '\n')
			buf[--plen] = 0;
		if(!plen) {
			out[len] = 0;
			return out;
		}
		grow(0, "form file", char, out, len + plen + 2, 0);
	}
}

FORM *read_form(
	const char		*path)		/* file to read list from */
{
	FORM			*form;
	FILE			*fp;		/* open file */
	static char		line[1024];	/* line buffer */
	char			*p, *key;	/* next char in line buf */
	DQUERY			*dq = 0;	/* default query pointer */
	ITEM			*item = 0;	/* item pointer, 0=form hdr */
	int			i;		/* item counter */
	CHART			*chart = 0;	/* next chart component */
	MENU			*menu = 0;	/* next menu item */

	/* Use same path as the GUI for relative names */
	if(!strchr(path, '/')) {
		char *cwd = NULL;
		size_t cwdsz;
		do {
			const char *env = getenv("GROK_FORM"), *ret;
			if(env && (fp = try_path(env, &path)))
				break;
			cwd = alloc(0, "cwd", char, (cwdsz = 80));
			while(!(ret = getcwd(cwd, cwdsz))) {
				if(errno != ENAMETOOLONG)
					fatal("cwd is too long");
				fgrow(0, "cwd", char, cwd, (cwdsz *= 2), NULL);
			}
			if((fp = try_path(cwd, &path)))
				break;
			if(strlen(cwd) + 9 > cwdsz)
				grow(0, "cwd", char, cwd, cwdsz + 9, NULL);
			strcat(cwd, "/grokdir");
			if((fp = try_path(cwd, &path)))
				break;
			if((fp = try_path(resolve_tilde(GROKDIR, NULL), &path)))
				break;
			fp = try_path(LIB "/grokdir", &path);
		} while(0);
		zfree(cwd);
	} else {
		path = resolve_tilde(path, "gf");
		fp = fopen(path, "r");
	}
	if (!fp) {
		create_error_popup(mainwindow, errno,
			"Failed to open form file %s", path);
		return NULL;
	}
	for(form = form_list; form; form = form->next)
		if(!strcmp(form->path, path)) {
			fclose(fp);
			return form;
		}
	form = form_create();
	form->path = mystrdup(path);
	for (;;) {
		if (!fgets(line, sizeof(line), fp))
			break;
		if ((i = strlen(line)) && line[i-1] == '\n')
			line[i-1] = 0;
		for (p=line; *p == ' ' || *p == '\t'; p++);
		if (*p == '#' || !*p)
			continue;
		for (key=p; *p && *p != ' ' && *p != '\t'; p++);
		if (*p)
			*p++ = 0;
		for (; *p == ' ' || *p == '\t'; p++);

		if (!strcmp(key, "item")) {
			if (!item_create(form, form->nitems)) {
				form_delete(form);
				return NULL;
			}
			chart = NULL;
			menu = NULL;
			item  = form->items[form->nitems-1];
			item->selected = false;
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
				char *s = get_string(fp, p, line, sizeof(line), true);
				if(!form->help)
					form->help = s;
				else {
					grow(0, "form file", char, form->help,
					     strlen(form->help) + strlen(s) + 1, 0);
					strcat(form->help, s);
					free(s);
				}
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
					item->search = atoi(p) ? true : false;
			else if (!strcmp(key, "rdonly"))
					item->rdonly = atoi(p) ? true : false;
			else if (!strcmp(key, "nosort"))
					item->nosort = atoi(p) ? true : false;
			else if (!strcmp(key, "defsort"))
					item->defsort = atoi(p) ? true : false;
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
			else if (!strcmp(key, "nmenu"))
					item->menu = zalloc(0, "form file",
							    MENU, (item->nmenu = atoi(p)));
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
			else if (!strcmp(key, "ch_auto")) {
					int xauto, yauto;
					sscanf(p, "%d %d", &xauto, &yauto);
					item->ch_xauto = xauto;
					item->ch_yauto = yauto;
			}
			else if (!strcmp(key, "ch_grid"))
					sscanf(p, "%lg %lg", &item->ch_xgrid,
							   &item->ch_ygrid);
			else if (!strcmp(key, "ch_snap"))
					sscanf(p, "%lg %lg", &item->ch_xsnap,
							   &item->ch_ysnap);
			else if (!strcmp(key, "ch_ncomp"))
					item->ch_comp = zalloc(0, "form file",
							      CHART, (item->ch_ncomp = atoi(p)));
			else if (!strcmp(key, "chart"))
					chart = !chart ?
						item->ch_comp : chart+1;
			else if (chart && !strcmp(key, "_ch_type"))
					chart->line = atoi(p);
			else if (chart && !strcmp(key, "_ch_fat")) {
					int xfat, yfat;
					sscanf(p, "%d %d", &xfat,
							   &yfat);
					chart->xfat = xfat;
					chart->yfat = yfat;
			}
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
			else
				fprintf(stderr,
					"%s: %s: ignored item keyword %s\n",
							progname, path, key);
		}
	}
	fclose(fp);
	/* verify_form() will also create the symtab */
	if(!verify_form(form, NULL, mainwindow)) {
		form_delete(form);
		return NULL;
	}
	form->next = form_list;
	form_list = form;
	return form;
}

