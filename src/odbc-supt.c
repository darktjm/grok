/*
 * ODBC support routines.  See odbc-supt.h.
 */

/* In order to make transferring this to other projects as easy as possible,
 * I don't include anything grok-related */
/* I also try to make things as generic as possible.  The only grok-ism
 * I am aware of is the list of supported column types (db_coltype) */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <regex.h>
#include "odbc-supt.h"

/* These are mostly copied from my previous ODBC-using projects */

SQLLEN db_nulllen = SQL_NULL_DATA;
SQLLEN db_len1 = 1;

static SQLHENV henv = SQL_NULL_HENV;

/* ODBC has its own error reporting mechanism; gather message into a buffer */
char *odbc_msg(const db_conn *conn)
{
    SQLSMALLINT htype;
    SQLHANDLE h;
    SQLRETURN ret = SQL_SUCCESS;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    static SQLCHAR state[7];
    SQLINTEGER rc;
    static SQLCHAR msg[1024];
    SQLSMALLINT len;
    int i;
    char *emsg = NULL, *nmsg;

    pthread_mutex_lock(&lock); // for static buffers
    htype = !conn || !conn->dbc ? SQL_HANDLE_ENV : conn->stmt ? SQL_HANDLE_STMT : SQL_HANDLE_DBC;
    while(1) {
	for(i = 1; SQL_SUCCEEDED(ret); i++) {
	    h = htype == SQL_HANDLE_ENV ? henv : htype == SQL_HANDLE_DBC ? conn->dbc : conn->stmt;
	    ret = SQLGetDiagRec(htype, h, i, state, &rc, msg, sizeof(msg), &len);
	    /* sqliteodbc+unixODBC does not return a messaage */
	    /* so fall back to SQLError(), which does */
	    if(!SQL_SUCCEEDED(ret) && !emsg)
		ret = SQLError(htype == SQL_HANDLE_ENV ? henv : 0,
			       htype == SQL_HANDLE_DBC ? conn->dbc : 0,
			       htype == SQL_HANDLE_STMT ? conn->stmt : 0,
			       state, &rc, msg, sizeof(msg), &len);
	    if(SQL_SUCCEEDED(ret)) {
		rc = emsg ? asprintf(&nmsg, "%s\nODBC[%d/%s]: %s", emsg, rc, state, msg)
		    : asprintf(&nmsg, "ODBC[%d/%s]: %s", rc, state, msg);
		if(rc < 0)
		    exit(1);
		if(emsg)
		    free(emsg);
		emsg = nmsg;
	    }
	    /* some versions of unixODBC used to crash with sqliteodbc after the
	     * first iteration.  Not sure what version(s), so abort for all */
// #ifdef SQL_ATTR_UNIXODBC_VERSION
//	    break;
// #endif
	}
	if(htype == SQL_HANDLE_STMT)
	    htype = SQL_HANDLE_DBC;
	else if(htype == SQL_HANDLE_DBC)
	    htype = SQL_HANDLE_ENV;
	else
	    break;
    }
    pthread_mutex_unlock(&lock);
    if(!emsg)
	emsg = strdup("Unknown ODBC error");
    return emsg;
}

int db_init()
{
    SQLRETURN ret;
    /* why did I do this? */
    ret = SQLSetEnvAttr(SQL_NULL_HANDLE, SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)SQL_CP_ONE_PER_DRIVER, 0);
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if(SQL_SUCCEEDED(ret))
	ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
			    (SQLPOINTER)SQL_OV_ODBC3, 0);
    if(!SQL_SUCCEEDED(ret)) {
	char *msg = odbc_msg(NULL);
	if(!msg)
	    msg = strdup("Unkown ODBC error");
	fputs(msg, stderr);
	free(msg);
	return 0;
    }
    return 1;
}

int db_open(const char *dbconnect, db_conn *conn)
{
    SQLRETURN ret;
    /* new connection: create dbc & stmt handles */
    if(!(conn->conn_str = strdup(dbconnect))) {
	perror("db ident");
	exit(1);
    }
    ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn->dbc);
    if(SQL_SUCCEEDED(ret))
	/* FIXME:  Prompt for missing info */
	/* Could prompt for user name/password on errors:
	 * 28000 - invalid user name or password
	 * 08004 - rejected connection (assuming auth failure)
	 *         this is also returned for e.g. missing/invalid Database=
	 */
	ret = SQLDriverConnect(conn->dbc, 0, (SQLCHAR *)dbconnect, SQL_NTS, NULL,
			       0, NULL, SQL_DRIVER_NOPROMPT);
    if(SQL_SUCCEEDED(ret))
	/* use explicit commit */
	ret = SQLSetConnectAttr(conn->dbc, SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_OFF,
				SQL_IS_INTEGER);
