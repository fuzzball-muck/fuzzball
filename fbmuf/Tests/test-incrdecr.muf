@prog test-incrdecr
1 99999 d
1 i
var globalv
lvar localv
  
: incvartest[ dummy var:v int:num str:typ -- ]
    num @ v @ !
    v @ ++
    v @ @ num @ 1 +
    dup dbref? if dbcmp else = then
    not if typ @ " var increment failed." strcat abort then
;
  
: decvartest[ dummy var:v num str:typ -- ]
    num @ v @ !
    v @ --
    v @ @ num @ 1 -
    dup dbref? if dbcmp else = then
    not if typ @ " var decrement failed." strcat abort then
;
  
: incdecvartest[ var:v num str:typ -- ]
    0 v @ num @ typ @ incvartest
    0 v @ num @ typ @ decvartest
;
  
: vartest[ var:v str:typ -- ]
    v @ 0   typ @ " integer" strcat incdecvartest
    v @ 0.3 typ @ " float"   strcat incdecvartest
    v @ #0  typ @ " dbref"   strcat incdecvartest
;
  
: test-incrdecr[ scopedv -- ]
    0 ++ 1 = not if "Integer increment failed." abort then
    0 -- -1 = not if "Integer decrement failed." abort then
    0.3 ++ 1.3 = not if "Float increment failed." abort then
    0.3 -- -0.7 = not if "Float decrement failed." abort then
    #0 ++ #1 dbcmp not if "Dbref increment failed." abort then
    #0 -- #-1 dbcmp not if "Dbref decrement failed." abort then
  
    scopedv "Scoped var" vartest
    globalv "Global var" vartest
    localv  "Local var"  vartest
;
.
c
q
@register #me test-incrdecr=tmp/prog1
@set $tmp/prog1=3
 
