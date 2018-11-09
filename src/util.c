/*
 * Initializes everything and starts a calendar window for the current
 * month. The interval timer (for autosave and entry recycling) and a
 * few routines used by everyone are also here.
 *
 *	section_name(db,n)		return name of section nsect
 *	resolve_tilde(path)		return path with ~ replaced with home
 *					directory as found in $HOME
 *	find_file(buf, name,exec)	find file <name> and store the path
 *					in <buf>. Return FALSE if not found.
 *	fatal(char *fmt, ...)		Prints an error message and exits.
 *	print_button(w, fmt, ...)	Prints a string into a string Label
 *					or PushButton. %s may not be NULL.
 *	print_text_button(w, fmt, ...)	Prints a string into a Text button.
 *					%s may not be NULL.
 *	print_text_button_s(w, str)	Prints a string, str may be NULL
 *	set_icon(shell, sub)		changes the icon pixmap of a window
 *	truncate_string(str,len,sfont)	truncate string to fit into <len>
 *	strlen_in_pixels(str, sfont)	return length of string in pixels
 *
 *	to_octal(n)			convert ascii to octal string
 *	to_ascii(str, def)		convert octal string to ascii
 */

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <QtWidgets>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include "layout-qss.h"
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "version.h"
#include "bm_icon.h"



/*
 * extract a section name from the section paths by removing the extension
 * and common substrings.
 */

const char *section_name(
	register DBASE	 *dbase,		/* contains section array */
	int		 n)			/* 0 .. dbase->nsects-1 */
{
	static char	 name[40];		/* returned name */
	register SECTION *sect;			/* section list */
	register char	 *path;			/* path to reduce */
	register char	 *trunc;		/* trailing unique part */
	register int	 i;			/* section counter */

	if (!dbase || n < 0 || n >= dbase->nsects)
		return("");
	sect = dbase->sect;
	path = sect[n].path;
	if (!path || !*path)
		return("");
	trunc = strrchr(sect[n].path, '/');
	while (trunc > path) {
		for (i=dbase->nsects-1; i >= 0; i--)
			if (i != n && strncmp(path, sect[i].path, trunc-path))
				break;
		if (i < 0)
			break;
		while (--trunc > path && *trunc != '/');
	}
	if (!trunc)
		trunc = path;
	else if (*trunc == '/')
		trunc++;
	i = strlen(trunc);
	if (i > (int)sizeof(name)-1)
		sprintf(name, "...%s", trunc + i - (sizeof(name)-4));
	else
		strcpy(name, trunc);
	if ((trunc = strrchr(name, '.')) && trunc > strrchr(name , '/'))
		*trunc = 0;
	return(name);
}


/*
 * If <path> begins with a tilde, replace the tilde with $HOME. This is used
 * for the database files, and the holiday file (see holiday.c). If the path
 * is relative (ie., does not begin with ~ or !) and does not begin with "./",
 * prepend GROKDIR and append ext. This is the default, all files are normally
 * in ~/.grok. Also strip unnecessary leading and trailing / and /. to prevent
 * forms from appearing multiple times in the database pulldown.
 */

char *resolve_tilde(
	char		*path,			/* path with ~ */
	const char	*ext)			/* append extension unless 0 */
{
	struct passwd	*pw;			/* for searching home dirs */
	static char	pathbuf[1024];		/* path with ~ expanded */
	char		buf[1024];		/* buf for prepending GROKDIR*/
	char		*p, *q;			/* username copy pointers */
	const char	*home = 0;		/* home dir (if ~ in path) */
	int		i;			/* strip trailing / and /. */

	if (*path != '~' && *path != '/' && (*path != '.' || path[1] != '/')) {
		sprintf(buf, "%s/%s", GROKDIR, path);
		path = buf;
		if (ext) {
			if ((p = strrchr(path, '.')) && !strcmp(p+1, ext))
				*p = 0;
			strcat(path, ".");
			strcat(path, ext);
		}
	}
	if (*path == '~') {
		if (!path[1] || path[1] == '/') {
			*pathbuf = 0;
			if (!(home = getenv("HOME")))
				home = getenv("home");
			path += 2;
		} else {
			for (path++,q=pathbuf; *path && *path!='/'; path++,q++)
				*q = *path;
			path++;
			*q = 0;
			if (pw = getpwnam(pathbuf))
				home = pw->pw_dir;
		}
		if (!home) {
			fprintf(stderr,
				"%s: can't evaluate ~%s in %s, using .\n",
						progname, pathbuf, path);
			home = ".";
		}
		sprintf(pathbuf, "%s/%s", home, path);
		path = pathbuf;
	}
	for (i=strlen(path)-1; i > 0; )
		if (path[i] == '/' || path[i] == '.' && path[i-1] == '/')
			path[i--] = 0;
		else
			break;
	return(path);
}


