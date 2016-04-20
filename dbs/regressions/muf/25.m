: shouldbe[ str:val -- ]
    val @ number? not if
        val @ " is a number, but tests as not being one!"
    then
;
 
: shouldnotbe[ str:val -- ]
    val @ number? if
        val @ " is not a number, but tests as being one!"
    then
;
 
: main[ str:args -- ]
    "+" shouldnotbe
    "-" shouldnotbe
    "1+" shouldnotbe
    "1-" shouldnotbe
    "01" shouldbe
    "1" shouldbe
    "a" shouldnotbe
    "09" shouldbe
    "0x9" shouldnotbe
    "1e4" shouldnotbe
    1 10 1 for
        intostr shouldbe
    repeat
    "4120000000" shouldbe
;
 
