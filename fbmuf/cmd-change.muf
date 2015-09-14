@prog cmd-change
1 99999 d
1 i
( cmd-change                                                         )
(   This is a program that will let you replace errors in a message  )
(   or property so that you can fix up typo's without having to      )
(   retype the entire thing.                                         )
(                                                                    )
( Usage:  change <object>=<prop>:/<old>/<new>      or                )
(         change <object>=<mesg>;/<old>/<new>                        )
(                                                                    )
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
 
: in-string? (str searchstr -- bool)
    instr dup not if
      "I don't see the sequence you want me to change." .tell
    then
;
  
: replace-text ( str old new -- str )
(doesn't crash the server like subst with expanding strs to >4096 chars )
    3 pick 3 pick instr dup if
      1 - 4 rotate swap strcut
      4 pick strlen strcut swap pop
      4 pick 4 pick replace-text
      3 pick swap strcat strcat
      rot rot
    else pop
    then
    pop pop
;
  
: error
    "Name: Change v1.02   Written by Tygryss   Last updated 3/31/92" .tell
    "Desc: Lets you replace some text in a message or property with" .tell
    "       new text.  Useful for fixing typos in a long message." .tell
    " " .tell
    "Syntax: change <object>=<propname>:/<old>/<new>   or" .tell
    "        change <object>=<mesgtype>;/<old>/<new>" .tell
    " " .tell
    "<mesgtype> can be name/desc/succ/osucc/fail/ofail/drop/odrop" .tell
    "The first character after the : or ; is the delimiter character," .tell
    "  in this case a '/'." .tell
;
  
: change-main
		"me" match me !
    "=" .split
    dup not if error exit then
    swap .stripspaces
    dup not if error exit then
    swap dup ":" instr over ";" instr
    over not over not and if error exit then
    dup not if pop 5000 then swap
    dup not if pop 5000 then swap
    < if ( : for property? )
        ":" .split
        dup not if error exit then
        swap .stripspaces
        dup not if error exit then
        swap 1 strcut swap over over
        instr not if error exit then
        swap over .split rot over swap
        instr if error exit then
        4 rotate match
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
        4 rotate over over 
				dup checkperms
				getpropstr
        dup not if
            "I can't change a property that doesn't exist." .tell
            pop pop pop pop pop exit
        then
        dup 6 pick in-string? not if pop pop pop pop pop exit then
        5 rotate 5 rotate replace-text
        dup not if
            pop remove_prop
        else
            0 addprop
        then
        "Property changed." .tell
    else  ( ; for message? )
        ";" .split
        dup not if error exit then
        swap .stripspaces
        dup not if error exit then
        swap 1 strcut swap over over
        instr not if error exit then
        swap over .split rot over swap
        instr if error exit then
        4 rotate match
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
        dup 5 rotate
        dup "@" 1 strncmp not if 1 strcut swap pop then
        dup      "name" stringcmp not if
            pop name 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setname
        else dup "desc" stringcmp not if
            pop desc 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setdesc
        else dup "succ" stringcmp not if
            pop succ 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setsucc
        else dup "osucc" stringcmp not if
            pop osucc 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setosucc
        else dup "fail" stringcmp not if
            pop fail 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setfail
        else dup "ofail" stringcmp not if
            pop ofail 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setofail
        else dup "drop" stringcmp not if
            pop drop 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setdrop
        else dup "odrop" stringcmp not if
            pop odrop 4 rotate 4 rotate
            3 pick 3 pick in-string? not if pop pop pop pop exit then
            replace-text setodrop
        else
            "I don't recognize the field named \""
            swap strcat "\"" strcat .tell exit
        then then then then then then then then
        "Message changed." .tell
    then
;
.
c
q
@register #me cmd-change=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=/_/de:A scroll containing a spell called cmd-change
