@prog con-recordhost
1 9999 d
1 i
( con-recordhost 1.0     records the last host you connected from )
$def DISPLAY_MESG "## You last connected from %n"
  
: recordhost
   me @ "@/host" getpropstr
$ifdef DISPLAY_MESG
   dup if DISPLAY_MESG over "%n" subst .tell then
$endif
   me @ "@/lasthost" rot 0 addprop
   me @ descriptors
   begin
      dup 1 > while
      rot pop 1 -
   repeat
   pop descrcon conhost
   me @ "@/host" rot 0 addprop
;
.
c
q
@register #prop #0:_connect con-recordhost=lasthost
@set con-recordhost=W
@set con-recordhost=L
