( smell6.muf by Natasha@HLM
  A smell, feel, and taste program for Fuzzball 6.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.0
$note A smell, feel, and taste program for Fuzzball 6.
 
$include $lib/match
$include $lib/strings
 
$define dict_pastforms {
    "feel" "felt"
    "taste" "tasted"
}dict $enddef
 
var obj
 
: rtn-parse  ( db str -- str' )
    dup "%ved" instr if
        dict_pastforms command @ array_getitem  ( db str' strVed )
        dup not if pop command @ "ed" strcat then  ( db str' strVed )
        "%ved" subst  ( db str' )
    then  ( db str' )
 
    command @ "%v" subst  ( db str' )
    obj @ "%obj" subst  ( db str' )
 
    over name "%N" subst  ( db str' )
    pronoun_sub  ( str' )
;
 
: rtn-set-notify ( strSucc strProp strQ strY -- )
    dup not if pop  ( strSucc strProp strQ )
        command @ swap fmtstring .tell  ( strSucc strProp )
        read strip  ( strSucc strProp strY' )
        dup "." strcmp not if pop "Cancelled." .tell exit then  ( strSucc strProp strY' )
        dup not if  ( strSucc strProp strY' )
            pop swap pop  ( strProp )
            me @ command @ rot fmtstring remove_prop  (  )
            "Cleared." .tell exit  (  )
        then  ( strSucc strProp strY' )
    else swap pop then  ( strSucc strProp strY )
 
    me @ command @ 4 rotate fmtstring  ( strSucc strY dbMe strProp' )
    3 pick setprop  ( strSucc strY )
    command @ rot fmtstring .tell  (  )
;
 
: do-set
    "Your %s message has been set to: %s" "_prefs/%s" "What do you want to %s like? Enter '.' to cancel or ' ' (space) to clear."
    4 rotate rtn-set-notify
;
 
: do-notify  ( strY -- )
    "Your %s notify message has been set to: %s" "_prefs/%s_notify" "What do you want to see when someone %ss you? Enter '.' to cancel or ' ' to clear."
    4 rotate rtn-set-notify
;
 
: do-help pop .showhelp ;
 
$define dict_hashcmds {
    "set" 'do-set
    "format" 'do-notify
    "notify" 'do-notify
    "help" 'do-help
}dict $enddef
 
: main
    command @ tolower command !
 
    STRparse dup if "=" swap strcat strcat else pop then  ( strX strY )
    over over or not if pop pop "help" "" then  ( strX strY )
 
    swap dup if  ( strY strZ )
        dict_hashcmds over array_getitem  ( strY strX ? )
        dup address? if  ( strY strX adr )
            swap pop execute  (  )
        else pop  ( strY strX )
            "I don't know what you mean by '#%s'." fmtstring .tell  ( strY )
            pop  (  )
        then exit  (  )
    then pop  ( strY )
 
    ( Whom am I %verbing? )
    "you" obj !
    dup match dup ok? if  ( strY db )
        swap pop  ( db )
        dup room? if  ( db )
            "here" obj !
            dup contents_array 0 array_insertitem  ( arrDb )
        else
            1 array_make  ( arrDb )
        then  ( arrDb )
    else  ( strY db )
        pop  ( strY )
        " " explode_array  ( arrStr )
        0 array_make swap foreach swap pop  ( arrDb str )
            .noisy_match dup ok? if swap array_appenditem else pop then  ( arrDb )
        repeat  ( arrDb )
    then  ( arrDb )
    dup array_count  ( arrDb intHowMany )
    dup not if pop pop exit then  ( arrDb intHowMany )
    1 - var! mult  ( arrDb )
 
    foreach swap pop  ( db )
        dup command @ "(%s)" fmtstring 0  ( db db strHow boolOdelay )
 
        ( Message. )
        3 pick 3 pick 3 pick  ( db db strHow boolOdelay db strHow boolOdelay )
        "_prefs/" command @ strcat -3 rotate  ( db db strHow boolOdelay db strProp strHow boolOdelay )
        parseprop  ( db db strHow boolOdelay strValue )
        dup not if pop "You %v nothing special." then  ( db db strHow boolOdelay strValue )
        4 pick swap rtn-parse  ( db db strHow boolOdelay strValue )
        mult @ if 4 pick "%D: %s" fmtstring then  ( db db strHow boolOdelay strValue )
        .tell  ( db db strHow boolOdelay )
 
        ( Notify. )
        command @ "_prefs/%s_notify" fmtstring  ( db db strHow boolOdelay strProp )
        4 pick over getprop int? if  ( db db strHow boolOdelay strProp )
            pop pop pop pop "[ %N just %ved %obj! ]"  ( db strNotify )
        else  ( db db strHow boolOdelay strProp )
            -3 rotate  ( db db strProp strHow boolOdelay )
            parseprop  ( db strNotify )
        then  ( db strNotify )
        dup strip if
            me @ swap rtn-parse  ( db strNotify )
            notify  (  )
        else pop pop then  (  )
    repeat  (  )
;
