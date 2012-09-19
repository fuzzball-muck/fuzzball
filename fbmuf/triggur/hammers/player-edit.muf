@prog player-edit
1 19999 d
i
( menu-based player editor	)
( By Triggur               )
( Thanks go to:
	K'has for M3 and lots of betatesting and ideas
	Lynx for the gen-desc routines
   Points for the description snapshot routines
	Riss for ideas and testing
	WhiteFire for the quota and name check routines
	Hormel for Spam

  If you must, contact kevink@moonpeak.com.

  1.0-1.2 - original development
  1.3     - security fixes as per Flinthoof's observations
  1.4     - added helpstaff assisting, description snapshot code
  1.41    - fixed problem with snapshot code-- thanks baramir!
  1.42    - K'has noted that the player 'everguest' couldn't use it
            because of the guestie check.  changed it to block anybody
            whose name _begins_ with "guest".
          - Now using $lib/install for all installations
          - Changed all $defines into configurable parameters
             {now installing this is easy as upload/@install, done.}
          - Fixed snapshot code to not create so many cameras
)

$include $lib/lmgr
$include $lib/strings
$include $lib/props
$include $lib/edit
$include $lib/editor
$include $lib/install


( my stuff )
var target     ( dbref of player being edited )
var tstr
var mpidesc
var tint
var done

( THESE NO LONGER REQUIRE MUCK-SPECIFIC CUSTOMIZATION.  JUST RUN
  @INSTALL ON THIS PROGRAM ONCE UPLOADED. ) 
$define DEFAULT_FLYPROP "_flight?" $enddef
$define PFLYPROP "@/flyprop" $enddef
$define FLYSTRING prog PFLYPROP getpropstr $enddef
$define DEFAULT_SAYPROP "_say/def/say" $enddef
$define PSAYPROP "@/sayprop" $enddef
$define SAYPROP prog PSAYPROP getpropstr $enddef
$define DEFAULT_OSAYPROP "_say/def/osay" $enddef
$define POSAYPROP "@/osayprop" $enddef
$define OSAYPROP prog POSAYPROP getpropstr $enddef
$define DEFAULT_DESCPROP "{look-notify:{eval:{list:redesc}}}" $enddef
$define PDESCPROP "@/descprop" $enddef
$define DESCPROP prog PDESCPROP getpropstr $enddef
( this defines the # of seconds that can elapse after a player is given
  permission to run the program on another player's behalf, before the
  permission expires.  Default = 7200 {2 hours} )
$define DEFAULT_TIMEOUT 7200 $enddef
$define PTIMEPROP "@/timeout" $enddef
$define TIMEOUT prog PTIMEPROP getpropstr atoi $enddef

( where we store our own revision )
$define REVPROP "@/revision" $enddef
( current revision number of the program )
$define REVISION "1.42" $enddef
( editplayer command global )
$define EPCMD "editplayer;ep" $enddef
( editplayer registry name )
$define EPREG "editplayer" $enddef

lvar valtext
lvar valdef
lvar valcurr
: get-config-val ( s1 s2 s3 -- s )
  valcurr ! valtext ! valdef !

  me @ " " notify
  me @ valtext @ notify
  me @ "(Enter '.' to use the current setting of '" valcurr @ strcat "'."
      strcat notify
  me @ " Enter ',' to use the default setting of '" valdef @ strcat "'."
      strcat notify
  me @ " Or enter a new value)" notify
  read dup "." strcmp not if
    pop valcurr @
  else dup "," strcmp not if
    pop valdef @
  then then
;