#if 0
    if(SQL_SUCCEEDED(ret))
	ret = SQLSetConnectAttr(conn->dbc, SQL_ATTR_TRACE,
				(SQLPOINTER)SQL_OPT_TRACE_ON,
				SQL_IS_INTEGER);
#endif
    if(SQL_SUCCEEDED(ret))
	ret = SQLAllocHandle(SQL_HANDLE_STMT, conn->dbc, &conn->stmt);
    if(!SQL_SUCCEEDED(ret)) {
	db_err();
	free(conn->conn_str);
	conn->conn_str = NULL;
	return 0;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    struct { SQLSMALLINT type, olen; } nameinfo[] = {
	{ SQL_DBMS_NAME }, { SQL_DBMS_VER }, { SQL_DM_VER },
	{ SQL_DRIVER_NAME }, { SQL_DRIVER_ODBC_VER }, { SQL_DRIVER_VER }
    };
#pragma GCC diagnostic pop
#define NUM_NAMEINFO (sizeof(nameinfo)/sizeof(nameinfo[0]))
    /* Note:  this code doesn't work on my system w/ iODBC-3.52.15 and
     *        firebird's ODBC driver 2.0.5.156.  First, it crashes w/
     *        NULL/0 as the output buffer, and if that's fixed, it chops
     *        the output len in half (and iODBC then truncates to the
     *        lower len).  In short, don't use this combination, since
     *        I'm not going to work around that mess any more.  */
    int len = 0;
    for(size_t i = 0; i < NUM_NAMEINFO; i++) {
	ret = SQLGetInfo(conn->dbc, nameinfo[i].type, NULL, 0, &nameinfo[i].olen);
	if(!SQL_SUCCEEDED(ret))
	    break;
	len += nameinfo[i].olen;
	if(i)
	    ++len;
    }
    if(len) {
	SQLSMALLINT olen, off = 0;
	if((conn->ident = (char *)malloc(len + 1))) {
	    for(size_t i = 0; i < NUM_NAMEINFO; i++) {
		if(i)
		    conn->ident[off++] = ' ';
		ret = SQLGetInfo(conn->dbc, nameinfo[i].type,
				 conn->ident + off, len + 1 - off, &olen);
		/* if(olen != nameinfo[i].olen) fatal error; */
		if(!SQL_SUCCEEDED(ret))
		    break;
		off += olen;
	    }
	    /* valgrind gives a hit on dmbs_name, so manually terminate */
	    /* The ref man says return is 0-terminated, so this is a bug */
	    /* in unixODBC or one of the ODBC drivers, most likely */
	    conn->ident[off] = 0;
	}
    } else
	conn->ident = strdup("Unknown");
    if(!conn->ident) {
	perror("db ident");
	exit(1);
    }
    /* ignore errors for now, but print them to stderr */
    ret = db_exec_subst(DBS_INIT, conn, EXS_RET_PRINT_RES, 1);
    if(SQL_FAILED(ret)) {
	db_close(conn);
	return 0;
    }
    return 1;
}

void db_close(db_conn *conn)
{
    /* close db: free dbc & stmt handles (keep env) */
    if(conn->ident)
	free(conn->ident);
    if(conn->conn_str)
	free(conn->conn_str);
    if(conn->stmt) {
	SQLCancel(conn->stmt);
	db_next();
	SQLFreeHandle(SQL_HANDLE_STMT, conn->stmt);
    }
    if(conn->dbc) {
	db_rollback();
	SQLFreeHandle(SQL_HANDLE_DBC, conn->dbc);
    }
    for(int i = 0; i < NUM_CT; i++) {
	if(conn->otype[i].type)
	    free(conn->otype[i].type);
	if(conn->otype[i].parms)
	    free(conn->otype[i].parms);
    }
    for(int i = 0; i < conn->num_subst_ovr; i++) {
	if(conn->subst_ovr[i].pat)
	    free(conn->subst_ovr[i].pat);
	if(conn->subst_ovr[i].val)
	    free(conn->subst_ovr[i].val);
    }
    if(conn->subst_ovr)
	free(conn->subst_ovr);
    memset(conn, 0, sizeof(*conn));
}

