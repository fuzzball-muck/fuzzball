@program test-array_join
1 9999 d
1 i
: test-array_join[ str:arg -- ]
    {
        "This is a"
        3.14159
        "test for"
        8
        "with a lock of"
        "#1&!#1" parselock
    }list " " array_join
    "This is a 3.14159 test for 8 with a lock of One(#1P*)&!One(#1P*)"
    smatch not if
        "Unexpected result." abort
    then
;
.
c
q
