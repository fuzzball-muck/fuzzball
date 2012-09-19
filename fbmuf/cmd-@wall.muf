@prog cmd-@wall
1 9999 d
1 i
: wallit
    preempt
    "me" match "w" flag? not if
        pop me @ "Permission denied."
        notify exit
    then

    "You shout, \"" over strcat "\"" strcat
    me @ swap notify

    "me" match name " shouts, \"" strcat swap strcat "\"" strcat
    var mesg mesg !

    {
        #-1 descr_array
        { descr }list
    } array_ndiff
    foreach
        descrcon mesg @ connotify
        pop
    repeat
;
.
c
q
@set cmd-@wall=W
