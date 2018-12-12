/*
 * draw card canvas. This canvas is used by the form editor to allow the
 * user to position items in a form and to set their size. The canvas is
 * drawn using Xlib calls; items are represented by colored rectangles.
 *
 *	destroy_card_menu(card)
 *	create_card_menu(form, dbase, wform)
 *	card_readback_texts(card, nitem)
 *	format_time_data(data, timefmt)
 *	fillout_card(card, deps)
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>
#include <QtWidgets>
#include <time.h>
#include "grok.h"
#include "form.h"
#include "chart-widget.h"
#include "proto.h"

static void create_item_widgets(CARD *, int);
static void card_callback(int, CARD *, bool f = false, int i = 0);
void card_readback_texts(CARD *, int);
static BOOL store(CARD *, int, const char *, int menu = 0);

// Th Card window can be open multiple times, and should self-destruct on
// close.  The old close callback also freed the card, as below.
class CardWindow : public QDialog {
public:
    CARD *card;
    ~CardWindow() { free(card); }
};


/*
 * Read back any unread text widgets. Next, destroy the card widgets, and
 * the window if there is one (there is one if create_card_menu() was called
 * with wform==0). All the associated data structures are cleared, except
 * the referenced database, if any. Do not use the CARD struct after calling
 * destroy_card_menu().
 */

void destroy_card_menu(
	CARD		*card)		/* card to destroy */
{
	int		i;		/* item counter */

	if (!card)
		return;
	card_readback_texts(card, -1);
	// Unlike original Motif, I don't ceate a subwidget inside the container
	// So wform is either the mainwindow's widget or shell
	if (card->shell) {
		card->shell->close();
		delete card->shell; // also deletes wform
	} else if (card->wform) {
		// wform won't be deleted, so delete its children instead.
		if(card->wstat)
			delete card->wstat;
		if(card->wcard)
			delete card->wcard;
		card->wstat = card->wcard = 0;
	}
	for (i=0; i < card->nitems; i++) {
		card->items[i].w0 = 0;
		card->items[i].w1 = 0;
	}
	card->wform = card->shell = 0;
}


/*
 * create a card, based on the form struct (which describes structure and
 * layout) and a database struct (which describes the contents). The database
 * is not accessed, it is stored to permit later entry through the form's
 * callbacks into the database. In addition, a form widget to install into
 * must be given. If a null form widget is passed, this routine creates a
 * new window with a form widget in it. The new card is not filled out.
 * A card data structure is returned that allows printing into the card,
 * entering data into the database through the card, and destroying the card.
 */

CARD *create_card_menu(
	FORM		*form,		/* form that controls layout */
	DBASE		*dbase,		/* database for callbacks, or 0 */
	QWidget		*wform)		/* form widget to install into, or 0 */
{
	CARD		*card;		/* new card being allocated */
	int		xs, ys, ydiv;	/* card size and divider */
	int		n;

							/*-- alloc card --*/
	n = sizeof(CARD) + sizeof(struct carditem) * form->nitems;
	if (!(card = (CARD *)malloc(n)))
		return(NULL);
	memset(card, 0, n);
	card->form   = form;
	card->dbase  = dbase;
	card->row    = -1;
	card->nitems = form->nitems;
	if (!mainwindow)
		return(card);
	xs   = pref.scale * form->xs;
	ys   = pref.scale * form->ys;
	ydiv = pref.scale * form->ydiv;
							/*-- make form --*/
	if (wform) {
		wform->resize(xs+6, ys+6);
		wform->setMinimumSize(xs+6, ys+6);
		card->wform = wform;
	} else {
		CardWindow *cw = new CardWindow;
		cw->card = card;
		cw->setAttribute(Qt::WA_DeleteOnClose);
		card->shell = cw;
		card->shell->setWindowTitle("Card");
		set_icon(card->shell, 1);
		card->shell->resize(xs+6, ys+6);
		card->shell->setMinimumSize(xs+6, ys+6);
		// This doesn't seem to work; it's probably just a layout hint:
		// card->shell->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		// But this does.
		card->shell->setMaximumSize(xs+6, ys+6);
		wform = card->wform = card->shell;
		card->shell->setObjectName("wform");
		popup_nonmodal(card->shell);
	}
	card->wstat = card->wcard = 0;
	if (ydiv) {
	    card->wstat = new QWidget(wform);
	    card->wstat->setAutoFillBackground(true);  // should be true already, but isn't?
	    card->wstat->resize(xs+6, ydiv);
	    card->wstat->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	    card->wstat->setObjectName("staticform");
	}
	if (ydiv < ys) {
	    QFrame *f = new QFrame(wform);
	    f->setAutoFillBackground(true);  // should be true already, but isn't?
	    f->show(); // why is this needed?
	    f->move(0, ydiv);
	    f->resize(xs+6, ys-ydiv+6);
	    f->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	    f->setLineWidth(3);
	    f->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	    f->setObjectName("cardframe");
	    card->wcard = new QWidget(f);
	    card->wcard->resize(xs, ys-ydiv);
	    card->wcard->move(3, 3);
	    // probably not necessary to set resize policy
	    // and definitely not necessary to place a dummy label within
	    card->wcard->setObjectName("cardform");
	}
							/*-- create items --*/
	for (n=0; n < card->nitems; n++)
		create_item_widgets(card, n);
	// Weird.  Not only do these need explicit show()s, they need to
	// be done here, at the end.
	if(card->wstat)
		card->wstat->show();
	if(card->wcard)
		card->wcard->show();
	return(card);
}


/*
 * create the widgets in the card menu, at the location specified by the
 * item in the form. The resulting widgets are stored in the card item.
 */

