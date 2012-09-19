@program lib-arrays
1 9999 d
1 i
(*
  Lib-Arrays v1.1
  Author: Chris Brine [Moose/Van]
   Email: ashitaka@home.com
    Date: August 15th, 2000
  
  Modified by Revar to take advantage of newer MUF primitives, etc.
  Extended by Revar <foxen@belfry.com>
 
  Note: These were only made for string-list one-dimensional arrays,
        and not dictionaries or any other kind.
  Note: Ansi codes are accepted in these functions.
  
  Lib-Arrays demands either ProtoMUCK v1.00+ or FuzzballMUCK v6.00b2+
  PS: Arrays were originaly constructed in FuzzballMUCK but brought
      over to ProtoMUCK during our update of the FB part of the code.
 
  Functions:
    ArrCommas [ a -- s ]
     - Takes an array of strings and returns a string with them seperated by commas.
    ArrLeft [ a @1 @2 icol schar -- a ]
     - Aligns all text in the given range to the left in the number of
        columns and with the given char.
    ArrRight [ a @1 @2 icol schar -- a ]
     - Same as ArrLeft, but it is aligned to the right instead.
    ArrCenter [ a @1 @2 icol schar -- a ]
     - Same as ArrLeft, but it is aligned to the center instead.
    ArrIndent [ a @1 @2 icol schar -- a ]
     - Indents the text in the given range by the number of columns
       given.
    ArrFormat [ a @1 @2 icol -- a ]
     - Formats the given range in the array to a specific number of columns; however, it will only
       seperate the line at the last space before icol... that way it doesn't cut it off in the
       middle of any word.
    ArrJoinRng [ a @1 @2 schar -- a ]
     - Joins a range of text.  schar is the seperating char.
    ArrList [ d a @1 @2 iBolLineNum -- ]
     - List the contents of the range in the array; if iBolLineNum is not equal to 0 then
       it will display the line numbers as well. 'd' is the object that the list is displayed to.
    ArrSearch [ @ a ? -- i ]
     - Searches through the array for any item containing '?' starting at the given index
    ArrCopy [ a @1 @2 @3 -- a ]
     - Copies the given range to the position of @3.
    ArrMove [ a @1 @2 @3 -- a ]
     - Moves the given range to the position of @3
    ArrPlaceItem [ @ a ? -- a ]
     - Place an item at the exact position, moving the old one that was there down
       [Ie. An object switcheroo after array_insertitem]
    ArrShuffle [ a -- a ]
     - Randomize the array.
    ArrReplace [ a @1 @2 s1 s2 -- a ]
     - Replace any 's1' text with 's2' in the given range for the array.
    ArrSort [ a i -- a ]
     - Sorts the array in order.  i = 1 for forward, i = 0 for reverse
    ArrMPIparse [ d a @1 @2 s i -- a ]
     - Parse the given lines in an array and returns the parsed lines.
      d = Object to apply permission checking to [or use for parseprop under FuzzballMUCKs since they do not have PARSEMPI]
      a = The starting array
     @1 = The first index marker to parse
     @2 = The last index marker to parse
      s = String containing the &how variable's contents
      i = Integer used for {delay} on whether it is shown to the player or room.
    ArrKey? [ a @ -- i ]
     - Checks to see if '@' is an index marker in the given array/dictionary.
 
    ArrProcess [ dict:arr addr:func -- dict:result ]
      - Runs every array element through a transforming function that can
        change either the value or the index each item.  The function should
        have the signature [ idx:index any:val -- idx:index' any:val' ]
      arr    = initial array list/dictionary to process.
      func   = address of function to call for each item
      result = resulting array with each element transformed.

    ArrFilter  [ dict:arr addr:func -- dict:passed dict:failed ]
      - Runs a filtering function on every array item, sorting the given
        array into two dictionaries:  Those items that passed the filter,
        and those that didn't.  The filtering function should have the
        signature [ idx:index any:val -- bool:passed ].  Both returned
        arrays _will_ be dictionaries, with the keys of each filtered
        array item matching those of the original array item.
      arr    = initial array list/dictionary to filter.
      func   = address of function to call for each item
      passed = resulting array with each passing element.
      failed = resulting array with each failing element.
*)
 
