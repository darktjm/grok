grok
name       music
dbase      music
comment    by Steven Hughes
cdelim     :
size       628 236
help	'This is a database used to catalog a music collection.

item
type       Input
name       composer
pos        12 12
size       412 28
mid        88 28
sumwid     15
column     0
search     1
defsort    1
label      Composer
ljust      1
lfont      0
ifont      0

item
type       Input
name       selection
pos        12 44
size       412 28
mid        88 28
sumwid     50
sumcol     1
column     1
search     1
code       
codetxt    
label      Selection
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Input
name       musicians
pos        12 92
size       412 28
mid        88 28
column     2
search     1
code       
codetxt    
label      Musicians
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Input
name       conductor
pos        12 124
size       412 28
mid        88 28
column     3
search     1
code       
codetxt    
label      Conductor
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Input
name       title
pos        12 156
size       412 28
mid        88 28
column     4
nosort     1
code       
codetxt    
label      CD Title
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Input
name       number
pos        12 188
size       412 28
mid        88 28
column     5
search     1
nosort     1
code       
codetxt    
label      CD Number
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      
