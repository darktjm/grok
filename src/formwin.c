/*
 * Create the form edit window and the form edit canvas window.
 *
 *	create_formedit_window		Create form edit window and realize
 *	fillout_formedit_widget_by_code	Redraw single item
 */

#include "config.h"
#include <X11/Xos.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelP.h>
#include <Xm/PushBP.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Protocols.h>
#include <X11/cursorfont.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

static void formedit_callback(Widget, int, XmToggleButtonCallbackStruct *);
static int readback_item(int);

extern Display		*display;	/* everybody uses the same server */
extern GC		gc;		/* everybody uses this context */
extern struct config	config;		/* global configuration data */
extern CARD 		*curr_card;	/* card being displayed in main win */
extern Pixel		color[NCOLS];	/* colors: COL_* */
extern XFontStruct	*font[NFONTS];	/* fonts: FONT_* */
extern XmFontList	fontlist[NFONTS];

static BOOL		have_shell;	/* message popup exists if TRUE */
static Widget		shell;		/* popup menu shell */
static FORM		*form;		/* current form definition */
int			curr_item;	/* current item, 0..form.nitems-1 */

char			plan_code[] = "tlwWrecnmsSTA";	/* code 0x260..0x26c */


#define ANY 0xfffe
#define ITX 1<<IT_INPUT  | 1<<IT_TIME | 1<<IT_NOTE
#define TXT 1<<IT_PRINT  | ITX
#define TXF 1<<IT_CHOICE | TXT
#define FLG 1<<IT_CHOICE | 1<<IT_FLAG
#define BAS ITX | FLG
#define IDF TXF | FLG
#define TIM 1<<IT_TIME
#define BUT 1<<IT_BUTTON
#define VIW 1<<IT_VIEW
#define CHA 1<<IT_CHART

/*
 * next free: 108-10a,114 (global), 237 (field), 30b (chart component)
 */

