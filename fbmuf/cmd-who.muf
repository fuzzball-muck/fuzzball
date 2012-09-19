@prog cmd-who
1 99999 d
1 i
$def MIN_IDLE 3 (minutes)
  
: do_and
    dup ", " rinstr
    dup if
        1 + strcut "and " swap
        strcat strcat
    else pop
    then
;
  
: idletime (d -- i)
    descriptors
    begin dup 1 > while rot pop 1 - repeat
    not if -1 exit then
    descrcon conidle
;
  
: who-loop (pupstr idlestr asleepstr awakestr dbref -- pupstr idlestr asleepstr awakestr)
    dup #-1 dbcmp if
        pop do_and
        4 rotate do_and
        4 rotate do_and
        4 rotate do_and
        4 rotate exit
    then
  
    dup player? if
        dup awake? if
            dup idletime dup MIN_IDLE 60 * < if
                pop over if
                    swap ", " strcat swap
                then
                dup name rot swap strcat swap
            else
                60 / intostr "[" swap strcat "m]" strcat
                5 pick if
                    5 rotate ", " strcat -5 rotate
                then
                over name swap strcat 5 rotate swap strcat -4 rotate
            then
        else
            dup "DARK" flag? not if
                3 pick if
                    rot ", " strcat rot rot
                then
                dup name 4 rotate swap strcat rot rot
            then
        then
    else
        dup "_listen" propdir?
        over "_listen" getpropstr or
        over "_puppet?" getpropstr tolower "y" 1 strncmp not or
	if
            5 pick if
                5 rotate ", " strcat -5 rotate
            then
            dup name 6 rotate swap strcat -5 rotate
        then
    then
    next who-loop
;
  
: cmd-who
    "me" match me !
    "" "" "" "" me @ location contents who-loop
    dup me @ name strcmp not if
        pop "You are the only one awake here."
    else
        "The players awake here are "
        swap strcat "." strcat
    then
    me @ swap notify
    swap
    dup if
        dup ", and " instr not if
            "Only " swap strcat
            " is idle here." strcat
        else
            "The idlers here are " swap strcat
            "." strcat
        then
        me @ swap notify
    else pop
    then
    me @ location "DARK" flag? not if
        dup not if
            pop "There are no sleepers here."
        else
            dup ", and " instr not if
                "Only " swap strcat
                " is asleep here." strcat
            else
                "The sleepers here are " swap strcat
                "." strcat
            then
        then
        me @ swap notify
    else pop
    then
    dup if
        dup ", and " instr not if
            "The only puppet here is " swap strcat
        else
            "The puppets here are " swap strcat
            "." strcat
        then
        me @ swap notify
    else pop
    then
;
.
c
q
@register #me cmd-who=tmp/prog1
@set $tmp/prog1=/_/de:A scroll containing a spell called cmd-who
