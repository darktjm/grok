/*
 * this file is part of the form editor, it is mostly called by callbacks in
 * formwin.c. It allocates forms and items, finds good defaults for things,
 * and does some form geometry operations. Actual form drawing is done in
 * preview.c (for the edit canvas) and cardwin.c (for final cards).
 *
 *	form_create()			allocate form with default parms
 *	form_delete(form)		free form and all items in it
 *	verify_form(form,bug,shell)	make sure that form is consistent
 *	form_edit_script(form,sh,fn)	start up editor for form script
 *	form_sort(form)			sort form's items by y/x position
 *
 *	item_deselect(form)		deselect all items in canvas
 *	item_create(form, nitem)	allocate item and figure out defaults
 *	item_delete(form, nitem)	free item and remove from form
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <QtWidgets>
#include "config.h"

#if defined(GROK) || defined(PLANGROK)

#include "grok.h"
#include "form.h"
#include "proto.h"

#define XSNAP(x)	((x)-(x)%form->xg)
#define YSNAP(y)	((y)-(y)%form->yg)

#ifndef GROK
void clone_chart_component(to, from) CHART *to, *from; {}
void del_chart_component(item) ITEM *item; {}
#endif

static void		set_form_defaults(FORM *);


/*------------------------------------------------- operations on form ------*/
/*
 * allocate a new empty form struct, and fill it with reasonable default
 * paramaters.
 */

FORM *form_create(void)
{
	FORM		*form;		/* new form */

	/* both users assume success, so this should be fatal */
	form = alloc(0, "create form", FORM, 1);
	set_form_defaults(form);
	return(form);
}


static void set_form_defaults(
	FORM		*form)		/* for to initialize */
{
	memset(form, 0, sizeof(FORM));
	form->cdelim	 = ':';
	form->xg	 = 4;
	form->yg	 = 4;
	form->xs	 = XSNAP(400);
	form->ys	 = YSNAP(200);
	form->autoquery  = -1;
	form->syncable	 = true;
}


/*
 * clone a form. This is done when the form editor is started with a template
 * form, we mustn't overwrite the form currently being displayed in the main
 * menu.
 */

#ifdef GROK
FORM *form_clone(
	FORM		*parent)	/* old form */
{
	FORM		*form;		/* new form */
	int		i;

	/* user assumes success, so this should be fatal */
	form = alloc(0, "clone form", FORM, 1);
	*form = *parent;
	form->path    = mystrdup(parent->path);
	form->name    = mystrdup(parent->name);
	form->dbase   = mystrdup(parent->dbase);
	form->comment = mystrdup(parent->comment);
	form->help    = mystrdup(parent->help);

	/* I guess allowing failure on either of these two is OK */
	if (form->nitems &&
	    !(form->items = alloc(mainwindow, "clone form fields", ITEM *, form->size)))
		form->nitems = 0;
	/* But of course this will fail fatally */
	for (i=0; i < form->nitems; i++)
		form->items[i] = item_clone(parent->items[i]);

	if (form->nqueries) {
		/* Again, probably OK if it fails */
		if (!(form->query = alloc(mainwindow, "clone queries", DQUERY, form->nqueries)))
			form->nqueries = 0;
		/* but this loop will fail fatally */
		for (i=0; i < form->nqueries; i++) {
			form->query[i].suspended = parent->query[i].suspended;
			form->query[i].name = mystrdup(parent->query[i].name);
			form->query[i].query= mystrdup(parent->query[i].query);
		}
	}
	form->fields = NULL;
	return(form);
}
#endif /* GROK */


/*
 * destroy a form struct and all its items. The pointer passed is not freed.
 * Make sure to remove all windows that display a view of this form first.
 * Initialize the form with defaults (this is used when reading form file).
 */

void form_delete(
	FORM		*form)		/* form to delete */
{
	DQUERY		*dq;		/* default query entry */
	int		i;

	if (!form)
		return;
	for (i=form->nitems - 1; i >= 0; --i)
		item_delete(form, i);

	zfree(form->items);
	zfree(form->path);
	zfree(form->name);
	zfree(form->dbase);
	zfree(form->comment);
	zfree(form->help);
	zfree(form->planquery);
	if (form->query) {
		for (i=0; i < form->nqueries; i++) {
			dq = &form->query[i];
			zfree(dq->name);
			zfree(dq->query);
		}
		free(form->query);
	}
	if (form->fields)
		delete form->fields;
	set_form_defaults(form);
}

