@edit rideplay2
1 19999 d
i
( ride action sequence player2 )
( this one plays the script ONLY to the person who triggered it, so as
  to simulate rides that allow multiple people in them, but every rider
  sees something different at any point in time { as in a water slide } )
( run this by setting the ride room's _arrive to run this program )
( works with tramlock.muf )
( trigger = the action used to enter the ride )
( rideroom: _delay prop set to # of seconds between 'poses' )
( rideroom: _script propdir set to sequential pose strings )
( rideroom: _out prop set to dbref of room to dump riders in when done )
  
( By triggur of Furrymuck 	)
(
  If you must, contact pferd@netcom.com.
)
  
var repcount
var currline
  
( --------------------------------------------------------------- )
: main 
  background
( start the ride ) 
  
( show the rider the script ) 
  loc @ "_script#" getpropstr atoi repcount !
  1 currline !
  repcount @ begin
    me @ loc @ "_script#/" currline @ dup intostr swap 1 + currline ! strcat
      getpropstr notify
    loc @ "_delay" getpropstr atoi sleep
  repcount @ 1 - dup repcount ! 0 = until
  
( force the rider out! )
  loc @ "_out" getpropstr atoi dup if
    dbref currline !
    me @ me @ name " " strcat currline @ odrop strcat pronoun_sub
      currline @ getlink swap #-1 swap notify_except
    me @ currline @ getlink moveto
  then

( done )
;

.
c
q