/*
 * locate a program or file, and return its complete path. This is used by
 * the daemon to locate notifier and user programs, and by grok to locate
 * grok.hlp. Assume that <buf> has space for 1024 chars. PATH is a macro
 * defined by the Makefile.
 */

#ifndef PATH
#define PATH 0
#endif
#define DEFAULTPATH "/usr/local/bin:/usr/local/lib:/bin:/usr/bin:/usr/sbin:/usr/ucb:/usr/bsd:/usr/bin/X11:."

BOOL find_file(
	char			*buf,		/* buffer for returned path */
	const char		*name,		/* file name to locate */
	BOOL			exec)		/* must be executable? */
{
	int			method;		/* search path counter */
	const char		*path;		/* $PATH or DEFAULTPATH */
	int			namelen;	/* len of tail of name */
	register const char	*p;		/* string copy pointers */
	register char		*q;		/* string copy pointers */

	if (*name == '/') {				/* begins with / */
		strcpy(buf, name);
		return(TRUE);
	}
	if (*name == '~') { 				/* begins with ~ */
		strcpy(buf, resolve_tilde((char *)name, 0)); /* file; better not have trailing / */
		return(TRUE);
	}
	namelen = strlen(name);
	for (method=0; ; method++) {	
		switch(method) {
		  case 0:   path = PATH;		break;
		  case 1:   path = getenv("GROK_PATH");	break;
		  case 2:   path = getenv("PATH");	break;
		  case 3:   path = DEFAULTPATH;		break;
		  default:  return(FALSE);
		}
		if (!path)
			continue;
		do {
			q = buf;
			p = path;
			while (*p && *p != ':' && q < buf + 1021 - namelen)
				*q++ = *p++;
			*q++ = '/';
			strcpy(q, name);
			if (!access(buf, exec ? X_OK : R_OK))
				return(TRUE);
			*buf = 0;
			path = p+1;
		} while (*p);
	}
	return(FALSE); /* for lint */
}


/*
 * whenever something goes seriously wrong, this routine is called. It makes
 * code easier to read. fatal() never returns.
 */

/*VARARGS*/
void fatal(const char *fmt, ...)
{
	va_list			parm;

	va_start(parm, fmt);
	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, parm);
	va_end(parm);
	putc('\n', stderr);
	exit(1);
}


/*
 * Ultrix doesn't have strdup, so we'll need to define one locally.
 */

char *mystrdup(
	const char *s)
{
	register char *p = NULL;

	if (s && (p = (char *)malloc(strlen(s)+1)))
		strcpy(p, s);
	return(p);
}


/*---------------------------------------------------------------------------*/
/*
 * draw some text into a button. This is here because it's used by many
 * routines.
 */

static void set_w_text(QWidget *w, QString &string)
{
	QPushButton		*b;
	QLabel			*l;
	QLineEdit		*le;
	QTextEdit		*te;

	if((b = dynamic_cast<QPushButton *>(w)))
		b->setText(string);
	else if((l = dynamic_cast<QLabel *>(w)))
		l->setText(string);
	else if((le = dynamic_cast<QLineEdit *>(w))) {
		le->setText(string);
		le->setCursorPosition(string.size());
	} else if((te = dynamic_cast<QTextEdit *>(w))) {
		te->setPlainText(string);
		te->textCursor().setPosition(string.size());
	}
}

