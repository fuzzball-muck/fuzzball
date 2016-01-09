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
  
  
: split
    swap over over swap
    instr dup not if
        pop swap pop ""
    else
        1 - strcut rot
        strlen strcut
        swap pop
    then
;
  
  
: rsplit
    swap over over swap
    rinstr dup not if
        pop swap pop ""
    else
        1 - strcut rot
        strlen strcut
        swap pop
    then
;
  
  
: sms ( str -- str')
    dup "  " instr if
        " " "  " subst 'sms jmp
    then
;
  
  
: fillfield (str padchar fieldwidth -- padstr)
  rot strlen - dup 1 < if pop pop "" exit then
  swap over begin swap dup strcat swap 2 / dup not until pop
  swap strcut pop
;
  
: left (str fieldwidth -- str')
  over " " rot fillfield strcat
;
  
: right (str fieldwidth -- str')
  over " " rot fillfield swap strcat
;
  
: center (str fieldwidth -- str')
  over " " rot fillfield
  dup strlen 2 / strcut
  rot swap strcat strcat
;
  
  
: STRasc ( c -- i )
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" strcat
    swap instr dup if
        31 +
    then
;
  
: STRchr ( i -- c )
    dup 31 > over 128 < and if
        " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" strcat
        swap 32 - strcut swap pop 1 strcut pop
    else
        pop "."
    then
;
  
: STRparse ( s -- s1 s2 s3 ) (
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
    
  
public split
public rsplit
public sms
public fillfield
public left
public right
public center
public STRasc
public STRchr
public STRparse
