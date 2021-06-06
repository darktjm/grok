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
	const char	*(*func)(const CARD*, FILE *);
} builtins[] = {
	{ "html",	"(auto HTML)",		mktemplate_html },
	{ "text",	"(auto text)",		mktemplate_plain },
	{ "fancytext",	"(auto fancy text)",	mktemplate_fancy },
	{ "sql",	"(auto SQL)",		mktemplate_sql }
};

int get_template_nbuiltins(void) { return(NBUILTINS); }

static const char *eval_template(
	CARD		*card,
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
	path = alloc(0, "template path", char, strlen(card->form->path) + 1 + strlen(name) + 1);
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
	static char	*buf = 0;	/* temp name */
	static size_t	buflen = 0;

	for (seq=0; seq < MAXFILES; seq++)
		if (templates[seq]) {
			free(templates[seq]);
			templates[seq] = 0;
		}
	for (seq=0; seq < NBUILTINS; seq++) {
		if(func) {
			int nlen = strlen(builtins[seq].name);
			int dlen = strlen(builtins[seq].desc);
			grow(0, "template name", char, buf, nlen + dlen + 3, &buflen);
			memcpy(buf, builtins[seq].name, nlen);
			memset(buf + nlen, ' ', 2);
			memcpy(buf + nlen + 2, builtins[seq].desc, dlen + 1);
			(*func)(seq, buf);
		}
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
	CARD		*card,		/* need this for form name */
	bool		pr_template)	/* if true, export template itself to ofp */
{
	FILE		*fp;		/* template file (input) */
	char		*path;		/* template file path */
	const char	*ret;		/* returned error message or 0 */

	if (!ntemps)
		list_templates(0, card);
	if (pr_template && !ofp /*  && !oname */) /* for now, grok doesn't support writing to a named file */
		ofp = stdout;
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
				// return eval_template(card, stdin, "stdin", flags, ofp, oname);
				static char buf[1024];
				int n;
				fp = pr_template ? ofp : tmpfile();
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
				if(pr_template)
					return 0;
				rewind(fp);
				return eval_template(card, fp, "stdin", flags, ofp, oname);
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
		fp = pr_template ? ofp : tmpfile();
		if(!fp)
			return "Can't open temporary file for template output";
		if (!(ret = (*builtins[seq].func)(card, fp))) {
			if(!pr_template) {
				rewind(fp);
				ret = eval_template(card, fp, builtins[seq].name, flags, ofp, oname);
			}
		} else if(!pr_template)
			fclose(fp);
	} else {
		path = get_template_path(templates[seq - NBUILTINS], 0, card);
		if (!(fp = fopen(path, "r")))
			ret = "failed to open template file";
		else if(pr_template) {
			static char buf[1024];
			int n;
			while(1) {
				n = fread(buf, 1, sizeof(buf), fp);
				// FIXME: retry on EAGAIN or EINTR
				if(!n) // FIXME: report errors
					break;
				fwrite(buf, n, 1, ofp);
			}
			fflush(ofp); // FIXME: report errors
			ret = NULL;
		} else
			ret = eval_template(card, fp, path, flags, ofp, oname);
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
			if ((err = (*builtins[seq].func)(card, ofp)))
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

bool delete_template(
	QWidget		*shell,		/* export window widget */
	int		seq,		/* template to delete, >= NBUILTINS */
	CARD		*card)		/* need this for form name */
{
	char		*path;

	if (seq < NBUILTINS) {
		create_error_popup(shell,0,"Cannot delete a builtin template");
		return(false);
	}
	path = get_template_path(0, seq, card);
	if (unlink(path)) {
		create_error_popup(shell, errno, "Failed to delete %s", path);
		free(path);
		return(false);
	}
	free(path);
	return(true);
}


/*****************************************************************************
 **				parse template				    **
 *****************************************************************************/

#define ISSPACE(c) ((c)==' ' || (c)=='\t' || (c)=='\n')
#define ISOCTAL(c) ((c)>='0' && (c)<='7')
#define NEST		20

static const char html_subst[] = "<=&lt; >=&gt; &=&amp; \n=<BR>";
static const char *eval_command(CARD *&, char *, bool *, int);
static const char *sort_foreach_db(char *&word, CARD *card, CARD *ocard, bool first);
static const char *parse_db_name(CARD *card, char *&word, const char *&db_name);
static void free_other_db(CARD *);
static const char *putstring(const char *);

struct forstack {
	long	offset;	/* file offset following FOREACH */
	int	num;	/* current offset into array */
	int	nquery;	/* # if elements in query (var index in array FOREACH) */
	int	*query;	/* array for normal FOREACH */
	char	*array;	/* array for string-as-array FOREACH */
	int	nest;	/* n_true_if at time of FOREACH */
	CARD	*card;	/* enclosing card if a db change was made */
	int	*default_query;	/* enclosing default query from -x option or 0 */
	int	default_nquery;	/* enclosing number of rows in default_query */
};

enum opcode { O_EXPR, O_IF, O_ELSE, O_ELSEIF, O_ENDIF, O_FOREACH, O_END,
	      O_QUIT, O_FILE, O_SUBST };
const char quit_cmd[] = "QUIT";
static const struct { enum opcode opcode; const char *name; } opcode_list[] = {
	{ O_IF,		"IF",		},
	{ O_ELSE,	"ELSE",		},
	{ O_ELSEIF,	"ELSEIF",	},
	{ O_ENDIF,	"ENDIF"		},
	{ O_FOREACH,	"FOREACH"	},
	{ O_END,	"END"		},
	{ O_QUIT,	quit_cmd	},
	{ O_FILE,	"FILE"		},
	{ O_SUBST,	"SUBST"		},
	{ O_EXPR,	0		}
};

static FILE		*ofp;		/* output file */
static bool		close_ofp;	/* close output file? */
static char		*outname;	/* current output filename, 0=stdout */
static FILE		*ifp;		/* template file */
static char		*subst[256] = {};	/* current character substitutions */
static int		*default_query;	/* default query from -x option or 0 */
static int		default_nquery;	/* number of rows in default_query */
static int		default_row;	/* row to use outside foreach loops */
static int		n_true_if;	/* number of true nested \{IF}'s */
static int		n_false_if;	/* number of false nested \{IF}'s */
std::vector<bool>	had_true;
static struct forstack	forstack[NEST];	/* context for current foreach loop */
static int		forlevel;	/* forstack index, -1=not in a loop */
static int		forskip;	/* # of empty loops, skip to END */


/*
 * evaluate a template and print its results to an output file or stdout.
 * Return 0 if all went well, or an error string.
 */

static const char *eval_template(
	CARD		*card,
	FILE		*iifp,		/* template file */
	const char	*iname,		/* template name */
	int		flags,		/* flags a..z */
	FILE		*iofp,		/* output file, if already open */
	char		*oname)		/* default output filename, 0=stdout */
{
	char		*word = NULL;	/* command string in \{ } */
	size_t		wordlen = 0;
	const char	*p;
	int		indx = 0;	/* next free char in word[] */
	int		line = 1;	/* line number in template */
	int		c, prevc;	/* curr end prev char from template */
	int		bracelevel = 0;	/* if inside \{ }, brace level > 0 */
	bool		quote = false;	/* inside \{ }: inside quoted text? */
	bool		eat_nl = false;	/* ignore \n after \{COMMAND} */
	int		i;

	ifp = iifp;
	default_row   = card->row;
	default_query = 0;
	if ((default_nquery = card->nquery)) {
		default_query = alloc(0, "query", int, default_nquery);
		tmemcpy(int, default_query, card->query, default_nquery);
	}
	n_true_if = n_false_if = forskip = 0;
	had_true.clear();
	forlevel = -1;
	outname = oname ? mystrdup(resolve_tilde(oname, 0)) : 0;
	ofp = iofp;
	close_ofp = !iofp;

	fgrow(0, "template parse", char, word, 1, &wordlen);
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
				if ((p = eval_command(card, word+1, &eat_nl, flags))) {
					if (p == quit_cmd)
						*word = 0;
					else {
						int len = strlen(iname) + strlen(p) + 20;
						fgrow(0, "error message",
						      char, word, len, &wordlen);
						sprintf(word, "%s line %d: %s",
							iname, line, p);
					}
					break;
				}
			} else {
				/* +1 for c, + 1 for term 0, + 1 for EXPR's term brace */
				grow(0, "template parse", char, word, indx + 3, &wordlen);
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
			eat_nl = false;
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
	while (forlevel >= 0) {
		if(forstack[forlevel].card) {
			free_other_db(card);
			card = forstack[forlevel].card;
		}
		free(forstack[forlevel--].query);
	}

	zfree(card->query);
	card->query  = default_query;
	card->nquery = default_nquery;
	card->row    = default_row;

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
	CARD		*&card,
	char		*word,		/* command to evaluate */
	bool		*eat_nl,	/* if command, set to true (skip \n) */
	int		flags)		/* template flags a..z */
{
	char		*cmd;		/* extracted command word */
	const char	*pc, *err;
	struct forstack	*sp;		/* current foreach stack level */
	int		i, nq, *qu;

	*eat_nl = true;
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
	  case O_ELSEIF:
	  	if (forskip) {
			if (opcode_list[i].opcode == O_IF)
				n_false_if++;
			break;
		}
		if (opcode_list[i].opcode == O_IF) {
			if (n_false_if) {
				n_false_if++;
				break;
			}
			if ((int)had_true.size() > n_false_if + n_true_if + 1)
				had_true[n_false_if + n_true_if + 1] = false;
		} else {
			if (n_false_if > 1)
				return 0;
			if (n_false_if) {
				if ((int)had_true.size() > n_false_if + n_true_if &&
				    had_true[n_false_if + n_true_if])
					return 0;
				n_false_if--;
				/* drop through to if processing */
			} else if (n_true_if) {
				if (forlevel >= 0 &&
				    forstack[forlevel].nest == n_true_if)
					return "ELSEIF before END";
				had_true.resize(n_true_if + 1);
				had_true[n_true_if] = true;
				n_false_if++;
				n_true_if--;
				return 0;
			} else
				return "ELSEIF without IF";
		}
		if ((*word == '-' || *word == '+') && word[1] >= 'a' && word[1] <= 'z' && !word[2]) {
			if(!!(flags & (1<<(word[1] - 'a'))) == (*word == '-'))
				n_true_if++;
			else
				n_false_if++;
		} else {
			if (evalbool(card, word))
				n_true_if++;
			else
				n_false_if++;
		}
		break;

	  case O_ELSE:
		if (forskip || n_false_if > 1)
			return 0;
		if (n_false_if) {
			if((int)had_true.size() > n_false_if + n_true_if &&
			   had_true[n_false_if + n_true_if])
				return 0; /* "Too many ELSEs"; -- only true if all elses were ELSE */
			n_false_if = 0;
			n_true_if++;
		} else if (n_true_if) {
			if (forlevel >= 0 &&
			    forstack[forlevel].nest == n_true_if)
				return "ELSE before END";
			had_true.resize(n_true_if + 1);
			had_true[n_true_if] = true;
			n_true_if--;
			n_false_if++;
		} else
			return("ELSE without IF");
		break;

	  case O_ENDIF:
		if (n_false_if)
			n_false_if--;
		else if (n_true_if) {
			if (forlevel >= 0 &&
			    forstack[forlevel].nest == n_true_if)
				return "ENDIF before END";
			n_true_if--;
		} else
			return("ENDIF without IF");
		break;

	  case O_FOREACH: {
		if (forskip || n_false_if) {
			forskip++;
			break;
		}
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
			sp->nest = n_true_if;
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
			sp->array = zstrdup(evaluate(card, word));
			{
				char sep, esc;
				get_form_arraysep(card->form, &sep, &esc);
				int begin, after = -1;
				do {
					next_aelt(sp->array, &begin, &after, sep, esc);
				} while(after >= 0 && begin == after &&
					sp->nquery < 0);
				if(after < 0) {
					forskip++;
					zfree(sp->array);
					forlevel--;
				} else {
					int var = (sp->nquery < 0 ? -sp->nquery : sp->nquery) - 1;
					char c = sp->array[after];
					*unescape(sp->array + begin, sp->array + begin, after - begin, esc) = 0;
					set_var(card, var, zstrdup(sp->array + begin));
					// no need to re-escape and re-store value
					sp->array[after] = c;
				}
				sp->num = after;
			}
			break;
		}
		CARD *other_db = NULL;
		if(*word == '@') {
			FORM *form;
			DBASE *dbase;
			const char *db_name, *msg;
			char c;
			word++;
			if((msg = parse_db_name(card, word, db_name)))
				return msg;
			c = *word;
			*word = 0;
			if(!*db_name || !((form = read_form(db_name))))
				return "Can't find database";
			if(!(dbase = read_dbase(form)))
				dbase = dbase_create(form);
			form->dbpath = dbase->path;
			other_db = create_card_menu(form, dbase, 0, true);
			other_db->prev_form = zstrdup(card->form->name);
			other_db->last_query = -1;
			*word = c;
		}
		CARD *ncard = other_db ? other_db : card;
		/* sorting this db requires a new card to avoid resorting old */
		if (!other_db && (*word == '+' || *word == '-')) {
			other_db = create_card_menu(card->form, card->dbase, 0, true);
			other_db->prev_form = zstrdup(card->prev_form);
			other_db->last_query = -1;
			ncard = other_db;
		}
		/* sorting is recursive to invert sort order */
		/* I'd prefer to delay until after query, since it's pointless
		 * to sort if the query returns nothing, but that's OK for now */
		if(const char *msg = sort_foreach_db(word, ncard, card, !!other_db)) {
			free_other_db(other_db);
			return msg;
		}
		if (*word) {
			query_any(SM_SEARCH, ncard, word);
			nq = ncard->nquery;
			qu = ncard->query;
		} else if (!other_db || other_db->form == card->form) {
			nq = default_nquery;
			qu = default_query;
		} else {
			const FORM *form = ncard->form;
			/* do_query, but without saving in history */
			/* also, don't bother setting row */
			if(form->autoquery < 0)
				query_all(ncard);
			else {
				const char *q = form->query[form->autoquery].query;
				if(*q == '/')
					query_search(SM_SEARCH, ncard, q + 1);
				else
					query_eval(SM_SEARCH, ncard, q);
			}
			nq = ncard->nquery;
			qu = ncard->query;
			
		}
		if (nq) {
			sp = &forstack[++forlevel];
			sp->offset = ftell(ifp);
			sp->nest = n_true_if;
			sp->num    = 0;
			sp->nquery = nq;
			sp->query  = alloc(0, "query", int, nq);
			tmemcpy(int, sp->query, qu, nq);
			ncard->row = sp->query[sp->num];
			if(other_db) {
				sp->card = card;
				if (card->form != other_db->form) {
					sp->default_nquery = default_nquery;
					sp->default_query = default_query;
					default_nquery = nq;
					default_query = sp->query;
				} else
					sp->default_nquery = -1;
				card = other_db;
			} else
				sp->card = NULL;
		} else {
			free_other_db(other_db);
			forskip++;
		}
		break;
	  }
	  case O_END:
		if (forskip) {
			forskip--;
			break;
		}
		if (forlevel < 0)
			return("END without FOREACH");
		sp = &forstack[forlevel];
		if (n_false_if || sp->nest != n_true_if)
			return("IF before END");
		if(!sp->query) {
			char sep, esc;
			get_form_arraysep(card->form, &sep, &esc);
			int begin, after = sp->num;
			do {
				next_aelt(sp->array, &begin, &after, sep, esc);
			} while(after >= 0 && begin == after &&
				sp->nquery < 0);
			if(after < 0) {
				zfree(sp->array);
				forlevel--;
			} else {
				int var = (sp->nquery < 0 ? -sp->nquery : sp->nquery) - 1;
				char c = sp->array[after];
				*unescape(sp->array + begin, sp->array + begin, after - begin, esc) = 0;
				set_var(card, var, zstrdup(sp->array + begin));
				// no need to re-escape and re-store value
				sp->array[after] = c;
				fseek(ifp, sp->offset, SEEK_SET);
				sp->num = after;
			}
			break;
		}
		if (++sp->num < sp->nquery) {
			card->row = sp->query[sp->num];
			fseek(ifp, sp->offset, SEEK_SET);
		} else {
			free(sp->query);
			if (sp->card) {
				free_other_db(card);
				card = sp->card;
				if(sp->default_nquery >= 0) {
					default_query = sp->default_query;
					default_nquery = sp->default_nquery;
				}
			}
			forlevel--;
		}
		break;

	  case O_FILE:
		if (forskip || n_false_if)
			break;
		if (ofp && close_ofp)
			fclose(ofp);
		zfree(outname);
		outname = zstrdup(evaluate(card, word));
		ofp = 0;
		close_ofp = true;
		break;

	  case O_SUBST:
		if (forskip || n_false_if)
			break;
		if(!strcmp(word, "HTML"))
			substitute_setup(subst, html_subst);
		else {
			backslash_subst(word);
			substitute_setup(subst, word);
		}
		break;

	  case O_EXPR:
		*eat_nl = false;
		if (forskip || n_false_if)
			break;
		if(!*word) // Allow newline suppression with blank expr
			break;
		/* word is now guaranteed to have space before & after */
		/* I have no idea why this never crashed before */
		*--word = '{';
		strcat(word, "}");

		pc = evaluate(card, word);
		if(!pc)
			return "error in expression";
		cmd[1] = 0;
		while ((cmd[0] = *pc++))
			if ((err = putstring(subst[(unsigned char)cmd[0]] ? subst[(unsigned char)cmd[0]] : cmd)))
				return(err);
		break;

	  case O_QUIT:
		if (!forskip && !n_false_if)
			return quit_cmd;
	}
	return(0);
}

