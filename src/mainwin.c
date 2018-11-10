/*
 * Create the main window and everything in it.
 *
 *	create_mainwindow(toplvl)	Create all widgets in main window
 *	print_info_line()		print database status into main window
 *	remake_dbase_pulldown()		put known forms into dbase pulldown
 *	remake_section_pulldown()	put known sections into sect pulldown
 *	remake_query_pulldown()		put predefined queries into pulldown
 *	remake_sort_pulldown()		put sort criteria into sort pulldown
 *	switch_form(formname)		switch mainwindow to new form
 *	search_cards(mode,card,string)	do a query for an expression or string
 */

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef DIRECT
#include <sys/dir.h>
#define  dirent direct
#else
#include <dirent.h>
#endif
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define OFF		16		/* margin around summary and card */
#define NHIST		20		/* remember this many search strings */

#ifndef LIB
#define LIB "/usr/local/lib"
#endif

static void append_search_string(char *);
static void file_pulldown	(int);
static void newform_pulldown	(int);
static void help_pulldown	(int);
static void dbase_pulldown	(int);
static void section_pulldown	(int);
static void mode_callback	(int);
static void sect_callback	(int);
static void query_pulldown	(int item, bool set = false);
static void sort_pulldown	(int item, bool set = false);
static void search_callback	(int);
static void requery_callback	(void);
static void clear_callback	(void);
static void letter_callback	(int);
static void pos_callback	(int);
static void new_callback  (void);
static void dup_callback  (void);
static void del_callback  (void);

CARD 			*curr_card;	/* card being displayed in main win, */
char			*prev_form;	/* previous form name */
QMainWindow		*mainwindow;	/* popup menus hang off main window */
int			last_query=-1;	/* last query pd index, for ReQuery */
#if 0
static int		win_xs, win_ys;	/* size of main window w/o sum+card */
#endif
static QMenu		*dbpulldown;	/* dbase pulldown menu widget */
static QMenu		*sectpulldown;	/* sectn pulldown menu widget */
static QMenu		*sortpulldown;	/* sort  pulldown menu widget */
static QMenu		*qpulldown;	/* query pulldown menu widget */

static QLabel		*w_info;	/* info label, for "n found" */
static QLabel		*w_mtime;	/* shows last modify time of card */
static QLineEdit	*w_search;	/* search string text widget */
static QPushButton	*w_prev;	/* search prev arrow */
static QPushButton	*w_next;	/* search next arrow */
       QVBoxLayout	*mainform;	/* form for summary table */
       QWidget		*w_summary;	/* widget to replace in mainform */
static QWidget		*w_card;	/* form for card */
static QPushButton	*w_left;	/* button: previous card */
static QPushButton	*w_right;	/* button: next card */
static QPushButton	*w_new;		/* button: start a new card */
static QPushButton	*w_dup;		/* button: duplicate a card */
static QPushButton	*w_del;		/* button: delete current card */
static QComboBox	*w_sect;	/* button: section popup for card */
static int		defsection;	/* default section */

static Searchmode	searchmode;	/* current search mode */
static const char	* const modename[] = { "All", "In query", "Narrow", "Widen",
					"Widen in Query", "Find & select" };


/*
 * create the main window with a menu bar and empty forms for the summary
 * and the card. The forms will be filled later. This routine is called
 * once during startup, before the first switch_form().
 */

