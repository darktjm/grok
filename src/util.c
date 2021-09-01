/*
 * Initializes everything and starts a calendar window for the current
 * month. The interval timer (for autosave and entry recycling) and a
 * few routines used by everyone are also here.
 *
 *	section_name(db,n)		return name of section nsect
 *	resolve_tilde(path)		return path with ~ replaced with home
 *					directory as found in $HOME
 *	find_file(buf, name,exec)	find file <name> and store the path
 *					in <buf>. Return false if not found.
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
#if HAS_ASPRINTF && !defined(_GNU_SOURCE)
#define _GNU_SOURCE // asprintf
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <QtWidgets>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include "layout-qss.h"
#include "grok.h"
#include "form.h"
#include "proto.h"
#if USE_COLOR_ICON
#define static static const
#include "Grok.xpm"
#undef static
#else
#include "bm_icon.h"
#endif



/*
 * extract a section name from the section paths by removing the extension
 * and common substrings.
 */

const char *section_name(
	const DBASE	 *dbase,		/* contains section array */
	int		 n)			/* 0 .. dbase->nsects-1 */
{
	static char	 name[40];		/* returned name */
	SECTION 	*sect;			/* section list */
	char	 	*path;			/* path to reduce */
	char	 	*trunc;			/* trailing unique part */
	int	 	i;			/* section counter */

	if (!dbase || n < 0 || n >= dbase->nsects)
		return("");
	sect = dbase->sect;
	path = sect[n].path;
	if (!path || !*path)
		return("");
	trunc = strrchr(sect[n].path, '/');
	if (!trunc)
		trunc = path;
	while (trunc > path) {
		for (i=dbase->nsects-1; i >= 0; i--)
			if (i != n && strncmp(path, sect[i].path, trunc-path))
				break;
		if (i < 0)
			break;
		while (--trunc > path && *trunc != '/');
	}
	if (*trunc == '/')
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

const char *resolve_tilde(
	const char	*path,			/* path with ~ */
	const char	*ext)			/* append extension unless 0 */
{
	struct passwd	*pw;			/* for searching home dirs */
	static char	*pathbuf = 0;		/* path with ~ expanded */
	static size_t	pathbuflen = 0;		/* allocated length */
	const char	*prefix = 0;		/* GROKDIR if need be */
	const char	*hpath;			/* path to scan for ~ */
	const char	*p;			/* username copy pointers */
	const char	*home = 0;		/* home dir (if ~ in path) */
	int		i;			/* strip trailing / and /. */
	int		tlen;			/* target length */
	int		plen;			/* path length */

	if (*path != '~' && *path != '/' && (*path != '.' || path[1] != '/')) {
		prefix = GROKDIR;
		if (ext && (p = strrchr(path, '.')) && !strcmp(p+1, ext))
			ext = 0;
	} else
		ext = 0;
	plen = strlen(path);
	tlen = plen + 1 + (prefix ? strlen(prefix) + 1 : 0) + (ext ? strlen(ext) + 1 : 0);
	hpath = prefix ? prefix : path;
	if (*hpath == '~') {
		const char *who;
		if (!hpath[1] || hpath[1] == '/') {
			hpath++;
			who = "";
			if (!(home = getenv("HOME")))
				home = getenv("home");
			if(home)
				tlen += strlen(home) - 1;
			/* else ~ -> . below */
		} else {
			for(p = hpath + 1; *p && *p != '/'; p++);
			fgrow(0, "tilde_expand", char, pathbuf,
			      (int)(p - hpath), &pathbuflen);
			tmemcpy(char, pathbuf, hpath + 1, p - hpath - 1);
			pathbuf[p - hpath - 1] = 0;
			who = pathbuf;
			if ((pw = getpwnam(pathbuf)))
				home = pw->pw_dir;
			tlen -= (int)(p - hpath);
			hpath = p;
			if(home)
				tlen += strlen(home);
			else
				tlen++; /* "." below */
		}
		if (!home) {
			fprintf(stderr,
				"%s: can't evaluate ~%s in %s, using .\n",
						progname, who, hpath);
			home = ".";
		}
	}
	if(prefix)
		prefix = hpath;
	else {
		plen -= hpath - path;
		path = hpath;
	}
	/* old code might've caught homedirs with trailing /, but I don't care */
	for (i=plen-1; i > 0; )
		if (path[i] == '/' || (path[i] == '.' && path[i-1] == '/')) {
			i--;
			tlen--;
		} else
			break;
	if(i == plen - 1 && !prefix && !ext && !home)
		return(path);
	plen = i + 1;
	fgrow(0, "tilde_expand", char, pathbuf, tlen, &pathbuflen);
	i = 0;
	if(home) {
		i = strlen(home);
		tmemcpy(char, pathbuf, home, i);
	}
	if(prefix) {
		strcpy(pathbuf + i, prefix);
		i += strlen(prefix) + 1;
		pathbuf[i - 1] = '/';
	}
	tmemcpy(char, pathbuf + i, path, plen);
	i += plen;
	if(ext) {
		pathbuf[i++] = '.';
		strcpy(pathbuf + i, ext);
	} else
		pathbuf[i] = 0;
	return pathbuf;
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

const char *find_file(
	const char		*name,		/* file name to locate */
	bool			exec)		/* must be executable? */
{
	int			method;		/* search path counter */
	const char		*path;		/* $PATH or DEFAULTPATH */
	int			namelen;	/* len of tail of name */
	const char		*p;		/* string copy pointer */
	static char		*ret = 0;	/* if found in path */
	static size_t		retlen;

	if (*name == '/')				/* begins with / */
		return name;
	if (*name == '~') 				/* begins with ~ */
		return resolve_tilde(name, 0);
	namelen = strlen(name);
	for (method=0; ; method++) {	
		switch(method) {
		  case 0:   path = PATH;		break;
		  case 1:   path = getenv("GROK_PATH");	break;
		  case 2:   path = getenv("PATH");	break;
		  case 3:   path = DEFAULTPATH;		break;
		  default:  return NULL;
		}
		if (!path)
			continue;
		do {
			for(p = path; *p && *p != ':'; p++);
			grow(0, "find file", char, ret, namelen + (int)(p - path) + 2, &retlen);
			memcpy(ret, path, p - path);
			ret[p - path] = '/';
			strcpy(ret + (int)(p - path) + 1, name);
			if (!access(ret, exec ? X_OK : R_OK))
				return ret;
			path = p+1;
		} while (*p);
	}
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


/* This used to be here for systems w/o strdup */
/* But now it's here to allow NULL and check for errors */
char *mystrdup(
	const char *s)
{
	char *d = s ? strdup(s) : NULL;
	if(s && !d)
		fatal("No memory for string");
	return d;
}

/* This also converts "" to NULL */
char *zstrdup(
	const char *s)
{
	return BLANK(s) ? NULL : mystrdup(s);
}

/*---------------------------------------------------------------------------*/
/*
 * draw some text into a button. This is here because it's used by many
 * routines.
 *
 * this used to set cursor position for text widgets, but that shouldn't
 * be done in a commonly used callback.
 */

static void set_w_text(QWidget *w, QString &string)
{
	if(QPushButton *b = dynamic_cast<QPushButton *>(w))
		b->setText(string);
	else if(QLabel *l = dynamic_cast<QLabel *>(w))
		l->setText(string);
	else if(QLineEdit *le = dynamic_cast<QLineEdit *>(w))
		le->setText(string);
	else if(QTextEdit *te = dynamic_cast<QTextEdit *>(w))
		te->setPlainText(string);
	else if(QComboBox *cb = dynamic_cast<QComboBox *>(w))
		cb->setCurrentText(string);
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
	bool			set)
{
	// Don't generate an event of any kind if it's already set correctly
	QAbstractButton *b = dynamic_cast<QAbstractButton *>(w);
	if (b && b->isChecked() != !!set)
		b->setChecked(set);
}


/*
 * return the text string in a text button. If ptr==0, return a static
 * buffer containing the text; else assume that ptr points to a strdup'ed
 * pointer. Free it if nonzero, then strdup the new text.
 */

static char *readbutton(
	QWidget			*w,
	char			**ptr,
	bool			skipblank,
	bool			noblanks)
{
	static char		*buf = NULL;
	static size_t		bufsize = 0;
	int			size;
	QString			string;

	if (w) {
		int			s = 0, e, i;
		if(QLineEdit *le = dynamic_cast<QLineEdit *>(w))
			string = le->text();
		else if(QComboBox *cb = dynamic_cast<QComboBox *>(w))
			string = cb->currentText();
		else
			string = dynamic_cast<QTextEdit *>(w)->toPlainText();
		e = string.size();
		if (skipblank) {
			while(s < e && (string[s] == ' ' || string[s] == '\t'))
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

		fgrow(0, "reading button", char, buf, size, &bufsize);
		tmemcpy(char, buf, bytes.data(), size - 1);
		buf[size - 1] = 0;
	} else if (buf)
		*buf = 0;
	if (ptr) {
		zfree(*ptr);
		*ptr = zstrdup(buf);
	}
	return(buf);
}

char *read_text_button_noskipblank(QWidget *w, char **ptr)
	{ return(readbutton(w, ptr, false, false)); }

char *read_text_button(QWidget *w, char **ptr)
	{ return(readbutton(w, ptr, true, false)); }

char *read_text_button_noblanks(QWidget *w, char **ptr)
	{ return(readbutton(w, ptr, true, true)); }


/*
 * set icon for the application or a shell.
 */

/*ARGSUSED*/
void set_icon(
	QWidget			*shell,
UNUSED	int			sub)		/* 0=main, 1=submenu */
{
	static QIcon *icon;
	if(!icon) {
#if USE_COLOR_ICON
		icon = new QIcon(QPixmap(Grok));
#else
		icon = new QIcon(QBitmap::fromData(QSize(bm_icon_width, bm_icon_height), bm_icon_bits));
#endif
	}
	/* there is only 1 icon, so sub is actually ignored */
	shell->setWindowIcon(*icon);
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
	QWidget		*w,		/* widget string will show in */
	QString		&str,		/* string to truncate */
	int		len)		/* max len in pixels */
{
	w->ensurePolished();
	const QFontMetrics &fs(w->fontMetrics());
	while(str.size() > 0) {
		if(fs.boundingRect(str).width() <= len)
			break;
		str.chop(1);
	}
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
	w->ensurePolished();
	return w->fontMetrics().boundingRect(string).width();
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
	LayoutQSS *lq = new LayoutQSS(l);
	l->addWidget(lq);
	lq->ensurePolished();
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
	size_t len = bytes.size();
	if(!len)
		return NULL;
	char *ret = alloc(0, "string", char, len + 1);
	tmemcpy(char, ret, bytes.data(), len);
	ret[len] = 0;
	return ret;
}

void *abort_malloc(
	QWidget		*parent,	/* for message popups; 0 = fatal */
	const char	*purpose,	/* for messages; may be 0 */
	void		*old,		/* for realloc; may be 0 */
	size_t		len,		/* how much, minimum */
	size_t		*mlen,		/* if non-NULL, track size of buffer */
	int		flags)		/* additional flags; see proto.h */
{
	if(!len) {
		if(old && (flags & AM_SHRINK)) {
			free(old);
			old = NULL;
			if(mlen)
				*mlen = 0;
		}
		return old;
	}
	void *ret;
	size_t newsize = len, osize = old && mlen ? *mlen : 0;

	if(mlen) {
		newsize = osize ? osize : MIN_AM_SIZE;
		if((flags & AM_SHRINK) && newsize > len) {
			if(newsize > MAX_AM_DOUBLE)
				newsize -= MAX_AM_DOUBLE * ((newsize - len) / MAX_AM_DOUBLE - 1);
			while(newsize > MIN_AM_SIZE && len < newsize / 2)
				newsize /= 2;
		}
		while(newsize < len && newsize < MAX_AM_DOUBLE)
			newsize *= 2;
		if(newsize < len) {
			size_t nb = (len - newsize + MAX_AM_DOUBLE - 1) / MAX_AM_DOUBLE;
			newsize += nb * MAX_AM_DOUBLE;
		}
		*mlen = newsize;
	}
	if(newsize == osize)
		return old;
	if(old && !(flags & AM_REALLOC)) {
		free(old);
		old = NULL;
		osize = 0;
	}
	if(!old)
		ret = malloc(newsize);
	else
		ret = realloc(old, newsize);
	if(!ret) {
		if(!parent) {
			if(purpose)
				fatal("Memory allocation failed for %s", purpose);
			else
				fatal("Memory allocation error");
		}
		/* recoverable errors that allocate memory (e.g. for the error
		 * popup) seem like a bad idea. */
		/* freeing old before dying prevents leak-on-exit, but who cares? */
		/* Maybe I shouldn't free it at all, since non-fatal reallocs */
		/* generally want to keep what's been alloced so far */
		zfree(old);
		if(mlen)
			*mlen = 0;
		/* Maybe I need a flag to suppress notification */
		if(purpose)
			create_error_popup(parent, errno, "Memory allocation failed for %s", purpose);
		else
			create_error_popup(parent, errno, "Memory allocation failed");
		return NULL;
	}
	if((flags & AM_ZERO) && newsize > osize)
		memset((char *)ret + osize, 0, newsize - osize);
	return ret;
}

/* realpath is broken pretty much everywhere, at least in how I want to use
 * it.  None of them seem to be able to handle missing files (including
 * Qt's FileInfo::canonicalize...() functions).  The standard also doesn't
 * guarantee behavior I desire, such as removal of duplicate slashes, even
 * though GNU libc's realpath does remove them. */
/* On the other hand, this is probably completely broken for mingw */
/* FIXME: this should be non-fatal; return NULL on errors */
/* then again, maybe some memory errors should be fatal. */
const char *canonicalize(const char *path, bool dir_only)
{
	static char *cwd = 0;
	static size_t cwdsize;
	static char *link_buf = 0;
	static size_t lbuf_len;
	static char *full_path = 0;
	static size_t fpsize;
	static char *canon_out = 0;
	static size_t canon_size;
	size_t canon_len;
	char *p, *q;
	int link_loop, nprev;
	ssize_t link_len;

	if(!path || *path != '/') {
		if(!cwd) {
			grow(0, "canonicalize", char, cwd, 10, &cwdsize);
			while(!getcwd(cwd, cwdsize)) {
				if(errno != ERANGE)
					fatal("Can't read current directory");
				grow(0, "canonicalize", char, cwd, cwdsize * 2, &cwdsize);
			}
		}
		size_t cwd_len = strlen(cwd);
		grow(0, "canonicalize", char, full_path, cwd_len + strlen(STR(path)) + 2, &fpsize);
		memcpy(full_path, cwd, cwd_len);
		full_path[cwd_len] = '/';
		strcpy(full_path + cwd_len + 1, STR(path));
	} else {
		grow(0, "canonicalize", char, full_path, strlen(path) + 1, &fpsize);
		strcpy(full_path, STR(path));
	}
	if(dir_only)
		*strrchr(full_path, '/') = 0;
	if(!*full_path)
		return "/";
	canon_len = 0;
	nprev = 0;
	link_loop = 0;
	p = full_path + strlen(full_path);
	while(p > full_path) {
		while(p > full_path && p[-1] == '/') p--;
		if(p == full_path)
			break;
		if(p[-1] == '.' && p[-2] == '/') {
			p -= 2;
			continue;
		}
		if(p[-1] == '.' && p[-2] == '.' && p[-3] == '/') {
			nprev++;
			p -= 3;
			continue;
		}
		grow(0, "canonicalize", char, link_buf, 10, &lbuf_len);
		*p = 0;
		while((link_len = readlink(full_path, link_buf, lbuf_len)) == (ssize_t)lbuf_len)
			grow(0, "canonicalize", char, link_buf, lbuf_len * 2, &lbuf_len);
		if(link_len <= 0 && errno != EINVAL && errno != ENOENT)
			fatal("Can't read sym link");
		if(link_len > 0) {
			/* 100 is an arbitrary limit of my patience */
			if(++link_loop == 100)
				fatal("sym link loop in path");
			if(link_buf[0] == '/')
				p = full_path;
			else
				/* assumes getcwd returns an absolute path */
				while(p[-1] != '/') p--;
			q = full_path;
			grow(0, "canonicalize", char, full_path,
			     link_len + 1 + (int)(p - full_path), &fpsize);
			memcpy(full_path + (int)(p - q), link_buf, link_len);
			p = full_path + (int)(p - q) + link_len;
			continue;
		}
		link_loop = 0;
		for(q = p; q > full_path && *q != '/'; q--);
		if(nprev) {
			nprev--;
			p = q;
			continue;
		}
		grow(0, "canonicalize", char, canon_out, canon_len + (int)(p - q) + 1, &canon_size);
		memmove(canon_out + (int)(p - q), canon_out, canon_len);
		memcpy(canon_out, q, (int)(p - q));
		canon_len += p - q;
		p = q;
	}
	if(!canon_len)
		canon_out[canon_len++] = '/';
	canon_out[canon_len] = 0;
	return canon_out;
}

// If asprintf is not available in libc:
#if !HAS_ASPRINTF
/* slow, simplistic asprintf replacement */
int vasprintf(char **strp, const char *fmt, va_list ap)
{
    int len = vsnprintf(NULL, 0, fmt, ap);
    *strp = (char *)malloc(len + 1);
    if(!*strp)
	return -1;
    vsnprintf(*strp, len + 1, fmt, ap);
    return len;
}

int asprintf(char **strp, const char *fmt, ...)
{
    va_list al;
    va_start(al, fmt);
    int ret = vasprintf(strp, fmt, al);
    va_end(al);
    return ret;
}
#endif
 
