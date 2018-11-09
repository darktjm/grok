/*
 * execute a template. Perform all embedded expressions and substitutions.
 *
 *	eval_template()
 */

#include "config.h"
#include <sys/types.h>
#ifdef DIRECT
#include <sys/dir.h>
#define  dirent direct
#else
#include <dirent.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h> /* mkdir */
#include <errno.h>
#include <QtWidgets>
#include "grok.h"
#include "form.h"
#include "proto.h"


/*****************************************************************************
 **			find and exec templates				    **
 *****************************************************************************/

#define NBUILTINS	3		/* number of builtins (with modes) */
#define MAXFILES	128		/* max # of user-defined templates */

static int	ntemps;			/* # of valid user-defined templates */
static char	*templates[MAXFILES];	/* list of user-defd template names */

static struct builtins {
	const char	*name;
	const char	*desc;
	const char	*(*func)(char *, int);
	int		mode;
} builtins[NBUILTINS] = {
	{ "html",	"  (builtin HTML)",		mktemplate_html, 0 },
	{ "html-s",	"(builtin HTML, summary only)",	mktemplate_html, 1 },
	{ "html-d",	"(builtin HTML, data only)",	mktemplate_html, 2 }
};

static void *allocate(int n)
	{void *p=malloc(n); if (!p) fatal("no memory"); return(p);}

int get_template_nbuiltins(void) { return(NBUILTINS); }


/*
 * return complete path for a template name, or sequential number. The name
 * is allocated, free it afterwards. Create the <formname>.tm directory if
 * it doesn't exist, just in case.
 */

char *get_template_path(
	const char	*name,		/* template name or 0 */
	int		seq,		/* if name==0, sequential number */
	CARD		*card)		/* need this for form name */
{
	char		*path, *p;	/* returned path */

	if (!name)
		name = seq < NBUILTINS ? builtins[seq].name
				       : templates[seq - NBUILTINS];
	path = (char *)allocate(strlen(card->form->path) + 1 + strlen(name) + 1);
	strcpy(path, card->form->path);
	if (p = strrchr(path, '.'))
		strcpy(p, ".tm");
	if (access(path, F_OK))
		mkdir(path, 0777);
	strcat(path, "/");
	strcat(path, name);
	return(path);
}


/*
 * call a callback once for every available template, with a descriptive name.
 */

void list_templates(
	void	(*func)(int, char *),	/* callback with seq# and name */
	CARD		*card)		/* need this for form name */
{
	DIR		*dir;		/* open directory file */
	struct dirent	*dp;		/* one directory entry */
	int		seq;		/* template counter */
	char		*path;		/* directory with templates */
	char		buf[64];	/* temp name */

	for (seq=0; seq < MAXFILES; seq++)
		if (templates[seq]) {
			free(templates[seq]);
			templates[seq] = 0;
		}
	for (seq=0; seq < NBUILTINS; seq++) {
		sprintf(buf, "%s  %s", builtins[seq].name,
				       builtins[seq].desc);
		if (func)
			(*func)(seq, buf);
	}
	path = get_template_path("", 0, card);
	dir = opendir(path);
	free(path);
	if (!dir)
		return;
	ntemps = 0;
	while ((dp = readdir(dir)) && ntemps < MAXFILES)
		if (*dp->d_name != '.') {
			templates[ntemps++] = mystrdup(dp->d_name);
			if (func)
				(*func)(ntemps + NBUILTINS, dp->d_name);
		}
	(void)closedir(dir);
}


/*
 * got a name from -x option or from template list in export dialog. Figure
 * out which template is meant and execute it.
 */

const char *exec_template(
	char		*oname,		/* output file name, 0=stdout */
	const char	*name,		/* template name to execute */
	int		seq,		/* if name is 0, execute by seq num */
	CARD		*card)		/* need this for form name */
{
	char		tmp[1024];	/* temp file for builtin template */
	char		*path;		/* template file path */
	const char	*p;		/* template file path */
	const char	*ret;		/* returned error message or 0 */

	if (!ntemps)
		list_templates(0, card);
	if (name) {
		for (seq=0; seq < NBUILTINS; seq++)
			if (!strcmp(name, builtins[seq].name))
				break;
		if (seq == NBUILTINS)
			for (; seq < ntemps + NBUILTINS; seq++)
				if (!strcmp(name, templates[seq - NBUILTINS]))
					break;
	}
	if (seq >= NBUILTINS + ntemps)
		return("no such template");
	if (seq < NBUILTINS) {
		if (!(p = getenv("TMPDIR")))
			p = "/usr/tmp";
		strncpy(tmp, p, sizeof(tmp)-32);
		sprintf(tmp+strlen(tmp), "/groktm%d", getpid());
		unlink(tmp);
		if (!(ret = (*builtins[seq].func)(tmp, builtins[seq].mode)))
			ret = eval_template(tmp, oname);
		unlink(tmp);
	} else {
		path = get_template_path(templates[seq - NBUILTINS], 0, card);
		ret = eval_template(path, oname);
		free(path);
	}
	return(ret);
}


