@prog cmd-@archive
1 99999 d
1 i
( @const object=aefi           )
( @dig roomname=parent=regname )
( @act exitname=source=regname )
( @cre thingname=value=regname )
( @reg object=regname          )
  
$include $lib/strings
$include $lib/match
$include $lib/edit
  
: show-help
{
"Syntax: @archive <object>[=1acefil]"
" @archive <object>=1    Archive only that object."
" @archive <object>=a    Archive all, regardless of owner.  (wizards only)."
" @archive <object>=c    Don't archive contents."
" @archive <object>=e    Archive objects not in this room's environment."
" @archive <object>=f    Don't archive floater child rooms unless linked to."
" @archive <object>=i    Archive, including even globally registered objects."
" @archive <object>=l    Don't follow links or droptos in archiving."
" @archive <object>=p    Don't archive programs at all."
"NOTE: Turn off your client's wordwrap before logging an @archive output."
} EDITdisplay
;
  
lvar originalobj
lvar here?
lvar owned?
lvar one?
lvar nofloater?
lvar nocontents?
lvar nolinks?
lvar noprogs?
lvar playercnt
lvar roomcnt
lvar exitcnt
lvar thingcnt
lvar progcnt
  
: wait-for-buffer ( -- )
    descr descrflush
    begin
        descr descrbufsize
        4096 < while
        1 sleep
    repeat
;
 
: clear-refnames ( -- )
  me @ "_tempreg" remove_prop
;
  
: get-refname (d -- s)
  me @ over dbcmp if pop "me" exit then
  #0 over dbcmp if pop "#0" exit then
  me @ "_tempreg/" rot int intostr strcat getpropstr
  dup if "$" swap strcat then
;
  
: is-refname (d -- s)
  me @ "_tempreg/" rot int intostr strcat getpropstr
  not not
;
  
: set-refname (d s -- )
  me @ "_tempreg/" 4 rotate int intostr strcat rot 0 addprop
;
  
: in-environ? (d -- i)
  begin
    dup while
    dup originalobj @ dbcmp if pop 1 exit then
    location
  repeat pop 0
;
  
: dump-registration-loop ( d d s -- )
  begin
    over swap nextprop
    dup while
    over over getpropstr
    dup "#" 1 strncmp not if 1 strcut swap pop then
    dup not if pop "-1" then
    atoi dbref 4 pick dbcmp if
      "@register "
      3 pick me @ dbcmp if "#me " strcat then
      4 pick name strcat "=" strcat
      over 6 strcut swap pop strcat
      me @ swap notify
    then
    over over propdir? if
      3 pick 3 pick 3 pick "/" strcat
      dump-registration-loop
    then
  repeat
  pop pop pop
;
  
: dump-registration ( d d -- )
  (searchforobj propsobj )
  "/_reg/" dump-registration-loop
;
  
: get-globalrefs-loop (d s -- )
  begin
    over swap nextprop dup while
    over over getpropstr dup if
      dup "#" 1 strncmp not if 1 strcut swap pop then
      dup number? if
        atoi dbref over dup "/" instr
        strcut swap pop set-refname
      else pop
      then
    else pop
    then
    over over propdir? if
      over over "/" strcat get-globalrefs-loop
    then
  repeat pop pop
;
  
: get-globrefs ( -- )
  #0 "_reg/" get-globalrefs-loop
;
  
  
: translate-lockstr (s -- s)
  "" swap
  dup "*UNLOCKED*" stringcmp not if pop pop "" exit then
  begin
    dup "#" instr over or while
    "#" split
    rot rot strcat swap
    dup atoi intostr strlen
    strcut swap atoi dbref
    get-refname dup not if pop "(me&!me)" then
    rot swap strcat swap
  repeat
  strcat
;
  
: dump-lock (d -- )
  me @ "wizard" flag? if pop exit then
  dup "@/flk" getprop
  dup lock? not if pop pop exit then
  unparselock
  translate-lockstr
  "@flock " rot get-refname strcat
  "=" strcat swap strcat
  wait-for-buffer
  me @ swap notify
