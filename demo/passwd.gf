grok
name       passwd
dbase      /etc/passwd
comment    useless but hopefully instructive generic database lister. Author: thomas@bitrot.in-berlin.de
cdelim     :
rdonly     1
proc       0
grid       4 4
size       400 179
divider    0
autoq      -1
help	'This database isn't really good for anything except as a demo of how to
help	'set up a user interface for a generic database file as grok likes it, i.e.,
help	'/etc/passwd.
query_s    0
query_n    Open accounts
query_q    {_nopwd == ""}
query_s    0
query_n    Locked accounts
query_q    {_nopwd == "*"}
query_s    0
query_n    User accounts
query_q    ({_nopwd != "*"} && {substr(_user, 0, 1) != "U"})
query_s    0
query_n    Accounts with root privileges
query_q    (_uid == 0)

item
type       Input
name       user
pos        12 12
size       368 28
mid        48 28
sumwid     10
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      User
ljust      0
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       gecos
pos        12 44
size       368 28
mid        48 28
sumwid     20
sumcol     1
column     4
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Name
ljust      0
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       home
pos        12 76
size       368 28
mid        48 28
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
label      Home
ljust      0
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       shell
pos        12 108
size       368 28
mid        48 28
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
label      Shell
ljust      0
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       uid
pos        12 140
size       100 28
mid        48 28
sumwid     0
sumcol     0
column     2
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      UID
ljust      0
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Input
name       gid
pos        116 140
size       100 28
mid        48 28
sumwid     0
sumcol     0
column     3
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      GID
ljust      1
lfont      1
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     100
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Flag
name       nopwd
pos        292 140
size       88 28
mid        48 28
sumwid     0
sumcol     0
column     1
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       *
codetxt    
label      No logins
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     0
maxlen     100
ijust      0
ifont      4
p_act      
a_act      
q_dbase    
query      
q_summ     1
q_first    0
q_last     1
q_last     1
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0
