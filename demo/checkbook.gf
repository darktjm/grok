grok
name       checkbook
dbase      checkbook
comment    By David L. Johnson, dlj0@lehigh.edu
cdelim     :
size       696 155
autoq      0
query_s    1
query_n    note
query_q    

item
type       Label
name       typel
pos        12 12
size       52 28
mid        52 28
column     0
label      Type:
lfont      0
maxlen     0
ifont      0

item
type       Choice
name       type
pos        68 12
size       100 28
mid        92 28
sumwid     4
column     0
code       c
codetxt    chk
label      Check
lfont      0
gray       
freeze     
invis      
skip       
default    c
maxlen     0
ifont      0
p_act      

item
type       Choice
name       type
pos        152 12
size       92 28
mid        92 28
sumwid     4
column     0
code       a
codetxt    atm
label      ATM
lfont      0
gray       
freeze     
invis      
skip       
default    c
maxlen     0
ifont      0
p_act      

item
type       Choice
name       type
pos        224 12
size       96 28
mid        92 28
sumwid     4
column     0
code       p
codetxt    pos
label      POS
lfont      0
gray       
freeze     
invis      
skip       
default    c
maxlen     0
ifont      0
p_act      

item
type       Choice
name       type
pos        296 12
size       84 28
mid        84 28
sumwid     4
column     0
code       d
codetxt    dep
label      Dep
lfont      0
gray       
freeze     
invis      
skip       
default    c
maxlen     0
ifont      0
p_act      

item
type       Choice
name       type
pos        364 12
size       76 28
mid        76 28
sumwid     4
column     0
code       s
codetxt    sc
label      SC
lfont      0
gray       
freeze     
invis      
skip       
default    c
maxlen     0
ifont      0
p_act      

item
type       Choice
name       type
pos        424 12
size       76 28
mid        76 28
sumwid     4
column     0
code       e
codetxt    err
label      Error
lfont      0
gray       
freeze     
invis      
skip       
default    c
maxlen     0
ifont      0
p_act      

item
type       Time
name       date
pos        516 12
size       144 28
mid        44 28
sumwid     8
sumcol     5
column     5
search     1
code       
codetxt    
label      Date
lfont      0
gray       
freeze     
invis      
skip       
default    (date)
maxlen     8
ifont      0
p_act      

item
type       Input
name       number
pos        12 48
size       148 28
mid        48 28
sumwid     5
sumcol     1
column     1
search     1
code       
codetxt    
label      Ck.#
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     5
ifont      0
p_act      

item
type       Input
name       Payee
pos        164 48
size       404 28
mid        68 28
sumwid     20
sumcol     2
column     2
search     1
code       
codetxt    
label      To/From
lfont      0
gray       
freeze     
invis      
skip       
default    
maxlen     30
ifont      0
p_act      

item
type       Flag
name       clear
pos        576 48
size       108 28
mid        68 28
sumwid     4
sumcol     6
column     6
search     1
code       c
codetxt    Clr
label      Clear
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
name       debit
pos        12 84
size       248 28
mid        80 28
sumwid     8
sumcol     3
column     3
search     1
code       
codetxt    
label      Debit Amt
lfont      0
gray       
freeze     
invis      
skip       
default    0
maxlen     8
ifont      0
p_act      

item
type       Input
name       credit
pos        304 84
size       264 28
mid        84 28
sumwid     8
sumcol     4
column     4
search     1
code       
codetxt    
label      Credit Amt
lfont      0
gray       
freeze     
invis      
skip       
default    0
maxlen     8
ifont      0
p_act      

item
type       Flag
name       tax
pos        576 84
size       128 28
mid        68 28
sumwid     4
sumcol     7
column     7
search     1
code       t
codetxt    tax
label      Tax Ded
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
name       note
pos        12 120
size       448 28
mid        48 28
sumwid     30
sumcol     8
column     8
search     1
code       
codetxt    
label      Note
lfont      0
gray       
freeze     
invis      
skip       
default    
ifont      0
p_act      

item
type       Print
name       Balance
pos        468 120
size       192 28
mid        68 28
column     0
search     1
code       
codetxt    
label      Balance
lfont      0
gray       
freeze     
invis      
skip       
default    ((sum(_credit)) - (sum(_debit)))
maxlen     0
ifont      0
p_act      
