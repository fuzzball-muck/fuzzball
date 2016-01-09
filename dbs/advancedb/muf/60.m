( cmd-pose.muf by Natasha@HLM
  A pose program distilled from Natasha@HLM's say program.
 
  Copyright 2002-2003 Natasha@HLM. Copyright 2002-2003 Here Lie Monsters.
  "@view $box/mit for license information.
)
$include $lib/ignore
$include $lib/strings
 
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$note Pose program with %N posing, $lib/ignore, and color stripping.
$version 1.002
 
$def str_program "saypose"
$def str_posepunctuation ":!?',-. "  (from Natasha's dotcom)
$def prop_nickname "_prefs/pose/nickname?"
$def prop_color "_prefs/say/color"
$def prop_meow "_prefs/say/meow"
$def prop_nicks "_prefs/say/nicks?"
 
: do-pose  ( str -- )
    ( Does the message need a space? )
    dup if
        dup 1 strcut pop  ( str strChar )
        str_posepunctuation swap instr
    else 1 then if "" else " " then  ( str strSpace )
 
    ( What's my@ name again? )
    me @ prop_nickname getpropstr .yes? if  ( str strSpace )
        me @ "%n" pronoun_sub  ( str strSpace strNick }  Use %n, not %N, so the first letter is not automatically capitalized Natasha@HLM 4 Aug 2003 )
        dup 3 strcut pop me @ name 3 strcut pop stringcmp over pmatch ok? or over " " instr or if  ( str strSpace strNick }  Do three-letter check insensitive to case Natasha@HLM 4 Aug 2003; disallow spaces Natasha@HLM 8 Dec 2003 )
            pop me @ name  ( str strSpace strName )
        then  ( str strSpace strName )
    else me @ name then  ( str strSpace strName )
 
 
    ( Meowed )
    var meowed
    var who
    loc @ contents_array  ( str strSpace strName arrHere )
    dup prop_meow me @ "*{%d}*" fmtstring array_filter_prop  ( str strSpace strName arrHere arrMeow )
    dup if  ( str strSpace strName arrHere arrMeow )
        dup rot array_diff  ( str strSpace strName arrMeow arrNonmeow )
        who !  ( str strSpace strName arrMeow )
        dup meowed !  ( str strSpace strName arrMeow )
 
        4 pick ansi_strip  ( str strSpace strName arrMeow str )
        "\\b_?[A-Z0-9_]+_?\\b" "MEOW" REG_ALL regsub  ( str strSpace strName arrMeow str' )
        "\\b_?[A-Z0-9][A-Za-z_0-9]*[a-z][A-Za-z0-9_]*\\b" "Meow" REG_ALL regsub  ( str strSpace strName arrMeow str' )
        "\\b[a-z][A-Za-z0-9]*\\b" "meow" REG_ALL regsub  ( str strSpace strName arrMeow str' )
        4 pick me @ "%D%s%s" fmtstring  ( str strSpace strName arrMeow str" )
        1 array_make swap array_notify  ( str strSpace strName )
 
    else pop who ! 0 array_make meowed ! then  ( str strSpace strName )
 
 
    who @ dup me @ str_program array_get_ignorers  ( str strSpace strName arrWho arrOption )
 
    over prop_nicks "no" array_filter_prop array_union  ( str strSpace strName arrWho arrOption )
    5 pick "\[[" instr if  ( str strSpace strName arrWho arrNoNick )
        over prop_color "no" array_filter_prop  ( str strSpace strName arrWho arrNoNick arrNoColor )
        array_union  ( str strSpace strName arrWho arrOption )
    then  ( str strSpace strName arrWho arrOption )
    swap pop  ( str strSpace strName arrOption )
 
    ( Notify normally. )
    loc @ over  ( str strSpace strName arrOption dbRoom arrOption )
    6 pick 6 pick 6 pick "%s%s%s" fmtstring  ( str strSpace strName arrOption dbRoom arrOption strFull )
    var! withboth array_vals withboth @ notify_exclude  ( str strSpace strName arrOption )
 
    ( Notify special. )
    dup if  ( str strSpace strName arrOption )
        me @ str_program array_filter_ignorers  ( str strSpace strName arrOption' )
        4 pick 4 pick 4 rotate "%s%s%s" fmtstring var! withnick  ( str strSpace arrOption )
        rot rot me @ "%D%s%s" fmtstring var! withname  ( arrOption )
        foreach swap pop  ( db )
            dup prop_nicks getpropstr .no? if withname else withnick then @  ( db strMsg )
            over prop_color getpropstr .no? if ansi_strip then  ( db strMsg' )
            notify  (  )
        repeat  (  )
    else pop pop pop pop then  (  )
;
 
: do-help pop pop .showhelp ;
: do-ignore pop str_program cmd-ignore-add ;
: do-unignore pop str_program cmd-ignore-del ;
 
: do-nickname pop pop  ( -- )
    me @ prop_nickname "yes" setprop  (  )
    "You will now pose with your %N nickname, if the first three letters match those of your actual character name, no one has your %N for a real name, and your %N contains no spaces." .tell  (  )
;
: do-unnickname pop pop  ( strY strZ -- )
    me @ prop_nickname remove_prop
    "You will now pose with your actual character name." .tell  (  )
;
 
: do-nicks pop pop  ( -- )
    me @ prop_nicks remove_prop  (  )
    "You will see poses with nicknames of players who have enabled that feature." .tell  (  )
;
: do-unnicks pop pop  ( strY strZ -- )
    me @ prop_nicks "no" setprop
    "You will see all poses with players' actual character names." .tell  (  )
;
 
: do-grey  ( strY strZ -- )
    pop pop  (  )
    me @ prop_color "no" setprop
    me @ "You will not see color in others' poses. Note you will see color in your own poses." fmtstring .tell
;
: do-ungrey  ( strY strZ -- )
    pop pop  (  )
    me @ prop_color remove_prop
    "You will see color in poses." .tell
;
 
$define dict_commands {
    "help"    'do-help
    "ignore"  'do-ignore
    "!ignore" 'do-unignore
    "name"    'do-unnickname
    "!name"   'do-nickname
    "nickname" 'do-nickname
    "!nickname" 'do-unnickname
    "nicks"   'do-nicks
    "!nicks"  'do-unnicks
    "grey"    'do-grey
    "gray"    'do-grey
    "!grey"   'do-ungrey
    "!gray"   'do-ungrey
}dict $enddef
 
: main  ( str -- )
    dup STRparse  ( str strX strY strZ )
    3 pick "#" stringpfx if  ( str strX strY strZ )
        pop pop pop  ( str )
        "#" split strcat  ( str' )
        do-pose exit  (  )
    then  ( str strX strY strZ )
    3 pick not if pop pop pop do-pose exit then
    4 rotate pop  ( strX strY strZ )
 
    rot dict_commands over array_getitem  ( strY strZ strX ? )
    dup address? if  ( strY strZ strX adr )
        swap pop  ( strY strZ adr )
        execute  (  )
    else pop  ( strY strZ strX )
        "I don't recognize the command '#%s'. Try 'say #help' for help, or using '##' to say something starting with '#'." fmtstring .tell  ( strY strZ )
        pop pop  (  )
    then  (  )
;
