grok
name       bibliofile
dbase      biblio
comment    bibliography of history references. (c) James C. McPherson 10/1995. version 1.0. email mljmcphe@dingo.cc.uq.oz.au, j.mcpherson@central1.library.uq.oz.au
cdelim     |
rdonly     0
proc       0
grid       4 4
size       776 331
divider    0
autoq      -1
help	'	Biblio	(c) James C. McPherson, 10/1995 v1.0
help	'
help	'The format of the database follows the Oxbridge style for footnotes, namely
help	'
help	'author, {\it{title}}, (place: publisher, date).
help	'
help	'This is the format adopted by the accompanying perl script "grok2tex.p" 
help	'(see separate README). If you want to put in a text with editors, make sure
help	'the box "Editor" is checked; similarly for articles.
help	'
help	'Basically, all the fields are there for you to put in whatever data you wish.
help	'If you use grok2tex.p then you must put in key values. 
help	'
help	'I hope you find the form and script as useful as I do.
help	'
help	'James C. McPherson
help	'
help	'mljmcphe@dingo.cc.uq.oz.au
help	'j.mcpherson@central1.library.uq.oz.au
help	'
help	'I, James McPherson, hereby place the Bibliofile card idea fully in the public
help	'domain. Anybody may use the Bibliofile idea and accompanying perl code, and
help	'modifications may be made also, on the condition that modifications get
help	'sent to me at mljmcphe@dingo.cc.uq.oz.au.

query_s    0
query_n    Article
query_q    ={Article}

item
type       Input
name       Author
pos        12 12
size       460 28
mid        92 28
sumwid     15
sumcol     0
column     0
search     1
rdonly     0
nosort     0
defsort    1
timefmt    0
code       
codetxt    
label      Author
ljust      1
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
type       Flag
name       Editor
pos        500 12
size       116 28
mid        92 28
sumwid     2
sumcol     3
column     2
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       (ed)
codetxt    T
label      Editor
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
type       Flag
name       item0
pos        632 12
size       116 28
mid        92 24
sumwid     2
sumcol     6
column     8
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       Art
codetxt    A
label      Article
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
name       Title
pos        12 48
size       756 28
mid        92 28
sumwid     25
sumcol     1
column     1
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Title
ljust      1
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
ifont      1
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
name       Address
pos        12 84
size       756 28
mid        92 28
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
label      Address
ljust      1
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
type       Input
name       Publisher
pos        12 120
size       756 28
mid        92 28
sumwid     20
sumcol     4
column     4
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Publisher
ljust      1
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
type       Input
name       Date
pos        12 156
size       368 28
mid        92 28
sumwid     5
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
ljust      1
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
type       Input
name       Pages
pos        392 156
size       176 28
mid        64 28
sumwid     0
sumcol     0
column     9
search     0
rdonly     0
nosort     1
defsort    0
timefmt    0
code       
codetxt    
label      Pages
ljust      1
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
type       Input
name       Edition
pos        600 156
size       168 28
mid        76 28
sumwid     2
sumcol     10
column     10
search     1
rdonly     0
nosort     0
defsort    1
timefmt    0
code       
codetxt    
label      Edition
ljust      1
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
name       item5
pos        12 192
size       708 28
mid        92 28
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
label      Call Number
ljust      1
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
type       Note
name       Annotation
pos        12 228
size       756 56
mid        92 28
sumwid     0
sumcol     0
column     7
search     1
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Annotation
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
type       Input
name       KEY
pos        320 292
size       168 28
mid        76 28
sumwid     15
sumcol     11
column     11
search     1
rdonly     0
nosort     0
defsort    1
timefmt    0
code       
codetxt    
label      Key
ljust      1
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
