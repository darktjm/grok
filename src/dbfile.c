/*
 * read and write database files.
 *
 *	write_dbase(path)		write dbase
 *	read_dbase(form, force)		read dbase into empty dbase struct
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
#include "grok.h"
#include "form.h"
#include "proto.h"

/* prior versions did not disallow these as the field delimiter
 * but it's pretty clear they won't work.  ESC always escapes the
 * next character as data, and R_SEP always delimits rows */
#define R_SEP	'\n'			/* row (card) separator */
#define ESC	'\\'			/* treat next char literally */


/*
 * This used to read both path.db and path, in that order.
 * Now it only reads the first one it finds
 * If it can't find either, path is set to .db for non-proc and plain for proc
 */

const char *db_path(
	const FORM	*form)
{
	static char		*pathbuf = 0;	/* file name with path */
	static size_t		pathbuflen;
	const char		*path = form->dbase;

	if (*path != '/' && *path != '~' && form->dir) {
		int dlen = strlen(form->dir), plen = strlen(path);
		grow(0, "db path", char, pathbuf, plen + dlen + 2, &pathbuflen);
		memcpy(pathbuf, form->dir, dlen);
		pathbuf[dlen] = '/';
		memcpy(pathbuf + dlen + 1, path, plen + 1);
		path = pathbuf;
	}
	path = canonicalize(resolve_tilde(path, 0), false);
	grow(0, "db path", char, pathbuf, strlen(path) + 4, &pathbuflen);
	sprintf(pathbuf, "%s.db", path);
	for(DBASE *dbase = dbase_list; dbase; dbase = dbase->next)
		if(!strcmp(dbase->path, pathbuf))
			return pathbuf;
	/* FIXME: change F_OK to R_OK? */
	if (!access(pathbuf, F_OK))
		/* even if there is nothing in this file, use it */
		return pathbuf;
	*strrchr(pathbuf, '.') = 0;
	for(DBASE *dbase = dbase_list; dbase; dbase = dbase->next)
		if(!strcmp(dbase->path, pathbuf))
			return pathbuf;
	if (!access(pathbuf, F_OK) || form->proc)
		return pathbuf;
	strcat(pathbuf, ".db");
	return pathbuf;
}


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
 * Returns false if the file could not be written.
 */

static bool write_file (DBASE *, int);
static bool write_tfile(const DBASE *, int);

bool write_dbase(
	DBASE			*dbase,		/* form and items to write */
	bool			force)		/* write even if not modified*/
{
	int			s;		/* section index */
	bool			ok = true;

	if (dbase->nsects == 1 && (dbase->modified || force)) {
		ok  = write_file (dbase, 0);
		ok &= write_tfile(dbase, 0);
	} else
		for (s=0; s < dbase->nsects; s++)
			if (dbase->sect[s].modified &&
					(!dbase->sect[s].rdonly || force)) {
				ok &= write_file (dbase, s);
				ok &= write_tfile(dbase, s);
			}
	dbase_delete(NULL);
	return(ok);
}


static bool write_file(
	DBASE			*dbase,		/* form and items to write */
	int			nsect)		/* section to write */
{
	SECTION			*sect;		/* section to write */
	const char		*path;		/* file to write list to */
	FILE			*fp;		/* open file */
	int			r, c;		/* row and column counters */
	int			hicol;		/* # of columns of card -1 */
	char			*value;		/* converted data to write */
	char			*p;		/* string copy pointer */

	sect = &dbase->sect[nsect];
	path = sect->path ? sect->path : dbase->path;
	if (dbase->form->proc) {
		char *cmd = alloc(mainwindow, "procedural command", char,
				  strlen(path) + strlen(dbase->form->name) + 5);
		if(!cmd)
			return(false);
		sprintf(cmd, "%s -w %s", path, dbase->form->name);
		if (!(fp = popen(cmd, "w"))) {
			create_error_popup(mainwindow, errno,
				"Failed to run procedural command %s", cmd);
			free(cmd);
			return(false);
		}
		free(cmd);
	} else
		if (!(fp = fopen(path, "w"))) {
			create_error_popup(mainwindow, errno,
				"Failed to create database file %s", path);
			return(false);
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
				for (p=value; *p; p++) {
					if (*p == R_SEP ||
					    *p == dbase->form->cdelim ||
					    *p == ESC)
						fputc(ESC, fp);
					fputc(*p, fp);
				}
			}
			if (c < hicol)
				fputc(dbase->form->cdelim, fp);
		}
		fputc(R_SEP, fp);
	}
	if (dbase->form->proc) {
		if (pclose(fp)) {
			create_error_popup(mainwindow, errno,
				"%s:\nfailed to create database", path);
			return(false);
		}
	} else
		fclose(fp);
	if (sect)
		sect->modified = false;
	dbase->modified = false;
	sect->mtime = time(0);
	return(true);
}


