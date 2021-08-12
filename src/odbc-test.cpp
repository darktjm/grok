#include <stdio.h>
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include "config.h"
#include "grok.h"
#include <QtWidgets>
#include "form.h"
#include "proto.h"
#include "odbc-supt.h"

/*
 * SQLGetInfo() ->
 *    SQL_AGGREGATE_FUNCTIONS
 *      -> mask: SQL_AF_* AVG/COUNT/DISTINCT/MAX/MIN/SUM/ALL
 *    SQL_ALTER_TABLE ->
 *      -> mask: SQL_AT_* (p. 3296)
 *    SQL_DRIVER_NAME -> duh
 *    SQL_DRIVER_VER -> nn.nn.nnnn[extra]
 *    SQL_DBMS_NAME -> duh
 *    SQL_DBMS_VER -> nn.nn.nnnn[extra]
 *    SQL_DM_VER -> nn.nn.nnnn[extra]
 *    SQL_MAX_COLUMN_NAME_LEN
 */

int main(int argc, const char **argv)
{
    if ((progname = strrchr(argv[0], '/')) && progname[1])
	progname++;
    else
	progname = argv[0];
    db_init();
    db_conn *conn = (db_conn *)calloc(sizeof(*conn), 1);
    bool drop_tabs = false;
    while(--argc && **++argv == '-') {
	switch((*argv)[1]) {
	  default:
	    usage:
		fputs("Usage: odbc-test [flags] connection-string [...]\n"
		      "flags:\n"
		      "\t-s f\tPrepend database feature substitutionss from file f\n"
		      "\t-d\tDrop tables first\n",
		      stderr);
	    exit(1);
	  case 's':
	    if(argc < 2)
		goto usage;
	    db_parse_subst_ovr(*++argv, conn);
	    argc--;
	    break;
	  case 'd':
	    drop_tabs = true;
	    break;
	}
    }
    if(argc < 1)
	return 1;
    for(int i = 0; i < argc; i++) {
	std::cout << "conn: " << argv[i] << '\n';
	if(!db_open(argv[i], conn))
	    exit(1);
	if(drop_tabs)
	    drop_grok_tabs(conn);
	create_grok_tabs(conn);
	QDir files("/usr/local/share/grok/grokdir", "*.gf");
	QFileInfoList fl = files.entryInfoList();
	files.setPath(QDir::homePath() + "/.grok");
	fl.append(files.entryInfoList());
	for(auto j = fl.begin(); j != fl.end(); j++) {
	    char *path = qstrdup(j->absoluteFilePath());
	    std::cout << "form: " << path << '\n';
	    FORM *f = read_form(path);
	    if(!f)
		continue;
	    free(path);
	    path = strdup(f->name);
	    if(!sql_write_form(conn, f)) {
		form_delete(f);
		free(path);
		continue;
		exit(1);
	    }
	    form_delete(f);

	    f = sql_read_form(conn, path);
	    if(f)
		std::cout << "saved and loaded\n";
	    else
		exit(1);
	    free(path);
	}
	db_close(conn);
    }
}

/////////////////////////////////////////////////////

// stuff to keep linker happy
const char *progname;
GrokMainWindow	*mainwindow;
CARD *card_list;
struct pref pref;

void card_readback_texts(UNUSED CARD *card, UNUSED int which) {}
void remake_dbase_pulldown() {}
void fillout_formedit_widget_proc() {}
void redraw_canvas_item(UNUSED ITEM *item) {}
void destroy_card_menu(UNUSED CARD *card) {}
void build_card_menu(UNUSED CARD *card, UNUSED QWidget *p) {}
void fillout_card(UNUSED CARD *card, UNUSED bool f) {}
void edit_file(UNUSED const char *f, UNUSED bool x, UNUSED bool y, UNUSED const char *t, UNUSED const char *h) {}
void print_info_line() {}

DQUERY *add_dquery(
	FORM		*fp)		/* form to add blank entry to */
{
	zgrow(0, "query", DQUERY, fp->query, fp->nqueries, fp->nqueries + 1, 0);
	return(&fp->query[fp->nqueries++]);
}

void create_error_popup(
	UNUSED QWidget	*widget,
	UNUSED int	error,
	const char	*fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
