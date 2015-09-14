@prog cmd-edit
1 99999 d
1 i
( cmd-edit
(   This is a program that will let you replace errors in a message  )
(   or property so that you can fix up typo's without having to      )
(   retype the entire thing.                                         )
(                                                                    )
( Usage:  edit <object>=<prop>      or                               )
(         edit <object>=@<mesg>                                      )
(                                                                    )
( CHANGES: Added a 'checkperms' routine to prevent non-wiz users     )
(          from changing @wizard or ~restricted props -- Jessy 7/00  )
(                                                                    )
  
$include $lib/strings
  
: checkperms ( s --  )
  dup "@" stringpfx
  over "/@" instr
  3 pick "~" stringpfx
  4 rotate "/~" instr or or or
  me @ "W" flag? not and if
    "Permission denied." .tell pid kill
  then
;
 
: replace-text ( str -- str )
    "Please enter the text it should be changed to." .tell
    "##edit> " swap strcat .tell read
;
  
: error
    "Name: Edit v1.02   Written by Tygryss   Last updated 3/31/92" .tell
    "Desc: Lets you use the tinyfugue /grab feature to edit a message" .tell
    "       or property.  Requires tinyfugue 1.5.0 or later with this" .tell
    "       trigger defined:  /def -fg -p100 -t\"##edit> *\" = /grab %-1" .tell
    " " .tell
    "Syntax: edit <object>=<propname>   or" .tell
    "        edit <object>=@<mesgtype>" .tell
    " " .tell
    "<mesgtype> can be name/desc/succ/osucc/fail/ofail/drop/odrop" .tell
;
  
: change-main
		"me" match me !
    "=" .split .stripspaces
    dup not if error exit then
    swap .stripspaces
    dup not if error exit then
    swap dup "@" 1 strncmp if ( property? )
        swap match
        dup #-1 dbcmp if
            "I don't see that here."
            .tell exit
        then
        dup #-2 dbcmp if
            "I don't know which one you mean!"
            .tell exit
        then
        dup #-3 dbcmp if
            "I don't know what you mean!"
            .tell exit
        then
        dup owner me @ dbcmp not
        me @ "w" flag? not and if
            "Permission denied."
            .tell exit
        then
        swap over over 
				dup checkperms
				getpropstr
        replace-text
        dup not if
            pop remove_prop
        else
            0 addprop
        then
        "Property changed." .tell
    else  ( ; for message? )
        1 strcut swap pop
        swap match
        dup #-1 dbcmp if
            "I don't see that here."
            .tell exit
        then
        dup #-2 dbcmp if
            "I don't know which one you mean!"
            .tell exit
        then
        dup #-3 dbcmp if
            "I don't know what you mean!"
            .tell exit
        then
        dup owner me @ dbcmp not
        me @ "w" flag? not and if
            "Permission denied."
            .tell exit
        then
        swap dup "name" stringcmp not if
            pop dup name replace-text setname
        else dup "desc" stringcmp not if
            pop dup desc replace-text setdesc
        else dup "succ" stringcmp not if
            pop dup succ replace-text setsucc
        else dup "osucc" stringcmp not if
            pop dup osucc replace-text setosucc
        else dup "fail" stringcmp not if
            pop dup fail replace-text setfail
        else dup "ofail" stringcmp not if
            pop dup ofail replace-text setofail
        else dup "drop" stringcmp not if
            pop dup drop replace-text setdrop
        else dup "odrop" stringcmp not if
            pop dup odrop replace-text setodrop
        else pop error
        then then then then then then then then
        "Message changed." .tell
    then
;
.
c
q
@register #me cmd-edit=tmp/prog1
@set $tmp/prog1=W
