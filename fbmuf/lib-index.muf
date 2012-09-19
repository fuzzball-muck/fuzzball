@prog lib-index
1 99999 d
1 i
( lib-index
  by ErmaFelna
 
  This library consists of a set of routines to manipulate elements
    consisting of name:value pairs.  It stores both a 'list' of all the
    elements [not a list as in the list manager here], and an 'index'
    which contains a list of all the names.  The index is used for the
    purposes of sorting and matching.
 
  The standard set of parameters for all routines in this library contains:
    dbref - the database reference of the object the index exists on
    index - the string containing the name of the index
    name  - the string containing the name of the element in the index
    value - the string containing the value of the named element
  And one extra on output:
    error - a boolean integer: true if the operation succeeded.
 
  All routines except 'index-matchrange' exist in two forms: the base
    forms, which take exactly the parameters listed below, and remove
    their input behind them; and the standard or 'std-' forms, which
    must be given all four of the input parameters, and which will
    return all of them as well.  The routines like 'std-index-add' will
    null out any parameters that aren't used by the base form, and will
    still add an error code to the top of the stack on exit if the base
    form does.
 
  The following routines exist in this library:
 
    index-match: [ dbref index name -- name' error ]
      This is the core reason for this library: it does a complex three
      level matching scheme.  The first attempt is an exact match by
      property name.  The second is a partial exact matching, like in
      exits, where it matches anything in the original name delimited
      by semicolons.  If there are multiple matches at this level, then
      one of them is chosen at random.  The third match attempt tries
      to match the starts of words: anything that comes immediatly
      after a space.  So 'mat' will match 'mattress' and 'lit match',
      but not 'rematch'.
      If the 
      This is the core reason for this library: it does a partial word
      match on the name given, trying to find any element in the index
      with a name that has the given name string as a prefix.  This
      type of match is similar to that performed by the server.  If the
      error code is true, the name' string is good; if the error code
      is false, than the name' string is either null for nothing found,
      or the first element found if there were multiple matches.
 
    index-matchrange: [ dbref index name -- name1' ... namen' n ]
      This routine performs a partial word match search like the usual
      match routine, but it returns a stack range of every value that
      it matches.  Because of the unusual output form of this routine,
      it does not exist in a 'std-' form.
 
    index-envmatch: [ dbref index name -- name' error ]
      The envmatch routine performs a match similar to the standard
      index-match, except that if it doesn't find the value on the
      dbref given, it will try again on the dbref above that.  And it
      will keep scanning up the dbref's until it either finds one or
      more matching element or it hits the top of the environment
      tree.  Return values are the same as index-match.
 
    index-add: [ dbref index name value -- error ]
      The add routine will fail out if an element with that exact name
      exists already; otherwise, it will add the new name to the end
      of the index, and add the new element into the list.
 
    index-add-sort: [ dbref index name value -- error ]
      The sorted add is similar to the usual add routine, except that
      instead of adding it to the end of the index, it adds it right
      before the element whose name is the closest one following it in
      alphabetical order.  This was, so long as the index is always
      added to by this routine, it will remain in alphabetical order.
 
    index-write: [ dbref index name value -- error ]
      This routine is used only to edit the elements that alread exist
      in the index.  It will make the value of the named element equal
      to the value given; if the element does not exist, it will fail.
 
    index-set: [ dbref index name value -- ]
      This combines the add and write routines.  It does not fail out,
      but will add the element if it doesn't already exist, and edit
      it if it does.
 
    index-remove: [ dbref index name -- error ]
      This routine removes an existing element from the list and its
      name from the index.  If the element does not exist in the list,
      it fails.
 
    index-delete: [ dbref index name -- ]
      This acts like the index-remove, except that it simply exits
      if the name is not in the index, rather than failing.
 
    index-value: [ dbref index name -- value ]
      This returns the value associated with the named element.
 
    index-first: [ dbref index -- name ]
      This routine returns the first name in the index.
 
    index-last:
      This routine returns the last name in the index.
 
    index-next: [ dbref index name -- name' ]
      This routine takes the name, and returns the one immediately
      after it in the index.
 
    index-prev: [ dbref index name -- name' ]
      This routine takes the name, and returns the one immediately
      previous to it in the index.
)
 
$include $lib/match
$include $lib/stackrng
 
: int-quickerror  ( dbref index name value errstring -- 0 )
  .tell pop pop pop pop 0
;
 
: int-error  ( dbref index name value errstring -- 0 )
  5 rotate intostr "%d" subst
  4 rotate "%i" subst
  rot "%n" subst
  swap "%v" subst
  .tell 0
;
 
: errorcheck ( dbref index name value -- " " " " noerror? )
  depth 3 >
  if
    4 pick dbref?
    if
      4 pick ok?
      if
        3 pick string?
        if
          3 pick
          if
            over string?
            if
              over
              if
                dup string?
                if
                  dup
                  if
                    1 exit
                  then
                  "Index: Value argument is null."
                else
                  "Index: Value argument is not string."
                then
              else
                "Index: Name argument is null."
              then
            else
              "Index: Name argument is not string."
            then
          else
            "Index: Index argument is null."
          then
        else
          "Index: Index argument is not string."
        then
      else
        "Index: Object argument is bad dbref."
      then
    else
      "Index: Object argument is not dbref."
    then
  else
    "" "" "" "" "Index: function called with insufficient arguments."
  then
  int-quickerror
;
 
: editcheck ( dbref index name value -- " " " " noerror? )
  me @ 5 pick .controls
  if
    1 exit
  then
  trig owner 5 pick .controls trig "_public" getpropstr and
  if
    3 pick 1 strcut "_.@" swap instr
    if
      1 exit
    else
      "Index: index '%i' cannot be written on object #%d." strcat
    then
  else
    "Index: Permission denied on object #%d."
  then
  int-error
;
 
: int-getIPrange  ( dbref index -- indexp1 ... indexpn n )
  0 rot rot "/" strcat
  begin
    "i" strcat over over getpropstr dup
  while
    -4 rotate rot 1 + rot rot
  repeat
  pop pop pop
;
 
: int-compIPrange  ( indexp1 ... indexpn n -- indexp1' ... indexpn' n' )
  dup
  begin
    1 - dup
  while
    dup 3 + pick strlen
    over 3 + pick strlen + 4001 <
    if
      dup 3 + rotate
      over 3 + rotate
      2 strcut swap pop strcat
      over -2 swap - rotate
      swap 1 - swap
    then
  repeat
  pop
;
 
: int-setIPrange  ( indexp1 ... indexpn n dbref index -- )
  "/" strcat
  begin
    "i" strcat rot dup
  while
    1 - dup -4 rotate 3 pick 3 pick rot 6 + rotate
    0 addprop
  repeat
  pop
  begin
    over over getpropstr
  while
    over over remove_prop
    "i" strcat
  repeat
  pop pop
;
 
: int-next-word-match  ( dbref prop left match -- " "' "' " matched )
  begin
    over over instring dup
    if
      rot swap 1 + strcut swap dup ";;" rinstr 2 + strcut swap pop
      swap strcat dup ";;" instr 1 - strcut rot rot break
    else
      pop swap pop swap "i" strcat swap
      3 pick 3 pick getpropstr swap over not
      if
        "" break
      then
    then
  repeat
  ";" "; " subst
;
 
: int-start-word-match  ( dbref index match -- dbref prop "" match )
  swap "/" strcat "" rot
;
 
: int-exact-match?  ( dbref index match -- matched? )
  swap "/t-" strcat swap strcat getpropstr not not
;
 
: int-matchrange  ( dbref index match -- matches ... count )
  int-start-word-match 0 -5 rotate
  begin
    int-next-word-match dup
  while
    -6 rotate 5 rotate 1 + -5 rotate
  repeat
  pop pop pop pop pop
;
 
: index-matchrange  ( dbref index match -- matches ... count )
  " " errorcheck
  if
    pop " " swap strcat int-matchrange
  else
    0
  then
;
 
: int-match  ( dbref index match -- matched error? )
  3 pick 3 pick 3 pick int-exact-match?
  if
    rot pop swap pop 1
  else
    3 pick 3 pick "; " 4 pick strcat ";" strcat int-matchrange dup
    if
      random 256 / over % 2 + rotate -4 3 pick - rotate
      2 + popn 1
    else
      pop " " swap strcat int-start-word-match
      int-next-word-match dup -6 rotate not not dup
      if
        pop int-next-word-match not
      then
      -5 rotate pop pop pop pop
    then
  then
;
 
: index-match  ( dbref index match -- matched error? )
  " " errorcheck
  if
    pop int-match
  else
    "" 0
  then
;
 
: int-envmatch  ( dbref index match -- dbref' matched error? )
  begin
    3 pick 3 pick 3 pick int-match over not
  while
    pop pop rot location rot rot 3 pick not
    if
      "" 0 break
    then
  repeat
  rot pop rot pop
;
 
: index-envmatch  ( dbref index match -- dbref' matched error? )
  " " errorcheck
  if
    pop int-envmatch
  else
    #-1 "" 0
  then
;
 
: int-write  ( dbref index name value -- )
  rot "/t-" strcat rot strcat swap 0 addprop
;
 
: int-add  ( dbref index name value -- )
  over "; " ";" subst "" 6 pick 6 pick "/" strcat
  begin
    dup "i" strcat 3 pick over getpropstr dup
  while
    -5 rotate swap pop rot pop
  repeat
  pop pop rot dup
  if
    dup strlen 5 pick strlen + 4000 >
    if
      pop "i" strcat ";;"
    then
  else
    pop "i" strcat ";;"
  then
  " " strcat 4 rotate strcat ";;" strcat 0 addprop
  int-write
;
 
: index-add  ( dbref index name value -- errcode )
  errorcheck
  if
    editcheck
    if
      4 pick 4 pick 4 pick int-exact-match?
      if
        "Index-add: name '%n' to add already exists in index #%d:%i" int-error
      else
        int-add 1
      then
    then
  then
;
 
: int-add-sort  ( dbref index name value -- )
  4 pick 4 pick 4 pick 4 rotate int-write
  3 pick 3 pick "/t-" strcat 3 pick strcat nextprop
  3 pick strlen 1 + strcut swap pop
  4 pick 4 pick int-getIPrange
  dup 3 + rotate over 3 + rotate dup
  if
    ";; " swap "; " ";" subst strcat ";;" strcat 3 pick
    begin
      dup
    while
      dup 4 + pick 3 pick instring dup
      if
        over 5 + rotate swap
        strcut ";;" swap strcat
        -5 4 pick - rotate -5 3 pick - rotate
        4 rotate 1 + -4 rotate
        break
      then
      pop 1 -
    repeat
  else
    0
  then
  swap pop swap ";; " swap "; " ";" subst strcat ";;" strcat swap
  -2 swap - rotate 1 +
  int-compIPrange
  dup 3 + rotate over 3 + rotate
  int-setIPrange
;
 
: index-add-sort  ( dbref index name value -- errcode )
  errorcheck
  if
    editcheck
    if
      4 pick 4 pick 4 pick int-exact-match?
      if
        "Index-add-sort: name '%n' to add already exists in index #%d:%i"
        int-error
      else
        int-add-sort 1
      then
    then
  then
;
 
: index-write  ( dbref index name value -- errcode )
  errorcheck
  if
    editcheck
    if
      4 pick 4 pick 4 pick int-exact-match?
      if
        int-write 1
      else
        "Index-write: name '%n' to edit doesn't exist in index #%d:%i"
        int-error
      then
    then
  then
;
 
: index-set  ( dbref index name value -- errcode )
  errorcheck
  if
    editcheck
    if
      4 pick 4 pick 4 pick int-exact-match?
      if
        int-write
      else
        int-add
      then
      1
    then
  then
;
 
: int-remove  ( dbref index name -- )
  ";; " over "; " ";" subst strcat ";;" strcat 4 pick 4 pick "/" strcat
  begin
    "i" strcat over over getpropstr dup
  while
    dup 5 pick instring dup
    if
      strcut 5 rotate strlen 2 - strcut swap pop strcat 0 addprop 1 break
    else
      pop pop
    then
  repeat
  not
  if
    pop pop pop pop
  then
  swap "/t-" strcat swap strcat remove_prop
;
 
: index-remove  ( dbref index name -- errcode )
  " " errorcheck
  if
    editcheck
    if
      pop 3 pick 3 pick 3 pick int-exact-match?
      if
        int-remove 1 exit
      else
        "" "Index-remove: name '%n' to remove doesn't exist in index #%d:%i"
        int-error
      then
    then
  then
;
 
: index-delete  ( dbref index name -- )
  " " errorcheck
  if
    editcheck
    if
      pop int-remove
    then
  then
;
 
: int-value  ( dbref index name -- value )
  swap "/t-" strcat swap strcat getpropstr
;
 
: index-value  ( dbref index name -- value )
  " " errorcheck
  if
    pop int-value
  then
;
 
: int-first  ( dbref index -- first )
  "/i" strcat getpropstr dup
  if
    3 strcut swap pop
    dup ";;" instr dup
    if
      1 - strcut pop ";" "; " subst
    else
      pop pop ""
    then
  else
    pop ""
  then
;
 
: index-first ( dbref index -- first )
  " " " " errorcheck
  if
    pop pop int-first
  else
    ""
  then
;
 
: int-last  ( dbref index -- last )
  "/" strcat "" rot rot
  begin
    "i" strcat over over getpropstr dup
  while
    4 rotate pop rot rot
  repeat
  swap pop swap pop
  if
    dup strlen 1 - strcut pop
    dup ";; " rinstr 2 + strcut swap pop
  else
    pop ""
  then
  ";" "; " subst
;
 
: index-last ( dbref index -- last )
  " " " " errorcheck
  if
    pop pop int-last
  else
    ""
  then
;
 
: int-next  ( dbref index name -- next )
  ";; " swap "; " ";" subst strcat ";;" strcat rot rot "/" strcat
  begin
    "i" strcat over over getpropstr dup
  while
    dup 5 pick instring dup
    if
      5 pick strlen + strcut swap pop
      dup ";;" instr dup
      if
        1 - strcut pop
      else
        pop pop "i" strcat over over getpropstr dup
        if
          3 strcut swap pop
          dup ";;" instr 1 - strcut pop
        then
      then
      break
    then
    pop pop
  repeat
  -4 rotate pop pop pop ";" "; " subst
;
 
: index-next  ( dbref index name -- next )
  " " errorcheck
  if
    pop int-next
  else
    ""
  then
;
 
: int-prev  ( dbref index name -- prev )
  ";; " swap "; " ";" subst strcat ";;" strcat
  ";;" 4 rotate 4 rotate "/" strcat
  begin
    "i" strcat over over getpropstr dup
  while
    dup 6 pick instring dup
    if
      1 - strcut pop
      dup ";;" rinstr dup
      if
        2 + strcut swap pop
      else
        pop pop 3 pick dup strlen 1 - strcut pop
        dup ";;" rinstr dup
        if
          2 + strcut swap
        then
        pop
      then
      break
    then
    pop -4 rotate rot pop
  repeat
  -5 rotate pop pop pop pop ";" "; " subst
;
 
: index-prev  ( dbref index name -- prev )
  " " errorcheck
  if
    pop int-prev
  else
    ""
  then
;
 
( And the non-deleting macros... )
( All of these take 'dbref index name value', and return the same list
  with the addition of an error value for the following routines:
  match, envmatch, add, write, remove )
 
: std-index-match
  pop 3 pick 3 pick rot
  index-match
  "" swap
;
 
: std-index-envmatch
  pop 3 pick 3 pick rot
  index-envmatch
  "" swap
;
 
: std-index-add
  4 pick 4 pick 4 pick 4 pick
  index-add
;
 
: std-index-add-sort
   4 pick 4 pick 4 pick 4 pick
  index-add-sort
;
 
: std-index-write
  4 pick 4 pick 4 pick 4 pick
  index-write
;
 
: std-index-set
  4 pick 4 pick 4 pick 4 pick
  index-set
;
 
: std-index-remove
  pop 3 pick 3 pick 3 pick
  index-remove
  "" swap
;
 
: std-index-delete
  pop 3 pick 3 pick 3 pick
  index-delete
  ""
;
 
: std-index-value
  pop 3 pick 3 pick 3 pick
  index-value
;
 
: std-index-first
  pop pop over over
  index-first
  ""
;
 
: std-index-last
  pop pop over over
  index-last
  ""
;
 
: std-index-next
  pop 3 pick 3 pick rot
  index-next
  ""
;
 
: std-index-prev
  pop 3 pick 3 pick rot
  index-prev
  ""
;
 
public index-match                   public std-index-match
public index-matchrange
public index-envmatch                public std-index-envmatch
public index-add                     public std-index-add
public index-add-sort                public std-index-add-sort
public index-write                   public std-index-write
public index-set                     public std-index-set
public index-remove                  public std-index-remove
public index-delete                  public std-index-delete
public index-value                   public std-index-value
public index-first                   public std-index-first
public index-last                    public std-index-last
public index-next                    public std-index-next
public index-prev                    public std-index-prev
.
c
q
@register lib-index=lib/index
@register #me lib-index=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=3
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called lib-index
@set $tmp/prog1=/_defs/.index-add:"$lib/index" match "index-add" call
@set $tmp/prog1=/_defs/.index-add-sort:"$lib/index" match "index-add-sort" call
@set $tmp/prog1=/_defs/.index-delete:"$lib/index" match "index-delete" call
@set $tmp/prog1=/_defs/.index-envmatch:"$lib/index" match "index-envmatch" call
@set $tmp/prog1=/_defs/.index-first:"$lib/index" match "index-first" call
@set $tmp/prog1=/_defs/.index-last:"$lib/index" match "index-last" call
@set $tmp/prog1=/_defs/.index-match:"$lib/index" match "index-match" call
@set $tmp/prog1=/_defs/.index-matchrange:"$lib/index" match "index-matchrange" call
@set $tmp/prog1=/_defs/.index-next:"$lib/index" match "index-next" call
@set $tmp/prog1=/_defs/.index-prev:"$lib/index" match "index-prev" call
@set $tmp/prog1=/_defs/.index-remove:"$lib/index" match "index-remove" call
@set $tmp/prog1=/_defs/.index-set:"$lib/index" match "index-set" call
@set $tmp/prog1=/_defs/.index-value:"$lib/index" match "index-value" call
@set $tmp/prog1=/_defs/.index-write:"$lib/index" match "index-write" call
@set $tmp/prog1=/_defs/.std-index-add:"$lib/index" match "std-index-add" call
@set $tmp/prog1=/_defs/.std-index-add-sort:"$lib/index" match "std-index-add-sort" call
@set $tmp/prog1=/_defs/.std-index-delete:"$lib/index" match "std-index-delete" call
@set $tmp/prog1=/_defs/.std-index-envmatch:"$lib/index" match "std-index-envmatch" call
@set $tmp/prog1=/_defs/.std-index-first:"$lib/index" match "std-index-first" call
@set $tmp/prog1=/_defs/.std-index-last:"$lib/index" match "std-index-last" call
@set $tmp/prog1=/_defs/.std-index-match:"$lib/index" match "std-index-match" call
@set $tmp/prog1=/_defs/.std-index-next:"$lib/index" match "std-index-next" call
@set $tmp/prog1=/_defs/.std-index-prev:"$lib/index" match "std-index-prev" call
@set $tmp/prog1=/_defs/.std-index-remove:"$lib/index" match "std-index-remove" call
@set $tmp/prog1=/_defs/.std-index-set:"$lib/index" match "std-index-set" call
@set $tmp/prog1=/_defs/.std-index-value:"$lib/index" match "std-index-value" call
@set $tmp/prog1=/_defs/.std-index-write:"$lib/index" match "std-index-write" call
@set $tmp/prog1=/_docs:@list $lib/index=1-109
@set $tmp/prog1=/_lib-version:1.2
