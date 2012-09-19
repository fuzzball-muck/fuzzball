@prog cmd-lsedit
1 99999 d
1 i
$include $lib/lmgr
$include $lib/editor
$include $lib/strings
$def LMGRgetcount lmgr-getcount
$def LMGRgetrange lmgr-getrange
$def LMGRputrange lmgr-putrange
$def LMGRdeleterange lmgr-deleterange
  
: LMGRdeletelist
  over over LMGRgetcount
  1 4 rotate 4 rotate LMGRdeleterange
;
  
  
  
: LMGRgetlist
  over over LMGRgetcount
  rot rot 1 rot rot
  LMGRgetrange
;
  
  
: lsedit-loop  ( listname dbref {rng} mask currline cmdstr -- )
    EDITORloop
    dup "save" stringcmp not if
        pop pop pop pop
        3 pick 3 + -1 * rotate
        over 3 + -1 * rotate
        dup 5 + pick over 5 + pick
        over over LMGRdeletelist
        1 rot rot LMGRputrange
        4 pick 4 pick LMGRgetlist
        dup 3 + rotate over 3 + rotate
        "< List saved. >" .tell
        "" lsedit-loop exit
    then
    dup "abort" stringcmp not if
        "< list not saved. >" .tell
        pop pop pop pop pop pop pop pop pop exit
    then
    dup "end" stringcmp not if
        pop pop pop pop pop pop
        dup 3 + rotate over 3 + rotate
        over over LMGRdeletelist
        1 rot rot LMGRputrange
        "< list saved. >" .tell exit
    then
;
  
: cmd-lsedit
    "me" match me !
    "=" .split strip
    dup not if
        "You must specify a listname.  Syntax: lsedit <obj>=<listname>" .tell
        pop pop exit
    then
    "/" swap strcat
    begin dup "//" instr while "/" "//" subst repeat
    dup "/@" instr
    over "/~" instr or
    me @ "wizard" flag? not and if
        "Permission denied." .tell
        pop pop exit
    then
    swap strip
    dup not if
        "You must specify an object.  Syntax: lsedit <obj>=<listname>" .tell
        pop pop exit
    then
    match dup not if pop
        "I don't know what object you mean.  Syntax: lsedit <obj>=<list>" .tell
        pop exit
    else dup #-2 dbcmp if pop
        "I don't know which one you mean.  Syntax: lsedit <obj>=<list>" .tell
        pop exit
    then then
    me @ over owner dbcmp not
    me @ "w" flag? not and if
        pop pop "Permission denied." .tell exit
    then
"<    Welcome to the list editor.  You can get help by entering '.h'     >"
.tell
"< '.end' will exit and save the list.  '.abort' will abort any changes. >"
.tell
"<    To save changes to the list, and continue editing, use '.save'     >"
.tell
    over over LMGRgetlist
    "save" 1 ".i $" lsedit-loop
;
.
c
q
@register cmd-lsedit=global/lsedit
@register #me cmd-lsedit=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=/_/de:A scroll containing a spell called cmd-lsedit
@set $tmp/prog1=_version:1.4
