/*
 * Create and destroy the add-section popup.
 *
 *	destroy_newsect_popup()
 *	create_newsect_popup()
 */

#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

static BOOL	have_shell = FALSE;	/* message popup exists if TRUE */
static QDialog	*shell;		/* popup menu shell */
static QWidget	*w_name;	/* name entry widget */


/*
 * destroy a popup. Remove it from the screen, and destroy its widgets.
 * It's too much trouble to keep them for next time.
 */

void destroy_newsect_popup(void)
{
	if (have_shell) {
		have_shell = FALSE;
		shell->close();
		delete shell;
	}
}


/*
 * create a new-section popup as a separate application shell.
 */

static void add_callback(void);
static void can_callback(void);

void create_newsect_popup(void)
{
	QVBoxLayout		*form;
	QWidget			*w, *wt;

	destroy_newsect_popup();
	if (!curr_card || !curr_card->dbase)
		return;

	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new QDialog();
	shell->setWindowTitle("New Section");
	set_icon(shell, 1);
	form = new QVBoxLayout(shell);
	add_layout_qss(form, "addSectForm");
	bind_help(shell, "addsect");

							/*-- section name --*/
	w = new QLabel("Enter a short name for the new section:");
	form->addWidget(w);

	w_name = new QLineEdit;
	form->addWidget(w_name);
	set_text_cb(w_name, add_callback());

	wt = new QLabel(!curr_card->dbase->havesects && curr_card->dbase->nrows
			? "All cards will be put into the new section."
			: "The new section will be empty.");
	form->addWidget(wt);

							/*-- buttons --*/
	QDialogButtonBox *hb = new QDialogButtonBox;
	form->addWidget(hb);

	// Note: I'm not adding help callbacks to individual buttons, even
	// though the Xm grok did, because they all bind to the same thing
	w = mk_button(hb, 0, dbbb(Help));
	set_button_cb(w, help_callback(shell, "addsect"));

	w = mk_button(hb, 0, dbbb(Cancel));
	set_button_cb(w, can_callback());

	w = mk_button(hb, "Add", dbbr(Accept));
	reinterpret_cast<QPushButton *>(w)->setDefault(true);
	set_button_cb(w, add_callback());

	popup_nonmodal(shell);
	set_dialog_cancel_cb(shell, can_callback());
	have_shell = TRUE;
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * All of these routines are direct X callbacks.
 */

static void add_callback(void)
{
	DBASE		*dbase;
	SECTION		*sect;
	char		*name, *p;
	int		i, s, fd;
	const char	*path;
	char		*oldp = NULL, *newp = NULL, *dir = NULL;
	BOOL		nofile = FALSE;

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
	oldp = alloc(0, "sect file name", char, strlen(path) + 5);
	sprintf(oldp, "%s.old", path);
	dir = alloc(0, "sect file name", char, strlen(path) + 4);
	sprintf(dir, "%s.db", path);
	newp = alloc(0, "sect file anme", char, strlen(path) + strlen(name) + 8);
	sprintf(newp, "%s.db/%s.db", path, name);
	if (!dbase->havesects) {
		(void)unlink(oldp);
		if (link(dir, oldp))
			if (!(nofile = errno == ENOENT)) {
				free(oldp);
				free(newp);
				free(dir);
				create_error_popup(shell, errno,
					"Could not link %s\nto %s", dir, oldp);
				return;
			}
		if (unlink(dir) && !nofile) {
			free(oldp);
			free(newp);
			free(dir);
			create_error_popup(shell, errno,
				"Could not unlink\n%s", dir);
			int UNUSED ret = link(oldp, dir);
			return;
		}
		if (mkdir(dir, 0700)) {
			free(oldp);
			free(newp);
			free(dir);
			create_error_popup(shell, errno,
				"Could not create directory\n%s", dir);
			return;
		}
		if (!nofile && link(oldp, newp)) {
			free(oldp);
			free(newp);
			free(dir);
			create_error_popup(shell, errno,
			       "Could not link %s\nto %s,\nleaving file in %s",
							oldp, newp, oldp);
			return;
		}
		(void)unlink(oldp);
	}
	free(dir);
	free(oldp);
	if (dbase->havesects || nofile) {
		if ((fd = creat(newp, 0600)) < 0) {
			free(newp);
			create_error_popup(shell, errno,
				"Could not create empty file\n%s", newp);
			return;
		}
		close(fd);
	}
	if (dbase->havesects) {
		/* there is no point in converting this to use abort_malloc() */
		/* Especially since this "recovers" */
		i = (dbase->nsects+1) * sizeof(SECTION);
		if (!(sect = (SECTION *)(dbase->sect ? realloc(dbase->sect,i):malloc(i)))) {
			free(newp);
			/* I don't see how this could succeed if the alloc failed */
			create_error_popup(mainwindow, errno,
						"No memory for new section");
			return;
		}
		dbase->sect = sect;
		azero(SECTION, dbase->sect, dbase->nsects, 1);
		sect = &dbase->sect[dbase->nsects];
		dbase->currsect = dbase->nsects++;
	} else {
		sect = dbase->sect;
		if (sect->path)
			free(sect->path);
	}
	sect->mtime	= time(0);
	sect->path	= newp;
	sect->modified	= TRUE;
	dbase->modified	= TRUE;
	dbase->havesects= TRUE;

	remake_section_pulldown();
	print_info_line();
	destroy_newsect_popup();
}

static void can_callback(void)
{
	destroy_newsect_popup();
}
