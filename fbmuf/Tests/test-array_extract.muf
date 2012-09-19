@program test-array_extract
1 9999 d
1 i
: test-array_extract[ str:arg -- ]
    {
        "foo" "This is a"
        "pi" 3.14159
        3 "test for"
        "eight" 8
        "nine" "with a lock of"
        "lock" "#1&!#1" parselock
    }dict
    {
        "pi"
        3
        "foobarbazqux"
        3
    }list
    array_extract
    dup array_count 2 = not if
        "Wrong number of entries in result" abort
    then
    dup "pi" [] 3.14159 = not if
        "String entry pi not found" abort
    then
    dup 3 [] "test for" strcmp if
        "Integer entry 3 not found" abort
    then
    pop
    {
        "foo" "This is a"
        "pi" 3.14159
        3 "test for"
        "eight" 8
        "nine" "with a lock of"
        "lock" "#1&!#1" parselock
    }dict
    {
    }list
    array_extract
    dup array_count if
        "Wrong number of entries in null extract result" abort
    then
    pop
    {
    }dict
    {
        3
    }list
    array_extract
    dup array_count if
        "Wrong number of entries in null array result" abort
    then
    pop
;
.
c
q