#define JUST(j) Qt::AlignVCenter | \
	       (j==J_LEFT   ? Qt::AlignLeft : \
		j==J_RIGHT  ? Qt::AlignRight \
			     : Qt::AlignHCenter)

const char * const font_prop[F_NFONTS] = {
	"helvFont",
	"helvObliqueFont",
	"helvSmallFont",
	"helvLargeFont",
	"courierFont"
};

static QLabel *mk_label(QWidget *wform, ITEM &item, int w, int h)
{
	QLabel *lab = new QLabel(item.label, wform);
	lab->setObjectName("label");
	lab->move(item.x, item.y);
	lab->resize(w, h);
	lab->setAlignment(JUST(item.labeljust));
	lab->setProperty(font_prop[item.labelfont], true);
	return lab;
}

static QWidget *mk_groupbox(QWidget *wform, ITEM &item)
{
	QWidget *w;
	// Turns out QGroubBox always makes room for a title
	if(!BLANK(item.label)) {
		QGroupBox *gb = new QGroupBox(wform);
		gb->setTitle(item.label);
		gb->setAlignment(JUST(item.labeljust));
		gb->setProperty(font_prop[item.labelfont], true);
		w = gb;
	} else
		// so use a widget if no title.  I'd use a frame, but I have
		// no idea what style to use
		w = new QWidget(wform);
	w->setObjectName("label");
	w->move(item.x, item.y);
	w->resize(item.xs, item.ys);
	return w;
}

// The only way to suppress an event is to override it.  Qt doesn't
// provide any focus-related signals, anyway.

#define OVERRIDE_INIT(c,i) {\
	ITEM *ip; \
	if (c && (ip = c->form->items[i]) && \
	    !BLANK(ip->skip_if)) { \
		card = c; \
		item = i; \
	} else \
		card = NULL; \
}

#define OVERRIDE_FOCUS(p) \
	int item; \
	CARD *card; \
	bool refocusing = false; /* avoid refocus loop */ \
	void focus_init(CARD *c, int i) { card = c; item = i; } \
	void focusInEvent(QFocusEvent *e) { \
		Qt::FocusReason r = e->reason(); \
		if(card && !refocusing && \
		   (r == Qt::TabFocusReason || r == Qt::BacktabFocusReason) && \
		   evalbool(card, card->form->items[item]->skip_if)) { \
			e->accept(); \
			refocusing = true; \
			QWidget *w = this; \
			while(1) { \
				if(r == Qt::TabFocusReason) \
					w = w->nextInFocusChain(); \
				else \
					w = w->previousInFocusChain(); \
				if(!w) { \
					refocusing = false; \
					return; \
				} \
				if(w->focusPolicy() & Qt::TabFocus) { \
					w->setFocus(r); \
					refocusing = false; \
					return; \
				} \
			} \
		} \
		p::focusInEvent(e); \
	}

struct CardLineEdit : public QLineEdit {
	CardLineEdit(CARD *c, int i, QWidget *p) : QLineEdit(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QLineEdit)
};

struct CardComboBox : public QComboBox {
	CardComboBox(CARD *c, int i, QWidget *p) : QComboBox(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QComboBox)
};

struct CardDateTimeEdit : public QDateTimeEdit {
	CardDateTimeEdit(CARD *c, int i, QWidget *p) : QDateTimeEdit(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QDateTimeEdit)
};

struct CardDateEdit : public QDateEdit {
	CardDateEdit(CARD *c, int i, QWidget *p) : QDateEdit(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QDateEdit)
};

struct CardTimeEdit : public QTimeEdit {
	CardTimeEdit(CARD *c, int i, QWidget *p) : QTimeEdit(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QTimeEdit)
};

struct CardListWidget : public QListWidget {
	CardListWidget(CARD *c, int i, QWidget *p) : QListWidget(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QListWidget)
};

struct CardInputComboBox : public QComboBox {
	CardInputComboBox(CARD *c, int i, QWidget *p) : QComboBox(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QComboBox)
	/* The following are QStringList for convenient addItems() usage */
	/* I guess I could add it to carditem, but here is fine */
	QStringList c_static, c_dynamic;	/* what to fill in */
};

struct CardDoubleSpinBox : public QDoubleSpinBox {
	CardDoubleSpinBox(CARD *c, int i, QWidget *p) : QDoubleSpinBox(p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QDoubleSpinBox)
};

struct CardTextEdit : public QTextEdit {
	// Unlike others, this always needs card & item
	// Evaluating a blank skip_if is at least fast.
	CardTextEdit(CARD *c, int i, QWidget *p) : QTextEdit(p), item(i), card(c) {}
	OVERRIDE_FOCUS(QTextEdit)
	// There is no equivalent to QLineEdit's editingFinished, so at least
	// do something when focus is lost.
	// FIXME:  This probably gets called if it gets skipped due to skip_if
	void focusOutEvent(QFocusEvent *e) {
		QTextEdit::focusOutEvent(e);
		card_callback(item, card, true);
	}
};

struct CardRadioButton : public QRadioButton {
	CardRadioButton(CARD *c, int i, const QString &s, QWidget *p) : QRadioButton(s, p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QRadioButton)
};

struct CardCheckBox : public QCheckBox {
	CardCheckBox(CARD *c, int i, const QString &s, QWidget *p) : QCheckBox(s, p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QCheckBox)
};

struct CardPushButton : public QPushButton {
	CardPushButton(CARD *c, int i, const QString &s, QWidget *p) : QPushButton(s, p) OVERRIDE_INIT(c,i)
	OVERRIDE_FOCUS(QPushButton)
};

