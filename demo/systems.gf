grok
name       systems
dbase      systems
comment    Systems Database written by Mike Dotson (dotson@stt3.com)
cdelim     :
rdonly     0
proc       0
grid       4 4
size       619 424
divider    0
autoq      -1
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
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    1
timefmt    0
code       
codetxt    
label      Host Name
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      0
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
name       hostid
pos        192 20
size       160 28
mid        60 28
sumwid     10
sumcol     1
column     1
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Host ID
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      0
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
name       ipadd
pos        360 20
size       140 28
mid        24 28
sumwid     12
sumcol     4
column     2
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      IP
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
ijust      0
ifont      0
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
type       Label
name       plabel
pos        12 64
size       88 28
mid        28 28
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
label      Processor
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
ifont      0
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
type       Choice
name       processor
pos        104 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       sparc
codetxt    Sparc
label      Sparc
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
ifont      0
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
type       Choice
name       processor
pos        184 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       sgi
codetxt    SGI
label      SGI
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
ifont      0
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
type       Choice
name       processor
pos        264 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       intel
codetxt    Intel
label      Intel
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
ifont      0
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
type       Choice
name       processor
pos        344 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       hp
codetxt    HP
label      HP
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
ifont      0
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
type       Choice
name       processor
pos        424 64
size       76 28
mid        28 28
sumwid     10
sumcol     2
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       dec
codetxt    DEC
label      DEC
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
ifont      0
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
name       ptype
pos        104 96
size       156 28
mid        52 28
sumwid     0
sumcol     0
column     4
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Type
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     20
ijust      0
ifont      0
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
name       speed
pos        264 96
size       156 28
mid        56 28
sumwid     0
sumcol     0
column     5
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Speed
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      0
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
name       num
pos        424 96
size       76 28
mid        44 28
sumwid     0
sumcol     0
column     6
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Num
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     2
ijust      0
ifont      0
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
type       Label
name       oslabel
pos        12 144
size       36 28
mid        36 28
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
label      OS
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
ifont      0
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
type       Choice
name       OS
pos        52 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       Solaris
codetxt    Solaris
label      Solaris
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
ifont      0
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
type       Choice
name       OS
pos        148 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       irix
codetxt    IRIX
label      Irix
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
ifont      0
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
type       Choice
name       OS
pos        244 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       hpux
codetxt    HPUX
label      HPUX
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
ifont      0
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
type       Choice
name       OS
pos        340 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       linux
codetxt    Linux
label      Linux
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
ifont      0
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
type       Choice
name       OS
pos        436 144
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       bsd
codetxt    Free BSD
label      Free BSD
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
ifont      0
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
type       Choice
name       OS
pos        96 176
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       winnt
codetxt    WinNT
label      WinNT
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
ifont      0
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
type       Choice
name       OS
pos        192 176
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       win95
codetxt    Win95
label      Win95
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
ifont      0
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
type       Choice
name       OS
pos        288 176
size       92 28
mid        28 28
sumwid     10
sumcol     3
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       doswin
codetxt    DOS/Win
label      DOS/Win
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
ifont      0
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
name       osver
pos        12 212
size       156 28
mid        48 28
sumwid     0
sumcol     0
column     8
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      OS ver
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      0
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
name       memory
pos        188 212
size       156 28
mid        52 28
sumwid     0
sumcol     0
column     9
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Memory
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      0
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
name       diskspace
pos        364 212
size       156 28
mid        72 28
sumwid     0
sumcol     0
column     10
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Disk Space
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10
ijust      0
ifont      0
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
type       Note
name       patches
pos        12 248
size       508 72
mid        28 28
sumwid     0
sumcol     0
column     11
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Patches
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     1000
ijust      0
ifont      0
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
type       Note
name       misc
pos        12 324
size       508 84
mid        28 28
sumwid     0
sumcol     0
column     12
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Other Notes
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     1000
ijust      0
ifont      0
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