;
  
  
: dump-props-loop (s d s -- ) (refname object propdir -- )
  begin
    wait-for-buffer
    (refname object propdir -- )
    begin
      over swap nextprop
      (refname object propname -- )
      dup not if pop pop pop exit then
      "/" over strcat "/@" instr not
      me @ "wizard" flag? or
    until
    (refname object propname -- )
    over over getprop
    (refname object propname propval -- )
    dup string? if
      "/_/de:/_/sc:/_/fl:/_/dr" 3 pick tolower instr if
        (refname object propname propval -- )
        dup "@" 1 strncmp not if
          (refname object propname propval -- )
          1 strcut dup number? if
            " " split swap atoi dbref
            dup get-refname dup not if swap intostr then
            swap pop " " strcat swap strcat
          then
          strcat
        then
      then
      "@propset " 5 pick strcat
      "=str:" strcat 3 pick strcat
      ":" strcat swap strcat
      me @ swap notify
    else (not a string)
      dup int? if
        dup if
          "@propset " 5 pick strcat
          "=int:" strcat 3 pick strcat
          ":" strcat swap intostr strcat
          me @ swap notify
        else pop
        then
      else (not an int.)
        dup dbref? if
          dup get-refname
          dup not if "#" rot int intostr strcat then swap pop
          "@propset " 5 pick strcat
          "=dbref:" strcat 3 pick strcat
          ":" strcat swap strcat
          me @ swap notify
        else (not a dbref.)
          dup float? if
            dup if
              "@propset " 5 pick strcat
              "=float:" strcat 3 pick strcat
              ":" strcat swap ftostr strcat
              me @ swap notify
            else pop
            then
          else (not a float  Must be a lock.  Fun fun parse time.)
            (refname object propname propval -- )
            unparselock translate-lockstr
            "@propset " 5 pick strcat
            "=lock:" strcat 3 pick strcat
            ":" strcat swap strcat
            me @ swap notify
          then (float?)
        then (dbref?)
      then (int?)
    then (string?)
    (refname object propname -- )
    over over blessed? if
        "@bless " 4 pick strcat
        "=" strcat over strcat
        me @ swap notify
    then
    over over propdir? if
      3 pick 3 pick 3 pick
      "/" strcat dump-props-loop
    then
  repeat
;
  
: dump-props (d -- )  (object -- )
  dup get-refname swap "/" dump-props-loop
;
  
: dump-flags (d -- )
  dup unparseobj dup "#" rinstr strcut swap pop
  dup strlen 1 - strcut pop
  dup atoi intostr strlen strcut swap pop
  dup if
    1 strcut "RPEFM" 3 pick instr if
      swap pop "" swap
    then strcat
  then
  begin
    dup while
    dup "M" 1 strncmp not if 1 strcut swap pop continue then
    "@set " 3 pick get-refname strcat
    "=" strcat swap 1 strcut rot rot strcat
    wait-for-buffer
    me @ swap notify
  repeat
  pop pop
;
  
