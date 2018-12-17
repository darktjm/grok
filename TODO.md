I'm working on grok because I'm too lazy to continue working on my
own, similar application, and it's easier to be motivated when the
application already works, mostly.  However, grok needs a lot of work,
still.  Some of that work could have been done back when xmbase_grok
was still fresh, but I wasn't interested at the time.  After all (or
even most) of the below changes, grok will be almost what I wanted to
do anyway. Most of the other features I had planned were probably
pointless, anyway. Actually, most of the features I have planned below
are pointless, as well.  I'm not sure I'll be able to remain motivated
long enough to make much of a dent in this list.  I started on grok
for my game database, and I guess I'd like to get back to playing
games again...

   -- Thomas J. Moore

Bugs
----

- Clicking on a radio button (update=ALL) causes menu table to narrow
  in form editor.  Qt's resizing is a mystery to me, and one I don't
  relish investigating.

- Sometimes 1st widget of card appears even though no record is
  selected in main card window display.  Speaking of which, the static
  area doesn't really work any differently from the bottom area, as far
  as I can tell, even though the docs say otherwise, so maybe I need
  to revisit that. 

- Main window sizing issues:

    - The summary list is too wide.  The right-most column almost
      always extends too far, and I don't know how to shrink it.

    - The window doesn't resize to a narrow version on database switch
      (but it does do so on initial load and randomly, for some
      reason). resize(minimumSize()) after adjustSize() does help,
      but not enough.  Even "switching" to the same database
      produces random results.

    - When the main window resizes after a database switch, it not
      only resizes the window, but also moves it.  It should stay in
      one place.  Saving and restoring the position after
      adjustSize() does nothing, so maybe it's being moved
      elsewhere.  This may well be a window manager issue, though; I'm
      one of the few people who still uses FVWM, which I have been
      using for over 20 years now.  Some weirdnesses only apply to
      FVWM.

    I will probably need to eventually break down and dredge thorugh
    the Qt source to figure out how resizing actually works.  Adding
    the auto-grow/shrink tables to the form editor had resizing issues,
    as well.  I expect changing contents' sizes should cause a ripple
    effect throughout the hierarchy to adjust to this size, but
    instead, I have to call adjustSize() manually on the parent
    widgets.  Twice.  Calling updateGeometry() doesn't seem to do
    anything, except when it does.

- The layout of the form item editor needs tweaking.  At the very
  least, the chart options do not seem to have the right label
  width.  Also, items in the scroll area are too wide, or the scroll
  area is too narrow, depending on how you look at it, but only if
  the chart options appear. Otherwise, the opposite issue exists.  It
  appears that the layout is partially being influenced by invisible
  widgets.

- As expected, letter buttons still don't work. The first press works,
  but subsequent presses go all weird.

- Look more closely at all Qt-related licenses.  I don't want this
  program to be GPL.  In particular, QUiLoader seems to use a
  different license (or at least The Qt Co. thought it important
  enough to list the license in the introductory text).

- Note fields are currently not able to limit maximum length.

- The Canvas doesn't always highlight the widget loaded into the form
  editor window.  In particular, the first one and the one that gets
  selected after deleting a widget don't get highlighted.

- The canvas' close event is called on edit shutdown, even though it
  fortunately seems to have no effect.  Anything which disables
  the close callback like that should probably just be destroy'd.

- The help popup for questions immediately gets pushed behind question popup

- Invalid long options pop up the usage() after the fork.  Fixing this
  without disabling fork entirely is probably never going to happen,
  because QApplication silently dies if it's run before the fork.

Code Improvements
-----------------

- Have form_clone() fail gracefully and free its results on failure,
  aborting the form edit instead of killing grok.  In general,
  failures during form editing should just kill the form editor.

- constify *everything*

- protect all file I/O with EAGAIN/EINTR loops

- Since I'm using C++ anyway, convert BOOL, TRUE and FALSE to their
  C++ lower-case equivalents.  C supports them too, but with too many
  underscores.

- Identify abortable processes and catch all C++ exceptions to abort
  the process instead of the entire program.  In particular,
  expression evaluation and template processing should abort rather than
  kill grok.  Similarly, the new abort-on-alloc-failure code isn't
  as forgiving as it should be.

- On all file writes, fflush before fclose, and report on ferror().
  Normally I'd stop writing as soon as an error occurs, but that's too
  much trouble and probably not worth it.

- Check errors on all file reads, and report/abort if so.  I suppose
  that delaying until just before fclose would be sufficient.

- Consistently make blank strings NULL, and stop checking for blanks.

- Be more consistent on use of 0 vs NULL for pointers.  Yeah, my own
  additions are probably even more inconsistent than the old code was.
  The advantage of NULL is that it only works in pointer contexts.
  The advantage of 0 is that it's shorter to write.  In particular,
  use zstrdup instead of mystrdup in contexts where NULL is acceptable.

- Use specific pointer types instead of base classes to avoid the
  dynamic_cast overhead.  I'm not sure the compiler is smart enough
  to eliminate that itself.  In fact, it'd be nice if it were
  possible to build without rtti, but Qt adds its own rtti to every
  moc-compiled class (and has its own version of dynamic_cast), so
  that's not happening.  Well, Qt claims that it doesn't use rtti,
  but they really just mean that they use their own instead of the
  language-defined one. They say something about crossing dynamic
  link library boudaries, but I think that may be a
  Windows/MSVC-only issue.

- Use binary searches in lexer (kw and pair_) and other areas that use
  long lists of strings.  Also, use array sizes rather than
  0-termination in general.

