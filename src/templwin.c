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

static void button_callback(CARD *card, int code, bool set = false);
static void file_export_callback(const QString &file);

static void mklist(CARD *card);
static void editfile(CARD *card, char *);
static void askname(CARD *card, bool);

class TemplDialog : public QDialog {
    public:
	TemplDialog() {}
	QListWidget	*list;
	int		list_nlines;
	CARD export_card = {};
};

static TemplDialog		*shell = 0;	/* popup menu shell */


/*
 * destroy popup. Remove it from the screen, and destroy its widgets.
 */

static void destroy_templ_popup(void)
{
	if (shell) {
		if (pref.modified)
			write_preferences();
		shell->close();
		delete shell;
		shell = NULL;
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

static const QStringList what_rows = {
	"Current card only", "Current search results", "All cards in current section", "All cards"
};
static const char select_codes[] = "CSeA";

#define BUILTIN_FLAGS "sdn"

static void create_templ_print_popup(CARD *card, bool print)
{
	struct menu	*mp;			/* current menu[] entry */
	QWidget		*w=0;
	QBoxLayout	*form;
	QDialogButtonBox *hb = 0;
	QBoxLayout	*hbox = 0;

	destroy_templ_popup();

	if (!card) {
		create_error_popup(mainwindow, 0, "Select a database to %s", print ? "print" : "export");
		return;
	}

	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	shell = new TemplDialog;
	if (print)
		shell->setWindowTitle("Grok Print");
	else
		shell->setWindowTitle("Grok Export");
	set_icon(shell, 1);
	form = new QVBoxLayout(shell);
	add_layout_qss(form, "exportform");
    	/* I could use from->setContentsMargins() to 16 but I'll leave at default */
	/* Same applies to setSpacing() */
	bind_help(shell, print ? "print" : "export");

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
		  w = shell->list = new QListWidget;
		  shell->list->setSelectionMode(QAbstractItemView::SingleSelection);
		  // shell->list->setAttribute("courierFont", true); // This isn't necessary
		  shell->list->addItem("QT is a pain in the ass");
		  shell->list->setMinimumHeight(shell->list->sizeHintForRow(0) * NLINES);
		  shell->list->clear();
		  shell->list_nlines = 0;
		  mklist(card);
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
		char buf[26 * 2 + 1];
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
		set_button_cb(w, button_callback(card, mp->code));
	    else if (mp->type == 'F' || mp->type == 'f')
		set_button_cb(w, button_callback(card, mp->code, set), bool set);
	    else if (mp->type == 'M')
		set_popup_cb(w, button_callback(card, mp->code + i), int, i);
	    else if (mp->code == 0x30)
		set_textr_cb(w, button_callback(card, mp->code));
	    else if(mp->type == 'T')
		set_text_cb(w, button_callback(card, mp->code));
	    mp->widget = w;
	}
	if(print) {
		hb = new QDialogButtonBox;
		form->addWidget(hb);
		for (mp=print_buttons; APTR_OK(mp, print_buttons); mp++) {
			w = mk_button(hb, mp->text, mp->role);
			if(mp->code == 0x46)
				reinterpret_cast<QPushButton *>(w)->setDefault(true);
			set_button_cb(w, button_callback(card, mp->code));
			mp->widget = w;
		}
	}
	// close does a reject by default, so no extra callback needed
	set_dialog_cancel_cb(shell, button_callback(card, 0x42));

	popup_nonmodal(shell);
}

void create_templ_popup(CARD *card)
{
    create_templ_print_popup(card, false);
}

void create_print_popup(CARD *card)
{
    create_templ_print_popup(card, true);
}

/*
 * put the current list of templates into the template list widget
 */

static void save_cb(UNUSED int seq, char *name)
{
	shell->list->addItem(name);
	shell->list_nlines++;
}

static void mklist(CARD *card)
{
	shell->list->clear();
	shell->list_nlines = 0;
	list_templates(save_cb, card);
	if (pref.xlistpos >= shell->list_nlines)
		pref.xlistpos = shell->list_nlines-1;
	shell->list->setCurrentRow(pref.xlistpos);
}


/*-------------------------------------------------- callbacks --------------*/


static int get_list_seq(void)
{
	int i = shell->list->currentRow();
	if(i >= 0) {
		pref.xlistpos = i;
		return(true);
	} else {
		create_error_popup(shell, 0, "Please choose a template name");
		return(false);
	}
}

#define SECT_OK(db,r) ((db)->currsect < 0 ||\
		       (db)->currsect == (db)->row[r]->section)