: dump-obj (d -- )
  wait-for-buffer
  dup ok? not if pop exit then
  one? @ if dup originalobj @ dbcmp not if pop exit then then
  owned? @ if dup owner originalobj @ owner dbcmp not if pop exit then then
  here? @ if dup in-environ? not if pop exit then then
  noprogs? @ if dup program? if pop exit then then
  dup is-refname if pop exit then
  dup room? if
    nolinks? @ not if
      dup getlink dump-obj
    then
    dup location dump-obj
    roomcnt @ 1 + roomcnt !
    "tmp/room" roomcnt @ intostr strcat
    (dbref regname)
    "@dig " 3 pick name strcat
    "=" strcat 3 pick location get-refname strcat
    "=" strcat over strcat
    me @ swap notify
    over swap set-refname
    dup getlink if
      "@link " over get-refname strcat
      "=" strcat over getlink get-refname strcat
      me @ swap notify
    then
    dup dump-lock
    dup dump-flags
    dup dump-props
    nocontents? @ not if
      dup contents
      begin
        dup while
        nofloater? @ if
          dup room? if
            next continue
          then
        then
        dup dump-obj
        next
      repeat pop
    then
    dup exits
    begin
      dup while
      dup dump-obj (dump exit)
      next
    repeat pop
    pop exit
  then
  dup player? if
    ( showplayers? @ not if pop exit then )
    dup originalobj @ dbcmp if
      nolinks? @ not if
        dup getlink dump-obj (dump room or object linked to)
      then
      playercnt @ 1 + playercnt !
      "tmp/player" playercnt @ intostr strcat
      "@pcreate " 3 pick name strcat
      "=<password>" strcat
      me @ swap notify
      "@register #me *" 3 pick name strcat
      "=" strcat over strcat
      me @ swap notify
      over swap set-refname
      "@link " over get-refname strcat
      "=" strcat over getlink get-refname strcat
      me @ swap notify
      dup dump-lock
      dup dump-flags
      dup dump-props
      nocontents? @ not if
        dup contents
        begin
          dup while
          dup dump-obj  (dump thing contents)
          next
        repeat pop
      then
      dup exits
      begin
        dup while
        dup dump-obj (dump exit)
        next
      repeat pop
    then
    pop exit
  then
  dup thing? if
    nolinks? @ not if
      dup getlink dump-obj (dump room or object linked to)
    then
    thingcnt @ 1 + thingcnt !
    "tmp/thing" thingcnt @ intostr strcat
    (dbref refname)
    "@create " 3 pick name strcat
    "=" strcat 3 pick pennies 1 + 5 * intostr strcat
    "=" strcat over strcat
    me @ swap notify
    over swap set-refname
    "@tel " over get-refname strcat
    "=" strcat over location get-refname strcat
    me @ swap notify
    "@link " over get-refname strcat
    "=" strcat over getlink get-refname strcat
    me @ swap notify
    dup dump-lock
    dup dump-flags
    dup dump-props
    nocontents? @ not if
      dup contents
      begin
        dup while
        dup dump-obj  (dump thing contents)
        next
      repeat pop
    then
    dup exits
    begin
      dup while
      dup dump-obj (dump exit)
      next
    repeat pop
    pop exit
  then
  dup exit? if
    nolinks? @ not if
      dup getlink dump-obj (dump room or object linked to)
    then
    exitcnt @ 1 + exitcnt !
    "tmp/exit" exitcnt @ intostr strcat
    (dbref refname)
    "@action " 3 pick name strcat
    "=" strcat 3 pick location get-refname strcat
    "=" strcat over strcat
    me @ swap notify
    over swap set-refname
    "@link " over get-refname strcat
    "=" strcat over getlink get-refname strcat
    me @ swap notify
    dup dump-lock
    dup dump-flags
    dup dump-props
    pop exit
  then
  dup program? if
    progcnt @ 1 + progcnt !
    "tmp/prog" progcnt @ intostr strcat
    (dbref refname)
    "@prog " 3 pick name strcat
    me @ swap notify
    me @ "1 99999 d" notify
    me @ "1 i" notify
    ( Old code: me @ "@list #" 4 pick intostr strcat force )
    over 0 0 program_getlines
    0 over array_count 1000 for
        over swap dup 999 + array_getrange
        { me @ }list array_notify
        wait-for-buffer
    repeat
    pop
    (dbref refname)
    me @ "." notify
    me @ "c" notify
    me @ "q" notify
    (dbref refname)
    over #0 dump-registration
    over me @ dump-registration
    over name "@register #me " swap strcat
    "=" strcat over strcat
    me @ swap notify
    over swap set-refname
    dup dump-lock
    dup dump-flags
    dup dump-props
    pop exit
  then
;
  
: archiver
  clear-refnames
  "=" split strip swap strip
  dup not if pop pop show-help exit then
  .match_controlled
  dup not if pop pop exit then
  swap tolower
  me @ "wizard" flag? not if "" "a" subst then
  dup "e" instr not here? !
  dup "a" instr not owned? !
  dup "c" instr nocontents? !
  dup "f" instr nofloater? !
  dup "l" instr nolinks? !
  dup "1" instr one? !
  dup "p" instr noprogs? !
  "i" instr not if get-globrefs then
  dup originalobj !
  me @ "[Start Dump]" notify
  dump-obj
  me @ "[End Dump]" notify
  clear-refnames
;
.
c
q
@register #me cmd-@archive=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3

