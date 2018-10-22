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
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef DIRECT
#include <sys/dir.h>
#define  dirent direct
#else
#include <dirent.h>
#endif
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelP.h>
#include <Xm/ArrowBP.h>
#include <Xm/ArrowBG.h>
#include <Xm/Text.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <X11/cursorfont.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

#define OFF		16		/* margin around summary and card */
#define NHIST		20		/* remember this many search strings */

#ifndef LIB
#define LIB "/usr/local/lib"
#endif

static void append_search_string(char *);
static void file_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void newform_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void help_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void dbase_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void section_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void mode_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void sect_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void query_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void sort_pulldown	(Widget, int, XmToggleButtonCallbackStruct *);
static void search_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void requery_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void clear_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void letter_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void pos_callback	(Widget, int, XmToggleButtonCallbackStruct *);
static void new_callback  (Widget, XtPointer, XmToggleButtonCallbackStruct *);
static void dup_callback  (Widget, XtPointer, XmToggleButtonCallbackStruct *);
static void del_callback  (Widget, XtPointer, XmToggleButtonCallbackStruct *);

CARD 			*curr_card;	/* card being displayed in main win, */
char			*prev_form;	/* previous form name */
Widget			mainwindow;	/* popup menus hang off main window */
int			last_query=-1;	/* last query pd index, for ReQuery */
static Dimension	win_xs, win_ys;	/* size of main window w/o sum+card */
static Widget		menubar;	/* menu bar in main window */
static Widget		dbpulldown;	/* dbase pulldown menu widget */
static Widget		sectpulldown;	/* sectn pulldown menu widget */
static Widget		sortpulldown;	/* sort  pulldown menu widget */
static Widget		qpulldown;	/* query pulldown menu widget */
static Widget		sectpdcall;	/* button that calls section pulldn */

static Widget		form;		/* w_sect etc are on this form */
static Widget		w_info;		/* info label, for "n found" */
static Widget		w_mtime;	/* shows last modify time of card */
static Widget		w_search;	/* search string text widget */
static Widget		w_prev;		/* search prev arrow */
static Widget		w_next;		/* search next arrow */
       Widget		w_summary;	/* form for summary table */
static Widget		w_letter[28];	/* buttons for a,b,c,...,z,misc,* */
static Widget		w_card;		/* form for card */
static Widget		w_left;		/* button: previous card */
static Widget		w_right;	/* button: next card */
static Widget		w_new;		/* button: start a new card */
static Widget		w_dup;		/* button: duplicate a card */
static Widget		w_del;		/* button: delete current card */
static Widget		w_sect;		/* button: section popup for card */
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
	XmString	s[20];
	Widget		fmenu, w, searchform, popup;
	Arg		args[30];
	long		n, i, wid;
	char		buf[10];

	mainwindow = XtCreateManagedWidget("mainwindow",
			xmMainWindowWidgetClass, toplevel, NULL, 0);

							/*-- menu bar --*/
	s[0] = XmStringCreateSimple((char *)"File");
	s[1] = XmStringCreateSimple((char *)"Database");
	s[2] = XmStringCreateSimple((char *)"Section");
	s[3] = XmStringCreateSimple((char *)"Sort");
	s[4] = XmStringCreateSimple((char *)"Query");
	s[5] = XmStringCreateSimple((char *)"Help");
	menubar = XmVaCreateSimpleMenuBar(mainwindow, (char *)"menubar",
			FIX_MENUBAR
			XmVaCASCADEBUTTON, s[0], 'F',
			XmVaCASCADEBUTTON, s[1], 'D',
			XmVaCASCADEBUTTON, s[2], 'e',
			XmVaCASCADEBUTTON, s[3], 'S',
			XmVaCASCADEBUTTON, s[4], 'Q',
			XmVaCASCADEBUTTON, s[5], 'H',
			NULL);
	if (w = XtNameToWidget(menubar, "button_5"))
		XtVaSetValues(menubar, XmNmenuHelpWidget, w, NULL);
	sectpdcall = XtNameToWidget(menubar, "button_2");
	for (n=0; n < 5; n++)
		XmStringFree(s[n]);

	s[0]  = XmStringCreateSimple((char *)"Find & select");
	s[1]  = XmStringCreateSimple((char *)"Print...");
	s[2]  = XmStringCreateSimple((char *)"Export...");
	s[3]  = XmStringCreateSimple((char *)"Preferences...");
	s[4]  = XmStringCreateSimple((char *)"Form editor");
	s[5]  = XmStringCreateSimple((char *)"About...");
	s[6]  = XmStringCreateSimple((char *)"Save");
	s[7]  = XmStringCreateSimple((char *)"Quit");
	s[8]  = XmStringCreateSimple((char *)"Rambo Quit");
	s[9]  = XmStringCreateSimple((char *)"Ctrl-F");
	s[10] = XmStringCreateSimple((char *)"Ctrl-P");
	s[11] = XmStringCreateSimple((char *)"Ctrl-E");
	s[12] = XmStringCreateSimple((char *)"Ctrl-R");
	s[13] = XmStringCreateSimple((char *)"Ctrl-S");
	s[14] = XmStringCreateSimple((char *)"Ctrl-Q");
	fmenu = XmVaCreateSimplePulldownMenu(menubar, (char *)"file", 0,
			(XtCallbackProc)file_pulldown,
			FIX_MENUBAR
			XmVaPUSHBUTTON,    s[0], 'F',  "Ctrl<Key>F", s[9],
			XmVaPUSHBUTTON,    s[1], 'P',  "Ctrl<Key>P", s[10],
			XmVaPUSHBUTTON,    s[2], 'E',  "Ctrl<Key>E", s[11],
			XmVaPUSHBUTTON,    s[3], 'r',  "Ctrl<Key>R", s[12],
			XmVaCASCADEBUTTON, s[4], 'o',
			XmVaPUSHBUTTON,    s[5], 'A',  NULL, NULL,
			XmVaPUSHBUTTON,    s[6], 'S',  "Ctrl<Key>S", s[13],
			XmVaPUSHBUTTON,    s[7], 'Q',  "Ctrl<Key>Q", s[14],
			XmVaPUSHBUTTON,    s[8], NULL, NULL, NULL,
			NULL);
