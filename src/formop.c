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
	set_form_defaults(form);
}


/*
 * print an error report and return FALSE if there are problems with the
 * form. If there is an error, set *bug to the item# of the first buggy
 * item; otherwise set to form->nitems. This is used to highlight the
 * first incorrect item.
 */

#ifdef GROK
#define ISFLAG(i) (i->type == IT_FLAG || i->type == IT_CHOICE)

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
	if (!form->name || !*form->name) {
		sprintf(msg+i, "Form has no name\n");
		i += strlen(msg+i);
	}
	if (!form->dbase || !*form->dbase) {
		sprintf(msg+i, "Form has no database%s\n",
					form->name ? "using form name" : "");
		i += strlen(msg+i);
		form->dbase = mystrdup(form->name);
	}
	if (form->cdelim < 1) {
		sprintf(msg+i, "Illegal field delimiter, using TAB\n");
		i += strlen(msg+i);
		form->cdelim = '\t';
	}
	for (nitem=0; nitem < form->nitems; nitem++) {
		i0 = i;
		item = form->items[nitem];
		sumwidth += item->sumwidth;
		sprintf(name, "Field \"%s\" (#%d)",
					item->name ? item->name : "", nitem);
		if (!item->name || !*item->name) {
			char newname[40];
			sprintf(newname, "item%d", nitem);
			sprintf(msg+i, "%s has no internal name, using %s\n",
								name, newname);
			i += strlen(msg+i);
			item->name = mystrdup(newname);
		}
		if (item->xs <= 0 || item->ys <= 0) {
			if (!item->xs) item->xs = 10;
			if (!item->ys) item->ys = 10;
			sprintf(msg+i, "%s has zero size, setting to %d %d\n",
						name, item->xs, item->ys);
			i += strlen(msg+i);
		}
		if (!item->label && (item->type == IT_BUTTON ||
				     item->type == IT_LABEL)) {
			sprintf(msg+i, "%s has no label, using \"%s\"\n",
								name, name);
			i += strlen(msg+i);
			item->label = mystrdup(name);
		}
		if (!item->flagcode && ISFLAG(item)) {
			sprintf(msg+i, "%s has no flag code\n", name);
			i += strlen(msg+i);
		}
		if (!item->pressed && item->type == IT_BUTTON) {
			sprintf(msg+i, "%s has no button action\n", name);
			i += strlen(msg+i);
		}
		for (nq=0; nq < form->nqueries; nq++) {
		    dq = &form->query[nq];
		    if (!dq->suspended) {
			if (!dq->name) {
				sprintf(msg+i, "Query %d has no name\n", nq+1);
				i += strlen(msg+i);
			}
			if (!dq->query) {
				sprintf(msg+i, "Query %d has no query\n",nq+1);
				i += strlen(msg+i);
			}
		    }
		}
		for (ni=nitem+1; ni < form->nitems; ni++) {
			it = form->items[ni];
			if (item->type == IT_CHOICE && it->type == IT_CHOICE) {
				BOOL eq  = !strcmp(item->name, it->name);
				BOOL sam = FALSE;
				const char *m  = 0;
				if      ((item->column   == it->column) != eq)
					m = "dbase column";
				else if ((item->sumcol   == it->sumcol) != eq)
					m = "summary column";
				else if ((item->sumwidth == it->sumwidth)!=eq)
					m = "summary width";
				else if ((item->flagcode == it->flagcode)&&eq){
					m = "flag code";
					sam = TRUE;
				}
				if (m && (eq || sam)) {
					sprintf(msg+i,
		       "(choice) has %s internal name as #%d, but has %s %s\n",
						eq  ? "same" : "different", ni,
						sam ? "same" : "different", m);
					i += strlen(msg+i);
				}
				continue;
			}
			if (item->name	&& it->name
					&& !strcmp(item->name, it->name)) {
				sprintf(msg+i,"%s has same name as field #%d\n"
								,name, ni);
				i += strlen(msg+i);
			}
			if (IN_DBASE(item->type) && IN_DBASE(it->type)
					&& item->column == it->column
					&& (!ISFLAG(item) || !ISFLAG(it))) {
				sprintf(msg+i,
			   "%s uses same dbase column as field \"%s\" (#%d)\n",
					name, it->name ? it->name : "", ni);
				i += strlen(msg+i);
			}
			if (IN_DBASE(item->type) && IN_DBASE(it->type)
					&& item->sumwidth>0 && it->sumwidth>0
					&& item->sumcol == it->sumcol) {
				sprintf(msg+i,
			 "%s uses same summary column as field \"%s\" (#%d)\n",
					name, it->name ? it->name : "", ni);
				i += strlen(msg+i);
			}
			if (i > (int)sizeof(msg)-1024) {
				sprintf(msg+i, "Too many errors, aborted.");
				break;
			}
		}
		if (i > (int)sizeof(msg)-1024) {
			sprintf(msg+i, "Too many errors, aborted.");
			break;
		}
		if (i > i0 && bug && *bug == form->nitems)
			*bug = nitem;
	}
	if (form->nitems && sumwidth == 0) {
		sprintf(msg+i, "Summary is empty, all summary widths are 0\n");
		i += strlen(msg+i);
	}
	if (i)
		create_error_popup(shell, 0, msg);
	else {
		for (i=nitem=0; nitem < form->nitems; nitem++) {
			item = form->items[nitem];
			if (item->type == IT_NOTE && item->maxlen <= 100) {
				sprintf(msg+i,
		"Warning: note field \"%s\" (#%d) has short max length %d",
					item->name ? item->name : "", nitem,
					item->maxlen);
				i += strlen(msg+i);
			}
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
"Please specify a database name first.\n\
The database name will be used as script name.");
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
		item->added	   = mystrdup(item->added);
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
	if (item->flagcode)	free((void *)item->flagcode);
	if (item->flagtext)	free((void *)item->flagtext);
	if (item->gray_if)	free((void *)item->gray_if);
	if (item->freeze_if)	free((void *)item->freeze_if);
	if (item->invisible_if)	free((void *)item->invisible_if);
	if (item->skip_if)	free((void *)item->skip_if);
	if (item->idefault)	free((void *)item->idefault);
	if (item->pressed)	free((void *)item->pressed);
	if (item->added)	free((void *)item->added);
	if (item->ch_bar)	free((void *)item->ch_bar);
	

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
	item->flagcode	   = mystrdup(parent->flagcode);
	item->flagtext	   = mystrdup(parent->flagtext);
	item->gray_if	   = mystrdup(parent->gray_if);
	item->freeze_if	   = mystrdup(parent->freeze_if);
	item->invisible_if = mystrdup(parent->invisible_if);
	item->skip_if	   = mystrdup(parent->skip_if);
	item->idefault	   = mystrdup(parent->idefault);
	item->pressed	   = mystrdup(parent->pressed);
	item->added	   = mystrdup(parent->added);
	item->ch_bar	   = 0;
	item->ch_nbars	   = 0;

	if (item->ch_comp) {
		item->ch_comp = (CHART*)malloc(item->ch_ncomp * sizeof(CHART));
		for (i=0; i < item->ch_ncomp; i++)
			clone_chart_component(&item->ch_comp[i],
					    &parent->ch_comp[i]);
	}
	return(item);
}

#endif /* GROK || PLANGROK */