/*
 * print an error report and return false if there are problems with the
 * form. If there is an error, set *bug to the item# of the first buggy
 * item; otherwise set to form->nitems. This is used to highlight the
 * first incorrect item.
 */

#ifdef GROK
#define ISMULTI(i) (i->type == IT_MENU  || i->type == IT_RADIO || \
		    i->type == IT_FLAGS || i->type == IT_MULTI)
#define ISFLAG(i) (i->type == IT_FLAG  || i->type == IT_CHOICE)

static void add_field_name(QString &msg, const FORM *form, int id)
{
	const ITEM *item = form->items[id % form->nitems];
	if(!IN_DBASE(item->type))
		msg += qsprintf("\"%s\" (#%d)", STR(item->label), id);
	else if(!item->multicol)
		msg += qsprintf("\"%s\" (#%d)",
			!BLANK(item->label) ? item->label : STR(item->name), id);
	else
		msg += qsprintf("\"%s\" (#%d)-\"%s\" (#%d)",
				STR(item->label), id % form->nitems,
				STR(item->menu[id / form->nitems].label),
				id / form->nitems);
}

static void verify_col(const FORM *form, const char *type, int **acol,
		       size_t *nacol, QString &msg, int col, int id)
{
	// GUI doesn't allow >999 anyway (but that was a guess as well)
	// and it's "personal" databases, so how could there be so many?
	if(col > 9999) {
		msg += "Field ";
		add_field_name(msg, form, id);
		msg += qsprintf(" uses a very large %s column (%d)\n",
				type, col);
		return;
	}
	*acol = talloc(0, "column verify", int, *acol, col + 1, nacol, AM_ZERO|AM_REALLOC);
	if((*acol)[col]) {
		int oid = (*acol)[col] - 1;
		const ITEM *it = form->items[id % form->nitems];
		const ITEM *oit = form->items[oid % form->nitems];
		if(it->type == IT_CHOICE && oit->type == IT_CHOICE &&
		   strcmp(it->name, oit->name)) {
			msg += "Field ";
			add_field_name(msg, form, id);
			msg += " (choice) has different internal name as ";
			add_field_name(msg, form, oid);
			msg += qsprintf(", but has same %s column\n", type);
			return;
		} else if(it->type != IT_CHOICE || oit->type != IT_CHOICE) {
			msg += "Field ";
			add_field_name(msg, form, id);
			msg += qsprintf(" uses same %s column as field ", type);
			add_field_name(msg, form, oid);
			msg += '\n';
			return;
		}
	} else
		(*acol)[col] = id + 1;
	return;
}

