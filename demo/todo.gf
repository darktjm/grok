grok
name       todo
dbase      todo
comment    todo list. Author: thomas@bitrot.in-berlin.de
cdelim     :
rdonly     0
proc       0
grid       4 4
size       681 427
divider    0
autoq      4
help	'Things to do. You may want to extend the form by adding Bug Version fields
help	'using the form editor, but make sure you do that before using New to start
help	'entering data for the first time. This form may change in future versions;
help	'charts could be used to plot tasks on a time axis.
query_s    0
query_n    Urgent unfinished tasks
query_q    {_status == "u"}
query_s    0
query_n    Unfinished tasks
query_q    ({_status == "u"} || {_status == "w"} || {_status == "t"} || {_status == "t"})
query_s    0
query_n    Tasks in progress
query_q    {_status == "p"}
query_s    0
query_n    Tasks in test
query_q    {_status == "t"}
query_s    0
query_n    Unfinished tasks assigned to me
query_q    (({_status == "u"} || {_status == "w"} || {_status == "t"} || {_status == "t"}) && {_assigned == user})
query_s    0
query_n    Unassigned tasks
query_q    {_assigned == ""}
query_s    0
query_n    Unscheduled tasks
query_q    (_begin == 0 || _days == 0)

item
type       Input
name       title
pos        12 12
size       656 28
mid        80 28
sumwid     80
sumcol     7
column     10
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Title
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     80
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       project
pos        12 44
size       196 28
mid        80 28
sumwid     6
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Project
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    {_project[last]}
pattern    
minlen     1
maxlen     10
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       assigned
pos        244 44
size       196 28
mid        80 28
sumwid     6
sumcol     3
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Assigned to
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    {user}
pattern    
minlen     1
maxlen     40
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Time
name       begin
pos        472 44
size       196 28
mid        80 28
sumwid     8
sumcol     4
column     6
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Begin
ljust      0
lfont      0
gray       {_status == "i"}
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       id
pos        12 76
size       196 28
mid        80 28
sumwid     4
sumcol     1
column     1
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      ID number
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    (max(_id)+1)
pattern    
minlen     1
maxlen     10
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       reported_by
pos        244 76
size       196 28
mid        80 28
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
label      Reported by:
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     40
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       days
pos        472 76
size       196 28
mid        80 28
sumwid     3
sumcol     5
column     7
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Days
ljust      0
lfont      0
gray       {_status == "i"}
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       priority
pos        12 108
size       196 28
mid        80 28
sumwid     2
sumcol     2
column     2
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Priority
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     4
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Time
name       reported_on
pos        244 108
size       196 28
mid        80 28
sumwid     0
sumcol     0
column     5
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Reported on:
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    (date)
pattern    
minlen     1
maxlen     10
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       fixversion
pos        472 108
size       196 28
mid        80 28
sumwid     0
sumcol     0
column     8
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Fix Version:
ljust      0
lfont      0
gray       {_status == "i"}
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Label
name       statusl
pos        12 140
size       76 28
mid        52 28
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
label      Status:
ljust      0
lfont      0
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       status
pos        92 140
size       72 28
mid        52 28
sumwid     6
sumcol     6
column     9
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       u
codetxt    urgent
label      Urgent
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       status
pos        168 140
size       64 28
mid        52 28
sumwid     6
sumcol     6
column     9
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       w
codetxt    todo
label      Todo
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       status
pos        236 140
size       96 28
mid        52 28
sumwid     6
sumcol     6
column     9
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       p
codetxt    progress
label      In progress
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       status
pos        336 140
size       76 28
mid        52 28
sumwid     6
sumcol     6
column     9
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       t
codetxt    test
label      In test
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       status
pos        416 140
size       88 28
mid        52 28
sumwid     6
sumcol     6
column     9
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       f
codetxt    finished
label      Finished
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       status
pos        508 140
size       84 28
mid        52 28
sumwid     6
sumcol     6
column     9
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       d
codetxt    deferred
label      Deferred
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       status
pos        596 140
size       72 28
mid        52 28
sumwid     6
sumcol     6
column     9
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       i
codetxt    ignore
label      Ignore
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Note
name       report
pos        12 180
size       656 236
mid        56 16
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
label      Report:
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0