static void create_item_widgets(
	CARD		*card,		/* card the item is added to */
	int		nitem)		/* number of item being added */
{
	ITEM		item;		/* describes type and geometry */
	struct carditem	*carditem;	/* widget pointers stored here */
	QWidget		*wform;		/* static part form, or card form */
	BOOL		editable;

	item = *card->form->items[nitem];
	if (item.y < card->form->ydiv) {		/* static or card? */
		wform  = card->wstat;
	} else {
		wform  = card->wcard;
		item.y -= card->form->ydiv;
	}
	item.x  *= pref.scale;
	item.y  *= pref.scale;
	item.xs *= pref.scale;
	item.ys *= pref.scale;
	item.xm *= pref.scale;
	item.ym *= pref.scale;

	if (!wform)
		return;

	carditem = &card->items[nitem];
	carditem->w0 = carditem->w1 = 0;
	editable = item.type != IT_PRINT
			&& (!card->dbase || !card->dbase->rdonly)
			&& !card->form->rdonly
			&& !item.rdonly
			&& !evalbool(card, item.freeze_if);

	switch(item.type) {
	  case IT_LABEL:			/* a text without function */
		carditem->w0 = mk_label(wform, item, item.xs, item.ys);
		break;

	  case IT_INPUT:			/* arbitrary line of text */
	  case IT_PRINT:			/* non-editable text */
	  case IT_TIME:				/* date and/or time */
		if (item.xm > 6)
		  carditem->w1 = mk_label(wform, item, item.xm - 6, item.ys);
		{
		  QLineEdit *le = NULL;
		  CardInputComboBox *cb = NULL;
		  QDateTimeEdit *dt = NULL;
		  QWidget *w;
		  if (item.type == IT_TIME && item.timewidget) {
			switch(item.timefmt) {
			    case T_DATE:
				dt = new CardDateEdit(card, nitem, wform);
				dt->setDisplayFormat(pref.mmddyy ? "M/dd/yy" :
						                   "d.MM.yy");
				dt->setCalendarPopup(true);
				break;
			    case T_TIME:
			    case T_DURATION:
				dt = new CardTimeEdit(card, nitem, wform);
				dt->setDisplayFormat(pref.ampm ? "H:mma" :
						                 "HH:mm");
				break;
			    case T_DATETIME:
				dt = new CardDateTimeEdit(card, nitem, wform);
				dt->setDisplayFormat(pref.mmddyy ?
						       (pref.ampm ? "M/dd/yy  H:mma" :
						                    "M/dd/yy  HH:mm") :
						       (pref.ampm ? "d.MM.yy  H:mma" :
							            "d.MM.yy  HH:mm"));
				dt->setCalendarPopup(true);
				break;
			}
			w = dt;
		  	// le = dt->lineEdit();  // protected, of course.  I hate Qt.
		  } else if (item.type != IT_INPUT || (!item.nmenu && !item.dcombo) || !editable) {
			le = new CardLineEdit(card, nitem, wform);
			w = le;
		  } else {
			cb = new CardInputComboBox(card, nitem, wform);
			w = cb;
			cb->setEditable(true); // needs to be before lineEdit()
			le = cb->lineEdit();
			if (item.nmenu)
				for(int i = 0; i < item.nmenu; i++)
					cb->c_static.append(item.menu[i].label);
		  }
		  carditem->w0 = w;
		  w->setObjectName("input");
		  w->move(item.x + item.xm, item.y);
		  w->resize(item.xs - item.xm, item.ys);
		  if(dt)
			dt->setAlignment(JUST(item.inputjust));
		  else
			le->setAlignment(JUST(item.inputjust));
		  w->setProperty(font_prop[item.inputfont], true);
		  if(dt) {
			/* item.maxlen doesn't make sense for dates, anyway */
			dt->setReadOnly(!editable);
		  } else {
			le->setMaxLength(item.maxlen?item.maxlen:10);
			le->setReadOnly(!editable);
			// le->setTextMargins(l, r, 2, 2);
		  }
		  if (editable) {
#if 0 // Screw it.  Updating after every keystroke is insane (and breaks dates).
			  // Make this update way too often, just like IT_NOTE.
			  set_qt_cb(QLineEdit, textChanged, le,
				    card_callback(nitem, card, false));
#endif
			  if (dt)
				  set_spin_cb(dt, card_callback(nitem, card, true));
			  else if (cb)
				  // Only update dependent widgets when done
				  set_combo_cb(cb, card_callback(nitem, card, true));
			  else
				  // Only update dependent widgets when done
				  set_text_cb(le, card_callback(nitem, card, true));
		  }
		}
		break;

	  case IT_NUMBER: {				/* number spin box */
		// I used to use QSpinBox if digits == 0, but QSpinBox
		// only supports int, not long, and 2^32 is just too small
		// for things like storage sizes these days.  Besides, the
		// expression language deals exclusively with floats, anyway.
		if (item.xm > 6)
		  carditem->w1 = mk_label(wform, item, item.xm - 6, item.ys);
		QDoubleSpinBox *sb = new CardDoubleSpinBox(card, nitem, wform);
		carditem->w0 = sb;
		sb->setObjectName("input");
		sb->move(item.x + item.xm, item.y);
		sb->resize(item.xs - item.xm, item.ys);
		sb->setAlignment(JUST(item.inputjust));
		sb->setProperty(font_prop[item.inputfont], true);
		sb->setReadOnly(!editable);
		if(item.min == item.max) {
			if(!item.digits)
				// FIXME:  the -1 bit probably won't work
				item.max = pow(FLT_RADIX, DBL_MANT_DIG) - 1;
			else
				item.max = DBL_MAX;
			if(item.min < 0)
				item.min = -item.max;
		}
		sb->setRange(item.min, item.max);
		sb->setDecimals(item.digits);
		sb->setSingleStep(pow(10, -item.digits));
		// sb->setTextMargins(l, r, 2, 2);
		if (editable)
			set_spin_cb(sb, card_callback(nitem, card, true));
		break;
	  }

	  case IT_NOTE:				/* multi-line text */
		if (item.ym > 6)
			carditem->w1 = mk_label(wform, item, item.xs, item.ym);
		{
		  QTextEdit *te = new CardTextEdit(card, nitem, wform);
		  carditem->w0 = te;
		  te->setObjectName("note");
		  te->move(item.x, item.y + item.ym);
		  te->resize(item.xs, item.ys - item.ym);
		  te->setProperty(font_prop[item.inputfont], true);
		  te->setLineWrapMode(QTextEdit::NoWrap);
		  // tjm - FIXME: need to use a callback to enforce this
		  // te->setMaxLength(item.maxlen);
		  te->setAlignment(JUST(item.inputjust));
		  te->setReadOnly(!editable);
		  // QSS doesn't support :read-only for QTextEdit
		  if (!editable)
			te->setProperty("readOnly", true);
#if 0 // Screw it.  Updating after every keystroke is insane.
		   else {
			// This updates way too often:  every character.
			set_qt_cb(QTextEdit, textChanged, te, card_callback(nitem, card, false));
			// But there is no equivalent to editingFinished here
			// Instead, focusOutEvent is overridden above.
		   }
#endif
		}
		break;

	  case IT_MENU:
		if (item.xm > 6)
		  carditem->w1 = mk_label(wform, item, item.xm - 6, item.ys);
		{
		  QComboBox *cb = new CardComboBox(card, nitem, wform);
		  cb->setObjectName("menu");
		  cb->move(item.x + item.xm, item.y);
		  cb->resize(item.xs - item.xm, item.ys);
		  cb->setProperty(font_prop[item.inputfont], true);
		  for(int i = 0; i < item.nmenu; i++)
			cb->addItem(item.menu[i].label);
		  set_popup_cb(cb, card_callback(nitem, card, true, i), int, i);
		  carditem->w0 = cb;
		  break;
		}
	  case IT_MULTI:
		if (item.ym > 6)
		  carditem->w1 = mk_label(wform, item, item.xs, item.ym);
		{
		  char desc[3], &sep = desc[1], &esc = desc[0];
		  get_form_arraysep(card->form, &sep, &esc);
		  desc[2] = 0;
		  QListWidget *list = new CardListWidget(card, nitem, wform);
		  list->setObjectName("list");
		  list->move(item.x, item.y + item.ym);
		  list->resize(item.xs, item.ys - item.ym);
		  list->setProperty(font_prop[item.inputfont], true);
		  list->setSelectionMode(QAbstractItemView::MultiSelection);
		  for(int n = 0; n < item.nmenu; n++) {
			list->addItem(item.menu[n].label);
			if(!item.multicol) {
				char *val = item.menu[n].flagcode;
				int nesc = countchars(val, desc);
				if(nesc) {
					int vlen = strlen(val);
					/* no outlet for error, so fatal */
					val = alloc(0, "escaping", char, vlen + nesc + 1);
					*escape(val, item.menu[n].flagcode, vlen, esc, desc) = 0;
				}
				list->item(n)->setData(Qt::UserRole, val);
				if(nesc)
					free(val);
			}
		  }
		  set_qt_cb(QListWidget, itemSelectionChanged, list, card_callback(nitem, card));
		  carditem->w0 = list;
		  break;
		}

	  case IT_RADIO:
	  case IT_FLAGS: {
		carditem->w0 = mk_groupbox(wform, item);
		// This forces a "line break" if the computed size of the
		// buttons in the line is too wide.  When this happens,
		// the number of columns is reduced until all added widgets
		// fit.  Vertical fit is up to the user to fix.  The minimum
		// number of columns is 1, so that may require user action
		// to force a fit, as well.
		// I suppose I could modify the FlexLayout example instead
		// of doing this, but I don't feel like it.
		/* no outlet for error, so fatal */
		unsigned int *colwidth = alloc(0, "layout", unsigned int, item.nmenu);
		QGridLayout *l = new QGridLayout(carditem->w0);
		l->setSpacing(0); // default; qss may override
		l->setMargin(0);
		add_layout_qss(l, "buttongroup");
		unsigned int ncol = item.nmenu, col = 0, row = 0, curwidth = 0, n, bw;
		unsigned int mwidth = carditem->w0->contentsRect().width();
		for(n = 0; n < (unsigned int)item.nmenu; n++) {
			QAbstractButton *b;
			if(item.type == IT_RADIO)
				b = new CardRadioButton(card, nitem, item.menu[n].label, carditem->w0);
			else
				b = new CardCheckBox(card, nitem, item.menu[n].label, carditem->w0);
			b->setProperty(font_prop[item.inputfont], true);
			b->ensurePolished();
			bw = b->sizeHint().width();
			if(colwidth[col] < bw) {
				curwidth += bw - colwidth[col];
				colwidth[col] = bw;
			}
			// FIXME:  the layout's content margins may need
			//         to be added as well.
			if(col > 0 && !row)
				curwidth += l->spacing();
			while(ncol > 1 && curwidth > mwidth) {
				// The first row is cut off at col
				// Remaing rows are cut by 1 at a time
				ncol = row ? ncol - 1 : col;
				if(row && ncol > 1) {
					memset(colwidth, 0, ncol * sizeof(*colwidth));
					curwidth = row = col = 0;
					QListIterator<QObject *>iter(carditem->w0->children());
					while(iter.hasNext()) {
						QWidget *w = dynamic_cast<QAbstractButton *>(iter.next());
						if(!w)
							continue;
						bw = w->sizeHint().width();
						if(colwidth[col] < bw) {
							curwidth += bw - colwidth[col];
							colwidth[col] = bw;
						}
						// FIXME:  the layout's content margins may need
						//         to be added as well.
						if(col > 0 && !row)
							curwidth += l->spacing();
						if(curwidth > mwidth)
							break;
						if(++col == ncol) {
							col = 0;
							++row;
						}
					}
				} else {
					curwidth -= colwidth[col];
					// FIXME:  the layout's content margins may need
					//         to be removed as well.
					if(col > 0)
						curwidth -= l->spacing();
					++row;
					col = 0;
					bw = b->sizeHint().width();
					if(colwidth[col] < bw) {
						curwidth += bw - colwidth[col];
						colwidth[col] = bw;
					}
				}
			}
			if(++col >= ncol) {
				col = 0;
				++row;
			}
			set_button_cb(b, card_callback(nitem, card, c, n), bool c);
		}
		row = col = 0;
		QListIterator<QObject *>iter(carditem->w0->children());
		while(iter.hasNext()) {
			QWidget *w = dynamic_cast<QAbstractButton *>(iter.next());
			if(!w)
				continue;
			l->addWidget(w, row, col);
			if(++col == ncol) {
				col = 0;
				++row;
			}
		}
		free(colwidth);
		break;
	  }
	  case IT_CHOICE:			/* diamond on/off switch */
	  case IT_FLAG: {			/* square on/off switch */
		  QAbstractButton *b;
		  if (item.type == IT_CHOICE)
			  b = new CardRadioButton(card, nitem, item.label, wform);
		  else
			  b = new CardCheckBox(card, nitem, item.label, wform);
		  carditem->w0 = b;
		  b->move(item.x, item.y);
		  b->resize(item.xs, item.ys);
		  b->setAutoExclusive(false);
		  b->setProperty(font_prop[item.labelfont], true);
		  // Qt doesn't allow label alignment.  Probably for the best.
		  // b->setAlignment(JUST(item.labeljust));
		  // seriously, "label"?
		  b->setObjectName("label");
		  if (card->dbase && !card->dbase->rdonly
				&& !card->form->rdonly
				&& !item.rdonly)
			set_button_cb(b, card_callback(nitem, card, c), bool c);
	  }
		break;

	  case IT_BUTTON: {			/* pressable button */
		QPushButton *b = new CardPushButton(card, nitem, item.label, wform);
		carditem->w0 = b;
		b->move(item.x, item.y);
		b->resize(item.xs, item.ys);
		b->setProperty(font_prop[item.labelfont], true);
		b->setObjectName("button");
		set_button_cb(b, card_callback(nitem, card));
		break;
	  }

	  case IT_CHART: {			/* chart display */
		GrokChart *c = new GrokChart(wform);
		c->move(item.x, item.y);
		c->resize(item.xs, item.ys);
		// not sure where item.label should go, so I'll just drop it.
		c->setObjectName("chart");
		carditem->w0 = c;
		// callback is built-in event overrides
		break;
	  }
	  case IT_NULL:
	  case NITEMS: ;
	}
}