bool verify_form(
	FORM		*form,		/* form to verify */
	int		*bug,		/* retuirned buggy item # */
	QWidget		*shell)		/* error popup parent */
{
	int		nitem, ni;	/* item counter */
	ITEM		*item, *it;	/* item pointer */
	int		nq;		/* query pointer */
	DQUERY		*dq;		/* query pointer */
	QString		msg;		/* error messages */
	int		i0;		/* next free char in msg */
	int		sumwidth = 0;	/* total length of summary */

	if (bug)
		*bug = form->nitems;
	if (!form->name || !*form->name)
		msg += "Form has no name\n";
	if (!form->dbase || !*form->dbase) {
		msg += "Form has no database";
		if(form->name)
			msg += "; using form name";
		msg += '\n';
		form->dbase = mystrdup(form->name);
	}
	/* FIXME: disallow these in GUI as well */
	if (form->cdelim < 1 || form->cdelim == '\\' || form->cdelim == '\n') {
		msg += "Illegal field delimiter, using TAB\n";
		form->cdelim = '\t';
	}
	if ((form->asep == form->aesc && form->asep) ||
	    (!form->asep && form->aesc == '|') ||
	    (!form->aesc && form->asep == '\\')) {
		if (form->asep == '\\') {
			msg += "Array delimiter same as array escape;"
				   " using vertical bar\n";
			form->asep = '|';
		} else {
			msg += "Array escape same as array delimiter; "
				   "using backslash\n";
			form->aesc = '\\';
		}
	}
	for (nq=0; nq < form->nqueries; nq++) {
	    dq = &form->query[nq];
	    if (!dq->suspended) {
		if (!dq->name)
			msg += qsprintf("Query %d has no name\n", nq+1);
		if (!dq->query)
			msg += qsprintf("Query %d has no query\n", nq+1);
	    }
	}
	if(form->fields)
		delete form->fields;
	/* Rather than looping through items to find dups */
	/* fields go into symtab, and dcol/scol stores location of cols/scols */
	FIELDS *sym = form->fields = new FIELDS;
	int *dcol = 0, *scol = 0;
	size_t ndcol, nscol;
	for (nitem=0; nitem < form->nitems; nitem++) {
		i0 = msg.size();
		item = form->items[nitem];
		if (!IS_MULTI(item->type))
			item->multicol = false; // silent fix
		if(!item->multicol)
			sumwidth += item->sumwidth;
		if (IN_DBASE(item->type) && !item->multicol && BLANK(item->name)) {
			if(!BLANK(item->label)) {
				item->name = mystrdup(item->label);
				for(char *s = item->name; *s; s++) {
					if(isupper(*s))
						*s = tolower(*s);
					else if(!isalnum(*s))
						*s = '_';
				}
				if(isdigit(*item->name))
					*item->name += 'A' - '0';
			} else {
				char newname[40];
				sprintf(newname, "item%d", nitem);
				item->name = mystrdup(newname);
			}
			msg += "Field ";
			add_field_name(msg, form, nitem);
			msg += qsprintf(" has no internal name; using %s\n",
								item->name);
		}
		// FIXME: also check for other invalid characters
		if (IN_DBASE(item->type) && !item->multicol &&
		    isdigit(*item->name)) {
			*item->name += 'A' - '0';
			msg += "Field ";
			add_field_name(msg, form, nitem);
			msg += qsprintf(" has a leading digit on its internal name; using %s\n",
				     				item->name);
		}
		if (item->xs <= 0 || item->ys <= 0) {
			if (!item->xs) item->xs = 10;
			if (!item->ys) item->ys = 10;
			msg += "Field ";
			add_field_name(msg, form, nitem);
			msg += qsprintf(" has zero size; setting to %d %d\n",
						item->xs, item->ys);
		}
		if (!item->label && (item->type == IT_BUTTON ||
				     item->type == IT_LABEL)) {
			msg += "Field ";
			add_field_name(msg, form, nitem);
			QString name;
			add_field_name(name, form, nitem);
			item->label = qstrdup(name);
			msg += qsprintf(" has no label; using \"%s\"\n",
								item->label);
		}
		if (!item->flagcode && ISFLAG(item)) {
			item->flagcode = mystrdup(item->type == IT_CHOICE ? item->name : "1");
			msg += "Field ";
			add_field_name(msg, form, nitem);
			msg += qsprintf(" has no flag code; setting to %s\n",
					STR(item->flagcode));
		}
		if (!item->pressed && item->type == IT_BUTTON) {
			msg += "Field ";
			add_field_name(msg, form, nitem);
			msg += " has no button action\n";
		}
		if (item->multicol) {
			// FIXME: enforce this in GUI as well
			//  that way this will never be an error
			//  either that, or add these flags to the table (ugh)
			if(!item->nosort || item->defsort) {
				msg += "Field ";
				item->multicol = false; // don't give menu item name
				add_field_name(msg, form, nitem);
				item->multicol = true;
				msg += " does not support sorting; disabled\n";
			}
			item->nosort = true;
			item->defsort = false;
		}
		if (item->nmenu && IS_MENU(item->type)) {
			for(int m = 0; m < item->nmenu; m++) {
				MENU *menu = &item->menu[m];
				if(item->multicol)
					sumwidth += menu->sumwidth;
				if(!menu->label) {
					msg += "Field ";
					add_field_name(msg, form, nitem + m * form->nitems);
					msg += " has a blank label";
					if(item->type == IT_INPUT ||
					   (!menu->flagcode && !menu->flagtext &&
					    (!item->multicol ||
					     (!menu->name && !menu->column)))) {
						memmove(item->menu + m,
							item->menu + m + 1,
							(item->nmenu - m - 1) * sizeof(MENU));
						msg += "; deleting\n";
						item->nmenu--;
						--m;
						continue;
					} else if(item->multicol) {
						if(menu->name)
							menu->label = mystrdup(menu->name);
						else {
							char newname[40];
							sprintf(newname, "item%d.%d", nitem, m);
							menu->label = mystrdup(newname);
						}
						msg += qsprintf("; labeling it %s\n",
								menu->label);
					} else
						msg += '\n';
				}
				if(item->multicol && !menu->name) {
					char newname[40], *pref = item->label;
					if(BLANK(pref)) {
						sprintf(newname, "item%d", nitem);
						pref = newname;
					}
					menu->name = alloc(0, "menu name", char,
							   strlen(pref) + 1 + strlen(menu->label) + 1);
					sprintf(menu->name, "%s_%s", pref, menu->label);
					for(pref = menu->name; *pref; pref++) {
						if(isupper(*pref))
							*pref = tolower(*pref);
						else if(!isalnum(*pref))
							*pref = '_';
					}
					if(isdigit(*menu->name))
						*menu->name += 'A' - '0';
					msg += "Field ";
					add_field_name(msg, form, nitem + m * form->nitems);
					msg += qsprintf(" has no internal name;"
							" setting to %s\n",
							menu->name);
				}
				if(item->type != IT_INPUT && BLANK(menu->flagcode)) {
					menu->flagcode = mystrdup(item->multicol ? "1" : menu->label);
					msg += "Field ";
					add_field_name(msg, form, nitem + m * form->nitems);
					msg += qsprintf(" has blank code;"
							" setting to %s\n",
							menu->flagcode);
				}
			}
			// second loop for menu items now that label/code fixed
			for(int m = 0; m < item->nmenu; m++) {
				MENU *menu = &item->menu[m];
				for(int n = m + 1; n < item->nmenu; n++) {
					MENU *omenu = &item->menu[n];
					if(!strcmp(STR(menu->label),
						   STR(omenu->label))) {
					    msg += "Field ";
					    add_field_name(msg, form, nitem + n * form->nitems);
					    msg += qsprintf(" has duplicate label %s",
							    STR(menu->label));
					    if(item->type == IT_INPUT) {
						memmove(item->menu + n,
							item->menu + n + 1,
							(item->nmenu - n - 1) * sizeof(MENU));
						msg += "; deleting\n";
						item->nmenu--;
						--n;
						continue;
					    } else
						msg += '\n';
					}
					// multicol name/col dups are found below
					if(item->type != IT_INPUT && 
					   !item->multicol &&
					   !strcmp(STR(menu->flagcode),
						   STR(omenu->flagcode))) {
					    msg += "Field ";
					    add_field_name(msg, form, nitem + n * form->nitems);
					    msg += qsprintf(" has duplicate code %s\n",
							    STR(menu->flagcode));
					}
				}
			}
		}
		if(IN_DBASE(item->type)) {
			if(!item->multicol) {
				ITEM *oitem = NULL;
				auto it = sym->find(item->name);
				if(it != sym->end())
					oitem = form->items[it->second % form->nitems];
				if(oitem && (item->type != IT_CHOICE ||
					     oitem->type != IT_CHOICE)) {
					msg += "Field ";
					add_field_name(msg, form, nitem);
					msg += " has the same name as ";
					add_field_name(msg, form, it->second);
					msg += '\n';
				} else if (oitem) {
					// both are IT_CHOICE w/ same name
					const char *m  = 0;
					if      (item->column   != oitem->column)
						m = "dbase column";
					else if (item->sumcol   != oitem->sumcol)
						m = "summary column";
					else if (item->sumwidth != oitem->sumwidth)
						m = "summary width";
					// flag code dups checked below
					if (m) {
						msg += "Field ";
						add_field_name(msg, form, nitem);
						msg += qsprintf(
							" (choice) has same internal name as #%d, but has different %s\n",
							     it->second % form->nitems,
							     m);
					}
				} else
					(*sym)[item->name] = nitem;
				verify_col(form, "dbase", &dcol, &ndcol, msg,
					   item->column, nitem);
				if(item->sumwidth)
					verify_col(form, "summary", &scol, &nscol,
						   msg, item->sumcol, nitem);
			} else
				for(int m = 0; m < item->nmenu; m++) {
					auto it = sym->find(item->menu[m].name);
					if(it != sym->end()) {
						msg += "Field ";
						add_field_name(msg, form, nitem + m * form->nitems);
						msg += qsprintf(" has the same internal name (%s) as ",
								item->menu[m].name);
						add_field_name(msg, form, it->second);
					} else
						(*sym)[item->menu[m].name] = nitem + form->nitems * m;
					verify_col(form, "dbase", &dcol, &ndcol,
						   msg, item->menu[m].column,
						   nitem + m * form->nitems);
					if(item->menu[m].sumwidth)
						verify_col(form, "summary", &scol,
							   &nscol, msg,
							   item->menu[m].sumcol,
							   nitem + m * form->nitems);
				}
		}
		if (msg.size() > i0 && bug && *bug == form->nitems)
			*bug = nitem;
	}
	// uniqueness of choice codes needs to be checked in subloop still
	// This is done in a second main loop so that all codes have been set
	for (nitem=0; nitem < form->nitems; nitem++) {
		item = form->items[nitem];
		if (item->type == IT_CHOICE) {
			for (ni=nitem+1; ni < form->nitems; ni++) {
				it = form->items[ni];
				if (it->type == IT_CHOICE &&
				    !strcmp(item->name, it->name) &&
				    !strcmp(item->flagcode, it->flagcode)) {
					msg += "Field ";
					add_field_name(msg, form, ni);
					msg += qsprintf(
		       " has same internal name as #%d, but has same flag code\n",
							ni);
				}
			}
		}
	}
	zfree(dcol);
	zfree(scol);
	if (form->nitems && sumwidth == 0)
		msg += "Summary is empty:  all summary widths are 0\n";
	if (msg.size()) {
		char *s = qstrdup(msg);
		create_error_popup(shell, 0, s);
		free(s);
		return false;
	} else {
		for (nitem=0; nitem < form->nitems; nitem++) {
			item = form->items[nitem];
			if (item->type == IT_NOTE && item->maxlen <= 100)
				msg += qsprintf(
		"Warning: note field \"%s\" (#%d) has short max length %d",
					STR(item->name), nitem,
					item->maxlen);
		}
		if (msg.size()) {
			char *s = qstrdup(msg);
			create_error_popup(shell, 0, s);
			free(s);
		}
		return true;
	}
}


