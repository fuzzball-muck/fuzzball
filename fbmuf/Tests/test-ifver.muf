@prog test-ifver
1 99999 d
1 i
$version 1.010
$lib-version 1.010
 
: main[ str:args -- ]
$ifver this 1.001
    "1.001 is okay." pop
$else
    "$IFVER 1.001 failed." abort
$endif
$ifver this 1.009
    "1.009 is okay." pop
$else
    "$IFVER 1.009 failed." abort
$endif
$ifver this 1.010
    "1.010 is okay." pop
$else
    "$IFVER 1.010 failed." abort
$endif
$ifver this 1.011
    "$IFVER 1.011 failed." abort
$else
    "1.011 is okay." pop
$endif
$ifver this 1.1
    "$IFVER 1.1 failed." abort
$else
    "1.1 is okay." pop
$endif
 
 
$ifnver this 1.001
    "$IFNVER 1.001 failed." abort
$else
    "!1.001 is okay." pop
$endif
$ifnver this 1.009
    "$IFNVER 1.009 failed." abort
$else
    "!1.009 is okay." pop
$endif
$ifnver this 1.010
    "$IFNVER 1.010 failed." abort
$else
    "!1.010 is okay." pop
$endif
$ifnver this 1.011
    "!1.011 is okay." pop
$else
    "$IFNVER 1.011 failed." abort
$endif
$ifnver this 1.1
    "!1.1 is okay." pop
$else
    "$IFNVER 1.1 failed." abort
$endif
 
 
$iflibver this 1.001
    "LIB1.001 is okay." pop
$else
    "$IFLIBVER 1.001 failed." abort
$endif
$iflibver this 1.009
    "LIB1.009 is okay." pop
$else
    "$IFLIBVER 1.009 failed." abort
$endif
$iflibver this 1.010
    "LIB1.010 is okay." pop
$else
    "$IFLIBVER 1.010 failed." abort
$endif
$iflibver this 1.011
    "$IFLIBVER 1.011 failed." abort
$else
    "LIB1.011 is okay." pop
$endif
$iflibver this 1.1
    "$IFLIBVER 1.1 failed." abort
$else
    "LIB1.1 is okay." pop
$endif
 
 
$ifnlibver this 1.001
    "$IFNLIBVER 1.001 failed." abort
$else
    "!LIB1.001 is okay." pop
$endif
$ifnlibver this 1.009
    "$IFNVER 1.009 failed." abort
$else
    "!LIB1.009 is okay." pop
$endif
$ifnlibver this 1.010
    "$IFNLIBVER 1.010 failed." abort
$else
    "!LIB1.010 is okay." pop
$endif
$ifnlibver this 1.011
    "!LIB1.011 is okay." pop
$else
    "$IFNLIBVER 1.011 failed." abort
$endif
$ifnlibver this 1.1
    "!LIB1.1 is okay." pop
$else
    "$IFNLIBVER 1.1 failed." abort
$endif
;
.
c
q
@register #me test-ifver=tmp/prog1
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called test-ifver
@propset $tmp/prog1=str:/_lib-version:1.010
@propset $tmp/prog1=str:/_version:1.010

