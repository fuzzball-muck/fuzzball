@prog lib-lmgr
1 99999 d
1 i
( ***** List Manager Object - LMGR *****  Version 1.2
  
 LMGR-ClearElem -- Clears an element in the list -- does NOT delete
     <elem#> <list-name> <dbref> LMGRclearelem
  
 LMGR-GetElem -- Get an element of a list
     <elem#> <list-name> <dbref> LMGRgetelem -- string
  
 LMGR-PutElem -- Put an element into a list
     <val> <elem#> <list-name> <dbref> LMGRputelem
  
 LMGR-GetRange -- Get a range of elements from a list
     <count> <first-elem> <list-name> <dbref> LMGRgetrange -- {strrange}
   returns the element values [strings] on the stack, with <count> on top
  
 LMGR-FullRange -- Define entire list for getrange purposes
     <list-name> <dbref> LMGRfullrange -- <num-elements> 1 <list-name> <dbref>
   returns the parms on the stack, ready for LMGR-GetRange
  
 LMGR-GetBRange -- Get a range of elements from a list
              Different from 'GetRange' in that the top element on the
              stack is the first element from the range.
     <count> <first-elem> <list-name> <dbref> LMGRgetbrange -- {bstrrange}
   returns the element values [strings] on the stack, with <count> on top
  
 LMGR-PutRange -- Put a range of elements into a list
     <values> <count> <first-elem> <list-name> <dbref> LMGRputrange
  
 LMGR-ClearRange -- Clears a range of elements in the list -- does NOT delete
     <count> <first-elem> <list-name> <dbref> LMGRclearrange
  
 LMGR-DeleteRange -- Delete a range of elements from the list, shifting the
                later elements back to fill the gap.
     <count> <first-elem> <list-name> <dbref> LMGRdeleterange
  
 LMGR-InsertRange -- Insert a range of elemnts into a list
     <values> <count> <first-elem> <list-name> <dbref> LMGRinsertrange
  
 LMGR-MoveRange -- Move [copy] a range of elements inside a list
     <dest> <count> <source> <list-name> <dbref> LMGRmoverange
  
 LMGR-CopyRange -- Copy a range of elements from one list into another,
              inserting into the new list
     <dst> <cnt> <src> <src-lst> <src-ref> <dst-lst> <dst-ref> LMGRcopyrange
  
 LMGR-DeleteList -- Delete an entire list.
     <list-name> <dbref> LMGRdeletelist
  
 LMGR-Getlist -- Get an entire list.
     <list-name> <dbref> LMGRgetlist
)
  
(standard list writing format)
$def COUNTSUFFIX "#"
$def ITEMNUMSEP "#/"   ( "" in old format )
  
  
: safeclear (d s -- )
  over over propdir? if
    over over "" -1 addprop
    "" 0 addprop
  else
    remove_prop
  then
;
  
  
: lmgr-getoldelem (elem list db -- str)
  swap rot intostr strcat getpropstr
;
  
: lmgr-getmidelem ( elem list db -- str )
  swap "/" strcat rot intostr strcat getpropstr
;
  
: lmgr-getnewelem ( elem list db -- str )
  swap "#/" strcat rot intostr strcat getpropstr
;
  
: lmgr-getelem (elem list db -- str)
  "isd" checkargs
  3 pick 3 pick 3 pick lmgr-getnewelem
  dup if -4 rotate pop pop pop exit then
  pop 3 pick 3 pick 3 pick lmgr-getmidelem
  dup if -4 rotate pop pop pop exit then
  pop lmgr-getoldelem
;
  
  
: lmgr-setcount ( count list db -- )
  "isd" checkargs
  swap COUNTSUFFIX strcat rot dup if
    intostr 0 addprop
  else
    pop remove_prop
  then
;
  
: lmgr-getnewcount ( list db -- count )
  swap "#" strcat getpropstr atoi
;
  
: lmgr-getoldcount ( list db -- count )
  swap "/#" strcat getpropstr atoi
;
  
: lmgr-getnocount-loop ( item list db -- count )
  3 pick 3 pick 3 pick lmgr-getelem
  not if pop pop 1 - exit then
  rot 1 + rot rot
  lmgr-getnocount-loop
