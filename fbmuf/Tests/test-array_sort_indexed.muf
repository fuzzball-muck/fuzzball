@prog test-array_sort_indexed
1 99999 d
1 i
lvar mylist
 
: comparator[ any:a any:b int:sorttype -- int:comparison ]
    0 var! atype
    0 var! btype
    sorttype @ SORTTYPE_DESCENDING bitand if
        a @ b @ a ! b !
    then
    a @ int?    if 1 atype ! then
    a @ float?  if 1 atype ! then
    a @ dbref?  if 2 atype ! then
    a @ string? if 3 atype ! then
    b @ int?    if 1 btype ! then
    b @ float?  if 1 btype ! then
    b @ dbref?  if 2 btype ! then
    b @ string? if 3 btype ! then
    atype @ btype @ = not if
        atype @ btype @ - exit
    then
    atype @ 1 = if
        a @ b @ - exit
    then
    atype @ 2 = if
        a @ int b @ int - exit
    then
    atype @ 3 = if
        a @ b @
        sorttype @ SORTTYPE_CASEINSENS bitand if
            stringcmp
        else
            strcmp
        then
        exit
    then
    0
;
 
: dump-arrays[ arr:arr -- ]
    arr @
    foreach swap pop
        "" swap
        foreach
            { rot rot "=" swap "          " }join strcat
        repeat
        .tell
    repeat
    "---" .tell
;
 
: test-results[ list:mylist idx:key int:sorttype -- ]
    mylist @ sorttype @ key @ array_sort_indexed
    0 var! previdx
    foreach swap pop
        key @ [] dup var! curridx
        previdx @
        
        sorttype @ comparator 0 <
        previdx @ int? if previdx @ 0 = not else 0 then
        and if
            {
                "Ordering failed for"
                sorttype @ SORTTYPE_DESCENDING bitand if
                    " descending"
                else
                    " ascending"
                then
                sorttype @ SORTTYPE_CASEINSENS bitand if
                    " case insensitive"
                else
                    " case sensitive"
                then
                "."
            }join abort
        then
        curridx @ previdx !
    repeat
;
 
: test-array_sort[ str:arg -- ]
    var mylist
    {
        { #3  "Foo" 23 #18 }list
        { 2.7 "bar" 42 #1  }list
        { "a" "Baz" 12 #27 }list
        { 99  "qux" 34 #-1 }list
    }list mylist !
    
    0 3 1 for var! col
        mylist @ col @ SORTTYPE_CASE_ASCEND test-results
        mylist @ col @ SORTTYPE_CASE_DESCEND test-results
        mylist @ col @ SORTTYPE_NOCASE_ASCEND test-results
        mylist @ col @ SORTTYPE_NOCASE_DESCEND test-results
    repeat
 
 
    {
        { "val" #3    "val2" "Foo"   "val3" 23 }dict
        { "val" 2.7   "val2" "bar"   "val3" 12 }dict
        { "val" "a"   "val2" "Baz" }dict
        { "val" 99    "val2" "qux" }dict
    }list mylist !
    
    { "val" "val2" "val3" "val_3" }list
    foreach swap pop col !
        mylist @ col @ SORTTYPE_CASE_ASCEND test-results
        mylist @ col @ SORTTYPE_CASE_DESCEND test-results
        mylist @ col @ SORTTYPE_NOCASE_ASCEND test-results
        mylist @ col @ SORTTYPE_NOCASE_DESCEND test-results
    repeat
;
.
c
q
@register #me test-array_sort_indexed=tmp/prog1
@register #me test-array_sort_indexed=tmp/prog1
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called test-array_sort_indexed

