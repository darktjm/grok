grok
name       phonelog
dbase      phonelog
comment    phone log. Author: thomas@bitrot.in-berlin.de
cdelim     :
rdonly     0
proc       0
grid       4 4
size       768 366
divider    44
autoq      -1
help	'For keeping logs of phone calls. This database can be called from the phone
help	'directory database, and vice versa.

item
type       Button
name       phone
pos        616 8
size       140 28
mid        44 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    2
code       
codetxt    
label      Phone Directory
ljust      2
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     0
ijust      0
ifont      4
p_act      {switch("phone", "*")}
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
name       name
pos        12 56
size       248 28
mid        44 28
sumwid     12
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Name
ljust      0
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       company
pos        264 56
size       180 28
mid        64 28
sumwid     10
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    2
code       
codetxt    
label      Company
ljust      0
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Time
name       date
pos        448 56
size       216 28
mid        44 28
sumwid     8
sumcol     2
column     2
search     0
rdonly     0
nosort     0
defsort    0
timefmt    2
code       
codetxt    
label      Date
ljust      0
lfont      1
gray       
freeze     
invis      
skip       
default    (date)
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Flag
name       callout
pos        676 56
size       80 28
mid        44 28
sumwid     0
sumcol     0
column     3
search     0
rdonly     0
nosort     0
defsort    0
timefmt    2
code       o
codetxt    
label      Call out
ljust      2
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
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
type       Input
name       topic
pos        12 92
size       652 28
mid        44 28
sumwid     50
sumcol     4
column     4
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Topic
ljust      0
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
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Note
name       log
pos        12 140
size       744 216
mid        44 16
sumwid     0
sumcol     0
column     5
search     1
rdonly     0
nosort     0
defsort    0
timefmt    2
code       
codetxt    
label      Log
ljust      0
lfont      1
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
