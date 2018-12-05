grok
name       diary
dbase      diary
comment    I use this to keep track of my work. Author: thomas@bitrot.in-berlin.de
cdelim     :
size       624 285
help	'Use this database to keep track of daily work. The time field can be used
help	'to keep statistics on daily hours.

item
type       Time
name       date
pos        12 12
size       136 28
mid        40 28
sumwid     10
column     0
search     1
label      Date:
lfont      0
default    (date)
maxlen     9
ifont      4

item
type       Time
name       time
pos        180 12
size       136 28
mid        40 28
column     1
timefmt    3
code       
codetxt    
label      Time:
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     9
ifont      4
p_act      

item
type       Flag
name       holiday
pos        540 12
size       76 28
mid        40 28
column     2
code       h
codetxt    
label      Holiday
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
type       Note
name       note
pos        12 48
size       604 224
mid        40 20
sumwid     100
sumcol     1
column     3
search     1
code       
codetxt    
label      Note:
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     10000
ifont      4
p_act      
