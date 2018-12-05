grok
name       workplan
dbase      workplan
cdelim     :
size       685 241
autoq      1
query_s    0
query_n    Unfinished
query_q    ({_status!="o"} && {_status!="d"})
query_s    0
query_n    Important unfinished
query_q    ({_status!="o"} && {_status!="d"} && _priority < 3)
query_s    0
query_n    Missed deadlines
query_q    ({_status!="o"} && {_status!="d"} && _priority < 3 && {_end!=""} && _end < date)
query_s    0
query_n    Remaining deadlines
query_q    ({_status!="o"} && {_status!="d"} && _priority < 3 && _end >= date)
query_s    0
query_n    Priority 1
query_q    (_priority==1)
query_s    0
query_n    Priority 2
query_q    (_priority==2)
query_s    0
query_n    No length or deadline
query_q    ({_status!="o"} && {_status!="d"} && (_days==0 || {_end==""}))

item
type       Input
name       id
pos        12 12
size       104 28
mid        52 28
sumwid     4
column     0
label      ID
ljust      1
lfont      1
default    (max(_id)+1)
maxlen     10
ifont      0

item
type       Input
name       name
pos        120 12
size       112 28
mid        44 28
sumwid     6
sumcol     1
column     1
search     1
code       
codetxt    
label      Name
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Input
name       project
pos        236 12
size       124 28
mid        52 28
sumwid     6
sumcol     2
column     2
search     1
code       
codetxt    
label      Project
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Input
name       priority
pos        364 12
size       92 28
mid        56 28
sumwid     1
sumcol     3
column     3
defsort    1
code       
codetxt    
label      Priority
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    2
maxlen     10
ifont      0
p_act      

item
type       Input
name       days
pos        460 12
size       80 28
mid        44 28
sumwid     2
sumcol     6
column     5
code       
codetxt    
label      Days
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    2
maxlen     10
ifont      0
p_act      

item
type       Time
name       begin
pos        544 12
size       124 28
mid        52 20
column     4
code       
codetxt    
label      Begin
lfont      1
gray       
freeze     
invis      
skip       
default    (date)
maxlen     10
ifont      0
p_act      

item
type       Input
name       title
pos        12 44
size       528 28
mid        52 28
sumwid     80
sumcol     8
column     7
search     1
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
maxlen     1000
ifont      0
p_act      
plan_if    n

item
type       Time
name       end
pos        544 44
size       124 28
mid        52 20
sumwid     6
sumcol     7
column     9
code       
codetxt    
label      Finish
lfont      1
gray       
freeze     
invis      
skip       
default    
maxlen     10
ifont      0
p_act      
plan_if    t

item
type       Label
name       label
pos        12 76
size       52 28
mid        52 28
column     0
search     1
code       
codetxt    
label      Status
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
maxlen     0
ifont      0
p_act      

item
type       Choice
name       status
pos        68 76
size       68 28
mid        52 28
sumwid     4
sumcol     5
column     6
code       t
codetxt    TODO
label      Todo
lfont      0
gray       
freeze     
invis      
skip       
default    t
maxlen     0
ifont      0
p_act      

item
type       Choice
name       status
pos        140 76
size       96 28
mid        52 28
sumwid     4
sumcol     5
column     6
code       p
codetxt    prog
label      In progress
lfont      0
gray       
freeze     
invis      
skip       
default    t
maxlen     0
ifont      0
p_act      

item
type       Choice
name       status
pos        240 76
size       80 28
mid        52 28
sumwid     4
sumcol     5
column     6
code       e
codetxt    test
label      In test
lfont      0
gray       
freeze     
invis      
skip       
default    t
maxlen     0
ifont      0
p_act      

item
type       Choice
name       status
pos        324 76
size       64 28
mid        52 28
sumwid     4
sumcol     5
column     6
code       o
codetxt    ok
label      Done
lfont      0
gray       
freeze     
invis      
skip       
default    t
maxlen     0
ifont      0
p_act      

item
type       Choice
name       status
pos        392 76
size       80 28
mid        52 28
sumwid     4
sumcol     5
column     6
code       d
codetxt    def
label      Deferred
lfont      0
gray       
freeze     
invis      
skip       
default    t
maxlen     0
ifont      0
p_act      

item
type       Note
name       description
pos        12 112
size       660 116
mid        52 16
column     8
search     1
code       
codetxt    
label      Description
lfont      1
gray       
freeze     
invis      
skip       
default    
maxlen     10000
ifont      4
p_act      
plan_if    m