/*-------------------------------------------------- callbacks --------------*/


/*
 * the user pressed the mouse or the return key on an item in the card.
 * If the card references a row in a database, use store() to change the
 * column of that row that is referenced by the item. Redraw the entire
 * card (because a Choice item turns off other Choice items). This is
 * used for all item types except charts, which are more complicated
 * because of drawing and use built-in event overrides instead of a
 * callback.
 */

static void card_callback(
	int			nitem,
	CARD			*card,
	bool			flag,
	int			index)
{
	ITEM		*item;
	const char	*n;
	bool		redraw = true;

	if (nitem >= card->nitems ||			/* illegal */
	    card->dbase == 0	  ||			/* preview dummy card*/
	    card->row < 0)				/* card still empty */
		return;

	item = card->form->items[nitem];
	switch(item->type) {
	  case IT_INPUT:				/* arbitrary input */
	  case IT_TIME:					/* date and/or time */
	  case IT_NOTE:					/* multi-line text */
	  case IT_NUMBER:				/* numbers */
		// Note:  there used to be code here to advance via skip_if.
		// The code was never called in 1.5/OpenMotif-2.3.8, and
		// seriously broke the Qt version when I replaced the signal
		// with one that actually gets generated.
		// Now I respect skip_if on all fields via tab intercepts
		card_readback_texts(card, nitem);
		redraw = flag;
		break;

	  case IT_CHOICE:				/* diamond on/off */
		// flag should always be true, since radio buttons don't toggle
		// however, this callback may be accidentally invoked when
		// the other choices get unchecked
		if (!flag || !store(card, nitem, item->flagcode))
			return;
		break;

	   case IT_MENU:				/* menu item chosen */
	   case IT_RADIO:			/* radio selection changed */
		// flag should always be true, since radio buttons don't toggle
		// however, this callback may be accidentally invoked when
		// the other choices get unchecked
		if (!flag || !store(card, nitem, item->menu[index].flagcode))
			return;
		break;

	  case IT_FLAG:					/* square on/off */
		if (!(n = item->flagcode))
			return;
		// Old code toggled database value directly.  Now it
		// respects the state of the GUI.
		if (!store(card, nitem, flag ? n : 0))
			return;
		break;

	   case IT_FLAGS: {			/* square in group on/off */
		if (item->multicol) {
			if (!store(card, nitem,
				   flag ? item->menu[index].flagcode : NULL,
				   index))
				return;
			break;
		}
		char *old = dbase_get(card->dbase, card->row, item->column);
		char *val = item->menu[index].flagcode;
		// blanks aren't valid, but form validator should deal with it
		// if(!val || !*val) return;
		int vlen = strlen(val);
		if(!old && !flag) // first shortcut: blank means all unset
			   return;
		// escape code if necessary
		char desc[3], &sep = desc[1], &esc = desc[0];
		get_form_arraysep(card->form, &sep, &esc);
		desc[2] = 0;
		int nesc = countchars(val, desc);
		if(nesc) {
			/* no outlet for error, so fatal */
			val = alloc(0, "escaping", char, nesc + vlen + 1);
			*escape(val, item->flagcode, vlen, esc, desc) = 0;
			vlen += nesc;
		}
		if(!old) { // second shortcut: setting from blank is escaped val
			if(!store(card, nitem, val)) {
				if(nesc)
					free(val);
				return;
			}
			if(nesc)
				free(val);
			break;
		}
		// final shortcut: if already in/out as needed, do nothing
		int obegin, oafter;
		if(find_elt(old, val, vlen, &obegin, &oafter, sep, esc) == flag) {
			if(nesc)
				free(val);
			return;
		}
		char *tmp;
		if(flag) { // add to old at insert loc returned by find_elt
			int len = strlen(old);
			/* no outlet for error, so fatal */
			tmp = alloc(0, "flag extract", char, len + vlen + 2);
			memcpy(tmp, old, obegin);
			if(obegin)
				tmp[obegin++] = sep;
			memcpy(tmp + obegin, val, vlen);
			if(!obegin)
				tmp[vlen + obegin++] = sep;
			memcpy(tmp + vlen + obegin, old + obegin - 1, len - obegin + 2);
		} else { // remove from old
			tmp = mystrdup(old);
			if(old[oafter])
				memcpy(tmp + obegin, old + oafter + 1,
				       strlen(old + oafter));
			else
				tmp[obegin ? obegin - 1 : 0] = 0;
		}
		if(nesc)
			free(val);
		if (!store(card, nitem, *tmp ? tmp : NULL)) {
			free(tmp);
			return;
		}
		free(tmp);
		break;
	  }

	   case IT_MULTI: {		/* selection set changed in list */
		/* Since there is no single element change like above, */
		/* rebuild from scratch every time */
		QListWidget *lw = reinterpret_cast<QListWidget *>(card->items[nitem].w0);
		if(item->multicol) {
			for(int m = 0; m < item->nmenu; m++) {
				char *val = NULL;
				if(lw->item(m)->checkState() == Qt::Checked)
					val = item->menu[m].flagcode;
				if (!store(card, nitem, val, m))
					return;
			}
		} else {
			char sep, esc;
			get_form_arraysep(card->form, &sep, &esc);
			QString val;
			QListIterator<QListWidgetItem *> sel(lw->selectedItems());
			while(sel.hasNext()) {
				const QListWidgetItem *item = sel.next();
				if(val.length())
					val += sep;
				val += item->data(Qt::UserRole).toString();
			}
			char *s = qstrdup(val);
			// FIXME: use insertion sort above, instead.
			//        or at least don't use 16-bit QStrings.
			if(!toset(s, sep, esc))
				fatal("No memory for set conversion");
			if (!store(card, nitem, *s ? s : NULL)) {
				free(s);
				return;
			}
			free(s);
		}
		break;
	  }

	  case IT_BUTTON:				/* pressable button */
		if ((redraw = item->pressed)) {
			if (switch_name) free(switch_name);
			if (switch_expr) free(switch_expr);
			switch_name = switch_expr = 0;
			n = evaluate(card, item->pressed);
			if (switch_name) {
				switch_form(switch_name);
				card = curr_card;
			}
			if (switch_expr)
				search_cards(SM_SEARCH, card, switch_expr);
			if (!BLANK(n)) {
				int UNUSED ret = system(n);
			}
			if (switch_name) free(switch_name);
			if (switch_expr) free(switch_expr);
			switch_name = switch_expr = 0;
		}
		break;
	  default: ;
	}
	if (redraw)
		fillout_card(card, TRUE);
}


