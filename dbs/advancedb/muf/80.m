( watchfor 2.1
  Sends players' presence information to subscribed players.
 
  If you enable notification, when any player in your watch list connects,
  disconnects, reconnects, goes away, or comes back, you will receive a
  message about it. Connection messages will be suppressed if players are
  in the same room {if it's not set Dark}, as it's assumed you can tell.
 
  When a player connects, he has a grace period {default 30 seconds} before
  notification is sent. During this time he can change his name, enable
  suppression, or disconnect to prevent display of the connection to
  folks subscribed to his presence.
 
  Properties
  _prefs/wf/on?      If "no", you will not receive presence messages.
  _prefs/wf/list     Your watchfor list. A reflist.
  _prefs/wf/@hide    The people who will not receive your presence even if
                     they subscribe. A reflist.
  _prefs/wf/@show    The people who can receive your presence even if you
                     hide from everyone.
  _prefs/wf/hide?    If "yes", only players in your exception list can
                     receive your presence {they must still put you in their
                     watchfor lists}.
  _prefs/wf/format   The format of presence messages you receive. Can have:
                     %n  the player's name
                     %v  the related verb {connected, disconnected}
                     plus any timefmt formatting codes.
  _prefs/wf/away?    If "no", you will not receive presence messages sent by
                     an external program via wf-announce {that is, away and
                     back messages}.
  _prefs/wf/idle?    If "yes", you will see players' idle times {or away
                     status} in the wf display.
 
  Version history
  2.0              created by tireless antebellum Forth-dwarves
  2.01 26 May 1995 Donatello: Patched in exceptions to the hidefrom list. You
                   now hide from Someone if Someone is in your hidefrom list
                   and not in your exception list. The string #all makes sense
                   only in the hidefrom list. Added by request.
  2.02 16 Aug 1995 Ruffin: Make dark players not show or be announced.
  2.03  6 Nov 1997 Pentrath: Added Nightwind's #clean to this version. Grace
                   time also extended to 60 seconds.
  2.04 14 Apr 1998 Natasha@SPR: Added @/wf_msg prop.
  2.05 30 Dec 1999 Natasha@SPR: Redefined '.pmatch' to get around the pmatch
                   bug where, say, 'Joe' in your wf list when there's no 'Joe'
                   character on the muck {through a name change or @toading}
                   would appear to exist and be awake, if 'Joel' existed and
                   was awake. Also added some comments to list-awake-watched
                   for not much reason.
  2.1  26 Sep 2003 Natasha@HLM: cleaned up for Fuzzball 6:
                   - removed temporary list feature
                   - does not notify wizards immediately
                   - no %l for wizards' formats
                   - defaults on
                   - exports wf-announce and wf-list-watched functions for
                     callers in prog's _cancall reflist, for away notifies
                     and so whodoing can be !W
                   - use real timefmt, not just %t
)
 
$iflib $lib/away $include $lib/away $endif
$include $lib/timestr
 
$def grace_time 60 (seconds)
 
$def prop_enabled "_prefs/wf/on?"
$def prop_format "_prefs/wf/format"
$def prop_list "_prefs/wf/list"
$def prop_hide? "_prefs/wf/hide?"
$def prop_hidelist "_prefs/wf/@hide"
$def prop_showlist "_prefs/wf/@show"
$def prop_spam $iflib $lib/away "_prefs/wf/away?" $else "_prefs/wf/spam?" $endif
$def prop_idle "_prefs/wf/idle?"
 
( The time at which the grace expires. Must be a time, not a flag, in case wf is killed or crashes before time expires. )
$def prop_grace "@/watchfor_grace"
 
