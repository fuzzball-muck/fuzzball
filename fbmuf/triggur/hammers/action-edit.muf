@prog actedit
1 19999 d
i
( menu-based action editor	)
( By triggur of Furrymuck 	)
( Thanks go to:
        K'has for M3 and lots of betatesting and ideas
        Lynx for the gen-desc routines
        Riss for ideas and testing
        WhiteFire for the quota routines
        Flinthoof for zetatesting
        Hormel for Spam

   If you must, contact kevink@moonpeak.com.
)
(gen desc things )
$include $lib/lmgr
$include $lib/strings
$include $lib/props
$include $lib/edit
lvar lbracket   ( count of left brackets found )
lvar search     ( current place to search )
var ltstr
var ltstr2
var ltint
var ltint2

( my things )
var dbnum	( generic dbref var )
var tstr	( a coupla generic temp strings )
var tstr2
var mpidesc	( using mpi description or not? )
var numexits	( # of exits in this room )
var tint	( a coupla generic temp ints )
var tint2
var len1
var len2
$include $lib/editor

$define print me @ swap notify $enddef

( CHANGE THESE DEFINES TO FIT YOUR SITE!!! )
  ( change this $define line if you don't want quota checking )
$ifdef __muck=furry
 $define CHECKQUOTAS 1 $enddef
$else
 $define CHECKQUOTAS 0 $enddef
$endif
  ( change this to the string to check action quota against)
$define ACTIONQUOTA "/@quota/exits" $enddef
  ( set this to the name of the prop containing the say verb )
$define SAYPROP "_say/def/say" $enddef
  ( set this to the name of the prop containing the osay verb )
$define OSAYPROP "_say/def/osay" $enddef
  ( set this prop to the cost in pennies of creating a room )
$define ACTIONCOST 1 $enddef
  ( set this to the string to look for for furry's @6800 equivalent...
    Do NOT use @$desc, as this is automatically searched for already,
    UNLESS there is no such local equiv.  Then just use "@$desc " )
$ifdef __muck=furry
 $define GENDESC "@6800 " $enddef
$else
 $define GENDESC "@$desc " $enddef   (BD has no such public equivalent)
$endif
  ( set this to the string to insert into the room descriptions )
$define DESCPROP "{eval:{list:redesc}}" $enddef

( ------------------------------------------------------------------------- )
: ok_name?
    dup not                                 if pop 0 exit then
    dup 1 strcut pop "*" stringcmp 0 =      if pop 0 exit then
    dup 1 strcut pop "#" stringcmp 0 =      if pop 0 exit then
    dup "=" instr                           if pop 0 exit then
    dup "&" instr                           if pop 0 exit then
    dup "|" instr                           if pop 0 exit then
    ( Check for ! at the start of each word )
    dup " !" instr                          if pop 0 exit then
    dup 1 strcut pop "!" stringcmp 0 =      if pop 0 exit then
    dup "me" stringcmp 0 =                  if pop 0 exit then
    dup "home" stringcmp 0 =                if pop 0 exit then
    dup "here" stringcmp 0 =                if pop 0 exit then

    pop 1 exit
;
( --------------------------------------------------------------- )
( gen-desc v2.0 written on May 11, 1992)

 
( --------------------------------------------------------------- )
: format-print ( s -- )
        dup not if
                pop exit
        then
        trigger @ swap pronoun_sub
        "_screen_width" dup me @ swap .locate-prop dup ok? if
                swap getpropstr atoi
        else
          pop pop "_screenwidth" dup me @ swap .locate-prop dup ok? if
                swap getpropstr atoi
          else
            pop pop "screenwidth" dup me @ swap .locate-prop dup ok? if
                swap getpropstr atoi
            else
                pop pop 78
            then
          then
        then                                            ( str width )
        dup 1 < if
            pop 1020
        then
        1 swap " -" swap dup 2 /                        ( str 1 " " w w/2 )
        EDITformat                                      ( s1 s2 ... sn n )
( THIS IS WHERE WE HOOK IN TO CAPTURE THE STRINGS!)
(        EDITdisplay)
;
 
( --------------------------------------------------------------- )

: add-desc-line ( s -- )
  dup not if   ( neglect empty lines )
    pop exit
  then
  format-print
  ltint2 !
  begin
    ltint2 @ not if break then
    ltint2 @ rotate ltstr !  (save string... )
    me @ "retempdesc#" getpropstr atoi 1 + intostr dup
    me @ swap "retempdesc#" swap 0 addprop
    me @ swap "retempdesc#/" swap strcat ltstr @ 0 addprop

    ltint2 @ 1 - ltint2 !
  repeat
  
  exit
;
: my-do-sub
  ltstr ! ltstr2 !  (store ickies)
  dup me @ swap getpropstr
  dup if
    begin		( this is REALLY cheeze )
      ltint @ not if break then
      dup me @ swap getpropstr dup if
        swap pop continue
      else
        pop break
      then
      ltint @ 1 - ltint !
    repeat
    swap pop rot swap strcat   (stick this string to the before string )
     swap ""
  else
    pop strip dup "%sub[" instr dup not if
      pop me @ swap "Error: unknown prop '" swap strcat "'." strcat notify
      "" ""
    else
      4 + strcut swap pop  ( snip off "%sub[" )
      dup "]" rinstr dup not if
        pop pop pop me @ "ERROR: bad format of %sub[], missing ']'." notify
        "" ""
      else
        1 - strcut pop
        ltint @ 1 + ltint !
        "%sub[" "do-sub" my-do-sub 
      then
    then
  then
  exit
;
 
: get-legal-prop ( d s -- s' )
        over owner me @ owner dbcmp
        3 pick owner trigger @ owner dbcmp
        4 pick "_proploc_ok?" getpropstr .yes?
        or or if
                getpropstr
        else
                pop pop ""
        then
;
 
: count-char ( s c -- i )
        0 rot rot                                       ( 0 s c )
        begin over over instr dup while                 ( i s c pos )
                rot swap strcut swap pop                ( i c sr )
                rot 1 +                                 ( c sr i+1 )
                swap rot                                ( i+1 sr c )
        repeat
        pop pop pop                                     ( i )
;
 
: count-brackets ( s -- )
        dup "[" count-char swap "]" count-char          ( il ir )
        - lbracket @ +
        lbracket !
;
 
: cut-next-delimiter ( s c -- sl sr )
        "" rot rot                                      ( "" s c )
        0 lbracket !
        begin
                over over instr dup if                  ( sl sr c pos )
                        rot swap 1 - strcut 1 strcut swap pop
                                                        ( sl c sm sr )
                        over count-brackets
                        lbracket @ not if
                            rot pop                     ( sl sm sr )
                            rot rot strcat swap         ( sl+sm sr )
                            exit
                        else
                            swap 3 pick strcat          ( sl c sr sm+c )
                            4 rotate swap strcat        ( c sr sl+sm+c )
                            swap rot                    ( sl+sm+c sr c )
                            dup "]" strcmp not if
                                lbracket @ 1 - lbracket !
                            then
                            0                           ( sl+sm+c sr c 0 )
                        then
                else
                        pop pop strcat ""               ( sl+sr "" )
                        exit
                then
        until
        pop
;
 
: get-next-token ( s -- sl sr args token 1 or s 0 )
        dup "%" instr dup if                            ( s pos )
                1 - strcut dup "[" instr dup if         ( sl sr pos )
                        strcut "]" cut-next-delimiter   ( sl token args sr )
                        swap rot 1                      ( sl sr args token 1 )
                else
                        pop strcat 0
                then
        else
                pop 0
        then
;
 
: split-args ( args -- a[1] a[2] ... a[n] n )
        1
        begin swap "," cut-next-delimiter dup while     ( ... n a[n] a[n+1] )
                rot swap                                ( ... a[n] n a[n+1] )
                dup if
                        swap 1 +                        ( ... a[n] a[n+1] n+1 )
                else
                        swap
                then
        repeat
        pop swap
;
 
: wipe-list ( s[1] ... s[n] n -- )
        begin dup while
                swap pop 1 -
        repeat
        pop
;
 
: select-item ( s a[1] f[1] a[2] f[2] ... a[n] f[n] n -- s f[n] )
        dup 2 * 2 + rotate                              ( ... n s )
        begin over while
                swap 1 - swap                           ( ... n-1 s )
                4 rotate 4 rotate                       ( ... n s a[n] f[n] )
                rot rot over swap smatch if             ( ... n f[n] s )
                        begin 3 pick while
                                4 rotate pop 4 rotate pop
                                3 rotate 1 - -3 rotate
                        repeat                          ( 0 f[n] s )
                        pop swap pop exit               ( f[n] )
                else
                        swap pop                        ( ... n s )
                then
        repeat
        pop pop 0
;
 
lvar debug
: eval-loop ( s -- s' )
        dup not if
            exit
        then
        "" swap                                         ( "" s )
        begin get-next-token while                      ( sl sm sr args token )
                5 rotate 5 rotate strcat -4 rotate      ( sl' sr args token )
                me @ "_debug" getpropstr 1 strcut pop "y" stringcmp not if
                        me @ trigger @ owner dbcmp
                        me @ "W" flag? or if
                                over over swap strcat "]" strcat
                                debug !
                        then
                then
                dup
                "%sub\\[" "do-sub"
                "%rand\\[" "do-rand"
                "%time\\[" "do-time"
                "%date\\[" "do-date"
                "%concat\\[" "do-concat"
                "%select\\[" "do-select"
                "%if\\[" "do-if"
                "%yes\\[" "do-yes"
                "%no\\[" "do-no"
                "%me\\[" "do-me"
                "%here\\[" "do-here"
                "%@*\\[" "do-program"
                "%prog#*\\[" "do-progmultargs"
                "%prog$*\\[" "do-progmultargs"
                "%\\+*\\[" "do-macro"
                "%char\\[" "do-char"
                "%otell\\[" "do-otell"
                "%prand\\[" "do-prand"
                18 select-item                          ( sl' sr args token f )
                dup if
dup "do-sub" stringcmp not if	( el Triggur Hacko )
   my-do-sub
else
                        "$desc-lib" match swap call     ( sl' sr result )
then
                        eval-loop
                        me @ "_debug" getpropstr 1 strcut pop "y" stringcmp not
   if
                                me @ trigger @ owner dbcmp
                                me @ "W" flag? or if
                                        debug @ " = " strcat over strcat
                                        .tell
                                then
                        then
                else
                        pop swap strcat "]" strcat      ( sl' sr token+args )
                        1 strcut rot                    ( sl' "%" rest sr )
                        strcat swap                     ( sl' sr' "%" )
                then
                rot swap strcat swap                    ( sl'+result sr )
        repeat
        strcat                                          ( s' )
;
: my-do-list
  pop pop
  "#" strcat ltstr2 !  (save list name)
  swap
  add-desc-line   (save BEFORE )
  1 ltint !
  begin		( copy to list )
    ltint @ me @ ltstr2 @ getpropstr atoi > if break then
    me @ ltstr2 @ "/" strcat ltint @ intostr strcat getpropstr add-desc-line
    ltint @ 1 + ltint !
  repeat
  "" swap
  ""
  exit
;
: my-do-newline
  pop pop
  pop
  swap add-desc-line
  "" swap
  ""
  exit
;
 
: second-pass ( s -- s' )
        dup not if
            exit
        then
        "" swap                                         ( "" s )
        begin get-next-token while                      ( sl sm sr args token )
                5 rotate 5 rotate swap strcat -4 rotate ( sl' sr args token )
                dup
                "%list\\[" "list"
                "%call\\[" "newline"
                "%nl\\[" "newline"
                3 select-item                           ( sl' sr args token f )
                dup if
dup "list" stringcmp not if my-do-list 
else dup "newline" stringcmp not if my-do-newline then then
                        ("$desc-lib" match swap call)     ( sl' sr result )
                        eval-loop
                else
                        pop swap strcat "]" strcat      ( sl' sr token+args )
                        1 strcut rot                    ( sl' "%" rest sr )
                        strcat swap                     ( sl' sr' "%" )
                then
                rot swap strcat swap                    ( sl'+result sr )
        repeat
        strcat                                          ( s' )
;
: escape-chars ( s -- s' )
        "%char[" "%" .asc intostr strcat "]" strcat      ( s "%char[num]" )
        "%%" subst                                      ( s1 )
        begin dup "\\" instr dup while                  ( s1 pos )
              1 - strcut 1 strcut swap pop 1 strcut     ( sl c sr )
              swap .asc intostr                          ( sl sr num )
              "%char[" swap strcat "]" strcat           ( sl sr "%char[num]" )
              swap strcat strcat                        ( s' )
        repeat
        pop
;
 
: gen-desc ( s -- )
  0 ltint !
  me @ "retempdesc#" "0" 0 addprop
  escape-chars
  eval-loop
  second-pass
  dup if
    format-print
    dup ltint ! if	( add remaining strings to the list )
      begin
        ltint @ not if break then
        ltint @ rotate add-desc-line
        ltint @ 1 - ltint !
      repeat    
    else
      pop
    then
  else
    pop
  then
;

( --------------------------------------------------------------- )
: make-action ( -- success )
$ifdef CHECKQUOTAS=1
  me @ ACTIONQUOTA getpropstr atoi dup if
   me @ stats pop pop pop pop swap pop    ( get the # exits owned )
    swap pop > not if
      me @ "Sorry... you cannot create another action.. you have reached your quota." notify
      #0 exit
    then
  else
    pop
  then
$endif

  me @ pennies ACTIONCOST < if
    me @ "Sorry... you cannot afford another action.  They cost " ACTIONCOST intostr strcat " to build." strcat notify
    0 exit
  then

  loc @ "new action" newexit
  dup not if
     me @ "ERROR:  Could not create action, reason unknown.  Ask a wizard or helpstaff for help." notify
  else
    me @ 0 ACTIONCOST - addpennies
  then
  exit
;
( --------------------------------------------------------------- )
: in-program-tell ( s -- i )
( ----- 1/23/98 --------------------- )
( Emergency fix for bug found by Flinthoof )
  me @ "W" flag? not me @ location owner me @ dbcmp not and if (not owner and not wizard!)
    me @ "You cannot edit this room.  Type '" trig name strcat " #HELP' for help." strcat notify
    pid kill
  then
( ----- END PATCH --------- )

  striplead
  dup ":" instr 1 = if          ( player posed )
    1 strcut swap pop
    me @ name " " strcat swap strcat
    loc @ swap #-1 swap notify_except
    1 exit
  else dup "\"" instr 1 = if    ( player spoke )
    dup me @ swap "You " me @ SAYPROP getpropstr dup not if pop "say, " then strcat
    swap strcat "\"" strcat notify
    loc @ swap me @ swap me @ name " " strcat me @ SAYPROP getpropstr dup not if
 pop "says, " then strcat
    swap strcat "\"" strcat notify_except
    1 exit
  else                          ( neither... return 0 )
    pop
  then then
  0 exit
;

( --------------------------------------------------------------- )
: padto ( s i -- s )
  len1 !
  dup strlen len2 !
  len1 @ len2 @ < if	( snip string to fit )
    len1 @ strcut pop
    exit
  then
  len1 @ len2 @ > if	( pad out )
    "                                          " len1 @ len2 @ - strcut pop
    strcat
  then
  exit
;

( --------------------------------------------------------------- )
: header ( -- )
  me @ "Action Hammer V1.1 (C) 1994 by Triggur" notify
  me @ "  (with extra minters for Flinters)   " notify
  me @ "--------------------------------------" notify
  me @ " " notify
  exit
;

( --------------------------------------------------------------- )
: show-exits ( -- )
  me @ "#    dbnum  name(s)" notify
  me @ "------------------------------------------------------------" notify
  1 tint !
  loc @ exits
  begin			( loop through all exits here )
    dup dbnum !
    #-1 dbcmp if
      break
    then
    dbnum @ owner me @ dbcmp not if	( not owner... skip )
      dbnum @ next
      continue
    then
    me @ tint @ intostr "." strcat 5 padto		( append exit num )
    dbnum @ int intostr 7 padto strcat	( append dbref num )
    dbnum @ name strcat			( append names )
    notify				(...aaaannnnnd PRINT )
    tint @ 1 + tint ! 
    dbnum @ next
  repeat
  tint @ numexits ! 
  me @ " " notify
  tint @ 1 = if
    me @ "(You own no action in this room)" notify
    me @ " " notify
    me @ "1. Create a new action." notify
    me @ " " notify
    me @ "Enter '1' to create a new action, or 'Q' to QUIT." notify
  else
    me @ tint @ intostr ". Create a new action." strcat notify
    me @ " " notify
    me @ "Enter option number to modify, or 'Q' to QUIT." notify
  then
  exit
;

( --------------------------------------------------------------- )
: show-exit ( -- )

  
  me @ "NUMBER   : " dbnum @ intostr strcat notify

  me @ "1. NAMES : " dbnum @ name strcat notify

  dbnum @ desc tstr !
  tstr @ "{" instr 1 = if    ( description in MPI)
    1 mpidesc !
    me @ "2. MPI DESC:" notify
    1 tint !
    begin
      dbnum @ "redesc#/" tint @ intostr strcat getpropstr
      dup not if
        pop
        break
      then
      me @ swap notify
      tint @ 1 + tint !
    repeat
    tint @ 1 = if
      me @ "   ''" notify
    then
  else
    0 mpidesc !
    me @ "2. PROP DESC: '" tstr @ strcat "'" strcat notify
  then

  me @ "3. Action is" dbnum @ "d" flag? not if " NOT" strcat then " invisible." strcat notify

  me @ "4. Broadcasting is" dbnum @ "_broadcast?" getpropstr "yes" stringcmp if " NOT" strcat then " enabled." strcat notify

  dbnum @ "_/lok" getpropstr if
    me @ "5. Lock is " dbnum @ "_/lok" getpropstr strcat notify
  else
    me @ "5. Exit is NOT locked." notify
  then

  dbnum @ succ if 
    me @ "6. When used, user sees '" dbnum @ succ strcat "'." strcat notify
  else
    me @ "6. When used, user sees nothing." notify
  then

  dbnum @ osucc if
    me @ "7. When used, others see '<name> " dbnum @ osucc strcat "'." strcat notify
  else
    me @ "7. When used, others see nothing." notify
  then

  dbnum @ fail if
    me @ "8. When locked, user sees '" dbnum @ fail strcat "'." strcat notify
  else
    me @ "8. When locked, user sees nothing." notify
  then

  dbnum @ ofail if
    me @ "9. When locked, others see '<name> " dbnum @ ofail strcat "'." strcat notify
  else
    me @ "9. When locked, others see nothing." notify
  then

  dbnum @ odrop if
    me @ "10. When done, others see '<name> " dbnum @ odrop strcat "'." strcat notify
  else
    me @ "10. When done, others see nothing." notify
  then

  me @ "11. Action is linked to " dbnum @ getlink tint2 !
     tint2 @ program? if
       "program '" strcat tint2 @ name strcat "' (#" strcat tint2 int
       intostr strcat ")." strcat notify
     else tint2 @ room? if
       "room '" strcat tint2 @ name strcat "' (#" strcat tint2 int
       intostr strcat ")." strcat notify
     else
       "nothing." strcat notify
     then then
  
  me @ " " notify
  me @ "12. Delete this action." notify
  me @ "13. Return to list of actions." notify
  me @ " " notify
  me @ "Enter '1' through '13', or 'Q' to QUIT." notify
  exit
;

( --------------------------------------------------------------- )
: edit-desc
  me @ " " notify
  mpidesc @ not if
    begin
      me @ "Would you like to convert this description to MPI (This only works well for SIMPLE descriptions, not complex @$desc ones)? (YES/no)" notify
      read
      dup in-program-tell not if break then pop
    repeat
    1 strcut pop "n" stringcmp if	( convert to MPI! )
      dbnum @ desc striplead "@$desc " instr 1 =
      dbnum @ desc "" "@$desc " subst "" GENDESC subst

      gen-desc   (turn desc into raw strings )
      me @ "retempdesc#" getpropstr atoi tint !
      dbnum @ DESCPROP setdesc
      dbnum @ "redesc#" tint @ intostr 0 addprop  (store # of lines )
      begin
        tint @ not if break then                ( store new desc )
        dbnum @ "redesc#/" tint @ intostr strcat me @ "retempdesc#/" tint @ intostr strcat getpropstr 0 addprop
        tint @ 1 - tint !
      repeat
      me @ "Description converted.  Entering editor..." notify
      1 mpidesc !
    then
  then
  mpidesc @ if	( edit MPI )
   1 tint !
   begin
     dbnum @ "redesc#/" tint @ intostr strcat getpropstr
     dup if
       1 tint @ + tint !
     else
       pop
       break
     then
   repeat
   tint @ 1 -
   EDITOR
   "end" stringcmp not if	( editor saved! )
     dbnum @ "redesc#" getpropstr atoi tint !  (erase old text)
     begin
       dbnum @ "redesc#/" tint @ intostr strcat remove_prop
       tint @ 1 - dup tint !
     1 < until

     dup tint !	( store the new description list )
     dbnum @ "redesc#" tint @ intostr 0 addprop  (store new # of lines)
     not if exit then	( exit if description was just cleared )
     begin
       dbnum @ swap "redesc#/" tint @ intostr strcat swap 0 addprop
       tint @ 1 - dup tint !
     not until 
   else			( editor aborted! )
     dup tint !
     not if exit then ( exit if null description on stack )
     begin	( remove all items on stack )
       pop
       tint @ 1 - dup tint !
     not until 
   then
  else		( edit prop )
    begin
      me @ "Enter the new action description, or '.' to abort." notify
      read
      dup in-program-tell not if break then pop
    repeat
    dup "." stringcmp if
      dbnum @ swap setdesc
    else
      pop
    then
  then
  exit
;

( --------------------------------------------------------------- )
: main 
  header
  me @ "B" flag? not if
    me @ "Sorry, you must be set BUILDER to use this command." notify
    exit
  then

  dup "#HELP" stringcmp not if	(user needs help)
    me @ "This program lets you edit attributes of the actions of the room you" notify
    me @ "are in, provided you own it.  Type '" trig name strcat "' to use it." strcat notify
    me @ " " notify
    me @ "For more detailed information, type '" trig name strcat " #HELP2" strcat notify
    exit
  else dup "#HELP2" stringcmp not if	(user needs mondo help)
"When creating a new room, the following steps make sense:  First create" print
"the room and name it.  Then write a description for it, and then set" print
"any of the special properties you need on it.  Then enter the ACTION" print
"EDITOR and create exits from the new room to your other rooms (or even" print
"someone else's if they've set the room LINKABLE).  Be sure to go to" print
"_those_ rooms and make exits in the reverse direction... from them _to_" print
"your new room... because exits are unidirectional." print
" " print
"It is possible to create actions in the room that do not connect with" print
"other rooms but are just to print a message.  Create a new action in" print
"the room (example: FEEP) and lock it so that no one can use it" print
"(example: *Triggur&!*Triggur).  Now set the fail messages and you" print
"are done.  When someone types FEEP, the messages are displayed." print
" " print
"For more information on building, ask a wizard or helpstaff to direct" print
"you to the local rule/guideline/suggestion documentation." print
" " print
    exit
  else dup if
    me @ location owner me @ dbcmp not if
      me @ "Sorry, you do not own this room." notify
      exit
    then
  then then

  me @ location dbnum !
  me @ "W" flag? not dbnum @ owner me @ dbcmp not if (not owner and not wizard!)
    me @ "Permission denied.  Type '" trig name strcat " #HELP' for help." strcat notify
    exit
  then

begin
  begin   
    #-1 dbnum !
    begin
      show-exits
      read
      dup in-program-tell not if break then pop
    repeat
    dup "Q" stringcmp not if		( halt )
      me @ "Done." notify
      exit
    then
    dup atoi numexits @ = if		( create new exit )
      make-action
      dup int 1 < if  ( error )
        me @ "*****ERROR:  Could not create exit.  You may have exceeded your quota or be out of pennies." notify
        me @ "You have " me @ pennies intostr strcat " pennies left." strcat notify
        me @ "Type @QUOTA to see your limits." notify
        exit
      then
      me @ swap "Exit created with dbref# " swap intostr strcat notify
      me @ " " notify
      continue
    then

    atoi tint !
    tint @ not tint @ numexits @ > or if
      me @ "Invalid command." notify
      continue
    then 

    loc @ exits
    begin			( find the right exit to change )
      dup dbnum !
      #-1 dbcmp if
        break
      then
      dbnum @ owner me @ dbcmp not if	( not owner... skip )
        dbnum @ next
        continue
      then
      tint @ 1 - dup tint !
      not if 		( found it! )
        break
      then
      dbnum @ next
    repeat
  
    #-1 dbnum @ dbcmp if	( not found... go back to menu )
      continue
    then 
    break
  repeat

  begin
    begin
      show-exit
      read
      dup in-program-tell not if break then pop
    repeat

    dup "Q" stringcmp not if		( halt )
      me @ "Done." notify
      exit
    then
    dup "1" stringcmp not if	( edit name )
      pop
      begin
        me @ "Enter new action names separated by ';'s or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        dup ok_name? if
          dbnum @ swap setname
        else
          pop me @ "That is NOT a valid name for an exit." notify
        then
      then

    else dup "2" stringcmp not if	( edit desc )
      pop
      edit-desc

    else dup "3" stringcmp not if	( toggle dark )
      pop
      dbnum @ "d" flag? if
        dbnum @ "!d" set
      else 
        dbnum @ "d" set
      then

    else dup "4" stringcmp not if	( toggle broadcast )
      pop
      dbnum @ "_broadcast?" getpropstr "yes" stringcmp if
        dbnum @ "_broadcast?" "yes" 0 addprop
      else
        dbnum @ "_broadcast?" remove_prop
      then

    else dup "5" stringcmp not if	( edit lock)
      pop
      begin
        me @ "Enter lock expression, a single space to unlock, or '.' to abort (quit and type HELP @LOCK for info)." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        strip
        dup if
          dbnum @ swap setlockstr not if
            me @ "ERROR: LOCK EXPRESSION BAD.  ABORTING." notify
          then
        else
          dbnum @ "_/lok" remove_prop
        then
      else
        pop
      then

    else dup "6" stringcmp not if	( edit succ )
      pop
      begin
        me @ "Enter what user sees when used or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        striptail dup if
          dbnum @ swap setsucc
        else
          dbnum @ "_/sc" remove_prop
        then
      else
        pop
      then

    else dup "7" stringcmp not if	( edit osucc )
      pop
      begin
        me @ "Enter what others see when used or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        striptail dup if
          dbnum @ swap setosucc
        else
          dbnum @ "_/os" remove_prop
        then
      else
        pop
      then

    else dup "8" stringcmp not if	( edit fail )
      pop
      begin
        me @ "Enter what user sees if locked or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        striptail dup if
          dbnum @ swap setfail
        else
          dbnum @ "_/fa" remove_prop
        then
      else
        pop
      then

    else dup "9" stringcmp not if	( edit ofail )
      pop
      begin
        me @ "Enter what others see if locked or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        striptail dup if
          dbnum @ swap setofail
        else
          dbnum @ "_/ofl" remove_prop
        then
      else
        pop
      then

    else dup "10" stringcmp not if	( edit odrop )
      pop
      begin
        me @ "Enter what others see when finished or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        striptail dup if
          dbnum @ swap setodrop
        else
          dbnum @ "_/od" remove_prop
        then
      else
        pop
      then

    else dup "11" stringcmp not if	( edit destination )
      begin
        me @ "Enter dbref # to link this action to or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      "" "#" subst dup number? not if
        me @ "Aborted." notify
        pop continue
      then
      atoi dbref
      dup dup room? not swap program? not and if
        me @ "ERROR:  The number you specified is not a room or program." notify
        pop continue
      then
      dup tint ! 
      dup "L" flag? not tint @ owner me @ dbcmp not and if 
        me @ "ERROR:  You cannot link to that item." notify
        pop continue
      then
      dbnum @ #-1 setlink	( clear last link, if any)
      dbnum @ tint @ setlink ( cast new link )

    else dup "12" stringcmp not if	( delete exit )
      begin
        me @ "Delete the action named " dbnum @ name strcat "? (NO/yes)" strcat notify
        read
        dup in-program-tell not if break then pop
      repeat
      1 strcut pop "y" stringcmp if
        me @ "Aborted." notify
        continue
      else
        dbnum @ recycle
        break
      then

    else dup "13" stringcmp not if	( go back to exit menu)
      break
    else
      pop
      me @ "Invalid command!" notify
    then then then then then then then then then then then then then then
    me @ " " notify
  repeat

  header 
repeat
;
.
c
q
