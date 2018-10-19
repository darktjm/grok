/*
 * configuration file for grok. Try to figure out what kind of system this
 * is, and turn nonportable features on or off. Recognized systems are:
 *
 *   SGI IRIX		if "sgi" is defined
 *   HP/UX 9.x		if "__hpux" is defined
 *   IBM AIX		if "__TIMESTAMP__" is defined (there is nothing better)
 *   MIPS Magnum	if "mips" is defined and "sgi" is not (this is a guess)
 *   DEC OSF/1		if "osf" is defined (this is a guess)
 *   BSDI 386		if "bsdi" is defined (this is a guess)
 *   ULTRIX		if "ultrix" is defined (this is a guess)
 *
 * If you have one of the systems for which I don't know the distinguishing
 * predefined symbol (lines that say "this is a guess"), please send mail to
 * thomas@bitrot.de telling me how you fixed it! Ask me for a small shell
 * script that dumps all predefined symbols on your system's cpp (not included
 * because it is copylefted).
 *
 * Configurable parameters are:
 *
 *   MYCONST		either "const" or "" (the latter is for Ultrix)
 *   FIXMBAR		on some systems, pulldowns always come up empty.
 *			Define this to enable a weird hack that fixes this.
 *   NOMSEP		remove all separator lines in pulldowns. Surprisingly
 *			many Motif implementations have trouble with this.
 *			Defining this fixes empty or truncated pulldowns
 *   DIRECT		for BSD systems that use the older "direct" package,
 *			if /usr/include/dirent.h does not exist
 */

/*---------------------------------------------------------------------------*/
/*
 * This is the user-modifiable part. Hack away.
 */

#define GROK					/* for src sharing with plan */
#define GROKDIR		"~/.grok"		/* grok working dir */
#define PREFFILE	".grokrc"		/* preference file in GROKDIR*/
#define HELP_FN		"grok.hlp"		/* help text file */
#define	PSPOOL_A	"lp"			/* default ascii spool string*/
#define	PSPOOL_P	"lp"			/* default PostScript spool s*/


/*---------------------------------------------------------------------------*/
/*
 * The rest is for porting only.
 */

#if !defined(sgi)
#define NOMSEP		1
#endif

#if defined(osf) || defined(bsdi)
#define DIRECT	1
#endif

#ifdef ultrix
#define MYCONST			/* use this if cc doesn't know "const" */
#else
#define MYCONST		const
#endif

#ifdef FIXMBAR
#define FIX_MENUBAR XmVaSEPARATOR,
#else
#define FIX_MENUBAR
#endif

#ifdef NOMSEP
#define XM_VA_SEPARATOR
#else
#define XM_VA_SEPARATOR XmVaSEPARATOR,
#endif
