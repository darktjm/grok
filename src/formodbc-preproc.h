/*
 * Preprocessor support for formodbc.c
 *
 * The main purpose of this is to add comments to the preprocessor macros
 * without cluttering up formodbc.c.
 */

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
#define TUPLE_FOR_EACH(t, /* macro, */ ...) \
    BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(t), _TUPLE_FOR1, (t)((__VA_ARGS__)))
////////////////////////////////////////////////////////////////

// Relatively generic for_each callbacks for the above (and others)

// give a comma-separated list of C string constants
#define STRING_LIST(z, n, v) \
    BOOST_PP_COMMA_IF(n) BOOST_PP_STRINGIZE(v)

// give a comma-separated list of SQL strings as a C string constant
#define COMMA_DBSTR(z, n, v) \
    BOOST_PP_STRINGIZE(BOOST_PP_COMMA_IF(n)) \
    "'" BOOST_PP_STRINGIZE(v) "'"

// give a comma-separated list of SQL column names as a C str const
// column names are double-quoted so that reserved words are OK
#define COMMA_DBNAME(z, n, v) \
    BOOST_PP_STRINGIZE(BOOST_PP_COMMA_IF(n)) "\"" BOOST_PP_STRINGIZE(v) "\""

// give a comma-separated list of ? for each call as C str const
#define COMMA_Q(z, n, v) \
    BOOST_PP_STRINGIZE(BOOST_PP_COMMA_IF(n)) "?"

// give a comma-separated list of col = ? entries for use in UPDATE
// But I never bind, so this is unused/untested
#define UPDATE_BIND(z, n, v) \
    COMMA_DBNAME(z, n, v) " = ?"

////////////////////////////////////////////////////////////////

// Not so generic stuff, specific to how the preprocessor-defined
// table definitions work:
//
// each table is a macro w/ name of table defining a tuple of column defs
// each column def is a tuple of:
//    name, (type[, prec[, constr]]), bind/col suffix[, (ivar[, icond[, pref]])
//                                                   [, (ovar[, val[, pref]])]]
// or just a single-element tuple:
//    table_constraint

// Return the list of columns for t as a C string for pasting into SQL
// t's name itself isn't used, so t can be a subset of elements if needed
// (e.g. stripping out id)
#define SQL_COLS(t) TUPLE_FOR_EACH(t, _concat_namec)
// column name is elt 0, but only if >1 elt
#define _concat_namec(z, n, v) \
    BOOST_PP_EXPR_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(v), 1), \
	COMMA_DBNAME(z, n, BOOST_PP_TUPLE_ELEM(0, v)))

// In fact, since id is the first column, here's a shortcut for stripping it:
#define NID(t) BOOST_PP_TUPLE_REMOVE(t, 0)

// companion: return ? for eaach element (column) in t as a C string
// for pasting as binding parameters into SQL INSERT statements
#define INS_QS(t) TUPLE_FOR_EACH(t, _concat_q)
// column name is elt 0, but only if >1 elt
#define _concat_q(z, n, v) \
    BOOST_PP_EXPR_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(v), 1), \
	COMMA_Q(z, n, v))

// Bind all column definitions in t
// assumes do_bind<bind/col suffix> is a macro taking 2 parms:
//   parm 1 is the value to bind, and parm 2 is added to the condition
//   that selects binding the value instead of NULL.
// While it's possible to supply parameter number, instead it is expected
// tht do_bind*() will set it automaticaly.  This is not only more convenient,
// it also allows easier insertion of parameters before the ones in t.
// Like the above, t's name isn't used, so it can be a filtered list
// pref is the default input prefix
#define BIND_ALL(pref, t) TUPLE_FOR_EACH(t, BIND_ONE, pref)
// First, filter out constraints, ensuring that lower calls always have
// at least cd[0] .. cd[2].
#define BIND_ONE(z, n, cd, pref) \
    BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(cd),1), \
	_bind_one,BOOST_PP_TUPLE_EAT(2))(cd, pref)
// Then, call do_bind<cd[2]>(<input prefix> <input var>, <input cond>);
#define _bind_one(cd, pref) \
    BOOST_PP_CAT(do_bind, BOOST_PP_TUPLE_ELEM(2, cd))(_ipref(cd, pref) _ivar(cd), _icond(cd));