/*
 * the database is procedural, and the edit button was pressed. Start up an
 * editor window with fname, up to the first blank (this is useful for passing
 * cmd line options later).
 */

void form_edit_script(
	FORM		*form,		/* form to edit */
	QWidget		*shell,		/* error popup parent */
	const char	*fname)		/* file name of script (dbase name) */
{
	char		*path = zstrdup(fname), *q;

	if (!fname || !*fname) {
		create_error_popup(shell, 0,
"Please specify a database name first.\n"
"The database name will be used as script name.");
		return;
	}
	form->proc = true;
	fillout_formedit_widget_by_code(0x105);

	for (q=path; *q && *q != ' ' && *q != '\t'; q++);
	*q = 0;
	fname = resolve_tilde(path, "db");
	edit_file(fname, false, true, "Procedural Database", "procdbedit");
	free(path);
}


/*
 * sort the form's items by y, then x position (lower left corner). Later
 * traversal in order can then be done simply by going down the list.
 */

static int icompare(
	const void	*u,
	const void	*v)
{
	ITEM		*iu = *(ITEM **)u;
	ITEM		*iv = *(ITEM **)v;

	return(iu->y+iu->ys == iv->y+iv->ys ? iu->x+iu->xs - iv->x-iv->xs
					    : iu->y+iu->ys - iv->y-iv->ys);
}


