/*
 *  read and write form definition from ODBC database.
 *
 *   drop_grok_tabs(db_conn)	Drop all grok form definiton tables and data
 *   create_grok_tabs(db_conn)	Create/adjust all grok from def tables
 *   sql_write_form(conn, form)	Save a form to the database
 *   sql_read_form(conn, name)	Load a form from database
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <QtWidgets>
#include "config.h"
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "odbc-supt.h"
#include <boost/preprocessor.hpp>

// BOOST_PP has LIST_FOR_EACH and SEQ_FOR_EACH, but not TUPLE_FOR_EACH
// This version takes user data as extra parms and passes them down
// Not sure it this will work for everyone...  I just stabbed in the dark.
#define _TUPLE_FOR1_EXEC(f, z, n, v, a) f(z, n, v)
#define _TUPLE_FOR1_EXECM(f, z, n, v, a) f(z, n, v, a)
#define _TUPLE_REST(t, ...) (__VA_ARGS__)
#define _TUPLE_FOR1(z, n, opt) \
    BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(BOOST_PP_SEQ_ELEM(1, opt)), 1), \
	_TUPLE_FOR1_EXECM, _TUPLE_FOR1_EXEC) \
		(BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(1, opt)), z, n, \
		 BOOST_PP_TUPLE_ELEM(n, BOOST_PP_SEQ_ELEM(0, opt)), \
		 BOOST_PP_REMOVE_PARENS(_TUPLE_REST BOOST_PP_SEQ_ELEM(1, opt)))
#define BOOST_PP_TUPLE_FOR_EACH(t, /* macro, */ ...) \
    BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(t), _TUPLE_FOR1, (t)((__VA_ARGS__)))
////////////////////////////////////////////////////////////////


/* each table is a macro w/ name of table defining a tuple of column defs */
/* each column def is a tuple of:
 *    name, (type[, prec[, constr]]), bind/col suffix[, (ivar[, icond[, pref]])
 *                                                   [, (ovar[, val[, pref]])]]
 *  type, ivar, ovar tuples may omit parens if just one
 *  input bind may be null if (!(ivar icond)); missing icond is blank
 *  output value from db is v; value set to var is val (v if missing)
 *  ivar/ovar pref overrides table's var name prefix; mainly to blank it.
 *  missing ivar uses same var name as column name
 *  missing ovar uses same var/prefix as ivar and val v
 */
/* notes:
 *   id field type & constraint are automatic; parms beyond name ignored
 *   BIT is automatically NOT NULL
 *   postgres advertises a max VARCHAR of 255, so use that instead of 256
 */
/* the reason I'm using macro magic here insted of hand-coded tables like I
 * used to (see grok_db_def below) is that everything is in one place, so
 * I won't have to deal with counting columns or binding them in the correct
 * order or adjusting column names in multiple places, etc.  If this could
 * all be done using string values, I'd do it,  but I have to make code
 * changes for binding/retreiving, so macros are the best way.  Luckily,
 * boost has a comprehensive preprocessor hack library.
 * There are some major disadvantages:  Hard to deal with compiler error
 * messages, even with better modern support for macros.  Hard to debug, since
 * gdb doesn't support macros very well.  Hard to deal with preprocessor
 * error messages.  Takes for bloody ever to preprocess.  One tool I use
 * to help with error messages is to look at the preprocessor output:
 * g++ -xc++ -Wp,-P -fPIC -E formodbc.c \
 *    -I/usr/include/qt5/{,QtWidgets} 2>/tmp/x | sed 's/" "//g' | \
 *    clang-format --style=GNU | sed 's/R "/R"/' >/tmp/x.cpp
 * (clang++ works as well; I'm just used to using gcc)
 * (clang-format is the only c++ formatter I know of; GNU indent can'do c++)
 * (-fPIC is to avoid generating an error w/ Qt includes)
 */

#define FKEY_FORM_REF "NOT NULL REFERENCES grok_form_def(\"id\") ON DELETE CASCADE"
#define grok_form_def ( \
    ( id,		INTEGER, 64, (fid,,) ), \
    ( name,		(VARCHAR, 128, "NOT NULL UNIQUE"), sn ), \
    ( dbase,		(VARCHAR, 128, "NOT NULL"), sn ), \
    ( comment,		(VARCHAR, 1024), s ), \
    ( cdelim,		(CHAR, 1), c, (cdelim, || 1) ), \
    ( adelim,		(CHAR, 1), c, (asep, && f->asep != '|') ), \
    ( aesc,		(CHAR, 1), c, (aesc, && f->aesc != '\\') ), \
    ( sumheight,	INTEGER, i ), \
    ( rdonly,		BIT, b ), \
    ( proc,		BIT, b ), \
    ( syncable,		BIT, b ), \
    ( gridx,		INTEGER, i, (xg, != 4) ), \
    ( gridy,		INTEGER, i, (yg, != 4) ), \
    ( sizex,		INTEGER, i, (xs, != 400) ), \
    ( sizey,		INTEGER, i, (ys, != 200) ), \
    ( divider,		INTEGER, i, ydiv ), \
    ( autoq,		INTEGER, i, (autoquery, != -1) ), \
    ( planquery,	(VARCHAR, 255), s ), \
    ( help,		LONGVARCHAR, s ) \
)

