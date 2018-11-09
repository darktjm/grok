/*
 * Various popups, such as the About... dialog.
 *
 *	create_about_popup()
 *					Create About info popup
 *	create_error_popup(widget, error, fmt, ...)
 *					Create error popup with Unix error
 */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "version.h"

#ifndef __DATE__
#define __DATE__ ""
#endif


/*---------------------------------------------------------- about ----------*/
/*
 * install the About popup. Nothing can be done with it except clicking it
 * away. This is called from the File pulldown in the main menu.
 */

static const char about_message[] =
	"\n"
	"Graphical Resource Organizer Kit\n"
	"Version " VERSION "\n"
	"Compiled " __DATE__ "\n\n"
	"Author: Thomas Driemeyer <thomas@bitrot.de>\n"
	"Qt port, fixes, and enhancements by Thomas J. Moore\n\n"
	"Homepage: http://www.bitrot.de/grok.html\n"
	"Better yet, https://bitbucket.org/darktjm/grok\n";

void create_about_popup(void)
{
	QMessageBox::about(mainwindow, "About", about_message);
}


/*---------------------------------------------------------- errors ---------*/
/*
 * install an error popup with a message. If a nonzero error is given,
 * insert the appropriate Unix error message. The user must click this
 * away. The widget is some window that the popup goes on top of.
 */

/*VARARGS*/
void create_error_popup(QWidget *widget, int error, const char *fmt, ...)
{
	va_list			parm;
	QMessageBox		*dialog;

	va_start(parm, fmt);
	QString msg(QString::vasprintf(fmt, parm));
	va_end(parm);
	msg.prepend("ERROR:\n\n");
	if (error) {
		msg.append('\n');
		msg.append(strerror(error));
	}
	if (!widget) {
		const QByteArray bytes(msg.toLocal8Bit());
		fprintf(stderr, "%s: %.*s\n", progname, (int)bytes.size(), bytes.data());
		return;
	}
	dialog = new QMessageBox(QMessageBox::Warning, "Grok Error", msg,
				 QMessageBox::Ok, widget);
	popup_nonmodal(dialog);
	// FIXME:  when does this ever get deleted?
}


/*---------------------------------------------------------- question -------*/
/*
 * put up a dialog with a message, and wait for the user to press either
 * the OK or the Cancel button. When the OK button is pressed, call the
 * callback. If Cancel is pressed, the callback is not called. The widget
 * is some window that the popup goes on top of. The help message is some
 * string that appears in grok.hlp after %%.
 */

/*VARARGS*/
void create_query_popup(
	QWidget		*widget,		/* window that caused this */
	void		(*callback)(void),	/* OK callback */
	const char	*help,			/* help text tag for popup */
	const char	*fmt, ...)		/* message */
{
	va_list		parm;
	QMessageBox::StandardButtons buttons = QMessageBox::Ok | QMessageBox::Cancel;

	va_start(parm, fmt);
	QString msg(QString::vasprintf(fmt, parm));
	va_end(parm);

	if(help)
		buttons |= QMessageBox::Help;

	// QMessageBox kills itself when any button is pressed, even if
	// a callback is added.  So, to support Help, I have to run it in
	// a loop.
	while(1) {
		switch(QMessageBox::question(widget, "Grok Dialog", msg,
					     buttons, QMessageBox::Cancel)) {
		    case QMessageBox::Ok:
			callback();
			return;
		    case QMessageBox::Help:
			// this will be immediately pushed to the back
			// when question() runs again.  <sigh>
			help_callback(widget, help);
			break;
		    default:
			return;
		}
	}
}


/*---------------------------------------------------------- dbase info -----*/
/*
 * install the Database info popup. It displays useful information about
 * the named card. This is called from the Help pulldown in the main menu.
 */

static const char info_message[] = "\n\
  Form name:  %s\n\
  Form path:  %s\n\
  Form comment:  %s\n\n\
  Default query:  %s\n\n\
  Database name:  %s%s%s\n\
  Database size:  %d cards,  %d columns\n\n\
  Help information:\n%s\n\n\
  Sections:\
";

void create_dbase_info_popup(
	CARD		*card)
{
	FORM		*form;
	DBASE		*dbase;
	QString		msg;
	char		date[20];
	struct tm	*tm;

	if (!card)
		return;
	form  = card->form;
	dbase = card->dbase;
	msg = QString::asprintf(info_message,
		form && form->name	? form->name	      : "(none)",
		form && form->path	? form->path	      : "(none)",
		form && form->comment	? form->comment	      : "(none)",
		form && form->autoquery >= 0
					? form->query[form->autoquery].name
					: "(none)",
		form  && form->dbase	? form->dbase	      : "(none)",
		dbase && dbase->rdonly ||
		form  && form->rdonly	? " (read-only)"      : "",
		form  && form->proc	? " (procedural)"     : "",
		dbase			? dbase->nrows	      : 0,
		dbase			? dbase->maxcolumns-1 : 0,
		form  && form->help	? form->help	      : "     (none)");
	if (!dbase || !dbase->nsects)
		msg.append(" (none)");
	else {
		register SECTION *sect = dbase->sect;
		int		 n;
		for (n=0; n < dbase->nsects; n++, sect++) {
			tm = localtime(&sect->mtime);
			if (pref.mmddyy)
				sprintf(date, "%2d/%02d/", tm->tm_mon+1,
							   tm->tm_mday);
			else
				sprintf(date, "%2d.%02d.", tm->tm_mday,
							   tm->tm_mon+1);
			sprintf(date+6, "%2d %2d:%02d",    tm->tm_year % 100,
							   tm->tm_hour,
							   tm->tm_min);
			msg.append(QString::asprintf(
				"\n      %s:  %d cards, %s, \"%s\" %s%s",
				section_name(dbase, n),
				sect->nrows == -1 ? 0 : sect->nrows,
				date,
				sect->path,
				sect->rdonly   ? " (read only)" : "",
				sect->modified ? " (modified)"  : ""));
		}
	}
	QMessageBox::information(mainwindow, "Database Info", msg,
				 QMessageBox::Ok);
}
