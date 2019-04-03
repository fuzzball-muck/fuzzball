( cmd-whospecies.muf by Natasha@HLM
  Whospecies redux for Fuzzball 6.
 
  cmd-whospecies uses line formats to completely modularize how it appears.
  It looks up the environment, so area builders can customize its appearance
  if they wish.
 
  There are three line formats: header, line, and footer. See "man
  fmtstring" for directions on how to write line formats. This program actually
  uses array_fmtstrings, so each variable has a namein brackets between the
  % and the type character. In "header" and "footer", the variables are:
 
  %[awake]i  The number of awake players in the listing.
  %[count]i  The total number of players in the listing.
   %[room]s  The name of the room being listed, prepended with a space. In
             #far listings, this is the null string.
 %[region]s  The region of the room being listed. In #far listings, this is
             #0's region.
 
  You can also use "%%-", which will be replaced with a string of dashes,
  and \[ which is parsed to the escape character, suitable for building
  ANSI color sequences.
 
  Unlike in header and footer lines, the "line" setting *does not* parse
  "\[" into the escape code. You'll have to pre-parse it when you save the
  setting, by saving it with @mpi {store} and your raw ansi colors or {attr}.
 
  Variables set in the "line" format:
 
  %[statcolor]s  The special color associated with that player's status.
 %[awakecolor]s  "\[[1m" if the player is awake; the reset code otherwise.
                 Use this to make sleeping players' lines dim.
   %[sexcolor]s  The special color associated with that player's gender. The
                 color depends on the player's absolute {%a} pronoun setting:
                 the color sequences are taken from, eg, _prefs/ws/object/his
                 and _prefs/ws/object/hers up the environment from the user.
       %[name]s  The player's name.
       %[idle]s  How long the player has been idle, eg, "2m", or the null
                 string if the player is asleep or less than two minutes idle.
          %[n]s  The player's name. If the player is asleep or idle, it
                 also includes an "[asleep]" or "[2m idle]" tag after.
      %[doing]s  The player's @doing, if he has one. Shown as the value of the
                 _prefs/ws/doing prop from up the environment, with "<my
                 pretty doing>" replaced by the user's @doing setting.
     %[status]s  The values of these properties.
    %[species]s
        %[sex]s
 
  Copyright 2002-2003 Natasha O'Brien. Copyright 2002-2003 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.0
$note Whospecies redux for Fuzzball 6 with customizable output.
 
$include $lib/strings
$include $lib/table
$include $lib/timestr
$iflib $lib/alias $include $lib/alias $endif
$iflib $lib/away $include $lib/away $endif  (added Natasha@HLM 27 December 2002)
 
$def prop_status "status"
$def prop_dim_sleepers "_prefs/ws/dimsleepers"
$def prop_doing "_prefs/ws/doing"
$def prop_idle_time "_prefs/ws/idle"
$def prop_region "_region"
$def prop_far_object "_prefs/ws/object"
$def str_redacted "\[[1;30mredacted"
 
lvar metadata
lvar idletime
lvar doingfmt
lvar is_afar
lvar redacts
lvar sorthow
$def rtn-isRedacted? redacts @ over int array_getitem
 
( *** utility bits )
 
: rtn-parse-hf  ( str -- str' }  Parses a header or footer for special codes. )
    "\[" "\\[" subst  ( str )
 
    metadata @ 1 array_make swap array_fmtstrings  ( arr )
    dup array_first pop array_getitem  ( str )
 
    ( Fix the line. )
    dup "%-" instr if  ( str )
        79 over ansi_strlen 2 - -  ( str intLength )
        "--------------------------------------------------------------------------------" swap strcut pop  ( str strLine )
        "%-" subst  ( str )
    then  ( str )
;
 
: rtn-get-pref  ( db str -- str' }  Gets db's pref of the given name. )
    "_prefs/ws/" swap strcat  ( db str )
    envpropstr swap pop  ( str' )
;
 
: rtn-valid-who  ( db -- bool }  Returns true if this db is a player we can list. )
    dup ok? if
        dup thing? over "z" flag? and if me @ swap owner ignoring? not exit then  ( db )
        dup player? if me @ swap ignoring? not else pop 0 then  ( bool )
    else pop 0 then  ( bool )
;
 
( *** data getters )
 
: rtn-data-name name ;
: rtn-data-idle owner descrleastidle dup -1 = if 0 else descridle dup idletime @ > then if stimestr else pop "" then ;
$def MAXINT 2147483647
: rtn-data-idlen  ( db -- int )
    owner  ( db )
    $iflib $lib/away dup away-away? if pop MAXINT 1 - else $endif  ( db )
        descrleastidle dup -1 = if pop MAXINT else descridle then  ( int )
    $iflib $lib/away then $endif  ( int )
;
 
: rtn-data-doing  ( db -- str )
    rtn-isRedacted? if pop "" exit then
    doingfmt @ dup if  ( db strFmt )
        swap "_/do" getpropstr dup if  ( strFmt strDoing )
            "%[doing]s" subst  ( str )
        else pop pop "" then  ( str )
    else pop pop "" then  ( str )
;
 
: rtn-data-awakecolor  ( db -- str )
    awake? not not "\[[%im" fmtstring
;
: rtn-data-bright pop "\[[1m" ;
 
: rtn-data-statcolor  ( db -- str )
    rtn-isRedacted? if pop "" exit then
    prop_status getpropstr strip  ( strStatus )
    dup if  ( strStatus )
        prog "_stat2color/" rot strcat getpropstr  ( strColor )
        dup if "\[" "\\[" subst then  ( strColor )
    then  ( str }  strColor or an empty strStatus. )
;
 
: rtn-data-sexcolor  ( db -- str }  Gets the character's gender, with color and whatnot. )
    rtn-isRedacted? if pop "" exit then  ( db )
    "_prefs/ws/color/%a" pronoun_sub  ( strProp )
    1 try  ( strProp )
        me @ swap envpropstr swap pop  ( strColor )
    catch pop "" endcatch  ( strColor )
    dup if "\[" "\\[" subst else pop "\[[37m" then  ( strColor )
;
 
: rtn-data-n  ( db -- str )
    dup name  ( db str )
    over awake? not if  ( db str )
        "\[[0;34m[\[[1;34masleep\[[0;34m]" strcat  ( db str )
    else over "i" flag? if  ( db str )
        "\[[0;31m[\[[1;31mbusy\[[0;31m]" strcat  ( db str )
$iflib $lib/away
    else over away-away? if  ( db str }  Changed to use $lib/away fn Natasha@HLM 27 December 2002 )
        "\[[0;36m[\[[1;36maway\[[0;36m]" strcat  ( db str )
$endif
    else over rtn-data-idle dup if  ( str strIdle )
        strip "\[[0;36m[\[[1;36m%s idle\[[0;36m]" fmtstring strcat  ( str )
    else pop then then then $iflib $lib/away then $endif  ( db str )
 
    over thing? 3 pick "z" flag? and if
        over owner "\[[0;35m[\[[1;35m%D's\[[0;35m]" fmtstring strcat  ( db str )
    then  ( db str )
 
    swap pop  ( str )
;
 
: rtn-data-status rtn-isRedacted? if pop str_redacted else prop_status getpropstr then ;
: rtn-data-species rtn-isRedacted? if pop str_redacted else "species" getpropstr then ;
: rtn-data-sex rtn-isRedacted? if pop str_redacted else "sex" getpropstr then ;
 
$define dict_atdata {
    "n"     'rtn-data-n
    "name"  'rtn-data-name
    "idle"  'rtn-data-idle
    "doing" 'rtn-data-doing
 
    "sexcolor"   'rtn-data-sexcolor
    "statcolor"  'rtn-data-statcolor
    "awakecolor" 'rtn-data-awakecolor
 
    "status"  'rtn-data-status
    "species" 'rtn-data-species
    "sex"     'rtn-data-sex
}dict $enddef
 
( *** #commands )
 
: do-help pop pop .showhelp ;
 
: do-object  ( strY strZ -- )
    pop pop  (  )
    me @ prop_far_object "yes" setprop  (  )
    "You object to ws #far." .tell
;
 
: do-!object  ( strY strZ -- )
    pop pop  (  )
    me @ prop_far_object remove_prop  (  )
    "You allow ws #far." .tell
;
 
: do-bright  ( strY strZ -- )
    pop pop  (  )
    me @ prop_dim_sleepers "no" setprop  (  )
    "Sleepers will not be shown dim." .tell  (  )
;
 
: do-!bright  ( strY strZ -- )
    pop pop  (  )
    me @ prop_dim_sleepers remove_prop  (  )
    "Sleepers will be shown with dim colors." .tell  (  )
;
 
: do-doing  ( strY strZ -- )
    pop pop  (  )
    me @ prop_doing "yes" setprop  (  )
    "Players' @doings will be shown where available." .tell  (  )
;
 
: do-!doing  ( strY strZ -- )
    pop pop  (  )
    me @ prop_doing remove_prop  (  )
    "Players' @doings will not be shown." .tell  (  )
;
 
: do-idle  ( strY strZ -- )
    pop  ( strY )
    dup number? not if  ( strY )
        "'%s' is not a number." fmtstring .tell  (  )
        exit  (  )
    then atoi  ( int )
    me @ prop_idle_time 3 pick intostr setprop  ( int )
    mtimestr "You will only see idle times more than %s." fmtstring .tell  (  )
;
 
: do-far  ( strY strZ -- arrDb }  Lists all the people in strY, if they allow it. )
    pop  ( strY )
$iflib $lib/alias
    me @ swap alias-expand  ( arrDb )
$else
    " " explode_array  ( arrNames )
    { }list swap  ( arrNames )
    foreach swap pop  ( arrDb strName )
        dup .pmatch  ( arrDb strName db )
        dup rtn-valid-who not if  ( arrDb strName db )
            pop  ( arrDb strName )
            "I don't see whom you mean by '%s'." fmtstring .tell  ( arrDb )
            continue  ( arrDb )
        then swap pop  ( arrDb db )
        
        swap array_appenditem  ( arrDb )
    repeat  ( arrDb )
$endif
    1 is_afar !  ( arrDb )
;
 
: do-listhere  ( strY strZ -- arrContents }  Returns an array of all the players/objects here whose names start with strY. )
    pop  ( strY )
    loc @ contents_array  ( strY arrContents )
    over if
        0 array_make swap  ( strY arrContents' arrContents )
        foreach swap pop  ( strY arrContents db )
            dup name 4 pick stringpfx if  ( strY arrContents db )
                dup rot array_appenditem swap  ( strY arrContents db )
            then  ( strY arrContents db )
        pop repeat  ( strY arrContents )
    then  ( strY arrContents' )
    swap pop  ( arrContents' )
;
 
: do-sort  ( strY strZ -- arrContents )
    pop " " split strip swap strip sorthow !  ( -strY )
    "" do-listhere  ( arrContents )
;
 
: do-setup  ( strY strZ -- )
    pop pop  (  )
 
 
    me @ prog controls not if
        "You must be a wizard or own the program object to #setup." .tell
        exit
    then  (  )
 
    trig "ws;whospec;whospecies" setname
    trig "_/de" prog "{muf:%d,#help}" fmtstring setprop
 
    ( Set configuration props. )
 
    #0 "_prefs/ws/header" "\[[1;37mStat  Name_______\[[0;37m_______\[[1;30m______  \[[37mSex__\[[0;37m___\[[1;30m___  \[[37mSpecies_________\[[0;37m_________\[[1;30m_________\[[0m" setprop
    #0 "_prefs/ws/line"   "%[awakecolor]s%[statcolor]s%-4.4[status]s\[[0m%[awakecolor]s  \[[37m%-24.24[n]s  %[awakecolor]s%[sexcolor]s%-11.11[sex]s  \[[37m%[species]s%[doing]s" setprop
    #0 "_prefs/ws/doing"  "\r        \[[1;35mDoing: \[[0;35m%[doing]s" setprop
    #0 "_prefs/ws/footer" "\[[1;30m-\[[0;37m-\[[1m-( %.20[region]s )-\[[0;37m-\[[1;30m-%%--\[[0;37m-\[[1m-( %[awake]i/%[count]i%.35[room]s )-\[[0;37m-\[[1;30m-\[[0m" setprop
 
    prog "_stat2color/IC"  "\[[32m" setprop
    prog "_stat2color/OOC" "\[[33m" setprop
 
    #0 "_prefs/ws/color/his"  "\[[36m" setprop
    #0 "_prefs/ws/color/hers" "\[[35m" setprop
 
    "Action renamed and @described. Default configuration options set." .tell
;
 
$define dict_commands {
    ""        'do-listhere
    "far"     'do-far
    "sort"    'do-sort
    "setup"   'do-setup
    "object"  'do-object
    "!object" 'do-!object
    "bright"  'do-bright
    "!bright" 'do-!bright
    "idle"    'do-idle
    "doing"   'do-doing
    "!doing"  'do-!doing
    "help"    'do-help
}dict $enddef
 
: main  ( str -- )
    0 is_afar !
    "" sorthow !
 
    ( Have we entered a command? )
    dup if  ( str )
        STRparse  ( strX strY strZ }  #X y=z )
        0 try
            dict_commands 4 pick array_getitem  ( strX strY strZ adr )
        catch endcatch  ( strX strY strZ ? }  ? is either adr or catch's strError )
        dup address? if  ( strX strY strZ adr )
            4 rotate pop  ( strY strZ adr )
            execute  (  )
            0 try
                dup array? not if
                    exit
                then
            catch pop exit endcatch  ( arrWho )
        else  ( strX strY strZ ? )
            pop pop pop
            "I don't understand the command '%s'." fmtstring .tell  (  )
            exit  (  )
        then  ( arrWho )
        "room" "" "region" #0 prop_region getpropstr 2 array_make_dict metadata !  ( arrWho )
    else pop  (  )
        loc @  ( db )
        dup "d" flag? me @ 3 pick controls not and if  ( db )
            pop "This room is too dark to see." .tell  (  )
            exit  (  )
        then  ( db )
        " " over name strcat
        "room" swap "region" loc @ prop_region envpropstr swap pop 2 array_make_dict metadata !  ( db )
        contents_array  ( arrWho )
    then  ( arrWho )
 
    ( Are these valid people? )
    0 array_make_dict redacts !
    0 var! numAwake
    0 array_make swap foreach swap pop  ( arrWho db )
        dup rtn-valid-who if  ( arrWho db )
 
            dup awake? if numAwake ++ then  ( arrWho db )
 
            is_afar @ if  ( arrWho db )
                dup prop_far_object getpropstr .yes? if  ( arrWho db )
                    1 redacts @ 3 pick int array_setitem redacts !  ( arrWho db )
                then  ( arrWho db )
            then  ( arrWho db )
 
            swap array_appenditem  ( arrWho )
        else pop then  ( arrWho )
    repeat  ( arrWho )
    dup not if
        "No one to show." .tell  ( arrWho )
        pop exit  (  )
    then  ( arrWho )
 
    metadata @  ( arrWho dictMetadata )
    over array_count swap "count" array_setitem  ( arrWho dictMetadata )
    numAwake @ swap "awake" array_setitem  ( arrWho dictMetadata )
    metadata !  ( arrWho )
    array_reverse var! data  (  )
 
    ( Get the line format. )
    me @ "line" rtn-get-pref  ( str )
    "\[" "\\[" subst  ( str )
    var! linefmt  (  )
 
    ( Where are the processors? )
    dict_atdata  ( dict )
    ( Hey, set some options. )
    me @ prop_dim_sleepers getpropstr .no? if  ( dict )
        'rtn-data-bright swap "awakecolor" array_setitem
    then var! processors  (  )
    me @ prop_idle_time getpropstr dup number? if atoi else pop 180 then idletime !
    me @ prop_doing getpropstr .yes? if  (  )
        loc @ "doing" rtn-get-pref  ( str )
        "\[" "\\[" subst  ( str )
        "\r" "\\r" subst  ( str )
    else "" then doingfmt !  (  )
 
    linefmt @ 0 array_make_dict processors @ data @  ( strLinefmt dictHeaders dictProcessors arrData )
    ( Sort how? )
    sorthow @ if sorthow @ else me @ "sort" rtn-get-pref then  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
    dup if dup "{name|idle|sex|species|status}" smatch if  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
        dup "idle" stringcmp not if  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
            pop  ( strLinefmt dictHeaders dictProcessors arrData )
            'rtn-data-idlen rot "idlen" array_setitem swap  ( strLinefmt dictHeaders dictProcessors arrData )
            "idlen"  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
        then  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
    else  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
        pop ""  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
    then then  ( strLinefmt dictHeaders dictProcessors arrData strSorthow )
 
    me @ "header" rtn-get-pref  ( strLinefmt dictHeaders dictProcessors listData strSorthow strHeader )
    rtn-parse-hf .tell  ( strLinefmt dictHeaders dictProcessors listData strSorthow )
 
    table-table-sort  (  )
 
    me @ "footer" rtn-get-pref  ( strFooter )
    rtn-parse-hf .tell  (  )
;