void form_sort(
	FORM		*form)		/* form to sort */
{
	if (!form || !form->items || !form->nitems)
		return;
	qsort(form->items, form->nitems, sizeof(ITEM *), icompare);
}
#endif /* GROK */


/*------------------------------------------------- operations on form items */
/*
 * deselect all items. The caller must make sure to set curr_item to the
 * number of items in the current form (which means no selection). Here, only
 * the selected flags are cleared. This happens when Unselect is pressed, but
 * also if an item is added (we only want the added one selected afterwards).
 */

#ifdef GROK
void item_deselect(
	FORM		*form)		/* describes form and all items in it*/
{
	ITEM		*item;		/* new item struct */
	int		i;		/* index of item */

	for (i=0; i < form->nitems; i++) {
		item = form->items[i];
		if (item->selected) {
			item->selected = false;
			redraw_canvas_item(item);
		}
	}
}
#endif /* GROK */


/*
 * insert an item into the form, at the current field <nitem>. Fields are
 * stored in the form, which contains an array of pointers that point to
 * the field data structures. That way, we don't need to re-allocate the
 * form struct itself, and items can be easily re-arranged.
 * This assumes that fe_item_deselect has been called.
 * Provide reasonable defaults for the new field.
 */

bool item_create(
	FORM		*form,		/* describes form and all items in it*/
	int		nitem)		/* the current item, insert point */
{
	ITEM		*item;		/* new item struct */
	ITEM		*prev;		/* prev item, plundered for defaults */
	int		i, j, n, t;	/* various counters */
	char		buf[40];	/* temp for default strings */

							/* allocate array */
	/* at least one user assumes success, so just use grow()/fatal */
	/* besides, old code leaked items on failed realloc */
	/* this also uses the 32/double method rather than CHUNK */
	grow(0, "form field", ITEM *, form->items, form->nitems + 1, &form->size);

							/* allocate item */
	/* again, recovery is pointless */
	item = alloc(0, "form field", ITEM, 1);
	for (i=form->nitems-1; i >= nitem; i--)
		form->items[i+1] = form->items[i];
	form->items[nitem] = item;
	form->nitems++;
							/* defaults */
	if (nitem < form->nitems-1) {
		(void)memcpy(item, form->items[nitem+1], sizeof(ITEM));
		item->name	   = mystrdup(item->name);
		item->flagcode	   = mystrdup(item->flagcode);
		item->flagtext	   = mystrdup(item->flagtext);
		item->label	   = mystrdup(item->label);
		item->gray_if	   = mystrdup(item->gray_if);
		item->freeze_if	   = mystrdup(item->freeze_if);
		item->invisible_if = mystrdup(item->invisible_if);
		item->skip_if	   = mystrdup(item->skip_if);
		item->idefault	   = mystrdup(item->idefault);
		item->pressed	   = mystrdup(item->pressed);
	} else {
		memset(item, 0, sizeof(ITEM));
		item->type	   = IT_INPUT;
		item->selected	   = true;
		item->labeljust	   = J_LEFT;
		item->inputjust	   = J_LEFT;
		item->column       = 1;
		item->maxlen	   = 100;
		item->ch_xmax	   = 1;
		item->ch_ymax	   = 1;
		item->x 	   = XSNAP(8) + form->xg;
		item->xs	   = XSNAP(form->xs - 32);
		item->xm	   = XSNAP(item->xs / 4);
		item->y 	   = YSNAP(8) + form->xg;
		item->ys	   = YSNAP(24) + form->yg;
		item->ym	   = item->ys;
		if (nitem) {
			prev = form->items[nitem-1];
			for (i=0; i < form->nitems; i++)
				if (form->items[i]->type == item->type) {
					prev = form->items[i];
					item->xs = prev->xs;
					item->ys = prev->xs;
					item->xm = prev->ym;
					item->ym = prev->ym;
					break;
				}
			item->labelfont = prev->labelfont;
			item->inputfont = prev->inputfont;
		}
	}
							/* move to bottom */
	for (i=0; i < form->nitems; i++) {
		j = form->items[i]->y + form->items[i]->ys;
		j = YSNAP(j + 8);
		if (i != nitem && j > item->y)
			item->y = j;
	}
							/* find unused column*/
	item->column = 0;
	if (item->type == IT_CHOICE || item->type == IT_FLAG) {
		if (item->flagcode)
			free(item->flagcode);
		item->flagcode = 0;
		if (item->flagtext)
			free(item->flagtext);
		item->flagtext = 0;
	} else {
		if (IN_DBASE(item->type)) {
			n = form->nitems;
			for (i=0; i < n; i++)
				for (j=0; j < n; j++) {
					t = form->items[j]->type;
					if (IN_DBASE(t) &&
					    form->items[j] != item &&
					    form->items[j]->column ==
					   		 item->column) {
						item->column++;
						break;
					}
				}
		}
		sprintf(buf, "item%ld", item->column);
		item->name = mystrdup(buf);
	}
	return(true);
}


