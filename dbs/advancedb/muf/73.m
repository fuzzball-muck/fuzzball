( cmd-staff.muf by Natasha@HLM
  A simple wizard lister for Fuzzball 6.
  
  Copyright 2004 Natasha O'Brien. Copyright 2004 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$def prop_duty "_prefs/staff/duty"
$def prop_spec "_prefs/staff/spec"
 
$include $lib/table
$include $lib/strings
 
: rtn-name name ;
: rtn-duty  ( db -- str )
 dup awake? if
  prop_duty getpropstr .yes? if
   "\[[1;32mon duty"
  else
   "\[[1;33moff duty"
  then
 else
  pop "\[[1;34masleep"
 then  ( str )
;
: rtn-spec prop_spec getpropstr ;
 
 
: do-on pop
 me @ prop_duty "yes" setprop
 "You are on duty." .tell
;
: do-off pop
 me @ prop_duty remove_prop
 "You are off duty." .tell
;
: do-spec
 dup if
  me @ prop_spec rot setprop
  "Your specialties have been set." .tell
 else
  pop me @ prop_spec remove_prop
  "Your specialties have been cleared." .tell
 then
;
: do-help pop .showhelp ;
 
 
$define dict_commands {
 "on" 'do-on
 "off" 'do-off
 "spec" 'do-spec
 "help" 'do-help
}dict $enddef
 
: main
 STRparse pop  ( strX strY }  cmd #x y )
 
 dict_commands 3 pick tolower array_getitem  ( strX strY ? )
 dup address? if  ( strX strY adr )
  rot pop  ( strY adr )
  execute  (  )
  exit  (  )
 then  ( strX strY ? )
 pop swap pop pop  ( strY )
 
 
 "\[[1mWizards" "%26s" fmtstring .tell
 " " .tell
 
 "%-16.16[name]s  %8.8[duty]s\[[0m  %-50.50[spec]s"
 { "name" "\[[1mName" "duty" "\[[1mStatus" "spec" "\[[1mSpecialties" }dict
 { "name" 'rtn-name "duty" 'rtn-duty "spec" 'rtn-spec }dict
 
 {
 #-1 begin #-1 "" "pw" findnext dup ok? while
  dup
 repeat pop
 }list
 
 table-table
;
