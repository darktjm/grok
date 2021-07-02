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
#include <dirent.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define OFF		16		/* margin around summary and card */
#define NHIST		20		/* remember this many search strings */

#ifndef LIB
#define LIB "/usr/local/lib"
#endif

enum file_menu {
    FM_FINDSEL, FM_PRINT, FM_EXPORT, FM_PREFS, FM_CHECKREF, FM_SAVE, FM_REVERT,
    FM_QUIT, FM_SAVEALL
};

enum formed_menu {
    NFM_CURRENT, NFM_NEW, NFM_DUP
};

#define QM_AUTO -2
#define QM_ALL  -1

static void file_pulldown	(enum file_menu);
static void formed_pulldown	(enum formed_menu);
static void dbase_pulldown	(int);
static void section_pulldown	(int);
static void mode_callback	(Searchmode);
static void sect_callback	(int);
static void query_pulldown	(int item, bool set = false);
static void sort_pulldown	(int item, bool set = false);
static void search_callback	(int);
static void requery_callback	(void);
static void clear_callback	(void);
static void letter_callback	(int);
static void pos_callback	(int);
static void new_callback	(void);
static void dup_callback	(void);
static void del_callback	(void);
static bool multi_save_revert	(bool);
static void check_references	(void);

/*
 * The following is part one in a fix for a bug that had been plaguing
 * grok since 2.0:  the main window is supposed to shrink to fit the
 * contents when starting up or switching to new databases, but instead it
 * would only do that part of the time.  The rest of the time, the window
 * was too big (often double or triple width, and 40%+ extra height).
 *
 * The underlying cause is both my misunderstanding of the function I was
 * using (it is, after all, my first ever Qt project) and bizarre, possibly
 * xcb/X11/fvwm-related behavior.   The function I call is adjustSize(),
 * which I took to be similar to tk's pack function, to shrink the window
 * to fit its contents.  The shrinking actually doesn't happen.  So, to
 * fix that, I added a resize(minimumSize()) right after adjustSize().  This
 * always works on startup, but randomly fails on switches.  It turns out
 * that this is due to the fact that adjustSize()'s resize and the subsequent
 * explicit resize were being executed in a random order.  I still don't
 * know what causes this random order yet, but it needed to be fixed.
 *
 * My fix involves making the adjustSize() resize work correctly to begin
 * with.  Qt offers only one "shrink to fit" option:  shrink to fit
 * the sizeHint (but not the minimumSizeHint).  Thus the following causes
 * the central widget to always return its minimum size as a size hint
 * rather than some random value deteremined by its current size and
 * whether or not the letter buttons are there.  More on the fix below,
 * in resize_mainwindow().
 */

class MinWidget : public QWidget {
    public:
	QSize sizeHint() const { QWidget::sizeHint(); return minimumSizeHint(); }
};

GrokMainWindow		*mainwindow;	/* popup menus hang off main window */
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
static QWidget		*w_card;	/* form for card */
static QPushButton	*w_left;	/* button: previous card */
static QPushButton	*w_right;	/* button: next card */
static QPushButton	*w_new;		/* button: start a new card */
static QPushButton	*w_dup;		/* button: duplicate a card */
static QPushButton	*w_del;		/* button: delete current card */
static QComboBox	*w_sect;	/* button: section popup for card */
static int		defsection;	/* default section */

static Searchmode	searchmode = SM_FIND;	/* current search mode */
static const char	* const modename[] = { "All", "In Query", "Narrow", "Widen",
					"Widen in Query", "Find && Select" };


/*
 * create the main window with a menu bar and empty forms for the summary
 * and the card. The forms will be filled later. This routine is called
 * once during startup, before the first switch_form().
 */

void create_mainwindow()
{
	QWidget		*w;
	long		i;
	char		buf[10];
	QMenuBar	*menubar;
	QMenu		*menu, *submenu;
	QVBoxLayout	*mainform;	/* form for summary table */

	mainwindow = new GrokMainWindow();
	set_icon(mainwindow, 0); // from main(); there is no separate "toplevel"

							/*-- menu bar --*/
	menubar = mainwindow->menuBar();
	menu = menubar->addMenu("&File");
	menu->setTearOffEnabled(true);
	bind_help(menu, "pd_file");
	menu->addAction("&Find && select", [=](){file_pulldown(FM_FINDSEL);},
			Qt::CTRL|Qt::Key_F);
	menu->addAction("&Check References", [=](){file_pulldown(FM_CHECKREF);});
	menu->addAction("&Print...", [=](){file_pulldown(FM_PRINT);},
			Qt::CTRL|Qt::Key_P);
	menu->addAction("&Export...", [=](){file_pulldown(FM_EXPORT);},
			Qt::CTRL|Qt::Key_E);
	menu->addAction("P&references...", [=](){file_pulldown(FM_PREFS);},
			Qt::CTRL|Qt::Key_R);
	submenu = menu->addMenu("F&orm Editor");
	if (restricted)
		submenu->setEnabled(false);
	submenu->setTearOffEnabled(true);
	bind_help(submenu, "pd_file");
	submenu->addAction("&Edit current form...", [=](){formed_pulldown(NFM_CURRENT);});
	submenu->addAction("&Create new form from scratch...", [=](){formed_pulldown(NFM_NEW);});
	submenu->addAction("Create, use current as &template...", [=](){formed_pulldown(NFM_DUP);});
	menu->addAction("&Save", [=](){file_pulldown(FM_SAVE);},
			Qt::CTRL|Qt::Key_S);
	menu->addAction("Save &All", [=](){file_pulldown(FM_SAVEALL);},
			Qt::CTRL|Qt::SHIFT|Qt::Key_S);
	menu->addAction("Re&vert/Save...", [=](){file_pulldown(FM_REVERT);},
			Qt::CTRL|Qt::SHIFT|Qt::Key_R);
	menu->addAction("&Quit", [=](){file_pulldown(FM_QUIT);},
			Qt::CTRL|Qt::Key_Q);
	
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

	menu->addAction("&About...", [=](){create_about_popup();});
	menu->addAction("On &context", [=](){QWhatsThis::enterWhatsThisMode();},
			Qt::CTRL|Qt::Key_H);
	menu->addAction("Current &database", [=](){create_dbase_info_popup(mainwindow->card);},
			Qt::CTRL|Qt::Key_D);
	menu->addSeparator();
	menu->addAction("&Introduction", [=](){help_callback(mainwindow, "intro");});
	menu->addAction("&Getting help", [=](){help_callback(mainwindow, "help");});
	menu->addAction("&Troubleshooting", [=](){help_callback(mainwindow, "trouble");});
	menu->addAction("&Files and programs", [=](){help_callback(mainwindow, "files");});
	menu->addAction("&Expression grammar", [=](){help_callback(mainwindow, "grammar");},
			Qt::CTRL|Qt::Key_G);
	menu->addAction("En&vironment and QSS", [=](){help_callback(mainwindow, "resources");});

	w = new MinWidget;
	mainwindow->setCentralWidget(w);
	w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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

	w_search = new QLineEdit;
	w_search->sizePolicy().setHorizontalStretch(1);
	hb->addWidget(w_search);
	set_textr_cb(w_search, search_callback(0));
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
	mainform->addWidget(create_summary_widget());

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
	  const QMargins &m = w->contentsMargins();
	  w->setContentsMargins(2, 2, m.top(), m.bottom());
	  hb->addWidget(w);
	  set_button_cb(w, letter_callback(i));
	  bind_help(w, "letters");
	 }
	}
							/*-- card --*/
	// this isn't really necessary, since the card is in a frame
	mainform->addWidget(mk_separator());

	w_card = new QWidget;
	w_card->setFixedSize(400, 6);
	w_card->setObjectName("cardform");
	bind_help(w_card, "card");
	mainform->addWidget(w_card);

	mainform->addLayout(bb); // needs to be last so it's on bottom

	create_summary_menu(0);
	remake_dbase_pulldown();
	remake_section_pulldown();
	remake_query_pulldown();
	remake_sort_pulldown();
}