static struct _template {
	unsigned
	short	sensitive;	/* when type is n, make sensitive if & 1<<n */
	int	type;		/* Text, Label, Rradio, Fflag, -line */
	int	code;		/* code given to (global) callback routine */
	char	*text;		/* label string */
	char	*help;		/* label string */
	Widget	widget;		/* allocated widget */
} template[] = {
	{  ~0, 'L',	 0,	"Form name:",		"fe_form",	},
	{  ~0, 'T',	0x101,	" ",			"fe_form",	},
	{  ~0, 'L',	 0,	"Referenced database:",	"fe_form",	},
	{  ~0, 'T',	0x102,	" ",			"fe_form",	},
	{  ~0, 'L',	 0,	"dbase field delim:",	"fe_delim",	},
	{  ~0, 't',	0x103,	" ",			"fe_delim",	},
	{   0, 'F',	 0,	" ",			0,		},
	{  ~0, 'f',	0x104,	"Read only",		"fe_rdonly",	},
	{  ~0, 'f',	0x114,	"Timestamps/Synchronizable","fe_sync",	},
	{  ~0, 'f',	0x105,	"Procedural",		"fe_proc",	},
	{  ~0, 'B',	0x106,	"Edit",			"fe_proc",	},
	{  ~0, 'L',	 0,	"Comment:",		"fe_ref",	},
	{  ~0, 'T',	0x107,	" ",			"fe_ref",	},

	{  ~0, 'L',	 0,	"Form",			0,		},
	{  ~0, 'B',	0x10c,	"Queries",		"fe_query",	},
	{  ~0, 'B',	0x10b,	"Def Help",		"fe_defhelp",	},
	{  ~0, 'B',	0x10d,	"Debug",		"fe_debug",	},
	{  ~0, 'B',	0x10e,	"Preview",		"fe_preview",	},
	{  ~0, 'B',	0x10f,	"Help",			"edit",		},
	{  ~0, 'B',	0x110,	"Cancel",		"fe_cancel",	},
	{  ~0, 'B',	0x111,	"Done",			"fe_done",	},
	{   0, '-',	 0,	" ",			0,		},
	{  ~0, 'L',	 0,	"Field",		"fe_buts",	},
	{  ~0, 'B',	0x112,	"Add",			"fe_add",	},
	{ ANY, 'B',	0x113,	"Delete",		"fe_delete",	},
	{   0, '[',	 0,	"attrs",		0,		},

	{ ANY, 'L',	 0,	   "Field type:",	"fe_type",	},
	{   0, 'R',	 0,	   " ",			0,		},
	{ ANY, 'r',	IT_INPUT,  "Input",		"fe_type",	},
	{ ANY, 'r',	IT_TIME,   "Time",		"fe_type",	},
	{ ANY, 'r',	IT_NOTE,   "Note",		"fe_type",	},
	{ ANY, 'r',	IT_LABEL,  "Label",		"fe_type",	},
	{ ANY, 'r',	IT_PRINT,  "Print",		"fe_type",	},
	{ ANY, 'r',	IT_CHOICE, "Choice",		"fe_type",	},
	{ ANY, 'r',	IT_FLAG,   "Flag",		"fe_type",	},
	{ ANY, 'r',	IT_BUTTON, "Button",		"fe_type",	},
	{ ANY, 'r',	IT_CHART,  "Chart",		"fe_type",	},
#if 0
	{ ANY, 'r',	IT_VIEW,   "View",		"fe_type",	},
#endif
	{ ANY, 'L',	 0,	"Flags:",		"fe_flags",	},
	{   0, 'F',	 0,	" ",			0,		},
	{ BAS, 'f',	0x200,	"Searchable",		"fe_flags",	},
	{ BAS, 'f',	0x201,	"Read only",		"fe_flags",	},
	{ BAS, 'f',	0x202,	"Not sortable",		"fe_flags",	},
	{ BAS, 'f',	0x203,	"Default sort",		"fe_flags",	},
	{ ANY, 'L',	 0,	"Internal field name:",	"fe_int",	},
	{ ANY, 't',	0x204,	" ",			"fe_int",	},
	{ BAS, 'L',	 0,	"Database column:",	"fe_column",	},
	{ BAS, 't',	0x205,	" ",			"fe_column",	},
	{ BAS, 'L',	 0,	"Summary column:",	"fe_sum",	},
	{ BAS, 't',	0x206,	" ",			"fe_sum",	},
	{ BAS, 'l',	 0,	"Width in summary:",	"fe_sum",	},
	{ BAS, 't',	0x207,	" ",			"fe_sum",	},
	{ FLG, 'L',	 0,	"Choice/flag code:",	"fe_flag",	},
	{ FLG, 't',	0x208,	" ",			"fe_flag",	},
	{ FLG, 'l',	 0,	"shown in summary as",	"fe_flag",	},
	{ FLG, 't',	0x236,	" ",			"fe_flag",	},
	{ TIM, 'L',	 0,	"Time format:",		"fe_time",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ TIM, 'r',	0x209,	"Date",			"fe_time",	},
	{ TIM, 'r',	0x20a,	"Time",			"fe_time",	},
	{ TIM, 'r',	0x20b,	"Date+time",		"fe_time",	},
	{ TIM, 'r',	0x20c,	"Duration",		"fe_time",	},
	{   0, '-',	 0,	" ",			0,		},

	{ ANY, 'L',	 0,	"Label Justification:",	"fe_ljust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ ANY, 'r',	0x20d,	"left",			"fe_ljust",	},
	{ ANY, 'r',	0x20e,	"center",		"fe_ljust",	},
	{ ANY, 'r',	0x20f,	"right",		"fe_ljust",	},
	{ ANY, 'L',	 0,	"Label font:",		"fe_ljust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ ANY, 'r',	0x210,	"Helv",			"fe_lfont",	},
	{ ANY, 'r',	0x211,	"HelvO",		"fe_lfont",	},
	{ ANY, 'r',	0x212,	"HelvN",		"fe_lfont",	},
	{ ANY, 'r',	0x213,	"HelvB",		"fe_lfont",	},
	{ ANY, 'r',	0x214,	"Courier",		"fe_lfont",	},
	{ ANY, 'L',	 0,	"Label text",		"fe_ltxt",	},
	{ ANY, 'T',	0x21a,	" ",			"fe_ltxt",	},
	{   0, '-',	 0,	" ",			0,		},

	{ TXT, 'L',	 0,	"Input Justification:",	"fe_ijust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ TXT, 'r',	0x21b,	"left",			"fe_ijust",	},
	{ TXT, 'r',	0x21c,	"center",		"fe_ijust",	},
	{ TXT, 'r',	0x21d,	"right",		"fe_ijust",	},
	{ TXT, 'L',	 0,	"Input font:",		"fe_ijust",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ TXT, 'r',	0x215,	"Helv",			"fe_ifont",	},
	{ TXT, 'r',	0x216,	"HelvO",		"fe_ifont",	},
	{ TXT, 'r',	0x217,	"HelvN",		"fe_ifont",	},
	{ TXT, 'r',	0x218,	"HelvB",		"fe_ifont",	},
	{ TXT, 'r',	0x219,	"Courier",		"fe_ifont",	},
	{ ITX, 'L',	 0,	"Max input length:",	"fe_range",	},
#if 0
	{ ITX, 't',	0x21e,	" ",			"fe_range",	},
#endif
	{ ITX, 't',	0x21f,	" ",			"fe_range",	},
	{ IDF, 'L',	 0,	"Input default:",	"fe_def",	},
	{ IDF, 'T',	0x220,	" ",			"fe_def",	},
#if 0
	{ ITX, 'L',	 0,	"Input pattern:",	"fe_pattern",	},
	{ ITX, 'T',	0x221,	" ",			"fe_pattern",	},
#endif
	{   0, '-',	 0,	" ",			0,		},

	{ BAS, 'L',	 0,	"Calendar interface:",	"fe_plan",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ TIM, 'r',	0x260,  "Date+time",		"fe_plan",	},
	{ BAS, 'r',	0x261,  "Length",		"fe_plan",	},
	{ BAS, 'r',	0x262,  "Early warn",		"fe_plan",	},
	{ BAS, 'r',	0x263,  "Late warn",		"fe_plan",	},
	{ BAS, 'r',	0x264,  "Day repeat",		"fe_plan",	},
	{ TIM, 'r',	0x265,  "End date",		"fe_plan",	},
	{ BAS, 'L',	 0,	"",			"fe_plan",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ BAS, 'r',	0x266,  "Color",		"fe_plan",	},
	{ BAS, 'r',	0x267,  "Note",			"fe_plan",	},
	{ BAS, 'r',	0x268,  "Message",		"fe_plan",	},
	{ BAS, 'r',	0x269,  "Script",		"fe_plan",	},
	{ BAS, 'r',	0x26a,  "Suspended",		"fe_plan",	},
	{ BAS, 'r',	0x26b,  "No time",		"fe_plan",	},
	{ BAS, 'r',	0x26c,  "No alarm",		"fe_plan",	},
	{ BAS, 'L',	 0,	"Shown in calendar if:","fe_plan",	},
	{ BAS, 'T',	0x228,	" ",			"fe_plan",	},
	{   0, '-',	 0,	" ",			0,		},

	{ ANY, 'L',	 0,	"Grayed out if:",	"fe_gray",	},
	{ ANY, 'T',	0x222,	" ",			"fe_gray",	},
	{ ANY, 'L',	 0,	"Invisible if:",	"fe_inv",	},
	{ ANY, 'T',	0x223,	" ",			"fe_inv",	},
	{ ANY, 'L',	 0,	"Read-only if:",	"fe_ro",	},
	{ ANY, 'T',	0x224,	" ",			"fe_ro",	},
	{ ANY, 'L',	 0,	"Skip if:",		"fe_skip",	},
	{ ANY, 'T',	0x225,	" ",			"fe_skip",	},
	{   0, '-',	 0,	" ",			0,		},

	{ BUT, 'L',	 0,	"Action when pressed:",	"fe_press",	},
	{ BUT, 'T',	0x226,	" ",			"fe_press",	},
	{ BUT, 'L',	 0,	"Action when added:",	"fe_add",	},
	{ BUT, 'T',	0x227,	" ",			"fe_add",	},
	{   0, '-',	 0,	" ",			0,		},

	{ CHA, 'L',	 0,	"Chart X range:",	"fe_chart",	},
	{ CHA, 't',	0x280,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"to",			"fe_chart",	},
	{ CHA, 't',	0x281,	" ",			"fe_chart",	},
	{ CHA, 'F',	 0, 	" ",			0,		},
	{ CHA, 'f',	0x28c,	"automatic",		"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart Y range:",	"fe_chart",	},
	{ CHA, 't',	0x282,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"to",			"fe_chart",	},
	{ CHA, 't',	0x283,	" ",			"fe_chart",	},
	{ CHA, 'F',	 0, 	" ",			0,		},
	{ CHA, 'f',	0x28d,	"automatic",		"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart XY grid every:",	"fe_chart",	},
	{ CHA, 't',	0x284,	" ",			"fe_chart",	},
	{ CHA, 't',	0x285,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart XY snap every:",	"fe_chart",	},
	{ CHA, 't',	0x286,	" ",			"fe_chart",	},
	{ CHA, 't',	0x287,	" ",			"fe_chart",	},
/*	{ CHA, 'L',	 0,	"Chart X label every:",	"fe_chart",	},
	{ CHA, 't',	0x288,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"text:",		"fe_chart",	},
	{ CHA, 'T',	0x289,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Chart Y label every:",	"fe_chart",	},
	{ CHA, 't',	0x28a,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"text:",		"fe_chart",	},
	{ CHA, 'T',	0x28b,	" ",			"fe_chart",	}, */
	{ CHA, 'L',	 0,	"Chart component:",	0,		},
	{ CHA, 'B',	0x290,	"Add",			"fe_chart",	},
	{ CHA, 'B',	0x291,	"Delete",		"fe_chart",	},
	{ CHA, 'l',	0x294,	"none",			"fe_chart",	},
	{ CHA, 'B',	0x293,	"Next",			"fe_chart",	},
	{ CHA, 'B',	0x292,	"Previous",		"fe_chart",	},

	{   0, '{',	 0,	"comps",		0,		},
	{ CHA, 'L',	 0, 	"Component flags",	"fe_chart",	},
	{ CHA, 'F',	 0, 	" ",			0,		},
	{ CHA, 'f',	0x301,	"Line",			"fe_chart",	},
	{ CHA, 'f',	0x302,	"X fat",		"fe_chart",	},
	{ CHA, 'f',	0x303,	"Y fat",		"fe_chart",	},
	{ CHA, 'L',	 0,	"Exclude if:",		"fe_chart",	},
	{ CHA, 'T',	0x304,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Color 0..7:",		"fe_chart",	},
	{ CHA, 'T',	0x305,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	"Label:",		"fe_chart",	},
	{ CHA, 'T',	0x306,	" ",			"fe_chart",	},

	{ CHA, 'L',	 0,	"X position:",		"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x310,	"next free",		"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x311,	"same as previous",	"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x312,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x314,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x313,	"drag field",		"fe_chart",	},
	{ CHA, 't',	0x315,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"multiplied by",	"fe_chart",	},
	{ CHA, 't',	0x316,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"plus",			"fe_chart",	},
	{ CHA, 't',	0x317,	" ",			"fe_chart",	},

	{ CHA, 'L',	 0,	"Y position:",		"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x320,	"next free",		"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x321,	"same as previous",	"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x322,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x324,	" ",			"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x323,	"drag field",		"fe_chart",	},
	{ CHA, 't',	0x325,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"multiplied by",	"fe_chart",	},
	{ CHA, 't',	0x326,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"plus",			"fe_chart",	},
	{ CHA, 't',	0x327,	" ",			"fe_chart",	},

	{ CHA, 'L',	 0,	"X size:",		"fe_chart",	},
/*	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x330,	"next free",		"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x331,	"same as previous",	"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	}, */
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x332,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x334,	" ",			"fe_chart",	},
/*	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x333,	"drag field",		"fe_chart",	},
	{ CHA, 't',	0x335,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"multiplied by",	"fe_chart",	},
	{ CHA, 't',	0x336,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"plus",			"fe_chart",	},
	{ CHA, 't',	0x337,	" ",			"fe_chart",	}, */

	{ CHA, 'L',	 0,	"Y size:",		"fe_chart",	},
/*	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x340,	"next free",		"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x341,	"same as previous",	"fe_chart",	},
	{ CHA, 'L',	 0,	" ",			"fe_chart",	}, */
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x342,	"expression",		"fe_chart",	},
	{ CHA, 'T',	0x344,	" ",			"fe_chart",	},
/*	{ CHA, 'L',	 0,	" ",			"fe_chart",	},
	{   0, 'R',	 0,	" ",			0,		},
	{ CHA, 'r',	0x343,	"drag field",		"fe_chart",	},
	{ CHA, 't',	0x345,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"multiplied by",	"fe_chart",	},
	{ CHA, 't',	0x346,	" ",			"fe_chart",	},
	{ CHA, 'l',	 0,	"plus",			"fe_chart",	},
	{ CHA, 't',	0x347,	" ",			"fe_chart",	}, */

	{   0, '}',	 0,	" ",			0,		},
#if 0
	{   0, '-',	 0,	" ",			0,		},

	{ VIW, 'L',	 0,	"Show query result as:","fe_result",	},
	{   0, 'F',	 0,	" ",			0,		},
	{ VIW, 'f',	0x231,	"Summary:",		"fe_result",	},
	{ VIW, 'f',	0x232,	"first card:",		"fe_result",	},
	{ VIW, 'f',	0x233,	"last card:",		"fe_result",	},
	{ VIW, 'L',	 0,	"Database:",		"fe_query",	},
	{ VIW, 'T',	0x234,	" ",			"fe_query",	},
	{ VIW, 'L',	 0,	"Query:",		"fe_query",	},
	{ VIW, 'T',	0x235,	" ",			"fe_query",	},
#endif

	{   0, ']',	 0,	" ",			0,		},
	{   0,  0,	 0,	0,			0		},
};


/*-------------------------------------------------- create window ----------*/
/*
 * destroy form edit shell and remove it from the screen. If the form being
 * edited was created by create_formedit_window(), destroy it; otherwise
 * leave it intact and display it in the main window (using switch_form()).
 */

void destroy_formedit_window(void)
{
	if (have_shell) {
		XtPopdown(shell);
		XtDestroyWidget(shell);
		have_shell = FALSE;
	}
}


/*
 * create form edit shell and all the buttons in it, but don't fill in
 * any data yet. If <def> is nonzero, edit the form <def> (the one currently
 * being displayed in the main window); if <def> is zero, start a new form.
 * If <copy> is TRUE, don't use <def> directly, create a copy by changing
 * form name to something that won't overwrite the original.
 */

void create_formedit_window(
	FORM			*def,		/* new form to edit */
	BOOL			copy,		/* use a copy of <def> */
	BOOL			new)		/* ok to change form name */
{
	struct _template	*tp;
	int			len;		/* width of first column */
	WidgetClass		class;
	String			cback;
	Widget			cform, outerform=0, innerform=0;
	Widget			scroll=0, chart=0;
	Widget			top, left, prev=0, prevline=0;
	Arg			args[30];
	long			t, n;
	Atom			closewindow;

	if (def && have_shell && def == form)		/* same as before */
		return;
	if (!def)
		form = form_create();
	else {
		form = form_clone(def);
		if (copy) {
			if (form->name)  free(form->name);
			form->name = 0;
		}
	}
	if (new && form->path) {
		free(form->path);
		form->path = 0;
	}
	create_canvas_window(form);			/* canvas window */
	if (have_shell) {
		sensitize_formedit();			/* re-use window */
		return;
	}
	n = 0;
	XtSetArg(args[n], XmNdeleteResponse,	XmDO_NOTHING);		n++;
	XtSetArg(args[n], XmNiconic,		False);			n++;
	shell = XtAppCreateShell("Form Editor", "Grok",
			applicationShellWidgetClass, display, args, n);
	set_icon(shell, 1);

	cform = XtCreateWidget("editform", xmFormWidgetClass,
			shell, NULL, 0);
	XtAddCallback(cform, XmNhelpCallback,
			(XtCallbackProc)help_callback, (XtPointer)"edit");

	for (len=0, tp=template; tp->type; tp++)
		if (tp->type == 'L') {
			n = strlen_in_pixels(tp->text, FONT_HELV_S);
			if (n > len)
				len = n;
		}
	for (t=0, tp=template; tp->type; tp++, t++) {
		n = 0;
		switch(tp->type) {
		  case ']':
			XtManageChild(cform);
			XtManageChild(scroll);
			cform = outerform;
			prev  = scroll;
			continue;
		  case '}':
			XtManageChild(cform);
			XtManageChild(chart);
			cform = innerform;
			prev  = chart;
			continue;
		  case 'L':
		  case '-':
		  case '[':
		  case '{':
			top  = prevline = prev;
			left = 0;
			break;
		  default:
			top  = prevline;
			left = prev;
		}
		if (top) {
		   XtSetArg(args[n], XmNtopAttachment,	XmATTACH_WIDGET);  n++;
		   XtSetArg(args[n], XmNtopWidget,	top);		   n++;
		} else {
		   XtSetArg(args[n], XmNtopAttachment,	XmATTACH_FORM);	   n++;
		}
		if (left) {
		   XtSetArg(args[n], XmNleftAttachment,	XmATTACH_WIDGET);  n++;
		   XtSetArg(args[n], XmNleftWidget,	left);		   n++;
		   XtSetArg(args[n], XmNleftOffset,	8);		   n++;
		} else {
		   XtSetArg(args[n], XmNleftAttachment,	XmATTACH_FORM);	   n++;
		   XtSetArg(args[n], XmNleftOffset,	8);		   n++;
		}
		if (tp->type == 'T' || tp->type == 't' || tp->type == 'L') {
		   XtSetArg(args[n], XmNalignment,  XmALIGNMENT_BEGINNING);n++;
		}
		if (tp->type == 'T' || tp->type == '-') {
		   XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM);	   n++;
		}
		if (tp->type == 'B') {
		   XtSetArg(args[n], XmNwidth,		90);		   n++;
		}
		if (tp->type == 't') {
		   XtSetArg(args[n], XmNwidth,		100);		   n++;
		   XtSetArg(args[n], XmNtopOffset,	2);		   n++;
		}
		if (tp->code == 0x101 && !new) {
		   XtSetArg(args[n], XmNrightOffset,	8);		   n++;
		   XtSetArg(args[n], XmNtopOffset,	2);		   n++;
		   XtSetArg(args[n], XmNeditable,	FALSE);		   n++;
		} else if (tp->type == 'T' || tp->type == 't') {
		   XtSetArg(args[n], XmNrightOffset,	8);		   n++;
		   XtSetArg(args[n], XmNbackground,   color[COL_TEXTBACK]);n++;
		   XtSetArg(args[n], XmNtopOffset,	2);		   n++;
		} else {
		   XtSetArg(args[n], XmNtopOffset,	6);		   n++;
		   XtSetArg(args[n], XmNhighlightThickness, 1);		   n++;
		}
		if (tp->type == 'L') {
		   XtSetArg(args[n], XmNwidth,		len+4);		   n++;
		}
		if (tp->type == 'F' || tp->type == 'R') {
		   XtSetArg(args[n], XmNpacking,	XmPACK_TIGHT);	   n++;
		   XtSetArg(args[n], XmNspacing,	4);		   n++;
		}
		if (tp->type == 'f' || tp->type == 'r') {
		   XtSetArg(args[n], XmNfillOnSelect,	True);		   n++;
		   XtSetArg(args[n], XmNselectColor,	color[COL_TOGGLE]);n++;
		   cback = XmNvalueChangedCallback;
		} else
		   cback = XmNactivateCallback;
		if (tp->type == 'f') {
		   XtSetArg(args[n], XmNindicatorType,	XmN_OF_MANY);	   n++;
		}
		if (tp->type == 'r') {
		   XtSetArg(args[n], XmNindicatorType,	XmONE_OF_MANY);	   n++;
		}
		if (tp->type == '[') {
		   XtSetArg(args[n], XmNscrollingPolicy,XmVARIABLE);	   n++;
		   XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);   n++;
		   XtSetArg(args[n], XmNbottomOffset,	8);		   n++;
		   XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM);	   n++;
		   XtSetArg(args[n], XmNrightOffset,	8);		   n++;
		   XtSetArg(args[n], XmNheight,		480);		   n++;
		}
		if (tp->type == '{') {
		   XtSetArg(args[n], XmNshadowType,	XmSHADOW_IN);	   n++;
		   XtSetArg(args[n], XmNbottomAttachment,XmATTACH_FORM);   n++;
		   XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);    n++;
		   XtSetArg(args[n], XmNrightAttachment,XmATTACH_FORM);	   n++;
		   XtSetArg(args[n], XmNrightOffset,	8);		   n++;
		   XtSetArg(args[n], XmNleftOffset,	8);		   n++;
		   XtSetArg(args[n], XmNbottomOffset,	8);		   n++;
		}
		XtSetArg(args[n],    XmNfontList, fontlist[FONT_HELV_S]);  n++;

		switch(tp->type) {
		  case '-': class = xmSeparatorWidgetClass;		break;
		  case 'L': class = xmLabelWidgetClass;			break;
		  case 'l': class = xmLabelWidgetClass;			break;
		  case 't': class = xmTextWidgetClass;			break;
		  case 'T': class = xmTextWidgetClass;			break;
		  case 'R': class = xmRowColumnWidgetClass;		break;
		  case 'F': class = xmRowColumnWidgetClass;		break;
		  case 'f': class = xmToggleButtonWidgetClass;		break;
		  case 'r': class = xmToggleButtonWidgetClass;		break;
		  case '[': class = xmScrolledWindowWidgetClass;	break;
		  case '{': class = xmFrameWidgetClass;			break;
		  case 'B': class = xmPushButtonWidgetClass;		break;
		}
		tp->widget =
		prev = XtCreateManagedWidget(tp->text, class, cform, args, n);
		if (tp->code && !strchr("RFl", tp->type))
			XtAddCallback(prev, cback, (XtCallbackProc)
					formedit_callback, (XtPointer)t);
		if (tp->help)
			XtAddCallback(prev, XmNhelpCallback, (XtCallbackProc)
					help_callback, (XtPointer)tp->help);
		if (tp->type == '[') {
			XtUnmanageChild(prev);
			outerform = cform;
			scroll = prev;
			prev   = 0;
			cform  = XtCreateWidget("escrform",
					xmFormWidgetClass, scroll, NULL, 0);
		}
		if (tp->type == '{') {
			XtUnmanageChild(prev);
			innerform = cform;
			chart  = prev;
			prev   = 0;
			cform  = XtCreateWidget("chartform",
					xmFormWidgetClass, chart, NULL, 0);
		}
	}
	XtManageChild(cform);
	sensitize_formedit();
	fillout_formedit();
	XtPopup(shell, XtGrabNone);
	closewindow = XmInternAtom(display, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, closewindow,
			(XtCallbackProc)formedit_callback, (XtPointer)0);
	have_shell = TRUE;
}