const char *db_coldef_type(db_conn *conn, enum db_coltype ct, SQLSMALLINT prec)
{
    static char obuf[128]; /* Firebird's UNICODE_LONGVARCHR is 44 chars long! */
    /* [..] = is c99, but c++ lags behind */
    /* so keep this in order and expllicitly add [0] */
    static const SQLSMALLINT odbc_type[NUM_CT] = {
	0, /* [CT_BIT] = */ SQL_BIT, /* [CT_INTEGER] = */ SQL_INTEGER,
	/* [CT_VARCHAR] = */ SQL_VARCHAR, /* [CT_DOUBLE] = */ SQL_DOUBLE,
	/* [CT_CHAR] = */ SQL_CHAR, /* [CT_LONGVARCHAR] = */ SQL_LONGVARCHAR };
    SQLRETURN ret;
    if(!conn->otype[ct].type) {
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	SQLLEN rlen;
	pthread_mutex_lock(&lock);
	ret = SQLGetTypeInfo(conn->stmt, odbc_type[ct]);
	if(!SQL_FAILED(ret))
	    ret = db_fetch();
	if(SQL_FAILED(ret))
	    goto err;
	if(ret == SQL_NO_DATA) {
	    db_next();
	    fprintf(stderr, "Invalid database; does not support type %d\n", odbc_type[ct]);
	    goto err;
	}
	db_icol(3, conn->otype[ct].size); // COLUMN_SIZE
	if(!SQL_FAILED(ret))
	    db_icol(18, conn->otype[ct].scale); // NUM_PREC_RADIX
	if(!SQL_FAILED(ret))
	    db_icol(9, conn->otype[ct].searchable); // SEARCHABLE
	if(!SQL_FAILED(ret))
	    db_scol(1, obuf, sizeof(obuf), rlen); // TYPE_NAME
	if(!SQL_FAILED(ret)) {
	    obuf[rlen] = 0;
	    if(!(conn->otype[ct].type = strdup(obuf))) {
		perror("type name");
		goto err;
	    }
	    db_scol(6, obuf, sizeof(obuf), rlen); // CREATE_PARAMS
	}
	if(!SQL_FAILED(ret) && rlen) {
	    obuf[rlen] = 0;
	    if(!(conn->otype[ct].parms = strdup(obuf))) {
		perror("type name");
		free(conn->otype[ct].type);
		conn->otype[ct].type = NULL;
		goto err;
	    }
	}
	if(SQL_FAILED(ret)) {
err:
	    pthread_mutex_unlock(&lock);
	    db_next();
	    return NULL;
	}
	pthread_mutex_unlock(&lock);
	db_next();
    }
    if(prec && conn->otype[ct].size < prec)
	return NULL;
    return conn->otype[ct].type;
}

static void set_db_subst(db_conn *conn, enum db_subst_code which);

unsigned long db_get_pkey_id(db_conn *conn, const char *tab)
{
    set_db_subst(conn, DBS_NEXT_ID);
    if(!conn->subst[DBS_NEXT_ID] || !conn->subst[DBS_NEXT_ID][0])
	return 0;
    SQLRETURN ret = db_exec_subst(DBS_NEXT_ID, conn, EXS_RET_SELECT, 0, tab);
    if(SQL_FAILED(ret))
	return ~0UL;
    ret = db_fetch();
    SQLUINTEGER id;
    if(SQL_SUCCEEDED(ret))
	db_icol(1, id);
    if(!SQL_SUCCEEDED(ret)) {
	db_err();
	db_next();
	return ~0UL;
    }
    db_next();
    return id;
}