: ArrCommas ( a -- s )
    dup array_count 2 > if
        array_reverse dup 0 []
        swap 0 array_delitem array_reverse
        ", " array_join
        ", and " strcat
        swap strcat
    else
        ", and " array_join
    then
;
 
: ArrLeft[ arr:oldarr idx:startpos idx:endpos int:col str:char -- arr:newarr ]
    0 array_make var! newarray
    oldarr @
    FOREACH
        swap dup startpos @ >= swap endpos @ <= and if
            begin
                dup ansi_strlen col @ < while
                char @ strcat
            repeat
        then
        newarray @ array_appenditem newarray !
    REPEAT
    newarray @
;
 
: ArrRight[ arr:oldarr idx:startpos idx:endpos int:col str:char -- arr:newarr ]
   0 array_make var! newarray
   oldarr @
   FOREACH
      swap dup startpos @ >= swap endpos @ <= and if
         begin
            dup ansi_strlen col @ < while
            char @ swap strcat
         repeat
      then
      newarray @ array_appenditem newarray !
   REPEAT
   newarray @
;
 
: ArrCenter[ arr:oldarr idx:startpos idx:endpos int:col str:char -- arr:newarr ]
   0            var! idx
   0 array_make var! newarray
   oldarr @
   FOREACH
      swap dup startpos @ >= swap endpos @ <= and if
         begin
            dup ansi_strlen col @ < while
            idx @ if
               char @ swap strcat 0 idx !
            else
               char @ strcat 1 idx !
            then
         repeat
      then
      newarray @ array_appenditem newarray !
   REPEAT
   newarray @
;
 
: ArrIndent[ arr:oldarr idx:startpos idx:endpos int:col str:char -- arr:newarr ]
   0 array_make var! newarray
   oldarr @
   FOREACH
      swap dup startpos @ >= swap endpos @ <= and if
         1 col @ 1 FOR
            pop char @ swap strcat
         REPEAT
      then
      newarray @ array_appenditem newarray !
   REPEAT
   newarray @
;
 
: ArrFormat[ arr:oldarr idx:startpos idx:endpos int:col -- arr:newarr ]
   0 array_make var! newarray
   oldarr @
   FOREACH
      swap dup startpos @ >= swap endpos @ <= and if
         begin
            dup ansi_strlen col @ > while
            col @ ansi_strcut swap " " rsplit rot strcat swap
            newarray @ array_appenditem newarray !
         repeat
      then
      newarray @ array_appenditem newarray !
   REPEAT
   newarray @
;
 
: ArrJoinRng[ arr:oldarr idx:startpos idx:endpos str:char -- arr:newarr ]
    oldarr @ array_count var! insize
    var newarr
  
    oldarr @ -1 startpos @ -- array_getrange newarr !
  
    oldarr @ startpos @ endpos @ array_getrange
    char @ array_join newarr @ array_appenditem newarr !
  
    oldarr @ endpos @ ++ insize @ array_getrange
    newarr @ dup array_count swap array_setrange
;
 
: ArrList ( d a @1 @2 iBolLineNum -- )
   var dbobj var bolnum bolnum ! 4 rotate dbobj !
   over -1 = if pop pop 0 over array_count 1 - then
   3 pick array_count 1 < if pop pop pop exit then
   dup 0 < if pop 0 then
   over 0 < if swap pop 0 swap then
   3 pick array_count 1 - over over > if swap pop dup then
   3 pick over > if rot pop -3 rotate else pop then
   over over > if pop dup then
   array_getrange
   FOREACH
      bolnum @ if
         swap 1 + intostr "\[[1;37m" swap strcat "\[[0m: " strcat
         1 array_make 0 0 5 " " ArrRight array_vals pop
      else
         swap pop ""
      then
      swap strcat
      dbobj @ swap notify
   REPEAT
