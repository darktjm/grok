grok
name       occasions
dbase      occasions
comment    Steven Hughes:  database for occasions to be entered into "Plan"
cdelim     :
rdonly     0
proc       0
grid       4 4
size       645 131
divider    0
autoq      -1
help	'This is a simple database used to place dates into plan.

item
type       Input
name       occasion
pos        12 12
size       464 28
mid        80 24
sumwid     40
sumcol     2
column     1
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Occasion:
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      0
ifont      0
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
type       Time
name       date
pos        484 12
size       152 28
mid        48 28
sumwid     10
sumcol     1
column     0
search     1
rdonly     0
nosort     0
defsort    1
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
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      0
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
type       Flag
name       done
pos        12 48
size       108 28
mid        84 28
sumwid     4
sumcol     0
column     2
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       OK
codetxt    OK
label      Completed:
ljust      1
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
ifont      0
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
pos        132 48
size       504 72
mid        72 24
sumwid     0
sumcol     0
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
maxlen     1000
ijust      0
ifont      0
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