// input var is element 0 of cd[3] if present; converting non-tuple cd[3] to
// 1-element tuple if it's just the var name.  Otherwise, it's cd[0].
// care must be taken to not access cd[3] if not present; thus the EAT()
#define _ivar(cd) \
    BOOST_PP_TUPLE_ELEM(0, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 4), \
	    cd BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(3, cd))))
// input cond is element 1 of cd[3] if present
// care must be taken to not access cd[3] if not present; thus the EAT()
// care must be taken to ensure cd[3] has at least 2 elements; thus the
// empty , at the end and the use of (,) as a default.
#define _icond(cd) \
    BOOST_PP_TUPLE_ELEM(1, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 4), \
	    (,) BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(3, cd)),))
// input prefix is element 2 of cd[3] if present, or default pref otherwise
// first extract cd[3] and check for presence of element 2 in _ipref2
// care must be taken to not access cd[3] if not present; thus the EAT()
// always passes cd[3] as a tuple of at least 1 (possibly blank) element
#define _ipref(cd, pref) \
    _ipref2(pref, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 4), \
	    BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(3, cd))))
// care must be taken to not access idef[2] if not present.  rather than
// using EAT this time, I'm building a tuple w/ pref as 0 and idef as 1..3,
// and thus choose element 0 or 3 depending on idef's size.  Just for variety.
#define _ipref2(pref, idef) \
    BOOST_PP_TUPLE_ELEM(BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(idef),3),0,3), ( \
         pref, BOOST_PP_REMOVE_PARENS(idef)))

// retrieve all data for columns in t after a fetch
// assumes do_col<bind/col suffix> is a macro taking 2 parms:
//   parm 1 is the value to fetch into, and parm 2 is a mangled version of
//   the data, which is named v.  It is expected that the macro will leave
//   the default in place if the column is NULL.
// While it's possible to supply column number, instead it is expected
// tht do_col*() will set it automaticaly.  This is not only more convenient,
// it also allows easier insertion of columns before the ones in t.
// Like the above, t's name isn't used, so it can be a filtered list
// pref is the default output prefix
#define FETCH_ALL(t, pref) TUPLE_FOR_EACH(t, FETCH_ONE, pref)
// First, filter out constraints, ensuring that lower calls always have
// at least cd[0] .. cd[2].
#define FETCH_ONE(z, n, cd, pref) \
    BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(cd),1), \
	_fetch_one,BOOST_PP_TUPLE_EAT(2))(cd, pref)
// Then, call do_col<cd[2]>(<output prefix> <output var>, <output xlate>);
#define _fetch_one(cd, pref) \
    BOOST_PP_CAT(do_col, BOOST_PP_TUPLE_ELEM(2, cd)) \
	(_opref(cd, pref) _ovar(cd), _oval(cd));
// output var is element 0 of cd[4] if present, and input var otherwise
// care must be taken to not access cd[4] if not present, thus if on fn name
#define _ovar(cd) \
    BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 5), _ivar, _ovar2)(cd)
// like for input var, element 0 need not be a tuple if no other elements
#define _ovar2(cd) \
    BOOST_PP_TUPLE_ELEM(0, (BOOST_PP_REMOVE_PARENS(BOOST_PP_TUPLE_ELEM(4, cd))))
// output value is element 1 of cd[4] if present, or v otherwise.
// care must be taken to not access cd[4] if not present, thus EAT
// care must be taken not to access element 1 if not present, thus the ,v
#define _oval(cd) \
    BOOST_PP_TUPLE_ELEM(1, (BOOST_PP_REMOVE_PARENS( \
	BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 5), \
	    BOOST_PP_TUPLE_EAT(2), BOOST_PP_TUPLE_ELEM)(4, cd)),v))
// output prefix is element 2 of cd[4] if present, or input prefix otherwise
// first call ipref if cd[4] not present
#define _opref(cd, pref) \
    BOOST_PP_IIF(BOOST_PP_LESS(BOOST_PP_TUPLE_SIZE(cd), 5), _ipref, _opref2)(cd, pref)
// Then, extract cd[4] as a tuple and use _ipref2 to check for element 2
#define _opref2(cd, pref) \
    _ipref2(pref, (BOOST_PP_REMOVE_PARENS(BOOST_PP_TUPLE_ELEM(4, cd))))
