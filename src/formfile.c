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
#include <set>
#include <vector>

static const char * const itemname[NITEMS] = {
	"None", "Label", "Print", "Input", "Time", "Note", "Choice", "Flag",
	"Button", "Chart", "Number", "Menu", "Radio", "Multi", "Flags",
	"Reference", "Referers"};


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
	if (!form->path) {
		path = resolve_tilde(path, "gf");
		form->dir = mystrdup(canonicalize(path, true));
		form->path = mystrdup(canonicalize(path, false));
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
	write_str("dbase      ", form->dbname);
	write_str("comment    ", form->comment);
	write_str("cdelim     ", to_octal(form->cdelim)); /* always written */
	if ((form->asep && form->asep != '|') ||
	    (form->aesc && form->aesc != '\\')) {
	    /* to_octal isn't re-entrant, so one at a time */
	    fprintf(fp, "adelim     %s ", to_octal(form->asep ? form->asep : '|'));
	    fprintf(fp, "%s\n", to_octal(form->aesc ? form->aesc : '\\'));
	}
	write_int("sumheight  ", form->sumheight);
	write_int("rdonly     ", form->rdonly);
	write_int("proc       ", form->proc);
	write_int("syncable   ", form->syncable, != true);
	write_2intc("grid       ", form->xg, != 4, form->yg, != 4);
	write_2intc("size       ", form->xs, != 400, form->ys, != 200);
	write_int("divider    ", form->ydiv);
	write_int("autoq      ", form->autoquery, != -1);
	write_str("planquery  ", form->planquery);
	for (i=0; i < form->nreferer; i++)
		write_str("child      ", form->referer[i]);

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
		write_int("search     ", IFL(item->,SEARCH));
		write_int("rdonly     ", IFL(item->,RDONLY));
		write_int("nosort     ", IFL(item->,NOSORT));
		write_int("defsort    ", IFL(item->,DEFSORT));
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
			write_int("mcol       ", IFL(item->,MULTICOL));
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
			write_2int("ch_auto    ", IFL(item->,CH_XAUTO),
						  IFL(item->,CH_YAUTO));
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
		if (item->type == IT_FKEY || item->type == IT_INV_FKEY) {
			write_str("fk_db      ", item->fkey_form_name);
			write_int("fk_header  ", IFL(item->,FKEY_HEADER));
#if 0
			write_int("fk_search  ", IFL(item->,FKEY_SEARCH));
#endif
			write_int("fk_multi   ", IFL(item->,FKEY_MULTI));
			write_int("nfkey      ", item->nfkey);
			for (int n = 0; n < item->nfkey; n++) {
				write_str("fkey       ", item->fkey[n].name);
				write_int("_fk_key    ", item->fkey[n].key);
				write_int("_fk_disp   ", item->fkey[n].display);
			}
		}
	}
	fclose(fp); /* FIXME: report errors */
	remake_dbase_pulldown();
	path = db_path(form);
	/* auto-creation of procedurals won't work */
	/* best to just force user to edit */
	/* if they want to do it later, they can set it to /bin/true */
	if (!form->proc && access(path, F_OK) && errno == ENOENT) {
		if (!(fp = fopen(path, "w"))) {
			create_error_popup(mainwindow, errno,
"The form was created successfully, but the\n"
"database file %s cannot be created.\n"
"No cards can be entered into the new Form.\n\nProblem: ", path);
			return(false);
		}
		fclose(fp);
	}
	if (access(path, form->proc ? X_OK : R_OK)) {
		create_error_popup(mainwindow, errno,
"The form was created successfully, but the\n"
"database file %s exists but is not readable.\n"
"No cards can be entered into the new Form.\n\nProblem: ", path);
		return(false);
	}
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
	if(len < 3 || strcmp(buf + len - 3, ".gf"))
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

struct tab_loading {
	const char *path;
	const char *dir;
};

static bool operator<(tab_loading a, tab_loading b)
{
	int c = strcmp(a.dir, b.dir);
	if(c)
		return c < 0 ? true : false;
	return strcmp(a.path, b.path) < 0;
}

static std::set<tab_loading> loading_forms;

