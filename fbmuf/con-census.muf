@prog con-census
1 9999 d
1 i
(Connect time census program by Garth Minette <foxen@netcom.com>)
(Version 1.2)
  
(Number of users listed in sorted list of most hours online that month)
$def TOP_LIST_COUNT 20
  
  
: GetOnTimeProp (player -- dbref propname)
  prog "_players/%m/" systime timefmt
  rot int intostr strcat
;
  
: update-record (d s i -- )
  3 pick 3 pick getpropval
  over < if
    "" swap addprop
  else
    pop pop pop
  then
;
  
: check-max-today
  prog "_day/%y/%m/%d" systime timefmt
  concount update-record
;
  
: check-max-record
  prog "_most_on_ever"
  concount update-record
;
  
: log-player-login
  me @ awake? 1 > if exit then
  me @ "@/logintime" "" systime addprop
  me @ "@/notedtime" remove_prop
;
  
: english-delta-time (i -- s)
  60 / dup 60 %
  dup if intostr " minutes " strcat else pop "" then
  swap 60 /
  dup if intostr " hours " strcat else pop "" then
  swap strcat
;
  
: get-recorded-time-period ( -- s)
  prog timestamps pop pop pop
  systime swap - 86400 /
  "%d" systime timefmt atoi
  over over > if swap then pop
  dup not if pop "today" exit then
  dup 1 = if pop "in the last day" exit then
  intostr " days" strcat
  "in the last " swap strcat
;
  
: announce-usage
  me @ GetOnTimeProp getpropval
  dup 60 < if pop exit then
  "## You have been online for "
  swap english-delta-time strcat
  get-recorded-time-period strcat
  me @ swap notify
;
  
  
: login-handler
  check-max-today
  check-max-record
  log-player-login
  
  me @ "_prefs/logintime?" getpropstr
  "yes" stringcmp not if announce-usage then
;
  
: into2digits (i -- s)
  dup intostr swap 10 < if "0" swap strcat then
;
  
: int2time (i -- s)
  dup 60 % into2digits swap 60 /
  dup 60 % into2digits "." strcat swap 60 /
  "0000" swap intostr strcat
  dup strlen 4 - strcut swap pop
  "." strcat swap strcat swap strcat
;
  
: update-sorted-moston (dbref oldtime newtime -- )
  prog "_mosttime/%m/" systime timefmt
  over over 6 rotate int2time strcat over over remove_prop
  "-" strcat 6 pick int intostr strcat remove_prop
  rot int2time strcat "-" strcat 3 pick int intostr strcat
  rot unparseobj
  3 pick 3 pick 3 pick 0 addprop
  
  pop 0
  begin
    dup TOP_LIST_COUNT < while
    3 pick 3 pick nextprop
    dup not if pop break then
    rot pop swap 1 +
  repeat
  TOP_LIST_COUNT >= if
    remove_prop
  else
    pop pop
  then
;
  
: update-usetime (d -- )
  dup "@/notedtime" getpropval
  over "@/notedtime" "" systime addprop
  over "@/logintime" getpropval
  over over < if swap then pop
  dup not if pop systime then systime swap -
  (d i d s)
  over GetOnTimeProp
  over over getpropval
  (d i d s i)
  4 rotate over + swap over
  6 rotate rot rot
  (d s i d i i)
  update-sorted-moston
  "" swap addprop
;
  
  
: logout-handler
  me @ update-usetime
  me @ awake? if exit then
  me @ "@/logintime" remove_prop
  me @ "@/notedtime" remove_prop
;
  
: startup-handler
  60 sleep
  begin
    600 sleep
    #-1 descriptors
    begin
      0 sleep
      dup while 1 - swap
      descrcon dup not if pop continue then
      condbref update-usetime
    repeat
    pop
  repeat
;
  
: dispatcher
  command @ "Queued event." stringcmp not if
    dup "Connect" stringcmp not if pop login-handler exit then
    dup "Disconnect" stringcmp not if pop logout-handler exit then
    dup "Startup" stringcmp not if pop startup-handler exit then
    pop exit
  then
  announce-usage
  pop exit
;
.
c
q
@set con-census=W
@set con-census=A
@set con-census=L
@set con-census=3
@reg #prop #0:_connect con-census=census
@reg #prop #0:_disconnect con-census=census
