grok
name       phonelog
dbase      phonelog
comment    phone log. Author: thomas@bitrot.in-berlin.de
cdelim     :
size       768 366
divider    44
help	'For keeping logs of phone calls. This database can be called from the phone
help	'directory database, and vice versa.

item
type       Button
name       phone
pos        616 8
size       140 28
mid        44 28
column     0
timefmt    2
label      Phone Directory
ljust      2
lfont      0
maxlen     0
ifont      4
p_act      {switch("phone", "*")}

item
type       Input
name       name
pos        12 56
size       248 28
mid        44 28
sumwid     12
column     0
search     1
code       
codetxt    
label      Name
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
name       company
pos        264 56
size       180 28
mid        64 28
sumwid     10
sumcol     1
column     1
timefmt    2
code       
codetxt    
label      Company
lfont      1
gray       
freeze     
invis      
skip       
default    
ifont      4
p_act      

item
type       Time
name       date
pos        448 56
size       216 28
mid        44 28
sumwid     8
sumcol     2
column     2
timefmt    2
code       
codetxt    
label      Date
lfont      1
gray       
freeze     
invis      
skip       
default    (date)
ifont      4
p_act      

item
type       Flag
name       callout
pos        676 56
size       80 28
mid        44 28
column     3
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
maxlen     0
ifont      4
p_act      

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
code       
codetxt    
label      Topic
lfont      1
gray       
freeze     
invis      
skip       
default    
ifont      4
p_act      

item
type       Note
name       log
pos        12 140
size       744 216
mid        44 16
column     5
search     1
timefmt    2
code       
codetxt    
label      Log
lfont      1
gray       
freeze     
invis      
skip       
default    
maxlen     10000
ifont      4
p_act      
