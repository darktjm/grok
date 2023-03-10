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
static bool store(CARD *, int, const char *, int menu = 0, bool force = false);

// The Card window can be open multiple times, and should self-destruct on
// close.  The old close callback also freed the card, as below.
class CardWindow : public QDialog {
public:
    CARD *card;
    ~CardWindow() { free_card(card); }
};

/* ItemEd is like CardWindow, but actually edits an item */
ItemEd::ItemEd(QWidget *parent, const FORM *form, int row,
	       bool init, int rest_item) : QDialog(parent ? parent : mainwindow)
{
	setAttribute(Qt::WA_DeleteOnClose);
	QBoxLayout *l = new QBoxLayout(QBoxLayout::TopToBottom, this);
	QWidget *w = new QWidget;
	w->setObjectName("wform");
	l->addWidget(w);
	l->addWidget(mk_separator());
	QDialogButtonBox *bb = new QDialogButtonBox;
	QAbstractButton *b = mk_button(bb, "Done", dbbr(Accept));
	set_button_cb(b, accept()); /* why is this necessary? */
	l->addWidget(bb);
	char *rest_val = 0;
	if(rest_item >= 0)
		rest_val = zstrdup(dbase_get(form->dbase, row,
					     form->items[rest_item]->column));
	card = create_card_menu(const_cast<FORM *>(form),
				w, false, rest_item, rest_val);
	card->row = row;
	card->shell = this;
	/* following is from mainwin.c: add_card */
	if (init)
		for (int i=0; i < form->nitems; i++) {
			const ITEM *item = form->items[i];
			if (IN_DBASE(item->type) && item->idefault)
				dbase_put(card->form->dbase, row, item->column,
					  evaluate(card, item->idefault));
		}
	fillout_card(card, false);
	/* select first text input widget for focus if possible */
	for (int i=0; i < form->nitems; i++) {
		const ITEM *item = form->items[i];
		if (item->type == IT_INPUT || item->type == IT_TIME
					   || item->type == IT_NOTE
					   || item->type == IT_NUMBER) {
			card->items[i].w0->setFocus();
			break;
		}
	}
	setWindowTitle("Edit Card");
	set_icon(this, 1);
	setModal(true);
}

ItemEd::~ItemEd()
{
	card_readback_texts(card, -1);
	for(CARD *c = card_list; c; c = c->next)
		if(c->fkey_next == card)
			c->fkey_next = c; // flag: don't traverse when freeing
	free_card(card);
}

CARD *card_list;	/* all allocated cards */

void free_card(
	CARD		*card)		/* card to destroy */
{
	CARD **prev;
	query_none(card);
	zfree(card->sorted);
	zfree(card->prev_form);
	zfree(card->letter_mask);
	zfree(card->rest_val);
	for(prev = &card_list; *prev; prev = &(*prev)->next)
		if(*prev == card) {
			*prev = card->next;
			break;
		}
	form_delete(card->form);
	dbase_prune();
	free(card);
}

void free_fkey_card(
	CARD		*card)		/* card to destroy */
{
	if (!card)
		return;
	CARD *next;
	while ((next = card->fkey_next)) {
		free_card(card);
		if(card == next) // flag set by ~ItemEd: don't traverse further
			break;
		card = next;
	}
}

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
	if (!card)
		return;
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
	tzero(struct carditem, card->items, card->nitems);
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
	QWidget		*wform,		/* form widget to install into, or 0 */
	bool		no_gui,		/* true to just init card */
	int		rest_item,	/* parent restrict GUI item */
	char		*rest_val)	/* parent restrict GUI val */
{
	CARD		*card;		/* new card being allocated */
	int		n;

							/*-- alloc card --*/
	n = sizeof(CARD);
	if(!no_gui)
		n += sizeof(struct carditem) * form->nitems;
	if (!(card = (CARD *)calloc(n, 1)))
		return(NULL);
	card->next = card_list;
	card_list = card;
	card->form   = form;
	card->row    = -1;
	card->rest_item = rest_item;
	card->rest_val = rest_val;
	card->nitems = no_gui ? 0 : form->nitems;
	if (!no_gui)
		build_card_menu(card, wform);
	return(card);
}