FORM *read_form(
	const char		*path,		/* file to read list from */
	bool			force,		/* overrwrite loaded forms */
	QWidget			*parent)	/* error popup parent */
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
	FKEY			*fkey = 0;	/* next fkey key field */

	/* Use same path as the GUI for relative names */
	if(!strchr(path, '/')) {
		do {
			const char *env = getenv("GROK_FORM");
			if(env && (fp = try_path(env, &path)))
				break;
			/* canonicalize will expand . and relative grokdir */
			if((fp = try_path(".", &path)))
				break;
			if((fp = try_path("grokdir", &path)))
				break;
			if((fp = try_path(resolve_tilde(GROKDIR, NULL), &path)))
				break;
			fp = try_path(LIB "/grokdir", &path);
		} while(0);
	} else {
		path = resolve_tilde(path, "gf");
		fp = fopen(path, "r");
	}
	if (!fp) {
		create_error_popup(parent, errno,
			"Failed to open form file %s", path);
		return NULL;
	}
	p = mystrdup(canonicalize(path, false));
	char *dir = mystrdup(canonicalize(path, true));
	tab_loading tl = { p, dir };
	if (loading_forms.find(tl) != loading_forms.end()) {
		zfree(dir);
		zfree(p);
		return 0;
	}
	if(!force)
		for(form = form_list; form; form = form->next) {
			if(!strcmp(form->path, p)) {
				fclose(fp);
				if(strcmp(form->dir, dir)) {
					/* same form symlinked to a different dir */
					for(FORM *f = form->next; f; f = f->next)
						if(!strcmp(f->path, p) &&
						   !strcmp(f->dir, dir)) {
							/* or maybe not */
							free(p);
							free(dir);
							f->deleted = false;
							return f;
						}
					form = form_clone(form);
					form->path = p;
					form->dir = dir;
					verify_form(form, 0, 0); // to create symtab
				} else {
					free(p);
					free(dir);
				}
				return form;
			}
		}
	loading_forms.insert(tl);
	form = form_create();
	form->path = p;
	form->dir = dir;
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
				/* this.. isn't really recoverable */
				form_delete(form);
				return NULL;
			}
			chart = NULL;
			menu = NULL;
			fkey = NULL;
			item  = form->items[form->nitems-1];
			IFC(item->,SELECTED);
			item->ch_curr  = 0;
								/* header */
		} else if (!item) {
			if (!strcmp(key, "grok"))
					;
			else if (!strcmp(key, "name"))
					STORE(form->name, p);
			else if (!strcmp(key, "dbase"))
					STORE(form->dbname, p);
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
			} else if (!strcmp(key, "sumheight"))
					form->sumheight = atoi(p);
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
			else if (!strcmp(key, "child")) {
					zgrow(0, "form file", char *, form->referer,
					      form->nreferer, form->nreferer + 1, 0);
					STORE(form->referer[form->nreferer], p);
					++form->nreferer;
			} else if (!strcmp(key, "query_s")) {
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
					IFV(item->,SEARCH,atoi(p));
			else if (!strcmp(key, "rdonly"))
					IFV(item->,RDONLY, atoi(p));
			else if (!strcmp(key, "nosort"))
					IFV(item->,NOSORT, atoi(p));
			else if (!strcmp(key, "defsort"))
					IFV(item->,DEFSORT, atoi(p));
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
				 	IFV(item->,MULTICOL, atoi(p));
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
					IFV(item->,CH_XAUTO, xauto);
					IFV(item->,CH_YAUTO, yauto);
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
			else if (!strcmp(key, "fk_db"))
					STORE(item->fkey_form_name, p);
			else if (!strcmp(key, "fk_header"))
					IFV(item->,FKEY_HEADER, atoi(p));
#if 0
			else if (!strcmp(key, "fk_search"))
					IFV(item->,FKEY_SEARCH, atoi(p));
#endif
			else if (!strcmp(key, "fk_multi"))
					IFV(item->,FKEY_MULTI, atoi(p));
			else if (!strcmp(key, "nfkey"))
					item->fkey = zalloc(0, "form file",
							    FKEY, (item->nfkey = atoi(p)));
			else if (!strcmp(key, "fkey")) {
					fkey = !fkey ? item->fkey : fkey+1;
					if (!fkey) {
						fprintf(stderr, "no nfkey specified; fkey ignored");
						continue;
					}
					STORE(fkey->name, p);
			}
			else if (fkey && !strcmp(key, "_fk_key"))
					fkey->key = atoi(p) ? true : false;
			else if (fkey && !strcmp(key, "_fk_disp"))
					fkey->display = atoi(p) ? true : false;
			else
				fprintf(stderr,
					"%s: %s: ignored item keyword %s\n",
							progname, path, key);
		}
	}
	fclose(fp);
	/* verify_form() will also create the symtab */
	if(!verify_form(form, NULL, parent)) {
		loading_forms.erase(tl);
		form_delete(form);
		return NULL;
	}
	loading_forms.erase(tl);
	if(!force) {
		form->next = form_list;
		form_list = form;
		return form;
	}
	/* for forced loads, replace all versions of this path in form list */
	/* first pass: replace forms and collect possible dbase replacements */
	/* replacments will be stored here.  This will sort of screw up the */
	/* lru order of dbase_list, but I don't care */
	DBASE *repl_dbase = 0;
	for(FORM **prev = &form_list; *prev; )
		if(!strcmp((*prev)->path, form->path)) {
			FORM *nform, *oform = *prev;
			if(!strcmp((*prev)->dir, form->dir)) {
				nform = form;
				*prev = oform->next;
			} else {
				nform = form_clone(form);
				nform->path = oform->path;
				nform->dir = oform->dir;
				nform->next = oform->next;
				verify_form(nform, 0, 0); /* create symtab */
				*prev = nform;
				prev = &(*prev)->next;
				oform->path = oform->dir = NULL;
			}
			const char *path = db_path(nform);
			for(DBASE **dbp = &dbase_list; *dbp; )
				if(*dbp == oform->dbase) {
					if(!strcmp(path, (*dbp)->path) &&
					   oform->proc == nform->proc &&
					   (!oform->proc || !strcmp(oform->name, nform->name))) {
						nform->dbase = oform->dbase;
						dbp = &(*dbp)->next;
						continue;
					}
					/* save db updates for 2nd pass */
					DBASE *r = *dbp;
					*dbp = r->next;
					r->next = repl_dbase;
					repl_dbase = r;
				} else
					dbp = &(*dbp)->next;
			for(CARD *card = card_list; card; card = card->next)
				if(card->form == oform) {
					/* it's unsafe to readback texts with new form */
					card_readback_texts(card, -1);
					card->form = nform;
				}
			oform->dbase = 0;
			form_delete(oform);
			if(!*prev)
				break;
		} else
			prev = &(*prev)->next;
	form->next = form_list;
	form_list = form;
	/* 2nd pass: update databases if form->dbname/proc/name changed */
	while(repl_dbase) {
		DBASE *odbase = repl_dbase;
		repl_dbase = odbase->next;
		bool gone = true;
		for(FORM **prev = &form_list; prev; ) {
			FORM *f = *prev;
			if(f->dbase != odbase) {
				prev = &(*prev)->next;
				continue;
			}
			/* no point in re-reading cache placeholders */
			if(f->deleted) {
				*prev = f->next;
				f->dbase = 0;
				form_delete(f);
				continue;
			}
			read_dbase(f);
			/* in case you actually have 2 forms w/ same dbase */
			gone = false;
			prev = &(*prev)->next;
		}
		if(!gone) {
			dbase_clear(odbase);
			free(odbase);
		}
	}
	/* 3rd pass: reload windows using affected forms */
	for(CARD *card = card_list; card; card = card->next)
		/* FIXME: verify fkeys never get caught here: fkey_next, fksel->fcard */
		/*  fkey_next would be easy enough to check/fix here, but not fcard */
		if(card->wform && !strcmp(card->form->path, form->path)) {
			if(card->nitems != form->nitems) {
				CARD *ncard = card;
				bgrow(0, "new form", CARD, card, items, form->nitems, NULL);
				if(card != ncard) {
					if(mainwindow->card == card)
						mainwindow->card = ncard;
					for(CARD **pc = &card_list; *pc; pc = &(*pc)->next)
						if(*pc == card) {
							*pc = ncard;
							break;
						}
				        card = ncard;
				}
				card->nitems = form->nitems;
			}
			QWidget *wform = card->wform;
			destroy_card_menu(card);
			build_card_menu(card, wform);
			fillout_card(card, false);
		}
	dbase_prune();
	return form;
}

static int bcomp_name(
	const void	*u,
	const void	*v)
{
	return(strcmp((char *)u, *(char **)v));
}

FORM *read_child_form(
	const FORM		*form,
	const char		*child)
{
	const char **cname =
		(const char **)bsearch(child, form->childname, form->nchild,
				       sizeof(*form->childname), bcomp_name);
	if(!cname)
		return read_form(child, false, NULL);
	int cno = cname - form->childname;
	if(!form->childform[cno])
		form->childform[cno] = read_form(child, false, NULL);
	return form->childform[cno];
}