/*-------------------------------------------------- menu printing ----------*/
/*
 * sensitize only those items in the form window that are needed by the
 * current type. The procedural edit button is an exception, its
 * sensitivity is determined as a special case because it doesn't
 * depend on the item type.
 */

void sensitize_formedit(void)
{
	Arg			args;
	struct _template	*tp;
	int			mask;
	ITEM			*item;

	item = curr_item >= form->nitems ? 0 : form->items[curr_item];
	mask = 1 << (item ? item->type : IT_NULL);
	for (tp=template; tp->type; tp++) 
		if (tp->sensitive) {
			XtSetArg(args, XmNsensitive,
				tp->code == 0x106 ? form->proc :
				tp->code == 0x113 ? item != 0 :
				tp->sensitive & mask ? True : False);
			XtSetValues(tp->widget, &args, 1);
		}
}


/*
 * fillout_formedit draws all the current item's values into the form
 * definition window.
 * fillout_formedit_widget_by_code draws a single value into the form.
 * The code is searched for in the field list; see struct _template.
 * This is somewhat inefficient, an index into template[] would be
 * faster but would be guaranteed to get out of sync if template[] is
 * changed.
 * fillout_formedit_widget also draws a single field, but gets a pointer
 * to the right template[] line. The previous two routines use this.
 */