#define grok_form_def_child ( \
    ( form_id,		(INTEGER, 0, FKEY_FORM_REF), 64, (fid,,) ), \
    ( name,		(VARCHAR, 128), sn, /* [prefix] */ ) \
)

#define grok_form_def_query ( \
    ( seq,		INTEGER, in, (j,,) ), \
    ( form_id,		(INTEGER, 0, FKEY_FORM_REF), 64, (fid,,) ), \
    ( suspended,	BIT, b ), \
    ( name,		(VARCHAR, 128), s), \
    ( query,		(VARCHAR, 1024), s) \
)

#define concat_dbstr(z, n, v) \
    BOOST_PP_STRINGIZE(BOOST_PP_COMMA_IF(n)) \
    "'" BOOST_PP_STRINGIZE(v) "'"
#define check_constr(c, vals) "NOT NULL CHECK (\"" #c "\" IN (" \
				BOOST_PP_TUPLE_FOR_EACH(vals, concat_dbstr) "))"
#define string_list(z, n, v) \
    BOOST_PP_COMMA_IF(n) BOOST_PP_STRINGIZE(v)
#define check_xlate_tab(chk) \
    static const char * const xlate_##chk[] = { \
	BOOST_PP_TUPLE_FOR_EACH(check_##chk, string_list) \
    }; \
 \
    static int chk##_to_code(const char *v) \
    { \
	for(size_t i = 0; i < sizeof(xlate_##chk)/sizeof(xlate_##chk[0]); i++) \
	    if(!strcmp(v, xlate_##chk[i])) \
		return i; \
	return -1; \
    }

#define check_item_type ( \
    None, Label, Print, Input, Time, Note, Choice, \
    Flag, Button, Chart, Number, Menu, Radio, Multi, \
    Flags, Reference, Referers)
check_xlate_tab(item_type);
#define check_just ( Left, Center, Right)
check_xlate_tab(just);
#define check_font ( Helv, HelvO, HelvN, HelvB, Courier )
check_xlate_tab(font);
#define check_timefmt ( Date, Time, DateTime, Duration )
check_xlate_tab(timefmt);
#define check_dcombo ( None, Query, All )
check_xlate_tab(dcombo);

#define FKEY_FORM_ITEM_REF \
		"NOT NULL REFERENCES grok_form_item(\"id\") ON DELETE CASCADE"
#define grok_form_item ( \
    ( id,	INTEGER, 64, (iid,,) ), \
    ( seq,	INTEGER, in, (j,,) ), \
    ( form_id,	(INTEGER, 0, FKEY_FORM_REF), 64, (fid,,) ), \
    ( type,	(VARCHAR, 10, check_constr(type, check_item_type)), sn, \
				(xlate_item_type[item.type],,), \
				(type, (ITYPE)item_type_to_code(v)) ), \
    ( name,	(VARCHAR, 128), s ), \
    ( column,	INTEGER, in ), \
    ( label,	(VARCHAR, 128), s ), \
    ( ljust,	(VARCHAR, 10, check_constr(ljust, check_just)), sn, \
				(xlate_just[item.labeljust],,), \
				(labeljust, (JUST)just_to_code(v)) ), \
    ( lfont,	(VARCHAR, 10, check_constr(lfont, check_font)), sn, \
				(xlate_font[item.labelfont],,), \
				(labelfont, font_to_code(v)) ), \
    ( ijust,	(VARCHAR, 10, check_constr(ljust, check_just)), sn, \
				(xlate_just[item.inputjust],,), \
				(inputjust, (JUST)just_to_code(v)) ), \
    ( ifont,	(VARCHAR, 10, check_constr(lfont, check_font)), sn, \
				(xlate_font[item.inputfont],,), \
				(inputfont, font_to_code(v)) ), \
    ( posx,	INTEGER, in, x ), \
    ( posy,	INTEGER, in, y ), \
    ( sizex,	INTEGER, in, xs ), \
    ( sizey,	INTEGER, in, ys ), \
    ( midx,	INTEGER, in, xm ), \
    ( midy,	INTEGER, in, ym ), \
    ( maxlen,	INTEGER, i, (maxlen, != 100) ), \
    ( default,	(VARCHAR, 255), s, idefault ), \
    ( search,	BIT, ib, (SEARCH,,) ), \
    ( rdonly,	BIT, ib, (RDONLY,,) ), \
    ( nosort,	BIT, ib, (NOSORT,,) ), \
    ( defsort,	BIT, ib, (DEFSORT,,) ), \
    ( sumwid,	INTEGER, i, sumwidth ), \
    ( sumcol,	INTEGER, i ), \
    ( sumprint,	(VARCHAR, 255), s ), \
    /*  float only */ \
    ( range_min,	DOUBLE, f, (min, && item.type == IT_NUMBER) ), \
    ( range_max,	DOUBLE, f, (max, && item.type == IT_NUMBER) ), \
    ( digits,		INTEGER, i,  (digits, && item.type == IT_NUMBER) ), \
    /* date only */ \
    ( timefmt,		(VARCHAR, 10, check_constr(timefmt, check_timefmt)), sn, \
				(xlate_timefmt[item.timefmt],,), \
				(timefmt, (TIMEFMT)timefmt_to_code(v)) ), \
    ( datewidget,	BIT, b, (item.timewidget & 1,,), \
				(timewidget,item.timewidget|(v?0:1)) ), \
    ( cal,		BIT, b, (item.timewidget & 2,,), \
				(timewidget,item.timewidget|(v?0:2)) ), \
    /* flag/choice only */ \
    ( code,	(VARCHAR, 32), s, flagcode ), \
    ( codetxt,	(VARCHAR, 128), s, flagtext ), \
    /* button only */ \
    ( p_act,	(VARCHAR, 255), s, pressed ), \
    /* condition exprs */ \
    ( gray,	(VARCHAR, 255), s, gray_if ), \
    ( freeze,	(VARCHAR, 255), s, freeze_if ), \
    ( invis,	(VARCHAR, 255), s, invisible_if ), \
    ( skip,	(VARCHAR, 255), s, skip_if ), \
    /* menu */ \
    ( mcol,	BIT, ib, (MULTICOL,,) ), \
    ( dcombo,	(VARCHAR, 5, check_constr(dcombo, check_dcombo)), sn, \
				(xlate_dcombo[item.dcombo],,), \
				(dcombo, (DCOMBO)dcombo_to_code(v)) ), \
    /* plan */ \
    ( plan_if,	(CHAR, 1), c ), \
    /* fkey */ \
    ( fk_form,		(VARCHAR, 128), s, fkey_form_name ), \
    ( fk_header,	BIT, ib, (FKEY_HEADER,,) ), \
    ( fk_search,	BIT, ib, (FKEY_SEARCH,,) ), \
    ( fk_multi,		BIT, ib, (FKEY_MULTI,,) ), \
    /* too much chart crap; moved to own table */ \
    /* constraints */ \
    ( UNIQUE("form_id", "seq") ) \
)

#define grok_form_item_menu ( \
    ( item_id,	(INTEGER, 0, FKEY_FORM_ITEM_REF), 64, (iid,,) ), \
    ( seq,	INTEGER, in, (k,,) ), \
    ( label,	(VARCHAR, 128, "NOT NULL"), sn ), \
    ( code,	(VARCHAR, 32), s, flagcode ), \
    ( codetxt,	(VARCHAR, 128), s, flagtext ), \
    ( name,	(VARCHAR, 128), s ), \
    ( column,	INTEGER, i ), \
    ( sumcol,	INTEGER, i ), \
    ( sumwid,	INTEGER, i, sumwidth ), \
    /* constraints */ \
    ( UNIQUE("item_id", "seq") ) \
)

#define grok_form_item_fkey ( \
    ( item_id,	(INTEGER, 0, FKEY_FORM_ITEM_REF), 64, (iid,,) ), \
    ( seq,	INTEGER, in, (k,,) ), \
    ( name,	(VARCHAR, 128, "NOT NULL"), sn ), \
    ( key,	BIT, b ), \
    ( disp,	BIT, b, display ), \
    /* constraints */ \
    ( UNIQUE("item_id", "seq") ) \
)

