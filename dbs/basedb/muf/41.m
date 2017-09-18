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
   pop descrhost
   me @ "@/host" rot 0 addprop
;
