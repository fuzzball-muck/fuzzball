@prog cmd-fetch
1 99999 d
1 i
$include $lib/strings
$include $lib/match
  
: fetch
  " from " .split .strip swap .strip swap
  (itemS contS)
  dup not if
    pop trigger @ "_prefs/container" getpropstr
    dup not if pop me @ "_prefs/container" getpropstr then
    dup not if
      me @ "Syntax:  fetch <object> from <container>" notify
      me @ "  or:  fetch <object>   (with a _prefs/container set)"
      notify exit
    then
  then
  match dup #-2 dbcmp if
    me @ "I don't know which container you mean." notify exit
  then
  dup not if
    me @ "I don't see that container here." notify exit
  then
  dup me @ location dbcmp not
  over location me @ dbcmp not and if
    me @ "You must be carrying a container to remove something from it."
    notify exit
  then

  (itemS contD)
  dup rot dup "all" stringcmp not if pop "*" then .multi_rmatch
  (contD itemDn ... itemD1 itemcountI)
  dup not if
    me @ "I don't see that item in the container." notify exit
  then
  (contD itemDn ... itemD1 itemcountI)
  dup 2 + rotate
  (itemDn ... itemD1 itemcountI contD)
  begin
    over while     (If all items handled, then exit)
    swap 1 - swap  (decrement counter)
    rot
    dup thing? over program? or not if pop continue then
    over room? if
      me @ over locked? if
	dup fail dup not if
	  pop "You can't pick " over name strcat " up." strcat
	then .tell
	dup ofail if
	  me @ name " " strcat over ofail strcat
	  me @ swap pronoun_sub
	  me @ location me @ rot notify_except
	then
        pop continue
      else
	dup succ dup not if pop "Taken." then .tell
	dup osucc if
	  me @ name " " strcat over osucc strcat
	  me @ swap pronoun_sub
	  me @ location me @ rot notify_except
	then
      then
    else
      "Fetching " over name strcat
      " from " strcat 3 pick name strcat
      "." strcat .tell
    then
    (itemDn ... itemD2 itemcountI-- contD itemD1)
    me @ moveto
  repeat
;
.
c
q
@register #me cmd-fetch=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=/_/de:A scroll containing a spell called cmd-fetch
#ifdef NEW
@action fetch;retrieve;grab=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
#endif
