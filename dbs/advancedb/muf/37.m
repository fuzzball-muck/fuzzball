( neobootme.muf by Natasha@HLM.
 
  Copyright 1999 Natasha O'Brien.
  "@view $box/mit" for licensing information.
 
  A new @bootme program that works under the theory that booting all the users
  but the one who did the '@bootme' is more intuitive than booting all but the
  newest user.
 
  Anyone who's wanted to use @bootme to remotely disconnect a connection from
  another room, and has happened to disconnect oneself instead, understands
  that booting the lowest connection number isn't always desired. This program
  works under the theory that the connection that did the '@bootme' will be
  the least idle connection {since it just issued a command, after all}, and
  thusly boots all one's connections but the least idle.
 
)
: main ( s -- )
  pop  (  )
 
  ( Do we even need to @bootme? )
  me @ descriptors 2 < if  ( ix..i1 )
    "You only have the one connection." .tell exit
  then  ( ix..i1 }  ix..i1=user's descriptors )
 
  ( Remove the descriptor that has been idle for the least time, on the
    theory that that's the one who did the @bootme. Boot the others off. )
  begin depth 1 > while  ( ix..i2 i1 )
    over descrcon conidle  ( ix..i2 i1 it2 }  it2=number of seconds i2 has been connected )
    over descrcon conidle  ( ix..i2 i1 it2 it1 )
    ( If it2 is greater than it1, we'll want to boot i2, not i1. )
    > if  ( ix..i2 i1 )
      ( Get the one we should boot on top of the stack. )
      swap  ( ix..i1 i2 )
    then  ( ix..i? i? )
    descrcon  ( ix..i? ic }  id=connection we want to boot )
    dup "You have been booted by yourself." connotify conboot  ( ix..i? )
  repeat  ( i }  i=the one descriptor left over )
  descrcon "Old connections booted." connotify  (  )
;
