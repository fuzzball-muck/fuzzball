@edit tramlock
1 19999 d
i
( exit lock that charges pennies )
( exit: _cost prop contains pennie charge amount )
( exit: _busy prop contains text of fail message when ride is busy )
( exit: running prop set to dbref of running program pid when busy )
( By triggur of Furrymuck 	)
(
  If you must, contact pferd@netcom.com.
)

( --------------------------------------------------------------- )
: main 
  trigger @ "running" getpropstr  atoi ispid? if
    me @ trigger @ "_busy" getpropstr notify
    0 exit
  then

  trigger @ "_cost" getpropstr atoi dup me @ pennies > if
    me @ trigger @ "_short" getpropstr notify
    pop 0
  else
    0 swap - me @ swap addpennies
    1
  then
;
.
c
q