static const char *sort_foreach_db(char *&word, CARD *card, CARD *ocard, bool first)
{
	while(ISSPACE(*word))
		word++;
	if(*word != '+' && *word != '-') {
		if(first) {
			const FORM *form = card->form;
			for (int i=0; i < form->nitems; i++)
				if (IFL(form->items[i]->,DEFSORT)) {
					dbase_sort(card, form->items[i]->column, 0);
					break;
				}
		}
		return NULL;
	}
	bool rev = *word++ == '-';
	const char *field, *msg;
	if((msg = parse_db_name(ocard, word, field)))
		return msg;
	char c = *word;
	int fn;
	*word = 0;
	if(field && *field == '_')
		field++;
	if(!field || !*field)
		return "Invalid field name";
	if(isdigit(*field))
		fn = atoi(field);
	else {
		const FORM *form = card->form;
		const FIELDS *s = form->fields;
		auto it = s->find((char *)field);
		if(it != s->end()) {
			int i = it->second % form->nitems;
			int m = it->second / form->nitems;
			fn = IFL(form->items[i]->,MULTICOL)?
				form->items[i]->menu[m].column :
				form->items[i]->column;
		} else
			return "Invalid field name";
	}
	*word = c;
	sort_foreach_db(word, card, ocard, false);
	dbase_sort(card, fn, rev, !first && card->sorted);
	return NULL;
}

