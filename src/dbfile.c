/*
 * read and write database files.
 *
 *	write_dbase(form, path)		write dbase
 *	read_dbase(form, path)		read dbase into empty dbase struct
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <dirent.h>
#include <QtWidgets>
#include "config.h"

#if defined(GROK) || defined(PLANGROK)

#include "grok.h"
#include "form.h"
#include "proto.h"

/* prior versions did not disallow these as the field delimiter
 * but it's pretty clear they won't work.  ESC always escapes the
 * next character as data, and R_SEP always delimits rows */
#define R_SEP	'\n'			/* row (card) separator */
#define ESC	'\\'			/* treat next char literally */

static int	ctimex_next;		/* for generating unique row->ctimex */


/*
 * If the print spooler dies for some reason, print a message. Don't let
 * grok die because of the broken pipe signal.
 */

static void broken_pipe_handler(int sig)
{
	create_error_popup(mainwindow, 0,
			"Procedural script aborted with signal %d\n", sig);
	signal(SIGPIPE, SIG_IGN);
}


/*
 * Write the database and all the items in it. The <path> is from the
 * forms <dbase> field. Both the database and the form will be stored
 * in a CARD structure; both are required to access cards. If the
 * database is procedural, open a pipe to the database process and add
 * -r option.
 * Returns FALSE if the file could not be written.
 */

static BOOL write_file (DBASE *, const FORM *, int);
static BOOL write_tfile(const DBASE *, const FORM *, int);

BOOL write_dbase(
	DBASE			*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	BOOL			force)		/* write even if not modified*/
{
	int			s;		/* section index */
	BOOL			ok = TRUE;

	if (dbase->nsects == 1 && (dbase->modified || force)) {
		ok  = write_file (dbase, form, 0);
		ok &= write_tfile(dbase, form, 0);
	} else
		for (s=0; s < dbase->nsects; s++)
			if (dbase->sect[s].modified &&
					(!dbase->sect[s].rdonly || force)) {
				ok &= write_file (dbase, form, s);
				ok &= write_tfile(dbase, form, s);
			}
	return(ok);
}


static BOOL write_file(
	DBASE			*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	int			nsect)		/* section to write */
{
	SECTION			*sect;		/* section to write */
	const char		*path;		/* file to write list to */
	FILE			*fp;		/* open file */
	int			r, c;		/* row and column counters */
	int			hicol;		/* # of columns of card -1 */
	char			*value;		/* converted data to write */
	char			buf[40];	/* for date/time conversion */
	ITEM			*item;		/* item describing value */
	int			i;		/* index of item */
	char			*p;		/* string copy pointer */

	sect = &dbase->sect[nsect];
	path = sect->path ? sect->path : form->dbase;
	if (!path || !*path) {
		create_error_popup(mainwindow, 0,
			"Database has no name, cannot save to disk");
		return(FALSE);
	}
	path = resolve_tilde(path, "db");
	if (form->proc) {
		char *cmd = alloc(mainwindow, "procedural command", char,
				  strlen(path) + strlen(form->name) + 5);
		if(!cmd)
			return(FALSE);
		sprintf(cmd, "%s -w %s", path, form->name);
		if (!(fp = popen(cmd, "w"))) {
			create_error_popup(mainwindow, errno,
				"Failed to run procedural command %s", cmd);
			return(FALSE);
		}
	} else
		if (!(fp = fopen(path, "w"))) {
			create_error_popup(mainwindow, errno,
				"Failed to create database file %s", path);
			return(FALSE);
		}
	for (r=0; r < dbase->nrows; r++) {
		if (nsect != dbase->row[r]->section)
			continue;
#if 0 /* 1.4.3: always write out all fields */
		for (hicol=dbase->row[r]->ncolumns-1; hicol > 0; hicol--)
			if (dbase_get(dbase, r, hicol))
				break;
#else
		hicol=dbase->row[r]->ncolumns-1;
#endif
		for (c=0; c <= hicol; c++) {
			if ((value = dbase_get(dbase, r, c))) {
				for (i=form->nitems-1; i >= 0; i--) {
					item = form->items[i];
					if (item->column == c)
						break;
				}
				if (i >= 0 && item->type == IT_TIME) {
					long secs = atoi(value);
					switch(item->timefmt) {
					  case T_DATE:
						sprintf(value = buf, "%s",
							mkdatestring(secs));
						break;
					  case T_TIME:
						sprintf(value = buf, "%s",
							mktimestring(secs, 0));
						break;
					  case T_DATETIME:
						sprintf(value = buf, "%s %s",
							mkdatestring(secs),
							mktimestring(secs, 0));
						break;
					  case T_DURATION:
						sprintf(value = buf, "%s",
							mktimestring(secs, 1));
						break;
					}
				}
				for (p=value; *p; p++) {
					if (*p == R_SEP ||
					    *p == form->cdelim ||
					    *p == ESC)
						fputc(ESC, fp);
					fputc(*p, fp);
				}
			}
			if (c < hicol)
				fputc(form->cdelim, fp);
		}
		fputc(R_SEP, fp);
	}
	if (form->proc) {
		if (pclose(fp)) {
			create_error_popup(mainwindow, errno,
				"%s:\nfailed to create database", path);
			return(FALSE);
		}
	} else
		fclose(fp);
	if (sect)
		sect->modified = FALSE;
	dbase->modified = FALSE;
	sect->mtime = time(0);
	print_info_line();
	return(TRUE);
}


