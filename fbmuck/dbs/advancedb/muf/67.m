( cmd-say.muf by Natasha@HLM
 
  Copyright 2002-2003 Natasha@HLM. Copyright 2002-2003 Here Lie Monsters.
  "@view $box/mit for license information.
)
$include $lib/ignore
$include $lib/strings
$include $lib/match
 
$def str_program "saypose"
$def prop_third "_prefs/say/third"
$def prop_quotes "_say/def/quotes"
$def prop_overb "_say/def/osay"
$def prop_verb "_say/def/say"
$def prop_split "_prefs/say/split"
$def prop_color "_prefs/say/color"
$def prop_meow "_prefs/say/meow"
 
lvar randomWord
 
: do-say  ( str -- )
    "" randomWord !
 
    var quotes
    var verb
    var overb
    var myname
    var who
    var meowed
    me @ prop_third getpropstr .yes? not var! not-third
 
    ( Get the third-person verb. )
    me @ prop_overb getpropstr dup if  ( str strOverb )
        strip dup dup "," instr not and if "," strcat then
    else pop "says," then  ( str strOverb )
    me @ "%D %s" fmtstring overb !  ( str )
 
    ( Get the first-person verb {if any}. )
    not-third @ if  ( str )
        ( Get the first-person verb. )
        me @ prop_verb getpropstr dup if  ( str strVerb )
            strip dup dup "," instr not and if "," strcat then
        else pop "say," then  ( str strVerb )
        "You %s" fmtstring  ( str strVerb )
    else overb @ then verb !  ( str )
 
    ( Do the quotes. )
    me @ prop_quotes getpropstr dup if  ( str strQuotes )
        "%%" "%" subst
        "%s" "%%m" subst  ( str strQuotes )
    else pop "\"%s\"" then  ( str strQuotes )
    quotes !  ( str )
 
 
    ( Anyone #meowing this player? Go ahead and notify before special formatting. )
    loc @ contents_array  ( str arrHere )
    dup prop_meow me @ "*{%d}*" fmtstring array_filter_prop  ( str arrHere arrMeow )
    dup if  ( str arrHere arrMeow )
        dup rot array_diff  ( str arrMeow arrNonmeow )
        who !  ( str arrMeow )
        dup meowed !
 
        over ansi_strip  ( str arrMeow str )
        "\\b[A-Z0-9]+\\b" "MEOW" REG_ALL regsub  ( str arrMeow str' )
        "\\b[A-Z0-9][A-Za-z0-9]*[a-z][A-Za-z0-9]*\\b" "Meow" REG_ALL regsub  ( str arrMeow str' )
        "\\b[a-z][A-Za-z0-9]*\\b" "meow" REG_ALL regsub  ( str arrMeow str' )
        me @ "%D meows, \"%s\"" fmtstring  ( str arrMeow str" )
        1 array_make swap array_notify  ( str )
 
    else pop who ! 0 array_make meowed ! then  ( str )
 
 
    ( Put the message in the quotes. )
    dup ",," instr if  ( str )
        me @ prop_split getpropstr .no? not  ( str bool )
    else 0 then if  ( str )
        ( A split say. )
        ",," split  ( str- -str )
        swap "%%" "%" subst swap  ( str' )
 
        dup ",," instr if
            ",," split  ( str- -str- -str )
            "%%" "%" subst  ( str' )
            swap dup if  ( str- -str -str- )
                dup me @ name instr over "%n" instr or over "%N" instr or if  ( str- -str -str- )
                    me @ name "%n" subst me @ name "%N" subst  ( str- -str -str- )
                else
                    me @ swap "%s %D," fmtstring  ( str- -str -str- )
                then  ( str- -str -str- )
                dup "*[-!.,:;]" smatch not if "," strcat then  ( str- -str -str- )
                dup verb ! overb !  ( str- -str )
            else pop then  ( str- -str )
        then  ( str- -str )
 
        dup if
            quotes @ " %%s " strcat quotes @ strcat  ( str- -str strQuotes )
        else  ( str- -str )
            quotes @ " %%s%s" strcat  ( str- -str strQuotes )
            verb @ ",$" "." 0 regsub verb !
            overb @ ",$" "." 0 regsub overb !
        then  ( str- -str strQuotes )
 
        -3 rotate swap  ( strQuotes -str str- )
        dup "*[-!.,:;]" smatch not if "," strcat then  ( strQuotes -str str- )
        rot  ( -str str- strQuotes )
    else  ( str )
        "%%" "%" subst  ( str' )
        ( A singular say. )
        "%%s " quotes @ strcat  ( str strQuotes )
    then fmtstring  ( str' )
 
    ( Say. )
    verb @ over fmtstring .tell  ( str' )
 
    ( Put the osay together. )
    overb @ swap fmtstring  ( str" )
 
    ( Is there color to avoid? )
    dup "\[[" instr if
        ( Who doesn't get color? )
        who @ me @ str_program array_filter_ignorers prop_color "no" array_filter_prop  ( str" arrGreyed )
 
        ( OK, tell the colorless ones. )
        dup if  ( str" arrGreyed )
            over ansi_strip 1 array_make  ( str" arrGreyed arrMsg )
            over array_notify  ( str" arrGreyed )
        then  ( str" arrGreyed )
    else 0 array_make then  ( str" arrGreyed )
    meowed @ array_union  ( str" arrGreyed' )
    who @ me @ str_program array_get_ignorers array_union  ( str" arrGreyed" )
 
    loc @ swap  ( str" arrGreyed )
    array_vals  ( str" dbHere dbGreyedN..dbGreyed1 intN )
    me @ swap ++  ( str" dbHere dbGreyedN'..dbGreyed1' intN' )
    dup 3 + rotate  ( dbHere dbGreyedN..dbGreyed1 intN str" )
    notify_exclude
;
 
: do-help pop pop .showhelp ;
: do-ignore pop str_program cmd-ignore-add ;
: do-unignore pop str_program cmd-ignore-del ;
 
: do-third  ( strY strZ -- )
    pop pop  (  )
    me @ prop_third "yes" setprop
    me @ "You will see your own says in the third person (\"%D says\")." fmtstring .tell
;
: do-unthird  ( strY strZ -- )
    pop pop  (  )
    me @ prop_third remove_prop
    "You will see your own says in the second person (\"You say\")." .tell
;
 
: do-grey  ( strY strZ -- )
    pop pop  (  )
    me @ prop_color "no" setprop
    me @ "You will not see color in any says. Note you will see color in your own says." fmtstring .tell
;
: do-ungrey  ( strY strZ -- )
    pop pop  (  )
    me @ prop_color remove_prop
    "You will see color in says." .tell
;
 
: do-meow  ( strY strZ -- )
    pop  ( strY )
    dup if
        .noisy_pmatch dup ok? not if pop exit then  ( db )
        me @ prop_meow 3 pick reflist_find if  ( db )
            "%D is already in your #meow list." fmtstring .tell exit  (  )
        then  ( db )
        me @ prop_meow 3 pick reflist_add  ( db )
        "%D added." fmtstring .tell
    else
        me @ prop_meow array_get_reflist  ( arr )
        "" swap foreach swap pop "%D %s" fmtstring repeat
        "Your meowlist: " swap strcat .tell
    then
;
: do-unmeow  ( strY strZ -- )
    pop  ( strY )
    .noisy_pmatch dup ok? not if pop exit then  ( db )
    me @ prop_meow 3 pick reflist_find not if  ( db )
        "%D is not in your #meow list." fmtstring .tell exit  (  )
    then  ( db )
    me @ prop_meow 3 pick reflist_del  ( db )
    "%D removed." fmtstring .tell
;
 
$define dict_commands {
    "help"    'do-help
    "ignore"  'do-ignore
    "!ignore" 'do-unignore
    "meow"    'do-meow
    "!meow"   'do-unmeow
    "third"   'do-third
    "!third"  'do-unthird
    "grey"    'do-grey
    "gray"    'do-grey
    "!grey"   'do-ungrey
    "!gray"   'do-ungrey
}dict $enddef
 
: main  ( str -- )
    dup STRparse  ( str strX strY strZ )
    3 pick "#" stringpfx if pop pop pop 1 strcut swap pop do-say exit then
    3 pick not if pop pop pop do-say exit then
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