/* 3 formats are supported for db names and sort fields:
 *   "-enclosed string with \-escaped next char
 *   {}-enclosed expression
 *   plain text, terminated by next space or +, -
 */

static const char *parse_db_name(CARD *card, char *&word, const char *&db_name)
{
	while(ISSPACE(*word))
		word++;
	db_name = word;
	if(*word == '"' || *word == '\'') {
		char q = *word, *d = word;
		while(*++word && *word != q) {
			if(*word == '\\' && word[1])
				word++;
			*d++ = *word;
		}
		if(!*word)
			return "Unterminated name";
		*d = 0;
		word++;
	} else if(*word == '{') {
		int bnest = 0;
		char c;
		while(*++word && (bnest || *word != '}')) {
			if(*word == '"') {
				while(*++word && *word != '"')
					if(*word == '\\' && word[1])
						word++;
				if(!*word)
					return "Unterminated name";
			} else if(*word == '{')
					bnest++;
			else if(*word == '}')
				bnest--;
		}
		if(!*word)
			return "Unterminated name";
		c = *++word;
		*word = 0;
		db_name = evaluate(card, db_name);
		if(!db_name)
			return "Error in name expression";
		*word = c;
	} else {
		while(*word && (*word != '+' && *word != '-' && !ISSPACE(*word)))
			word++;
	}
	return NULL;
}