#define FKEY_FORM_ITEM_CHART_REF \
		"NOT NULL REFERENCES grok_form_item_chart(\"item_id\") ON DELETE CASCADE"
#define grok_form_item_chart ( \
    /* only one per item, so item_id is sufficent */ \
    /* ( id,	INTEGER, 64, (cid,,) ), */ \
    ( item_id,		(INTEGER, 0, "PRIMARY KEY " FKEY_FORM_ITEM_REF), 64, (iid,,) ), \
    ( xrange_min,	DOUBLE, f, ch_xmin ), \
    ( xrange_max,	DOUBLE, f, ch_xmax ), \
    ( yrange_min,	DOUBLE, f, ch_ymin ), \
    ( yrange_max,	DOUBLE, f, ch_ymax ), \
    ( autox,		BIT, ib, (CH_XAUTO,,) ), \
    ( autoy,		BIT, ib, (CH_YAUTO,,) ), \
    ( gridx,		DOUBLE, f, ch_xgrid ), \
    ( gridy,		DOUBLE, f, ch_ygrid ), \
    ( snapx,		DOUBLE, f, ch_xsnap ), \
    ( snapy,		DOUBLE, f, ch_ysnap ) \
)

#define check_chart_comp_mode ( Next, Same, Expr, Drag )
check_xlate_tab(chart_comp_mode);

