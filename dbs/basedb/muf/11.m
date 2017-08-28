( PROPS Library -- Contains useful property handling routines.
  
  setpropstr [dbref propname strval -- ]
    sets a property, or removes it if the string is null.
  
  envprop [startdbref propname -- stringval]
    searches down the environment tree from the given object, looking for
    a property with the given name.  Returns it's value, or a null string
    if it wasn't found.
  
  envsearch [startdbref propname -- locationdbref]
    Also searches down the envtree for the given property, but returns the
    dbref of the object it was found on instead of the string value.  Returns
    #-1 if it cannot find the property.
  
  locate-prop [startdbref propname -- locationdbref]
    Given a property name and dbref, finds the property, whether on the dbref
    itself, an environment of the dbref, or a proploc of the dbref.  If none,
    returns #-1.
  
)

$doccmd @list $lib/props=1-20
  
: setpropstr (dbref propname strval -- )
    dup not if
        pop remove_prop
    else
        0 addprop
    then
;
  
: envprop (startdbref propname -- stringvalue)
  envpropstr swap pop
;
  
: envsearch (startdbref propname -- locationdbref)
  envpropstr pop
;
  
: proplocsearch ( startdbref propname -- locationdbref )
  over swap                                   ( d d s )
  me @ "search" remove_prop
  begin over ok? while
    over over getpropstr if
      me @ "search" remove_prop
      pop swap pop exit               ( d )
    then
    over intostr "search/" swap strcat me @ swap getpropstr if
      me @ "search" remove_prop
      pop pop intostr
      "Error: loop in proploc found at #" swap strcat .tell
      #-1 exit
    then
    over intostr "search/" swap strcat me @ swap "yes" 0 addprop
    swap "_proploc" getpropstr dup if       ( d s d' )
      atoi dbref swap                 ( d d' s )
    else
      pop #-1 swap
    then
  repeat
  me @ "search" remove_prop
  pop pop pop #-1 exit
;
 
: locate-prop ( d s -- d' )
    over ok? not over not or if
        pop pop #-1 exit
    then
    over over proplocsearch dup ok? if              ( d s d' )
        rot rot pop pop exit
    then
    pop envsearch
;
  
PUBLIC setpropstr $libdef setpropstr
PUBLIC envprop $libdef envprop
PUBLIC envsearch $libdef envsearch
PUBLIC locate-prop $libdef locate-prop

$pubdef .envprop "$lib/props" match "envprop" call
$pubdef .envsearch "$lib/props" match "envsearch" call
$pubdef .locate-prop "$lib/props" match "locate-prop" call
$pubdef .setpropstr "$lib/props" match "setpropstr" call
