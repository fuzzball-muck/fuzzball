@prog lib-strings
1 99999 d
1 i
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
public STRparse
.
c
q
@register lib-strings=lib/strings
@register #me lib-strings=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called stringslib
@set $tmp/prog1=/_defs/.asc:ctoi
@set $tmp/prog1=/_defs/.blank?:striplead not
@set $tmp/prog1=/_defs/.center:"%|*s" fmtstring
@set $tmp/prog1=/_defs/.chr:itoc dup not if pop "." then
@set $tmp/prog1=/_defs/.command_parse:"$lib/strings" match "STRparse" call
@set $tmp/prog1=/_defs/.fillfield:"$lib/strings" match "fillfield" call
@set $tmp/prog1=/_defs/.left:"%-*s" fmtstring
@set $tmp/prog1=/_defs/.right:"%*s" fmtstring
@set $tmp/prog1=/_defs/.rsplit:rsplit
@set $tmp/prog1=/_defs/.singlespace:"$lib/strings" match "sms" call
@set $tmp/prog1=/_defs/.sls:striplead
@set $tmp/prog1=/_defs/.sms:"$lib/strings" match "sms" call
@set $tmp/prog1=/_defs/.split:split
@set $tmp/prog1=/_defs/.strip:strip
@set $tmp/prog1=/_defs/.stripspaces:strip
@set $tmp/prog1=/_defs/.sts:striptail
@set $tmp/prog1=/_defs/STRasc:ctoi
@set $tmp/prog1=/_defs/STRblank?:striplead not
@set $tmp/prog1=/_defs/STRcenter:"%|*s" fmtstring
@set $tmp/prog1=/_defs/STRchr:itoc dup not if pop "." then
@set $tmp/prog1=/_defs/STRfillfield:"$lib/strings" match "fillfield" call
@set $tmp/prog1=/_defs/STRleft:"%-*s" fmtstring
@set $tmp/prog1=/_defs/STRparse:"$lib/strings" match "STRparse" call
@set $tmp/prog1=/_defs/STRright:"%*s" fmtstring
@set $tmp/prog1=/_defs/STRrsplit:rsplit
@set $tmp/prog1=/_defs/STRsinglespace:"$lib/strings" match "sms" call
@set $tmp/prog1=/_defs/STRsls:striplead
@set $tmp/prog1=/_defs/STRsms:"$lib/strings" match "sms" call
@set $tmp/prog1=/_defs/STRsplit:split
@set $tmp/prog1=/_defs/STRstrip:strip
@set $tmp/prog1=/_defs/STRsts:striptail
@set $tmp/prog1=/_docs:@list $lib/strings=1-29


