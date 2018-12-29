( Shout 4.0 by Natasha
  based on Shout 3.2 by Riss
 
  To install:
  - Install the program and fiddle the $defs.
  - Set the program W.
  - Make a @shout;@wshout;@wall command linked to program
 
  Added:
  - built-in #who
  - ignore provided by $lib/ignore
  - 'blog' support via fb6 userlog prim
  - #wizon and #wizoff instead of #on and #off
  - channels don't need #create-d, just #join them
  - A lack of custom formats for wizards
 
  Changed internals:
  - *prop defs renamed prop_*
  - DO* words renamed do-*
)
 
$version 4.0
$author Riss, Natasha
$note Public message channels for players and wizards.
 
$include $lib/reflist
$include $lib/editor
 
$iflib $lib/ignore
    $include $lib/ignore
$else
    $include $lib/match
    $def prop_ignore "_prefs/shout/ignore"
$endif
 
( Minimum number of seconds between shouts. )
$def int_shouttime 300
( $def to charge pennies. )
$undef tocharge
( $def to allow * public shout-spoofs. )
$undef spoof
( $def to enable channels. Natasha@HLM 1 June 2002 )
$def channels
( $def to log dbrefs and names of shouters. )
$undef logging
( $def to enable #blog, which sends some shouts to fb6 userlog. Natasha@HLM 3 June 2003 )
$undef blog
 
$def prop_hearwiz "_prefs/shout/hear?"
$def prop_hearpub "_prefs/shout/hearpub?"
$def prop_pubtime "@shout/time"
$def prop_ban "@shout/banned"
$def prop_smatch "_prefs/shout/smatch"
$def prop_monitor "_monitor"
$def prop_limit "_prefs/shout/limit"
 
$ifdef blog  ( Natasha@HLM 3 June 2003 )
    $def prop_blog "_prefs/shout/blog?"
$endif
 
$def propdir_timelastused "/tlu/"
$def propdir_lists "/^lists/"
$def listbasei propdir_lists strlen
 
$ifdef channels
    $def prop_lastchan "_prefs/shout/lastchan"
    $def int_listmax 100
    $def int_listnamemax 16
    $def int_listage 604800 
$endif
 
VAR TheLine
VAR InLine
VAR charge
VAR listeners
VAR list
VAR awakeonly?
 
 
: dohelp  ( -- }  Prints the default #help. )
    {
    "@shout by Riss"
    "Sends public messages to everyone who's listening for them."
    " "
    "@shout <message>    Send a public shout."
    "@shout :<pose>      Send a public pose."
$ifdef spoof
    "@shout *<message>   Make a shout-spoof."
    "@shout !<message>   Make an entirely freeform shout. (Wizards only.)"
$endif
    "@shout #pubon       Hear everyone's @shouts."
    "@shout #puboff      Stop listening to everyone's @shouts."
    " "
    "@wshout <message>   Make a wizard shout. (Wizards only.)"
    "@shout #wizon       Hear wizard shouts."  (Use instead of #on/off Natasha@HLM 1 January 2003)
    "@shout #wizoff      Stop listening to wizard shouts."  (Use instead of #on/off Natasha@HLM 1 January 2003)
    " "
    "@wall <message>     Make an urgent wizard shout. (Wizards only.)"
    "                    Urgent wizard shouts cannot be turned off."
    " "
    "@shout #help2       View more options."
 
$ifdef tocharge
    " " .tell
$ifdef channels
    "Shouts cost 1 penny per character (except on channels), so long shouts cost more."
$else
    "Shouts cost 1 penny per character, so long shouts cost more."
$endif
$endif
 
    }tell  (  )
;
 
: dohelp2  ( -- }  Displays screen two of #help. )
    {
    "@shout by Riss"
    " "
    "@shout #who             See who will hear your public shout."
    "@shout #ignore <name>   Ignore <name>'s shouts."
    "@shout #!ignore <name>  Stop ignoring <name>'s shouts."
    "@shout #ignore          List whose shouts you're ignoring."
    "@shout #smatch          Edits your list of 'stop word' patterns. Public shouts"
    "                        that contain any patterns in your list will be ignored,"
    "                        no matter who @shouted it. Each line of the list should"
    "                        be a smatch pattern (for example: *birthday* ). See"
    "                       'man smatch' for details."
    "@shout #limit <number>  Ignore shouts more than <number> characters long."
    "@shout #limit 0         Stop ignoring long @shouts."
    "@shout #limit           Show your length limit."
 
$ifdef blog  ( Natasha@HLM 3 June 2003 )
    "@shout #blog            Toggle whether your shouts are sent to the shout weblog."
$endif
 
    me @ "W" flag? if
        " "
        "Wizard commands:"
$ifdef channels
        "@shout #wipe <chan>     Wipes out a channel."
$endif
        "@shout #log             Shows last few folks who @shouted."
        "@shout #mon | #!mon     Turns the @shout monitor on or off."
    then
 
$ifdef channels
    " "
    "For commands relating to channels: @shout #help3"
$endif
 
    }tell
;
 
$ifdef channels
: dohelp3  ( -- }  Show channel help. )
    {
    "@shout #list              List all channels."
    "@shout #list <pattern>    List channels matching the smatch pattern."
    "@shout #awake [<pattern>] Like #list, but only lists awake members."
    "@shout #mine              Lists channels you are on."
    "@shout #create <chan>     Creates channel and makes you a member."
    "@shout #join <chan>       Joins you to a channel."
    "@shout #leave <chan>      Removes you from a channel list."
    " "
    int_listnamemax "Channel names can be at most %i characters, which must be letters," fmtstring
    "numbers, or the dash ('-')."
    }tell
;
 
: setlist  ( strChannelName -- boolValid }  Check the given channel name for validity. Also, set the global "list" variable and user's most recently used channel. )
    strip tolower  ( str )
    dup not if
        pop me @ prop_lastchan getpropstr
    then  ( str )
 
    dup "*[^-a-z0-9-]*" smatch if  ( str )
        "List names cannot contain special characters." .tell  ( str )
        pop 0 exit  ( boolValidName )
    then  ( str )
 
    dup strlen int_listnamemax > if
        int_listnamemax "List names can be at most %i characters." fmtstring .tell  ( str )
        pop 0 exit  ( boolValidName )
    then  ( str )
 
    dup list !  ( str )
    me @ prop_lastchan rot setprop  (  )
    1
;
$endif
 
: rtn-wizStar?  ( strCmd -- bool }  Return true if user is a wizard doing a <strCmd>, whatever <strCmd> is. )
    (me @ "W" flag?
    command @ "@wshout" stringcmp not
    and}  Begin catch nonwizard errors thrown by wizit? and wizall? Natasha@HLM 21 October 2002 )
    dup command @ stringcmp if  ( strCmd )
        pop 0 exit  ( bool )
    then  ( strCmd )
 
    me @ "W" flag? not if  ( strCmd )
        ( Exception! )
        "Only wizards can %s." fmtstring abort  (  )
    then pop  (  )
 
    1  ( bool }  End catch nonwizard errors thrown by wizit? and wizall? Natasha@HLM 21 October 2002 )
