( cmd-run.muf by Natasha@HLM
  A free, simple "run" program for Fuzzball 6.
 
  Type "run dir,dir,dir..." to zip through those rooms without actually
  going through them. No, you can't break through locks: they're all
  tested and you're left in the last room you were in before. Everyone
  in the rooms you run through sees intermediate messages, and you see
  lists of all the objects in those rooms--but no descriptions or lists
  of obvious exits {since you already know where you're going, right?}.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for licensing information.
)
$include $lib/strings
 
$def prop_msg_osucc "_run/osucc"
$def prop_msg_othru "_run/othru"
$def prop_msg_odrop "_run/odrop"
$def str_msg_osucc  "zips off with a start."
$def str_msg_othru "zips through at full tilt."
$def str_msg_odrop  "pants from running."
 
$def prop_relpath "_prefs/run/paths/%d/%s"
$def prop_glopath "_prefs/run/paths/%s"
 
lvar exitcommands
lvar viewOnly
 
: rtn-exit?  ( db -- bool )
    dup ok? if
        dup exit? if
            dup getlink room? if
                pop 1 exit  ( bool )
            then
        then
    then
    pop 0  (  bool )
;
 
: rtn-exitmatch  ( db str -- db' }  Return the dbref of the exit from db called str. )
    strip  ( Make 'run  peak' work }  Natasha@HLM 1 December 2002 )
 
    ( Do I have an appropriate room-relative path? )
    me @ over 4 pick prop_relpath fmtstring getpropstr  ( db str strExpansion )
    dup not if pop  ( db str )
        ( Do I have an appropriate global path? )
        me @ over prop_glopath fmtstring getpropstr  ( db str strExpansion )
        dup not if pop  ( db str )
            ( Does that room's environment have an appropriate relative path? )
            over over over prop_relpath fmtstring envpropstr swap pop  ( db str strExpansion )
            dup not if pop  ( db str )
                ( Does that room's environment have an appropriate global path? )
                over over prop_glopath fmtstring envpropstr swap pop  ( db str strExpansion )
                dup not if  ( db str strExpansion )
                    ( Swap str and the null, so we pop the correct one below. )
                    swap  ( db strNull str )
                then
            then
        then
    then  ( db str str' )
 
    dup "," instr if  ( db str str' )
        ( We have more exits to add to exitcommands. )
        "," explode_array  ( db str arrExits )
        ( The first we keep for rmatching, though. )
        dup array_first pop  ( db str arrExits idxFirst )
        over over array_getitem -3 rotate  ( db str strFirst arrExits idxFirst )
        array_delitem  ( db str strFirst arrExits' )
        exitcommands @ dup array_first pop rot array_insertrange exitcommands !  ( db str strFirst )
        
        over strip over strip stringcmp if  ( db str strFirst )
            ( They're different; strFirst can be a path. )
            swap pop  ( db strFirst )
            1 sleep
            rtn-exitmatch  ( db' )
            exit  ( db' )
        then  ( db str strFirst )
    then  ( db str strFirst )
 
    swap pop #-1  ( db str' db' )
    begin 3 pick ok? over rtn-exit? not and while pop  ( db str' )
        over over rmatch  ( db str' db' )
        rot location -3 rotate  ( db str' db' )
    repeat -3 rotate pop pop  ( db' )
;
 
: cmd-setpath  ( strY strZ -- }  Set a path strY from here going strZ. )
    me @ 3 pick loc @ prop_relpath fmtstring  ( strY strZ dbMe strProp )
    over over getpropstr dup if  ( strY strZ dbMe strProp strOldPath )
        -5 rotate  ( strOldPath strY strZ dbMe strProp )
        pop pop pop  ( strOldPath strY )
        loc @ swap "You've already set your '%s' path from %D to '%s'." fmtstring .tell  (  )
    else pop  ( strY strZ dbMe strProp )
        rot setprop  ( strY )
        "Path '%s' set." fmtstring .tell  (  )
    then  (  )
;
 
: cmd-unsetpath  ( strY strZ -- }  Remove path strY from here. )
    pop me @ over loc @ prop_relpath fmtstring  ( strY dbMe strProp )
    over over getpropstr if  ( strY dbMe strProp )
        remove_prop  ( strY )
        "Path '%s' unset." fmtstring .tell  (  )
    else  ( strY dbMe strProp )
        pop pop loc @ swap "You don't have a '%s' path from %D." fmtstring .tell  (  )
    then  (  )
;
 
: cmd-view  ( strY strZ -- strY }  Set viewOnly flag and return. Since strY is the normal argument, leave it. )
    pop 1 viewOnly !
;
: cmd-help pop pop .showhelp ;
 
$define dict_commands {
    "help" 'cmd-help
    "set"  'cmd-setpath
    "unset" 'cmd-unsetpath
    "!set" 'cmd-unsetpath
    "del"  'cmd-unsetpath
    "rem"  'cmd-unsetpath
    "view" 'cmd-view
}dict $enddef
 
: main  ( str -- )
    0 viewOnly !
 
    dup not if pop "#help" then  ( str )
    dup STRparse  ( str strX strY strZ )
    3 pick if  ( str strX strY strZ )
        4 rotate pop  ( strX strY strZ )
        rot dict_commands over array_getitem  ( strY strZ strX ? )
        dup address? if  ( strY strZ strX adr )
            swap pop execute  (  )
        else  ( strY strZ strX ? )
            pop command @ swap "I don't know what you mean by '#%s'. Try '%s #help'." fmtstring .tell  ( strY strZ )
            pop pop  (  )
        then
        viewOnly @ not if exit then  (  )
    else pop pop pop then  ( str )
 
    0 var! moved
    me @ location var! start
    start @ swap  ( db str )
 
    "," explode_array exitcommands !
    begin exitcommands @ array_first while  ( db idx )
        exitcommands @ over array_getitem  ( db idx str )
        exitcommands @ rot array_delitem exitcommands !  ( db str )
 
        moved @ if  ( db str )
            swap  ( str db )
 
            dup "d" flag? if  ( str db )
                "You %s through %D, a dark room."  ( str db strMsg )
            else  ( str db )
                "" over contents_array  ( str db strContents arrContents )
                dup array_count if  ( str db strContents arrContents )
                    foreach swap pop  ( str db strContents dbContent )
                        name ", " swap strcat strcat  ( str db strContents )
                    repeat  ( str db strContents )
                    2 strcut swap pop  ( str db strContents )
                    "You %s through %D, past: " swap strcat  ( str db strMsg )
                else  ( str db strContents arrContents )
                    pop pop  ( str db strMsg )
                    "You %s through %D."  ( str db strMsg )
                then  ( str db strMsg )
            then  ( str db strMsg )
            over command @ rot fmtstring  ( str db strMsg )
            .tell  ( str db )
 
            dup  ( str db )
            me @ dup dup prop_msg_othru getpropstr dup not if pop str_msg_othru then pronoun_sub  ( str db db dbMe strMsg )
            swap "%D %s" fmtstring  ( str db db strMsg )
            swap 0 rot notify_exclude  ( str db )
 
            swap  ( db str )
        then
 
        over over rtn-exitmatch  ( db str dbExit? )
        dup rtn-exit? not if  ( db str dbExit? )
            pop "I don't see which way you mean by '%s'." fmtstring .tell  ( db )
            0 break  ( db 0 }  Needs something to buffer the pop. )
        then  ( db str dbExit )
 
        me @ over locked? if  ( db str dbExit )
            pop "The exit '%s' is locked." fmtstring .tell  ( db )
            0 break  ( db 0 }  Needs something to buffer the pop. )
        then  ( db str dbExit )
 
        getlink -3 rotate  ( db' db str )
        pop pop  ( db' )
 
        viewOnly @ not moved !  ( db' }  Unless viewOnly, put 1 in moved. If viewOnly, then not moved, so we'll keep skipping the part that says where we've been. )
    repeat pop  ( db )
 
    viewOnly @ if
        "You would end up in %D." fmtstring .tell  (  )
        exit  (  )
    then  ( db )
 
    dup start @ dbcmp if pop exit then  ( db )
 
    start @  ( db dbHere )
    me @ dup dup prop_msg_osucc getpropstr dup not if pop str_msg_osucc then pronoun_sub  ( db dbHere dbMe strMsg )
    4 pick name "%r" subst  ( db dbHere dbMe strMsg )
    swap "%D %s" fmtstring  ( db dbHere strMsg )
    over me @ 1 4 rotate notify_exclude  ( db dbHere )
 
    me @ dup dup prop_msg_odrop getpropstr dup not if pop str_msg_odrop then pronoun_sub  ( db dbHere dbMe strMsg )
    rot name "%r" subst  ( db dbMe strMsg )
    swap "%D %s" fmtstring  ( db strMsg )
    over 0 rot notify_exclude  ( db )
 
    me @ swap moveto  (  )
;