#ifdef XmNtearOffModel
	XtVaSetValues(fmenu, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
#endif
	XtAddCallback(fmenu, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pd_file");
	for (n=0; n < 15; n++)
		XmStringFree(s[n]);

	if (restricted && (w = XtNameToWidget(fmenu, "button_4")))
		XtVaSetValues(w, XmNsensitive, FALSE, NULL);

	s[0] = XmStringCreateSimple((char *)"Edit current form...");
	s[1] = XmStringCreateSimple((char *)"Create new form from scratch...");
	s[2] = XmStringCreateSimple((char *)"Create, use current as template...");
	fmenu = XmVaCreateSimplePulldownMenu(fmenu, (char *)"newform", 4,
			(XtCallbackProc)newform_pulldown,
			FIX_MENUBAR
			XmVaPUSHBUTTON,    s[0], 'E',  NULL, NULL,
			XmVaPUSHBUTTON,    s[1], 'C',  NULL, NULL,
			XmVaPUSHBUTTON,    s[2], 't',  NULL, NULL,
			NULL);
#ifdef XmNtearOffModel
	XtVaSetValues(fmenu, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
#endif
	XtAddCallback(fmenu, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pd_file");
	for (n=0; n < 3; n++)
		XmStringFree(s[n]);

	dbpulldown = XmVaCreateSimplePulldownMenu(menubar, (char *)"dbase", 1,
			(XtCallbackProc)dbase_pulldown,
			FIX_MENUBAR
			NULL);
	XtAddCallback(dbpulldown, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pd_dbase");

	sectpulldown = XmVaCreateSimplePulldownMenu(menubar, (char *)"section", 2,
			(XtCallbackProc)section_pulldown,
			FIX_MENUBAR
			NULL);
	XtAddCallback(sectpulldown, XmNhelpCallback,
			(XtCallbackProc)help_callback,(XtPointer)"pd_section");

	sortpulldown = XmVaCreateSimplePulldownMenu(menubar, (char *)"sort", 3,
			(XtCallbackProc)sort_pulldown,
			FIX_MENUBAR
			NULL);
	XtAddCallback(sortpulldown, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pd_sort");

	qpulldown = XmVaCreateSimplePulldownMenu(menubar, (char *)"query", 4,
			(XtCallbackProc)query_pulldown,
			FIX_MENUBAR
			NULL);
	XtAddCallback(qpulldown, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pd_query");

	s[0] = XmStringCreateSimple((char *)"On context");
	s[1] = XmStringCreateSimple((char *)"Current database");
	s[2] = XmStringCreateSimple((char *)"Introduction");
	s[3] = XmStringCreateSimple((char *)"Getting help");
	s[4] = XmStringCreateSimple((char *)"Troubleshooting");
	s[5] = XmStringCreateSimple((char *)"Files and programs");
	s[6] = XmStringCreateSimple((char *)"Expression grammar");
	s[7] = XmStringCreateSimple((char *)"Variables and X Resources");
	s[8] = XmStringCreateSimple((char *)"Ctrl-H");
	s[9] = XmStringCreateSimple((char *)"Ctrl-D");
	s[10]= XmStringCreateSimple((char *)"Ctrl-G");
	fmenu = XmVaCreateSimplePulldownMenu(menubar, (char *)"help", 5,
			(XtCallbackProc)help_pulldown,
			FIX_MENUBAR
			XmVaPUSHBUTTON, s[0], 'C',  "Ctrl<Key>H", s[8],
			XmVaPUSHBUTTON, s[1], 'D',  "Ctrl<Key>D", s[9],
			XM_VA_SEPARATOR
			XmVaPUSHBUTTON, s[2], 'I',  NULL, NULL,
			XmVaPUSHBUTTON, s[3], 'G',  NULL, NULL,
			XmVaPUSHBUTTON, s[4], 'T',  NULL, NULL,
			XmVaPUSHBUTTON, s[5], 'F',  NULL, NULL,
			XmVaPUSHBUTTON, s[6], 'E',  "Ctrl<Key>G", s[10],
			XmVaPUSHBUTTON, s[7], 'V',  NULL, NULL,
			NULL);
#ifdef XmNtearOffModel
	XtVaSetValues(fmenu, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
#endif
	for (n=0; n < 10; n++)
		XmStringFree(s[n]);

	form = XtCreateWidget((char *)"mainform", xmFormWidgetClass,
			mainwindow, NULL, 0);
							/*-- search string --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		10);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	OFF);			n++;
	XtSetArg(args[n], XmNheight,		34);			n++;
	searchform = XtCreateManagedWidget((char *)"searchform", xmFormWidgetClass,
			form, args, n);
	XtAddCallback(searchform, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	w = XtCreateManagedWidget((char *)"Requery", xmPushButtonWidgetClass,
			searchform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)requery_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		5);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	5);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNrightOffset,	10);			n++;
	XtSetArg(args[n], XmNarrowDirection,	XmARROW_RIGHT);		n++;
	XtSetArg(args[n], XmNsensitive,		FALSE);			n++;
	XtSetArg(args[n], XmNforeground,	color[COL_BACK]);	n++;
	w = w_next = XtCreateManagedWidget((char *)"next", xmArrowButtonWidgetClass,
			searchform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)search_callback, (XtPointer)1);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		5);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	5);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	w = XtCreateManagedWidget((char *)"C", xmPushButtonWidgetClass,
			searchform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)clear_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNtopOffset,		5);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	5);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w);			n++;
	XtSetArg(args[n], XmNarrowDirection,	XmARROW_LEFT);		n++;
	XtSetArg(args[n], XmNsensitive,		FALSE);			n++;
	XtSetArg(args[n], XmNforeground,	color[COL_BACK]);	n++;
	w = w_prev = XtCreateManagedWidget((char *)"prev", xmArrowButtonWidgetClass,
			searchform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)search_callback, (XtPointer)-1);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	OFF);			n++;
	w = XtCreateManagedWidget((char *)"Search", xmPushButtonWidgetClass,
			searchform, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)search_callback, (XtPointer)0);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");

#ifdef XmCSimpleOptionMenu
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	0);			n++;
	XtSetArg(args[n], XmNfontList,		fontlist[FONT_STD]);	n++;
	popup = XmCreatePulldownMenu(form, (char *)"modepd", NULL, 0);
	for (i=0; i < SM_NMODES; i++) {
		w = XtCreateManagedWidget(modename[i],
				xmPushButtonGadgetClass, popup, NULL, 0);
		XtAddCallback(w, XmNactivateCallback,
				(XtCallbackProc)mode_callback, (XtPointer)i);
	}
	XtSetArg(args[n], XmNmarginHeight,	0);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	XtSetArg(args[n], XmNsubMenuId,		popup);			n++;
	XtSetArg(args[n], XmNlabelString,	0);			n++;
	w = XmCreateOptionMenu(searchform, (char *)"modeoption", args, n);
	XtManageChild(w);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");
#endif

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w);			n++;
	XtSetArg(args[n], XmNleftOffset,	3);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNrightWidget,	w_prev);		n++;
	XtSetArg(args[n], XmNrightOffset,	5);			n++;
	XtSetArg(args[n], XmNpendingDelete,	True);			n++;
	XtSetArg(args[n], XmNbackground,	color[COL_TEXTBACK]);	n++;
	XtSetArg(args[n], XmNfontList,		fontlist[FONT_HELV]);	n++;
	w_search = XtCreateManagedWidget((char *)" ", xmTextWidgetClass,
			searchform, args, n);
	XtAddCallback(w_search, XmNactivateCallback,
			(XtCallbackProc)search_callback, NULL);
	XtAddCallback(w_search, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"search");

							/*-- info line --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		searchform);		n++;
	XtSetArg(args[n], XmNtopOffset,		5);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	OFF);			n++;
	XtSetArg(args[n], XmNalignment, 	XmALIGNMENT_BEGINNING);	n++;
	XtSetArg(args[n], XmNfontList,		fontlist[FONT_STD]);	n++;
	w_info = XtCreateManagedWidget((char *)" ", xmLabelWidgetClass,
			form, args, n);
	XtAddCallback(w_info, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"info");

	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		searchform);		n++;
	XtSetArg(args[n], XmNtopOffset,		5);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_info);		n++;
	XtSetArg(args[n], XmNleftOffset,	OFF);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	OFF);			n++;
	XtSetArg(args[n], XmNalignment, 	XmALIGNMENT_END);	n++;
	XtSetArg(args[n], XmNfontList,		fontlist[FONT_STD]);	n++;
	w_mtime = XtCreateManagedWidget((char *)" ", xmLabelWidgetClass,
			form, args, n);
	XtAddCallback(w_mtime, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"info");

							/*-- buttons --*/
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	OFF);			n++;
	XtSetArg(args[n], XmNwidth,		30);			n++;
	XtSetArg(args[n], XmNheight,		30);			n++;
	XtSetArg(args[n], XmNlabelType,		XmPIXMAP);		n++;
	XtSetArg(args[n], XmNlabelPixmap,	pixmap[PIC_LEFT]);	n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	w_left = XtCreateManagedWidget((char *)"Left", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w_left, XmNactivateCallback,
			(XtCallbackProc)pos_callback, (XtPointer)-1);
	XtAddCallback(w_left, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pos");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_left);		n++;
	XtSetArg(args[n], XmNleftOffset,	4);			n++;
	XtSetArg(args[n], XmNwidth,		30);			n++;
	XtSetArg(args[n], XmNheight,		30);			n++;
	XtSetArg(args[n], XmNlabelType,		XmPIXMAP);		n++;
	XtSetArg(args[n], XmNlabelPixmap,	pixmap[PIC_RIGHT]);	n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	w_right = XtCreateManagedWidget((char *)"Right", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w_right, XmNactivateCallback,
			(XtCallbackProc)pos_callback, (XtPointer)1);
	XtAddCallback(w_right, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"pos");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_right);		n++;
	XtSetArg(args[n], XmNleftOffset,	16);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	w_new = XtCreateManagedWidget((char *)"New", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w_new, XmNactivateCallback,
			(XtCallbackProc)new_callback, (XtPointer)0);
	XtAddCallback(w_new, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"new");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_new);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	w_dup = XtCreateManagedWidget((char *)"Dup", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w_dup, XmNactivateCallback,
			(XtCallbackProc)dup_callback, (XtPointer)0);
	XtAddCallback(w_dup, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"dup");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_dup);			n++;
	XtSetArg(args[n], XmNleftOffset,	8);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNsensitive,		False);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	w_del = XtCreateManagedWidget((char *)"Delete", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w_del, XmNactivateCallback,
			(XtCallbackProc)del_callback, (XtPointer)0);
	XtAddCallback(w_del, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"del");

	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	OFF);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	w = XtCreateManagedWidget((char *)"Help", xmPushButtonWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc)help_callback, (XtPointer)"card");
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"card");

							/*-- summary --*/
#if 0
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w_info);		n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	w = XtCreateManagedWidget((char *)" ", xmSeparatorWidgetClass,
			form, args, n);
