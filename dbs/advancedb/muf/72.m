(
Spoof version 2.62
Originally by Lynx, last revised February 25, 1998 by Nightwind
 
Type 'spoof' for usage instruction.
Type 'spoof #install' for instructions on installing a spoof action.
Type 'spoof #examples' for instructions on using and installing spoofs.
 
To spoof something beginning with #, add "#" in front of it, i.e. 'spoof ##'
 
Ported with permission by Natasha@HLM 18 June 2002
)
$include $lib/ignore  ( Honor say #ignore Natasha@HLM 20 January 2003 )
$define STORE_LIMIT
   loc @ "/./spoof/store_limit" getprop dup string? if atoi then
   dup not if pop 10 then
   dup 20 > if pop 20 then
$enddef
(------------------------------------------------------------------------)
: process-entry  ( s -- )
    dup "=" instr 1 - strcut 1 strcut swap pop
    swap atoi dbref
    dup player? not over ok? and if owner then
    dup player? if name else pop "<Unknown>" then
    "                    " strcat 17 strcut pop ": " strcat swap
    strcat "#recent: " swap strcat
    74 ansi_strcut pop  (limit to 74 chars in recall, avoid spam)  ( changed strcut to ansi_strcut Natasha@HLM 7 February 2003 )
    me @ swap notify
;
: do-display-recent  ( -- )
    loc @ "/./spoof/disallow_recent?" getprop if
        me @ "#recent command not allowed in this room." notify
        exit
    then
    loc @ "/./spoof/ptr" getprop dup not if
        pop "No spoofs have been made in this room."
        me @ swap notify exit
    then
    dup begin
        loc @ "/./spoof/" 3 pick intostr strcat getprop
        dup not if pop break then
        process-entry
        1 - dup 0 <= if pop STORE_LIMIT then
        over over =
    until
    pop pop
    me @ "#recent:  Done." notify
;
: log-this-message  ( s -- )
    (stores in format 'dbref=full spoof message')
    me @ int intostr "=" strcat swap strcat
    loc @ "/./spoof/ptr" getprop dup if
        1 + dup STORE_LIMIT > if pop 1 then
    else pop 1
    then
    loc @ "/./spoof/" 3 pick intostr strcat 4 rotate setprop
    loc @ "/./spoof/ptr" rot setprop
;
: do-convert-history  ( history ptr -- )
    STORE_LIMIT   ( changeto=X cur-ptr store-limit=N -- )
    loc @ "/./spoof/" 4 pick 1 + intostr strcat getprop not
    3 pick 3 pick < and if
        (#recent has not scrolled full yet)
        3 pick 3 pick >= if
            (x >= ptr, so just set newmax := x)
            pop pop loc @ "/./spoof/store_limit" rot setprop
        else
            (x < ptr)
            ( for i:=1 to x do {copy a[i+ptr-x] to a[i]} )
            1 begin dup 5 pick <= while
                dup 4 pick + 5 pick - loc @ "/./spoof/" rot intostr
                strcat getprop
                loc @ "/./spoof/" 4 pick intostr strcat rot setprop
                1 +
            repeat pop pop   (x ptr --)
            ( for i:=x+1 to ptr do {erase a[i]} )
            over 1 + begin dup 3 pick <= while
                loc @ "/./spoof/" 3 pick intostr strcat remove_prop
                1 +
            repeat pop pop   (x --)
            loc @ "/./spoof/ptr" 3 pick setprop
            loc @ "/./spoof/store_limit" rot setprop
        then
    else
        (#recent has scrolled around already, so here's our algoritm)
        3 pick over < if 3 pick else dup then  (x ptr n min[x,n]=j --)
        (extract last j items to stack)
        rot over begin dup while
            loc @ "/./spoof/" 4 pick intostr strcat getprop
            -6 rotate (stuff at bottom of stack)
            1 -
            swap 1 - dup 0 <= if pop STORE_LIMIT then swap
        repeat pop pop
        (now erase all old props)          ({rng} x n j -- )
        swap begin dup while
            loc @ "/./spoof/" 3 pick intostr strcat remove_prop
            1 -
        repeat pop
        (now stick things back into props)          ({rng} x j -- )
        1 begin dup 3 pick <= while
            loc @ "/./spoof/" 3 pick intostr strcat 6 rotate setprop
            1 +
        repeat pop
        (set new values)          (x j --)
        loc @ "/./spoof/ptr" rot setprop
        loc @ "/./spoof/store_limit" rot setprop
    then
;
: do-set-history  ( argumentstr -- )
    loc @ owner me @ dbcmp
    me @ "Wizard" flag? or not if
        pop me @ "You don't control this room!" notify exit
    then
    strip dup number? not if
        pop me @ "That is not a number." notify exit
    then
    atoi dup 1 < over 20 > or if
        pop me @ "Number is out of range, must be a value in 1-20." notify exit
    then
    dup loc @ "/./spoof/ptr" getprop dup if
        do-convert-history
    else
        pop pop loc @ "/./spoof/store_limit" 3 pick setprop
    then
    "#history set to " swap intostr strcat " entries." strcat
    me @ swap notify
;
: notify_and_store  ( d1 d2 s -- )
    dup log-this-message
    1 swap "saypose" notify_exclude_ignorers (notify_exclude}  Honor say #ignore Natasha@HLM 20 January 2003 )
;
: convert-name ( s -- "_trigger" + s )
    trigger @ intostr swap strcat
;
: get-prop ( s -- s' )
    trigger @ over getpropstr dup if                          ( name prop )
        over convert-name over prog rot rot ( name prop prog name' prop )
        0 addprop                                             ( name prop )
        trigger @ owner me @ dbcmp if
            over trigger @ swap remove_prop
        then
    then                                                      ( name prop )
    dup not if
        pop prog swap convert-name getpropstr
    then ( prop )
;
: put-prop ( prop name -- )
    trigger @ over getpropstr if
        trigger @ over remove_prop
    then
    convert-name swap prog rot rot                    ( prog name' prop )
    dup if
        0 addprop
    else
        pop remove_prop
    then
;
: clean-list ( a[n] ... a[1] n i -- a[n] ... a[2] n-1 i )
    over if
        rot pop
        swap 1 - swap
        clean-list
    else
        swap pop
    then
;
: spoof_ok-iter ( a[n] ... a[1] n -- a[n] ... a[2] n-1 or
                                     a[n] ... a[1] n 0 or
                                     a[n] ... a[1] n 1 )
    over me @ owner name stringcmp not
    3 pick atoi dbref me @ dbcmp or if
        0
    else
        dup 1 = if
            1
        else
            1 - swap pop
            spoof_ok-iter
        then
    then
;
: env_spoof_ok? ( db -- i or db' )
    dup ok? if
        dup "_spoof_lock" getpropstr dup if
            " " explode spoof_ok-iter
            clean-list
            dup not if
                swap pop
            else
                pop location env_spoof_ok?
            then
        else
            pop location env_spoof_ok?
        then
    else
        pop 1
    then
;
: spoof_ok? ( db -- i )
    "_spoof_lock" get-prop dup if
        " " explode spoof_ok-iter
        clean-list
        dup not if
            exit
        then
    then
    pop prog "_spoof_lock" getpropstr dup if
        " " explode spoof_ok-iter
        clean-list
        dup not if
            exit
        then
    then
    pop loc @ env_spoof_ok?
;
: spoof-tattletale-msg ( d -- )
    "# spoofed by " me @ name strcat " #" strcat
    notify
;
: match-and-skip-empty ( s -- )
    dup if
        match
    else
        pop #-1
    then
;
: spoof-tattletale-iter
    dup 0 > if
        1 - swap
        dup atoi dbref
        dup ok? if
            dup player? if
                dup location loc @ dbcmp if
                    spoof-tattletale-msg
                else
                    pop
                then
            else
                pop
            then
        else
            pop
        then
        dup match-and-skip-empty dup ok? if
            dup name rot stringcmp not if
                dup player? if
                    spoof-tattletale-msg
                else
                    pop
                then
            else
                pop
            then
        else
            pop pop
        then
        spoof-tattletale-iter
    else
        pop
    then
;
: env-spoof-tattletale ( db -- db' )
    dup ok? if
        dup "_spoof_tattletale" getpropstr dup if
            " " explode spoof-tattletale-iter
        else
            pop
        then
        location env-spoof-tattletale
    then
;
: spoof-tattletale
    "_spoof_tattletale" get-prop dup if
        " " explode spoof-tattletale-iter
    else
        pop
    then
    loc @ env-spoof-tattletale pop
    prog "_spoof_tattletale" getpropstr dup if
        " " explode spoof-tattletale-iter
    else
        pop
    then
;
: first-word ( s -- s' )
    dup " " instr dup if
        1 - strcut pop
    else
        pop
    then
;
: clip-first-word ( s -- s' )
    dup " " instr dup if
        strcut swap pop
    else
        pop pop ""
    then
;
: do-pmatch
        .pmatch
;
: matches-player-name ( s -- d )
    first-word
    dup me @ name stringcmp not if
        pop 0
    else
        do-pmatch ok?
    then
;
: free-spoof
    loc @ swap #-1 swap notify_and_store
    spoof-tattletale
;
: restricted-spoof
    dup matches-player-name if
        me @ "Spoofs cannot begin with player names." notify
    else
        loc @ swap #-1 swap notify_and_store
        spoof-tattletale
    then
;
: default-spoof
    dup matches-player-name ok? if
        me @ "Spoofs cannot begin with player names." notify
    else
        "( " swap strcat " )" strcat loc @ swap #-1 swap notify_and_store
        spoof-tattletale
    then
;
: puppet-ok?-iter ( d -- 1 or 0 )
    dup
    "_puppet_ok?" getpropstr dup if
        1 strcut pop "y" stringcmp not if
            pop 1
        else
            pop 0
        then
    else
        pop location dup ok? if
            puppet-ok?-iter
        else
            pop 0
        then
    then
;
: puppet-ok?
    loc @ puppet-ok?-iter
;
: puppet-spoof
    puppet-ok? if
        trigger @ location name " " strcat swap strcat
        dup matches-player-name if
            me @ "Spoofs cannot begin with player names." notify
            pop exit
        then
        loc @ swap #-1 swap notify_and_store
        spoof-tattletale
    else
        me @ "This area does not allow puppet spoofs." notify
    then
;
: in-string? ( s s1 -- 1 or 0 )
    dup strlen                                ( s s1 len )
    3 pick swap strcut pop                    ( s s1 sl )
    over stringcmp not if                     ( s s1 )
        pop pop 1
    else
        swap 1 strcut swap pop swap           ( s' s1 )
        over if
            in-string?
        else
            pop pop 0
        then
    then
;
: action-spoof-ok?
    dup me @ name in-string? if
        pop 1 exit
    then
    "%n" in-string? if
        1 exit
    then
    0
;
: action-spoof
    dup action-spoof-ok? if
        "**RESERVED-NAME**" "%%n" subst
        me @ name "%n" subst
        "%n" "**RESERVED-NAME**" subst
        me @ swap pronoun_sub
        free-spoof
    else
        default-spoof
    then
;
: format-spoof
    "**RESERVED-MESSAGE**" "%m" subst
    me @ name "%%n" subst
    me @ swap pronoun_sub
    "%m" "**RESERVED-MESSAGE**" subst
    swap "%m" subst
    loc @ swap #-1 swap notify_and_store
    spoof-tattletale
;
: add-ok?
    me @ trigger @ owner dbcmp if
        1
    else
        me @ "Wizard" flag? if
            1
        else
            me @ #21 dbcmp
        then
    then
;
: bracket-spaces ( s -- s' )
    " " swap strcat " " strcat
;
: trim-spaces ( s -- s' )
    1 strcut swap pop
    dup if
        dup strlen 1 - dup if
            strcut pop
        else
            pop pop ""
        then
    else
        pop ""
    then
;
: add-tattletale
    add-ok? not if
        me @ "You don't own this spoof." notify
        exit
    then
    dup "!" 1 strncmp not if
        1 strcut swap pop
        do-pmatch
        dup ok? if
            dup name "Removing " swap strcat " from tattletale list." strcat
            me @ swap notify
            "_spoof_tattletale" get-prop bracket-spaces
            over intostr bracket-spaces " " swap subst
            swap name bracket-spaces " " swap subst
            trim-spaces
            "_spoof_tattletale" put-prop
            exit
        else
            pop
            me @ "No such person to remove from the tattletale list!" notify
            exit
        then
    then
    do-pmatch
    dup ok? if
        dup name "Added " swap strcat " to tattletale list." strcat
        me @ swap notify
        intostr
        "_spoof_tattletale" get-prop
        dup if
            " " strcat
        then
        swap strcat
        "_spoof_tattletale" put-prop
    else
        pop
        me @ "No such person to add to the tattletale list." notify
    then
;
: add-lock
    add-ok? not if
        me @ "You don't own this spoof." notify
        exit
    then
    dup "!" 1 strncmp not if
        1 strcut swap pop
        do-pmatch
        dup ok? if
            dup name "Removing " swap strcat " from lock list." strcat
            me @ swap notify
            "_spoof_lock" get-prop bracket-spaces
            over intostr bracket-spaces " " swap subst
            swap name bracket-spaces " " swap subst
            trim-spaces
            "_spoof_lock" put-prop
            exit
        else
            pop
            me @ "No such person to remove from the lock list." notify
            exit
        then
    then
    do-pmatch
    dup ok? if
        dup name "Added " swap strcat " to lock list." strcat
        me @ swap notify
        intostr
        "_spoof_lock" get-prop
        dup if
            " " strcat
        then
        swap strcat
        "_spoof_lock" put-prop
    else
        pop
        me @ "No such person to add to the lock list!" notify
    then
;
: do-set-room-nosee  ( -- )
    loc @ owner me @ dbcmp
    me @ "Wizard" flag? or not if
        me @ "You don't own this room." notify
        exit
    then
    loc @ "/./spoof/disallow_recent?" "yes" setprop
    me @ "The #recent command will no longer function in this room." notify
;
: do-set-room-see  ( -- )
    loc @ owner me @ dbcmp
    me @ "Wizard" flag? or not if
        me @ "You don't own this room." notify
        exit
    then
    loc @ "/./spoof/disallow_recent?" remove_prop
    me @ "The #recent command will now function in this room." notify
;
: spoof-free
    add-ok? not if
        me @ "You don't own this spoof." notify
        exit
    then
    trigger @ "_spoof" "free" 0 addprop
    me @ "Spoof set free (messages can be any format)" notify
;
: spoof-restricted
    add-ok? not if
        me @ "You don't own this spoof." notify
        exit
    then
    trigger @ "_spoof" "restricted" 0 addprop
    me @ "Spoof set restricted (messages will be in parentheses and name-checked)" notify
;
: spoof-puppet
    add-ok? not if
        me @ "You don't own this spoof." notify
        exit
    then
    trigger @ "_spoof" "puppet" 0 addprop
    me @ "Spoof set puppet (messages will be prepended by object name)" notify
;
: spoof-action
    add-ok? not if
        me @ "You don't own this spoof." notify
        exit
    then
    trigger @ "_spoof" "action" 0 addprop
    me @ "Spoof set action (messages must have name or %n in them)" notify
;
: spoof-message
    add-ok? not if
        me @ "You don't own this spoof." notify
        exit
    then
    dup if
        dup "Spoof set to format: " swap strcat me @ swap notify
        trigger @ swap "_spoof" swap 0 addprop
    else
        me @ "Spoof format set to default." notify
        pop trigger @ "_spoof" remove_prop
    then
;
: spoof-feep ( s -- )
    dup if
        dup "Spoof #feep set to: " swap strcat me @ swap notify
        "_spoof_feep" put-prop
    else
        pop "_spoof_feep" get-prop
        dup if
            me @ swap "Spoof #feep: " swap strcat notify
        else
            pop me @ "No spoof #feep message has been set yet." notify
        then
    then
;
: check-spoof-iter ( n1 n2 ... nn i -- n1 ... n<n-1> i' )
    1 -
    swap dup atoi dbref
    dup ok? if
        dup player? if
            name swap pop
        else
            pop
        then
    else
        pop
    then
    "    " swap strcat
    me @ swap notify
    dup if
        check-spoof-iter
    then
;
: env-check-spoof ( db -- db' )
    dup ok? if
        dup "_spoof_tattletale" getpropstr dup if
            "Tattletale list on " 3 pick name strcat me @ swap notify
            " " explode check-spoof-iter pop
        else
            pop
        then
        dup "_spoof_lock" getpropstr dup if
            "Lock list on " 3 pick name strcat me @ swap notify
            " " explode check-spoof-iter pop
        else
            pop
        then
        location env-check-spoof
    then
;
: check-spoof
    "_spoof_tattletale" get-prop dup if
        me @ "Tattletale list on this spoof:" notify
        " " explode check-spoof-iter pop
    else
        pop me @ "No one on this spoof's tattletale list." notify
    then
    "_spoof_lock" get-prop dup if
        me @ "This spoof's lock list:" notify
        " " explode check-spoof-iter pop
    else
        pop me @ "No one on this spoof's lock list." notify
    then
    loc @ env-check-spoof pop
    prog "_spoof_tattletale" getpropstr dup if
        me @ "Global tattletale list:" notify
        " " explode check-spoof-iter pop
    else
        pop
    then
    prog "_spoof_lock" getpropstr dup if
        me @ "Global lock list:" notify
        " " explode check-spoof-iter pop
    else
        pop
    then
;
: spoof-help-1
    me @ "Spoof v2.62 usage: spoof <message>" notify
    me @ "                  anonymously show message to all in the room." notify
    me @ " " notify
    me @ "Note: to spoof a message beginning with #, you must double the #." notify
    me @ "      spoof ## This is a message beginning with one #." notify
    me @ " " notify
    me @ "Type 'spoof #install' for help on installing spoof exits in rooms." notify
    me @ "Type 'spoof #examples' for examples of how you might install spoofs." notify
    me @ "Type 'spoof #commands' for a list of additional #keywords." notify
    me @ "Type 'spoof #changes' to see new changes in spoof." notify
;
: spoof-changes
    me @ "Spoof v2.62 changes:" notify
    me @ "Added spoof #history." notify
    me @ " " notify
    me @ "Spoof v2.61 changes:" notify
    me @ "Added spoof #nosee and spoof #see (see spoof #commands for help)." notify
    me @ "Fixed the bug with using spoof #check (thanks Points!)." notify
    me @ " " notify
    me @ "Spoof v2.6 changes:" notify
    me @ "spoof #recent added, tells most recent spoofs in a room and who did them." notify
    me @ " " notify
    me @ "Spoof v2.5 changes:" notify
    me @ "Owners may now set _spoof_tattletale:(list of names and numbers) on rooms" notify
    me @ "to allow them to see who did *all* spoofs including personal spoofs or " notify
    me @ "puppets.  The same applies to _spoof_lock.  However this must be done" notify
    me @ "manually." notify
    me @ " " notify
    me @ "spoof #check will now work for any spoof and will tell you what tattletales" notify
    me @ "and locks there are on the spoof, the room or its environments, and the" notify
    me @ "global spoof tattletale and locks." notify
    me @ " " notify
    me @ "Also, set an environment _puppet_ok?:no if you do not want puppets to be" notify
    me @ "used in it." notify
;
: spoof-commands
    me @ "Spoof v2.6x commands:" notify
    me @ " " notify
    me @ "Type 'spoof #free' to let people spoof any message." notify
    me @ "Type 'spoof #restricted' to prevent people from spoofing messages starting" notify
    me @ "    with player names." notify
    me @ "Type 'spoof #action' to allow people to make free-style spoofs." notify
    me @ "Type 'spoof #puppet' to make an NPC object." notify
    me @ "Type 'spoof #message <format> to set a special format with %m for the message." notify
    me @ "Type 'spoof #tattletale <name>' to add someone to the spoof's tattletale." notify
    me @ "Type 'spoof #lock <name>' to add someone to the spoof's lock list." notify
    me @ "Type 'spoof #recent' to see who did the 10 most recent spoofs in the room." notify
    me @ "Type 'spoof #history N' where N=# of lines (1-20) to keep for the #recent command (default 10)." notify
    me @ "Type 'spoof #nosee' to not allow #recent to work in the room." notify
    me @ "Type 'spoof #see' to let #recent work in the room again." notify
    me @ "Type 'spoof #check' to see who is on the spoof's tattletale and lock lists." notify
    me @ "  The above commands (except #recent) all require you to own the spoof." notify
    me @ "  #tattletale and #lock let you type !name to remove a name." notify
    me @ "Type 'spoof #feep' to...  Well, it's a feep.  Find out for yourself. *'gryn*" notify
    me @ " " notify
;
: spoof-help-2
    me @ "Spoof v2.4 installation:" notify
    me @ " " notify
    me @ "Exits may be linked to the spoof program ($spoof) from a player," notify
    me @ "object, or room, by doing this:" notify
    me @ "    @action spoof = <object>" notify
    me @ "    @link spoof = $spoof" notify
    me @ " " notify
    me @ "Unless a spoof action is on a room, it is always restricted." notify
    me @ "A restricted spoof can never begin with the name of a player and" notify
    me @ "is enclosed in parentheses." notify
    me @ " " notify
    me @ "A spoof exit on a room can be set as follows:" notify
    me @ "    spoof #free" notify
    me @ "    spoof #restricted" notify
    me @ "    spoof #message (some message with %m)" notify
    me @ " " notify
    me @ "A free spoof lets the players spoof whatever they like, whether or" notify
    me @ "not it is the name of a player.  A restricted spoof is like a free" notify
    me @ "spoof except that it will not let people spoof other people's names." notify
    me @ "And a message spoof, which is any string with %m in it, will substitute" notify
    me @ "the spoofed message for %m." notify
    me @ " " notify
    me @ "Action Spoof:" notify
    me @ "    Type 'spoof #action' to format your spoof to allow free-style actions." notify
    me @ "The spoof must be on a room, and you must own the spoof to set it.  A" notify
    me @ "free-style action will let you have your name in it anywhere, or %n," notify
    me @ "and will perform pronoun substitutions.  (use %%n for your %n property)" notify
    me @ " " notify
    me @ "Example: spoof In a flash of fur, Lynx disappears." notify
    me @ " " notify
    me @ "Puppet Spoof:" notify
    me @ "    Type 'spoof #puppet' to format your spoof to allow puppet actions." notify
    me @ "The spoof must be on an object, and you must own the spoof to set it." notify
    me @ "Also, the room or an environment of the room must be set _puppet_ok?:yes" notify
    me @ "to allow you to use puppets.  A puppet spoof will put the name of the object" notify
    me @ "in front of all spoofs, allowing you to simulate a puppet." notify
    me @ " " notify
    me @ "Example: @action spoof = fire-lizard" notify
    me @ "         @link spoof = $spoof" notify
    me @ "         spoof #puppet" notify
    me @ "         spoof chirps and looks around hopefully for food." notify
    me @ "         fire-lizard chirps and looks around hopefully for food." notify
    me @ " " notify
    me @ "You may set a spoof to reveal who spoofed to a few select people." notify
    me @ "They will be told only if they are in the same room." notify
    me @ "    spoof #tattletale player1" notify
    me @ "    (this will tell them even if they change names later)" notify
    me @ " " notify
    me @ "You may prevent certain people from using your spoof program." notify
    me @ "    spoof #lock player1" notify
    me @ "    (this will work even if they change names later)" notify
    me @ " " notify
    me @ "For examples, type 'spoof #examples'" notify
;
: spoof-help-3
    me @ "Spoof v2.3 examples:" notify
    me @ " " notify
    me @ "Example of a spoof message on yourself:" notify
    me @ "  > @action spoof = me" notify
    me @ "  > @link spoof = $spoof" notify
    me @ "  > spoof This is a test." notify
    me @ "  ( This is a test. )" notify
    me @ "  > spoof Wizard jumps up and down." notify
    me @ "  Error: spoofs cannot begin with player names!" notify
    me @ "  > @recycle spoof" notify
    me @ " " notify
    me @ "Example of spoof formats on a room:" notify
    me @ "  > @open spoof = $spoof" notify
    me @ "  > spoof This is the default restricted form." notify
    me @ "  ( This is the default restricted form." notify
    me @ "  > spoof #message On the stage, %m" notify
    me @ "  > spoof Romeo carols praises to Juliet's virtues." notify
    me @ "  On the stage, Romeo carols praises to Juliet's virtues." notify
    me @ "  > spoof #free" notify
    me @ "  > spoof Wizard toads you." notify
    me @ "  Wizard toads you." notify
    me @ " " notify
    me @ "Examples of spoof tattletale:" notify
    me @ "  > spoof #tattletale Lynx" notify
    me @ "  > @set spoof = _spoof:free" notify
    me @ "  > spoof You have been booted off the system!" notify
    me @ "  You have been booted off the system!" notify
    me @ "  # spoofed by Lynx #" notify
    me @ "Note that only Lynx sees the line in #'s." notify
    me @ " " notify
    me @ "Examples of spoof locks:" notify
    me @ "  > spoof #lock Lynx" notify
    me @ "  > spoof This is a test." notify
    me @ "  Error: you are not allowed to spoof here!" notify
;
: strip-left-spaces ( s -- s' )
    dup " " 1 strncmp not if
        1 strcut swap pop strip-left-spaces
    then
;
: spoof
    strip-left-spaces
    dup "##" 2 strncmp not if
        1 strcut swap pop
    else
        dup "#help" stringcmp not if
            pop spoof-help-1
            exit
        then
        dup "#changes" stringcmp not if
            pop spoof-changes
            exit
        then
        dup "#commands" stringcmp not if
            pop spoof-commands
            exit
        then
        dup "#recent" stringcmp not if
            pop do-display-recent
            exit
        then
        dup "#nosee" stringcmp not if
            pop do-set-room-nosee
            exit
        then
        dup first-word "#history" stringcmp not if
            clip-first-word do-set-history
            exit
        then
        dup "#see" stringcmp not if
            pop do-set-room-see
            exit
        then
        dup "#install" stringcmp not if
            pop spoof-help-2
            exit
        then
        dup "#examples" stringcmp not if
            pop spoof-help-3
            exit
        then
        dup "#free" stringcmp not if
            pop spoof-free
            exit
        then
        dup "#restricted" stringcmp not if
            pop spoof-restricted
            exit
        then
        dup "#action" stringcmp not if
            pop spoof-action
            exit
        then
        dup "#puppet" stringcmp not if
            pop spoof-puppet
            exit
        then
        dup first-word "#message" stringcmp not if
            clip-first-word spoof-message
            exit
        then
        dup first-word "#tattletale" stringcmp not if
            clip-first-word add-tattletale
            exit
        then
        dup first-word "#lock" stringcmp not if
            clip-first-word add-lock
            exit
        then
        dup "#check" stringcmp not if
            pop check-spoof
            exit
        then
        dup first-word "#feep" stringcmp not if
            clip-first-word spoof-feep
            exit
        then
        dup 1 strcut pop "#" strcmp not if
            me @ "Invalid #keyword." notify
            me @ "(If you want a # as the first character in your spoof, use ## (the first # will be eliminated))" notify
            exit
        then
    then
    spoof_ok? not if
        me @ "You are not allowed to spoof here." notify
        exit
    then
    dup not if
        pop " "
    then
    trigger @ location room? if
        trigger @ "_spoof" getpropstr dup if
            dup "free" stringcmp not if
                pop free-spoof
            else
                dup "restricted" stringcmp not if
                    pop restricted-spoof
                else
                    dup "action" stringcmp not if
                        pop action-spoof
                    else
                        dup "puppet" stringcmp not if
                            pop puppet-spoof
                        else
                            format-spoof
                        then
                    then
                then
             then
        else
            pop default-spoof
        then
    else
        trigger @ "_spoof" getpropstr dup if
            "puppet" stringcmp not if
                puppet-spoof
            else
                default-spoof
            then
        else
            pop default-spoof
        then
    then
;
: spoof-main ( s -- )
    dup not if pop spoof-help-1 exit then
    "\r" explode
    begin dup while            ( stringn ... string2 string1 n )
        swap spoof             ( stringn ... string2 n )
        dup int? not if
            pop
        then
        1 -                    ( stringn ... string2 n-1 )
    repeat pop
;
