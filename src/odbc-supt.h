#ifndef _ODBC_SUPT_H
#define _ODBC_SUPT_H
/*
 * ODBC support routines and macros
 */
/* In order to make transferring this to other projects as easy as possible,
 * I don't include anything grok-related */
/* I also try to make things as generic as possible.  The only grok-ism
 * I am aware of is the list of supported column types (db_coltype) */
#include <sqlext.h>
#include <stdarg.h>

/* non-portable database features accessed using string substitutions */
typedef enum db_subst_code {
    DBS_INIT,         /* \n\n-separated initial SQL to run on open */
    DBS_CONCAT_AGGR,  /* aggr fun to concat non-null %1$s separated by , */
    DBS_CONCAT_AGGR2, /* aggr fun to concat non-null %1$s separated by %2$s */
    DBS_ID_TYPE,      /* type of ID fields */
    DBS_ID_CONSTR,    /* primary key constraint for ID fields */
    DBS_ID_CREATE,    /* create a sequence for table %1$s if necessary */
    DBS_ID_DROP,      /* drop a sequence for table %l$s after table dropped */
    DBS_NEXT_ID,      /* if non-blank, get next ID for table %1$s;
		       * else auto (UPDATE ... RETURNING id) */
    NUM_DBS
} db_subst_code;

/* internally, use db_subst_code as an array index to these: */
typedef struct db_subst {
    const char *pat; /* case-insensitive POSIX extended regex */
    const char *val; /* printf formatting string */
} db_subst;

/* user overrides store the code as well: */
typedef struct db_subst_ovr {
    db_subst_code which;
    char *pat;
    char *val;
} db_subst_ovr;

/* column types used by grok; CT_x maps to SQL_x */
enum db_coltype {
    CT_CONSTRAINT, // must be 0
    CT_BIT, CT_INTEGER, CT_VARCHAR, CT_DOUBLE, CT_CHAR, CT_LONGVARCHAR,
    NUM_CT
};

/* interesting info about the column type */
typedef struct otype_desc {
    char *type, *parms;
    int size, scale, searchable;
} otype_desc;

/* a database connection */
typedef struct db_conn {
    char *ident;		/* info gotten by SQLGetInfio(), set by db_open:
				 * SQL_DBMS_NAME[ SQL_DBMS_VER[ SQL_DM_VER
				 * [ SQL_DRIVER_NAME[ SQL_DRIVER_ODBC_VER
				 * [:SQL_DRIVER_VER]]]]]
				 */
    char *conn_str;		/* The connection string */
    SQLHDBC dbc;		/* DBC handle set by db_open */
    SQLHSTMT stmt;		/* STMT handle set by db_open */
    const char *subst[NUM_DBS];	/* selected substitution for this database */
    				/* filled in as needed */
    db_subst_ovr *subst_ovr;	/* user overrides; use db_parse_subst_ovr */
    int num_subst_ovr;		/* size of above array */
    otype_desc otype[NUM_CT];	/* column type defs; filled in as needed */
} db_conn;

/* Initialize ODBC support.  Call only once, before anything else */
int db_init(void);
/* Parse a substitution override file and append results to conn->subst_ovr */
/* first call should be with blank conn */
/* call this before db_open() in case DBS_INIT was overridden */
int db_parse_subst_ovr(const char *fname, db_conn *conn);
/* open database connection and execute DBS_INIT */
int db_open(const char *dbconnect, db_conn *conn);
/* close database connection and free/clear conn's data */
void db_close(db_conn *conn);

/* non-reentrant */
/* fill in conn->ctype[ct] and return its type field, or NULL on error */
/* prec is only useful on types with varying length */
const char *db_coldef_type(db_conn *conn, enum db_coltype ct, SQLSMALLINT prec);

/* perform substitution given by which */
char *do_db_subst(enum db_subst_code which, db_conn *conn, ...);
char *vdo_db_subst( enum db_subst_code which, db_conn *conn, va_list al);
/* perform SQL given by substitution which.  blank does nothing.
 * multiple statements are separated by a blank line */
/* if commit is true, commit after each successful statement */
typedef enum db_exec_subst_ret {
    EXS_RET_IGNORE_RES, /* neither print nor abort on errors */
    EXS_RET_PRINT_RES, /* print but don't abort on errors */
    EXS_RET_ABORT_ERR, /* print and abort on errors */
    EXS_RET_SELECT /* abort on errors; don't free last stmt if success */
} db_exec_subst_ret;
SQLRETURN db_exec_subst(db_subst_code which, db_conn *conn,
			db_exec_subst_ret rt, int commit, ...);
SQLRETURN vdb_exec_subst(db_subst_code which, db_conn *conn,
			 db_exec_subst_ret rt, int commit, va_list al);

#ifdef __cplusplus
#include <string>
#include <vector>
typedef std::vector<std::string> strlist;
/* get list of tables using SQLTables() */
strlist db_tables(db_conn *conn);
/* get list of columns for table using SQLColumns() */
strlist db_cols(db_conn *conn, const char *table);
#endif

