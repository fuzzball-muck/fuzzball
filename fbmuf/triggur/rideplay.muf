@edit rideplay
1 19999 d
i
( ride action sequence player )
( run this by setting the ride room's _arrive to run this program )
( works with tramlock.muf )
( trigger = the action used to enter the ride )
( trigger : _start prop set to text to print in outer room when ride starts )
( trigger : _stop prop set to text to print in outer room when ride stops )
( trigger : running prop set to running program pid when busy )
( trigger : _exitact prop set to dbref of exit from ride to lock )
( rideroom: running prop set to running program pid when busy )
( rideroom: countdown prop set to # of seconds left before starting )
( rideroom: started prop set to dbref of running program pid when waiting
              to start )
( rideroom: _delay prop set to # of seconds between 'poses' )
( rideroom: _script propdir set to sequential pose strings )
( rideroom: _out prop set to dbref of exit to dump riders thru when done )
( rideroom: _startdelay prop set to # of seconds to wait before setting 
              the running prop and starting the ride {allows people to 
              join} )
  
( By triggur of Furrymuck 	)
(
  If you must, contact pferd@netcom.com.
)
  
var repcount
var currline
  
( --------------------------------------------------------------- )
: main 
  preempt
  loc @ "started" getpropstr atoi ispid? if ( already started )
    me @ loc @ "countdown" getpropstr atoi 
      loc @ "_startdelay" getpropstr atoi + systime - intostr 
      "The ride will start in " swap strcat " seconds." strcat notify
    pid kill
  then
   
  loc @ "running" getpropstr atoi ispid? if ( already running )
    pid kill
  then
  
  trigger @ "running" getpropstr atoi ispid? if ( already running )
    pid kill
  then
  
( wait for the ride to start )
  loc @ "started" pid intostr 0 addprop 
  loc @ "countdown" systime intostr 0 addprop
  background
  me @ "The ride will start in " loc @ "_startdelay" getpropstr strcat
  " seconds." strcat notify
  loc @ "_startdelay" getpropstr atoi sleep
  
( start the ride ) 
  trigger @ "running" pid intostr 0 addprop 
  trigger @ "_exitact" getpropstr atoi dbref "running" pid intostr 0 addprop 
  loc @ "started" remove_prop
  loc @ "countdown" remove_prop
  
( tell those outside the ride that it has just started )
  trigger @ location #-1 trigger @ "_start" getpropstr notify_except
  
( show those inside the ride sequence )
  loc @ "_script#" getpropstr atoi repcount !
  1 currline !
  repcount @ begin
    loc @ loc @ "_script#/" currline @ dup intostr swap 1 + currline ! strcat
      getpropstr #-1 swap notify_except
     loc @ "_delay" getpropstr atoi sleep
  repcount @ 1 - dup repcount ! 0 = until
  
( tell those outside the ride that it has just ended )
  trigger @ location #-1 trigger @ "_stop" getpropstr notify_except
 
( open the doors back up ) 
  trigger @ "running" remove_prop
  trigger @ "_exitact" getpropstr atoi dbref "running" remove_prop 

( force ALL room contents to destination room! )
  loc @ "_out" getpropstr atoi dup if
    dbref currline !
    begin loc @ contents
      dup #-1 dbcmp not if
        dup dup name " " strcat currline @ odrop strcat pronoun_sub
		   currline @ getlink swap #-1 swap notify_except
        dup currline @ getlink moveto
     then
     #-1 dbcmp until
  then

( done )
;

.
c
q