- Use symbolic names (either enums or strings) instead of cryptic hex
  action codes.

Minor UI Improvements
---------------------

- Move search mode selection to menu, or something.  Maybe add hotkeys
  so it's easier to do.  I rarely switch, and having that big old
  menu button take up space while the search text shrinks is awful
  (maybe also remove the Search button while I'm at it, since
  pressing return does the same thing).  Plus, I"m not even sure I
  understand what all of those options do.  I only use the first and
  last.  I should read the code.  Maybe this ties into why the alpha
  buttons don't work.  Maybe make ctrl-f switch the mode to the last
  by default?

- file dialog is always GTK+.  Is there a way to make it "native QT"?
  Setting style does not affect this.

- Use SH_FormLayoutLabelAlignment to determine label aligment in form
  editor.  Maybe add a "default" alignment for card labels, as well.

- Make some of the more global look & feel preferences available in
  the preferences dialog.  Fine-tuning can still be done with style
  sheets.

- All widgets should auto-adjust based on font size.  This appears to
  be impossible to do portably with multi-line widgets (QTextEdit,
  QTreeWidget, QTableWidget) as they do not give any measure of
  border areas, padding, scrollbar size, etc.  I have made a guess
  on some widgets, and they appear to be OK, but a different style
  may screw it all up.  Either that, or stop supporting font size
  adjustments and only support full ui replacements.  I feel like
  GUI design has regressed in the last 20 years.  I made all my
  Amiga GUIs completely font-independent.  Note that I'm not talking
  about card widgets, here.  That's a different issue and much harder
  to solve.

- Add a color setter QSS similar to the layout one that only sets the
  palette rather than overriding the style as well.  Either that, or
  move some of those aux QSS settings into the preferences editor.
  Either that, or do the full .ui thing and forget supporting QSS at
  all.  QSS sucks.

- Some shortcuts, such as ^G and ^Q, should be made global.  This
  would be obsoleted by the use of .ui files.

- Named query editor, Menu editor:

    - Maybe retain column width betwen invocations, or even in prefs.
      I don't want to do the common Windows thing and make all
      positions and sizes persistent, though.

    - Add a Sort button (but how to sort?  Just screw it and use Qt's
      sort?  Or strcasecmp?  Or Unicode's collation sequences?
      Unless Qt's sort uses the latter (it's undocumented), I
      might have to pull in a Unicode library.  Probably not my
      own, though, since I don't really maintain it any more,
      and building it's a pain.) I guess sorting/collation
      sequence is an issue I will have to look into in more
      detail for grok as a whole; strcasecmp probably doesn't
      cut it. Note that this could be incorporated into the
      header, as is common.

    - Support drag-move for reordering.

- Actually look into the plan interface.  At the very least reduce its
  footprint on the form editor my making the radio group a menu.
  Maybe even make it a one-liner (i.e., menu & plan_if on one line).

    - Note that -p doesn't support Flag List or Flag Group fields in
      either mode, really, and probably never will since I don't want
      to add more fields to the menu table.

Important UI improvements
-------------------------

-   Do something about the help text.  Converting the built-in text to
    basic HTML improved the appearance a bit, so at least
    appearance-related changes can wait. It is not possible to
    globally override QWhatsThis.  Instead, I'd have to subclass
    every widget with whatsThis text and override the keyboard and
    mouse events (probably among others) to call my own routine,
    instead.  Making a pseudo-what's this cursor would probably be
    way too much trouble.

    For now, verify all help texts are mostly legible, and maybe
    convert some to HTML.  Also, verify that all help topics are
    actually present in grok.hlp, and that all topics present in
    grok.hlp are actually used.

- The manual needs cleaning up as well, but not that desperately.
  I'll probably just leave it for now, and just make sure it's
  relatively complete.  Running tidy on it causes more problems than
  it fixes.

- Add list of field names to expression grammar help text, similar to
  fields text in query editor.  Or, just make that a separate
  universal popup.  Or, add it to Database Info.

- Use QUiLoader for the main GUI.  Give every action and major widget
  a name, and map that to the user-supplied .ui file. That's
  probably the only way I'll ever be able to support Android or
  other small displays, at least: complete GUI overrides.  This will
  probably also require use of the "Qt Resource" facility to store
  the default GUI layouts in the binary.  Unfortunately, this will be
  a perfect storm of many things I hate about modern softwre:  large,
  inscrutable XML files, moc and implement-by-subclass, linear search
  by name, a GUI designer which is most decidedly not oriented towards
  making GUIs without setting nearly every paraameter to non-default,
  and much more.  Yay!  Then again, maybe I can coerce things to be a
  little tamer, like I did for the wxvbam GUI (which the OO purists at
  that project promptly spent a year trying to squash, along with
  reverting all of my attempts to make the GUI look less like it was
  designed by a 6-year-old).

- Support partly automated layout by laying out the form's widgets
  in an automatic grid, instead.  Probably also necessary in order to
  support Android or other small displays.  Maybe at least support
  automatic "line breaking" similar to the Choice/Flag Group widgets.

- Translation of GUI labels.  Maybe even an option to provide
  translation on database values (or at least database labels). This
  makes some sort of automatic layout almost necessary. Note that
  the official 1.5.3 had partial German translation, but I will
  probably want to use more standard translation methods (e.g. LANG=
  instead of a pulldown, .po[t] files)

- Support an external editor.  Or maybe make the built-in one better.
  I'm leaning on the former, since I don't want to implement a text
  editor in grok, but if there's already somthing pre-built, such as
  the scintilla support, I'll use it.  Qt's current QTextEdit requires
  work to make it usable.  It doesn't even support control characters
  in the text, so it's useless for editing "fancy" templates.