#else
	w = w_info;
#endif
	n = 0;
	XtSetArg(args[n], XmNwidth,		400);			n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		4);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	OFF);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	OFF);			n++;
	w = w_summary = XtCreateManagedWidget((char *)"summform", xmFormWidgetClass,
			form, args, n);
	XtAddCallback(w, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"summary");

							/*-- letters --*/
	if (pref.letters) {
	 n = 0;
	 XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	 XtSetArg(args[n], XmNtopWidget,	w_summary);		n++;
	 XtSetArg(args[n], XmNtopOffset,	5);			n++;
	 XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	 XtSetArg(args[n], XmNleftOffset,	OFF+2);			n++;
	 XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	 XtSetArg(args[n], XmNrightOffset,	OFF);			n++;
	 w = XtCreateManagedWidget((char *)"letters", xmFormWidgetClass,
			form, args, n);
	 XtAddCallback(w, XmNhelpCallback,
	 		(XtCallbackProc)help_callback,(XtPointer)"letters");

	 for (i=0; i < 28; i++) {
	  wid = strlen_in_pixels("W", FONT_STD);
	  sprintf(buf, i < 26 ? "%c" : i==26 ? "misc" : "all", (int)i+'A');
	  n = 0;
	  if (i == 0) {
	   XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	  } else {
	   XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	   XtSetArg(args[n], XmNleftWidget,	w_letter[i-1]);		n++;
	  }
	  if (i < 26) {
	   XtSetArg(args[n], XmNwidth,		wid + 6);		n++;
	  }
	  XtSetArg(args[n], XmNheight,		wid + 6);		n++;
	  XtSetArg(args[n], XmNleftOffset,	0);			n++;
	  XtSetArg(args[n], XmNshadowThickness,	2);			n++;
	  XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);		n++;
	  XtSetArg(args[n], XmNhighlightThickness, 0);			n++;
	  w_letter[i] = XtCreateManagedWidget(buf, xmPushButtonWidgetClass,
					w, args, n);
	  XtAddCallback(w_letter[i], XmNactivateCallback,
			(XtCallbackProc)letter_callback, (XtPointer)i);
	  XtAddCallback(w_letter[i], XmNhelpCallback,
	 		(XtCallbackProc) help_callback, (XtPointer)"letters");
	 }
	}
							/*-- card --*/
	n = 0;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		OFF);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	w = XtCreateManagedWidget(" ", xmSeparatorWidgetClass,
			form, args, n);
	n = 0;
	XtSetArg(args[n], XmNwidth,		400);			n++;
	XtSetArg(args[n], XmNheight,		6);			n++;
	XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNtopWidget,		w);			n++;
	XtSetArg(args[n], XmNtopOffset,		OFF);			n++;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNbottomWidget,	w_new);			n++;
	XtSetArg(args[n], XmNbottomOffset,	8);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNleftOffset,	OFF);			n++;
	XtSetArg(args[n], XmNrightAttachment,	XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightOffset,	OFF);			n++;
	w_card = XtCreateManagedWidget("cardform", xmFormWidgetClass,
			form, args, n);
	XtAddCallback(w_card, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"card");

	XtManageChild(form);
	XtManageChild(menubar);
	create_summary_menu(curr_card, w_summary, mainwindow);
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
	Arg		args[2];
	Dimension	xs=0, ys=0;

	if (!win_ys) {
		XtSetArg(args[0], XmNwidth,  &win_xs);
		XtSetArg(args[1], XmNheight, &win_ys);
		XtGetValues(mainwindow, args, 2);
		win_ys -= 1;
	}
	if (curr_card && curr_card->form) {
		xs = pref.scale * curr_card->form->xs + 2;
		ys = pref.scale * curr_card->form->ys + 2;
	}
	if (win_xs > xs) xs = win_xs;
	XtSetArg(args[0], XmNwidth,  xs + 2*2 + 2*16);
	XtSetArg(args[1], XmNheight, win_ys + ys + 2*3);
	XtSetValues(XtParent(mainwindow), args, 2);
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

	if (!display)
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
	Widget	widget;		/* button widgets, destroyed before remake */
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
			db[i].widget = XtCreateManagedWidget(" ",
				xmSeparatorWidgetClass, dbpulldown, NULL, 0);