/*
 * read back all text fields. This is necessary whenever a card is removed
 * or redrawn because we might not have received a callback for a changed
 * text input field. We get callbacks only if the user presses Return, and
 * not even that on multi-line IT_NOTE fields.
 * The <which> parameter, if >= 0, narrows the readback to a particular item.
 */

void card_readback_texts(
	CARD		*card,		/* card that is displayed in window */
	int		which)		/* all if < 0, one item only if >= 0 */
{
	int		nitem;		/* counter for searching text items */
	int		start, end;	/* first and last item to read back */
	char		*data;		/* data string to store */
	time_t		time;		/* parsed time */
	char		buf[40];	/* if numeric time, temp string */

	if (!card || !card->form || !card->form->items || !card->dbase
		  ||  card->form->rdonly
		  ||  card->dbase->rdonly
		  ||  card->row >= card->dbase->nrows)
		return;
	start = which >= 0 ? which : 0;
	end   = which >= 0 ? which : card->nitems-1;
	for (nitem=start; nitem <= end; nitem++) {
		if (!card->items[nitem].w0		||
		    card->form->items[nitem]->rdonly)
			continue;
		switch(card->form->items[nitem]->type) {
		  case IT_INPUT:
			data = read_text_button(card->items[nitem].w0, 0);
			(void)store(card, nitem, data);
			break;

		  case IT_NOTE:
			data = read_text_button_noskipblank(
						card->items[nitem].w0, 0);
			(void)store(card, nitem, data);
			break;

		  case IT_TIME:
			if (card->form->items[nitem]->timewidget) {
				time = reinterpret_cast<QDateTimeEdit *>
					(card->items[nitem].w0)->dateTime().toSecsSinceEpoch();
				sprintf(buf, "%ld", (long)time);
			} else {
				data = read_text_button(card->items[nitem].w0, 0);
				while (*data == ' ' || *data == '\t' || *data == '\n')
					data++;
				if (!*data)
					*buf = 0;
				else {
					switch(card->form->items[nitem]->timefmt) {
					    case T_DATE:
						time = parse_datestring(data);
						break;
					    case T_TIME:
						time = parse_timestring(data, FALSE);
						break;
					    case T_DATETIME:
						time = parse_datetimestring(data);
						break;
					    case T_DURATION:
						time = parse_timestring(data, TRUE);
					}
					sprintf(buf, "%ld", (long)time);
				}
			}
			(void)store(card, nitem, buf);
			if (which >= 0)
				fillout_item(card, nitem, FALSE);
			break;

		  case IT_NUMBER:
			sprintf(buf, "%.*lg", DBL_DIG + 1,
				reinterpret_cast<QDoubleSpinBox *>(card->items[nitem].w0)->value());
			(void)store(card, nitem, buf);
			break;

		  default: ;
		}
	}
}


