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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void done_callback   (void);
static void cancel_callback (void);
static void delete_callback (void);
static void clear_callback  (void);

static BOOL		have_shell = FALSE;	/* message popup exists if TRUE */
static char		**source;	/* ptr to string ptr of default */
static char		*sourcefile;	/* if nonzero, file to write back to */
static QDialog		*shell;		/* popup menu shell */
static QTextEdit	*text;		/* text widget */
static QLabel		*w_name;	/* filename text between buttons */


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
	const char	*name;		/* file name */

	if (!have_shell)
		return;
	if (sourcefile) {
		if (!(fp = fopen(name = sourcefile, "w"))) {
			int e = errno;
			fp = fopen(name = "/tmp/grokback", "w");
			create_error_popup(mainwindow, e, fp
				? "Failed to create file %s, wrote to %s"
				: "Failed to create file %s or backup file %s",
							sourcefile, name);
			if (!fp)
				return;
		}
		string = 0;
		read_text_button(text, &string);
		if (!string || !*string) {
			fclose(fp);
			unlink(name);

		} else if (fwrite(string, strlen(string), 1, fp) != 1) {
			create_error_popup(mainwindow, errno,
					"Failed to write to file %s", name);
			fclose(fp);
			free(string);
			return;
		} else
			fclose(fp);
		free(string);
		// FIXME: this shouldn't be done for templates
		if (chmod(name, 0700))
			create_error_popup(mainwindow, errno,
			 "Failed to chmod 700 %s\nThe file is not executable.",
								name);
		free(sourcefile);
		sourcefile = 0;

	} else if (source) {
		if (*source)
			free(*source);
		string = 0;
		read_text_button(text, &string);
		*source = !BLANK(string) ? string : 0;
		if(string && !*string)
			free(string);
		source = 0;
	}
	shell->hide();
	delete shell;
	have_shell = FALSE;
}


/*
 * create an editor popup as a separate application shell.
 */

void create_edit_popup(
	const char		*title,		/* menu title string */
	char			**initial,	/* initial default text */
	BOOL			readonly,	/* not modifiable if TRUE */
	const char		*helptag)	/* help tag */
{
	QBoxLayout		*form, *buttons = 0;
	QPushButton		*but=0;

	destroy_edit_popup();

	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new QDialog;
	shell->setWindowTitle(title);
	set_icon(shell, 1);
	form = new QVBoxLayout(shell);
	add_layout_qss(form, "editform");
	bind_help(shell, helptag);

							/*-- left buttons --*/
	buttons = new QHBoxLayout; // QDialogButtonBox is only buttons
	add_layout_qss(buttons, NULL);
	if (!readonly) {
		buttons->addWidget((but = mk_button(NULL, "Delete")));
		but->setFocusPolicy(Qt::ClickFocus);
		set_button_cb(but, delete_callback());
		bind_help(but, "msg_delete");
		buttons->addWidget((but = mk_button(NULL, "Clear")));
		but->setFocusPolicy(Qt::ClickFocus);
		set_button_cb(but, clear_callback());
		bind_help(but, "msg_clear");
	}
							/*-- filename --*/
	w_name = new QLabel;
	buttons->addStretch(1);
	buttons->addWidget(w_name);
	buttons->addStretch(1);
	
							/*-- right buttons --*/
	but = mk_button(NULL, readonly ? "Done" : "Save");
	buttons->addWidget(but);
	set_button_cb(but, done_callback());
	bind_help(but, "msg_done");
	if (!readonly) {
		but = mk_button(NULL, NULL, dbbb(Cancel));
		buttons->addWidget(but);
		set_button_cb(but, cancel_callback());
		bind_help(but, "msg_cancel");
	}
	but = mk_button(NULL, NULL, dbbb(Help));
	buttons->addWidget(but);
	set_button_cb(but, help_callback(shell, helptag));

							/*-- text --*/
	text = new QTextEdit;
	form->addWidget(text);
	text->setReadOnly(readonly);
	// QSS doesn't support :read-only for QTextEdit
	if (readonly)
		text->setProperty("readOnly", true);
	text->setProperty("courierFont", true);
	text->setLineWrapMode(QTextEdit::NoWrap);
	if (!BLANK(initial))
		print_text_button_s(text, *initial);
	text->setProperty("colSheet", true);
	text->ensurePolished();
	// size(0, "M").width() -> averageCharWidth()
	text->setMinimumWidth(text->fontMetrics().averageCharWidth() * 80);
	text->setMinimumHeight(text->fontMetrics().height() * 24);

	form->addLayout(buttons);

	popup_nonmodal(shell);

	set_dialog_cancel_cb(shell, done_callback());
	have_shell = TRUE;
	source = readonly ? 0 : initial;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

static void done_callback(void)
{
	destroy_edit_popup();
}


static void discard(void)
{
	if (sourcefile)
		free(sourcefile);
	else if (!BLANK(source))
		free(*source);
	sourcefile = 0;
	source = 0;
	destroy_edit_popup();
}

static void cancel_callback(void)
{
	create_query_popup(shell, discard, "msg_discard",
		"Press OK to confirm discarding\nall changes to the text");
}


static void delete_callback(void)
{
	print_text_button_s(text, "");
	destroy_edit_popup();
}


static void clear_callback(void)
{
	print_text_button_s(text, "");
}


/*-------------------------------------------------- file I/O ---------------*/
/*
 * read file and put it into a text window. This is used for print-to-window.
 * The help argument determines what will come up when Help in the editor
 * window is pressed; it must be one of the %% tags in grok.hlp.
 */

void edit_file(
	const char	*name,		/* file name to read */
	BOOL		readonly,	/* not modifiable if TRUE */
	BOOL		create,		/* create if nonexistent if TRUE */
	const char	*title,		/* if nonzero, window title */
	const char	*helptag)	/* help tag */
{
	FILE		*fp;		/* file to read */
	long		size;		/* file size */
	char		*text = NULL;	/* text read from file */
	BOOL		writable;	/* have write permission for file? */

	name = resolve_tilde((char *)name, 0); /* it's a file; better not have trailing / */
	if (!title)
		title = name;
	if (access(name, F_OK)) {
		const char *fn;
		if (!create || readonly) {
			create_error_popup(mainwindow, errno,
						"Cannot open %s", name);
			return;
		}
		writable = TRUE;
		create_edit_popup(title, 0, readonly, helptag);
		fn = strrchr(name, '/');
		print_button(w_name, "File %s (new file)",
			     fn ? fn + 1 : name);
	} else {
		create = FALSE;
		if (access(name, R_OK)) {
			create_error_popup(mainwindow, errno,
						"Cannot read %s", name);
			return;
		}
		writable = !access(name, W_OK);

		if (!(fp = fopen(name, "r"))) {
			create_error_popup(mainwindow, errno,
						"Cannot read %s", name);
			return;
		}
		fseek(fp, 0, 2);
		size = ftell(fp);
		rewind(fp);
		if (size) {
			if (!(text = (char *)malloc(size+1))) {
				create_error_popup(mainwindow, errno,
						"Cannot alloc %d bytes for %s",
						size+1, name);
				fclose(fp);
				return;
			}
			if (fread(text, size, 1, fp) != 1) {
				create_error_popup(mainwindow, errno,
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
		if (!readonly) {
			const char *fn = strrchr(name, '/');
			print_button(w_name, "File %s %s",
				fn ? fn+1 : name,
				writable ? "" : "(read only)");
		}
	}
	if (!readonly && writable)
		sourcefile = mystrdup(name);
}
