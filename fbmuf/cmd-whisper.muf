@prog cmd-whisper
1 99999 d
1 i
( MUFwhisper    Copyright 4/30/91 by Revar <revar@belfry.com>    )
(                                                                )
( Code taken from MUFpage then modified for use as a whisper.    )
(                                                                )
( Program requires M3 setting for storage of the _whisp/ props   )
  
( CONFIGURATION )
  
( Restrict guests from whispering to anyone but a wizard )
( $def RESTRICTWIZ )
( Restrict guests from multi-whisper )
( $def RESTRICTMULT )
  
( Encrypt lastwhispere[d/dgroup/er/ers] props )
( $def ENCRYPTPROPS )
  
( Encryption key for lastwhispered props, where applicable )
( $def KEY "encryption key" )
  
( END OF CONFIGURATION )
  
  
$def VERSION "MUFwhisper v1.26 by Foxen, Nightwind, and Brenda"
$def UPDATED "Updated 06/22/00"
  
$def tell me @ swap notify
$def strip-leadspaces striplead
$def strip-trailspaces striptail
$def stripspaces strip
  
$ifdef ENCRYPTPROPS
( encryption stuff )
  
( better encryption. But slower. )
  
: asc (stringchar -- int)
    dup if
      " 1234567890-=!@#$%&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZXCVBNM<>?\"`~\\|^"
      swap instr 1 - exit
    then pop 0
;
  
: chr (int -- strchar)
    " 1234567890-=!@#$%&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZXCVBNM<>?\"`~\\|^"
    swap strcut 1 strcut pop swap pop
;
  
