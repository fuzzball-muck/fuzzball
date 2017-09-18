( MUFwhisper  Copyright 4/30/91 by Garth Minette                 )
(                                 tygryss@through.cs.caltech.edu )
(                                                                )
( Author reserves the right to update and edit this program on   )
( any system it is used on.                                      )
 
$def VERSION "MUFwhisper v1.05 by Tygryss"
$def UPDATED "Updated 7/13/92"
  
( *** routines to get and set properties *** )
  
: getlastwhisperr (playerdbref -- string)
     "_whisp/lastwhisperer" getpropstr
;
  
: setlastwhisperr (string playerdbref -- )
     "_whisp/lastwhisperer" rot setprop
;
  
: getlastwhisperd (playerdbref -- string)
     "_whisp/lastwhispered" getpropstr
;
  
: setlastwhisperd (string playerdbref -- )
     "_whisp/lastwhispered" rot setprop
;
  
: getlastwhisperdgroup (playerdbref -- string)
     "_whisp/lastwhisperedgroup" getpropstr
;
  
: setlastwhisperdgroup (string playerdbref -- )
     "_whisp/lastwhisperedgroup" rot setprop
;
  
( *** end of routines for getting and setting properties *** )
  
( misc simple routines )
  
  
: single-space (s -- s') (strips all multiple spaces down to a single space)
    dup "  " instr not if exit then
    " " "  " subst single-space
;
  
  
: comma-format (string -- formattedstring)
    strip single-space
    ", " " " subst
    dup ", " rinstr dup if
        1 - strcut 2 strcut
        swap pop " and "
        swap strcat strcat
    else pop
    then
;
  
  
  
: stringmatch? (str cmpstr #charsmin-- bool)
    swap over strcut swap
    4 rotate 4 rotate strcut rot rot
    stringcmp if pop pop 0 exit then
    swap over strlen strcut pop
    stringcmp not
;
  
  
( simple player matching )
  
: match-ambiguous-loop (name matched dbref -- dbref)
    dup not if pop swap pop exit then
    dup player? if
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
    1
;
  
  
( help stuff )
  
  
: show-help-list
    dup not if pop exit then
    dup 1 + rotate me @ swap notify
    1 - show-help-list
;
  
  
: show-changes
"v1.05  7/13/92  PropDirs release."
"v1.00  4/30/91  Initial release."
1 show-help-list
;
  
  
: show-help
VERSION "   " strcat UPDATED strcat "   Page1"
 strcat
"--------------------------------------------------------------------------"
"To whisper a message to another player:      'whisp <plyr> = <mesg>'"
"To do a pose style whisper to a player:      'whisp <player> = :<pose>'"
"To whisper to multiple people:               'whisp <plyr> <plyr> = <msg>'"
"To whisper again to the last players:        'whisp = <message>'"
"To reply to a whisper to you:                'whisp #r = <message>'"
"To reply to everyone in a multi-whisper:     'whisp #R = <message>'"
"To display what version this program is:     'whisp #version'"
"To display the latest program changes:       'whisp #changes'"
"To list who you last whispereed to, and who"
"                last whispereed to you:       'whisp #who'"
"---------------------- Words in <> are parameters. -----------------------"
13 show-help-list
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
    dup ":" 1 strncmp not if
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
    "%t" subst           (do reciever's names for %t in fmt string)
    rot "%m" subst       (do message sub for %m in format string)
    notify               (show constructed string to reciever)
;
  
  
  
( player getting stuff )
  
  
: get-playerdbrefs  (count nullstr null playersstr -- dbref_range ambig unrec)
    dup not if pop strip exit then
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
    over not if swap pop strip exit then
    3 pick 3 + rotate dup -5 rotate
    name strcat " " strcat
    swap 1 - swap refs2names
;
  
  
: remove-sleepers (dbrefrange count nullstr -- dbrefrange sleeperstr)
    over not if swap pop strip exit then
    3 pick 3 + rotate dup awake? if
        -4 rotate
    else
        name " " strcat strcat
        rot 1 - rot rot
    then
    swap 1 - swap remove-sleepers
;
  
  
: do-getplayers (players -- dbrefrange)
    strip single-space
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
  
  
: get-valid-whisperees (players -- dbrefrange players')
    do-getplayers
    do-sleepers
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
  
  
: multi-whisper (message player -- )
    get-valid-whisperees
    dup if
        dup me @ setlastwhisperd comma-format
        over 3 + rotate dup  (dbrng cnt names msg msg)
        "You whisper, \"%m\" to %n."       (dbrng cnt names msg msg fmt)
        4 pick "%n" subst    (dbrng cnt names msg msg fmt)
        me @ name "%i" subst (dbrng cnt names msg msg fmt)
        swap "%m" subst      (dbrng cnt names msg fmt)
        .tell whisper-toeach
    else pop pop pop
    then
;
  
  
: whisper-main
    strip
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
            pop VERSION "   " strcat UPDATED strcat
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
        "whisper: Illegal command: " swap strcat me @ swap notify
        "Type \"whisper #help\" for help." me @ swap notify exit
    then
    dup "=" instr not if
        "What do you want to whisper?"
        .tell pop exit
    else
        "=" split
        strip
        dup whisperpose? if
            1 strcut swap pop
            me @ name
            over 1 strcut " " split pop strlen 3 <
            "!?.,'" rot instr and
            not if " " strcat then
            swap strcat
        then
        swap strip single-space
        remember-whisperee
        multi-whisper        (do a message whisper)
    then
;