void create_mainwindow()
{
	QWidget		*w;
	QComboBox	*popup;
	long		i;
	char		buf[10];
	QMenuBar	*menubar;
	QMenu		*menu, *submenu;

	mainwindow = new QMainWindow();
	set_icon(mainwindow, 0); // from main(); there is no separate "toplevel"

							/*-- menu bar --*/
	menubar = mainwindow->menuBar();
	menu = menubar->addMenu("&File");
	menu->setTearOffEnabled(true);
	bind_help(menu, "pd_file");
	menu->addAction("&Find && select", [=](){file_pulldown(0);},
			QKeySequence("Ctrl+F"));
	menu->addAction("&Print...", [=](){file_pulldown(1);},
			QKeySequence("Ctrl+P"));
	menu->addAction("&Export", [=](){file_pulldown(2);},
			QKeySequence("Ctrl+E"));
	menu->addAction("P&references...", [=](){file_pulldown(3);},
			QKeySequence("Ctrl+R"));
	submenu = menu->addMenu("F&orm editor");
	if (restricted)
		submenu->setEnabled(false);
	submenu->setTearOffEnabled(true);
	bind_help(submenu, "pd_file");
	submenu->addAction("&Edit current form...", [=](){newform_pulldown(0);});
	submenu->addAction("&Create new form from scratch...", [=](){newform_pulldown(1);});
	submenu->addAction("Create, use current as &template...", [=](){newform_pulldown(2);});
	menu->addAction("&About...", [=](){file_pulldown(5);});
	menu->addAction("&Save", [=](){file_pulldown(6);},
			QKeySequence("Ctrl+S"));
	menu->addAction("&Quit", [=](){file_pulldown(7);},
			// FIXME:  Qt is ignoring Ctrl+Q
			QKeySequence("Ctrl+Q"));
	menu->addAction("Rambo Quit", [=](){file_pulldown(8);});
	
	dbpulldown = menu = menubar->addMenu("&Database");
	menu->setTearOffEnabled(true);
	bind_help(menu, "pd_dbase");

	sectpulldown = menu = menubar->addMenu("S&ection");
	menu->setTearOffEnabled(true);
	bind_help(menu, "pd_section");

	sortpulldown = menu = menubar->addMenu("&Sort");
	menu->setTearOffEnabled(true);
	bind_help(menu, "pd_sort");

	qpulldown = menu = menubar->addMenu("&Query");
	menu->setTearOffEnabled(true);
	bind_help(menu, "pd_query");

	// Qt seems to have no way to force this to the right
	// Qt also seems to have no way to make this the "help" menu
	menu = menubar->addMenu("&Help");
	menu->setTearOffEnabled(true);

	menu->addAction("On &context", [=](){help_pulldown(0);},
			QKeySequence("Ctrl+H"));
	menu->addAction("Current &database", [=](){help_pulldown(1);},
			QKeySequence("Ctrl+D"));
	menu->addSeparator();
	menu->addAction("&Introduction", [=](){help_pulldown(2);});
	menu->addAction("&Getting help", [=](){help_pulldown(3);});
	menu->addAction("&Troubleshooting", [=](){help_pulldown(4);});
	menu->addAction("&Files and programs", [=](){help_pulldown(5);});
	menu->addAction("&Expression grammar", [=](){help_pulldown(6);},
			QKeySequence("Ctrl+G"));
	menu->addAction("&Variables and X Resources", [=](){});

	w = new QWidget;
	mainwindow->setCentralWidget(w);
	mainform = new QVBoxLayout(w);
	add_layout_qss(mainform, "mainform");
							/*-- search string --*/
	QHBoxLayout *hb = new QHBoxLayout;
	add_layout_qss(hb, "searchform");
	mainform->addLayout(hb);
	// bind_help(hb, "search"); // not a widget, so how?

	w = new QPushButton("Search");
	hb->addWidget(w);
	set_button_cb(w, search_callback(0));
	bind_help(w, "search");

	popup = new QComboBox;
	for (i=0; i < SM_NMODES; i++)
		popup->addItem(modename[i]);
	set_popup_cb(popup, mode_callback(i), int, i);
	bind_help(popup, "search");
	hb->addWidget(popup);

	w_search = new QLineEdit;
	w_search->sizePolicy().setHorizontalStretch(1);
	hb->addWidget(w_search);
	set_text_cb(w_search, search_callback(0));
	bind_help(w_search, "search");

	QHBoxLayout *h = new QHBoxLayout;
	hb->addLayout(h);
	h->setSpacing(0);
	w_prev = new QPushButton(QIcon::fromTheme("go-previous"), "");
	h->addWidget(w_prev);
	w_prev->setEnabled(false);
	set_button_cb(w_prev, search_callback(-1));
	bind_help(w_prev, "search");

	w = new QPushButton("C");
	// This should be easier, but the non-motif default is to make
	// all buttons 80 pixels wide
	w->setMinimumWidth(strlen_in_pixels(w, "C")+8 /* +border?*/);
	// and because it's in its own hboxlayout, it gets bigger
	w->setMaximumWidth(w->minimumWidth());
	h->addWidget(w);
	set_button_cb(w, clear_callback());
	bind_help(w, "search");

	w_next = new QPushButton(QIcon::fromTheme("go-next"), "");
	h->addWidget(w_next);
	w_next->setEnabled(false);
	set_button_cb(w_next, search_callback(1));
	bind_help(w_next, "search");

	w = new QPushButton("Requery");
	hb->addWidget(w);
	set_button_cb(w, requery_callback());
	bind_help(w, "search");

							/*-- info line --*/
	hb = new QHBoxLayout;
	add_layout_qss(hb, NULL);
	mainform->addLayout(hb);

	w_info = new QLabel(" ");
	hb->addWidget(w_info);
	bind_help(w_info, "info");

	hb->addStretch(1);

	w_mtime = new QLabel(" ");
	hb->addWidget(w_mtime);
	bind_help(w_mtime, "info");

							/*-- buttons --*/
	// A QDialogButtonBox would seem appropriate, but it doesnt
	// support non-button widgets (like the section selector)
	QHBoxLayout *bb = new QHBoxLayout;
	add_layout_qss(bb, NULL);

	bb->addWidget(w_left = new QPushButton(pixmap[PIC_LEFT], ""), dbbr(Action));
	w_left->resize(30, 30);
	w_left->setEnabled(false);
	set_button_cb(w_left, pos_callback(-1));
	bind_help(w_left, "pos");

	bb->addWidget(w_right = new QPushButton(pixmap[PIC_RIGHT], ""), dbbr(Action));
	w_right->resize(30, 30);
	w_right->setEnabled(false);
	set_button_cb(w_right, pos_callback(1));
	bind_help(w_right, "pos");

	/* grok makes these 60-wide instead of 80-wide like the dialogs */
	/* but I'm going to use mk_button anyway */
	bb->addWidget(w_new = mk_button(NULL, "New"));
	set_button_cb(w_new, new_callback());
	bind_help(w_new, "new");

	bb->addWidget(w_dup = mk_button(NULL, "Dup"));
	set_button_cb(w_dup, dup_callback());
	bind_help(w_dup, "dup");

	bb->addWidget(w_del = mk_button(NULL, "Delete"));
	set_button_cb(w_del, del_callback());
	bind_help(w_del, "del");

	bb->addWidget(w_sect = new QComboBox);
	w_sect->hide();
	set_popup_cb(w_sect, sect_callback(i), int, i); // formerly in remake_popups
	bind_help(w_sect, "sect"); // formerly in remake_popup()

	bb->addStretch(100); // can't really do platform-dependent Help location

	bb->addWidget(w = mk_button(NULL, 0, dbbb(Help)));
	set_button_cb(w, help_callback(mainwindow, "card"));
	bind_help(w, "card");
							/*-- summary --*/
	w_summary = new QWidget; // to be replaced
	mainform->addWidget(w_summary);

							/*-- letters --*/
	if (pref.letters) {
	 hb = new QHBoxLayout;
	 add_layout_qss(hb, "letters");
	 // bind_help(hb, "letters"); // not a widget
	 hb->setContentsMargins(0, 0, 0, 0);
	 hb->setSpacing(0);
	 mainform->addLayout(hb);
	 for (i=0; i < 28; i++) {
	  sprintf(buf, i < 26 ? "%c" : i==26 ? "misc" : "all", (int)i+'A');
	  w = new QPushButton(buf);
	  w->setMinimumWidth(strlen_in_pixels(w, buf)+4/*border?*/);
	  int l, r, t, b;
	  w->getContentsMargins(&l, &r, &t, &b);
	  w->setContentsMargins(2, 2, t, b);
	  hb->addWidget(w);
	  set_button_cb(w, letter_callback(i));
	  bind_help(w, "letters");
	 }
	}
							/*-- card --*/
	// this isn't really necessary, since the card is in a frame
	mainform->addWidget(mk_separator());
	w_card = new QWidget;
	w_card->resize(400, 6);
	w_card->setObjectName("cardform");
	w_card->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	bind_help(w_card, "card");
	mainform->addWidget(w_card);

	mainform->addLayout(bb); // needs to be last so it's on bottom

	create_summary_menu(curr_card);
	remake_dbase_pulldown();
	remake_section_pulldown();
	remake_query_pulldown();
	remake_sort_pulldown();
}


