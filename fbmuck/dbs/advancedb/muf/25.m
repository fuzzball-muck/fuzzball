( lib-ansi-notify.muf by Natasha
  A simple program to do parse ~&123 ansi codes in a string to escape codes, on Fuzzball 6.
)
: rtn-ansify  ( str -- str' )
    var ret
    "~&" explode_array  ( ary )
    dup dup array_first pop  ( ary ary idxLast )
    swap over array_getitem ret !  ( ary idxLast )
    array_delitem  ( ary )
    foreach swap pop  ( str )
        dup not if pop continue then  ( str )
 
        dup "r*" smatch if  ( str )
            1 strcut swap pop  ( str' )
            "reset"  ( str' strAttrs )
        else dup "???*" smatch if  ( str )
            3 strcut  ( strCode str' )
            prog "_code2attr/" 4 rotate strcat getprop  ( str' strAttrs )
            dup int? if pop "reset" then  ( str' strAttrs )
        then then  ( str' strAttrs )
        textattr ( str' )
        dup strlen 4 - strcut dup "\[[0m" strcmp if strcat else pop then  ( str' )
        ret @ swap strcat ret !  (  )
    repeat  (  )
    ret @  ( str' )
;        
PUBLIC rtn-ansify
