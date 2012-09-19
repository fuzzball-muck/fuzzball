@edit creditlock
1 19999 d
i
( exit lock that charges pennies )
( exit: _cost prop contains pennie charge amount )
( By triggur of Furrymuck 	)
(
  If you must, contact pferd@netcom.com.
)

( --------------------------------------------------------------- )
: main 
  trigger @ "_cost" getpropstr atoi dup me @ pennies > if
    pop 0
  else
    0 swap - me @ swap addpennies
    1
  then
;
.
c
q