/*
 * resize the main window such that there is enough room for the summary
 * and the card. During startup, create_mainwindow() has stored the size
 * without summary and card, now add enough space for the new summary and
 * card and resize the window.
 */

void resize_mainwindow(void)
{
#if 0 // tjm - auto-resizing should be sufficient
	int	xs=0, ys=0;

	if (!win_ys) {
		const QSize size(mainwindow->size());
		win_xs = size.width();
		win_ys = size.height();
		win_ys -= 1;
	}
	if (curr_card && curr_card->form) {
		xs = pref.scale * curr_card->form->xs + 2;
		ys = pref.scale * curr_card->form->ys + 2;
	}
	if (win_xs > xs) xs = win_xs;
	mainwindow->resize(xs + 2*2 + 2*16, win_ys + ys + 2*3);
#else
	// FIXME: window moves -- saving & restoring position doesn't help
	// I'll probably have to override mainwindow->move().
	QPoint pos(mainwindow->pos());
	mainwindow->adjustSize();
	mainwindow->move(pos);
	// FIXME: window is too wide -- setting to "minimum size" doesn't work
	// except for the first time
	mainwindow->resize(mainwindow->minimumSize());
#endif
}


/*
 * print some enlightening info about the database into the info line between
 * the search string and the summary. This is called whenever something
 * changes: either the database was modified, or saved (modified is turned
 * off), or a search is done.
 */

void print_info_line(void)
{
	char		buf[256];
	register CARD	*card = curr_card;
	register DBASE	*dbase;
	int		s, n;

	if (!mainwindow)
		return;
	if (!card || !card->dbase || !card->form || !card->form->name) {
		strcpy(buf, "No database");
		print_button(w_mtime, "");
	} else {
		*buf = 0;
		dbase = card->dbase;
		if (card->form->syncable && card->row >= 0
					 && card->row < dbase->nrows) {
			time_t t = dbase->row[card->row]->mtime;
			time_t c = dbase->row[card->row]->ctime;
			if (c)
				sprintf(buf, "created %s %s",
						mkdatestring(c),
						mktimestring(c, FALSE));
			if (t && t != c)
				sprintf(buf+strlen(buf), "%schanged %s %s",
						c ? ", " : "",
						mkdatestring(t),
						mktimestring(t, FALSE));
		}
		print_button(w_mtime, buf);

		s = dbase->nsects > 1 ? dbase->currsect : -1;
		n = s >= 0 ? dbase->sect[s].nrows : dbase->nrows;
		if (!dbase->nrows)
			strcpy(buf, "No cards");
		else
			sprintf(buf, "%s: %d of %d card%s", card->form->name,
					card->nquery, n, n==1 ? "" : "s");
		if (s >= 0) {
			strcat(buf, " in section ");
			strcat(buf, section_name(dbase, s));
		}
		if (s >= 0 ? dbase->sect[s].rdonly : dbase->rdonly)
			strcat(buf, " (read only)");
		if (s >= 0 ? dbase->sect[s].modified : dbase->modified)
			strcat(buf, " (modified)");
	}
	print_button(w_info, buf);
}


/*-------------------------------------------------- variable pulldowns -----*/
/*
 * Read the current directory, the ./grokdir directory, the GROKDIR directory,
 * and the preference directories, and collect all form files (*.f) and put
 * them into the Form pulldown (pulldown #1). This is, of course, a flagrant
 * violation of the Motif style guide.
 */

#define MAXD	200		/* no more than 200 databases in pulldown */
static struct db {
	char	*name;		/* callback gets an index into this array */
	char	*path;		/* path where form file was found */
} db[MAXD];

static int append_to_dbase_list(
	long		nlines,		/* # of databases already in pulldown*/
	char		*path,		/* directory name */
	int		order)		/* "chapter" number for sorting */
{
	char		name[256];	/* tmp buffer for name */
	char		*p;		/* for removing extension */
	DIR		*dir;		/* open directory file */
	struct dirent	*dp;		/* one directory entry */
	int		num=0, i;

	name[0] = order + '0';
	path = resolve_tilde(path, 0);
	if (!(dir = opendir(path)))
		return(nlines);
	while (nlines < MAXD-!num) {
		if (!(dp = readdir(dir)))
			break;
		if (!(p = strrchr(dp->d_name, '.')) || strcmp(p, ".gf"))
			continue;
		*p = 0;
		for (i=0; i < nlines; i++)
			if (db[i].path && (pref.uniquedb ||
					 !strcmp(db[i].path, path))
				      && !strcmp(db[i].name+1, dp->d_name))
			    	break;
		if (i < nlines)
			continue;
		if (!num++ && nlines) {
			name[1] = 0;
			db[nlines].path = 0;
			db[nlines++].name = mystrdup(name);
		}
		strncpy(name+1, dp->d_name, sizeof(name)-2);
		db[nlines].name = mystrdup(name);
		db[nlines].path = mystrdup(path);
		nlines++;
	}
	(void)closedir(dir);
	return(nlines);
}


