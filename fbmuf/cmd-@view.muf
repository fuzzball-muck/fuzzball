@prog cmd-@view
1 99999 d
1 i
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
.
c
q
@register #me cmd-@view=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=/_/de:A scroll containing a spell called cmd-@view
@action @view=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
@set $tmp/exit1=/_/de:@$desc %list[desc]
@set $tmp/exit1=/desc#:3
@set $tmp/exit1=/desc#/1:Usage: @view program
@set $tmp/exit1=/desc#/2:Either #dbref (i.e. #10) or $name (i.e. $lib/lmgr)
@set $tmp/exit1=/desc#/3:Purpose: if a _docs string exists on the program, will display it (should be a string like "@list $lib/string=1-10") and ask if it should run it.  If so, then it will be as if you typed this command.
