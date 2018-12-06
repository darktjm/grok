/*
 * Create and destroy the export popup, and call the template functions.
 * The configuration is stored in the pref structure, but it can be changed
 * only here.
 *
 *	destroy_templ_popup()
 *	create_templ_popup()
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <QtWidgets>
#include <QtPrintSupport>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define NLINES	15		/* number of lines in list widget */

static void button_callback(int code, bool set = false);
static void file_export_callback(const QString &file);

static void mklist(void);
static void editfile(char *);
static void askname(BOOL);

static BOOL	have_shell = FALSE;	/* message popup exists if TRUE */
static QDialog	*shell;		/* popup menu shell */
static QListWidget	*list;		/* template list widget */
static int	list_nlines;	/* # of lines displayed in scroll list */
static BOOL	modified;	/* preferences have changed */


/*
 * destroy popup. Remove it from the screen, and destroy its widgets.
 */

static void destroy_templ_popup(void)
{
	if (have_shell) {
		if (modified)
			write_preferences();
		have_shell = FALSE;
		shell->close();
		delete shell;
	}
}


/*
 * create a print popup as a separate application shell. The popup is
 * initialized with data from pref. The menu array is the template for
 * generating the upper part of the popup window. The code is used to
 * identify buttons in callbacks. Code bits 4..7 are used to group
 * one-of-many choices.
 */

static struct menu {
	char	type;		/* Label, Scroll, b/q/Button, Text, -line */
	long	code;		/* unique identifier, 0=none */
	int 	role;
	const char *text;
	QWidget	*widget;
} menu[] = {
	{ 'M',	0x50,	0,		"menu"			},
	{ 'L',	0,	0,		"Using Template:"	},
	{ 'S',	0x10,	0,		"scroll"		},
	{ 'B',	0x20,	dbbr(Action),	"Create"		},
	{ 'b',	0x21,	dbbr(Action),	"Dup"			},
	{ 'b',	0x22,	dbbr(Action),	"Edit"			},
	{ 'q',	0x23,	dbbr(Action),	"Delete"		},
	{ '-',	0,	0,		"div"			},
	{ 'L',	0,	0,		"WIth Flags:"		},
	{ 'T',	0x11,	0,		"text"			},
	{ 'F',	0x12,	-'d',		"Summary"		},
	{ 'F',	0x13,	'n',		"Notes"			},
	{ 'f',	0x14,	-'s',		"Cards"			},
	{ 'L',	0,	1,		"To File:"		},
	{ 'T',	0x30,	0,		"text"			},
	{ 'q',	0x40,	dbbr(Action),	"..."			},
	{ 'B',	0x41,	dbbr(Accept),	"Export"		},
	{ 'b',	0x44,	dbbr(Accept),	"Preview"		},
	{ 'b',	0x42,	dbbb(Cancel),	0			},
	{ 'q',	0x43,	dbbb(Help),	0			}
};

static struct menu print_buttons[] = {
	{ 'b',	0x46,	dbbr(Accept),	"Print"			},
	{ 'b',	0x47,	dbbr(Accept),	"Preview"		},
	{ 'b',	0x42,	dbbb(Cancel),	0			},
	{ 'q',	0x45,	dbbb(Help),	0			}
};

static QStringList what_rows = {
	"Current card only", "Current search results", "All cards in current section", "All cards"
};
static const char select_codes[] = "CSeA";

#define BUILTIN_FLAGS "sdn"