static void make_dbase_pulldown(
	long		nlines)		/* # of databases already in pulldown*/
{
	int		i;

	for (i=0; i < nlines; i++) {
		if (!db[i].path) {
#			ifndef NOMSEP
			dbpulldown->addSeparator();
#			endif
		} else {
			QString n(db[i].name+1);
			n.replace('&', "&&");
			dbpulldown->addAction(n, [=](){dbase_pulldown(i);});
		}
	}
}


static int compare_db(
	register const void	*u,
	register const void	*v)
{
	return(strcmp(((struct db *)u)->name, ((struct db *)v)->name));
}


void remake_dbase_pulldown(void)
{
	int		n;
	char		path[1024], *env;

	dbpulldown->clear();
	for (n=0; n < MAXD; n++) {
		if (db[n].path)
			free((void *)db[n].path);
		if (db[n].name)
			free((void *)db[n].name);
		db[n].name = 0;
		db[n].path = 0;
	}
	env = getenv("GROK_FORM");
	strcpy(path, env ? env : "./");
	n = append_to_dbase_list(0, path, 0);
	strcpy(path, "./grokdir");
	n = append_to_dbase_list(n, path, 1);
	strcpy(path, GROKDIR);
	n = append_to_dbase_list(n, path, 2);
	strcpy(path, LIB "/grokdir");
	n = append_to_dbase_list(n, path, 3);
	qsort(db, n, sizeof(struct db), compare_db);
	make_dbase_pulldown(n);
	// tearoff already enabled above
}


/*
 * After a database was loaded, there is a section list in the dbase struct.
 * Present it in a pulldown if there are at least two sections.
 */

#define MAXSC  200		/* no more than 200 sections in pulldown */

void remake_section_pulldown(void)
{
	int		maxn;
	long		n;

	sectpulldown->clear();
	if (!curr_card || !curr_card->dbase || curr_card->form->proc) {
		sectpulldown->setEnabled(false);
		return;
	}
	maxn = curr_card->dbase->havesects ? curr_card->dbase->nsects : 0;
	if (maxn > MAXSC - 2)
		maxn = MAXSC - 2;
	if(maxn)
		sectpulldown->addAction("All", [=](){section_pulldown(0);});
	for (n=0; n < maxn; n++) {
		QString s(section_name(curr_card->dbase, n));
		s.replace('&', "&&");
		sectpulldown->addAction(s, [=](){section_pulldown(n + 1);});
	}
	sectpulldown->addAction("New ...", [=](){section_pulldown(n + 1);});
	sectpulldown->setEnabled(true);
	// teearoff already enabled above
	remake_section_popup(TRUE);
}


/*
 * Put all the default queries in the form into the Query pulldown (pulldown
 * #3). Add a default query "All" at the top. This is yet another violation
 * of the Motif style guide. So what.
 */

#define MAXQ	100		/* no more than 100 queries in pulldown */
static QActionGroup	*qag = 0;	/* for readio-like queries */

void remake_query_pulldown(void)
{
	long		i;		/* # of queries in pulldown */
	int		n;		/* max # of lines in pulldown */

	qpulldown->clear();
	if(qag) {
		delete qag;
		qag = 0;
	}
	if (!curr_card || !curr_card->form)
		return;

	QAction *aq = qpulldown->addAction("Autoquery",
				   [=](bool c){query_pulldown(-2, c);});
	aq->setCheckable(true);
	aq->setChecked(pref.autoquery);

	if (pref.autoquery)
		qag = new QActionGroup(qpulldown);

	n = curr_card->form->nqueries > MAXQ-2 ? MAXQ-2
					       : curr_card->form->nqueries;
	for (i=0; i <= n; i++) {
		DQUERY *dq = i ? &curr_card->form->query[i-1] : 0;
		if (i && (dq->suspended || !dq->name || !dq->query))
			continue;
		QString name(i ? dq->name : "All");
		name.replace('&', "&&");
		if (pref.autoquery) {
			QAction *a = qpulldown->addAction(name,
					 [=](){query_pulldown(i-1);});
			a->setCheckable(true);
			a->setChecked(curr_card->form->autoquery == i-1);
			qag->addAction(a);
		} else {
			qpulldown->addAction(name, [=](){query_pulldown(i-1);});
		}
	}
	// tearoff already set above
	last_query = curr_card->form->autoquery;
}


/*
 * put a new option popup menu on the card's Section button
 */

static void	remake_popup(void);

void remake_section_popup(
	BOOL		newsects)	/* did the section list change? */
{
	if (newsects)
		remake_popup();

	if (!curr_card	|| !curr_card->dbase
			||  curr_card->row < 0
			||  curr_card->row >= curr_card->dbase->nrows
			|| !curr_card->dbase->havesects) {
		if (w_sect->isVisible()) {
			w_sect->setEnabled(false);
			// I've no idea what this is doing here:
			w_del->setEnabled(false);
		}
		return;
	}
	if (w_sect->isVisible()) {
		printf("%d\n", curr_card->dbase->row[curr_card->row]->section);
		w_sect->setEnabled(true);
		w_sect->setCurrentIndex(
			curr_card->dbase->row[curr_card->row]->section);
		// I've no idea what this is doing here
		w_del->setEnabled(true);
	}
}