/* pat == NULL -> default/end of list */
static const db_subst *const default_subst[NUM_DBS] = {
    /* DBS_INIT */
    (const db_subst[]) {
	{ "sqlite", "PRAGMA foreign_keys = true" },
	{ "mariadb|mysql", "SET sql_mode='ansi'" },
	{ NULL, "" }
    },
    /* DBS_CONCAT_AGGR */
    (const db_subst[]) {
	{ "postgres", "string_agg(%1$s,',')" },
	{ "firebird", "list(%1$s)" },
	{ NULL, "group_concat(%1$s)" }
    },
    /* DBS_CONCAT_AGGR2 */
    (const db_subst[]) {
	{ "postgres", "string_agg(%1$s,'%2$s')" },
	{ "firebird", "list(%1$s,'%2$s')" },
	{ "mariadb|mysql", "group_concat(%1$s separator '%2$s')" },
	{ NULL, "group_concat(%1$s,'%2$s')" }
    },
    /* DBS_ID_TYPE */
    (const db_subst[]) {
	{ NULL, "INTEGER" }
    },
    /* DBS_ID_CONSTR */
    (const db_subst[]) {
	// { "sqlite", "PRIMARY KEY AUTOINCREMENT" },
	{ "mariadb|mysql", "PRIMARY KEY AUTO_INCREMENT" },
	{ "postgres|firebird", "GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY" },
	{ NULL, "PRIMARY KEY" }
    },
    /* DBS_ID_CREATE */
    (const db_subst[]) {
	{ "postgres|firebird|mariadb|mysql", "" },
	{ NULL, "CREATE TABLE pkey_sequence ("
			"id INTEGER NOT NULL,"
			"name VARCHAR(128) UNIQUE NOT NULL)\n"
		"\n"
		"INSERT INTO pkey_sequence (id, name) VALUES (0, '%1$s')" }
    },
    /* DBS_ID_DROP */
    (const db_subst[]) {
	{ "postgres|firebird|mariadb|mysql", "" },
	{ NULL, "DELETE FROM pkey_sequence WHERE name = '%1$s'" }
    },
    /* DBS_NEXT_ID */
    (const db_subst[]) {
	{ "postgres|firebird|mariadb|mysql", "" },
	{ NULL, "UPDATE pkey_sequence SET id = id + 1 WHERE name = '%1$s'\n"
		"\n"
		"SELECT id FROM pkey_sequence WHERE name = '%1$s'" }
    }
};

static int set_subst(db_conn *conn, enum db_subst_code which,
		      const char *pat, const char *val)
{
    regex_t re;
    int cret;
    if(!val)
	val = "";
    if(!pat) {
	conn->subst[which] = val;
	return 1;
    }
    if((cret = regcomp(&re, pat, REG_EXTENDED | REG_ICASE | REG_NOSUB))) {
	char buf[100];
	regerror(cret, &re, buf, sizeof(buf));
	fputs(buf, stderr);
	regfree(&re);
	return 0;
    }
    if(!regexec(&re, conn->ident, 0, NULL, 0) ||
       !regexec(&re, conn->conn_str, 0, NULL, 0)) {
	regfree(&re);
	conn->subst[which] = val;
	return 1;
    }
    regfree(&re);
    return 0;
}

static void set_db_subst(db_conn *conn, enum db_subst_code which)
{
    if(conn->subst[which])
	return;
    /* pass 1: explicit patterns */
    for(int i = 0; i < conn->num_subst_ovr; i++)
	if(conn->subst_ovr[i].which == which && conn->subst_ovr[i].pat &&
	   set_subst(conn, which, conn->subst_ovr[i].pat, conn->subst_ovr[i].val))
	    return;
    for(const db_subst *sub = default_subst[which]; sub->pat; sub++)
	if(set_subst(conn, which, sub->pat, sub->val))
	    return;
    /* pass 2: defaults */
    for(int i = 0; i < conn->num_subst_ovr; i++)
	if(conn->subst_ovr[i].which == which && !conn->subst_ovr[i].pat) {
	    set_subst(conn, which, NULL, conn->subst_ovr[i].val);
	    return;
	}
    for(const db_subst *sub = default_subst[which]; ; sub++)
	if(!sub->pat) {
	    set_subst(conn, which, NULL, sub->val);
	    return;
	}
}

char *vdo_db_subst( enum db_subst_code which, db_conn *conn, va_list al)
{
    char *ret;
    set_db_subst(conn, which);
    if(!conn->subst[which] || !conn->subst[which][0])
	return NULL;
    int pret = vasprintf(&ret, conn->subst[which], al);
    if(pret < 0) {
	perror("subst");
	exit(1);
    }
    return ret;
}

char *do_db_subst(enum db_subst_code which, db_conn *conn, ...)
{
    va_list al;
    va_start(al, conn);
    char *ret = vdo_db_subst(which, conn, al);
    va_end(al);
    return ret;
}

