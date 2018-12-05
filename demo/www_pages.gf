grok
name       www_pages
dbase      www_pages
comment    hello, world
cdelim     :
size       712 302
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
column     3
defsort    1
label      Added on:
lfont      1
maxlen     20
ifont      0

item
type       Choice
name       service
pos        208 12
size       76 28
mid        68 28
sumwid     3
column     0
code       w
codetxt    WWW
label      WWW
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      0
p_act      

item
type       Choice
name       service
pos        288 12
size       80 28
mid        68 28
sumwid     3
column     0
code       f
codetxt    FTP
label      FTP
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      0
p_act      

item
type       Choice
name       service
pos        372 12
size       80 28
mid        68 28
sumwid     3
column     0
code       t
codetxt    Tel
label      Telnet
lfont      0
gray       
freeze     
invis      
skip       
default    w
maxlen     0
ifont      0
p_act      

item
type       Button
name       run
pos        580 12
size       124 28
mid        68 28
column     0
code       
codetxt    
label      Start NetScape
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     0
ifont      0
p_act      {system("netscape -remote 'openURL("._url.")' &")}

item
type       Flag
name       home
pos        80 44
size       108 28
mid        68 28
sumwid     1
sumcol     2
column     2
code       h
codetxt    H
label      Home page
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
type       Menu
name       type
pos        208 44
size       184 28
mid        44 28
sumwid     7
sumcol     1
column     1
label      Type:
lfont      1
nmenu      6
menu       Cool
_m_code    c
_m_codetxt Cool
menu       Archive
_m_code    a
_m_codetxt Archive
menu       Company
_m_code    y
_m_codetxt Company
menu       Search
_m_code    s
_m_codetxt Search
menu       Boring
_m_code    b
_m_codetxt Boring
menu       Misc
_m_code    m
_m_codetxt Misc
default    m
maxlen     0
ifont      0

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
label      URL:
lfont      1
maxlen     1000
ifont      0

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
code       
codetxt    
label      Keywords:
lfont      1
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
name       item5
pos        12 152
size       692 140
mid        68 16
column     6
search     1
nosort     1
code       
codetxt    
label      Note:
lfont      1
gray       
freeze     
invis      
skip       
default    
maxlen     10000
ifont      0
p_act      