static void remake_popup(void)
{
	int		i, n;

	w_sect->clear();
	w_sect->hide();
	n = curr_card->dbase->nsects;
	if (n < 2)
		return;

	for (i=0; i < n; i++) {
		QString str(section_name(curr_card->dbase, i));
		str.replace('&', "&&");
		w_sect->addItem(str);
		// my use of a QComboBox makes marking individual items
		// read-only impossible, but as far as I can tell, it's
		// not possible to set a section read-only, anyway
		// (Instead, the form's read-only status should prevail)
		// callback is set above for entire widget
	}
	w_sect->show();
}


/*
 * Put all the columns in the summary into the Sort pulldown (pulldown #2),
 * except those that have the nosort flag set.
 * What you hear weeping is the author of the Motif style guide.
 */

#define MAXS	100		/* no more than 100 criteria in pulldown */
static QActionGroup	*sag = 0;	/* for radio-like sort menu */

void remake_sort_pulldown(void)
{
	int	sort_col[2*MAXS+1];	/* column for each pulldown item */
	char		buf[256];	/* pulldown line text */
	register ITEM	*item;		/* scan items for sortable columns */
	int		i;		/* item counter */
	int		j;		/* skip redundant choice items */
	int		n;		/* # of lines in pulldown */

	sortpulldown->clear();
	if(sag) {
		delete sag;
		sag = 0;
	}

	if (!curr_card || !curr_card->form || !curr_card->dbase)
		return;

	QAction *rs = sortpulldown->addAction("Reverse sort",
					      [=](bool c){sort_pulldown(-1, c);});
	rs->setCheckable(true);
	rs->setChecked(pref.revsort);
	sag = new QActionGroup(mainwindow);

	for (n=1, i=0; i < curr_card->form->nitems; i++) {
		item = curr_card->form->items[i];
		if (!IN_DBASE(item->type) || item->nosort)
			continue;
		for (j=1; j < n; j++)
			if (item->column == sort_col[j])
				break;
		if (j < n)
			continue;
		sprintf(buf, "by %.200s", item->type==IT_CHOICE ? item->name
								: item->label);
		if (buf[j = strlen(buf)-1] == ':')
			buf[j] = 0;
		QString str(buf);
		str.replace('&', "&&");
		QAction *a = sortpulldown->addAction(str,
					[=](){sort_pulldown(item->column);});
		a->setCheckable(true);
		a->setChecked(item->column==pref.sortcol);
		sag->addAction(a);
		sort_col[n++] = item->column;
	}
	// tearoff already set above
}


/*-------------------------------------------------- switch databases -------*/
/*
 * switch main window to new database. This happens on startup if a form
 * name is given on the command line, and whenever a new form name is chosen
 * from the database pulldown. ".gf" is appended to formname if necessary. If
 * formname == 0, kill the current form and don't switch to a new one. Sort
 * the new database by the default sort criterion if there is one.
 */

void switch_form(
	char		*formname)	/* new form name */
{
	char		name[1024], *p;	/* capitalized formname */
	int		i;

	if (curr_card) {
		if (prev_form)
			free(prev_form);
		prev_form = 0;
		if (curr_card->form)
			prev_form = mystrdup(curr_card->form->name);
		destroy_card_menu(curr_card);
		if (curr_card->dbase) {
			if (curr_card->dbase->modified &&
			   !curr_card->dbase->rdonly &&
			   !curr_card->form->rdonly)
				if (!write_dbase(curr_card->dbase,
						 curr_card->form, FALSE))
					return;
			dbase_delete(curr_card->dbase);
			free((void *)curr_card->dbase);
		}
		if (curr_card->form) {
			form_delete(curr_card->form);
			free((void *)curr_card->form);
		}
		free((void *)curr_card);
		curr_card = 0;
	}
	if (formname && *formname) {
		FORM  *form  = form_create();
		DBASE *dbase = dbase_create();
		if (read_form(form, formname))
			(void)read_dbase(dbase, form,
					form->dbase ? form->dbase : formname);

		curr_card = create_card_menu(form, dbase, w_card);
		curr_card->form  = form;
		curr_card->dbase = dbase;
		curr_card->row   = 0;
		col_sorted_by    = 0;
		for (i=0; i < form->nitems; i++)
			if (form->items[i]->defsort) {
				pref.sortcol = form->items[i]->column;
				pref.revsort = FALSE;
				dbase_sort(curr_card, pref.sortcol, 0);
				break;
			}
		if (form->autoquery >= 0 && form->autoquery < form->nqueries)
			query_any(SM_SEARCH, curr_card,
				  form->query[form->autoquery].query);
		else
			query_all(curr_card);
		create_summary_menu(curr_card);

		strcpy(name, formname);
		if (*name >= 'a' && *name <= 'z')
			*name += 'A' - 'a';
		if (p = strrchr(name, '.'))
			*p = 0;
		if (p = strrchr(name, '/'))
			p++;
		else
			p = name;
		if (mainwindow) {
			mainwindow->setWindowIconText(p);
			fillout_card(curr_card, FALSE);
		}
	} else
		if (mainwindow)
			mainwindow->setWindowIconText("None");

	if (mainwindow) {
		w_left->setEnabled(formname != 0);
		w_right->setEnabled(formname != 0);
		w_del->setEnabled(formname != 0);
		w_dup->setEnabled(formname != 0);
		w_new->setEnabled(formname != 0);
		
		remake_query_pulldown();
		remake_sort_pulldown();
		remake_section_pulldown();	/* also sets w_sect, w_del */
		resize_mainwindow();
		if (curr_card && curr_card->dbase)
			curr_card->dbase->modified = FALSE;
		print_info_line();
	}
}


