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

#define CHUNK		10		/* alloc 10 new item ptrs at a time */
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

	if (!(form = (FORM *)malloc(sizeof(FORM)))) {
		create_error_popup(mainwindow, errno, "Can't create form");
		return(0);
	}
	set_form_defaults(form);
	return(form);
}


static void set_form_defaults(
	FORM		*form)		/* for to initialize */
{
	memset((void *)form, 0, sizeof(FORM));
	form->cdelim	 = ':';
	form->xg	 = 4;
	form->yg	 = 4;
	form->xs	 = XSNAP(400);
	form->ys	 = YSNAP(200);
	form->autoquery  = -1;
	form->syncable	 = TRUE;
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

	if (!(form = (FORM *)malloc(sizeof(FORM)))) {
		create_error_popup(mainwindow, errno, "Can't clone form");
		return(0);
	}
	*form = *parent;
	form->path    = mystrdup(parent->path);
	form->name    = mystrdup(parent->name);
	form->dbase   = mystrdup(parent->dbase);
	form->comment = mystrdup(parent->comment);
	form->help    = mystrdup(parent->help);

	if (form->nitems &&
	    !(form->items = (ITEM **)malloc(form->size * sizeof(ITEM *)))) {
		create_error_popup(mainwindow, errno, "Can't clone field list");
		form->nitems = 0;
	}
	for (i=0; i < form->nitems; i++)
		form->items[i] = item_clone(parent->items[i]);

	if (form->query) {
		if (!(form->query = (DQUERY *)malloc(form->nqueries * sizeof(DQUERY)))) {
			create_error_popup(mainwindow, errno, "Queries lost");
			form->nqueries = 0;
		}
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

	if (form->items)	free((void *)form->items);
	if (form->path)		free((void *)form->path);
	if (form->name)		free((void *)form->name);
	if (form->dbase)	free((void *)form->dbase);
	if (form->comment)	free((void *)form->comment);
	if (form->help)		free((void *)form->help);
	if (form->planquery)	free((void *)form->planquery);
	if (form->query) {
		for (i=0; i < form->nqueries; i++) {
			dq = &form->query[i];
			if (dq->name)	free((void *)dq->name);
			if (dq->query)	free((void *)dq->query);
		}
		free((void *)form->query);
	}
	if (form->fields)
		delete form->fields;
	set_form_defaults(form);
}

/*
 * print an error report and return FALSE if there are problems with the
 * form. If there is an error, set *bug to the item# of the first buggy
 * item; otherwise set to form->nitems. This is used to highlight the
 * first incorrect item.
 */

#ifdef GROK
#define ISMULTI(i) (i->type == IT_MENU  || i->type == IT_RADIO || \
		    i->type == IT_FLAGS || i->type == IT_MULTI)
#define ISFLAG(i) (i->type == IT_FLAG  || i->type == IT_CHOICE)

static void verify_col(const FORM *form, const char *type, int **acol,
		       int *nacol, char *msg, int *i, const char *name,
		       int col, int id)
{
	// GUI doesn't allow >999 anyway (but that was a guess as well)
	// and it's "personal" databases, so how could there be so many?
	if(col > 9999) {
		*i += sprintf(msg+*i,
			"%s uses a very large %s column (%d)\n",
			name, type, col);
		return;
	}
	if(col >= *nacol) {
		int oacol = *nacol;
		while((*nacol *= 2) < col);
		*acol = (int *)realloc(*acol, *nacol * sizeof(*acol));
		memset(*acol + oacol, 0, (*nacol - oacol) * sizeof(**acol));
	}
	if((*acol)[col]) {
		const ITEM *it = form->items[id % form->nitems];
		const ITEM *oit = form->items[((*acol)[col] - 1) % form->nitems];
		if(it->type == IT_CHOICE && oit->type == IT_CHOICE &&
		   strcmp(it->name, oit->name))
			*i+=sprintf(msg+*i, "(choice) has different internal name as #%d, but has same %s\n",
				    (*acol)[col] - 1, type);
		else if(it->type != IT_CHOICE || oit->type != IT_CHOICE)
			*i+=sprintf(msg+*i, "%s uses same internal name as field \"%s\" (#%d)\n",
				    name, oit->name, (*acol)[col] - 1);
	} else
		(*acol)[col] = id + 1;
}

