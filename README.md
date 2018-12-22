grok - Graphical Resource Organizer Kit
=======================================

grok, formerly called "xmbase-grok", before which it was called
"grok", is a simple database manager and UI builder that can -

  *  keep phone and address lists (like a rolodex)
  *  store phone call logs
  *  keep todo lists
  *  manage any other database after simple GUI-driven customization
  *  customized data export and printing, HTML and plain text export built in

See the build section below if you just want to get started using grok.

More precisely, grok is a program for displaying and editing strings
arranged in a grid of rows and columns.  Each row is presented as a "card"
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
"demo" directory. If grok is started in a directory that contains
databases, the contents of that directory are presented in grok
in addition to the ones from ~/.grok. This is meant to allow experimenting
with grok without having to copy files to one's home directory.

Send bug reports to <https://bitbucket.org/darktjm/grok/issues>.  I will
likely ignore bug reports from anonymous users, since it's impossible
to reply to them, so you may need to create an Atlassian/bitbucket
account before reporting.  Don't forget to include the version number
as printed by "grok -v".  If you have new applications (forms) that
would be of general interest, I'd appreciate to get a copy for
inclusion in the next release.  I will also review git pull requests,
but I don't actually use git/bitbucket for source code control, and
will likely lose some metadata if I merge them.  You can also bug
thomas@bitrot.de if you like, but he is not responsible for any
changes that I made.

You can see the tags (releases) on the download page:

> <https://bitbucket.org/darktjm/grok/downloads/?tab=tags>

You can see my development in progress on my bitbucket page:

> <https://bitbucket.org/darktjm/grok>

You may also wish to check out some of the other files included in
this project, such as my notes on the QT port (QT-README.md), my
current plans (TODO.md) and the high-level change list (HISTORY).
You can also see the HTML documentation in the doc directory.  The
original README (now absorbed here) mentioned a PostScript/TeX
manual, but that was not in the above-mentioned archive (I believe it
was converted to HTML in 1.5; 1.4.3 still has the TeX sources).

Building:
--------

Before starting, install C++ development tools (C++-11 minimum; tested
with gcc and clang), cmake (tested with 3.13.1), Qt 5 (tested with
5.11.3) and GNU Bison (tested with 3.2.2):

>     cmake . && make
>     # if you want to install it now:
>     sudo make install/strip
>     # if you want to package it:
>     make install/strip DESTDIR=/tmp/pkg_root

If you prefer, you can build from an alternate directory and/or
compile in parallel:

>     mkdir build && cd build && cmake .. && make -j4

If you don't like the default installation locations, you can set set
them most easily by replacing cmake with either ccmake or cmake-gui.
In particular, CMAKE_INSTALL_PREFIX is a common variable to change,
and you can change it on the cmake command line as well:

>     cmake -DCMAKE_INSTALL_PREFIX=/usr .

xmbase-grok:
-----------

This application is a continuation of xmbase-grok, originally written
by Thomas Driemeyer.  That application used Motif instead of Qt.  I
made some changes to the latest version I found, released 23 May 2001:

>   <ftp://ftp.fu-berlin.de/pub/unix/graphics/grok/xmbase_grok-1.5.tar.gz>

The first checkin of this project is an exact copy of this archive.  I
initially made changes to address issues I noticed while using grok
with my own database, and released those changes as "1.5.1":

> <https://bitbucket.org/darktjm/grok/get/xmbase_grok-1.5.1.tar.bz2>

That corresponds to the git tag `xmbase_grok-1.5.1`, and is the last
version I released using Motif.  It builds at least on Linux using
OpenMotif 2.3.8.  It turned out that additional changes were made by
Thomas Driemeyer (up to version 1.5.4), but I have no intention of
merging these changes with the Motif version.  Instead, I merged a few
small changes from that release into the Qt version.  The only major
component in 1.5.4 but not yet in the Qt version is the partial
translation to German.

If you like the Motif look, Qt sort of supports it with a style
plugin.  This used to come with Qt, but now it has been moved to its
[own site](https://github.com/qt/qtstyleplugins.git), where it will
slowly rot away due to lack of maintenance.  Once the plugin is
installed, you can invoke it with the Qt standard command-line
agrument `-style=motif`.  Don't expect that style to make grok look
exactly like before, though.  Not only did I make some fundamental
changes in how a few things worked, but Qt's Motif emulation is
imperfect at best.  I also do all of my testing with the default
Linux/gtk-like style, so there may be issues specific to the Motif
plugin.  If you find any, feel free to report them.

Copyright:
---------

Grok is Copyrighted by Thomas Driemeyer, 1993-2001. License
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

Additional changes were made by Thomas J. Moore.  All changes by that
author, as represented by the SCM changes at
<https://bitbucket.org/darktjm/grok/> are granted to the public
domain.
