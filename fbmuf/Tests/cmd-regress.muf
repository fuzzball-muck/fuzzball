@program cmd-regress
1 9999 d
1 i
: regress[ str:arg -- ]
    "test-*" var! pattern
    0            var! total
    0            var! fails

    #-1 begin
        #-1 pattern @ "F!Z" findnext
        dup while
        dup var! currprog
        currprog @ name 5 strcut swap pop toupper var! currname
        total @ 1 + total !
        0 try
            0 try
                arg @ currprog @ call
                "Success" "bg_green,bold,black" textattr
                " - " strcat currname @ strcat
                .tell
            catch
                "FAILURE" "bg_red,bold,black" textattr
                " - " strcat currname @ strcat
                ": " strcat swap strcat
                .tell
                fails @ 1 + fails !
            endcatch
            "cleanup" abort
        catch
            pop
        endcatch
    repeat
    pop

    fails @ total @
    "Regression tests complete.  %i tests, %i failures."
    fmtstring .tell
;
.
c
q

