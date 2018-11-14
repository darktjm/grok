Introduction:  The search for a Motif replacement
-------------------------------------------------

After the issues I had with Motif's multi-line text widget, I looked
into a number of alternate toolkits.  I figure if I'm going to go
through the effort of converting to a different toolkit, it should
have more to offer than just a functional multi-line text widget.  One
possible future goal would be to support other operating systems, such
as Windows or Android.  I concentrated mostly on C-based toolkits, and
rejected most of them:

  - [libui](https://github.com/andlabs/libui) - Alpha state, subject
    to massive change.  Main obvious thing missing is ability to
    manage text correctly.  No Android port, but at least all 3
    major desktop environments (GTK+ on UNIX)

  - [MX](https://github.com/clutter-project/mx) - OpenGL-based
    (actually, clutter-based).  Really crappy docs -- no idea
    where to start (ref only, non-functional tests).  Clutter is
    poorly documented, as well.  It's a complex library with
    little gain. Not even standard dialogs.

  - [GTK+](https://www.gtk.org/) - heavily influenced by GNOME. 
    GTKAndroid is pretty much unusable.  Ports to Windows and Mac
    are decidedly non-native. I judge UIs by their scrollbars, and
    GTK+-3's default scrollbar is complete garbage, losing 20+
    years of UI design.

  - [XForms](http://xforms-toolkit.org/) - The first UNIX GUI
    toolkit I ever wrote anything with. Not portable, and not
    native-looking anywhere.  Maybe better than Motif, but that's
    not saying much.

  - [Tk](https://www.tcl.tk/) - Tcl first, C a remote second (no
    direct calls to high-level functions at all).  There's
    AndroidWish, but I want to write native C.

  - [OTK](https://sourceforge.net/projects/otk/) - OpenGL GUI, but
    missing some important features, like scrolled panels.  Also
    only uses a custom vector font (i.e., no TrueType support).
    Also dead since 2014, apparently.

  - [EFL](https://www.enlightenment.org/about-efl) - Enlightenment:
    glitzy, but not very usable.  Not really good for anything but
    X/Linux, but nonetheless works on Mac, Win, raw framebuffer,
    but there are no obvious instructions on how to use it on Win
    or fb, so forget it.

  - [nuklear](https://github.com/vurtun/nuklear) - OpenGL GUI, but
    missing some important features, like top-level windows (which
    could be simulated using SDL, I guess), broken text areas
    (totally insane in xft demo, no horizontal scrollbar, no
    connection between X selection and GUI selection (maybe just
    needs work in the X driver), no selection at all in
    single-line text widgets, and clips text too much near bottom
    of widget), undocumented and mostly broken low-level interface
    code (in demo, rather than a sane place).

  - [ecere](http://ecere.org/) - Uses its own OO programming
    language based on C, and, like many modern toolkits, expects
    people to use its own IDE. Supposedly makes it easy to make
    Linux, Mac, Win, Android, HTML(!) apps, but to me, "easy"
    means "well-documented", which is the main way that ecere
    fails (poor, mostly tutorial-level documentation).  Not worth
    the effort, even though it is slightly more C-compatible than C++.

  - [wxWidgets](https://wxwidgets.org/) - My old go-to portability
    toolkit.  No Android, despite SoC attempts.  No point in going
    C++ if it doesn't give me a huge advantage over C toolkits.

  - [cegui](http://cegui.org.uk/) - Mostly the same issues as
    nuklear, but also C++.

  - html5/JavaScript - Need to (re-)learn a lot of tech to get it
    working, but CrossCode proves it can probably be done.  On the
    other hand, it requires rewriting *everything*.

The only C-based GUI toolkit I found that even came close to what I
wanted was [IUP](http://www.tecgraf.puc-rio.br/iup/).  It's by the Lua
team, but unlike Tk, it's C first and Lua second.  It uses native
controls, just like wxWidgets, and even has a Motif version (and GTK+
as well for UNIX targets).  It supports Mac and Windows, but it does
not and probably never will support Android.  It's unpopular, so
probably not available on most Linux distributions, and a pain in the
ass to build along with its dependencies (I never did get the
webkit-based controls to build).  I made a good start on porting grok
to IUP/CD/IM, but then I got the Motif text area widgets to work a
little better, so the urgent need for a C toolkit disappeared, and I
dropped it.

Qt
--

In the end, I just decided that using [Qt](https://qt.io/) would be
easiest, even though I have avoided it like the plague in the past. 
Qt is actually what I like to call (C++)++, in that it uses its own
preprocessor because C++ isn't good enough for it.  I have avoided moc
as much as possible (I had to resort to it to suppoort extra qss
properties), and, in fact, have tried to minimize the changes to the
code, so I'm sure OO purists will scream if they look at this code
(good!).  Here are some additional notes about the Qt port:

If you like the Motif look, Qt sort of supports it with a style
plugin.  This used to come with Qt, but now it has been moved to its
[own site](https://github.com/qt/qtstyleplugins.git), where it will
slowly rot away due to lack of maintenance.  Once the plugin is
installed, you can invoke it with the Qt standard command-line
agrument `-style=motif`.  Don't expect that style to make grok look
exactly like before, though.  Not only did I make some fundamental
changes in how a few things worked, but Qt's Motif emulation is
imperfect at best.

Qt, like most modern toolkits (and, necessarily, all portable
toolkits) does not support X/Xt resources (even when using the motif
style plugin).  This means no single location for configuration, no
universal restyling and rebinding, and no grok-specific out-of-band
(i.e., not controlled by the built-in preference editor) preferences,
at least via Xt resources.  It also means no `editres` or other
standard introspection tool.  On the other hand, I have tried to
reflect the resource usage into Qt style sheets (qss) by setting
object names and adding properties where appropriate. The default
style sheet corresponds to the default resources, and can be
overridden with the Qt standard `-stylesheet=`*file* argument.  The
introspection can be accomplised by
[Qt-Inspector](https://github.com/robertknight/Qt-Inspector.git) or a
similar tool (apparently commercial Qt customers get the Qt Object
Inspector, which does the same thing; this is not to be confused with
the designer's Object Inspector, which only works for IDE-designed
objects, as far as I know).  You can also build a hierarchy for use
with qss using [in-application tree
printing](https://brendanwhitfield.wordpress.com/2016/06/09/inspecting-qts-object-hierarchy/),
but I didn't, and I don't recommend it, because you'll have to
annotate every dialog/window separately.

I didn't name every widget object.  In particular, labels and buttons
are not named after their labels.  In Motif, the button/label text is
automatically set to the name if not set explicitly, so the easiest
way to set the label is to just set the name.  Qt requires an explicit
extra step to set the object name to the label, but it isn't
necessary.  Instead, just use the text property selector; e.g.:

>     QPushButton[text="Help"] { color: green; }

It is not possible to style non-widgets.  This means that the drawn
items in the bar charts and form editor canvas must get their
preferences from elsewhere.  It is also not possible to style layouts
directly, since they are not widgets.  I have added a hack to support
these cases using hidden widgets for layouts and actually using widget
overrides in the other two cases (since they are needed for mouse and
paint event captures anyway).

However, QSS has some serious issues.  One of the ways it emulates
native look and feel is to have style plugins.  Setting a stylesheet
parameter often overrides the style plugin entirely.  For example,
grok set the background of checkboxes to red.  Doing the same in Qt
causes the entire checkbox graphics to be removed and replaced by a
plain block.  Overriding any colors is unsafe: Qt does not bother
calculating things like frame colors, selection colors, and cursor
colors (or, like Motif in some cases, foreground colors) based on a
background color, either.  Also, it's impossible to query QSS:
everything is just done in some hidden code behind the scenes, with no
control or means to create your own widgets which can be styled with
anything but qproperty- tags.  It's also not well enough documented:
it's impossible to know how or when QSS overrides properties set by
direct function calls or styles.  This is much like how layouts work
in Qt: everything is hidden, and it's virtually impossible to tell
what is called, and when.

Then again, it's not possible to set a lot of low-level things in Qt,
because it's "handled by the style", which in turn can't be queried
itself.  For example, text widget cursor color appears to always be
thin and black in GTK+3, eliminating the possibility of dark-colored
widgets.  Not only can't the color be set in QSS, I don't think it can
be set anywhere in Qt except by modifiying the renderer itself.
Similarly, QPlatformTheme has a lot of useful information that could
be used to e.g. set standard shortcuts, standard labels, standard
layouts, etc., but it is hidden in a private structure.  It seems like
the creators of a so-called portable toolkit have made it as hard as
possible to actually make portable applications.  Then again, maybe
I'm missing something.  Like using XML and/or designer to define all
the user interface, as it seems to want (not gonna happen).

As a side issue, I had to jump through extra hoops to get gentoo to
provide me with debug information, even though it should have been as
simple as adding debug to the USE flags.  Not only did I have to add
the nostrip FEATURE, but I had to explicitly add -ggdb3 to CXXFLAGS. 
This is also a failing of the Qt build itself, and compiling with
OPTION+=debug should, but doesn't, do that automatically (or maybe
it's because of gentoo's attempts at integrating CXXFLAGS).
