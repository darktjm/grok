/*
 * Create and destroy the add-section popup.
 *
 *	destroy_newsect_popup()
 *	create_newsect_popup()
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include <Xm/Protocols.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static BOOL	have_shell = FALSE;	/* message popup exists if TRUE */
static Widget	shell;		/* popup menu shell */
static Widget	w_name;		/* name entry widget */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_newsect_popup(void)
{
	if (have_shell) {
		XtPopdown(shell);
		XtDestroyWidget(shell);
		have_shell = FALSE;
	}
}


/*
 * create a new-section popup as a separate application shell.
 */

static void add_callback(Widget, int, XmToggleButtonCallbackStruct *);
static void can_callback(Widget, int, XmToggleButtonCallbackStruct *);

void create_newsect_popup(void)
{
	Widget			form, w, wt;
	Arg			args[20];
	int			n;
	Atom			closewindow;

	destroy_newsect_popup();
	if (!curr_card || !curr_card->dbase)
		return;

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("New Section", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);
	form = XtCreateManagedWidget("addsectform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"addsect");

							/*-- section name --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		12);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	w = XtCreateManagedWidget("Enter a short name for the new section:",
				xmLabelWidgetClass, form, args, n);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	16);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	w_name = XtCreateManagedWidget("sectname",
				xmTextWidgetClass, form, args, n);
	XtAddCallback(w_name, XmNactivateCallback,
				(XtCallbackProc)add_callback, NULL);

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_name);		n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	wt = XtCreateManagedWidget(
		!curr_card->dbase->havesects && curr_card->dbase->nrows
			? "All cards will be put into the new section."
			: "The new section will be empty.",
		xmLabelWidgetClass, form, args, n);

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		wt);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Add",
			xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)add_callback, (XtPointer)"addsect");
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"addsect");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		wt);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Cancel",
			xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)can_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"addsect");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		wt);			n++;
	XtSetArg(args[n], XmNtopOffset,		16);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget("Help",
			xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)help_callback, (XtPointer)"addsect");
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"addsect");

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)can_callback, (XtPointer)0);
	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void add_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	register DBASE			*dbase;
	register SECTION		*sect;
	register char			*name, *p;
	register int			i, s, fd;
	char				*path, old[1024], new[1024], dir[1024];
	BOOL				nofile = FALSE;

	if (!curr_card || !(dbase = curr_card->dbase)) {
		destroy_newsect_popup();
		return;
	}
	name = read_text_button(w_name, 0);
	while (*name == ' ' || *name == '\t')
		name++;
	for (p=name; *p; p++)
		if (strchr(" \t\n/!$&*()[]{};'\"`<>?\\|", *p))
			*p = '_';
	if ((p = strrchr(name, '.')) && !strcmp(p, ".db"))
		*p = 0;
	for (s=0; s < dbase->nsects; s++)
		if (!strcmp(name, section_name(dbase, s))) {
			create_error_popup(shell, 0,
				"A section with this name already exists.");
			return;
		}
	path = resolve_tilde(curr_card->form->dbase, 0);
	sprintf(old, "%s.old", path);
	sprintf(dir, "%s.db", path);
	sprintf(new, "%s.db/%s.db", path, name);
	if (!dbase->havesects) {
		(void)unlink(old);
		if (link(dir, old))
			if (!(nofile = errno == ENOENT)) {
				create_error_popup(shell, errno,
					"Could not link %s\nto %s", dir, old);
				return;
			}
		if (unlink(dir) && !nofile) {
			create_error_popup(shell, errno,
				"Could not unlink\n%s", dir);
			(void)link(old, dir);
			return;
		}
		if (mkdir(dir, 0700)) {
			create_error_popup(shell, errno,
				"Could not create directory\n%s", dir);
			return;
		}
		if (!nofile && link(old, new)) {
			create_error_popup(shell, errno,
			       "Could not link %s\nto %s,\nleaving file in %s",
							old, new, old);
			return;
		}
		(void)unlink(old);
	}
	if (dbase->havesects || nofile) {
		if ((fd = creat(new, 0600)) < 0) {
			create_error_popup(shell, errno,
				"Could not create empty file\n%s", new);
			return;
		}
		close(fd);
	}
	if (dbase->havesects) {
		i = (dbase->nsects+1) * sizeof(SECTION);
		if (!(sect = dbase->sect ? realloc(dbase->sect,i):malloc(i))) {
			create_error_popup(toplevel, errno,
						"No memory for new section");
			return;
		}
		dbase->sect = sect;
		mybzero(sect = &dbase->sect[dbase->nsects], sizeof(SECTION));
		dbase->currsect = dbase->nsects++;
	} else {
		sect = dbase->sect;
		if (sect->path)
			free(sect->path);
	}
	sect->mtime	= time(0);
	sect->path	= mystrdup(new);
	sect->modified	= TRUE;
	dbase->modified	= TRUE;
	dbase->havesects= TRUE;

	remake_section_pulldown();
	print_info_line();
	destroy_newsect_popup();
}

/*ARGSUSED*/
static void can_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	destroy_newsect_popup();
}