/*
 * Read a database from a file. The database must be empty (just created
 * with dbase_create()). Returns false if the file could not be read.
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

static bool read_dir_or_file (DBASE *, const char *);
static bool read_file	     (DBASE *, const char *, time_t);
static bool read_tfile	     (DBASE *, const char *);

DBASE *read_dbase(
	const FORM		*form,		/* contains column delimiter */
	bool			force)		/* revert if already loaded */
{
	DBASE			*dbase;
	int			nread = 0;	/* # of successful reads */
	DBASE			*forced;
	const char		*path = db_path(form);

	for(dbase = dbase_list; dbase; dbase = dbase->next)
		if(!strcmp(dbase->path, path) &&
		   dbase->form->proc == form->proc &&
		   (!form->proc || !strcmp(form->name, dbase->form->name))) {
			if(form->cdelim != dbase->form->cdelim ||
			   form->syncable != dbase->form->syncable ||
			   strcmp(form->proc ? form->name : "",
				  dbase->form->proc ? dbase->form->name : "")) {
				bool reject = true;
				if(force) {
					const FORM *f;
					for(f = form_list; f; f = f->next)
						if(!strcmp(f->dbpath, dbase->path))
							break;
					if(f)
						for( ; f; f = f->next)
							if(!strcmp(f->dbpath, dbase->path))
								break;
					if(!f) {
						dbase->modified = true;
						reject = false;
					}
				}
				if(reject)
					/* FIXME:  reject more gracefully */
					/* If possible, purge database instead */
					fatal("Database loaded with incompatible definitions");
			}
			DBASE *next;
			/* if you want to reload an unmodified db */
			/* you'll have to switch and then purge */
			if(!force || !dbase->modified)
				return dbase;
			next = dbase->next;
			dbase_clear(dbase);
			tzero(DBASE, dbase, 1);
			dbase->next = next;
			dbase->form = form;
			dbase->path = mystrdup(path);
			break;
		}
	forced = dbase;
	if (!dbase)
		dbase = dbase_create(form);
	if (!access(dbase->path, F_OK))
		nread += read_dir_or_file(dbase, dbase->path);
	/* formerly done after load in dbase_switch(), */
	/* but should be everwhere */
	dbase->modified = false;
	if (!forced) {
		dbase->next = dbase_list;
		dbase_list = dbase;
	}
	return dbase;
}


/*
 * read path. If it's a directory, recurse. Return false if nothing was found.
 */

#define MAXSC  1000		/* no more than 200 sections */

static int compare_name(
	const void	*u,
	const void	*v)
{
	return(strcmp(*(char **)u, *(char **)v));
}

static bool read_dir_or_file(
	DBASE			*dbase,		/* form and items to write */
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
		return(false);
	if (!(statbuf.st_mode & S_IFDIR))
		return(read_file (dbase, path, statbuf.st_mtime) &&
		       read_tfile(dbase, path));
	if (!(dir = opendir(path)))
		return(false);
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
		nfiles += read_dir_or_file(dbase, pathbuf);
	}
	dbase->havesects = true;
	return(nfiles > 0);
}


/*
 * read one plain file and append to dbase section list. Return false on error.
 */

