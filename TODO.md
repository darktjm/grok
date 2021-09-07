I'm working on grok because I'm too lazy to continue working on my
own, similar application, and it's easier to be motivated when the
application already works, mostly.  However, grok needs a lot of work,
still.  Some of that work could have been done back when xmbase_grok
was still fresh, but I wasn't interested at the time.  After all (or
even most) of the below changes, grok will be almost what I wanted to
do anyway. Most of the other features I had planned were probably
pointless, anyway.  Actually, most of the features I have planned below
are pointless, as well.  I'm not sure I'll be able to remain motivated
long enough to make much of a dent in this list.  I started on grok
for my game database, and I guess I'd like to get back to playing
games again...

As a side note: I'm starting to hit limits in grok that I can't fix
without breaking backwards+forwards compatibility.  Maybe it's time
for 3.0, which no longer makes any such guarantees.  I suppose I could
provide a conversion program to move old databases to 3.0, but nothing
for the opposite conversion.  Here are some of the issues that i may
completely break in 3.0:

  - formulas; this is probably the hardest to convert, since foreach()
    acts as an eval().  I will replace this with Lua or SQL.

  - Date storage, entry and presentation

  - File format.  I may just chuck csv entirely and go for sqlite3, or
    at least I'll no longer worry about supporting 8-bit character sets
    (and use UTF-8 or UTF-16 instead).

    - Sections need to get a reason to live or go.

    - Dates need to be stored differently, as mentioned above

    - Sort order of on-disk data no longer matches last sort before
      save, so I've already got an incompatible change, there (albeit
      easy to undo).

				-- Thomas J. Moore

Features in Progress
--------------------

Bugs
----

- Click on list with card update changes selection.  This is due to
  re-sort sometimes, but other times who knows?

- Sometimes displayed card and selected summary line mismatch, still

- Sometimes 1st widget of card appears even though no record is
  selected in main card window display.  Speaking of which, the static
  area doesn't really work any differently from the bottom area, as far
  as I can tell, even though the docs say otherwise, so maybe I need
  to revisit that.

- When the main window resizes after a database switch, it not only
  resizes the window, but also moves it.  It should stay in one
  place.  Saving and restoring the position after adjustSize() does
  nothing, so maybe it's being moved elsewhere.  This may well be a
  window manager issue, though; I'm one of the few people who still
  uses FVWM, which I have been using for over 20 years now.  Some
  weirdnesses only apply to FVWM.

  I have debugged the other sizing issues, but have no patience for
  this one.  If anyone else ever complains about it, I might look
  into it again.

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

- The help popup for questions immediately gets pushed behind question
  popup.  The same is true for multi-save and any other modal dialogs.
  If possible, maybe I should make the help dialog temporarily modal
  itself.  Or, I could just force a What's This? display, maybe.

- Invalid long options pop up the usage() after the fork.  Fixing this
  without disabling fork entirely is probably never going to happen,
  because QApplication silently dies if it's run before the fork.

- Sometimes form editor crashes when writing form def, causing it to
  be wiped out -- I should write to temporary file and then move on success

- Error during FOREACH loop on export double frees query results

- [Verify] fkey card structures must be non-graphical, or a form
  reload might realloc() and invalidate pointers

- Fkey forms' path should be relative to referring form's path

- Some fkey data validity checks' suggested fixes untested

- Little testing of fkey_multi or multi-field keys

- Changing db of fkey field spews errors on stderr.

Code Improvements
-----------------

- Stop passing/storing dbase when form passed/stored as well.  Stop
  passing/storing form when item passed/stored as well.

- constify *everything*

  - remove cheats, and at the very least use C++'s cumbersome const_cast.

- protect all file I/O with EAGAIN/EINTR loops

- On all file writes, fflush before fclose, and report on ferror().
  Normally I'd stop writing as soon as an error occurs, but that's too
  much trouble and probably not worth it.  Maybe make backups before
  overwriting and delete when successfully written.

