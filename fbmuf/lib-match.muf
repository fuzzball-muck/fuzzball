@prog lib-match
1 99999 d
1 i
( Matching library
  .noisy_match  [ itemname -- itemdbref ]
    Takes a string with a possible item name in it.  It returns the
      dbref of the object is if is found.  If it is not found, it tells
      the user that it doesn't see that here and returns #-1.  If it
      finds several matches it tells the user that it doesn't know which
      item was meant, and returns #-2.
  
  .noisy_pmatch   [ playername -- playerdbref]
    This routine takes a string with a possible playername and returns
      the dbref of that player, if it can find it.  If it cannot find the
      player, then it tells the user that it doesn't know who that is,
      and it returns #-1.
  
  .controls  [ playerdbref objectdbref -- controlled? ]
    This routine takes a player dbref and an object dbref and returns an
      integer value of 1 if the player controls that object.  Otherwise
      it returns a 0.
  
  .match_controlled  [ itemname -- itemdbref ]
    This routine basically does a .noisy_match, and checks that what is
      returned is controlled by the user.  If it is not, then it tells the
      user Permission Denied, and returns #-1.  Otherwise it returns the
      dbref of the item matched.
  
  .multi_rmatch  [objref smatchstr -- dn .. d1 n]
      This function takes a dbref for the thing/player/room that it is
      to match in, and a smatch style comparison string and returns the
      dbrefs of all the contained objects within who's names matched the
      string.  There is an integer on top giving how many dbrefs were
      returned.  If no items were matched, it only returns a 0.
  
  .table_match  [ xnone xambig sn xn .. s1 x1 n comp func -- smat xmat ]
      This function takes, in order:
          - a data value of any type to return if no matches are made.
              {xnone}
          - a data value of any type to return if the match is amiguous.
              {xambig}
          - a range of comparator {sn - s1}, and data {xn - x1} pairs of
              any type.
          - an integer count of how many comparator-data pairs are on
              the stack to be compared against. {n}
          - a value of the same type as the comparators, that will be
              checked against each comparator.  {comp}
          - the address of the comparator function that is used to compare
              comp against s1 through sn.  This function should take the
              two datums for comparison and return a 1 for a match or a 0
              for a non-match.  {func}
        This function tests comp against s1 through sn, returning the
          matching compatator-data pair if it finds one match.  It returns
          a null string and xnone if no matches are found.  It returns a
          null string and xambig if more than one match was found.
  
  .std_table_match
    This function runs .table_match with a standard comparator fuction that
      expects the comparators to be strings.  The match comparator routine
      matches if comp matches the beginning of the comparator exactly.
      ie:  a comp of "#h" would match a comparator {s1 - sn} of "#help".
)
 
$include $lib/stackrng
 
: noisy_match (s -- d)
  dup if match else pop #-1 then
  dup not if
    me @ "I don't see that here!" notify exit
  then
  dup #-2 dbcmp if
    me @ "I don't know which one you mean!" notify exit
  then
;
  
  
: noisy_pmatch ( s -- d )
  .pmatch dup not if
    me @ "I don't recognize anyone by that name." notify
  then
;
  
  
: controls (player object -- bool)
  owner over dbcmp
  swap "wizard" flag? or
;
  
  
: match_controlled (s -- d)
  noisy_match dup ok? if
    me @ over controls not if
      pop #-1 me @ "Permission denied." notify
    then
  then
;
 
: table_compare ( possible tomatch -- match? )
  dup strlen strncmp not
;
 
: table_loop
( xnone xdouble str1 x1 ... strn xn n tomatch prog pick -- strmat xmat )
  dup 4 >
  if
    dup rotate over rotate
( ..... tomatch prog found? pick stri xi )
    over 7 pick 7 pick execute
( ..... tomatch prog found? pick stri xi match? )
    if
      0 4 pick - rotate 0 3 pick - rotate
( ..... tomatch prog found? pick )
      swap
      if
        popn
        swap pop "" swap exit
      else
        1 swap
      then
    else
      pop pop
    then
    2 - table_loop
  else
    pop
    if
      pop pop rot pop rot pop
    else
      pop pop pop "" swap
    then
  then
;
 
: table_match
( xnone xdouble str1 x1 ... strn xn n tomatch prog -- strmat xmat )
  0 4 rotate 2 * 4 + table_loop
;
 
: std_table_match
  'table_compare table_match
;
  

: multi_rmatch-loop (i s d -- dn .. d1 n)
  dup not if pop pop exit then
  over over name swap
  "&" explode dup 2 + rotate
  begin
    over not if pop pop 0 break then
    swap 1 - swap dup 4 rotate strip
    dup not if pop pop continue then
    dup "all" stringcmp not if pop "*" then
    "*" swap strcat "*" strcat
    smatch if
      pop begin
	dup while
	1 - swap pop
      repeat
      pop 1 break
    then
  repeat
  if rot 1 + rot 3 pick then
  next multi_rmatch-loop
;
  
: multi_rmatch (d s -- dn .. d1 n)
  over over rmatch dup int 0 >= if
    dup thing? over program? or if
      rot rot pop pop 1 exit
    then
  then
  pop
  0 swap rot contents
  multi_rmatch-loop
;
  


PUBLIC noisy_match
PUBLIC noisy_pmatch
PUBLIC controls
PUBLIC match_controlled
PUBLIC table_match
PUBLIC std_table_match
PUBLIC multi_rmatch
.
c
q
@register lib-match=lib/match
@register #me lib-match=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called lib-match
@set $tmp/prog1=/_defs/.controls:"$lib/match" match "controls" call
@set $tmp/prog1=/_defs/.match_controlled:"$lib/match" match "match_controlled" call
@set $tmp/prog1=/_defs/.noisy_match:"$lib/match" match "noisy_match" call
@set $tmp/prog1=/_defs/.noisy_pmatch:"$lib/match" match "noisy_pmatch" call
@set $tmp/prog1=/_defs/.std_table_match:"$lib/match" match "std_table_match" call
@set $tmp/prog1=/_defs/.table_match:"$lib/match" match "table_match" call
@set $tmp/prog1=/_defs/.multi_rmatch:"$lib/match" match "multi_rmatch" call
@set $tmp/prog1=/_docs:@list $lib/match=1-60
