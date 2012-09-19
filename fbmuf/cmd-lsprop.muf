@prog cmd-lsprop
1 99999 d
1 i
( cmd-lsprop - display all props on an object, or in subdirectories on
  an object, that match the given wildcard expression.
)
$include $lib/match
$include $lib/strings
  
lvar propcount
  
: showprop (d s -- )
  over over getpropstr dup if
    ": (string) " swap strcat
  else
    pop over over getpropval dup if
      ": (integer) " swap intostr strcat
    else
      pop ": (no value)"
    then
  then
  3 pick 3 pick propdir? if "/" else "" then
  3 pick swap strcat swap strcat
  me @ swap notify
  propcount @ 1 + propcount !
;
  
  
: listprops (s d s -- ) (smatchstr objref startprop)
  begin
    begin
      over swap nextprop
      dup if
        (if player isn't wizard, skip @props)
        dup "/@" instr if
          me @ "wizard" flag? not if continue then
        then
      then
    1 until
    dup while   (exit if we're done with this propdir.)
  
    (search contents of sub-propdirs for matches)
    over over propdir? if
      3 pick 3 pick 3 pick "/" strcat
      listprops
    then
  
    (if prop doesn't match, ignore it)
    over over propdir? if "/" else "" then
    over swap strcat 4 pick smatch not if continue then
  
    showprop
  repeat
  pop pop pop
;
  
  
: plist (s -- )
  0 propcount !
  "=" .split strip swap strip
  .match_controlled dup not if pop exit then
  swap "*" strcat
  dup "/" 1 strncmp if "/" swap strcat then
  swap "/" listprops
  propcount @ intostr " properties listed." strcat
  me @ swap notify
;
.
c
q
@set cmd-lsprop=w
#ifdef NEW
@action lsprop;lsp=#0=tmp/exit1
@link $tmp/exit1=cmd-lsprop
#endif
