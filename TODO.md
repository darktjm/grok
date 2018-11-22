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

> -- Thomas J. Moore

Features in Progress
--------------------

- Add all new field types to verify_form()

- Support editing the "menu" field (IT_INPUT combo box static values)
  in a query editor-like dynamic table.  Same goes for flagcode and
  flagtext for the other menu-style widgets.  Either the fields are
  grayed out and a popup button is added to edit the list, or the
  query-editor-like GUI is inserted in-place, and grows and shrinks
  automatically (since I don't want scrollbar items in a panel that is
  already scrollable).

- Numeric fields should also support step adjustment.  At least verify
  that setting digits sets step adjustment to 10^-digits.

- For multi-select lists and checkbox groups, perhaps support storing
  individual flags by having an additional array field of columns.
  This might interfere with the common ->column usage, though.  The
  same comment applies if I want to have individual flag variables,
  as well.  It's best to just force users to test with set
  intersection.  Perhaps I'll revisit this if I get SQL working,
  since having an infinitely resizable field is not such a good idea
  there.  Note that if I support flag variables and flog storage
  separately, I'd have to also modify the field get/set routines to
  correclty extract/store the values; best to support all or nothing.
  This should also provide array semantics for summary column & width.
  One compromise for the latter would be to have a flag to display a
  column for every possible value, each at the same width.

- Automatically make the flag code equal to the label if blank.

- Support stepping over all possible values of a field whose
  Choice/flag code and/or combo text is an array.  Maybe ? instead
  of + to select Choice/flag codes and ?+ to select combo/label
  text.  To support the multi-field thing, you'd need a way to
  select the group (perhaps any field name in the group) and then
  you can step through the possible field names with ?&.  Or, only ?
  and multiple elt-bindings such as elt, eltlabel, eltvar.

Bugs
----

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

- The layout of the form item editor needs tweaking.  At the very
  least, the chart options do not seem to have the right label
  width.  Also, items in the scroll area are too wide, or the scroll
  area is too narrow, depending on how you look at it, but only if
  the cart options appear. Otherwise, the opposite issue exists.  It
  appears that the layout is partially being influenced by invisible
  widgets.

- As expected, letter buttons still don't work. The first press works,
  but subsequent presses go all weird.

- on parse error, any string yylvals leak

- I should probably test the memory leak fixes I made a little better
  to ensure no crashing, but I'm too lazy.  I'll fix them when I
  encounter problems.

- Look more closely at all Qt-related licenses.  I don't want this
  program to be GPL.  In particular, QUiLoader seems to use a
  different license (or at least Trolltech thought it important
  enough to list the license in the introductory text).

- Note fields are currently not able to limit maximum length.

- Actually fix some of those buffer overflow bugs, instead of just
  cranking the buffer size.  Some of them already disappeared due to
  the use of QString as the target for sprintf, but some still
  remain.

Code Improvements
-----------------

- Die with message on all memory allocation failures.  I could add
  checks after every new one I added, but instead, I'll make a wrapper
  that does it for me.

- Since I'm using C++ anyway, convert BOOL, TRUE and FALSE to their
  C++ lower-case equivalents.  C supports them too, but with too many
  underscores.

- Consistently make blank strings NULL, and stop checking for blanks.

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

- Put database name and modified flag in title bar

- file dialog is always GTK+.  Is there a way to make it "native QT"?

- Use SH_FormLayoutLabelAlignment to determine label aligment in form
  editor.

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

- Named query editor:

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

Important UI improvements
-------------------------

- Do something about the help text.  Either make my own What's This?
  override or format the help text in a way that the built-in What's
  this displays better (perhaps even rich text).  Also, verify that all
  help topics are actually present in grok.hlp, and that all topics
  present in grok.hlp are actually used.

- Do something about the manual.  I actually prefer a latex manual,
  which could be converted to HTML if desired.  The current HTML
  manual needs to run through tidy, and be lower-case in general.
  Then, I need to actually read it, and verify that it contains all
  the necessary information.  Then, I need to add screenshots,
  probably.  What would really be cool is if I could generate the
  full manual, help text, and man page from the same source, but
  I've been trying that for years with my literate stuff and it's
  just not practical.

- Document the "standard" Qt command-line options, especially given
  that they seem to only be documented in the
  QApplication/QGuiApplication constructor documentation.

- Add list of field names to expression grammar help text, similar to
  fields text in query editor.  Or, just make that a separate
  universal popup.

