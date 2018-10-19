grok
name       weather
dbase      weather
comment    chart demo. Author: thomas@bitrot.in-berlin.de
cdelim     :
rdonly     0
proc       0
grid       4 4
size       431 286
divider    136
autoq      -1
help	'I don't suggest to actually use this for anything, it's just a demo to show
help	'off charts. Support for charts is pretty basic in this version; there are
help	'about a million ways to do this right and all are a lot of work. Awaiting
help	'comments and suggestions...

item
type       Chart
name       tempchart
pos        12 11
size       408 112
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
label      Temperature
ljust      0
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
ch_xrange  0 1
ch_yrange  -10 40
ch_auto    1 1
ch_grid    0 10
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   1
chart
_ch_type   0
_ch_fat    0 0
_ch_excl   
_ch_color  (_temp < 0)
_ch_label  Temperature
_ch_mode0  2
_ch_expr0  (_date)
_ch_field0 0
_ch_mul0   0
_ch_add0   0
_ch_mode1  0
_ch_expr1  
_ch_field1 0
_ch_mul1   0
_ch_add1   0
_ch_mode2  2
_ch_expr2  86400
_ch_field2 0
_ch_mul2   0
_ch_add2   0
_ch_mode3  2
_ch_expr3  (_temp)
_ch_field3 0
_ch_mul3   0
_ch_add3   0

item
type       Time
name       date
pos        12 148
size       156 28
mid        40 28
sumwid     8
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    1
timefmt    0
code       
codetxt    
label      Date:
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
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       temp
pos        216 148
size       196 28
mid        84 28
sumwid     4
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Temperature:
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
name       press
pos        216 180
size       196 28
mid        84 28
sumwid     4
sumcol     2
column     2
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Pressure:
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
q_last     1
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Input
name       humidity
pos        216 212
size       196 28
mid        84 28
sumwid     4
sumcol     3
column     3
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Humidity:
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
q_last     1
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       weather
pos        12 248
size       68 28
mid        64 28
sumwid     2
sumcol     5
column     5
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       s
codetxt    
label      Sunny
ljust      0
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
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       weather
pos        84 248
size       108 28
mid        64 28
sumwid     2
sumcol     5
column     5
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       p
codetxt    
label      Partly cloudy
ljust      0
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
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       weather
pos        196 248
size       72 28
mid        64 28
sumwid     2
sumcol     5
column     5
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       c
codetxt    
label      Cloudy
ljust      0
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
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       weather
pos        272 248
size       84 28
mid        64 28
sumwid     2
sumcol     5
column     5
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       l
codetxt    
label      Light rain
ljust      0
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
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0

item
type       Choice
name       weather
pos        360 248
size       60 28
mid        52 28
sumwid     2
sumcol     5
column     5
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       r
codetxt    
label      Rain
ljust      0
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
ch_xrange  0 1
ch_yrange  0 1
ch_auto    0 0
ch_grid    0 0
ch_snap    0 0
ch_label   0 0
ch_xexpr   
ch_yexpr   
ch_ncomp   0
