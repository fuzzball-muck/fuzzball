@prog sayfront3-11.MUF
1 99999 d
1 i
( Frontend to say. sayset;sayfilter;sayhelp
`
 Property on sayset PROGRAM : _trigger : DBint of say action
                              _help/   : help texts
                              _say/    : filter registration
)
$define verb-chop-const 18 $enddef
$define OOC "_OOC" $enddef
$define toOlower "ooo" "o O" subst tolower "o O" "ooo" subst $enddef
lvar snoop ( I_am_snooping flag )
lvar ploc ( storage object )
lvar mee ( virtual player and normal/adhoc/query/ooc flags location )
: controls? ( d -- i )
  owner "me" match owner dbcmp snoop @ or
;
: on-match ( patt s -- i )  ( !foo +foo match foo and !foo, +foo )
                            (  foo|bar  matches foo or bar )
  over "|" instr dup if
    rot swap 1 - strcut 1 strcut swap pop swap
    3 pick on-match if pop pop 1 exit then
    swap on-match
  else
    pop over 1 strcut swap dup "!" strcmp swap "+" strcmp and if pop else
      over strcmp not if pop pop 1 exit then
    then
    strcmp not
  then
;
: on-execute ( an sn ... a1 s1 n s -- address 0 [call] )
             (                     -- s 1       [nocall] )
  rot over on-match if
    pop 1 -
    begin dup while 2 put 2 put 1 - repeat
    pop 0
  else
    2 put 1 - dup if swap on-execute else pop 1 then
  then
;
: wordcut ( m -- m1 strip[m2*] )
  dup " " instr dup if 1 - strcut strip else pop "" then
;
: tabulate ( base col add -- base )
  rot "                                        " dup strcat (80) strcat
  rot strcut pop swap strcat
;
: moveprop ( db to from -- ) ( Copies 'from' prop&dir to 'to', remvs 'from' )
  3 pick over propdir? if
    dup strlen over "/" strcat 5 pick swap
    over swap nextprop swap 0
    begin
      pop over while over over
      swap nextprop swap rot
      over over dup 7 pick strcut 10 pick 2 put strcat swap
      3 pick over propdir? if moveprop else
        3 pick swap getpropstr 0 addprop
      then
    repeat
    pop pop pop
  then
  3 pick swap over over getpropstr
  -3 rotate remove_prop  ( Also kills non-subdirs )
  0 addprop
;
: filtername ( "2634" -- "Replace" )  ( "#2345" if unknown )
  dup "_say/db/" swap strcat
  prog swap getpropstr dup if swap pop else pop "#" swap strcat then
;
: filternumber ( "replace" -- "2345" ) ( unchanged if unknown )
dup not if pop "xnullx" exit then
  tolower dup "_say/filter/" swap strcat
  prog swap getpropstr dup if swap pop else pop then
;
: isch? ( s -- s' i )
  "+:" over on-match if "co" ":" subst 1 exit then
  "+/" over on-match if "sl" "/" subst 1 exit then
  "+~" over on-match if "tw" "~" subst 1 exit then
  "+@" over on-match if "at" "@" subst 1 exit then
  dup strlen dup 1 = if exit then
  2 = if dup 1 strcut pop "+" strcmp not if 1 exit then then
  "+def|+ooc|+sl|+co|+sp|+at|+tw" over on-match
;
: ch-prop ( ch -- "_say/ch/[o]say" )  ( ch != "+" )
  dup "+" 1 strncmp if "/osay" else
     1 strcut swap pop "/say" then
  strcat "_say/" swap strcat
;
: do-prop ( "verb" "propname" db -- )
  swap rot dup "$" 1 strncmp not if
    filternumber dup atoi if "$" swap strcat else
      1 strcut swap pop atoi dup if
        intostr "$" swap strcat
      else
        "sayset: I don't recognise that verb-program. I've taken no action."
        .tell pop pop pop exit
      then
    then
then dup if "sayset: Verb set." else "sayset: Verb reset." then .tell
  dup if 0 addprop else pop remove_prop then
;
: ch-verb ( "verb" "ch|+ch" -- ) ( sayset [+]<ch> verb  <ch>=={?|sp|def} )
  dup "def" strcmp not if
    over strlen verb-chop-const > if 
        swap verb-chop-const strcut pop 
        "Sayset: Verb longer than " verb-chop-const intostr strcat 
        " characters. Truncated to '" strcat over strcat "'" strcat .tell 
        swap 
    then 
  then
  dup "+" strcmp if ch-prop else pop "_say/+/osay" then ploc @ do-prop
;
: here-verb ( "verb" "here|+here" -- ) (  sayset [+]here verb  )
  loc @ controls? not if
    pop pop "sayset: You don't control this location!" .tell
  exit then
  ch-prop loc @ do-prop
;
: ooc-setverb ( "verb" "ooc|+ooc|!ooc" -- ) (  sayset [+!]ooc [verb]  )
  "say: Flag updated." .tell
  "!ooc" over strcmp not if
    mee @ OOC remove_prop pop exit then
  over over "ooc" strcmp or not if
    ploc @ "/_say/ooc" propdir? not if
      ploc @ "_say/ooc/say"    ""            0 addprop
      ploc @ "_say/ooc/osay"   "(OOC) says," 0 addprop
      ploc @ "_say/ooc/quotes" "(%m)"        0 addprop
      ploc @ "_say/ooc/#" "2"                0 addprop
      ploc @ "_say/ooc/1"
          prog "_say/filter/halt" getpropstr 0 addprop
    then
    mee @ OOC "yes" 0 addprop pop exit then
  ch-prop ploc @ do-prop
;
: say-verb ( "verb" "say|+say" -- )  (  sayset [+]say verb  )
  prog owner me @ dbcmp not if
    pop pop "sayset: You don't control that!" .tell
  exit then
  ch-prop prog "_trigger" getpropstr atoi dbref do-prop
;
: flags ( "params" "[!]{adhoc|query|normal}" -- ) (flags always on player/puppt)
  dup 1 strcut swap "!" strcmp if
    pop mee @ "_say/" rot strcat "yes" 0 addprop
  else
    mee @ "_say/" rot strcat remove_prop pop
  then
  pop "sayset: Flag updated." .tell
;
: bcst-set ( "params" "[!]broadcast}" -- )
  loc @ controls? not if
    pop pop "sayset: You don't control this location!" .tell exit
  then
  swap pop 1 strcut pop "!" strcmp not if
    loc @ "_say/nobcst" "yes" 0 addprop
  else
    loc @ "_say/nobcst" remove_prop
  then
  "sayset: Flag updated." .tell
;
: notify-set ( "[#]progDBref" "notify" -- )
  pop
  loc @ controls? not if
    pop "sayset: You don't control this location!" .tell
  exit then
  dup "#" 1 strncmp not if 1 strcut swap pop then atoi
  dup not if
    pop loc @ "_say/notify" remove_prop "sayset: Notify removed." .tell exit
  then
  dbref
  dup program? not if "sayset: That's not a program!" .tell pop exit then
  int intostr loc @ "_say/notify" rot 0 addprop
  "sayset: Notify set." .tell
;
: clear-ch ( "<ch>" "clear" -- )
  pop wordcut pop isch? if
    ploc @ "_say/" rot strcat remove_prop
  else
    "here" strcmp if
      "sayset: Format: sayset clear <ch-or-here>'." .tell exit
    else
      loc @ controls? not if
        pop pop "sayset: You don't control this location!" .tell
      exit then
      loc @ "_say/here" remove_prop
    then
  then
  "sayset: Properties cleared." .tell
;
: suffix-ch ( "<ch> <junk>" "[!|+]suffix" -- )
  swap wordcut pop
  dup not if pop "def" then
  isch? not if
    pop "sayset: Format: sayset " swap strcat " <ch>" strcat .tell
    exit
  then
  dup "+" strcmp if dup "+" 1 strncmp not if 1 strcut swap pop then then
  "_say/ch/suffix" swap "ch" subst
  swap dup "!" 1 strncmp if
    "+" 1 strncmp if
      ploc @ swap "s" 0 addprop (Suffix-leaving type)
    else
      ploc @ swap "S" 0 addprop (Suffix-removing type)
    then
    "sayset: Suffix switch set."
  else
    pop ploc @ swap remove_prop
    "sayset: Suffix switch cleared."
  then
  .tell
;
: quotes ( "<ch> '%m'" "quotes" -- )
 ( sayset quotes <ch> quote-string [ quote string of form <<%m>> "%m" etc] )
 pop wordcut over "say" strcmp not if
   dup "!" instr over "*" instr or if
     "sayset: characters '!' and '*' are hard-wired bad in quotes." .tell
     "" "!" subst "" "*" subst
   then
   swap pop prog "_trigger" getpropstr
   atoi dbref getlink "_say/badquotes" rot 0 addprop
"sayset: Badquotes set." .tell exit then
 over dup strlen 1 = swap
 "def|ooc|sp" swap on-match or not if
   "sayset: Format: sayset quotes <ch> \"%m\" [or <<%m>> etc]" .tell
   "sayset: To set or clear default quotes, use 'def' for <ch>." .tell
   pop pop exit
 then
 ( "<ch>" "'%m'" )
 swap isch? not if   ( Convert / and : )
   pop pop "sayset quotes: Unrecognised <ch>, but this shouldn't appear." .tell exit
 then
 "_say/" swap strcat "/quotes" strcat ploc @ swap rot
 toOlower dup "%m" instr not over "" " %m " subst "" "%m" subst
 dup strlen 6 > swap strlen 2 < or or over and if
   "sayset: Quotes should be 4..8 chars containing '%m', eg <<%m>>." .tell
   "sayset: Or 6..10 containing ' %m ', eg o/~ %m o/~." .tell
   pop "" (Use say-supplied default)
 then
 strip dup dup strlen 2 - dup 0 < if pop 0 then
    strcut swap pop "%m" strcmp not
 over "%m" 2 strncmp not or if
   pop "" "sayset: You can't start or end quotes with %m." .tell
 then
 prog "_trigger" getpropstr atoi dbref getlink
 "_say/badquotes" getpropstr dup if
   over begin
     dup not if pop pop break then
     1 strcut 3 pick 3 pick
     instr if pop "sayset: You can't use '" swap strcat
           "' in quotes." strcat .tell pop pop "" break then
   swap pop repeat
 else pop then
 dup "!" instr over "*" instr or if
   pop "" "sayset: You can't use '!' or '*' in quotes." .tell
 then
dup if 0 addprop "sayset: Quotes have been set." .tell exit then
 "sayset: The old quotes have been cleared. Say will use the global default."
 .tell pop remove_prop
;
: feep-set
  me @ "_say/feep/name" getpropstr dup if
    "Restoring previous game..." .tell
    me @ "_say/feep/name" remove_prop
    me @ "_say/feep/guess" getpropstr
    me @ "_say/feep/guessed" getpropstr
    me @ "_say/feep/lives" getpropstr atoi -4 rotate
  else
 pop 7 begin
    begin random dbtop int % dbref dup thing? if break then pop repeat
   dup "d" flag? over "_private?" getpropstr "yes" stringcmp not or not
      if break then pop repeat
name
    "" over
    begin
      1 strcut swap dup toupper dup tolower strcmp if pop "." then
    rot swap strcat swap dup not until
    "and 'suspend' to halt the game. (Resume with 'sayset feep'.)"
    "You have seven lives and lose a life with each error. Type 'quit' to stop,"
    "I'm thinking about something, and you have to guess it letter-by-letter."
    .tell .tell .tell
  then
;
: feep ( "params" "feep" -- )
  pop pop "Welcome to Warwick's sayset feep!" .tell
  feep-set
( lives name guess ch-guessed )
  begin
    "You have '" 3 pick strcat "' so far and have " strcat
    5 pick dup intostr swap 1 - if " lives left." else " life left." then
    strcat strcat .tell
    read tolower
    dup dup "suspend" strcmp and not if pop
      "Suspending feep game. Restart with 'sayset feep'." .tell
      me @ "_say/feep/guessed" rot 0 addprop
      me @ "_say/feep/guess"   rot 0 addprop
      me @ "_say/feep/name"    rot 0 addprop
      me @ "_say/feep/lives"   rot intostr 0 addprop "" then
    dup "quit" strcmp not if
      pop pop pop "Quitting. The thing was '" swap strcat "'" strcat .tell
      pop "" then
    1 strcut pop dup if
      over over instr if
        "You've tried that letter before." .tell pop continue then
      dup rot strcat -4 rotate
      3 pick tolower over rinstr dup if
        "Good! You found one of the letters." .tell
        4 pick -4 rotate
        begin
          rot swap 1 - 4 pick over strcut 1 strcut pop swap 5 put
          -3 rotate strcut 1 strcut 4 rotate 2 put strcat strcat swap
        3 pick tolower over rinstr dup not until
        pop rot pop 4 rotate swap
        4 pick 4 pick strcmp not if
          pop pop pop swap dup 1 - if
           "Well done! You guessed '" rot strcat "' with " strcat
           swap intostr " lives left."
          else
           pop "You guessed '" swap "' with one life remaining. Pppht."
          then
          strcat strcat .tell ""
        then
      else
        pop "Letter not found -- you lose a life." .tell
        4 rotate swap 5 pick 1 - dup if 5 put else
          pop pop pop pop
          "You don't have a life. You lose! (It was '" swap strcat "')"
          strcat .tell pop
          break
        then
      then
    then
  not until
;
: say-summary
  "Summary of current say settings..." .tell  " " .tell
  ploc @ "/_say/"
  begin
    over swap nextprop
    dup while
    over over propdir? not if continue then
    dup "/_say/feep" strcmp not if continue then
( Find <ch> )
    dup dup "/" rinstr strcut swap pop
    "   " swap strcat dup strlen 3 - strcut swap pop
( Find suffix switch )
    3 pick 3 pick "/suffix" strcat getpropstr " " swap strcat strcat
( Find osay and deal with verb-program naming )
    3 pick 3 pick "/osay" strcat getpropstr dup if
      dup "$" 1 strncmp not if
        1 strcut swap pop filtername
        dup "#" 1 strncmp not if 1 strcut "$" 2 put strcat then
      then
      6 swap tabulate
    else pop then
( Find osay and quotes )
    3 pick 3 pick "/say" strcat getpropstr dup if
      18 swap tabulate
    else pop then
    3 pick 3 pick "/quotes" strcat getpropstr dup if
      30 swap tabulate
    else pop then
( Do filter section: column 42-80 chars )
    40 "[ " tabulate
    3 pick 3 pick "/#" strcat getpropstr if
      1 swap dup strlen
      begin
        5 pick 5 pick "/" strcat
        5 pick intostr strcat getpropstr dup while filtername
        over over strlen + 78 > if
          swap pop swap .tell
          " " swap 42 swap " " strcat tabulate dup strlen
        else
          swap pop " " strcat strcat dup strlen
        then
        3 pick 1 + 3 put
      repeat
      pop pop swap pop
    then
    "]" strcat .tell
  repeat
  pop pop
  mee @ "_say/adhoc" getpropstr
    if "Ad-hoc verbs enabled, " else "Ad-hoc verbs disabled, " then
  mee @ "_say/query" getpropstr
    if "Query enabled, " else "Query disabled, " then strcat
  mee @ "_say/normal" getpropstr
    if "NoSplit enabled." else "NoSplit disabled." then strcat
  mee @ OOC getpropstr tolower "yes" strcmp not
    if " Out-of-character." strcat then
  " " .tell .tell
;
: say-set ( message -- )
  strip dup not if pop say-summary exit then
  wordcut swap isch? over "+ooc" swap on-match not and if ch-verb exit then
  'say-verb    "+say"
  'feep        "feep"
  'clear-ch    "clear"
  'ooc-setverb "!ooc|+ooc"
  'here-verb   "+here"
  'bcst-set    "!broadcast"
  'quotes      "quotes"
  'flags       "!adhoc|!normal|!query"
  'suffix-ch   "!suffix|+suffix"
  'notify-set  "notify"
  10 22 pick ( items 2*items+2 )
  on-execute
  if pop "sayset: I don't recognise '" swap strcat
         "' as a function or a <ch>." strcat .tell pop
  else execute then
;
: sayflist ( db -- ) ( list _say/filter/* -> _say/db/[] : _say/desc/[] )
"-------------+----------------------------------------------------------------"
"Name         ! Description   [for further help, type 'sayhelp <Name>']"
  over .tell .tell dup .tell swap
  "_say/filter/"
  begin
   over swap nextprop
   dup not if pop pop .tell exit then
   over over getpropstr
   3 pick over
   dup atoi dbref dup owner dup player? if name else pop "Toaded" then
   swap int intostr "/" rot strcat strcat -3 rotate
   "_say/db/" swap strcat getpropstr
   rot 5 pick "_say/desc/" rot strcat getpropstr
   "! " swap strcat 13 swap tabulate
   "(#" rot ")" strcat strcat dup strlen 79 swap - swap tabulate
   .tell
  repeat
;
: flist ( n/0 db ch params func )  ( sayfilter <ch>/say [flist] )
  pop pop rot pop
  dup "say" stringcmp not if pop sayflist ( db -- ) exit then
  over over "_say/" swap strcat "/1" strcat getpropstr not if
  swap pop "Macro '" swap strcat "' has no filters." strcat .tell
  else
    prog swap "/" strcat 1
    begin
      over over intostr strcat
      dup 6 pick swap
      "_say/" swap strcat getpropstr
      dup not if pop pop pop  pop pop pop  "** End of list**" .tell exit then
      dup "_say/db/" swap strcat
      6 pick swap getpropstr
      dup if swap pop else pop "db#" swap strcat then
      ": " swap strcat strcat .tell
      1 +
    repeat
  then
;
: sayfadd ( n/0 db ch <FName dbint descr> -- )
( sayfilter say  fadd <FilterName> <dbint> <50 char descr> )
  swap pop rot
  if "sayfilter: .n irrelevant to 'say fadd'. Continuing." .tell then
  wordcut wordcut swap dup atoi not if
    1 strcut swap pop dup atoi not if
      "sayfilter: say fadd <FilterName> <Filterdb> <Description>" .tell
      pop pop pop pop exit
    then
  then
  "_say/db/" over strcat
  5 pick swap getpropstr dup not if pop else
    "sayfilter: filter #" 3 rotate strcat
    " already registered. 'sayfilter say ftake " strcat swap strcat
    "' first." strcat .tell
  pop pop pop exit then
  "_say/filter/" 4 pick tolower strcat 5 pick swap getpropstr if
    "sayfilter: '" 4 pick strcat "' already exists. 'sayfilter say ftake "
    strcat 4 pick strcat "' first." strcat .tell
  pop pop pop pop exit then
  4 pick "_say/desc/" 3 pick strcat 4 rotate 0 addprop
  3 pick "_say/db/" 3 pick strcat 4 pick 0 addprop
  "_say/filter/" rot tolower strcat swap 0 addprop
  "sayfilter: Filter registered." .tell
;
: fadd ( n/0 db ch params func -- )
  pop over "say" strcmp not if sayfadd exit then
( sayfilter <ch> fadd <filtername/dbint> )
  1 strcut over "#" strcmp not if swap pop else strcat then
  dup atoi not if
    dup "$" 1 strncmp not if
      "sayfilter: That's not a filter, it's a verb-program." .tell
      "sayfilter: You probably want 'sayset " rot strcat " " strcat
                                       swap strcat "'" strcat .tell
      pop pop exit
    then
  dup not if pop "xnullx" then
    "_say/filter/" swap tolower strcat
    prog swap getpropstr dup not if
      pop pop pop pop
      "sayfilter: I don't recognise that filter." .tell
      "sayfilter: Type 'sayfilter' to list registered filters." .tell exit
    then
  then
( n/0 db ch "1234" )
( Insert filter into list at n/end )
  3 pick "_say/" 4 pick strcat "/#" strcat over over getpropstr atoi
( n/0 db ch "1234" db "_say#" # )
  dup not if pop 1 then
  dup -4 rotate 1 + intostr 0 addprop
  5 rotate dup not if pop else
( move _say/ch/# into #+1, back to n -> n+1 )
( db ch "1234" # n )
    over over < if pop dup "sayfilter: Appending to the end." .tell then
    dup 0 < if pop 1 "sayfilter: Inserting as item 1." .tell then
    swap 5 pick 5 pick
    "_say/" swap strcat "/" strcat rot
    1 + over over intostr strcat 4 pick swap remove_prop ( Kill _say/ch/#+1 )
    begin
      over over intostr strcat 4 pick swap
      4 pick 4 pick 1 - dup 9 pick >= while
      intostr strcat
      moveprop 1 -
    repeat
    pop pop pop  pop pop pop  pop
  then
  "_say/" 4 rotate strcat "/" strcat swap intostr strcat
  swap atoi intostr 0 addprop
  "sayfilter: Filter added." .tell
;
: sayftake ( n/0 db ch "filtername/dbint" -- )
( sayfilter say ftake <filtername/dbint> )
  3 put pop
( "fname/db%" db )
  over atoi if swap atoi else
    dup "_say/filter/" 4 rotate tolower strcat getpropstr dup if
      atoi dup not if
        pop pop
        "sayfilter: I don't recognise that filter name." .tell
        "sayfilter: Type 'sayfilter' to list registered filters." .tell
        exit
      then
    else pop 0 then
  then
( db db% )
  dup not if
    pop pop "sayfilter: I can't use that filter name/dbref." .tell exit
  then
  intostr over over "_say/db/" swap strcat over over getpropstr
  "_say/filter/" swap tolower strcat 3 pick swap remove_prop remove_prop
  "_say/desc/" swap strcat remove_prop
  "sayfilter: Filter deregistered." .tell
;
: ftake ( n/0 db ch params func -- )
( sayfilter <ch> ftake.n    sayfilter <ch> ftake <filtername> )
  pop over "say" stringcmp not if sayftake exit then
( n/0 db ch "?fname" )
  4 pick 0 > if pop else
   (No number. Determine listpos of fname.)
   dup filternumber dup atoi not if
     dup "#" 1 strncmp if pop
       "sayfilter: I don't recognise filter '" swap strcat "'" strcat .tell
       "sayfilter: Type 'sayfilter' to list registered filters." .tell
       pop pop pop exit
     else
       1 strcut swap pop
     then
   then
   atoi intostr     ( Filter's program number now found. )
   swap pop 1 4 put
   "_say/" 3 pick strcat "/" strcat
   begin
     dup 5 pick swap 7 pick intostr strcat getpropstr dup if
       3 pick strcmp if 5 pick 1 + 5 put else pop pop break then
     else
       pop pop pop
       "sayfilter: I can't find that filter in the '" swap strcat
            "' filter list." strcat .tell
       pop pop exit
     then
   repeat
  then
( n db ch )
  over over "_say/" swap "/#" strcat strcat getpropstr atoi
  dup 1 = over 0 = or if
    pop "sayfilter: There are no filters in the '"
        over "' list anyway!" strcat strcat .tell
    "_say/" swap "/#" strcat strcat remove_prop pop
    exit
  then
( n db ch # )
  dup 5 pick dup 1 < -3 rotate <= or if
    "sayfilter: Position must be 1.." swap 1 - intostr strcat
                                  "." strcat .tell pop pop pop exit
  then pop
 ( now. move _say/ch/n+1 onto n etc )
( db _say/ch/ n )
  "_say/" swap strcat "/" strcat rot
  over over intostr strcat 4 pick swap remove_prop ( Kill _say/ch/n )
  begin
    over over intostr strcat 4 pick swap
    4 pick 4 pick 1 + intostr strcat 3 pick over getpropstr while
    moveprop 1 +
  repeat
  pop pop pop swap "#" strcat swap dup 1 <= if
    pop remove_prop
  else
    intostr 0 addprop
  then
  "sayfilter: Filter removed." .tell
;
: filter-word ( n/0 db ch params func -- )
( sayfilter <ch> < <word> <rest-of-data> > )
  " " strcat swap strcat
  4 pick if
    over "_say/" swap strcat "/" strcat
    5 pick intostr strcat
    4 pick swap getpropstr atoi
    dup if dbref call else
      pop pop pop "sayfilter: That filter number was out of range." "" 1
    then
  else
    begin
      4 pick 1 + 4 put
      over "_say/" swap strcat "/" strcat
      5 pick intostr strcat
      4 pick swap getpropstr atoi
      dup while
      dbref call 5 (notify | halt) bitand dup if dup then
    until
  then
( n me_notify other_notify 1 )
( n db ch params 0 )
  1 bitand if dup not if pop else
      loc @ me @ rot mee @ name " " strcat swap strcat notify_except
    then
    "sayfilter: Completed. " swap strcat .tell pop
  else
    3 put pop pop wordcut pop
      "sayfilter: Function '" swap strcat "' not recognised by filters."
    strcat .tell
  then
;
: say-filter ( message -- )
  strip dup not if pop prog sayflist exit then
  wordcut swap
  isch? if ploc @ else
    dup "say" stringcmp not if prog else    ( registered filters )
      dup "here" strcmp not if loc @ else   ( location filters )
        " " strcat swap strcat "def" ploc @
      then
    then
  then
  swap rot wordcut swap
  dup not if pop "flist" then
  dup "." instr dup if 1 - strcut 1 strcut 1 put atoi then
  -5 rotate
( n/0 db ch params func )
  4 pick controls? not if
    "sayfilter: You don't control that!" .tell pop pop pop  pop pop
  exit then
  dup "flist" stringcmp not if flist exit then
  dup "fadd"  stringcmp not if fadd  exit then
  dup "ftake" stringcmp not if ftake exit then
  3 pick "say" strcmp if filter-word else
    "sayfilter: You can't use 'say' here." .tell
    pop pop pop pop pop
  then
;
: say-help ( message -- ) ( sayhelp [<filtername>] [n] )
  strip wordcut prog
  3 pick dup dup atoi not and if
    dup 1 strcut pop "#" strcmp if
      "_say/filter/" swap tolower strcat getpropstr
      dup not if pop pop pop
        "sayhelp: I don't recognise that filtername." .tell
        "sayhelp: Type 'sayfilter' for registered filters." .tell exit
      then
    else 1 strcut 2 put pop then
  else pop pop "" then
  dup if atoi dbref 2 put else pop pop prog swap then
  atoi dup not if pop 1 then intostr
  "_help/" swap strcat "#/" strcat
 ( db "_help/n#/" )
  over int intostr
  dup "_say/db/" swap strcat prog swap getpropstr
  "(#" strcat over strcat "): " strcat
  "_say/desc/" rot strcat prog swap getpropstr
  strcat .tell
 ( db "_help/n#/" )
  over over "1" strcat getpropstr dup not if pop pop pop else
    1 swap begin .tell
      1 + over over intostr strcat 4 pick swap getpropstr dup not
    until pop pop pop pop
  then
;
: puppet ( params -- params "Object is..." db 0 )
         ( params -- me notify 1 )
  wordcut swap pop wordcut swap dup "#" 1 strncmp over and if match else
    1 strcut swap pop atoi dbref
  then
  dup int 1 < if
    pop pop me @ "sayfront: I can't use that puppet name or number." 1 exit
  then
  dup owner me @ dbcmp not if
    pop pop me @ "sayfront: The puppet must be owned by you." 1 exit
  then
  "sayfront: Object '" over name "' " strcat strcat swap
  0
;
: start ( params -- )
  dup "puppet" 6 strncmp if me @ mee ! else
    puppet if notify exit then
    dup mee ! "_puppet?" getpropstr if pop else
      over if "is not a puppet." else
        mee @ "_puppet?" "yes" 0 addprop
        "is now a puppet."
      then strcat .tell pop exit
    then
  then
  dup "!puppet" 7 strncmp not if
    puppet if notify exit then
    mee ! swap pop mee @ "_puppet?" getpropstr if
      mee @ "_puppet?" remove_prop
      "unpuppetted."
    else "is already not a puppet." then
    strcat .tell exit
  then
  0 snoop !
  dup "snoop" 5 strncmp not if
    "me" match .muckerlevel 3 < if
      pop "sayfront: You are not M3 or above." .tell exit
    then
    wordcut swap pop  ( kill 'snoop' )
    wordcut swap dup not if
      pop pop "sayfront: snoop <player/present object> [<command>]" .tell
    exit then
    dup match int 1 < if .pmatch else match then
    dup int 1 < if
      pop pop me @ "sayfront: I can't use that object name." .tell exit
    then
    dup mee ! owner "** SAYSET: " me @ name strcat
    " is snooping the 'say' properties of '" strcat
    mee @ name strcat "' **" strcat notify
    1 snoop !
  then
  mee @ "_proploc" getpropstr dup if
    atoi dbref dup ok? if dup "_say/" propdir? not if pop mee @ then
      dup controls? not if
        "sayfront: You must own your _proploc (#" swap int intostr strcat
        ")" strcat .tell pop
      exit then
    else
      "sayfront: Bad proploc object (#" swap int intostr strcat ")" strcat .tell
    mee @ then
  else pop mee @ then ploc !
  command @ "sayset"    strcmp not if say-set    exit then
  command @ "sayfilter" strcmp not if say-filter exit then
  command @ "sayhelp"   strcmp not if say-help   exit then
  command @ "sayreg"    strcmp not if
    prog "_sayreg" getpropstr atoi dbref call exit
  then
  if "sayfront: Please tell " trig owner name strcat
     ": Sayfront trigger '" strcat command @ strcat "' unrecognised."
     strcat .tell pop
  then
;
.
c
q
@register #me sayfront3-11.MUF=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called sayset3-11.MUF
@propset $tmp/prog1=str:/_help/1#:21
@propset $tmp/prog1=str:/_help/1#/1:SayHelp screen 1: Basic information
@propset $tmp/prog1=str:/_help/1#/10:                                      or "Yeeargh!!,,noisily %n shouts,,
@propset $tmp/prog1=str:/_help/1#/11:To enable [disable] ad-hoc verbs:        sayset adhoc   [sayset !adhoc]
@propset $tmp/prog1=str:/_help/1#/12:To make spilt says look unsplit:         sayset normal  [sayset !normal]
@propset $tmp/prog1=str:/_help/1#/13:`
@propset $tmp/prog1=str:/_help/1#/14:To see if someone is awake and present:  "?player   [or "?player,,?player  etc]
@propset $tmp/prog1=str:/_help/1#/15:To speak iff someone is awake:           "?player,,what to say [to player]
@propset $tmp/prog1=str:/_help/1#/16:To enable [disable] player-query:        sayset query   [sayset !query]
@propset $tmp/prog1=str:/_help/1#/17:To [un]set Out-Of-Character mode:        sayset ooc     [sayset !ooc]
@propset $tmp/prog1=str:/_help/1#/18:`
@propset $tmp/prog1=str:/_help/1#/19:To show all your say settings:           sayset
@propset $tmp/prog1=str:/_help/1#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/1#/20:`
@propset $tmp/prog1=str:/_help/1#/21:More help: sayhelp 2    LONG tutorial: sayhelp 99    Porting say: sayhelp 12
@propset $tmp/prog1=str:/_help/1#/3:To say something:                        "something.      [or 'say something.']
@propset $tmp/prog1=str:/_help/1#/4:To say something in two bits:            "something,,in two bits.
@propset $tmp/prog1=str:/_help/1#/5:To set your 'says' verb (seen/others):   sayset def barks,
@propset $tmp/prog1=str:/_help/1#/6:To set your 'say'  verb (seen by you):   sayset +def bark,
@propset $tmp/prog1=str:/_help/1#/7:To set your normal quoting pattern:      sayset quotes def <<%m>> [or "%m" etc]
@propset $tmp/prog1=str:/_help/1#/8:`
@propset $tmp/prog1=str:/_help/1#/9:To say something with an ad-hoc verb:    "Hullo there,,squeaks,,How are you?
@propset $tmp/prog1=str:/_help/10#:21
@propset $tmp/prog1=str:/_help/10#/1:SayHelp screen 10: The $verb interface.      [Feeeep!!]
@propset $tmp/prog1=str:/_help/10#/10: If the second verb is empty, the first is copied.
@propset $tmp/prog1=str:/_help/10#/11: Properties should be on db, and look like _say/ch/osay/whatever.
@propset $tmp/prog1=str:/_help/10#/12:`
@propset $tmp/prog1=str:/_help/10#/13: You may trap 'message' for commands iff it starts with ',,', but check that
@propset $tmp/prog1=str:/_help/10#/14:  the owner of db is the current player.
@propset $tmp/prog1=str:/_help/10#/15: If you set message blank (because it was a command), the player wo'n't speak.
@propset $tmp/prog1=str:/_help/10#/16:`
@propset $tmp/prog1=str:/_help/10#/17: The only use I can think for this is random verbs, and I've written that
@propset $tmp/prog1=str:/_help/10#/18: [badly] already, $RandomVerbs. If you have other ideas or a nicer
@propset $tmp/prog1=str:/_help/10#/19: implementation, please do tell.
@propset $tmp/prog1=str:/_help/10#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/10#/20:`
@propset $tmp/prog1=str:/_help/10#/21:sayhelp 5:rooms/envs  6:puppets  7,8,9:filter-writing  10:$verb  11:boringadmin
@propset $tmp/prog1=str:/_help/10#/3:Another tie-in, allowing program control of verbs.
@propset $tmp/prog1=str:/_help/10#/4:`
@propset $tmp/prog1=str:/_help/10#/5:Under 'sayset <ch> $<prog-number/name>',  whenever that 'verb' is found
@propset $tmp/prog1=str:/_help/10#/6: by say, the program is called and 'sayset +<ch> whatever' is ignored.
@propset $tmp/prog1=str:/_help/10#/7:`
@propset $tmp/prog1=str:/_help/10#/8:  call  ( message ch db "_say/ch" -- message ch "barks," "bark," )
@propset $tmp/prog1=str:/_help/10#/9:`
@propset $tmp/prog1=str:/_help/11#:30
@propset $tmp/prog1=str:/_help/11#/1:SayHelp screen 11: Administration an' yakyakyak.
@propset $tmp/prog1=str:/_help/11#/10:  sayreg take <say-name>       Deregisters the filter from Say.
@propset $tmp/prog1=str:/_help/11#/11:      or del  <say-name>
@propset $tmp/prog1=str:/_help/11#/12:`
@propset $tmp/prog1=str:/_help/11#/13:sayreg allow <playername>      Allows a player of M1/2 to register own filters
@propset $tmp/prog1=str:/_help/11#/14:sayreg ban   <playername>      Deregisters player.
@propset $tmp/prog1=str:/_help/11#/15:sayreg list                    Lists registered players.
@propset $tmp/prog1=str:/_help/11#/16:`
@propset $tmp/prog1=str:/_help/11#/17: These are a pretty version of: _say/db/1234 : DisplayName     [Note capitals]
@propset $tmp/prog1=str:/_help/11#/18: [On the sayfront program]      _say/desc/1234 : Short description text
@propset $tmp/prog1=str:/_help/11#/19:                                _say/filter/displayname : 1234   [no capitals]
@propset $tmp/prog1=str:/_help/11#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/11#/20:`
@propset $tmp/prog1=str:/_help/11#/21:sayset say says, : set global default. The default default is 'says,'
@propset $tmp/prog1=str:/_help/11#/22:sayset +say say, : set global default. The default default is 'say,'
@propset $tmp/prog1=str:/_help/11#/23: [On say trigger]   _say/say/osay:says,   _say/say/say:say,
@propset $tmp/prog1=str:/_help/11#/24:`
@propset $tmp/prog1=str:/_help/11#/25:sayset quotes say xyz : Outlaw characters xyz from any quote templates.
@propset $tmp/prog1=str:/_help/11#/26: [On say program]  _say/badquotes:xyz
@propset $tmp/prog1=str:/_help/11#/27:`
@propset $tmp/prog1=str:/_help/11#/28: sayset/filter snoop <player/obj> whatever    Treat player/Obj as a puppet.
@propset $tmp/prog1=str:/_help/11#/29:`
@propset $tmp/prog1=str:/_help/11#/3:INSTALLATION HAS MOVED TO sayhelp 12.
@propset $tmp/prog1=str:/_help/11#/30: Installation is now in sayhelp 12
@propset $tmp/prog1=str:/_help/11#/4:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/11#/5:sayreg:    Allows people other than me (author) to register/deregister filrers.
@propset $tmp/prog1=str:/_help/11#/6:------
@propset $tmp/prog1=str:/_help/11#/7:  sayreg add <sayname> $<name-from-@register> <Description of filter>
@propset $tmp/prog1=str:/_help/11#/8:  sayreg add <sayname> #<dbref> <Description of filter>
@propset $tmp/prog1=str:/_help/11#/9:                               These register the program with say.
@propset $tmp/prog1=str:/_help/12#:58
@propset $tmp/prog1=str:/_help/12#/1:SayHelp screen 12: Installation
@propset $tmp/prog1=str:/_help/12#/10:You may not sell or cause to be sold, any or all of this say system, for 
@propset $tmp/prog1=str:/_help/12#/11:profit or otherwise, alone or in some kind of accumulation, without first 
@propset $tmp/prog1=str:/_help/12#/12:having consulted me and abiding by any constraints I impose.
@propset $tmp/prog1=str:/_help/12#/13:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/12#/14:Say consists of three main programs: say, sayfront, and sayreg. Say actually
@propset $tmp/prog1=str:/_help/12#/15:does the speech bit; sayfront is a nicer front-end for setting say up, and
@propset $tmp/prog1=str:/_help/12#/16:appears to the user as sayset, sayfilter, and sayhelp. Sayreg is called through
@propset $tmp/prog1=str:/_help/12#/17:sayfront (for seedy historical reasons), and registers sayfilters to sayfilter's
@propset $tmp/prog1=str:/_help/12#/18:sayfilter list. :)
@propset $tmp/prog1=str:/_help/12#/19:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/12#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/12#/20:1. Type 'say' on its own. You will see say's dbref at the end of the version
@propset $tmp/prog1=str:/_help/12#/21:    message. Type sayreg on its own for its dbref. The dbref of sayfront is at
@propset $tmp/prog1=str:/_help/12#/22:    the top of this page.
@propset $tmp/prog1=str:/_help/12#/23:*  If you have problems with .broadcast add this before the var lines:
@propset $tmp/prog1=str:/_help/12#/24:                $def .broadcast pop
@propset $tmp/prog1=str:/_help/12#/25:   If you have problems with .popn, add the line
@propset $tmp/prog1=str:/_help/12#/26:                $def .popn begin dup while 1 - swap pop repeat pop
@propset $tmp/prog1=str:/_help/12#/27:*  If you have problems with .pmatch, add the line
@propset $tmp/prog1=str:/_help/12#/28:                $def .pmatch pop #0
@propset $tmp/prog1=str:/_help/12#/29:   If you have problems with .tell, add the line
@propset $tmp/prog1=str:/_help/12#/3:Permission is granted to port SAY and all filters which list as `Warwick'
@propset $tmp/prog1=str:/_help/12#/30:                $def .tell me @ swap notify
@propset $tmp/prog1=str:/_help/12#/31:   If you have problems with .muckerlevel, add the lines
@propset $tmp/prog1=str:/_help/12#/32:                $define .muckerlevel dup "Wizard" flag?
@propset $tmp/prog1=str:/_help/12#/33:                 if mlevel if 4 else 0 then else mlevel then $enddef
@propset $tmp/prog1=str:/_help/12#/34:   You should remove the lines, particularly those marked *, from say,
@propset $tmp/prog1=str:/_help/12#/35:   sayfront, or sayreg if your version contains them, but can work without them.
@propset $tmp/prog1=str:/_help/12#/36:2. Use @list to take a copy of those three programs on this mud, and @prog
@propset $tmp/prog1=str:/_help/12#/37:    to copy them to the new mud as say.muf, sayfront.muf, and sayreg.muf.
@propset $tmp/prog1=str:/_help/12#/38:    They MUST each be @set =L
@propset $tmp/prog1=str:/_help/12#/39:3. @act  say=#0       and  @act  sayset;sayhelp;sayfilter;sayreg=#0
@propset $tmp/prog1=str:/_help/12#/4:on sayfilter's list to another MUCK, so long as my name and copyrights remain,
@propset $tmp/prog1=str:/_help/12#/40:`  @link say=say.muf  and  @link sayset=sayfront.muf
@propset $tmp/prog1=str:/_help/12#/41:    The owner of the programs must also own the actions.
@propset $tmp/prog1=str:/_help/12#/42:4. @set say.muf=_version: and @set say.muf=_message: to the lines you see when
@propset $tmp/prog1=str:/_help/12#/43:    you type 'say' on its own. (Don't copy this MUCK's dbref for sayversion)
@propset $tmp/prog1=str:/_help/12#/44:5. @set sayfront.muf=_trigger:1234 (dbint of new global say action; no #)
@propset $tmp/prog1=str:/_help/12#/45:`  @set sayfront.muf=_sayreg:1234 (dbint of new sayreg program; no #)
@propset $tmp/prog1=str:/_help/12#/46:`  @set sayreg.muf=H
@propset $tmp/prog1=str:/_help/12#/47:6. Type sayfilter on this mud for a list of filters. Their dbrefs are listed
@propset $tmp/prog1=str:/_help/12#/48:    with them. You must transfer at least the Replace, Null, Halt and
@propset $tmp/prog1=str:/_help/12#/49:    @RandomVerbs filters.
@propset $tmp/prog1=str:/_help/12#/5:and so long as it's clear which filters are mine. (You can mention `Warwick'
@propset $tmp/prog1=str:/_help/12#/50:   Filters MUST be @set =L
@propset $tmp/prog1=str:/_help/12#/51:    (eg sayreg add Replace #1234 Replace filter. See sayhelp 4)
@propset $tmp/prog1=str:/_help/12#/52:7. Helptexts: lsedit sayfront.muf=_help/n  where 'n' is each page of the
@propset $tmp/prog1=str:/_help/12#/53:    helptext. Similarly for the filters: lsedit the filter program.
@propset $tmp/prog1=str:/_help/12#/54:    (ie _help/1 _help/2 .. _help/12. NB sayhelp 99)
@propset $tmp/prog1=str:/_help/12#/55:8. @set all program objects = 3 [Some don't need this, but set them anyway]
@propset $tmp/prog1=str:/_help/12#/56:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/12#/57:Bonus random tip: To get MPI executed in @lock  (courtesy of Revar)
@propset $tmp/prog1=str:/_help/12#/58:` @lock obj=_propname:true  @set obj=_propname:{if:{MPITEST},true}
@propset $tmp/prog1=str:/_help/12#/6:as the first word in the filter's description under sayreg, or preferably you
@propset $tmp/prog1=str:/_help/12#/7:can create an M3 character with my name to own the programs, which I get to use
@propset $tmp/prog1=str:/_help/12#/8:if I drop in to your MUCK :)
@propset $tmp/prog1=str:/_help/12#/9:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/2#:25
@propset $tmp/prog1=str:/_help/2#/1:SayHelp screen 2: Macro verb/filters.  [Shortcut for alternate speech styles]
@propset $tmp/prog1=str:/_help/2#/10:To set <ch>'s quoting:              sayset quotes <ch> o/~%mo/~
@propset $tmp/prog1=str:/_help/2#/11:To set space's verb(s):             sayset sp sniffles,      [sayset +sp etc ]
@propset $tmp/prog1=str:/_help/2#/12:To set the ooc verb(s):             sayset ooc says(OOC),   [sayset +ooc etc ]
@propset $tmp/prog1=str:/_help/2#/13:To unset <ch>'s verb:               sayset <ch>    [into 'says,' if <ch>==def]
@propset $tmp/prog1=str:/_help/2#/14:To unset everything on <ch>:        sayset clear <ch> [filters quotes & verbs]
@propset $tmp/prog1=str:/_help/2#/15:To seek <ch> as a suffix:           sayset suffix <ch>       [ch can be blank]
@propset $tmp/prog1=str:/_help/2#/16:                                or sayset +suffix <ch> [to remove ch if found]
@propset $tmp/prog1=str:/_help/2#/17:                                or sayset !suffix <ch> [to make <ch> a prefix]
@propset $tmp/prog1=str:/_help/2#/18:`
@propset $tmp/prog1=str:/_help/2#/19:If you unset +<ch>, you will see your speech as others see it, ie 'Fred barks'
@propset $tmp/prog1=str:/_help/2#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/2#/20:  rather than 'You bark,'.  If you .only. set +<ch>, it will not be used.
@propset $tmp/prog1=str:/_help/2#/21:If a verb ends with a comma, split say prepends your name [or 'you' for +<ch>]
@propset $tmp/prog1=str:/_help/2#/22:  If not, the name is appended.  '%n' is moved to the beginning in unsplit say.
@propset $tmp/prog1=str:/_help/2#/23:Quote formats must be 4..8 chars long, and contain (but not start/end with) %m.
@propset $tmp/prog1=str:/_help/2#/24:`
@propset $tmp/prog1=str:/_help/2#/25:For more help, type 'sayhelp 3'
@propset $tmp/prog1=str:/_help/2#/3:<ch> means a single character, 'sp' for space, 'def' for default, 'ooc' for ooc
@propset $tmp/prog1=str:/_help/2#/4:`
@propset $tmp/prog1=str:/_help/2#/5:To use the verb for macro <ch>:     "<ch>Aaargh!    [eg " Pooh!   "\Aaaargh! ]
@propset $tmp/prog1=str:/_help/2#/6:To set <ch>'s verb (seen/others):   sayset <ch> sings,    [eg sayset \ yells,]
@propset $tmp/prog1=str:/_help/2#/7:                                 or sayset <ch> quietly %n whistles,
@propset $tmp/prog1=str:/_help/2#/8:                                 or sayset <ch> barks%2 gruffly.
@propset $tmp/prog1=str:/_help/2#/9:To set <ch>'s verb (seen by you):   sayset +<ch> sing,
@propset $tmp/prog1=str:/_help/3#:22
@propset $tmp/prog1=str:/_help/3#/1:SayHelp screen 3: Speech filters.      [Speech filters modify 'spoken' text]
@propset $tmp/prog1=str:/_help/3#/10:To list all registered filters:    sayfilter 
@propset $tmp/prog1=str:/_help/3#/11:To get help from a filter:         sayhelp <filter-name/dbinteger> 1,2,3...
@propset $tmp/prog1=str:/_help/3#/12:`
@propset $tmp/prog1=str:/_help/3#/13:You may stack filters, but filter operations are not generally commutative.
@propset $tmp/prog1=str:/_help/3#/14: A <ch> macro will use default's filters if it has no filter of its own set.
@propset $tmp/prog1=str:/_help/3#/15:                                 Giving it the filter 'Null' will stop this.
@propset $tmp/prog1=str:/_help/3#/16:Example: sayfilter flist
@propset $tmp/prog1=str:/_help/3#/17:         def/filter/1: RandomSplit
@propset $tmp/prog1=str:/_help/3#/18:         def/filter/2: Replace
@propset $tmp/prog1=str:/_help/3#/19:         def/filter/3: db#38748
@propset $tmp/prog1=str:/_help/3#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/3#/20:         sayfilter ftake.2         [Removes filter number 2]
@propset $tmp/prog1=str:/_help/3#/21:      or sayfilter ftake replace   [Removes earliest 'replace' filter, ie 2]
@propset $tmp/prog1=str:/_help/3#/22:For more help, type 'sayhelp 4'
@propset $tmp/prog1=str:/_help/3#/3:A filter is a MUF program which processes your speech. The replace filter
@propset $tmp/prog1=str:/_help/3#/4:is probably the most useful to the casual user; see sayhelp 4's example.
@propset $tmp/prog1=str:/_help/3#/5:`
@propset $tmp/prog1=str:/_help/3#/6:To add a filter to <ch>:           sayfilter <ch> fadd <filter-name/dbint>
@propset $tmp/prog1=str:/_help/3#/7:To remove a filter from <ch>:      sayfilter <ch> ftake.n
@propset $tmp/prog1=str:/_help/3#/8:                                or sayfilter <ch> ftake <filter-name>
@propset $tmp/prog1=str:/_help/3#/9:To list all filters on <ch>:       sayfilter <ch> flist
@propset $tmp/prog1=str:/_help/4#:23
@propset $tmp/prog1=str:/_help/4#/1:SayHelp screen 4: The Replace filter (#70315)  [A working example filter]
@propset $tmp/prog1=str:/_help/4#/10:To list a replace list:            sayfilter <ch> list[.n]
@propset $tmp/prog1=str:/_help/4#/11:`
@propset $tmp/prog1=str:/_help/4#/12:[ You only need use the .n if an earlier filter, 'RandomSplit' in the example,
@propset $tmp/prog1=str:/_help/4#/13:                                              understands 'add' 'take' 'list' ]
@propset $tmp/prog1=str:/_help/4#/14:Example:
@propset $tmp/prog1=str:/_help/4#/15:    sayset clear %     [This clears out the % macro]
@propset $tmp/prog1=str:/_help/4#/16:    sayfilter % fadd replace
@propset $tmp/prog1=str:/_help/4#/17:    sayset % purrs                sayfilter % list
@propset $tmp/prog1=str:/_help/4#/18:    sayfilter % add er urr          _say/%/1/1: "er" -> "urr"
@propset $tmp/prog1=str:/_help/4#/19:    sayfilter % add or orr          _say/%/1/2: "or" -> "orr"
@propset $tmp/prog1=str:/_help/4#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/4#/20:    sayfilter % add \ is\  's\      _say/%/1/3: " is " -> "'s "
@propset $tmp/prog1=str:/_help/4#/21:    (Continue with next column)   "%It is perfectly normal,,
@propset $tmp/prog1=str:/_help/4#/22:                                  "%It's purrfectly norrmal," purrs KittyKat.
@propset $tmp/prog1=str:/_help/4#/23:sayhelp 5 (rooms/envs)  6 (puppets)  7,8,9 (filter-writing)  10 (boringadmin)
@propset $tmp/prog1=str:/_help/4#/3:The Replace filter replaces a list of <search> strings with <replace> strings
@propset $tmp/prog1=str:/_help/4#/4:wherever it can find them. (You can have different lists on different <ch>s,
@propset $tmp/prog1=str:/_help/4#/5:but if you set nothing for a given <ch>, the default list will be used [if it
@propset $tmp/prog1=str:/_help/4#/6:exists]). You may escape spaces '\ ' and escapes '\\'.
@propset $tmp/prog1=str:/_help/4#/7:`
@propset $tmp/prog1=str:/_help/4#/8:To add to a replace list:          sayfilter <ch> add[.n]  <search> <replace>
@propset $tmp/prog1=str:/_help/4#/9:To remove an item from the list:   sayfilter <ch> take[.n] <item-listnumber>
@propset $tmp/prog1=str:/_help/5#:31
@propset $tmp/prog1=str:/_help/5#/1:SayHelp screen 5: Room and environmental settings.
@propset $tmp/prog1=str:/_help/5#/10:          sayset +here bubble,
@propset $tmp/prog1=str:/_help/5#/11:          sayfilter here fadd replace
@propset $tmp/prog1=str:/_help/5#/12:          sayfilter here add able abubble
@propset $tmp/prog1=str:/_help/5#/13:          "It's indescribable!
@propset $tmp/prog1=str:/_help/5#/14:          Freda bubbles, "It's indescribabubble!"
@propset $tmp/prog1=str:/_help/5#/15:          You bubble, "It's indescribabubble!"
@propset $tmp/prog1=str:/_help/5#/16:`
@propset $tmp/prog1=str:/_help/5#/17:The Halt filter stops say climbing the environment for more filters.
@propset $tmp/prog1=str:/_help/5#/18:Typical use is 'sayfilteryset here fadd Halt' after a room's filters.
@propset $tmp/prog1=str:/_help/5#/19:`
@propset $tmp/prog1=str:/_help/5#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/5#/20:_listening and .broadcast are given an unsplit version of speech.
@propset $tmp/prog1=str:/_help/5#/21: You may inhibit say .broadcasting in the current room with 'sayset !broadcast'.
@propset $tmp/prog1=str:/_help/5#/22:`
@propset $tmp/prog1=str:/_help/5#/23: To add a local notification system. (Like couches), sayset notify dbref in
@propset $tmp/prog1=str:/_help/5#/24: a room or its environment. The dbref is to a program-object, with a
@propset $tmp/prog1=str:/_help/5#/25: template of ( "split-say" "unsplit-say" -- ), which should notify to the
@propset $tmp/prog1=str:/_help/5#/26: occupants of the room (including puppets and the room itself), depending
@propset $tmp/prog1=str:/_help/5#/27: upon the setting of each player's _say/normal? property. Normal is the
@propset $tmp/prog1=str:/_help/5#/28: unsplit version. If both are the same, you can do a notify_except for
@propset $tmp/prog1=str:/_help/5#/29: efficiency.
@propset $tmp/prog1=str:/_help/5#/3:If <ch> is 'here', then you change the current room's defaults [if you own it].
@propset $tmp/prog1=str:/_help/5#/30:`
@propset $tmp/prog1=str:/_help/5#/31:sayhelp 5:rooms/envs  6:puppets  7,8,9:filter-writing  10:$verb  11:boringadmin
@propset $tmp/prog1=str:/_help/5#/4:`
@propset $tmp/prog1=str:/_help/5#/5:A environmental verb setting is only used in preference to a player's default
@propset $tmp/prog1=str:/_help/5#/6:verb, but all environmental filters are applied to the player's speech after
@propset $tmp/prog1=str:/_help/5#/7:personal filters.
@propset $tmp/prog1=str:/_help/5#/8:`
@propset $tmp/prog1=str:/_help/5#/9: Example: sayset here bubbles,       [ Example: underwater ]
@propset $tmp/prog1=str:/_help/6#:20
@propset $tmp/prog1=str:/_help/6#/1:SayHelp screen 6: Puppets.
@propset $tmp/prog1=str:/_help/6#/10:  @link bs = #xxxxx                        [link 'bs' to say]
@propset $tmp/prog1=str:/_help/6#/11:  sayset puppet Warwick's                  [tell say that object is a puppet]
@propset $tmp/prog1=str:/_help/6#/12:  sayset puppet Warwick's def squeaks,     [the puppet squeaks..]
@propset $tmp/prog1=str:/_help/6#/13:  sayfilter puppet Warwick's fadd Jumble   [..and jumbles its words a bit]
@propset $tmp/prog1=str:/_help/6#/14:`
@propset $tmp/prog1=str:/_help/6#/15:If you type 'bs Hiya, I'm a light snack!', the puppet will talk.
@propset $tmp/prog1=str:/_help/6#/16:`
@propset $tmp/prog1=str:/_help/6#/17:If a room is set '_puppet_okay?:no', puppets will not function there.
@propset $tmp/prog1=str:/_help/6#/18:Typing 'sayset !puppet puppetname-or-#dbref' stops it being a say-puppet.
@propset $tmp/prog1=str:/_help/6#/19:`
@propset $tmp/prog1=str:/_help/6#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/6#/20:sayhelp 5:rooms/envs  6:puppets  7,8,9:filter-writing  10:$verb  11:boringadmin
@propset $tmp/prog1=str:/_help/6#/3:A puppet is an object with a messaging action attached, controlled by a player
@propset $tmp/prog1=str:/_help/6#/4:(see also spoof #help). To set up a say-puppet:
@propset $tmp/prog1=str:/_help/6#/5:`
@propset $tmp/prog1=str:/_help/6#/6:  @create Warwick's Breakfast            [spaces in the name become '-' in say]
@propset $tmp/prog1=str:/_help/6#/7:  @action bs = Warwick's                 ['bs' is this puppet's speech action]
@propset $tmp/prog1=str:/_help/6#/8:  say                                    [find out say's program number]
@propset $tmp/prog1=str:/_help/6#/9:  Say version n.nn by Warwick  (#xxxxx)
@propset $tmp/prog1=str:/_help/7#:22
@propset $tmp/prog1=str:/_help/7#/1:SayHelp screen 7: Writing your own filter.   [Part one, the main bit]
@propset $tmp/prog1=str:/_help/7#/10: 'message' is the player's speech.         default ->def and environment ->here
@propset $tmp/prog1=str:/_help/7#/11:`
@propset $tmp/prog1=str:/_help/7#/12:Return Bits: 0 (=1)  Abort filtering and say to display notify messages if set.
@propset $tmp/prog1=str:/_help/7#/13:('OR' these  1 (=2)  Request a filterlist re-call after speech printed.
@propset $tmp/prog1=str:/_help/7#/14:  together)  2 (=4)  No more filters run after this, but allow bit1 re-calling.
@propset $tmp/prog1=str:/_help/7#/15:`
@propset $tmp/prog1=str:/_help/7#/16:Bit 2 is mainly documented for compleatness. Users would normally fadd Halt.
@propset $tmp/prog1=str:/_help/7#/17:`
@propset $tmp/prog1=str:/_help/7#/18:Filter properties should be on db and of the form '_say/<ch>/n/whatever', with
@propset $tmp/prog1=str:/_help/7#/19: '/n/' signless (positive). ('n' -ve signals an ongoing re-call situation).
@propset $tmp/prog1=str:/_help/7#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/7#/20: Normal filters must treat re-calls as normal calls.
@propset $tmp/prog1=str:/_help/7#/21:`
@propset $tmp/prog1=str:/_help/7#/22:sayhelp 5:rooms/envs  6:puppets  7,8,9:filter-writing  10:$verb  11:boringadmin
@propset $tmp/prog1=str:/_help/7#/3:A filter is a MUF program whose transaction looks like this:
@propset $tmp/prog1=str:/_help/7#/4:       ( +/-n db <ch> message -- +/-n db <ch> changed-message 0/2/4/6 )
@propset $tmp/prog1=str:/_help/7#/5:    or ( +/-n db <ch> message -- +/-n me_notify others_notify 1 )
@propset $tmp/prog1=str:/_help/7#/6:`
@propset $tmp/prog1=str:/_help/7#/7: 'n'  is your place in the filterlist. (-ve means re-calling. See below.)
@propset $tmp/prog1=str:/_help/7#/8: 'db' is a relevant DBref.
@propset $tmp/prog1=str:/_help/7#/9: <ch> is the macro character, except ' '(space) becomes sp, ':' ->co, '/' ->sl,
@propset $tmp/prog1=str:/_help/8#:21
@propset $tmp/prog1=str:/_help/8#/1:SayHelp screen 8: Writing your own filter.   [Part two, extending sayfilter]
@propset $tmp/prog1=str:/_help/8#/10:You can choose your own function names, but don't use 'here', 'ooc', 'say',
@propset $tmp/prog1=str:/_help/8#/11: 'def', or any 1 or 2 character sequences lest they are read as <ch>.
@propset $tmp/prog1=str:/_help/8#/12:`
@propset $tmp/prog1=str:/_help/8#/13:The '.n' system (shown with the replace filter) is organised by say itself and
@propset $tmp/prog1=str:/_help/8#/14:is transparent to filters. You will not get the '.n' on the function word.
@propset $tmp/prog1=str:/_help/8#/15:`
@propset $tmp/prog1=str:/_help/8#/16:On entry, n, db and <ch> are appropriately set, message=='<function> <params>'.
@propset $tmp/prog1=str:/_help/8#/17:Use return value 0 if you don't recognise <function>.
@propset $tmp/prog1=str:/_help/8#/18:Use return 1 if you do, having set any properties you want to set in response.
@propset $tmp/prog1=str:/_help/8#/19: Return any errors in the me_notify string, or blank or '>> Done' if no errors.
@propset $tmp/prog1=str:/_help/8#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/8#/20:`
@propset $tmp/prog1=str:/_help/8#/21:sayhelp 5:rooms/envs  6:puppets  7,8,9:filter-writing  10:@verb  11:boringadmin
@propset $tmp/prog1=str:/_help/8#/3:The command 'sayfilter <ch> <function> <params>' is passed to filters using
@propset $tmp/prog1=str:/_help/8#/4:the same entrypoint as say, and you should check for it using 'command @'.
@propset $tmp/prog1=str:/_help/8#/5:`
@propset $tmp/prog1=str:/_help/8#/6:Minimally:     command @ "sayfilter" stringcmp not if 0 exit then
@propset $tmp/prog1=str:/_help/8#/7:`
@propset $tmp/prog1=str:/_help/8#/8:It is provided to make setting filter properties easier for the user.
@propset $tmp/prog1=str:/_help/8#/9:`
@propset $tmp/prog1=str:/_help/9#:18
@propset $tmp/prog1=str:/_help/9#/1:SayHelp screen 9: Writing your own filter.   [Part three, help for filters]
@propset $tmp/prog1=str:/_help/9#/10:I would prefer filters to do one thing, not many distinct things dependant
@propset $tmp/prog1=str:/_help/9#/11:on a property. This makes registration and use easier. Note that filters aren't
@propset $tmp/prog1=str:/_help/9#/12:restricted simply to altering speech, they can open/close doors on finding a
@propset $tmp/prog1=str:/_help/9#/13:keyword (for a 'here' filter) for instance, or even be tied to a morph program!
@propset $tmp/prog1=str:/_help/9#/14:`
@propset $tmp/prog1=str:/_help/9#/15:page #mail Warwick, or better email me (see pinfo Warwick) [or hassle the MUF
@propset $tmp/prog1=str:/_help/9#/16: wiz if I've not been on] to ask about registering filters.
@propset $tmp/prog1=str:/_help/9#/17:`
@propset $tmp/prog1=str:/_help/9#/18:sayhelp 5:rooms/envs  6:puppets  7,8,9:filter-writing  10:$verb  11:boringadmin
@propset $tmp/prog1=str:/_help/9#/2:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/9#/3:Help should be in a similar format to sayhelp screen 4, and arranged on the
@propset $tmp/prog1=str:/_help/9#/4:filter in the properties   _help/1#/1 2 3 ... lines for screen 1
@propset $tmp/prog1=str:/_help/9#/5:                           _help/2#/1 2 3 ... lines for screen 2  etc
@propset $tmp/prog1=str:/_help/9#/6:`
@propset $tmp/prog1=str:/_help/9#/7:A screen should be shorter than 22 lines, and the header should mention the
@propset $tmp/prog1=str:/_help/9#/8: program's registered name. sayhelp #nnnn shows help on any object #nnnn.
@propset $tmp/prog1=str:/_help/9#/9:`
@propset $tmp/prog1=str:/_help/99#:248
@propset $tmp/prog1=str:/_help/99#/1:SayHelp screen 99: Long tutorial
@propset $tmp/prog1=str:/_help/99#/10:Example computer output as:   - Warwick says, "This is an example"
@propset $tmp/prog1=str:/_help/99#/100:   - "You wouldn't dare!" growls Warwick, looking all fierce.
@propset $tmp/prog1=str:/_help/99#/101:`
@propset $tmp/prog1=str:/_help/99#/102:The trailing commas here distinguish a simple split say from an adhoc verb.
@propset $tmp/prog1=str:/_help/99#/103: Otherwise, you'd get:
@propset $tmp/prog1=str:/_help/99#/104:`
@propset $tmp/prog1=str:/_help/99#/105:   > "You wouldn't dare!,,growls %n, looking all fierce.
@propset $tmp/prog1=str:/_help/99#/106:   - "You wouldn't dare!" says Warwick, "growls %n, looking all fierce."
@propset $tmp/prog1=str:/_help/99#/107:`
@propset $tmp/prog1=str:/_help/99#/108:`
@propset $tmp/prog1=str:/_help/99#/109:A detail here is that in split speech, the name usually appears after the verb.
@propset $tmp/prog1=str:/_help/99#/11:`
@propset $tmp/prog1=str:/_help/99#/110: Putting a comma at the end of a verb makes say put the name before the verb.
@propset $tmp/prog1=str:/_help/99#/111:`
@propset $tmp/prog1=str:/_help/99#/112:   > "Look,,squeaks,,         - "Look," squeaks Warwick.
@propset $tmp/prog1=str:/_help/99#/113:   > "Look,,squeaks,,,        - "Look," Warwick squeaks.
@propset $tmp/prog1=str:/_help/99#/114:`
@propset $tmp/prog1=str:/_help/99#/115:This also happens to verbs set with sayset. A trailing comma has no effect on
@propset $tmp/prog1=str:/_help/99#/116: non-split style speech.
@propset $tmp/prog1=str:/_help/99#/117:`
@propset $tmp/prog1=str:/_help/99#/118:`
@propset $tmp/prog1=str:/_help/99#/119:Two special verb styles are available and both influence split speech.
@propset $tmp/prog1=str:/_help/99#/12:I will not echo the 'I've done that' reports from the computer.
@propset $tmp/prog1=str:/_help/99#/120:`
@propset $tmp/prog1=str:/_help/99#/121:There is an old form which disables you from using split speech for a macro:
@propset $tmp/prog1=str:/_help/99#/122:`
@propset $tmp/prog1=str:/_help/99#/123:   > sayset ! says, "%m"
@propset $tmp/prog1=str:/_help/99#/124:`
@propset $tmp/prog1=str:/_help/99#/125:   > "!This,,is,,split,,not,,at,,all
@propset $tmp/prog1=str:/_help/99#/126:   - Warwick says, "This,,is,,split,,not,,at,,all"
@propset $tmp/prog1=str:/_help/99#/127:`
@propset $tmp/prog1=str:/_help/99#/128:A second form allows you to put text after the second half of a split message:
@propset $tmp/prog1=str:/_help/99#/129:`
@propset $tmp/prog1=str:/_help/99#/13:`
@propset $tmp/prog1=str:/_help/99#/130:   > sayset ! says%2 and growls.
@propset $tmp/prog1=str:/_help/99#/131:`
@propset $tmp/prog1=str:/_help/99#/132:   > "!Shut up!
@propset $tmp/prog1=str:/_help/99#/133:   - Warwick says, "Shut up!" and growls.
@propset $tmp/prog1=str:/_help/99#/134:   > "!It's mine.,,bog off!
@propset $tmp/prog1=str:/_help/99#/135:   - "It's mine." Warwick says, "Bog off!" and growls.
@propset $tmp/prog1=str:/_help/99#/136:`
@propset $tmp/prog1=str:/_help/99#/137:----------------  QUERY
@propset $tmp/prog1=str:/_help/99#/138:`
@propset $tmp/prog1=str:/_help/99#/139:After writing split speech, I decided to do something about saying 'bye' to
@propset $tmp/prog1=str:/_help/99#/14:--------------------  SIMPLE VERBS
@propset $tmp/prog1=str:/_help/99#/140: people who had just left, but whose departure the lag had hidden.
@propset $tmp/prog1=str:/_help/99#/141:`
@propset $tmp/prog1=str:/_help/99#/142:   > sayset !query            [Disables this facility]
@propset $tmp/prog1=str:/_help/99#/143:   > sayset query             [Enables this facility]
@propset $tmp/prog1=str:/_help/99#/144:   > "?Riss,,Bye!
@propset $tmp/prog1=str:/_help/99#/145:   - Warwick says, "Bye!" to Riss.
@propset $tmp/prog1=str:/_help/99#/146:or - Riss is here but asleep.
@propset $tmp/prog1=str:/_help/99#/147:or - I don't see that here.
@propset $tmp/prog1=str:/_help/99#/148:`
@propset $tmp/prog1=str:/_help/99#/149:   > "?Riss,,?Kimi,,?Fuzz,,Aaargh!
@propset $tmp/prog1=str:/_help/99#/15:`
@propset $tmp/prog1=str:/_help/99#/150:   - Warwick says, "Aaargh!" to Fuzz, Riss and Kimi
@propset $tmp/prog1=str:/_help/99#/151:(This only works if Riss, Kimi *and* Fuzz are all present and awake.)
@propset $tmp/prog1=str:/_help/99#/152:`
@propset $tmp/prog1=str:/_help/99#/153:If you don't put a message at the end, you will just get a status report.
@propset $tmp/prog1=str:/_help/99#/154:`
@propset $tmp/prog1=str:/_help/99#/155:   > "?Riss
@propset $tmp/prog1=str:/_help/99#/156:   - Riss is here and awake.
@propset $tmp/prog1=str:/_help/99#/157:or - Riss is here but asleep
@propset $tmp/prog1=str:/_help/99#/158:`
@propset $tmp/prog1=str:/_help/99#/159:Note, if you're using this, you can't use the macro '?'
@propset $tmp/prog1=str:/_help/99#/16:I originally wrote say because I wanted to provide a way of switching verbs
@propset $tmp/prog1=str:/_help/99#/160:`
@propset $tmp/prog1=str:/_help/99#/161:---------------  OOC (Out Of Character) MODE
@propset $tmp/prog1=str:/_help/99#/162:`
@propset $tmp/prog1=str:/_help/99#/163:This was requested by Riss.
@propset $tmp/prog1=str:/_help/99#/164:`
@propset $tmp/prog1=str:/_help/99#/165:The intention here is to distinguish between a character who can answer
@propset $tmp/prog1=str:/_help/99#/166: techincal questions and ones who are capable of asking questions like
@propset $tmp/prog1=str:/_help/99#/167: 'What is a Florida?'
@propset $tmp/prog1=str:/_help/99#/168:`
@propset $tmp/prog1=str:/_help/99#/169:The flag _ooc is changed from _ooc:yes to nonexistence. Hopefully, pose and
@propset $tmp/prog1=str:/_help/99#/17: quickly.  This works as follows:
@propset $tmp/prog1=str:/_help/99#/170: other programs will support it sometime.
@propset $tmp/prog1=str:/_help/99#/171:`
@propset $tmp/prog1=str:/_help/99#/172:   > sayset !ooc      [Unsets OOC mode]
@propset $tmp/prog1=str:/_help/99#/173:   > sayset ooc
@propset $tmp/prog1=str:/_help/99#/174:   > "Type @link ba = #2
@propset $tmp/prog1=str:/_help/99#/175:   - Warwick says (OOC), (Type @link bs = #2)
@propset $tmp/prog1=str:/_help/99#/176:`
@propset $tmp/prog1=str:/_help/99#/177:You can customise OOC mode by typing 'ooc' where you could type 'def'.
@propset $tmp/prog1=str:/_help/99#/178:`
@propset $tmp/prog1=str:/_help/99#/179:---------------  FILTERS
@propset $tmp/prog1=str:/_help/99#/18:`
@propset $tmp/prog1=str:/_help/99#/180:`
@propset $tmp/prog1=str:/_help/99#/181:This was the other major introduction to speech.  A filter is a program,
@propset $tmp/prog1=str:/_help/99#/182: seperate from the say program, which is called by say to modify text or
@propset $tmp/prog1=str:/_help/99#/183: perform some other function.  If you are familiar with pipes in Unix, then
@propset $tmp/prog1=str:/_help/99#/184: this is similar.
@propset $tmp/prog1=str:/_help/99#/185:`
@propset $tmp/prog1=str:/_help/99#/186:To see a brief description of what sort of filters are available, type
@propset $tmp/prog1=str:/_help/99#/187:`
@propset $tmp/prog1=str:/_help/99#/188:   > sayfilter
@propset $tmp/prog1=str:/_help/99#/189:`
@propset $tmp/prog1=str:/_help/99#/19:A 'verb' is defined as the text that forms the verb after which your speech
@propset $tmp/prog1=str:/_help/99#/190:Name         | Description, For more help, type 'sayhelp <filtername>'
@propset $tmp/prog1=str:/_help/99#/191:-------------+----------------------------------------------------------
@propset $tmp/prog1=str:/_help/99#/192:$RandomVerbs | Not a filter. Selects random verbs.
@propset $tmp/prog1=str:/_help/99#/193:Reverse      | Reverses speech character by character
@propset $tmp/prog1=str:/_help/99#/194:Replace      | Standard text replacing filter
@propset $tmp/prog1=str:/_help/99#/195:-------------+----------------------------------------------------------
@propset $tmp/prog1=str:/_help/99#/196:`
@propset $tmp/prog1=str:/_help/99#/197:Because filters have help on them, I can only give general help here.
@propset $tmp/prog1=str:/_help/99#/198:`
@propset $tmp/prog1=str:/_help/99#/199:Filter help from  > sayhelp <filtername>
@propset $tmp/prog1=str:/_help/99#/2:--------------------------------
@propset $tmp/prog1=str:/_help/99#/20: appears.  The normal (or 'default') verbs are 'says' or 'say'. If you want to
@propset $tmp/prog1=str:/_help/99#/200:`
@propset $tmp/prog1=str:/_help/99#/201:A 'filter' which begins with '$' is a verb-program and supplies verbs via a
@propset $tmp/prog1=str:/_help/99#/202: program rather than reading fixed properties.
@propset $tmp/prog1=str:/_help/99#/203:`
@propset $tmp/prog1=str:/_help/99#/204:   > sayhelp $RandomVerbs
@propset $tmp/prog1=str:/_help/99#/205:   - ....
@propset $tmp/prog1=str:/_help/99#/206:   > sayset def $RandomVerbs
@propset $tmp/prog1=str:/_help/99#/207:`
@propset $tmp/prog1=str:/_help/99#/208: If a verb program takes commands, they are issued with
@propset $tmp/prog1=str:/_help/99#/209:`
@propset $tmp/prog1=str:/_help/99#/21: change those to 'purrs', for example:
@propset $tmp/prog1=str:/_help/99#/210:   > ",,<command>
@propset $tmp/prog1=str:/_help/99#/211:`
@propset $tmp/prog1=str:/_help/99#/212: See the verb-program helps for more information. eg > sayhelp $RandomVerbs
@propset $tmp/prog1=str:/_help/99#/213:`
@propset $tmp/prog1=str:/_help/99#/214:A normal filter modifies spoken text. You add a filter to a macro with
@propset $tmp/prog1=str:/_help/99#/215:`
@propset $tmp/prog1=str:/_help/99#/216:   > sayfilter def fadd <Filtername>     ['def' can also be '%' etc]
@propset $tmp/prog1=str:/_help/99#/217:`
@propset $tmp/prog1=str:/_help/99#/218:List which filters are on a macro with
@propset $tmp/prog1=str:/_help/99#/219:`
@propset $tmp/prog1=str:/_help/99#/22:`
@propset $tmp/prog1=str:/_help/99#/220:   > sayfilter def flist
@propset $tmp/prog1=str:/_help/99#/221:   - def/1 : Replace
@propset $tmp/prog1=str:/_help/99#/222:   - def/2 : Jumble
@propset $tmp/prog1=str:/_help/99#/223:   - def/3 : Quotation
@propset $tmp/prog1=str:/_help/99#/224:`
@propset $tmp/prog1=str:/_help/99#/225:And remove a filter with
@propset $tmp/prog1=str:/_help/99#/226:   > sayfilter def ftake <filtername>
@propset $tmp/prog1=str:/_help/99#/227:or > sayfilter def ftake.<position-in-flist-list>
@propset $tmp/prog1=str:/_help/99#/228:`
@propset $tmp/prog1=str:/_help/99#/229:To send a command to a filter on a macro, eg 'def',
@propset $tmp/prog1=str:/_help/99#/23:   > sayset def purrs,       [This sets the 'Warwick purrs' verb others see]
@propset $tmp/prog1=str:/_help/99#/230:`
@propset $tmp/prog1=str:/_help/99#/231:   > sayfilter def <command>
@propset $tmp/prog1=str:/_help/99#/232:`
@propset $tmp/prog1=str:/_help/99#/233:If you wanted to send the command 'foo bar 5' to the filter 'Quotation' (number
@propset $tmp/prog1=str:/_help/99#/234: 3 on the list), but you thought 'Replace' might know about it too, you can
@propset $tmp/prog1=str:/_help/99#/235: type:
@propset $tmp/prog1=str:/_help/99#/236:`
@propset $tmp/prog1=str:/_help/99#/237:   > sayfilter def foo.3 bar 5
@propset $tmp/prog1=str:/_help/99#/238:`
@propset $tmp/prog1=str:/_help/99#/239:This also works in taking fliters off macros.
@propset $tmp/prog1=str:/_help/99#/24:   > sayset +def purr,       [This sets the 'You purr' verb which only you see]
@propset $tmp/prog1=str:/_help/99#/240:`
@propset $tmp/prog1=str:/_help/99#/241:          > sayfilter def ftake.2
@propset $tmp/prog1=str:/_help/99#/242:but also: > sayfilter def ftake Jumble
@propset $tmp/prog1=str:/_help/99#/243:`
@propset $tmp/prog1=str:/_help/99#/244:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/99#/245:Sorry, this is scruffy at the end, but I'm tired and this should get you to the
@propset $tmp/prog1=str:/_help/99#/246: stage of being able to read sayhelp 1-11.
@propset $tmp/prog1=str:/_help/99#/247:. Err, sorry. I said it was late. 3-10
@propset $tmp/prog1=str:/_help/99#/248:-------------------------------------------------------------------------------
@propset $tmp/prog1=str:/_help/99#/25:`
@propset $tmp/prog1=str:/_help/99#/26:The verb switching facility works by looking at the first character of the
@propset $tmp/prog1=str:/_help/99#/27: message you give say, and seeing if it knows to associate that character
@propset $tmp/prog1=str:/_help/99#/28: with a verb.  The character is called the macro character.
@propset $tmp/prog1=str:/_help/99#/29:`
@propset $tmp/prog1=str:/_help/99#/3:`
@propset $tmp/prog1=str:/_help/99#/30:   > "%It's a green Wombat!                [Would normally produce..]
@propset $tmp/prog1=str:/_help/99#/31:   - Warwick says, "%It's a green Wombat!"
@propset $tmp/prog1=str:/_help/99#/32:`
@propset $tmp/prog1=str:/_help/99#/33:   > sayset % gasps,                       [But after this 'sayset'..]
@propset $tmp/prog1=str:/_help/99#/34:   > "%It's a green Wombat!
@propset $tmp/prog1=str:/_help/99#/35:   - Warwick gasps, "It's a green Wombat!"
@propset $tmp/prog1=str:/_help/99#/36:`
@propset $tmp/prog1=str:/_help/99#/37:If you want to see a 'You gasp' type message, you'll need to tell say about
@propset $tmp/prog1=str:/_help/99#/38: that verb too.  Put a '+' before the '%', the macro character.
@propset $tmp/prog1=str:/_help/99#/39:`
@propset $tmp/prog1=str:/_help/99#/4:Right, someone surprised me by saying that the sayhelp documentation was
@propset $tmp/prog1=str:/_help/99#/40:   > sayset +% gasp,
@propset $tmp/prog1=str:/_help/99#/41:   > "%It's a green Wombat!
@propset $tmp/prog1=str:/_help/99#/42:   - You gasp, "It's a green Wombat!"
@propset $tmp/prog1=str:/_help/99#/43:`
@propset $tmp/prog1=str:/_help/99#/44:Of course, you're not limited to the macro '%'. You may use any other
@propset $tmp/prog1=str:/_help/99#/45: single character. '~' and '@' no longer cause errors.
@propset $tmp/prog1=str:/_help/99#/46: If you want to set space as a macro, you must type
@propset $tmp/prog1=str:/_help/99#/47:`
@propset $tmp/prog1=str:/_help/99#/48:   > sayset sp squeaks,              > sayset +sp squeak,
@propset $tmp/prog1=str:/_help/99#/49:`
@propset $tmp/prog1=str:/_help/99#/5: confusing.  I suppose that one of the problems with developing a program
@propset $tmp/prog1=str:/_help/99#/50:To see your current state, type 'sayset' on its own.
@propset $tmp/prog1=str:/_help/99#/51:`
@propset $tmp/prog1=str:/_help/99#/52:You can clear verbs by putting a blank verb: > sayset def   > sayset +def
@propset $tmp/prog1=str:/_help/99#/53:`
@propset $tmp/prog1=str:/_help/99#/54:You can clear *everything* (verbs, quotes and filters) from a macro with
@propset $tmp/prog1=str:/_help/99#/55:`
@propset $tmp/prog1=str:/_help/99#/56:   > sayset clear def          > sayset clear %        ..and so on.
@propset $tmp/prog1=str:/_help/99#/57:`
@propset $tmp/prog1=str:/_help/99#/58:--------------------  QUOTES
@propset $tmp/prog1=str:/_help/99#/59:`
@propset $tmp/prog1=str:/_help/99#/6: is over-familiarity.  Believe me, the documentation on previous versions
@propset $tmp/prog1=str:/_help/99#/60:You can also set customised quotes for 'def' or a given macro character.
@propset $tmp/prog1=str:/_help/99#/61:`
@propset $tmp/prog1=str:/_help/99#/62:   > sayset quotes def <<%m>>
@propset $tmp/prog1=str:/_help/99#/63:   > "I'm hungry.
@propset $tmp/prog1=str:/_help/99#/64:   - Warwick says, <<I'm hungry>>
@propset $tmp/prog1=str:/_help/99#/65:`
@propset $tmp/prog1=str:/_help/99#/66:If a macro doesn't have its own quotes, it will use those for 'def'.
@propset $tmp/prog1=str:/_help/99#/67:`
@propset $tmp/prog1=str:/_help/99#/68:To set quotes to 'default', miss the %m bit out:  > sayset quotes &
@propset $tmp/prog1=str:/_help/99#/69:`
@propset $tmp/prog1=str:/_help/99#/7: were far worse.  I'd better not take up technical writing as a career.
@propset $tmp/prog1=str:/_help/99#/70:--------------------  SPLIT SPEECH AND ADHOC VERBS
@propset $tmp/prog1=str:/_help/99#/71:`
@propset $tmp/prog1=str:/_help/99#/72:After I had written the verb macro code, I decided to allow verbs to appear
@propset $tmp/prog1=str:/_help/99#/73: in the middle of speech.  This is not universally popular, and so a player
@propset $tmp/prog1=str:/_help/99#/74: may request only to see speech once it has been forced into the normal
@propset $tmp/prog1=str:/_help/99#/75: style, like  Warwick says, "Whatever".
@propset $tmp/prog1=str:/_help/99#/76:`
@propset $tmp/prog1=str:/_help/99#/77:   > sayset normal           [Forces others' speech into non-split format]
@propset $tmp/prog1=str:/_help/99#/78:   > sayset !normal          [You can see the split style speech]
@propset $tmp/prog1=str:/_help/99#/79:`
@propset $tmp/prog1=str:/_help/99#/8:`
@propset $tmp/prog1=str:/_help/99#/80: To utter spech in a split format, type ',,' where the verb should go.
@propset $tmp/prog1=str:/_help/99#/81:`
@propset $tmp/prog1=str:/_help/99#/82:   > "This is,,split style speech.
@propset $tmp/prog1=str:/_help/99#/83:   - "This is," Warwick says, "split style speech."
@propset $tmp/prog1=str:/_help/99#/84:`
@propset $tmp/prog1=str:/_help/99#/85:   > "This is split too,,
@propset $tmp/prog1=str:/_help/99#/86:   - "This is split too," Warwick says.
@propset $tmp/prog1=str:/_help/99#/87:`
@propset $tmp/prog1=str:/_help/99#/88:   > "%You can also,,use macros.
@propset $tmp/prog1=str:/_help/99#/89:   - "You can also," Warwick gasps, "use macros."
@propset $tmp/prog1=str:/_help/99#/9:Things you type appear as:    > "Type from after the '>' character
@propset $tmp/prog1=str:/_help/99#/90:`
@propset $tmp/prog1=str:/_help/99#/91:If you want to, you may use a verb without setting a macro, by allowing
@propset $tmp/prog1=str:/_help/99#/92: 'ad-hoc' verbs.
@propset $tmp/prog1=str:/_help/99#/93:`
@propset $tmp/prog1=str:/_help/99#/94:   > sayset !adhoc                             [Disallows 'adhoc' verbs]
@propset $tmp/prog1=str:/_help/99#/95:   > sayset adhoc                              [Allows 'adhoc' verbs]
@propset $tmp/prog1=str:/_help/99#/96:   > "This,,complains,,is getting confusing.
@propset $tmp/prog1=str:/_help/99#/97:   - "This," complains Warwick, "is getting confusing."
@propset $tmp/prog1=str:/_help/99#/98:`
@propset $tmp/prog1=str:/_help/99#/99:   > "You wouldn't dare!,,growls %n, looking all fierce,,
