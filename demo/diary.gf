grok
name       diary
dbase      diary
comment    I use this to keep track of my work. Author: thomas@bitrot.in-berlin.de
cdelim     :
rdonly     0
proc       0
grid       4 4
size       624 285
divider    0
autoq      -1
help	'Use this database to keep track of daily work. The time field can be used
help	'to keep statistics on daily hours.

item
type       Time
name       date
pos        12 12
size       136 28
mid        40 28
sumwid     10
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Date:
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    (date)
pattern    
minlen     1
maxlen     9
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
name       time
pos        180 12
size       136 28
mid        40 28
sumwid     0
sumcol     0
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    3
code       
codetxt    
label      Time:
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     9
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
name       holiday
pos        540 12
size       76 28
mid        40 28
sumwid     0
sumcol     0
column     2
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       h
codetxt    
label      Holiday
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
type       Note
name       note
pos        12 48
size       604 224
mid        40 20
sumwid     100
sumcol     1
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Note:
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
