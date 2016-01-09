( cmd-spam.muf by Natasha@HLM
  Sets and clears one's nospam property for use with .notify_nospam and
  related macros.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$include $lib/strings
$def prop_nospam "_prefs/nospam"
 
: do-yes  ( -- )
    me @ prop_nospam remove_prop  (  )
    "You will again see noisy messages from various programs." .tell  (  )
;
 
: do-no  ( -- )
    me @ prop_nospam "yes" setprop  (  )
    "You will no longer see noisy messages from various programs." .tell  (  )
;
 
: main  ( str -- )
    STRparse  ( strX strY strZ )
    pop pop tolower  ( strX )
    { "yes" 'do-yes "no" 'do-no }dict swap array_getitem  ( ? )
    dup address? not if pop .showhelp exit then  ( adr )
    execute  (  )
;