: cypher (key chars -- chars')
    1 strcut asc swap asc
    over 89 > over 89 > or if chr swap chr strcat swap pop exit then
    dup 10 / 10 *
    4 pick 10 + rot 10 % - 10 %
    rot dup 10 / 10 *
    5 rotate 10 + rot 10 % - 10 %
    4 rotate + chr -3 rotate + chr strcat
;
  
: crypt2 (key string -- string')
    swap 9 % 100 + "" rot
    begin dup while
        2 strcut 4 pick rot cypher
        rot swap strcat swap
    repeat pop swap pop
;
  
( Faster and better encryption )
  
: encrypt3 (key string -- string')
    swap intostr KEY over strcat strcat strencrypt
;
  
: decrypt3 (key string -- string')
    swap intostr KEY over strcat strcat strdecrypt
;
  
$endif
( Encryption dispatchers )
  
$ifdef ENCRYPTPROPS 
: pencrypt ( i s -- s )
    dup not if swap pop exit then
$ifdef __version>Muck2.2fb5.46
    encrypt3 "!" swap strcat
$else
    crypt2 "*" swap strcat
$endif
;
$else
    $def pencrypt swap pop
$endif
  
$ifdef ENCRYPTPROPS
: pdecrypt ( i s -- s )
    dup not if swap pop exit then
    dup 1 strcut pop "*!" swap instr
    dup 1 = if pop 1 strcut swap pop crypt2 exit then
$ifdef __version>Muck2.2fb5.46
    dup 2 = if pop 1 strcut swap pop decrypt3 exit then
$endif
    pop swap pop
;
$endif
  
( *** routines to get and set properties *** )
  
( *** BEGIN PERSONAL PROPS *** )
  
$def setpropstr setprop
  
: get-lastversion ( -- versionstr)
    me @ "_whisp/lastversion" getpropstr
;
  
: getprepend (playerdbref -- string)
     "_whisp/prepend" getpropstr dup not if
        me @ "_whisp/prepend" "W>>>" setprop
     else strip " " strcat
     then
;
  
: setprepend (string playerdbref -- )
     "_whisp/prepend" rot setpropstr
;
  
: set-lastversion (versionstr -- )
    me @ "_whisp/lastversion" rot setpropstr
;
  
: getlastwhisperr (playerdbref -- string)
     "_whisp/lastwhisperer" getpropstr
;
  
: setlastwhisperr (string playerdbref -- )
     "_whisp/lastwhisperer" rot setpropstr
;
  
: getlastwhisperd (playerdbref -- string)
     "_whisp/lastwhispered" getpropstr
;
  
: setlastwhisperd (string playerdbref -- )
     "_whisp/lastwhispered" rot setpropstr
;
  
: getlastwhisperdgroup (playerdbref -- string)
     "_whisp/lastwhisperedgroup" getpropstr
;
  
: setlastwhisperdgroup (string playerdbref -- )
     "_whisp/lastwhisperedgroup" rot setpropstr
;
  
: get-ignoremsg (playerdbref -- string)
    "_whisp/ignoremsg" getpropstr
;
  
: set-ignoremsg (playerstring dbref -- )
    "_whisp/ignoremsg" rot setpropstr
;
  
: set-whisper-on ( -- )
    me @ "_whisp/notusing?" remove_prop
;
  
: set-whisper-off ( -- )
    me @ "_whisp/notusing?" "yes" setpropstr
;
  
( *** END PERSONAL PROPS *** )
  
( *** END PROPS ON PROG *** )
  
( *** end of routines for getting and setting properties *** )
  
( misc simple routines )
  
: single-space (s -- s') (strips all multiple spaces down to a single space)
    begin
        dup "  " instr while
        " " "  " subst
    repeat
;
  
: comma-format (string -- formattedstring)
    stripspaces single-space
    ", " " " subst
    dup ", " rinstr dup if
        1 - strcut 2 strcut
        swap pop " and "
        swap strcat strcat
    else pop
    then
;
  
: stringmatch? (str cmpstr #charsmin-- bool)
    3 pick strlen > if pop pop 0 exit then swap stringpfx
;
  
( simple player matching )
  
: match-ambiguous-loop (name matched dbref -- dbref)
    dup not if pop swap pop exit then
    dup thing? over "zombie" flag? and
    over player? or if
        dup name 4 pick strlen strcut pop
        4 pick stringcmp not if
            over if
                swap pop #-2 swap
            else
                swap pop dup
            then
        then
    then
    next match-ambiguous-loop
;
  
: match-ambiguous (playername -- dbref)
    #-1 me @ location contents
    match-ambiguous-loop
;
  
: player-match? (playername -- [dbref] succ?)
    dup match
    dup int 0 < if
        pop match-ambiguous
    else
        dup player? not if
            pop match-ambiguous
        else
            swap pop
        then
    then
    dup #-2 dbcmp if pop -1 exit then
    dup not if pop 0 exit then
    dup location me @ location dbcmp not if
        me @ "Wizard" flag? not if
            pop 0 exit
        then
    then
    1
;
  
: do-pmatch  ( playername -- [dbref] succ? )
    .pmatch dup #-2 dbcmp if pop -1 exit then
    dup ok? if 1 else pop 0 then
;
  
: partial-match ( playername -- [dbref] succ? )
    dup strlen 3 < if pop 0 exit then
    part_pmatch dup ok? if 1 else pop 0 then
;
  
( misc stuff )
  
: havened?  (playerdbref -- haven?)
    "haven" flag?
;
  
: notusing?  (playerdbref -- notusing?)
    "_whisp/notusing?" getpropstr "yes" strcmp not
;
  
(
Sample for maybe later coding
: get-valid-pagees [players -- dbrefrange players']
$ifdef RESTRICTWIZ
    me @ name tolower "guest" 5 strncmp not if do-nonwiz then
$endif
;
)
  
$include $lib/reflist
$define IGNORELIST "_whisp/ignoring" $enddef
$define PRIORITYLIST "_whisp/prioritizing" $enddef
  
: being-ignored? (dbref -- bool)
    IGNORELIST me @ REF-inlist?
;
  
: prioritized?  (dbref -- bool)
    PRIORITYLIST me @ REF-inlist?
;
  
( help stuff )
  
: show-help-list
    dup not if pop exit then
    dup 1 + rotate me @ swap notify
    1 - show-help-list
;
  
: show-changes
"v1.00  4/30/91  Initial release."
"v1.03  3/31/92  Make whisperpose more intelligent.  Made it list who all"
"                 a whisper was directed to."
"v1.10  5/20/94  Made whisper work with zombies.  Optimized some code."
"v1.20  9/30/97  Added routines from MUFpage, added #ignore, #!ignore."
"                Added #on and #off to stop receiving whispers completely."
"                Added #priority and #!priority."
"v1.21 10/20/97  Made #priority and #ignore match remotely."
"v1.22 12/15/97  Excluded Guest* characters from #on, #off, #pri, #ign."
"v1.23 06/11/98  Fixed bug that allowed a trick to remotely whisper."
"v1.24 11/22/98  Added #prepend and #!prepend"
"v1.25 04/08/99  Added a default value and %W recognition to #prepend."
"v1.26 06/22/00  Added auto-cleaners of bad refs to #priority and #ignore."
13 show-help-list
;
  
: show-help
VERSION "   " strcat UPDATED strcat "   Page 1" strcat
"--------------------------------------------------------------------------"
"To whisper a message to another player:      'whisp <plyr> = <mesg>'"
"To do a pose style whisper to a player:      'whisp <player> = :<pose>'"
"To whisper to multiple people:               'whisp <plyr> <plyr> = <msg>'"
"To whisper again to the last players:        'whisp = <message>'"
"To ignore a player's whispers:               'whisp #ignore <plyr> <plyr>'"
"To unignore a player's whispers:             'whisp #!ignore <plyr> <plyr>'"
"To stop whispers from reaching you:          'whisp #off'"
"To allow whispers again:                     'whisp #on'"
"To start prepending using previous string:   'whisp #prepend'"
"Set prepend string for incoming whispers:    'whisp #prepend <string>'"
"                                              %W returns time in a prepend"
"To stop prepending incoming whispers:        'whisp #!prepend'"
"Allow whispers despite an #off setting:      'whisp #priority <plyr> <plyr>'"
"Clear names from the above list:             'whisp #!priority <plyr> <plyr>'"
"To reply to a whisper to you:                'whisp #r = <message>'"
"To reply to everyone in a multi-whisper:     'whisp #R = <message>'"
"To display what version this program is:     'whisp #version'"
"To display the latest program changes:       'whisp #changes'"
"To list who you last whispereed to, and who"
"                last whispereed to you:      'whisp #who'"
"---------------------- Words in <> are parameters. -----------------------"
23 show-help-list
;
  
: clean-list  ( d s -- )
   over over REF-first dup not if pop pop pop exit then
   begin dup while
      3 pick 3 pick 3 pick REF-next swap
      dup player? not if
         4 pick 4 pick rot REF-delete
      else
         pop
      then
   repeat
   pop pop pop
;
  
: show-whisper-prioritizing ( -- )
    me @ PRIORITYLIST clean-list
    "You are currently whisper prioritizing "
    me @ PRIORITYLIST REF-list dup not if pop "no one" then
    strcat "." strcat me @ swap notify
;
  
: show-whisper-ignoring ( -- )
    me @ IGNORELIST clean-list
    "You are currently whisper ignoring "
    me @ IGNORELIST REF-list dup not if pop "no one" then
    strcat "." strcat me @ swap notify
;
  
: show-who-info ( -- )
    "You last whispered to "
    me @ getlastwhisperd comma-format
    dup not if pop "no one" then
    strcat "." strcat me @ swap notify
  
    "You were last whispered to by "
    me @ getlastwhisperr
    dup not if pop "no one" then
    strcat "." strcat me @ swap notify
  
    "The last group whisper included "
    me @ getlastwhisperdgroup
    comma-format dup not if pop "no one else" then
    strcat "." strcat me @ swap notify
  
    show-whisper-ignoring
    show-whisper-prioritizing
;
  
( remember stuff )
  
: extract-player-loop (<range> str dbref -- string)
    3 pick not if pop swap pop exit then
    4 rotate dup if
        over name over stringcmp not if pop
        else
            rot dup if " " strcat then
            swap strcat swap
        then
    then
    rot 1 - rot rot extract-player-loop
;
  
: extract-player (dbref string -- string)
    single-space " " explode dup 2 + rotate
    "" swap extract-player-loop
;
  
: remember-whisperr (playerdbref -- )
    dup me @ name swap setlastwhisperr
    me @ getlastwhisperd
    over swap extract-player
    swap setlastwhisperdgroup
;
  
: remember-whisperee (player[s] -- player[s])
    dup not if        (is a player specified?)
        pop me @      (if not, use last player whisperd...)
        getlastwhisperd
    else
        single-space  (...otherwise, use the player given...)
    then
;
  
( whisper stuff )
  
: whisperpose? (string -- bool)
    2 strcut pop
    dup strlen 1 >
    over ":" 1 strncmp not and if
      1 strcut swap pop
      " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,':"
      swap instr
    else pop 0
    then
;
  
: whisper-them-inform (names message dbref format -- )
    me @ name "%n" subst (do name substitution for %n in format string)
    4 rotate "you"
    4 pick name subst
    "%t" subst                        (do reciever's names for %t in fmt string)
    rot "%m" subst                    (do message sub for %m in format string)
                                      (add prepend string if prepending is activated)      
    swap dup dup "_whisp/not_prepending?" getpropval not if
        pop swap
    else
        rot rot getprepend "%l:%M%p" systime timefmt strip
        dup rot swap "%w" subst swap "%W" subst
        swap strcat
    then
    notify                            (show constructed string to reciever)
;
( player getting stuff )
  
: get-playerdbrefs-remote  (cnt "" "" playersstr -- dbref_range ambig unrec)
    dup not if pop stripspaces exit then
    " " split swap
    dup do-pmatch dup 0 > if
        pop swap pop 5 rotate
        1 + -5 rotate -5 rotate
    else
        -1 = if
            "\"" swap strcat
            "\" " strcat 4 rotate
            swap strcat -3 rotate
        else
            "\"" swap strcat
            "\" " strcat rot
            swap strcat swap
        then
    then
    get-playerdbrefs-remote
;
  
: get-playerdbrefs  (count nullstr null playersstr -- dbref_range ambig unrec)
    dup not if pop stripspaces exit then
    " " split swap
    dup player-match? dup 0 > if
        pop swap pop 5 rotate
        1 + -5 rotate -5 rotate
    else
        -1 = if
            "\"" swap strcat
            "\" " strcat 4 rotate
            swap strcat -3 rotate
        else
            "\"" swap strcat
            "\" " strcat rot
            swap strcat swap
        then
    then
    get-playerdbrefs
;
  
: refs2names  (dbrefrange count nullstr -- dbrefrange namestr)
    over not if swap pop stripspaces exit then
    3 pick 3 + rotate dup -5 rotate
    name strcat " " strcat
    swap 1 - swap refs2names
;
  
: remove-sleepers (dbrefrange count nullstr -- dbrefrange sleeperstr)
    over not if swap pop stripspaces exit then
    3 pick 3 + rotate dup awake? if
        -4 rotate
    else
        name " " strcat strcat
        rot 1 - rot rot
    then
    swap 1 - swap remove-sleepers
;
  
: remove-ignorers (dbrefrange count nullstr -- dbrefrange ignorestr)
    over not if swap pop stripspaces exit then
    3 pick 3 + rotate dup being-ignored? not if
        -4 rotate
    else
        name " " strcat strcat
        rot 1 - rot rot
    then
    swap 1 - swap remove-ignorers
;
  
: remove-notusing (dbrefrange count nullstr -- dbrefrange notusingstr)
    over not if swap pop stripspaces exit then
    3 pick 3 + rotate dup notusing? not over prioritized? or if
        -4 rotate
    else
        name " " strcat strcat
        rot 1 - rot rot
    then
    swap 1 - swap remove-notusing
;
  
: do-getplayers-remote (players -- dbrefrange)
    stripspaces single-space
    0 "" "" 4 rotate get-playerdbrefs-remote
    dup if
        comma-format dup " " instr
        "I don't recognize the player"
        swap if "s" strcat then
        " named " strcat swap strcat
        "." strcat
        me @ swap notify
    else pop
    then
    dup if
        comma-format dup " " instr
        "The name"
        over if "s" strcat then
        " " strcat rot strcat
        swap if " are " else " is " then strcat
        "ambiguous." strcat
        me @ swap notify
    else pop
    then
;
  
: do-getplayers (players -- dbrefrange)
    stripspaces single-space
    0 "" "" 4 rotate get-playerdbrefs
    dup if
        comma-format dup " " instr
        "I don't see the player"
        swap if "s" strcat then
        " named " strcat swap strcat
        " here." strcat
        me @ swap notify
    else pop
    then
    dup if
        comma-format dup " " instr
        "The name"
        over if "s" strcat then
        " " strcat rot strcat
        swap if " are " else " is " then strcat
        "ambiguous." strcat
        me @ swap notify
    else pop
    then
;
  
: do-sleepers (dbrefrange -- dbrefrange')
    dup "" remove-sleepers
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently asleep." strcat
        strcat me @ swap notify
    else pop
    then
;
  
: do-ignores (dbrefrange -- dbrefrange')
    dup "" remove-ignorers
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently whisper ignoring you." strcat
        strcat me @ swap notify
    else pop
    then
;
  
: do-notusing (dbrefrange -- dbrefrange')
    dup "" remove-notusing
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently not using Whisper." strcat
        strcat me @ swap notify
    else pop
    then
;
  
: get-valid-whisperees (players -- dbrefrange players')
    do-getplayers
    do-sleepers
    do-ignores
    do-notusing
    dup "" refs2names
;
  
: get-valid-priority-names (players -- dbrefrange players')
    do-getplayers-remote
    dup "" refs2names
;
  
: get-valid-ignore-names (players -- dbrefrange players')
    do-getplayers-remote
    dup "" refs2names
;
  
( multi stuff )
  
: whisper-toeach (dbrefrange names message -- )
    3 pick not if pop pop pop exit then
    3 pick 3 + rotate 3 pick 3 pick rot
    (dbrefrng names msg names msg dbref)
    dup remember-whisperr
    "%n whispers, \"%m\" to %t."
    whisper-them-inform
    rot 1 - rot rot
    whisper-toeach
;
  
: multi-priority (string -- )
    get-valid-priority-names
    dup if
        "You add " swap comma-format strcat " to your priority list."
        strcat me @ swap notify
        begin dup while
            1 - swap
            me @ PRIORITYLIST rot REF-add
        repeat
        pop
    else pop pop
    then
    show-whisper-prioritizing
;
  
: multi-unpriority (string -- )
    get-valid-priority-names
    dup if
        "You remove " swap comma-format strcat " from your priority list."
        strcat me @ swap notify
        begin dup while
            1 - swap
            me @ PRIORITYLIST 3 pick REF-inlist? if
                me @ PRIORITYLIST rot REF-delete
            else pop
            then
        repeat
        pop
    else pop pop
    then
    show-whisper-prioritizing
;
  
: multi-ignore (string -- )
    get-valid-ignore-names
    dup if
        "You add " swap comma-format strcat " to your ignore list."
        strcat me @ swap notify
        begin dup while
            1 - swap
            me @ IGNORELIST rot REF-add
        repeat
        pop
    else pop pop
    then
    show-whisper-ignoring
;
  
: multi-unignore (string -- )
    get-valid-ignore-names
    dup if
        "You remove " swap comma-format strcat " from your ignore list."
        strcat me @ swap notify
        begin dup while
            1 - swap
            me @ IGNORELIST 3 pick REF-inlist? if
                me @ IGNORELIST rot REF-delete
            else pop
            then
        repeat
        pop
    else pop pop
    then
    show-whisper-ignoring
;
  
: deactivate-prepend ( -- )
    me @ dup "_whisp/not_prepending?" 0 setprop
    "Prepending deactivated." notify
;
  
: add-prepend (string --)
    dup " " strcat me @ setprepend
    
    me @ "_whisp/not_prepending?" 1 setprop
    "Prepending activated." me @ swap notify
    "Prepend string set to: " swap strcat dup
    me @ swap notify
;
  
: activate-prepend ( -- )
    me @ "_whisp/not_prepending?" getpropval not if
        me @ getprepend add-prepend
    else
        me @ getprepend
        "You are currently prepending: " swap strcat dup
        me @ swap notify
    then
;
  
: multi-whisper (message player -- )
    get-valid-whisperees
    dup if
        dup me @ setlastwhisperd comma-format
        over 3 + rotate dup  (dbrng cnt names msg msg)
        "You whisper, \"%m\" to %n."       (dbrng cnt names msg msg fmt)
        4 pick "%n" subst    (dbrng cnt names msg msg fmt)
        me @ name "%i" subst (dbrng cnt names msg msg fmt)
        swap "%m" subst      (dbrng cnt names msg fmt)
        tell whisper-toeach
    else pop pop pop
    then
;
  
: whisper-main
    get-lastversion dup VERSION strcmp if
        dup if
        "Whisper has been upgraded.  Type 'whisper #changes' to see the latest mods."
            tell
            "You last used " over strcat tell
        then
        VERSION set-lastversion
    then pop
 
    stripspaces
    dup "#R" 2 strncmp not if
        2 strcut swap pop
        me @ getlastwhisperdgroup
        " " strcat swap strcat
        "#r" swap strcat
    then
    dup "#r" 2 strncmp not if
        2 strcut swap pop
        me @ getlastwhisperr
        " " strcat swap strcat
    then
    dup "#" 1 strncmp not if   (if it begins with #, then it is a command)
        dup "#version" 2 stringmatch? if
            pop VERSION "  " strcat UPDATED strcat
            me @ swap notify exit
        then
        dup "#changes" 2 stringmatch? if
            pop show-changes exit
        then
        dup "#help" 2 stringmatch? if
            pop show-help exit
        then
        dup "#who" 2 stringmatch? if
            pop show-who-info exit
        then
        dup "#prepend" 3 stringmatch? if
            pop activate-prepend exit
        then
        dup "#!prepend" 4 stringmatch? if
            pop deactivate-prepend exit
        then
        me @ name tolower "guest" 5 strncmp if
            dup "#on" 3 stringmatch? if
                pop set-whisper-on "You will now receive whispers as normal."
                tell exit
            then
            dup "#off" 3 stringmatch? if
                pop set-whisper-off 
                "You will no longer receive whispers at all." tell 
                "Use 'whisp #on' to restore." tell exit
            then
            dup " " split swap "#priority" 3 stringmatch? if
                multi-priority exit
            else pop
            then
            dup " " split swap "#!priority" 4 stringmatch? if
                multi-unpriority exit
            else pop
            then
            dup " " split swap "#ignore" 2 stringmatch? if
                multi-ignore exit
            else pop
            then
            dup " " split swap "#!ignore" 3 stringmatch? if
                multi-unignore exit
            else pop
            then
            dup " " split swap "#prepend" 3 stringmatch? if
                add-prepend exit
            else pop
            then
        then
        "whisper: Illegal command: " swap strcat me @ swap notify
        "Type \"whisper #help\" for help." me @ swap notify exit
    then
    dup "=" instr not if
        "What do you want to whisper?"
        tell pop exit
    else
        "=" split
        stripspaces
        dup whisperpose? if
            1 strcut swap pop
            me @ name
            over 1 strcut " " split pop strlen 3 <
            "!?.,'" rot instr and
            not if " " strcat then
            swap strcat
        then
        swap stripspaces single-space
        remember-whisperee
        multi-whisper        (do a message whisper)
    then
;
.
c
q
@register #me cmd-whisper=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3
@action whisper;whispe;whisp;whis;whi;wh;w=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1