static void set_export_card(CARD *card)
{
	const int *oquery;
	tzero(CARD, &shell->export_card, 1);
	if(!card || !card->dbase)
		return;
	shell->export_card = *card;
	card = &shell->export_card;
	switch(pref.pselect) {
	    case 'S':
		oquery = card->query;
		card->query = alloc(0, "query", int, card->nquery);
		tmemcpy(int, card->query, oquery, card->nquery);
		break;
	    case 'C':
		card->nquery = card->qcurr < card->nquery;
		card->query = alloc(0, "query", int, card->nquery);
		if(card->nquery)
			card->query[0] = card->qcurr;
		break;
	    case 'e': {
		int n;
		for(int i = n = 0; i < card->dbase->nrows; i++)
			if(SECT_OK(card->dbase, i))
				n++;
		card->nquery = n;
		card->query = alloc(0, "query", int, card->nquery);
		if(card->nquery)
			/* sorting sorts dbase, so no need to resort */
			for(int i = n = 0; i < card->dbase->nrows; i++)
				if(SECT_OK(card->dbase, i))
					card->query[n++] = i;
		break;
	    }
	    case 'A':
		card->nquery = card->dbase->nrows;
		card->query = alloc(0, "query", int, card->nquery);
		if(card->nquery)
			/* sorting sorts dbase, so no need to resort */
			for(int i = 0; i < card->dbase->nrows; i++)
				card->query[i] = i;
		break;
	}
}


static void unset_export_card(CARD *card)
{
	if(card)
		tmemcpy(struct var, card->var, shell->export_card.var, 26);
	zfree(shell->export_card.query);
}

static bool do_export(CARD *card)
{
	struct menu	*mp;		/* for finding text widget */
	const char	*err;

	for (mp=menu; mp->code != 0x30; mp++);
	read_text_button_noblanks(mp->widget, &pref.xfile);
	
	if (!pref.xfile) {
		create_error_popup(shell,0,"Please enter an output file name");
		return(false);
	}
	if (!get_list_seq())
		return(false);

	set_export_card(card);
	if(pref.xfile[0] != '|')
		err = exec_template(pref.xfile, NULL, 0, pref.xlistpos, pref.xflags, &shell->export_card);
	else {
		FILE *f = popen(pref.xfile + 1, "w");
		if(!f)
			err = "Can't open pipe";
		else {
			err = exec_template(NULL, f, 0, pref.xlistpos, pref.xflags, &shell->export_card);
			pclose(f);
		}
	}
	if (err) {
		unset_export_card(card);
		create_error_popup(shell, 0, "Export failed:\n%s", err);
		return(false);
	}
	unset_export_card(card);
	pref.modified = true;
	return(true);
}

static bool export_to_doc(CARD *card, QTextDocument &doc)
{
	const char	*err;

	if (!get_list_seq())
		return(false);

	FILE *f = tmpfile();
	if(!f) {
		create_error_popup(shell, 0, "Can't create output file");
		return(false);
	}
	set_export_card(card);
	if ((err = exec_template(0, f, 0, pref.xlistpos, pref.xflags, &shell->export_card))) {
		unset_export_card(card);
		create_error_popup(shell, 0, "Export failed:\n%s", err);
		fclose(f);
		return(false);
	}
	unset_export_card(card);
	fflush(f);
	unsigned long len = ftell(f);
	rewind(f);
	char *cdoc = alloc(shell, "template results", char, len + 1);
	if(!cdoc) {
		fclose(f);
		return(false);
	}
	/* Weird that glibc complains about ignoring read failure but not write */
	/* especially given how unlikely this particular read is to fail */
	if(fread(cdoc, len, 1, f) != 1)
		fatal("Readback of template restuls failed");
	fclose(f);
	cdoc[len] = 0;
	QString s(cdoc);
	free(cdoc);
	static QFont *printfont; // only for plain text; HTML assumed to be OK
	if(!printfont) {
		QWidget *w = new QWidget;
		w->setProperty("printFont", true);
		w->setProperty("courierFont", true);
		w->ensurePolished();
		printfont = new QFont(w->font());
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
		doc.setDefaultFont(*printfont);
	} else {
		if(Qt::mightBeRichText(s))
			doc.setHtml(s);
		else {
			doc.setDefaultFont(*printfont);
			doc.setPlainText(s);
		}
	}
	return(true);
}