/*
 * copy a numbered template to a given template name, and return the complete
 * (allocated) path for the new template name. If the source is a builtin,
 * create a template. If an error occurs, return 0.
 */

char *copy_template(
	QWidget		*shell,		/* export window widget */
	char		*tar,		/* target template name */
	int		seq,		/* source template number */
	CARD		*card)		/* need this for form name */
{
	char		*src;		/* source template name */
	const char	*err = 0;	/* error message */
	FILE		*ifp=0, *ofp=0;	/* source and target files */
	int		c;		/* for file copying */

	tar = get_template_path(tar, 0, card);
	if (seq < NBUILTINS) {
		if (err = (*builtins[seq].func)(tar, builtins[seq].mode))
			create_error_popup(shell, 0,
					"Failed to create %s:\n%s", tar, err);
	} else {
		src = get_template_path(0, seq, card);
		if (!(ifp = fopen(src, "r")))
			create_error_popup(shell, errno,
					err = "Failed to open %s", src);
		else if (!(ofp = fopen(tar, "w")))
			create_error_popup(shell, errno,
					err = "Failed to create %s", tar);
		else
			while ((c = fgetc(ifp)) != EOF)
				fputc(c, ofp);
		if (ifp) fclose(ifp);
		if (ofp) fclose(ofp);
		free(src);
	}
	if (err) free(tar);
	return(err ? 0 : tar);
}


/*
 * delete a template by sequential number
 */

BOOL delete_template(
	QWidget		*shell,		/* export window widget */
	int		seq,		/* template to delete, >= NBUILTINS */
	CARD		*card)		/* need this for form name */
{
	char		*path;

	if (seq < NBUILTINS) {
		create_error_popup(shell,0,"Cannot delete a builtin template");
		return(FALSE);
	}
	path = get_template_path(0, seq, card);
	if (unlink(path)) {
		create_error_popup(shell, errno, "Failed to delete %s", path);
		free(path);
		return(FALSE);
	}
	free(path);
	return(TRUE);
}


/*****************************************************************************
 **				parse template				    **
 *****************************************************************************/

#define ISSPACE(c) ((c)==' ' || (c)=='\t')
#define ISOCTAL(c) ((c)>='0' && (c)<='7')
#define NEST		10

static char html_subst[] = "<=&lt; >=&gt; &=&amp; \n=<BR>";
static const char *eval_command(char *, BOOL *);
static void backslash_subst(char *);
static const char *putstring(const char *);

struct forstack { long offset; int num; int nquery; int *query; };

enum opcode { O_EXPR, O_IF, O_ENDIF, O_FOREACH,
	      O_END, O_QUIT, O_FILE, O_SUBST };
static const struct { enum opcode opcode; const char *name; } opcode_list[] = {
	{ O_IF,		"IF",		},
	{ O_ENDIF,	"ENDIF"		},
	{ O_FOREACH,	"FOREACH"	},
	{ O_END,	"END"		},
	{ O_QUIT,	"QUIT"		},
	{ O_FILE,	"FILE"		},
	{ O_SUBST,	"SUBST"		},
	{ O_EXPR,	0		}
};

static FILE		*ofp;		/* output file */
static char		*outname;	/* current output filename, 0=stdout */
static FILE		*ifp;		/* template file */
static char		*subst[256];	/* current character substitutions */
static int		*default_query;	/* default query from -x option or 0 */
static int		default_nquery;	/* number of rows in default_query */
static int		default_row;	/* row to use outside foreach loops */
static int		n_true_if;	/* number of true nested \{IF}'s */
static int		n_false_if;	/* number of false nested \{IF}'s */
static struct forstack	forstack[NEST];	/* context for current foreach loop */
static int		forlevel;	/* forstack index, -1=not in a loop */
static int		forskip;	/* # of empty loops, skip to END */



/*
 * evaluate a template and print its results to an output file or stdout.
 * Return 0 if all went well, or an error string.
 */