static void create_templ_print_popup(bool print)
{
	struct menu	*mp;			/* current menu[] entry */
	QWidget		*w=0;
	QBoxLayout	*form;
	QDialogButtonBox *hb = 0;
	QBoxLayout	*hbox = 0;

	destroy_templ_popup();

	if (!curr_card) {
		create_error_popup(mainwindow, 0, "Select a database to %s", print ? "print" : "export");
		return;
	}

	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new QDialog;
	if (print)
		shell->setWindowTitle("Grok Print");
	else
		shell->setWindowTitle("Grok Export");
	set_icon(shell, 1);
	form = new QVBoxLayout(shell);
	add_layout_qss(form, "exportform");
    	/* I could use from->setContentsMargins() to 16 but I'll leave at default */
	/* Same applies to setSpacing() */
	bind_help(shell, "tempname");

	form->addWidget(new QLabel(print ? "Print" : "Export"));

	for (mp=menu; APTR_OK(mp, menu); mp++) {
	    w = NULL;
	    switch(mp->type) {
	      case 'B': if(!print) hb = new QDialogButtonBox; FALLTHROUGH
	      case 'b':
	      case 'q':
		  if(!print) {
		      w = mk_button(hb, mp->text, mp->role);
		      if(mp->code == 0x41)
			      reinterpret_cast<QPushButton *>(w)->setDefault(true);
		  }
		break;
	      case 'F':
	      case 'f': w = new QCheckBox(mp->text);	break;
	      case 'M': {
		  const char *psel = strchr(select_codes, pref.pselect);
		  QComboBox *cb = new QComboBox;
		  cb->addItems(what_rows);
		  cb->setCurrentIndex(psel ? (int)(psel - select_codes) : 1);
		  w = cb;
		  break;
	      }
	      case 'T': if(!print || mp->code == 0x11) { w = new QLineEdit; w->setObjectName(mp->text); } break;
	      case '-': if(!print) w = mk_separator();		break;
	      case 'L':	if(!print || !mp->role) w = new QLabel(mp->text);  break;
	      case 'S': {
		  w = list = new QListWidget;
		  list->setSelectionMode(QAbstractItemView::SingleSelection);
		  // list->setAttribute("courierFont", true); // This isn't necessary
		  list->addItem("QT is a pain in the ass");
		  list->setMinimumHeight(list->sizeHintForRow(0) * NLINES);
		  list->clear();
		  list_nlines = 0;
		  mklist();
		  break;
	      }
	    }
	    if (!w)
		continue;
	    if (mp->type == 'T') {
		hbox = new QHBoxLayout;
		form->addLayout(hbox);
	    }
	    if (!hb)
		(hbox ? hbox : form)->addWidget(w);
	    if (mp->type == 'B')
		form->addWidget(hb);
	    if (mp->type == 'q' || mp->type == 'f') {
		hb = 0;
		hbox = 0;
	    }
	    if (mp->code == 0x30 && pref.xfile)
		print_text_button(w, pref.xfile);
	    if (mp->code >= 0x12 && mp->code < 0x20) {
		bool notf = mp->role < 0;
		int flag = notf ? -mp->role : mp->role;
		flag = 1 << (flag - 'a');
		set_toggle(w, !(pref.xflags & flag) == notf);
	    }
	    if (mp->code == 0x11 && pref.xflags) {
		char buf[53];
		int i, m;
		char *p;
		for (i = 0, m = 1, p = buf; i < 26; i++, m <<= 1)
		    if (pref.xflags & m && !strchr(BUILTIN_FLAGS, 'a' + i)) {
			*p++ = '-';
			*p++ = 'a' + i;
		    }
		*p = 0;
		print_text_button(w, buf);
	    }

	    if (mp->type == 'b' || mp->type == 'q' || mp->type == 'B')
		set_button_cb(w, button_callback(mp->code));
	    else if (mp->type == 'F' || mp->type == 'f')
		set_button_cb(w, button_callback(mp->code, set), bool set);
	    else if (mp->type == 'M')
		set_popup_cb(w, button_callback(mp->code + i), int, i);
	    else if (mp->code == 0x30)
		set_textr_cb(w, button_callback(mp->code));
	    else if(mp->type == 'T')
		set_text_cb(w, button_callback(mp->code));
	    mp->widget = w;
	}
	if(print) {
		hb = new QDialogButtonBox;
		form->addWidget(hb);
		for (mp=print_buttons; APTR_OK(mp, print_buttons); mp++) {
			w = mk_button(hb, mp->text, mp->role);
			if(mp->code == 0x46)
				reinterpret_cast<QPushButton *>(w)->setDefault(true);
			set_button_cb(w, button_callback(mp->code));
			mp->widget = w;
		}
	}
	// close does a reject by default, so no extra callback needed
	set_dialog_cancel_cb(shell, button_callback(0x42));

	popup_nonmodal(shell);
	have_shell = TRUE;
	modified = FALSE;
}