;
 
: ArrSearch[ idx:start arr:arr any:searchfor -- int:foundat ]
    array_findval
    foreach
        dup start @ >= if
            swap pop exit
        then
        pop pop
    repeat
    -1
;
  
 
: ArrCopy ( a @1 @2 @3 -- a )
   var! arrpos
   3 pick rot rot array_getrange arrpos @ swap array_insertrange
;
 
: ArrMove ( a @1 @2 @3 -- a )
   var! arrpos
   3 pick 3 pick 3 pick array_getrange -4 rotate array_delrange
   dup array_count arrpos @ < if dup array_count else arrpos @ then rot array_insertrange
;
 
: ArrPlaceItem ( @ a ? -- a )
   3 pick rot swap array_insertitem
   over over swap array_getitem
   3 pick 3 rotate swap array_delitem
   rot array_insertitem
;
 
: ArrShuffle ( a -- a )
   var newarray 0 array_make newarray !
   dup array_count not if exit then
   1 over array_count 1 FOR
      pop
      dup array_count random swap % over over array_getitem rot rot array_delitem swap newarray @ array_appenditem newarray !
   REPEAT
   pop newarray @
;
 
: ArrReplace ( a @1 @2 s1 s2 -- a )
   var oldtext oldtext ! var newtext newtext !
   var endpos endpos ! var firstpos firstpos !
   var newarray 0 array_make newarray !
   FOREACH
      swap dup firstpos @ >= swap endpos @ <= and if
         newtext @ oldtext @ subst
      then
      newarray @ array_appenditem newarray !
   REPEAT
   newarray @
;
 
: ArrMPIparse ( d a @1 @2 s i -- a )
   var imesg imesg ! var stype stype !
   var endpos endpos ! var firstpos firstpos !
   var dbobj swap dbobj !
   var newarray 0 array_make newarray !
   FOREACH
      swap dup firstpos @ >= swap endpos @ <= and if
$ifdef __proto
         dbobj @ swap stype @ imesg @ parsempi
$else
         "@/mpi/" systime intostr strcat dup rot
         dbobj @ rot rot setprop
         dbobj @ over stype @ imesg @ parseprop
         dbobj @ swap remove_prop
$endif
         newarray @ array_appenditem newarray !
      swap
         pop
      then
   REPEAT
   newarray @
;
 
: ArrKey? ( a @ -- i )
   over dictionary? if
      swap array_keys array_make swap dup int? not
      over string? not and if pop pop 0 exit then
      array_findval array_count 0 >
   else
      dup int? not if pop pop 0 exit then
      swap array_count over swap < swap 0 >= and
   then
;
 
  
(dir = 1 for forward, dir = 0 for reverse)
: ArrSort[ arr:oldarr int:dir -- arr:newarr ]
    (FIXME: allow for lexical-numerical sorting)
    if SORTTYPE_CASE_ASCEND else SORTTYPE_CASE_DESCEND then
    array_sort
;
  
: ArrProcess ( dict:arr addr:func -- dict:result )
    0 array_make swap
    foreach
        4 pick execute
        rot rot array_setitem
    repeat
    swap pop
;
  
: ArrFilter (dict:arr addr:func -- dict:result dict:remainder)
    0 array_make_dict dup rot (addr arr' arr2' arr)
    foreach                   (addr arr' arr2' idx val)
        over over 7 pick execute if
            4 rotate rot array_setitem swap
        else
            rot rot array_setitem
        then
    repeat
    rot pop
;
  
