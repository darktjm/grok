#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include "odbc-supt.h"

/* ODBC test/demo program -- print all supported data types (SQLGetTypeInfo) */
/* usage:  odbc-types "connection string" "connection string" ... */
/* output is sorted by 1st value of conn str (e.g. <val> in Driver=<val>;...
 * or DSN=<val>;... or Database=<va>... -- it doesn't care about the keyword)
 */
#ifndef AUX_DB_CONN
/* Uses an SQLite3 database to construct result, using the conn. str: */
#define AUX_DB_CONN "Driver=SQLite;Database=:memory:"
// #define AUX_DB_CONN "Driver=SQLite;Database=/tmp/types.db"
/* PostgreSQL could be used as well: */
// #define AUX_DB_CONN "Driver=PostgreSQL"
/* Or maybe even Firebird: */
// #define AUX_DB_CONN "Driver=Firebird;Database=localhost:/tmp/dummy.db;UID=dummyu;PWD=dummyp"
#endif

/* convert numeric DATA_TYPE/SQL_DATA_TYPE code */
static void dt_code(char *buf)
{
    switch(atoi(buf)) {
      case SQL_CHAR: strcpy(buf, "SQL_CHAR"); break;
      case SQL_VARCHAR: strcpy(buf, "SQL_VARCHAR"); break;
      case SQL_LONGVARCHAR: strcpy(buf, "SQL_LONGVARCHAR"); break;
      case SQL_WCHAR: strcpy(buf, "SQL_WCHAR"); break;
      case SQL_WVARCHAR: strcpy(buf, "SQL_WVARCHAR"); break;
      case SQL_WLONGVARCHAR: strcpy(buf, "SQL_WLONGVARCHAR"); break;
      case SQL_DECIMAL: strcpy(buf, "SQL_DECIMAL"); break;
      case SQL_NUMERIC: strcpy(buf, "SQL_NUMERIC"); break;
      case SQL_SMALLINT: strcpy(buf, "SQL_SMALLINT"); break;
      case SQL_INTEGER: strcpy(buf, "SQL_INTEGER"); break;
      case SQL_REAL: strcpy(buf, "SQL_REAL"); break;
      case SQL_FLOAT: strcpy(buf, "SQL_FLOAT"); break;
      case SQL_DOUBLE: strcpy(buf, "SQL_DOUBLE"); break;
      case SQL_BIT: strcpy(buf, "SQL_BIT"); break;
      case SQL_TINYINT: strcpy(buf, "SQL_TINYINT"); break;
      case SQL_BIGINT: strcpy(buf, "SQL_BIGINT"); break;
      case SQL_BINARY: strcpy(buf, "SQL_BINARY"); break;
      case SQL_VARBINARY: strcpy(buf, "SQL_VARBINARY"); break;
      case SQL_LONGVARBINARY: strcpy(buf, "SQL_LONGVARBINARY"); break;
	/* DATA_TYPE only (concise) */
      case SQL_TYPE_DATE: strcpy(buf, "SQL_TYPE_DATE"); break;
      case SQL_TYPE_TIME: strcpy(buf, "SQL_TYPE_TIME"); break;
      case SQL_TYPE_TIMESTAMP: strcpy(buf, "SQL_TYPE_TIMESTAMP"); break;
	/* my headers don't have: */
	/* SQL_TYPE_UTCDATETIME SQL_TYPE_UTCTIME */
      case SQL_INTERVAL_MONTH: strcpy(buf, "SQL_INTERVAL_MONTH"); break;
      case SQL_INTERVAL_YEAR: strcpy(buf, "SQL_INTERVAL_YEAR"); break;
      case SQL_INTERVAL_YEAR_TO_MONTH: strcpy(buf, "SQL_INTERVAL_YEAR_TO_MONTH"); break;
      case SQL_INTERVAL_DAY: strcpy(buf, "SQL_INTERVAL_DAY"); break;
      case SQL_INTERVAL_HOUR: strcpy(buf, "SQL_INTERVAL_HOUR"); break;
      case SQL_INTERVAL_MINUTE: strcpy(buf, "SQL_INTERVAL_MINUTE"); break;
      case SQL_INTERVAL_SECOND                : strcpy(buf, "SQL_INTERVAL_SECOND                "); break;
      case SQL_INTERVAL_DAY_TO_HOUR: strcpy(buf, "SQL_INTERVAL_DAY_TO_HOUR"); break;
      case SQL_INTERVAL_DAY_TO_MINUTE: strcpy(buf, "SQL_INTERVAL_DAY_TO_MINUTE"); break;
      case SQL_INTERVAL_DAY_TO_SECOND: strcpy(buf, "SQL_INTERVAL_DAY_TO_SECOND"); break;
      case SQL_INTERVAL_HOUR_TO_MINUTE: strcpy(buf, "SQL_INTERVAL_HOUR_TO_MINUTE"); break;
      case SQL_INTERVAL_HOUR_TO_SECOND: strcpy(buf, "SQL_INTERVAL_HOUR_TO_SECOND"); break;
      case SQL_INTERVAL_MINUTE_TO_SECOND: strcpy(buf, "SQL_INTERVAL_MINUTE_TO_SECOND"); break;
#if (ODBCVER >= 0x0350)
      case SQL_GUID: strcpy(buf, "SQL_GUID"); break;
#endif
	/* SQL_DATA_TYPE only (2-part w/ SQL_DATETIME_SUB) */
      case SQL_INTERVAL: strcpy(buf, "SQL_INTERVAL"); break;
      case SQL_DATETIME: strcpy(buf, "SQL_DATETIME"); break;
	/* obsolete type, I guess, from the headers */
      case SQL_TIMESTAMP: strcpy(buf, "SQL_TIMESTAMP"); break;
	/* probably never occurs */
      case SQL_UNKNOWN_TYPE: strcpy(buf, "SQL_UNKNOWN_TYPE"); break;
    }
}

