@prog test-abs-sign
1 99999 d
1 i
: main[ str:Args -- ]
    99999999  abs 99999999 = not if "Positive abs failed." abort then
    -99999999 abs 99999999 = not if "Negative abs failed!" abort then
    99999999  sign  1 = not if "Positive sign failed." abort then
    -99999999 sign -1 = not if "Negative sign failed." abort then
    0 sign 0 = not if "Zero sign failed." abort then
;
.
c
q
@register #me test-abs-sign=tmp/prog1
@set $tmp/prog1=3