SQLRETURN vdb_exec_subst(db_subst_code which, db_conn *conn,
			 db_exec_subst_ret rt, int commit, va_list al)
{
    char *sql = vdo_db_subst(which, conn, al);
    SQLRETURN ret = SQL_SUCCESS;
    if(sql) {
	char *s = sql, *e, *ee;
	while(1) {
	    ee = s;
	    while((ee = e = strchr(ee, '\n'))) {
		while(*++ee != '\n' && isspace(*ee));
		if(*ee++ == '\n')
		    break;
	    }
	    if(e)
		*e = 0;
	    ret = db_stmt(s);
	    if(SQL_FAILED(ret)) {
		if(rt == EXS_RET_PRINT_RES || rt == EXS_RET_ABORT_ERR)
		    db_err();
		db_next();
		if(rt == EXS_RET_ABORT_ERR || rt == EXS_RET_SELECT)
		    return ret;
	    } else if(commit)
		db_commit();
	    if(e || rt != EXS_RET_SELECT)
		db_next();
	    if(!e)
		break;
	    s = ee;
	}
	free(sql);
    }
    return ret;
}

SQLRETURN db_exec_subst(db_subst_code which, db_conn *conn,
			db_exec_subst_ret rt, int commit, ...)
{
    va_list al;
    va_start(al, commit);
    SQLRETURN ret = vdb_exec_subst(which, conn, rt, commit, al);
    va_end(al);
    return ret;
}

const struct dbs_keyent {
    const char *name;
    enum db_subst_code code;
} db_ovr_lookup[] = {
    { "CONCAT_AGGR", DBS_CONCAT_AGGR },
    { "CONCAT_AGGR2", DBS_CONCAT_AGGR2 },
    { "ID_CONSTR", DBS_ID_CONSTR },
    { "ID_CREATE", DBS_ID_CREATE },
    { "ID_DROP", DBS_ID_DROP }, 
    { "ID_TYPE", DBS_ID_TYPE },
    { "INIT", DBS_INIT },
    { "NEXT_ID", DBS_NEXT_ID }
};
static int subst_srch_cmp(const void *_a, const void *_b)
{
    /* a is always key, and b a pointer to base[n] */
    return strcasecmp((char *)_a, ((struct dbs_keyent *)_b)->name);
}

/* libc really needs this.  I have to reimplement it all the time */
static int fgets_full(char **buf, size_t off, size_t *bufsz, FILE *f)
{
    size_t olen = 0;
    if(!*buf) {
	*bufsz = 128;
	while(*bufsz <= off)
	    *bufsz *= 2;
	if(!(*buf = (char *)malloc(*bufsz))) {
	    perror("read buf");
	    exit(1);
	}
    }
    while(1) {
	if(!fgets(*buf + off, *bufsz - olen - off, f))
	    return !!olen;
	olen += strlen(*buf + olen + off);
	if(!olen || (*buf)[olen + off - 1] == '\n')
	    return 1;
	if(!(*buf = (char *)realloc(*buf, (*bufsz *= 2)))) {
	    perror("read buf");
	    exit(1);
	}
    }
}

static int read_qword(FILE *f, char **buf, size_t *bufsz, size_t off)
{
    size_t d;
    char q = (*buf)[off];
    /* word may be quoted by '..' or ".." */
    if(q != '\'' && q != '"')
	q = 0;
    else
	off++;
    for(d = off; (*buf)[off]; off++) {
	char c = (*buf)[off];
	if(q) {
	    if(c == q) {
		/* doubling the quote is a quote; otherwise end-of-word */
		if((*buf)[++off] != q)
		    break; /* probably ought to flag !isspace(nxt) as err */
	    } else if(c == '\n')
		fgets_full(buf, off + 1, bufsz, f);
	    (*buf)[d++] = c;
	} else if(c == '\\') { /* if not quoted, \ escapes next */
	    if(isspace((c = (*buf)[++off]))) {
		/* a \ at eol (or at last visible location on line) adds \n
		 * and skips to first non-space on next line */
		int ep;
		for(ep = off + 1; isspace((*buf)[ep]); ep++);
		if((*buf)[ep - 1] == '\n') {
		    fgets_full(buf, off, bufsz, f);
		    while(isspace((*buf)[off]))
			off++;
		    off--;
		    (*buf)[d++] = '\n';
		} else
		    (*buf)[d++] = c;
	    }
	} else if(isspace(c))
	    break;
	else
	    (*buf)[d++] = c;
    }
    /* move off to beginning of next word */
    while(isspace((*buf)[off]))
	off++;
    (*buf)[d] = 0;
    return off;
}