/*
 * Read a database from a file. The database must be empty (just created
 * with dbase_create()). Returns FALSE if the file could not be read.
 * Print a warning if the database is supposed to be writable but isn't.
 *
 * This works by first reading in fields, allocating one string for each.
 * The strings are stored in a long list. Escaped separators are resolved.
 * Row separators are stored as null pointers in the list. When the end
 * of the file is reached, the number of rows and columns is known, and
 * the pointers in the list are put in the right places of the dbase array.
 */

#define LCHUNK	4096		/* alloc this many new list ptrs */
#define BCHUNK	1024		/* alloc this many new chars for item */

static BOOL find_db_file     (DBASE *, const FORM *, const char *);
static BOOL read_dir_or_file (DBASE *, const FORM *, const char *);
static BOOL read_file	     (DBASE *, const FORM *, const char *, time_t);
static BOOL read_tfile	     (DBASE *, const FORM *, const char *);

BOOL read_dbase(
	DBASE			*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	const char		*path)		/* file to read list from */
{
	static char		*pathbuf = 0;	/* file name with path */
	static size_t		pathbuflen;
	BOOL			ret;		/* return code, FALSE=error */

	if (!path || !*path) {
		create_error_popup(mainwindow, 0,
			"Database has no name, cannot read from disk");
		return(FALSE);
	}
	dbase_delete(dbase);
#ifdef GROK
	init_variables();
#endif
	ctimex_next = 0;
	if (*path != '/' && *path != '~' && form->path) {
		const char *p;
		int plen = strlen(path);
		if((p = strrchr(form->path, '/'))) {
			grow(0, "db path", char, pathbuf, (int)(p - form->path) + plen + 2, &pathbuflen);
			memcpy(pathbuf, form->path, p-form->path+1);
			memcpy(pathbuf + (p-form->path)+1, path, plen + 1);
			path = pathbuf;
		}
	}
	if (!(ret = find_db_file(dbase, form, path))) {
		DBASE *ndb = dbase_create();
		*dbase = *ndb;
		free(ndb);
		create_error_popup(mainwindow, 0, "Failed to read %s", path);
	}

	dbase->currsect = -1;
	dbase->modified = FALSE;
	print_info_line();
	return(ret);
}


/*
 * read both path.db and path, in that order. Return FALSE if both failed.
 */