/*
 * store a data item in the database. The database is just a big array of
 * rows and columns of char strings indexed by the card's row and the item
 * in the card to be set. Items reference a column. Some items, like labels,
 * do not reference any column at all, do not call store() with one of these.
 * This function may modify card->row because of resorting.
 */

static BOOL store(
	CARD		*card,		/* card the item is added to */
	int		nitem,		/* number of item being added */
	const char	*string,	/* string to store in dbase */
	int		menu)		/* menu item to store, if applicable */
{
	BOOL		newsum = FALSE;	/* must redraw summary? */
	const ITEM	*item;
	int		col;

	if (nitem >= card->nitems		||
	    card->dbase == 0			||
	    card->dbase->rdonly			||
	    card->form->items[nitem]->rdonly || card->form->rdonly)
		return(FALSE);

	item = card->form->items[nitem];
	col = item->multicol ? item->menu[menu].column : item->column;
	if (!dbase_put(card->dbase, card->row, col, string))
		return(TRUE);

	print_info_line();
	if (pref.sortcol == col) {
		dbase_sort(card, pref.sortcol, pref.revsort);
		newsum = TRUE;
	} else
		replace_summary_line(card, card->row);

	if (pref.autoquery) {
		do_query(card->form->autoquery);
		newsum = TRUE;
	}
	if (newsum) {
		create_summary_menu(card);
		scroll_summary(card);
	}
	return(TRUE);
}