BOOL verify_form(
	FORM		*form,		/* form to verify */
	int		*bug,		/* retuirned buggy item # */
	QWidget		*shell)		/* error popup parent */
{
	int		nitem, ni;	/* item counter */
	ITEM		*item, *it;	/* item pointer */
	int		nq;		/* query pointer */
	DQUERY		*dq;		/* query pointer */
	char		name[256];	/* current item's name */
	char		msg[16384];	/* error messages */
	int		i0, i = 0;	/* next free byte in msg[] */
	int		sumwidth = 0;	/* total length of summary */

	if (bug)
		*bug = form->nitems;
	if (!form->name || !*form->name)
		i += sprintf(msg+i, "Form has no name\n");
	if (!form->dbase || !*form->dbase) {
		i += sprintf(msg+i, "Form has no database%s\n",
					form->name ? "using form name" : "");
		form->dbase = mystrdup(form->name);
	}
	if (form->cdelim < 1) {
		i += sprintf(msg+i, "Illegal field delimiter, using TAB\n");
		form->cdelim = '\t';
	}
	if ((form->asep == form->aesc && form->asep) ||
	    (!form->asep && form->aesc == '|') ||
	    (!form->aesc && form->asep == '\\')) {
		if (form->asep == '\\') {
			i += sprintf(msg + i, "Array delimiter same as array escape; using vertical bar\n");
			form->asep = '|';
		} else {
			i += sprintf(msg + i, "Array escape same as array delimiter; using backslash\n");
			form->aesc = '\\';
		}
	}
	for (nq=0; nq < form->nqueries; nq++) {
	    dq = &form->query[nq];
	    if (!dq->suspended) {
		if (!dq->name)
			i += sprintf(msg+i, "Query %d has no name\n", nq+1);
		if (!dq->query)
			i += sprintf(msg+i, "Query %d has no query\n",nq+1);
	    }
	}
	if(form->fields)
		delete form->fields;
	/* Rather than looping through items to find dups */
	/* fields go into symtab, and dcol/scol stores location of cols/scols */
	FIELDS *sym = form->fields = new FIELDS;
	int *dcol, ndcol = 10, *scol, nscol = 10;
	dcol = (int *)calloc(ndcol, sizeof(*dcol));
	scol = (int *)calloc(nscol, sizeof(*scol));
	for (nitem=0; nitem < form->nitems; nitem++) {
		i0 = i;
		item = form->items[nitem];
		if (!IS_MULTI(item->type))
			item->multicol = false; // silent fix
		if(!item->multicol)
			sumwidth += item->sumwidth;
		sprintf(name, "Field \"%s\" (#%d)",
					STR(item->name), nitem);
		if (IN_DBASE(item->type) && !item->multicol && BLANK(item->name)) {
			if(!BLANK(item->label)) {
				item->name = strdup(item->label);
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
			i += sprintf(msg+i, "%s has no internal name, using %s\n",
								name, item->name);
		}
		if (IN_DBASE(item->type) && !item->multicol &&
		    isdigit(*item->name)) {
			*item->name += 'A' - '0';
			i += sprintf(msg+i, "%s has a leading digit on its internal name, using %s\n",
				     				name, item->name);
		}
		if (item->xs <= 0 || item->ys <= 0) {
			if (!item->xs) item->xs = 10;
			if (!item->ys) item->ys = 10;
			i += sprintf(msg+i, "%s has zero size, setting to %d %d\n",
						name, item->xs, item->ys);
		}
		if (!item->label && (item->type == IT_BUTTON ||
				     item->type == IT_LABEL)) {
			i += sprintf(msg+i, "%s has no label, using \"%s\"\n",
								name, name);
			item->label = mystrdup(name);
		}
		if (!item->flagcode && ISFLAG(item)) {
			item->flagcode = mystrdup(item->type == IT_CHOICE ? item->name : "1");
			i += sprintf(msg+i, "%s has no flag code, setting to %s\n", name, STR(item->flagcode));
		}
		if (!item->pressed && item->type == IT_BUTTON)
			i += sprintf(msg+i, "%s has no button action\n", name);
		if (item->multicol) {
			// FIXME: enforce this in GUI as well
			//  that way this will never be an error
			//  either that, or add these flags to the table (ugh)
			if(!item->nosort || item->defsort)
				i += sprintf(msg+i, "%s does not support sorting; disabled\n", name);
			item->nosort = TRUE;
			item->defsort = FALSE;
		}
		if (item->nmenu && IS_MENU(item->type)) {
			for(int m = 0; m < item->nmenu; m++) {
				MENU *menu = &item->menu[m];
				if(item->multicol)
					sumwidth += menu->sumwidth;
				if(!menu->label) {
					i += sprintf(msg+i, "%s has a blank label", name);
					if(item->type == IT_INPUT ||
					   (!menu->flagcode && !menu->flagtext &&
					    (!item->multicol ||
					     (!menu->name && !menu->column)))) {
						memmove(item->menu + m,
							item->menu + m + 1,
							(item->nmenu - m - 1) * sizeof(MENU));
						i += sprintf(msg+i, ", deleting\n");
						item->nmenu--;
						--m;
						continue;
					} else if(item->multicol) {
						if(menu->name)
							menu->label = strdup(menu->name);
						else {
							char newname[40];
							sprintf(newname, "item%d.%d", nitem, m);
							menu->label = strdup(newname);
						}
						i += sprintf(msg+i, ", labeling it %s\n", menu->label);
					} else
						msg[i++] = '\n';
				}
				if(item->multicol && !menu->name) {
					char newname[40], *pref = item->label;
					if(BLANK(pref)) {
						sprintf(newname, "item%d", nitem);
						pref = newname;
					}
					menu->name = (char *)malloc(strlen(pref) + 1 + strlen(menu->label) + 1);
					sprintf(menu->name, "%s_%s", pref, menu->label);
					for(pref = menu->name; *pref; pref++) {
						if(isupper(*pref))
							*pref = tolower(*pref);
						else if(!isalnum(*pref))
							*pref = '_';
					}
					if(isdigit(*menu->name))
						*menu->name += 'A' - '0';
					i += sprintf(msg+i, "%s-%s has no internal name, setting to %s\n", name, menu->label, menu->name);
				}
				if(item->type != IT_INPUT && BLANK(menu->flagcode)) {
					menu->flagcode = strdup(item->multicol ? "1" : menu->label);
					i += sprintf(msg+i, "%s has blank code; setting to %s\n", name, menu->flagcode);
				}
			}
			// second loop for menu items now that label/code fixed
			for(int m = 0; m < item->nmenu; m++) {
				MENU *menu = &item->menu[m];
				for(int n = m + 1; n < item->nmenu; n++) {
					MENU *omenu = &item->menu[n];
					if(!strcmp(STR(menu->label),
						   STR(omenu->label))) {
					    i += sprintf(msg+i, "%s has duplicate label %s", name, STR(menu->label));
					    if(item->type == IT_INPUT) {
						memmove(item->menu + n,
							item->menu + n + 1,
							(item->nmenu - n - 1) * sizeof(MENU));
						i += sprintf(msg+i, ", deleting last one\n");
						item->nmenu--;
						--n;
						continue;
					    } else
						msg[i++] = '\n';
					}
					// multicol name/col dups are found below
					if(item->type != IT_INPUT && 
					   !item->multicol &&
					   !strcmp(STR(menu->flagcode),
						   STR(omenu->flagcode)))
					    i += sprintf(msg+i, "%s has duplicate code %s\n", name, STR(menu->flagcode));
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
					     oitem->type != IT_CHOICE))
					i += sprintf(msg+i, "%s has a the same name as \"%s\" (#%d)\n", name,
						     oitem->label, it->second % form->nitems);
				else if (oitem) {
					// both are IT_CHOICE w/ same name
					const char *m  = 0;
					if      (item->column   != oitem->column)
						m = "dbase column";
					else if (item->sumcol   != oitem->sumcol)
						m = "summary column";
					else if (item->sumwidth != oitem->sumwidth)
						m = "summary width";
					// flag code dups checked below
					if (m)
						i += sprintf(msg+i,
							"(choice) has same internal name as #%d, but has different %s\n",
							     it->second % form->nitems,
							     m);
				} else
					(*sym)[item->name] = nitem;
				verify_col(form, "dbase", &dcol, &ndcol, msg, &i, name, item->column, nitem);
				if(item->sumwidth)
					verify_col(form, "summary", &scol, &nscol, msg, &i, name, item->sumcol, nitem);
			} else
				for(int m = 0; m < item->nmenu; m++) {
					auto it = sym->find(item->menu[m].name);
					if(it != sym->end())
						i += sprintf(msg+i, "%s-%d has a duplicate internal name %s\n",
							     name, m, item->menu[m].name);
					else
						(*sym)[item->menu[m].name] = nitem + form->nitems * m;
					verify_col(form, "dbase", &dcol, &ndcol, msg, &i, name, item->menu[m].column, nitem + m * form->nitems);
					if(item->menu[m].sumwidth)
						verify_col(form, "summary", &scol, &nscol, msg, &i, name, item->menu[m].sumcol, nitem + m * form->nitems);
				}
		}
		if (i > (int)sizeof(msg)-1024) {
			sprintf(msg+i, "Too many errors, aborted.");
			break;
		}
		if (i > i0 && bug && *bug == form->nitems)
			*bug = nitem;
	}
	// uniqueness of choice codes needs to be checked in subloop still
	// This is done in a second main loop so that all codes have been set
	for (nitem=0; nitem < form->nitems; nitem++) {
		item = form->items[nitem];
		if (item->type == IT_CHOICE) {
			sprintf(name, "Field \"%s\" (#%d)", item->name, nitem);
			for (ni=nitem+1; ni < form->nitems; ni++) {
				it = form->items[ni];
				if (it->type == IT_CHOICE &&
				    !strcmp(item->name, it->name) &&
				    !strcmp(item->flagcode, it->flagcode)) {
					i += sprintf(msg+i,
		       "%s has same internal name as #%d, but has same flag code\n",
							name, ni);
				}
			}
			if (i > (int)sizeof(msg)-1024) {
				i += sprintf(msg+i, "Too many errors, aborted.");
				break;
			}
		}
	}
	free(dcol);
	free(scol);
	if (form->nitems && sumwidth == 0)
		i += sprintf(msg+i, "Summary is empty, all summary widths are 0\n");
	if (i)
		create_error_popup(shell, 0, msg);
	else {
		for (i=nitem=0; nitem < form->nitems; nitem++) {
			item = form->items[nitem];
			if (item->type == IT_NOTE && item->maxlen <= 100)
				i += sprintf(msg+i,
		"Warning: note field \"%s\" (#%d) has short max length %d",
					STR(item->name), nitem,
					item->maxlen);
		}
		if (i)
			create_error_popup(shell, 0, msg);
		i = 0;
	}
	return(!i);
}


