( Set of library routines for doing 'look' functions. )
( The following functions are included in this library:
  safecall:  x d --
       Takes a dbref which is assumed to be a command or @desc-like program
       that takes one parameter, usually a string, and returns no values.]
       It ensures that none of the variables me, loc, trigger, or command
       are modified, and that no garbage is left behind on the stack.
  unparse:  d -- s
       Takes a dbref, and returns either just its name, or the name plus
       flags, depending on the permissions of me @.
  contents-filter:  a d -- d... i
       Takes the address of a 'filter' routine and a dbref, and returns a
       range on the stack of the filtered contents of the object.  The first
       item to print is the bottom of the stack range.  The filter should
       be  d -- i; it takes a dbref and returns a true/false value to say
       whether or not the dbref should be put into the list.
  get-contents:  d -- d... i
       Takes a dbref, and returns the list of its contents, filtered through
       the standard filter which acts like the server's contents list:
       Dark rooms don't list anything unless the room or the objects are
       yours, dark objects not owned by you don't show, and you don't show.
       This list has the first element in the contents at the bottom of the
       stack.
  long-display:  d... i --
       List the dbref stack range given, in the usual format for the server.
       All elements on separate lines, using unparse.  The bottom element is
       printed first.
  short-list:  d... i -- s
       Turns the range of dbrefs on the stack into a properly formatted
       string, with commas.  1 element is just returned, 2 elements returns
       '1 and 2', more elements return '1, 2, 3, and 4' or similar.  Returns
       a null string if there are no elements.  Again, the bottom element is
       first in the list.
  short-display:  d... i --
       Calls short-list, then prints out "You see <string>." to the user.
       Prints "You see nothing." if nothing is on the list.
  list-contents:  s d --
       Calls get-contents followed by long-display to print out all of the
       contents of the given dbref.  If there are any contents listed, then
       the string on the stack is printed out, for "Contents:" or the like.
       If the contents list is empty, the string is ignored.
  str-desc:  s --
       Takes string 's', and prints it out as a description.  Matches the
       '@###' and '@$prog' values properly, and uses them with the present
       trigger value.  If neither of these exist, or if they're invalid,
       the rest of the string is just printed out.
  dbstr-desc:  d s --
       Runs str-desc, using the value d on the stack as the effective
       trigger value.
  db-desc:  d --
       Does a full description of the object, including name and succ/fail
       if the dbref given is a room, and contents.  All programs run with
       the dbref given in 'trigger @'.  Will return the proper values for
       dbref's #-1 and #-2 as well.
  cmd-look:  s --
       Does a match function, then calls db-desc with the results.  This
       will simulate the usual 'look' command.
)
 
$doccmd @list __PROG__=!@1-58
$lib-version 1.31
 
$include $lib/match
$include $lib/stackrng
$include $lib/strings
 
lvar sme
lvar sloc
lvar strigger
lvar scommand
lvar sdepth
lvar realtrig
 
: safecall  ( x d -- )
  me @ sme !
  loc @ sloc !
  trigger @ strigger !
  command @ scommand !
  depth sdepth !
  call
  sme @ me !
  sloc @ loc !
  strigger @ trigger !
  scommand @ command !
  depth 2 + sdepth @ - popn
;
 
: control? ( d -- i )
  me @ swap controls
;
 
: dark? ( d -- i )
  dup "Dark" flag? swap control? not and
;
 
: unparse ( d -- s )
  me @ "Silent" flag?
  if
    name exit
  then
  dup control? not
  if
    dup "Link_OK" flag? not
    if
      dup "Chown_OK" flag? not
      if
        dup "Abode" flag? not
        if
          name exit
        then
      then
    then
  then
  unparseobj
;
 
