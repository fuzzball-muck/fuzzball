@prog cmd-@when
1 99999 d
1 i
$define .nif not if $enddef
$define .tellme me @ swap notify $enddef
: process-time (i -- s)
                   ( Processes time to format: Day Month Date 24hr-time Year )
   "%a %b %d %H:%M:%S %Y" swap timefmt
;
: parse-dbref (s -- d i)
                   ( Returns 0 if object does not match anything. )
                   ( Returns 1 if object exists but not controlled by me @ )
                   ( Returns 2 if object is controlled by me @ )
                   ( Returns 3 if object is garbage )
                   ( Returns 4 if object is over dbtop )
                   ( Returns 5 if object is not uniquely determined )
   dup match dup #-1 dbcmp if
      pop dup "#" 1 strncmp if
         pop #-1 0 exit
      then 1 strcut swap pop atoi dup dbtop intostr atoi < .nif
         pop #-1 4 exit
      else
         dbref
      then
   then dup #-2 dbcmp if
      pop #-1 5 exit
   then dup ok? .nif
      pop #-1 3 exit
   then dup owner me @ dbcmp me @ "W" flag? or
   1 +
;
: listflags (d -- s)
                  ( Gives a listing of flags on an item. )
                  ( E.g. A program set SDL returns: )
                  ( "SETUID DEBUGGING LINK_OK " )
   "" over unparseobj "#" explode begin 1 - dup while
      swap pop repeat pop
   "W" instr if
      "WIZARD " strcat
   then over "Q" flag? if
      "QUELL " strcat
   then over "S" flag? if
      over player? if
         "SILENT " strcat
      else
         over program? if
            "SETUID " strcat
         else
            "STICKY " strcat
         then
      then
   then over "D" flag? if
      over program? if
        "DEBUGGING " strcat
      else
        "DARK " strcat
      then
   then over "L" flag? if
      "LINK_OK " strcat
   then over "M" flag? if 2 else 0 then
   3 pick "N" flag? if 1 else 0 then +
   dup if
      "MUCKER" swap intostr strcat
      " " strcat strcat
   else pop
   then over "B" flag? if
      "BUILDER " strcat
   then over "C" flag? if
      "CHOWN_OK " strcat
   then over "J" flag? if
      "JUMP_OK " strcat
   then over "H" flag? if
      over program? if
         "HARDUID " strcat
      else 
         "HAVEN " strcat
      then
   then over "A" flag? if
      over program? if
         "AUTOSTART " strcat
      else
         "ABODE " strcat
      then
   then swap pop
;
: gettype (d -- s)
                  ( Returns a one-word type of the object )
   dup thing? if
      "THING" swap pop exit
   then dup program? if
      "PROGRAM" swap pop exit
   then dup player? if
      "PLAYER" swap pop exit
   then dup room? if
      "ROOM" swap pop exit
   then
   "EXIT" swap pop
;
: when  ( The main body... )
   dup "" strcmp .nif
      me @ location intostr "#" swap strcat
   then parse-dbref dup .nif
      "I don't see that here." .tellme exit
   then dup 5 = if
      "I don't know which one you mean." .tellme exit
   then dup 4 = if
      "I don't see that here." .tellme exit
   then dup 3 = if
      "<garbage> is garbage." .tellme exit
   then 1 = if
      dup player? if
         name " is a player." strcat
      else
         owner "Owner: " swap name strcat
      then .tellme exit
   then dup unparseobj " Owner: " strcat over owner name strcat .tellme
   "Type: " over gettype strcat
   "  Flags: " 3 pick listflags dup "" strcmp .nif
      pop pop
   else
      strcat strcat
   then .tellme timestamps 4 rotate
   process-time "Created:     " swap strcat .tellme
   rot process-time "Modified:    " swap strcat .tellme
   swap process-time "Last used:   " swap strcat .tellme
   intostr "Usecount:    " swap strcat .tellme
;
.
c
q
@register #me cmd-@when=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=2
#ifdef NEW
@action @when;@whe;@wh=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
#endif
