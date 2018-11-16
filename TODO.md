Unless otherwise stated, assume 0% complete:

  - Use Qt5 instead of Motif.  This is now about 80% complete:  it
    works similarly to the old interface, but it could use some work:

    - Fix critical bugs:

      - Main window sizing issues:

        - The summary list is too wide.  The right-most column almost
	  always extends too far, and I don't know how to shrink it.

        - The window doesn't resize to a narrow version on database
	  switch (but it does do so on initial load and
	  randomly, for some reason). resize(minimumSize())
	  after adjustSize() does help, but not enough.  Even
	  "switching" to the same database produces random results.

        - When the main window resizes after a database switch, it
	  not only resizes the window, but also moves it.  It should
	  stay in one place.  Saving and restoring the position after
	  adjustSize() does nothing, so maybe it's being moved
	  elsewhere.

    - Selection in the summary window is sometimes ignored (actually,
      I think it gets reverted when it thinks the db is
      modified). The original Motif GUI did that sometimes, too.
      Also, it occasionally crashes when popping around.  Maybe
      events are being generated before having been fully
      processed?

    - As expected, letter buttons still don't work
	
    - file dialog is always GTK+.  Is there a way to make it "native QT"?

    - Memory leaks:

      - on parse error, any string yylvals leak
      
      - I should probably test the memory leak fixes I made a little
	better to ensure no crashing, but I'm too lazy.  I'll fix them
	when I encounter problems.
	  
    - Some shortcuts, such as ^G and ^Q, should be made global.

    - The layout of the form item editor needs tweaking.  At the very
      least, the chart options do not seem to have the right
      label width.  Also, I should probably use
      SH_FormLayoutLabelAlignment. Also, items in the scroll
      area are too wide, or the scroll area is too narrow,
      depending on how you look at it.

    - Add a color setter QSS similar to the layout one that only sets
      the palette rather than overriding the style as well.  Either
      that, or move some of those aux QSS settings into the preferences
      editor.  QSS sucks.

    - Make the GUI less rigid via layout helpers.  Note that I have
      run into at least one Qt anti-feature in this regard:  It is
      impossible to correctly and portably size multi-line widgets
      (list or table) by number of rows, either by setting the size or
      by forcing resize increments.  It is probably also near
      impossible to control if/when scrollbars appear.

    - Translation of GUI labels.  Maybe even an option to provide
      translation on database values (or at least database labels).
      This makes some sort of automatic layout almost necessary.
      Note that the official 1.5.3 had partial German translation,
      but I will probably want to use more standard translation
      methods (e.g. LANG= instead of a pulldown, .po[t] files)

    - Numeric inputs (with spinboxes and min/max and int/float).  I
      have already converted the numeric inputs in grok's GUI to
      spinboxes.  This requires a new field and/or flag (I prefer
      field, since the GUI is cluttered enough as is).  It is not
      possible to know if a field is currently numeric.  It's just in
      how it is used.

    - Date/Time edit popups.  Qt's QDateTime and related widgets might
      work, but they have a few flaws:  the spinner disappears if the
      date popup is present, and the spinner at least relies on the
      fixed format of the field to select which part of the date/time
      to modify.  My old HTML GUI added a time field directly to the
      calendar popup, which seems best.  I haven't experimented much
      with them, though, so maybe it's at least possible to support
      grok's parser for input, even if only to be immediately changed
      to the fixed format.

    - Make some of the more global look & feel preferences available
      in the preferences dialog.  Fine-tuning can still be done with
      style sheets.

    - Use specific pointer types instead of base classes to avoid the
      dynamic_cast overhead.  I'm not sure the compiler is smart enough
      to eliminate that itself.  In fact, it'd be nice if it were
      possible to build without rtti, but Qt adds its own rtti to
      every moc-compiled class (and has its own version of dynamic_cast),
      so that's not happening.  Well, Qt claims that it doesn't use
      rtti, but they really just mean that they use their own instead
      of the language-defined one.  They say something about crossing
      dynamic link library boudaries, but I think that may be a
      Windows/MSVC-only issue.

    - Document the "standard" Qt command-line options, especially
      given that they seem to only be documented in the
      QApplication/QGuiApplication constructor fuction documentation.

  - Remove Rambo Quit, and make Quit and window-close work like that
    instead.  This matches what most other applications do.  Also,
    make database switches (at least via the GUI) make saves optional,
    as well.  Maybe keep modified databases in memory so that
    programatic switches don't need to auto-save, either.  After an
    expression or template that does programatic switches finishes,
    pop up a message indicating that there are unsaved changes in other
    databases.

  - Minor attempts at supporting UTF-8.  This is about 20% complete.
    I don't like the idea of using QChar everywhere, and I definitely
    don't like the idea of converting to/from QByteArrays, but there's
    not much else I can do.

  - Support non-mutable expressions.  As mentioned below, it's
    possible to do mass edits with mutating search expressions.  For
    example, I could say `(_size = 0)` to alter `size` in every
    single record (maybe even by accident).  At the very least the
    search string field should be db-non-mutable, and perhaps also
    variable-non-mutable.  In fact, only a few places should explicitly
    allow side effects of any kind in expressions.  One way to keep
    allowing side effects everywhere else would be to have an explicit
    command such as mutable to prefix any mutating expression, e.g.
    `{mutable; _name = "hello"}` or `(mutable, _size = 0)`.  Off the
    top of my head, the only things which should be mutable by default
    are the foreach non-condition expression, button actions, and
    standalone expressions in templates.  It's a saftey feature, not a
    security feature.

  - Make array separator & escape char a form option (form def file, GUI)

  - Make templates' FOREACH directive understands arrays:

    - \\{FOREACH [v a} -> step over values of a, setting variable v

       Alternately, \\{FOREACH [a} \\{ELT} to avoid using a variable,
       but then it wouldn't be possible to use the element value in
       expressions.  This could be solved by using the elt special
       token as above.

    - \\{FOREACH [+v a} -> step over non-blank values of a, setting v

       Alternately, \\{FOREACH [+a} \\{ELT} as above.

  - Use binary searches in lexer (kw and pair_)

  - Cycle gadgets/popup menus/whatever the platform calls it to
    replace radio boxes.  Basically looks like a pushbutton with a
    label showing the currently selected value.  This takes up much
    less space than a radio group, and only loses the ability to
    see what other values are available at a glance.

    - Label text, Choice/flag code, shown in summary are all array
      values.  The GUI grays out these fields and instead provides a
      pushbutton to edit them in an expandable list, similar to the
      "Queries" editor.

   - Radio groups:  displayed as a grid of radio buttons surrounded by
     a frame.  Otherwise just like the above item.

   - Combo box option for text fields.  The list can be filled with
     predefined values and/or the values currently in the database.  For
     predefined values, the Input default field is overloaded as an
     array with index 0 being the actual default, and subsequent values
     being displayed in the combo dropdown.

  - Add multi-select list widgets, whose value is a set.  Otherwise,
    same as Cycle gadgets above.

    - Also, perhaps if the Databse column is an array of the correct
      length, support storing as flags after all

    - Also, perhaps if the Internal field name is an array of the
      correct length, support setting additional variables as flags

    - Also, perhaps support stepping over all possible values of a field
      whose Choice/flag code and/or Label text is an array.  Maybe ?
      instead of + to select Choice/flag codes and ?+ to select Label
      text.  To support the multi-field thing, you'd need a way to
      select the group (perhaps any field name in the group) and then
      you can step through the possible field names with ?&.  Or, only
      ? and multiple elt-bindings such as elt, eltlabel, eltvar.

  - Add checkbox group "widgets" whose value is stored in a single
    field as a set; otherwise just like Radio groups above.

  - Add "Never blank" flag (requires a default value) and a "Unique"
    flag similar to SQL.  Unique implies Never Blank, unlike SQL's
    ambiguous behavior in that regard.  I can't think of a way to
    indicate multi-field uniqueness, though, so that will have to
    be unenforced, unless the uniqueness flag is a number instead of
    just a flag.  In that case, multiple uniqueness groups can be made,
    where 0/blank indicates non-unique.  Perhaps not just numbers, but
    bits, so a list of numbers isn't needed if a field is part of
    multiple groups.

  - Have "s in s" return the location of the first string in the
    second, plus one to keep it boolean.

  - Add list of field names to expression grammar help text, similar
    to fields text in query editor.  Or, just make that a separate
    universal popup.

  - Export to window.  In fact, replace Print interface with
    automatically generated text templates, and support Export to pipe
    (e.g. 1st character is vertical bar) for "printing".  Making it a
    combo box with the defined printers would sort of compensate for
    losing those preferences, I guess.  Note that the overstrike feature
    isn't possible with current expressions, so a function to that
    effect would need to be added, unless regex stuff is done first
    (e.g. bold == `gsub(s, ".", "\\0\008\\0")`)

    - Maybe support "transient" templates, which aren't saved to files.
      This is to compensate for lack of an SQL command line.
      Problem with this is the frustration when it crashes and the
      template is lost.  So maybe not such a good idea.  It's not so
      hard to manage the files.

  - SQL-style many-to-one foreign key references.  The "child" database
    contains a set of fields which must match a similar set of fields
    in the "parent" database.  Both for storage in the databse and in
    expressions, the value of the reference field is an array of the
    values of the referenced fields if the number of fields is more than
    one; otherwise, it is the plain value of the referenced field.
    Either way, a blank value indicates no reference, rather than a
    reference to a blank key; i.e., keys must be Never Empty (although
    multi-field references can have empty values).  Unlike SQL, no
    guarantee can be made of validity.  A menu option is provided
    to check a "child" database for bad references, with automatic
    resolution either blanking the reference or removing the child
    entirely.  Cascade deletes are only possible if the parent
    expliclitly lists child tables.  I used to want two modes:
    child-focused and parent-focused, but inatead, all children
    are child-focused, and parent tables can become parent-focused
    by adding a virtual child reference.

    - The reference is only present in the child table.  Two arrays
      specify the list of fields to store and the list of fields to
      display.  A set of popup lists is presented for parent selection,
      providing several selection modes.  An optional search field below
      restricts the parents selectable in the popups.  The only other
      option would be to limit the values in other widgets when a value
      is given for one.  The order of selection doesn't really matter,
      as long as you don't mind long lists.  Note that fields cascade;
      that is, if you display a foreign key field in a parent table, the
      parent table's list of fields to display is displayed instead of
      the foreign key field's value.  In form.cgi, I didn't give
      headers for each "subfield", but I suppose it wouldn't hurt to
      add an option to do so.  Also, there should be an easy way to
      pop to the referenced parent record.  In form.cgi I made the
      label a button, but that doesn't really make sense here.
      Perhaps an additional "Edit" button.

    - In the parent table, a virtual field may be added which is the
      list of children currently pointing to it.  This is a scrollable
      list widget, with buttons below it.  The list widget shows the
      desired fields of the child table, with or without a header.
      The buttons below the list are to add ("+") an item, or remove
      ("-") or edit ("=") the selected item in the list.   Adding or
      editing pops up a new top-level window similar to the main, but
      restricted to a parent.  When restricted to a parent, the
      display of the foreign key field is read-only, and searches are
      forcibly limited to that value as well.  Adding and editing a
      selected record just pop up the standard add and edit fields.
      Editing without first selecting something pops up the default
      no-record form.  In addition, this field can be invisible.  In
      either case, deleting a record with such a field does cascade
      deletes (i.e., all children referencing this record are deleted
      as well) and cascade foreign key modification (i.e., changing
      the key will either delete children or update their referene).

  - Many-to-many foreign key references.  In an SQL database, I
    would normally create a table containing reference pairs.
    This is not necessarily the most efficient way to do this in
    CSV-files like what grok uses.  Instead, the keys are stored
    as strings-as-sets (or sets of arrays if multi-field).  This
    means there is once again a "child" and "parent" table, where
    the "parent" knows nothing of the child by default.  The child
    and/or the parent presents an interface similar to the child,
    but instead of a popup selection list, it is a static
    seelction list, possibly with multi-select.  In the "parent"
    table, visible virtual references are presented the same way
    as in the child, but lists children instead.  When popping to
    an edit, rather than restricted mode, it sets the initial
    search to the selected item(s) and picks the first for
    editing.  For adding, the field itself needs to be initially
    selecting the source "parent".  For convenience, rather than
    using "parent" style virtual references, the "parent" can be
    made a "child" field as well, essentially duplicating the key list
    but in the inverse direction.

  - Support automatically generated search version of form, or
    separate search form definition, similar to form.cgi's.  I suppose
    having the field name popup reduces this need a bit.

  - Have a generic SQL exporter similar to the HTML one.  This may also
    involve cascading.

  - Support SQL database and form definition storage.  Support at
    least SQL-standards sqlite, and maybe also postgres.  Instead of a
    file name, a database connection string and table name are
    provided.  These can be provided directly on the command-line,
    with a connection string provided by preferences or command line.
    Of course each database has its own supported types, and e.g.
    sqlite3 doesn't support altering table definitions, so adding and
    removing fields requires copying the table.

    - Support storing the .gf file in the database as well; if a
      database connection string is in prefs, look for likely form
      definition tables and add them to the Databse listing.

    - Support converting a table into a fairly automatically generated
      form, as long as field types are understandable.  Similar to my
      old form.cgi, also read constraints for field type hints.  Of
      course each database has its own way of providing access to table
      definition metadata, so this is a little complicated.  For
      example, form.cgi used Oracle's proprietary metadata tables, and
      sqlite3 only provides the raw schema defintion.

    - Support many-to-many references via a helper table.  All reference
      fields become virtual parent-style fields, with double-indirection
      to get to the dsiplayed fields.

  - Labeled frames.  These would also always be pushed to the back in
    the form editor GUI unless explicitly selected, and the interior
    within the form editor is transparent as well.  Come to think of
    it, there are no horizontal separator lines, labeled or not, either.

  - Tabs.  These do more damage to the ability to see the entire
    record's information at a glance than anything above.
    However, they can be useful.  You can simulate tabs right now
    using buttons that switch to other databases, but it would be
    nice to make it easier.  A few possibilities:

    - Part of the form editor:  add a "tab" widget, which works as
      a tabbed window in the editor window as well.  Widgets added
      within the bounds of the tab window are added to the currently
      selected tab.
      
      - Alternately, take advantage of invisible_if's now dynamic
        behavior.  Just add a "hide now" button next to
	invisible_if that hides this item and any item with
	the exact same hide expression in the canvas.  That
	way, you can design overlapping widgets without
	interference (and, unfortunately, without guidance).
	Alternatevely, add a tearable menu of all invisible_if
	expressions for fast switching between "tabs"

    - As a special form of the foriegn key feature:  one-to-one
      relationships.  Each tab corresponds to a child table, and within
      is displayed the full edit form of the child table, with parent
      key reference fields removed.  When a new parent record is
      created, all one-to-one child records are created at the same
      time.  This can be enforced by making the foreign key field
      itself "unique".

    - Tabs for the top part to provide alternate summaries, such as
      graphs and template execution results.  Then again, template
      execution into a pipe would take care of most of my use cases
      for this.

    The second form is sort of how you do it with buttons, except that
    you have to make the main table a "tab" as well, sort of.  In
    other words, while visiting children, the parent form's fields are
    read-only.

  - A string expression to execute to validate a field.  Returns
    blank or a message to tell the user the field is invalid.  The
    validator should run whenever the field is done editing as well as
    whenever the record is left/about to be saved (even if the field
    wasn't edited; this way, dependent fields can validate each other)
    Although I don't like doing it this way, this is one way to do the
    non-blank/unuqueness checks instead of adding more configuration
    options.  Maybe have a way to execute the validator against all
    existing data.

  - Save/restore database as tar file.  Maybe look into the
    (nonexistent?) sync feature a little more.

  - Support at least some level of undo.  Right now, the best you can do
    is Rambo-Quit and reload.  There isn't even a way to prevent saving
    when switching databases, which would be hard to work around given
    that only one can be loaded at a time.

  - Actually fix some of those buffer overflow bugs, instead of just
    cranking the buffer size.  Some of them already disappeared due to
    the use of QString as the target for sprintf, but some still remain.

  - Add a summary line preview (at least the headers) to the form editor
    canvas and preview

  - Add a media inset widget of some kind.  If we were on X, we could
    just make it a captured application.  I'll look into what Qt has
    to offer in that respect.

  - Do something about scaling factor.  Either use it to adjust font
    sizes at the same time, or drop it entirely.  Really only relevant
    for pixel-placed widgets, anyway, which I also want to eliminate.

  - Remove the chart widget, and replace it with a generic inset widget.
    Static charts can be made using external programs like R.  Making
    them interactive might be difficult, though.  I mean, the only
    reason I have for wanting a chart is to display the distribution
    of games of different types, which is an aggregate number
    corresponding to nothing (except maybe a search string).  The
    other way to go would be to use QtChartView and support whatever
    Qt supports without much thought.  The more I support, the more
    per-item config options must be present.  I think that maybe charts
    should be popups, either static or via QChartView in some way,
    rather than forcing them onto the static region of the card.

  - Maybe this is an opportunity to make grok scriptable while
    running, like the old Amiga AREXX port.  I guess dbus would be the
    modern equivalent.  Most simple would be to just execute a random
    string and return the string result.  However, an array to retrieve
    the current search results would be nice as well.

  - Use lua instead of a custom language for querying and templates and
    pretty much everything else (and a per-db startup script and
    per-record startup script).  My original idea was to use SQL
    for this, but lua is a lot easier for simple, imperative
    programming.  Summary functions could be provided as a library
    to compensate for the lack of them (including "group by",
    which returns a table indexed by the grouping columns).  This does
    not mean that I will reconsider IUP for the GUI toolkit.

  - Use whatever Qt provides for designer-designed GUIs.  Probably give
    every action and major widget a name, and map that to the
    user-supplied GUI file.  That's probably the only way I'll ever be
    able to support Android or other small displays, at least:  complete
    GUI overrides.  This will probably also require use of the "Qt
    Resource" facility to store the default GUI layouts in the binary.

  - Support automated layout by sorting the form's widgets and laying
    them out in a grid, instead.  Probalby also necessary in order to
    support Android or other small displays.

  - Port to another OS.  Easiest would probably be Mac, if it weren't
    for the fact that I have no Mac to test on and dropped my Apple
    developer ID years ago when they switched to llvm so they wouldn't
    have to give away their development kit for free any more.  Maybe
    things have changed since then to address the latter issue, but I
    don't care any more.  Windows would require replacing all
    UNIX-isms (pretty much all uses of unistd.h, at the very least).
    Android would require major surgery, even though I used that as
    criteria for choosing Qt.

After all (or even most) of the above changes, grok will be almost
what I wanted to do anyway.  The only missing items are mass edit
(sort of possible using mutating expressions in the search
expression), multi-edit (probably not as useful as it appears), purely
automatic layout (or at the very least character-based layout like
Fiasco) and literate, clean code.  Most of the other features I had
planned were probably pointless, anyway.
