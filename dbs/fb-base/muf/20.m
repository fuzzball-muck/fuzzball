(Calculate and remember the server's local time zone.)
: tzoffset ( -- i)
  0 timesplit -5 rotate pop pop pop pop
  178 > if 24 - then
  60 * + 60 * +
;
  
: cmd-uptime
$ifdef __VERSION<MUCK2.2fb4.5
  trig not if #0 "_uptime" "" systime addprop exit then
  #0 "_uptime" getpropval
$else
  #0 "_sys/startuptime" getpropval
$endif
  dup systime swap -
  tzoffset - timesplit -5 rotate pop pop pop pop 4 rotate pop
  "Up " swap 1 - intostr strcat " days, " strcat
  swap intostr strcat " hours, " strcat
  swap intostr strcat " minutes since " strcat
  swap "%H:%M %m/%d/%y" swap timefmt strcat .tell
;
