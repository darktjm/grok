#!/bin/env perl

## (c) James C. McPherson, 06-10-1995, v1.0
## mljmcphe@dingo.cc.uq.oz.au
## j.mcpherson@central1.library.uq.oz.au


#  transforms grok dbs into tex bibliographies, requires keys to search for.
#                                              ^^^^^^^^
#Version 2 (or whatever I call it) will take keys from stdin or just
#  munge the whole bibliographic database you've got.
#  This isn't the most tidy or pretty code that you've ever seen ;) but
#  I'm no perl expert and definitely _not_ a programmer -- i'm a historian or
#  librarian (depending on which day of the week it is and who the company is ;>)
#
#  I think the comments are reasonably explanatory. If not, then if you've got
#   (a) a basic understanding of perl, or
#   (b) know someone who does ;)
#  then you should be able to follow the code.
#  
# I, James McPherson, hereby place the Bibliofile card idea fully in the public
# domain. Anybody may use the Bibliofile idea and accompanying perl code, and
# modifications may be made also, on the condition that modifications get
# sent to me at mljmcphe@dingo.cc.uq.oz.au.
#
#  If you don't like it, then that's a shame -- I'd just like to have my
#  name on it (probably for vanity I guess). ;)
#
#  James C. McPherson. Historian and Library denizen
#  for email see above copyright stamp.


## usage: grok2tex.p keylist



# GROK is the source database
open (GROK, "/home/jcm/.grok/Biblio") || die "Unable to open Biblio: $!\n";

# nBIB is the destination/output file
open (nBIB, ">/home/jcm/tex/asn/generalbib.tex") || die "Unable to open output file:$!\n";


open (KEYLIST,"@ARGV") || die "Unable to open keylist specified: $!\n";


@KEYS = <KEYLIST>;  # our list of keys is the whole file

print "@KEYS\n";

close KEYLIST;


# we split the 
$SecondaryHeader = "\{\\bf\{Secondary Sources\}\}\n\n\\begin\{sloppypar\}\n\\begin\{description\}\n";
$SecondaryFooter = "\\end\{description\}\n\\end\{sloppypar\}\n";
$ArticleHeader = "\{\\bf\{Articles\}\}\n\n\\begin\{sloppypar\}\n\\begin\{description\}\n";
$ArticleFooter = $SecondaryFooter;

$ArtNum = 0;
$SecNum = 0;



while (<GROK>) {
    ($Author,$Title,$ed,$addr,$publisher,$date,
     $callNo,$annote,$article,$pp,$edition,$HASH) = split(/\|/,$_);
    chop;			# these are the fields as specified
                                # in bibliofile.gf, which we chop the 
                                # trailing '\n' off.

# if the hash key is in the keylist, continue, else read next record

    $count = 0;
    foreach $key (@KEYS) {
	if ($count < 2) {
	    if ($key eq $HASH) {   # the key in our list matches the key 
                                   # in the current record so we can 
                                   # output it to the new file
		
# article:
# output format: author, "article_title", in {\it{publisher}} date, pages.
               if ($article eq 'Art') {
		    $output = join('',"\\item ",
				   $Author," ",
				   "\"",$Title,
				   "\" in \{\\it\{",
				   $publisher,
				   "\}\} vol ",
				   $date,", ",
				   $pp,"\n");
# put the formatted record on the tail of the list of articles
		    @Articles[$ArtNum] = $output;
		    ++($ArtNum);
		} elsif ($ed eq '(ed)') {

# text with editor -- doesn't handle multiple editors yet
# output format: author (ed) {\it{title}} (place: publisher, date).
		    $output = join('',"\\item ",
				   $Author,". (ed),",
				   "\{\\it\{",
				   $Title,
				   "\}\} ",
				   "(",$addr,": ",$publisher,", ",$date,")\n");
# put the formatted record on the tail of the list of secondary sources
		    @Secondary[$SecNum] = $output;
		    ++($SecNum);
		} else {
# text -- default
# output format: author {\it{title}} (place: publisher, date).
		    $output = join('',"\\item ",
				   $Author,". ",
				   "\{\\it\{",
				   $Title,
				   "\}\} ","(",
				   $addr,": ",
				   $publisher,", ",
				   $date,")\n"); 
# put the formatted record on the tail of the list of secondary sources
		    @Secondary[$SecNum] = $output;
		    ++($SecNum);	# 
		}
		++($count);
	    }
	}
    }			       
}			       


# now we output both lists to the destination file.
print nBIB "$SecondaryHeader";
print nBIB @Secondary;
print nBIB "$SecondaryFooter";

print nBIB "$ArticleHeader";
print nBIB @Articles;
print nBIB "$ArticleFooter";


# tidy up our workspace and we're done.
close GROK;
close nBIB;
