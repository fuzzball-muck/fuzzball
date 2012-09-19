@prog test-systime_precise
1 99999 d
1 i
: main[ str:args -- ]
    systime systime_precise floor int
    = not if "SYSTIME != floor(SYSTIME_PRECISE)" abort then
 
    systime_precise var! stime
    1 100 1 for pop repeat
    systime_precise var! etime
    etime @ stime @ >= not if
        "Endtime < starttime" abort
    then
    etime @ stime @ -
    0.01 > if
        etime @ stime @ -
        "100 iteration for loop timed at %.6f sec."
        fmtstring
        abort
    then
;
.
c
q
@register #me test-systime_precise=tmp/prog1
@set $tmp/prog1=3


