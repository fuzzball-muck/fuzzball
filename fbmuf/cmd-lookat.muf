/sub off
@prog cmd-lookat
1 9999 d
1 i
( cmd-lookat  Copyright 9/11/91 by Garth Minette                 )
(                                 foxen@netcom.com               )
  
$def VERSION "MUFlookat v1.30 by Foxen"
$def UPDATED "Updated 2/7/94"
  
(version 1.00 9/11/91 Foxen)
(version 1.20 6/05/93 Foxen)
(version 1.30 2/07/94 Foxen)
  
  
$include $lib/props
  
: tell (string -- )
    me @ swap notify
;
  
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
  
  
$def strip-leadspaces striplead
$def strip-trailspaces striptail
$def stripspaces striplead striptail
  
: single-space (s -- s') (strips all multiple spaces down to a single space)
    dup "  " instr not if exit then
    " " "  " subst single-space
;
  
  
( help stuff )
  
  
: show-help-list
    dup not if pop exit then
    dup 1 + rotate me @ swap notify
    1 - show-help-list
;
  
  
  
: show-help
VERSION "   " strcat UPDATED strcat "   Page1" strcat
"----------------------------------------------------------------------------"
"Syntax:  lookat <player>'s <object>"
"    or:  lookat <player> <object>"
"    Lets you look at an object that a player is carrying, and see its @desc."
"  "
"You can set these properties on objects:"
"    _remote_desc:<desc>  Shown instead of the objects @desc.  If this (or"
"                          the @desc if this doesn't exist) uses a program"
"                          like @6800, it will handle it properly."
"    _remote_look?:yes    Allows lookat'ing for that object.  On a player,"
"                          allows looking at anything they carry unless the"
"                          object is set _remote_look?:no"
"----------------------------------------------------------------------------"
14 show-help-list
;
  
  
  
  
: main
    dup "#help" stringcmp not if pop show-help exit then
    dup " " instr not if
        "Syntax:    lookat <player>'s <object>"
        tell pop exit
    then
    " " split stripspaces swap stripspaces
    dup strlen 2 > if
        dup dup strlen 2 - strcut
        dup "'s" stringcmp not if
            pop match
            dup ok? not if
                pop dup match
            then
        else strcat match
        then
    else dup match
    then
    dup not if
        pop "I don't see \"" swap strcat
        "\" here." strcat tell pop exit
    then
    dup #-2 dbcmp if
        "I don't know which player you mean."
        tell pop pop pop exit
    then
    swap pop swap rmatch
    dup not if
        "I don't see them carrying that."
        tell pop exit
    then
    dup #-2 dbcmp if
        "I don't know which object you mean."
        tell pop exit
    then
    dup "_remote_look?" .envprop "no" stringcmp not
    over "dark" flag? or if
        "You can't see that clearly."
        tell pop exit
    then
    dup "_remote_desc" "(_remote_desc)" 1 parseprop
    dup not if pop dup "_/de" "(@Desc)" 1 parseprop then
    dup not if pop "You see nothing special." then
    dup "@" 1 strncmp not if
        1 strcut swap pop
        " " split over number?
        3 pick "$" 1 strncmp not or if
            swap dup "$" 1 strncmp not
            if match else atoi dbref then
            dup program? if
                rot trigger ! call
            else pop tell pop
            then
        else
            " " swap strcat strcat
            "@" swap strcat tell pop
        then
    else tell pop
    then
;
.
c
q
/sub on