#			endif
		} else {
			db[i].widget = XtCreateManagedWidget(db[i].name+1,
				xmPushButtonGadgetClass, dbpulldown, NULL, 0);
			XtAddCallback(db[i].widget, XmNactivateCallback,
				(XtCallbackProc)dbase_pulldown, (XtPointer)(unsigned long)i);
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

	for (n=0; n < MAXD; n++) {
		if (db[n].widget)
			XtDestroyWidget(db[n].widget);
		if (db[n].path)
			free((void *)db[n].path);
		if (db[n].name)
			free((void *)db[n].name);
		db[n].widget = 0;
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
	sprintf(path, "%s/grokdir", LIB);
	n = append_to_dbase_list(n, path, 3);
	qsort(db, n, sizeof(struct db), compare_db);
	make_dbase_pulldown(n);
#ifdef XmNtearOffModel
	XtVaSetValues(dbpulldown, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
#endif
}


/*
 * After a database was loaded, there is a section list in the dbase struct.
 * Present it in a pulldown if there are at least two sections.
 */

#define MAXSC  200		/* no more than 200 sections in pulldown */
static Widget  scwidget[MAXSC];	/* button widgets, destroyed before remake */

void remake_section_pulldown(void)
{
	int		maxn;
	long		n;
	char		*name[MAXSC];

	for (n=0; n < MAXSC; n++) {
		if (scwidget[n])
			XtDestroyWidget(scwidget[n]);
		scwidget[n] = 0;
	}
	if (!curr_card || !curr_card->dbase || curr_card->form->proc) {
		XtVaSetValues(sectpdcall, XmNsensitive, FALSE, NULL);
		return;
	}
	maxn = curr_card->dbase->havesects ? curr_card->dbase->nsects + 2 : 1;
	if (maxn > MAXSC)
		maxn = MAXSC;
	for (n=0; n < maxn; n++)
		name[n] = mystrdup(
			  n == maxn-1 ? "New ..." :
  			  n == 0      ? "All"
				      : section_name(curr_card->dbase, n-1));
	for (n=0; n < maxn; n++) {
		scwidget[n] = XtCreateManagedWidget(name[n],
				xmPushButtonGadgetClass, sectpulldown, NULL,0);
		XtAddCallback(scwidget[n], XmNactivateCallback,
				(XtCallbackProc)section_pulldown,(XtPointer)n);
		free(name[n]);
	}
	XtVaSetValues(sectpdcall, XmNsensitive, TRUE, NULL);
#ifdef XmNtearOffModel
	XtVaSetValues(sectpulldown, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
#endif
	remake_section_popup(TRUE);
}


/*
 * Put all the default queries in the form into the Query pulldown (pulldown
 * #3). Add a default query "All" at the top. This is yet another violation
 * of the Motif style guide. So what.
 */

#define MAXQ	100		/* no more than 100 queries in pulldown */
static Widget	qwidget[MAXQ];	/* button widgets, destroyed before remake */

void remake_query_pulldown(void)
{
	long		i;		/* # of queries in pulldown */
	int		n;		/* max # of lines in pulldown */
	Arg		args[10];

	for (n=0; n < MAXQ; n++) {
		if (qwidget[n])
			XtDestroyWidget(qwidget[n]);
		qwidget[n] = 0;
	}
	if (!curr_card || !curr_card->form)
		return;

	XtSetArg(args[0], XmNindicatorType, XmN_OF_MANY);
	XtSetArg(args[1], XmNselectColor,   color[COL_TOGGLE]);
	XtSetArg(args[2], XmNset,	    pref.autoquery);
	qwidget[0] = XtCreateManagedWidget("Autoquery",
			xmToggleButtonWidgetClass, qpulldown, args, 3);
	XtAddCallback(qwidget[0], XmNvalueChangedCallback,
			(XtCallbackProc)query_pulldown, (XtPointer)-2);

	n = curr_card->form->nqueries > MAXQ-2 ? MAXQ-2
					       : curr_card->form->nqueries;
	for (i=0; i <= n; i++) {
		DQUERY *dq = i ? &curr_card->form->query[i-1] : 0;
		const char *name = i ? dq->name : "All";
		if (i && (dq->suspended || !dq->name || !dq->query))
			continue;
		if (pref.autoquery) {
			XtSetArg(args[0], XmNindicatorType, XmONE_OF_MANY);
			XtSetArg(args[1], XmNselectColor,   color[COL_TOGGLE]);
			XtSetArg(args[2], XmNset, curr_card->form->autoquery ==
									i-1);
			qwidget[i+1] = XtCreateManagedWidget(name,
				xmToggleButtonWidgetClass, qpulldown, args, 3);
			XtAddCallback(qwidget[i+1], XmNvalueChangedCallback,
				(XtCallbackProc)query_pulldown,
				(XtPointer)(i-1));
		} else {
			qwidget[i+1] = XtCreateManagedWidget(name,
				xmPushButtonGadgetClass, qpulldown, NULL, 0);
			XtAddCallback(qwidget[i+1], XmNactivateCallback,
				(XtCallbackProc)query_pulldown,
				(XtPointer)(i-1));
		}
	}
#ifdef XmNtearOffModel
	XtVaSetValues(qpulldown, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
#endif
	last_query = curr_card->form->autoquery;
}


/*
 * put a new option popup menu on the card's Section button
 */

static void	remake_popup(void);
static Widget	*pwidgets;		/* label widgets in popup */
static int	pnwidgets;		/* number of widgets in pwidgets[] */

void remake_section_popup(
	BOOL		newsects)	/* did the section list change? */
{
#ifdef XmCSimpleOptionMenu
	Arg		args[2];

	if (newsects)
		remake_popup();

	if (!curr_card	|| !curr_card->dbase
			||  curr_card->row < 0
			||  curr_card->row >= curr_card->dbase->nrows
			|| !curr_card->dbase->havesects) {
		if (w_sect) {
			XtSetArg(args[0], XmNsensitive, FALSE);
			XtSetValues(w_sect, args, 1);
			XtSetValues(w_del,  args, 1);
			XtSetValues(XmOptionButtonGadget(w_sect), args, 1);
		}
		return;
	}
	if (w_sect) {
		XtSetArg(args[0], XmNsensitive, TRUE);
		XtSetArg(args[1], XmNmenuHistory, pwidgets[
			curr_card->dbase->row[curr_card->row]->section]);
		XtSetValues(w_sect, args, 2);
		XtSetValues(w_del,  args, 1);
		XtSetValues(XmOptionButtonGadget(w_sect), args, 1);
	}
#endif
}


static void remake_popup(void)
{
#ifdef XmCSimpleOptionMenu
	static Widget	popup;		/* the popup menu */
	XmString	str;		/* for labels in pwidgets, temp */
	Arg		args[20];	/* for option menu creation */
	int		i, n;

	if (w_sect) {
		XtDestroyWidget(w_sect);
		w_sect = 0;
	}
	if (popup) {
		XtDestroyWidget(popup);
		popup = 0;
	}
	if (pwidgets) {
		for (i=0; i < pnwidgets; i++)
			XtDestroyWidget(pwidgets[i]);
		free(pwidgets);
		pwidgets = 0;
	}
	pnwidgets = curr_card->dbase->nsects;
	if (pnwidgets < 2) {
		pnwidgets = 0;
		return;
	}
	if (!(pwidgets = (Widget *)malloc(sizeof(Widget) * pnwidgets)))
		return;

	popup = XmCreatePulldownMenu(form, (char *)"pulldown", NULL, 0);
	str = XmStringCreateSimple((char *)"");
	n = 0;
	XtSetArg(args[n], XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget,	w_del);			n++;
	XtSetArg(args[n], XmNmarginHeight,	0);			n++;
	XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);	n++;
	XtSetArg(args[n], XmNleftWidget,	w_del);			n++;
	XtSetArg(args[n], XmNwidth,		60);			n++;
	XtSetArg(args[n], XmNhighlightThickness,1);			n++;
	XtSetArg(args[n], XmNsubMenuId,		popup);			n++;
	XtSetArg(args[n], XmNlabelString,	str);			n++;
	w_sect = XmCreateOptionMenu(form, (char *)"sectoption", args, n);
	XtAddCallback(w_sect, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"sect");
	XmStringFree(str);

	for (i=0; i < pnwidgets; i++) {
		XtSetArg(args[0], XmNsensitive,
				!curr_card->dbase->sect[i].rdonly);
		pwidgets[i] = XtCreateManagedWidget(
				section_name(curr_card->dbase, i),
				xmPushButtonGadgetClass, popup, NULL, 0);
		XtAddCallback(pwidgets[i], XmNactivateCallback,
				(XtCallbackProc)sect_callback, (XtPointer)(unsigned long)i);
	}
	XtManageChild(w_sect);
#endif
}


/*
 * Put all the columns in the summary into the Sort pulldown (pulldown #2),
 * except those that have the nosort flag set.
 * What you hear weeping is the author of the Motif style guide.
 */

#define MAXS	100		/* no more than 100 criteria in pulldown */
static Widget	swidget[2*MAXS+1];	/* widgets, destroyed before remake */

void remake_sort_pulldown(void)
{
	int	sort_col[2*MAXS+1];	/* column for each pulldown item */
	char		buf[256];	/* pulldown line text */
	register ITEM	*item;		/* scan items for sortable columns */
	int		i;		/* item counter */
	int		j;		/* skip redundant choice items */
	int		n;		/* # of lines in pulldown */
	Arg		args[10];

	for (n=0; n < MAXS; n++) {
		if (swidget[n])
			XtDestroyWidget(swidget[n]);
		swidget[n] = 0;
	}
	if (!curr_card || !curr_card->form || !curr_card->dbase)
		return;

	XtSetArg(args[0], XmNindicatorType, XmN_OF_MANY);
	XtSetArg(args[1], XmNselectColor,   color[COL_TOGGLE]);
	XtSetArg(args[2], XmNset,	    pref.revsort);
	swidget[0] = XtCreateManagedWidget((char *)"Reverse sort",
			xmToggleButtonWidgetClass, sortpulldown, args, 3);
	XtAddCallback(swidget[0], XmNvalueChangedCallback,
			(XtCallbackProc)sort_pulldown, (XtPointer)-1);

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
		XtSetArg(args[0], XmNindicatorType, XmONE_OF_MANY);
		XtSetArg(args[1], XmNselectColor,   color[COL_TOGGLE]);
		XtSetArg(args[2], XmNset,	   item->column==pref.sortcol);
		swidget[n] = XtCreateManagedWidget(buf,
			xmToggleButtonWidgetClass, sortpulldown, args, 3);
		XtAddCallback(swidget[n], XmNvalueChangedCallback,
				(XtCallbackProc)sort_pulldown,
				(XtPointer)item->column);
		sort_col[n++] = item->column;
	}
#ifdef XmNtearOffModel
	XtVaSetValues(sortpulldown, XmNtearOffModel, XmTEAR_OFF_ENABLED,NULL);
#endif
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
	Arg		args[5];
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
		create_summary_menu(curr_card, w_summary, mainwindow);

		strcpy(name, formname);
		if (*name >= 'a' && *name <= 'z')
			*name += 'A' - 'a';
		if (p = strrchr(name, '.'))
			*p = 0;
		if (p = strrchr(name, '/'))
			p++;
		else
			p = name;
		if (toplevel) {
			XtVaSetValues(toplevel, XmNiconName, p, NULL);
			fillout_card(curr_card, FALSE);
		}
	} else
		if (toplevel)
			XtVaSetValues(toplevel, XmNiconName, (char *)"None", NULL);

	if (toplevel) {
		XtSetArg(args[0], XmNsensitive, formname != 0);
		XtSetValues(w_left,  args, 1);
		XtSetValues(w_right, args, 1);
		XtSetValues(w_del,   args, 1);
		XtSetValues(w_dup,   args, 1);
		XtSetValues(w_new,   args, 1);

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
	int		i, j;		/* query count, query index */
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

/*ARGSUSED*/
static void file_pulldown(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	card_readback_texts(curr_card, -1);
	switch (item) {
	  case 0: {						/* find&sel */
		char *string = XmTextGetString(w_search);
		if (string)
			find_and_select(string);
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


/*ARGSUSED*/
static void newform_pulldown(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
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
			create_error_popup(toplevel, 0,
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
			create_error_popup(toplevel, 0,
			"Please choose database from Database pulldown first");
		break;
	}
}


/*ARGSUSED*/
static void help_pulldown(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	Cursor				cursor;
	Widget				w;

	switch (item) {
	  case 0:						/* context */
		cursor = XCreateFontCursor(display, XC_question_arrow);
		if (w = XmTrackingLocate(mainwindow, cursor, False)) {
			data->reason = XmCR_HELP;
			XtCallCallbacks(w, XmNhelpCallback, &data);
		}
		XFreeCursor(display, cursor);
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


/*ARGSUSED*/
static void dbase_pulldown(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	char				path[1024];

	card_readback_texts(curr_card, -1);
	sprintf(path, "%s/%s.gf", db[item].path, db[item].name+1);
	switch_form(path);
	remake_dbase_pulldown();
}


/*ARGSUSED*/
static void section_pulldown(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
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

		create_summary_menu(curr_card, w_summary, mainwindow);

		curr_card->row = curr_card->query ? curr_card->query[0]
						  : curr_card->dbase->nrows;
		fillout_card(curr_card, FALSE);
	}
}


/*ARGSUSED*/
static void query_pulldown(
	Widget				widget,	/* 0 if called by ReQuery */
	int				item,	/* -1: all, -2: autoquery */
	XmToggleButtonCallbackStruct	*data)	/* 0 if called by ReQuery */
{
	print_info_line();
	if (!curr_card || !curr_card->dbase || !curr_card->dbase->nrows)
		return;
	card_readback_texts(curr_card, -1);
	if (item == -2) {
		pref.autoquery = data->set;
		item = curr_card->form->autoquery;
	} else if (pref.autoquery)
		curr_card->form->autoquery = item;

	do_query(item);

	create_summary_menu(curr_card, w_summary, mainwindow);

	curr_card->row = curr_card->query ? curr_card->query[0]
					  : curr_card->dbase->nrows;
	fillout_card(curr_card, FALSE);
	remake_query_pulldown();
}


/*ARGSUSED*/
static void sort_pulldown(
	Widget				widget,
	int				item,	/* -1 is reverse flag */
	XmToggleButtonCallbackStruct	*data)
{
	if (item < 0)
		pref.revsort = data->set;
	else
		pref.sortcol = item;

	card_readback_texts(curr_card, -1);
	dbase_sort(curr_card, pref.sortcol, pref.revsort);
	create_summary_menu(curr_card, w_summary, mainwindow);
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
		char *string = XtMalloc(strlen(query));
		strcpy(string, query+1);
		append_search_string(string);
		query_search(SM_SEARCH, curr_card, query+1);
	} else {
		char *query = curr_card->form->query[qmode].query;
		if (pref.query2search) {
			char *string = XtMalloc(strlen(query)+1);
			strcpy(string, query);
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

/*ARGSUSED*/
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
	create_summary_menu(card, w_summary, mainwindow);
	card->row = card->nquery ? card->query[0] : card->dbase->nrows;
	fillout_card(card, FALSE);
}


/*
 * Clear the search string.
 */

/*ARGSUSED*/
static void clear_callback(
	Widget				widget,
	int				inc,
	XmToggleButtonCallbackStruct	*data)
{
	print_text_button_s(w_search, "");
	XmProcessTraversal(w_search, XmTRAVERSE_CURRENT);
}


/*
 * Searches. When a search string is entered, read it from the button and
 * search for it. The arrows put a string from the history array into the
 * button, where it can be searched for. The query pulldown can append a
 * string to the history if pref.query2search is on.
 */

static char	*history[NHIST];
static int	s_curr, s_offs;

/*ARGSUSED*/
static void mode_callback(
	Widget				widget,
	int				item,	/* one of SM_* */
	XmToggleButtonCallbackStruct	*data)
{
	searchmode = (Searchmode)item;
}


/*ARGSUSED*/
static void search_callback(
	Widget				widget,
	int				inc,
	XmToggleButtonCallbackStruct	*data)
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
		char *string = XmTextGetString(w_search);
		if (searchmode != SM_FIND)
			search_cards(searchmode, curr_card, string);
		else if (curr_card->nquery > 0)
			find_and_select(string);
		append_search_string(string);
	}
}


/*ARGSUSED*/
static void requery_callback(
	Widget				widget,
	int				inc,
	XmToggleButtonCallbackStruct	*data)
{
	if (last_query >= 0)
		query_pulldown(0, last_query, 0);
}


static void append_search_string(
	char		*text)
{
	Arg		arg;

	if (text) {
		print_text_button_s(w_search, text);
		s_offs = 0;
		if (history[s_curr] && *history[s_curr]
				    && strcmp(history[s_curr], text))
			s_curr = (s_curr + 1) % NHIST;
		if (history[s_curr])
			XtFree(history[s_curr]);
		history[s_curr] = text;
	}
	XtSetArg(arg, XmNsensitive, s_offs < 0);
	XtSetValues(w_next, &arg, 1);
	XtSetArg(arg, XmNsensitive, s_offs > -NHIST+1 &&
				history[(s_curr + s_offs + NHIST -1) % NHIST]);
	XtSetValues(w_prev, &arg, 1);
}


/*
 * One of the letter buttons (a..z, misc, all) was pressed. Do the appropriate
 * search and display the result in the summary.
 */

/*ARGSUSED*/
static void letter_callback(
	Widget				widget,
	int				letter,
	XmToggleButtonCallbackStruct	*cbs)
{
	if (!curr_card || !curr_card->dbase || !curr_card->dbase->nrows)
		return;
	card_readback_texts(curr_card, -1);
	query_letter(curr_card, letter);
	create_summary_menu(curr_card, w_summary, mainwindow);
	curr_card->row = curr_card->query ? curr_card->query[0]
					  : curr_card->dbase->nrows;
	fillout_card(curr_card, FALSE);
}


/*
 * go forward or backward in the summary list if there is one. If there is
 * no summary, we must be adding cards and the user wants to see previously
 * added cards; step in the raw database in this case.
 */

/*ARGSUSED*/
static void pos_callback(
	Widget				widget,
	int				inc,
	XmToggleButtonCallbackStruct	*data)
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

/*ARGSUSED*/
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
		create_error_popup(toplevel, 0,
					"Section file\n\"%s\" is read-only",
					dbase->sect[s].path);
		return;
	}
	dbase->currsect = s;
	if (!dbase_addrow(&card->row, dbase)) {
		dbase->currsect = save_sect;
		create_error_popup(toplevel, errno,
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
			create_error_popup(toplevel, errno,
							"No memory for query");
		else {
			card->query = newq;
			card->query[card->nquery++] = card->row;
		}
	dbase_sort(card, pref.sortcol, pref.revsort);
	create_summary_menu(card, w_summary, mainwindow);
	fillout_card(card, FALSE);
	scroll_summary(card);
	print_info_line();
	for (i=0; i < card->form->nitems; i++, item++) {
		item = card->form->items[i];
		if (item->type == IT_INPUT || item->type == IT_TIME
					   || item->type == IT_NOTE) {
			XmProcessTraversal(card->items[i].w0,
						XmTRAVERSE_CURRENT);
			break;
		}
	}
}


/*ARGSUSED*/
static void new_callback(
	Widget				widget,
	XtPointer			null,
	XmToggleButtonCallbackStruct	*cbs)
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

/*ARGSUSED*/
static void dup_callback(
	Widget				widget,
	XtPointer			null,
	XmToggleButtonCallbackStruct	*cbs)
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

/*ARGSUSED*/
static void del_callback(
	Widget				widget,
	XtPointer			null,
	XmToggleButtonCallbackStruct	*data)
{
	register CARD			*card = curr_card;
	register int			*p, *q;
	int				i, s;

	if (card->dbase->nrows == 0 || card->row >= card->dbase->nrows)
		return;
	s = card->dbase->row[card->row]->section;
	if (card->dbase->sect[s].rdonly) {
		create_error_popup(toplevel, 0,
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
	create_summary_menu(card, w_summary, mainwindow);
}


/*
 * assign new section to the current card (called from the option popup)
 */

/*ARGSUSED*/
static void sect_callback(
	Widget				widget,
	int				item,
	XmToggleButtonCallbackStruct	*data)
{
	register CARD			*card = curr_card;
	register SECTION		*sect = card->dbase->sect;
	int				olds, news;

	olds = card->dbase->row[card->row]->section;
	news = item;
	if (sect[olds].rdonly)
		create_error_popup(toplevel, 0,
			"section file\n\"%s\" is read-only", sect[olds].path);
	else if (sect[news].rdonly)
		create_error_popup(toplevel, 0,
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