static BOOL find_db_file(
	DBASE			*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	const char		*path)		/* file to read list from */
{
	static char		*pathbuf = 0;	/* file name with path */
	static size_t		pathbuflen;
	int			nread = 0;	/* # of successful reads */

	grow(0, "db path", char, pathbuf, strlen(path) + 4, &pathbuflen);
	sprintf(pathbuf, "%s.db", path);
	if (!access(pathbuf, F_OK))
		nread += read_dir_or_file(dbase, form, pathbuf);
	if (!nread && !access(path, F_OK))
		/* FIXME: delete pathbuf, maybe */
		nread += read_dir_or_file(dbase, form, path);
	return(nread > 0);
}


/*
 * read path. If it's a directory, recurse. Return FALSE if nothing was found.
 */

#define MAXSC  1000		/* no more than 200 sections */

static int compare_name(
	const void	*u,
	const void	*v)
{
	return(strcmp(*(char **)u, *(char **)v));
}

static BOOL read_dir_or_file(
	DBASE			*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	const char		*path)		/* file to read list from */
{
	char			*name[MAXSC];	/* directory listing */
	static char		*pathbuf = 0;	/* file name with path */
	static size_t		pathbuflen;
	char			*ext;		/* ignore .ts timestamp files*/
	struct stat		statbuf;	/* is path a directory? */
	DIR			*dir;		/* open directory file */
	struct dirent		*dp;		/* one directory entry */
	int			nfiles = 0;	/* # of files in directory */
	int			i, n=0;

	if (stat(path, &statbuf))
		return(FALSE);
	if (!(statbuf.st_mode & S_IFDIR))
		return(read_file (dbase, form, path, statbuf.st_mtime) &&
		       read_tfile(dbase, form, path));
	if (!(dir = opendir(path)))
		return(FALSE);
	while ((dp = readdir(dir)) && n < MAXSC)
		if (*dp->d_name != '.' &&
		   (!(ext = strrchr(dp->d_name, '.')) || strcmp(ext, ".ts")))
			name[n++] = mystrdup(dp->d_name);
	(void)closedir(dir);
	if (n)
		qsort(name, n, sizeof(char *), compare_name);

	int plen = strlen(path);
	for (i=0; i < n; i++) {
		int nlen = strlen(name[i]);
		grow(0, "db path", char, pathbuf, plen + nlen + 2, &pathbuflen);
		memcpy(pathbuf, path, plen);
		pathbuf[plen] = '/';
		memcpy(pathbuf + plen + 1, name[i], nlen + 1);
		free(name[i]);
		nfiles += read_dir_or_file(dbase, form, pathbuf);
	}
	dbase->havesects = TRUE;
	return(nfiles > 0);
}


/*
 * read one plain file and append to dbase section list. Return FALSE on error.
 */