#define grok_form_item_chart_comp ( \
    ( chart_id,	(INTEGER, 0, FKEY_FORM_ITEM_CHART_REF), 64, (cid,,) ), \
    ( seq,	INTEGER, in, (k,,) ), \
    ( line,	BIT, b ),  /* was "type" -- makes no sense */ \
    ( xfat,	BIT, b ), \
    ( yfat,	BIT, b ), \
    ( excl,	(VARCHAR, 255), s, excl_if ), \
    ( color,	(VARCHAR, 128), s ), \
    ( label,	(VARCHAR, 128), s ), \
    ( modex,	(VARCHAR, 10, check_constr(modex, check_chart_comp_mode)), sn, \
				(xlate_chart_comp_mode[c.value[0].mode],,), \
				(value[0].mode, chart_comp_mode_to_code(v)) ), \
    ( exprx,	(VARCHAR, 255), s, value[0].expr ), \
    ( fieldx,	INTEGER, i, value[0].field ), \
    ( mulx,	DOUBLE, f, value[0].mul ), \
    ( addx,	DOUBLE, f, value[0].add ), \
    ( modey,	(VARCHAR, 10, check_constr(modey, check_chart_comp_mode)), sn, \
				(xlate_chart_comp_mode[c.value[1].mode],,), \
				(value[1].mode, chart_comp_mode_to_code(v)) ), \
    ( expry,	(VARCHAR, 255), s, value[1].expr ), \
    ( fieldy,	INTEGER, i, value[1].field ), \
    ( muly,	DOUBLE, f, value[1].mul ), \
    ( addy,	DOUBLE, f, value[1].add ), \
    ( sizex,	(VARCHAR, 255), s, value[2].expr ), \
    ( sizey,	(VARCHAR, 255), s, value[3].expr ), \
    /* constraints */ \
    ( UNIQUE("chart_id", "seq") ) \
)

struct coldef {
    const char *name;
    db_coltype ct;
    SQLSMALLINT prec;
    const char *constr;
};

#define _type_ct(x) BOOST_PP_TUPLE_REPLACE(x, 0, BOOST_PP_CAT(CT_, BOOST_PP_TUPLE_ELEM(0, x)))
#define TAB_COL_DEF(z, n, c) \
    { BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, c)) \
	BOOST_PP_COMMA_IF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(c), 1)) \
	BOOST_PP_REMOVE_PARENS( \
	    BOOST_PP_EXPR_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(c), 1), \
		_type_ct((BOOST_PP_REMOVE_PARENS(BOOST_PP_TUPLE_ELEM(1, c)))))) \
	},

#define TAB_DEF(t) \
    { #t, (const coldef[]) { \
	BOOST_PP_TUPLE_FOR_EACH(t, TAB_COL_DEF) \
	{} \
    }}

/* order matters:  created in forward order, deleted in reverse order */
struct tabdef {
    const char *name;
    const coldef *cols;
} grok_db_def[] = {
    TAB_DEF(grok_form_def),
    TAB_DEF(grok_form_def_child),
    TAB_DEF(grok_form_def_query),
    TAB_DEF(grok_form_item),
    TAB_DEF(grok_form_item_menu),
    TAB_DEF(grok_form_item_fkey),
    TAB_DEF(grok_form_item_chart),
    TAB_DEF(grok_form_item_chart_comp)
};
#define N_GROK_TABS (sizeof(grok_db_def)/sizeof(*grok_db_def))

void drop_grok_tabs(db_conn *conn)
{
    for(int i = N_GROK_TABS - 1; i >= 0; i--) {
	std::string dt = std::string("DROP TABLE "/*"IF EXISTS "*/) + grok_db_def[i].name;
	SQLRETURN ret = db_stmt(dt.c_str());
	if(SQL_FAILED(ret))
	    db_err(); // print, but ignore error
	db_next();
	db_exec_subst(DBS_ID_DROP, conn, EXS_RET_PRINT_RES, 1, grok_db_def[i].name);
    }
    db_trans(SQL_COMMIT);
}

