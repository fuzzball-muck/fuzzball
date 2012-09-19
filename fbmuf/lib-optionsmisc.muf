@prog lib-optionsmisc
1 99999 d
1 i
( $lib/optionsmisc Copyright 8/13/2003 by Revar <revar@belfry.com> )
( Released under the GNU LGPL.                                     )
(                                                                  )
( A MUF library to provide common internal support for gui and     )
( menu driven editing of option values.                            )
(                                                                  )
( v1.000 --  8/13/2003 -- Revar <revar@belfry.com>                 )
(     Initial relase.                                              )

$author Revar Desmera <revar@belfry.com>
$version 1.000
$lib-version 1.000
$note Released under the LGPL.

(
    Docs go here.
)

$include $lib/case

( ------------------------------------------------------------- )


: optmisc_equalvals?[ any:val1 any:val2 -- bool:equal ]
    { val1 @ }list
    { val2 @ }list
    array_compare not
;


: optmisc_timespan2str[ int:timespan -- str:result ]
    {
    "%id %02i:%02i:%02i"
    timespan @
    dup 86400 / swap 86400 %
    dup 3600 / swap 3600 %
    dup 60 / swap 60 %
    } reverse fmtstring
;


: optmisc_objtype_normalize[ validtypes -- list:validtyes ]
    validtypes @
    dup not if
        pop "any"
    then
    dup string? if
        { swap }list
    then
    dup "any" array_matchval if
        { "room" "thing" "exit" "player" "program" "garbage" "bad" }list array_union
    then
    dup "room" array_matchval if
        "abode"  swap array_appenditem
    then
    dup "thing" array_matchval if
        "zombie"  swap array_appenditem
        "vehicle" swap array_appenditem
    then
;


: optmisc_objtype_check[ ref:who ref:obj dict:item -- str:errs ]
    item @ "label" [] var! label
    item @ "objtype" [] optmisc_objtype_normalize dup var! validtypes
    obj @ case
        #-1 dbcmp when "nothing" array_matchval end
        #-3 dbcmp when "home"    array_matchval end
        ok? not   when "bad"     array_matchval end
        exit?     when "exit"    array_matchval end
        player?   when "player"  array_matchval end
        program?  when "program" array_matchval end
        room?     when
            0
            obj @ "a" flag? if over "abode" array_matchval or then
            over "room" array_matchval or
            swap pop
        end
        thing?    when
            0
            obj @ "z" flag? if over "zombie"  array_matchval or then
            obj @ "v" flag? if over "vehicle" array_matchval or then
            over "thing" array_matchval or
            swap pop
        end
        default pop "garbage" array_matchval end
    endcase
    not if
        {
            "'%s' must be one of the following types: %s"
            label @
            validtypes @ ", " array_join
        } reverse fmtstring
        exit
    then
    obj @ ok? if
        item @ "linkable" [] if
            who @ obj @ controls not
            obj @ "L" flag? not and
            obj @ "A" flag? obj @ room? and not and
            if
                {
                    "'%s' must be an object you can link to."
                    label @
                } reverse fmtstring
                exit
            then
        then
        item @ "control" [] if
            obj @ who @ controls not if
                {
                    "'%s' must be an object you control."
                    label @
                } reverse fmtstring
                exit
            then
        then
    then
    ""
;


: optmisc_dbref_option_list[ ref:who list:item -- list:objstrs ]
    { }list var! out

    ( Me, Here, and Home )
    who @ who @ item @ optmisc_objtype_check not if
        "Me" out @ array_appenditem out !
    then
    who @ who @ location item @ optmisc_objtype_check not if
        "Here" out @ array_appenditem out !
    then
    who @ #-1 item @ optmisc_objtype_check not if
        "*NOTHING* (#-1)" out @ array_appenditem out !
    then
    who @ #-3 item @ optmisc_objtype_check not if
        "*HOME* (#-3)" out @ array_appenditem out !
    then

    ( Player's inventory )
    who @ contents_array
    foreach swap pop
        who @ over item @ optmisc_objtype_check not if
            unparseobj out @ array_appenditem out !
        else pop
        then
    repeat

    ( Room contents )
    who @ location contents_array
    foreach swap pop
        who @ over item @ optmisc_objtype_check not if
            unparseobj out @ array_appenditem out !
        else pop
        then
    repeat

    ( Environment rooms )
    who @ location location
    begin
        dup while
        who @ over item @ optmisc_objtype_check not if
            dup unparseobj out @ array_appenditem out !
        then
        location
    repeat pop

    ( Player exits )
    who @ exits_array
    foreach swap pop
        who @ over item @ optmisc_objtype_check not if
            unparseobj out @ array_appenditem out !
        else pop
        then
    repeat

    ( Room exits )
    who @ location exits_array
    foreach swap pop
        who @ over item @ optmisc_objtype_check not if
            unparseobj out @ array_appenditem out !
        else pop
        then
    repeat

    out @
;


: optmisc_dbref_unparse[ ref:who ref:obj -- str:objstr ]
    obj @ ok? if
        obj @ player? if
            obj @ who @ dbcmp if
                "Me" exit
            else
                "*" obj @ name strcat exit
            then
        then
        obj @ who @ location dbcmp if
            "Here" exit
        then
        who @ obj @ controls if
            obj @ unparseobj exit
        then
        obj @ name exit
    then
    obj @ #-1 dbcmp if
        "*NOTHING* (#-1)" exit
    then
    obj @ #-3 dbcmp if
        "*HOME* (#-3)" exit
    then
    obj @ int "#%i" fmtstring
;


: optmisc_dbref_parse[ str:objstr ref:who -- ref:obj ]
    objstr @ "me" stringcmp not if who @ exit then
    objstr @ "here" stringcmp not if who @ location exit then
    objstr @ not if #-1 exit then
    objstr @ match
    dup int 0 >= if exit then
    dup #-2 dbcmp if exit then
    pop
    objstr @ "*(#[0-9]*)" smatch if
        objstr @ "(#" rsplit swap pop ")" rsplit pop
        dup 1 strcut pop number? if
            atoi dbref exit
        else pop
        then
    then
    #-1
;


public optmisc_equalvals?
public optmisc_timespan2str
public optmisc_objtype_normalize
public optmisc_objtype_check
public optmisc_dbref_option_list
public optmisc_dbref_unparse
public optmisc_dbref_parse

$libdef optmisc_equalvals?
$libdef optmisc_timespan2str
$libdef optmisc_objtype_normalize
$libdef optmisc_objtype_check
$libdef optmisc_dbref_option_list
$libdef optmisc_dbref_unparse
$libdef optmisc_dbref_parse
.
c
q
@register lib-optionsmisc=lib/optionsmisc
@register #me lib-optionsmisc=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3

