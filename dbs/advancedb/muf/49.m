( cmd-laston.muf by Natasha@HLM
  A laston for Fuzzball 6.
 
  Forked from cmd-whodoing for Fuzzball 6. Factored table code into $lib/table.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
 
  Version history
  1.001, 16 March 2003: don't use .rtimestr macro, use rtimestr in
    $lib/timestr. Default USE_ATLASTON to on.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.001
$note A Fuzzball 6 laston.
 
$include $lib/strings
$include $lib/table
$include $lib/timestr
$iflib $lib/alias $include $lib/alias $endif  ( Honor aliases issue417 Natasha@HLM 9 January 2003 )
 
$def prop_wsobject "_prefs/ws/object"
$def prop_wflist "_prefs/wf/list"
$def prop_def "_prefs/laston/def"
$def prop_laston "@/last/on"
$def prop_lastoff "@/last/off"
 
( Uncomment if a _connect event sets users' @/laston properties. )
$def USE_ATLASTON
 
var lineformat
var metadata
 
 
: rtn-lasttime  ( db strProp -- intTime }  Gets the systime in the given property. If the program is not configured to use @/laston properties, the player's last usetime time of connection} is used instead. )
$ifdef USE_ATLASTON
    getpropstr  ( strTime )
    dup number? if atoi else pop -1 then  ( intTime )
$else
    pop timestamps pop -3 rotate pop pop  ( intTime )
$endif
;
 
: rtn-lastcon  ( db -- intTime }  Gets the systime when the given dbref most recently connected. )
    prop_laston rtn-lasttime
;
: rtn-lastdiscon  ( db -- intTime }  Gets the systime when the given dbref most recently disconnected. Use this time for when a player was last 'on.' )
    prop_lastoff rtn-lasttime
;
 
 
( ** data getters  { db -- str }  )
: get-name name ;
: get-con rtn-lastcon "%R %e %b %Y" swap timefmt ;
: get-discon rtn-lastdiscon "%R %e %b %Y" swap timefmt ;
 
: get-laston  ( db -- str )
    dup awake? if pop "\[[1;32mOnline now\[[0m" exit then  ( db )
    rtn-lastdiscon dup -1 = if  ( intTime )
        pop "\[[1;33mNever connected\[[0m" exit  ( str )
    then  ( intTime )
    systime swap -  ( intSince )
    rtimestr  ( str )
;
 
 
( ** list makers  { str -- arr }  )
: do-help pop .showhelp "exit" abort ;
: do-all
    dup not if do-help then
$iflib $lib/alias
    me @ swap alias-expand  ( Honor aliases issue417 Natasha@HLM 9 January 2003 )
$else
    .noisy_pmatch
    dup ok? if 1 array_make else pop "exit" abort then
$endif
;
: do-watchfor pop me @ prop_wflist array_get_reflist ;
: do-room  ( strY -- arr )
    pop loc @ contents_array  ( arrContents )
    0 array_make swap foreach swap pop  ( arrPlayers db )
        dup player? if swap array_appenditem else pop then
    repeat  ( arrPlayers )
;
 
 
$define dict_commands {
    "" 'do-all
    "room" 'do-room
    "here" 'do-room
    "wf" 'do-watchfor
    "wat" 'do-watchfor
    "watchfor" 'do-watchfor
    "help" 'do-help
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
 
    "%-16.16[name]s %-18.18[laston]s %-20.20[con]s %-20.20[discon]s" lineformat !
    {
        "name"   'get-name
        "laston" 'get-laston
        "con"    'get-con
        "discon" 'get-discon
    }dict metadata !  ( strY adr )
 
    2 try
        execute  ( arr )
    catch exit endcatch  ( arr )
    dup array_count not if  ( arr )
        "No one to show." .tell  ( arr )
        pop exit  (  )
    then  ( arr )
 
    lineformat @  ( arr strLinefmt )
    { "name" "\[[1;37mName" "laston" "\[[1;37mLast on" "con" "\[[1;37mLast connected" "discon" "\[[1;37mLast disconnected" }dict  ( arr strLinefmt dictHeaders )
    metadata @  ( arr strLinefmt dictHeaders dictProcessors )
    4 rotate  ( strLinefmt dictHeaders dictProcessors arr )
 
    table-table  (  )
;
