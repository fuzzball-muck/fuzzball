$def maxidle 120
$def discon-mesg "Auto-disconnected for inactivity."
: idleboot
  concount begin
    dup while
    dup conidle maxidle 60 * > if
      dup condbref "wizard" flag? not if
$ifdef discon-mesg
        dup discon-mesg connotify
$endif
        dup conboot
      then
    then
    1 -
  repeat
  pop
;
: main
  background
  begin
    600 sleep
    idleboot
  0 until
;
