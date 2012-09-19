@prog test-array_cut
1 99999 d
1 i
: arrcmp[ arr:a arr:b str:testname -- ]
    a @ b @ array_compare if
        testname @ " check failed." strcat abort
    then
;
 
: main[ str:args -- ]
    { "Alpha" "Beta" "Gamma" }list var! mylist
    
    mylist @ -1 array_cut
    { "Alpha" "Beta" "Gamma" }list "Index -1, second result" arrcmp
    {                        }list "Index -1, first result" arrcmp
    
    mylist @ 0 array_cut
    { "Alpha" "Beta" "Gamma" }list "Index 0, second result" arrcmp
    {                        }list "Index 0, first result" arrcmp
    
    mylist @ 1 array_cut
    { "Beta" "Gamma" }list "Index 1, second result" arrcmp
    { "Alpha"        }list "Index 1, first result" arrcmp
    
    mylist @ 2 array_cut
    { "Gamma"        }list "Index 2, second result" arrcmp
    { "Alpha" "Beta" }list "Index 2, first result" arrcmp
    
    mylist @ 3 array_cut
    {                        }list "Index 3, second result" arrcmp
    { "Alpha" "Beta" "Gamma" }list "Index 3, first result" arrcmp
    
    mylist @ 4 array_cut
    {                        }list "Index 4, second result" arrcmp
    { "Alpha" "Beta" "Gamma" }list "Index 4, first result" arrcmp
;
.
c
q
@register #me test-array_cut=tmp/prog1
@set $tmp/prog1=3