static void free_other_db(CARD *card)
{
	if(card) {
		FORM *form = card->form;
		DBASE *dbase = card->dbase;
		free_card(card);
		form_delete(form);
		dbase_delete(dbase);
	}
}

/*
 * Set up substitutions by parsing the "x=y x=y ..." substitution string into
 * an array of replacement strings, one per character code 0..255. This is used
 * by the SUBST command above, and the tr command in the regular grammar.
 * Return an error message or 0 if all went well.
 */

const char *substitute_setup(
	char		**array,	/* where to store substitutions */
	const char	*cmd)		/* x=y x=y ... command string */
{
	int		i;
	const char	*q;
	
	while (*cmd) {
		// tjm - FIXME:  if 1st char is =, it will go too far
		// In fact, this should only work if at least 1 ws has been
		// skipped
		if (*cmd == '=' && cmd[1] != '=')
			cmd--;
		i = *cmd++;
		if (*cmd++ != '=')
			return("malformed substition, expected"
				      " <char>=<string> list");
		/* there is no good reason for the 255-char limit */
		for (q=cmd; /* q < cmd+255 && */ *q && !ISSPACE(*q); q++);
		zfree(array[i]);
		if(q != cmd) {
			/* even non-fatal alloc isn't mild enough */
			/* instead, just return an error message */
			if(!(array[i] = (char *)malloc(q - cmd + 1)))
				return("no memory for substitution list");
			memcpy(array[i], cmd, q - cmd);
			array[i][q - cmd] = 0;
		}
		cmd = q;
		while (ISSPACE(*cmd)) cmd++;
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
