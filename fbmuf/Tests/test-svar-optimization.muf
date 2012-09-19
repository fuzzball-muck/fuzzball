@prog test-svar-optimization
1 99999 d
1 i
: argstest[ str:a int:b ref:c -- ]
    a @ string? not if "Function arg a not a string."   abort then
    b @ int? not    if "Function arg b not an integer." abort then
    c @ dbref? not  if "Function arg a not a dbref."    abort then
    a @ "foo" strcmp    if "Function arg a has unexpected value."   abort then
    b @ 1234 = not      if "Function arg b has unexpected value."   abort then
    c @ #1234 dbcmp not if "Function arg c has unexpected value."   abort then
;
 
: main[ str:args -- ]
    "bar" var! a
    
    a @ pop "foo" a !
    a @ string? not if
        "SVAR fetch-and-clear optimization failed!" abort
    then
    a @ "foo" strcmp if
        "SVAR fetch-and-clear optimization failed! [bad strval]" abort
    then
    
    a @ pop 1 if
        a @ string? not if
            "SVAR fetch-and-clear optim branch test failed." abort
        then
        a @ "foo" strcmp if
            "SVAR fetch-and-clear optim branch test failed! [bad strval]" abort
        then
    then
    "baz" a !
    
    "foo" 1234 #1234 argstest
;
.
c
q
@register #me test-svar-optimization=tmp/prog1
@set $tmp/prog1=3


