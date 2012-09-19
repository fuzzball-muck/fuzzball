( cmd-list by Natasha@HLM
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$include $lib/strings
$include $lib/match
: main  ( str )
    STRparse  ( strX strY strZ )
    rot "help" stringcmp not if pop pop .showhelp exit then  ( strY strZ )
    
    swap .noisy_match dup ok? not if pop pop exit then swap  ( db strZ )
    array_get_proplist dup if  ( arr )
        me @ 1 array_make array_notify  (  )
    else
        pop "There is no such list." .tell  (  )
    then  (  )
;
