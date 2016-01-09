( userlist.muf by Natasha@HLM
  Lists any and all players on the muck whose names match a given pattern.
 
  Copyright 2002-2003 Natasha O'Brien. Copyright 2002-2003 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.0
$note Lists any and all players on the muck whose names match a given pattern.
 
$include $lib/strings
 
$undef SHOW_PAUSE
 
lvar v_count
lvar v_pattern
lvar v_matcher
lvar v_line
lvar v_getsortkey
 
 
: rtn-match-nothing  ( str -- bool )
    pop 1  ( bool )
;
: rtn-match-pattern  ( str -- bool )
    v_pattern @ smatch  ( bool )
;
 
: rtn-add  ( strName -- }  Add the given name to the userlist. )
    ( Longify. )
    "                   " strcat 19 strcut pop  ( strName )
 
    ( Add. )
    me @ "_temp/userlist#/" v_line @ intostr strcat  ( strName dbMe strLineprop )
    over over getpropstr  ( strName dbMe strLineprop strLine )
    4 rotate strcat  ( dbMe strLineprop strLine )
    setprop  (  )
 
    ( Increment. )
    v_count @ 1 + dup 3 > if  ( intCount )
        v_line @ 1 + v_line !
        pop 0  ( intCount )
    then v_count !  (  )
;
 
 
$define dict_commands {
    "help" 'do-help
    "create" 'do-sortcreate
}dict $enddef
 
 
: do-help .showhelp "exit" abort ;
 
 
( db -- ? }  Turn a dbref into its sort key. )
: sort-name name ;
: sort-create timestamps pop pop pop ;
 
: do-sortcreate 'sort-create v_getsortkey ! ;
 
: main  ( str -- }  List all the players whose name contain the given. )
    strip tolower STRparse pop  ( strX strY )
 
    'sort-name v_getsortkey !  ( strX strY )
 
    ( Display help if we need it. )
    over if
        swap dict_commands over array_getitem  ( strY strX ? )
        dup address? if  ( strY strX ? )
            swap pop  ( strY ? )
            1 try execute catch exit endcatch  ( strY )
        else
            pop "What do you mean by '#%s'? Try 'userlist #help'." fmtstring .tell  ( strY )
            pop exit  (  )
        then  ( strY )
    then  ( str }  str==strY )
 
    dup not if
        "Try 'userlist <pattern>' to list only matching players, 'userlist *' to list everyone, or 'userlist #help' for help." .tell
        pop exit  (  )
    then  ( str )
 
    ( Set up the matching thing. )
    ( Go ahead and skip the matching part if it's a star. )
    dup "*" strcmp not if pop "" then v_pattern !  (  )
 
    ( Arrange tempstuff. )
    1 v_line !
    0 v_count !
 
    ( Make the list. )
    background  (  )
$ifdef SHOW_PAUSE
    "Generating list. Please wait."  .tell
$endif
 
    ( Produce the player list. )
    0 array_make  ( arr )
    #-1 begin #-1 v_pattern @ "P" findnext dup while  ( arr db )
        "name" over name "key"  ( arr db str"Name" strName str"Key" )
        4 pick v_getsortkey @ execute  ( arr db str"Name" strName str"Key" ?Key )
        2 array_make_dict  ( arr db dictUnit )
        rot array_appenditem swap  ( arr db )
    repeat pop  ( arr )
 
    ( Sort it. )
    SORTTYPE_CASEINSENS SORTTYPE_DESCENDING bitor "key" array_sort_indexed  ( arr )
 
    "Players matched:" .tell  ( arrList )
 
    ( Unfurl the list. )
    ( Uh, these strings are actually dictionaries. )
    array_explode  ( intKeyN strN..intKey1 str1 intN )
    begin dup while  ( intKeyN strN..intKey1 str1 intN )
        dup 2 > if
            3 - -7 rotate  ( intKeyN strN..intKey4 str4 intN intKey3 str1 intKey2 str2 intKey1 str1 )
            swap pop rot pop 4 rotate pop  ( intKeyN strN..intKey4 str4 intN str3 str2 str1 )
        else  ( intKeyN strN..intKey1 str1 intN )
            dup 2 = if
                2 - -5 rotate  ( intN intKey2 str2 intKey1 str1 )
                swap pop rot pop  ( intN str2 str1 )
                "name" "" 1 array_make_dict -3 rotate  ( intN str3 str2 str1 )
            else
                1 - -3 rotate  ( intN intKey1 str1 )
                swap pop  ( intN str1 )
                "name" "" 1 array_make_dict dup rot  ( str1 )
            then
        then  ( intKeyN strN..intKey4 str4 intN str3 str2 str1 )
        rot "name" array_getitem
        rot "name" array_getitem
        rot "name" array_getitem
        "    %-20.20s  %-20.20s  %-20.20s" fmtstring .tell
    repeat pop  (  )
 
    "Done." .tell
;
