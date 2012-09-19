$include $lib/strings
$include $lib/match
  
  
: listem-loop (count dbref -- )
  dup not if
    pop intostr " exits." strcat
.tell exit
  then
  swap 1 + swap dup unparseobj .tell
  next listem-loop
;
  
  
: main
  .stripspaces dup not if
    pop "here"
  then
  .match_controlled dup not if pop exit then
  dup exit? over program? or if
    "That object doesn't have any exits."
    .tell pop exit
  then
  exits dup not if
    "That object has no exits on it."
    .tell pop exit
  then
  0 swap listem-loop
;