;
  
: lmgr-getnocount ( list db -- count )
  1 rot rot lmgr-getnocount-loop
;
  
: lmgr-getcount (list db -- count)
  "sd" checkargs
  over over lmgr-getnewcount
  dup if rot rot pop pop exit then
  pop over over lmgr-getoldcount
  dup if rot rot pop pop exit then
  pop lmgr-getnocount
;
  
  
: lmgr-putelem ( str elem list db -- )
  "sisd" checkargs
  over over LMGR-GETCOUNT 4 pick < if
    3 pick 3 pick 3 pick LMGR-SETCOUNT
  then
  swap ITEMNUMSEP strcat rot intostr strcat rot 0 addprop
;
  
: lmgr-clearelem ( elem list db -- )
  "isd" checkargs
  dup 3 pick 5 pick intostr strcat remove_prop
  dup 3 pick "/" strcat 5 pick intostr strcat remove_prop
  swap "#/" strcat rot intostr strcat remove_prop
;
  
  
: lmgr-getrange_loop ( ... count count first name db -- elems... n )
  4 rotate dup if
( count first name db count )
    1 - -4 rotate
( count count-1 first name db )
    rot dup 4 pick 4 pick LMGR-GETELEM
( count count-1 name db first elem )
    -6 rotate 1 + -3 rotate
( elem count count-1 first+1 name db )
    'lmgr-getrange_loop jmp
( elem ... count )
  else
( ... count first name db 0 )
    pop pop pop pop
  then
;
  
: lmgr-getrange ( count first name db -- elems... n )
  "iisd" checkargs
  4 pick -5 rotate lmgr-getrange_loop
;
 
: lmgr-fullrange ( list obj -- count start list obj )
  "sd" checkargs
  over over lmgr-getcount -3 rotate 1 -3 rotate
;
  
: lmgr-getbrange_loop ( ... count count first name db -- elems... n )
  4 rotate dup if
( count first name db count )
    1 - -4 rotate
( count count-1 first name db )
    rot 1 - dup 4 pick 4 pick LMGR-GETELEM
( count count-1 name db first-1 elem )
    -6 rotate -3 rotate
( elem count count-1 first-1 name db )
    'lmgr-getbrange_loop jmp
( elem ... count )
  else
( ... count first name db 0 )
    pop pop pop pop
  then
;
  
: lmgr-getbrange ( count first name db -- elems... n )
  "iisd" checkargs
  rot 4 pick dup -6 rotate + -3 rotate lmgr-getbrange_loop
;
  
  
: lmgr-putrange_loop ( elems... count first name db which -- )
  5 pick over over over = if
( count first name db count count count )
    pop pop pop pop pop pop pop
( )
  else
( elems... count first name db which count which )
    - 5 + rotate
( elems... count first name db which elem )
    over 6 pick + 5 pick 5 pick LMGR-PUTELEM
( elems... count first name db which )
    1 + 'lmgr-putrange_loop jmp
( )
  then
;
  
: lmgr-putrange ( elems... count first name db -- )
  "{s}isd" checkargs
  0 lmgr-putrange_loop
;
  
: lmgr-putbrange ( elems... count first name db -- )
  "{s}isd" checkargs
  4 rotate dup if
( elems... first name db count )
    1 - -4 rotate
( elems... count first name db )
    5 rotate 4 pick 4 pick 4 pick LMGR-PUTELEM
( elems... count first name db )
    rot 1 + -3 rotate
( elems... count first name db )
    'lmgr-putbrange jmp
( )
  else
( 0 first name db )
    pop pop pop pop
( )
  then
;
  
: lmgr-clearrange ( count first name db -- )
  "iisd" checkargs
  4 rotate dup if
( first name db count )
    1 - -4 rotate
( count first name db )
    rot dup 4 pick 4 pick LMGR-CLEARELEM
( count name db first )
    1 + -3 rotate
( count first+1 name db )
    'lmgr-clearrange jmp
( )
  else
( first name db 0 )
    pop pop pop pop
( )
  then
;
  
  
: lmgr-moverange_loop ( dest count src name db inc -- )
  5 rotate dup if
( dest src name db inc count )
    1 - -5 rotate
