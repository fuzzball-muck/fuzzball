@program test-array_matchval
1 9999 d
1 i
: test-array_matchval[ str:arg -- ]
    {
        "foo" "This is a"
        "pi" 3.14159
        3 "test for"
        "eight" 8
        "nIne" "with a lock of"
        "lock" "#1&!#1" parselock
    }dict
    "*i*"
    array_matchval
    dup array_count 2 = not if
        "Wrong number of entries in result" abort
    then
    dup "foo" [] "This is a" strcmp if
        "String entry foo not found" abort
    then
    dup "nIne" [] "with a lock of" strcmp if
        "String entry nIne not found" abort
    then
    pop
    { }dict "" array_matchval
    array_count if
        "Wrong number of entries in null list result." abort    
    then
;
.
c
q