static void append_coldef(db_conn *conn, std::string &st, const char *tab,
			  const coldef *cd)
{
    if(cd->ct)
	st += '"';
    st += cd->name;
    if(cd->ct)
	st += '"';
    if(!strcmp(cd->name, "id")) {
	db_exec_subst(DBS_ID_CREATE, conn, EXS_RET_PRINT_RES, 1, tab);
	st += ' ';
	st += do_db_subst(DBS_ID_TYPE, conn);
	st += ' ';
	st += do_db_subst(DBS_ID_CONSTR, conn, tab);
    } else if(cd->ct) {
	enum db_coltype ct = cd->ct;
	const char *t = db_coldef_type(conn, ct, cd->prec);
	if(!t && !conn->otype[ct].type) {
	    fprintf(stderr, "Can't get info on type %d\n", ct);
	    exit(1);
	}
	if(!t) {
	    fprintf(stderr, "Column too big for db: %s.%s (%d/%d)\n",
		    tab, cd->name, conn->otype[ct].size, cd->prec);
	    if(ct != CT_VARCHAR || // size was too big
	       !(t = db_coldef_type(conn, CT_LONGVARCHAR, cd->prec)) ||
	       !(conn->otype[ct].searchable & SQL_ALL_EXCEPT_LIKE))
		exit(1);
	    fputs("Using LONGVARCHAR\n", stderr);
	    ct = CT_LONGVARCHAR;
	}
	st += ' ';
	int tpos = st.length();
	st += t;
	const char *ppos = strchr(t, '(');
	char buf[20];
	if(ppos || conn->otype[ct].parms)
	    sprintf(buf, "%d", cd->prec);
	if(ppos) {
	    const char *epos = strchr(ppos + 1, ')');
	    st.replace(tpos + (ppos - t), epos - ppos, buf);
	} else if(conn->otype[ct].parms) {
	    st += '(';
	    st += buf;
	    if(strchr(conn->otype[ct].parms, ',')) {
		st += ',';
		st += buf;
	    }
	    st += ')';
	}
	if(cd->ct == CT_BIT) {
/* Firebird's ODBC driver 2.0.5.156 had trouble converting long ints to
 * SQL_BIT for some reason.  Maybe that was fixed by the post-release commit
 * bdabf7ef3d8cfdd1a4cb55f40a0d603447619d04, which I apply to my own
 * system build.  If it's still a bug, define USE_CHAR_BOOL to 1.  As
 * time goes by, though, I won't be maintaining this, so it might stop
 * working some day.  In fact, once a new Firebird ODBC release is made,
 * I'll probably drop this code.  */
#define USE_CHAR_BOOL 0 /* use char 'N'/'Y' instead of native boolean */
#if USE_CHAR_BOOL
	    t = db_coldef_type(conn, CT_CHAR, 1);
	    if(!t && !conn->otype[CT_CHAR].type) {
		fputs("Can't get info on type CHAR\n", stderr);
		exit(1);
	    }
	    st.erase(tpos);
	    st += "CHAR CHECK(\"";
	    st += cd->name;
	    st += "\" IN ('N', 'Y'))";
#else
	    st += " NOT NULL";
#endif
	}
#if 1
	if(cd->constr) {
	    st += ' ';
	    st += cd->constr;
	}
#endif
    }
}

static bool cicompare(const std::string a, const std::string b)
{
    return strcasecmp(a.c_str(), b.c_str()) < 0;
}

void create_grok_tabs(db_conn *conn)
{
    strlist tabs = db_tables(conn);
    for(size_t i = 0; i < N_GROK_TABS; i++) {
	if(!binary_search(tabs.begin(), tabs.end(), grok_db_def[i].name,
			  cicompare)) {
	    std::string st("CREATE TABLE ");
	    st += grok_db_def[i].name;
	    st += " (\n  ";
	    for(const coldef *cd = grok_db_def[i].cols; cd->name; cd++) {
		if(cd != grok_db_def[i].cols)
		    st += ",\n  ";
		append_coldef(conn, st, grok_db_def[i].name, cd);
	    }
	    st += "\n)";
	    SQLRETURN ret = db_stmt(st.c_str());
	    if(SQL_FAILED(ret)) {
		db_err();
		exit(1);
	    }
	    db_next();
	} else {
	    // check that db is correct.
	    // not sure what to do about constraints; just ignore for now
	    // not going to bother with types, either, for now
	    // I mean, I could do alter table on all columns regardless
	    // and just ignore errors, but syntax varies and it's hard to
	    // deal with constraints.
	    // So, all I can do for now is drop excess columns and create
	    // missing ones.
	    strlist cols = db_cols(conn, grok_db_def[i].name);
	    for(const coldef *cd = grok_db_def[i].cols; cd->name; cd++) {
		if(!binary_search(cols.begin(), cols.end(), cd->name,
				  cicompare)) {
		    std::string st("ALTER TABLE ");
		    st += grok_db_def[i].name;
		    st += " ADD ";
		    append_coldef(conn, st, grok_db_def[i].name, cd);
		    SQLRETURN ret = db_stmt(st.c_str());
		    if(SQL_FAILED(ret))
			exit(1);
		    db_next();
		}
	    }
#if 1 // dropping excess columns is not really necessary
	    for(auto j = cols.begin(); j != cols.end(); j++) {
		int k;
		for(k = 0; grok_db_def[i].cols[k].ct; k++)
		    if(!strcasecmp(j->c_str(), grok_db_def[i].cols[k].name))
			break;
		if(!grok_db_def[i].cols[k].ct)
		    continue;
		std::string st = "ALTER TABLE ";
		st += grok_db_def[i].name;
		st += " DROP \"";
		st += *j;
		st += '"';
		SQLRETURN ret = db_stmt(st.c_str());
		if(SQL_FAILED(ret)) {
		    db_err();
		    exit(1);
		}
		db_next();
	    }
#endif
	}
    }
    db_trans(SQL_COMMIT);
}