/*
 * delete a field from the form definition.
 */

void item_delete(
	FORM		*form,		/* describes form and all items in it*/
	int		nitem)		/* the current item, insert point */
{
	ITEM		*item = form->items[nitem];
	int		i;

	zfree(item->name);
	zfree(item->label);
	zfree(item->sumprint);
	zfree(item->flagcode);
	zfree(item->flagtext);
	zfree(item->gray_if);
	zfree(item->freeze_if);
	zfree(item->invisible_if);
	zfree(item->skip_if);
	zfree(item->idefault);
	zfree(item->pressed);
	zfree(item->ch_bar);

	for (i=0; i < item->nmenu; i++)
		menu_delete(&item->menu[i]);
	zfree(item->menu);

	for (i=0; i < item->ch_ncomp; i++) {
		item->ch_curr = i;
		del_chart_component(item);
	}
	zfree(item->ch_comp);
	free(item);

	for (i=nitem; i < form->nitems-1; i++)
		form->items[i] = form->items[i+1];
	form->nitems--;
}

void menu_delete(MENU *m)
{
	zfree(m->label);
	zfree(m->name);
	zfree(m->flagcode);
	zfree(m->flagtext);
}


/*
 * clone a field from the form definition. This is used when the form is
 * cloned when the form editor starts up with the form that is also being
 * displayed (never operate on the displayed form directly).
 */