const char *eval_template(
	const char	*iname,		/* template filename */
	char		*oname)		/* default output filename, 0=stdout */
{
	char		word[4096];	/* command string in \{ } */
	const char	*p;
	int		indx = 0;	/* next free char in word[] */
	int		line = 1;	/* line number in template */
	int		c, prevc;	/* curr end prev char from template */
	int		bracelevel = 0;	/* if inside \{ }, brace level > 0 */
	BOOL		quote = FALSE;	/* inside \{ }: inside quoted text? */
	BOOL		eat_nl = FALSE;	/* ignore \n after \{COMMAND} */
	int		i;

	default_row   = curr_card->row;
	default_query = 0;
	if (default_nquery = curr_card->nquery) {
		default_query = (int *)allocate(default_nquery * sizeof(int));
		memcpy(default_query, curr_card->query,
					 default_nquery * sizeof(int));
	}
	n_true_if = n_false_if = forskip = 0;
	forlevel = -1;
	outname = oname ? mystrdup(resolve_tilde(oname, 0)) : 0;
	ofp = 0;

	if (!(ifp = fopen(iname, "r")))
		return("failed to open template file");
	for (prevc=0;; prevc=c) {
		c = fgetc(ifp);
		if (feof(ifp)) {
			putstring("\n");
			*word = 0;
			break;
		}
		if (c == '\n')
			line++;
		if (bracelevel) {
			if (prevc != '\\') {
				quote ^= c == '"';
				if (!quote) {
					bracelevel += c == '{';
					bracelevel -= c == '}';
				}
			}
			if (!bracelevel) {
				word[indx] = 0;
				if (p = eval_command(word, &eat_nl)) {
					sprintf(word, "%s line %d: %s",
							iname, line, p);
					break;
				}
			} else {
				if (indx == sizeof(word)-1) {
					sprintf(word, 
						"%s line %d: command too long",
						iname, line);
					break;
				}
				word[indx++] = c;
			}
		} else {
			word[0] = 0;
			if (prevc == '\\') {
				if (c == '{') {
					indx = 0;
					bracelevel++;
				} else
					word[0] = c;
				c = 0;
			} else if (c != '\\')
				word[0] = c;
			if (!n_false_if && !forskip && word[0] &&
						(word[0] != '\n' || !eat_nl)) {
				word[1] = 0;
				if (p = putstring(word)) {
					sprintf(word, "%s line %d: %s",
							iname, line, p);
					break;
				}
			}
			eat_nl = FALSE;
		}
	}
	if (feof(ifp))
		if (n_true_if || n_false_if)
			sprintf(word, "%s EOF: %d unterminated IF",
					iname, n_true_if + n_false_if);
		else if (forlevel > -1 || forskip)
			sprintf(word, "%s EOF: unterminated FOREACH",
					iname);
	fclose(ifp);
	if (ofp)
		fclose(ofp);
	if (outname)
		free(outname);
	while (forlevel >= 0)
		free(forstack[forlevel--].query);

	if (curr_card->query)
		free(curr_card->query);
	curr_card->query  = default_query;
	curr_card->nquery = default_nquery;
	curr_card->row    = default_row;

	for (i=0; i < 256; i++)
		if (subst[i]) {
			free(subst[i]);
			subst[i] = 0;
		}
	return(*word ? mystrdup(word) : 0);
}


/*
 * evaluate a \{ ... } command. Template commands are evaluated here; every-
 * thing else is passed to the normal expression evaluator and its results
 * are printed. Return 0 if all went well, or an error message string.
 */