/*
 * find the next card in the summary that matches the search text widget
 * contents, and select it. (Ctrl-F)
 */

static void find_and_select(
	char		*string)	/* contents of search text widget */
{
	int		i, j = 0;	/* query count, query index */
	int		oldrow;		/* if search fails, stay put */

	if (!curr_card)
		return;
	oldrow = curr_card->row;
	card_readback_texts(curr_card, -1);
	for (i=0; i < curr_card->nquery; i++) {
		j = (curr_card->qcurr + i + 1) % curr_card->nquery;
		curr_card->row = curr_card->query[j];
		if (match_card(curr_card, string))
			break;
	}
	if (i == curr_card->nquery) {
		curr_card->row = oldrow;
		print_button(w_info, "No match.");
	} else {
		curr_card->qcurr = j;
		fillout_card(curr_card, FALSE);
		scroll_summary(curr_card);
		print_info_line();
	}
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * some item in one of the menu bar pulldowns was pressed. All of these
 * routines are direct X callbacks.
 */

static void rambo_quit(void) { exit(0); }

static void file_pulldown(
	int				item)
{
	card_readback_texts(curr_card, -1);
	switch (item) {
	  case 0: {						/* find&sel */
		char *string = qstrdup(w_search->text());
		if (*string)
			find_and_select(string);
		free(string);
		break; }

	  case 1:						/* print */
		create_print_popup();
		break;

	  case 2:						/* export */
		create_templ_popup();
		break;

	  case 3:						/* preference*/
		create_preference_popup();
		break;

	  case 5:						/* about */
		create_about_popup();
		break;

	  case 6:						/* save */
		if (curr_card && curr_card->form && curr_card->dbase)
			if (curr_card->form->rdonly)
				create_error_popup(mainwindow, 0,
				   "Database is marked read-only in the form");
			else
				(void)write_dbase(curr_card->dbase,
						  curr_card->form, TRUE);
		else
			create_error_popup(mainwindow,0,"No database to save");
		print_info_line();
		break;

	  case 7:						/* quit */
		if (curr_card &&  curr_card->dbase
			      &&  curr_card->dbase->modified
			      && !curr_card->form->rdonly
			      && !write_dbase(curr_card->dbase,
					      curr_card->form, FALSE))
			break;
		exit(0);

	  case 8:						/* rambo quit*/
		if (curr_card &&  curr_card->dbase
			      && !curr_card->dbase->rdonly
			      &&  curr_card->dbase->modified
			      && !curr_card->form->rdonly)
			create_query_popup(mainwindow, rambo_quit, "rambo",
				"OK to discard changes and quit?");
		else
			exit(0);
	}
}


static void newform_pulldown(
	int				item)
{
	card_readback_texts(curr_card, -1);
	switch (item) {
	  case 0:						/* current */
		if (curr_card && curr_card->form) {
			if (curr_card->dbase		&&
			   !curr_card->dbase->rdonly	&&
			    curr_card->dbase->modified	&&
			   !curr_card->form->rdonly	&&
			   !write_dbase(curr_card->dbase,
					curr_card->form, FALSE))
						return;
			create_formedit_window(curr_card->form, FALSE, FALSE);
		} else
			create_error_popup(mainwindow, 0,
		     "Please choose database to edit\nfrom Database pulldown");
		break;

	  case 1:						/* new */
		switch_form(0);
		create_formedit_window(0, FALSE, TRUE);
		break;

	  case 2:						/* clone */
		if (curr_card && curr_card->form) {
			if (curr_card->dbase		&&
			   !curr_card->dbase->rdonly	&&
			    curr_card->dbase->modified	&&
			   !curr_card->form->rdonly	&&
			   !write_dbase(curr_card->dbase,
					curr_card->form, FALSE))
						return;
			create_formedit_window(curr_card->form, TRUE, TRUE);
		} else
			create_error_popup(mainwindow, 0,
			"Please choose database from Database pulldown first");
		break;
	}
}


static void help_pulldown(
	int				item)
{
	switch (item) {
	  case 0:						/* context */
		QWhatsThis::enterWhatsThisMode();
		break;

	  case 1:						/* database */
		create_dbase_info_popup(curr_card);
		break;

	  case 2:						/* intro */
		help_callback(mainwindow, "intro");
		break;

	  case 3:						/* help */
		help_callback(mainwindow, "help");
		break;

	  case 4:						/* trouble */
		help_callback(mainwindow, "trouble");
		break;

	  case 5:						/* files */
		help_callback(mainwindow, "files");
		break;

	  case 6:						/* expr */
		help_callback(mainwindow, "grammar");
		break;

	  case 7:						/* resources */
		help_callback(mainwindow, "resources");
		break;
	}
}


static void dbase_pulldown(
	int				item)
{
	char				path[1024];

	card_readback_texts(curr_card, -1);
	sprintf(path, "%s/%s.gf", db[item].path, db[item].name+1);
	switch_form(path);
	remake_dbase_pulldown();
}


static void section_pulldown(
	int				item)
{
	card_readback_texts(curr_card, -1);
	if (item == curr_card->dbase->nsects+1 || item == MAXSC-1 ||
					!curr_card->dbase->havesects)
		create_newsect_popup();
	else {
		curr_card->dbase->currsect = defsection = item-1;
		if (pref.autoquery)
			do_query(curr_card->form->autoquery);
		else
			query_all(curr_card);

		create_summary_menu(curr_card);

		curr_card->row = curr_card->query ? curr_card->query[0]
						  : curr_card->dbase->nrows;
		fillout_card(curr_card, FALSE);
	}
}


static void query_pulldown(
	int		item,	/* -1: all, -2: autoquery */
	bool		set)
{
	print_info_line();
	if (!curr_card || !curr_card->dbase || !curr_card->dbase->nrows)
		return;
	card_readback_texts(curr_card, -1);
	if (item == -2) {
		pref.autoquery = set;
		item = curr_card->form->autoquery;
	} else if (pref.autoquery)
		curr_card->form->autoquery = item;

	do_query(item);

	create_summary_menu(curr_card);

	curr_card->row = curr_card->query ? curr_card->query[0]
					  : curr_card->dbase->nrows;
	fillout_card(curr_card, FALSE);
	remake_query_pulldown();
}


static void sort_pulldown(
	int		item,	/* -1 is reverse flag */
	bool		set)
{
	if (item < 0)
		pref.revsort = set;
	else
		pref.sortcol = item;

	card_readback_texts(curr_card, -1);
	dbase_sort(curr_card, pref.sortcol, pref.revsort);
	create_summary_menu(curr_card);
	curr_card->row = curr_card->query ? curr_card->query[0]
					  : curr_card->dbase->nrows;
	fillout_card(curr_card, FALSE);
	remake_sort_pulldown();
}


/*
 * perform a query: -1 = all cards; 0..nqueries-1 = execute specified query.
 * This is called from the pulldown above, and when store feels the need to
 * requery, if pref.autoquery is on.
 */

void do_query(
	int		qmode)		/* -1=all, or query number */
{
	register ROW	**row;		/* list of row struct pointers */
	register int	i;

	for (row=curr_card->dbase->row,i=curr_card->dbase->nrows; i; i--,row++)
		(*row)->selected = 0;
	if (curr_card->row < curr_card->dbase->nrows)
		curr_card->dbase->row[curr_card->row]->selected |= 4;
	if (curr_card->nquery)
		curr_card->dbase->row[curr_card->query[curr_card->qcurr]]->
								selected |= 2;
	if (qmode == -1)
		query_all(curr_card);

	else if (*curr_card->form->query[qmode].query == '/') {
		char *query = curr_card->form->query[qmode].query;
		char *string = strdup(query + 1);
		append_search_string(string);
		query_search(SM_SEARCH, curr_card, query+1);
	} else {
		char *query = curr_card->form->query[qmode].query;
		if (pref.query2search) {
			char *string = strdup(query);
			append_search_string(string);
		}
		query_eval(SM_SEARCH, curr_card, query);
	}
	for (row=curr_card->dbase->row,i=curr_card->dbase->nrows; i; i--,row++)
		if ((*row)->selected & 4)
			curr_card->row = curr_card->dbase->nrows - i;
	for (i=curr_card->nquery-1; i >= 0; i--)
		if (curr_card->query[i] == curr_card->row) {
			curr_card->qcurr = i;
			break;
		}
	last_query = qmode;
}



/*
 * search for a search string, or for matching expressions. Put matching
 * cards into the summary. The string is an expression if it begins with
 * '(' or '{', or a search string otherwise. This is called from the
 * switch statement in expressions too.
 */

void search_cards(
	Searchmode	mode,		/* search, narrow, widen, ... */
	CARD		*card,
	char		*string)
{
	if (!string || !*string)
		return;
	print_info_line();
	card_readback_texts(card, -1);
	if (!card || !card->dbase || !card->dbase->nrows)
		return;
	query_any(mode, card, string);
	create_summary_menu(card);
	card->row = card->nquery ? card->query[0] : card->dbase->nrows;
	fillout_card(card, FALSE);
}


/*
 * Clear the search string.
 */

static void clear_callback(void)
{
	print_text_button_s(w_search, "");
	w_search->setFocus();
}


/*
 * Searches. When a search string is entered, read it from the button and
 * search for it. The arrows put a string from the history array into the
 * button, where it can be searched for. The query pulldown can append a
 * string to the history if pref.query2search is on.
 */

static char	*history[NHIST];
static int	s_curr, s_offs;

static void mode_callback(
	int				item)	/* one of SM_* */
{
	searchmode = (Searchmode)item;
}


static void search_callback(
	int				inc)
{
	if (inc) {
		int o = s_offs + inc;
		if (o > -NHIST && o <= 0 && history[(s_curr + o) % NHIST]) {
			s_offs = o;
			print_text_button_s(w_search,
					history[(s_curr + o + NHIST) % NHIST]);
		}
		append_search_string(0);
	} else {
		char *string = qstrdup(w_search->text());
		if (searchmode != SM_FIND)
			search_cards(searchmode, curr_card, string);
		else if (curr_card->nquery > 0)
			find_and_select(string);
		append_search_string(string);
	}
}


static void requery_callback(void)
{
	if (last_query >= 0)
		query_pulldown(last_query);
}


static void append_search_string(
	char		*text)
{
	if (text) {
		print_text_button_s(w_search, text);
		s_offs = 0;
		if (history[s_curr] && *history[s_curr]
				    && strcmp(history[s_curr], text))
			s_curr = (s_curr + 1) % NHIST;
		if (history[s_curr])
			free(history[s_curr]);
		history[s_curr] = text;
	}
	w_next->setEnabled(s_offs < 0);
	w_prev->setEnabled(s_offs > -NHIST+1 &&
				history[(s_curr + s_offs + NHIST -1) % NHIST]);
}


/*
 * One of the letter buttons (a..z, misc, all) was pressed. Do the appropriate
 * search and display the result in the summary.
 */

static void letter_callback(
	int				letter)
{
	if (!curr_card || !curr_card->dbase || !curr_card->dbase->nrows)
		return;
	card_readback_texts(curr_card, -1);
	query_letter(curr_card, letter);
	create_summary_menu(curr_card);
	curr_card->row = curr_card->query ? curr_card->query[0]
					  : curr_card->dbase->nrows;
	fillout_card(curr_card, FALSE);
}


/*
 * go forward or backward in the summary list if there is one. If there is
 * no summary, we must be adding cards and the user wants to see previously
 * added cards; step in the raw database in this case.
 */

static void pos_callback(
	int				inc)
{
	register CARD			*card = curr_card;

	card_readback_texts(curr_card, -1);
	if (card->qcurr + inc >= 0 &&
	    card->qcurr + inc <  card->nquery) {
		card->qcurr += inc;
		card->row    = card->query[card->qcurr];
		fillout_card(card, FALSE);
		scroll_summary(card);
		print_info_line();

	} else if (card->nquery == 0 && card->row + inc >= 0
				     && card->row + inc <  card->dbase->nrows){
		card->row += inc;
		fillout_card(card, FALSE);
		print_info_line();
	}
}


/*
 * add a new card to the database.
 * When finished, find the first text input item and put the cursor into it.
 */

static void add_card(
	BOOL		dup)
{
	register CARD	*card = curr_card;
	register ITEM	*item;
	register DBASE	*dbase = card->dbase;
	int		*newq;
	int		i, s, save_sect = dbase->currsect;
	int		oldrow = card->row;

	card_readback_texts(card, -1);
	s = defsection < dbase->nsects ? defsection : 0;
	if (dbase->sect[s].rdonly) {
		create_error_popup(mainwindow, 0,
					"Section file\n\"%s\" is read-only",
					dbase->sect[s].path);
		return;
	}
	dbase->currsect = s;
	if (!dbase_addrow(&card->row, dbase)) {
		dbase->currsect = save_sect;
		create_error_popup(mainwindow, errno,
					"No memory for new database row");
		return;
	}
	dbase->currsect = save_sect;
	if (dup)
		for (i=0; i < dbase->maxcolumns; i++)
			dbase_put(dbase, card->row, i,
				mystrdup(dbase_get(dbase, oldrow, i)));
	else
		for (i=0; i < card->form->nitems; i++, item++) {
			item = card->form->items[i];
			if (IN_DBASE(item->type) && item->idefault)
				dbase_put(dbase, card->row, item->column,
				     mystrdup(evaluate(card, item->idefault)));
		}
	if (card->qcurr = card->nquery)
		if (!(newq = (int *)realloc(card->query, card->dbase->nrows *
								sizeof(int*))))
			create_error_popup(mainwindow, errno,
							"No memory for query");
		else {
			card->query = newq;
			card->query[card->nquery++] = card->row;
		}
	dbase_sort(card, pref.sortcol, pref.revsort);
	create_summary_menu(card);
	fillout_card(card, FALSE);
	scroll_summary(card);
	print_info_line();
	for (i=0; i < card->form->nitems; i++, item++) {
		item = card->form->items[i];
		if (item->type == IT_INPUT || item->type == IT_TIME
					   || item->type == IT_NOTE) {
			card->items[i].w0->setFocus();
			break;
		}
	}
}


static void new_callback(void)
{
	add_card(FALSE);
	if (pref.autoquery) {
		pref.autoquery = FALSE;
		print_button(w_info, "Autoquery in Query pulldown disabled");
		remake_query_pulldown();
	}
}


/*
 * duplicate a card. This does the same as New, but fills the card with
 * the same data as the current card.
 */

static void dup_callback(void)
{
	if (curr_card->row >= 0)
		add_card(TRUE);
}


/*
 * delete a card. Since the summary should not be deleted, the deleted card
 * must be removed from the query[] list. Since the query list contains the
 * row numbers of the cards it lists, and rows may get renumbered if one is
 * removed, row indices > deleted row must be decremented.
 */

static void del_callback(void)
{
	register CARD			*card = curr_card;
	register int			*p, *q;
	int				i, s;

	if (card->dbase->nrows == 0 || card->row >= card->dbase->nrows)
		return;
	s = card->dbase->row[card->row]->section;
	if (card->dbase->sect[s].rdonly) {
		create_error_popup(mainwindow, 0,
				"section file\n\"%s\" is read-only",
				card->dbase->sect[s].path);
		return;
	}
	dbase_delrow(card->row, card->dbase);
	if (card->row >= card->dbase->nrows)
		card->row = card->dbase->nrows - 1;
	p = q = &card->query[0];
	for (i=0; i < card->nquery; i++, p++) {
		*q = *p - (*p > card->row);
		q += *p != card->row;
	}
	card->nquery -= p - q;
	print_info_line();
	fillout_card(card, FALSE);
	create_summary_menu(card);
}


/*
 * assign new section to the current card (called from the option popup)
 */

static void sect_callback(
	int				item)
{
	register CARD			*card = curr_card;
	register SECTION		*sect = card->dbase->sect;
	int				olds, news;

	olds = card->dbase->row[card->row]->section;
	news = item;
	if (sect[olds].rdonly)
		create_error_popup(mainwindow, 0,
			"section file\n\"%s\" is read-only", sect[olds].path);
	else if (sect[news].rdonly)
		create_error_popup(mainwindow, 0,
			"section file\n\"%s\" is read-only", sect[news].path);
	else {
		sect[olds].nrows--;
		sect[news].nrows++;
		sect[olds].modified = TRUE;
		sect[news].modified = TRUE;
		card->dbase->row[card->row]->section = defsection = news;
		card->dbase->modified = TRUE;
		print_info_line();
	}
	remake_section_popup(FALSE);
}
