@prog test-exceptions
1 99999 d
1 i
: test-exceptions
    pop
    0 TRY
        -1 sleep
    CATCH
        dup "Timetravel*" smatch not if abort else pop then
    ENDCATCH
 
    0 TRY
        -1 sleep
    CATCH_DETAILED
        array? not if
            "CATCH_DETAILED did not return a dictionary."
        then
    ENDCATCH
 
    1
    0 try
        2
        2 try
            -1 sleep
        catch
            "TRY-CATCH stack locking is not monotonically increasing!" abort
        endcatch
    catch
        dup "Stack protection fault." stringcmp if abort else pop then
    endcatch
    pop
;
.
c
q
@register #me test-exceptions=tmp/prog1
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called test-exceptions

