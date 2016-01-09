(
  Generic Pose v2.0
  by Tygryss, modified then rewritten by JaXoN
  then modified again by Tygryss }=>
)
 
: generic-pose
  dup if
    dup strlen 3 > if
      dup "'" 1 strncmp not if
        dup 4 strcut pop " " instr not if " " swap strcat then
      then
    then
    ". ,':!?-"
    over 1 strcut pop instr not if " " swap strcat then
  then
  me @ name swap strcat
  me @ location #-1 rot notify_except
;