;
 
: wizit?  ( -- bool }  Return true if user is a wizard doing a @wshout. )
    "@wshout" rtn-wizStar?
;
: wizall?  ( -- bool }  Return true if user is a wizard doing a @wall. )
    "@wall" rtn-wizStar?
;
 
: recordtime  ( -- }  Save now as the last time the user did a public shout. )
    me @ prop_pubtime systime setprop  (  )
;
: gettime  ( -- int }  Get the last time the user did a public shout. )
     me @ prop_pubtime getpropval  ( int )
;
: checktime  ( -- bool }  Return true if NOT enought time has elapsed for the user to make a public shout now. )
    gettime int_shouttime +  ( int }  The systime when the user can next shout. )
    systime >  ( bool }  True if the systime when the user can next shout is in the future. )
;
: timetogo  ( -- }  Tell the user in how many more seconds he can really public shout. )
    gettime int_shouttime + systime -  ( int )
    "Come back in %i seconds." fmtstring .tell  (  )
;
 
: isasmatch?  ( db -- boolMatches }  Return true if TheLine matches one of db's smatch stoppatterns. )
    prop_smatch array_get_proplist  ( arr )
    foreach swap pop  ( strPattern )
        TheLine @ swap smatch if  (  )
            1 exit  (  )
        then  (  )
    repeat  (  )
    0  (  )
;
 
: hearthis  ( db -- bool }  Return true if db should hear the given shout. )
    dup player? not if  ( db )
        ( Bad dbref. )
$ifdef channels
        prog propdir_lists list @ strcat rot reflist_del  (  )