int db_parse_subst_ovr(const char *fname, db_conn *conn)
{
    FILE *f = fopen(fname, "r");
    if(!f) {
	perror(fname);
	return 0;
    }
    char *buf = NULL;
    size_t bufsz;
    while(fgets_full(&buf, 0, &bufsz, f)) {
	char *s, *e, *p;
	for(s = buf; *s && isspace(*s); s++);
	if(!*s)
	    continue; /* skip blank lines */
	e = s + strlen(s);
	if(*s == '#') /* skip comments */
	    continue;
	/* match keyword */
	for(p = s; *p && !isspace(*p); p++);
	if(*p) {
	    *p++ = 0;
	    while(*p && isspace(*p))
		p++;
	}
	if(!*p || *p == '#') {
	    if(e[-1] == '\n')
		e[-1] = 0;
	    fprintf(stderr, "%s: %s: missing regex, subst\n", fname, s);
	    free(buf);
	    fclose(f);
	    return 0;
	}
	const struct dbs_keyent *key =
	    (struct dbs_keyent *)bsearch(s, db_ovr_lookup, NUM_DBS,
					 sizeof(*db_ovr_lookup),
					 subst_srch_cmp);
	if(!key) {
	    fprintf(stderr, "%s: %s: unkown key\n", fname, s);
	    free(buf);
	    fclose(f);
	    return 0;
	}
	int re = p - buf, sub = read_qword(f, &buf, &bufsz, re);
	if(buf[sub] && buf[sub] != '#') {
	    int eoff = read_qword(f, &buf, &bufsz, sub);
	    if(buf[eoff] && buf[eoff] != '#') {
		fprintf(stderr, "%s: extra text at end of line (%s)\n", fname,
			buf + eoff);
		free(buf);
		fclose(f);
		return 0;
	    }
	} else {
	    /* swap */
	    sub ^= re;
	    re ^= sub;
	    sub ^= re;
	}
	if(buf[re] == '"' || buf[re] == '\'')
	    re++;
	if(buf[sub] == '"' || buf[sub] == '\'')
	    sub++;
	db_subst_ovr t = {};
	t.which = key->code;
	if(buf[re]) {
	    if(!(t.pat = strdup(buf + re))) {
		perror("subst ovr");
		exit(1);
	    }
	}
	if(buf[sub]) {
	    if(!(t.val = strdup(buf + sub))) {
		perror("subst ovr");
		exit(1);
	    }
	}
	/* yep, one at a time.  very inefficient.  */
	if(!conn->subst_ovr) {
	    conn->num_subst_ovr = 1;
	    conn->subst_ovr = (db_subst_ovr *)malloc(sizeof(db_subst_ovr));
	} else
	    conn->subst_ovr =
		(db_subst_ovr *)realloc(conn->subst_ovr,
					++conn->num_subst_ovr * sizeof(db_subst_ovr));
	if(!conn->subst_ovr) {
	    perror("subst ovr");
	    exit(1);
	}
	(conn->subst_ovr)[conn->num_subst_ovr - 1] = t;
    }
    free(buf);
    fclose(f);
    return 1;
}

#ifdef __cplusplus
strlist db_tables(db_conn *conn)
{
    strlist tabs;
    char buf[32];

    /* FIXME:  not sure how this behaves with catalog/schema */
    SQLRETURN ret = SQLTables(conn->stmt, NULL, 0, NULL, 0, (SQLCHAR *)"%", 1,
			      (SQLCHAR *)"TABLE", 5);
    if(SQL_SUCCEEDED(ret))
	while(1) {
	    SQLLEN rlen;
	    ret = db_fetch();
	    if(!SQL_SUCCEEDED(ret))
		break;
	    db_scol(3, buf, sizeof(buf), rlen);
	    if(!SQL_SUCCEEDED(ret))
		break;
	    buf[rlen] = 0;
	    tabs.push_back(buf);
	}
    db_next();
    return tabs;
}

strlist db_cols(db_conn *conn, const char *table)
{
    strlist cols;
    char buf[32];

    /* FIXME:  not sure how this behaves with catalog/schema */
    SQLRETURN ret = SQLColumns(conn->stmt, NULL, 0, NULL, 0,
			       (SQLCHAR *)table, SQL_NTS, (SQLCHAR *)"%", 1);
    if(SQL_SUCCEEDED(ret))
	while(1) {
	    SQLLEN rlen;
	    ret = db_fetch();
	    if(!SQL_SUCCEEDED(ret))
		break;
	    db_scol(4, buf, sizeof(buf), rlen);
	    if(!SQL_SUCCEEDED(ret))
		break;
	    buf[rlen] = 0;
	    cols.push_back(buf);
	}
    db_next();
    return cols;
}
#endif
