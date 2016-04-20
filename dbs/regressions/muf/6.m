: main[ str:Args -- ]
    99999999  abs 99999999 = not if "Positive abs failed." abort then
    -99999999 abs 99999999 = not if "Negative abs failed!" abort then
    99999999  sign  1 = not if "Positive sign failed." abort then
    -99999999 sign -1 = not if "Negative sign failed." abort then
    0 sign 0 = not if "Zero sign failed." abort then
;