- Check errors on all file reads, and report/abort if so.  I suppose
  that delaying until just before fclose would be sufficient.

- Have form_clone() fail gracefully and free its results on failure,
  aborting the form edit instead of killing grok.  In general,
  failures during form editing should just kill the form editor.

  On the other hand, the form editor should also auto-save and support
  resuming from auto-save.

- Identify abortable processes and catch all C++ exceptions to abort
  the process instead of the entire program.  In particular,
  expression evaluation and template processing should abort rather than
  kill grok.  Similarly, the new abort-on-alloc-failure code isn't
  as forgiving as it should be.

- Consistently make blank strings NULL, and stop checking for blanks.
  In particular, use zstrdup instead of mystrdup in contexts where
  NULL is acceptable.

- Be more consistent on use of 0 vs NULL for pointers.  Yeah, my own
  additions are probably even more inconsistent than the old code was.
  The advantage of NULL is that it only works in pointer contexts.
  The advantage of 0 is that it's shorter to write.

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
  long lists of strings.  Hash tables are too much trouble, though.

- Use static array sizes rather than 0-termination in general.

- Maybe split some callbacks into multiple functions, since they don't really
  share code.

Minor UI Improvements
---------------------

- Maybe add hotkeys for search mode so it's easier to do.  Maybe remove
  the Search button since pressing return does the same thing.  Plus, I"m
  not even sure I understand what all of those modes do (probably at
  least because widen was broken, and the entire widget isn't even documented
  outside of grok.hlp and HISTORY).  I only use the first and last.

  It would be best to have a full stack of query strings, since data
  changes make the simplistic use of a mask broken (as does the letter
  code in general).

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

  - Maybe retain column width betwen invocations, or even in
    prefs. I don't want to do the common Windows thing and make
    all positions and sizes persistent, though.

  - Add a Sort button (but how to sort?  Just screw it and use
    Qt's sort?  Or strcasecmp?  Or Unicode's collation sequences?
    Unless Qt's sort uses the latter (it's undocumented), I might
    have to pull in a Unicode library.  Probably not my own,
    though, since I don't really maintain it any more, and
    building it's a pain.) I guess sorting/collation sequence is
    an issue I will have to look into in more detail for grok as a
    whole; strcasecmp probably doesn't cut it. Note that this
    could be incorporated into the header, as is common.

  - Support drag-move for reordering.

- fkey search restriction text field: a search expression applied to
  the parent database to restrict what parents are selectable (although
  it never removes the current value, if any).  Clicking the label link
  adds this search expression to the other table by default as well.
  For multi and inv tables, this also filters what's displayed.

  But it really isn't necessary for fkey fields, as initial string
  search is supported by Qt combobox widgets, and you shouldn't have
  a huge db, anyway.

  It also really isn't necessary for inv_fkey fields, as parent
  restricted mode allows you to search among the listing.

- auto-delete of refered-to row if reference changed or removed
  (cascade delete).  May never implement this, as it might become
  annoying to ask user every time.  For now, (!referenced) will return
  true if a row is an "orphan" in need of deletion.

- auto-adjust of key field names if changed in foreign db (cascade
  key field definition).

- auto-adjust/clear/delete of key values in other databases if
  key changes (cascade key field modification)

- Actually look into the plan interface.  At the very least reduce its
  footprint on the form editor my making the radio group a menu.
  Maybe even make it a one-liner (i.e., menu & plan_if on one line).

  - Note that -p doesn't support Flag List or Flag Group fields in
    either mode, really, and probably never will since I don't
    want to add more fields to the menu table.

- freeze_if should probably be as dynamic as invisible_if.  Currently,
  some widgets change to a different form if read-only, so they need to
  be switched between forms.  In fact, the form it uses is determined at
  card window creation time, so readonly is very non-dynamic.

- Directly editable table for inv_fkey or fkey_multi

- All loaded databases should be in the menu regardless of whether or not
  they normally would be.

Important UI improvements
-------------------------