/////////////////////////////////////////////////////

bool sql_write_form(db_conn *conn, const FORM *f)
{
    SQLRETURN ret;
    int bind, nint;
    SQLINTEGER ints[100];
    SQLUBIGINT fid, iid, cid;

#define do_db(x) do { \
    ret = x; \
    if(SQL_FAILED(ret)) \
	goto rollback; \
} while(0)
#define do_prep(s) do { \
    do_db(db_prep(s)); \
    bind = nint = 0; \
} while(0)
#define do_bindsn(s, ...) do_db(db_binds(++bind, s))
#define do_binds(s, ...) do_db(((s) __VA_ARGS__) ? db_binds(++bind, s) : db_bindnull(++bind, SQL_VARCHAR))
#define do_bindf(f, ...) do_db(((f) __VA_ARGS__) ? db_bindf(++bind, f) : db_bindnull(++bind, SQL_DOUBLE))
#define do_bindi(i, ...) do { \
    do_db(i __VA_ARGS__ ? db_bindi(++bind, ints[nint]) : db_bindnull(++bind, SQL_INTEGER)); \
    ints[nint++] = i; \
} while(0)
#define do_bindin(i, ...) do_bindi(i, || 1)
#define do_bind64 do_bindin
#if USE_CHAR_BOOL
#define do_bindb(b, ...) do_db(db_bindc(++bind, ((b) ? "Y" : "N")))
#else
#define do_bindb(f, ...) do_bindin(!!(f))
#endif
#define do_bindib(f, ...) do_bindb(IFL(item.,f))
#define do_bindc(c, ...) do_db(((c) __VA_ARGS__) ? db_bindc(++bind, c) : db_bindnull(++bind, SQL_CHAR))
#define do_exec() do_db(db_exec())
#define do_execn() do { do_exec(); db_next(); } while(0)

    do_prep("DELETE FROM grok_form_def WHERE \"name\" = ?");
    do_bindsn(f->name);
    do_execn();

#define concat_namec(z, n, v) \
    BOOST_PP_EXPR_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(v), 1), \
	BOOST_PP_STRINGIZE(BOOST_PP_COMMA_IF(n)) \
	"\"" BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, v)) "\"")
#define SQL_COLS(t) BOOST_PP_TUPLE_FOR_EACH(t, concat_namec)
#define concat_q(z, n, v) \
    BOOST_PP_EXPR_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(v), 1), \
	BOOST_PP_STRINGIZE(BOOST_PP_COMMA_IF(n)) "?")
#define INS_QS(t) BOOST_PP_TUPLE_FOR_EACH(t, concat_q)
#define NID(t) BOOST_PP_TUPLE_REMOVE(t, 0)
#define BIND_ALL(pref, t) BOOST_PP_TUPLE_FOR_EACH(t, bind_one, pref)
#define bind_one(z, n, cd, pref) \
    BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(cd),1), \
	_bind_one,BOOST_PP_TUPLE_EAT(2))(cd, pref)
#define _bind_one(cd, pref) \
    BOOST_PP_CAT(do_bind, BOOST_PP_TUPLE_ELEM(2, cd))(ipref(cd, pref) ifield(cd), icond(cd));
#define ifield(cd) \
    BOOST_PP_TUPLE_ELEM(0, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 4), \
	    cd BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(3, cd))))
#define icond(cd) \
    BOOST_PP_TUPLE_ELEM(1, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 4), \
	    (,) BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(3, cd)),))
#define ipref(cd, pref) \
    _ipref(pref, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 4), \
	    BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(3, cd))))
#define _ipref(pref, idef) \
    BOOST_PP_TUPLE_ELEM(BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(idef),3),0,3), ( \
         pref, BOOST_PP_REMOVE_PARENS(idef)))

#define insert_form_tab_row(t, pref) do { \
    do_prep("INSERT INTO " #t " (" SQL_COLS(t) ") VALUES (" INS_QS(t) ")"); \
    BIND_ALL(pref, t); \
    do_execn(); \
} while(0)

