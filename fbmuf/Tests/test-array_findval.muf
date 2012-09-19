@program test-array_findval
1 9999 d
1 i
: test-array_findval[ str:arg -- ]
    {
        "foo" 8
        "pi" 3.14159
        3 "test for"
        "eight" 8
        "nIne" "with a lock of"
        "lock" "#1&!#1" parselock
    }dict
    8
    array_findval
    dup array_count 2 = not if
        "Wrong number of entries in result" abort
    then
    dup 1 [] "foo" strcmp if
        "String entry foo not found" abort
    then
    dup 0 [] "eight" strcmp if
        "String entry eight not found" abort
    then
    pop
    { }dict "" array_findval
    array_count if
        "Wrong number of entries in null list result." abort    
    then
;
.
c
q