static bool read_file(
	DBASE			*dbase,		/* form and items to write */
	const char		*path,		/* file to read list from */
	time_t			mtime)		/* file modification time */
{
	SECTION			*sect;		/* new section */
	FILE			*fp;		/* open file */
	int			nc = 0;		/* size of curr line */
	int			col = 0;	/* current column being added*/
	int			row = -1;	/* current row being added */
	char			*buf;		/* buffer for one item */
	int			bindex = 0;	/* next free byte in buf */
	int			bsize;		/* size of buf in bytes */
	unsigned char		c, c0;		/* next char from file */
	bool			error;		/* in true, abort */
	int			i;

							/* step 1: open file */
	signal(SIGPIPE, broken_pipe_handler);
	if (dbase->form->proc) {
		char *cmd = alloc(mainwindow, "procedural command", char,
				  strlen(path) + strlen(dbase->form->name) + 5);
		if(!cmd)
			return(false);
		sprintf(cmd, "%s -r %s", path, dbase->form->name);
		if (!(fp = popen(cmd, "r"))) {
			create_error_popup(mainwindow, errno,
				"Failed to read procedural database %s", path);
			return(false);
		}
	} else {
		if (!(fp = fopen(path, "r")))
			return(false);
	}
							/* step 2: new sectn */
	i = (dbase->nsects+1) * sizeof(SECTION);
	if (!(sect = (SECTION *)(dbase->sect ? realloc(dbase->sect, i) : malloc(i)))) {
		create_error_popup(mainwindow, errno,
			"No memory for section %s", path);
		dbase->form->proc ? pclose(fp) : fclose(fp);
		return(false);
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
		if (feof(fp) || c0 == dbase->form->cdelim || c0 == R_SEP) {
			if (!nc) {
				if (c0 == dbase->form->cdelim) col++;
				if (feof(fp))	break;
				else		continue;
			}
			buf[bindex] = 0;			/* next col */
								/* .. date? */
			/* The old code converted the date to a number for
			 * internal storage, and then back to a string in
			 * write_dbase(), both using the current date format
			 * regardless of what the format was when it was
			 * written.  I prefer to keep internal and
			 * external representation the same, though.
			 * This now combines the two, so that if the format
			 * of the field changed, it is picked up.  The new
			 * format will not be written until the databse
			 * is saved.  This is an accurate emulation of the
			 * old behavior. */
			const char *val = buf;
			if(*buf) {
				const FORM *form = dbase->form;
				for (i = 0; i < form->nitems; i++)
					if(form->items[i]->type == IT_TIME &&
					   form->items[i]->column == col) {
						TIMEFMT fmt = form->items[i]->timefmt;
						time_t time = parse_time_data(buf, fmt);
						val = format_time_data(time, fmt);
						break;
					}
			}
								/* store str */
			if (row < 0)
				if ((error |= !dbase_addrow(&row, dbase)))
					break;
			dbase_put(dbase, row, col++, val);
			dbase->row[row]->mtime  = 0;
			dbase->row[row]->ctime  = 0;
			dbase->row[row]->ctimex = dbase->ctimex_next++;
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
				if ((error |= !newb))
					break;
				buf = newb;
			}
			buf[bindex++] = c;
			nc++;
		}
	}
								/* done. */
	zfree(buf);
	error |= dbase->form->proc ? !!pclose(fp) : !!fclose(fp);
	sect->modified = false;
	sect->rdonly   =
	dbase->rdonly  = dbase->form->proc ? false : !!access(path, W_OK);
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

static bool write_tfile(
	const DBASE		*dbase,		/* form and items to write */
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
	path = sect->path ? sect->path : dbase->path;
	if (!(p = strrchr(path, '.')) || strcmp(p, ".db"))
		p = path + strlen(path);
	grow(0, "db file name", char, pathbuf, (int)(p - path) + 4, &pathlen);
	strcpy(pathbuf + (p - path), ".ts");
	path = pathbuf;
	if (!dbase->form->syncable) {
		unlink(path);
		return(true);
	}
	if (!(fp = fopen(path, "w"))) {
		create_error_popup(mainwindow, errno,
			"Failed to create timestamp file %s", path);
		return(false);
	}
	for (r=0; r < dbase->nrows; r++)
		if (nsect == dbase->row[r]->section)
			fprintf(fp, "%ld %ld %ld\n", (long)dbase->row[r]->mtime,
						     (long)dbase->row[r]->ctime,
						     (long)dbase->row[r]->ctimex);
	fclose(fp);
	return(true);
}


/*
 * read one timestamp file for a database file that we have just successfully
 * read. It's fine if the file does not exist. Rely on the fact that the data
 * file has just been read, and can be referenced from dbase->currsect.
 */

static bool read_tfile(
	DBASE			*dbase,		/* form and items to write */
	const char		*path)		/* file to read list from */
{
	static char		*pathbuf = 0;	/* file name with path */
	static size_t		pathbuflen;
	SECTION			*sect;		/* new section */
	FILE			*fp;		/* open file */
	int			r = 0;		/* current row to change */
	char			*p;		/* temp pointer */
	char			line[128];	/* one timestamp per line */

	if (!dbase->form->syncable)
		return(true);
	path = resolve_tilde(path, 0);
	grow(0, "db file name", char, pathbuf, strlen(path) + 4, &pathbuflen);
	strcpy(pathbuf, path);
	path = pathbuf;
	if (!(p = strrchr(pathbuf, '.')) || strcmp(p, ".db"))
		p = pathbuf + strlen(pathbuf);
	strcpy(p, ".ts");
	if (!(fp = fopen(path, "r")))
		return(true);
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
			if(dbase->ctimex_next <= ctx)
				dbase->ctimex_next = ctx + 1;
		}
	fgetc(fp);
	if (!feof(fp) || r != dbase->nrows) {
		create_error_popup(mainwindow, 0, "Length of timestamp file %s\n"
			"does not agree with its database file.\n", path);
		fclose(fp);
		for (r=0; r < dbase->nrows; r++)
			if (dbase->currsect == dbase->row[r]->section)
				dbase->row[r]->mtime = sect->mtime;
		return(false);
	}
	fclose(fp);
	return(true);
}