- Remove Rambo Quit, and make Quit and window-close work like that
  instead.  This matches what most other applications do.  Also,
  make database switches (at least via the GUI) make saves optional,
  as well. Since button actions are the only way other than the menu
  to switch databases, this should be prtty easy.

- Date/Time edit popups.  Qt's QDateTime and related widgets might
  work, but they have a few flaws: the spinner disappears if the
  date popup is present, and the spinner at least relies on the
  fixed format of the field to select which part of the date/time to
  modify.  My old HTML GUI added a time field directly to the
  calendar popup, which seems best.  I haven't experimented much
  with them, though, so maybe it's at least possible to support
  grok's parser for input, even if only to be immediately changed to
  the fixed format.

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

- Support partly automated layout by sorting the form's widgets and
  laying them out in a grid, instead.  Probably also necessary in
  order to support Android or other small displays.  Maybe at least
  support automatic "line breaking" similar to the Choice/Flag Group
  widgets.

- Translation of GUI labels.  Maybe even an option to provide
  translation on database values (or at least database labels). This
  makes some sort of automatic layout almost necessary. Note that
  the official 1.5.3 had partial German translation, but I will
  probably want to use more standard translation methods (e.g. LANG=
  instead of a pulldown, .po[t] files)

- Export to window.  In fact, replace Print interface with
  automatically generated text templates, and support Export to pipe
  (e.g. 1st character is vertical bar) for "printing".  Making it a
  combo box with the defined printers would sort of compensate for
  losing those preferences, I guess.  Note that the overstrike
  feature can be done with the new regex functions, .g. bold ==
  `gsub(s, ".", "\\0\008\\0")`)

    - Maybe support "transient" templates, which aren't saved to
      files. This is to compensate for lack of an SQL command
      line. Problem with this is the frustration when it crashes
      and the template is lost.  So maybe not such a good idea.
      It's not so hard to manage the files.  On the other hand,
      a history could be saved to disk, just like readline
      support in the SQL command-line tools.

        - While it's easy enough to force the use of files in the GUI,
	  it might be nice to either read a template from
	  stdin or give it as a (very long) command-line
	  option, like I do with sed all the time.  Grok was
	  always supposed to be a command-line query tool as
	  well, and this allows more control over the output
	  format.

- Do something about scaling factor.  Either use it to adjust font
  sizes at the same time, or drop it entirely.  Really only relevant
  for pixel-placed widgets, anyway, which I also want to eliminate.

- Remove width limits in summary list.

- Support multiple named summary listings.  Also, have a way to query
  what fields are in that listing.

Infrastructure Improvements
---------------------------

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

- Either disallow backslash as a field separator or allow separate
  specification of the escape character, like I did for arrays.
  Perhaps also support some of the other CSV conventions, like
  optional quotes around values and an optional header line.

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

  - Database checks in verify_form().  Right now, it only validates
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
  is Rambo-Quit and reload.  There isn't even a way to prevent
  saving when switching databases, which would be hard to work
  around given that only one can be loaded at a time.  Periodic
  autosave could be useful as well, similar to editors' recovery
  files.  Formatting the recovery file as an undo log would be ideal.

- Have "s in s" return the location of the first string in the second,
  plus one to keep it boolean.  Or, just leave it alone and let
  people who want this use match() instead.

- Have a generic SQL exporter similar to the HTML one.  This may also
  involve cascading if foreign keys are in.  This is much easier to do
  and almost eliminates the need for the SQL support feature below.  It
  can be combined with a sample sqlite script to produce a grok
  database file directly, like what I used to convert my old GCStar
  database to grok.

- Save/restore database files as tar file.  Export can be used to save
  it into a friendlier format.  Maybe also support diff (not tar -d, but
  a file-by-file smart diff).  Maybe make it zip instead, since nobody
  wants tar these days.

- Import support for formats other than manually copied raw db files.
  I guess fields would need to be selected as keys, and optionally
  some way of doing sync-like import based on field mod times (thus
  making that feature actually useful).  Perhaps rather than using a
  pair of files for this, have a combined standard export format.

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
  reconsider IUP for the GUI toolkit.

- Support freedesktop.org standard paths by default, at least as part
  of the search paths.  This may need to come from QStandardPaths or
  some such in order to be portable.