- Do something about the help text.  Converting the built-in text to
  basic HTML improved the appearance a bit, so at least
  appearance-related changes can wait.  It is not possible to
  globally override QWhatsThis.  Instead, I'd have to subclass every
  widget with whatsThis text and override the keyboard and mouse
  events (probably among others) to call my own routine, instead.
  Making a pseudo-what's this cursor would probably be way too much
  trouble.

- The manual needs cleaning up as well, but not that desperately.
  I'll probably just leave it for now, and just make sure it's
  relatively complete.  Running tidy on it causes more problems than
  it fixes.

- Add a <name>.ru file for recently used things.  Right now, this
  should include search expressions and export file names.  In
  addition, maybe move other "recently used" preferences into the .ru
  file, like the selected export and print options.  Also, for
  searches, maybe have a combo box for selecting search expressions
  that includes the recently used ones (probably difficult given the
  nature of the table editing).

- Support recent export names in combo box.  Also, maybe support a set
  of predefined export names that are permanently stored.  Maybe
  combine this with the ability to delete history entries or make them
  persistent (a checkbox next to each) in the pref editor.

- Sort by expression: add a menu item below sort fields to pop up a
  string requester; last expression is then added to menu.  In fact,
  keep a history of expressions, and have a preference window popup
  that allows assigning names to them for permanent presence in the
  menu, and in any case keep 5-10 last expressions in the <name>.ru
  file described above.  Also, in the new @db stuff, if the sort field
  starts with ( or { interpret as a string/numeric expression.

- No sorting really for fkey tables.  Should probably try to sort by
  foreign table's default sort order, or by first displayed field.
  Actually, multi-fkey sorted by key set order.

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
  instead of a pulldown, .po[t] files).  Qt provides its own
  translation routines that are, of course, completely different from
  what I expect.  I either have to invest in Qt's method or write my
  own, non-portable method.  I suppose I could use LGPL GNU gettext,
  if it is sufficient.  Otherwise, I'm stuck with Qt's method.  Does
  Qt even provide tools for message extraction?

- Support an external editor.  Or maybe make the built-in one better.
  I'm leaning on the former, since I don't want to implement a text
  editor in grok, but if there's already somthing pre-built, such as
  the scintilla support, I'll use it.  Qt's current QTextEdit requires
  work to make it usable.  It doesn't even support control characters
  in the text, so it's useless for editing "fancy" templates.  Also,
  the editor should query and restore file permissions if possible.

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

- Don't make form editor a dialog, so that it doesn't get placed at
  the center of the screen.  Or, at least don't make canvas a
  dialog, so it doesn't get placed under the editor form.  It's
  annoying to have to always move the form to the side to find the
  canvas, and then to move that to the side so they don't overlap.

- Support multiple views; i.e. multiple main windows.  So far, it's
  possible to have multiple card structures, each of which supports its
  own sort order and query.  For now, don't worry too much about the
  same row being edited in multiple windows at once, although a change
  should probably at least force a refresh in all other windows.

  Perhaps as a later feature, support use of tabs instead of separate
  windows.  I don't really care for MDI, so I don't think I'll do that.

  Also, in the style of emacs, retain search and sort criteria of
  previous view on dbase switch.  Currently, I discard the card, and
  only retain the dbase & form if the dbase is modified or cached.
  
  Popping up a cardwin meets some of the needs.

Infrastructure Improvements
---------------------------

- Add support for multiple template flags in a single condition.  For example,
  using operators |& and parentheses [] (() and {} are already used for
  full expressions).

- Allow importing templates from other databases (and this database)
  using \\{IMPORT *db* *template* *search*}.  Maybe an optional
  first parameter is a variable to load template result into for
  further text mangling, or auto-mangling with a set of regex
  parameters and their associated replacements.  Once again, some
  sort of limits need to be placed on the arguments so that IMPORT
  can parse them.

- Change disk storage of dates to number of seconds since epoch.  Main
  issues I can see are that external tools may depend on the format
  and that I don't want to have to deal with time zones.  While the
  original code always used the current time zone, it didn't really
  matter because it was stored as plain text.  Storing it internally
  as gmtime shouldn't be an issue, unless the user made formulas that
  crossed DST boundaries.  I guess grok dates suck in general, and
  maybe it would be best to rename Time fields to OldTime and start
  over from scratch.

- What are sections good for?  I need to figure that out.  The HISTORY
  file entry didn't really say why the feature was added.  Here's a few
  guesses:

  - Merging read-only or shared data from another source with your own
    database.  This is one aspect I can't see a valid replacement for.
    However, I can see the need being rare enough that procedural
    databases could perform the merge for you.

  - Large RDBMSes have a "partitioning" feature, but that is for
    storing parts on different disks (not useful for small personal
    databases) and improved search/sort performance when dealing with
    data only in a limited number of partitions (not implemented in
    grok anyway; regardless of the on-disk structure, in-memory is
    always the entire database as a single array).

  - There are functions which deal with sections, such as section and
    ssum, but they could be better implmeented as an explicit field
    in the database.  It has always been possible to do alternate
    searches using foreach(), and now the dbase prefix means you don't
    even have to go through the hassle of that.

  - External programs that access the database files directly.  This
    shouldn't really be supported; instead, people should be encouraged
    to use templates.  However, there is no equivalent import feature,
    so maybe I'll wait until I do something about that to wipe this
    feature.

  None of the above reasons seem compelling for such an invasive
  feature.  I suppose that I couuld at least hide the section menu
  if there are no sections, and move the "Add Section" option to the
  File menu.  That way, people are discouraged from using this.

- Add global pref to support regex patterns in basic search strings,
  since that was apparently part of the original plan wrt regexes.
  Right now, you have to do (match(_field,"regex")), and can't search
  across multiple fields easily like regular searches do.

- Allow preference changes (temporary or permanent) on the command
  line.  This allows the command line to be somewhat independent of
  GUI preferences.  Full independence would break backwards
  compatibility.

- Support stepping over all possible values of a field with a menu.
  Maybe ? instead of + to select Choice/flag codes and ?+ to select
  combo/label text.  To support the multi-field thing, you'd need a
  way to select the group (perhaps any field name in the group) and
  then you can step through the possible field names with ?&.

- Support non-mutable expressions.  As mentioned below, it's
  possible to do mass edits with mutating search expressions.  For
  example, I could say `(_size = 0)` to alter `size` in every single
  record (maybe even by accident).  At the very least the search
  string field should be db-non-mutable, and perhaps also
  variable-non-mutable (or semi-variable-non-mutable: you can assign
  to variables, but they are reverted to the original value after
  the search).  In fact, only a few places should explicitly allow
  side effects of any kind in expressions.  One way to keep allowing
  side effects everywhere else would be to have an explicit command
  such as mutable to prefix any mutating expression, e.g. `{mutable;
  _name = "hello"}` or `(mutable, _size = 0)`.  Off the top of my
  head, the only things which should be mutable by default are the
  foreach non-condition expression, button actions, and standalone
  expressions in templates.  It's a saftey feature, not a security
  feature.

- Make Print widget's name refer to the label text, rather than a
  database column.  Support Print widgets in summary, expressions,
  auto-template full listings, search, sort, fkey-visible.

- support for inv_fkey in auto-templates, summary, expressions,
  search, sort, fkey-visible.

- Support UTF-8.  I don't like the idea of using QChar everywhere, and
  I definitely don't like the idea of converting to/from QByteArrays,
  but there's not much else I can do.  I don't even know if QChar is
  32-bit (it/s advertised as 16-bit, but it might still support
  32-bit values with manual surrogate split/merge; this only matters for
  data retention and character class checks).

  At the least, if the current locale's encoding isn't UTF-8, the
  current encoding should be auto-translated to UTF-8 internally, and
  converted back before writing out files.  Otherwise, a scan of
  data should make it obvious if it isn't UTF-8, and an option
  should be provided to convert to UTF-8 permanently or just
  in-memory.

  Multi-byte (but not necessarily multi-character, even if the
  desired multi-character sequence is a single glyph) delimiters
  must be supported.  Anything that scans one character at a time
  needs to be re-evaluated.

  On the other hand, Qt uses 16-bit everywhere, and if I ever start
  using Qt functions instead of POSIX functions, I'll have to use it
  anyway, so maybe I'll use UCS-16 internally.  Even if I don't do
  that, supporting UTF-16 files with an explicit BOM as their first
  character seems like a good idea.

  My idea of using iconv for auto-conversion will probably not work.
  iconv expects a charset string, and I can only get such a string
  from POSIX nl_langinfo(CODESET), which isn't on non-POSIX systems.
  There are half-assed implementations for Windows, but I don't know
  if Android is POSIX, and in the end, it's more trouble than it's
  worth.  Note that although GNU gettext comes with one of those
  half-assed replacements, it's part of the GPL code, and as such
  unusable, since I have no intention of making grok GPL.  Qt's
  character set conversion routines don't seem very useful at first
  glance.

  Of course the C library also has mbtowc and wctomb, but it does
  not define what a wc is (i.e., what encoding), so I don't think
  that's usable, either.  It's so nice that all the code to do the
  conversions is there, but it's all unusable.

- Database checks in verify_form().  Right now, it only validates
  the form definition itself, but not the data in the database.
  Instead, all incompatible data should be brought up in a single
  dialog with a checkbox next to each offering to convert, ingore,
  or abandon changes.  It's important to offer them all at once as
  some are interdependent.  Plus, it's annoying to have multiple
  popups.

  - Changing the array separator/esc should check at least data
    defined to be array values (currently Flag Lists & Flag
    Groups) and also maybe check all other data for separator/esc
    characters. Formulas just can't be checked.

  - Changing the field delimiter should offer to convert the
    existing database to the new delimiter.

  - Changing the column currently assigned to a variable should
    offer to move the data around.  All such changes should be
    offered at once, so that swapping data around works as
    intended.  Also, all unassigned columns should pop up in the
    "Check" warning popup.  Maybe add an "ignore" item type to
    avoid such warnings.  An 'ignore" item type can be simulated
    right now using an invisible input field, but prior versions
    seemed to have wiped out such fields (or at least disabled
    fields).

  - Changing the type of field assigned to a column should prompt
    some sort of action, as well.  Especially if the column types
    are somewhat incompatible.

    - Conversion to a numeric or time/date field should check
      all values are numbers or blanks, and are in valid range

    - Conversion to a choice or flag field should, at "Done"
      time, verify database has correct values.  Same with any
      edit of codes.  Same with menu, radio, multi, flags fields.

  - Changing formulas should have some way of checking what
    variables they reference.  This is complicated by nested evals
    (foreach()), but even they can be checked if they are fixed
    strings.  This would require either making two versions of the
    parser or simply running the lexer and detecting foreach().
    This sort of thing may be useful, anyway, to make *_if updates
    faster.

- Support at least some level of undo.  Right now, the best you can do
  is abandon all changes and reload.  Periodic autosave could be
  useful as well, similar to editors' recovery files.  Formatting
  the recovery file as an undo log would be ideal.  For an interesting
  take on using the underlying database for this once everything is
  stored there, see <sqlite3 doc>/html/undoredo.html.  In fact,
  something like the CSV module for SQLite3 could probably be used to
  make the old files "work" with it.

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

- Make text/fancytext templates take a "w" flag to enable wrapping
  instead of clipping at max line length.  Note that this encroaches
  on the "probably won't fly" task of multi-line listings, but I meant
  in the GUI as well, which is much harder due to lack of native Qt
  support.  It also doesn't cover HTML, which needs a different line
  break algorithm.

- Add a 't' flag to add timestamps to the auto-generated templates.

- Make a generic RST exporter, and support that for printing.  Maybe
  also md/discount, but I have come to hate md.  Supporting RST for
  printing probably means some sort of configuration to set the
  command to convert it to HTML, since I don't want to parse it
  internally.

- Have a flag for the SQL exporter to split or merge
  multicol(-capable) fields.

- Generate an sqlite3 script to convert the databse created by the
  automatic exporter into a grok database file directly (or via an
  additional sed script, if needed), similar to how I initially
  imported my games db from GCStar.

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

- Import support for formats other than manually created raw db files.
  I guess fields would need to be selected as keys (or mass replace
  the results of a query, including all records).  Sync-like import
  could be supported as well, but that requires some sort of key as
  well as a way to specify the import file's time stamps.  Also, add
  a way to query time stamps in expressions, so they can be exported.
  I don't think a generic pattern-based importer would be a good idea,
  as it would be way too much effort and be limited by the kinds of
  supported patterns.  At worst, fully document the db file format and
  officially support raw db file access (which would preculde such
  cnages as storing dates as numbers or other disruptive format
  changes).  That at least needs to be partially done anyway, as the
  raw db format is expected to/from procedural databases.

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
  
  - using lua from SQL would be more difficult without native db
    support.  ODBC separates you from the database too much.
    
    - sqlite3: https://github.com/abiliojr/sqlite-lua
    - postgres: https://github.com/pllua/pllua
    - mysql, firebird have no such equivalents atm, so have to use UDF/UDR,
      which is complex and mostly undocumented in both.  No, examples
      do not serve as documentation.

- Support freedesktop.org standard paths by default, at least as part
  of the search paths.  This may need to come from QStandardPaths or
  some such in order to be portable.

- Since I'm using Qt anyway, replace UNIXisms with Qt-equivalents
  where possible for portability.  Some more obvious UNIXisms are:

  - Pretty much anything that needs <unistd.h> or any other
    UNIX-specific headers: <sys/*.h>, <fcntl.h>, <pwd.h>.  This
    includes the UNIX-specific user, uid and gid, and possibly
    access.  The system call may be OK, but the actual usages are
    probably UNIX-specific.

  - Use of / as a path separator and : as a path list separator.
    Windows likes using \ and ;, although / is at least supported.
    Note that the OS/2 changes in 1.5.4 already addressed this a
    bit, but I didn't copy those changes over.

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

- Make Date/Time edit widgets work the way I want them to.  Until
  these issues are fixed, I have made the use of Qt date/time
  widgets optional.  The fact that they reduce the display space
  with the arrow/spinner button(s) and don't support durations
  larger than 23:59 isn't that big a deal.

  - Maybe make the "use widgets" switch a global pref, instead

  - Qt wants to restrict selection and editing to fixed "fields"
    within the fixed-format date, disallowing entry of special
    values (like grok and form.cgi's "today"/"tomorrow"/etc.) or
    values formatted differently (form.cgi ran through a list of
    recognizable strftime formats, such as ISO YYYY-MM-DD and the
    locale default).

  - Also, numbers do not wrap: moving minutes from 59 to 00 or day
    from max(month) to 1 is simply not allowed, rather than
    incrementing the next field up.  The only field that partially
    supports wrapping is hours when in am/pm mode, which allows
    going from am to pm (but only 11a<->12p, not 11p<->12a).

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
    tabbed window in the canvas as well.  Widgets added or moved
    to within the bounds of the tab window are added to the
    currently selected tab.

    - Alternately, take advantage of invisible_if's now dynamic
      behavior.  Just add a "hide now" button next to
      invisible_if that hides this item and any item with the
      exact same hide expression in the canvas. That way, you
      can design overlapping widgets without interference (and,
      unfortunately, without guidance).  Alternatevely, add a
      tearable menu of all invisible_if expressions for fast
      switching between "tabs"

  - As a special form of the foreign key feature: one-to-one
    relationships.  Each tab corresponds to a child database, and
    within is displayed the full edit form of the child database,
    with parent key reference fields removed.  When a new parent
    record is created, all one-to-one child records are created at
    the same time.  This can be enforced by making the foreign key
    field itself "unique".

  - Tabs for the top part to provide alternate summaries, such as
    graphs and template execution results.  Then again, template
    execution into a pipe would take care of most of my use cases
    for this.

  The second form is sort of how you do it with buttons, except that
  you have to make the main table a "tab" as well, sort of. In other
  words, while visiting children, the parent form's fields are
  read-only.

- Add a password widget, possibly just as a flag for Input widgets.
  It's only really useful if the underlying storage encrypts, so
  maybe also support a per-form encryption method, using openssl
  (EVP) or the like.  Not sure how to get the passphrase, though.
  Prompting every time might be annoying, and there is some sort of
  "standard" session keyring that the web browsers use.  Maybe
  qtkeyring, since I'm using qt anyway?  Password fields should
  never be stored unencrypted in memory, and be obscured (both value
  and length) in the GUI. Perhaps have a "show" checkbox to show the
  value (or use a label link like I did for fkey fields), after
  giving the encryption password again and also hiding it again
  after a configurable timeout.  The encrypted value would be stored
  as base64-encoded string to avoid grok's inability to deal with 0s.
  
  Note that if I'm going to add per-field encyrption anyway, I might
  also consider full-file encyrption (or per-row encryption, to allow
  seeking in the file).  The app isn't really designed securely enough
  for that, though.

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
  serious application for a serious database, with no media frivolity).

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

  - At the very least, document the current chart operation a
    little better.  The HTML docs don't even cover the chart's
    item configuration.

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

  - Support converting a table into a fairly automatically
    generated form, as long as field types are understandable.
    Similar to my old form.cgi/genform, also read constraints for
    field type hints.  Of course table definition metadata access
    is not standardized, so each database will require
    custom-written support.  For example, genform used Oracle's
    proprietary metadata tables, and sqlite3 only provides the raw
    schema defintion.  Also, genform relied on some
    product-specific schema standards, some of which can't even be
    enforced, since they relied on Oracle features (like sequences
    and disabled constraints).  A lot of sqlite and mysql
    databases are also very sloppily designed, basically using
    unconstrained TEXT and NUMBER fields liberally, which are hard
    to handle reasonably.

  - Support many-to-many references via a helper table.  All
    reference fields become virtual parent-style fields, with
    double-indirection to get to the displayed fields.

- Support connection strings with missing values, such as passwords.
  These are prompted for at startup for at least as many connection
  strings as necessary to load the first database.  Canceling the dialog
  skips to the next db.  A menu item allows reconnecting/reprompting.

  - For ODBC, use a ? prefix to indicate some of the parameters can be
    changed.  For those, use a syntax similar to SQLBrowseConnect, but
    with support for password indicators:
      conn str = ?parm[;parm...]
      parm = [*]var[:prompt]=vals
      vals = [*]? | val[:prompt][,val[:prompt]...]
    * in front of parm means optional; * in front of ? means password.
    If a keyring is available, use the same var name to look up key.
    The substituted connection string is only used for connection and
    maybe substitution into db init SQL, and then wiped and freed (or
    maybe just altered so that passwords are converted back to *?).
    Not sure how ODBC connection strings are meant to be quoted
    - note that SQLBrowseConnect() and SQLDriverConnect() with prompting
      are both unusable.  The latter isn't implemented for non-Windows
      for any of the drivers I've tried, and the former is only on
      Firbird.  Even then, the former doesn't distinguish password-like
      fields from others, so you have to guess that PWD/Password is the
      only one.
  - for SQL init string, use %-enclosed names for parameter subst (%% = %).
    Since conn. parms needn't be valid, extra info can be substituted.
    For example:  init sqlite "pragma key='%EncKey%'".  Maybe have a
    :-separated specifier instead; 1st elt is format (e.g. '-escaped
    for EncKey above).

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
