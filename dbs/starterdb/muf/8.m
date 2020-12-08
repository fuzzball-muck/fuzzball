( ***** Misc String routines -- STR *****
 sms         [  str -- str'  ]  strips out mult. internal spaces
 fillfield   [  str char width -- padstr  ] return padding string to width chars
 STRparse    [  str -- str1 str2 str3  ] " #X Y  y = Z"  ->  "X" "Y y" " Z"
)
 
$doccmd @list __PROG__=!@1-5
 
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
 
public sms              $libdef sms
public fillfield        $libdef fillfield
public STRparse         $libdef STRparse
