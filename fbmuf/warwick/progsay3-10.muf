@prog say3-10.MUF
1 99999 d
1 i
( Say program v 3.10 By Warwick on FurryMUCK2.2fb4.1  18th February 1993
 ----
 say is major action,  sayhelp;sayset;sayfilter provide front ending
 ----
 Properties on say program-object:
   _convert : optional <DBint of property conversion program>
   _version : Say version 3.10 {c} 1993 by Warwick on FurryMUCK.
   _message : Type 'sayhelp' for clarified help. Concerns to Warwick asap.
)
$define verb-chop-const 18 $enddef
$define OOC "_OOC" $enddef
$define toOlower "ooo" "o O" subst tolower "o O" "ooo" subst $enddef
lvar query
lvar ploc
lvar mee
: show-version ( -- )
  prog "_version" getpropstr
       " (#" prog int intostr ")" strcat strcat strcat .tell
  prog "_message" getpropstr .tell
;
: split-message? ( message -- partone parttwo 1 ) ( message -- message 0 )
    dup ",," instr
    dup not if exit then
    1 - strcut 2 strcut
    swap pop 1
;
: cut-end ( message -- messag e )         ( $define?  Phht => 'Primitive')
    dup if dup strlen 1 - strcut else "" then
;
: broadcast ( s -- s )
   loc @ "_say/nobcst" envpropstr swap pop not if dup .broadcast then
;
: do-query ( "query"          -- 0 )
           ( "query,,request" -- "request" 1 ) ( and update var query )
  split-message? dup if rot else swap then
  loc @ swap rmatch
  dup int 0 < if
    0 swap int - "Object is your home!"
                 "I don't know which one you mean."
                 "I can't see that here."
    4 rotate rotate -3 rotate pop pop swap
    if swap pop then
    .tell 0 exit
  then
( message 1 db  or 0 db )
  dup player? not if
    pop if pop then
    "That is not a player."
  else
    dup awake? if
      swap if
        name query @ dup if
          dup " and " instr if
            3 strcut rot
            " " swap "," strcat strcat
            swap strcat strcat
          else
            " and " strcat swap strcat
          then
        else
          pop " to " swap strcat
        then
        query ! 1 exit
      then
      " is here and awake."
    else
      swap if swap pop then
      " is here but asleep."
    then
    swap name swap strcat
  then
  .tell 0
