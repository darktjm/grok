grok
name       countries
dbase      countries
comment    Countries and capitals, form by thomas@bitrot.in-berlin.de
cdelim     :
size       539 284
help	'The country database is from 1992 and may be inaccurate or obsolete.
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

item
type       Input
name       country
pos        12 12
size       512 36
mid        0 28
sumwid     30
column     0
search     1
rdonly     1
label      Country
lfont      0
ijust      2
ifont      3

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
code       
label      Capital:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Print
name       population
pos        12 100
size       236 28
mid        128 28
column     0
rdonly     1
code       
label      Population:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    (_3 / 1000000)
ijust      2
ifont      0
p_act      

item
type       Label
name       million
pos        252 100
size       68 28
mid        52 28
column     0
rdonly     1
code       
label      million
lfont      0
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Input
name       growth
pos        12 136
size       236 28
mid        128 28
column     4
rdonly     1
code       
label      Population growth:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Label
name       percent
pos        252 136
size       52 28
mid        52 28
column     0
rdonly     1
code       
label      %
lfont      0
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Input
name       area
pos        12 172
size       236 28
mid        128 28
column     1
rdonly     1
code       
label      Total Area:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Print
name       area_mi
pos        252 172
size       156 28
mid        52 28
column     0
rdonly     1
code       
label      km^2
lfont      0
gray       
freeze     
invis      
skip       
default    {printf("%.8g", (_area / 2.5889))}
maxlen     0
ijust      2
ifont      0
p_act      

item
type       Label
name       milabel1
pos        412 172
size       56 28
mid        52 28
column     0
rdonly     1
code       
label      mi^2
lfont      0
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Input
name       landarea
pos        12 208
size       236 28
mid        128 28
column     2
rdonly     1
code       
label      Land Area:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Print
name       landarea_mi
pos        252 208
size       156 28
mid        52 28
column     0
rdonly     1
code       
label      km^2
lfont      0
gray       
freeze     
invis      
skip       
default    {printf("%.8g", (_landarea / 2.5889))}
ijust      2
ifont      0
p_act      

item
type       Label
name       milabel2
pos        412 208
size       56 28
mid        52 28
column     0
rdonly     1
code       
label      mi^2
lfont      0
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      

item
type       Input
name       currency
pos        12 244
size       440 28
mid        128 28
column     6
search     1
rdonly     1
code       
label      Currency:
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
ijust      2
ifont      0
p_act      
