: partial-match-loop (onlinerange name dbref -- dbref)
    3 pick not if rot rot pop pop exit then
    4 rotate dup name
    4 pick strlen strcut pop
    4 pick stringcmp not if
      over if
        over over dbcmp not
        if pop #-2 then
      then swap
    then
    pop rot 1 - rot rot
    partial-match-loop
;
  
: partial-match (name -- dbref)
    online dup 2 + rotate #-1
    partial-match-loop
    dup #-2 dbcmp if pop #-1 then
;
  
: def-pmatch (name -- dbref)
    dup "me" stringcmp not if
      pop "me" match exit
    then
    dup "#" 1 strncmp not if
      1 strcut swap pop
      dup number? if
        atoi dbref
        dup player? not if pop #-1 then
      else
        pop #-1
      then
    else
      "*" over strcat match
      dup not if
        pop partial-match
      else swap pop
      then
    then
    dup #-2 dbcmp if pop #-1 then
;
