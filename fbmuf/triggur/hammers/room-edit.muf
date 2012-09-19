( menu-based room editor        )
( By triggur of Furrymuck       )
( Thanks go to:
        K'has for M3 and lots of betatesting and ideas
        Lynx for the gen-desc routines
        Riss for ideas and testing
        WhiteFire for the quota routines
        Flinthoof for zetatesting
        Hormel for Spam
   
  05-12-99 Changed checking on throw allowed to room to reflect what the
           throw program actually checks for  -Nightwind
  
  If you must, contact kevink@moonpeak.com.
)
(gen-desc stuff)
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
   
(my stuff)
var dbnum
var tstr
var tstr2
var mpidesc
var tint
var tint2
$include $lib/editor
   
$define print me @ swap notify $enddef
   
( CHANGE THESE DEFINES TO FIT YOUR SITE!!! )
  ( set this to the succ string that autolists exits in a room )
$ifdef __muck=furry
 $define AUTOLIST "@14118" $enddef
$else
 $define AUTOLIST "@127" $enddef
$endif
  ( set this to the yes/no prop that allows pets/puppets )
$define PETALLOW "pets_allowed?" $enddef
  ( set this to the maximum line length in changing propdesc->mpidesc )
$define MAXLINELEN 75 $enddef
  ( set this to the dbref # of the default parent for new rooms )
$ifdef __muck=furry
 $define DEFAULTPARENT #15100 $enddef
$else
 $define DEFAULTPARENT #2411 $enddef
$endif
  ( set this to the dbref # of the action editor program )
$ifdef __muck=furry
  $define ACTIONEDITOR #67045 $enddef
$else
 $define ACTIONEDITOR #1293 $enddef
$endif
  ( set this to the name of the prop containing the say verb )
$define SAYPROP "_say/def/say" $enddef
  ( set this to the name of the prop containing the osay verb )
$define OSAYPROP "_say/def/osay" $enddef
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
  ( set this prop to the cost in pennies of creating a room )
$define ROOMCOST 10 $enddef
  ( set this to either 1 or 0 depending on whether room quotas are enabled )
$ifdef __muck=furry
 $define CHECKQUOTAS 1 $enddef
$else
 $define CHECKQUOTAS 0 $enddef
$endif
  ( if using quota checking, set this to the prop used for room quota )
$define ROOMQUOTA "/@quota/rooms" $enddef
   
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
: make-room ( -- success )
$ifdef CHECKQUOTAS=1
  me @ ROOMQUOTA getpropstr atoi dup if
   me @ stats pop pop pop pop pop swap pop    ( get the # rooms owned )
   > not if
      me @ "Sorry... you cannot create another room.. you have reached your quota." notify
      #0 exit  
    then
  else
    pop
  then
$endif 
 
  me @ pennies ROOMCOST < if
    me @ "Sorry... you cannot afford another room.  They cost " ROOMCOST intostr strcat " to build." strcat notify
    #0 exit
  then
 
  tstr @ ok_name? not if
    me @ "That is NOT a valid name for a room." notify
    #0 exit
  then
 
  DEFAULTPARENT tstr @ newroom dup
  dup not if 
     me @ "ERROR:  Could not create room, reason unknown.  Ask a wizard or helpstaff for help." notify
  then
  exit
;
 
( --------------------------------------------------------------- )
: header ( -- )
  me @ "Room Hammer V1.5 (C) 1994 by Triggur" notify
  me @ "  (with extra minters to Flinters)  " notify
  me @ "------------------------------------" notify
  me @ " " notify
  exit
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
    begin               ( this is REALLY cheeze )
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
dup "do-sub" stringcmp not if   ( el Triggur Hacko )
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
  "#" strcat ltstr2 !  (save list name )
  swap
  dup if add-desc-line   (save BEFORE ) else pop then
  1 ltint !
  begin         ( copy to list )
    ltint @ dbnum @ ltstr2 @ getpropstr atoi > if break then
    dbnum @ ltstr2 @ "/" strcat ltint @ intostr strcat getpropstr add-desc-line
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
    dup ltint ! if      ( add remaining strings to the list )
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
: in-program-tell ( s -- i )
( ----- 1/23/98 --------------------- )
( Emergency fix for bug found by Flinthoof )
  me @ location dbnum !
  me @ "W" flag? not dbnum @ owner me @ dbcmp not and if (not owner and not wizard!)
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
    loc @ swap me @ swap me @ name " " strcat me @ SAYPROP getpropstr dup not if pop "says, " then strcat 
    swap strcat "\"" strcat notify_except
    1 exit
  else                          ( neither... return 0 )
    pop
  then then
  0 exit
;
( --------------------------------------------------------------- )
: show-room ( -- )
 
  me @ "NUMBER   : " dbnum @ intostr strcat notify
 
  me @ "1. NAME  : '" dbnum @ name strcat "'" strcat notify
 
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
 
  me @ "3. Room is" dbnum @ "d" flag? not if " NOT" strcat then " dark." strcat notify
 
  me @ "4. Throwing is" dbnum @ "_throw_allowed?" envprop (from $lib/props) striplead "n" stringpfx if " NOT" strcat then " allowed." strcat notify
 
  me @ "5. Linking is" dbnum @ "l" flag? not if " NOT" strcat then " allowed." strcat notify
 
  me @ "6. Pets are" dbnum @ PETALLOW getpropstr "no" stringcmp not if " NOT" strcat then " allowed." strcat notify
 
  me @ "7. Exits are" dbnum @ "/_/sc" getpropstr AUTOLIST stringcmp if " NOT" strcat then " automatically listed." strcat notify
 
  me @ "8. Room can" dbnum @ "c" flag? not if " NOT" strcat then " change ownership." strcat notify
  
  me @ "9. Room's parent room is " dbnum @ location name strcat "(#" strcat dbnum @ location int intostr strcat ")." strcat notify
 
  me @ "10. This room is " dbnum @ "_public?" getpropstr "yes" stringcmp if "NOT " strcat then "publically sweepable." strcat notify
 
  me @ "11. Others can" dbnum @ "a" flag? not if " NOT" strcat then " set this room as their homes." strcat notify
 
  me @ "12. Edit this room's exits." notify
  me @ "13. Create a new room." notify
  me @ "14. Teleport to edit one of your other rooms." notify
  me @ " " notify
 
  me @ "Enter '1' through '14', or 'Q' to QUIT." notify
  exit
;
 
( --------------------------------------------------------------- )
: edit-desc
  me @ " " notify
  mpidesc @ not if
    begin
      me @ "Would you like to convert this description to MPI (this is only for SIMPLE descriptions, not complex @$desc ones)? (YES/no)" notify
      read
      dup in-program-tell not if break then pop
    repeat
    1 strcut pop "n" stringcmp if       ( convert to MPI! )
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
  mpidesc @ if  ( edit MPI )
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
   "end" stringcmp not if       ( editor saved! )
     dbnum @ "redesc#" getpropstr atoi tint !  (erase old text)
     begin
       dbnum @ "redesc#/" tint @ intostr strcat remove_prop
       tint @ 1 - dup tint !
     1 < until
 
     dup tint ! ( store the new description list )
     dbnum @ "redesc#" tint @ intostr 0 addprop  (store new # of lines)
     not if exit then   ( exit if description was just cleared )
     begin
       dbnum @ swap "redesc#/" tint @ intostr strcat swap 0 addprop
       tint @ 1 - dup tint !
     not until 
   else                 ( editor aborted! )
     dup tint !
     not if exit then ( exit if null description on stack )
     begin      ( remove all items on stack )
       pop
       tint @ 1 - dup tint !
     not until 
   then
  else          ( edit prop )
    begin
      me @ "Enter the new room description, or '.' to abort." notify
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
 
  dup "#HELP" stringcmp not if  (user needs help)
    me @ "This program lets you edit attributes of the room you" notify
    me @ "are in, provided you own it.  Type '" trig name strcat "' to use it," strcat notify
    me @ "or '" trig name strcat " <room name>' to create a new room from anywhere." strcat notify
    me @ " " notify
    me @ "For more detailed information, type '" trig name strcat " #HELP2'" strcat notify
    exit
  else dup "#HELP2" stringcmp not if (user needs lotsa help)
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
    tstr !
    begin
      me @ "Do you want to create a new room called '" tstr @ strcat "' and edit it? (NO/yes)" strcat notify
      read
      dup in-program-tell not if break then pop
    repeat
    1 strcut pop "y" stringcmp if
      me @ "Aborted." notify
      exit
    then
    make-room
    dup int 1 < if  ( error )
      exit
    then
    dup DESCPROP setdesc
    dup "redesc#" "1" 0 addprop
    dup "redesc#/1" " " 0 addprop
    dup me @ swap "Room created with dbref# " swap intostr strcat
    " ... WRITE THIS NUMBER DOWN!" strcat notify
    me @ " " notify
    begin
      me @ "Would you like to teleport to the new room and edit it now? (YES/no)" notify
      read
      dup in-program-tell not if break then pop
    repeat
    1 strcut pop "n" stringcmp if
      loc @ me @ me @ name " teleports off to work on a new room." strcat notify_except
      me @ swap moveto  ( go to the new room )
      me @ location loc !       ( set loc )
      me @ location dbnum !     ( set dbnum )
    else
      me @ "Done." notify
      exit
    then 
  then then then
 
  loc @ dbnum !
  me @ "W" flag? not dbnum @ owner me @ dbcmp not if (not owner and not wizard!)
    me @ "You cannot edit this room.  Type '" trig name strcat " #HELP' for help." strcat notify
    exit
  then
 
  loc @ me @ me @ name " starts hammering on the room." strcat notify_except
  begin   
    show-room
 
    read
    dup in-program-tell if
      pop
      continue
    then
 
    dup "Q" stringcmp not if            ( halt )
      me @ "Done." notify
      loc @ me @ me @ name " finishes hammering on the room." strcat notify_except
      exit
 
    else dup "1" stringcmp not if       ( edit name )
      pop
      begin
        me @ "Enter new room name or '.' to abort." notify
        read
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        dup ok_name? if
          dbnum @ swap setname
        else
          pop
          me @ "That is NOT a valid name for a room." notify
        then
      then
    else dup "2" stringcmp not if       ( edit desc )
      pop
      edit-desc
    else dup "3" stringcmp not if       ( toggle dark )
      pop
      dbnum @ "d" flag? if
        dbnum @ "!d" set
      else 
        dbnum @ "d" set
      then
    else dup "4" stringcmp not if       ( toggle throw )
      pop
      dbnum @ "_throw_allowed?" envprop (from $lib/props) striplead 
      "n" stringpfx if
        dbnum @ "_throw_allowed?" "yes" setprop
      else
        dbnum @ "_throw_allowed?" "no" setprop
      then
(     pop
      dbnum @ "j" flag? if
        dbnum @ "!j" set
      else 
        dbnum @ "j" set
      then
)
    else dup "5" stringcmp not if       ( toggle links )
      pop
      dbnum @ "l" flag? if
        dbnum @ "!l" set
      else 
        dbnum @ "l" set
      then
    else dup "6" stringcmp not if       ( toggle pets )
      pop
      dbnum @ PETALLOW getpropstr "no" stringcmp not if
        dbnum @ PETALLOW "yes" 0 addprop
        dbnum @ "z" set
      else
        dbnum @ PETALLOW "no" 0 addprop
        dbnum @ "!z" set
      then
    else dup "7" stringcmp not if       ( toggle autolist )
      pop
      dbnum @ "/_/sc" getpropstr AUTOLIST stringcmp not if
        dbnum @ "/_/sc" remove_prop
      else
        dbnum @ "/_/sc" AUTOLIST 0 addprop
      then
 
    else dup "8" stringcmp not if       ( toggle chown )
      pop
      dbnum @ "c" flag? if
        dbnum @ "!c" set
      else 
        dbnum @ "c" set
      then
 
    else dup "9" stringcmp not if       ( edit parent )
      begin
        me @ "Enter dbref # of new parent or '.' to abort." notify
        read "" "#" subst
        dup in-program-tell not if break then pop
      repeat
      dup number? not if
        me @ "Aborted." notify
        pop continue
      then
      atoi dbref
      dup room? not if
        me @ "ERROR:  The number you specified is not a room." notify
        pop continue
      then
      dup tint ! 
      dup "A" flag? not tint @ owner me @ dbcmp not and if 
        me @ "ERROR:  You cannot link to that room." notify
        pop continue
      then
      1 tint2 !         (flag that it's ok to do it)
      begin                     (make sure we're not setting up a loop )
        dup #-1 dbcmp if
          break
        then
        dup dbnum @ dbcmp if
          me @ "ERROR:  Cannot set up cyclic parentage." notify
          0 tint2 !
        then
        location
      repeat
      tint2 @ if
        dbnum @ tint @ moveto
      then
 
    else dup "10" stringcmp not if      ( toggle _public? )
      dbnum @ "_public?" getpropstr "yes" stringcmp not if
        dbnum @ "_public?" remove_prop
      else
        dbnum @ "_public?" "yes" 0 addprop
      then
 
    else dup "11" stringcmp not if      ( toggle home links )
      pop
      dbnum @ "a" flag? if
        dbnum @ "!a" set
      else 
        dbnum @ "a" set
      then
 
    else dup "12" stringcmp not if      ( edit exits )
      ACTIONEDITOR call
      me @ location dbnum !
             
    else dup "13" stringcmp not if      ( new room )
      me @ name "'s new room" strcat tstr !
      make-room
      dup int 1 < if  ( error )
        exit
      then
      dup DESCPROP setdesc
      dup "redesc#" "1" 0 addprop
      dup "redesc#/1" " " 0 addprop
      dup me @ swap "Room created with dbref# " swap intostr strcat
      " ... WRITE THIS NUMBER DOWN!" strcat notify
      me @ " " notify
      begin
        me @ "Would you like to teleport to the new room and edit it now? (YES/no)" notify
        read
        dup in-program-tell not if break then pop
      repeat
      "no" stringcmp if
        loc @ me @ me @ name " teleports off to work on a new room." strcat notify_except
        me @ swap moveto        ( go to the new room )
        me @ location loc !     ( set loc )
        me @ location dbnum !   ( set dbnum )
      then 
 
    else dup "14" stringcmp not if      ( teleport to room )
      begin
        me @ "Enter dbref # of the room you want to edit or '.' to abort." notify
        read "" "#" subst
        dup in-program-tell not if break then pop
      repeat
      dup number? not if
        me @ "Aborted." notify
        pop continue
      then
      atoi dbref
      dup room? not if
        me @ "ERROR:  The number you specified is not a room." notify
        pop continue
      then
      dup owner me @ dbcmp not if 
        me @ "ERROR:  The room you specified does not belong to you." notify
        pop continue
      then
      loc @ me @ me @ name " teleports off to work on another room." strcat notify_except
      me @ swap moveto  ( go to the new room )
      me @ location loc !       ( set loc )
      me @ location dbnum !     ( set dbnum )
 
    else
      pop
      me @ "Invalid command!" notify
    then then then then then then then then then then then then then then then
    me @ " " notify
    me @ " " notify
    me @ " " notify
    me @ " " notify
    me @ " " notify
  repeat
 
;
-----


John "Nightwind" Boelter
--
"Purranoia: the fear your cats are up to something"  --Fortune cookie program
(nighty@fur.com)


