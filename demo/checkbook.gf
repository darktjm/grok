grok
name       checkbook
dbase      checkbook
comment    By David L. Johnson, dlj0@lehigh.edu
cdelim     :
rdonly     0
proc       0
grid       4 4
size       696 155
divider    0
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
label      Type:
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       type
pos        68 12
size       100 28
mid        92 28
sumwid     4
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       c
codetxt    chk
label      Check
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    c
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       type
pos        152 12
size       92 28
mid        92 28
sumwid     4
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       a
codetxt    atm
label      ATM
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    c
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       type
pos        224 12
size       96 28
mid        92 28
sumwid     4
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       p
codetxt    pos
label      POS
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    c
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       type
pos        296 12
size       84 28
mid        84 28
sumwid     4
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       d
codetxt    dep
label      Dep
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    c
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       type
pos        364 12
size       76 28
mid        76 28
sumwid     4
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       s
codetxt    sc
label      SC
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    c
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Choice
name       type
pos        424 12
size       76 28
mid        76 28
sumwid     4
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       e
codetxt    err
label      Error
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    c
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Date
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    (date)
pattern    
minlen     1
maxlen     8
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Ck.#
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     5
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      To/From
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    
pattern    
minlen     1
maxlen     30
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       c
codetxt    Clr
label      Clear
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Debit Amt
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    0
pattern    
minlen     1
maxlen     8
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Credit Amt
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    0
pattern    
minlen     1
maxlen     8
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       t
codetxt    tax
label      Tax Ded
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

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
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Note
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0

item
type       Print
name       Balance
pos        468 120
size       192 28
mid        68 28
sumwid     0
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Balance
ljust      0
lfont      0
gray       
freeze     
invis      
skip       
default    ((sum(_credit)) - (sum(_debit)))
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
ch_grid    0
ch_vert    0
ch_axis    0
ch_scroll  0
ch_ncomp   0
