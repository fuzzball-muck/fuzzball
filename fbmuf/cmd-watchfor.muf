@prog con-announce
1 99999 d
1 i
( CON-Announce 2.00 -- announces logins/logouts to interested players.
  Also known as cmd-watchfor ver. 2.00
  
  If you "@set me=_prefs/con_announce?:yes", then when any player listed
  by name in your "_prefs/con_announce_list" property logs in or out, you
  will be notified.  You will not be informed if the connecting player is
  in the same room as you, unless the room is set DARK;  it is assumed that
  you already know that they are connecting/disconnecting.
  When a person logs in, they have a half minute grace period in which they
  can rename themselves, or log back off, before it is announced that they
  have logged on.  This is for privacy purposes.  Wizards are told of player
  logins immediately, however.  This is for possible security reasons.
  
  Properties:
  
    _prefs/con_announce?    If set "yes" on yourself, then you will be told
                              when people log in.
 
    _prefs/con_announce_list    Contains a list of space seperated case-
                                 insensitive names.  When one of these people
                                 connects or disconnects, you will see a
                                 message telling you so.  Wizards may also
                                 use dbrefs in the form #1234 in place of
                                 names.  If this property is not set, then
                                 you will not be informed when anyone logs
                                 in or out.  Wizards, however, are informed
                                 of ALL logins/logouts when this prop is not
                                 set.
 
    _prefs/con_announce_once    Same as _prefs/con_announce_list, except that
                                 as soon as it announces a player connecting/
                                 disconnecting, it clears their name from the
                                 property.  DBrefs are not usable in this prop.
 
    _prefs/con_announce_fmt    Contains a format string for the announcement
                                message you see when another player connects/
                                disconnects.  %n subs to the player's name;
                                %t to the current time; %v to the verb
                                "connected", "disconnected", or "reconnected".
                                %l subs to the players location for a wizard's
                                format string.  The default format string is:
                                "Somewhere on the Muck, %n has %v."
)
  
  
$include $lib/strings
$include $lib/reflist
$include $lib/edit
  
$def grace_time 30 (seconds)
  
$def announce_prop      "_prefs/con_announce?"
$def announce_fmt_prop  "_prefs/con_announce_fmt"
$def announce_list_prop "_prefs/con_announce_list"
$def announce_once_prop "_prefs/con_announce_once"
$def announce_hide_prop "_prefs/@con_announce_hide"
  
$def logintime_prop     "@/AnnLITime"
  
  
( $def if you want players to see all connects/
  disconnects when no _prefs/con_announce_list is set.)
$undef MORTAL_SEE_ALL
  
  
lvar discon?
  
: yes-prop? (d s -- i)
    getpropstr "yes" stringcmp not
;
  
: setproperty (d s s -- )
    dup if
        0 addprop
    else
        pop remove_prop
    then
;
  
: find-action ( -- s)
    discon? @ if "disconnected" exit then
    me @ awake? 1 = if
        "connected"
    else
        "reconnected"
    then
;
  
  
: can_see_all? (dListowner -- seeall?)
    dup announce_list_prop getpropstr
    if pop 0 exit then
$ifdef MORTAL_SEE_ALL
    pop 1
$else
    "wizard" flag?
$endif
;
  
  
: remove_listitem (sList sItem -- sList)
    " " swap strip strcat " " strcat
    " " rot strip strcat " " strcat swap
    over tolower swap tolower instr
    dup if
        (sList iPos)
        strcut
        (sPrelist sPostlist)
        dup " " instr dup if
            strcut swap pop
        else pop
        then
        (sPrelist sPostlist)
        strcat
    else
        pop
    then
    strip .sms
;
  
  
: in_strlist? (sName sList -- bool)
    dup not if pop pop 0 exit then
    (sName sList)
    " " swap strcat " " strcat tolower
    swap " " swap strcat " " strcat
    (sList sName)
    tolower instr
;
  
  
(returns 0 if not in list, 1 if in list)
: in_permlist? (dListowner sName -- inlist?)
    swap announce_list_prop getpropstr
    in_strlist?
;
  
: ref_in_permlist? (dListowner dWho -- inlist?)
    over "wizard" flag? if
        "#" swap int intostr
        strcat in_permlist?
    else pop pop 0
    then
;
  
  
: name_or_ref_in_hidelist? (dListowner dWho -- inlist?)
    dup "wizard" flag? if pop pop 0 exit then  (nowhere to hide from wizards!)
    swap announce_hide_prop getpropstr
    (dWho sList)
    over name over in_strlist?
    (dWho sList bInlist?)
    "#all" 3 pick in_strlist? or
    (dWho sList bInlist?)
    "#" 4 rotate int intostr strcat rot in_strlist? or
    (bInlists?)
;
  
  
(returns 0 if not in list, 1 if in list)
: in_templist? (dListowner sName -- inlist?)
    swap announce_once_prop getpropstr
    in_strlist?
;
  
: refconv (s -- s)   (name to refstr)
    me @ "wizard" flag? not if me @ 1 addpennies then
    dup .pmatch dup player?
    if name swap then pop
