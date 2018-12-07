/*
 * execute a template. Perform all embedded expressions and substitutions.
 *
 *	eval_template()
 */

#include "config.h"
#include <sys/types.h>
#include <dirent.h>
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

#define NBUILTINS	ALEN(builtins)	/* number of builtins (with modes) */
#define MAXFILES	128		/* max # of user-defined templates */

static int	ntemps;			/* # of valid user-defined templates */
static char	*templates[MAXFILES];	/* list of user-defd template names */

static struct builtins {
	const char	*name;
	const char	*desc;
	const char	*(*func)(FILE *);
} builtins[] = {
	{ "html",	"(auto HTML)",		mktemplate_html },
	{ "text",	"(auto text)",		mktemplate_plain },
	{ "fancytext",	"(auto fancy text)",	mktemplate_fancy },
};

static void *allocate(int n)
	{void *p=malloc(n); if (!p) fatal("no memory"); return(p);}

int get_template_nbuiltins(void) { return(NBUILTINS); }

static const char *eval_template(
	FILE		*ifp,		/* template file */
	const char	*iname,		/* template name */
	int		flags,		/* flags a..z */
	FILE		*ofp,		/* output file, if already open */
	char		*oname);	/* default output filename, 0=stdout */


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
	if ((p = strrchr(path, '.')))
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
	FILE		*ofp,		/* output file descriptor if already open */
	const char	*name,		/* template name to execute */
	int		seq,		/* if name is 0, execute by seq num */
	int		flags,		/* flags a..z */
	CARD		*card)		/* need this for form name */
{
	FILE		*fp;		/* template file (input) */
	char		*path;		/* template file path */
	const char	*ret;		/* returned error message or 0 */

	if (!ntemps)
		list_templates(0, card);
	if (name) {
		int len = strlen(name);
		while(1) {
			for (seq=0; seq < NBUILTINS; seq++)
				if (!memcmp(name, builtins[seq].name, len) &&
				    !builtins[seq].name[len])
					break;
			if (seq == NBUILTINS)
				for (; seq < ntemps + NBUILTINS; seq++)
					if (!memcmp(name, templates[seq - NBUILTINS], len) &&
					    !templates[seq - NBUILTINS][len])
						break;
			if(seq < ntemps + NBUILTINS)
				break;
			/* stdin only possible in non-interactive mode */
			if(!app && len == 1 && name[0] == '+') {
				/* It'd be nice, but FOREACH seeks */
				// return eval_template(stdin, "stdin", flags, ofp, oname);
				static char buf[1024];
				int n;
				fp = tmpfile();
				if(!fp)
					return "Can't open temporary file for template output";
				while(1) {
					n = fread(buf, 1, sizeof(buf), stdin);
					// FIXME: retry on EAGAIN or EINTR
					if(!n) // FIXME: report errors
						break;
					fwrite(buf, n, 1, fp);
				}
				fflush(fp); // FIXME: report errors
				rewind(fp);
				return eval_template(fp, "stdin", flags, ofp, oname);
			}
			if(len < 2 || name[len - 2] != '-' ||
			   name[len - 1] < 'a' || name[len - 1] > 'z')
				break;
			flags |= 1<<(name[len -1] - 'a');
			len -= 2;
		}
	}
	if (seq >= NBUILTINS + ntemps)
		return("no such template");
	if (seq < NBUILTINS) {
		fp = tmpfile();
		if(!fp)
			return "Can't open temporary file for template output";
		if (!(ret = (*builtins[seq].func)(fp))) {
			rewind(fp);
			ret = eval_template(fp, builtins[seq].name, flags, ofp, oname);
		} else
			fclose(fp);
	} else {
		path = get_template_path(templates[seq - NBUILTINS], 0, card);
		if (!(fp = fopen(path, "r")))
			ret = "failed to open template file";
		else
			ret = eval_template(fp, path, flags, ofp, oname);
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
		if (!(ofp = fopen(tar, "w")))
			create_error_popup(shell, errno,
					   err = "Failed to create %s", tar);
		else {
			if ((err = (*builtins[seq].func)(ofp)))
				create_error_popup(shell, 0,
						   "Failed to create %s:\n%s", tar, err);
			fclose(ofp);
		}
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

#define ISSPACE(c) ((c)==' ' || (c)=='\t' || (c)=='\n')
#define ISOCTAL(c) ((c)>='0' && (c)<='7')
#define NEST		10

static char html_subst[] = "<=&lt; >=&gt; &=&amp; \n=<BR>";
static const char *eval_command(char *, BOOL *, int);
static const char *putstring(const char *);

struct forstack { long offset; int num; int nquery; int *query; char *array; };

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
static bool		close_ofp;	/* close output file? */
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
	FILE		*iifp,		/* template file */
	const char	*iname,		/* template name */
	int		flags,		/* flags a..z */
	FILE		*iofp,		/* output file, if already open */
	char		*oname)		/* default output filename, 0=stdout */
{
	char		*word;		/* command string in \{ } */
	int		wordlen;
	const char	*p;
	int		indx = 0;	/* next free char in word[] */
	int		line = 1;	/* line number in template */
	int		c, prevc;	/* curr end prev char from template */
	int		bracelevel = 0;	/* if inside \{ }, brace level > 0 */
	BOOL		quote = FALSE;	/* inside \{ }: inside quoted text? */
	BOOL		eat_nl = FALSE;	/* ignore \n after \{COMMAND} */
	int		i;

	ifp = iifp;
	default_row   = curr_card->row;
	default_query = 0;
	if ((default_nquery = curr_card->nquery)) {
		default_query = (int *)allocate(default_nquery * sizeof(int));
		memcpy(default_query, curr_card->query,
					 default_nquery * sizeof(int));
	}
	n_true_if = n_false_if = forskip = 0;
	forlevel = -1;
	outname = oname ? mystrdup(resolve_tilde(oname, 0)) : 0;
	ofp = iofp;
	close_ofp = !iofp;

	word = (char *)malloc((wordlen = 32));
	for (prevc=0;; prevc=c) {
		c = fgetc(ifp);
		if (feof(ifp)) {
			// I fail to see the need for excess newlines
			// putstring("\n");
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
				if ((p = eval_command(word+1, &eat_nl, flags))) {
					if (!strcmp(p, "QUIT"))
						*word = 0;
					else {
						int len = strlen(iname) + strlen(p) + 20;
						if(len > wordlen)
							word = (char *)realloc(word, (wordlen = len));
						sprintf(word, "%s line %d: %s",
							iname, line, p);
					}
					break;
				}
			} else {
				/* + 1 for terminating 0, + 1 for EXPR's terminating brace */
				if (indx == wordlen - 2)
					word = (char *)realloc(word, (wordlen *= 2));
				word[indx++] = c;
			}
		} else {
			word[0] = 0;
			if (prevc == '\\') {
				word[0] = c;
				if (c == '{') {
					indx = 1;
					bracelevel++;
				}
				c = 0;
			} else if (c != '\\')
				word[0] = c;
			if (!n_false_if && !forskip && !bracelevel &&
						(word[0] != '\n' || !eat_nl)) {
				word[1] = 0;
				if ((p = putstring(word))) {
					sprintf(word, "%s line %d: %s",
							iname, line, p);
					break;
				}
			}
			eat_nl = FALSE;
		}
	}
	if (feof(ifp)) {
		if (n_true_if || n_false_if)
			sprintf(word, "%s EOF: %d unterminated IF",
					iname, n_true_if + n_false_if);
		else if (forlevel > -1 || forskip)
			sprintf(word, "%s EOF: unterminated FOREACH",
					iname);
	}
	fclose(ifp);
	if (ofp && close_ofp)
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
	if (*word)
		return word;
	else {
		free(word);
		return NULL;
	}
}