- Support recent export names in combo box.  Also, maybe support a set
  of predefined export names that are permanently stored.  Maybe
  combine this with the ability to delete history entries or make them
  persistent (a checkbox next to each) in the pref editor.

- Do something about scaling factor.  Either use it to adjust font
  sizes at the same time, or drop it entirely.  Really only relevant
  for pixel-placed widgets, anyway, which I also want to eliminate.
  
  As a stopgap instead of purely automatic layout, this could be used
  to fit the form to the current font:

  The preference no longer adjust pixel placement at all.  It only
  adjusts font size.  In fact, it could just be dropped entirely in
  favor of picking the default font size in the preferences.  However,
  it is still kept internally.

  If a preference is set, the internal scaling factor auto-adjusts
  to the current fonts (just the 5 supported fonts).  The form is
  assumed to have been designed for the defaults (in fact, perhaps
  the the form auto-unadjusts to the defaults on save, and
  verify_form checks for clipping), and when the current font
  dimensions differ, the entire GUI is scaled to compensate.  It
  only checks fonts actually used by the form.

  All storage and adjustment of coordinates is in floating point, only
  converting to integer at the last possible moment.

  At the minimum, all heights need to be adjusted to the new font
  height.  Since fonts are maybe proportional, width is a different
  matter.  Text widgets can be adjusted according to the average glyph
  width.  Labels can be adjusted according to whether or not any
  labels will end up being clipped.  Note that Print labels are
  treated like text input fields in this case, and may end up clipped.

  If that isn't enough, have a menu item or button during form
  design that performs auto-layout.  This brings up a popup showing
  the current fields and their ordering, and allows minor
  adjustments to the layout process (at least to compensate for
  widget order issues).  How the actual automatic layout works I
  have no idea.

  A slightly different approach is needed for translation, though.
  For this, just expand or shrink translated labels to the new length,
  and adjust all coordinates at or to the right of the end of the label
  by the same amount.  This should probably never shrink, even though
  ideographic languages tend to be much shorter.  Right-to-left
  languages can probably never be supported, unless I inevert the entire
  interface (but that doesn't account for the fact that right-to-left
  languages often have left-to-right sub-texts).  Then again, given
  the current user base (only 1 as far as I know), what's the point?

- The above stop-gap measure for auto-resizing deals well enough with
  the inflexibility aspect of pixel placement, but does nothing to
  address the inconvenience of pixel placement.  I really have no idea
  how best to address this, but here are a few ideas:

  - The coordinates and sizes should be visible in the form editor.
    This makes it easy to ensure alignment.  Even the demos had one
    form that misaligned the right side of one of the elemnts.  The
    current method of choosing initial size and position is good and
    convenient, but not enough.

- Have a next/previous field button, and a way to select a field by
  name and/or column and/or label in the form editor.  This probably
  also means that fields that are added or moved are re-sorted
  immediately; this has the side benefit of showing the tab order
  during form design.

- Adjust the editor canvas cursor during hover, rather than just on click.

- Support a Fiasco-like GUI:

  - Support detachable/hidable card, listing, navigation buttons,
    search.  Maybe move some of the button functions into the
    menus.  The form editor doesn't even have a menu.

  - Instead of the canvas being a separate window, replace the current
    card display with the canvas (which would auto-detach for
    convenient sizing if not already detached).  The formedit
    window would become two separate popups: a global form info
    popup (which comes up by default when not editing the current
    form) and the item-specific advanced functions.  The
    navigation buttons at the bottom would navigate between fields
    instead of records, and the Add/Delete button would also get a
    widget type and label field for quick adding of simple
    widgets.
    
    The listing could remain, as a way of setting sumcol and sumwidth
    by dragging and dropping, and maybe also editing the sortable and
    default sort flags.  The menu would remain, but be replaced
    by menu items more appropriate to editing the form.

    The search line could be replaced by a way to search for fields by
    name and/or column.

- Support multiple named summary listings (i.e., column order and
  default sort).  Also, have a way to query what fields are in that
  listing.

- Add a summary line preview (at least the headers) to the form editor
  canvas and preview.  Perhaps even allow drag & drop to move around
  and resize.  Better yet, remove the col/width fields entirely and make
  the "preview" thing the way to set those values.

- Support multiple views; i.e. multiple main windows.

- Don't make form editor a dialog, so that it doesn't get placed at
  the center of the screen.  Or, at least don't make canvas a
  dialog, so it doesn't get placed under the editor form.  It's
  annoying to have to always move the form to the side to find the
  canvas, and then to move that to the side so they don't overlap.

Infrastructure Improvements
---------------------------

- What are sections good for?  I need to figure that out.  Maybe
  it's to merge read-only data from another source with your
  database?  Any other feature I can think of is better handled by
  search expressions. I'm not sure the section feature is worth
  keeping in its current state.

- Add global pref to support regex patterns in basic search strings,
  since that was apparently part of the original plan wrt regexes.
  Right now, you have to do (match(_field,"regex")), and can't search
  across multiple fields easily like regular searches do.

- Allow preference changes (temporary or permanent) on the command
  line.  This allows the command line to be somewhat independent on
  GUI preferences.  Full independence would break backwards
  compatibility.

- Support stepping over all possible values of a field with a menu.
  Maybe ? instead of + to select Choice/flag codes and ?+ to select
  combo/label text.  To support the multi-field thing, you'd need a
  way to select the group (perhaps any field name in the group) and
  then you can step through the possible field names with ?&.