( dest count-1 src name db inc )
    4 rotate dup 5 pick 5 pick LMGR-GETELEM
( dest count-1 name db inc src elem )
    7 rotate swap over 7 pick 7 pick LMGR-PUTELEM
( count-1 name db inc src dest )
    3 pick + -6 rotate
( dest+inc count-1 name db inc src )
    over + -4 rotate
( dest+inc count-1 src+inc name db inc )
    'lmgr-moverange_loop jmp
( )
  else
( dest src name db 0 inc )
    pop pop pop pop pop pop
( )
  then
;
  
: lmgr-moverange ( dest count src name db -- )
  "iiisd" checkargs
  5 rotate 4 rotate over over < if
( count name db dest src )
    -4 rotate -5 rotate 1
( count name db dest src inc )
  else
( count name db dest src )
    5 pick + 1 - -4 rotate
( count src+count-1 name db dest )
    5 pick + 1 - -5 rotate
( dest+count-1 count src+count-1 name db )
    -1
( dest+count-1 count src+count-1 name db inc )
  then
( dest count src name db inc )
  lmgr-moverange_loop
( )
;
  
: lmgr-insertrange ( elem-1 ... elem-n count first list db -- )
  "{s}isd" checkargs
  3 pick 5 pick over + swap
( elem-1 ... elem-n count first list db first+count first )
  4 pick 4 pick LMGR-GETCOUNT
( elem-1 ... elem-n count first list db first+count first list-count )
  over - 1 + swap
( elem-1 ... elem-n count first list db first+count range-count first )
  5 pick 5 pick LMGR-MOVERANGE
( elem-1 ... elem-n count first list db )
  LMGR-PUTRANGE
( )
;
  
: lmgr-deleterange ( count first list db -- )
  "iisd" checkargs
  over over LMGR-GETCOUNT
( count first list db list-count )
  4 pick 6 pick over +
( count first list db list-count first first+count )
  3 pick
( count first list db list-count first first+count list-count )
  over - 1 + swap
( count first list db list-count first range-count first+count )
  6 pick 6 pick LMGR-MOVERANGE
( count first list db list-count )
  5 rotate swap over - 1 +
( first list db count delstart )
  1 - 4 rotate 4 rotate 4 pick 4 pick 1 + 4 pick 4 pick LMGR-CLEARRANGE
( first count delstart list db )
  LMGR-SETCOUNT pop pop
( )
;
  
  
: lmgr-extractrange ( count first list db -- elem-1 ... elem-n n )
  "iisd" checkargs
  4 pick 4 pick 4 pick 4 pick LMGR-GETRANGE
( count first list db elem-1 ... elem-n n )
  dup 5 + rotate over 5 + rotate 3 pick 5 + rotate 4 pick 5 + rotate
( elem-1 ... elem-n n count first list db )
  LMGR-DELETERANGE
( elem-1 ... elem-n n )
;
  
  
: LMGR-deletelist
  "sd" checkargs
  over over LMGR-getcount
  1 4 rotate 4 rotate LMGR-deleterange
;
  
  
: LMGR-getlist
  "sd" checkargs
  over over LMGR-getcount
  rot rot 1 rot rot
  LMGR-getrange