/*
 * the database is procedural, and the edit button was pressed. Start up an
 * editor window with fname, up to the first blank (this is useful for passing
 * cmd line options later).
 */

void form_edit_script(
	FORM		*form,		/* form to edit */
	QWidget		*shell,		/* error popup parent */
	char		*fname)		/* file name of script (dbase name) */
{
	char		path[1024], *p, *q;

	if (!fname || !*fname) {
		create_error_popup(shell, 0,
"Please specify a database name first.\n"
"The database name will be used as script name.");
		return;
	}
	form->proc = TRUE;
	fillout_formedit_widget_by_code(0x105);

	for (p=fname, q=path; *p && *p != ' ' && *p != '\t'; p++, q++)
		*q = *p;
	*q = 0;
	fname = resolve_tilde(path, "db");
	edit_file(fname, FALSE, TRUE, "Procedural Database", "procdbedit");
}


/*
 * sort the form's items by y, then x position (lower left corner). Later
 * traversal in order can then be done simply by going down the list.
 */

static int icompare(
	const void	*u,
	const void	*v)
{
	register ITEM	*iu = *(ITEM **)u;
	register ITEM	*iv = *(ITEM **)v;

	return(iu->y+iu->ys == iv->y+iv->ys ? iu->x+iu->xs - iv->x-iv->xs
					    : iu->y+iu->ys - iv->y-iv->ys);
}