( Don't see rooms.  Don't see programs you can't link to. )
: std-filter ( d -- i )
  begin
    0 over me @ dbcmp not
  while
    over program? dup
    if
      pop over control? 3 pick "Link_OK" flag? or not
    then
    not
  while
    over room? not
  while
    over dark? not
  while
    pop 1 1
  until
  swap pop
;
 
: contents-filter ( a d -- d... i )
  contents 0 rot rot
  begin
    dup
  while
    over over swap execute
    if
      rot 1 + rot rot dup -4 rotate
    then
    next
  repeat
  pop pop
;
 
: get-contents ( d -- d... i )
  dup dark?
  if
    pop 0
  else
    'std-filter swap contents-filter
  then
;
 
: long-display ( d... i -- )
  begin
    dup
  while
    1 - dup 2 + rotate
    dup dbref?
    if
      unparse
    then
    .tell
  repeat
  pop
;
 
: short-list ( d... i -- s )
  dup not if pop "" exit then
  dup 3 <
  if
    1 - dup 2 + rotate name over
    if
      " " strcat
    then
  else
    ""
    begin
      over 1 >
    while
      swap 1 - swap over 3 + rotate name ", " strcat strcat
    repeat
  then
  swap
  if
    "and " strcat swap name strcat
  then
;
 
: short-display ( d... i -- )
  short-list dup
  if
    "You see " swap strcat "." strcat .tell
  else
    "You see nothing." .tell
  then
;  
 
: list-contents ( s d -- )
  get-contents dup
  if
    dup 2 + rotate .tell
    long-display
  else
    pop pop
  then
;
 
: str-desc ( s -- )
  strip dup
  if
    dup "@" 1 strncmp
    if
      .tell
    else
      1 strcut swap pop " " split strip swap
      dup "$" 1 strncmp
      if
        atoi dbref
      else
        match
      then
      dup ok?
      if
        dup trigger @ owner swap controls over "Link_OK" flag? or
        if
          safecall
        else
          pop pop "Permission Denied" .tell
        then
      else
        pop .tell
      then
    then
  else
    pop "You see nothing special." .tell
  then
;
 
: dbstr-desc ( d s -- )
  swap trigger @ realtrig ! trigger !
  str-desc
  realtrig @ trigger !
;
 
: db-desc ( d -- )
  dup #-1 dbcmp
  if
    pop "I don't see that here." .tell exit
  then
  dup #-2 dbcmp
  if
    pop "I don't know which one you mean!" .tell exit
  then
  dup trigger @ realtrig ! trigger !
  dup room?
  if
    dup unparse .tell
  then
  dup desc str-desc
  dup room?
  if
    me @ over locked?
    if
      dup fail dup
      if
        str-desc
      else
        pop
      then
      dup ofail .otell
    else
      dup succ dup
      if
        str-desc
      else
        pop
      then
      dup osucc .otell
    then
    "Contents:" over list-contents
  else
    dup player?
    if
      "Carrying:" over list-contents
    then
  then
  realtrig @ trigger !
  pop
;
 
: cmd-look ( s -- )
  dup not
  if
    pop "here"
  then
  match db-desc
;
 
public cmd-look
public contents-filter
public db-desc
public dbstr-desc
public get-contents
public list-contents
public long-display
public safecall
public short-display
public short-list
public str-desc
public unparse
 
$pubdef .cmd-look               __PROG__ "cmd-look" call
$pubdef .contents-filter        __PROG__ "contents-filter" call
$pubdef .db-desc                __PROG__ "db-desc" call
$pubdef .dbstr-desc             __PROG__ "dbstr-desc" call
$pubdef .get-contents           __PROG__ "get-contents" call
$pubdef .list-contents          __PROG__ "list-contents" call
$pubdef .long-display           __PROG__ "long-display" call
$pubdef .safecall               __PROG__ "safecall" call
$pubdef .short-display          __PROG__ "short-display" call
$pubdef .short-list             __PROG__ "short-list" call
$pubdef .str-desc               __PROG__ "str-desc" call
$pubdef .unparse                __PROG__ "unparse" call