/*
 * resize the main window such that there is enough room for the summary
 * and the card, and no more.
 */

void resize_mainwindow(void)
{
	/*
	 * This used to be one line:  mainwindow->adjustSize().  Later,
	 * I added a second line when I realized that adjustSize doesn't
	 * shrink to fit:  mainwindow->resize(mainwindow->minimumSize()).
	 * This worked well enough most of the time, but still randomly
	 * screwed up, because the resizes were happinging in a random
	 * order.  Instead of trying to figure out and fix that issue,
	 * I ensure that adjustSize() gets the size right the first time
	 * around.  Part of its problem was that the central widget was
	 * reporting a weird size sometimes, so I now run adjustSize on it
	 * (which doesn't trigger a main window resize, so it's safe).
	 */
	QWidget *c = mainwindow->centralWidget();
	c->adjustSize();
	c->updateGeometry();  // probably not useful, and possibly unsafe
	/*
	 * I then ensure that the main window will shrink to fit by using
	 * the layout's SetFixedSize constraint, which forces resizing to
	 * sizeHint.  The sizeHint for the central widget is still too big,
	 * so I override it above by returning minimumSizeHint, instead.
	 * After shrinking to fit, SetFixedSize disallows all resizing,
	 * so the window becomes non-resizable.  That was the original
	 * intention in the Motif version, but I prefer allowing the
	 * user to stretch the width for a wider search text widget and
	 * to stretch the height for more visible rows (rather than requiring
	 * the user to set the # of rows preference every time).
	 */
	QLayout *l = mainwindow->layout();
	l->setSizeConstraint(QLayout::SetFixedSize);

	mainwindow->adjustSize();

	/* Now that the window is the correct size, make it resizable again */
	l->setSizeConstraint(QLayout::SetDefaultConstraint);
	mainwindow->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

	/*
	 * Now, assuming Qt doesn't randomly revert the above 2 lines,
	 * everything is as planned.  I changed databases a lot and never
	 * saw it fail.  If I ever do, I'll have to come up with another
	 * fix.  <sigh> I hate Qt.
	 */
}


/*
 * print some enlightening info about the database into the info line between
 * the search string and the summary. This is called whenever something
 * changes: either the database was modified, or saved (modified is turned
 * off), or a search is done.
 */

void print_info_line()
{
	char		buf[128];
	const CARD	*card;
	const DBASE	*dbase;
	int		s, n;

	if (!mainwindow)
		return;
	card = mainwindow->card;
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
						mktimestring(c, false));
			if (t && t != c)
				sprintf(buf+strlen(buf), "%schanged %s %s",
						c ? ", " : "",
						mkdatestring(t),
						mktimestring(t, false));
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
		mainwindow->setWindowModified(dbase->modified);
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
static int ndb;

static int append_to_dbase_list(
	long		nlines,		/* # of databases already in pulldown*/
	const char	*path,		/* directory name */
	int		order)		/* "chapter" number for sorting */
{
	static char	*name = 0;	/* tmp buffer for displayed name */
	static size_t	namelen;
	char		*p;		/* for removing extension */
	DIR		*dir;		/* open directory file */
	struct dirent	*dp;		/* one directory entry */
	int		num=0, i;

	grow(0, "database pulldown", char, name, 80, &namelen);
	name[0] = order + '0';
	path = canonicalize(resolve_tilde(path, 0), false);
	/* may as well check for path dup right here to skip dir read */
	/* FIXME: ugh.  need to insert into db using binary insertion */
	/* and then find dups using binary search. same in loop below */
	for(i = 0; i < nlines; i++)
		if(db[i].path && !strcmp(db[i].path, path))
			return nlines;
	if (!(dir = opendir(path)))
		return(nlines);
	while (nlines < MAXD-!num) {
		if (!(dp = readdir(dir)))
			break;
		if (!(p = strrchr(dp->d_name, '.')) || strcmp(p, ".gf"))
			continue;
		*p = 0;
		for (i=0; i < nlines; i++)
			/* already eliminated same-path above */
			if (db[i].name && pref.uniquedb &&
			    !strcmp(db[i].name+1, dp->d_name))
			    	break;
		if (i < nlines)
			continue;
		if (!num++ && nlines) {
			name[1] = 0;
			db[nlines].path = 0;
			db[nlines++].name = mystrdup(name);
		}
		grow(0, "database pulldown", char, name, strlen(dp->d_name) + 2, &namelen);
		strcpy(name + 1, dp->d_name);
		db[nlines].name = mystrdup(name);
		db[nlines].path = mystrdup(path);
		nlines++;
	}
	(void)closedir(dir);
	return(nlines);
}


static void make_dbase_pulldown(void)
{
	int		i;
	static QActionGroup *ag = 0;

	dbpulldown->clear();
	if(!ag)
		ag = new QActionGroup(dbpulldown);
	for (i=0; i < ndb; i++) {
		if (!db[i].path) {
			dbpulldown->addSeparator();
			continue;
		}
		bool loaded = false, shown = false, mod = false;
		for(FORM *form = form_list; form; form = form->next)
			if(!strcmp(form->dir, db[i].path)) {
				const char *n = strrchr(form->path, '/');
				const char *ext = strrchr(form->path, '.');
				if(!ext || strcmp(ext, ".gf"))
					continue;
				if(memcmp(n + 1, db[i].name + 1, ext - n - 1) ||
				   db[i].name[ext - n])
					continue;
				loaded = true;
				if(mainwindow->card && mainwindow->card->form == form)
					shown = true;
				for(DBASE *dbase = dbase_list; dbase; dbase = dbase->next)
					if(dbase->form == form) {
						mod = dbase->modified;
						break;
					}
				break;
			}
		QString n(db[i].name+1);
		n.replace('&', "&&");
		if(mod)
			n.append('*');
		QAction *act = dbpulldown->addAction(n, [=](){dbase_pulldown(i);});
		ag->addAction(act);
		act->setCheckable(true);
		act->setChecked(shown);
		if(loaded) {
			QFont f = act->font();
			f.setBold(true);
			act->setFont(f);
		}
	}
}