void build_card_menu(
	CARD		*card,		/* initialized non-GUI card */
	QWidget		*wform)		/* form widget to install into, or 0 */
{
	FORM		*form = card->form;
	int		xs, ys, ydiv;	/* card size and divider */
	int		n;

	xs   = pref.scale * form->xs;
	ys   = pref.scale * form->ys;
	ydiv = pref.scale * form->ydiv;
							/*-- make form --*/
	if (wform) {
		wform->setFixedSize(xs+6, ys+6);
		card->wform = wform;
	} else {
		CardWindow *cw = new CardWindow;
		cw->card = card;
		cw->setAttribute(Qt::WA_DeleteOnClose);
		card->shell = cw;
		card->shell->setWindowTitle("Card");
		set_icon(card->shell, 1);
		card->shell->setFixedSize(xs+6, ys+6);
		wform = card->wform = card->shell;
		card->shell->setObjectName("wform");
		popup_nonmodal(card->shell);
	}
	card->wstat = card->wcard = 0;
	if (ydiv) {
	    card->wstat = new QWidget(wform);
	    card->wstat->setAutoFillBackground(true);  // should be true already, but isn't?
	    card->wstat->resize(xs+6, ydiv);
	    card->wstat->setObjectName("staticform");
	}
	if (ydiv < ys) {
	    QFrame *f = new QFrame(wform);
	    f->setAutoFillBackground(true);  // should be true already, but isn't?
	    f->move(0, ydiv);
	    f->setFixedSize(xs+6, ys-ydiv+6);
	    f->show(); // why is this needed?
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
		// so use a frame if no title.  I have no idea what style to use,
		// since Qt doesn't tell me what style the groupbox uses.
		w = new QFrame(wform);
	w->setObjectName("groupbox");
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

/* QCalandarWidget which reverts to today if the date is 0 */
struct myCalendarWidget : public QCalendarWidget {
/* setSelected Date isn't virtual, so this is convoluted */
#if 0
	myCalendarWidget(QWidget *p = nullptr) : QCalendarWidget(p) {};
	void setSelectedDate(const QDate &date) {
		QDate d = date;
		if((QDateTime(date, QTime()).toSecsSinceEpoch()) <= 0)
			d = QDate::currentDate();
		QCalendarWidget::setSelectedDate(d);
	}
#else
	myCalendarWidget(QWidget *p = nullptr) : QCalendarWidget(p) {
		QObject::connect(this, &QCalendarWidget::selectionChanged,
				 this, &myCalendarWidget::valChanged);
	};
	void valChanged() {
		QDate d = selectedDate();
		if((QDateTime(d, QTime()).toSecsSinceEpoch()) <= 0)
			setSelectedDate(QDate::currentDate());
	}
#endif
};

struct CardDateTimeEdit : public QDateTimeEdit {
	CardDateTimeEdit(CARD *c, int i, QWidget *p) : QDateTimeEdit(p) {
		OVERRIDE_INIT(c,i)
		setCalendarPopup(true);
		cw = new myCalendarWidget();
		setCalendarWidget(cw);
		setFormat();
		/* setDateTime isn't virtual, so signal is used to set format */
		QObject::connect(this, &QDateTimeEdit::dateTimeChanged,
				 this, &CardDateTimeEdit::setFormat);
	}
	void setFormat() {
#if 0
		if(dateTime().toSecsSinceEpoch() <= 0) {
			/* FIXME:  there is no way to remove the date */
			setDisplayFormat(" ");
			return;
		}
#endif
		setDisplayFormat(pref.mmddyy ?
				       (pref.ampm ? "M/dd/yy  H:mma" :
				                    "M/dd/yy  HH:mm") :
				       (pref.ampm ? "d.MM.yy  H:mma" :
					            "d.MM.yy  HH:mm"));
	}
        myCalendarWidget *cw;
	~CardDateTimeEdit() { delete cw; }
	OVERRIDE_FOCUS(QDateTimeEdit)
};

struct CardDateEdit : public QDateEdit {
	CardDateEdit(CARD *c, int i, QWidget *p) : QDateEdit(p) {
		OVERRIDE_INIT(c,i)
		setCalendarPopup(true);
		cw = new myCalendarWidget();
		setCalendarWidget(cw);
		setFormat();
		/* setDateTime isn't virtual, so signal is used to set format */
		QObject::connect(this, &QDateEdit::dateTimeChanged,
				 this, &CardDateEdit::setFormat);
	}
	void setFormat() {
#if 0
		QDateTime d(dateTime());
		d.setTime(QTime());
		if(d.toSecsSinceEpoch() <= 0) {
			/* FIXME:  there is no way to remove the date */
			setDisplayFormat(" ");
			return;
		}
#endif
		setDisplayFormat(pref.mmddyy ? "M/dd/yy" :
				     "d.MM.yy");
	}
        myCalendarWidget *cw;
	~CardDateEdit() { delete cw; }
	OVERRIDE_FOCUS(QDateEdit)
};

/* Given how broken the above is, maybe allow a plain text box w/ calendar */
struct CardInputCalendar : public QComboBox {
	CardInputCalendar(CARD *c, int i, QWidget *p, bool t) : QComboBox(p) {
		OVERRIDE_INIT(c,i)
		setEditable(true);
		setInsertPolicy(NoInsert);
		addItem("");
		has_time = t;
		cw = new QCalendarWidget();
		cw->hide();
		/* keep cw in sync w/ text */
		QObject::connect(this, &QComboBox::currentTextChanged,
				 this, &CardInputCalendar::setCalendar);
#if 0
		/* toggle calendar off w/ combo sel -- isn't ever called */
		/* instead, showPopup is called on every press */
		QObject::connect(this, &QComboBox::activated,
				 this, &CardInputCalendar::togCalendar);
#endif
#if 0
		/* any date selection changes field */
		QObject::connect(cw, &QCalendarWidget::selectionChanged,
				 this, &CardInputCalendar::calDate);
#endif
		/* double-click or enter closes widget */
		QObject::connect(cw, &QCalendarWidget::activated,
				 this, &CardInputCalendar::doneCalendar);
	}
	void doneCalendar() {
		calDate();
		hidePopup();
	}
	void calDate() {
		const QDate &date = cw->selectedDate();
		if(!has_time)
			setCurrentText(date.toString(pref.mmddyy ? "M/dd/yy" :
						     "d.MM.yy"));
		else {
			std::string s = currentText().toStdString();
			time_t sec = parse_datetimestring(s.c_str());
			QDateTime d;
			d.setSecsSinceEpoch(sec);
			d.setDate(date);
			setCurrentText(date.toString(pref.mmddyy ?
				       (pref.ampm ? "M/dd/yy  H:mma" :
				                    "M/dd/yy  HH:mm") :
				       (pref.ampm ? "d.MM.yy  H:mma" :
					            "d.MM.yy  HH:mm")));
		}
	}
	void showPopup() {
		if(!cw->isHidden()) {
			hidePopup();
			return;
		}
		QPoint pos = (this->layoutDirection() == Qt::RightToLeft) ? this->rect().bottomRight() : this->rect().bottomLeft();
		pos = this->mapToGlobal(pos);
		/* no way to handle popup falling "off screen" w/o qt private */
		cw->move(pos);
		cw->show();
	}
	void hidePopup() {
		cw->hide();
		QComboBox::hidePopup();
	}
	/* move currentText into calendar */
	void setCalendar() {
		std::string s = currentText().toStdString();
		time_t sec = has_time ? parse_datetimestring(s.c_str()) :
			                parse_datestring(s.c_str());
		QDateTime d;
		d.setSecsSinceEpoch(sec);
		cw->setSelectedDate(d.date());
	}
	OVERRIDE_FOCUS(QComboBox)
	bool has_time;
	QCalendarWidget *cw;
	~CardInputCalendar() { delete cw; }
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

struct FKeySelector : public CardComboBox {
	FKeySelector(CARD *c, int i, QWidget *p, CARD *fc, ITEM *fit, bool ro, int n) :
		CardComboBox(c, i, p), fcard(fc), next(this), item(fit), fkey(n) {
		if(ro) {
			setProperty("readOnly", true);
			// FIXME:  this should instead transform to read-only
			// lineedit like the regular input+menu does
			setEnabled(false);
		}
	}

	void AddGroup(FKeySelector *f) {
		FKeySelector **prev = &f->next;
		next = f;
		while(*prev != f)
			prev = &(*prev)->next;
		*prev = this;
	}
	void fcard_from(FKeySelector *fk) {
		fcard = fk->fcard;
		if(!share) {
			share = fk;
			if(!fk->share)
				fk->share = this;
			else {
				FKeySelector *f;
				for(f = fk; f->share != fk; f = f->share);
				f->share = this;
			}
				
		}
	}
	/* refilter all widgets */
	void refilter() {
		set_filter();
		for(FKeySelector *f = next; f != this; f = f->next)
			f->set_filter();
	}
	/* look up row in fcard->dbase corresponding to given row in base table */
	int lookup(const char *key, int keyno, bool only = false, CARD *fc = 0)
	{
		if (!fc)
			fc = fcard;
		CARD *oc = fc->fkey_next;
		if (oc->fkey_next) {
			int r = lookup(key, keyno, only, oc);
			if (only || r < 0)
				return r;
			key = dbase_get(fc->form->dbase, r, fc->form->items[fc->qcurr]->column);
			keyno = 0; /* no way to handle multi here */
		}
		resolve_fkey_fields(oc->form->items[fc->qcurr]);
		return fkey_lookup(fc->form->dbase, oc->form->items[fc->qcurr],
				   key, keyno);
	}
	void clear_group(FKeySelector *end = 0) {
		if (end == this)
			return;
		base_row = -1;
		QSignalBlocker sb(this);
		setCurrentIndex(0);
		next->clear_group(end ? end : this);
	}
	void group_setval(int row, const char *key, int keyno);
	// I wanted to just scan up parent() for a QTableWidget, and check
	// if # rows == 0, but by the time this gets called as a child of
	// the tw being deleted, the tw has transformed to a plain QWidget
	// It would still be possible to account for that, but the "share"
	// link isn't that terrible.
	~FKeySelector() {
		FKeySelector *f;
		if(share) {
			FKeySelector *o = share->share;
			if(o == this)
				share->share = 0;
			else {
				for(f = share; f->share != this; f = f->share);
				f->share = share;
			}
		} else
			free_cards();
		for(f = this; f->next != this; f = f->next);
		f->next = next;
	}
    private:
	void free_cards() {
		FKeySelector *f = this;
		do {
			if (!f->fcard || !f->fcard->fkey_next) {
				f->fcard = 0;
				f = f->next;
				continue;
			}
			for (FKeySelector *f2 = f->next; f2 != f; f2 = f2->next) {
				if (!f2->fcard)
					continue;
				for (CARD *p = f->fcard; p && p->fkey_next; p = p->fkey_next) {
					CARD **q = &f2->fcard;
					while ((*q)->fkey_next && *q != p)
						q = &(*q)->fkey_next;
					if (*q == p) {
						*q = 0;
						break;
					}
				}
				if(f2->fcard && !f2->fcard->fkey_next)
					f2->fcard = 0;
			}
			free_fkey_card(f->fcard);
			f->fcard = 0;
			// The only time this should affect anything is on
			// full table delete, where all should end up 0
			// anyway
			for(FKeySelector *o = f->share; o && o != f; o = o->share)
				o->fcard = 0;
			f = f->next;
		} while (f != this);
	}
	typedef std::vector<bool> rowrestrict;
	// Remove rows in c->dbase that don't match val in col
	// Also, set base_row if using base card and only one row matches
	// If br_only is true, just set base_row
	void row_restrict(const CARD *c, int col, const char *val, rowrestrict &restrict,
			  bool br_only = false)
	{
		base_row = -1;
		for (int r = 0; r < c->form->dbase->nrows; r++)
			if (!restrict[r]) {
				const char *dv = dbase_get(c->form->dbase, r, col);
				bool is_ok = !strcmp(STR(dv), STR(val));
				if (!br_only)
					restrict[r] = !is_ok;
				// card w/ fkey field has fkey_next == 0
				// base card is card pointing to that
				if (is_ok && !c->fkey_next->fkey_next)
					base_row = base_row == -1 ? r : -2;
				if (br_only && base_row == -2)
					return;
			}
	}

	// Restrict an fkey field based on restrictions on foreign db value
	// fc/frestrict = foreign db; item is def of fkey field
	// fc->fkey_next is the child db to restrict
	void fkey_restrict(const CARD *fc, const ITEM *item, rowrestrict &restrict,
			   rowrestrict &frestrict)
	{
		const CARD *c = fc->fkey_next;
		for (int r = 0; r < fc->form->dbase->nrows; r++)
			if (!frestrict[r]) {
				char *v = fkey_of(fc->form->dbase, r, item);
				row_restrict(c, item->column, v, restrict);
				zfree(v);
			}
	}

	// Create a rowrestrict (ret) that is true if a row is to be suppressed
	//  fk = widget to restrict
	//  fc = card to restrict on (fc->dbase)
	void restrict(const FKeySelector *fk, const CARD *fc, rowrestrict &ret) {
		bool recurse = fk != this;
		base_row = -1;
		rowrestrict tmp;
		rowrestrict &cur = fc == fk->fcard ? ret : tmp;
		/* first, restrict based on current value */
		cur.resize(fk->fcard->form->dbase->nrows, false);
		if (fk->currentIndex() > 0) {
			resolve_fkey_fields(fk->item);
			if(!fk->item->fkey_form)
				return;
			if (fk->item->fkey_form != fk->fcard->form) {
				fk->fcard->form = fk->item->fkey_form;
				read_dbase(fk->fcard->form);
			}
			const ITEM *fit = fk->item->fkey[fk->fkey].item;
			if(!fit)
				return;
			char *val = qstrdup(fk->currentData().toString());
			int col = fit->column;
			if (IFL(fit->,MULTICOL))
				col = fk->item->fkey[fk->fkey].menu->column;
			/* but actually only restrict if not current widget */
			row_restrict(fk->fcard, col, val, cur, !recurse);
			free(val);
			/* if this value uniquely specifies a base_row,
			 * no need to bother restricting any more */
			if (base_row >= 0)
				return;
		}
		/* then, restrict based on peers */
		if (!recurse)
			for (FKeySelector *ofk = fk->next; ofk != fk; ofk = ofk->next) {
				if (ofk->currentIndex() <= 0)
					continue;
				const CARD *c;
				for (c = ofk->fcard; c; c = c->fkey_next)
					if (c == fk->fcard)
						break;
				if (!c)
					continue;
				base_row = -1;
				restrict(ofk, c, cur);
				if (base_row >= 0)
					return;
			}
		/* then, restrict downwards if needed */
		if (fk->fcard != fc) {
			const ITEM *fit = fk->item;
			const CARD *c = fk->fcard;

			do {
				rowrestrict frestrict = cur;
				if (c->fkey_next == fc)
					cur = ret;
				else {
					tmp.clear();
					cur = tmp;
				}
				cur.resize(c->fkey_next->form->dbase->nrows, false);
				fkey_restrict(c, fit, cur, frestrict);
				if (base_row >= 0)
					return;
			        fit = c->fkey_next->form->items[c->qcurr];
				c = c->fkey_next;
			/* continue all the way down to set base_row if possible */
			} while(c->fkey_next);
		}
	}

	// Fill in menu from table, removing values that can't match due to
	// restrictions
	void set_filter() {
		rowrestrict filter;
		restrict(this, fcard, filter);
		struct str2 { QString data, disp; };
		QList<str2> sl;
		/* const */ DBASE *dbase = fcard->form->dbase;
		resolve_fkey_fields(item);
		if (item->fkey_form != fcard->form) {
			fcard->form = item->fkey_form;
			dbase = read_dbase(fcard->form);
		}
		const ITEM *fit = item->fkey[fkey].item;
		if (!fit)
			return;
		int col = fit->column;
		if (IFL(fit->,MULTICOL))
			col = item->fkey[fkey].menu->column;
		for (int r = 0; r < dbase->nrows; r++) {
			if (!filter[r]) {
				const char *disp = dbase_get(dbase, r, col), *data = disp;
				char *f = summary_display(disp, fcard, r, fit,
							  item->fkey[fkey].menu);
				QString d = QString(STR(data));
				sl.append({d, data == disp ? d : QString(STR(disp))});
				zfree(f);
			}
		}
		if (sl.count() == 0) {
			/* either foreign db empty or invalid key */
			/* in any case, add all possible values */
			for (int r = 0; r < dbase->nrows; r++) {
				const char *disp = dbase_get(dbase, r, col), *data = disp;
				char *f = summary_display(disp, fcard, r, fit,
							  item->fkey[fkey].menu);
				QString d = QString(STR(data));
				sl.append({d, data == disp ? d : QString(STR(disp))});
				zfree(f);
			}
		}
		switch (fit->type) {
		    case IT_TIME:
			std::sort(sl.begin(), sl.end(),
				  [&](const str2 &a, const str2 &b) {
					  if (b.data.isEmpty())
						  return false;
					  if (a.data.isEmpty())
						  return true;
					  char *as = qstrdup(a.data);
					  char *bs = qstrdup(b.data);
					  time_t at = parse_time_data(as, fit->timefmt);
					  time_t bt = parse_time_data(bs, fit->timefmt);
					  zfree(as);
					  zfree(bs);
					  return at < bt;
				  });
			break;
		    case IT_NUMBER:
			std::sort(sl.begin(), sl.end(),
				  [&](const str2 &a, const str2 &b) {
					  return a.data.toDouble() < b.data.toDouble();
				  });
			break;
		    default:
			std::sort(sl.begin(), sl.end(),
				  [&](const str2 &a, const str2 &b) {
					  return a.data.compare(b.data, Qt::CaseInsensitive) < 0;
				  });
		}
		for(auto s = sl.begin(); s != sl.end(); s++)
			while(s + 1 != sl.end() && s[1].data == s->data)
				s = sl.erase(s);
		QString sel = currentData().toString();
		bool blank = currentIndex() <= 0;
		QSignalBlocker sb(this);
		clear();
		addItem("");
		for(auto s = sl.begin(); s != sl.end(); s++)
			addItem(s->disp, s->data);
		if (count() == 2)
			setCurrentIndex(1);
		else if (blank)
			setCurrentIndex(0);
		else
			setCurrentIndex(findData(sel));
		return;
	}
    public:
	int base_row = -1;
    private:
	CARD *fcard;
	FKeySelector *next;
	FKeySelector *share = 0;
	ITEM *item;
	int fkey;
};

static void create_item_widgets(
	CARD		*card,		/* card the item is added to */
	int		nitem)		/* number of item being added */
{
	// ITEM		&item;		/* describes type and geometry */
	struct carditem	*carditem;	/* widget pointers stored here */
	QWidget		*wform;		/* static part form, or card form */
	bool		editable;

	ITEM &item = *card->form->items[nitem];
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
			&& (!card->form->dbase || !card->form->dbase->rdonly)
			&& !card->form->rdonly
			&& !IFL(item.,RDONLY)
			&& nitem != card->rest_item
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
		  if (item.type == IT_TIME && (item.timewidget & 1)) {
			switch(item.timefmt) {
			    case T_DATE:
				dt = new CardDateEdit(card, nitem, wform);
//				dt->setTimeSpec(Qt::UTC);
				break;
			    case T_TIME:
			    case T_DURATION:
				dt = new CardTimeEdit(card, nitem, wform);
				dt->setDisplayFormat(pref.ampm ? "H:mma" :
						                 "HH:mm");
				break;
			    case T_DATETIME:
				dt = new CardDateTimeEdit(card, nitem, wform);
//				dt->setTimeSpec(Qt::UTC);
				break;
			}
			w = dt;
		  	// le = dt->lineEdit();  // protected, of course.  I hate Qt.
		  } else if (item.type == IT_TIME && (item.timewidget & 2) &&
			     (item.timefmt == T_DATE || item.timefmt == T_DATETIME)) {
			  CardInputCalendar *cic;
			  w = cic = new CardInputCalendar(card, nitem, wform,
							  item.timefmt == T_DATETIME);
			  le = cic->lineEdit();
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
		  if (editable)
			set_popup_cb(cb, card_callback(nitem, card, true, i), int, i);
		  else {
			// FIXME: !editable should be more visible
			cb->setEnabled(false);
			cb->setProperty("readOnly", true);
		  }
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
			if(!IFL(item.,MULTICOL)) {
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
		  if (editable)
		  	set_qt_cb(QListWidget, itemSelectionChanged, list, card_callback(nitem, card));
		  else {
			// FIXME: !editable should be more visible
			list->setEnabled(false);
			list->setProperty("readOnly", true);
		  }
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
		unsigned int *colwidth = zalloc(0, "layout", unsigned int, item.nmenu);
		QGridLayout *l = new QGridLayout(carditem->w0);
		l->setSpacing(0); // default; qss may override
		l->setContentsMargins(0, 0, 0, 0);
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
			b->setEnabled(editable);
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
				if(ncol == 1)
					break;
				if(row) {
					tzero(unsigned int, colwidth, ncol);
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
				col--; /* gcc had better wrap this */
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
		  if (editable)
			set_button_cb(b, card_callback(nitem, card, c), bool c);
		  break;
	  }

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

	  case IT_INV_FKEY:
	  case IT_FKEY: {
		int n;
		bool multi = item.type == IT_INV_FKEY || IFL(item.,FKEY_MULTI);
		QLabel *lw = 0;
		if (multi) {
			if (item.ym > 6)
				carditem->w1 = lw = mk_label(wform, item, item.xs, item.ym);
		} else {
			if (item.xm > 6)
				carditem->w1 = lw = mk_label(wform, item, item.xm - 6, item.ys);
		}
		if(lw) {
			QString lab = "<a href=\"" + QString::number(nitem) + "\">";
			// The default label text is AutoText, so respect that
			if (Qt::mightBeRichText(lw->text()))
				lab += lw->text();
			else
				lab += lw->text().toHtmlEscaped();
			lab += "</a>";
			lw->setText(lab);
			set_qt_cb(QLabel, linkActivated, lw, card_callback(nitem, card, true));
		}
		/* allow pure table link similar to "see also" in form.cgi */
		if (item.type == IT_INV_FKEY && item.ys - item.ym <= 4)
			  break;
		carditem->w0 = new QFrame(wform);
		carditem->w0->move(item.x + (multi ? 0 : item.xm),
				   item.y + (multi ? item.ym : 0));
		carditem->w0->resize(item.xs - (multi ? 0 : item.xm),
				     item.ys - (multi ? item.ym : 0));
		carditem->w0->setObjectName("fkeygroup");
		carditem->w0->setProperty(font_prop[item.inputfont], true);
		if(!editable)
			  carditem->w0->setProperty("readOnly", true);

		QGridLayout *l = new QGridLayout(carditem->w0);
		l->setSpacing(0); // default; qss may override
		l->setContentsMargins(0, 0, 0, 0);
		add_layout_qss(l, "fkeygroup");
		int nvis = 0;
		for (n = 0; n < item.nfkey; n++)
			  if(item.fkey[n].display)
				  nvis++;
		resolve_fkey_fields(&item);
		FORM *fform = item.fkey_form;
		if(fform)
			read_dbase(fform);
		CARD *fcard = create_card_menu(fform);
		fcard->fkey_next = card;
		fcard->qcurr = nitem;
		QTableWidget *mw = multi ? new QTableWidget(carditem->w0) : 0;
		if (multi) {
			mw->setObjectName("mwfkeytable");
			mw->setSelectionBehavior(QAbstractItemView::SelectRows);
			l->addWidget(mw, 0, 0);
			if (!IFL(item.,FKEY_HEADER))
				mw->horizontalHeader()->setVisible(false);
			mw->verticalHeader()->setVisible(false);
		}
		if (item.type == IT_INV_FKEY) {
			// I originally added these to any multi, but
			// all FKEY_MULTI functions can be done w/o buttons
			QBoxLayout *bb = new QBoxLayout(QBoxLayout::RightToLeft);
			l->addLayout(bb, 1, 0);
			QPushButton *b;
			// b = new QPushButton("=");
			b = new QPushButton(QIcon::fromTheme("document-open"), "");
			b->setMinimumWidth(1);
			// b->setEnabled(editable);
			set_button_cb(b, card_callback(nitem, card, false, -4));
			if(editable) {
				bb->addWidget(b, 1);
				// b = new QPushButton("-");
				b = new QPushButton(QIcon::fromTheme("list-remove"), "");
				b->setMinimumWidth(1);
				set_button_cb(b, card_callback(nitem, card, false, -3));
				bb->addWidget(b, 1);
				// b = new QPushButton("+");
				b = new QPushButton(QIcon::fromTheme("list-add"), "");
				b->setMinimumWidth(1);
				set_button_cb(b, card_callback(nitem, card, false, -2));
				bb->addWidget(b, 1);
			}
			b = new QPushButton(QIcon::fromTheme("view-refresh"), "");
			b->setMinimumWidth(1);
			set_button_cb(b, card_callback(nitem, card, false, -5));
			bb->addWidget(b, 1);
#if 0
			if (IFL(item.,FKEY_SEARCH)) {
				CardLineEdit *le = new CardLineEdit(card, nitem, carditem->w0);
				set_text_cb(le, card_callback(nitem, card, false, -1));
				bb->addWidget(le);
				bb->setStretchFactor(le, 5);
			}
#endif
		}
		/* add dummies/headers for inv as well */
		add_fkey_row(carditem->w0, card, nitem, !editable, l, mw, 0,
			     item, fcard);
		if (item.type == IT_INV_FKEY) {
			mw->removeRow(0);
			break;
		}
#if 0
		if (IFL(item.,FKEY_SEARCH)) {
			CardLineEdit *le = new CardLineEdit(card, nitem, carditem->w0);
			set_text_cb(le, card_callback(nitem, card, false, -1));
			l->addWidget(le, IFL(item.,FKEY_HEADER) ? 2 : 1, 0,
				     1, l->columnCount());
		}
#endif
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

/* external use only */
void refilter_fkey(FKeySelector *fks)
{
	fks->refilter();
}

int fkey_group_val(FKeySelector *fks)
{
	return fks->base_row;
}

void fkey_group_setval(FKeySelector *fks, int row, const char *key, int keyno)
{
	fks->group_setval(row, key, keyno);
}
/* end external use only */

void FKeySelector::group_setval(int row, const char *key, int keyno)
{
	FKeySelector *w;

	if (row < 0 && BLANK(key)) {
		/* if non-specific selected, leave alone */
		/* otherwise blank out all widgets */
		w = this;
		while(1) {
			if (w->currentIndex() <= 0)
				break;
			w = w->next;
			if(w == this) {
				w = 0;
				break;
			}
		}
		if(!w) {
			/* blank if none of the widgets are blank */
			w = this;
			do {
				QSignalBlocker sb(w);
				w->setCurrentIndex(0);
				w = w->next;
			} while(w != this);
		}
		refilter();
		return;
	}
	DBASE *fdbase = 0;
	w = this;
	char *bkey = 0;
	if(row >= 0) {
		const CARD *c;
		for(c = fcard; c->fkey_next->fkey_next; c = c->fkey_next);
		key = bkey = fkey_of(c->form->dbase, row, item);
	}
	do {
		resolve_fkey_fields(w->item);
		const FKEY *fk = &w->item->fkey[w->fkey];
		ITEM *fitem = fk->item;
		const MENU *menu = fk->menu;
		int col = IFL(fitem->,MULTICOL) ? menu->column :
				fitem->column;
		QSignalBlocker sb(w);
		w->clear();
		w->addItem("");
		row = w->lookup(key, keyno);
		if (row < 0 ) {
			/* invalid key: ignore for now */
			w->setCurrentIndex(0);
		} else {
			fdbase = w->fcard->form->dbase;
			w->addItem(dbase_get(fdbase, row, col), dbase_get(fdbase, row, col));
			w->setCurrentIndex(1);
		}
		w = w->next;
	} while(w != this);
	zfree(bkey);
	refilter();
}

static FKeySelector *add_fkey_field(
	QWidget *p, CARD *card, int nitem, bool ro, /* widget & callback info */
	QString toplab, FKeySelector *prev, /* context */
	QGridLayout *l, QTableWidget *mw, int row, int &col,  /* where to put it */
	ITEM &item, CARD *fcard, int n,    /* fkey info */
	int ncol) /* ncol is set to # of cols if row > 0 (for callback) */
{
	resolve_fkey_fields(&item);
	const FORM *fform = item.fkey_form;
	if(!fform)
		return new FKeySelector(card, nitem, mw ? 0 : p, fcard,
					&item, ro, n);
	ITEM *fit = item.fkey[n].item;
	if(!fit)
		return new FKeySelector(card, nitem, mw ? 0 : p, fcard,
					&item, ro, n);
	const MENU *fm = item.fkey[n].menu;
	if (fit->type == IT_FKEY) {
		resolve_fkey_fields(fit);
		FORM *fitform = fit->fkey_form;
		if(!fitform)
			return new FKeySelector(card, nitem, mw ? 0 : p, fcard,
						fit, ro, n);
		QString lab;
		CARD *nfcard = 0;
		if (!row) {
			if (fit->label) {
				if (toplab.isEmpty())
					lab = fit->label;
				else
					lab = toplab + '/' + fit->label;
			}
			if(fcard) {
				read_dbase(fitform);
				nfcard = create_card_menu(fitform);
				nfcard->fkey_next = fcard;
				nfcard->qcurr = item.fkey[n].index % fform->nitems;
			}
		}
		for (n = 0; n < fit->nfkey; n++)
			if (fit->fkey[n].display)
				prev = add_fkey_field(p, card, nitem, ro,
						      lab, prev,
						      l, mw, row, col,
						      *fit, nfcard, n,
						      ncol);
		return prev;
	}
	if (!row && mw && mw->columnCount() <= col)
		mw->setColumnCount(col + 1);
	if (!row && IFL(item.,FKEY_HEADER)) {
		const char *lab = fm ? (fm->label ? fm->label : "") :
					 fit->label ? fit->label : "";
		if (mw)
			mw->setHorizontalHeaderItem(col, new QTableWidgetItem(lab));
		else
			l->addWidget(new QLabel(lab), 0, col);
	}
	FKeySelector *w = new FKeySelector(card, nitem, mw ? 0 : p, fcard,
					   &item, ro, n);
	if (mw) {
		/* FIXME:  only add row if not read-only */
		if (row >= mw->rowCount())
			mw->setRowCount(row + 1);
		mw->setCellWidget(row, col, w);
		if (!fcard)
			w->fcard_from(static_cast<FKeySelector *>(mw->cellWidget(row ? row - 1 : 1, col)));
	} else
		l->addWidget(w, IFL(item.,FKEY_HEADER) ? 1 : 0, col);
	set_popup_cb(w, card_callback(nitem, card, false, col), int,);
	if (prev)
		w->AddGroup(prev);
	++col;
	return w;
}

FKeySelector *add_fkey_row(
	QWidget *p, CARD *card, int nitem, bool ro, /* widget & callback info */
	QGridLayout *l, QTableWidget *mw, int row,  /* where to put it */
	ITEM &item, CARD *fcard)	/* fkey info */
{
	int col = 0;
	FKeySelector *fks = 0;
	for (int k = 0; k < item.nfkey; k++)
		if (item.fkey[k].display)
			fks = add_fkey_field(p, card, nitem, ro, "", fks,
					     l, mw, row, col,
					     item, fcard, k, 0);
	return fks;
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

	if (nitem >= card->nitems   ||			/* illegal */
	    card->form->dbase == 0  ||			/* preview dummy card*/
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
		if (IFL(item->,MULTICOL)) {
			if (!store(card, nitem,
				   flag ? item->menu[index].flagcode : NULL,
				   index))
				return;
			break;
		}
		char *old = dbase_get(card->form->dbase, card->row, item->column);
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
			if(!old[(oafter = obegin)])
				tmp[obegin++] = sep;
			memcpy(tmp + obegin, val, vlen);
			if(old[oafter])
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
		if(IFL(item->,MULTICOL)) {
			for(int m = 0; m < item->nmenu; m++) {
				char *val = NULL;
				if(lw->item(m)->isSelected())
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
			/* passing in &mainwindow->card allows switch to work */
			n = evaluate(card, item->pressed, &mainwindow->card);
			card = mainwindow->card;
			if (!BLANK(n)) {
				int UNUSED ret = system(n);
			}
		}
		break;
	  case IT_FKEY:
	  case IT_INV_FKEY: {
		if (flag) { /* label click */
			card_readback_texts(card, -1);
			int sel = card->row;
			QString fpath = card->form->path;
			DBASE *dbase = card->form->dbase;
			bool blank = item->type == IT_FKEY &&
				BLANK(dbase_get(dbase, sel, item->column));
			/* FIXME: ensure that this card/form/dbase stays valid */
			resolve_fkey_fields(item);
			if(item->fkey_form) {
				int fit = -1;
				char *fk = 0;
				if(item->type == IT_INV_FKEY)
					for(int n = 0; n < item->nfkey; n++)
						if(item->fkey[n].key) {
							fit = item->fkey[n].index;
							resolve_fkey_fields(item->fkey[n].item);
							fk = fkey_of(dbase, sel,
								     item->fkey[n].item,
								     true);
							break;
						}
				switch_form(mainwindow->card, item->fkey_form->path,
					    fit, fk);
			}
			if (item->type == IT_INV_FKEY || sel < 0 || blank)
				return;
			/*  {r="";foreach(k,+@"origdb":_ref[row],"_key1==k[0]&&..","(r=1)");r} */
			/*  for single, just {k=@"origdb":_ref[row];_key1==k[0]&&..} */
			/* multi clobbers r and k; single only k */
			QString q;
			if (IFL(item->,FKEY_MULTI))
				q = "{r=\"\";foreach(k,+";
			else
				q = "{k=";
			q += "@\"";
			// maybe someday I'll strip leading path if unnecessary
			q += fpath.replace(QRegularExpression("([\\\\\"])"),
						"\\\\\\1");
			// maybe someday I'll replace numeric field with name
			q += "\":_"+ QString::number(item->column) +
				"[" + QString::number(sel) +
				"]";
			if (IFL(item->,FKEY_MULTI))
				q += ",\"";
			else
				q += ';';
			int keylen = keylen_of(item);
			int keys[keylen];
			copy_fkey(item, keys);
			for (int i = 0; i < keylen; i++) {
				const FKEY &fk = item->fkey[keys[i]];
				ITEM *fit = fk.item;
				// maybe someday I'll replace numeric field with name
				int col = IFL(fit->,MULTICOL) ?
					fk.menu->column :
					fit->column;
				if (i > 0)
					q += "&&";
				q += '_' + QString::number(col) + "==k";
				if (keylen > 1)
					q += '[' + QString::number(i) + ']';
			}
			if (IFL(item->,FKEY_MULTI))
				q += "\",\"(r=1)\");r}";
			else
				q += '}';
			char *str = qstrdup(q);
			if (IFL(item->,FKEY_MULTI))
				query_eval(SM_SEARCH, mainwindow->card, str);
			else
				find_and_select(str);
			if (pref.query2search)
				append_search_string(str);
			else
				free(str);
			return;
		}
		bool multi = IFL(item->,FKEY_MULTI) || item->type == IT_INV_FKEY;
		FKeySelector *w = 0;
		QListIterator<QObject *>iter(card->items[nitem].w0->children());
		QTableWidget *tw = 0;
		int elt = 0;
		if (multi) {
			while(iter.hasNext()) {
				tw = dynamic_cast<QTableWidget *>(iter.next());
				if(tw) {
					elt = tw->currentRow();
					w = static_cast<FKeySelector *>(
				   		 tw->cellWidget(elt, index));
					break;
				}
			}
		} else {
			while(iter.hasNext()) {
				w = dynamic_cast<FKeySelector *>(iter.next());
				if(!w)
					continue;
				if(index <= 0)
					break;
				index--;
				w = 0;
			}
		}
		switch(index) {
#if 0
		    case -1: /* search */
			// FIXME: filter fkey db
			break;
#endif
		    case -2: /* + Add */ {
			const ITEM *fitem = 0;
			int fitno = 0;
			for(int n = 0; n < item->nfkey; n++)
				    if(item->fkey[n].key) {
					    fitem = item->fkey[n].item;
					    fitno = item->fkey[n].index;
					    break;
				    }
			if(!fitem)
				return;  // I guess I should pop an error up
			char *key = fkey_of(card->form->dbase, card->row, fitem, true);
			if(BLANK(key)) {
				zfree(key);
				create_error_popup(mainwindow, errno,
						   "This card cannot be referenced because it has a blank key.");
				return;
			}
			/* partly copied from mainwin:add_card() */
			DBASE *dbase = read_dbase(item->fkey_form);
			/* presumably dbase->currsect is correct */
			if (!dbase_addrow(&elt, dbase)) {
				/* this is stupid:  if there's no room for a row,
				 * there's probably also no room for a new popup window */
				create_error_popup(mainwindow, errno,
						   "No memory for new database row");
				free(key);
				return;
			}
			dbase_put(dbase, elt, fitem->column, key);
			free(key);
			(new ItemEd(card->shell, item->fkey_form, elt, true,
				    fitno))->exec();
			fillout_item(card, nitem, false);
		  	return;
		    }
		    case -3: /* - Remove */ {
			if(elt < 0)
				    return;
			/* partly copied from popup.c:create_save_popup */
			QString msg("Removing:\n");
			CARD fcard = {};
			fcard.form = item->fkey_form;
			read_dbase(item->fkey_form);
			const ITEM *fitem = 0;
			for(int n = 0; n < item->nfkey; n++)
				    if(item->fkey[n].key) {
					    fitem = item->fkey[n].item;
					    break;
				    }
			char *sbuf = 0;
			size_t sbuf_len = 0;
			elt = tw->item(elt, 0)->data(Qt::UserRole).toInt();
			make_summary_line(&sbuf, &sbuf_len, &fcard, elt);
			msg += sbuf;
			msg += "\n\nDelete entire card?\n"
			       "If not, just clear reference.\n";
			bool cont = true;
			while(cont) {
				cont = false;
				switch(QMessageBox::question(card->shell,
							     "Delete Reference",
							     msg,
							     QMessageBox::Yes |
							     QMessageBox::No |
							     QMessageBox::Cancel |
							     QMessageBox::Help,
							     QMessageBox::No)) {
				    case QMessageBox::Yes:
					dbase_delrow(elt, fcard.form->dbase);
					fillout_item(card, nitem, false);
					break;
				    case QMessageBox::No:
					if(!IFL(fitem->,FKEY_MULTI))
						dbase_put(fcard.form->dbase, elt, fitem->column, 0);
					else {
						char *key = fkey_of(card->form->dbase,
								    card->row,
								    fitem);
						char *oval = dbase_get(fcard.form->dbase, elt, fitem->column);
						int begin, after;
						char esc, sep;
						get_form_arraysep(item->fkey_form, &esc, &sep);
						if(find_unesc_elt(oval, key,
								  &begin, &after,
								  esc, sep)) {
							if(!begin && !oval[after])
								dbase_put(fcard.form->dbase, elt, fitem->column, 0);
							else {
								if (begin)
									--begin;
								/* this modifies value in dbase */
								memmove(oval + begin, oval + after, strlen(oval + after) + 1);
							}
						}
					}
					fillout_item(card, nitem, false);
					break;
				    case QMessageBox::Help:
					help_callback(card->shell, "delinvref");
					cont = true;
					break;
				    default:
					break;
				}
			}
			return;
		    }
		    case -4: { /* = Edit */
			if(elt < 0)
				    return;
			int fitno = 0;
			for(int n = 0; n < item->nfkey; n++)
				    if(item->fkey[n].key) {
					    fitno = item->fkey[n].index;
					    break;
				    }
			read_dbase(item->fkey_form);
			(new ItemEd(card->shell, item->fkey_form,
				    tw->item(elt, 0)->data(Qt::UserRole).toInt(),
				    false, fitno))->exec();
			fillout_item(card, nitem, false);
		  	return;
		    }
		    case -5: /* refresh */
			fillout_item(card, nitem, false);
			return;
		}
		bool blank = w->currentIndex() <= 0;
		int odbrow = w->base_row;
		if (!blank || !multi)
			  w->refilter();
		if (blank && !multi && w->base_row)
			/* need to clear out all widgets to ensure blank remains */
			w->clear_group();
		int dbrow = blank ? -1 : w->base_row;
		if(odbrow == dbrow)
			break;
		resolve_fkey_fields(item);
		if (!item->fkey_form)
			break;
		char *v = dbrow < 0 ? 0 : fkey_of(read_dbase(item->fkey_form),
						  dbrow, item);
		if (multi) {
			QSignalBlocker sb(tw);
			char *ov = odbrow < 0 ? 0 :
				          fkey_of(read_dbase(item->fkey_form),
						  odbrow, item);
			char *a = dbase_get(card->form->dbase, card->row, item->column);
			char toesc[3];
			char &sep = toesc[1], &esc = toesc[0];
			int beg, aft;
			get_form_arraysep(card->form, &sep, &esc);
			bool force = false;
			/* First, remove old value */
			if (ov) {
				if (a && find_unesc_elt(a, ov, &beg, &aft, sep, esc)) {
					/* mangling a modifies dbase itself */
					/* thus store only sees change if a->0 */
					if (!beg && !a[aft])
						a = 0;
					else if (!a[aft]) {
						a[beg - 1] = 0;
						force = true;
					} else {
						memmove(a + beg, a + aft + 1, strlen(a + aft/* + 1*/)/* + 1*/);
						force = true;
					}
				}
				free(ov);
			}
			/* then, insert new value if non-blank */
			if (!v) {
				/* since dbrow != odbrow, odbrow was non-blank */
				/* so just delete the table row */
				tw->removeRow(elt);
				if (!store(card, nitem, a, 0, force))
					return;
			} else {
				if(!a) {
					/* easy way to just escape v */
					set_elt(&a, 0, v, card->form);
					free(v);
					v = a; /* for freeing later */
				} else {
					if (find_unesc_elt(a, v, &beg, &aft, sep, esc)) {
						/* already there: ignore */
						tw->removeRow(elt);
						a = 0; /* flag for below */
					} else {
						/* insert at returned beg */
						toesc[2] = 0;
						int nesc = countchars(v, toesc);
						int vlen = strlen(v) + nesc;
						int alen = strlen(a);
						char *na = alloc(0, "adding fkey", char, vlen + alen + 2);
						memcpy(na, a, beg);
						if(!a[(aft = beg)])
							na[beg++] = sep;
						escape(na + beg, v, vlen - nesc, esc, toesc);
						if(a[aft])
							na[vlen + beg++] = sep;
						memcpy(na + beg + vlen, a + beg - 1, alen - beg + 2);
						a = na;
					}
				}
				if (!ov) {
					/* adding entry inserts new blank row */
					FKeySelector *fks = 0;
					elt = tw->rowCount();
					fks = add_fkey_row(card->items[nitem].w0,
							   card, nitem, false,
							   0, tw, elt, *item, 0);
					fks->refilter();
				}
				if (a && !store(card, nitem, a)) {
					free(v);
					return;
				}
			}
		} else
			  if (!store(card, nitem, v)) {
				  free(v);
				  return;
			  }
		zfree(v);
		break;
	  }
	  default: ;
	}
	if (redraw)
		fillout_card(card, true);
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
	const char	*data;		/* data string to store */
	time_t		time;		/* parsed time */
	char		buf[40];	/* if numeric time, temp string */
	ITEM		*item;

	if (!card || !card->form || !card->form->items || !card->form->dbase
		  ||  card->form->rdonly
		  ||  card->form->dbase->rdonly
		  ||  card->row >= card->form->dbase->nrows)
		return;
	start = which >= 0 ? which : 0;
	end   = which >= 0 ? which : card->nitems-1;
	for (nitem=start; nitem <= end; nitem++) {
		item = card->form->items[nitem];
		if (!card->items[nitem].w0		||
		    IFL(card->form->items[nitem]->,RDONLY))
			continue;
		switch(item->type) {
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
			if (item->timewidget & 1) {
				QDateTime dt = reinterpret_cast<QDateTimeEdit *>
					(card->items[nitem].w0)->dateTime();
//				if(item->timefmt == T_DATE)
//					dt.setTime(QTime(0, 0));
				time = dt.toSecsSinceEpoch();
				if(time < 0)
					time = 0;
				data = ""; /* force use of time */
			} else {
				data = read_text_button(card->items[nitem].w0, 0);
				while (*data == ' ' || *data == '\t' || *data == '\n')
					data++;
				if (!*data)
					data = 0;
				else
					time = parse_time_data(data,
							       item->timefmt);
			}
			/* this used to store time as a number */
			/* but then converted to timefmt on write_dbase */
			/* It's safer to keep in-memory form same as on-disk */
			(void)store(card, nitem, data ?
				    format_time_data(time, item->timefmt) : 0);
			if (which >= 0)
				fillout_item(card, nitem, false);
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

static bool store(
	CARD		*card,		/* card the item is added to */
	int		nitem,		/* number of item being added */
	const char	*string,	/* string to store in dbase */
	int		menu,		/* menu item to store, if applicable */
	bool		force)		/* dbase changed even if string same */
{
	bool		newsum = false;	/* must redraw summary? */
	const ITEM	*item;
	int		col;

	if (nitem >= card->nitems		||
	    card->form->dbase == 0		||
	    card->form->dbase->rdonly		||
	    IFL(card->form->items[nitem]->,RDONLY) || card->form->rdonly)
		return(false);

	item = card->form->items[nitem];
	col = IFL(item->,MULTICOL) ? item->menu[menu].column : item->column;
	if (!dbase_put(card->form->dbase, card->row, col, string, force))
		return(true);

	print_info_line();
	if (pref.sortcol == col) {
		dbase_sort(card, pref.sortcol, pref.revsort);
		newsum = true;
	} else
		replace_summary_line(card, card->row);

	if (pref.autoquery) {
		do_query(card->form->autoquery);
		newsum = true;
	}
	if (newsum && card->form == mainwindow->card->form) {
		create_summary_menu(card);
		scroll_summary(card);
	}
	return(true);
}


/*-------------------------------------------------- drawing ----------------*/
/*
 * fill out a card, using the data from the row in the database referenced
 * in the card. If there is no database, print empty strings. If the deps
 * argument is true, a field has changed (and got reprinted separately), so
 * only PRINT and CHART items that may need updating are reprinted. This is
 * done to speed things up. It is called by routines that call store().
 */

void fillout_card(
	CARD		*card,		/* card to draw into menu */
	bool		deps)		/* if true, dependencies only */
{
	int		i;		/* item counter */

	if (card) {
		card->disprow = card->row;
		for (i=0; i < card->nitems; i++)
			fillout_item(card, i, deps);
	}
	// only rebuild if doing a full rebuild
	if (!deps)
		remake_section_popup(false);
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

static void ifkey_display(QTableWidget *tw, int tr, int &tc, ITEM *item,
			  CARD *card, int r, int mr)
{
	for (int f = 0; f < item->nfkey; f++) {
		if (!item->fkey[f].display)
			continue;
		ITEM *dit = item->fkey[f].item;
		const char *data = dit ? NULL : "???";
		if (dit && r >= 0) {
			int col;
			if (IFL(dit->,MULTICOL))
				col = item->fkey[f].menu->column;
			else
				col = dit->column;
			data = dbase_get(card->form->dbase, r, col);
		}
		if (dit && dit->type == IT_FKEY) {
			resolve_fkey_fields(dit);
			if (dit->fkey_form) {
				DBASE *db = read_dbase(dit->fkey_form);
				/* FIXME:  FKEY_MULTI? */
				int nr = fkey_lookup(db, dit, data);
				int otc = tc;
				CARD *fcard = create_card_menu(dit->fkey_form);
				ifkey_display(tw, tr, tc, dit, fcard, nr, mr);
				free_card(fcard);
				if(data && nr < 0) {
					QTableWidgetItem *twi = new QTableWidgetItem(data);
					twi->setData(Qt::UserRole, mr);
					/* FIXME:  colspan=(tc-otc+1) */
					/* QTableWidget doesn't seem to support colspan */
					tw->setItem(tr, otc, twi);
				}
				continue;
			}
			/* just show raw fkey if form lookup fails */
			/* FIXME:  colspan=(tc-otc+1) */
		}
		char *fr = summary_display(data, card, r, dit ? dit : item,
					   item->fkey[f].menu);
		QTableWidgetItem *twi = new QTableWidgetItem(data);
		twi->setData(Qt::UserRole, mr);
		tw->setItem(tr, tc++, twi);
		zfree(fr);
	}
}

void fillout_item(
	CARD		*card,		/* card to draw into menu */
	int		i,		/* item index */
	bool		deps)		/* if true, dependencies only */
{
	bool		sens, vis;	/* (de-)sensitize item */
	ITEM		*item;		/* describes type and geometry */
	QWidget		*w0, *w1;	/* input widget(s) in card */
	const char	*data = NULL;	/* value string in database */

	w0   = card->items[i].w0;
	w1   = card->items[i].w1;
	if (w0) w0->blockSignals(true);
	item = card->form->items[i];
	// FIXME:  Should above-div items always be excluded, like sens?
	vis = !item->invisible_if ||
	      (card->form->dbase && card->row >= 0
			   && card->row < card->form->dbase->nrows
			   && !evalbool(card, item->invisible_if));
	if (w0) w0->setVisible(vis);
	if (w1) w1->setVisible(vis);

	// FIXME:  Should above-div items always be excluded?
	// FIXME:  Should IT_LABELs be disabled when no data is loaded?
	sens = (item->type == IT_BUTTON && !item->gray_if) ||
	       item->y < card->form->ydiv ||
	       (card->form->dbase && card->row >= 0
			    && card->row < card->form->dbase->nrows
			    && !evalbool(card, item->gray_if));
	if(!IFL(item->,MULTICOL))
		data = dbase_get(card->form->dbase, card->row, item->column);

	// FIXME: !editable should be more visible
	bool roprop = w0 && w0->property("readOnly").toBool();

	if (w0 && !roprop) w0->setEnabled(sens);
	if (w1) w1->setEnabled(sens);

	switch(item->type) {
	  case IT_TIME:
		/* this used to parse time as a number */
		/* but that was converted from timefmt on read_dbase */
		/* It's safer to keep in-memory form same as on-disk */
		if (item->timewidget & 1) {
			if (!deps) {
				QDateTime dt = QDateTime();
				time_t t = parse_time_data(data, item->timefmt);
				/* if(t > 0) */ /* can't set to invalid date */
					dt.setSecsSinceEpoch(t);
				reinterpret_cast<QDateTimeEdit *>(w0)->
					setDateTime(dt);
#if 0 // this doesn't do anything useful
				/* signal isn't actually sent, so update fmt */
				if(item->timefmt == T_DATE)
					reinterpret_cast<CardDateEdit *>(w0)->
					         setFormat();
				else if (item->timefmt == T_DATETIME)
					reinterpret_cast<CardDateTimeEdit *>(w0)->
					         setFormat();
#endif
			}
			break;
		}
		data = BLANK(data) ? data :
			format_time_data(parse_time_data(data, item->timefmt),
					 item->timefmt);
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
				int nmax = card->form->dbase && item->dcombo == C_ALL ? card->form->dbase->nrows : card->nquery;
				for(int n = 0; n < nmax; n++) {
					char *other;
					int idx = item->dcombo == C_ALL ? n : card->query[n];
					if(card->row == idx)
						continue;
					other = dbase_get(card->form->dbase, idx, card->form->items[i]->column);
					if(!BLANK(other))
						set.insert(other);
				}
				// Remove duplicates of statics
				QStringListIterator iter(cb->c_static);
				while(iter.hasNext())
					set.remove(iter.next());
				QStringList &l = cb->c_dynamic;
				l = set.values();
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

	  case IT_MENU: {
		int n;
		for(n = 0; n < item->nmenu; n++) {
			// if(!item->menu[n].flagcode) continue;
			if(!strcmp(STR(item->menu[n].flagcode), STR(data))) {
				reinterpret_cast<QComboBox *>(w0)->setCurrentIndex(n);
				break;
			}
		}
		if(n == item->nmenu)
			// unknown value - this will act strange
			// best to set it to blank, but there is no blank
			reinterpret_cast<QComboBox *>(w0)->setCurrentIndex(-1);
		break;
	  }
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
			else if(IFL(item->,MULTICOL)) {
				data = dbase_get(card->form->dbase, card->row,
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
		if(IFL(item->,MULTICOL)) {
			for(int n = 0; n < item->nmenu; n++) {
				data = dbase_get(card->form->dbase, card->row,
						 item->menu[n].column);
				l->item(n)->setSelected(!strcmp(STR(data),
								item->menu[n].flagcode));
			}
		} else {
			char desc[3], &sep = desc[1], &esc = desc[0];
			get_form_arraysep(card->form, &sep, &esc);
			desc[2] = 0;
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

	  case IT_FKEY: {
		bool multi = IFL(item->,FKEY_MULTI);
		QListIterator<QObject *>iter(w0->children());
		if (multi) {
			QTableWidget *tw = 0;
			while(iter.hasNext()) {
				tw = dynamic_cast<QTableWidget *>(iter.next());
				if(tw)
					break;
			}
			if (!tw)
				break; // invalid form??
			if (BLANK(data)) {
				while (tw->rowCount() > 1)
					tw->removeRow(0);
				if (IFL(item->,RDONLY) && tw->rowCount() == 1)
					tw->removeRow(0);
				else
					static_cast<FKeySelector *>(tw->cellWidget(tw->rowCount() - 1, 0))->group_setval(-1, 0, 0);
				break;
			}
			resolve_fkey_fields(item);
			if(!item->fkey_form)
				break;
			FKeySelector *fk = static_cast<FKeySelector *>(tw->cellWidget(0, 0));
			/* This ensures entries are in fkey set order */
			/* FIXME:  support other sort orders */
			int n;
			for(n = 0;;n++) {
				int row = fk->lookup(data, n, true);
				if(row < 0) /* -2 needs Check References */
					break;
				int r;
				for(r = n; r < tw->rowCount(); r++)
					if(static_cast<FKeySelector *>(tw->cellWidget(r, 0))->base_row == row) {
						while(r-- > n)
							tw->removeRow(n);
						break;
					}
				if(r < tw->rowCount())
					continue;
				tw->insertRow(n);
				FKeySelector *fks = add_fkey_row(w0, card, i, roprop,
								 0, tw, n,
								 *item, 0);
				tw->resizeRowsToContents();
				fks->group_setval(row, 0, -1);
			}
			while(tw->rowCount() > n + 1)
				tw->removeRow(n);
			if(roprop) {
				if(tw->rowCount() == n + 1)
					tw->removeRow(n);
			} else {
				if(tw->rowCount() == n) {
					FKeySelector *fks = add_fkey_row(w0, card, i, false,
									 0, tw, n,
									 *item, 0);
					fks->group_setval(-1, 0, -1);
				} else {
					FKeySelector *bl = static_cast<FKeySelector *>(tw->cellWidget(n, 0));
					if(bl->count() == 0)
						bl->refilter();
				}
			}
		} else {
			FKeySelector *w = 0;
			while(iter.hasNext()) {
				w = dynamic_cast<FKeySelector *>(iter.next());
				if(w)
					break;
			}
			if (!w)
				break; // invalid form??
			w->group_setval(-1, data, 0);
		}
		break;
	  }

	  case IT_INV_FKEY: {
		if (!w0) /* label-only: nothing to do */
			  return;
		QListIterator<QObject *>iter(w0->children());
		QTableWidget *tw = 0;
		while(iter.hasNext()) {
			tw = dynamic_cast<QTableWidget *>(iter.next());
			if(tw)
				break;
		}
		if (!tw)
			break; // invalid form??
		/* FIXME:  just setRowCount(0) if read-only */
		/* FIXME:  make fully editable if all non-key fields visible */
	        /* only IN_DBASE:
		 *  IT_INPUT   short LineEdit, maybe with combo box
		 *  IT_TIME    short date/time widget
		 *  IT_NOTE    small text input box
		 *  IT_CHOICE  IT_MENU
		 *  IT_FLAG    raw
		 *  IT_NUMBER  short number input
		 *  IT_MENU    combobox pulldown
		 *  IT_RADIO   IT_MENU
		 *  IT_MULTI   ??  popup multiselect?
		 *  IT_MULTI mc same IT_FLAG
		 *  IT_FLAGS   IT_MULTI
		 *  IT_FKEY    line of sel widgets
		 */
		/* or, just screw it and pop up a cardwin when needed */
		while(tw->rowCount() > 0)
			  tw->removeRow(0);
		resolve_fkey_fields(item);
		ITEM *fit = 0;
		for (int n = 0; n < item->nfkey; n++)
			if (item->fkey[n].key) {
				fit = item->fkey[n].item;
				break;
			}
		if(!fit)
			  break;
		resolve_fkey_fields(fit);
		if(!fit->fkey_form)
			  break;
		char *key = fkey_of(card->form->dbase, card->row, fit);
		if (!key)
			  break;
		char sep, esc;
		get_form_arraysep(item->fkey_form, &sep, &esc);
		DBASE *db = read_dbase(item->fkey_form);
		CARD *fcard = create_card_menu(item->fkey_form);
		bool multi = !!IFL(fit->,FKEY_MULTI);
		for (int r = 0, tr = 0; r < db->nrows; r++) {
			bool match = false;
			const char *fk = dbase_get(db, r, fit->column);
			if (multi) {
				int alen;
				char **ar = split_array(item->fkey_form, fk, &alen);
				for (int n = 0; n < alen; n++)
					if ((match = !strcmp(ar[n], key)))
						break;
				for (int n = 0; n < alen; n++)
					free(ar[n]);
				free(ar);
			} else if(fk)
				match = !strcmp(key, fk);
			if (match) {
				tw->insertRow(tr);
				int tc = 0;
				ifkey_display(tw, tr, tc, item, fcard, r, r);
				tr++;
			}
		}
		free_card(fcard);
		tw->resizeColumnsToContents();
		free(key);
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
