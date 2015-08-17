@prog cmd-mv-cp
1 99999 d
i
( cmd-mv-cp
    cp [origobj][=prop],[destobj][=destprop]
      Copies prop from origobj to destobj, renaming it to destprop.

    mv [origobj][=prop],[destobj][=destprop]
      Moves prop from origobj to destobj, renaming it to destprop.

      Arguments in []s are optional.
      if origobj is omitted, it assumes a property on the user.
      if destobj is omitten, it assumes destobj is the same as origobj.
      if destprop is omitted, it assumes the same name as prop.
      if prop is omitted, it prompts for it.
      if both prop and origobj are omitted, it prompts for both.
      if both destobj and destprop are omitted, it prompts for them.

  CHANGES:
    9/99 Added checkperms [ s --  ] to allow the program to safely be
    set Wizard. cp and mv will copy or move ~ and @ props only if the
    user is a !Quelled Wizard. -- Jessy
    9/99 Added a line at the beginning of main to catch dbref spoofing.
    -- Jessy
)

$include $lib/strings
$include $lib/edit
$include $lib/match

lvar copy?

: checkperms ( s --  )
  dup "@" stringpfx
  over "/@" instr
  3 pick "~" stringpfx
  4 rotate "/~" instr or or or
  me @ "W" flag? not and if
    "Permission denied." .tell pid kill
  then
;

: cp-mv-prop ( d s d s -- i )
  dup checkperms
  3 pick checkperms
  4 pick 4 pick getpropstr
  dup if
    3 pick 3 pick rot 0 addprop
  else
    pop 4 pick 4 pick getpropval
    dup if
      3 pick 3 pick "" 4 rotate addprop
    else
      pop 4 pick 4 pick propdir? not if
        "I don't see what property to "
        copy? @ if "copy." else "move." then
        strcat .tell pop pop pop pop 0 exit
      then
    then
  then
  4 pick 4 pick propdir? if
    4 pick dup 5 pick "/" strcat nextprop
    (d s dd ds d ss)
    begin
      dup while
      over dup 3 pick nextprop
      4 rotate 4 rotate
      6 pick 6 pick "/" strcat
      3 pick dup "/" rinstr
      strcut swap pop strcat
      cp-mv-prop pop
    repeat
    pop pop
  then
  pop pop
  copy? @ not if remove_prop else pop pop then
  1
;

: cp-prop ( d s d s -- i )
  1 copy? ! cp-mv-prop
;
public cp-prop

: mv-prop ( d s d s -- i )
  0 copy? ! cp-mv-prop
;
public mv-prop

: strip-slashes ( s -- s' )
  begin
    dup while
    dup strlen 1 - strcut
    dup "/" strcmp if strcat exit then
    pop
  repeat
;

: parse-command ( s -- )
  "me" match me !
  command @ tolower
  "command @ = \"" over strcat "\"" strcat .tell
  "c" instr copy? !

  dup "#help" stringcmp not if
    pop
    copy? @ if
      "cp origobj=prop,destobj=destprop"
      "  Copies prop from origobj to destobj, renaming it to destprop."
    else
      "mv origobj=prop,destobj=destprop"
      "  Moves prop from origobj to destobj, renaming it to destprop."
    then
    " "
    "  if origobj is omitted, it assumes a property on the user."
    "  if destobj is omitted, it assumes destobj is the same as origobj."
    "  if destprop is omitted, it assumes it is the same name as prop."
    "  if prop is omitted, it asks the user for it."
    "  if both prop and origobj are omitted, it asks the user for both."
    "  if both destobj and destprop are omitted, it asks the user for them."
    depth EDITdisplay exit
  then

  "," .split .strip swap .strip
  "=" .split .strip swap .strip
  dup if
    .noisy_match
    dup not if pop exit then
  else
    over not if
      "Please enter the name of the original object."
      .tell pop read .strip
    then
    dup if
      .noisy_match
      dup not if pop exit then
    else pop me @
    then
  then
  -3 rotate
  begin
    strip-slashes
    dup not while
    "Please enter the name of the original property." .tell
    pop read .strip
  repeat
  swap
  "=" .split .strip strip-slashes swap .strip
  begin
    over over or not while
    pop pop
    "Please enter the name of the destination object." .tell
    read .strip
    "Please enter the name of the destination property." .tell
    read .strip
    strip-slashes swap
  repeat
  dup if
    .noisy_match
    dup not if pop exit then
  else
    pop 3 pick
  then
  swap
  dup not if pop over then

  (origdbref origpropname destdbref destpropname)
  cp-mv-prop if
    copy? @ if "Property copied."
    else "Property moved."
    then .tell
  then
;
.
c
q
@register cmd-mv-cp=global/mv-cp
@register #me cmd-mv-cp=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=/_/de:A scroll containing a spell called cmd-mv-cp
@set $tmp/prog1=/_defs/copy-prop:"$global/mv-cp" match "cp-prop" call
@set $tmp/prog1=/_defs/move-prop:"$global/mv-cp" match "mv-prop" call
@action cp;mv;propcp;propmv=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