/* convert numeric SQL_DATETIME_SUB code */
static void dt_subcode(char *buf, int type)
{
    switch(atoi(buf)) {
      case SQL_CODE_YEAR: strcpy(buf, type == SQL_INTERVAL ? "SQL_CODE_YEAR" : "SQL_CODE_DATE"); break;
      case SQL_CODE_MONTH: strcpy(buf, type == SQL_INTERVAL ? "SQL_CODE_MONTH" : "SQL_CODE_TIME"); break;
      case SQL_CODE_DAY: strcpy(buf, type == SQL_INTERVAL ? "SQL_CODE_DAY" : "SQL_CODE_TIMESTAMP"); break;
      case SQL_CODE_HOUR: strcpy(buf, "SQL_CODE_HOUR"); break;
      case SQL_CODE_MINUTE: strcpy(buf, "SQL_CODE_MINUTE"); break;
      case SQL_CODE_SECOND: strcpy(buf, "SQL_CODE_SECOND"); break;
      case SQL_CODE_YEAR_TO_MONTH: strcpy(buf, "SQL_CODE_YEAR_TO_MONTH"); break;
      case SQL_CODE_DAY_TO_HOUR: strcpy(buf, "SQL_CODE_DAY_TO_HOUR"); break;
      case SQL_CODE_DAY_TO_MINUTE: strcpy(buf, "SQL_CODE_DAY_TO_MINUTE"); break;
      case SQL_CODE_DAY_TO_SECOND: strcpy(buf, "SQL_CODE_DAY_TO_SECOND"); break;
      case SQL_CODE_HOUR_TO_MINUTE: strcpy(buf, "SQL_CODE_HOUR_TO_MINUTE"); break;
      case SQL_CODE_HOUR_TO_SECOND: strcpy(buf, "SQL_CODE_HOUR_TO_SECOND"); break;
      case SQL_CODE_MINUTE_TO_SECOND: strcpy(buf, "SQL_CODE_MINUTE_TO_SECOND"); break;
    }
}