- Support non-mutable expressions.  As mentioned below, it's possible
  to do mass edits with mutating search expressions.  For example, I
  could say `(_size = 0)` to alter `size` in every single record
  (maybe even by accident).  At the very least the search string
  field should be db-non-mutable, and perhaps also
  variable-non-mutable.  In fact, only a few places should
  explicitly allow side effects of any kind in expressions.  One way
  to keep allowing side effects everywhere else would be to have an
  explicit command such as mutable to prefix any mutating
  expression, e.g. `{mutable; _name = "hello"}` or `(mutable, _size
  = 0)`.  Off the top of my head, the only things which should be
  mutable by default are the foreach non-condition expression,
  button actions, and standalone expressions in templates.  It's a
  saftey feature, not a security feature.

- Make Print widget's name refer to the label text, rather than a
  database column.  Support Print widgets in listing, expressions,
  and anywhere else a column is usually required.

-   Support UTF-8.  This was going somewhere in the IUP port, but I have
    abandoned this and it is at 0% again.  I don't like the idea of
    using QChar everywhere, and I definitely don't like the idea of
    converting to/from QByteArrays, but there's not much else I can
    do.

    At the least, if the current locale isn't UTF-8, the current
    locale should be auto-translated to UTF-8 internally, and
    converted back before writing out files.  Otherwise, a scan of
    data should make it obvious if it isn't UTF-8, and an option
    should be provided to convert to UTF-8 permanently or just
    in-memory.

    Multi-byte (but not necessarily multi-character, even if the
    desired multi-character sequence is a single glyph) delimiters
    must be supported.  Anything that scans one character at a time
    needs to be re-evaluated.

-   Database checks in verify_form().  Right now, it only validates
    the form definition itself, but not the data in the database.
    Instead, all incompatible data should be brought up in a single
    dialog with a checkbox next to each offering to convert, ingore, or
    abandon changes.  It's important to offer them all at once as some
    are interdependent.  Plus, it's annoying to have multiple popups.

    - Changing the array separator/esc should check at least data
      defined to be array values (currently Flag Lists & Flag Groups)
      and also maybe check all other data for separator/esc characters.
      Formulas just can't be checked.

    - Changing the field delimiter should offer to convert the
      existing database to the new delimiter.

    - Changing the column currently assigned to a variable should
      offer to move the data around.  All such changes should be
      offered at once, so that swapping data around works as
      intended.  Also, all unassigned columns should pop up in
      the "Debug" warning popup.  Maybe add an "ignore" item
      type to avoid such warnings.  An 'ignore" item type can be
      simulated right now using an invisible input field, but
      prior versions seemed to have wiped out such fields (or at
      least disabled fields).

    - Changing the type of field assigned to a column should prompt
      some sort of action, as well.  Especially if the column types
      are somewhat incompatible.

        - Conversion to a numeric or time/date field should check all
          values are numbers or blanks, and are in valid range

        - Conversion to a choice or flag field should, at "Done" time,
          verify database has correct values.  Same with any edit of
          codes.  Same with planned menu, radio, multi, flags fields.

    - Changing formulas should have some way of checking what
      variables they reference.  This is complicated by nested
      evals (foreach()), but even they can be checked if they
      are fixed strings.  This would require either making two
      versions of the parser or simply running the lexer and
      detecting foreach().  This sort of thing may be useful,
      anyway, to make *_if updates faster.

- Support at least some level of undo.  Right now, the best you can do
  is abandon all changes and reload.  Periodic autosave could be
  useful as well, similar to editors' recovery files.  Formatting
  the recovery file as an undo log would be ideal.

- Support some way of diffing databases.  I'm not sure how best to
  deal with the fact that there's no key, and sort order is pretty
  random.  Maybe always sort by default sort field first, at least.
  I'm also not sure how to present changes.  This will probably be way
  more work than it's worth.

- Make the standard exports also export in a format resembling the
  current form layout, like Fiasco does.  This is probably very
  hard, given the nature of the layout.  For one thing, it's
  impossible to know what the target font size will be.

- Make the HTML template generate tidy-compliant HTML.  First I'll
  have to figure out what exactly tidy wants anchors to look like,
  since it barfs on grok's HTML documentation's anchors.

- Make text/facnytext templates take a "w" flag to enable wrapping
  instead of clipping at max line length.  Note that this encroaches
  on the "probably won't fly" task of multi-line listings, but I meant
  in the GUI as well, which is much harder due to lack of native Qt
  support.  It also doesn't cover HTML, which needs a different line
  break algorithm.

- Make a generic RST exporter, and support that for printing.  Maybe
  also md/discount, but I have come to hate md.  Supporting RST for
  printing probably means some sort of configuration to set the
  command to convert it to HTML, since I don't want to parse it
  internally.

- Have a generic SQL exporter similar to the HTML one.  This may also
  involve cascading if foreign keys are in.  This is much easier to do
  and almost eliminates the need for the SQL support feature below.  It
  can be combined with a sample sqlite script to produce a grok
  database file directly, like what I used to convert my old GCStar
  database to grok.

- Make a generic R exporter, for easier creation of graphs & such.
  Interactivity can wait for dbus support.  See <http://www.iplots.org>
  and <https://www.r-graph-gallery.com/interactive-charts/>, but
  I guess R doesn't have native dbus support so I'd have to write it
  myself or use some other shim.  I suppose I could also look into GNU
  plot and Octave, or even, gasp, OpenOffice and Gnumeric.  The latter
  probably also support the glitzy 3d pie charts that R discourages (I
  remember writing one at the same job where I created form.cgi in
  support of switching to R from SAS, but that was a losing battle,
  anyway). 

