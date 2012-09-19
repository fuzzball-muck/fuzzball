( cmd-reconnect by Natasha@HLM
  A Fuzzball 6 program for creating new players on the MUCK.
 
  Copyright 2002-2003 Natasha O'Brien. Copyright 2002-2003 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$include $lib/strings
: main  ( str -- )
    strip  ( str )
    dup not over "#" stringpfx or if pop .showhelp exit then  ( str )
    .singlespace " " split  ( strName strPass )
 
    ( Find old. )
    over .pmatch dup ok? not if  ( strName strPass db )
        pop pop "I don't see anyone named '%s'." fmtstring .tell  (  )
        exit  (  )
    then  ( strName strPass db )
    dup "Reconnecting as %D." fmtstring .tell  ( strName strPass db )
 
    rot pop swap  ( db strPass )
 
    me @ descrleastidle  ( db strPass intDescr )
    3 pick rot  ( db intDescr db strPass )
    3 try
        descr_setuser
    catch  ( db strErr )
        swap "Could not reconnect you as %D: %s" fmtstring .tell  (  )
        exit  (  )
    endcatch  ( db intReconnected )
 
    if  ( db )
        "Reconnected." notify  (  )
    else pop
        "Could not reconnect." .tell  (  )
    then  (  )
;
