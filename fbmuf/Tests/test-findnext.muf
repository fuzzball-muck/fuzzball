@program test-findnext
1 9999 d
1 i
: test-findnext[ str:arg -- ]
    "cmd-test*" var! pattern
    "F" var! typeflag
    "D" var! invflag

    #-1 begin
        #-1 pattern @
        typeflag @ "!" invflag @ strcat strcat
        findnext
        dup while
        dup name "cmd-test*" smatch not if
            "Name smatch failed!" abort
        then
        dup program? not if
            "Flag match failed!" abort
        then
        dup invflag @ flag? if
            "Inverse flag match failed!" abort
        then
    repeat
    pop
;
.
c
q