void print_button(QWidget *w, const char *fmt, ...)
{
	va_list			parm;

	if (w) {
		va_start(parm, fmt);
		if (!fmt) fmt = "";
		QString string(QString::vasprintf(fmt, parm));
		va_end(parm);
		set_w_text(w, string);
	}
}

void print_text_button_s(QWidget *w, const char *str)
{
	if (!w)
		return;
	if (!str)
		str = "";
	QString string(str);
	set_w_text(w, string);
}


/*
 * turn radio button on or off
 */

void set_toggle(
	QWidget			*w,
	BOOL			set)
{
	if (w)
		dynamic_cast<QAbstractButton *>(w)->setChecked(set);
}


/*
 * return the text string in a text button. If ptr==0, return a static
 * buffer containing the text; else assume that ptr points to a strdup'ed
 * pointer. Free it if nonzero, then strdup the new text.
 */

static char *readbutton(
	QWidget			*w,
	char			**ptr,
	BOOL			skipblank,
	BOOL			noblanks)
{
	static char		*buf = NULL;
	static int		bufsize = 0;
	int			size;
	QString			string;

	if (w) {
		QLineEdit		*le;
		int			s = 0, e, i;
		if((le = dynamic_cast<QLineEdit *>(w)))
			string = le->text();
		else
			string = dynamic_cast<QTextEdit *>(w)->toPlainText();
		e = string.size();
		if (skipblank) {
			while(s < e && string[s] == ' ' || string[s] == '\t')
				s++;
			if (noblanks)
				for (i = 0; i < e; i++)
					if(string[i] == ' ' || string[i] == '\t')
						string[i] = '_';
			/* note: if(noblanks), this will do nothing */
			/* that is probalby a bug - tjm */
			while(e > s && (string[e - 1] == ' ' || string[e - 1] == '\t'))
				e--;
		}
		string.chop(string.size() - e);
		string.remove(0, s);
		QByteArray bytes(string.toLocal8Bit());
		size = bytes.size() + 1;

		if (size > bufsize) {
			if (buf) free(buf);
			if (!(buf = (char *)malloc(bufsize = size)))
				return(0);
		}
		memcpy(buf, bytes.data(), size - 1);
		buf[size - 1] = 0;
	} else if (buf)
		*buf = 0;
	if (ptr) {
		if (*ptr) free(*ptr);
		*ptr = buf && *buf ? mystrdup(buf) : 0;
	}
	return(buf);
}

char *read_text_button_noskipblank(QWidget *w, char **ptr)
	{ return(readbutton(w, ptr, FALSE, FALSE)); }

char *read_text_button(QWidget *w, char **ptr)
	{ return(readbutton(w, ptr, TRUE, FALSE)); }

char *read_text_button_noblanks(QWidget *w, char **ptr)
	{ return(readbutton(w, ptr, TRUE, TRUE)); }


/*
 * set icon for the application or a shell.
 */

/* FIXME: change this to use Grok.xpm instead */
void set_icon(
	QWidget			*shell,
	int			sub)		/* 0=main, 1=submenu */
{
	static QIcon icon;
	if(icon.isNull())
		icon.addPixmap(QPixmap::fromImage(QImage(bm_icon_bits, bm_icon_width, bm_icon_height, QImage::Format_Mono)));
	/* there is only 1 icon, so sub is actually ignored */
	shell->setWindowIcon(icon);
}


/*
 * set mouse cursor to one of the predefined shapes. This is used in the
 * form editor's canvas drawing area. The list of supported cursors is in
 */

void set_cursor(
	QWidget			*w,		/* in which widget */
	Qt::CursorShape		n)		/* which cursor, one of XC_* */
{
	w->setCursor(QCursor(n));
}


