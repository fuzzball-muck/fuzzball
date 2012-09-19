@program cmd-3who
1 9999 d
1 i
: stimestr (i -- s)
    dup 86400 > if
        86400 / intostr "d" strcat 
    else dup 3600 > if
            3600 / intostr "h" strcat
        else dup 60 > if
                60 / intostr "m" strcat
            else
                intostr "s" strcat
            then
        then
    then
    "    " swap strcat
    dup strlen 4 - strcut swap pop
;
  
  
: mtimestr (i -- s)
    "" over 86400 > if
        over 86400 / intostr "d " strcat strcat
        swap 86400 % swap
    then
    over 3600 / intostr
    "00" swap strcat
    dup strlen 2 - strcut
    swap pop strcat ":" strcat
    swap 3600 % 60 / intostr
    "00" swap strcat
    dup strlen 2 - strcut
    swap pop strcat
;
  
  
: collate-entry (i -- s)
    dup condbref name
    over contime mtimestr
    over strlen over strlen +
    dup 19 < if
        "                   " (19 spaces)
        swap strcut swap pop
    else
        19 - rot dup strlen rot -
        strcut pop swap ""
    then
    swap strcat strcat
    swap conidle stimestr strcat
;
  
  
  
: get-namelist  ( -- {s})
    0 concount
    begin
        dup 0 > while
        dup collate-entry
        rot 1 + rot
        1 -
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
        .tell 3 -
    repeat
    dup if
        ""
        begin
            over 0 > while
            rot strcat "   " strcat
            swap 1 - swap
        repeat
        .tell
    then
    pop
;
  
  
: show-who
    preempt
    "Name         OnTime Idle  " dup strcat
    "Name         Ontime Idle" strcat .tell
    get-namelist
    show-namelist
    concount intostr
    " players are connected."
    strcat .tell
;
.
c
q
@set cmd-3who=w
#ifdef NEW
@action 3who;3w=#0=tmp/exit1
@link $tmp/exit1=cmd-3who
#endif