/*-------------------------------------------------- drawing ----------------*/
/*
 * return the text representation of the data string of an IT_TIME database
 * item. This is used by fillout_item and make_summary_line.
 */

const char *format_time_data(
	const char	*data,		/* string from database */
	TIMEFMT		timefmt)	/* new format, one of T_* */
{
	static char	buf[40];	/* for date/time conversion */
	int		value = data ? atoi(data) : 0;

	if (!data)
		return("");
	value = atoi(data);
	switch(timefmt) {
	  case T_DATE:
		data = mkdatestring(value);
		break;
	  case T_TIME:
		data = mktimestring(value, FALSE);
		break;
	  case T_DURATION:
		data = mktimestring(value, TRUE);
		break;
	  case T_DATETIME:
		sprintf(buf, "%s  %s", mkdatestring(value),
				       mktimestring(value, FALSE));
		data = buf;
	}
	return(data);
}


/*
 * fill out a card, using the data from the row in the database referenced
 * in the card. If there is no database, print empty strings. If the deps
 * argument is true, a field has changed (and got reprinted separately), so
 * only PRINT and CHART items that may need updating are reprinted. This is
 * done to speed things up. It is called by routines that call store().
 */

void fillout_card(
	CARD		*card,		/* card to draw into menu */
	BOOL		deps)		/* if TRUE, dependencies only */
{
	int		i;		/* item counter */

	if (card) {
		card->disprow = card->row;
		for (i=0; i < card->nitems; i++)
			fillout_item(card, i, deps);
	}
	// only rebuild if doing a full rebuild
	if (!deps)
		remake_section_popup(FALSE);
}


static const char *idefault(CARD *card, ITEM *item)
{
	char *idef = item->idefault;
	const char *eval;
	if(!idef || !*idef)
		return "";
	eval = evaluate(card, idef);
	return STR(eval);
}