/*
 * truncate <string> such that it is not longer than <len> pixels when
 * drawn with font <sfont>, by storing \0 somewhere in the string.
 * This used to take a font ID, but style sheets could change the font
 * of any widget, so best to pass in the widget.
 */

void truncate_string(
	QWidget *w,			/* widget string will show in */
	register char	*string,	/* string to truncate */
	register int	len)		/* max len in pixels */
{
	const QFontMetrics &fs(w->fontMetrics());
	QString str(string);
	int slen = str.size();
	while(str.size() > 0) {
		if(fs.boundingRect(str).width() <= len)
			break;
		str.chop(1);
	}
	if(slen == str.size())
		return;
	string[str.toLocal8Bit().size()] = 0;
}


/*
 * return the length of <string> in pixels, when drawn in <w>.
 * This used to take a font ID, but style sheets could change the font
 * of any widget, so best to pass in the widget.
 */

int strlen_in_pixels(
	QWidget		*w,		/* widget string will show in */
	const char	*string)	/* string to truncate */
{
	const QFontMetrics &fs(w->fontMetrics());
	return fs.boundingRect(string).width();
}


/*
 * convert ascii code to and from a string representation. This is used for
 * a few input buttons in the form editor window that accept single chars.
 */

const char *to_octal(
	int		n)		/* ascii to convert to string */
{
	static char	buf[8];

	if (n == '\t')			return("\\t");
	if (n == '\n')			return("\\n");
	if (n <= 0x20 || n >= 0x7f)	{ sprintf(buf, "\\%03o", n); }
	else				{ buf[0] = n; buf[1] = 0;    }
	return(buf);
}

char to_ascii(
	char		*str,		/* string to convert to ascii */
	int		def)		/* default if string is empty */
{
	int		n;
	char		*p = str;

	if (!p)
		return(def);
	while (*p == ' ' || *p == '\t') p++;
	if (!*p)
		return(def);
	if (*p == '\\') {
		if (p[1] == 't')	return('\t');
		if (p[1] == 'n')	return('\n');
		if (p[1] >= '0' &&
		    p[1] <= '7')	{ sscanf(p+1, "%o", &n); return(n); }
		if (p[1] == 'x')	{ sscanf(p+2, "%x", &n); return(n); }
		return('\\');
	} else
		return(*p);
}

QWidget *mk_separator(void)
{
	QFrame *f = new QFrame;
	f->setFrameStyle(QFrame::HLine | QFrame::Sunken);
	f->setObjectName("sep"); // for style sheets
	return f;
}

QPushButton *mk_button(
	QDialogButtonBox *bb,		/* target button box; may be 0 */
	const char	*label,		/* label, or 0 if standard */
	int		role)		/* role, or ID if label is NULL */
{
	QPushButton *w;
	if(bb) {
		if(label)
			w = bb->addButton(label, (dbbb(ButtonRole))role);
		else
			w = bb->addButton((dbbb(StandardButton))role);
	} else {
		if(label)
			w = new QPushButton(label);
		else
			// Qt provides no public way of obtaining standard button labels
			w = new QPushButton(QGuiApplicationPrivate::platformTheme()->standardButtonText(role));
	}
	w->setMinimumWidth(80);
#if 0
	/* As mentioned in QT-README.md, I won't be matching Motif's */
	/* usual convention of naming button widgets after their label */
	w->setObjectName(w->text());
#endif
	return w;
}

void add_layout_qss(QLayout *l, const char *name)
{
	if(name)
		l->setObjectName(name);
	l->addWidget(new LayoutQSS(l));
}

void popup_nonmodal(QDialog *d)
{
	d->setModal(false);
	d->show();
	d->raise();
	d->activateWindow();
}

char *qstrdup(const QString &str)
{
	const QByteArray bytes(str.toLocal8Bit());
	char *ret = (char *)malloc(bytes.size() + 1);
	memcpy(ret, bytes.data(), bytes.size());
	ret[bytes.size()] = 0;
	return ret;
}
