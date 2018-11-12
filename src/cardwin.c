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
#include <assert.h>
#include <QtWidgets>
#include <time.h>
#include "grok.h"
#include "form.h"
#include "chart-widget.h"
#include "proto.h"

static void create_item_widgets(CARD *, int);
static void card_callback(int, CARD *, bool f = false);
void card_readback_texts(CARD *, int);
static BOOL store(CARD *, int, const char *);

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
	register CARD	*card)		/* card to destroy */
{
	int		i;		/* item counter */

	if (!card)
		return;
	card_readback_texts((CARD *)card, -1);
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
		return((CARD *)0);
	memset((void *)card, 0, n);
	card->form   = form;
	card->dbase  = dbase;
	card->row    = -1;
	card->nitems = form->nitems;
	if (!mainwindow)
		return((CARD *)card);
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

// Qt in its glorious wisdom provides no signals related to focus, so
// I have to override the low-level event handler(s)

#define OVERRIDE_INIT(c,i) {\
	ITEM *ip; \
	if (c && (ip = c->form->items[i]) && \
	    ip->skip_if && *ip->skip_if) { \
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
		  QLineEdit *le = new CardLineEdit(card, nitem, wform);
		  carditem->w0 = le;
		  le->setObjectName("input");
		  le->move(item.x + item.xm, item.y);
		  le->resize(item.xs - item.xm, item.ys);
		  le->setAlignment(JUST(item.inputjust));
		  le->setProperty(font_prop[item.inputfont], true);
		  le->setMaxLength(item.maxlen?item.maxlen:10);
		  le->setReadOnly(!editable);
		  // le->setTextMargins(l, r, 2, 2);
		  if (editable) {
			// Make this update way too often, just like IT_NOTE.
			set_qt_cb(QLineEdit, textChanged, le,
				  card_callback(nitem, card, false));
			// But only update dependent widgets when done
			set_text_cb(le, card_callback(nitem, card, true));
		  }
		}
		break;

	  case IT_NOTE:				/* multi-line text */
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
		   else {
			// This updates way too often:  every character.
			set_qt_cb(QTextEdit, textChanged, te, card_callback(nitem, card, false));
			// But there is no equivalent to editingFinished here
			// Instead, focusOutEvent is overridden above.
		   }
		}
		break;

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
		  b->setProperty(font_prop[item.labelfont], true);
		  // Qt doesn't allow label alignment.  Probably for the best.
		  // b->setAlignment(JUST(item.labeljust));
		  // seriously, "label"?
		  b->setObjectName("label");
		  if (card->dbase && !card->dbase->rdonly
				&& !card->form->rdonly
				&& !item.rdonly)
			set_button_cb(b, card_callback(nitem, card, c), bool c);
		  else
			b->setEnabled(false); // tjm - added this for sanity
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
	bool			flag)
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

	  case IT_FLAG:					/* square on/off */
		if (!(n = item->flagcode))
			return;
		// Old code toggled database value directly.  Now it
		// respects the state of the GUI.
		if (!store(card, nitem, flag ? n : 0))
			return;
		break;

	  case IT_BUTTON:				/* pressable button */
		if ((redraw = item->pressed)) {
			if (switch_name) free(switch_name); switch_name = 0;
			if (switch_expr) free(switch_expr); switch_expr = 0;
			n = evaluate(card, item->pressed);
			if (switch_name) {
				switch_form(switch_name);
				card = curr_card;
			}
			if (switch_expr)
				search_cards(SM_SEARCH, card, switch_expr);
			if (n && *n)
				(void)system(n);
			if (switch_name) free(switch_name); switch_name = 0;
			if (switch_expr) free(switch_expr); switch_expr = 0;
		}
		break;
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
				  case T_DURATION:
					time = parse_timestring(data, TRUE);
					break;
				  case T_DATETIME:
					time = parse_datetimestring(data);
				}
				sprintf(buf, "%ld", (long)time);
			}
			(void)store(card, nitem, buf);
			if (which >= 0)
				fillout_item(card, nitem, FALSE);
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
	register CARD	*card,		/* card the item is added to */
	int		nitem,		/* number of item being added */
	const char	*string)	/* string to store in dbase */
{
	BOOL		newsum = FALSE;	/* must redraw summary? */

	if (nitem >= card->nitems		||
	    card->dbase == 0			||
	    card->dbase->rdonly			||
	    card->form->items[nitem]->rdonly || card->form->rdonly)
		return(FALSE);

	if (!dbase_put(card->dbase, card->row,
				card->form->items[nitem]->column, string))
		return(TRUE);

	print_info_line();
	if (pref.sortcol == card->form->items[nitem]->column) {
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


void fillout_item(
	CARD		*card,		/* card to draw into menu */
	int		i,		/* item index */
	BOOL		deps)		/* if TRUE, dependencies only */
{
	BOOL		sens, vis;	/* (de-)sensitize item */
	register ITEM	*item;		/* describes type and geometry */
	QWidget		*w0, *w1;	/* input widget(s) in card */
	const char	*data;		/* value string in database */
	const char	*eval;		/* evaluated expression, 0=error */

	w0   = card->items[i].w0;
	w1   = card->items[i].w1;
	item = card->form->items[i];
	// FIXME:  Should above-div items always be excluded, like sens?
	vis = !item->invisible_if ||
	       card->dbase && card->row >= 0
			   && card->row < card->dbase->nrows
			   && !evalbool(card, item->invisible_if);
	if (w0) w0->setVisible(vis);
	if (w1) w1->setVisible(vis);

	// FIXME:  Should above-div items always be excluded?
	// FIXME:  Should IT_LABELs be disabled when no data is loaded?
	sens = item->type == IT_BUTTON && !item->gray_if ||
	       item->y < card->form->ydiv ||
	       card->dbase && card->row >= 0
			   && card->row < card->dbase->nrows
			   && !evalbool(card, item->gray_if);
	// FIXME:  not pulling in data here causes databsae to be modified
	//         to the blank data.  Besides, I actually want it to
	//         show the data on disabled (but not invisible) fields
	data = /* !vis || !sens || item->type == IT_BUTTON ? 0 : */
	       dbase_get(card->dbase, card->row, card->form->items[i]->column);

	
	if (w0) w0->setEnabled(sens);
	if (w1) w1->setEnabled(sens);

	switch(item->type) {
	  case IT_TIME:
		// if (sens)
			data = format_time_data(data, item->timefmt);

	  case IT_INPUT:
		if (!deps) {
			print_text_button_s(w0, /* !sens ? " " : */ data ? data :
					item->idefault &&
					(eval = evaluate(card, item->idefault))
							? eval : "");
#if 0 // this gets called every time a character changes, so don't mess with the cursor
			if (w0)
				dynamic_cast<QLineEdit *>(w0)->setCursorPosition(0);
#endif
		}
		break;

	  case IT_NOTE:
		if (!deps && w0) {
			print_text_button_s(w0, /* !sens ? " " : */ data ? data :
					item->idefault &&
					(eval = evaluate(card, item->idefault))
							? eval : "");
			dynamic_cast<QTextEdit *>(w0)->textCursor().setPosition(0);
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

	  case IT_CHART:
		draw_chart(card, i);
		break;
	}
}