/* convert numeric SEARCHABLE code */
static void srch(char *buf)
{
    switch(atoi(buf)) {
      case SQL_UNSEARCHABLE: strcpy(buf, "SQL_UNSEARHABLE"); break;
      case SQL_LIKE_ONLY: strcpy(buf, "SQL_LIKE_ONLY"); break;
      case SQL_ALL_EXCEPT_LIKE: strcpy(buf, "SQL_ALL_EXCEPT_LIKE"); break;
      case SQL_SEARCHABLE: strcpy(buf, "SQL_SEARCHABLE"); break;
    }
}

int main(int argc, char **argv)
{
    const char *aux_db = AUX_DB_CONN;
    db_conn mem_conn = {}, *conn = &mem_conn, db_conn = {};
    while(--argc && **++argv == '-') {
	switch((*argv)[1]) {
	  default:
	    usage:
		fputs("Usage: odbc-types [flags] connection-string [...]\n"
		      "flags:\n"
		      "\t-d s\tUse connection string s for storing all type info\n"
		      "\t\t\tFor example: " AUX_DB_CONN "\n"
		      "\t-s f\tPrepend database feature substitutionss from file f\n",
		      stderr);
	    exit(1);
	  case 'd':
	    if(argc < 2)
		goto usage;
	    aux_db = *++argv;
	    argc--;
	    break;
	  case 's':
	    if(argc < 2)
		goto usage;
	    db_parse_subst_ovr(*++argv, conn);
	    argc--;
	    break;
	}
    }
    if(argc < 1)
	return 1;
    if(!db_init())
	exit(1);
    if(!db_open(aux_db, conn))
	exit(1);
#define abort_err() do { \
    if(!SQL_SUCCEEDED(ret)) { \
	fprintf(stderr, "ret %d: ", ret); \
	db_err(); \
	exit(1); \
    } \
} while(0)
    /* IF EXISTS would avoid error, but only if db supports it */
    SQLRETURN ret = db_stmt("DROP TABLE types");
    // abort_err();
    db_next();
    const char *itype = db_coldef_type(conn, CT_INTEGER, 0);
    const char *stype = db_coldef_type(conn, CT_VARCHAR, 128);
    if(!stype)
	stype = db_coldef_type(conn, CT_LONGVARCHAR, 128);
    if(!itype || !stype) {
	fputs("Database not supported for result table\n", stderr);
	exit(1);
    }
    char *sltype = (char *)malloc(strlen(stype) + 6);
    if(!sltype) {
	perror("query");
	exit(1);
    }
    strcpy(sltype, stype);
    char *pp = strchr(sltype, '(');
    if(pp++) {
	char *ep = strchr(pp, ')');
	if(!ep) {
	    fprintf(stderr, "varchar type %s is invalid", stype);
	    exit(1);
	}
	memmove(pp + 3, ep, strlen(ep));
	memcpy(pp, "128", 3);
    } else
	strcat(sltype, "(128)");
#define T_INTEGER "%1$s"
#define T_VARCHAR "%2$s"
    const char *ct = "CREATE TABLE types ("
	"DB " T_VARCHAR ","
	"TYPE_NAME " T_VARCHAR ","
	"DATA_TYPE " T_VARCHAR ","
	"COLUMN_SIZE " T_INTEGER ","
	"LITERAL_PREFIX " T_VARCHAR ","
	"LITERAL_SUFFIX " T_VARCHAR ","
	"CREATE_PARAMS " T_VARCHAR ","
	"NULLABLE " T_INTEGER ","
	"CASE_SENSITIVE " T_INTEGER ","
	"SEARCHABLE " T_VARCHAR ","
	"UNSIGNED_ATTRIBUTE " T_INTEGER ","
	"FIXED_PREC_SCALE " T_INTEGER ","
	"AUTO_UNIQUE_VALUE " T_INTEGER ","
	"LOCAL_TYPE_NAME " T_VARCHAR ","
	"MINIMUM_SCALE " T_INTEGER ","
	"MAXIMUM_SCALE " T_INTEGER ","
	"SQL_DATA_TYPE " T_VARCHAR ","
	"SQL_DATETIME_SUB " T_VARCHAR ","
	"NUM_PREC_RADIX " T_INTEGER ","
	"INTERVAL_PRECISION " T_INTEGER ")";
    int qlen = snprintf(NULL, 0, ct, itype, sltype);
    char *dq = (char *)malloc(qlen + 1);
    if(!dq) {
	perror("query");
	exit(1);
    }
    qlen = snprintf(dq, qlen + 1, ct, itype, sltype);
    free(sltype);
