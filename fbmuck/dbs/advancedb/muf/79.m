: show-docs ( d -- )
    "_docs" getpropstr dup if
        dup "Command to view: " swap strcat me @ swap notify
        "Run this command? (y/n)" .confirm if
            me @ swap force
        else
            me @ "Command cancelled." notify pop
        then
    else
        me @ "No documents available for that program, sorry." notify
        pop
    then
;
: show-defs-iter ( d s -- d s' )
    over over getpropstr
    over " = " strcat swap strcat 6 strcut swap pop
    me @ swap notify
    over swap nextprop
;
: show-defs ( d -- )
    dup "_defs/" nextprop dup if
        "Read definitions? (y/n)" .confirm if
            begin
                show-defs-iter
            dup not until
            pop pop
        else
            pop pop
        then
    else
        pop
    then
;
: @view
    dup not if
        me @ "Usage: @view program (#dbref or $name)" notify pop exit
    then
    match
    dup program? not if
        pop me @ "That isn't a program!" notify exit
    then
    dup show-docs
    show-defs
;
