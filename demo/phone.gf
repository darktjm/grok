grok
name       phone
dbase      phone
comment    Phone directory, form by thomas@bitrot.in-berlin.de
cdelim     :
size       756 291
divider    44
help	'Phone directory. Put the primary phone number in the "Phone" field, which
help	'will be put into the summary above the card. Company phone numbers can go
help	'into the "Company" field. You may want to want to turn on the "Letter search
help	'checks all words" mode in the Preferences menu to search for all initials.
help	'The "Call log" button switches to the Call Log database.
query_s    0
query_n    Friends
query_q    {_group == "f"}
query_s    0
query_n    Family
query_q    {_group == "y"}
query_s    0
query_n    Company
query_q    {_group == "c"}
query_s    0
query_n    Business
query_q    {_group == "b"}
query_s    0
query_n    Shopping
query_q    {_group == "s"}
query_s    0
query_n    Travel
query_q    {_group == "t"}
query_s    0
query_n    Miscellaneous
query_q    {_group == "m"}
query_s    0
query_n    People with email
query_q    {_email}
query_s    0
query_n    People with fax
query_q    {_fax}

item
type       Button
name       call_log
pos        616 8
size       128 28
mid        64 0
column     0
label      Call log
ljust      2
lfont      0
maxlen     0
ifont      4
p_act      {switch("phonelog", "{_name=='"._name."'}")}

item
type       Input
name       name
pos        12 56
size       352 28
mid        64 28
sumwid     30
sumcol     1
column     0
search     1
defsort    1
code       
codetxt    
label      Name
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
type       Input
name       address
pos        12 88
size       352 28
mid        64 28
column     1
search     1
code       
codetxt    
label      Address
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
type       Input
name       email
pos        12 120
size       352 28
mid        64 28
column     2
search     1
code       
codetxt    
label      Email
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
type       Input
name       phone
pos        12 152
size       352 28
mid        64 28
sumwid     30
sumcol     2
column     3
search     1
code       
codetxt    
label      Phone
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
type       Input
name       company
pos        12 184
size       352 28
mid        64 28
column     4
search     1
code       
codetxt    
label      Company
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
type       Input
name       fax
pos        12 216
size       352 28
mid        64 28
column     5
search     1
code       
codetxt    
label      Fax
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
type       Note
name       note
pos        376 56
size       368 188
mid        92 0
column     6
search     1
code       
codetxt    
label      Note
ljust      2
lfont      1
gray       
freeze     
invis      
skip       
default    
maxlen     10000
ifont      4
p_act      

item
type       Label
name       glabel
pos        12 252
size       60 28
mid        56 28
column     0
search     1
code       
codetxt    
label      Group:
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
type       Choice
name       group
pos        76 252
size       64 28
mid        64 28
sumwid     6
column     7
search     1
code       f
codetxt    Friend
label      Friend
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      

item
type       Choice
name       group
pos        156 252
size       68 28
mid        64 28
sumwid     6
column     7
search     1
code       y
codetxt    Family
label      Family
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      

item
type       Choice
name       group
pos        240 252
size       80 28
mid        64 28
sumwid     6
column     7
search     1
code       c
codetxt    Company
label      Company
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      

item
type       Choice
name       group
pos        340 252
size       80 28
mid        64 28
sumwid     6
column     7
search     1
code       b
codetxt    Business
label      Business
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      

item
type       Choice
name       group
pos        436 252
size       80 28
mid        64 28
sumwid     6
column     7
search     1
code       s
codetxt    Shop
label      Shopping
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      

item
type       Choice
name       group
pos        536 252
size       68 28
mid        64 28
sumwid     6
column     7
search     1
code       t
codetxt    Travel
label      Travel
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      

item
type       Choice
name       group
pos        620 252
size       60 28
mid        52 28
sumwid     6
column     7
search     1
code       h
codetxt    Host
label      Host
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      

item
type       Choice
name       group
pos        688 252
size       56 28
mid        56 24
sumwid     6
column     7
search     1
code       m
codetxt    Misc
label      Misc
ljust      1
lfont      0
gray       
freeze     
invis      
skip       
default    f
maxlen     0
ifont      4
p_act      
