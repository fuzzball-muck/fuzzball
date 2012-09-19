@prog test-properties
1 99999 d
1 i
: expect[ any:val any:expected str:errtext -- ]
    val @ dup "%?: %~" fmtstring
    expected @ dup "%?: %~" fmtstring
    strcmp if
        {
            "%s: Expected %? '%~', but got %? '%~'."
            errtext @
            expected @ dup
            val @ dup
        } reverse fmtstring
        abort
    then
;
 
: main[ str:args -- ]
    prog "_foo" "bar" setprop
    prog "_foo" getprop    "bar" "GETPROP" expect
    prog "_foo" getpropstr "bar" "GETPROPSTR" expect
    prog "_foo" getpropval  0    "GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "GETPROPFVAL" expect
    prog "_foo" envprop    "bar" "ENVPROP" expect
    prog "_foo" envpropstr "bar" "ENVPROPSTR" expect
 
    prog "_foo" "bar" 0 addprop
    prog "_foo" getprop    "bar" "ADDPROP-GETPROP" expect
    prog "_foo" getpropstr "bar" "ADDPROP-GETPROPSTR" expect
    prog "_foo" getpropval  0    "ADDPROP-GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "ADDPROP-GETPROPFVAL" expect
    prog "_foo" envprop    "bar" "ADDPROP-ENVPROP" expect
    prog "_foo" envpropstr "bar" "ADDPROP-ENVPROPSTR" expect
 
    prog "_foo" 5 setprop
    prog "_foo" getprop     5    "GETPROP" expect
    prog "_foo" getpropstr  ""   "GETPROPSTR" expect
    prog "_foo" getpropval  5    "GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "GETPROPFVAL" expect
    prog "_foo" envprop     5    "ENVPROP" expect
    prog "_foo" envpropstr  ""   "ENVPROPSTR" expect
 
    prog "_foo" "" 5 addprop
    prog "_foo" getprop     5    "ADDPROP-GETPROP" expect
    prog "_foo" getpropstr  ""   "ADDPROP-GETPROPSTR" expect
    prog "_foo" getpropval  5    "ADDPROP-GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "ADDPROP-GETPROPFVAL" expect
    prog "_foo" envprop     5    "ADDPROP-ENVPROP" expect
    prog "_foo" envpropstr  ""   "ADDPROP-ENVPROPSTR" expect
 
    prog "_foo" #1 setprop
    prog "_foo" getprop     #1   "GETPROP" expect
    prog "_foo" getpropstr  "#1" "GETPROPSTR" expect
    prog "_foo" getpropval  0    "GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "GETPROPFVAL" expect
    prog "_foo" envprop     #1   "ENVPROP" expect
    prog "_foo" envpropstr  "#1" "ENVPROPSTR" expect
 
    prog "_foo/" #1 setprop
    prog "_foo/" getprop     #1   "GETPROP" expect
    prog "_foo/" getpropstr  "#1" "GETPROPSTR" expect
    prog "_foo/" getpropval  0    "GETPROPVAL" expect
    prog "_foo/" getpropfval 0.0  "GETPROPFVAL" expect
    prog "_foo/" envprop     #1   "ENVPROP" expect
    prog "_foo/" envpropstr  "#1" "ENVPROPSTR" expect
 
    prog "_foo/" 5 setprop
    prog "_foo" getprop     5    "GETPROP" expect
    prog "_foo" getpropstr  ""   "GETPROPSTR" expect
    prog "_foo" getpropval  5    "GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "GETPROPFVAL" expect
    prog "_foo" envprop     5    "ENVPROP" expect
    prog "_foo" envpropstr  ""   "ENVPROPSTR" expect
 
    prog "_foo/" "" 5 addprop
    prog "_foo" getprop     5    "ADDPROP-GETPROP" expect
    prog "_foo" getpropstr  ""   "ADDPROP-GETPROPSTR" expect
    prog "_foo" getpropval  5    "ADDPROP-GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "ADDPROP-GETPROPFVAL" expect
    prog "_foo" envprop     5    "ADDPROP-ENVPROP" expect
    prog "_foo" envpropstr  ""   "ADDPROP-ENVPROPSTR" expect
 
    prog "_foo" "" setprop
    prog "_foo" getprop     0    "GETPROP" expect
    prog "_foo" getpropstr  ""   "GETPROPSTR" expect
    prog "_foo" getpropval  0    "GETPROPVAL" expect
    prog "_foo" getpropfval 0.0  "GETPROPFVAL" expect
    prog "_foo" envprop     0    "ENVPROP" expect
    prog "_foo" envpropstr  ""   "ENVPROPSTR" expect
 
    prog "_foo" "bar" setprop
    prog "_foo/" remove_prop
    prog "_foo" getprop     0    "GETPROP" expect
 
    prog "_foo" "bar" setprop
    prog "_foo/bar" "baz" setprop
    prog "_foo" remove_prop
    prog "_foo" getprop     0    "GETPROP" expect
    prog "_foo/bar" getprop 0    "GETPROP" expect
 
    prog "_foo" "bar" setprop
    prog "_foo/bar" "baz" setprop
    prog "_foo" "" setprop
    prog "_foo" getprop     0     "GETPROP" expect
    prog "_foo/bar" getprop "baz" "GETPROP" expect
 
    prog "_foo" "bar" setprop
    prog "_foo" remove_prop
    prog "_foo" getprop     0    "GETPROP" expect
;
.
c
q
@register #me test-properties=tmp/prog1
@set $tmp/prog1=3

