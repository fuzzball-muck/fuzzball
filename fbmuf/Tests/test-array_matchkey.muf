@program test-array_matchkey
1 9999 d
1 i
: test-array_matchkey[ str:arg -- ]
    {
        "foo" "This is a"
        "pi" 3.14159
        3 "test for"
        "eight" 8
        "nIne" "with a lock of"
        "lock" "#1&!#1" parselock
    }dict
    "*i*"
    array_matchkey
    dup array_count 3 = not if
        "Wrong number of entries in result" abort
    then
    dup "pi" [] 3.14159 = not if
        "String entry pi not found" abort
    then
    dup "eight" [] 8 = not if
        "String entry eight not found" abort
    then
    dup "nIne" [] "with a lock of" strcmp if
        "String entry nIne not found" abort
    then
    pop
    { }dict "" array_matchkey
    array_count if
        "Wrong number of entries in null list result." abort    
    then
;
.
c
q