#define insert_form_ptab_row(t, id, pref) \
    id = db_get_pkey_id(conn, #t); \
    if(id == ~0UL) \
	goto rollback; \
    do_prep(id ? "INSERT INTO " #t " (" SQL_COLS(t) ") VALUES (" INS_QS(t) ")" : \
	    "INSERT INTO " #t "(" SQL_COLS(NID(t)) ") VALUES (" INS_QS(NID(t)) ")" \
	    "RETURNING \"id\""); \
    if(id) \
	do_db(db_bind64(++bind, id)); \
    BIND_ALL(pref, NID(t)); \
    do_exec(); \
    if(!id) { \
	ret = db_fetch(); \
	if(SQL_SUCCEEDED(ret)) \
	    db_icol(1, id); \
	if(!SQL_SUCCEEDED(ret)) \
	    goto rollback; \
    } \
    db_next()

    insert_form_ptab_row(grok_form_def, fid, f->);

    for(int j = 0; j < f->nchild; j++)
	insert_form_tab_row(grok_form_def_child, f->children[j]);

    for(int j = 0; j < f->nqueries; j++) {
	DQUERY *dq = &f->query[j];
	if(!dq->name && !dq->query)
	    continue;
	insert_form_tab_row(grok_form_def_query, dq->);
    }

    for(int j = 0; j < f->nitems; j++) {
	const ITEM &item = *f->items[j];
	insert_form_ptab_row(grok_form_item, iid, item.);

	for(int k = 0; k < item.nmenu; k++) {
	    MENU &m = item.menu[k];
	    insert_form_tab_row(grok_form_item_menu, m.);
	}

	for(int k = 0; k < item.nfkey; k++) {
	    FKEY &fk = item.fkey[k];
	    insert_form_tab_row(grok_form_item_fkey, fk.);
	}

	if(item.type == IT_CHART) {
	    cid = iid;
	    insert_form_tab_row(grok_form_item_chart, item.);

	    for(int k = 0; k < item.ch_ncomp; k++) {
		CHART &c = item.ch_comp[k];
		insert_form_tab_row(grok_form_item_chart_comp, c.);
	    }
	}
    }
    db_commit();
    return true;
rollback:
    db_err();
    db_trans(SQL_ROLLBACK);
    return false;
}

#define FETCH_ALL(t, pref) BOOST_PP_TUPLE_FOR_EACH(t, fetch_one, pref)
#define fetch_one(z, n, cd, pref) \
    BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(cd),1), \
	_fetch_one,BOOST_PP_TUPLE_EAT(2))(cd, pref)
#define _fetch_one(cd, pref) \
    BOOST_PP_CAT(do_col, BOOST_PP_TUPLE_ELEM(2, cd)) \
	(opref(cd, pref) ofield(cd), oval(cd));
#define ofield(cd) \
    BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 5), ifield, _ofield)(cd)
#define _ofield(cd) \
    BOOST_PP_TUPLE_ELEM(0, (BOOST_PP_REMOVE_PARENS(BOOST_PP_TUPLE_ELEM(4, cd))))
#define oval(cd) \
    BOOST_PP_TUPLE_ELEM(1, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 5), \
	    (,v) BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(4, cd))))
#define opref(cd, pref) \
    BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 5), ipref, _opref)(cd, pref)
#define _opref(cd, pref) \
    _ipref(pref, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 5), \
	    BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(4, cd))))

FORM *sql_read_form(db_conn *conn, const char *name)
{
    SQLRETURN ret;
    int bind, nint;
    SQLUBIGINT fid, iid, cid;
    SQLHSTMT istmt = conn->stmt /* , cstmt = conn->stmt */;
    FORM *f = form_create();
    // f->path = f->dir = conn->conn_str;

#define do_col64(var, val) do { \
    SQLLEN rlen; \
    SQLBIGINT v; \
    ret = db_col(++bind, SQL_C_SBIGINT, &v, sizeof(v), &rlen); \
    if(SQL_FAILED(ret)) \
	goto rollback; \
    if(rlen != SQL_NULL_DATA) \
	var = val; \
} while(0)
#define do_coli do_col64
#define do_colin do_col64
#if USE_CHAR_BOOL
#define do_colb(var, val) do { \
    SQLLEN rlen; \
    SQLCHAR v; \
    ret = db_col(++bind, SQL_C_CHAR, &v, sizeof(v), &rlen); \
    if(SQL_FAILED(ret)) \
	goto rollback; \
    v = v == 'Y'; \
    var = val; \
} while(0)
#else
#define do_colb do_col64
#endif
#define do_colib(fl,val) do { \
    SQLCHAR v, vv = 0; \
    do_colb(vv, v); \
    v = vv; \
    IFV(item.,fl,val); \
} while(0)
#define do_colc(var,val) do { \
    SQLLEN rlen; \
    SQLCHAR v; \
    ret = db_col(++bind, SQL_C_CHAR, &v, sizeof(v), &rlen); \
    if(SQL_FAILED(ret)) \
	goto rollback; \
    if(rlen != SQL_NULL_DATA) \
	var = val; \
} while(0)
#if 0 /* shouldn't need to get data in parts, but who knows? */
    static char *sbuf = NULL;
    static size_t sbuf_len = 0;
    fgrow(0, "sql string rsult", char, sbuf, (passlen = 128), &sbuf_len);
