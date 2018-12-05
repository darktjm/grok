grok
name       occasions
dbase      occasions
comment    Steven Hughes:  database for occasions to be entered into "Plan"
cdelim     :
size       645 131
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
label      Occasion:
ljust      1
lfont      0
ifont      0
plan_if    n

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
defsort    1
code       
codetxt    
label      Date:
lfont      0
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
type       Flag
name       done
pos        12 48
size       108 28
mid        84 28
sumwid     4
column     2
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
maxlen     0
ifont      0
p_act      

item
type       Note
name       note
pos        132 48
size       504 72
mid        72 24
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
maxlen     1000
ifont      0
p_act      
