@prog cmd-put
1 99999 d
1 i
$include $lib/strings
$include $lib/match
  
: cont-put
  " in " .split .strip swap .strip swap
  dup not if
    pop trigger @ "_prefs/container" getpropstr
    dup not if pop me @ "_prefs/container" getpropstr then
    dup not if
      me @ "Syntax:  put <object> in <container>" notify exit
      me @ "    or:  put <object>   (with a _prefs/container set)"
      notify exit
    then
  then
  match dup #-2 dbcmp if
    me @ "I don't know which container you mean." notify exit
  then
  dup not if
    me @ "I don't see that container here." notify exit
  then
  dup location me @ dbcmp not if
    me @ "You must be carrying a container to put something in it."
    notify exit
  then
  (ItemS contD)
  
  me @ rot dup "all" stringcmp not if pop "*" then .multi_rmatch
  (ContD ItemDn .. ItemD1 itemcountI)
  dup not if
    me @ "I don't see that item in your inventory." notify exit
  then
  
  dup 2 + rotate
  (itemDn ... itemD1 itemcountI contD)
  begin
    over while     (If all items handled, then exit)
    swap 1 - swap  (decrement counter)
    rot over over dbcmp if
      "You can't put something inside itself.  Thats just plain silly."
      me @ swap notify pop continue
    then
    dup name "Putting " swap strcat
    " in " strcat 3 pick name strcat
    "." strcat .tell
    (itemDn ... itemD2 itemcountI-- contD itemD1)
    over moveto
  repeat
;
.
c
q
@register #me cmd-put=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=/_/de:A scroll containing a spell called cmd-put
#ifdef NEW
@action put;replace;stuff=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
#endif