void create_templ_popup(void)
{
    create_templ_print_popup(false);
}

void create_print_popup(void)
{
    create_templ_print_popup(true);
}

/*
 * put the current list of templates into the template list widget
 */

static void save_cb(UNUSED int seq, char *name)
{
	list->addItem(name);
	list_nlines++;
}

static void mklist(void)
{
	list->clear();
	list_nlines = 0;
	list_templates(save_cb, curr_card);
	if (pref.xlistpos >= list_nlines)
		pref.xlistpos = list_nlines-1;
	list->setCurrentRow(pref.xlistpos);
}


/*-------------------------------------------------- callbacks --------------*/


static int get_list_seq(void)
{
	int i = list->currentRow();
	if(i >= 0) {
		pref.xlistpos = i;
		return(TRUE);
	} else {
		create_error_popup(shell, 0, "Please choose a template name");
		return(FALSE);
	}
}

#define SECT_OK(db,r) ((db)->currsect < 0 ||\
		       (db)->currsect == (db)->row[r]->section)

/* This is nasty, but curr_card is used too much to just make a replacement */
static CARD *old_curr_card = 0;
static void set_export_card(void)
{
	static CARD new_card;

	if(pref.pselect == 'S' || !curr_card || !curr_card->dbase)
		return;
	old_curr_card = curr_card;
	new_card = *curr_card;
	curr_card = &new_card;
	switch(pref.pselect) {
	    case 'C':
		curr_card->nquery = curr_card->qcurr < curr_card->nquery;
		if(curr_card->nquery) {
			curr_card->query = (int *)malloc(sizeof(*curr_card->query));
			curr_card->query[0] = curr_card->qcurr;
		} else
			curr_card->query = NULL;
		break;
	    case 'e': {
		int n;
		for(int i = n = 0; i < curr_card->dbase->nrows; i++)
			if(SECT_OK(curr_card->dbase, i))
				n++;
		curr_card->nquery = n;
		if(curr_card->nquery) {
			curr_card->query = (int *)malloc(curr_card->nquery * sizeof(*curr_card->query));
			/* sorting sorts dbase, so no need to resort */
			for(int i = n = 0; i < curr_card->dbase->nrows; i++)
				if(SECT_OK(curr_card->dbase, i))
					curr_card->query[n++] = i;
		} else
			curr_card->query = NULL;
		break;
	    }
	    case 'A':
		curr_card->nquery = curr_card->dbase->nrows;
		if(curr_card->nquery) {
			curr_card->query = (int *)malloc(curr_card->nquery * sizeof(*curr_card->query));
			/* sorting sorts dbase, so no need to resort */
			for(int i = 0; i < curr_card->dbase->nrows; i++)
				curr_card->query[i] = i;
		} else
			curr_card->query = NULL;
		break;
	}
}


static void unset_export_card(void)
{
	if(old_curr_card) {
		zfree(curr_card->query);
		curr_card = old_curr_card;
		old_curr_card = NULL;
	}
}