//    fprintf(stderr, "mktab: %s\n", dq);
    ret = db_stmt(dq);
    abort_err();
    db_next();
    db_commit();
    ret = db_prep("INSERT INTO types (DB, TYPE_NAME, DATA_TYPE, COLUMN_SIZE,"
		  "LITERAL_PREFIX, LITERAL_SUFFIX, CREATE_PARAMS, NULLABLE,"
		  "CASE_SENSITIVE, SEARCHABLE, UNSIGNED_ATTRIBUTE, FIXED_PREC_SCALE,"
		  "AUTO_UNIQUE_VALUE, LOCAL_TYPE_NAME, MINIMUM_SCALE,"
		  "MAXIMUM_SCALE, SQL_DATA_TYPE, SQL_DATETIME_SUB, NUM_PREC_RADIX,"
		  "INTERVAL_PRECISION) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
		  "?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    abort_err();
    SQLLEN slen;
    char *dst;
    static char buf[1024];
    for(int i = 0; i < argc; i++) {
	/* not really safe to copy these, as they are freed by db_close() */
	/* so remember to zero out before db_close(). */
	db_conn.subst_ovr = mem_conn.subst_ovr;
	db_conn.num_subst_ovr = mem_conn.num_subst_ovr;
	if(!db_open(argv[i], &db_conn))
	    exit(1);
	char *s = strchr(argv[i], '=');
	if(s)
	    ++s;
	else
	    s = argv[i];
	char *e = strchr(s, ';');
	if(e)
	    *e = 0;
	ret = db_binds(1, s);
	abort_err();
	conn = &db_conn;
	ret = SQLGetTypeInfo(conn->stmt, SQL_ALL_TYPES);
	if(SQL_SUCCEEDED(ret))
	    while(1) {
		ret = db_fetch();
		if(!SQL_SUCCEEDED(ret))
		    break;
#define getval(n) do { \
    db_scol(n, dst, sizeof(buf) - (dst - buf), slen); \
    if(SQL_FAILED(ret)) \
	break; \
    dst[slen] = 0; \
} while(0)
		dst = buf;
		int dt_type = 0;
		for(int i = 1; i <= 19; i++) {
		    getval(i);
		    if(i == 2 /* DATA_TYPE */ || i == 16 /* SQL_DATA_TYPE */ ) {
			dt_type = atoi(dst);
			dt_code(dst);
		    } else if(i == 17 /* SQL_DATETIME_SUB */ )
			    dt_subcode(dst, dt_type);
		    else if(i == 9 /* SEARCHABLE */ )
			srch(dst);
		    slen += strlen(dst + slen);
		    conn = &mem_conn;
		    /* Postgres can't convert empty strings to int, so using
		     * db_binds for all columns doesn't work */
#if 0
		    /* solution 1:  use SQLDescribeParam to pick int or str */
		    /* issues:
		     *    - postgres doesn't retain SQL_INTEGER for int fields
		     *      solution:  check for string types instead
		     *    - sqlite always returns -1 (SQL_LONGVARCHAR)
		     *      ok, because I check for that as well
		     */
		    SQLSMALLINT type;
		    ret = SQLDescribeParam(conn->stmt, i + 1, &type, NULL, NULL, NULL);
		    abort_err();
//		    fprintf(stderr, "%d %d\n", i + 1, type);
		    if(type == SQL_VARCHAR || type == SQL_LONGVARCHAR) {
			ret = db_binds(i + 1, dst);
			dst += slen + 1;
		    } else {
			ret = db_bindi(i + 1, ints[nint]);
			ints[nint++] = atol(dst);
		    }
#else
		    /* solution 2:   bind empty strings to NULL */
		    /* GCC is so stupid sometimes */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
		    ret = db_binds(i + 1, *dst ? dst : NULL);
#pragma GCC diagnostic pop
		    dst += slen + 1;
#endif
		    abort_err();
		    conn = &db_conn;
		}
		conn = &mem_conn;
		ret = db_exec();
		abort_err();
		db_commit();
		db_keepnext();
		conn = &db_conn;
	    }
	if(ret == SQL_NO_DATA)
	    ret = SQL_SUCCESS;
	abort_err();
	db_conn.subst_ovr = NULL;
	db_conn.num_subst_ovr = 0;
	db_close(&db_conn);
	conn = &mem_conn;
	abort_err();
    }
    db_next();
    std::string q("SELECT DATA_TYPE, DB,");
    char *aggr;
