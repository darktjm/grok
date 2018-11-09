Graphical Resource Organizer Kit
================================

This is an old Motif-based generic personal database application.  My
first introduction to the concept came from [Fiasco on the Amiga](
http://aminet.net/search?name=fiasco), even though it's likely grok
came first.  Unlike Fiasco, grok is for UNIX, and is also free
software.  Since I actually implemented a database using it, I have
verified that it works with OpenMotif 2.3.8 on Linux.  I make no
guarantees for any other target.  I have adequately addressed (but not
fully fixed) a few bugs I found.  That's what this repository is: the
last official release of grok I know of ([xmbase_grok 1.5 from 23 May
2001](
ftp://ftp.fu-berlin.de/pub/unix/graphics/grok/xmbase_grok-1.5.tar.gz))
with a few extra fixes (I have taken the liberty to update the version
to 1.5.1).  See the original README below for information on the
original and its author (who no longer acknowledges its existence on
his own site); I take no credit for this application.  I am working on
a similar application that works a little differently, but I thought
I'd release this now that it's usable.

Since the time I wrote the above, I have made no progress on my own
application, so I have instead decided to make more substantial
changes to grok.  See TODO.md.  Among other things, the move to Qt5
now depends on Qt5 (tested with 5.11) instead of Motif (obviously) and
has different build instructions:

>     cd src; qmake && make && sudo make install

Note that if you want to install into a different root (e.g. when
generating a package), use `INSTALL_TARGET` instead of the traditional
`DESTDIR`:

>     make install INSTALL_TARGET=/tmp/pkg

All of my modifications to this source, as represented by the SCM
changes at <https://bitbucket.org/darktjm/grok/> are granted to the
public domain. Keep in mind that the original author may not
appreciate email about this version.  Once the TODO.md list has been
pared down a bit, I will shorten and reformat to be more like the
original documentation.

I have decided to include the orignal README verbatim (with formatting
changes to support markdown) below.

xmbase-grok - Graphical Resource Organizer Kit
----------------------------------------------

For people who don't read long READMEs, I'll describe compilation and
installation first:

  - edit Makefile and change GLIB and GBIN if the program and support files
     should be installed in a directory other than /usr/local/{bin,lib}.
  - run "make" and choose a system from the list
  - run "make *sys*"
  - run "grok" and choose a database from the Database pulldown
  - if you don't like grok, delete this directory and the ~/.grok directory
  - run "make install"
  - copy ../demo/* to ~/.grok
  - if your system is an SGI, copy ../misc/Grok.icon to ~/.icons/Grok.icon

You need Motif, LessTif (see http://www.lesstif.org) 0.88.0 or later, or
OpenMotif (see http://www.openmotif.org) to compile grok.

---------------------------------------

xmbase-grok, formerly just called "grok", is a simple database manager
and UI builder that can -

  *  keep phone and address lists (like a rolodex)
  *  store phone call logs
  *  keep todo lists
  *  manage any other database after simple GUI-driven customization
  *  customized data export, HTML export built in

More precisely, grok is a program for displaying and editing strings
arranged in a grid of rows and columns. Each row is presented as a "card"
consisting of multiple columns, or "fields", that allow data entry. The
presentation of the data is programmable; a user interface builder that
allows the user to arrange fields on a card graphically is part of grok.
Grok also supports a simple language that allows sophisticated queries
and data retrieval. Grok comes with the above examples and a few others
as pre-built applications.

Grok is not a general-purpose database program. It was designed for small
applications typical for desktop accessories. If you attempt to run your
major airline reservation system or your space shuttle project with it,
you are guaranteed to be disappointed.

The distribution contains sample applications and sample databases in the
"demo" directory. If grok is started in a directory that contains a
"demo" directory, the contents of that directory are presented in grok
in addition to the ones from ~/.grok. This is meant to allow experimenting
with grok without having to copy files to one's home directory.

The grok executable and the grok.help file should be copied to the GBIN
and GLIB directory listed in the makefile used, respectively (default is
/usr/local/bin and /usr/local/lib), although any other place in the search
path ($PATH) will also work. Run "make install" to install grok. In
addition, the distribution contains a PostScript user manual "Manual.ps"
that should be copied to a safe place, or printed. The TeX sources for
Manual.ps are in the doc directory.

Bug reports to thomas@bitrot.de. Don't forget to include the
version number as printed by "grok -v". If you have new applications
(forms) that would be of general interest, I'd appreciate to get a copy
for inclusion in the next release.

The main archive for grok is

>   <ftp://ftp.fu-berlin.de/pub/unix/graphics/grok/>

To subscribe to the plan mailing list, send a mail with "subscribe grok"
in the message body (not the subject) to majordomo@bitrot.de. The list
carries a low volume and is mainly used for announcements and patches.


Copyright:
---------

xmbase-grok is Copyrighted by Thomas Driemeyer, 1993-2001. License
to copy, publish, and distribute is granted to everyone provided that
three conditions are met:

- my name and email address, "Thomas Driemeyer <thomas@bitrot.de>"
  must remain in the distribution and any documentation that was not
  part of this distribution. In particular, my name and address must
  be shown in the About popup.
- if you redistribute a modified version, the fact that the version
  is modified must be stated in all places that my name is shown.
- this copyright notice must be included in your distribution.

If these conditions are met, you can do whatever you like. The idea is
that I would be pissed if someone else claimed he wrote the thing, and
I don't want bugs introduced by others attributed to me. Make as much
money with it as you can. Drop me a line, I am curious.

If you put plan on a CD, send me a free copy if your company policy
allows it and you want to. (Not obligatory, I just collect trophies.)

There are no implied or expressed warranties for grok. I do not claim it
is good for anything whatsoever, and if you lose your precious data or
your cat dies this is entirely your problem.

Note:  as per the license, I hereby notify all who read this that I,
Thomas J. Moore, have made changes to this distribution, in the form
of minor bug fixes.  I don't really feel like trying to report them
upstream, given that the author's primary web site has no mention of
grok any more.