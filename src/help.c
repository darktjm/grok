/*
 * print help popup on a particular topic. Help topics are identified by
 * a short string that is looked up in the help file. The help file contains
 * a list of button help entries; each consisting of a line "%% topic"
 * followed by help text lines. Leading blanks in text lines are ignored.
 * Lines beginning with '#' are ignored.
 *
 * To add a help text to a widget, call
 *	bind_help(widget, "topic")
 * and add "%%n topic" and the help text to the help file (n is a number
 * used for extracting a user's manual, it is not used by the grok program).
 *
 * help_callback(parent, topic)		Print help for a topic, usually for
 *					button <parent>.
 */

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"
#include "HtmlCssUtils.hpp"

static void done_callback    (void);
static void context_callback (void);
static char *get_text(const char *), *add_card_help(const char *, char *);

static bool		have_shell = false;	/* message popup exists if true */
static QDialog		*shell;			/* popup menu shell */


/*
 * destroy the help popup. Remove it from the screen, and destroy its widgets.
 */

void destroy_help_popup(void)
{
	if (have_shell) {
		have_shell = false;
		shell->close();
		delete shell;
	}
}



/*
 * look up the help text for <topic> and create a window containing the text.
 */

/*ARGSUSED*/
void help_callback(
UNUSED	QWidget			*parent,
	const char		*topic)
{
	static QTextEdit	*text_w;
	QVBoxLayout		*form;
	QWidget			*w;
	char			*message;
	int			nlines = 1;
	char			*p;

	if (!(message = get_text(topic)) ||
	    !(message = add_card_help(topic, message)))
		return;
	if (have_shell) {
		text_w->setText(message);
		popup_nonmodal(shell);
		free(message);
		return;
	}

	for (nlines=0, p=message; *p; p++)
		nlines += *p == '\n';
	if (nlines > 30)
		nlines = 30;

	//  Not going to bother disabling the close button here
	shell = new QDialog();
	shell->setWindowTitle("Help");
	set_icon(shell, 1);
	form = new QVBoxLayout(shell);
	add_layout_qss(form, "helpform");
	bind_help(shell, "help");

							/*-- buttons --*/
	QDialogButtonBox *bb = new QDialogButtonBox;
	w = mk_button(bb, "Dismiss", dbbr(Reject));
	set_button_cb(w, done_callback());
	bind_help(w, "help_done");

	w = mk_button(bb, "Context", dbbr(Action));
	set_button_cb(w, context_callback());
	bind_help(w, "help_ctx");
							/*-- text --*/
	text_w = new QTextEdit;
	form->addWidget(text_w);
	text_w->setProperty("helpFont", true);
	text_w->setReadOnly(true);
	// QSS doesn't support :read-only for QTextEdit
	text_w->setProperty("readOnly", true);
	text_w->setLineWrapMode(QTextEdit::NoWrap);
	text_w->setLineWrapMode(QTextEdit::NoWrap);
	text_w->setText(message);
	text_w->setProperty("colSheet", true);
	text_w->setProperty("colStd", true);
	text_w->ensurePolished();
	// size(0, "M").width() -> averageCharWidth()
	text_w->setMinimumWidth(text_w->fontMetrics().averageCharWidth() * 60);
	text_w->setMinimumHeight(text_w->fontMetrics().height() * (nlines + 1));
	// bind_help(text_w, "help"); // already in shell

	form->addWidget(bb);

	set_dialog_cancel_cb(shell, done_callback());

	popup_nonmodal(shell);
	have_shell = true;
	free(message);
}


static void done_callback(void)
{
	destroy_help_popup();
}


static void context_callback(void)
{
	QWhatsThis::enterWhatsThisMode();
}


static char *add_card_help(const char *topic, char *message)
{
	if (!strcmp(topic, "card") && curr_card && curr_card->form
						&& curr_card->form->help) {
		char *msg = (char *)realloc(message, strlen(message) +
				 	strlen(curr_card->form->help) + 2);
		if(!msg) {
			free(message);
			return NULL;
		}
		message = msg;
		strcat(message, "\n");
		strcat(message, curr_card->form->help);
	}
	return message;
}


static char *get_text(
	const char		*topic)
{
	FILE			*fp;		/* help file */
	const char		*fn;
	/* Using a fixed-length buffer here is OK, because help text */
	/* should never exceed 132 characters per line.  If it does, */
	/* the text will just get mangled a bit; easy enough to fix */
	static char		line[1024];	/* line buffer */
	char			*text;		/* text buffer */
	int			textsize;	/* size of text buffer */
	int			textlen = 0;	/* # of chars in text buffer */
	int			n;		/* for stripping trailing \n */
	char			*p;

	if (!(text = (char *)malloc(textsize = 4096)))
		return(0);
	*text = 0;
	if (!(fn = find_file(HELP_FN, false)) || !(fp = fopen(fn, "r"))) {
		sprintf(text, "Sorry, no help available,\n%s not found",
								HELP_FN);
		return(text);
	}
	for (;;) {					/* find topic */
		if (!fgets(line, sizeof(line), fp)) {
			strcpy(text, "Sorry, no help available on this topic");
			return(text);
		}
		if (line[0] != '%' || line[1] != '%')
			continue;
		line[strlen(line)-1] = 0; /* strip \n */
		for (p=line+2; *p >= '0' && *p <= '9'; p++);
		for (; *p == ' ' || *p == '\t'; p++);
		if (*p && *p != '\n' && !strcmp(p, topic))
			break;
	}
	for (;;) {					/* read text */
		if (!fgets(line, sizeof(line), fp))
			break;
		if (line[0] == '#')
			continue;
		if (line[0] == '%' && line[1] == '%')
			break;
		p = line[0] == '\t' ? line+1 : line;
		n = strlen(p);
		if (textlen + n + 1 > textsize) {
			char *otext = text;
			if (!(text = (char *)realloc(text, textsize += 4096))) {
				text = otext;
				break;
			}
		}
		memcpy(text + textlen, p, n);
		textlen += n;
	}
	while(textlen > 0 && text[textlen - 1] == '\n')
		textlen--;
	text[textlen] = 0;
	return(text);
}

void bind_help(
	QWidget		*parent,
	const char	*topic)
{
	static QWidget *w = NULL;
	static QString helpfont;
	char *message = get_text(topic);
	message = add_card_help(topic, message);
	if(message) {
		/* Qt gives HTML WhatsThis? better treatment than plain text */
		/* Plain text gets a narrow widget with auto-wrap, which */
		/* always looks awful. */
		/* HTML gets sized to the HTML's natural boundaries, which */
		/* can be overridden by not allowing line breaks. */
		/* With conversion, it's still bad in some places */
		/* I should probably allow HTML snippets; maybe auto-detect */
		QString s(QString(message).toHtmlEscaped());
		/* Using <pre> is OK, but it's monospaced, which isn't always */
		/* best.  However, I guess some help uses monospacing for */
		/* formatting.  */
		/* note that some sort of tag needs to be in there, or else */
		/* mightBeRichText() will think it's plain text */
		/* toHtmlEscaped() is rarely enough */
#if 0
		s.prepend("<pre>");
		s.append("</pre>");
#else
		if(!w) {
			w = new QWidget;
			w->setProperty("helpFont", true);
			w->setProperty("whatsThisFont", true);
			w->ensurePolished();
			helpfont = HtmlCssUtils::encodeCssFont(w->font());
			delete w;
		}
		s.prepend("<div style=\"white-space: pre; " + helpfont + "\">");
		s.append("</div>");
#endif
		parent->setWhatsThis(s);
		free(message);
	}
}
