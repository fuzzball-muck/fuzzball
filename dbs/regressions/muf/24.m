lvar mylvar
 
: firstfunc[ -- ]
    mylvar @ pop
    exit
    "foo!" mylvar !
;
 
: secondfunc[ -- ]
    mylvar @ string? not if
        "LVAR fetch-and-clear optimization failed! (3)" abort
    then
    mylvar @ "bar!" stringcmp if
        "LVAR fetch-and-clear optimization failed! (4)" abort
    then
;
 
: thirdfunc[ -- ]
    mylvar @
    secondfunc
    "foo!" mylvar !
;
 
: main[ str:args -- ]
    "bar!" mylvar !
    firstfunc
    mylvar @ string? not if
        "LVAR fetch-and-clear optimization failed!" abort
    then
    mylvar @ "bar!" strcmp if
        "LVAR fetch-and-clear optimization failed! (2)" abort
    then
    
    thirdfunc
;