static BOOL do_export(void)
{
	struct menu	*mp;		/* for finding text widget */
	const char	*err;

	for (mp=menu; mp->code != 0x30; mp++);
	read_text_button_noblanks(mp->widget, &pref.xfile);
	
	if (!pref.xfile) {
		create_error_popup(shell,0,"Please enter an output file name");
		return(FALSE);
	}
	if (!get_list_seq())
		return(FALSE);

	set_export_card();
	if(pref.xfile[0] != '|')
		err = exec_template(pref.xfile, NULL, 0, pref.xlistpos, pref.xflags, curr_card);
	else {
		FILE *f = popen(pref.xfile + 1, "w");
		if(!f)
			err = "Can't open pipe";
		else {
			err = exec_template(NULL, f, 0, pref.xlistpos, pref.xflags, curr_card);
			pclose(f);
		}
	}
	if (err) {
		unset_export_card();
		create_error_popup(shell, 0, "Export failed:\n%s", err);
		return(FALSE);
	}
	unset_export_card();
	modified = TRUE;
	return(TRUE);
}

static BOOL export_to_doc(QTextDocument &doc)
{
	const char	*err;

	if (!get_list_seq())
		return(FALSE);

	FILE *f = tmpfile();
	if(!f) {
		create_error_popup(shell, 0, "Can't create output file");
		return(FALSE);
	}
	set_export_card();
	if ((err = exec_template(0, f, 0, pref.xlistpos, pref.xflags, curr_card))) {
		unset_export_card();
		create_error_popup(shell, 0, "Export failed:\n%s", err);
		fclose(f);
		return(FALSE);
	}
	unset_export_card();
	fflush(f);
	unsigned long len = ftell(f);
	rewind(f);
	char *cdoc = (char *)malloc(len + 1);
	if(!cdoc) {
		create_error_popup(shell, 0, "No memory for results");
		fclose(f);
		return(FALSE);
	}
	fread(cdoc, len, 1, f);
	fclose(f);
	cdoc[len] = 0;
	QString s(cdoc);
	free(cdoc);
	static QWidget *w = NULL;
	static QFont printfont; // only for plain text; HTML assumed to be OK
	if(!w) {
		w = new QWidget;
		w->setProperty("printFont", true);
		w->setProperty("courierFont", true);
		w->ensurePolished();
		printfont = w->font();
		delete w;
	}
	if(s.indexOf('\b') != -1) {
		/* QTextDocument can't handle overstrike */
		/* so converte it to HTML */
		s = s.toHtmlEscaped();
		s.replace(QRegularExpression("_\b(&[^;]+;|.)"), "<U>\\1</U>");
		s.replace("_", "<U> </U>");
		s.replace(QRegularExpression("(&[^;]+;|.)\b(&[^;]+;|.)"), "<B>\\1</B>");
		s.replace(QRegularExpression("</B>( *)<B>"), "\\1");
		s.replace("</U><U>", "");
		s.prepend("<div style=\"white-space: pre;\">");
		s.append("</div>");
		doc.setHtml(s);
		doc.setDefaultFont(printfont);
	} else {
		if(Qt::mightBeRichText(s))
			doc.setHtml(s);
		else {
			doc.setDefaultFont(printfont);
			doc.setPlainText(s);
		}
	}
	return(TRUE);
}


