@prog MOSS-mailconvert-pmail
1 99999 d
i
( This is a simple <HA> program to convert page #mail to MOSS mail )
( Programs similar to this one HAVE to be careful on variable use, since it )
( executes a function in newmail, it needs to leave at LEAST 8 local )
( variables untouched, so I'm using $def varname num localvariable to )
( be on the safe side )
$include $lib/strings
( Set this to "page-mail#" for older versions of page )
$def pageprop "_page/"
$def mailprop "_page/mail#"
$def refto 15 localvar
$def converter 16 localvar
 
( Code borrowed from cmd-page )
 
: oproploc ( dbref -- dbref' )
    dup "_proploc" getpropstr
    dup if
        dup "#" 1 strncmp not if
            1 strcut swap pop
        then
        atoi dbref
        dup ok? if
            dup owner 3 pick
            dbcmp if swap then
        else swap
        then pop
    else pop
    then
;
 
( encryption stuff )
 
: transpose (char -- char')
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz 1234567890_"
    over instr dup if
        swap pop 1 -
        "wG8D kBQzWm4gbRXHOqaZiJPtUTN2pu6M0VjFlK3sdS9oYe5A_7IE1cnLvfyCrhx"
        swap strcut 1 strcut pop swap pop
    else
        pop
    then
;
 
: encrypt-charloop (nullstr string -- string')
    dup not if pop exit then
    1 strcut swap transpose
    rot swap strcat swap
    encrypt-charloop
;
 
: encrypt-loop (nullstr string -- string')
    dup not if pop exit then
    100 strcut "" rot
    encrypt-charloop
    rot swap strcat
    swap encrypt-loop
;
 
: encrypt (string -- string')
    "" swap encrypt-loop
;
 
( better encryption. But slower. )
 
: asc (stringchar -- int)
    dup if
      " 1234567890-=!@#$%&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZXCVBNM<>?\"~\\|^"
      swap instr 1 - exit
    then pop 0
;
 
: chr (int -- strchar)
    " 1234567890-=!@#$%&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZXCVBNM<>?\"~\\|^"
    swap strcut 1 strcut pop swap pop
;
 
: cypher (key chars -- chars')
    1 strcut asc swap asc
    over 89 > over 89 > or if chr swap chr strcat swap pop exit then
    dup 10 / 10 *
    4 pick 10 + rot 10 % - 10 %
    rot dup 10 / 10 *
    5 rotate 10 + rot 10 % - 10 %
    4 rotate + chr -3 rotate + chr strcat
;
 
: crypt2-loop (key strcrypt strnorm -- strcrypt)
    dup not if pop swap pop exit then
    2 strcut 4 pick rot cypher
    rot swap strcat swap
    crypt2-loop
;
 
: crypt2-loop2 (key strcrypt strnorm -- strcrypt)
    dup strlen 200 < if crypt2-loop exit then
    200 strcut swap 4 pick 4 rotate rot crypt2-loop
    swap crypt2-loop2
;
 
: crypt2 (key string -- string')
    swap 9 % 100 + "" rot crypt2-loop2
;
 
: getday ( -- int)
  systime dup 86400 % 86400 + time 60 * + 60 * + - 86400 % - 86400 /
;
 
( And my own stuff )
 
: mailparse ( s -- d readat day time s )
( "#dbref day[*readat]@hh:mm:ss ?encryptedmesg" )
  " " STRsplit swap
  dup "#" 1 strncmp not if
    1 strcut swap pop atoi dbref swap
    "@" STRsplit swap
    dup "*" instr if "*" STRsplit atoi swap else 0 swap then
    atoi rot " " STRsplit swap
    ":" STRsplit swap atoi 60 * swap
    ":" STRsplit swap atoi rot + 60 * swap
    atoi + swap
    dup "C" 1 strncmp not if
      1 strcut swap pop encrypt
    else
      dup "D" 1 strncmp not if
        1 strcut swap pop refto @ int swap crypt2
      then
    then
  else
    "Unhandled mail type: " swap strcat " " strcat swap strcat
    refto @ 0 0 0 5 rotate
( "Name (??HH:MM:SS> -- Message" )
  then
;
 
: daytime2systime ( day timesecs -- time )
  time 60 * + 60 * + swap - swap getday swap - 86400 * + systime swap -
;
 
: mailconvert ( d a -- )
  converter ! refto !
  refto @ oproploc dup mailprop getpropstr atoi 1 begin over while
    3 pick mailprop "/" strcat 3 pick intostr strcat getpropstr mailparse
    rot rot daytime2systime
    refto @ int intostr 5 rotate "Page-mail" 5 rotate 1 6 rotate 7 rotate
    dup if 1 else 0 then converter @ execute
    3 pick mailprop "/" strcat 3 pick intostr strcat remove_prop
    1 + swap 1 - swap
  repeat pop pop dup mailprop remove_prop pageprop "forward" strcat
  over over getpropstr rot rot remove_prop
  refto @ "_prefs/mail/forward" rot setprop
;
.
c
q
@set MOSS-mailconvert-pmail=!d
@set MOSS-mailconvert-pmail=!z
@set MOSS-mailconvert-pmail=!l
@set MOSS-mailconvert-pmail=w
