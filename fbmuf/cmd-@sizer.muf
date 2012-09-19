@program cmd-@sizer
1 9999 d
1 i
$include $lib/strings
$include $lib/match
  
$def locknodebytes 18
$def propdirbytes 28
$def propbytes 28
$def objbytes 96
  
lvar bytesused
lvar overhead
lvar propcount
  
: init ( -- )
    0 bytesused !
    0 overhead !
    0 propcount !
;
  
: inc_bytes ( i -- )
    bytesused @ + bytesused !
;
  
: inc_overhead ( -- )
    overhead @ 12 + overhead !
;
  
: props_sizer_rec (d s -- )
    over swap "/" strcat nextprop
    dup not if pop pop exit then
    inc_overhead
    propdirbytes inc_bytes
    begin
        dup while
        propcount @ 1 + propcount !
        inc_overhead
        propdirbytes inc_bytes
        inc_overhead
        dup strlen 1 + inc_bytes
        over over getpropstr
        dup if
            strlen 1 + inc_bytes
            inc_overhead
        else pop
        then
        over over propdir? if
            over over props_sizer_rec
        then
        over swap nextprop
    repeat
    pop pop
;
  
: props_sizer (d -- )
    "" props_sizer_rec
;
  
: lock_sizer_rec ({s} -- )
    dup begin
        dup while
        dup locknodebytes * inc_bytes
        inc_overhead
        2 /
    repeat pop
    begin
        dup while 1 -
        swap pop
    repeat pop
;
  
: lock_sizer (d -- )
    dup getlockstr "#" explode
    lock_sizer_rec
    getlockstr ":" explode
    lock_sizer_rec
;
  
: obj_sizer (d -- )
    objbytes inc_bytes
    dup name strlen 1 + inc_bytes
    inc_overhead
    dup props_sizer
    lock_sizer
;
  
: display_size (d -- )
    unparseobj
    " takes up an estimated " strcat
    bytesused @ overhead @ + intostr strcat
    " bytes of memory." strcat .tell
;
  
lvar who
lvar totalbytes
: main
    "me" match me !
    #-1 who !
    0 totalbytes !
    command @ "@sizeall" stringcmp not
    command @ "@sizeallq" stringcmp not or if
        .pmatch dup not if
            pop "I don't know who you mean." .tell
            exit
        then
        me @ over dbcmp not
        me @ "wizard" flag? not and if
            pop "Permission denied." .tell
            exit
        then
        "This will take some time to finish, so I'll tell you when I'm done."
        .tell who !
        0 begin
            dup dbtop int < while
            dup dbref ok? if
                dup dbref owner who @ dbcmp if
                    init dup dbref obj_sizer
                    command @ "@sizeallq" stringcmp if
                        dup dbref display_size
                    then
                    bytesused @ overhead @ +
                    totalbytes @ + totalbytes !
                    1 sleep
                then
            then
            1 +
        repeat pop
        totalbytes @ intostr
        " estimated bytes uses total." strcat
        "@sizeall complete." .tell
        .tell
    else
        .match_controlled
        dup not if pop exit then
        init dup obj_sizer
        display_size
    then
;
.
c
q
#ifdef NEW
@action @sizer;@sizeall;@sizeallq;@size;@siz;@si=#0=tmp/exit1
@link $tmp/exit1=cmd-@sizer
#endif