public ArrCommas ( a -- s )
public ArrLeft ( a @1 @2 icol schar -- a )
public ArrRight ( a @1 @2 icol schar -- a )
public ArrCenter ( a @1 @2 icol schar -- a )
public ArrIndent ( a @1 @2 icol schar -- a )
public ArrFormat ( a @1 @2 icol -- a )
public ArrJoinRng ( a @1 @2 schar -- a )
public ArrList ( d a @1 @2 iBolLineNum -- )
public ArrSearch ( @ a ? -- i )
public ArrCopy ( a @1 @2 @3 -- a )
public ArrMove ( a @1 @2 @3 -- a )
public ArrPlaceItem ( @ a ? -- a )
public ArrShuffle ( a -- a )
public ArrReplace ( a @1 @2 s1 s2 -- a )
public ArrSort ( a i -- a )
public ArrMPIparse ( d a @1 @2 s i -- a )
public ArrKey? ( a @ -- i )
  
public ArrProcess ( dict:arr addr:func -- dict:result )
public ArrFilter  ( dict:arr addr:func -- dict:result dict:remainder )
  
$lib-version 1.1
  
$pubdef ArrCommas    "$lib/arrays" match "ArrCommas" call
$pubdef ArrLeft      "$lib/arrays" match "ArrLeft" call
$pubdef ArrRight     "$lib/arrays" match "ArrRight" call
$pubdef ArrCenter    "$lib/arrays" match "ArrCenter" call
$pubdef ArrIndent    "$lib/arrays" match "ArrIndent" call
$pubdef ArrFormat    "$lib/arrays" match "ArrFormat" call
$pubdef ArrJoinRng   "$lib/arrays" match "ArrJoinRng" call
$pubdef ArrList      "$lib/arrays" match "ArrList" call
$pubdef ArrSearch    "$lib/arrays" match "ArrSearch" call
$pubdef ArrCopy      "$lib/arrays" match "ArrCopy" call
$pubdef ArrMove      "$lib/arrays" match "ArrMove" call
$pubdef ArrPlaceItem "$lib/arrays" match "ArrPlaceItem" call
$pubdef ArrShuffle   "$lib/arrays" match "ArrShuffle" call
$pubdef ArrReplace   "$lib/arrays" match "ArrReplace" call
$pubdef ArrSort      "$lib/arrays" match "ArrSort" call
$pubdef ArrMPIparse  "$lib/arrays" match "ArrMPIparse" call
$pubdef ArrKey?      "$lib/arrays" match "ArrKey?" call
  
$pubdef ArrProcess   "$lib/arrays" match "ArrProcess" call
$pubdef ArrFilter    "$lib/arrays" match "ArrFilter" call
  
$pubdef Array_Commas    "$lib/arrays" match "ArrCommas" call
$pubdef Array_Left      "$lib/arrays" match "ArrLeft" call
$pubdef Array_Right     "$lib/arrays" match "ArrRight" call
$pubdef Array_Center    "$lib/arrays" match "ArrCenter" call
$pubdef Array_Indent    "$lib/arrays" match "ArrIndent" call
$pubdef Array_Format    "$lib/arrays" match "ArrFormat" call
$pubdef Array_JoinRng   "$lib/arrays" match "ArrJoinRng" call
$pubdef Array_List      "$lib/arrays" match "ArrList" call
$pubdef Array_Search    "$lib/arrays" match "ArrSearch" call
$pubdef Array_Copy      "$lib/arrays" match "ArrCopy" call
$pubdef Array_Move      "$lib/arrays" match "ArrMove" call
$pubdef Array_PlaceItem "$lib/arrays" match "ArrPlaceItem" call
$pubdef Array_Shuffle   "$lib/arrays" match "ArrShuffle" call
$pubdef Array_Replace   "$lib/arrays" match "ArrReplace" call
$pubdef Array_MPIparse  "$lib/arrays" match "ArrMPIparse" call
  
$pubdef Array_Process   "$lib/arrays" match "ArrProcess" call
$pubdef Array_Filter    "$lib/arrays" match "ArrFilter" call
.
c
q
@register lib-arrays=lib/arrays
@set $lib/arrays=l
@set $lib/arrays=m3
@set $lib/arrays=v