static BOOL read_file(
	DBASE			*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	const char		*path,		/* file to read list from */
	time_t			mtime)		/* file modification time */
{
	SECTION			*sect;		/* new section */
	FILE			*fp;		/* open file */
	int			nc = 0;		/* size of curr line */
	int			col = 0;	/* current column being added*/
	int			row = -1;	/* current row being added */
	char			*buf, *p;	/* buffer for one item */
	int			bindex = 0;	/* next free byte in buf */
	int			bsize;		/* size of buf in bytes */
	unsigned char		c, c0;		/* next char from file */
	BOOL			error;		/* in TRUE, abort */
	ITEM			*item;		/* for time/date conversion */
	int			i;

							/* step 1: open file */
	signal(SIGPIPE, broken_pipe_handler);
	if (form->proc) {
		path = resolve_tilde(path, "db");
		char *cmd = alloc(mainwindow, "procedural command", char,
				  strlen(path) + strlen(form->name) + 5);
		if(!cmd)
			return(FALSE);
		sprintf(cmd, "%s -r %s", path, form->name);
		if (!(fp = popen(cmd, "r"))) {
			create_error_popup(mainwindow, errno,
				"Failed to read procedural database %s", path);
			return(FALSE);
		}
	} else {
		path = resolve_tilde(path, "db");
		if (!(fp = fopen(path, "r")))
			return(FALSE);
	}
							/* step 2: new sectn */
	i = (dbase->nsects+1) * sizeof(SECTION);
	if (!(sect = (SECTION *)(dbase->sect ? realloc(dbase->sect, i) : malloc(i)))) {
		create_error_popup(mainwindow, errno,
			"No memory for section %s", path);
		form->proc ? pclose(fp) : fclose(fp);
		return(FALSE);
	}
	dbase->sect = sect;
	memset(sect = &dbase->sect[dbase->nsects], 0, sizeof(SECTION));
	dbase->currsect = dbase->nsects++;
	sect->mtime = mtime;
	sect->path  = mystrdup(path);
							/* step 3: read list */
	buf = (char  *)malloc((bsize = BCHUNK) * sizeof(char));
	error = !buf;
	while (!error) {					/* read file:*/
		c = c0 = fgetc(fp);
		if (!feof(fp) && c == ESC)
			c = fgetc(fp);
								/* end of str*/
		if (feof(fp) || c0 == form->cdelim || c0 == R_SEP) {
			if (!nc) {
				if (c0 == form->cdelim) col++;
				if (feof(fp))	break;
				else		continue;
			}
			buf[bindex] = 0;			/* next col */
								/* .. date? */
			for (i=-1, p=buf; *p; p++)
				if (*p=='.' || *p==':' || *p=='/') {
					for (i=form->nitems-1; i >= 0; i--) {
						item = form->items[i];
						if (item->column == col)
							break;
					}
					break;
				}
			if (i >= 0 && item->type == IT_TIME)	/* .. -> int */
				switch(item->timefmt) {
				  case T_DATE:
					sprintf(buf, "%ld",
						(long)parse_datestring(buf));
					break;
				  case T_TIME:
					sprintf(buf, "%ld",
						(long)parse_timestring(buf, FALSE));
					break;
				  case T_DATETIME:
					sprintf(buf, "%ld",
						(long)parse_datetimestring(buf));
					break;
				  case T_DURATION:
					sprintf(buf, "%ld",
						(long)parse_timestring(buf, TRUE));
				}
								/* store str */
			if (row < 0)
				if (error |= !dbase_addrow(&row, dbase))
					break;
			dbase_put(dbase, row, col++, buf);
			dbase->row[row]->mtime  = 0;
			dbase->row[row]->ctime  = 0;
			dbase->row[row]->ctimex = ctimex_next++;
			bindex = 0;
			if (feof(fp) || c0 == R_SEP) {		/* next row */
				col = nc = 0;
				row = -1;
				if (feof(fp))
					break;
			}
		} else {					/* store char*/
			if (bindex+1 >= bsize) {
				char *newb = (char *)realloc(buf,
						(bsize += BCHUNK));
				if (error |= !newb)
					break;
				buf = newb;
			}
			buf[bindex++] = c;
			nc++;
		}
	}
								/* done. */
	zfree(buf);
	error |= form->proc ? !!pclose(fp) : !!fclose(fp);
	sect->modified = FALSE;
	sect->rdonly   =
	dbase->rdonly  = form->proc ? FALSE : !!access(path, W_OK);
	if (error)
		create_error_popup(mainwindow, errno,
			"Failed to allocate memory for\ndatabase %s", path);
	return(!error);
}


/*
 * write the timestamp file for a database file. It uses the same name as
 * the database file (extension .db) but uses the extension .ts. Timestamps
 * are written as numbers of seconds since 1/1/1970 (UTC) to avoid timezone
 * and other interpretation problems, and because external synchronization
 * programs may have an easier time determining which of two competing cards
 * is newer (the one with the higher number). No human is expected to ever
 * look at these. This writes the mtime to later find the newest version,
 * and the ctime (create time) and ctimex (disambiguifier) to identify cards.
 */