$else
        pop  (  )
$endif
        0 exit  ( bool )
    then  ( db )
 
    ( Always hear yourself. )
    dup me @ dbcmp if pop 1 exit then  ( db )
    dup awake? not if pop 0 exit then  ( db )
    ( Everyone hears @walls. )
    wizall? if pop 1 exit then  ( db )
 
    ( Am I even listening for public/wiz shouts? )
$ifdef channels
    list @ not if  ( db }  If it's a list shout, we already established they wanted to hear the shout by finding them in the list. )
$endif
        dup  ( db db )
        wizit? if prop_hearwiz else prop_hearpub then  ( db db strHearProp )
        getpropstr .no? if  ( db )
            pop 0 exit  ( bool )
        then  ( db )
$ifdef channels
    then  ( db )
$endif
 
    ( Ignore? )
$iflib $lib/ignore
    dup me @ "shout" ignores?  ( db bool }  Use $lib/ignore Natasha@HLM 1 January 2003; optionalized for 4.0 )
$else
    prop_ignore me @ REF-inlist?  ( db bool )
$endif
    if pop 0 exit then
 
    ( Exceeding length limit? )
    dup prop_limit getpropstr dup if  ( db strLimit )
        atoi charge @ < if  ( db )
            pop 0 exit  ( bool )
        then  ( db )
    else pop then  ( db )
 
    isasmatch?  ( boolSmatches }  Last check so it's OK to consume the db. )
    not  ( boolDoesNotSmatch )
;
 
: TellAll  ( str -- }  Notify shout str to everyone who should be notified of shout str. )
    TheLine !  (  )
 
$ifdef channels
    list @ if  (  )
        prog propdir_lists list @ strcat array_get_reflist  ( arr )
    else
$endif
        online_array 0 array_make array_union  ( arr )
$ifdef channels
    then  ( arr )
$endif
 
    0 array_make swap foreach swap pop  ( arr db )
        dup hearthis if
            swap array_appenditem  ( arr )
        else
            pop  ( arr )
        then  ( arr )
    repeat  ( arrDb )
 
    TheLine @ 1 array_make swap  ( arrShout arrDb )
    dup array_count listeners !  ( arrShout arrDb )
    array_notify  (  )
 
    ( Do monitoring. )
    prog prop_monitor getpropstr if
        TheLine @ "Monitor> " swap strcat 1 array_make  ( arrShout )
        prog prop_monitor array_get_reflist  ( arrShout arrDb )
        array_notify  (  )
    then
 
$ifdef blog  ( Blog support Natasha@HLM 3 June 2003; moved later for 4.0 )
    ( Blog. )
    me @ prop_blog getpropstr .yes?  ( bool )
$ifdef channels
    list @ not and  ( bool )
$endif
    if  ( bool )
        TheLine @ userlog
    then
$endif
;
 
: isfirstchar?  ( strString strChars -- bool }  Return true if the first character of strString is *any* of the characters in strChars. )
    swap  ( strChar strString )
    1 strcut pop  ( strChar strString- )
    instr  ( bool )
;
 
: get_pose  ( -- strPose strFormat }  
    "pose"  ( strChannel strPose )
 
    inline @ ":!?',-." isfirstchar? if
        "%m"  ( strChannel strPose strFormat )
    else
        " %m"  ( strChannel strPose strFormat )
        charge ++  ( strChannel strPose strFormat }  Add an extra one for the space. )
    then  ( strChannel strPose strFormat )
;
 
: evalline  ( -- str }  Return the format of the shout with '%m' for the actual message. )
    inline @ strlen charge !
 
    inline @ 1 strcut inline ! case  ( strChar )
$ifdef spoof
        "!" strcmp not me @ "w" flag? and when  (  )
            ( All done! )
            inline @ exit  ( str )
        end
        "*" strcmp not when "spoof" "( %m )" end  ( strPose strFormat )
