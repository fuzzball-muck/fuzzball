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
 
$doccmd @list __PROG__=!@1-51
$lib-version 1.2
 
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
 
public lmgr-clearelem           $libdef lmgr-clearelem
public lmgr-clearrange          $libdef lmgr-clearrange
public lmgr-deletelist          $libdef lmgr-deletelist
public lmgr-deleterange         $libdef lmgr-deleterange
public lmgr-extractrange        $libdef lmgr-extractrange
public lmgr-fullrange           $libdef lmgr-fullrange
public lmgr-getbrange           $libdef lmgr-getbrange
public lmgr-getcount            $libdef lmgr-getcount 
public lmgr-getelem             $libdef lmgr-getelem
public lmgr-getlist             $libdef lmgr-getlist
public lmgr-getrange            $libdef lmgr-getrange
public lmgr-insertrange         $libdef lmgr-insertrange
public lmgr-moverange           $libdef lmgr-moverange
public lmgr-putbrange           $libdef lmgr-putbrange
public lmgr-putelem             $libdef lmgr-putelem
public lmgr-putrange            $libdef lmgr-putrange
public lmgr-setcount            $libdef lmgr-setcount
 
$pubdef .lmgr-clearelem         __PROG__ "lmgr-clearelem" call
$pubdef .lmgr-clearrange        __PROG__ "lmgr-clearrange" call
$pubdef .lmgr-deletelist        __PROG__ "lmgr-deletelist" call
$pubdef .lmgr-deleterange       __PROG__ "lmgr-deleterange" call
$pubdef .lmgr-extractrange      __PROG__ "lmgr-extractrange" call
$pubdef .lmgr-fullrange         __PROG__ "lmgr-fullrange" call
$pubdef .lmgr-getbrange         __PROG__ "lmgr-getbrange" call
$pubdef .lmgr-getcount          __PROG__ "lmgr-getcount" call
$pubdef .lmgr-getelem           __PROG__ "lmgr-getelem" call
$pubdef .lmgr-getlist           __PROG__ "lmgr-getlist" call
$pubdef .lmgr-getrange          __PROG__ "lmgr-getrange" call
$pubdef .lmgr-insertrange       __PROG__ "lmgr-insertrange" call
$pubdef .lmgr-moverange         __PROG__ "lmgr-moverange" call
$pubdef .lmgr-putbrange         __PROG__ "lmgr-putbrange" call
$pubdef .lmgr-putelem           __PROG__ "lmgr-putelem" call
$pubdef .lmgr-putrange          __PROG__ "lmgr-putrange" call
$pubdef .lmgr-setcount          __PROG__ "lmgr-setcount" call
 
$pubdef LMGR-clearelem          __PROG__ "lmgr-clearelem" call
$pubdef LMGRclearrange          __PROG__ "lmgr-clearrange" call
$pubdef LMGRdeletelist          __PROG__ "lmgr-deletelist" call
$pubdef LMGRdeleterange         __PROG__ "lmgr-deleterange" call
$pubdef LMGRextractrange        __PROG__ "lmgr-extractrange" call
$pubdef LMGRfullrange           __PROG__ "lmgr-fullrange" call
$pubdef LMGRgetbrange           __PROG__ "lmgr-getbrange" call
$pubdef LMGRgetcount            __PROG__ "lmgr-getcount" call
$pubdef LMGRgetelem             __PROG__ "lmgr-getelem" call
$pubdef LMGRgetlist             __PROG__ "lmgr-getlist" call
$pubdef LMGRgetrange            __PROG__ "lmgr-getrange" call
$pubdef LMGRinsertrange         __PROG__ "lmgr-insertrange" call
$pubdef LMGRmoverange           __PROG__ "lmgr-moverange" call
$pubdef LMGRputbrange           __PROG__ "lmgr-putbrange" call
$pubdef LMGRputelem             __PROG__ "lmgr-putelem" call
$pubdef LMGRputrange            __PROG__ "lmgr-putrange" call
$pubdef LMGRsetcount            __PROG__ "lmgr-setcount" call