static BOOL write_tfile(
	const DBASE		*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	int			nsect)		/* section to write */
{
	SECTION			*sect;		/* section to write */
	const char		*path;		/* file to write list to */
	static char		*pathbuf = 0;	/* file name with path */
	static size_t		pathlen;
	FILE			*fp;		/* open file */
	int			r;		/* row counter */
	const char		*p;		/* string copy pointer */

	sect = &dbase->sect[nsect];
	path = sect->path ? sect->path : form->dbase;
	if (!path || !*path)
		return(FALSE);
	path = resolve_tilde(path, 0);
	if (!(p = strrchr(path, '.')) || strcmp(p, ".db"))
		p = path + strlen(path);
	grow(0, "db file name", char, pathbuf, (int)(p - path) + 4, &pathlen);
	strcpy(pathbuf + (p - path), ".ts");
	path = pathbuf;
	if (!form->syncable) {
		unlink(path);
		return(TRUE);
	}
	if (!(fp = fopen(path, "w"))) {
		create_error_popup(mainwindow, errno,
			"Failed to create timestamp file %s", path);
		return(FALSE);
	}
	for (r=0; r < dbase->nrows; r++)
		if (nsect == dbase->row[r]->section)
			fprintf(fp, "%ld %ld %ld\n", (long)dbase->row[r]->mtime,
						     (long)dbase->row[r]->ctime,
						     (long)dbase->row[r]->ctimex);
	fclose(fp);
	return(TRUE);
}


/*
 * read one timestamp file for a database file that we have just successfully
 * read. It's fine if the file does not exist. Rely on the fact that the data
 * file has just been read, and can be referenced from dbase->currsect.
 */

static BOOL read_tfile(
	DBASE			*dbase,		/* form and items to write */
	const FORM		*form,		/* contains column delimiter */
	const char		*path)		/* file to read list from */
{
	static char		*pathbuf = 0;	/* file name with path */
	size_t			pathbuflen;
	SECTION			*sect;		/* new section */
	FILE			*fp;		/* open file */
	int			r = 0;		/* current row to change */
	char			*p;		/* temp pointer */
	char			line[128];	/* one timestamp per line */

	if (!form->syncable)
		return(TRUE);
	path = resolve_tilde(path, 0);
	grow(0, "db file name", char, pathbuf, strlen(path) + 4, &pathbuflen);
	strcpy(pathbuf, path);
	path = pathbuf;
	if (!(p = strrchr(pathbuf, '.')) || strcmp(p, ".db"))
		p = pathbuf + strlen(pathbuf);
	strcpy(p, ".ts");
	if (!(fp = fopen(path, "r")))
		return(TRUE);
	sect = &dbase->sect[dbase->currsect];
	for (r=0; r < dbase->nrows; r++)
		if (dbase->currsect == dbase->row[r]->section) {
			long mt, ct, ctx;
			if (!fgets(line, sizeof(line), fp))
				break;
			sscanf(line, "%ld %ld %ld", &mt, &ct, &ctx);
			dbase->row[r]->mtime = mt;
			dbase->row[r]->ctime = ct;
			dbase->row[r]->ctimex = ctx;
		}
	fgetc(fp);
	if (!feof(fp) || r != dbase->nrows) {
		create_error_popup(mainwindow, 0, "Length of timestamp file %s\n"
			"does not agree with its database file.\n", path);
		fclose(fp);
		for (r=0; r < dbase->nrows; r++)
			if (dbase->currsect == dbase->row[r]->section)
				dbase->row[r]->mtime = sect->mtime;
		return(FALSE);
	}
	fclose(fp);
	return(TRUE);
}

#endif /* GROK || PLANGROK */