$endif
        ":" strcmp not when
            "pose"  ( strPose )
 
            inline @ 1 strcut pop ":!?',-." isfirstchar? if
                "%n%m"
            else
                "%n %m"  ( strPose strFormat )
                charge ++  ( strPose strFormat }  Charge extra for the space. )
            then
 
            ( While we're at it, bump up the price for including the name. )
            charge @ me @ name strlen + charge !  ( strPose strFormat )
        end
        default  ( strChar )
            ( Reattach. )
            inline @ strcat inline !  (  )
 
            "shout" "%n shouts \"%m\""  ( strPose strFormat )
        end
    endcase  ( strPose strFormat )
 
$ifdef channels
    ( Channel? )
    list @ if  ( strPose strFormat )
        " Channel " list @ strcat  ( strPose strFormat strChannel )
    else "" then  ( strPose strFormat strChannel )
    -3 rotate  ( strChannel strPose strFormat )
$endif
 
    ( 'Wiz' or 'Public'? )
    wizit? wizall? or if "Wiz" else "Public" then  ( {strChannel} strPose strFormat strWiz )
    swap  ( {strChannel} strPose strWiz strFormat )
 
    ( Assemble. )
    me @ name "%n" subst  ( {strChannel} strPose strWiz strFormat' }  Spoof's strFormat has no %n, so no change then. )
$ifdef channels
    "%s  (%s-%s%s)"  ( strChannel strPose strWiz strFormat' str )
$else
    "%s  (%s-%s)"  ( strPose strWiz strFormat' str )
$endif
    fmtstring  ( str' )
 
    inline @ "%m" subst  ( str" )
;
 
: dowizon  ( -- }  Set the user's preference to hear wizard shouts. )
    me @ prop_hearwiz remove_prop
    "You will hear wizard shouts." .tell
;
: dowizoff  ( -- }  Set the user's preference to not hear wizard shouts. )
    me @ prop_hearwiz "no" setprop
    "You will NOT hear wizard shouts. You will still hear urgent wizard shouts, as everyone does." .tell
;
 
: dopubon  ( -- }  Set the user's preference to hear public shouts from players. )
    me @ prop_hearpub remove_prop  ( pub channel should default on Natasha@HLM 17 June 2002 )
    "You will hear public shouts." .tell
;
: dopuboff  ( -- }  Set the user's preference to not hear public shouts. )
    me @ prop_hearpub "no" setprop  ( pub channel should default on Natasha@HLM 17 June 2002 )
    "You will not hear public shouts." .tell
;
 
$ifnlib $lib/ignore
: doignorelist  ( -- }  Show the user who he's ignoring. )
    "You ignore: "
    me @ prop_ignore getpropstr if 
        me @ prop_ignore REF-list
    else
        "no one"
    then
    strcat .tell  (  )
;
 
: doignore  ( str -- }  Add the person with the given name to the user's ignore list. )
    dup not if  ( str )
        pop doignorelist exit  (  )
    then  ( str )
    .noisy_pmatch  ( db )
    dup ok? not if pop exit then  ( db )
 
    me @ prop_ignore 3 pick REF-inlist? if  ( db )
        "You already ignore %D." fmtstring .tell  (  )
        exit  (  )
    then  ( db )
 
    me @ shoutignorelist 3 pick REF-add  ( db )
    "You will no longer hear shouts from %D." fmtstring .tell  (  )
;
 
: donignore  ( str -- }  Remove the person with the given name from the user's ignore list. )
    .noisy_pmatch
    dup ok? not if pop exit then  ( db )
 
    me @ prop_ignore 3 pick REF-inlist? if  ( db )
        "You aren't ignoring %D." fmtstring .tell  (  )
        exit  (  )
    then  ( db )
    
    me @ prop_ignore 3 pick REF-delete  ( db )
    "You will hear shouts from %D." fmtstring .tell  (  )
;
$endif
 
: dosmatch  ( -- }  Drop user into list editor. )
    {
    "You are about to be placed in the Shout SMATCH list editor."
    "Use standard LSEDIT commands to build your smatch ignore list, 1 per line."
    "Any shout that matches any of the patterns will be ignored."
    "Example: *birthday*    will ignore any shout with the word birthday in it."
    }tell
 
    me @ prop_smatch array_get_proplist array_vals  ( strN..str1 intN )
 
    EDITOR
 
    "end" stringcmp if  ( strN'..str1' intN' )
        popn  (  )
        "Smatch list edit aborted." .tell  (  )
    else
        array_make  ( arr )
        me @ prop_smatch rot array_put_proplist  (  )
        "Smatch list saved." .tell  (  )
    then  (  )
;
 
$ifdef channels
: test-awake dup player? if awake? else pop 0 then ;
: test-player player? ;
 
: listmembers  ( strChannelprop -- }  List the people listening to strChannel. )
    awakeonly? @ if 'test-awake else 'test-player then  ( strChannel addr )
    prog 3 pick  ( strChannelprop addr dbProg strChannelprop )
    REF-filter array_make  ( strChannelprop arr )
    ( If that's all valid players, set back so the nonplayers are removed. )
    awakeonly? @ not if  ( strChannelprop arr )
        prog  ( strChannelprop arr dbProg )
        3 pick  ( strChannelprop arr dbProg strChannelprop )
        3 pick  ( strChannelprop arr dbProg strChannelprop arr )
        array_put_reflist  ( strChannelprop arr )
    then  ( strChannelprop arr )
 
    "" swap foreach swap pop  ( strChannelprop str db )
        "%D %s" fmtstring  ( strChannelprop str' )
    repeat  ( strChannelprop strList )
 
    swap listbasei strcut swap pop  ( strList strChannel )
 
    propdir_timelastused over strcat getpropval  ( strList strChannel intTime )
    "%m/%d %H:%M" swap timefmt  ( strList strChannel strTime )
    swap  ( strList strTime strChannel )
 
    int_listnamemax dup  ( strList strTime strChannel intChanlen intChanlen )
    "%%-%i.%is  %%11.11s  %%s" fmtstring fmtstring  ( str )
 
    .tell  (  )
;
 
: wipeoldlist  ( strChannelprop -- boolExpired }  If the given list has not been used in the last so ever long, expire it. )
    dup listbasei strcut swap pop  ( strChannelprop strChannel )
    prog propdir_timelastused rot strcat getpropval  ( strChannelprop intLastused )
    int_listage +  ( strChannelprop intTimeToExpire )
    ( Has the time to expire passed? )
    systime < if  ( strChannelprop )
        prog swap remove_prop  ( strChannelprop )
        ("Expired %s" fmtstring .tell  (  }  Notify why? )
        1  ( strChannelprop boolExpired )
    else pop 0 then  ( boolExpired )
;
 
lvar listpattern
 
: test-mine ( strProp -- boolShow }  Return 1 if the user is in the given channel. )
    prog swap me @ REF-inlist?
;
: test-pattern ( strProp -- boolShow }  Return 1 if the channel name matches the given pattern. )
    listbasei strcut swap pop listpattern @ smatch
;
: test-all ( strProp -- boolShow }  Return 1 if the channel name matches the given pattern. )
    pop 1
;
 
: listchannels[ addr:test -- ] ( List the lists for which the given function is true. )
    prog propdir_lists nextprop ( str )
    dup not if  ( str )
        "-- No lists! --" .tell
        pop exit
    then  ( strFirst )
 
    "Names --" "Last used" "Channel"  ( strFirst strList strLastused strChannel )
    int_listnamemax dup  ( strFirst strList strTime strChannel intChanlen intChanlen )
    "%%-%i.%is  %%11.11s  %%s" fmtstring fmtstring  ( strFirst str )
    .tell  ( strFirst )
 
    begin dup while  ( strProp )
        dup wipeoldlist not if  ( strProp )
            dup test @ execute if  ( strProp )
                dup listmembers  ( strProp )
            then  ( strProp )
        then
    prog swap nextprop repeat
    "-- End channel list --" .tell
;
 
: dolistmine 'test-mine listchannels ;
: dolist  ( strPattern -- }  Lists list or members. )
    dup if
        listpattern ! 'test-pattern  ( addr )
    else
        pop 'test-all  ( addr )
    then  ( addr )
    listchannels  (  )
;
 
: dojoin  ( strChannel -- }  Add the user to the given list. )
    setlist not if exit then list @  ( strChannel )
 
    prog propdir_lists 3 pick strcat array_get_reflist array_count  ( strChannel intMembers )
    dup if  ( strChannel intMembers )
        int_listmax >= if  ( strChannel )
            "List %s is full." fmtstring .tell  (  )
            exit  (  )
        then  ( strChannel )
    else  ( strChannel intMembers )
        ( No one's there so assume we made it, and furthermore, set last-used time to now so the new channel doesn't expire. )
        pop  ( strChannel )
        prog propdir_timelastused 3 pick strcat systime setprop  ( strChannel )
    then  ( strChannel )
 
    prog propdir_lists 3 pick strcat me @ REF-add  ( strChannel )
    "You are listening to channel %s." fmtstring .tell  (  )
