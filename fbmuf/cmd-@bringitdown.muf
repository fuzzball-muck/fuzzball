@prog cmd-@bringitdown
1 99999 d
1 i
$def WARNTIMES 15 10 5 1

: wall[ str:mesg -- ]
    #-1 descr_array
    foreach swap pop
        mesg @ descrnotify
    repeat
;


: warn[ str:mesg str:whendown -- ]
    {
        "  "
        { "## WARNING: " __muckname " will be shutting down " whendown @ }join
        mesg @ if
            "## " mesg @ strcat
        then
        "## All building will be saved to disk, and we should be"
        "## back up within a couple minutes after the shutdown."
        "## If you have any questions, please page a wizard for help."
        "  "
    }list "\r" array_join wall
;


: main[ str:args -- ]

    me @ "wizard" flag?
$ifdef __muckname=FurryMUCK
    me @ #1026 dbcmp or
$endif
    not if
        "Permission denied." .tell
        pop exit
    then

    args @ tolower
    "#help" 5 strncmp not if
        "@bringitdown <message> to shut down the MUCK in 15 mins, warning the users." .tell
        "@bringitdown #restart <message> to restart in 15 mins, warning the users." .tell
        exit
    then

    "!@shutdown" var! thecmd
    args @ tolower
    "#restart" 8 strncmp not if
        args @ 8 strcut swap pop striplead args !
        "!@restart" thecmd !
    else
        args @ "#" 1 strncmp not if
            "Unrecognized #-option " swap strcat .tell
            exit
        then
    then

    { WARNTIMES }list SORTTYPE_DESCENDING array_sort var! sortedtimes
    sortedtimes @ dup array_count -- [] if
        0 sortedtimes @ array_appenditem sortedtimes !
    then
    sortedtimes @ 0 [] var! maxtime

    { }dict var! warnsystimes
    sortedtimes @
    foreach var! t pop
        totaltime @ t @ 60 * -
        systime +
        warnsystimes @ t @ ->[] warnsystimes !
    repeat

    sortedtimes @
    foreach var! t pop
        warnsystimes @ t @ [] var! nexttime
        nexttime @ systime > if nexttime @ systime - sleep then
        "NOW." var! whenstr
        t @ 0 > if
            t @ 1 = if "in %i minute." else "in %i minutes." then
            t @ swap fmtstring whenstr !
        then
        args @ whenstr @ warn
    repeat

    10 sleep
    #1 "@set me=!q" force
    #1 thecmd @ force
;
.
c
q
@register #me cmd-@bringitdown=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=3
@chown $tmp/prog1=#1
@action @bringitdown=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1