- Make generic exporters for common data interchange formats, like CSV
  and json and <shudder>XML.

- Save/restore database files as tar file.  Export can be used to save
  it into a friendlier format.  Maybe also support diff (not tar -d, but
  a file-by-file smart diff).  Maybe make it zip instead, since nobody
  wants tar these days.

- Import support for formats other than manually copied raw db files.
  I guess fields would need to be selected as keys, and optionally
  some way of doing sync-like import based on field mod times (thus
  making that feature actually useful).  Perhaps rather than using a
  pair of files for this, have a combined standard export format.

- Support going directly into the form editor for invalid gf files.
  Now that I verify the form on load, there is nothing short of manual
  editing of the file that will correct problems.  Not that there
  should ever be corrupt gf files in the first place.

- Maybe this is an opportunity to make grok scriptable while running,
  like the old Amiga AREXX port.  I guess dbus would be the modern
  equivalent.  Most simple would be to just execute a random string
  and return the string result.  However, an array to retrieve the
  current search results would be nice as well.

- Use lua instead of a custom language for querying and templates and
  pretty much everything else (and a per-db startup script and
  per-record startup script).  My original idea was to use SQL for
  this, but lua is a lot easier for simple, imperative programming.
  Summary functions could be provided as a library to compensate for
  the lack of them (including "group by", which returns a table
  indexed by the grouping columns).  This does not mean that I will
  reconsider IUP for the GUI toolkit.  It does, however, mean that I
  will re-evaluate the dbus idea, since lua can just do that for me.

- Support freedesktop.org standard paths by default, at least as part
  of the search paths.  This may need to come from QStandardPaths or
  some such in order to be portable.