;
 
: doleave  ( strChannel -- }  Remove the user from the given channel. )
    setlist not if EXIT then list @  ( strChannel )
 
    prog propdir_lists 3 pick strcat me @ REF-delete  ( strChannel )
    "You are no longer listening to channel %s." fmtstring .tell  (  )
;
 
: dowipe  ( strChannel -- }  If the user is a wizard, wipe out the given channel. )
    me @ "w" flag? not if
        "Only wizards can wipe channels." .tell  ( strChannel )
        pop exit  (  )
    then  ( strChannel )
 
    prog propdir_lists 3 pick strcat remove_prop  ( strChannel )
    "The channel %s is wiped." fmtstring .tell  (  )
;
$endif
 
: domonitor  ( -- }  Add the user to the monitor list. )
    prog prop_monitor me @ REF-add
    "Monitor on." .tell
;
 
: donmonitor  ( -- }  Remove the user from the monitor list. )
    prog prop_monitor me @ REF-delete
    "Monitor off." .tell
;
 
$ifdef logging
: showlogs ( -- )
    me @ "w" flag? not if "Eh?" .tell EXIT then
    "Names: " prog "logs/names" getpropstr strcat .tell
    "     refs: " prog "logs/refs" getpropstr strcat .tell
;
$endif
 
: dolimit  ( string -- )
    dup if
        atoi dup case
            0 < when
                "Limits should be greater than zero." .tell  ( intLimit )
                pop  (  )
            end
            when
                ( Set limit. )
                me @ prop_limit 3 pick setprop  ( intLimit )
                "You will hear shouts up to %i characters long." fmtstring .tell  (  )
            end
            default  ( intLimit intLimit )
                ( Clear limit. )
                pop pop  (  )
                me @ prop_limit remove_prop  (  )
                "You will hear shouts of any length." .tell  (  )
            end
        endcase
    else
        ( Show limit. )
        pop  (  )
        me @ prop_limit getpropval dup if  ( intLimit )
            "You hear shouts up to %i characters long." fmtstring .tell  (  )
        else
            pop  (  )
            "You have set no limit." .tell  (  )
        then
    then  (  )
