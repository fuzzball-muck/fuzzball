(Calculate and remember the server's local time zone.)
: tzoffset ( -- i)
  0 timesplit -5 rotate pop pop pop pop
  178 > if 24 - then
  60 * + 60 * +
;
  
: cmd-uptime
  #0 "_sys/startuptime" getpropval
  dup systime swap -
  tzoffset - timesplit -5 rotate pop pop pop pop 4 rotate pop
  "Up " swap 1 - intostr strcat " days, " strcat
  swap intostr strcat " hours, " strcat
  swap intostr strcat " minutes since " strcat
  swap "%H:%M %m/%d/%y" swap timefmt strcat .tell
;