;
  
  
PUBLIC lmgr-getcount
PUBLIC lmgr-setcount
PUBLIC lmgr-getelem
PUBLIC lmgr-putelem
PUBLIC lmgr-clearelem
PUBLIC lmgr-getrange
PUBLIC lmgr-fullrange
PUBLIC lmgr-getbrange
PUBLIC lmgr-putrange
PUBLIC lmgr-putbrange
PUBLIC lmgr-clearrange
PUBLIC lmgr-moverange
PUBLIC lmgr-insertrange
PUBLIC lmgr-deleterange
PUBLIC lmgr-extractrange
PUBLIC lmgr-deletelist
PUBLIC lmgr-getlist
.
c
q
@register lib-lmgr=lib/lmgr
@register #me lib-lmgr=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=B
@set $tmp/prog1=H
@set $tmp/prog1=S
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called lib-lmgr
@set $tmp/prog1=/_defs/.lmgr-clearelem:"$lib/lmgr" match "lmgr-clearelem" call
@set $tmp/prog1=/_defs/.lmgr-clearrange:"$lib/lmgr" match "lmgr-clearrange" call
@set $tmp/prog1=/_defs/.lmgr-deletelist:"$lib/lmgr" match "lmgr-deletelist" call
@set $tmp/prog1=/_defs/.lmgr-deleterange:"$lib/lmgr" match "lmgr-deleterange" call
@set $tmp/prog1=/_defs/.lmgr-extractrange:"$lib/lmgr" match "lmgr-extractrange" call
@set $tmp/prog1=/_defs/.lmgr-fullrange:"$lib/lmgr" match "lmgr-fullrange" call
@set $tmp/prog1=/_defs/.lmgr-getbrange:"$lib/lmgr" match "lmgr-getbrange" call
@set $tmp/prog1=/_defs/.lmgr-getcount:"$lib/lmgr" match "lmgr-getcount" call
@set $tmp/prog1=/_defs/.lmgr-getelem:"$lib/lmgr" match "lmgr-getelem" call
@set $tmp/prog1=/_defs/.lmgr-getlist:"$lib/lmgr" match "lmgr-getlist" call
@set $tmp/prog1=/_defs/.lmgr-getrange:"$lib/lmgr" match "lmgr-getrange" call
@set $tmp/prog1=/_defs/.lmgr-insertrange:"$lib/lmgr" match "lmgr-insertrange" call
@set $tmp/prog1=/_defs/.lmgr-moverange:"$lib/lmgr" match "lmgr-moverange" call
@set $tmp/prog1=/_defs/.lmgr-putbrange:"$lib/lmgr" match "lmgr-putbrange" call
@set $tmp/prog1=/_defs/.lmgr-putelem:"$lib/lmgr" match "lmgr-putelem" call
@set $tmp/prog1=/_defs/.lmgr-putrange:"$lib/lmgr" match "lmgr-putrange" call
@set $tmp/prog1=/_defs/.lmgr-setcount:"$lib/lmgr" match "lmgr-setcount" call
@set $tmp/prog1=/_defs/lmgr-clearelem:"$lib/lmgr" match "lmgr-clearelem" call
@set $tmp/prog1=/_defs/lmgr-clearrange:"$lib/lmgr" match "lmgr-clearrange" call
@set $tmp/prog1=/_defs/lmgr-deletelist:"$lib/lmgr" match "lmgr-deletelist" call
@set $tmp/prog1=/_defs/lmgr-deleterange:"$lib/lmgr" match "lmgr-deleterange" call
@set $tmp/prog1=/_defs/lmgr-extractrange:"$lib/lmgr" match "lmgr-extractrange" call
@set $tmp/prog1=/_defs/lmgr-fullrange:"$lib/lmgr" match "lmgr-fullrange" call
@set $tmp/prog1=/_defs/lmgr-getbrange:"$lib/lmgr" match "lmgr-getbrange" call
@set $tmp/prog1=/_defs/lmgr-getcount:"$lib/lmgr" match "lmgr-getcount" call
@set $tmp/prog1=/_defs/lmgr-getelem:"$lib/lmgr" match "lmgr-getelem" call
@set $tmp/prog1=/_defs/lmgr-getlist:"$lib/lmgr" match "lmgr-getlist" call
@set $tmp/prog1=/_defs/lmgr-getrange:"$lib/lmgr" match "lmgr-getrange" call
@set $tmp/prog1=/_defs/lmgr-insertrange:"$lib/lmgr" match "lmgr-insertrange" call
@set $tmp/prog1=/_defs/lmgr-moverange:"$lib/lmgr" match "lmgr-moverange" call
@set $tmp/prog1=/_defs/lmgr-putbrange:"$lib/lmgr" match "lmgr-putbrange" call
@set $tmp/prog1=/_defs/lmgr-putelem:"$lib/lmgr" match "lmgr-putelem" call
@set $tmp/prog1=/_defs/lmgr-putrange:"$lib/lmgr" match "lmgr-putrange" call
@set $tmp/prog1=/_defs/lmgr-setcount:"$lib/lmgr" match "lmgr-setcount" call
@set $tmp/prog1=/_docs:@list $lib/lmgr=1-50
@set $tmp/prog1=/_lib-version:1.2
