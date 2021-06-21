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
#include "grok.h"
#include "form.h"
#include "proto.h"

FORM		*form_list;	/* all loaded forms */

#define XSNAP(x)	((x)-(x)%form->xg)
#define YSNAP(y)	((y)-(y)%form->yg)

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

FORM *form_clone(
	FORM		*parent)	/* old form */
{
	FORM		*form;		/* new form */
	int		i;

	/* user assumes success, so this should be fatal */
	form = alloc(0, "clone form", FORM, 1);
	*form = *parent;
	form->path    = NULL;
	form->dir     = NULL;
	form->next    = NULL;
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


/*
 * destroy a form struct and all its items.
 * Make sure to remove all windows that display a view of this form first.
 */


void form_delete(
	FORM		*form)		/* form to delete */
{
	DQUERY		*dq;		/* default query entry */
	int		i;
	DBASE		*dbase;

	if (!form)
		return;
	for (dbase = dbase_list; dbase; dbase = dbase->next)
		if (dbase->form == form)
			break;
	if (dbase)
		return;
	for (FORM *f = form_list; f; f = f->next) {
		if (f == form)
			continue;
		for (i = 0; i < f->nitems; i++)
			if (f->items[i]->fkey_db == form)
				return;
	}
	/* a card can't have a form without a dbase, so no point in checking */
	for (i=form->nitems - 1; i >= 0; --i)
		item_delete(form, i);

	zfree(form->items);
	zfree(form->path);
	zfree(form->dir);
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
	for (FORM **prev = &form_list; *prev; prev = &(*prev)->next)
		if(*prev == form) {
			*prev = form->next;
			break;
		}
	if (form->fields)
		delete form->fields;
	free(form);
}

/*
 * print an error report and return false if there are problems with the
 * form. If there is an error, set *bug to the item# of the first buggy
 * item; otherwise set to form->nitems. This is used to highlight the
 * first incorrect item.
 */

#define ISMULTI(i) (i->type == IT_MENU  || i->type == IT_RADIO || \
		    i->type == IT_FLAGS || i->type == IT_MULTI)
#define ISFLAG(i) (i->type == IT_FLAG  || i->type == IT_CHOICE)

static void add_field_name(QString &msg, const FORM *form, int id)
{
	const ITEM *item = form->items[id % form->nitems];
	if(!IN_DBASE(item->type))
		msg += qsprintf("\"%s\" (#%d)", STR(item->label), id);
	else if(!IFL(item->,MULTICOL))
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
			msg += qsprintf(" (try %d)", avail_column(form, NULL));
			msg += '\n';
			return;
		}
	} else
		(*acol)[col] = id + 1;
	return;
}

static bool fkey_loop(const FORM *form, const ITEM *item,
		      const FORM *oform, const ITEM *oitem)
{
	if (item->type != IT_FKEY && item->type != IT_INV_FKEY)
		return false;
	if (!item->fkey_db)
		return false; /* actually, can't check */
	bool sform = !strcmp(item->fkey_db->name, oform->name);
	for (int n = 0; n < item->nfkey; n++) {
		/* FIXME: should probably check fkey_db & such instead */
		if (sform && oform->items[item->keys[n].item] == oitem)
			return true;
		if (fkey_loop(item->fkey_db, item->fkey_db->items[item->keys[n].item],
			      oform, oitem))
			return true;
		if (fkey_loop(item->fkey_db, item->fkey_db->items[item->keys[n].item],
			      form, item))
			return true;
	}
	return false;
}

