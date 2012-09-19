@prog sayreg3-10.MUF
1 99999 d
1 i
( SayRegister 3-10.   Registers filters. For use by players M3,4 or allowed.
  must be set HUID and called from sayfront.
)
: ismaster? ( -- i )
 "me" match .muckerlevel 2 >
;
: wordcut ( s -- s' first-word )
  dup " " instr dup if 1 - strcut 1 strcut swap pop else pop "" then swap
;
: do-pmatch ( s -- db )
  "me" match dup pennies 1000000 < if 1 addpennies else pop then
  .pmatch
;
: add-filter   ( "<say-name> <#dbref-or-$name> Description" -- )
  wordcut swap wordcut
  1 strcut
  over "$" strcmp not if
  strcat match
  else
    over "#" strcmp if strcat else swap pop then
      atoi dbref
  then
  dup ok? not if int
    dup -1 = if
      pop "sayreg: nothing found with that program reference." .tell exit
    then
    dup -2 = if
      pop "sayreg: ambiguous program reference." .tell exit
    then
    dup -3 = if
      pop
      "sayreg: You want to register your HOME?!" .tell
      "sayreg: Just for that, I'm going to remove all your properties." .tell
      "sayreg: Hang on... Ah, what the heck, I'll do it in background." .tell
      fork dup if "sayreg: Child pid is " swap intostr strcat .tell exit then
      pop
      "" ""
      begin
        "me" match swap nextprop dup while
        swap over strcat "                 " (17 spaces)
strcat
        dup strlen dup 79 > if pop .tell "" 0 1 sleep then
        240 bitand strcut pop swap
      repeat
      pop .tell "**done** (heh heh)" .tell
      exit
    then
  then
  dup program? not if
    pop "sayreg: That appears not to be a program-object." .tell exit
  then
  ismaster? not if
    dup owner "me" match dbcmp not if
      pop "sayreg: You don't own that. Ask a Master or a Wizard." .tell exit
    then
  then
  int intostr
(
  "sayname" "description" "db"
  ...
   _say/db/nnnn:Name  _say/filter/name:nnnn  _say/desc/nnnn:Description
)
  "_say/db/" over strcat caller swap getpropstr dup if
    "sayreg: That program-object is already registered as '" swap "'"
    strcat strcat .tell exit
  then pop
  "_say/filter/" 4 pick tolower strcat caller swap getpropstr dup if
    "sayreg: That say-name is already used by #" swap strcat .tell exit
  then pop
  "_say/desc/" over strcat caller swap 4 rotate 0 addprop
  "_say/db/" over strcat caller swap 4 pick 0 addprop
  "_say/regdby/" over strcat caller swap "me" match int intostr 0 addprop
  "_say/filter/" rot tolower strcat caller swap rot 0 addprop
  "sayreg: Filter registered." .tell
;
: take-filter  ( "<say-name> <junk>" -- )
  wordcut swap pop
  tolower "_say/filter/" over strcat
  caller swap getpropstr
  dup not if
    "sayreg: I can't find anything with that name." .tell exit
  then
  ismaster? not if
    dup atoi dbref owner "me" match dbcmp not if
      "sayreg: You don't own that. Ask a Master or a Wizard." .tell exit
    then
  then
( nam nnnn :: _say/db/nnn:FilterName  _say/filter/name:nnn _say/desc/nnn  )
  "_say/desc/" over strcat caller swap remove_prop
  "_say/db/" swap strcat caller swap remove_prop
  "_say/filter/" swap strcat caller swap remove_prop
  "sayreg: Filter deregistered." .tell
;
: allow-player ( s -- )
  do-pmatch int intostr dup "_sayreg/reg" swap strcat prog
  swap rot 0 addprop
  "sayreg: Player allowed." .tell
;
: ban-player   ( s -- )
  do-pmatch int intostr "_sayreg/reg" swap strcat prog swap remove_prop
  "sayreg: Player banned." .tell
;
: list-players ( s -- )
  pop "List of allowed players:" .tell
  "" "_sayreg/"
  begin
    prog swap nextprop dup while
    prog over getpropstr atoi dbref name
    rot swap strcat "                 " (17 spaces)
strcat dup strlen dup 79 > if pop .tell "" 0 then
    240 bitand strcut pop swap
  repeat
  pop .tell "**done**" .tell
;
: sayreg ( s -- )
  ismaster? not if
    "me" match int intostr
    "_sayreg/reg" swap strcat
    prog swap getpropstr not if
"sayreg: (#" prog int intostr strcat ")" strcat .tell
      "sayreg: You are not registered." .tell exit
    then
  then
"sayreg: (#" prog int intostr strcat ")" strcat .tell
  dup not if
    "sayreg add  <say-name> <#dbnum or $name> <Description>" .tell
    "sayreg take <say-name>"      .tell
"    or del <say-name>" .tell
    ismaster? if
      "sayreg allow <playername>   [ Allows M1 or M2 player to use add and take ]" .tell
      "sayreg ban   <playername>" .tell
      "sayreg list" .tell
    then
" " .tell "Type sayhelp 11 for some sort of explanation." .tell
    pop exit
  then
  wordcut
  dup "add"   strcmp not if pop add-filter   exit then
dup "del"   strcmp not if pop take-filter  exit then
  dup "take"  strcmp not if pop take-filter  exit then
  ismaster? if
    dup "allow" strcmp not if pop allow-player exit then
    dup "ban"   strcmp not if pop ban-player   exit then
    dup "list"  strcmp not if pop list-players exit then
  then
  pop pop "sayreg: I don't recognise that command." .tell
;
.
c
q
@register #me sayreg3-10.MUF=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=H
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called sayreg3-10.MUF
@propset $tmp/prog1=str:/_sayreg/reg101423:101423
@propset $tmp/prog1=str:/_sayreg/reg63516:63516