static void fillout_formedit_widget(struct _template *);

void fillout_formedit(void)
{
	struct _template	*tp;

	for (tp=template; tp->type; tp++)
		fillout_formedit_widget(tp);
}


void fillout_formedit_widget_by_code(
	int			code)
{
	struct _template	*tp;

	for (tp=template; tp->type; tp++)
		if (tp->code == code) {
			fillout_formedit_widget(tp);
			break;
		}
}


static void fillout_formedit_widget(
	struct _template	*tp)
{
	register ITEM		*item;
	register CHART		*chart;
	CHART			nullchart;
	Widget			w = tp->widget;

	if (tp->code < 0x100 || tp->code > 0x107) {
		if (!form->items || curr_item >= form->nitems ||
		    !(tp->sensitive & (1 << form->items[curr_item]->type))) {
			if (tp->type == 'T' || tp->type == 't')
				print_text_button_s(w, "");
			return;
		}
		item  = form->items[curr_item];
		chart = &item->ch_comp[item->ch_curr];
		if (!chart)
			mybzero((void *)(chart = &nullchart), sizeof(CHART));
	}
	switch(tp->code) {
	  case 0x101: print_text_button_s(w, form->name);		break;
	  case 0x102: print_text_button_s(w, form->dbase);		break;
	  case 0x107: print_text_button_s(w, form->comment);		break;
	  case 0x103: print_text_button_s(w, to_octal(form->cdelim));	break;
	  case 0x104: set_toggle(w, form->rdonly);			break;
	  case 0x114: set_toggle(w, form->syncable);			break;
	  case 0x105: set_toggle(w, form->proc);
		      fillout_formedit_widget_by_code(0x106);		break;
	  case 0x106: XtVaSetValues(w, XmNsensitive, form->proc, 0);	break;

	  case IT_LABEL:
	  case IT_PRINT:
	  case IT_INPUT:
	  case IT_TIME:
	  case IT_NOTE:
	  case IT_CHOICE:
	  case IT_FLAG:
	  case IT_BUTTON:
	  case IT_VIEW:
	  case IT_CHART:
		      set_toggle(w, item->type == tp->code);		break;

	  case 0x210:
	  case 0x211:
	  case 0x212:
	  case 0x213:
	  case 0x214: set_toggle(w, item->labelfont == tp->code-0x210);	break;

	  case 0x215:
	  case 0x216:
	  case 0x217:
	  case 0x218:
	  case 0x219: set_toggle(w, item->inputfont == tp->code-0x215);	break;

	  case 0x20d: set_toggle(w, item->labeljust == J_LEFT);		break;
	  case 0x20e: set_toggle(w, item->labeljust == J_CENTER);	break;
	  case 0x20f: set_toggle(w, item->labeljust == J_RIGHT);	break;

	  case 0x21b: set_toggle(w, item->inputjust == J_LEFT);		break;
	  case 0x21c: set_toggle(w, item->inputjust == J_CENTER);	break;
	  case 0x21d: set_toggle(w, item->inputjust == J_RIGHT);	break;

	  case 0x200: set_toggle(w, item->search);			break;
	  case 0x201: set_toggle(w, item->rdonly);			break;
	  case 0x202: set_toggle(w, item->nosort);			break;
	  case 0x203: set_toggle(w, item->defsort);			break;
	  case 0x209: set_toggle(w, item->timefmt == T_DATE);		break;
	  case 0x20a: set_toggle(w, item->timefmt == T_TIME);		break;
	  case 0x20b: set_toggle(w, item->timefmt == T_DATETIME);	break;
	  case 0x20c: set_toggle(w, item->timefmt == T_DURATION);	break;

	  case 0x260:
	  case 0x261:
	  case 0x262:
	  case 0x263:
	  case 0x264:
	  case 0x265:
	  case 0x266:
	  case 0x267:
	  case 0x268:
	  case 0x269:
	  case 0x26a:
	  case 0x26b:
	  case 0x26c: set_toggle(w, item->plan_if == plan_code[tp->code & 31]);
		      sensitize_formedit();
		      break;

	  case 0x21e: print_text_button(w, "%d", item->minlen);		break;
	  case 0x21f: print_text_button(w, "%d", item->maxlen);		break;
	  case 0x206: print_text_button(w, "%d", item->sumcol);		break;
	  case 0x207: print_text_button(w, "%d", item->sumwidth);	break;
	  case 0x205: print_text_button(w, "%d", item->column);		break;
	  case 0x204: print_text_button_s(w, item->name);		break;
	  case 0x208: print_text_button_s(w, item->flagcode);		break;
	  case 0x236: print_text_button_s(w, item->flagtext);		break;
	  case 0x21a: print_text_button_s(w, item->label);		break;
	  case 0x220: print_text_button_s(w, item->idefault);		break;
	  case 0x221: print_text_button_s(w, item->pattern);		break;

	  case 0x222: print_text_button_s(w, item->gray_if);		break;
	  case 0x223: print_text_button_s(w, item->invisible_if);	break;
	  case 0x224: print_text_button_s(w, item->freeze_if);		break;
	  case 0x225: print_text_button_s(w, item->skip_if);		break;

	  case 0x226: print_text_button_s(w, item->pressed);		break;
	  case 0x227: print_text_button_s(w, item->added);		break;
	  case 0x228: print_text_button_s(w, form->planquery);		break;

	  case 0x280: print_text_button(w, "%g", item->ch_xmin);	break;
	  case 0x281: print_text_button(w, "%g", item->ch_xmax);	break;
	  case 0x282: print_text_button(w, "%g", item->ch_ymin);	break;
	  case 0x283: print_text_button(w, "%g", item->ch_ymax);	break;
	  case 0x284: print_text_button(w, "%g", item->ch_xgrid);	break;
	  case 0x285: print_text_button(w, "%g", item->ch_ygrid);	break;
	  case 0x286: print_text_button(w, "%g", item->ch_xsnap);	break;
	  case 0x287: print_text_button(w, "%g", item->ch_ysnap);	break;
	  case 0x288: print_text_button(w, "%g", item->ch_xlabel);	break;
	  case 0x28a: print_text_button(w, "%g", item->ch_ylabel);	break;
	  case 0x289: print_text_button_s(w, item->ch_xexpr);		break;
	  case 0x28b: print_text_button_s(w, item->ch_yexpr);		break;
	  case 0x28c: set_toggle(w, item->ch_xauto);			break;
	  case 0x28d: set_toggle(w, item->ch_yauto);			break;
	  case 0x294: print_button(w, item->ch_ncomp ? "%d of %d" : "none",
				    item->ch_curr+1, item->ch_ncomp);	break;

	  case 0x301: set_toggle(w, chart->line);			break;
	  case 0x302: set_toggle(w, chart->xfat);			break;
	  case 0x303: set_toggle(w, chart->yfat);			break;
	  case 0x304: print_text_button_s(w, chart->excl_if);		break;
	  case 0x305: print_text_button_s(w, chart->color);		break;
	  case 0x306: print_text_button_s(w, chart->label);		break;

	  case 0x310: set_toggle(w, chart->value[0].mode == CC_NEXT);	break;
	  case 0x311: set_toggle(w, chart->value[0].mode == CC_SAME);	break;
	  case 0x312: set_toggle(w, chart->value[0].mode == CC_EXPR);	break;
	  case 0x313: set_toggle(w, chart->value[0].mode == CC_DRAG);	break;
	  case 0x314: print_text_button_s(w, chart->value[0].expr);	break;
	  case 0x315: print_text_button(w, "%d", chart->value[0].field);break;
	  case 0x316: print_text_button(w, "%g", chart->value[0].mul);	break;
	  case 0x317: print_text_button(w, "%g", chart->value[0].add);	break;

	  case 0x320: set_toggle(w, chart->value[1].mode == CC_NEXT);	break;
	  case 0x321: set_toggle(w, chart->value[1].mode == CC_SAME);	break;
	  case 0x322: set_toggle(w, chart->value[1].mode == CC_EXPR);	break;
	  case 0x323: set_toggle(w, chart->value[1].mode == CC_DRAG);	break;
	  case 0x324: print_text_button_s(w, chart->value[1].expr);	break;
	  case 0x325: print_text_button(w, "%d", chart->value[1].field);break;
	  case 0x326: print_text_button(w, "%g", chart->value[1].mul);	break;
	  case 0x327: print_text_button(w, "%g", chart->value[1].add);	break;

	  case 0x330: set_toggle(w, chart->value[2].mode == CC_NEXT);	break;
	  case 0x331: set_toggle(w, chart->value[2].mode == CC_SAME);	break;
	  case 0x332: set_toggle(w, chart->value[2].mode == CC_EXPR);	break;
	  case 0x333: set_toggle(w, chart->value[2].mode == CC_DRAG);	break;
	  case 0x334: print_text_button_s(w, chart->value[2].expr);	break;
	  case 0x335: print_text_button(w, "%d", chart->value[2].field);break;
	  case 0x336: print_text_button(w, "%g", chart->value[2].mul);	break;
	  case 0x337: print_text_button(w, "%g", chart->value[2].add);	break;

	  case 0x340: set_toggle(w, chart->value[3].mode == CC_NEXT);	break;
	  case 0x341: set_toggle(w, chart->value[3].mode == CC_SAME);	break;
	  case 0x342: set_toggle(w, chart->value[3].mode == CC_EXPR);	break;
	  case 0x343: set_toggle(w, chart->value[3].mode == CC_DRAG);	break;
	  case 0x344: print_text_button_s(w, chart->value[3].expr);	break;
	  case 0x345: print_text_button(w, "%d", chart->value[3].field);break;
	  case 0x346: print_text_button(w, "%g", chart->value[3].mul);	break;
	  case 0x347: print_text_button(w, "%g", chart->value[3].add);	break;

	  case 0x234: print_text_button_s(w, item->database);		break;
	  case 0x235: print_text_button_s(w, item->query);		break;
	  case 0x231: set_toggle(w, item->qsummary);			break;
	  case 0x232: set_toggle(w, item->qfirst);			break;
	  case 0x233: set_toggle(w, item->qlast);			break;
	}
}


