/*
 * configuration file for grok. Try to figure out what kind of system this
 * is, and turn nonportable features on or off.
 *
 * If you have a system which runs Qt, but grok does not support,
 * file a bug at https://bitbucket.org/darktjm/grok/issues/
 */

/*---------------------------------------------------------------------------*/
/*
 * This is the user-modifiable part. Hack away.
 */

#define GROK					/* for src sharing with plan */
#define GROKDIR		"~/.grok"		/* grok working dir */
#define PREFFILE	".grokrc"		/* preference file in GROKDIR*/
#define HELP_FN		"grok.hlp"		/* help text file */
#define QSS_FN		"grok.qss"		/* global style sheet */


/*---------------------------------------------------------------------------*/
/*
 * The rest is for porting only.
 */

#ifdef __GNUC__
#define FALLTHROUGH __attribute__((fallthrough));
#define UNUSED __attribute__((unused))
/* This warning is way too much trouble to "fix" */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#else
#define FALLTHRUGH /*FALLTHROUGH*/
#define UNUSED
#endif
