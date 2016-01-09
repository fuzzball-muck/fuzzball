( @stats.MUF )
( Freeware, by Nimravid for SPR )
 
: header ( d -- )
  "Stats for : " swap unparseobj strcat .tell
;
 
: pad ( s i -- s )
  "                             " 
  rot swap strcat 
  swap strcut pop
;
 
: make_str ( s i i )
  over 100 * over /
  intostr "%" strcat
  swap intostr 9 pad
  rot intostr 9 pad
  swap strcat
  swap strcat
  strcat
;
 
: stuff ( d -- )
  stats
  #-1 stats
  "            " "Owned" 9 pad "MUCK" 9 pad "% owned" strcat strcat strcat .tell
  "Garbage  :  " 9 rotate rot make_str .tell
  "Players  :  " 8 rotate rot make_str .tell
  "Programs :  " 7 rotate rot make_str .tell
  "Things   :  " 6 rotate rot make_str .tell
  "Exits    :  " 5 rotate rot make_str .tell
  "Rooms    :  " 4 rotate rot make_str .tell
  "==================================" .tell
  "Total    :  " rot rot make_str .tell
;
 
: main
  "me" match owner me !
  caller exit? not if "Hey now." .tell exit then
  dup if
    dup "#help" stringcmp not if pop .showhelp exit then  ( Added #help option Natasha@HLM 31 October 2002 )
    me @ "wizard" flag?
    if .pmatch
      dup ok? if 
        dup header stuff exit
      else
        pop "Invalid player ref." .tell
        exit
      then
    else
      pop "Sorry, WIZ only." .tell exit
    then
  else
  me @ dup header stuff
  then
;
 
