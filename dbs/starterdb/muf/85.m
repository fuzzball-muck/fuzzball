$include $lib/timestr
 
: collate-entry (i -- s)
    dup descrdbref name
    over descrtime mtimestr
    over strlen over strlen +
    dup 19 < if
        "                   " (19 spaces)
        swap strcut swap pop
    else
        19 - rot dup strlen rot -
        strcut pop swap ""
    then
    swap strcat strcat
    swap descridle stimestr strcat
;
 
: get-namelist  ( -- {s} )
    0 #-1 firstdescr
    begin dup while
        dup collate-entry
        rot 1 + rot
        nextdescr
    repeat
    pop
;
lvar col
: show-namelist ({s} -- )
    begin
        dup 3 >= while
        swap "   " strcat
        over 3 / 3 pick 3 % 2 + 3 / +
        dup col ! 2 +
        rotate strcat "   " strcat
        over 3 / 3 pick 3 % 1 +
        3 / + col @ + 1 +
        rotate strcat
        tell 3 -
    repeat
    dup if
        ""
        begin
            over 0 > while
            rot strcat "   " strcat
            swap 1 - swap
        repeat
        tell
    then
    pop
;
 
: show-who
    preempt
    "Name         OnTime Idle  " dup strcat
    "Name         Ontime Idle" strcat tell
    get-namelist
    show-namelist
    concount intostr
    " players are connected."
    strcat tell
;
