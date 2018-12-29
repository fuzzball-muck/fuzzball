( cmd-whodoing.muf by Natasha@HLM
  A tastefully colored WHO replacement for Fuzzball 6.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
 
  Version history:
  1.0: cleaned up for repository submission
  1.1, 13 March 2002: Say "N days" for on times of 10+ days {using
    $lib/rainbow if present}. Require $lib/timestr. Don't list oldest
    first when using name filter {eg "wd s"}.
  1.11, 27 March 2003: With #species, show species in normal color.
  1.2, 4 October 2003: Use $lib/wf to get watchfor list
  1.3, 12 October 2018: Fix minor issue with rainbow color ending
    up on some idle times. {Theo@HLM}
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.3
$note A tastefully colored WHO replacement for Fuzzball 6.
 
$include $lib/strings
$include $lib/table
$include $lib/timestr
$include $lib/wf
$iflib $lib/away $include $lib/away $endif
$iflib $lib/rainbow $include $lib/rainbow $endif
 
$def prop_wsobject "_prefs/ws/object"
$def prop_wflist "_prefs/con_announce_list"
$def prop_def "_prefs/wddef"
 
var lineformat
var metadata
 
 
( ** data getters  { intDescriptor -- str }  )
: get-name descrdbref name ;
: get-doing
    descrdbref
$iflib $lib/away
    dup away-away? if
        away-message dup if
            "Away: %s" fmtstring
        else
            pop "Away"
        then
    else
$endif
        "_/do" getpropstr
$iflib $lib/away
    then
$endif
;  (changed to use $lib/away fns Natasha@HLM 27 December 2002; add 'Away:' Natasha@HLM 9 January 2003; don't show ': ' if no away message is set Natasha@HLM 22 January 2003)
: get-on  ( intDescriptor -- str }  Return the mtimestr for how long this connection has been on. If $lib/rainbow is present, display 'N days' in rainbow color for anyone on for 10 days or more instead. )
    descrtime  ( intTime )
    dup 864000 (10 days) >= if  ( intTime }  Don't just mtimestr, use "N days" if 10 or more )
        86400 / "%i days" fmtstring  ( str )
        $iflib $lib/rainbow rainbow $endif
    else mtimestr then  ( str )
;
: get-idle  ( intDescriptor -- str }  honor $lib/away and color code Natasha@HLM 27 December 2002; Don't replace time with 'away', only color issue419 Natasha@HLM 9 January 2003 )
$ifnlib $lib/away
    descridle
$else
    dup descridle swap descrdbref away-away? if  ( intIdle )
        "\[[1;30m"  (  )
    else  ( intIdle )
$endif
        dup case  ( intIdle intIdle )
            3600 >= when "\[[1;31m" end
             600 >= when "\[[1;33m" end
              60 >= when "\[[0m"    end
            default pop  "\[[1;32m" end
        endcase  ( intIdle strColor )
$iflib $lib/away
    then  ( intIdle strColor )
$endif
    swap stimestr strcat  ( strTime )
;
: get-secure  ( intDescriptor -- str )
    dup descrdbref me @ dbcmp if
        descrsecure? if "\[[1m@\[[0m" else "" then
    else pop "" then  ( str )
;
 
: get-species  ( intDescr -- str )
    descrdbref  ( db )
    dup prop_wsobject getpropstr .yes? if
        pop "<redacted>"  ( str )
    else
        "species" getpropstr  ( str )
    then  ( str )
;
 
 
( ** list makers )
: rtn-filter  ( strY arr -- arr' )
    over if  ( strY arr )
        0 array_make swap foreach swap pop  ( strY arrShow intDescr )
            dup descrdbref name 4 pick stringpfx if swap array_appenditem else pop then  ( strY arrShow )
        repeat  ( strY arrShow )
    then swap pop  ( arr' )
;
: do-all #-1 descriptors array_make rtn-filter ;
: do-room  ( strY -- arr )
    do-all 0 array_make over foreach swap pop  ( arr arrDel intDescr )
        dup descrdbref location loc @ dbcmp not if swap array_appenditem else pop then  ( arr arrDel )
    repeat  ( arr arrDel )
    swap array_diff  ( arr )
;
: do-watchfor  ( strY -- arr )
    wf-find-watched swap  ( arrWfDb strY )
    do-all 0 array_make  ( arrWfDb arrAll arr )
    swap foreach swap pop  ( arrWfDb arr intDescr )
        3 pick over descrdbref array_findval if  ( arrWfDb arr intDescr )
            swap array_appenditem  ( arrWfDb arr )
        else pop then  ( arrWfDb arr )
    repeat  ( arrWfDb arr )
    swap pop  ( arr )
;
 
: do-species  ( strY -- arr )
    "%-16.16[name]s %8.8[on]s %4.4[idle]s%1.1[secure]s\[[0m%47.47[species]s" lineformat !
    'get-species metadata @ "species" array_setitem "doing" array_delitem metadata !  ( strY )
    do-all  (  )
;
 
: do-help pop .showhelp "exit" abort ;
 
: do-def  ( strY -- )
    me @ prop_def rot setprop
    "Set." .tell  (  )
    "exit" abort
;
 
$define dict_commands {
    "" 'do-all
    "room" 'do-room
    "watc" 'do-watchfor
    "wf" 'do-watchfor
    "spec" 'do-species
    "species" 'do-species
    "help" 'do-help
    "def"  'do-def
}dict $enddef
 
 
: main  ( str -- )
    dup not if pop me @ prop_def getpropstr then  ( str )
 
    STRparse pop  ( strX strY )
    swap dict_commands over array_getitem  ( strY strX ? )
    dup address? not if  ( strY strX ? )
        pop "I don't know what you mean by '#%s'." fmtstring .tell  ( strY )
        pop exit  (  )
    then  ( strY strX adr )
    swap pop  ( strY adr )
 
    "%-16.16[name]s %8.8[on]s %4.4[idle]s%1.1[secure]s\[[0m%47.47[doing]s" lineformat !
    {
        "name"  'get-name
        "secure" 'get-secure
        "doing" 'get-doing
        "on"    'get-on
        "idle"  'get-idle
    }dict metadata !  ( strY adr )
 
    2 try
        execute  ( arr )
    catch exit endcatch  ( arr )
    dup array_count var! count  ( arr )
 
    lineformat @  ( arr strLinefmt )
    { "name" "\[[1;37mName" "secure" " " "doing" "\[[1;37mDoing" "on" "\[[1;37mOn for" "idle" "\[[1;37mIdle" "species" "\[[1;37mSpecies" }dict  ( arr strLinefmt dictHeaders )
    metadata @  ( arr strLinefmt dictHeaders dictProcessors )
    4 rotate  ( strLinefmt dictHeaders dictProcessors arr )
 
    table-table  (  )
 
    #0 "_sys/max_connects" getpropval  ( intMax )
    online_array array_count  ( intMax intOnline )
    count @ "\[[1;37m%i players shown of %i online (max connected is %i)." fmtstring .tell
;