/*
 * evaluate a \{ ... } command. Template commands are evaluated here; every-
 * thing else is passed to the normal expression evaluator and its results
 * are printed. Return 0 if all went well, or an error message string.
 */

static const char *eval_command(
	char		*word,		/* command to evaluate */
	BOOL		*eat_nl,	/* if command, set to true (skip \n) */
	int		flags)		/* template flags a..z */
{
	char		*cmd;		/* extracted command word */
	const char	*pc, *err;
	struct forstack	*sp;		/* current foreach stack level */
	int		i, nq, *qu;

	*eat_nl = TRUE;
	while (ISSPACE(*word)) word++;
	for (i=strlen(word); i && ISSPACE(word[i-1]); i--);
	for (cmd=word; *word>='A' && *word<='Z'; word++);
	for (i=0; opcode_list[i].name; i++)
		if (!memcmp(opcode_list[i].name, cmd, word - cmd) &&
		    !opcode_list[i].name[word - cmd])
			break;
	while (ISSPACE(*word)) word++;
	switch(opcode_list[i].opcode) {
	  case O_IF:
		if (n_false_if) {
			n_false_if++;
			return(0);
		}
		if ((*word == '-' || *word == '+') && word[1] >= 'a' && word[1] <= 'z' && !word[2]) {
			if(!!(flags & (1<<(word[1] - 'a'))) == (*word == '-'))
				n_true_if++;
			else
				n_false_if++;
		} else {
			if (evalbool(curr_card, word))
				n_true_if++;
			else
				n_false_if++;
		}
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

		if (*word == '[') {
			/* array foreach is not like the others.. */
			word++;
		        while (ISSPACE(*word)) word++;
			bool nonblank = *word == '+';
			if(nonblank) {
				word++;
				while (ISSPACE(*word)) word++;
			}
			if(!isalpha(*word))
				return "Array FOREACH must specify variable";
			sp = &forstack[++forlevel];
			sp->offset = ftell(ifp);
			/* nquery -> variable + 1 */
			/*           or -(variable + 1) if nonblank */
			sp->nquery = *word >= 'a' ? *word - 'a' : *word + 26 - 'A';
			if(nonblank)
				sp->nquery = -(sp->nquery + 1);
			else
				sp->nquery++;
			sp->query  = NULL;
			word++;
		        while (ISSPACE(*word)) word++;
			sp->array = mystrdup(evaluate(curr_card, word));
			{
				char sep, esc;
				get_form_arraysep(curr_card->form, &sep, &esc);
				int begin, after = -1;
				do {
					next_aelt(sp->array, &begin, &after, sep, esc);
				} while(after >= 0 && begin == after &&
					sp->nquery < 0);
				if(after < 0)
					forskip++;
				else {
					int var = (sp->nquery < 0 ? -sp->nquery : sp->nquery) - 1;
					char c = sp->array[after];
					*unescape(sp->array + begin, sp->array + begin, after - begin, esc) = 0;
					set_var(var, strdup(sp->array + begin));
					// no need to re-escape and re-store value
					sp->array[after] = c;
				}
				sp->num = after;
			}
			break;
		}
		if (*word) {
			query_any(SM_SEARCH, curr_card, word);
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
		if(!sp->query) {
			char sep, esc;
			get_form_arraysep(curr_card->form, &sep, &esc);
			int begin, after = sp->num;
			do {
				next_aelt(sp->array, &begin, &after, sep, esc);
			} while(after >= 0 && begin == after &&
				sp->nquery < 0);
			if(after < 0) {
				if(sp->array)
					free(sp->array);
				forlevel--;
			} else {
				int var = (sp->nquery < 0 ? -sp->nquery : sp->nquery) - 1;
				char c = sp->array[after];
				*unescape(sp->array + begin, sp->array + begin, after - begin, esc) = 0;
				set_var(var, strdup(sp->array + begin));
				// no need to re-escape and re-store value
				sp->array[after] = c;
				fseek(ifp, sp->offset, SEEK_SET);
				sp->num = after;
			}
			break;
		}
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
		if (ofp && close_ofp)
			fclose(ofp);
		if (outname)
			free(outname);
		outname = mystrdup(evaluate(curr_card, word));
		ofp = 0;
		close_ofp = true;
		break;

	  case O_SUBST:
		if (forskip || n_false_if)
			break;
		/* (char *) cast is safe because no \-sust is needed */
		substitute_setup(subst, !strcmp(word, "HTML") ? (char *)html_subst : word);
		break;

	  case O_EXPR:
		*eat_nl = FALSE;
		if (forskip || n_false_if)
			break;
		if(!*word) // Allow newline suppression with blank expr
			break;
		/* word is now guaranteed to have space before & after */
		/* I have no idea why this never crashed before */
		*--word = '{';
		strcat(word, "}");

		pc = evaluate(curr_card, word);
		if(!pc)
			return "error in expression";
		cmd[1] = 0;
		while ((cmd[0] = *pc++))
			if ((err = putstring(subst[(unsigned char)cmd[0]] ? subst[(unsigned char)cmd[0]] : cmd)))
				return(err);
		break;

	  case O_QUIT:
		if (!forskip && !n_false_if)
			return "QUIT";
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
	if (!ofp) {
		if (!outname)
			ofp = stdout;
		else if (!(ofp = fopen(outname, "w")))
			return("Failed to create export file");
	}
	fputs(text, ofp);
	return(0);
}