;
 
$ifdef blog  ( Natasha@HLM 3 June 2003 )
: doblog  ( -- }  Set the user's preference to blog shouts. )
    "Your shouts will be sent to the shout weblog." .tell
    me @ prop_blog "yes" setprop
;
 
: donblog  ( -- }  Set the user's preference to not blog shouts. )
    "Your shouts will not be blogged." .tell
    me @ prop_blog remove_prop
;
$endif
 
: dowho  ( -- }  Displays a list of who will see a public shout.  Natasha@HLM 1 June 2002 )
    ""  ( str )
    online_array foreach swap pop  ( strList db )
        dup hearthis if  ( strList db )
            "%D %s" fmtstring  ( strList )
        else pop then  ( strList )
    repeat  ( strList )
 
    "Your shouts will be heard by: " swap strcat .tell  (  )
 
$ifdef blog
    me @ prop_blog getpropstr .yes? if "are" else "are not" then
    "Your shouts \[[1m%s\[[0m being sent to the shout weblog." fmtstring .tell
$endif
;
 
$define dict_commands {
    "help"   'dohelp
    "help2"  'dohelp2
    "wizon"  'dowizon  ( Make #wizon/off for wiz shouts and #on/off for all shouts Natasha@HLM 28 July 2002 )
    "wizoff" 'dowizoff
    "pubon"  'dopubon
    "puboff" 'dopuboff
    "who"    'dowho
    "smatch" 'dosmatch
    "limit"  'dolimit
    "mon"    'domonitor
    "!mon"   'donmonitor
$ifdef logging
    "log"    'showlogs
$endif
$ifnlib $lib/ignore
    "ignore"  'doignore
    "!ignore" 'donignore
$endif
$ifdef channels
    "help3" 'dohelp3
    "mine"  'dolistmine
    "list"  'dolist
    "join"  'dojoin
    "leave" 'doleave
    "wipe"  'dowipe
$endif
$ifdef blog  ( Natasha@HLM 3 June 2003 )
    "blog"  'doblog
    "!blog" 'donblog
$endif
}dict $enddef
 
: main[ str:arg -- ]
$ifdef channels
    "" list ! 0 awakeonly? !
