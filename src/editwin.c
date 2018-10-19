/*
 * Create and destroy a simple text editing popup. It is installed when
 * a procedural database is to be edited.
 * The string entered or removed by the user are stored back into the
 * proper string pointers.
 *
 *	destroy_edit_popup()
 *	create_edit_popup(title, initial, readonly, help)
 *	edit_file(name, readonly, create, title, help)
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/LabelP.h>
#include <Xm/LabelG.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/Protocols.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void done_callback   (Widget, int, XmToggleButtonCallbackStruct *);
static void cancel_callback (Widget, int, XmToggleButtonCallbackStruct *);
static void delete_callback (Widget, int, XmToggleButtonCallbackStruct *);
static void clear_callback  (Widget, int, XmToggleButtonCallbackStruct *);

extern int		errno;
extern XmFontList	fontlist[NFONTS];
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern Display		*display;	/* everybody uses the same server */
extern Widget		toplevel;	/* top-level shell for icon name */
static BOOL		have_shell;	/* message popup exists if TRUE */
static char		**source;	/* ptr to string ptr of default */
static char		*sourcefile;	/* if nonzero, file to write back to */
static Widget		shell;		/* popup menu shell */
static Widget		text;		/* text widget */
Widget			w_name;		/* filename text between buttons */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * If <sourcefile> is nonzero, write the text to that file. Otherwise,
 * if <source> is nonzero, store a pointer to the text there. Otherwise,
 * discard the text. The first choice implements editing scripts for
 * procedural databases.
 */

void destroy_edit_popup(void)
{
	char		*string;	/* contents of text widget */
	FILE		*fp;		/* file to write */
	char		*name;		/* file name */

	if (!have_shell)
		return;
	if (sourcefile) {
		if (!(fp = fopen(name = sourcefile, "w"))) {
			int e = errno;
			fp = fopen(name = "/tmp/grokback", "w");
			create_error_popup(toplevel, e, fp
				? "Failed to create file %s, wrote to %s"
				: "Failed to create file %s or backup file %s",
							sourcefile, name);
			if (!fp)
				return;
		}
		string = XmTextGetString(text);
		if (!string || !*string) {
			fclose(fp);
			unlink(name);

		} else if (fwrite(string, strlen(string), 1, fp) != 1) {
			create_error_popup(toplevel, errno,
					"Failed to write to file %s", name);
			fclose(fp);
			XtFree(string);
			return;
		} else
			fclose(fp);
		XtFree(string);
		if (chmod(name, 0700))
			create_error_popup(toplevel, errno,
			 "Failed to chmod 700 %s\nThe file is not executable.",
								name);
		free(sourcefile);
		sourcefile = 0;

	} else if (source) {
		if (*source)
			free(*source);
		string = XmTextGetString(text);
		*source = *string ? mystrdup(string) : 0;
		XtFree(string);
		source = 0;
	}
	XtPopdown(shell);
	XtDestroyWidget(shell);
	have_shell = FALSE;
}


/*
 * create an editor popup as a separate application shell.
 */