: do-install ( s -- s )
  pop ( dont currently care about revision #s )
  INSTALL-WIZ-CHECK

  prog "W" flag? not if
    me @ "This program must be set W to run.  To continue," notify
    me @ "Type     @set #" prog intostr strcat "=W" strcat notify
    me @ "Then     @install #" prog intostr strcat notify
    "" exit
  then
 
  prog "S" flag? not if
    me @ "This program must be set S to run.  To continue," notify
    me @ "Type     @set #" prog intostr strcat "=S" strcat notify
    me @ "Then     @install #" prog intostr strcat notify
    "" exit
  then

  prog "L" flag? not if
    me @ "This program must be set L to run.  To continue," notify
    me @ "Type     @set #" prog intostr strcat "=L" strcat notify
    me @ "Then     @install #" prog intostr strcat notify
    "" exit
  then
 
( create global command and registry entries )
  prog EPCMD add-global-command
  prog EPREG add-global-registry

( set default config params if unset )
  prog PFLYPROP getpropstr "" stringcmp not if  (flight )
     prog PFLYPROP DEFAULT_FLYPROP 0 addprop
  then
  prog PSAYPROP getpropstr "" stringcmp not if  (say )
     prog PSAYPROP DEFAULT_SAYPROP 0 addprop
  then
  prog POSAYPROP getpropstr "" stringcmp not if  (osay )
     prog POSAYPROP DEFAULT_OSAYPROP 0 addprop
  then
  prog PDESCPROP getpropstr "" stringcmp not if  (desc )
     prog PDESCPROP DEFAULT_DESCPROP 0 addprop
  then
  prog PTIMEPROP getpropstr "" stringcmp not if  (timeout )
     prog PTIMEPROP DEFAULT_TIMEOUT intostr 0 addprop
  then

( get new config params )
  prog PFLYPROP DEFAULT_FLYPROP
      "What is the name of the player prop that determines if they can fly?"
      FLYSTRING get-config-val 0 addprop
  prog PSAYPROP DEFAULT_SAYPROP
      "What is the name of the prop used by the SAY global that determines the speaker's FIRST PERSON say verb?"
      SAYPROP get-config-val 0 addprop
  prog POSAYPROP DEFAULT_OSAYPROP
      "What is the name of the prop used by the SAY global that determines the speaker's SECOND PERSON (osay) verb?"
      OSAYPROP get-config-val 0 addprop
  prog PDESCPROP DEFAULT_DESCPROP
      "What should I use for an MPI command to set as the player's description (the 'redesc' part is mandatory)?"
      DESCPROP get-config-val 0 addprop
  prog PTIMEPROP DEFAULT_TIMEOUT intostr
      "How many seconds can elapse after a player allows another to edit their attributes before that permission times out (7200 recommended)?"
      TIMEOUT intostr get-config-val 0 addprop

(TODO: CREATE CAMERA OBJECT HERE IF IT DOESNT EXIST, STORE IT IN ROOM 0 )

( set our own understanding of our own revision )
  prog REVPROP REVISION 0 addprop

  REVISION
;
PUBLIC do-install

: do-uninstall ( s -- s )
  pop
  UNINSTALL-WIZ-CHECK

( TODO: destroy camera object )

( remove global command )
  EPCMD remove-global-command

( remove  global registry )
  EPREG remove-global-registry

  REVISION
;
PUBLIC do-uninstall

( --------------description snapshot code-------------- )

lvar destobj
lvar subject
lvar propstr
lvar camera
lvar camint
lvar camint2

: desc-snapshot ( d1 d2 s -- )
                ( d1 = object dbref being looked at )
                ( d2 = object dbref on which to store description )
                ( s  = name of propdir in which to store description )

( set up the camera and tell it to take the picture )
  propstr !
  destobj !
  subject !
  subject @ location "The camera" newobject camera !
  camera @ "_/de" "%STOPIT" 0 addprop
  camera @ "_amcam" "yes" 0 addprop
  camera @ "_listen" prog intostr 0 addprop
  camera @ "look " subject @ name strcat force
  camera @ "look The camera" force

( wait for it to develop )
  begin
    camera @ "_done" getpropstr "" strcmp if
      break
    then
    1 sleep
  repeat

( save the description and @rec the camera )
  camera @ "_desc#" getpropstr atoi camint !
  destobj @ propstr @ "#" strcat remove_prop
  1 camint2 !
  begin
    camera @ "_desc#/" camint2 @ intostr strcat getpropstr "Carrying" instr if
      1 camint !
    else camera @ "_desc#/" camint2 @ intostr strcat getpropstr
    "You see nothing special." strcmp not if
      1 camint !
    else camera @ "_desc#/" camint2 @ intostr strcat getpropstr
    "sees you looking at " instr if
      1 camint !
    else
      destobj @ propstr @ "#/" strcat camint2 @ intostr strcat
      camera @ "_desc#/" camint2 @ intostr strcat getpropstr
      0 addprop
      destobj @ propstr @ "#" strcat camint2 @ intostr 0 addprop
    then then then
  camint2 @ 1 + camint2 !
  camint @ 1 - dup camint ! not until
  camera @ recycle
;

: camera-listen ( s -- )
  propstr !
 
  propstr @ "%STOPIT" strcmp not if
    me @ "_done" "yes" 0 addprop
    me @ "_listen" remove_prop
  else
    me @ "_desc#" getpropstr atoi 1 + dup
    me @ swap "_desc#" swap intostr 0 addprop
    me @ swap "_desc#/" swap intostr strcat propstr @ 0 addprop
  then
;
( ------------end description snapshot code------------ )

: header ( -- )
  me @ "Player Hammer V" REVISION strcat " (C) 1994-1998 by Triggur" strcat notify
  me @ "-------------------------------------------" notify
  me @ " " notify
  exit
;
 
: in-program-tell ( s -- i )
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
: show-player ( -- )

  me @ target @ dbcmp 0 = if (editing someone else)
    me @ "WARNING: YOU ARE EDITING " target @ name strcat "'S CHARACTER."
     strcat notify
  then

  me @ "Your name is " target @ name strcat "." strcat notify

  target @ desc tstr !
  tstr @ "{" instr 1 = if    ( description in MPI)
    1 mpidesc !
    me @ "2. Your MPI description is:" notify
    1 tint !
    begin
      target @ "redesc#/" tint @ intostr strcat getpropstr
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
    me @ "2. Your description is '" tstr @ strcat "'" strcat notify
  then

  me @ "3. Your sex is " target @ "sex" getpropstr strcat "." strcat notify
  
  me @ "4. Your species is " target @ "species_prop" getpropstr dup if target @ swap getpropstr else pop target @ "species" getpropstr then strcat "." strcat notify

  me @ "5. You can" target @ "j" flag? not if " NOT" strcat then " throw items." strcat notify

  me @ "6. Your home is " target @ getlink name strcat "(#" strcat target @ getlink int intostr strcat ")." strcat notify

  me @ "7. You can " target @ "_hand/hand_ok" getpropstr "yes" stringcmp if "NOT " strcat then  "hand/be handed items." strcat notify

  me @ "8. You can " target @ FLYSTRING getpropstr "yes" stringcmp if "NOT " strcat then  "fly." strcat notify

  me @ "9. Your smell message is '" target @ "_scent" getpropstr strcat "'." strcat notify

  me @ "10. When you speak you see 'You " target @ "_say/def/say" getpropstr strcat " [...]'." strcat notify

  me @ "11. When you speak others see '" target @ name strcat " " strcat target @ "_say/def/osay" getpropstr strcat " [...]'." strcat notify

  me @ " " notify
  me @ "12. Edit sweep messages." notify
(
  me @ "14. Edit your morphs." notify
  me @ "15. Edit your personal list of actions." notify
)
  me @ " " notify
  me @ "Enter '2' through '12', or 'Q' to QUIT." notify
  exit
;

( --------------------------------------------------------------- )
: do-sweeps

begin
  me @ " " notify
  me @ " " notify
  me @ " " notify
  target @ "_sweep_player" getpropstr if
    me @ "1. When you sweep a player it says '" target @ name strcat " " strcat target @ "_sweep_player" getpropstr strcat "'." strcat notify
  else
    me @ "1. When you sweep a player, the normal message is displayed." notify
  then

  me @ "_sweep" getpropstr if
    me @ "2. When you sweep several players it says '" target @ name strcat " " strcat target @ "_sweep" getpropstr strcat "'." strcat notify
  else
    me @ "2. When you sweep several players, the normal message is displayed." notify
  then

  me @ "_swept" getpropstr if
    me @ "3. When you get swept it says '" target @ name strcat " " strcat target @ "_swept" getpropstr strcat "'." strcat notify
  else
    me @ "3. When you get swept, the normal message is displayed." notify
  then

  me @ " " notify
  me @ "4. Return to main menu." notify
  me @ " " notify
  me @ "Enter '1' through '4' or 'Q' to QUIT." notify

  read striplead
  dup in-program-tell if pop continue then
  dup "Q" stringcmp not if
    1 done !
    break
  else dup "1" stringcmp not if	( edit _sweep_player )
    pop
    begin
      me @ "Enter what to say when you sweep a specific player or '.' to abort." notify
      me @ "The word '" target @ name strcat " ' will be inserted automatically... do not type it!" strcat notify
      read striplead
      dup in-program-tell not if break then pop
    repeat
    dup "." stringcmp if
      dup if
        target @ swap "_sweep_player" swap 0 addprop
      else
       target @ "_sweep_player" remove_prop
       pop
      then
    else
     pop
    then

  else dup "2" stringcmp not if	( edit _sweep )
    pop
    begin
      me @ "Enter what to say when you sweep several players or '.' to abort." notify
      me @ "The word '" target @ name strcat " ' will be inserted automatically... do not type it!" strcat notify
      read striplead
      dup in-program-tell not if break then pop
    repeat
    dup "." stringcmp if
      dup if
        target @ swap "_sweep" swap 0 addprop
      else
       target @ "_sweep" remove_prop
       pop
      then
    else
     pop
    then

  else dup "3" stringcmp not if	( edit _swept )
    pop
    begin
      me @ "Enter what to say when you get swept by someone or '.' to abort." notify
      me @ "The word '" target @ name strcat " ' will be inserted automatically... do not type it!" strcat notify
      read striplead
      dup in-program-tell not if break then pop
    repeat
    dup "." stringcmp if
      dup if
        target @ swap "_swept" swap 0 addprop
      else
       target @ "_swept" remove_prop
       pop
      then
    else
     pop
    then

   else dup "4" stringcmp not if	( edit _swept )
    break
  else
    me @ "Invalid command!" notify
  then then then then then
repeat
exit
;

( --------------------------------------------------------------- )
: edit-desc
  me @ " " notify
  mpidesc @ not if
    begin
      me @ "Would you like to convert this description to standard MPI (this may not work for complex MPI or MUF-based descriptions)? (YES/no)" notify
      read striplead
      dup in-program-tell not if break then pop
    repeat
    "no" stringcmp if	( convert to MPI! )
      target @ target @ "redesc" desc-snapshot
      target @ DESCPROP setdesc
      1 mpidesc !
    then 
  then
  mpidesc @ if	( edit MPI )
   1 tint !
   begin
     target @ "redesc#/" tint @ intostr strcat getpropstr
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
     target @ "redesc#" getpropstr atoi tint !  (erase old text)
     begin
       target @ "redesc#/" tint @ intostr strcat remove_prop
       tint @ 1 - dup tint !
     1 < until

     dup tint !	( store the new description list )
     target @ "redesc#" tint @ intostr 0 addprop  (store new # of lines)
     not if exit then	( exit if description was just cleared )
     begin
       target @ swap "redesc#/" tint @ intostr strcat swap 0 addprop
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
      target @ "Enter the new description, or '.' to abort." notify
      read striplead
      dup in-program-tell not if break then pop
    repeat
    dup "." stringcmp if
      target @ swap setdesc
    else
      pop
    then
  then
  exit
;

( --------------------------------------------------------------- )
: main 
  me @ "_amcam" getpropstr "yes" stringcmp not if
    camera-listen
    exit
  then

  prog REVPROP getpropstr REVISION strcmp if
    me @ "This program has not been installed yet!" notify
    me @ "Type   @install #" prog intostr strcat notify
    exit
  then

  0 done !
  me @ target !
  header

  me @ name tolower 5 strcut pop "guest" strcmp not if
    me @ "Sorry, this program can not be used by guests.  Please ask for an account." notify
    exit
  then

  dup "#clear" stringcmp not if (user done needing help)
    me @ "_helpstaff_assist_id" remove_prop
    me @ "_helpstaff_assist_time" remove_prop
    me @ "Done.  Helpstaff request has been cleared." notify
    exit 
  else dup "#allow" 6 strncmp not if (user about to get help)
    6 strcut swap pop strip .pmatch dup #-1 dbcmp 0 = not if
      me @ "Sorry, I don't know who that player is." notify
      exit
    then
    target !
    me @ "W" flag? target @ "W" flag? not and if
      me @ "Sorry, a non-wizard may not assist a wizard." notify
      exit
    then
 
    me @ "!!!!! " me @ name strcat " has allowed you to assist %o." strcat
      pronoun_sub target @ swap notify
    me @ "!!!!! You have allowed " target @ name strcat " to assist you."
      strcat notify
    me @ "_helpstaff_assist_id" target @ intostr 0 addprop
    me @ "_helpstaff_assist_time" systime intostr 0 addprop
    exit
  else dup "#assist" 7 strncmp not if (user giving help)
    7 strcut swap pop strip .pmatch dup #-1 dbcmp 0 = not if
      me @ "Sorry, I don't know who that player is." notify 
      exit
    then
    target !
    target @ "_helpstaff_assist_id" getpropstr atoi dbref me @ dbcmp 0 = if
      me @ target @ "Sorry, " target @ name strcat
       " has not given you permission to assist %o." strcat pronoun_sub notify
      exit
    then
    me @ "W" flag? not target @ "W" flag? and if
      me @ "Sorry, a non-wizard may not assist a wizard." notify
      exit
    then
 
    target @ "_helpstaff_assist_time" getpropstr atoi systime TIMEOUT - < if
      me @ target @ "Sorry, your permission to assist " target @ name strcat
       " has expired.  %S will need to type '" strcat command @ strcat
       " #allow " strcat me @ name strcat "' again." strcat pronoun_sub notify
      exit
    then
  else dup "#feep" stringcmp not if (user done needing help)
    me @ "There once was a horse from Nantucket" notify
    me @ "Whose MUF was so big he could tuck it" notify
    me @ "In Internet tar's" notify
    me @ "For wizardly taurs" notify
    me @ "And a charge to maintain? He could duck it!" notify
    exit
  else "" stringcmp if  (#help or unknown params)
    me @ "This program lets you edit your attributes." notify
    me @ "Type '" trig name strcat "' to use it." strcat notify
    me @ "To allow someone else to help you, type '" trig name strcat " #allow <playername>'." strcat notify
    me @ "To disallow someone else from helping you, type '" trig name strcat " #clear'." strcat notify
    me @ "To help someone else, type '" trig name strcat " #assist <playername>'." strcat notify
    exit
  then then then then then

  target @ "_desc_notify_looker" getpropstr not if
    target @ "_desc_notify_looker" "%S sees you looking at %o." 0 addprop
  then
  target @ "_desc_notify_looked" getpropstr not if
    target @ "_desc_notify_looked" "+++++ %N just looked at you!" 0 addprop
  then

  begin
    begin
      show-player

      read striplead
      dup in-program-tell not if break then pop
    repeat
    dup "Q" stringcmp not if		( halt )
      me @ "Done." notify
      exit

    else dup "1" stringcmp not if	( edit name )
      pop
me @ "Sorry... the ability to change your name has been removed.  To do this, exit the program and type '@name me=<new name> <your password>'." notify
(
      begin
        me @ "Enter your new name or '.' to abort." notify
        read 
        dup in-program-tell not if break then pop
      repeat
      "" " " subst dup "." stringcmp if
        dup ok_name? if
          target @ swap setname
        else
          pop
          me @ "That is NOT a valid name for a player." notify
        then
      then
)
    else dup "2" stringcmp not if	( edit desc )
      pop
      edit-desc

    else dup "3" stringcmp not if	( edit sex )
      pop
      begin
        me @ "Enter your sex or '.' to abort." notify
        read striplead
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        dup target @ swap "sex" swap 0 addprop  (set both, just to be sure)
        target @ swap "gender" swap 0 addprop
      then

    else dup "4" stringcmp not if	( edit species )
      pop
      begin
        me @ "Enter your species or '.' to abort." notify
        read striplead
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        target @ swap "species" swap 0 addprop
        target @ "species_prop" remove_prop
      then

    else dup "5" stringcmp not if	( toggle throw )
      pop
      target @ "J" flag? if
        target @ "!j" set
      else
        target @ "j" set
      then

    else dup "6" stringcmp not if	( set home )
      begin
        me @ "Enter dbref # of new home room or 'HERE' or '.' to abort." notify
        read "" "#" subst striplead
        dup in-program-tell not if break then pop
      repeat
      dup "here" stringcmp not if
        pop target @ location int intostr
      then
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
      dup "a" flag? not tint @ owner target @ dbcmp not and if 
        me @ "ERROR:  You cannot link to that room." notify
        pop continue
      then
      target @ tint @ setlink

    else dup "7" stringcmp not if	( toggle hand )
      target @ "_hand/hand_ok" getpropstr "yes" stringcmp if
        target @ "_hand/hand_ok" "yes" 0 addprop
      else
        target @ "_hand/hand_ok" remove_prop
      then

    else dup "8" stringcmp not if	( toggle flight )
      target @ FLYSTRING getpropstr "yes" stringcmp if
        target @ FLYSTRING "yes" 0 addprop
      else
        target @ FLYSTRING remove_prop
      then

    else dup "9" stringcmp not if	( edit scent )
      pop
      begin
        me @ "Enter your scent message or '.' to abort." notify
        read striplead
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        target @ swap "_scent" swap 0 addprop
        target @ "_smell_notify" getpropstr not if  ( add default notify )
          target @ "_smell_notify" "+++++%N just smelled you!" 0 addprop
        then
      then

    else dup "10" stringcmp not if	( edit say )
      pop
      begin
        me @ "Enter what you see when you speak or '.' to abort." notify
        me @ "The word 'You ' will be inserted automatically... do not type it!" notify
        read striplead
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        target @ swap "_say/def/say" swap 0 addprop
      then

    else dup "11" stringcmp not if	( edit osay )
      pop
      begin
        me @ "Enter what others see when you speak or '.' to abort." notify
        me @ "The word '" target @ name strcat " ' will be inserted automatically... do not type it!" strcat notify
        read striplead
        dup in-program-tell not if break then pop
      repeat
      dup "." stringcmp if
        target @ swap "_say/def/osay" swap 0 addprop
      then

    else dup "12" stringcmp not if	( edit sweeps )
      do-sweeps

    else dup "14" stringcmp not if	( edit morphs )
(      MORPHEDIT call)
me @ "[NOT IMPLEMENTED]" notify
    else dup "15" stringcmp not if	( enter personal actions editor )
me @ "[NOT IMPLEMENTED]" notify

    else
      pop
      me @ "Invalid command!" notify
    then then then then then then then then then then then then then then
    then
    done @ if
      me @ "Done." notify
      break
    then 

    me @ " " notify
    me @ " " notify
    me @ " " notify
  repeat

;
.
c
q
