( DBref list manager -- REF
  A reflist is a property on an object that contains a string with
  a series of space and # delimited dbrefs in it.  ie:
    reflist:#1234 #9364 #21 #6466 #37
  A reflist only will contain one copy of any one dbref within it.
  A reflist can be no longer than 4096 characters long.  Generally,
    this means around 500+ refs.
  
  REF-add  [objref reflistname dbreftoadd -- ]
    Adds a dbref to the dbreflist.  If the given dbref is already in
    the reflist, it moves it to the end of the reflist.
  
  REF-delete  [objref reflistname dbreftokill -- ]
    Removes a dbref from the dbreflist.
  
  REF-first [objref reflistname -- firstdbref]
    Returns the first dbref in the reflist.
  
  REF-next  [objref reflistname currdbref -- nextdbref]
    Returns the next dbref in the list after the one you give it.
    Returns #-1 at the end of the list.
  REF-inlist? [objref reflistname dbreftocheck -- inlist?]
    Returns whether or not the given dbref is in the dbreflist.
  
  REF-list  [objref reflistname -- liststr]
    Returns a comma delimited string with the names of all the objects
    in the given reflist.
  
  REF-allrefs [objref reflistname -- refx...ref1 refcount]
    Returns a range on the stack containing all the refs in the list,
    with the count of them on top.
  REF-filter [address objref reflistname -- refx...ref1 refcount]
    Returns a range of dbrefs on the stack, filtered from the given reflist.
    The filtering is done by a function that you pass the address of.  The
    filter routine is [d -- i].  It takes a dbref and returns a boolean int.
    If the integer is 0, the ref is not included in the returned list.  If
    the integer is not 0, the it is in the returned list.
  REF-editlist  [players? objref reflistname -- ]  
    Enters the user into an interactive editor that lets them add and remove
    objects from the given reflist.  'players?' is an integer boolean value,
    where if it is true, the list only lets you add players to it.  Otherwise
    it lets you add regular objects to it.
)
  
$include $lib/strings
$include $lib/props
$include $lib/look
$include $lib/match
  
: REF-delete (obj reflist killref -- )
  3 pick 3 pick getpropstr " " strcat
  swap int intostr " " strcat
  "#" swap strcat STRsplit
  strcat STRsms STRstrip setpropstr
;
  
: REF-add (obj reflist addref -- )
  3 pick 3 pick 3 pick REF-delete
  3 pick 3 pick getpropstr " " strcat
  swap int intostr " " strcat
  "#" swap strcat strcat
  STRsms STRstrip setpropstr
;
  
: REF-first (obj reflist -- firstref)
  getpropstr " " STRsplit pop
  dup not if pop #-1 exit then
  1 strcut swap pop atoi dbref
;
  
: REF-next (obj reflist currref -- nextref)
  rot rot getpropstr
  swap int intostr
  " " strcat "#" swap strcat
  STRsplit swap pop STRstrip
  dup not if pop #-1 exit then
  " " STRsplit pop
  1 strcut swap pop
  atoi dbref
;
  
: REF-inlist? (objref reflistname dbreftocheck -- inlist?)
  rot rot getpropstr " " strcat
  swap int intostr
  " " strcat "#" swap strcat
  instr
;
  
: REF-allrefs (d s -- dx...d1 i)
  getpropstr STRsms strip
  0 swap "#" explode
  begin
    dup while
    1 - swap strip
    dup not if pop continue then
    atoi dbref
    over 3 + -1 * rotate
    dup 2 + rotate 1 + over 2 + -1 * rotate
  repeat
  pop
;
: REF-list  (objref reflistname -- liststr)
  REF-allrefs .short-list
;
: REF-filter (a d s -- dx...d1 i)
  getpropstr STRsms strip
  0 rot rot begin
    striplead dup while
    "#" .split swap strip
    dup not if pop continue then
    atoi dbref dup 4 pick execute if
      -4 rotate rot 1 + rot rot
    else pop
    then
  repeat
  pop pop
;
: REF-editlist-help
  if
    "To add a player, enter their name.  To remove a player, enter their name"
    "with a ! in front of it.  ie: '!guest'.  To display the list, enter '*'"
    "on a line by itself.  To clear the list, enter '#clear'.  To finish"
    "editing and exit, enter '.' on a line by itself.  Enter '#help' to see"
    "these instructions again."
    strcat strcat strcat strcat .tell
  else
    "To add an object, enter its name or dbref.  To remove an object, enter"
    "its name or dbref with a ! in front of it.  ie: '!button'.  To display"
    "the list, enter '*' on a line by itself.  To clear the list, enter"
    "'#clear'.  To finish editing and exit, enter '.' on a line by itself."
    "Enter '#help' to see these instructions again."
    strcat strcat strcat strcat .tell
  then
;
: REF-editlist  (players? objref reflistname -- )
  3 pick REF-editlist-help
  "The object list currently contains:" .tell
  over over REF-list .tell
  begin
    read
    dup "." strcmp not if
      pop pop pop
      "Done." .tell break
    then
    dup "#list" stringcmp not
    over "*" strcmp not or if
      pop "The object list currently contains:" .tell
      over over REF-list .tell continue
    then
    dup "#clear" stringcmp not if
      pop over over remove_prop
      "Object list cleared." .tell continue
    then
    dup "#help" stringcmp not if
      pop 3 pick REF-editlist-help
      continue
    then
    dup "!" 1 strncmp not if
      1 strcut swap pop 1
    else 0
    then
    swap 5 pick if .noisy_pmatch else .noisy_match then
    dup ok? not if pop pop continue then
    4 pick 4 pick rot 4 rotate if
      3 pick 3 pick 3 pick REF-inlist? if
        REF-delete "Removed." .tell
      else
        pop pop pop
        "Not in object list." .tell
      then
    else
      REF-add "Added." .tell
    then
  repeat
;
PUBLIC REF-add
PUBLIC REF-delete
PUBLIC REF-first
PUBLIC REF-next
PUBLIC REF-list
PUBLIC REF-inlist?
PUBLIC REF-allrefs
PUBLIC REF-filter (address objref reflistname -- refx...ref1 refcount)
PUBLIC REF-editlist  (players? objref reflistname -- )