;
  
: convref (s -- s)   (refstr to name)
    dup "#" 1 strncmp if exit then
    dup "#all" stringcmp not if exit then
    dup match dup player? if
        name swap pop
    else pop
    then
;
  
  
: ref_or_name_in_strlist? (s sList -- bool)
    "ss" checkargs
    over "#" 1 strncmp not if
        over over in_strlist?
        rot convref rot in_strlist? or
    else
        over over in_strlist?
        rot refconv rot in_strlist? or
    then
    "i" checkargs
;
  
  
: is-oncer? (d -- i)
    dup announce_once_prop getpropstr
    strip dup tolower " " swap over strcat strcat
    me @ name tolower " " swap over strcat strcat
    instr dup not if pop pop pop 0 exit then
    1 - strcut me @ name strlen 1 + strcut swap pop strcat
    .sms strip announce_once_prop swap setproperty 1
;
  
: get-time ( -- s)
    "%X" systime timefmt
;
  
: do_format_subs (d -- s)
    dup announce_fmt_prop getpropstr  (get the announce format)
    dup not if pop "Somewhere on the muck, %n has %v." then
    "%n" "%N" subst me @ name "%n" subst
    "%v" "%V" subst find-action "%v" subst
    "%t" "%T" subst get-time "%t" subst
    "%l" "%L" subst
    over "wizard" flag? if  (only wizards can see location.)
        me @ location name "%l" subst
    else
        "somewhere" "%l" subst
    then
    swap pop
;
  
  
: announce (dowizzes? -- )
    preempt
    concount begin
        dup while
        dup condbref
        3 pick over "wizard" flag? if not then
        if pop 1 - continue then (skip conn. if player !matches wiz cond.)
        dup location me @ location dbcmp not (only ann. if !same room...)
        me @ location "dark" flag? or if  (...or room is set dark.)
            dup announce_prop yes-prop? if  (only announce to people listening)
                me @ over name_or_ref_in_hidelist? not if
                    dup me @ name in_permlist?
                    over me @ ref_in_permlist? or
                    over can_see_all? or
                    over is-oncer? or if  (if in list, announce)
                        do_format_subs
                        over swap connotify
                    else pop
                    then  (if in announce list)
                else pop
                then
            else pop
            then  (if listening)
        else pop
        then  (if not in same location)
        1 -
    repeat
    pop
    background
;
  
: sort-stringwords (s -- s)
    strip .sms " " explode
    0 1 EDITsort
    1 - swap convref
    begin
        over while swap 1 - swap rot
        convref dup not if pop continue then
        " " strcat swap strcat
    repeat
    swap pop strip
;
  
lvar shownheader
: list-awake-watched
    0 shownheader !
    me @ announce_list_prop getpropstr
    .sms strip " " explode
    "" begin
        over while
        swap 1 - swap rot
        dup "#" 1 strncmp
        (**** Consider letting non-wizards list by dbref ****)
        me @ "wizard" flag? not or
        if
            me @ "wizard" flag? not if me @ 1 addpennies then
            dup .pmatch
        else
            1 strcut swap pop atoi
            dup if
                dbref dup player? if
                    dup name swap
                else
                    pop "Droogy" #-1
                then
            else
                pop "Droogy" #-1
            then
        then
        dup not if pop pop continue then
        dup me @ name_or_ref_in_hidelist? if pop pop continue then
        awake? not if pop continue then
        "                  " strcat 18 strcut pop
        shownheader @ not if
            "Players online for whom you are watching:" .tell
            1 shownheader !
        then
        strcat dup strlen 60 > if .tell "" then
    repeat
    .tell pop
    shownheader @ not if
        "No one that you are watching for is online." .tell
    else
        "Done." .tell
    then
