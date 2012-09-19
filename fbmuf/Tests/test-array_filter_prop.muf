@prog test-array_filter_prop
1 99999 d
1 i
: main[ str:args -- ]
    #0 contents_array
    "_/de" "*e*" array_filter_prop
    var! filtered
    
    filtered @
    foreach
        swap pop
        "_/de/" getpropstr
        "*e*" smatch not if
            "returned item that didn't match filter!" abort
        then
    repeat
    
    filtered @
    #0 contents_array
    array_diff
    foreach
        swap pop
        "_/de" getpropstr
        "*e*" smatch if
            "didn't return item that matched filter!" abort
        then
    repeat
;
 
 
.
c
q
@register #me test-array_filter_prop=tmp/prog1
@set $tmp/prog1=3