/*-------------------------------------------------- button callbacks -------*/
/*
 * some item in one of the menu bar pulldowns was pressed. All of these
 * routines are direct X callbacks.
 */

/*ARGSUSED*/
static void formedit_callback(
	Widget				widget,
	int				indx,
	XmToggleButtonCallbackStruct	*data)
{
	switch(readback_item(indx)) {
	  case 1:
		fillout_formedit_widget(&template[indx]);
		if (form->items && curr_item < form->nitems)
			redraw_canvas_item(form->items[curr_item]);
		break;
	  case 2:
		sensitize_formedit();
		fillout_formedit();
		redraw_canvas();
	}
}


/*
 * read back all text widgets. This must be done before a new item is
 * selected or added, because we'll never see new text in text buttons
 * unless the user explicitly pressed Return on them.
 */

void readback_formedit(void)
{
	struct _template	*tp;
	int			t;

	if (curr_item < form->nitems)
		for (t=0, tp=template; tp->type; tp++, t++)
			if (tp->type == 'T' || tp->type == 't')
				(void)readback_item(t);
}


/*
 * do the operation requested by the widget: execute a function, or read
 * back a value into the form or current item. Return 0 if no canvas redraw
 * is required, 1 if the current item on the canvas must be redrawn, and 2
 * if the entire canvas must be redrawn.
 */