#define do_aggr(s) do { \
    aggr = do_db_subst(DBS_CONCAT_AGGR, conn, s); \
    q += aggr; \
    free(aggr); \
} while(0)
    do_aggr("distinct TYPE_NAME ||"
		      "REPLACE('('||CREATE_PARAMS||')','()','')");
    q += ',';
    do_aggr("distinct COLUMN_SIZE||''");
    q += ',';
    do_aggr("distinct MAXIMUM_SCALE||''");
    q += ',';
    do_aggr("distinct REPLACE('\"'||LITERAL_PREFIX||'\"','\"\"','')");
    q += ',';
    do_aggr("distinct REPLACE('\"'||LITERAL_SUFFIX||'\"','\"\"','')");
    q += ',';
    do_aggr("distinct REPLACE(SEARCHABLE,'SQL_','')");
    q += " FROM types GROUP BY DATA_TYPE, DB ORDER BY DATA_TYPE, DB";
#define ncol 8
    dst = buf;
    int len[ncol] = {};
    const char * const lab[ncol] =
    { "ODBC Data Type", "DB", "Native Type", "Max size", "Max Scale",
	"Lit Pre", "Lit Suf", "Searchable" };
    for(int i = 0; i < ncol; i++)
	len[i] = strlen(lab[i]);
//    fprintf(stderr, "%s\n", q.c_str());
    ret = db_stmt(q.c_str());
    if(SQL_SUCCEEDED(ret))
	while(1) {
	    ret = db_fetch();
	    if(!SQL_SUCCEEDED(ret))
		break;
	    for(int i = 1; i <= ncol; i++) {
		getval(i);
		if(slen > len[i - 1])
		    len[i - 1] = slen;
	    }
	}
    if(ret != SQL_NO_DATA && !SQL_SUCCEEDED(ret))
	db_err();
    else
	db_next();
    for(int i = 0; i < ncol - 1; i++) {
	if(i)
	    putchar(' ');
	printf("%-*s", len[i], lab[i]);
    }
    printf(" %s\n", lab[ncol - 1]);
    ret = db_stmt(q.c_str());
    if(SQL_SUCCEEDED(ret))
	while(1) {
	    ret = db_fetch();
	    if(!SQL_SUCCEEDED(ret))
		break;
	    for(int i = 1; i <= ncol; i++) {
		getval(i);
		if(i > 1)
		    putchar(' ');
		if(isdigit(*buf) || *buf == '-')
		    printf("%*s", len[i - 1], buf);
		else if(i == ncol)
		    fputs(buf, stdout);
		else
		    printf("%-*s", len[i - 1], buf);
	    }
	    putchar('\n');
	}
    if(ret != SQL_NO_DATA && !SQL_SUCCEEDED(ret))
	db_err();
    else
	db_next();
    db_close(conn);
}
