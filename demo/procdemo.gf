grok
name       procdemo
dbase      procdemo
comment    procedural demo. Author: thomas@bitrot.in-berlin.de
cdelim     :
proc       1
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
column     0
label      Name
lfont      0
ifont      0