void create_edit_popup(
	char			*title,		/* menu title string */
	char			**initial,	/* initial default text */
	BOOL			readonly,	/* not modifiable if TRUE */
	char			*helptag)	/* help tag */
{
	Widget			form, but=0, w, help;
	Arg			args[20];
	int			n;
	Atom			closewindow;

	destroy_edit_popup();

	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell(title, "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);
	form = XtCreateManagedWidget("editform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(form, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)helptag);

							/*-- buttons --*/
	if (!readonly) {
	  n = 0;
	  XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNleftOffset,	8);			n++;
	  XtSetArg(args[n], XmNwidth,		80);			n++;
	  XtSetArg(args[n], XmNtraversalOn,	False);			n++;
	  but = XtCreateManagedWidget("Delete", xmPushButtonWidgetClass,
			form, args, n);
	  XtAddCallback(but, XmNactivateCallback,
	  		(XtCallbackProc)delete_callback, (XtPointer)0);
	  XtAddCallback(but, XmNhelpCallback,
	  		(XtCallbackProc)help_callback,(XtPointer)"msg_delete");

	  n = 0;
	  XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	  XtSetArg(args[n], XmNleftWidget,	but);			n++;
	  XtSetArg(args[n], XmNleftOffset,	8);			n++;
	  XtSetArg(args[n], XmNwidth,		80);			n++;
	  XtSetArg(args[n], XmNtraversalOn,	False);			n++;
	  but = XtCreateManagedWidget("Clear", xmPushButtonWidgetClass,
			form, args, n);
	  XtAddCallback(but, XmNactivateCallback,
	  		(XtCallbackProc)clear_callback, (XtPointer)0);
	  XtAddCallback(but, XmNhelpCallback,
	  		(XtCallbackProc)help_callback, (XtPointer)"msg_clear");
	}
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = help = XtCreateManagedWidget("Help", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(help, XmNactivateCallback,
			(XtCallbackProc)help_callback, (XtPointer)helptag);
	if (!readonly) {
	  n = 0;
	  XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	  XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	  XtSetArg(args[n], XmNrightWidget,	w);			n++;
	  XtSetArg(args[n], XmNrightOffset,	8);			n++;
	  XtSetArg(args[n], XmNwidth,		80);			n++;
	  w = XtCreateManagedWidget("Cancel", xmPushButtonWidgetClass,
	 		 form, args, n);
	  XtAddCallback(w, XmNactivateCallback,
	  		(XtCallbackProc)cancel_callback, (XtPointer)0);
	  XtAddCallback(w, XmNhelpCallback,
	  		(XtCallbackProc)help_callback,(XtPointer)"msg_cancel");
	}
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		80);			n++;
	w = XtCreateManagedWidget(readonly ? "Done" : "Save",
			xmPushButtonWidgetClass, form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)done_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"msg_done");

							/*-- filename --*/
	n = 0;
	if (but) {
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	  XtSetArg(args[n], XmNleftWidget,	but);			n++;
	} else {
	  XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	}
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	w_name = XtCreateManagedWidget(" ", xmLabelWidgetClass,
			form, args, n);
							/*-- text --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		8);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	help);			n++;
	XtSetArg(args[n], XmNbottomOffset,	16);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	8);			n++;
	XtSetArg(args[n], XmNhighlightThickness,0);			n++;
	XtSetArg(args[n], XmNeditMode,		XmMULTI_LINE_EDIT);	n++;
	XtSetArg(args[n], XmNcolumns,		80);			n++;
	XtSetArg(args[n], XmNrows,		24);			n++;
	XtSetArg(args[n], XmNscrollVertical,	True);			n++;
	XtSetArg(args[n], XmNpendingDelete,	False);			n++;
	XtSetArg(args[n], XmNeditable,		!readonly);		n++;
	XtSetArg(args[n], XmNfontList,		fontlist[FONT_COURIER]);n++;
	text = XmCreateScrolledText(form, "text", args, n);
	if (initial && *initial) {
		XmTextSetString(text, *initial);
		XmTextSetInsertionPosition(text, strlen(*initial));
	}
	XtVaSetValues(text, XmNbackground, color[COL_SHEET], NULL);
	XtManageChild(text);

	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)done_callback, (XtPointer)shell);
	have_shell = TRUE;
	source = readonly ? 0 : initial;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

/*ARGSUSED*/
static void done_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	destroy_edit_popup();
}


static void discard(void)
{
	if (sourcefile)
		free(sourcefile);
	else if (source && *source)
		free(*source);
	sourcefile = 0;
	source = 0;
	destroy_edit_popup();
}

/*ARGSUSED*/
static void cancel_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	create_query_popup(widget, discard, "msg_discard",
		"Press OK to confirm discarding\nall changes to the text");
}


/*ARGSUSED*/
static void delete_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	XmTextSetString(text, "");
	destroy_edit_popup();
}


/*ARGSUSED*/
static void clear_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	XmTextSetString(text, "");
	XmTextSetInsertionPosition(text, 0);
}


/*-------------------------------------------------- file I/O ---------------*/
/*
 * read file and put it into a text window. This is used for print-to-window.
 * The help argument determines what will come up when Help in the editor
 * window is pressed; it must be one of the %% tags in grok.hlp.
 */

void edit_file(
	char		*name,		/* file name to read */
	BOOL		readonly,	/* not modifiable if TRUE */
	BOOL		create,		/* create if nonexistent if TRUE */
	char		*title,		/* if nonzero, window title */
	char		*helptag)	/* help tag */
{
	FILE		*fp;		/* file to read */
	long		size;		/* file size */
	char		*text = NULL;	/* text read from file */
	BOOL		writable;	/* have write permission for file? */

	name = resolve_tilde(name, 0);
	if (!title)
		title = name;
	if (access(name, F_OK)) {
		if (!create || readonly) {
			create_error_popup(toplevel, errno,
						"Cannot open %s", name);
			return;
		}
		writable = TRUE;
		create_edit_popup(title, 0, readonly, helptag);
		print_button(w_name, "File %s (new file)",
			(text = strrchr(name, '/')) ? text+1 : name);
	} else {
		create = FALSE;
		if (access(name, R_OK)) {
			create_error_popup(toplevel, errno,
						"Cannot read %s", name);
			return;
		}
		writable = !access(name, W_OK);

		if (!(fp = fopen(name, "r"))) {
			create_error_popup(toplevel, errno,
						"Cannot read %s", name);
			return;
		}
		fseek(fp, 0, 2);
		size = ftell(fp);
		rewind(fp);
		if (size) {
			if (!(text = malloc(size+1))) {
				create_error_popup(toplevel, errno,
						"Cannot alloc %d bytes for %s",
						size+1, name);
				fclose(fp);
				return;
			}
			if (fread(text, size, 1, fp) != 1) {
				create_error_popup(toplevel, errno,
						"Cannot read %s", name);
				fclose(fp);
				return;
			}
			text[size] = 0;
		}
		fclose(fp);
		create_edit_popup(title, text ? &text : 0,
				readonly || !writable, helptag);
		free(text);
		if (!readonly)
			print_button(w_name, "File %s %s",
				(text = strrchr(name, '/')) ? text+1 : name,
				writable ? "" : "(read only)");
	}
	if (!readonly && writable)
		sourcefile = mystrdup(name);
}
