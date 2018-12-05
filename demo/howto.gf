grok
name       howto
dbase      howto
comment    Helpful tips and procedures that I can't seem to remember, by Steven Hughes
cdelim     :
size       645 468
help	'Everytime I figure out how to do something useful on my
help	'computer, I use this simple database to document what I
help	'did.  This replaced a drawer full of notes, and it saves
help	'having to sort through manuals, manpages, etc., to get
help	'the one option switch I need.  I also keep lists of IP
help	'addresses assigned to me, etc., in this database.

item
type       Input
name       keyword
pos        12 8
size       304 28
mid        76 28
sumwid     15
column     0
search     1
label      Keyword:
lfont      0
ifont      0

item
type       Input
name       system
pos        320 8
size       300 28
mid        64 24
column     3
search     1
code       
codetxt    
label      System:
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
name       summary
pos        12 44
size       608 28
mid        76 28
sumwid     55
sumcol     1
column     1
search     1
code       
codetxt    
label      Summary:
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Note
name       syntax
pos        12 80
size       608 364
mid        76 28
sumcol     2
column     2
search     1
code       
codetxt    
label      How to Do it
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     1000
ifont      0
p_act      