static int compare_db(
	const void		*u,
	const void		*v)
{
	return(strcmp(((struct db *)u)->name, ((struct db *)v)->name));
}


void remake_dbase_pulldown(void)
{
	int		n;
	const char	*env;

	for (n=0; n < MAXD; n++) {
		zfree(db[n].path);
		zfree(db[n].name);
		db[n].name = 0;
		db[n].path = 0;
	}
	env = getenv("GROK_FORM");
	n = append_to_dbase_list(0, env ? env : "./", 0);
	n = append_to_dbase_list(n, "./grokdir", 1);
	n = append_to_dbase_list(n, GROKDIR, 2);
	n = append_to_dbase_list(n, LIB "/grokdir", 3);
	qsort(db, n, sizeof(struct db), compare_db);
	ndb = n;
	make_dbase_pulldown();
	// tearoff already enabled above
}


void add_dbase_list(QStringList &l)
{
	int		n;
	char		*gd = mystrdup(canonicalize(resolve_tilde(GROKDIR, 0), 0));
	char		*ld = mystrdup(canonicalize(resolve_tilde(LIB "/grokdir", 0), 0));

	for (n=0; n < ndb; n++) {
		if (!db[n].path)
			continue;
		const char *p = canonicalize(db[n].path, false);
		if (!strcmp(p, gd) || !strcmp(p, ld))
			l.append(db[n].name + 1);
		else
			l.append(QString(p) + '/' + (db[n].name + 1));
	}
	free(gd);
	free(ld);
}


/*
 * After a database was loaded, there is a section list in the dbase struct.
 * Present it in a pulldown if there are at least two sections.
 */

#define MAXSC  200		/* no more than 200 sections in pulldown */

void remake_section_pulldown()
{
	CARD		*card = mainwindow->card;
	int		maxn;
	long		n;

	sectpulldown->clear();
	if (!card || !card->dbase || card->form->proc) {
		sectpulldown->setEnabled(false);
		return;
	}
	maxn = card->dbase->havesects ? card->dbase->nsects : 0;
	if (maxn > MAXSC - 2)
		maxn = MAXSC - 2;
	if(maxn)
		sectpulldown->addAction("All", [=](){section_pulldown(0);});
	for (n=0; n < maxn; n++) {
		QString s(section_name(card->dbase, n));
		s.replace('&', "&&");
		sectpulldown->addAction(s, [=](){section_pulldown(n + 1);});
	}
	sectpulldown->addAction("New ...", [=](){section_pulldown(n + 1);});
	sectpulldown->setEnabled(true);
	// teearoff already enabled above
	remake_section_popup(true);
}


/*
 * Put all the default queries in the form into the Query pulldown (pulldown
 * #3). Add a default query "All" at the top. This is yet another violation
 * of the Motif style guide. So what.
 */

#define MAXQ	100		/* no more than 100 queries in pulldown */
static QActionGroup	*qag = 0;	/* for radio-like queries */
static QActionGroup	*smag = 0;	/* for search mode */

void remake_query_pulldown(void)
{
	CARD		*card = mainwindow->card;
	long		i;		/* # of queries in pulldown */
	int		n;		/* max # of lines in pulldown */

	qpulldown->clear();
	if(qag) {
		delete qag;
		qag = 0;
	}
	if(smag) {
		delete smag;
		smag = 0;
	}
	if (!card || !card->form)
		return;

	QMenu *submenu = qpulldown->addMenu("Search &Mode");
	QAction *aq;
	submenu->setTearOffEnabled(true);
	bind_help(submenu, "search");
	smag = new QActionGroup(submenu);
	for (i=0; i < SM_NMODES; i++) {
		aq = submenu->addAction(modename[i],
					[=](){mode_callback((Searchmode)i);});
		aq->setCheckable(true);
		aq->setChecked(i == searchmode);
		smag->addAction(aq);
	}

	aq = qpulldown->addAction("Autoquery",
				   [=](bool c){query_pulldown(QM_AUTO, c);});
	aq->setCheckable(true);
	aq->setChecked(pref.autoquery);

	if (pref.autoquery)
		qag = new QActionGroup(qpulldown);

	n = card->form->nqueries > MAXQ-2 ? MAXQ-2
					       : card->form->nqueries;
	for (i=0; i <= n; i++) {
		DQUERY *dq = i ? &card->form->query[i-1] : 0;
		if (i && (dq->suspended || !dq->name || !dq->query))
			continue;
		QString name(i ? dq->name : "All");
		name.replace('&', "&&");
		if (pref.autoquery) {
			QAction *a = qpulldown->addAction(name,
					 [=](){query_pulldown(i-1);});
			a->setCheckable(true);
			a->setChecked(card->form->autoquery == i-1);
			qag->addAction(a);
		} else {
			qpulldown->addAction(name, [=](){query_pulldown(i-1);});
		}
	}
	// tearoff already set above
	card->last_query = card->form->autoquery;
}


/*
 * put a new option popup menu on the card's Section button
 */

static void	remake_popup(void);

void remake_section_popup(
	bool		newsects)	/* did the section list change? */
{
	CARD		*card = mainwindow->card;

	if (newsects)
		remake_popup();

	if (!card	|| !card->dbase
			||  card->row < 0
			||  card->row >= card->dbase->nrows
			|| !card->dbase->havesects) {
		if (w_sect->isVisible()) {
			w_sect->setEnabled(false);
			// I've no idea what this is doing here:
			w_del->setEnabled(false);
		}
		return;
	}
	if (w_sect->isVisible()) {
		printf("%d\n", card->dbase->row[card->row]->section);
		w_sect->setEnabled(true);
		w_sect->setCurrentIndex(
			card->dbase->row[card->row]->section);
		// I've no idea what this is doing here
		w_del->setEnabled(true);
	}
}


