grok
name       weather
dbase      weather
comment    chart demo. Author: thomas@bitrot.in-berlin.de
cdelim     :
size       431 286
divider    136
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
column     0
label      Temperature
lfont      0
maxlen     0
ifont      4
ch_yrange  -10 40
ch_auto    1 1
ch_grid    0 10
ch_ncomp   1
chart
_ch_color  (_temp < 0)
_ch_label  Temperature
_ch_mode0  2
_ch_expr0  (_date)
_ch_mode2  2
_ch_expr2  86400
_ch_mode3  2
_ch_expr3  (_temp)

item
type       Time
name       date
pos        12 148
size       156 28
mid        40 28
sumwid     8
column     0
defsort    1
code       
codetxt    
label      Date:
lfont      1
gray       
freeze     
invis      
skip       
default    
ifont      4
p_act      

item
type       Input
name       temp
pos        204 148
size       204 28
mid        96 28
sumwid     4
sumcol     1
column     1
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
ifont      4
p_act      

item
type       Input
name       press
pos        204 180
size       204 28
mid        96 28
sumwid     4
sumcol     2
column     2
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
ifont      4
p_act      

item
type       Input
name       humidity
pos        204 212
size       204 28
mid        96 28
sumwid     4
sumcol     3
column     3
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
ifont      4
p_act      

item
type       Choice
name       weather
pos        12 248
size       68 28
mid        64 28
sumwid     2
sumcol     5
column     5
code       s
codetxt    
label      Sunny
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
name       weather
pos        84 248
size       108 28
mid        64 28
sumwid     2
sumcol     5
column     5
code       p
codetxt    
label      Partly cloudy
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
name       weather
pos        196 248
size       72 28
mid        64 28
sumwid     2
sumcol     5
column     5
code       c
codetxt    
label      Cloudy
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
name       weather
pos        272 248
size       84 28
mid        64 28
sumwid     2
sumcol     5
column     5
code       l
codetxt    
label      Light rain
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
name       weather
pos        360 248
size       60 28
mid        52 28
sumwid     2
sumcol     5
column     5
code       r
codetxt    
label      Rain
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     0
ifont      4
p_act      