- Since I'm using Qt anyway, replace UNIXisms with Qt-equivalents
  where possible for portability.  Some more obvious UNIXisms are:

    - Pretty much anything that needs <unistd.h> or any other
      UNIX-specific headers: <sys/*.h>, <fcntl.h>, <pwd.h>

    - Use of / as a path separator and : as a path list separator.
      Windows likes using \ and ;, although / is at least
      supported. Note that the OS/2 changes in 1.5.4 already
      addressed this a bit.

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

- Rename all card widgets to their variable name, if one is assigned.
  If not, rename to the label text.  Rename the card frame to the form
  name.  This allows QSS to do form fine tuning.  On the other hand,
  QSS sucks.

- Ensure that blanking a text field won't force it to the input
  default.  Maybe have more state than just deps on/off when refilling
  the form.

- I thought I read somewhere that Qt has a popup form of the
  multi-select list.  If so, support that, since it's more compact.

- Multi-select and choice group fields should support "collapsing" the
  main widget into a textual representation of selected items (either
  formula or automatic comma-separated).

- On "Done", clear all invisible fields to their default value.
  Currently, I only do that for chart options.

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
  the form editor GUI unless explicitly selected, and the interior
  within the form editor is transparent as well.  Come to think of
  it, there are no horizontal separator lines, labeled or not,
  either.

- Tabs.  These do more damage to the ability to see the entire
  record's information at a glance than anything above. However,
  they can be useful.  You can simulate tabs right now using buttons
  that switch to other databases, but it would be nice to make it
  easier.  A few possibilities:

    - Part of the form editor: add a "tab" widget, which works as a
      tabbed window in the editor window as well.  Widgets added
      within the bounds of the tab window are added to the
      currently selected tab.

        - Alternately, take advantage of invisible_if's now dynamic
          behavior.  Just add a "hide now" button next to
	  invisible_if that hides this item and any item
	  with the exact same hide expression in the canvas.
	  That way, you can design overlapping widgets
	  without interference (and, unfortunately, without
	  guidance). Alternatevely, add a tearable menu of
	  all invisible_if expressions for fast switching
	  between "tabs"

    - As a special form of the foreign key feature: one-to-one
      relationships.  Each tab corresponds to a child table, and
      within is displayed the full edit form of the child table,
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
  to offer in that respect.

- Add a summary line preview (at least the headers) to the form editor
  canvas and preview.  Perhaps even allow drag & drop to move around
  and resize.

- Remove the chart widget, and replace it with a generic inset widget.
  Static charts can be made using external programs like R.  Making
  them interactive might be difficult, though.  I mean, the only
  reason I have for wanting a chart is to display the distribution
  of games of different types, which is an aggregate number
  corresponding to nothing (except maybe a search string).  The
  other way to go would be to use QtChartView and support whatever
  Qt supports without much thought. The more I support, the more
  per-item config options must be present. I think that maybe charts
  should be popups, either static or via QChartView in some way,
  rather than forcing them onto the static region of the card.

Major Feature: Foreign Database References
------------------------------------------

- Make an easy way to access fields in other databases.  This is
  almost a prerequisite for foreign key support.  This makes the
  above item a little more complicated, as multiple modified
  databases may be in play.  It is also not currently possible to
  set an alternate sort order or search string.  Only
  foreach/FOREACH have the ability to alter the search string
  temporarily, but not the search order. Database name is relative
  to same path as current database, or, if not there, first one in
  search path.  Absolute paths are allowed, but should be
  discouraged.

    - Maybe an additional foreach() parameter gives the sort field and
      +/- order, like foreach("x", +_field[, +_field2 ...]).
      Supporting more than 1.5 sort fields is necessary for
      "group by" simulation, as well, so the \FOREACH{} should
      support sort overrides, as well.

    - Maybe similar to foreach(), have a function that executes an
      expression in the context of a new database.  Like foreach(),
      return values must be set in variables or other side effects;
      indb(dbname, search, expr).  Or, add an optional first
      argument prefixed by a char, like the sort thing above:
      foreach(:"databse"[, "search"], "expr")

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
  expliclitly lists child tables.  I used to want two modes:
  child-focused and parent-focused, but inatead, all children are
  child-focused, and parent tables can become parent-focused by
  adding a virtual child reference.

    - The reference is only present in the child table.  Two arrays
      specify the list of fields to store and the list of fields
      to display.  A set of popup lists is presented for parent
      selection, providing several selection modes.  An optional
      search field below restricts the parents selectable in the
      popups.  The only other option would be to limit the
      values in other widgets when a value is given for one.
      The order of selection doesn't really matter, as long as
      you don't mind long lists.  Note that fields cascade; that
      is, if you display a foreign key field in a parent table,
      the parent table's list of fields to display is displayed
      instead of the foreign key field's value.  In form.cgi, I
      didn't give headers for each "subfield", but I suppose it
      wouldn't hurt to add an option to do so.  Also, there
      should be an easy way to pop to the referenced parent
      record.  In form.cgi I made the label a button, but that
      doesn't really make sense here. Perhaps an additional
      "Edit" button.

    - In the parent table, a virtual field may be added which is the
      list of children currently pointing to it.  This is a
      scrollable list widget, with buttons below it.  The list
      widget shows the desired fields of the child table, with
      or without a header. The buttons below the list are to add
      ("+") an item, or remove ("-") or edit ("=") the selected
      item in the list.  Adding or editing pops up a new
      top-level window similar to the main, but restricted to a
      parent.  When restricted to a parent, the display of the
      foreign key field is read-only, and searches are forcibly
      limited to that value as well.  Adding and editing a
      selected record just pop up the standard add and edit
      fields. Editing without first selecting something pops up
      the default no-record form.  In addition, this field can
      be invisible.  In either case, deleting a record with such
      a field does cascade deletes (i.e., all children
      referencing this record are deleted as well) and cascade
      foreign key modification (i.e., changing the key will
      either delete children or update their referene).

- Many-to-many foreign key references.  In an SQL database, I would
  normally create a table containing reference pairs. This is not
  necessarily the most efficient way to do this in CSV-files like
  what grok uses.  Instead, the keys are stored as strings-as-sets
  (or sets of arrays if multi-field).  This means there is once
  again a "child" and "parent" table, where the "parent" knows
  nothing of the child by default.  The child and/or the parent
  presents an interface similar to the child, but instead of a popup
  selection list, it is a static seelction list, possibly with
  multi-select.  In the "parent" table, visible virtual references
  are presented the same way as in the child, but lists children
  instead.  When popping to an edit, rather than restricted mode, it
  sets the initial search to the selected item(s) and picks the
  first for editing.  For adding, the field itself needs to be
  initially selecting the source "parent".  For convenience, rather
  than using "parent" style virtual references, the "parent" can be
  made a "child" field as well, essentially duplicating the key list
  but in the inverse direction.

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
      definition tables and add them to the Databse listing.

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
  with a way to invoke it on a particular set of rows.  Similar to the
  search form above, except that blank and unset fields are unmodified.
  Unlike form.cgi, also support formulaic updates to fields.  Mass
  edits are currently possible with mutating search expressions right
  now.

- Support multi-edit: a listing with all fields, similar to the query
  editor I guess.  This is probably not as useful as it appears, at
  least in part because the GUI is fast enough that switching records
  isn't that painful.  To that effect, I should add a keyboard shortcut
  for next/prev record.

-   Purely automatic layout. like form.cgi.  Have globally
    configurable "column widths", maximum "row heights", default
    text widget sizes, etc.  Then, lay out in a low-density grid
    (form.cgi was 3 widgets wide, 6 if you count labels
    separately) and do "line breaks" as needed.  Unlike the
    List/Choice Group widgets, this doen't require recomputing
    anything about previous widgets when a break occurs, although
    I guess that could be done as well.

    Alternatively, use a "character" grid, making each widget span its
    "character length" in a grid.  This is similar to how I did listings
    in form.cgi, and is sort of how Fiasco does it (although it
    actually uses pixel placement and sizing based on the current font's
    size, instead, and movement in the canvas is via a "text cursor").

- Support multi-line-per-row listings, like the old form.cgi did.  I
  know nobody uses these anywhere else, but I find that to be
  superior to forcing the use of a horizontal scroll bar.  The
  "professional UI" guys had a fit with that (and most of the rest
  of form.cgi as well.  Their motto seems to be "remove anything
  that confuses me, whom this UI wasn't even designed for, with no
  idea how to replace the functionality").  Did I mention that I have
  no respect for professional UI people (or UX people, as they now
  call themselves)?

-   Literate code.  In 2003, I switched to literate programming.  It
    takes longer, and doesn't really have all of the advantages
    either its inventor or I claim.  Almost nobody does it, and
    probably for good reason.  In fact, so few people do it, that the
    wikipedia page frequently toggles back and forth between an
    accurate definition and what modern people consider literate,
    which is nothing more than extractable documentation from code
    (literate programming is not formatting the user documentation to
    fit the code, it's formatting the code to fit the code
    documentation, which is a cohesive document rather than disjoint
    fragments; integrating user documentation is a separate issue
    which I partially address with my literate programming tools,
    also available from my bitbucket account.)

    I should at least reformat all of the code according to my own
    indentation standards, instead of following the existing code
    blindly as I tend to do when modifying someone else's code.  Maybe
    I'll run it through some automated beautifier, like I did to
    correct Odin's atrocious brace placement.