static void button_callback(
	int				code,
	bool				set)
{
	switch(code) {
	  case 0x20:						/* Create */
		askname(FALSE);
		break;
	  case 0x21:						/* Dup */
		if (!get_list_seq())
			return;
		askname(TRUE);
		break;
	  case 0x22:						/* Edit */
		if (!get_list_seq())
			break;
		if (pref.xlistpos >= get_template_nbuiltins()) {
			char *path = get_template_path(0,
					pref.xlistpos, curr_card);
			editfile(path);
			free(path);
		} else
			create_error_popup(shell, 0,
				"Cannot edit a builtin template, use Dup");
		break;
	  case 0x23:						/* Delete */
		if (!get_list_seq())
			return;
		(void)delete_template(shell, pref.xlistpos, curr_card);
		mklist();
		break;
	  case 0x40:						/* Browse */
	{
		QFileDialog *d = new QFileDialog(shell, "Select Export Output File");
		if(pref.xfile && pref.xfile[0] != '|')
			d->selectFile(pref.xfile);
		d->setAcceptMode(QFileDialog::AcceptSave);
		set_file_dialog_cb(d, file_export_callback(fn), fn);
		// close does a reject by default, so no extra callback needed
		d->exec();
		delete d;
		break;
	}
	  case 0x11: {						/* flags */
		  char *p, *f;
		  struct menu *mp;
		  bool rewrite = false;
		  pref.xflags = 0;
		  for (mp=menu; APTR_OK(mp, menu); mp++)
			  if(mp->code >= 0x12 && mp->code < 0x20) {
				  QCheckBox *b = dynamic_cast<QCheckBox *>(mp->widget);
				  bool notf = mp->role < 0;
				  int flag = notf ? -mp->role : mp->role;
				  if(b->isChecked() == !notf)
					  pref.xflags |= 1<<(flag - 'a');
			  }
		  for (mp=menu; mp->code != 0x11; mp++);
		  f = read_text_button_noblanks(mp->widget, NULL);
		  for(p = f; *p; p++) {
			  if(*p == '-')
				  p++;
			  /* FIXME: put this in a validator */
			  if(*p < 'a' || *p > 'z') {
				  *p = 0;
				  rewrite = true;
				  create_error_popup(shell,0,"Options are letters a-z only");
				  break;
			  }
			  pref.xflags |= 1<<(*p - 'a');
			  if(strchr(BUILTIN_FLAGS, *p)) {
				  char *q = p + 1, fl = *p;
				  if(p > f && p[-1] == '-')
					  p--;
				  if(!*q)
					  *p = 0;
				  else
					  memmove(p, q, strlen(q) + 1);
				  p--;
				  rewrite = true;
				  for(struct menu *mp=menu; APTR_OK(mp, menu); mp++) {
					  if(mp->code >= 0x12 && mp->code < 0x20 &&
					     (mp->role == fl || mp->role == -fl)) {
						  set_toggle(mp->widget, mp->role > 0);
						  break;
					  }
				  }
			  }
		  }
		  if(rewrite) {
			  QSignalBlocker sb(mp->widget);
			  print_text_button_s(mp->widget, f);
		  }
		  break;
	  }
	  case 0x12:
	  case 0x13:
	  case 0x14: {
		struct menu *mp;
		for(mp = menu; mp->code != code; mp++);
		bool notf = mp->role < 0;
		int flag = notf ? -mp->role : mp->role;
		flag = 1 << (flag - 'a');
		if(set == !notf)
			pref.xflags |= flag;
		else
			pref.xflags &= ~flag;
		break;
	  }
	  case 0x30:						/* text */
	  case 0x41:						/* Export */
		if (do_export())
	  case 0x42:						/* Cancel */
		destroy_templ_popup();
		break;
	  case 0x43:						/* Help */
		help_callback(shell, "export");
		break;
	  case 0x45:						/* Help */
		help_callback(shell, "print");
		break;
	  case 0x44: {						/* Preview */
		QTextDocument *d =  new QTextDocument;
		if(!export_to_doc(*d)) {
			delete d;
			break;
		}
		/* create_edit_popup will take ownership of d */
		create_edit_popup("Export Preview", NULL, TRUE, "editprint", d);
		break;
	  }
	  case 0x46:
	  case 0x47: {	 					/* Print */
		QTextDocument d;
		modified = TRUE;
		if(!export_to_doc(d))
			break;
		destroy_templ_popup();
		if(code == 0x46) {
			QPrintDialog pd(pref.printer, mainwindow);
			/* not sure if this is needed or does anything */
			pd.setOption(QAbstractPrintDialog::PrintToFile);
			if(pd.exec() == QDialog::Accepted)
				d.print(pref.printer);
		} else {
			QPrintPreviewDialog pd(pref.printer, mainwindow);
			QObject::connect(&pd, &QPrintPreviewDialog::paintRequested,
					 &d, &QTextDocument::print);
			pd.exec(); /* dialog will do the printing if requested */
		}
		// just setting modified won't work, since popup is dead
		write_preferences();
		break;
	  }
	  case 0x50:
	  case 0x51:
	  case 0x52:
	  case 0x53: {	 					/* Cards */
		pref.pselect = select_codes[code - 0x50];
		modified = TRUE;
	  }
	}
}