: announce[ db:who str:action -- ]  ( Sends the 'Somewhere on the muck' message to all the people subscribed to the user's presence messages. )
    who @ location var! wholoc
 
    ( Who gets notified? )
    online_array  ( arrToNotify )
    prop_enabled "" array_filter_prop  ( arrToNotify )
    prop_list who @ "*{%d}*" fmtstring array_filter_prop  ( arrToNotify )
    who @ prop_hide? getpropstr .yes? if
        who @ prop_showlist array_get_reflist array_intersect  ( arrToNotify )
    else
        who @ prop_hidelist array_get_reflist swap array_diff  ( arrToNotify )
    then
 
    caller program? if
        prop_spam "" array_filter_prop  ( arrToNotify )
    else
        wholoc @ "dark" flag? not if
            wholoc @ contents_array swap array_diff  ( arrToNotify arrToRemove )
        then
    then   ( arrToNotify )
    me @ 1 array_make swap array_diff  ( arrToNotify )
 
    ( Notify those with the default format. )
    dup prop_format "" array_filter_prop dup if  ( arrSet arrDefault )
        dup rot array_diff swap  ( arrSet' arrDefault )
        action @ who @ "Somewhere on the muck, %D %s." fmtstring  ( arrSet' arrToNotify str )
        1 array_make swap array_notify  ( arrSet' )
    else pop then  ( arrSet )
 
    ( Notify those with special formats. )
    foreach swap pop  ( db )
        dup prop_format getpropstr  ( db str )
        "%n" "%N" subst me @ name "%n" subst  ( db str' )
        "%v" "%V" subst action @ "%v" subst
        systime timefmt  ( db str' )
        notify  (  )
    repeat  (  )
;
 
: names-to-arrays[ str:names -- arr arrBang arrBad ]  ( Splits the given string of names into two arrays of player dbrefs, one the names that were given plain and one given with exclamation points {not signs} before them. Unrecognized names are displayed with an error message. )
    0 array_make
    dup var! plus
    dup var! minus
    var! bad
 
    names @ strip dup if
        " " explode_array foreach swap pop  ( str )
            dup "!" stringpfx if 1 strcut swap pop minus else plus then swap  ( var str )
            dup pmatch  ( var str db )
            dup ok? if swap pop else pop swap pop bad swap then  ( var ? )
            over @ array_appenditem  ( var arr )
            swap !  (  )
        repeat  (  )
    else pop then  (  )
 
    plus @ minus @ bad @  ( arr arrBang arrBad )
;
 
: find-watched  ( -- )
    ( Find all online in user's list. )
    me @ prop_list array_get_reflist  ( arr )
    online_array 0 array_make array_union array_intersect  ( arr }  array_union is workaround for array_intersect bug )
 
    ( Remove those #hidefrom the user. )
    dup prop_hidelist me @ "*{%d}*" fmtstring array_filter_prop  ( arr arrHidefrom )
    swap array_diff  ( arr' )
 
    ( Remove those #hidefrom everyone, except those with user in #except. )
    dup prop_hide? "y*" array_filter_prop  ( arr' arrHide )
    dup if
        dup prop_showlist me @ "*{%d}*" fmtstring array_filter_prop  ( arr arrHide arrExcept )
        swap array_diff  ( arr' arrHide )
        swap array_diff  ( arr' )
    else pop then  ( arr' )
;
 
: list-watched  ( -- )
    find-watched  ( arr )
 
    me @ prop_idle getpropstr .yes? var! idle?
 
    0 var! ticker
    "" var! line  ( arr )
    foreach swap pop  ( db )
        dup me @ ignoring? if pop continue then  ( db )
 
        idle? @ if
$iflib $lib/away
            dup away-away? if
                "%D[away]" fmtstring  ( str )
            else
$endif
                dup descrleastidle descridle dup 180 > if  ( db intIdle )
                    stimestr strip swap "%D[%s]" fmtstring  ( str )
                else
                    pop name  ( str )
                then
$iflib $lib/away
            then
$endif
        else name then  ( str )
 
        "%-16.16s  " fmtstring  ( str )
 
        ticker @ ++ dup 4 = if  ( str int )
            pop 0 ticker !  ( str )
            line @ "%s%s\r" fmtstring line !  (  )
        else
            ticker !  ( str )
            line @ "%s%s" fmtstring line !  (  )
        then  (  )
    repeat  (  )
 
    line @ if
        "Players online for whom you are watching:" .tell  (  )
        line @ .tell
        "Done." .tell
    else
        "No one for whom you are watching is online." .tell
    then  (  )
;
 
: rtn-commas[ arr:who str:list -- str:names int:deleted ]
    0 var! deleted
    { who @ foreach swap pop  ( marker strN..str1 db )
        dup ok? if dup player? else 0 then if  ( marker strN..str1 db )
            name  ( marker strN..str2 str1 }  db's name is the new str1 )
        else  ( marker strN..str1 db )
            list @ if
                me @ list @ rot reflist_del
                deleted ++
            then
        then  ( marker strN..str1 )
    repeat }list  ( arr )
    SORTTYPE_CASEINSENS array_sort  ( arr' )
    ", " array_join  ( strNames )
    deleted @  ( strNames intDeleted )
;
 
: clean-list  ( -- )
    me @ prop_list array_get_reflist prop_list rtn-commas swap pop  ( intDeleted )
    me @ prop_hidelist array_get_reflist prop_hidelist rtn-commas swap pop +  ( intDeleted )
    me @ prop_showlist array_get_reflist prop_showlist rtn-commas swap pop +  ( intDeleted )
    "%i names removed from your lists." fmtstring .tell
;
 
: editlist[ str:names str:prop str:listname int:add -- ]  ( Adds the player{s} named str to the user's hidefrom list. )
    names @ names-to-arrays  ( arr arrBang arrBad )
 
    dup if  ( arr arrBang arrBad )
        ", " array_join " could not be identified." strcat .tell  ( arr arrBang )
    else pop then  ( arr arrBang )
 
    over over or if  (  )
        add @ if swap then  ( arrRemove arrAdd )
 
        me @ prop @ array_get_reflist  ( arrRemove arrAdd arr )
 
        over if
            over array_union swap  ( arrRemove arr' arrAdd )
 
            listname @ swap "" rtn-commas pop  ( arrRemove arr' strList strNames )
            "%s added to your %s list." fmtstring .tell  ( arrRemove arr' )
        else swap pop then  ( arrRemove arr' )
 
        over if
            over swap array_diff swap  ( arr' arrRemove )
 
            listname @ swap "" rtn-commas pop  ( arrRemove arr' strList strNames )
            "%s removed from your %s list." fmtstring .tell  ( arrRemove arr' )
        else swap pop then  ( arr' )
 
        me @ prop @ rot array_put_reflist  (  )
    else pop pop then  (  )
;
 
: add-hidefrom prop_hidelist "hidefrom"  1 editlist ;
: del-hidefrom prop_hidelist "hidefrom"  0 editlist ;
: add-except   prop_showlist "exception" 1 editlist ;
: del-except   prop_showlist "exception" 0 editlist ;
 
: list-hide ( -- }  Shows the user everyone from whom he is hiding {or isn't, if hiding from everyone}. )
    me @ prop_hide? getpropstr .yes? if
        me @ prop_showlist array_get_reflist  ( arr )
        dup if  ( arr )
            prop_showlist rtn-commas  ( str int )
            dup if "%i deleted players removed from your #show list." fmtstring .tell else pop then  ( str )
            "Hiding from everyone except: %s" fmtstring  ( str )
        else
            pop "Hiding from everyone."  ( str )
        then  ( str )
    else
        me @ prop_hidelist array_get_reflist  ( arr )
        dup if
            prop_hidelist rtn-commas  ( str int )
            dup if "%i deleted players removed from your #hide list." fmtstring .tell else pop then  ( str )
            "Hiding from: %s" fmtstring  ( str )
        else
            pop "Hiding from no one."  ( str )
        then  ( str )
    then  ( str )
    .tell  (  )
;
 
: do-event  ( str -- )
    "disconnect" stringcmp not var! discon?  (  )
 
    me @ prop_grace getpropval systime > if exit then  (  } Grace must be a time in case program halts before it can clear the prop. )
 
    discon? @ not if  (  )
        ( Notify user of people in *his* list online. )
        fork not if  (  )
            me @ prop_list getpropstr if
                list-watched
            then
            exit  (  )
        then  (  )
    then
 
    discon? @ not me @ awake? 1 = and if  (  }  Only provide new grace if not disconnecting and only connected once {ie, not reconnecting}. Reconnections *during* grace are already handled by exiting above. )
        me @ prop_grace systime grace_time + setprop  (  )
 
        grace_time sleep
        me @ prop_grace remove_prop
 
        me @ awake? not if exit then  (if not logged on, ignore)
    then  (  )
 
    me @  ( db )
    discon? @ if
        "disconnected"
    else
        me @ awake? 1 = if "connected" else "reconnected" then  ( db str )
    then  ( db str )
 
    announce  (  )
;
 
: allow-some-callers  ( -- )
    prog "_cancall" caller reflist_find not if
        "You cannot call $lib/wf functions." abort
    then  (  )
;
 
: wf-find-watched allow-some-callers find-watched ;
: wf-announce     allow-some-callers announce ;
 
: main  ( str -- )
    command @ "Queued Event." stringcmp not if do-event exit then  ( str -- )
 
    striplead dup "#" stringpfx if  ( str )
        1 strcut swap pop  ( str' )
        " " split swap  ( strArg strCmd )
 
        case
            "list" stringcmp not when pop  (  )
                me @ prop_list array_get_reflist  ( arr )
                prop_list rtn-commas  ( str int )
                dup if "%i deleted players were removed from your list." fmtstring .tell else pop then  ( str )
                dup if
                    "Currently watching for: " swap strcat .tell  (  )
                else
                    pop "Currently watching for no one." .tell  (  )
                then  (  )
                exit  (  )
            end
 
            "help" stringcmp not when pop .showhelp end
            "clean" stringcmp not when pop clean-list end
 
            "off" stringcmp not when pop
                me @ prop_enabled "no" setprop
                "You will not be messaged with players' presence changes." .tell
            end
            "on" stringcmp not when pop
                me @ prop_enabled remove_prop
                "You will receive messages when presence to which you've subscribed changes." .tell
            end
 
            "!hideall" stringcmp not when pop
                me @ prop_hide? remove_prop
                "Anyone not in your #hide list can now subscribe to your presence." .tell
            end
            "hideall" stringcmp not when pop
                me @ prop_hide? "yes" setprop
                "Only people in your #show exception list will receive your presence." .tell
            end
 
            "hide" stringpfx when  ( strArg )
                dup if add-hidefrom else pop list-hide then
            end
            dup "nohide" stringcmp swap "show" stringcmp and not when  ( strArg )
                dup if add-except else pop list-hide then
            end
 
            $iflib $lib/away "away" $else "spam" $endif stringpfx when pop
                me @ prop_spam remove_prop
                "You will see additional presence messages." .tell
            end
            $iflib $lib/away "!away" $else "!spam" $endif stringpfx when pop
                me @ prop_spam "no" setprop
                "You will no longer see additional presence messages." .tell
            end
 
            "idle" stringpfx when pop
                me @ prop_idle "yes" setprop
                "You will see players' idle times in the watchfor display." .tell
            end
            "!idle" stringpfx when pop
                me @ prop_idle remove_prop
                "You will no longer see players' idle times in the watchfor display." .tell
            end
 
            default "I don't know what you mean by '#%s'. 'wf #help' for help." fmtstring .tell end
        endcase
        exit
    then
 
    dup if  ( str )
        prop_list "watchfor" 1 editlist  (  )
    else pop  (  )
        list-watched
    then
;
 
PUBLIC wf-announce $libdef wf-announce
PUBLIC wf-find-watched $libdef wf-find-watched
 
