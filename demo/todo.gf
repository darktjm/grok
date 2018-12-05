grok
name       todo
dbase      todo
comment    todo list. Author: thomas@bitrot.in-berlin.de
cdelim     :
size       681 427
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
label      Title
lfont      0
maxlen     80
ifont      4

item
type       Input
name       project
pos        12 44
size       196 28
mid        80 28
sumwid     6
column     0
search     1
code       
codetxt    
label      Project
lfont      0
gray       
freeze     
invis      
skip       
default    {_project[last]}
maxlen     10
ifont      4
p_act      

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
code       
codetxt    
label      Assigned to
lfont      0
gray       
freeze     
invis      
skip       
default    {user}
maxlen     40
ifont      4
p_act      

item
type       Time
name       begin
pos        472 44
size       196 28
mid        80 28
sumwid     8
sumcol     4
column     6
code       
codetxt    
label      Begin
lfont      0
gray       {_status == "i"}
freeze     
invis      
skip       
default    
maxlen     10
ifont      4
p_act      

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
code       
codetxt    
label      ID number
lfont      0
gray       
freeze     
invis      
skip       
default    (max(_id)+1)
maxlen     10
ifont      4
p_act      

item
type       Input
name       reported_by
pos        244 76
size       196 28
mid        80 28
column     4
search     1
code       
codetxt    
label      Reported by:
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     40
ifont      4
p_act      

item
type       Input
name       days
pos        472 76
size       196 28
mid        80 28
sumwid     3
sumcol     5
column     7
code       
codetxt    
label      Days
lfont      0
gray       {_status == "i"}
freeze     
invis      
skip       
default    
maxlen     10
ifont      4
p_act      

item
type       Input
name       priority
pos        12 108
size       196 28
mid        80 28
sumwid     2
sumcol     2
column     2
code       
codetxt    
label      Priority
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     4
ifont      4
p_act      

item
type       Time
name       reported_on
pos        244 108
size       196 28
mid        80 28
column     5
search     1
code       
codetxt    
label      Reported on:
lfont      0
gray       
freeze     
invis      
skip       
default    (date)
maxlen     10
ifont      4
p_act      

item
type       Input
name       fixversion
pos        472 108
size       196 28
mid        80 28
column     8
code       
codetxt    
label      Fix Version:
lfont      0
gray       {_status == "i"}
freeze     
invis      
skip       
default    
maxlen     10
ifont      4
p_act      

item
type       Label
name       statusl
pos        12 140
size       76 28
mid        52 28
column     0
code       
codetxt    
label      Status:
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     0
ifont      4
p_act      

item
type       Choice
name       status
pos        92 140
size       72 28
mid        52 28
sumwid     6
sumcol     6
column     9
code       u
codetxt    urgent
label      Urgent
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      4
p_act      

item
type       Choice
name       status
pos        168 140
size       64 28
mid        52 28
sumwid     6
sumcol     6
column     9
code       w
codetxt    todo
label      Todo
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      4
p_act      

item
type       Choice
name       status
pos        236 140
size       96 28
mid        52 28
sumwid     6
sumcol     6
column     9
code       p
codetxt    progress
label      In progress
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      4
p_act      

item
type       Choice
name       status
pos        336 140
size       76 28
mid        52 28
sumwid     6
sumcol     6
column     9
code       t
codetxt    test
label      In test
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      4
p_act      

item
type       Choice
name       status
pos        416 140
size       88 28
mid        52 28
sumwid     6
sumcol     6
column     9
code       f
codetxt    finished
label      Finished
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      4
p_act      

item
type       Choice
name       status
pos        508 140
size       84 28
mid        52 28
sumwid     6
sumcol     6
column     9
code       d
codetxt    deferred
label      Deferred
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      4
p_act      

item
type       Choice
name       status
pos        596 140
size       72 28
mid        52 28
sumwid     6
sumcol     6
column     9
code       i
codetxt    ignore
label      Ignore
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      4
p_act      

item
type       Note
name       report
pos        12 180
size       656 236
mid        56 16
column     11
search     1
code       
codetxt    
label      Report:
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     10000
ifont      4
p_act      