#endif
#define do_cols(var, val) do { \
    SQLLEN rlen; \
    char *v, c; \
    ret = db_col(++bind, SQL_C_CHAR, &c, 1, &rlen); \
    if(SQL_FAILED(ret)) \
	goto rollback; \
    if(rlen == SQL_NULL_DATA) \
	rlen = 0; \
    if(!rlen) \
	v = 0; /* yes, even for required fields */ \
    else { \
	v = (char *)malloc(rlen + 1); \
	if(!v) { \
	    perror("sql string result"); \
	    exit(1); \
	} \
	ret = db_col(bind, SQL_C_CHAR, v, rlen + 1, &rlen); \
	if(SQL_FAILED(ret)) \
	    goto rollback; \
    } \
    var = val; \
} while(0)
#define do_colsn do_cols
#define do_colf(var, val) do { \
    SQLLEN rlen; \
    SQLDOUBLE v; \
    ret = db_col(++bind, SQL_C_DOUBLE, &v, sizeof(v), &rlen); \
    if(SQL_FAILED(ret)) \
	goto rollback; \
    if(rlen != SQL_NULL_DATA) \
	var = val; \
} while(0)

    do_prep("SELECT " SQL_COLS(grok_form_def) " FROM grok_form_def"
	    " WHERE \"name\" = ?");
    do_bindsn(name);
    do_exec();
    ret = db_fetch();
    if(!SQL_SUCCEEDED(ret))
	goto rollback;
    bind = 0;
    FETCH_ALL(grok_form_def, f->);
    db_next();

#define TUPLE_REM1 NID
#define TUPLE_REM2(x) NID(NID(x))
#define TUPLE_REM3(x) NID(NID(NID(x)))
#define read_form_tab(t, skip, a, op, alen, id, idv, extra) do { \
    do_prep("SELECT " SQL_COLS(TUPLE_REM##skip(t)) " FROM " #t \
	    " WHERE \"" #id "_id\" = ? " #extra); \
    do_db(db_bind64(1, idv)); \
    do_exec(); \
    while(SQL_SUCCEEDED(ret = db_fetch())) { \
	if(alen) \
	    *((void **)&a) = realloc(a, sizeof(*(a)) * ++alen); \
	else \
	    *((void **)&a) = malloc(sizeof(*(a)) * ++alen); \
	/* leaks old a[] and their contents, so may as well exit */ \
	if(!(a)) { \
	    perror("reading " #t); \
	    exit(1); \
	} \
	memset(&a[alen - 1], 0, sizeof(*(a))); \
	bind = 0; \
	FETCH_ALL(TUPLE_REM##skip(t), a[alen - 1] op); \
    } \
    if(SQL_FAILED(ret)) \
	goto rollback; \
    db_next(); \
} while(0)

    read_form_tab(grok_form_def_child, 1, f->children, , f->nchild, form, fid, );

    read_form_tab(grok_form_def_query, 2, f->query, ., f->nqueries, form, fid, ORDER BY "seq");

    do_prep("SELECT " SQL_COLS(grok_form_item) " FROM grok_form_item"
	    " WHERE \"form_id\" = ? ORDER BY \"seq\"");
    do_db(db_bind64(1, fid));
    do_exec();
    do_db(SQLAllocHandle(SQL_HANDLE_STMT, conn->dbc, &conn->stmt));
#define swap_istmt do { \
    HSTMT t = conn->stmt; \
    conn->stmt = istmt; \
    istmt = t; \
} while(0)
    swap_istmt;
    while(SQL_SUCCEEDED(ret = db_fetch())) {
	if(!item_create(f, f->nitems))
	    exit(1);
	ITEM &item = *f->items[f->nitems - 1];
	bind = 0;
	do_col64(iid,v);
	bind += 2; /* skip form_id, seq */
	FETCH_ALL(TUPLE_REM3(grok_form_item), item.);

	swap_istmt;

	read_form_tab(grok_form_item_menu, 2, item.menu, ., item.nmenu, item, iid, ORDER BY "seq");

	read_form_tab(grok_form_item_fkey, 2, item.fkey, ., item.nfkey, item, iid, ORDER BY "seq");

	if(item.type == IT_CHART) {
	    do_prep("SELECT " SQL_COLS(NID(grok_form_item_chart))
		    " FROM grok_form_item_chart"
		    " WHERE \"item_id\" = ?");
	    do_db(db_bind64(1, iid));
	    do_exec();
	    if(!(SQL_SUCCEEDED((ret = db_fetch()))))
		goto rollback;
	    /* no need for new stmt handle, since cid == iid */
	    FETCH_ALL(NID(grok_form_item_chart), item.);
	    db_next();
	    cid = iid;

	    read_form_tab(grok_form_item_chart_comp, 2, item.ch_comp, ., item.ch_ncomp, chart, cid, ORDER BY "seq");
	}
	swap_istmt;
    }
    db_next();
    SQLFreeHandle(SQL_HANDLE_STMT, istmt);
    istmt = conn->stmt;

    return f;
rollback:
    if(SQL_FAILED(ret))
	db_err();
#if 0
    if(conn->stmt != cstmt && conn->stmt != istmt) {
	db_next();
	SQLFreeHandle(SQL_HANDLE_STMT, conn->stmt);
	conn->stmt = cstmt;
    }
#endif
    if(conn->stmt != istmt) {
	db_next();
	SQLFreeHandle(SQL_HANDLE_STMT, conn->stmt);
	conn->stmt = istmt;
    }
    db_next();
    form_delete(f);
    return NULL;
}