/*-------------------------------- browse callbacks -------------------------*/

static void file_export_callback(
	const QString	&filename)
{
	struct menu			*mp;

	if (filename.size()) {
		if (pref.xfile)
			free(pref.xfile);
		pref.xfile = qstrdup(filename);
		for (mp=menu; mp->code != 0x30; mp++);
		print_text_button(mp->widget, pref.xfile);
	}
}


/*-------------------------------- ask for name -----------------------------*/
/*
 * user pressed Dup or Create. Ask for a new file name.
 */

static void text_callback	(void);
static void textcancel_callback	(void);

static BOOL		have_askshell = FALSE;	/* text popup exists if TRUE */
static QDialog		*askshell;	/* popup menu shell */
static QLineEdit	*text;		/* template name string */
static BOOL		duplicate;	/* dup file before editing */

static void askname(
	BOOL		dup)		/* duplicate file before editing */
{
	QWidget		*w;
	QBoxLayout	*form;
	QDialogButtonBox *b;

	duplicate = dup;
	if (have_askshell) {
		popup_nonmodal(askshell);
 		return;
	}
	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	askshell = new QDialog(shell);
	askshell->setWindowTitle("Template name");
	askshell->setObjectName("tempform"); // for style sheets

	set_icon(askshell, 1);
	bind_help(askshell, "tempname");

	form = new QVBoxLayout(askshell);
	add_layout_qss(form, "tempform");

	w = new QLabel("Name for new template:");
	form->addWidget(w);

	text = new QLineEdit;
	form->addWidget(text);
	set_textr_cb(text, text_callback());

	b = new QDialogButtonBox;
	form->addWidget(b);

	w = mk_button(b, 0, dbbb(Cancel));
	set_button_cb(w, textcancel_callback());

	w = mk_button(b, 0, dbbb(Help));
	set_button_cb(w, help_callback(askshell, "tempname"));

	// tjm - added this button mostly for consistency
	w = mk_button(b, "Create", dbbr(Accept));
	reinterpret_cast<QPushButton *>(w)->setDefault(true);
	set_button_cb(w, text_callback());

	// close does a reject by default, so no extra callback needed
	set_dialog_cancel_cb(shell, textcancel_callback());

	popup_nonmodal(askshell);
	have_askshell = TRUE;
}


static void textcancel_callback(void)
{
	if (have_askshell)
		delete askshell;
	have_askshell = FALSE;
}


static void text_callback(void)
{
	char				*name, *p;
	char				*string;

	string = qstrdup(text->text());
	for (name=string; *name == ' ' || *name == '\n'; name++);
	if (*name) {
		for (p=name; *p; p++)
			if (*p == '/' || *p == ' ' || *p == '\t')
				*p = '_';
		if (duplicate) {
			if (!(name = copy_template(askshell, name,
						   pref.xlistpos, curr_card)))
				return;
			mklist();
		} else
			name = get_template_path(name, 0, curr_card);

		editfile(name);
		free(name);
	}
	free(string);
	textcancel_callback();
}


/*-------------------------------- edit template file -----------------------*/
/*
 * user pressed Dup or Create. Ask for a new file name.
 */

static void editfile(
	char		*path)		/* path to edit */
{
	if (access(path, F_OK)) {
		FILE *fp = fopen(path, "w");
		fclose(fp);
		mklist();
	}
	edit_file(path, FALSE, TRUE, path, "tempedit");
}