/* These are mostly copied from my previous ODBC-using projects */

/* SQL_NO_DATA isn't really a failure, but it's not SQL_SUCCEEDED */
#define SQL_FAILED(rc) (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA)

/* macros for lazy me; assumes conn is the current connection */
#define db_stmt(s)  SQLExecDirect(conn->stmt, (SQLCHAR *)(s), SQL_NTS)
#define db_prep(s)  SQLPrepare(conn->stmt, (SQLCHAR *)(s), SQL_NTS)
#define db_exec()   SQLExecute(conn->stmt)
#define db_trans(t) SQLEndTran(SQL_HANDLE_DBC, conn->dbc, t)
#define db_commit() do { \
  if(!SQL_FAILED(ret)) \
    ret = db_trans(SQL_COMMIT); \
} while(0)
#define db_commit_keeperr() do { \
  if(SQL_FAILED(ret)) \
    db_trans(SQL_COMMIT); \
  else \
    db_commit(); \
} while(0)
#define db_rollback() db_trans(SQL_ROLLBACK)
#define db_next() do { \
    /* Firebird reqruires RESET_PARAMS or it gets screwy */ \
    /*   giving 07002 errors and such */ \
    SQLFreeStmt(conn->stmt, SQL_RESET_PARAMS); \
    SQLFreeStmt(conn->stmt, SQL_CLOSE); \
} while(0)
#define db_keepnext() SQLFreeStmt(conn->stmt, SQL_UNBIND);
/* I don't do output binding, so don't bother requiring related parms */
/* most drivers ignore sz/dig, but some may truncate input to sz/dig */
#define db_bindld(p, ct, st, sz, dig, v, lp) \
    SQLBindParameter(conn->stmt, p, SQL_PARAM_INPUT, ct, st, sz, dig, (void *)(v), 0, lp)
#define db_bind(p, ct, st, v, lp) \
    db_bindld(p, ct, st, 0, 0, v, lp)
#define db_bind64(p, i) \
    db_bind(p, SQL_C_SBIGINT, SQL_BIGINT, &i, NULL)
#define db_bindi(p, i) \
    db_bind(p, SQL_C_SLONG, SQL_BIGINT, &i, NULL)
#define db_bindf(p, f) \
    db_bindld(p, SQL_C_DOUBLE, SQL_DOUBLE, 15, 0, &f, NULL)
#define db_bindsl(p, s, l) \
    db_bindld(p, SQL_C_CHAR, SQL_VARCHAR, l, 0, s, &l)
extern SQLLEN db_nulllen;
#define db_bindnull(p, t) \
    db_bind(p, SQL_C_DEFAULT, t, NULL, &db_nulllen)
/* g++ -O2 gives a waring on the following saying strlen(s) may be null. */
/* in spite of the !(s) check.   Piece of crap. */
#define db_binds(p, s)     (!(s) ? db_bindnull(p, SQL_VARCHAR) : \
    db_bindld(p, SQL_C_CHAR, SQL_VARCHAR, strlen(s), 0, s, NULL))
extern SQLLEN db_len1;
#define db_bindc(p, c) \
    db_bindld(p, SQL_C_CHAR, SQL_CHAR, 1, 0, &c, &db_len1)
#define db_fetch() SQLFetch(conn->stmt)
#define db_col(...) SQLGetData(conn->stmt, __VA_ARGS__)
#define db_scol(n, buf, len, rlen) do { \
  ret = db_col(n, SQL_C_CHAR, buf, len, &rlen); \
  if(SQL_FAILED(ret) || rlen == SQL_NULL_DATA) \
    rlen = 0; \
} while(0)
#define db_icol(n, var) do { \
    SQLLEN rlen; \
    SQLBIGINT val; \
    ret = db_col(n, SQL_C_SBIGINT, &val, sizeof(val), &rlen); \
    if(SQL_FAILED(ret) || rlen == SQL_NULL_DATA) \
	val = 0; \
    var = val; \
} while(0)

/* ODBC has its own error reporting mechanism; gather message into a buffer */
char *odbc_msg(const db_conn *conn);
/* for even lazier me: use current conn and print error */
#define db_err() do { \
    char *__err = odbc_msg(conn); \
    fputs(__err, stderr); \
    putc('\n', stderr); \
    free(__err); \
} while(0)

/* for primary keys: get next ID to set for primary key field */
/* uses DBS_NEXT_ID(tab). */
/* If this is blank, returns 0; use INSERT non-id fields RETURNING id instead */
/* If there was an error, returns ~0UL. */
/* default, non-sequence is not multi-access safe, so at least */
/* do it in a transaction that includes a table lock */
/* versions which update are more safe; should auto-lock */
unsigned long db_get_pkey_id(db_conn *conn, const char *tab);

#endif
