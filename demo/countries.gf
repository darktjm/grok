name       countries
dbase      countries
comment    Countries and capitals, form by thomas@bitrot.in-berlin.de
cdelim     :
rdonly     0
proc       0
grid       4 4
size       539 284
divider    0
query_s    0
query_n    Population over 100 million
query_q    (_3 > 100000000)
query_s    0
query_n    Population over 10 million
query_q    (_3 > 10000000)
query_s    0
query_n    Population under 1 million
query_q    (_3 < 1000000)
query_s    0
query_n    Growing population
query_q    (_growth > 0)
query_s    0
query_n    Shrinking population
query_q    (_growth < 0)
help       'The country database is from 1992 and may be inaccurate or obsolete.

item
type       Input
name       country
pos        12 12
size       512 36
mid        0 28
sumwid     30
sumcol     0
column     0
search     1
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      Country
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      2
ifont      3
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
name       capital
pos        12 64
size       448 28
mid        128 28
sumwid     20
sumcol     1
column     5
search     1
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      Capital:
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
ijust      2
ifont      0
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
type       Print
name       population
pos        12 100
size       236 28
mid        128 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      Population:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    (_3 / 1000000)
pattern    
minlen     1
maxlen     100
ijust      2
ifont      0
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
name       million
pos        252 100
size       68 28
mid        52 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      million
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      2
ifont      0
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
name       growth
pos        12 136
size       236 28
mid        128 28
sumwid     0
sumcol     0
column     4
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      Population growth:
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
ijust      2
ifont      0
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
name       percent
pos        252 136
size       52 28
mid        52 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      %
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      2
ifont      0
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
name       area
pos        12 172
size       236 28
mid        128 28
sumwid     0
sumcol     0
column     1
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      Total Area:
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
ijust      2
ifont      0
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
type       Print
name       area_mi
pos        252 172
size       156 28
mid        52 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      km^2
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    {printf("%.8g", (_area / 2.5889))}
pattern    
minlen     1
maxlen     0
ijust      2
ifont      0
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
name       milabel1
pos        412 172
size       56 28
mid        52 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      mi^2
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      2
ifont      0
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
name       landarea
pos        12 208
size       236 28
mid        128 28
sumwid     0
sumcol     0
column     2
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      Land Area:
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
ijust      2
ifont      0
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
type       Print
name       landarea_mi
pos        252 208
size       156 28
mid        52 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      km^2
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    {printf("%.8g", (_landarea / 2.5889))}
pattern    
minlen     1
maxlen     100
ijust      2
ifont      0
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
name       milabel2
pos        412 208
size       56 28
mid        52 28
sumwid     0
sumcol     0
column     0
search     0
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      mi^2
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     100
ijust      2
ifont      0
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
name       currency
pos        12 244
size       440 28
mid        128 28
sumwid     0
sumcol     0
column     6
search     1
rdonly     1
nosort     0
defsort    0
timefmt    0
code       
label      Currency:
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
ijust      2
ifont      0
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
