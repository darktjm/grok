grok
name       procdemo
dbase      procdemo
comment    procedural demo. Author: thomas@bitrot.in-berlin.de
cdelim     :
rdonly     0
proc       1
grid       4 4
size       400 200
divider    0
autoq      -1
help	'This database is not useful for anything at all, it's a demo how procedural
help	'databases can be written. Start the form editor and press the Edit button to
help	'see how it works. It's about the most trivial procedure that can be written,
help	'but you'll get the idea. When the database is saved, it's written to
help	'/tmp/passwd because /etc/passwd is taboo.

item
type       Input
name       item0
pos        12 12
size       368 28
mid        92 28
sumwid     20
sumcol     0
column     0
search     0
rdonly     0
nosort     0
defsort    0
timefmt    0
code       
codetxt    
label      Name
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
