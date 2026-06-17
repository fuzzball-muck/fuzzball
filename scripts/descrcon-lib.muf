( A quick library to provide 'close enough' utility for the old-style
  connection count primitives that have been fully replaced by the
  file-descriptor based connection addressing system.  Please note
  that while this attempts to be 'old style' compatible, some timing
  issues COULD occur during run-times, especially for 'background'
  or otherwise time-sliced processes on servers with highly motile
  populations.  This should NOT be considered a way out of updating
  your MUF, only a way to tide you over for upgrade. -Points c2026 )
( This library provides temporarily replacements for the following
  deprecated primitives:
   - condescr
   - contime
   - conboot
   - conhost
   - conuser
   - concount
   - conidle
   - condbref
   - connotify
   - descrcon
  The self-installer uses Revar's "@register" program/action as a global.
  If you don't have this installed, you'll need to manually set the _reg
  prop appropriately on #0.
)

$CLEARDEFS ALL

( Note - arrays are 0 based, but connections were 1 based. )

: concount ( - i )
 #-1 descr_array array_count
;

public concount

: condescr ( i - i )
 1 - dup 0 < if
  pop -1 exit then
 dup #-1 descr_array dup array_count rot < if
  pop pop -1 exit then
 swap array_getitem
;

public condescr

: contime ( i - i )
 condescr dup 0 < if
  exit then
 descrtime
;

public contime

: conboot ( i -- )
 caller "w" flag? me @ "w" flag? or not if
  pop exit then
 condescr dup 0 < if
  pop exit then
 descrboot
;

public conboot

: conhost ( i - s )
 caller "w" flag? me @ "w" flag? or not if
  pop "" exit then
 condescr dup 0 < if
  pop "" exit then
 descrhost
;

public conhost

: conuser ( i - s )
 caller "w" flag? me @ "w" flag? or not if
  pop "" exit then
 condescr dup 0 < if
  pop "" exit then
 descruser
;

public conuser

: conidle ( i - i )
 condescr dup 0 < if
  exit then
 descridle
;

public conidle

: condbref ( i - d )
 condescr dup 0 < if
  pop #-1 exit then
 descrdbref
;

public condbref

: connotify ( i s -- )
 caller mlevel 3 >= me @ mlevel 3 >= or not if
  pop pop exit then
 swap condescr dup 0 < if
  pop pop exit then
 swap descrnotify
;

public connotify

: descrcon ( i - i )
 #-1 descr_array swap array_findval dup array_count 0 = if
  pop 0 exit then
 0 array_getitem 1 +
;

public descrcon

: setup ( -- )
 me @ "W" flag? not if
  me @ "Library must be owned/registered by a wizard!" notify
  exit
 else
  prog "W" flag? not if
   me @ "Library must be set W" notify
   me @ "@set " prog name strcat "=W" strcat notify
   me @ "Then re-run." notify
   exit
  then
 then
 prog "L" flag? not if prog "L" set then (linkable)
 prog "_docs" "@list #" prog intostr strcat "=1-24" strcat 1 addprop (docs)
 ( Library calls for primitive replacements. )
 prog "_defs/concount" "\"$lib/conops\" match \"concount\" call" 1 addprop
 prog "_defs/condescr" "\"$lib/conops\" match \"condescr\" call" 1 addprop
 prog "_defs/descrcon" "\"$lib/conops\" match \"descrcon\" call" 1 addprop
 prog "_defs/contime" "\"$lib/conops\" match \"contime\" call" 1 addprop
 prog "_defs/conboot" "\"$lib/conops\" match \"conboot\" call" 1 addprop
 prog "_defs/conhost" "\"$lib/conops\" match \"conhost\" call" 1 addprop
 prog "_defs/conuser" "\"$lib/conops\" match \"conuser\" call" 1 addprop
 prog "_defs/conidle" "\"$lib/conops\" match \"conidle\" call" 1 addprop
 prog "_defs/condbref" "\"$lib/conops\" match \"condbref\" call" 1 addprop
 prog "_defs/connotify" "\"$lib/conops\" match \"connotify\" call" 1 addprop
 ( Now do the same on #0 to make these global. )
 #0 "_defs/concount" "\"$lib/conops\" match \"concount\" call" 1 addprop
 #0 "_defs/condescr" "\"$lib/conops\" match \"condescr\" call" 1 addprop
 #0 "_defs/descrcon" "\"$lib/conops\" match \"descrcon\" call" 1 addprop
 #0 "_defs/contime" "\"$lib/conops\" match \"contime\" call" 1 addprop
 #0 "_defs/conboot" "\"$lib/conops\" match \"conboot\" call" 1 addprop
 #0 "_defs/conhost" "\"$lib/conops\" match \"conhost\" call" 1 addprop
 #0 "_defs/conuser" "\"$lib/conops\" match \"conuser\" call" 1 addprop
 #0 "_defs/conidle" "\"$lib/conops\" match \"conidle\" call" 1 addprop
 #0 "_defs/condbref" "\"$lib/conops\" match \"condbref\" call" 1 addprop
 #0 "_defs/connotify" "\"$lib/conops\" match \"connotify\" call" 1 addprop
 ( Force user to register library globally. )
 me @ "@reg " prog name strcat "=lib/conops" strcat force
 me @ "Done." notify
;
