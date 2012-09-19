@prog cmd-bootall
1 9999 d
1 i
: bootall
  "Disconnecting: " swap strcat
  var mesg mesg !
  "me" match me !

  #-1 descr_array
  { descr }list
  array_diff

  foreach
	descrcon
	dup mesg @ connotify
	dup conboot
	pop
  repeat

  "Done." .tell
;
.
c
q
#ifdef NEW
@action bootall=me=tmp/bootall
@link $tmp/bootall=cmd-bootall
#endif