ITEM *item_clone(
	ITEM		*parent)	/* item to clone */
{
	ITEM		*item;		/* target item */
	int		i;

	/* caller assumes success, so fail fatally */
	item = alloc(0, "clone form field", ITEM, 1);
	*item = *parent;
	item->name	   = mystrdup(parent->name);
	item->label	   = mystrdup(parent->label);
	item->sumprint	   = mystrdup(parent->sumprint);
	item->flagcode	   = mystrdup(parent->flagcode);
	item->flagtext	   = mystrdup(parent->flagtext);
	item->gray_if	   = mystrdup(parent->gray_if);
	item->freeze_if	   = mystrdup(parent->freeze_if);
	item->invisible_if = mystrdup(parent->invisible_if);
	item->skip_if	   = mystrdup(parent->skip_if);
	item->idefault	   = mystrdup(parent->idefault);
	item->pressed	   = mystrdup(parent->pressed);
	item->ch_bar	   = 0;
	item->ch_nbars	   = 0;

	if (item->nmenu) {
		item->menu = alloc(0, "clone form field", MENU, item->nmenu);
		for(i=0; i < item->nmenu; i++) {
			item->menu[i] = parent->menu[i];
			menu_clone(&item->menu[i]);
		}
	}
	if (item->ch_comp) {
		item->ch_comp = alloc(0, "clone form field", CHART, item->ch_ncomp);
		for (i=0; i < item->ch_ncomp; i++)
			clone_chart_component(&item->ch_comp[i],
					    &parent->ch_comp[i]);
	}
	return(item);
}

void menu_clone(MENU *m)
{
	m->label = zstrdup(m->label);
	m->flagcode = zstrdup(m->flagcode);
	m->flagtext = zstrdup(m->flagtext);
	m->name = zstrdup(m->name);
}

#endif /* GROK || PLANGROK */
