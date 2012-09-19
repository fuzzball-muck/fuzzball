@prog test-array_nested
1 99999 d
1 i
: print_array[ arr:in str:indent -- ]
    in @
    foreach var! val var! key
        me @ {
            indent @ key @
            val @ array? not if
                " = " val @
            then
        }join notify
        val @ array? if
            val @ indent @ "    " strcat
            print_array
        then
    repeat
;
 
: synth_nested_set[ any:val arr:arr arr:idxs -- arr:arr ]
    idxs @ 1 array_cut idxs ! 0 [] var! key
    idxs @ if
        val @
        arr @ key @ [] dup not if
            pop { }dict
        then
        idxs @
        synth_nested_set
        arr @ key @ ->[]
    else
        val @ arr @ key @ ->[]
    then
;
  
: synth_nested_del[ arr:arr arr:idxs -- arr:arr ]
    idxs @ 1 array_cut idxs ! 0 [] var! key
    idxs @ if
        arr @ key @ [] dup not if
            pop arr @ exit
        then
        idxs @
        synth_nested_del
        arr @ key @ ->[]
    else
        arr @ key @ array_delitem
    then
;
 
: synth_nested_get[ arr:arr arr:idxs -- any:val ]
    idxs @
    foreach swap pop
        arr @ swap []
        dup arr !
    not until
    arr @
;
 
: test_set[ arr:arr any:val arr:idxs -- arr:arr ]
   val @ arr @ idxs @ synth_nested_set dup
   val @ arr @ idxs @ array_nested_set
   array_compare if
       "  " print_array
       { "_SET failed on " idxs @ " " array_join "." }join abort
   then
;
 
: test_del[ arr:arr arr:idxs -- arr:arr ]
   arr @ idxs @ synth_nested_del dup
   arr @ idxs @ array_nested_del
   array_compare if
       "  " print_array
       { "_DEL failed on " idxs @ " " array_join "." }join abort
   then
;
 
: test_get[ arr:arr arr:idxs -- ]
   arr @ idxs @ synth_nested_get 1 array_make
   arr @ idxs @ array_nested_get 1 array_make
   array_compare if
       "  " print_array
       { "_GET failed on " idxs @ " " array_join "." }join abort
   then
;
 
: main[ str:args -- ]
    { }dict
    3      { "fa!" }list test_set
    4      { "fa!" }list test_set
    pi     { "Foo" "baz" "bar" }list test_set
    #1234  { "Foo" "baz" "bla" }list test_set
    51     { "Foo" "qux" "bar" }list test_set
    "nice" { "Gee" "qux" "bar" }list test_set
    
    dup { "Foo" "baz" "bla" }list test_get
    dup { "Foo" "baz" "bar" }list test_get
    
    { "Foo" "baz" "bla" }list test_del
    { "Foo" "qux" }list test_del
;
.
c
q
@register #me test-array_nested=tmp/prog1
@set $tmp/prog1=3