static void button_callback(
	CARD				*card,
	int				code,
	bool				set)
{
	switch(code) {
	  case 0x20:						/* Create */
		askname(card, false);
		break;
	  case 0x21:						/* Dup */
		if (!get_list_seq())
			return;
		askname(card, true);
		break;
	  case 0x22:						/* Edit */
		if (!get_list_seq())
			break;
		if (pref.xlistpos >= get_template_nbuiltins()) {
			char *path = get_template_path(0,
					pref.xlistpos, card);
			editfile(card, path);
			free(path);
		} else
			create_error_popup(shell, 0,
				"Cannot edit a builtin template, use Dup");
		break;
	  case 0x23:						/* Delete */
		if (!get_list_seq())
			return;
		(void)delete_template(shell, pref.xlistpos, card);
		mklist(card);
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
					  tmemmove(char, p, q, strlen(q) + 1);
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
		if (do_export(card))
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
		if(!export_to_doc(card, *d)) {
			delete d;
			break;
		}
		/* create_edit_popup will take ownership of d */
		create_edit_popup("Export Preview", NULL, true, "editprint", d);
		break;
	  }
	  case 0x46:
	  case 0x47: {	 					/* Print */
		QTextDocument d;
		pref.modified = true;
		if(!export_to_doc(card, d))
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
		pref.modified = true;
	  }
	}
}


/*-------------------------------- browse callbacks -------------------------*/

static void file_export_callback(
	const QString	&filename)
{
	struct menu			*mp;

	if (filename.size()) {
		zfree(pref.xfile);
		pref.xfile = qstrdup(filename);
		for (mp=menu; mp->code != 0x30; mp++);
		print_text_button(mp->widget, pref.xfile);
	}
}


/*-------------------------------- ask for name -----------------------------*/
/*
 * user pressed Dup or Create. Ask for a new file name.
 */

static void text_callback	(CARD *card);
static void textcancel_callback	(void);

class TmplAskName : public QDialog {
    public:
	TmplAskName(QWidget *parent) : QDialog(parent) {}
	QLineEdit	*text;		/* template name string */
	bool		duplicate;	/* dup file before editing */
};

static TmplAskName	*askshell = 0;	/* popup menu shell */

static void askname(
	CARD		*card,
	bool		dup)		/* duplicate file before editing */
{
	QWidget		*w;
	QBoxLayout	*form;
	QDialogButtonBox *b;

	askshell->duplicate = dup;
	if (askshell) {
		popup_nonmodal(askshell);
 		return;
	}
	// The proper way to ignore delete is to override QWindow::closeEvent()
	// Instead, I'll do nothing.  It makes more sense to issue a reject
	// (the default behavior), anyway - tjm
	askshell = new TmplAskName(shell);
	askshell->setWindowTitle("Template name");
	askshell->setObjectName("tempform"); // for style sheets

	set_icon(askshell, 1);
	bind_help(askshell, "tempname");

	form = new QVBoxLayout(askshell);
	add_layout_qss(form, "tempform");

	w = new QLabel("Name for new template:");
	form->addWidget(w);

	askshell->text = new QLineEdit;
	form->addWidget(askshell->text);
	set_textr_cb(askshell->text, text_callback(card));

	b = new QDialogButtonBox;
	form->addWidget(b);

	w = mk_button(b, 0, dbbb(Cancel));
	set_button_cb(w, textcancel_callback());

	w = mk_button(b, 0, dbbb(Help));
	set_button_cb(w, help_callback(askshell, "tempname"));

	// tjm - added this button mostly for consistency
	w = mk_button(b, "Create", dbbr(Accept));
	reinterpret_cast<QPushButton *>(w)->setDefault(true);
	set_button_cb(w, text_callback(card));

	// close does a reject by default, so no extra callback needed
	set_dialog_cancel_cb(shell, textcancel_callback());

	popup_nonmodal(askshell);
}


static void textcancel_callback(void)
{
	if (askshell)
		delete askshell;
	askshell = 0;
}


static void text_callback(CARD *card)
{
	char				*name, *p;
	char				*string;

	string = qstrdup(askshell->text->text());
	if(string)
		for (name=string; *name == ' ' || *name == '\n'; name++);
	if (string && *name) {
		for (p=name; *p; p++)
			if (*p == '/' || *p == ' ' || *p == '\t')
				*p = '_';
		if (askshell->duplicate) {
			if (!(name = copy_template(askshell, name,
						   pref.xlistpos, card)))
				return;
			mklist(card);
		} else
			name = get_template_path(name, 0, card);

		editfile(card, name);
		free(string);
	}
	zfree(string);
	textcancel_callback();
}


/*-------------------------------- edit template file -----------------------*/
/*
 * user pressed Dup or Create. Ask for a new file name.
 */

static void editfile(
	CARD		*card,
	char		*path)		/* path to edit */
{
	if (access(path, F_OK)) {
		FILE *fp = fopen(path, "w");
		fclose(fp);
		mklist(card);
	}
	edit_file(path, false, true, path, "tempedit");
}
