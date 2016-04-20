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
 
 