void form_sort(
	register FORM	*form)		/* form to sort */
{
	if (!form || !form->items || !form->nitems)
		return;
	qsort((void *)form->items, form->nitems, sizeof(ITEM *), icompare);
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
	register FORM	*form)		/* describes form and all items in it*/
{
	register ITEM	*item;		/* new item struct */
	int		i;		/* index of item */

	for (i=0; i < form->nitems; i++) {
		item = form->items[i];
		if (item->selected) {
			item->selected = FALSE;
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

BOOL item_create(
	register FORM	*form,		/* describes form and all items in it*/
	int		nitem)		/* the current item, insert point */
{
	register ITEM	*item;		/* new item struct */
	register ITEM	*prev;		/* prev item, plundered for defaults */
	register int	i, j, n, t;	/* various counters */
	char		buf[80];	/* temp for default strings */

							/* allocate array */
	if (!form->items || form->nitems >= form->size) {
		i = (form->size + CHUNK) * sizeof(ITEM *);
		if (!(form->items = (ITEM **)(form->items
					? realloc((void *)form->items, i)
					: malloc(i)))) {
			create_error_popup(mainwindow, errno,
					"Can't create field");
			return(FALSE);
		}
		form->size += CHUNK;
	}
							/* allocate item */
	if (!(item = (ITEM *)malloc(sizeof(ITEM)))) {
		create_error_popup(mainwindow, errno, "Can't create field");
		return(FALSE);
	}
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
		memset((void *)item, 0, sizeof(ITEM));
		item->type	   = IT_INPUT;
		item->selected	   = TRUE;
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
	return(TRUE);
}


/*
 * delete a field from the form definition.
 */

void item_delete(
	FORM		*form,		/* describes form and all items in it*/
	int		nitem)		/* the current item, insert point */
{
	register ITEM	*item = form->items[nitem];
	int		i;

	if (item->name)		free((void *)item->name);
	if (item->label)	free((void *)item->label);
	if (item->sumprint)	free((void *)item->sumprint);
	if (item->flagcode)	free((void *)item->flagcode);
	if (item->flagtext)	free((void *)item->flagtext);
	if (item->gray_if)	free((void *)item->gray_if);
	if (item->freeze_if)	free((void *)item->freeze_if);
	if (item->invisible_if)	free((void *)item->invisible_if);
	if (item->skip_if)	free((void *)item->skip_if);
	if (item->idefault)	free((void *)item->idefault);
	if (item->pressed)	free((void *)item->pressed);
	if (item->ch_bar)	free((void *)item->ch_bar);

	for (i=0; i < item->nmenu; i++)
		menu_delete(&item->menu[i]);
	if(item->menu)		free((void *)item->menu);

	for (i=0; i < item->ch_ncomp; i++) {
		item->ch_curr = i;
		del_chart_component(item);
	}
	if (item->ch_comp)
		free((void *)item->ch_comp);
	free((void *)item);

	for (i=nitem; i < form->nitems-1; i++)
		form->items[i] = form->items[i+1];
	form->nitems--;
}

void menu_delete(MENU *m)
{
	if(m->label)	free((void *)m->label);
	if(m->name)	free((void *)m->name);
	if(m->flagcode)	free((void *)m->flagcode);
	if(m->flagtext)	free((void *)m->flagtext);
}


/*
 * clone a field from the form definition. This is used when the form is
 * cloned when the form editor starts up with the form that is also being
 * displayed (never operate on the displayed form directly).
 */

ITEM *item_clone(
	register ITEM	*parent)	/* item to clone */
{
	register ITEM	*item;		/* target item */
	int		i;

	if (!(item = (ITEM *)malloc(sizeof(ITEM)))) {
		create_error_popup(mainwindow, errno, "Can't clone field");
		return(0);
	}
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
		item->menu = (MENU*)malloc(item->nmenu * sizeof(MENU));
		for(i=0; i < item->nmenu; i++) {
			item->menu[i] = parent->menu[i];
			menu_clone(&item->menu[i]);
		}
	}
	if (item->ch_comp) {
		item->ch_comp = (CHART*)malloc(item->ch_ncomp * sizeof(CHART));
		for (i=0; i < item->ch_ncomp; i++)
			clone_chart_component(&item->ch_comp[i],
					    &parent->ch_comp[i]);
	}
	return(item);
}

void menu_clone(MENU *m)
{
	if(m->label) m->label = strdup(m->label);
	if(m->flagcode) m->flagcode = strdup(m->flagcode);
	if(m->flagtext) m->flagtext = strdup(m->flagtext);
	if(m->name) m->name = strdup(m->name);
}

#endif /* GROK || PLANGROK */