;
  
  
lvar edlist
lvar listname
lvar userefs
: edit-list (sList sPlayers listname refs? -- sListout)
    userefs ! listname !
    swap strip .sms edlist !
    strip .sms " " explode
    begin
        dup while 1 - swap
        dup "!" 1 strncmp not if
            (a request to remove name from list)
            1 strcut swap pop
            dup not if pop continue then
            dup dup "#" 1 strncmp if "*" swap strcat then
            match dup player? if
                userefs @ if
                    "#" over int intostr strcat
                else
                    dup name
                then
                rot pop swap pop
            else pop
            then
            dup edlist @ ref_or_name_in_strlist? not if
                (Name isn't in list anyways)
                convref " wasn't in your " strcat
                listname @ strcat " list."
                strcat .tell
            else
                (name in list.  Remove)
                edlist @ over remove_listitem edlist !
                edlist @ over convref remove_listitem edlist !
                "Removing " swap convref strcat
                " from your " strcat listname @ strcat
                " list." strcat .tell
            then
        else
            (a request to add name to list)
            dup not if pop continue then
            dup dup "#" 1 strncmp if "*" swap strcat then
            match dup player? if
                userefs @ if
                    "#" over int intostr strcat
                else
                    dup name
                then
                rot pop swap
                dup awake? if
                    name " is currently online."
                    strcat .tell
                else pop
                then
            else pop
            then
            dup edlist @ ref_or_name_in_strlist? if
                (Name is already in list)
                convref " is already in your "
                strcat listname @ strcat "list."
                strcat .tell
            else
                (add name to list)
                edlist @ " " strcat over strcat
                .sms strip edlist !
                "Adding " swap convref strcat
                " to your " strcat listname @ strcat
                " list." strcat .tell
            then
        then
    repeat pop
    edlist @
    .sms strip sort-stringwords
;
  
  
: do-help-list (sx...s1 x -- )
    begin
        dup while 1 -
        dup 2 + rotate
        trigger @ name ";" .split pop
        " " strcat swap strcat .tell
    repeat pop
;
  
: do-help
    "-- warns you when given players log in or out."
    1 do-help-list
"---------------------------------------------------------------------------"
.tell
    "#on               Turns on login/logoff watching."
    "#off              Turns off login/logoff watching."
    "#help             Gives this help screen."
    "#list             Lists all players being watched for."
    "                  Lists all watched players currently online."
    "<player>          Adds the given player to your watch list."
    "!<player>         Removes the given player from your watch list."
    "                  Lists the players temporarily being watched for."
    "#temp <plyr>      Adds the given player to the temporary watch list."
    "#temp !<plyr>     Removes the given player from the temp. watch list."
    "#hidefrom <plyr>  Prevents <plyr> from being told of your logins."
    "#hidefrom !<plyr> Lets <plyr> be told of your logins again."
    "#hidefrom #all    Lets no one be informed of your logins/logouts."
    "#hidefrom !#all   Lets people see your logins/logouts again."
    "#hidefrom         Lists who you are hiding from."
    15 do-help-list
"For more detailed information on this program, type '@view $con/announce'"
.tell
;
  
: main
    command @ "Queued Event." stringcmp not if
        "disconnect" stringcmp not discon? !
        1 announce (announce to wizzes immediately)
        discon? @ if
            me @ logintime_prop getpropval systime swap -
            me @ logintime_prop remove_prop
            grace_time < if exit then (If disconn. within grace time, ignore)
        else
            me @ logintime_prop "" systime addprop
  
            fork not if
                me @ announce_list_prop getpropstr if
                    preempt list-awake-watched
                then
                exit
            then
  
            grace_time sleep
            me @ awake? not if exit then (if not logged on, ignore)
            me @ logintime_prop getpropval
            dup not if pop exit then (apparently disconnected during grace time)
            systime swap - grace_time < if exit then (reconn in grace.  ignore)
        then
        0 announce (announce to all others)
    else (run from action)  (Run away!  Aiiiiiigh!)
        strip dup "#" 1 strncmp not if
            " " .split swap
            dup "#list" stringcmp not if
                pop pop me @ announce_list_prop getpropstr
                .sms strip sort-stringwords
                dup not if pop "*no one*" then
                "Currently watching for: " swap strcat
                .tell exit
            then
            dup "#help" stringcmp not if
                pop pop do-help exit
            then
            dup "#off" stringcmp not if
                pop me @ announce_prop remove_prop
                "Watch turned off." .tell exit
            then
            dup "#on" stringcmp not if
                pop me @ announce_prop "yes" 0 addprop
                "Watch turned on." .tell exit
            then
            dup "#hidefrom" stringcmp not
            over "#hide" stringcmp not or if
                pop dup if
                    me @ announce_hide_prop getpropstr
                    swap "hidefrom" 1 edit-list
                    me @ announce_hide_prop rot setproperty
                    exit
                else
                    pop me @ announce_hide_prop getpropstr
                    .sms strip sort-stringwords
                    dup not if pop "*no one*" then
                    "#all" over in_strlist? if pop "*everyone*" then
                    "Hiding from: " swap strcat
                    .tell exit
                then
            then
            dup "#temp" stringcmp not if
                pop dup if
                    me @ announce_once_prop getpropstr
                    swap "temporary watchfor" 0 edit-list
                    me @ announce_once_prop rot setproperty
                    exit
                else
                    pop me @ announce_once_prop getpropstr
                    .sms strip sort-stringwords
                    dup not if pop "*no one*" then
                    "Temporarily watching for: " swap strcat
                    .tell exit
                then
            then
        then
        dup if
            me @ announce_list_prop getpropstr
            swap "permanent watchfor" 0 edit-list
            me @ announce_list_prop rot setproperty
        else
            pop list-awake-watched
            exit
        then
    then
;
(Remember, the Weasel Patrol is on the job!)
.
c
q
@register con-announce=con/announce
@register #prop #0:_connect con-announce=announce
@register #prop #0:_disconnect con-announce=announce
@set con-announce=L
@set con-announce=W
@set con-announce=_docs:@list $con/announce=1-42
(@action watchfor;wf=me=tmp/exit1)
(@link $tmp/exit1=con-announce)
(@desc $tmp/exit1=@$con/announce #help)