void fillout_item(
	CARD		*card,		/* card to draw into menu */
	int		i,		/* item index */
	BOOL		deps)		/* if TRUE, dependencies only */
{
	BOOL		sens, vis;	/* (de-)sensitize item */
	ITEM		*item;		/* describes type and geometry */
	QWidget		*w0, *w1;	/* input widget(s) in card */
	const char	*data = NULL;	/* value string in database */

	w0   = card->items[i].w0;
	w1   = card->items[i].w1;
	if (w0) w0->blockSignals(true);
	item = card->form->items[i];
	// FIXME:  Should above-div items always be excluded, like sens?
	vis = !item->invisible_if ||
	      (card->dbase && card->row >= 0
			   && card->row < card->dbase->nrows
			   && !evalbool(card, item->invisible_if));
	if (w0) w0->setVisible(vis);
	if (w1) w1->setVisible(vis);

	// FIXME:  Should above-div items always be excluded?
	// FIXME:  Should IT_LABELs be disabled when no data is loaded?
	sens = (item->type == IT_BUTTON && !item->gray_if) ||
	       item->y < card->form->ydiv ||
	       (card->dbase && card->row >= 0
			    && card->row < card->dbase->nrows
			    && !evalbool(card, item->gray_if));
	if(!item->multicol)
		data = dbase_get(card->dbase, card->row, item->column);

	if (w0) w0->setEnabled(sens);
	if (w1) w1->setEnabled(sens);

	switch(item->type) {
	  case IT_TIME:
		if (item->timewidget) {
			if (!deps) {
				QDateTime dt;
				dt.setSecsSinceEpoch(atol(STR(data)));
				reinterpret_cast<QDateTimeEdit *>(w0)->
					setDateTime(dt);
			}
			break;
		}
		data = format_time_data(data, item->timefmt);
		FALLTHROUGH
	  case IT_INPUT:
		if (!deps) {
			CardInputComboBox *cb = dynamic_cast<CardInputComboBox *>(w0);
			if(item->dcombo != C_NONE) {
				// sets are hash tables.  I'd rather have
				// sorted lists using insertion sort to avoid
				// the sort() call later, but I take what I
				// can get.  At least this is better than my
				// previous code that stuck to lists (very
				// likely using linear searches)
				QSet<QString> set;
				int nmax = card->dbase && item->dcombo == C_ALL ? card->dbase->nrows : card->nquery;
				for(int n = 0; n < nmax; n++) {
					char *other;
					int idx = item->dcombo == C_ALL ? n : card->query[n];
					if(card->row == idx)
						continue;
					other = dbase_get(card->dbase, idx, card->form->items[i]->column);
					if(!BLANK(other))
						set.insert(other);
				}
				// Remove duplicates of statics
				QStringListIterator iter(cb->c_static);
				while(iter.hasNext())
					set.remove(iter.next());
				QStringList &l = cb->c_dynamic;
				l = set.toList();
				// sorting is probably useful regardless
				// but I won't sort the statics
				l.sort();
			}
			if(item->menu || item->dcombo) {
				QSignalBlocker sb(cb->lineEdit());
				cb->clear();
				cb->addItems(cb->c_static);
				cb->addItems(cb->c_dynamic);
			}
			// The old code also moved the cursor, but that was never such a good idea
			print_text_button_s(w0, data ? data : idefault(card, item));
		}
		break;

	  case IT_NUMBER:
		if (!deps) {
			if (!data)
				data = idefault(card, item);
			reinterpret_cast<QDoubleSpinBox *>(w0)->
				setValue(data ? atof(data) : 0);
		}
		break;

	  case IT_NOTE:
		if (!deps && w0) {
			print_text_button_s(w0, data ? data : idefault(card, item));
			reinterpret_cast<QTextEdit *>(w0)->textCursor().setPosition(0);
		}
		break;

	  case IT_PRINT:
		print_text_button_s(w0, evaluate(card, item->idefault));
		break;

	  case IT_CHOICE:
	  case IT_FLAG:
		set_toggle(w0, data && item->flagcode
				    && !strcmp(data, item->flagcode));
		break;

	  case IT_MENU:
		for(int n = 0; n < item->nmenu; n++) {
			// if(!item->menu[n].flagcode) continue;
			if(!strcmp(STR(item->menu[n].flagcode), STR(data))) {
				reinterpret_cast<QComboBox *>(w0)->setCurrentIndex(n);
				break;
			}
		}
		// if n == item->nmenu, unknown value - this will act strange
		break;

	  case IT_RADIO:
	  case IT_FLAGS: {
		int n = 0;
		char desc[3], &sep = desc[1], &esc = desc[0];
		get_form_arraysep(card->form, &sep, &esc);
		desc[2] = 0;
		QListIterator<QObject *>iter(w0->children());
		while(iter.hasNext()) {
			QAbstractButton *w = dynamic_cast<QAbstractButton *>(iter.next());
			if(!w)
				continue;
			char *code = item->menu[n].flagcode;
			if(item->type == IT_RADIO)
				w->setChecked(!strcmp(STR(code), STR(data)));
			else if(item->multicol) {
				data = dbase_get(card->dbase, card->row,
						 item->menu[n].column);
				w->setChecked(!strcmp(STR(data), code));
			} else if(BLANK(data)) // shortcut
				w->setChecked(false);
			else {
				int qbegin, qafter;
				w->setChecked(find_unesc_elt(data, code,
						       &qbegin, &qafter, sep, esc));
			}
			n++;
		}
		break;
	  }

	  case IT_MULTI: {
		QListWidget *l = reinterpret_cast<QListWidget *>(w0);
		if(item->multicol) {
			for(int n = 0; n < item->nmenu; n++) {
				data = dbase_get(card->dbase, card->row,
						 item->menu[n].column);
				l->item(n)->setSelected(!strcmp(STR(data),
								item->menu[n].flagcode));
			}
		} else {
			char desc[3], &sep = desc[1], &esc = desc[0];
			get_form_arraysep(card->form, &sep, &esc);
			desc[2] = 0;
			get_form_arraysep(card->form, &sep, &esc);
			QSignalBlocker blk(l);
			for(int n = 0; n < item->nmenu; n++) {
				char *code = item->menu[n].flagcode;
				int fbegin, fafter;
				l->item(n)->setSelected(BLANK(data) ? false :
					find_unesc_elt(data, code,
						       &fbegin, &fafter,
						       sep, esc));
			}
		}
		break;
	  }

	  case IT_CHART:
		draw_chart(card, i);
		break;
	  case IT_BUTTON: /* nothing to do */
	  case IT_LABEL: /* nothing to do */
	  case IT_NULL:
	  case NITEMS: ;
	}
    if (w0) w0->blockSignals(false);
}
