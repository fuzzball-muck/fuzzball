( ***** Misc String routines -- STR *****
These routines deal with spaces in strings.
 STRblank?   [       str -- bool         ]  true if str null or only spaces
 STRsls      [       str -- str'         ]  strip leading spaces
 STRsts      [       str -- str'         ]  strip trailing spaces
 STRstrip    [       str -- str'         ]  strip lead and trail spaces
 STRsms      [       str -- str'         ]  strips out mult. internal spaces
  
These two are routines to split a string on a substring, non-inclusive.
 STRsplit    [ str delim -- prestr postr ]  splits str on first delim. nonincl.
 STRrsplit   [ str delim -- prestr postr ]  splits str on last delim. nonincl.
  
The following are useful for formatting strings into fields.
 STRfillfield [str char width -- padstr  ] return padding string to width chars
 STRcenter   [ str width -- str'         ]  center a string in a field.
 STRleft     [ str width -- str'         ]  pad string w/ spaces to width chars
 STRright    [ str width -- str'         ]  right justify string to width chars
  
The following are case insensitive versions of instr and rinstr:
 instring    [  str str2 -- position     ]  find str2 in str and return pos
 rinstring   [  str str2 -- position     ]  find last str2 in str & return pos
  
These convert between ascii integers and string character.
 STRasc      [      char -- i            ]  convert character to ASCII number
 STRchar     [         i -- char         ]  convert number to character
  
This routine is useful for parsing command line input:
  STRparse   [       str -- str1 str2 str3] " #X Y  y = Z"  ->  "X" "Y y" " Z"
)
 
$doccmd @list __PROG__=!@1-29
 
: sms ( str -- str')
  begin
    dup "  " instr while
    " " "  " subst
  repeat
;
 
: fillfield (str padchar fieldwidth -- padstr)
  rot ansi_strlen - dup 1 < if pop pop "" exit then
  swap over begin swap dup strcat swap 2 / dup not until pop
  swap ansi_strcut pop
;
 
: STRparse ( s -- s1 s2 s3 )
  (
    Before: " #option  tom dick  harry = message "
    After:  "option" "tom dick harry" " message "
  )
  "=" rsplit swap
  striplead dup "#" 1 strncmp not if
    1 strcut swap pop
    " " split
  else
    "" swap
  then
  strip sms rot
;
 
public sms
public fillfield
public STRparse         $libdef STRparse
 
$pubdef .asc            ctoi
$pubdef .blank?         striplead not
$pubdef .center         "%|*s" fmtstring
$pubdef .chr            itoc dup not if pop "." then
$pubdef .command_parse  __PROG__ "STRparse" call
$pubdef .fillfield      __PROG__ "fillfield" call
$pubdef .left           "%-*s" fmtstring
$pubdef .right          "%*s" fmtstring
$pubdef .rsplit         rsplit
$pubdef .singlespace    __PROG__ "sms" call
$pubdef .sls            striplead
$pubdef .sms            __PROG__ "sms" call
$pubdef .split          split
$pubdef .strip          strip
$pubdef .stripspaces    strip
$pubdef .sts            striptail
 
$pubdef STRasc          ctoi
$pubdef STRblank?       striplead not
$pubdef STRcenter       "%|*s" fmtstring
$pubdef STRchr          itoc dup not if pop "." then
$pubdef STRfillfield    __PROG__ "fillfield" call
$pubdef STRleft         "%-*s" fmtstring
$pubdef STRright        "%*s" fmtstring
$pubdef STRrsplit       rsplit
$pubdef STRsinglespace  __PROG__ "sms" call
$pubdef STRsls          striplead
$pubdef STRsms          __PROG__ "sms" call
$pubdef STRsplit        split
$pubdef STRstrip        strip
$pubdef STRsts          striptail
