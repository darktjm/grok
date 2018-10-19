grok
name       www_pages
dbase      www_pages
comment    hello, world
cdelim     :
rdonly     0
proc       0
grid       4 4
size       712 302
divider    0
autoq      -1
query_s    0
query_n    WEB pages
query_q    {_service == "w"}
query_s    0
query_n    FTP sites
query_q    {_service == "f"}
query_s    0
query_n    Telnet account
query_q    {_service == "t"}
query_s    0
query_n    Home pages
query_q    {_home == "h"}
query_s    0
query_n    Cool
query_q    {_type == "c"}
query_s    0
query_n    Archive sites
query_q    {_type == "a"}
query_s    0
query_n    Companies and Products
query_q    {_type == "y"}
query_s    0
query_n    Searchers and Directories
query_q    {_type == "s"}
query_s    0
query_n    Boring
query_q    {_type == "b"}
query_s    0
query_n    Miscellaneous
query_q    {_type == "m"}

item
type       Time
name       date
pos        12 12
size       140 28
mid        68 28
sumwid     0
sumcol     0
column     3
search     0
rdonly     0
nosort     0
defsort    1
timefmt    0
code       
codetxt    
label      Added on:
ljust      0
lfont      1
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
type       Choice
name       service
pos        208 12
size       76 28
mid        68 28
sumwid     3
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       w
codetxt    WWW
label      WWW
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
name       service
pos        288 12
size       80 28
mid        68 28
sumwid     3
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       f
codetxt    FTP
label      FTP
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
name       service
pos        372 12
size       80 28
mid        68 28
sumwid     3
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       t
codetxt    Tel
label      Telnet
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    w
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
type       Button
name       run
pos        580 12
size       124 28
mid        68 28
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
label      Start NetScape
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
p_act      {system("netscape -remote 'openURL("._url.")' &")}
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
type       Flag
name       home
pos        80 44
size       108 28
mid        68 28
sumwid     1
sumcol     2
column     2
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       h
codetxt    H
label      Home page
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
name       type
pos        208 44
size       76 28
mid        68 28
sumwid     7
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       c
codetxt    Cool
label      Cool
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    m
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
name       type
pos        288 44
size       80 28
mid        68 28
sumwid     7
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       a
codetxt    Archive
label      Archive
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    m
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
name       type
pos        372 44
size       80 28
mid        68 28
sumwid     7
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       y
codetxt    Company
label      Company
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    m
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
name       type
pos        456 44
size       80 28
mid        68 28
sumwid     7
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       s
codetxt    Search
label      Search
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    m
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
name       type
pos        540 44
size       80 28
mid        68 28
sumwid     7
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       b
codetxt    Boring
label      Boring
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    m
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
name       type
pos        624 44
size       76 28
mid        68 28
sumwid     7
sumcol     1
column     1
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       m
codetxt    Misc
label      Misc
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    m
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
name       url
pos        12 80
size       692 28
mid        68 28
sumwid     40
sumcol     4
column     4
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      URL:
ljust      0
lfont      1
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
type       Input
name       keys
pos        12 116
size       692 28
mid        68 28
sumwid     100
sumcol     5
column     5
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Keywords:
ljust      0
lfont      1
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
name       item5
pos        12 152
size       692 140
mid        68 16
sumwid     0
sumcol     0
column     6
search     1
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Note:
ljust      0
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     10000
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
