$define .tell me @ swap notify $enddef
$define sls striplead $enddef
$define sts striptail $enddef
$define strip sls sts $enddef
  
lvar regobj
lvar regprop
  
: split
    swap over over swap
    instr dup not if
        pop swap pop ""
    else
        1 - strcut rot
        strlen strcut
        swap pop
    then
;
  
: list-props ( d s -- )
  begin
    dup while
    dup strlen 1 - strcut
    dup "/" strcmp if
      strcat break
    else pop
    then
  repeat
  "/" strcat over swap nextprop
  begin
    dup while
    over over getpropstr
    dup if
      dup number? if
        atoi dbref
        dup ok? if
          dup unparseobj
          over "_version" getpropstr
          dup if
            "   Ver. " swap strcat strcat
          else pop
          then
          swap "_lib-version" getpropstr
          dup if
            "   Lib.ver. " swap strcat strcat
          else pop
          then
        else pop "<garbage>"
        then
      else pop "<garbage>"
      then
      over ": " strcat swap strcat
      regprop @ strlen strcut swap pop
      "  " swap strcat
      .tell
    else pop
    then
    over over propdir? if
      dup regprop @ strlen strcut swap pop
      "    " swap strcat
      "/ (directory)" strcat .tell
    then
    over swap nextprop
  repeat
  pop pop
;
  
: do-help
"Syntaxes:"
"  The following prefixes set what the target object and the target propdir"
"   are.  The default target propdir is \"_reg/\" and the default target"
"   object is #0."
"      #me"
"          Sets target object to you, and propdir to the default \"_reg/\"."
"      #prop <targobj>:<propdir>"
"          Sets target object to <targobj> and propdir to <propdir>."
"  The following are the command syntaxes"
"      @register [<prefix>]"
"          List all registered objects in the target propdir on target object."
"      @register [<prefix>] <subpropdir>"
"          List all registered objects in <subpropdir> in the target propdir."
"      @register [<prefix>] <object> = <name>"
"          Register <name> to <object> in the propdir on the target object."
"Example: @register #prop here:_disconnect gen-sweeproom=cleanup"
"  This registers the program gen-sweeproom as 'cleanup' in the _disconnect"
"   propdir on the current room."
  18
  begin
     dup while
     dup 1 + rotate .tell
     1 -
  repeat
;
  
: cmd-@register
  "me" match me !
  dup "#help" stringcmp not if
    do-help exit
  then
  sls dup tolower "#me" 3 strncmp not if
    " " split swap pop
    me @ regobj !
    "_reg/" regprop !
  else
    dup tolower "#prop" 5 strncmp not if
      " " split swap pop
      " " split swap
      ":" split
      (rest obj prop)
      swap dup not if pop "me" then
      dup "@" strcmp not if pop "#0" then
      match dup not if
        me @ "I don't see that target object here." notify
        pop pop pop exit
      then
      dup #-2 dbcmp if
        me @ "I don't know which target object you mean." notify
        pop pop pop exit
      then
      me @ over owner dbcmp
      me @ "wizard" flag? or not if
        me @ "Permission Denied." notify
        pop pop pop exit
      then
      regobj !
      dup not if pop "/" then      (if no propdir selected, use default)
      dup dup strlen 1 - strcut swap pop
      "/" strcmp if "/" strcat then    (if propdir doesn't end in /, append /)
      regprop !
    else
      me @ "w" flag? not
      over "=" instr and if
        me @ "Permission denied." notify
        pop exit
      then
      #0 regobj !
      "_reg/" regprop !
    then
  then
  
  dup "=" instr not if
    "Registered objects on "
    regobj @ unparseobj strcat
    ":" strcat .tell
    regobj @ regprop @ rot strcat list-props
    "Done." .tell exit
  then
  "=" split strip swap strip
  match dup not if
    "I don't see that object here." .tell pop exit
  then
  dup #-2 dbcmp if
    "I don't know which object you mean." .tell pop exit
  then
  
  int swap
  " " split if
    pop pop "You cannot have spaces in the registration name." .tell exit
  then
  regobj @ regprop @ 3 pick strcat getpropstr
  dup if
    dup "#" 1 strncmp not if 1 strcut swap pop then
    regprop @ 3 pick strcat " used to be registered to " strcat
    swap atoi dbref unparseobj strcat
    .tell
  else pop
  then
  regobj @ regprop @ 3 pick strcat 4 pick intostr 0 addprop
  regprop @ swap strcat " is now registered to " strcat
  swap dbref unparseobj strcat " on " strcat 
  regobj @ unparseobj strcat
  .tell
;