;
: find-infix ( message -- message' osayt )
             (         -- message  "" )
    split-message? if
        split-message? if
            1 strcut over "," strcmp not if
                -3 rotate strcat
            else
                strcat swap
            then
            rot ",," strcat rot strcat swap exit
        then
        ",," swap strcat strcat
    then
    ""
;
: these-verbs? ( message ch db "_say/ch" -- message sayt osayt ch 1 )
               ( message ch db "_say/ch" -- message ch 0 )
  over over "/osay" strcat getpropstr dup if
    dup "$" 1 strncmp if
      -3 rotate "/say" strcat getpropstr
    else
      1 strcut swap pop atoi dbref call ( db "_say/ch" -- osayt sayt )
    then
    dup if swap else pop dup then rot 1
  else
    pop pop pop 0
  then
;
: get-verbs ( essage m -- message' sayt osayt ch ) (checked OK)
mee @ OOC getpropstr "yes" strcmp not if
    swap strcat ploc @ "_say/ooc" propdir? if "ooc" else "def" then
  else
dup "/: @~" over instr dup not if pop else
  2 * 2 - "slcospattw" swap strcut swap pop 2 strcut pop swap pop
    then
 "_say/" over strcat ploc @ swap propdir? if
       "_say/" over strcat "/suffix" strcat ploc @ swap getpropstr if
         pop swap strcat "def"
       else
         swap pop
       then
     else
       pop swap strcat "def"
     then
     dup "def" strcmp not if
       pop dup cut-end swap pop
       dup "/: @~" over instr dup not if pop pop else
         2 * 2 - "slcospattw" swap strcut swap pop 2 strcut pop 2 put pop
       then
       "_say/" over strcat ploc @ swap propdir? if
         "_say/" over strcat "/suffix" strcat ploc @ swap getpropstr
         dup if
           "s" strcmp if swap cut-end pop swap then
         else
           pop pop "def"
         then
       else pop "def" then
     then
  then
swap mee @ "_say/adhoc" getpropstr if find-infix else "" then
  dup if dup 4 rotate exit then pop swap
  dup "def" strcmp if
    ploc @ "_say/" 3 pick strcat these-verbs? if exit then
  then
  loc @ "_say/here/osay" envpropstr dup if
    dup "$" 1 strncmp if
      swap "_say/here/say" getpropstr
    else
      "_say/here" swap 1 strcut swap pop atoi dbref call
    then                                  ( db "_say/ch" -- osayt sayt )
    dup if swap else pop dup then
  else
    pop pop
    ploc @ "_say/def" these-verbs? if
      over "%m" instr if exit then
      rot verb-chop-const strcut pop
      rot verb-chop-const strcut pop
      rot exit
    then
    trigger @ "_say/say" these-verbs? if exit then
    "say," "says,"
  then rot
;
: run-filters ( +-n db ch message ["filter1"] -- message notbit0 )
                                           (  -- notify onotify bit0 )
    0 -6 rotate
    begin
      atoi dbref call ( +-n db ch message -- +-n db ch message notbit0 )
                      (                   -- +-n me other bit0 )
                      ( bit1 - repeat request  bit2 - list term )
      dup 1 bitand if 4 rotate pop 4 rotate pop exit then  ( bit 0 )
      6 pick over bitor 6 put
      dup 4 bitand not while pop  ( Return bit2 stops list and env climbing )
      3 pick
      "_say/" 4 pick strcat "/" strcat
      6 pick dup 0 > if 1 + dup else 1 - 0 over - swap then
      7 put intostr strcat getpropstr
    dup not until
    pop 3 put pop pop swap
;
: do-filter ( ch message -- message' 0/2   or -- notify notify_except 1 )
  over "2." 2 strncmp if 1 else swap 2 strcut swap pop swap -1 then
  -3 rotate ploc @ dup -4 rotate
  3 pick "def" strcmp if "_say/ch/1" 4 pick "ch" subst getpropstr
                    else pop "" then
  dup not if
    pop "def" 2 put ploc @ "_say/def/1" getpropstr then
  dup if
    5 pick -5 rotate run-filters   ( Run <ch> or 'def' filters )
    dup 1 bitand if pop 1 4 rotate pop exit then  ( bit0 - notify exit )
    dup 4 bitand if 2 bitand rot pop exit then    ( bit2 - 'halt')
  else pop 2 put pop 0 then
  3 pick rot loc @
  begin
    "_say/here/1" envpropstr dup while
    swap dup -6 rotate -3 rotate "here" -3 rotate run-filters
    dup 1 bitand if 3 put 3 put 3 put exit then          ( bit0 - notify exit )
    dup 4 bitand if rot bitor 2 bitand 2 put 2 put exit then  ( bit2 - 'halt' )
    rot bitor -3 rotate 4 pick swap rot
    dup #0 dbcmp if "" break then location
  repeat
  pop pop 3 put pop
;
: mix-split ( quotes verb t m2 m1 -- result 1 )
  cut-end ".,:;?!" over instr not if strcat "," then strcat
  5 pick " " strcat swap "%m" subst swap
  4 rotate dup "OOC" instr if
    "%^ooc&*" "OOC" subst tolower "OOC" "%^ooc&*" subst
  else tolower then
  dup "%n" instr not if
cut-end ".2,;:?!" over instr if
      "%n " -3 rotate query @ swap strcat
    else
      " %n" query @ "," strcat strcat
    then
    strcat strcat
  else
    dup dup "%n" instr 1 + strcut swap pop
1 strcut pop "'\".,!? " swap dup not if pop " " then instr not if "%n " "%n" subst then
  then
4 rotate 2 = if "you" else mee @ name "-" " " subst then
  "%n" subst swap dup if
" " 5 rotate strcat swap "%m" subst over "%2" instr if "," swap strcat "%2" subst else strcat then
else
pop rot pop "" "%2" subst cut-end dup "," strcmp not if pop "." then
strcat then
strcat 1
;
: mix-verb ( quote message verb t -- quote message result t )
                                    ( t:0-csay 1-osay 2-say )
  3 pick -3 rotate
over "%m" instr not over and if ( Split allowed? )
    rot split-message? if
      swap dup if 6 pick -5 rotate mix-split exit then pop
    then
  else rot then ( do conventional )
  split-message? if ", " swap strcat strcat then
  rot dup "%m" instr not if
    3 pick 2 = if "" " you" subst "" "you " subst "You " swap strcat
             else "" " %n"  subst "" "%n "  subst then
    cut-end ":;?!.," over instr not if strcat "," then strcat
dup "%2" instr if ", " 6 pick strcat "%2" subst else " " 6 pick strcat strcat then
then
  swap "%m" subst
  query @ dup if "." strcat then strcat
  swap 2 = not if
    " " swap strcat broadcast ( broadcast csay )
mee @ name "-" " " subst swap strcat
  then 0
;
: my-notify (osay csay -- )  (checked OK)
mee @ 1 loc @ contents
  begin
    dup player? if dup awake? else 0 then if
dup mee @ dbcmp not if
        dup "_say/normal" getpropstr not if
          swap 1 + over
          over 4 + pick notify ( Do osay notify )
          over
    then then then
    next dup ok? not
  until
  pop
  loc @ over 3 + put
  dup 2 + rotate
  notify_exclude
;
: mix-for-notify ( quotes sayt osayt message -- 0 osay csay )
                 (                           -- 1 )
                                             (Broadcast during mix-verb csay)
  dup not if pop pop pop pop show-version 1 exit then
  -3 rotate over over strcmp if  ( sayt != osayt )
      -4 rotate 2 (say) mix-verb pop mee @ swap notify
      3 pick 1 (osay) mix-verb if (osay)
          -4 rotate rot 0 (csay) mix-verb pop 2 put pop 0 exit
      else (csay)
          3 put pop pop dup 0 exit
      then
  else  ( sayt==osayt )
      -4 rotate 1 (osay) mix-verb if (osay)
          dup mee @ swap notify
          -4 rotate rot 0 (csay) mix-verb pop 2 put pop 0 exit
      else (csay)
          3 put pop pop me @ over notify dup 0 exit
  then then
;
: mix-notify ( quotes sayt osayt message -- )
  mix-for-notify if exit then
  loc @ "_say/notify" envpropstr swap pop atoi dup if
    dbref
    "GUARD-STRING" -4 rotate dup -5 rotate
      call ( osay csay -- )
    "GUARD-STRING" strcmp if
      "Bad _say/notify routine -- complain to " swap owner strcat abort
    else pop then
  else
    pop my-notify
  then
;
: get-quotes ( ch -- quotes )
  dup "def" strcmp if ploc @ "_say/" rot strcat "/quotes" strcat getpropstr
                 else pop "" then
  dup not if
    pop ploc @ "_say/def/quotes" getpropstr dup not if pop "\"%m\"" exit then
  then
  strip toOlower dup "" " %m " subst "" "%m" subst
  dup strlen 6 > swap strlen 2 < or                      ( Length 4..8 )
  over "!" instr or  over "*" instr or                   ( '!' & '*' banned )
  over "%m" instr not or                                 ( Contains '%m' ...  )
  over dup strlen 2 - dup 0 < if pop 0 then
  strcut swap pop "%m" strcmp not or                     ( but not at the end )
  over "%m" 2 strncmp not or if 1 else                   ( ...or at the start )
    prog "_say/badquotes" getpropstr dup if
      over "" "%m" subst
      begin
        dup not if pop pop 0 break then
        1 strcut 3 pick 3 pick
        instr if "SAY: Bad quote character '" rot strcat "'." strcat .tell
                 pop pop 1 break then
        "" rot subst (Try to clear quotes quicker. Dunno if this is worth it.)
      repeat
    then
  then if
   pop "\"%m\"" "SAY: You need to set valid quotes using 'sayset quotes'" .tell
  then
;
: make-say ( message -- )
  dup if 1 strcut swap else pop show-version exit then
get-verbs -4 rotate dup not if pop "says," then ( "" <- "says," )
over not if swap pop dup then                 ( say<-osay )
4 pick 4 pick dup not if pop pop pop  pop pop pop exit then (mesg empty)
  do-filter dup 1 = if
pop dup if mee @ name "-" " " subst " " strcat swap strcat
loc @ mee @ rot broadcast notify_except
    else pop then
    .tell pop pop pop pop
  else
    -4 rotate 6 pick get-quotes
    -4 rotate mix-notify if
      ( Re-call filter support. Phhht )
      "2." rot strcat swap do-filter
      1 = if
        dup if
mee @ name "-" " " subst " " strcat swap strcat
          loc @ me @ rot broadcast notify_except
        else pop then
        .tell pop pop
      else pop then
    else pop pop then
  then
;
: start-say ( message -- )
dup "#help" stringcmp not over "##help" stringcmp not or if "*** FOR HELP ON SAY, TYPE: 'sayhelp' ***" .tell then
( Puppet? )
    me @ mee !
    trigger @ location dup thing? over "_puppet?" getpropstr and if
      dup "dark" flag? if
        pop "Say: Puppets cannot be dark." .tell pop exit
      then
      loc @ "_puppet_okay?" envpropstr swap pop "no" 2 strncmp not if
        "Say: Puppetry not allowed here." .tell pop pop exit
      then
      dup mee !
      name " " explode
      begin dup while
        1 - swap .pmatch #-1 over dbcmp not if
          "Say: Illegal puppet name! (%n)" swap name "%n" subst .tell
          .popn
          exit
        then
        pop
      repeat
      pop
    else pop then
( Proploc? )
mee @ dup ploc ! "_proploc" getpropstr atoi dup if dbref dup ok? not if pop mee @ then "me" match over owner dbcmp not if pop mee @ then dup "/_say" propdir? if ploc ! else pop then else pop then
( Version update? )
mee @ "_say/version" getpropstr
prog "_version" getpropstr strcmp mee @ owner me @ dbcmp and if
mee @ "_say_version" getpropstr if
mee @ "_say_version" remove_prop
      prog "_convert" getpropstr atoi dup if dbref ploc @ swap call
                                        else pop then
mee @ "_say/version" prog "_version" getpropstr 0 addprop show-version
     then
    then
( Query request? )
    "" query !
    dup "?" 1 strncmp not if
mee @ "_say/query" getpropstr "yes" strcmp not if
        begin
          1 strcut swap pop
          do-query not if exit then
          dup "?" 1 strncmp
        until
    then then
    make-say
;
.
c
q
@register #me say3-10.MUF=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called say3-10.MUF
@propset $tmp/prog1=str:/_message:Type 'sayhelp' for help on say. Local notify-routines now supported. Sayhelp 5
@propset $tmp/prog1=str:/_version:Say version 3.11a (c)1993 by Warwick on FurryMUCK
