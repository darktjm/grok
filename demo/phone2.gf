grok
name       phone
dbase      phone
comment    Phone directory,  modified by Steve Hughes from form by thomas@bitrot.in-berlin.de
cdelim     :
rdonly     0
proc       0
grid       4 4
size       737 338
divider    36
autoq      6
help	'Phone directory. Put the primary phone number in the "Phone" field, which
help	'will be put into the summary above the card. Company phone numbers can go
help	'into the "Company" field. You may want to want to turn on the "Letter search
help	'checks all words" mode in the Preferences menu to search for all initials.
help	'The "Call log" button switches to the Call Log database.
query_s    0
query_n    Business
query_q    {_group == "b"}
query_s    0
query_n    Personal
query_q    {_group == "p"}
query_s    0
query_n    Corps
query_q    {_group == "c"}
query_s    0
query_n    University
query_q    {_group == "u"}
query_s    0
query_n    Travel
query_q    {_group == "t"}
query_s    0
query_n    Other
query_q    {_group == "o"}
query_s    0
query_n    Quick List
query_q    {_quick == "qy"}
query_s    0
query_n    People with email
query_q    {_email}
query_s    0
query_n    People with fax
query_q    {_fax}

item
type       Button
name       call_log
pos        596 0
size       128 28
mid        64 0
sumwid     0
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Call log
ljust      2
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     0
ijust      0
ifont      4
p_act      {switch("phonelog", "{_last=='"._last."'}")}
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       last
pos        12 48
size       352 28
mid        92 28
sumwid     15
sumcol     1
column     0
search     1
rdonly     0
nosort     0
defsort    1
timefmt    0
code       
codetxt    
label      Last Name
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       first
pos        376 48
size       264 28
mid        80 28
sumwid     15
sumcol     2
column     1
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      First Name
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       title
pos        644 48
size       84 28
mid        40 28
sumwid     0
sumcol     0
column     2
search     0
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Title
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       position
pos        12 80
size       352 28
mid        92 28
sumwid     0
sumcol     0
column     3
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Position
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       company
pos        376 80
size       352 28
mid        80 24
sumwid     0
sumcol     0
column     4
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Company
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       work
pos        12 112
size       248 28
mid        92 28
sumwid     15
sumcol     3
column     5
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Work Phone
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       email
pos        272 112
size       456 28
mid        68 24
sumwid     0
sumcol     0
column     8
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Email
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       home
pos        12 144
size       248 28
mid        92 28
sumwid     15
sumcol     4
column     6
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Home Phone
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       address
pos        272 144
size       456 28
mid        68 28
sumwid     0
sumcol     0
column     9
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Address
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       fax
pos        12 176
size       248 28
mid        92 24
sumwid     0
sumcol     0
column     7
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Fax
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       city
pos        272 176
size       256 28
mid        68 28
sumwid     0
sumcol     0
column     10
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      City
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       state
pos        532 176
size       80 28
mid        44 28
sumwid     0
sumcol     0
column     11
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      State
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       zip
pos        616 176
size       112 28
mid        32 28
sumwid     0
sumcol     0
column     12
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Zip
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Label
name       qlist
pos        12 212
size       84 20
mid        56 16
sumwid     0
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Quick List:
ljust      2
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       quick
pos        100 212
size       72 20
mid        64 20
sumwid     0
sumcol     0
column     15
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       qy
codetxt    
label      Yes
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    f
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       quick
pos        176 212
size       60 20
mid        60 20
sumwid     0
sumcol     0
column     15
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       qn
codetxt    
label      No
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    f
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       country
pos        272 208
size       256 28
mid        68 28
sumwid     0
sumcol     0
column     13
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Country
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    United States
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Time
name       date
pos        536 208
size       192 28
mid        92 28
sumwid     0
sumcol     0
column     17
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      "Plan" Date
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
plan_if    t
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Label
name       glabel
pos        68 236
size       92 20
mid        56 16
sumwid     0
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Group:
ljust      2
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       pnote
pos        248 240
size       480 28
mid        92 28
sumwid     0
sumcol     0
column     18
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      "Plan" Note
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
plan_if    n
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       group
pos        12 260
size       96 20
mid        64 20
sumwid     10
sumcol     0
column     14
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       b
codetxt    Business
label      Business
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    b
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       group
pos        116 260
size       96 20
mid        64 20
sumwid     10
sumcol     0
column     14
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       u
codetxt    University
label      University
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    b
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Label
name       lab2
pos        228 276
size       56 20
mid        56 16
sumwid     0
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Notes:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       group
pos        12 284
size       96 20
mid        64 20
sumwid     10
sumcol     0
column     14
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       p
codetxt    Personal
label      Personal
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    b
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       group
pos        116 284
size       96 20
mid        64 20
sumwid     10
sumcol     0
column     14
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       t
codetxt    Travel
label      Travel
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    b
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       group
pos        12 308
size       96 20
mid        64 20
sumwid     10
sumcol     0
column     14
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       c
codetxt    Corps
label      Corps
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    b
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       group
pos        116 308
size       96 20
mid        64 20
sumwid     10
sumcol     0
column     14
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       o
codetxt    Other
label      Other
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    b
pattern    
minlen     0
maxlen     0
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Note
name       note
pos        288 272
size       440 64
mid        92 0
sumwid     0
sumcol     0
column     16
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Note
ljust      2
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     10000
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
ch_xrange  0 0
ch_yrange  0 0
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0