- Since I'm using Qt anyway, replace UNIXisms with Qt-equivalents
  where possible for portability.  Some more obvious UNIXisms are:

    - Pretty much anything that needs <unistd.h> or any other
      UNIX-specific headers: <sys/*.h>, <fcntl.h>, <pwd.h>.  This
      includes the UNIX-specific user, uid and gid, and possibly
      access.  The system call may be OK, but the actual usages
      are probably UNIX-specific.

    - Use of / as a path separator and : as a path list separator.
      Windows likes using \ and ;, although / is at least
      supported. Note that the OS/2 changes in 1.5.4 already
      addressed this a bit, but I didn't copy those changes over.

    - Installation paths (probably not an issue because the are all
      configurable in cmake, but macos and android probably require
      some work to package)

- Port to another OS.  Easiest would probably be MacOS, if it weren't
  for the fact that I have no Mac to test on and dropped my Apple
  developer ID years ago when they switched to llvm so they wouldn't
  have to give away their development kit for free any more.  Maybe
  things have changed since then to address the latter issue, but I
  don't care any more.  Windows would require the previous item, for
  starters, unless I want to call a cygwin port "Windows".  Android
  would require major surgery, even though I used that as criteria
  for choosing Qt.

Card Improvements
-----------------

-   Make Date/Time edit widgets work the way I want them to.  Until
    these issues are fixed, I have made the use of Qt date/time 
    widgets optional.  The fact that they reduce the display space
    with the arrow/spinner button(s) and don't support durations
    larger than 23:59 isn't that big a deal.

    - Maybe make the "use widgets" switch a global pref, instead

    - Qt wants to restrict selection and editing to fixed "fields"
      within the fixed-format date, disallowing entry of special
      values (like grok and form.cgi's "today"/"tomorrow"/etc.)
      or values formatted differently (form.cgi ran through a
      list of recognizable strftime formats, such as ISO
      YYYY-MM-DD and the locale default).

    - Also, numbers do not wrap: moving minutes from 59 to 00 or day
      from max(month) to 1 is simply not allowed, rather than
      incrementing the next field up.  The only field that
      partially supports wrapping is hours when in am/pm mode,
      which allows going from am to pm (but only 11a<->12p, not
      11p<->12a).

- Optionally display/retain seconds in time fields.  Maybe as a global
  preference rather than per-field, although some fields may be too
  narrow for that.

- Add per-field help text.  If present, automatically add this to the
  form-wide help, as well, similar to form.cgi's help facility.

- Rename all card widgets to their variable name, if one is assigned.
  If not, rename to the label text.  Rename the card frame to the form
  name.  This allows QSS to do form fine tuning.  On the other hand,
  QSS sucks.

- Ensure that blanking a text field won't force it to the input
  default.  Maybe have more state than just deps on/off when refilling
  the form.

- Support setting step size for numeric fields.

- I thought I read somewhere that Qt has a popup form of the
  multi-select list.  If so, support that, since it's more compact.

- Multi-select and flag group fields should support "collapsing" the
  main widget into a textual representation of selected items (either
  formula or automatic comma-separated).

- On "Done", clear all invisible field options to their default value.
  Currently, I only do that for chart options and, to a lesser extent,
  menus.

- Add "Never blank" flag (requires a default value) and a "Unique"
  flag similar to SQL.  Unique implies Never Blank, unlike SQL's
  ambiguous behavior in that regard.  I can't think of a way to
  indicate multi-field uniqueness, though, so that will have to be
  unenforced, unless the uniqueness flag is a number instead of just
  a flag.  In that case, multiple uniqueness groups can be made,
  where 0/blank indicates non-unique.  Perhaps not just numbers, but
  bits, so a list of numbers isn't needed if a field is part of
  multiple groups.

- A string expression to execute to validate a field.  Returns blank
  or a message to tell the user the field is invalid.  The validator
  should run whenever the field is done editing as well as whenever
  the record is left/about to be saved (even if the field wasn't
  edited; this way, dependent fields can validate each other)
  Although I don't like doing it this way, this is one way to do the
  non-blank/unuqueness checks instead of adding more configuration
  options.  Maybe have a way to execute the validator against all
  existing data.

Major Card Features
-------------------

- Labeled frames.  These would also always be pushed to the back in
  the canvas unless explicitly selected, and the interior within the
  form editor is transparent as well.  Come to think of it, there
  are no horizontal separator lines, labeled or not, either.

- Tabs.  These do more damage to the ability to see the entire
  record's information at a glance than anything above.  However,
  they can be useful.  You can simulate tabs right now using buttons
  that switch to other databases, but it would be nice to make it
  easier.  A few possibilities:

    - Part of the form editor: add a "tab" widget, which works as a
      tabbed window in the canvas as well.  Widgets added or moved to
      within the bounds of the tab window are added to the
      currently selected tab.

        - Alternately, take advantage of invisible_if's now dynamic
          behavior.  Just add a "hide now" button next to
	  invisible_if that hides this item and any item
	  with the exact same hide expression in the canvas.
	  That way, you can design overlapping widgets
	  without interference (and, unfortunately, without
	  guidance).  Alternatevely, add a tearable menu of
	  all invisible_if expressions for fast switching
	  between "tabs"

    - As a special form of the foreign key feature: one-to-one
      relationships.  Each tab corresponds to a child database, and
      within is displayed the full edit form of the child database,
      with parent key reference fields removed.  When a new
      parent record is created, all one-to-one child records are
      created at the same time.  This can be enforced by making
      the foreign key field itself "unique".

    - Tabs for the top part to provide alternate summaries, such as
      graphs and template execution results.  Then again,
      template execution into a pipe would take care of most of
      my use cases for this.

    The second form is sort of how you do it with buttons, except
    that you have to make the main table a "tab" as well, sort of.
    In other words, while visiting children, the parent form's
    fields are read-only.

- Add a media inset widget of some kind.  If we were on X, we could
  just make it a captured application.  I'll look into what Qt has
  to offer in that respect.  At the very least I would like a generic
  viewer for images and movies, and maybe just a play button for
  audio.  I don't want to store the binary data, though, especially
  since grok has problems with \0 in files right now.  Instead, just
  store a file name and either autodetect the file type or make the
  file type part of the field definition, as well.  This feature was
  also in the todo sample database, but it's not as though every other
  such application doesn't have one, as well (well, yeah, every other
  means just Fiasco and GCStar to me right now, as form.cgi was a
  serious application for a serious database, with no media frivolty).

- Remove the chart widget, and replace it with a generic inset widget.
  Static charts can be made using external programs like R.  Making
  them interactive might be difficult, though.  I mean, the only
  reason I have for wanting a chart is to display the distribution
  of games of different types, which is an aggregate number
  corresponding to nothing (except maybe a search string).  The
  other way to go would be to use QChartView and support whatever
  Qt supports without much thought. The more I support, the more
  per-item config options must be present. I think that maybe charts
  should be popups, either static or via QChartView in some way,
  rather than forcing them onto the static region of the card.

    - At the very least, document the current chart operation a little
      better.  The HTML docs don't even cover the chart's item
      configuration.

Major Feature: Foreign Database References
------------------------------------------

-    Make an easy way to access fields in other databases.  This is
     almost a prerequisite for foreign key support.  This makes
     no auto-save on switch a little more complicated, as
     multiple modified databases may be in play.  It is also not
     currently possible to set an alternate sort order or search
     string.  Only foreach/FOREACH have the ability to alter the
     search string temporarily, but not the search order.
     Database name is relative to same path as current database,
     or, if not there, first one in search path.  Absolute paths
     are allowed, but should be discouraged.

    - Adding a foreach parameter for the database name conflicts with
      the array foreach syntax.  I guess I could just add
      brackets around the variable to resolve this. Alternately,
      I could add something around the database name, instead,
      or use some sort of operator.  For example, @ or ::
      foreach(@"db", "search", "expr).  FOREACH can't parse the
      db name easily, though.  Either there needs to be a
      terminator as well, or the dbase expression needs to be
      limited. I guess I could only allow literal strings and
      curly brace-enclosed expresions, as long as it's easy
      enough to skip over it (curly braces can't be anywhere but
      strings and nested braces, so that might be easier than it
      sounds).

    - Maybe an additional foreach() parameter gives the sort field and
      +/- order, like foreach([@"db",] +_field[, +_field2 ...], "search").
      Supporting more than 1.5 sort fields is necessary for
      "group by" simulation, as well, so the FOREACH should
      support sort overrides, as well.

    - Allow importing templates from other databases (and this database)
      using \\{IMPORT *db* *template* *search*}.  Maybe an optional
      first parameter is a variable to load template result into for
      further text mangling.  Once again, some sort of lmits need to
      be placed on the arguments so that IMPORT can parse them.

    - Switching databases no longer forces a save.  The Save menu item
      saves if only the currently foremost visible database is modified;
      otherwise, it pops up a dialog asking which tables to save
      (default checkmarks on all).  This dialog is also popped up on
      application exit.  This is sort of unsafe, though, so maybe this
      feature also needs autosaves of some sort first (at least
      something similar to editor recovery files).

- SQL-style many-to-one foreign key references.  The "child" database
  contains a set of fields which must match a similar set of fields
  in the "parent" database.  Both for storage in the databse and in
  expressions, the value of the reference field is an array of the
  values of the referenced fields if the number of fields is more
  than one; otherwise, it is the plain value of the referenced
  field. Either way, a blank value indicates no reference, rather
  than a reference to a blank key; i.e., keys must be Never Empty
  (although multi-field references can have empty values).  Unlike
  SQL, no guarantee can be made of validity.  A menu option is
  provided to check a "child" database for bad references, with
  automatic resolution either blanking the reference or removing the
  child entirely.  Cascade deletes are only possible if the parent
  expliclitly lists child databases.  I used to want two modes:
  child-focused and parent-focused, but inatead, all children are
  child-focused, and parent databases can become parent-focused by
  adding a virtual child reference.

    - The reference is only present in the child database.  An array
      in the field definition (similar to menu) specifies the
      list of parent database fields to consider.  Each field
      may be for storage (i.e. part of the reference key),
      display, or both. Like form.cgi, a row of popup menus (one
      for each displayed field) is used to select and display
      the parent. I suppose there could also be a cutoff, after
      which selection is not possible, but instead only display
      as a label, but that adds complexity for no real
      advantage; it's not like I have to pass the entire
      database as a javascript table like I did in form.cgi,
      with cutoffs for form reloading in order to load stuff
      that didn't fit (these days I'd use AJAX-style server
      callbacks to do those instead of reloading the page, but
      that was a different era).  A text field below (optional?)
      is a search expression applied to the parent database to
      restrict what parents are selectable (although it never
      removes the current value, if any).  A button to the left
      of this text field acts as a link to the parent database,
      similar to form.cgi's label link. This switches to the
      parent form (or pops it up in a new window, if that
      functionality is available), with the child's restriction
      expression as the default search expression and the
      child's selected parent, if any, loaded into the card.
      Naturally, this requires the ability to have multiple
      forms loaded at once.

      The order of selection in the row of popups doesn't really
      matter, as long as you don't mind long lists. As with
      form.cgi, selecting a value in one column restricts the
      other columns to available values, and a special blank at
      the top undoes this selection. Note that the row of popups
      cascades; that is, if you display a foreign key field from
      the parent database, the parent's line of widgets will be
      shown instead of a single widget for that field.

      It wouldn't hurt to have an option to display headers
      above each column as well, which are the label from the
      parent database. This makes two features I never got
      around to implementing in form.cgi, the other being the
      search field which I only implemented in the search forms,
      which I am not implementing in grok.

    - In the parent form, a virtual field may be added which is
      the list of children currently pointing to it.  This is a
      scrollable table widget, with buttons below it.  The table
      widget shows the desired fields of the child database,
      with or without a header. The buttons below the list are
      to add ("+") an item or remove ("-") or edit ("=") the
      selected item in the list.  Adding or editing pops to the
      child table (or pops up a new window if that feature is
      available), but in a special parent-restricted mode.  This
      restricts all searches to include the parent, and makes
      the parent field read-only for editing and adding (where
      it is obviously initialized to the parent field value).
      Alternately, if all of the child form's fields are visible
      in the widget, the table could become directly editable,
      much like the query and menu editor tables.  I'm not sure
      how sorting should work, though.  It would have to be by
      field, rather than the up/down arrows used by the
      menu/query editors.  I think it would be best to only
      support the child form's default search field, in forward
      order.  Deleting requires thought: do I actually delete
      the child, or do I just clear out the child's reference?
      Maybe make that a field option or pop up a question
      whenever deleting.  Note that this is a feature I had planned
      for form.cgi as well, but never got around to it.

      In addition, the form has a list of "child databases" which is
      just a way of supporting cascade deletes and other features that
      need the reverse relationship.  I was going to just require
      invisible virtual fields like above, but the canvas doesn't
      really support invisible fields well, and a list of children is
      easer to store and scan.  Naturally, virtual fields' child
      databases are automatically in this list.  One way to sort of
      enforce this list would be to check the parent database in
      validate_form() of the child and auto-add it if desired.  Also,
      when removing the parent reference from the child form, offer to
      remove the parent's child references as well.

      Deleting a record in a parent table (which explicitly
      lists children, via either of the above two methods) does
      cascade deletes (i.e., all children referencing this
      record are deleted as well) and cascade foreign key
      modification (i.e., changing the key will either delete
      children or update their reference; this means that all
      child forms must be loaded at the same time to know what
      fields are actually being used as a key).  Whether a child
      is deleted or just has its reference blanked could just
      depend on whether or not the field is never-blank, or the
      delete popup gives options for what to do, defaulted to
      what it thinks is right.  Note that form.cgi disn't
      support changing the key (the key field was an invisible
      unique integer id in all tables, anyway) and only
      supported full cascade deletes (but at least told the user
      how many children were going to be deleted in the delete
      confirmation dialog, which doesn't even exist in grok).
      form.cgi had the advantage of reading the entire schema
      metadata, so it could mark parents automatically.

-     Many-to-many foreign key references.  In an SQL database, I
      would normally create a table containing reference pairs.
      This is not necessarily the most efficient way to do this
      in CSV-files like what grok uses.  Instead, the keys are
      stored as strings-as-sets (or sets of arrays for
      multi-field keys).  This means there is once again a
      "child" and "parent" database, where the "parent" knows
      nothing of the child by default.  The child and/or the
      parent presents an interface similar to the virtual child
      reference field for parents above.  The last row of the
      table is an editable row, though, and is a line of popups
      similar to the interface for child databases, above, and
      its search field is presented to the right of the normal
      virtual field buttons.  In the "parent" database, visible
      virtual references are presented the same way.  When
      popping to an edit, there are no restrictions other than
      applying the restriction, if any, to the initial query.
      Adding adds the currently selected item in the bottom
      "editable" row.  For convenience and safety, rather than
      using virtual references, the "parent" can be given a
      stored reference like the "child", making them full peers,
      but duplicating the key list in the inverse direction.
      This is yet another feature I never got around to supporting
      correctly in form.cgi, but our product didn't really have any
      such relationships, so it wasn't a big deal.

Major Feature:  SQL Support
---------------------------

- Support SQL database and form definition storage.  Support at least
  SQL-standards sqlite, and maybe also postgres.  Instead of a file
  name, a database connection string and table name are provided.
  These can be provided directly on the command-line, with a
  connection string provided by preferences or command line. Of
  course each database has its own supported types, and e.g. sqlite3
  doesn't support altering table definitions, so adding and removing
  fields requires copying the table.

    - Support storing the .gf file in the database as well; if a
      database connection string is in prefs, look for likely form
      definition tables and add them to the Database listing.

    - Support converting a table into a fairly automatically generated
      form, as long as field types are understandable.  Similar
      to my old form.cgi/genform, also read constraints for
      field type hints.  Of course table definition metadata
      access is not standardized, so each database will require
      custom-written support.  For example, genform used
      Oracle's proprietary metadata tables, and sqlite3 only
      provides the raw schema defintion.  Also, genform relied
      on some product-specific schema standards, some of which
      can't even be enforced, since they relied on Oracle
      features (like sequences and disabled constraints).  A lot
      of sqlite and mysql databases are also very sloppily
      designed, basically using unconstrained TEXT and NUMBER
      fields liberally, which are hard to handle reasonably.

    - Support many-to-many references via a helper table.  All
      reference fields become virtual parent-style fields, with
      double-indirection to get to the displayed fields.

Stuff that will probably never fly
----------------------------------

- Support automatically generated search version of form, or separate
  search form definition, similar to form.cgi's.  Basically, convert
  radio buttons to checkboxes (and form.cgi used yes/no radios instead
  of checkboxes, so convert checkboxes to tri-states) and single-select
  lists into multi-select lists, and allow basic pattern matching
  expressions in text fields.  I suppose having the field name popup
  reduces this need a bit.  It's really more restrictive and only
  slightly more convenient than using a search formula.

- Support automatically generated mass edit version of form, along
  with a way to invoke it on a particular set of rows.  Similar to
  the search form above, except that blank and unset fields are
  unmodified. Unlike form.cgi, also support formulaic updates to
  fields.  Mass edits are possible with mutating search expressions
  right now.

  Come to think of it, grok doesn't have any mass operations.  No
  multi-select in summary list, and no multi-delete, either.  Not
  even a convenient way to move a lot of records into a new section.

- Support multi-edit: a listing with all fields, similar to the query
  editor I guess.  This is probably not as useful as it appears, at
  least in part because the GUI is fast enough that switching records
  isn't that painful.  To that effect, I should add a keyboard shortcut
  for next/prev record.

- Purely automatic layout. like form.cgi.  Have globally
  configurable "column widths", maximum "row heights", default text
  widget sizes, etc.  Then, lay out in a low-density grid (form.cgi
  was 3 widgets wide, 6 if you count labels separately) and do "line
  breaks" as needed.  Unlike the List/Choice Group widgets, this
  doen't require recomputing anything about previous widgets when a
  break occurs, although I guess that could be done as well.

  Alternatively, use a "character" grid, making each widget span its
  "character length" in a grid.  This is similar to how I did
  listings in form.cgi, and is sort of how Fiasco does it (although
  it actually uses pixel placement and sizing based on the current
  font's size, instead, and movement in the canvas is via a "text
  cursor").

- Support multi-line-per-row listings, like the old form.cgi did.  I
  know nobody uses these anywhere else, but I find that to be
  superior to forcing the use of a horizontal scroll bar.  The
  "professional UI" guys had a fit with that (and most of the rest
  of form.cgi as well.  Their motto seems to be "remove anything
  that confuses me, whom this UI wasn't even designed for, with no
  idea how to replace the functionality").  Did I mention that I have
  no respect for professional UI people (or UX people, as they now
  call themselves)?

- Port to HTML/JavaScript, as I had intended with my own program.
  grok would be a standalone or fastcgi process that serves up the
  forms and also provides list filler results ("AJAX").  Then again,
  the dbus support feature, if implemented, does pretty much the same
  thing in a different format.

- Literate code.  In 2003, I switched to literate programming.  It
  takes longer, and doesn't really have all of the advantages either
  its inventor or I claim.  Almost nobody does it, and probably for
  good reason.  In fact, so few people do it, that the wikipedia
  page frequently toggles back and forth between an accurate
  definition and what modern people consider literate, which is
  nothing more than extractable documentation from code (literate
  programming is not formatting the user documentation to fit the
  code, it's formatting the code to fit the code documentation,
  which is a cohesive document rather than disjoint fragments;
  integrating user documentation is a separate issue which I
  partially address with my literate programming tools, also
  available from my bitbucket account.)

  I should at least reformat all of the code according to my own
  indentation standards, instead of following the existing code
  blindly as I tend to do when modifying someone else's code.  Maybe
  I'll run it through some automated beautifier, like I did to
  correct Odin's atrocious brace placement.