static const char *eval_command(
	char		*word,		/* command to evaluate */
	BOOL		*eat_nl)	/* if command, set to true (skip \n) */
{
	char		cmd[256];	/* extracted command word */
	char		*p, *q;		/* for searching the command */
	const char	*pc, *err;
	struct forstack	*sp;		/* current foreach stack level */
	int		i, nq, *qu;

	*eat_nl = TRUE;
	while (ISSPACE(*word)) word++;
	for (i=strlen(word); i && ISSPACE(word[i-1]); i--);
	for (p=word, q=cmd; q < cmd+255 && *p>='A' && *p<='Z'; *q++ = *p++);
	*q = 0;
	while (ISSPACE(*p)) p++;

	for (i=0; opcode_list[i].name; i++)
		if (!strcmp(opcode_list[i].name, cmd))
			break;
	switch(opcode_list[i].opcode) {
	  case O_IF:
		if (n_false_if) {
			n_false_if++;
			return(0);
		}
		if (evalbool(curr_card, p))
			n_true_if++;
		else
			n_false_if++;
		break;

	  case O_ENDIF:
		if (n_false_if)
			n_false_if--;
		else if (n_true_if)
			n_true_if--;
		else
			return("ENDIF without IF");
		break;

	  case O_FOREACH:
		if (forskip)
			forskip++;
		if (forskip || n_false_if)
			break;
		if (forlevel == NEST-1)
			return("FOREACH nested too deep");

		if (*p) {
			query_any(SM_SEARCH, curr_card, p);
			nq = curr_card->nquery;
			qu = curr_card->query;
		} else {
			nq = default_nquery;
			qu = default_query;
		}
		if (curr_card->nquery) {
			sp = &forstack[++forlevel];
			sp->offset = ftell(ifp);
			sp->num    = 0;
			sp->nquery = nq;
			sp->query  = (int *)allocate(nq * sizeof(int));
			memcpy(sp->query, qu, nq * sizeof(int));
			curr_card->row = sp->query[sp->num];
		} else
			forskip++;
		break;

	  case O_END:
		if (n_false_if)
			break;
		if (forskip) {
			forskip--;
			break;
		}
		if (forlevel < 0)
			return("END without FOREACH");
		sp = &forstack[forlevel];
		if (++sp->num < sp->nquery) {
			curr_card->row = sp->query[sp->num];
			fseek(ifp, sp->offset, SEEK_SET);
		} else {
			free(sp->query);
			forlevel--;
		}
		break;

	  case O_FILE:
		if (forskip || n_false_if)
			break;
		if (ofp)
			fclose(ofp);
		if (outname)
			free(outname);
		outname = mystrdup(evaluate(curr_card, p));
		ofp = 0;
		break;

	  case O_SUBST:
		if (forskip || n_false_if)
			break;
		/* (char *) cast is safe because no \-sust is needed */
		substitute_setup(subst, !strcmp(p, "HTML") ? (char *)html_subst : p);
		break;

	  case O_EXPR:
		*eat_nl = FALSE;
		if (forskip || n_false_if)
			break;
		*--p = '{';
		strcat(p, "}");
		pc = evaluate(curr_card, p);
		cmd[1] = 0;
		if(!pc)
			return "error in expression";
		while (cmd[0] = *pc++)
			if (err = putstring(subst[(unsigned char)cmd[0]] ? subst[(unsigned char)cmd[0]] : cmd))
				return(err);
	}
	return(0);
}


/*
 * Set up substitutions by parsing the "x=y x=y ..." substitution string into
 * an array of replacement strings, one per character code 0..255. This is used
 * by the SUBST command above, and the tr command in the regular grammar.
 * Return an error message or 0 if all went well.
 */

const char *substitute_setup(
	char		**array,	/* where to store substitutions */
	char		*cmd)		/* x=y x=y ... command string */
{
	int		i;
	char		*q;
	
	backslash_subst(cmd);
	while (*cmd) {
	        char endc;
		// tjm - FIXME:  if 1st char is =, it will go too far
		// In fact, this should only work if at least 1 ws has been
		// skipped
		if (*cmd == '=' && cmd[1] != '=')
			cmd--;
		i = *cmd++;
		if (*cmd++ != '=')
			return("malformed substition, expected"
				      " <char>=<string> list");
		for (q=cmd; q < cmd+255 && *q && !ISSPACE(*q); q++);
		endc = *q;
		*q = 0;
		if (array[i])
			free(subst[i]);
		array[i] = *cmd ? mystrdup(cmd) : 0;
		if(endc) {
			cmd = q+1;
			while (ISSPACE(*cmd)) cmd++;
		} else
		    cmd = q;
	}
	return(0);
}


/*
 * replace all \... in text with their ascii equivalents. Recognizes \octal,
 * \n, \r, \t.
 */

void backslash_subst(
	char		*text)		/* text to replace */
{
	char		*q;		/* result pointer */
	int		i;

	for (q=text; *text; text++)
		if (*text == '\\')
			if (ISOCTAL(text[1])) {
				i = 0;
				do i = i*8 + *++text - '0';
				while (ISOCTAL(text[1]));
				*q++ = i;
			} else
				switch(*++text) {
				  case 'n': *q++ = '\n'; break;
				  case 'r': *q++ = '\r'; break;
				  case 't': *q++ = '\t'; break;
				  default:  *q++ = *text;
				}
		else
			*q++ = *text;
	*q = 0;
}


/*
 * send a string to the export file. If it isn't open yet, open it now (this
 * lets the template override the output file name).
 */

static const char *putstring(
	const char	*text)		/* string to export */
{
	if (!ofp)
		if (!outname)
			ofp = stdout;
		else if (!(ofp = fopen(outname, "w")))
			return("Failed to create export file");
	fputs(text, ofp);
	return(0);
}