bool verify_form(
	FORM		*form,		/* form to verify */
	int		*bug,		/* returned buggy item # */
	QWidget		*shell)		/* error popup parent */
{
	int		nitem, ni;	/* item counter */
	ITEM		*item, *it;	/* item pointer */
	int		nq;		/* query pointer */
	DQUERY		*dq;		/* query pointer */
	QString		msg;		/* error messages */
	int		i0;		/* next free char in msg */
	int		sumwidth = 0;	/* total length of summary */
	bool		ret;

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
			IFC(item->, MULTICOL); // silent fix
		if(!IFL(item->,MULTICOL))
			sumwidth += item->sumwidth;
		if (IN_DBASE(item->type) && !IFL(item->,MULTICOL) && BLANK(item->name)) {
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
		if (IN_DBASE(item->type) && !IFL(item->,MULTICOL) &&
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
		if ((item->type == IT_FKEY || item->type == IT_INV_FKEY) &&
		    item->fkey_db) { /* can't check if fkey_db not yet loaded */
			int nvis = 0, nkey = 0;
			for (int n = 0; n < item->nfkey; n++) {
				int itid = item->keys[n].item;
				if (itid < 0) {
					nvis = nkey = -99999;
					continue;
				}
				const ITEM *fitem = item->fkey_db->items[itid];
				if (IFL(fitem->,MULTICOL))
					itid += item->keys[n].menu * item->fkey_db->nitems;
				if (item->keys[n].key) {
					nkey++;
					if (item->type == IT_INV_FKEY &&
					    fitem->fkey_db &&
					    (fitem->type != IT_FKEY ||
					     strcmp(fitem->fkey_db->name,
						    form->name))) {
						/* FIXME:  enforce in editor */
						msg += "Field ";
						add_field_name(msg, form, nitem);
						msg += " key ";
						add_field_name(msg, item->fkey_db, itid);
						msg += " is not a valid key field";
					}
				}
				if (item->keys[n].display) {
					nvis++;
					if (item->type == IT_INV_FKEY &&
					    item->keys[n].key) {
						msg += "Field ";
						add_field_name(msg, form, nitem);
						msg += " displays key ";
						add_field_name(msg, item->fkey_db, itid);
						msg += "; setting invisible\n";
						nvis--;
						item->keys[n].display = false;
					}
				}
				if (fkey_loop(item->fkey_db, fitem, form, item)) {
					msg += "Field ";
					add_field_name(msg, form, nitem);
					msg += " has a reference loop.\n";
				}
			}
			if (!nvis && (item->type != IT_INV_FKEY || item->ys - item->ym > 4)) {
				msg += "Field ";
				add_field_name(msg, form, nitem);
				msg += " displays no data.\n";
			}
			/* FIXME:  enforce this in GUI as well */
			if (nvis && item->type == IT_INV_FKEY && item->ys - item->ym <= 4) {
				msg += "Field ";
				add_field_name(msg, form, nitem);
				msg += ": displayed data ignored.\n";
			}
			if (!nkey) {
				msg += "Field ";
				add_field_name(msg, form, nitem);
				msg += " has no matching key.\n";
			}
			/* FIXME:  enforce this in GUI as well */
			if (nkey > 1 && item->type == IT_INV_FKEY) {
				msg += "Field ";
				add_field_name(msg, form, nitem);
				msg += " must specify only one key.\n";
			}
		}
		if (IFL(item->,MULTICOL)) {
			// FIXME: enforce this in GUI as well
			//  that way this will never be an error
			//  either that, or add these flags to the table (ugh)
			if(!IFL(item->,NOSORT) || IFL(item->,DEFSORT)) {
				msg += "Field ";
				IFC(item->,MULTICOL); // don't give menu item name
				add_field_name(msg, form, nitem);
				IFS(item->,MULTICOL);
				msg += " does not support sorting; disabled\n";
			}
			IFS(item->,NOSORT);
			IFC(item->,DEFSORT);
		}
		if (item->nmenu && IS_MENU(item->type)) {
			for(int m = 0; m < item->nmenu; m++) {
				MENU *menu = &item->menu[m];
				if(IFL(item->,MULTICOL))
					sumwidth += menu->sumwidth;
				if(!menu->label) {
					msg += "Field ";
					add_field_name(msg, form, nitem + m * form->nitems);
					msg += " has a blank label";
					if(item->type == IT_INPUT ||
					   (!menu->flagcode && !menu->flagtext &&
					    (!IFL(item->,MULTICOL) ||
					     (!menu->name && !menu->column)))) {
						memmove(item->menu + m,
							item->menu + m + 1,
							(item->nmenu - m - 1) * sizeof(MENU));
						msg += "; deleting\n";
						item->nmenu--;
						--m;
						continue;
					} else if(IFL(item->,MULTICOL)) {
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
				if(IFL(item->,MULTICOL) && !menu->name) {
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
					menu->flagcode = mystrdup(IFL(item->,MULTICOL) ? "1" : menu->label);
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
					   !IFL(item->,MULTICOL) &&
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
			if(!IFL(item->,MULTICOL)) {
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
	ret = !msg.size();
	for (nitem=0; nitem < form->nitems; nitem++) {
		item = form->items[nitem];
		if (item->type == IT_NOTE && item->maxlen <= 100)
			msg += qsprintf(
	"Warning: note field \"%s\" (#%d) has short max length %d",
				STR(item->name), nitem,
				item->maxlen);
	}
	if (form->dbase)
		check_loaded_forms(msg, form);
	if(msg.size()) {
		char *s = qstrdup(msg);
		create_error_popup(shell, 0, s);
		free(s);
	}
	return ret;
}


bool check_loaded_forms(QString &msg, FORM *form)
{
	const char *dbpath;
	const char *formpath = form->path, *formdir = form->dir;
	char *tpath = 0, *tdir = 0;
	bool must_unload = false;

	/* form->dir has to be valid before db_path */
	if(!formpath) {
		const char *path = resolve_tilde(form->name, "gf");
		formdir = form->dir = tdir = mystrdup(canonicalize(form->path, true));
		if(!form->dir)
			fatal("Cant resolve form path");
		formpath = form->path = tpath = mystrdup(canonicalize(path, false));
	}
	dbpath = db_path(form);
	/* but don't fix form's path while still possibly editing */
	if(tpath)
		form->path = form->dir = NULL;
	for(DBASE *dbase = dbase_list; dbase; dbase = dbase->next) {
		if(!strcmp(dbase->path, dbpath) &&
		   form->proc == dbase->form->proc &&
		   (!form->proc || !strcmp(form->name, dbase->form->name))) {
			if(dbase->form->cdelim != form->cdelim) {
				msg += "Warning: this form's databse is "
				       "already loaded using a different "
				       "column delimiter.\n"
				       "You will be required to unload the "
				       "database before saving this form.\n\n";
				must_unload = true;
			}
			if(dbase->form->syncable != form->syncable) {
				msg += "Warning: this form's database is "
				       "already loaded with";
				if(form->syncable)
					msg += "out";
				msg += " timestamps.\n"
				       "They will be ";
				if(form->syncable)
					msg += "created";
				else
					msg += "deleted";
				msg += " the first time you save the databse.\n\n";
			}
			if(!strcmp(dbase->form->path, formpath) &&
			   !strcmp(dbase->form->dir, formdir)) {
				/* FIXME:  if any new fields are dates, and old
				 * field isn't the same date fmt, convert.
				 * But that's too much trouble; instead just
				 * purge existing dbase and let loader take
				 * care of it, since it has to do so, anyway */
				for(int i = 0; i < form->nitems; i++)
					if(form->items[i]->type == IT_TIME) {
						int j;
						for(j = 0; j < dbase->form->nitems; j++)
							if(dbase->form->items[j]->type == IT_TIME &&
							   dbase->form->items[j]->column == form->items[i]->column)
								break;
						if(j < dbase->form->nitems &&
						   dbase->form->items[j]->timefmt != form->items[i]->timefmt) {
							msg += "Warning:  a field's time format changed. "
							       "this will require a database reload.";
							must_unload = true;
						}
					}
			}
		} else if(!strcmp(dbase->form->path, formpath) &&
			  !strcmp(dbase->form->dir, formdir)) {
			if(mainwindow->card && dbase == mainwindow->card->dbase)
				card_readback_texts(mainwindow->card, -1);
			if(dbase->modified)
				msg += "Warning: this form's database changed "
				       "but unsaved changes remain in the old "
				       "one.\n"
				       "You will be required to save or "
				       "discard those changes before saving "
				       "this form.";
			else
				msg += "Warning: this form's databse changed; "
				       "all previously loaded databases using "
				       "this form will be unloaded.";
			must_unload = true;
		}
	}
	zfree(tpath);
	zfree(tdir);
	return must_unload;
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


/*------------------------------------------------- operations on form items */
/*
 * deselect all items. The caller must make sure to set curr_item to the
 * number of items in the current form (which means no selection). Here, only
 * the selected flags are cleared. This happens when Unselect is pressed, but
 * also if an item is added (we only want the added one selected afterwards).
 */

void item_deselect(
	FORM		*form)		/* describes form and all items in it*/
{
	ITEM		*item;		/* new item struct */
	int		i;		/* index of item */

	for (i=0; i < form->nitems; i++) {
		item = form->items[i];
		if (IFL(item->,SELECTED)) {
			IFC(item->,SELECTED);
			redraw_canvas_item(item);
		}
	}
}


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
	int		i, j;		/* various counters */
	char		buf[40];	/* temp for default strings */

							/* allocate array */
	/* at least one user assumes success, so just use grow()/fatal */
	/* besides, old code leaked items on failed realloc */
	/* this also uses the 32/double method rather than CHUNK */
	grow(0, "form field", ITEM *, form->items, form->nitems + 1, &form->size);

	for (i=form->nitems-1; i >= nitem; i--)
		form->items[i+1] = form->items[i];
	form->nitems++;
							/* defaults */
	if (nitem < form->nitems-1) {
		item = item_clone(form->items[nitem+1]);
		form->items[nitem] = item;
	} else {
		/* again, recovery is pointless */
		item = zalloc(0, "form field", ITEM, 1);
		form->items[nitem] = item;
		item->type	   = IT_INPUT;
		IFS(item->,SELECTED);
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
	if (item->type == IT_CHOICE || item->type == IT_FLAG) {
		if (item->flagcode)
			free(item->flagcode);
		item->flagcode = 0;
		if (item->flagtext)
			free(item->flagtext);
		item->flagtext = 0;
	} else {
		if (IN_DBASE(item->type) && !IFL(item->,MULTICOL))
			item->column = avail_column(form, item);
		else if(IFL(item->,MULTICOL)) {
			for(i = 0; i < item->nmenu; i++)
				item->menu[i].column = avail_column(form, i ? NULL : item);
		}
		zfree(item->name);
		sprintf(buf, "item%ld", item->column);
		item->name = mystrdup(buf);
	}
	return(true);
}


int avail_column(const FORM *form, const ITEM *item)
{
    int n = form->nitems, col = 0, t, i, j, k, m;
    for (i=m=0; i < n; i++) {
	if(IFL(form->items[i]->,MULTICOL)) {
	    if(m < form->items[i]->nmenu) {
		m++;
		i--;
	    } else {
		m = 0;
		continue;
	    }
	}
	for (j=0; j < n; j++) {
	    t = form->items[j]->type;
	    if (form->items[j] == item)
		continue;
	    if (IN_DBASE(t) && !IFL(form->items[j]->,MULTICOL) &&
		form->items[j]->column == col) {
		col++;
		break;
	    } else if (IFL(form->items[j]->,MULTICOL)) {
		for (k = 0; k < form->items[j]->nmenu; k++) {
		    if (form->items[j]->menu[k].column == col) {
			col++;
			j--;
			break;
		    }
		   }
	    }
	}
    }
    return col;
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

	while (item->ch_ncomp) {
		item->ch_curr = item->ch_ncomp - 1;
		del_chart_component(item);
	}
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
	if (item->nfkey) {
		item->keys = alloc(0, "clone form field", FKEY, item->nfkey);
		tmemcpy(FKEY, item->keys, parent->keys, item->nfkey);
		for(i=0; i < item->nmenu; i++) {
			item->menu[i] = parent->menu[i];
			menu_clone(&item->menu[i]);
		}
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
