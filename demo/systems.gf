grok
name       systems
dbase      systems
comment    Systems Database written by Mike Dotson (dotson@stt3.com)
cdelim     :
size       619 424
help	'This is a simple form created to keep
help	'track of the systems on a network.  It
help	'is by no means complete but it does 
help	'contain the essentials *for my needs*.
help	'
help	'Todo:  
help	'  Allow multiple OS's for Intel boxes.
help	'  Add Network cable number for tracking
help	'    purposes to the hub.
help	'  
query_s    0
query_n    Sparcs
query_q    {_processor == "sparc"}
query_s    0
query_n    SGI
query_q    {_processor == "sgi" }
query_s    0
query_n    Intel
query_q    {_processor == "intel"}
query_s    0
query_n    HP
query_q    {_processor == "hp"}
query_s    0
query_n    DEC
query_q    {_processor == "dec"}

item
type       Input
name       hostname
pos        12 20
size       172 28
mid        72 28
sumwid     10
column     0
search     1
defsort    1
label      Host Name
lfont      0
maxlen     10
ifont      0

item
type       Input
name       hostid
pos        192 20
size       160 28
mid        60 28
sumwid     10
sumcol     1
column     1
search     1
code       
codetxt    
label      Host ID
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     10
ifont      0
p_act      

item
type       Input
name       ipadd
pos        360 20
size       140 28
mid        24 28
sumwid     12
sumcol     4
column     2
code       
codetxt    
label      IP
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Label
name       plabel
pos        12 64
size       88 28
mid        28 28
column     0
code       
codetxt    
label      Processor
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
type       Choice
name       processor
pos        104 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
code       sparc
codetxt    Sparc
label      Sparc
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
type       Choice
name       processor
pos        184 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
code       sgi
codetxt    SGI
label      SGI
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
type       Choice
name       processor
pos        264 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
code       intel
codetxt    Intel
label      Intel
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
type       Choice
name       processor
pos        344 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
code       hp
codetxt    HP
label      HP
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
type       Choice
name       processor
pos        424 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
code       dec
codetxt    DEC
label      DEC
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
type       Input
name       ptype
pos        104 96
size       156 28
mid        52 28
column     4
search     1
code       
codetxt    
label      Type
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     20
ifont      0
p_act      

item
type       Input
name       speed
pos        264 96
size       156 28
mid        56 28
column     5
search     1
code       
codetxt    
label      Speed
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     10
ifont      0
p_act      

item
type       Input
name       num
pos        424 96
size       76 28
mid        44 28
column     6
search     1
code       
codetxt    
label      Num
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     2
ifont      0
p_act      

item
type       Label
name       oslabel
pos        12 144
size       36 28
mid        36 28
column     0
code       
codetxt    
label      OS
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
type       Choice
name       OS
pos        52 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       Solaris
codetxt    Solaris
label      Solaris
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
type       Choice
name       OS
pos        148 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       irix
codetxt    IRIX
label      Irix
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
type       Choice
name       OS
pos        244 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       hpux
codetxt    HPUX
label      HPUX
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
type       Choice
name       OS
pos        340 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       linux
codetxt    Linux
label      Linux
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
type       Choice
name       OS
pos        436 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       bsd
codetxt    Free BSD
label      Free BSD
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
type       Choice
name       OS
pos        96 176
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       winnt
codetxt    WinNT
label      WinNT
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
type       Choice
name       OS
pos        192 176
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       win95
codetxt    Win95
label      Win95
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
type       Choice
name       OS
pos        288 176
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
code       doswin
codetxt    DOS/Win
label      DOS/Win
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
type       Input
name       osver
pos        12 212
size       156 28
mid        48 28
column     8
search     1
code       
codetxt    
label      OS ver
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     10
ifont      0
p_act      

item
type       Input
name       memory
pos        188 212
size       156 28
mid        52 28
column     9
search     1
code       
codetxt    
label      Memory
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     10
ifont      0
p_act      

item
type       Input
name       diskspace
pos        364 212
size       156 28
mid        72 28
column     10
search     1
code       
codetxt    
label      Disk Space
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     10
ifont      0
p_act      

item
type       Note
name       patches
pos        12 248
size       508 72
mid        28 28
column     11
code       
codetxt    
label      Patches
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     1000
ifont      0
p_act      

item
type       Note
name       misc
pos        12 324
size       508 84
mid        28 28
column     12
code       
codetxt    
label      Other Notes
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     1000
ifont      0
p_act      
