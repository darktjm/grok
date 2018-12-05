grok
name       passwd
dbase      /etc/passwd
comment    useless but hopefully instructive generic database lister. Author: thomas@bitrot.in-berlin.de
cdelim     :
rdonly     1
size       400 179
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
column     0
search     1
label      User
lfont      1
ifont      4

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
name       home
pos        12 76
size       368 28
mid        48 28
column     5
search     1
code       
codetxt    
label      Home
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
name       shell
pos        12 108
size       368 28
mid        48 28
column     6
search     1
code       
codetxt    
label      Shell
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
name       uid
pos        12 140
size       100 28
mid        48 28
column     2
search     1
code       
codetxt    
label      UID
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
name       gid
pos        116 140
size       100 28
mid        48 28
column     3
search     1
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
ifont      4
p_act      

item
type       Flag
name       nopwd
pos        292 140
size       88 28
mid        48 28
column     1
search     1
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
ifont      4
p_act      