static void cancel_callback(void)
{
	destroy_formedit_window();
	destroy_canvas_window();
	destroy_query_window();
	form_delete(form);
	free((void *)form);
	form = 0;
}

static int readback_item(
	int			indx)
{
	struct _template	*tp = &template[indx];
	register ITEM		*item = 0, *ip;
	register CHART		*chart = 0;
	Widget			w = tp->widget;
	int			code, i;
	BOOL			all = FALSE; /* redraw oll or one? */

	if (curr_item < form->nitems) {
		item  = form->items[curr_item];
		chart = &item->ch_comp[item->ch_curr];
	}
	if (!chart && (tp->code & 0x300) == 0x300)
		return(0);

	switch(tp->code) {
	  case 0x101: (void)read_text_button_noblanks(w, &form->name);
		      if (form->name && !form->dbase) {
				form->dbase = mystrdup(form->name);
				fillout_formedit_widget_by_code(0x102);
		      }
		      break;

	  case 0x102: (void)read_text_button_noblanks(w, &form->dbase);	break;
	  case 0x107: (void)read_text_button(w, &form->comment);	break;
	  case 0x103: form->cdelim=to_ascii(read_text_button(w,0),':');	break;
	  case 0x104: form->rdonly     ^= TRUE;				break;
	  case 0x114: form->syncable   ^= TRUE;				break;
	  case 0x105: form->proc       ^= TRUE; sensitize_formedit();	break;
	  case 0x106: form_edit_script(form, w, form->dbase);		break;

	  case 0x112: readback_formedit();
		      item_deselect(form);
		      (void)item_create(form, curr_item);
		      form->items[curr_item]->selected = TRUE;
		      all = TRUE;
		      break;

	  case 0x113: item_deselect(form);
		      item_delete(form, curr_item);
		      if (curr_item >= form->nitems)
				if (curr_item)
				    curr_item--;
				else
				    item_deselect(form);
	 	      all = TRUE;
		      break;

	  case 0x10b: create_edit_popup("Card Help Editor",
					 &form->help, FALSE, "fe_help");
		      break;

	  case 0x10c: create_query_window(form);
		      break;

	  case 0x10d: if (!verify_form(form, &i, shell) && i < form->nitems) {
				item_deselect(form);
				curr_item = i;
				form->items[i]->selected = TRUE;
				redraw_canvas_item(form->items[i]);
				fillout_formedit();
				sensitize_formedit();
		      }
		      break;

	  case 0x10e: create_card_menu(form, 0, 0);
		      break;

	  case 0x10f: help_callback(shell, "edit");
	 	      return(0);

	  case 0x111: readback_formedit();
		      if (!verify_form(form, &i, shell)) {
				if (i < form->nitems) {
					item_deselect(form);
					curr_item = i;
					form->items[i]->selected = TRUE;
					redraw_canvas_item(form->items[i]);
					fillout_formedit();
					sensitize_formedit();
				}
				return(0);
		      }
	  	      destroy_formedit_window();
		      destroy_canvas_window();
		      destroy_query_window();
		      form_sort(form);
		      if (!write_form(form))
				return(0);
		      switch_form(form->name);
		      form_delete(form);
		      free((void *)form);
		      form = 0;
	 	      return(0);

	  case 0x110: create_query_popup(shell, cancel_callback,
				"form_cancel", "Ok to discard changes?");
	 	      return(0);

	  case IT_LABEL:
	  case IT_PRINT:
	  case IT_INPUT:
	  case IT_TIME:
	  case IT_NOTE:
	  case IT_CHOICE:
	  case IT_FLAG:
	  case IT_BUTTON:
	  case IT_VIEW:
	  case IT_CHART:
	 	      item->type = tp->code;
		      all = TRUE;
		      break;

	  case 0x202: item->nosort ^= TRUE;
		      if (item->name)
				for (i=0; i < form->nitems; i++) {
					ip = form->items[i];
					if (ip->name &&
					    !strcmp(item->name, ip->name))
						ip->nosort = item->nosort;
				}
		      break;

	  case 0x203: item->defsort ^= TRUE;
		      if (item->name)
				for (i=0; i < form->nitems; i++)
				    if (i != curr_item) {
					ip = form->items[i];
					ip->defsort = !strcmp(item->name,
					     ip->name) ? item->defsort : FALSE;
				}
		      break;

	  case 0x210:
	  case 0x211:
	  case 0x212:
	  case 0x213:
	  case 0x214: item->labelfont = tp->code - 0x210;
		      for (code=0x210; code <= 0x214; code++)
				fillout_formedit_widget_by_code(code);
		      all = TRUE;
		      break;

	  case 0x215:
	  case 0x216:
	  case 0x217:
	  case 0x218:
	  case 0x219: item->inputfont = tp->code - 0x215;
		      for (code=0x215; code <= 0x219; code++)
				fillout_formedit_widget_by_code(code);
		      all = TRUE;
		      break;

	  case 0x260:
	  case 0x261:
	  case 0x262:
	  case 0x263:
	  case 0x264:
	  case 0x265:
	  case 0x266:
	  case 0x267:
	  case 0x268:
	  case 0x269:
	  case 0x26a:
	  case 0x26b:
	  case 0x26c: item->plan_if = plan_code[tp->code & 31];
		      for (code=0x260; code <= 0x26c; code++)
				fillout_formedit_widget_by_code(code);
		      break;

	  case 0x20d: item->labeljust = J_LEFT;
		      fillout_formedit_widget_by_code(0x20e);
		      fillout_formedit_widget_by_code(0x20f);		break;
	  case 0x20e: item->labeljust = J_CENTER;
		      fillout_formedit_widget_by_code(0x20d);
		      fillout_formedit_widget_by_code(0x20f);		break;
	  case 0x20f: item->labeljust = J_RIGHT;
		      fillout_formedit_widget_by_code(0x20d);
		      fillout_formedit_widget_by_code(0x20e);		break;

	  case 0x21b: item->inputjust = J_LEFT;
		      fillout_formedit_widget_by_code(0x21c);
		      fillout_formedit_widget_by_code(0x21d);		break;
	  case 0x21c: item->inputjust = J_CENTER;
		      fillout_formedit_widget_by_code(0x21b);
		      fillout_formedit_widget_by_code(0x21d);		break;
	  case 0x21d: item->inputjust = J_RIGHT;
		      fillout_formedit_widget_by_code(0x21b);
		      fillout_formedit_widget_by_code(0x21c);		break;

	  case 0x200: item->search ^= TRUE;				break;
	  case 0x201: item->rdonly ^= TRUE;				break;
	  case 0x209: item->timefmt = T_DATE;		all = TRUE;	break;
	  case 0x20a: item->timefmt = T_TIME;		all = TRUE;	break;
	  case 0x20b: item->timefmt = T_DATETIME;	all = TRUE;	break;
	  case 0x20c: item->timefmt = T_DURATION;	all = TRUE;	break;

	  case 0x21e: item->minlen   = atoi(read_text_button(w, 0));	break;
	  case 0x21f: item->maxlen   = atoi(read_text_button(w, 0));	break;
	  case 0x206: item->sumcol   = atoi(read_text_button(w, 0));	break;
	  case 0x207: item->sumwidth = atoi(read_text_button(w, 0));	break;
	  case 0x205: item->column   = atoi(read_text_button(w, 0));	break;
	  case 0x204: (void)read_text_button(w, &item->name);		break;
	  case 0x208: (void)read_text_button(w, &item->flagcode);	break;
	  case 0x236: (void)read_text_button(w, &item->flagtext);	break;
	  case 0x21a: (void)read_text_button(w, &item->label);		break;
	  case 0x220: (void)read_text_button(w, &item->idefault);	break;
	  case 0x221: (void)read_text_button(w, &item->pattern);	break;

	  case 0x222: (void)read_text_button(w, &item->gray_if);	break;
	  case 0x223: (void)read_text_button(w, &item->invisible_if);	break;
	  case 0x224: (void)read_text_button(w, &item->freeze_if);	break;
	  case 0x225: (void)read_text_button(w, &item->skip_if);	break;

	  case 0x226: (void)read_text_button(w, &item->pressed);	break;
	  case 0x227: (void)read_text_button(w, &item->added);		break;
	  case 0x228: (void)read_text_button(w, &form->planquery);	break;

	  case 0x280: item->ch_xmin   = atof(read_text_button(w, 0));	break;
	  case 0x281: item->ch_xmax   = atof(read_text_button(w, 0));	break;
	  case 0x282: item->ch_ymin   = atof(read_text_button(w, 0));	break;
	  case 0x283: item->ch_ymax   = atof(read_text_button(w, 0));	break;
	  case 0x284: item->ch_xgrid  = atof(read_text_button(w, 0));	break;
	  case 0x285: item->ch_ygrid  = atof(read_text_button(w, 0));	break;
	  case 0x286: item->ch_xsnap  = atof(read_text_button(w, 0));	break;
	  case 0x287: item->ch_ysnap  = atof(read_text_button(w, 0));	break;
	  case 0x288: item->ch_xlabel = atof(read_text_button(w, 0));	break;
	  case 0x28a: item->ch_ylabel = atof(read_text_button(w, 0));	break;
	  case 0x289: (void)read_text_button(w, &item->ch_xexpr);	break;
	  case 0x28b: (void)read_text_button(w, &item->ch_yexpr);	break;
	  case 0x28c: item->ch_xauto ^= TRUE;				break;
	  case 0x28d: item->ch_yauto ^= TRUE;				break;

	  case 0x290: add_chart_component(item); all = TRUE;		break;
	  case 0x291: del_chart_component(item); all = TRUE;		break;
	  case 0x292: if (item->ch_curr) item->ch_curr--; all = TRUE;	break;
	  case 0x293: if (item->ch_curr < item->ch_ncomp-1) item->ch_curr++;
		      all = TRUE;					break;

	  case 0x301: chart->line ^= TRUE;				break;
	  case 0x302: chart->xfat ^= TRUE;				break;
	  case 0x303: chart->yfat ^= TRUE;				break;
	  case 0x304: (void)read_text_button(w, &chart->excl_if);	break;
	  case 0x305: (void)read_text_button(w, &chart->color);		break;
	  case 0x306: (void)read_text_button(w, &chart->label);		break;

	  case 0x310: chart->value[0].mode = CC_NEXT;			break;
	  case 0x311: chart->value[0].mode = CC_SAME;			break;
	  case 0x312: chart->value[0].mode = CC_EXPR;			break;
	  case 0x313: chart->value[0].mode = CC_DRAG;			break;
	  case 0x314: (void)read_text_button(w, &chart->value[0].expr);	break;
	  case 0x315: chart->value[0].field=atof(read_text_button(w,0));break;
	  case 0x316: chart->value[0].mul= atof(read_text_button(w, 0));break;
	  case 0x317: chart->value[0].add= atof(read_text_button(w, 0));break;

	  case 0x320: chart->value[1].mode = CC_NEXT;			break;
	  case 0x321: chart->value[1].mode = CC_SAME;			break;
	  case 0x322: chart->value[1].mode = CC_EXPR;			break;
	  case 0x323: chart->value[1].mode = CC_DRAG;			break;
	  case 0x324: (void)read_text_button(w, &chart->value[1].expr);	break;
	  case 0x325: chart->value[1].field=atof(read_text_button(w,0));break;
	  case 0x326: chart->value[1].mul= atof(read_text_button(w, 0));break;
	  case 0x327: chart->value[1].add= atof(read_text_button(w, 0));break;

	  case 0x330: chart->value[2].mode = CC_NEXT;			break;
	  case 0x331: chart->value[2].mode = CC_SAME;			break;
	  case 0x332: chart->value[2].mode = CC_EXPR;			break;
	  case 0x333: chart->value[2].mode = CC_DRAG;			break;
	  case 0x334: (void)read_text_button(w, &chart->value[2].expr);	break;
	  case 0x335: chart->value[2].field=atof(read_text_button(w,0));break;
	  case 0x336: chart->value[2].mul= atof(read_text_button(w, 0));break;
	  case 0x337: chart->value[2].add= atof(read_text_button(w, 0));break;

	  case 0x340: chart->value[3].mode = CC_NEXT;			break;
	  case 0x341: chart->value[3].mode = CC_SAME;			break;
	  case 0x342: chart->value[3].mode = CC_EXPR;			break;
	  case 0x343: chart->value[3].mode = CC_DRAG;			break;
	  case 0x344: (void)read_text_button(w, &chart->value[3].expr);	break;
	  case 0x345: chart->value[3].field=atof(read_text_button(w,0));break;
	  case 0x346: chart->value[3].mul= atof(read_text_button(w, 0));break;
	  case 0x347: chart->value[3].add= atof(read_text_button(w, 0));break;

	  case 0x234: (void)read_text_button(w, &item->database);	break;
	  case 0x235: (void)read_text_button(w, &item->query);		break;
	  case 0x231: item->qsummary ^= TRUE;				break;
	  case 0x232: item->qfirst   ^= TRUE;
	 	      item->qlast     = FALSE;
		      fillout_formedit_widget_by_code(0x233);		break;
	  case 0x233: item->qlast    ^= TRUE;
	 	      item->qfirst    = FALSE;
		      fillout_formedit_widget_by_code(0x232);		break;
	}

	/*
	 * the chart choice buttons are widely separated and must be handled
	 * by hand here, to make sure only one of each group is enabled.
	 */
	i = tp->code & 0x00f;
	if (tp->code >= 0x310 && tp->code <= 0x3ff && i < 4) {
		i = tp->code & 0x3f0;
		fillout_formedit_widget_by_code(i + 0);
		fillout_formedit_widget_by_code(i + 1);
		fillout_formedit_widget_by_code(i + 2);
		fillout_formedit_widget_by_code(i + 3);
	}

	/*
	 * if a choice item is modified, modify all other choice items with
	 * the same name too.
	 */
	if (item && item->name && item->type == IT_CHOICE)
		for (i=0; i < form->nitems; i++) {
			ip = form->items[i];
			if (i != curr_item && ip->type == IT_CHOICE
					   && ip->name
					   && !strcmp(ip->name, item->name)) {
				ip->column   = item->column;
				ip->search   = item->search;
				ip->rdonly   = item->rdonly;
				ip->nosort   = item->nosort;
				ip->defsort  = item->defsort;
	  			ip->sumcol   = item->sumcol;
				ip->sumwidth = item->sumwidth;
				ip->column   = item->column;
				if (item->idefault) {
					if (ip->idefault)
						free(ip->idefault);
	  				ip->idefault =mystrdup(item->idefault);
				}
				redraw_canvas_item(ip);
			}
		}
	return(all ? 2 : 1);
}