static void remake_popup(void)
{
	CARD		*card = mainwindow->card;
	int		i, n;

	w_sect->clear();
	w_sect->hide();
	n = card->dbase->nsects;
	if (n < 2)
		return;

	for (i=0; i < n; i++) {
		QString str(section_name(card->dbase, i));
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
	CARD		*card = mainwindow->card;
	int	sort_col[2*MAXS+1];	/* column for each pulldown item */
	char		buf[128];	/* pulldown line text */
	ITEM		*item;		/* scan items for sortable columns */
	int		i;		/* item counter */
	int		j;		/* skip redundant choice items */
	int		n;		/* # of lines in pulldown */

	sortpulldown->clear();
	if(sag) {
		delete sag;
		sag = 0;
	}

	if (!card || !card->form || !card->dbase)
		return;

	QAction *rs = sortpulldown->addAction("Reverse sort",
					      [=](bool c){sort_pulldown(-1, c);});
	rs->setCheckable(true);
	rs->setChecked(pref.revsort);
	sag = new QActionGroup(mainwindow);

	for (n=1, i=0; i < card->form->nitems; i++) {
		item = card->form->items[i];
		if (!IN_DBASE(item->type) || IFL(item->,NOSORT))
			continue;
		for (j=1; j < n; j++)
			if (item->column == sort_col[j])
				break;
		if (j < n)
			continue;
		sprintf(buf, "by %.100s",
			(item->type==IT_CHOICE || !item->label) ? item->name
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
	CARD		*&card,		/* card to switch */
	char		*formname)	/* new form name */
{
	int		i;
	char		*prev_form = 0;

	if(mainwindow)
		mainwindow->setUpdatesEnabled(false);
	if (card) {
		DBASE		*odbase = card->dbase;
		FORM		*oform = card->form;
		card_readback_texts(card, -1);
		destroy_card_menu(card);
		if(oform)
			prev_form = mystrdup(oform->name);
		free_card(card);
		form_delete(oform);
		dbase_delete(odbase);
		card = 0;
	}
	if (!BLANK(formname)) {
		FORM  *form;
		DBASE *dbase = NULL;
		if ((form = read_form(formname)))
			dbase = read_dbase(form);
		/* old code always created form & dbase, even if invalid */
		/* so for now, this code does, too */
		if (!form) {
			form = form_create();
			form->name = strdup(formname);
			form->dbase = strdup(formname);
		}
		if (!dbase)
			dbase = dbase_create(form);
		form->dbpath = dbase->path;

		card = create_card_menu(form, dbase, w_card, !mainwindow);
		if(!card)
			fatal("No memory for initial form");
		card->form  = form;
		card->prev_form = prev_form;
		card->dbase = dbase;
		card->row   = 0;
		card->last_query = -1;
		card->col_sorted_by = 0;
		for (i=0; i < form->nitems; i++)
			if (IFL(form->items[i]->,DEFSORT)) {
				pref.sortcol = form->items[i]->column;
				pref.revsort = false;
				dbase_sort(card, pref.sortcol, 0);
				break;
			}
		if (form->autoquery >= 0 && form->autoquery < form->nqueries)
			query_any(SM_SEARCH, card,
				  form->query[form->autoquery].query);
		else
			query_all(card);
		create_summary_menu(card);

		if (mainwindow) {
			QString name(formname);
			if ((i = name.lastIndexOf('.')) > 0)
				name.truncate(i);
			if ((i = name.lastIndexOf('/')) >= 0)
				name.remove(0, i + 1);
			/* QString doesn't have * */
			if (name[0] >= 'a' && name[0] <= 'z')
				/* QCharRef can't be manipulated as an int */
				name[0] = name[0].toUpper();
			mainwindow->setWindowIconText(name);
			name.append(" - grok [*]");
			mainwindow->setWindowTitle(name);
			fillout_card(card, false);
		}
	} else {
		/* unlike old code, prev_form is now lost */
		/* in a way, this makes sense, since prev_form is only */
		/* used in expressions, which are really only valid when */
		/* a form is loaded, and when that eventually happens, */
		/* prev_form will have been NULL< anyway */
		zfree(prev_form);
		if (mainwindow) {
			mainwindow->setWindowIconText("None");
			mainwindow->setWindowTitle("grok");
		}
	}

	if (mainwindow) {
		w_left->setEnabled(formname != 0);
		w_right->setEnabled(formname != 0);
		w_del->setEnabled(formname != 0);
		w_dup->setEnabled(formname != 0);
		w_new->setEnabled(formname != 0);
		
		remake_query_pulldown();
		remake_sort_pulldown();
		remake_section_pulldown();	/* also sets w_sect, w_del */
		print_info_line();
		resize_mainwindow();
		make_dbase_pulldown();
		mainwindow->setUpdatesEnabled(true);
	}
}


/*
 * find the next card in the summary that matches the search text widget
 * contents, and select it. (Ctrl-F)
 */

void find_and_select(
	char		*string)	/* contents of search text widget */
{
	CARD				*card = mainwindow->card;
	int		i, j = 0;	/* query count, query index */
	int		oldrow;		/* if search fails, stay put */

	if (!card)
		return;
	oldrow = card->row;
	card_readback_texts(card, -1);
	for (i=0; i < card->nquery; i++) {
		j = (card->qcurr + i + 1) % card->nquery;
		card->row = card->query[j];
		if (match_card(card, string))
			break;
	}
	if (i == card->nquery) {
		card->row = oldrow;
		print_button(w_info, "No match.");
	} else {
		card->qcurr = j;
		fillout_card(card, false);
		scroll_summary(card);
		print_info_line();
	}
}


/*-------------------------------------------------- callbacks --------------*/
/*
 * some item in one of the menu bar pulldowns was pressed. All of these
 * routines are direct X callbacks.
 */

static void file_pulldown(
	enum file_menu			item)
{
	CARD				*card = mainwindow->card;
	card_readback_texts(card, -1);
	switch (item) {
	  case FM_FINDSEL: {					/* find&sel */
		char *string = qstrdup(w_search->text());
		if (string) {
			find_and_select(string);
			free(string);
		}
		break; }

	  case FM_PRINT:					/* print */
		create_print_popup(card);
		break;

	  case FM_EXPORT:					/* export */
		create_templ_popup(card);
		break;

	  case FM_PREFS:					/* preference*/
		create_preference_popup();
		break;

	  case FM_CHECKREF:					/* checkrefs */
		check_references();
		break;

	  case FM_SAVE:						/* save */
		if (card && card->form && card->dbase) {
			if (card->form->rdonly)
				create_error_popup(mainwindow, 0,
				   "Database is marked read-only in the form");
			else {
				(void)write_dbase(card->dbase, true);
				/* formerly in write_dbase() */
				print_info_line();
			}
		} else
			create_error_popup(mainwindow,0,"No database to save");
		print_info_line();
		break;

	  case FM_QUIT:						/* quit*/
		if (multi_save_revert(true))
			exit(0);
		break;

	 case FM_SAVEALL:					/* save all */
		for (DBASE *db = dbase_list; db; db = db->next)
			if (db->modified)
				write_dbase(db, true);
		/* formerly in write_dbase() */
		print_info_line();
		break;

	 case FM_REVERT:					/* Revert */
		multi_save_revert(false);
		break;
	}
}


static void formed_pulldown(
	enum formed_menu			item)
{
	CARD				*card = mainwindow->card;
	card_readback_texts(card, -1);
	switch (item) {
	  case 0:						/* current */
		if (card && card->form)
			create_formedit_window(card->form, false);
		else
			create_error_popup(mainwindow, 0,
		     "Please choose database to edit\nfrom Database pulldown");
		break;

	  case 1:						/* new */
		switch_form(mainwindow->card, 0);
		create_formedit_window(0, false);
		break;

	  case 2:						/* clone */
		if (card && card->form)
			create_formedit_window(card->form, true);
		else
			create_error_popup(mainwindow, 0,
			"Please choose database from Database pulldown first");
		break;
	}
}


static void dbase_pulldown(
	int			item)
{
	CARD			*card = mainwindow->card;
	static char		*path = 0;
	static size_t		pathlen;

	card_readback_texts(card, -1);
	grow(0, "dbase switch", char, path,
	     strlen(db[item].path) + strlen(db[item].name+1) + 5, &pathlen);
	sprintf(path, "%s/%s.gf", db[item].path, db[item].name+1);
	switch_form(mainwindow->card, path);
}


static void section_pulldown(
	int				item)
{
	CARD				*card = mainwindow->card;
	card_readback_texts(card, -1);
	if (item == card->dbase->nsects+1 || item == MAXSC-1 ||
					!card->dbase->havesects)
		create_newsect_popup(card);
	else {
		card->dbase->currsect = defsection = item-1;
		if (pref.autoquery)
			do_query(card->form->autoquery);
		else
			query_all(card);

		create_summary_menu(card);

		card->row = card->query ? card->query[0]
						  : card->dbase->nrows;
		fillout_card(card, false);
	}
}


static void query_pulldown(
	int		item,	/* -1: all, -2: autoquery */
	bool		set)
{
	CARD		*card = mainwindow->card;
	print_info_line();
	if (!card || !card->dbase || !card->dbase->nrows)
		return;
	card_readback_texts(card, -1);
	if (item == QM_AUTO) {
		pref.autoquery = set;
		item = card->form->autoquery;
	} else if (pref.autoquery)
		card->form->autoquery = item;

	do_query(item);

	create_summary_menu(card);

	card->row = card->query ? card->query[0]
					  : card->dbase->nrows;
	fillout_card(card, false);
	remake_query_pulldown();
}


static void sort_pulldown(
	int		item,	/* -1 is reverse flag */
	bool		set)
{
	CARD		*card = mainwindow->card;
	if (item < 0)
		pref.revsort = set;
	else
		pref.sortcol = item;

	card_readback_texts(card, -1);
	dbase_sort(card, pref.sortcol, pref.revsort);
	create_summary_menu(card);
	card->row = card->query ? card->query[0]
					  : card->dbase->nrows;
	fillout_card(card, false);
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
	CARD		*card = mainwindow->card;
	int		i;

	if (qmode == -1)
		query_all(card);

	else if (*card->form->query[qmode].query == '/') {
		char *query = card->form->query[qmode].query;
		char *string = mystrdup(query + 1);
		append_search_string(string);
		query_search(SM_SEARCH, card, query+1);
	} else {
		char *query = card->form->query[qmode].query;
		if (pref.query2search) {
			char *string = mystrdup(query);
			append_search_string(string);
		}
		query_eval(SM_SEARCH, card, query);
	}
	for (i=card->nquery-1; i >= 0; i--)
		if (card->query[i] == card->row) {
			card->qcurr = i;
			break;
		}
	card->last_query = qmode;
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
	fillout_card(card, false);
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
	Searchmode			item)	/* one of SM_* */
{
	searchmode = (Searchmode)item;
}


static void search_callback(
	int				inc)
{
	CARD				*card = mainwindow->card;
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
			search_cards(searchmode, card, string);
		else if (card->nquery > 0)
			find_and_select(string);
		append_search_string(string);
	}
}


static void requery_callback(void)
{
	CARD				*card = mainwindow->card;
	if (card->last_query >= 0)
		query_pulldown(card->last_query);
}


void append_search_string(
	char		*text)
{
	if (text) {
		print_text_button_s(w_search, text);
		s_offs = 0;
		if (!BLANK(history[s_curr])
				    && strcmp(history[s_curr], text))
			s_curr = (s_curr + 1) % NHIST;
		zfree(history[s_curr]);
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
	CARD				*card = mainwindow->card;
	if (!card || !card->dbase || !card->dbase->nrows)
		return;
	card_readback_texts(card, -1);
	query_letter(card, letter);
	create_summary_menu(card);
	card->row = card->query ? card->query[0]
					  : card->dbase->nrows;
	fillout_card(card, false);
}


/*
 * go forward or backward in the summary list if there is one. If there is
 * no summary, we must be adding cards and the user wants to see previously
 * added cards; step in the raw database in this case.
 */

static void pos_callback(
	int				inc)
{
	CARD				*card = mainwindow->card;
	card_readback_texts(card, -1);
	if (card->qcurr + inc >= 0 &&
	    card->qcurr + inc <  card->nquery) {
		card->qcurr += inc;
		card->row    = card->query[card->qcurr];
		fillout_card(card, false);
		scroll_summary(card);
		print_info_line();

	} else if (card->nquery == 0 && card->row + inc >= 0
				     && card->row + inc <  card->dbase->nrows){
		card->row += inc;
		fillout_card(card, false);
		print_info_line();
	}
}


/*
 * add a new card to the database.
 * When finished, find the first text input item and put the cursor into it.
 */

static void add_card(
	bool		dup)
{
	CARD		*card = mainwindow->card;
	ITEM		*item;
	DBASE		*dbase = card->dbase;
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
		/* this is stupid:  if there's no room for a row,
		 * there's probably also no room for a new popup window */
		create_error_popup(mainwindow, errno,
					"No memory for new database row");
		return;
	}
	dbase->currsect = save_sect;
	if (dup)
		for (i=0; i < dbase->maxcolumns; i++)
			dbase_put(dbase, card->row, i,
				  dbase_get(dbase, oldrow, i));
	else
		for (i=0; i < card->form->nitems; i++) {
			item = card->form->items[i];
			if (IN_DBASE(item->type) && item->idefault)
				dbase_put(dbase, card->row, item->column,
					  evaluate(card, item->idefault));
		}
	if ((card->qcurr = card->nquery)) {
		grow(0, "No memory for query", int, card->query, card->dbase->nrows, NULL);
		card->query[card->nquery++] = card->row;
	}
	dbase_sort(card, pref.sortcol, pref.revsort);
	create_summary_menu(card);
	fillout_card(card, false);
	scroll_summary(card);
	print_info_line();
	for (i=0; i < card->form->nitems; i++) {
		item = card->form->items[i];
		if (item->type == IT_INPUT || item->type == IT_TIME
					   || item->type == IT_NOTE
					   || item->type == IT_NUMBER) {
			card->items[i].w0->setFocus();
			break;
		}
	}
}


static void new_callback(void)
{
	add_card(false);
	if (pref.autoquery) {
		pref.autoquery = false;
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
	if (mainwindow->card->row >= 0)
		add_card(true);
}


/*
 * delete a card. Since the summary should not be deleted, the deleted card
 * must be removed from the query[] list. Since the query list contains the
 * row numbers of the cards it lists, and rows may get renumbered if one is
 * removed, row indices > deleted row must be decremented.
 */

static void del_callback(void)
{
	CARD				*card = mainwindow->card;
	int				*p, *q;
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
	p = q = &card->sorted[0];
	for (i=0; i < card->dbase->nrows + 1; i++, p++) {
		*q = *p - (*p > card->row);
		q += *p != card->row;
	}
	print_info_line();
	fillout_card(card, false);
	create_summary_menu(card);
}


/*
 * assign new section to the current card (called from the option popup)
 */

static void sect_callback(
	int				item)
{
	CARD				*card = mainwindow->card;
	SECTION				*sect = card->dbase->sect;
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
		sect[olds].modified = true;
		sect[news].modified = true;
		card->dbase->row[card->row]->section = defsection = news;
		card->dbase->modified = true;
		print_info_line();
	}
	remake_section_popup(false);
}

static bool multi_save_revert(
	bool		is_quit)
{
	DBASE *dbase;
	QDialog *dlg;
	QGridLayout *form;
	QString dlg_label;
	QLabel *top_line;
	DBASE *cur_dbase = is_quit || !mainwindow->card ? 0 : mainwindow->card->dbase;
	QCheckBox **checkboxes = 0;
	size_t nrows = 0, checkboxes_size;
	QLabel *label;

	if(!dbase_list) /* does nothing if no loaded databases */
		return true;
	for(dbase = dbase_list; dbase; dbase = dbase->next)
		if(dbase->modified)
			break;
	if(!dbase && is_quit) /* does nothing if quit & nothing to save */
		return true;
	/* FIXME: for multi-window, also skip databases displayed in window */
	/*        for fkey, also skip referenced tables */
	if(!dbase && dbase_list == cur_dbase && !dbase_list->next) {
		/* does almost nothing if nothing to save/revert or purge */
		create_error_popup(mainwindow, 0, "Nothing to revert or purge.");
		return true;
	}
	/* FIXME: auto-save-all before allowing fatal exit from this point on */
	dlg = new QDialog(mainwindow);
	dlg->setWindowTitle(is_quit ? "Revert/Save Databases" :
				      "Revert/Save/Purge Databases");
	dlg->setObjectName("multisave");
	set_icon(dlg, 1);
	bind_help(dlg, is_quit ? "quit" : "revert");
	form = new QGridLayout(dlg);
	dlg_label = dbase ?
		"One or more databses are modified.  If you revert "
		"them, all changes will be lost."
	      : "One or more databases are in memory, but not "
		"displayed.  Do you want to purge them from memory?";
	if(is_quit)
		dlg_label += "\nYou must explicitly Revert or Save all of "
			     "them before quitting the application.";
	top_line = new QLabel(dlg_label);
	form->addWidget(top_line, 0, 0, 1, 4);
	form->addItem(new QSpacerItem(0, 20), 1, 0, 1, 4);
	if(dbase) {
		form->addWidget(new QLabel("Save"), 2, 0);
		form->addWidget(new QLabel("Revert"), 2, 1);
	}
	if(!is_quit)
		form->addWidget(new QLabel("Purge"), 2, 2);
	form->setColumnStretch(3, 1);
	for(int m = 0; m < 2; m++) {
		for(dbase = dbase_list; dbase; dbase = dbase->next) {
			if(dbase->modified != !m)
				continue;
			if(dbase == cur_dbase && !dbase->modified)
				continue;
			zgrow(0, "save dialog", QCheckBox *, checkboxes,
			      nrows * 4, nrows * 4 + 4, &checkboxes_size);
			if(!m) {
				QCheckBox *s = new QCheckBox, *r = new QCheckBox;
				checkboxes[nrows * 4] = s;
				s->setChecked(dbase != cur_dbase);
				set_button_cb(s, if(c) r->setChecked(false),
					      bool c);
				form->addWidget(s, 3 + nrows, 0);
				checkboxes[nrows * 4 + 1] = r;
				r->setChecked(dbase == cur_dbase);
				set_button_cb(r, if(c) s->setChecked(false),
					      bool c);
				form->addWidget(r, 3 + nrows, 1);
			}
			if(!is_quit) {
				checkboxes[nrows * 4 + 2] = new QCheckBox;
				form->addWidget(checkboxes[nrows * 4 + 2], 3 + nrows, 2);
			}
			label = new QLabel(dbase->form->name);
			label->setToolTip(QString::asprintf(
				"Form: %s\nDatabase: %s%s",
				dbase->form->path, dbase->path,
				dbase->form->proc ? " (procedural)" : ""));
			form->addWidget(label, 3 + nrows, 3);
			checkboxes[nrows * 4 + 3] = reinterpret_cast<QCheckBox *>(dbase);
			nrows++;
		}
		if(is_quit)
			break;
	}
	form->addWidget(mk_separator(), 4 + nrows, 0, 1, 4);
	QDialogButtonBox *bb = new QDialogButtonBox;
	QAbstractButton *b;
	b = mk_button(bb, 0, dbbb(Help));
	set_button_cb(b, help_callback(dlg, is_quit ? "quit" : "revert"));
	b = mk_button(bb, 0, dbbb(Cancel));
	set_button_cb(b, dlg->reject());
	b = mk_button(bb, 0, dbbb(Ok));
	set_button_cb(b, dlg->accept());
	form->addWidget(bb, 5 + nrows, 0, 1, 4);
	bool ret;
	while(1) {
		size_t i;
		ret = dlg->exec();
		if(!ret)
			break;
		for(i = 0; i < nrows; i++)
			if(checkboxes[i * 4] &&
			   !checkboxes[i * 4]->isChecked() &&
			   !checkboxes[i * 4 + 1]->isChecked() &&
			   (is_quit || checkboxes[i * 4 + 2]->isChecked())) {
				if(!is_quit)
					top_line->setText(dlg_label +
							  QString("  You cannot purge a database without first saving or reverting it."));
				break;
			}
		if(i < nrows)
			continue;
		for(i = 0; i < nrows; i++) {
			dbase = reinterpret_cast<DBASE *>(checkboxes[i * 4 + 3]);
			if(checkboxes[i * 4]) {
				if(checkboxes[i * 4]->isChecked()) {
					write_dbase(dbase, true);
					if(dbase == cur_dbase)
						/* formerly in write_dbase() */
						print_info_line();
				} else if(!is_quit && checkboxes[i * 4 + 1]->isChecked()) {
					read_dbase(dbase->form, true);
					if(dbase == cur_dbase) {
						create_summary_menu(mainwindow->card);
						fillout_card(mainwindow->card, false);
						print_info_line();
					}
				}
			}
			if(!is_quit && checkboxes[i * 4 + 2]->isChecked()) {
				for(DBASE **prev = &dbase_list; *prev; prev = &(*prev)->next)
					if(*prev == dbase) {
						*prev = dbase->next;
						break;
					}
				dbase_clear(dbase);
				free(dbase);
			}
		}
		break;
	}
	free(checkboxes);
	delete dlg;
	return ret;
}

static int cmp_badref(const void *_a, const void *_b)
{
	const badref *a = (const badref *)_a, *b = (const badref *)_b;
	if(a->reason != b->reason)
		return a->reason - b->reason;
	if(a->form != b->form) {
		int c = strcmp(a->form->name, b->form->name);
		if(!c)
			return a->form - b->form;
		return c;
	}
	if(a->fform != b->fform) {
		if(!a->fform)
			return -1;
		if(!b->fform)
			return 1;
		int c = strcmp(a->fform->name, b->fform->name);
		if(!c)
			return a->fform - b->fform;
		return c;
	}
	if(a->item != b->item)
		return a->item - b->item;
	char *aval = dbase_get(a->dbase, a->row, a->form->items[a->item]->column);
	if(IFL(a->form->items[a->item]->,FKEY_MULTI))
		aval = unesc_elt_at(a->form, aval, a->keyno);
	char *bval = dbase_get(b->dbase, b->row, b->form->items[b->item]->column);
	if(IFL(b->form->items[b->item]->,FKEY_MULTI))
		bval = unesc_elt_at(b->form, bval, b->keyno);
	int c = strcmp(STR(aval), STR(bval));
	if(IFL(a->form->items[a->item]->,FKEY_MULTI))
		zfree(aval);
	if(IFL(b->form->items[b->item]->,FKEY_MULTI))
		zfree(bval);
	if(c)
		return c;
	return a->row - b->row;
}

static void fkey_callback(FKeySelector *fks)
{
	refilter_fkey(fks);
}

static void check_references(void)
{
	CARD *card = mainwindow->card;
	int nbadref = 0;
	badref *badrefs = 0;
	check_db_references(card->form, card->dbase, &badrefs, &nbadref);
	if(!nbadref) {
		QMessageBox *dialog = new QMessageBox(QMessageBox::Information,
						      "Reference Check",
						      "All references seem OK",
						      QMessageBox::Ok, mainwindow);
		popup_nonmodal(dialog);
	} else {
		QDialog *dlg = new QDialog(mainwindow);
		dlg->setWindowTitle("Reference Check");
		dlg->setObjectName("refcheck");
		set_icon(dlg, 1);
		bind_help(dlg, "refcheck");
		QBoxLayout *main = new QBoxLayout(QBoxLayout::TopToBottom, dlg);
		main->addWidget(new QLabel("Foreign reference inconsistencies"
					   " have been detected."));
		QScrollArea *sc = new QScrollArea(dlg);
		main->addWidget(sc);
		QWidget *msgw = new QWidget(sc);
		msgw->setMinimumSize(800, 600);
		sc->setWidget(msgw);
		QGridLayout *form = new QGridLayout(msgw);
		form->setSizeConstraint(QLayout::SetMinAndMaxSize);
		form->setColumnStretch(1, 1);
		main->addWidget(mk_separator());
		QDialogButtonBox *bb = new QDialogButtonBox;
		QAbstractButton *b;
		b = mk_button(bb, 0, dbbb(Help));
		set_button_cb(b, help_callback(dlg, "refcheck"));
		b = mk_button(bb, 0, dbbb(Cancel));
		set_button_cb(b, dlg->reject());
		b = mk_button(bb, 0, dbbb(Ok));
		set_button_cb(b, dlg->accept());
		main->addWidget(bb);
		QCheckBox *cb = 0, *cb2;  // init to shut gcc up
		int r = 0;
		char *sbuf = 0;
		size_t sbuf_len = 0;
		CARD card = {};
		qsort(badrefs, nbadref, sizeof(*badrefs), cmp_badref);
		for(int i = 0; i < nbadref; i++) {
			if(i)
				form->addWidget(mk_separator(), r++, 0, 1, 3);
			switch(badrefs[i].reason) {
			    case BR_MISSING: {
				/* FIXME:  only give "Delete card" for first
				 *         occurence of this card if more than
				 *         one badref applies to it */
				ITEM *item = badrefs[i].form->items[badrefs[i].item];
				char *data = dbase_get(badrefs[i].dbase,
						       badrefs[i].row,
						       item->column);
				if(IFL(item->,FKEY_MULTI))
					data = unesc_elt_at(badrefs[i].form,
							    data, badrefs[i].keyno);
				form->addWidget(new QLabel(
					qsprintf("Form '%s' has no card matching"
						 " row %d, key %d, field '%s'"
						 " of form '%s' (%s)",
						 badrefs[i].fform->name, badrefs[i].row, badrefs[i].keyno,
						 badrefs[i].form->items[badrefs[i].item]->name,
						 badrefs[i].form->name, data)),
						r++, 0, 1, 3);
				if(IFL(item->,FKEY_MULTI))
					zfree(data);
				card.form = const_cast<FORM *>(badrefs[i].form);
				card.dbase = const_cast<DBASE *>(badrefs[i].dbase);
				make_summary_line(&sbuf, &sbuf_len, &card,
						  badrefs[i].row);
				form->addWidget(new QLabel(sbuf), r++, 0, 1, 3);
				form->addWidget((cb = new QCheckBox), r, 0);
				cb->setChecked(true);
				form->addWidget(new QLabel("Set to:"), r, 1);
				QGridLayout *l = new QGridLayout;
				CARD *fcard = create_card_menu((FORM *)badrefs[i].fform,
							       (DBASE *)badrefs[i].fdbase, 0, true);
				fcard->fkey_next = &card;
				fcard->qcurr = badrefs[i].item;
				form->addLayout(l, r, 2);
				FKeySelector *fks = 0;
				int col = 0;
				for(int n = 0; n < item->nfkey; n++)
					    if(item->fkey[n].display) {
						    fks = add_fkey_field(msgw, &card,
									 badrefs[i].item,
									 "", fks, l, 0, 0, col,
									 *item, fcard, n, 0);
						    set_popup_cb(reinterpret_cast<QComboBox *>(fks), fkey_callback(fks), int,);
					    }
				l->addItem(new QSpacerItem(1, 1), 0, col);
				l->setColumnStretch(col, 1);
				refilter_fkey(fks);
				form->addWidget((cb2 = new QCheckBox), r + 1, 0);
				form->addWidget(new QLabel("Delete card"), r + 1, 1);
				set_button_cb(cb, if(c) cb2->setChecked(false),
					      bool c);
				set_button_cb(cb2, if(c) cb->setChecked(false),
					      bool c);
				r += 2;
				break;
			    }
			    case BR_DUP: {
				const ITEM *pitem = 0;  // init to shut gcc up
				char *pdata = 0;
				while(i < nbadref && badrefs[i].reason == BR_DUP) {
					const ITEM *item = badrefs[i].form->items[badrefs[i].item];
					char *data = dbase_get(badrefs[i].dbase,
							       badrefs[i].row,
							       item->column);
					if(IFL(item->,FKEY_MULTI))
						data = unesc_elt_at(badrefs[i].form,
								    data, badrefs[i].keyno);
					if(!pdata) {
						pdata = strdup(data);
						form->addWidget(new QLabel(
					qsprintf("Form '%s' has more than one card matching row %d, key %d, field '%s' of form '%s' (%s)",
						 badrefs[i].fform->name, badrefs[i].row, badrefs[i].keyno,
						 badrefs[i].form->items[badrefs[i].item]->name,
						 badrefs[i].form->name, pdata)),
						r++, 0, 1, 3);
					} else if(strcmp(pdata, data))
						break;
					else if(badrefs[i-1].form != badrefs[i].form)
						form->addWidget(new QLabel(QString(badrefs[i].form->name) + ':'));
					pitem = item;
					card.form = const_cast<FORM *>(badrefs[i].form);
					card.dbase = const_cast<DBASE *>(badrefs[i].dbase);
					make_summary_line(&sbuf, &sbuf_len, &card,
							  badrefs[i].row);
					form->addWidget(new QLabel(sbuf), r++, 0, 1, 3);
					if(IFL(item->,FKEY_MULTI))
						zfree(data);
					i++;
				}
				i--;
				zfree(pdata);
				pdata = dbase_get(badrefs[i].dbase,
						  badrefs[i].row,
						  pitem->column);
				for(int start = 0;; start++) {
					start = fkey_lookup(badrefs[i].fdbase,
							    badrefs[i].form,
							    pitem, pdata,
							    badrefs[i].keyno,
							    start);
					if(start < 0)
						break;
					QAbstractButton *b;
					b = mk_button(0, "Change", 0);
					form->addWidget(b, r, 0);
					set_button_cb(b, (new ItemEd(dlg,
								     badrefs[i].fform,
								     badrefs[i].fdbase,
								     start))->exec());
					card.form = const_cast<FORM *>(badrefs[i].fform);
					card.dbase = const_cast<DBASE *>(badrefs[i].fdbase);
					make_summary_line(&sbuf, &sbuf_len, &card, start);
					form->addWidget(new QLabel(sbuf), r, 1, 1, 2);
					r++;
				}
				break;
			    }
			    case BR_NO_INVREF:
				form->addWidget(new QLabel(
					qsprintf("Form '%s' does not list '%s' as a referer",
						 badrefs[i].fform->name, badrefs[i].form->name)),
						r++, 0, 1, 3);
				form->addWidget((cb = new QCheckBox), r, 0);
				cb->setChecked(true);
				form->addWidget(new QLabel("Add to form's Referers list"),
						r++, 1, 1, 2);
				break;
			    case BR_NO_FORM: {
				form->addWidget(new QLabel(
					qsprintf("Can't load foreign db '%s'",
						 badrefs[i].form->items[badrefs[i].item]->fkey_form_name)),
					       r++, 0, 1, 3);
				form->addWidget((cb = new QCheckBox), r, 0);
				cb->setChecked(true);
				form->addWidget(new QLabel("Change to:"), r, 1);
				QComboBox *db = new QComboBox;
				QStringList sl;
				add_dbase_list(sl);
				db->addItems(sl);
				form->addWidget(db, r++, 2);
				break;
			    }
			    case BR_NO_CFORM: {
				form->addWidget(new QLabel(
					qsprintf("Can't load foreign db '%s'",
						 badrefs[i].form->children[badrefs[i].item])),
						r++, 0, 1, 3);
				form->addWidget((cb = new QCheckBox), r, 0);
				cb->setChecked(true);
				form->addWidget(new QLabel("Change to:"), r, 1);
				QComboBox *db = new QComboBox;
				QStringList sl;
				add_dbase_list(sl);
				db->addItem("");
				db->setCurrentIndex(0);
				db->addItems(sl);
				form->addWidget(db, r++, 2);
				break;
			    }
			    case BR_NO_FREF:
				form->addWidget(new QLabel(
					qsprintf("Database '%s' listed as referer, but does not reference '%s'",
						 badrefs[i].form->children[badrefs[i].item], badrefs[i].form->name)),
						r++, 0, 1, 3);
				form->addWidget((cb = new QCheckBox), r, 0);
				cb->setChecked(true);
				form->addWidget(new QLabel("Remove from form's Referers list"),
						r++, 1, 1, 2);
				break;
			}
		}
		zfree(sbuf);
		if(!dlg->exec())
			return;
		QListIterator<QObject *>iter(form->children());
#define getcb(v) do { \
	while(iter.hasNext()) \
		if((v = dynamic_cast<QCheckBox *>(iter.next()))) \
			break; \
} while(0)
		for(int i = 0; i < nbadref; i++)
			switch(badrefs[i].reason) {
			    case BR_MISSING:
				getcb(cb);
				if(cb->isChecked()) {
					/* FIXME:  get pulldown values */
					/* FIXME:  set new value for fkey field */
				}
				getcb(cb2);
				if(cb->isChecked()) {
					/* FIXME: delete badrefs[i].row */
					/*   note: adjust any other rows in same dbase */
				}
				break;
			    case BR_DUP:
				/* nothing to do; change is in popup */
				break;
			    case BR_NO_INVREF:
				getcb(cb);
				if(cb->isChecked()) {
					/* FIXME: add form to fform's children */
				}
				break;
			    case BR_NO_FORM:
				getcb(cb);
				if(cb->isChecked()) {
					/* FIXME:  read db pulldown & update form def */
				}
				break;
			    case BR_NO_CFORM:
				getcb(cb);
				if(cb->isChecked()) {
					/* FIXME:  read db pulldown & update children */
					/*  note: and adjust CFORM/FREF indices if deleting */
				}
				break;
			    case BR_NO_FREF:
				getcb(cb);
				if(cb->isChecked()) {
					/* FIXME:  remove form.children[item] */
					/*  note: and adjust CFORM/FREF indices */
				}
				break;
			}
	}
	zfree(badrefs);
}
