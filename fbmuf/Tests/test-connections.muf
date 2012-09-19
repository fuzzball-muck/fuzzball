@program test-connections
1 9999 d
1 i
$def tellme #1 descr_array 0 [] descrcon swap connotify

: show-em-dbref
    foreach
        swap pop
        dup "%D(%d)" fmtstring
        tellme
    repeat
    "(Done.)" tellme
;

: show-em-descr
    foreach
        swap pop var! dscr
        dscr @ descrcon var! con
        con @ conhost
        con @ contime
        con @ conidle
        con @ condbref dup
        "%D(%d)" fmtstring
        "%-20s %5is %5is %s" fmtstring
        tellme
    repeat
    "(Done.)" tellme
;

: test-connections
    "This test is not in correct form." abort
    pop
    online array_make array_reverse show-em-dbref
    online_array                    show-em-dbref
    #-1 descriptors array_make array_reverse show-em-descr
;
.
c
q
