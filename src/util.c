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
#include <X11/Xos.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <signal.h>
#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "version.h"
#include "bm_icon.h"



/*
 * extract a section name from the section paths by removing the extension
 * and common substrings.
 */

char *section_name(
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
	if (i > sizeof(name)-1)
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
	char		*ext)			/* append extension unless 0 */
{
	struct passwd	*pw;			/* for searching home dirs */
	static char	pathbuf[1024];		/* path with ~ expanded */
	char		buf[1024];		/* buf for prepending GROKDIR*/
	char		*p, *q;			/* username copy pointers */
	char		*home = 0;		/* home dir (if ~ in path) */
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
	char			*name,		/* file name to locate */
	BOOL			exec)		/* must be executable? */
{
	int			method;		/* search path counter */
	char			*path;		/* $PATH or DEFAULTPATH */
	int			namelen;	/* len of tail of name */
	register char		*p, *q;		/* string copy pointers */

	if (*name == '/') {				/* begins with / */
		strcpy(buf, name);
		return(TRUE);
	}
	if (*name == '~') { 				/* begins with ~ */
		strcpy(buf, resolve_tilde(name, 0));
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
void fatal(char *fmt, ...)
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
	register char *s)
{
	register char *p = NULL;

	if (s && (p = (char *)malloc(strlen(s)+1)))
		strcpy(p, s);
	return(p);
}


/*
 * some systems use mybzero (BSD), others memset (SysV), I'll roll my own...
 */

void mybzero(
	void		*p,
	register int	n)
{
	register char	*q = p;
	while (n--) *q++ = 0;
}


/*
 * Sinix doesn't have strcasecmp, so here is my own. Not as efficient as
 * the canonical implementation, but short, and it's not time-critical.
 */

int mystrcasecmp(
	register char *a,
	register char *b)
{
	register char ac, bc;

	for (;;) {
		ac = *a++;
		bc = *b++;
		if (!ac || !bc)
			break;
		if (ac <= 'Z' && ac >= 'A')	ac += 'a' - 'A';
		if (bc <= 'Z' && bc >= 'A')	bc += 'a' - 'A';
		if (ac != bc)
			break;
	}
	return(ac - bc);
}


/*---------------------------------------------------------------------------*/
/*
 * draw some text into a button. This is here because it's used by many
 * routines.
 */

void print_button(Widget w, char *fmt, ...)
{
	va_list			parm;
	Arg			args;
	XmString		string;
	char			buf[1024];

	if (w) {
		va_start(parm, fmt);
		if (!fmt) fmt = "";
		vsprintf(buf, fmt, parm);
		va_end(parm);
		string = XmStringCreateSimple(buf);
		XtSetArg(args, XmNlabelString, string);
		XtSetValues(w, &args, 1);
		XmStringFree(string);
	}
}

void print_text_button(Widget w, char *fmt, ...)
{
	va_list			parm;
	XmString		string;
	char			buf[1024];

	if (w) {
		va_start(parm, fmt);
		if (!fmt) fmt = "";
		vsprintf(buf, fmt, parm);
		va_end(parm);
		string = XmStringCreateSimple(buf);
		XmTextSetString(w, buf);
		XmTextSetInsertionPosition(w, strlen(buf));
		XmStringFree(string);
	}
}

void print_text_button_s(Widget w, char *str)
{
	XmString		string;

	if (!w)
		return;
	if (!str)
		str = "";
	string = XmStringCreateSimple(str);
	XmTextSetString(w, str);
	XmTextSetInsertionPosition(w, strlen(str));
	XmStringFree(string);
}


/*
 * turn radio button on or off
 */

void set_toggle(
	Widget			w,
	BOOL			set)
{
	Arg			arg;

	if (w) {
		XtSetArg(arg, XmNset, set);
		XtSetValues(w, &arg, 1);
	}
}


/*
 * return the text string in a text button. If ptr==0, return a static
 * buffer containing the text; else assume that ptr points to a strdup'ed
 * pointer. Free it if nonzero, then strdup the new text.
 */

static char *readbutton(
	Widget			w,
	char			**ptr,
	BOOL			skipblank,
	BOOL			noblanks)
{
	char			*string, *s;	/* contents of text widget */
	static char		*buf;
	static int		bufsize;
	int			size;

	if (w) {
		s = string = XmTextGetString(w);
		if (skipblank) {
			while (*s == ' ' || *s == '\t')
				s++;
			if (noblanks)
				for (size=0; s[size]; size++)
					if (s[size] == ' ' || s[size] == '\t')
						s[size] = '_';
			size = strlen(s);
			while (size && (s[size-1] == ' ' || s[size-1] == '\t'))
				size--;
			s[size++] = 0;
		} else
			size = strlen(s) + 1;

		if (size > bufsize) {
			if (buf) free(buf);
			if (!(buf = malloc(bufsize = size)))
				return(0);
		}
		strcpy(buf, s);
		XtFree(string);
	} else if (buf)
		*buf = 0;
	if (ptr) {
		if (*ptr) free(*ptr);
		*ptr = buf && *buf ? mystrdup(buf) : 0;
	}
	return(buf);
}

char *read_text_button_noskipblank(Widget w, char **ptr)
	{ return(readbutton(w, ptr, FALSE, FALSE)); }

char *read_text_button(Widget w, char **ptr)
	{ return(readbutton(w, ptr, TRUE, FALSE)); }

char *read_text_button_noblanks(Widget w, char **ptr)
	{ return(readbutton(w, ptr, TRUE, TRUE)); }


/*
 * set icon for the application or a shell.
 */

void set_icon(
	Widget			shell,
	int			sub)		/* 0=main, 1=submenu */
{
#ifndef sgi
	Pixmap			icon;

	if (icon = XCreatePixmapFromBitmapData(display,
				DefaultRootWindow(display),
				(char *)(sub ? bm_icon_bits : bm_icon_bits),
				bm_icon_width, bm_icon_height,
				BlackPixelOfScreen(XtScreen(shell)),
				WhitePixelOfScreen(XtScreen(shell)),
				DefaultDepth(display, DefaultScreen(display))))

		XtVaSetValues(shell, XmNiconPixmap, icon, NULL);
#endif
}


/*
 * set mouse cursor to one of the predefined shapes. This is used in the
 * form editor's canvas drawing area. The list of supported cursors is in
 * <X11/cursorfont.h>.
 */

void set_cursor(
	Widget			w,		/* in which widget */
	int			n)		/* which cursor, one of XC_* */
{
	static Cursor		cursor[XC_num_glyphs];

	if (!cursor[n])
		cursor[n] = XCreateFontCursor(display, n);
	XDefineCursor(display, XtWindow(w), cursor[n]);
}


/*
 * truncate <string> such that it is not longer than <len> pixels when
 * drawn with font <sfont>, by storing \0 somewhere in the string.
 */

void truncate_string(
	register char	*string,	/* string to truncate */
	register int	len,		/* max len in pixels */
	int		sfont)		/* font of string */
{
	while (*string) {
		len -= font[sfont]->per_char
		       [*string - font[sfont]->min_char_or_byte2].width;
		if (len < 0)
			*string = 0;
		else
			string++;
	}
}


/*
 * return the length of <string> in pixels, when drawn with <sfont>
 */

int strlen_in_pixels(
	register char	*string,	/* string to truncate */
	int		sfont)		/* font of string */
{
	register int	len = 0;	/* max len in pixels */

	while (*string) {
		len += font[sfont]->per_char
		       [*string - font[sfont]->min_char_or_byte2].width;
		string++;
	}
	return(len);
}


/*
 * convert ascii code to and from a string representation. This is used for
 * a few input buttons in the form editor window that accept single chars.
 */

char *to_octal(
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