$endif
    arg @ striplead dup "#" stringpfx if  ( str )
        1 strcut swap pop " " split swap  ( strYZ strX )
 
        dict_commands over array_getitem  ( strYZ strX ? )
        dup address? if  ( strYZ strX addr )
            swap pop  ( strYZ addr )
            execute exit  ( {strYZ} )
        then pop  ( strYZ strX )
 
        case
            "on"  stringcmp not when dowizon  dopubon  end
            "off" stringcmp not when dowizoff dopuboff end
            "ignore"  stringcmp not when "shout" cmd-ignore-add end
            "!ignore" stringcmp not when "shout" cmd-ignore-del end
$ifdef channels
            "awake" stringcmp not when 1 awakeonly? ! dolist end
$endif
            default  ( strYZ strX )
                command @ swap  ( strYZ strCmd strX )
                "I don't know what you mean by '#%s'. Try '%s #help'." fmtstring .tell  ( strYZ )
            end
        endcase  ( ...? )
        exit  ( ...? )
    then  ( str )
 
    me @ prop_ban getpropstr if  ( str )
        "You have been banned from using shout." .tell  ( str )
        pop exit  (  )
    then  ( str )
 
    (channel list parseing here)
$ifdef channels
    dup "=" instr if  ( str )
        ( Channel! )
        "=" split swap setlist not if pop exit then  ( str )
 
        prog propdir_lists list @ strcat getpropstr not if  ( str )
            list @ "The channel '%s' doesn't exist." fmtstring .tell  ( str )
            pop exit  (  )
        then  ( str )
    then  ( str )
$endif
    strip InLine !
    evalline  ( strShout )
 
    me @ "W" flag? not  ( strShout )
$ifdef channels
    list @ not and
$endif
    if  ( strShout )
 
        me @ prop_hearpub getpropstr .no? if  ( strShout }  pub channel should default on Natasha@HLM 17 June 2002 )
            "You must be @shout #pubon to make public shouts." .tell  ( strShout )
            pop exit  (  )
        then
 
        checktime if  ( strShout )
            int_shouttime 60 / "Sorry, you're only allowed one shout every %i minutes." fmtstring .tell  ( strShout )
            timetogo  ( strShout )
            pop exit  (  )
        then  ( strShout )
 
$ifdef tocharge
        charge @ me @ pennies > if  ( strShout )
            "pennies" sysparm "You dont have enough %s to shout that." fmtstring .tell  ( strShout )
            pop exit  (  )
        then  ( strShout )
        "pennies" sysparm charge @ "This shout costs %i %s." fmtstring .tell  ( strShout )
        me @ 0 charge @ - addpennies  ( strShout )
$endif
 
    then  ( strShout )
 
$ifdef channels
    list @ if
        prog propdir_lists list @ strcat me @ REF-inlist? not if
            list @ "You aren't listening to channel %s." fmtstring .tell  ( strShout )
            pop exit  (  )
        then
    then  ( strShout )
$endif
 
$ifdef logging
    PREEMPT  ( Chip_Unicorn chastised me once with this, heh. )
        prog "logs/refs" getpropstr " #" strcat me @ int intostr
        strcat dup " #" instr strcut swap pop
        prog "logs/refs" rot setprop
        prog "logs/names" getpropstr " " strcat me @ name
        strcat dup " " instr strcut swap pop
        prog "logs/names" rot setprop
$endif
    BACKGROUND  ( strShout )
 
$ifdef DEBUG
    TellAll  (  )
$else
    1 try  ( strShout )
        TellAll  (  )
    catch  ( strErr }  Catch nonwizard errors thrown by wizit? and wizall? Natasha@HLM 21 October 2002 )
        dup "Only wizards" stringpfx not if
            ( If it wasn't the wizit? or wizall? abort, tell us what it was. )
            "An error occurred: " swap strcat
        then  ( str' )
        .tell  (  )
    endcatch
$endif
 
    ( We no longer sleep during telling so it's OK to record time after. )
$ifdef channels
    list @ not if recordtime then
$else
    recordtime  ( str )
$endif
 
$ifdef channels
    list @ if
        prog propdir_timelastused list @ strcat systime setprop
    then
$endif
 
$ifdef blog
    me @ prop_blog getpropstr .yes?  ( bool )
$ifdef channels
    list @ not and  ( bool )
$endif
    if "and the web " else "" then  ( strWeb )
    listeners @ "%i folks %sheard that." fmtstring .tell  (  )
$else
    listeners @ intostr " folks heard that." strcat .tell  (  )
$endif
;
