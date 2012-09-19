@prog morph-edit
1 19999 d
i
( menu-based morph editor	)
( By triggur                    )
( Thanks go to:
        K'has for M3 and lots of betatesting and ideas
        Lynx for the gen-desc routines
        Points for the snapshot code
        Riss for ideas and testing
        Warwick for the #connect/#disconnect
        Hormel for Spam

   If you must, contact kevink@moonpeak.com.

   V1.41:  upgrades for new snapshot code, #connect, _morph# by
           revar's request.
   V1.42:  fix for flaw relying on propmv command forces
   V1.43:  further cutover to _morph# from morph#, bug finds by
				natasha_vixen
	V1.44:  Shassha noticed #disconnect wasn't working.  Now it
				updates the _disconnect/morph prop correctly.
   V1.45:  K'has noted that the player 'everguest' couldn't use it
           because of the guestie check.  changed it to block anybody
           whose name _begins_ with "guest".
           - removed the dependency on #27000, and moved cmd-cp-mv into
             the code directly
           - removed all dependence on $defines, all installation now
             done with the @install command.
   V1.50:  Nightwind pointed out an oddity with the MORPHCMD define.
)

$include $lib/lmgr
$include $lib/strings
$include $lib/match
$include $lib/props
$include $lib/edit
$include $lib/install

lvar lbracket   ( count of left brackets found )
lvar search     ( current place to search )
var ltstr
var ltstr2
var ltint
var ltint2
var morphcmd 
var target  ( dbref of the person being edited )
var tstr	( a coupla generic temp strings )
var mpidesc	( using mpi description or not? )
var tint	( a coupla generic temp ints )
var len1
var len2
var nummorphs
var morphname
var morphcmd
var morphmessage
var reglist
var tempargs

$include $lib/editor
$define tell me @ swap notify $enddef

( THESE $DEFINES NO LONGER NEED TO BE EDITED FOR A NEW INSTALL--
  JUST RUN THE @INSTALL COMMAND ON THIS PROGRAM TO INSTALL IT )
$define PSCENTPROP "@/scentprop" $enddef
$define SCENTPROP prog PSCENTPROP getpropstr $enddef
$define DEFAULT_SCENTPROP "_scent" $enddef
$define PSAYPROP "@/sayprop" $enddef
$define SAYPROP prog PSAYPROP getpropstr $enddef
$define DEFAULT_SAYPROP "_say/def/say" $enddef
$define POSAYPROP "@/osayprop" $enddef
$define OSAYPROP prog POSAYPROP getpropstr $enddef
$define DEFAULT_OSAYPROP "_say/def/osay" $enddef
$define PDESCPROP "@/descprop" $enddef
$define DESCPROP prog PDESCPROP getpropstr $enddef
$define DEFAULT_DESCPROP "{look-notify:{eval:{list:redesc}}}" $enddef
$define PTIMEPROP "@/timeout" $enddef
$define TIMEOUT prog PTIMEPROP getpropstr atoi $enddef
$define DEFAULT_TIMEOUT 7200 $enddef

( program revision number )
$define REVISION "1.50" $enddef 
( local storage prop for revision number )
$define REVPROP "@/revision" $enddef
( the global command for morph )
$define MORPHCOMMAND "morph;qmorph" $enddef
( the global registry for morph )
$define MORPHREG "morph" $enddef

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
  prog MORPHCOMMAND add-global-command
  prog MORPHREG add-global-registry

( set default config params if unset )
  prog PSCENTPROP getpropstr "" stringcmp not if  (flight )
     prog PSCENTPROP DEFAULT_SCENTPROP 0 addprop
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
  prog PSCENTPROP DEFAULT_SCENTPROP
      "What is the name of the player prop that determines if they can fly?"
      SCENTPROP get-config-val 0 addprop
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
      "How many seconds can elapse after a player allows another to edit their morphs before that permission times out (7200 recommended)?"
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
  MORPHCOMMAND remove-global-command

( remove  global registry )
  MORPHREG remove-global-registry

  REVISION
;
PUBLIC do-uninstall

( -----------------begin cmd-mv-cp code--------------------------- )
( cmd-mv-cp
    cp [origobj][=prop],[destobj][=destprop]
      Copies prop from origobj to destobj, renaming it to destprop.
  
    mv [origobj][=prop],[destobj][=destprop]
      Moves prop from origobj to destobj, renaming it to destprop.
  
      Arguments in []s are optional.
      if origobj is omitted, it assumes a property on the user.
      if destobj is omitten, it assumes destobj is the same as origobj.
      if destprop is omitted, it assumes the same name as prop.
      if prop is omitted, it prompts for it.
      if both prop and origobj are omitted, it prompts for both.
      if both destobj and destprop are omitted, it prompts for them.
)
  
lvar copy?
  
: cp-mv-prop ( d s d s -- i )
  4 pick 4 pick getpropstr
  dup if
    3 pick 3 pick rot 0 addprop
  else
    pop 4 pick 4 pick getpropval
    dup if
      3 pick 3 pick "" 4 rotate addprop
    else
      pop 4 pick 4 pick propdir? not if
        "I don't see what property to "
        copy? @ if "copy." else "move." then
        strcat .tell pop pop pop pop 0 exit
      then
    then
  then
  4 pick 4 pick propdir? if
    4 pick dup 5 pick "/" strcat nextprop
    (d s dd ds d ss)
    begin
      dup while
      over dup 3 pick nextprop
      4 rotate 4 rotate
      6 pick 6 pick "/" strcat
      3 pick dup "/" rinstr
      strcut swap pop strcat
      cp-mv-prop pop
    repeat
    pop pop
  then
  pop pop
  copy? @ not if remove_prop else pop pop then
  1
;
  
: cp-prop ( d s d s -- i )
  1 copy? ! cp-mv-prop
;
public cp-prop
  
: mv-prop ( d s d s -- i )
  0 copy? ! cp-mv-prop
;
public mv-prop
  
: strip-slashes ( s -- s' )
  begin
    dup while
    dup strlen 1 - strcut
    dup "/" strcmp if strcat exit then
    pop
  repeat
;
  
: parse-command ( s -- )
  command @ tolower
  "command @ = \"" over strcat "\"" strcat .tell
  "c" instr copy? !
  
  dup "#help" stringcmp not if
    pop
    copy? @ if
      "cp origobj=prop,destobj=destprop"
      "  Copies prop from origobj to destobj, renaming it to destprop."
    else
      "mv origobj=prop,destobj=destprop"
      "  Moves prop from origobj to destobj, renaming it to destprop."
    then
    " "
    "  if origobj is omitted, it assumes a property on the user."
    "  if destobj is omitted, it assumes destobj is the same as origobj."
    "  if destprop is omitted, it assumes it is the same name as prop."
    "  if prop is omitted, it asks the user for it."
    "  if both prop and origobj are omitted, it asks the user for both."
    "  if both destobj and destprop are omitted, it asks the user for them."
    depth EDITdisplay exit
  then
  
  "," .split .strip swap .strip
  "=" .split .strip swap .strip
  dup if
    .noisy_match
    dup not if pop exit then
  else
    over not if
      "Please enter the name of the original object."
      .tell pop read .strip
    then
    dup if
      .noisy_match
      dup not if pop exit then
    else pop me @
    then
  then
  -3 rotate
  begin
    strip-slashes
    dup not while
    "Please enter the name of the original property." .tell
    pop read .strip
  repeat
  swap
  "=" .split .strip strip-slashes swap .strip
  begin
    over over or not while
    pop pop
    "Please enter the name of the destination object." .tell
    read .strip
    "Please enter the name of the destination property." .tell
    read .strip
    strip-slashes swap
  repeat
  dup if
    .noisy_match
    dup not if pop exit then
  else
    pop 3 pick
  then
  swap
  dup not if pop over then
  
  (origdbref origpropname destdbref destpropname)
  cp-mv-prop if
    copy? @ if "Property copied."
    else "Property moved."
    then .tell
  then
;
( -----------------end cmd-mv-cp code--------------------------- )

( -----------------begin snapshot code--------------------------- )
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
    "sees you looking at " instr if
      1 camint !
    else
      destobj @ propstr @ "#/" strcat camint2 @ intostr strcat
      camera @ "_desc#/" camint2 @ intostr strcat getpropstr
      0 addprop
      destobj @ propstr @ "#" strcat camint2 @ intostr 0 addprop
    then then
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

( --------------end snapshot code---------------- )

: in-program-tell ( s -- i )
  striplead
  dup ":" instr 1 = if          ( player posed )
    1 strcut swap pop
    me @ name " " strcat swap strcat
    loc @ swap #-1 swap notify_except
    1 exit
  else dup "\"" instr 1 = if    ( player spoke )
    dup me @ swap "You " me @ SAYPROP getpropstr dup not if pop "say," then strcat " " strcat
    swap strcat "\"" strcat notify
    loc @ swap me @ swap me @ name " " strcat me @ OSAYPROP getpropstr dup not if pop "says," then strcat " " strcat
    swap strcat "\"" strcat notify_except
    1 exit
  else                          ( neither... return 0 )
    pop
  then then
  0 exit
;

( --------------------------------------------------------------- )
: morph-prop ( s -- s )
  "/_morph#/" morphcmd @ strcat "#/" strcat swap strcat
  exit
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
  me @ "Morph Hammer V" REVISION strcat " (C) 1994-1998 by Triggur" strcat notify
  me @ "------------------------------------------" notify
  me @ " " notify
  exit
;

( --------------------------------------------------------------- )
: do-help2
  header
  "  To use this program, edit your current attributes (by hand or" tell
  "with EDITPLAYER) to get exactly the form you want.  Type MORPH #ADD" tell
  "and your current description, sex, species, scent, say and osay" tell
  "will be copied as a new morph.  You will be expected to answer" tell
  "a few questions.  First, the 'command'  associated with this" tell
  "morph.  This should be one word like '2leg' or 'mwolf' or somesuch." tell
  "Next the name of the morph.  Only you will see this in MORPH #LIST." tell
  "Finally, the text it should print when you change to that new morph." tell
  " " tell
  "  To add another morph, simply edit your current description again" tell
  "and MORPH #ADD again.  Lather, rinse, repeat. :)" tell
  " " tell
  "WARNING:  If your description is currently using @$desc (@6800 on Furry)," tell
  "the whole description will be _converted_ to a single MPI list." tell
  " " tell
  "NOTE:  Be aware that when you change to a morph, your description" tell
  "will be overwritten.  If you have multiple descriptions you want" tell
  "to make morphs for, be sure to MORPH #ADD them all *before*" tell
  "trying to test one of the new morphs.  PLEASE DELETE YOUR OLD" tell
  "MORPHS!" tell
  " " tell
  "Also, please keep the number of morphs you have to under 10.  Thank you!" tell
;

( --------------------------------------------------------------- )
: do-help 
  header
  "This program lets you easily change your description and other attributes." tell
  " " tell
  "MORPH #ADD         - Add current description as a new morph" tell
  "MORPH #HELP        - this text" tell
  "MORPH #LIST        - list all the morphs you have and their commands" tell
  "MORPH #REMOVE      - Delete one of your morphs" tell
  "MORPH <command>    - change to a morph (see morph #list for commands)" tell
  "QMORPH <command>   - change to a morph quietly" tell
  " " tell
  "MORPH #CONNECT     - Set a morph to go to when awake" tell
  "MORPH #DISCONNECT  - Set a morph to go to when asleep" tell
  " " tell
  "MORPH #ALLOW <player> - allow the named player to edit your morphs" tell
  "MORPH #ASSIST <player> [#ADD|#LIST|#REMOVE|<command>]" tell
  "                      - edit another player's morphs." tell
  "MORPH #CLEAR          - stop allowing your morphs being edited" tell
  " " tell
  "Type MORPH #HELP2 for further instructions!" tell
  exit
;

( --------------------------------------------------------------- )
: do-morph-list 
  header
  target @ "_regmorphs" getpropstr    (fetch list of registered morphs )
  dup not if
    "(you have created no morphs with this program)" tell
    exit
  then

  " " explode nummorphs !
  1 tint !
  nummorphs @ not if
    "(you have created no morphs with this program)" tell
  else
    "         Command    ...morphs you to..." tell
    "----------------------------------------------------------" tell
    begin			( loop through all morphs here )
      tint @ nummorphs @ > if
        break
      then
      dup not if	(null... skip)
        pop
        tint @ 1 + tint !
        continue
      then
      morphcmd !
      "[q]morph " morphcmd @ 11 padto strcat toupper ( append command )
      target @ "name" morph-prop getpropstr strcat
      me @ swap notify		(...aaaannnnnd PRINT )
      tint @ 1 + tint ! 
    repeat
  then
  me @ " " notify
  exit
;

( --------------------------------------------------------------- )
: main 
  dup tempargs !
  strip

  me @ "_amcam" getpropstr "yes" stringcmp not if
    camera-listen
    exit
  then

  prog REVPROP getpropstr REVISION strcmp if
    me @ "This command has not yet been installed to the latest version." notify
    me @ "Type @install #" prog intostr strcat notify
    exit
  then

  me @ name tolower 5 strcut pop "guest" strcmp not if
    header
    me @ "Sorry, this program can not be used by guests.  Please ask for an account." notify
    exit
  then

  me @ target !

  dup "#help" stringcmp not if	(user needs help)
    do-help
    exit
  then

  dup "#help2" stringcmp not if	(user needs MORE help)
    do-help2
    exit
  then

  dup "#assist" 7 strncmp not if (user giving help)
    7 strcut swap pop strip

    dup " " instr dup if
      strcut morphcmd ! strip
    else
      pop "" morphcmd !
    then
    .pmatch dup #-1 dbcmp 0 = not if
      me @ "Sorry, I don't know who that player is." notify 
      exit
    then
    target !
    target @ "_helpstaff_assist_id" getpropstr atoi dbref me @ dbcmp 0 = if
      me @ target @ "Sorry, " target @ name strcat
       " has not given you permission to assist %o." strcat pronoun_sub notify
      exit
    then
    target @ "W" flag? me @ "W" flag? not and if
      me @ "Sorry, a non-wizard may not assist a wizard." notify
      exit
    then
    target @ "_helpstaff_assist_time" getpropstr atoi systime TIMEOUT - < if
      me @ target @ "Sorry, your permission to assist " target @ name strcat
       " has expired.  %S will need to type '" strcat command @ strcat
       " #allow " strcat me @ name strcat "' again." strcat pronoun_sub notify
      exit
    then
    morphcmd @
    target @ location loc !
    me @ "WARNING: YOU ARE EDITING " target @ name strcat "'S MORPHS!" strcat
     notify
  then

( perform user prop upgrade )
  target @ "_morph#" propdir? not target @ "morph#" propdir? and if
    me @ ">>>>Upgrading old morph properties..." notify
    target @ "morph#" target @ "_morph#" mv-prop pop
    me @ ">>>>Done." notify
  then

  dup not if  (user needs help or check last morph)
    target @ "_lastmorph" getpropstr dup if
      me @ swap "Your last morph was: " swap strcat notify
      "Current auto-connect morph: " target @ "_morph_connect" getpropstr 
       dup "" strcmp 0 = if
         pop "(none)"
       then
       strcat tell
      "Current auto-disconnect morph: " target @ "_morph_disconnect" getpropstr 
       dup "" strcmp 0 = if
         pop "(none)"
       then
       strcat tell
    else
      do-help
    then
    exit
  then

  dup "Connect" strcmp not if (auto-connect prop)
    pop target @ "_morph_connect" getpropstr 
    dup morphcmd !
    dup "" strcmp 0 = if
      exit
    then
  else dup "Disconnect" strcmp not if (auto-disconnect prop)
    pop target @ "_morph_disconnect" getpropstr
    dup morphcmd !
    dup "" strcmp 0 = if
      exit
    then
  then then

  dup "#clear" stringcmp not if (user done needing help)
    header
    me @ "_helpstaff_assist_id" remove_prop
    me @ "_helpstaff_assist_time" remove_prop
    me @ "Done.  Helpstaff request has been cleared." notify
    exit 
    me @ 

  else dup "#allow" 6 strncmp not if (user about to get help)
    header
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
  else dup "#feep" stringcmp not if (user done needing help)
    header
    me @ "There once was a horse from Nantucket" notify
    me @ "Whose MUF was so big he could tuck it" notify
    me @ "In Internet tar's" notify
    me @ "For wizardly taurs" notify
    me @ "And a charge to maintain? He could duck it!" notify
    exit
  else dup "" stringcmp if  (#help or unknown params)
    dup "#list" stringcmp not if  (user wants morph list)
      do-morph-list
      exit
    then
  then then then then

  dup "#connect" stringcmp not if (user wants to set connection morph)
    do-morph-list
    "Current auto-connect morph: " target @ "_morph_connect" getpropstr 
     dup "" strcmp 0 = if
       pop "(none)"
     then
     strcat tell
    begin
      " " tell
      "Enter the command associated with the morph you want to automatically morph to when you connect, '!' to clear, or '.' to abort." tell
      read strip
      dup in-program-tell if
        pop continue
      then
      dup " " instr if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup strlen 10 > if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup "." stringcmp not if
        "Aborted." tell
        exit
      then
      dup "!" stringcmp not if
        target @ "_connect/morph" remove_prop
        target @ "_morph_connect" remove_prop
        "Auto-connect morph has been turned off." tell
        exit
      then
      dup tstr !
      morphcmd ! 
      target @ "_regmorphs" getpropstr toupper " "  morphcmd @ strcat " " strcat toupper instr not if
        "That is not a valid morph command." tell
      else
        target @ "_connect/morph" prog intostr 0 addprop
        target @ "_morph_connect" morphcmd @ 0 addprop
        "Done." tell
        exit
      then
    repeat
    exit
  then

  dup "#disconnect" stringcmp not if (user wants to set disconnection morph)
    do-morph-list
    "Current auto-disconnect morph: " target @ "_morph_disconnect" getpropstr 
     dup "" strcmp 0 = if
       pop "(none)"
     then
     strcat tell
    begin
      " " tell
      "Enter the command associated with the morph you want to automatically morph to when you disconnect, '!' to clear, or '.' to abort." tell
      read strip
      dup in-program-tell if
        pop continue
      then
      dup " " instr if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup strlen 10 > if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup "." stringcmp not if
        "Aborted." tell
        exit
      then
      dup "!" stringcmp not if
        target @ "_disconnect/morph" remove_prop
        target @ "_morph_disconnect" remove_prop
        "Auto-disconnect morph has been turned off." tell
        exit
      then
      dup tstr !
      morphcmd ! 
      target @ "_regmorphs" getpropstr toupper " "  morphcmd @ strcat " " strcat toupper instr not if
        "That is not a valid morph command." tell
      else
        target @ "_disconnect/morph" prog intostr 0 addprop
        target @ "_morph_disconnect" morphcmd @ 0 addprop
        "Done." tell
        exit
      then
    repeat
    exit
  then

  dup "#remove" stringcmp not if  (user wants to remove a morph)
    do-morph-list
    begin
      " " tell
      "Enter the command associated with the morph you want to remove or '.' to abort." tell
      read strip
      dup in-program-tell if
        pop continue
      then
      dup " " instr if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup strlen 10 > if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup "." stringcmp not if
        "Aborted." tell
        exit
      then
      dup tstr !
      morphcmd ! 
      break
    repeat
    target @ "_regmorphs" getpropstr toupper " "  morphcmd @ strcat " " strcat toupper instr if
     begin
      " " tell
      "Are you SURE you want to remove the morph '" morphcmd @ toupper strcat "', titled '" strcat target @ "name" morph-prop getpropstr strcat "'? (NO/yes)" strcat tell
      read
      dup in-program-tell if
        pop continue
      then
      1 strcut pop "y" stringcmp not if  (yup...go for it)
        target @ "_regmorphs" getpropstr toupper
        "" " " morphcmd @ toupper strcat " " strcat subst
        target @ swap "_regmorphs" swap 0 addprop
        target @ "_morph#/" morphcmd @ strcat "#" strcat remove_prop
        "Morph removed." tell
  
        target @ "_regmorphs" getpropstr strip not if (remove all trace of us)
          target @ "_regmorphs" remove_prop
          target @ "_morph#" remove_prop
          target @ "_lastmorph" remove_prop
          "You no longer have any morphs using this program." tell
        then
        exit
      then
      "Aborted." tell
        exit
     repeat
    else
     "'" morphcmd @ strcat "' is not a valid morph command.  Aborting." strcat tell
     exit
    then
  then

  dup "#add" stringcmp not if  (user wants to add/change morph)
    header
    begin
      " " tell
      "Enter a one-word command less than 10 characters to associate with this morph or '.' to abort." tell
      read strip
      dup in-program-tell if
        pop continue
      then
      dup " " instr if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup strlen 10 > if
        "Error: The command must be a single word less than 10 characters." tell
	pop continue
      then
      dup "." stringcmp not if
        "Aborted." tell
        exit
      then
      dup tstr !
      morphcmd ! 
      break
    repeat
    target @ "_regmorphs" getpropstr toupper " "  tstr @ toupper strcat " " strcat instr if
      begin
        " " tell
        "You already have a morph associated with the '" tstr @ strcat "' command." strcat tell
        "Do you want to over-write it? (NO/yes)" tell
        read
        dup in-program-tell if
          pop continue
        then
        1 strcut pop "y" stringcmp not if  (yup...go for it)
          target @ "_regmorphs" getpropstr reglist !
          break  
        then
        "Aborted." tell
        exit
      repeat 
    else
      target @ "_regmorphs" getpropstr "  " strcat tstr @ strcat "  " strcat 
      reglist !
    then
    tstr @ morphname !

    begin
      " " tell
      target @ "name" morph-prop getpropstr dup if  (ask to replace old name)
        "The current name of this morph is '" swap strcat "'." strcat tell
        "Enter the new name, a single space to keep this one, or '.' to abort." tell
        read
        dup in-program-tell if
          pop continue
        then
        strip dup "." stringcmp not if   ( abort )
          "Aborted." tell
          exit
        then
        dup not if    ( keep existing )
          pop target @ "name" morph-prop getpropstr morphname !
          break
        else
          morphname !
          break
        then 
      else
        "Enter the name of this morph, a single space for the default, or '.' to abort." tell
        read
        dup in-program-tell if
          pop continue
        then
        strip dup "." stringcmp not if   ( abort )
          "Aborted." tell
          exit
        then
        dup not if    ( set from sex/species )
          pop target @ "sex" getpropstr " " target @ "species_prop" getpropstr
          dup if
            target @ swap getpropstr else pop strcat target @ "species"
            getpropstr
          then
          strcat morphname ! "Using default name '" morphname @ strcat
           "'." strcat tell 
          break
        else
          morphname !
          break
        then 
      then
    repeat 

    begin
      " " tell
      target @ "message" morph-prop getpropstr dup if  (ask to replace old message)
        "When you change to this shape it says '" target @ name strcat " " strcat swap strcat "'." strcat tell
        "Enter the new message, a single space to keep this one, or '.' to abort." tell
        "(Do not type your name... it will be automatically inserted)" tell
        read
        dup in-program-tell if
          pop continue
        then
        strip dup "." stringcmp not if   ( abort )
          "Aborted." tell
          exit
        then
        dup not if    ( keep existing )
          pop target @ "message" morph-prop getpropstr morphmessage !
          break
        else
          morphmessage !
          break
        then 
      else
        "Enter the message printed when you change to this shape, a single space for the default, or '.' to abort." tell
        "(Do not type your name... it will be automatically inserted)" tell
        read
        dup in-program-tell if
          pop continue
        then
        strip dup "." stringcmp not if   ( abort )
          "Aborted." tell
          exit
        then
        dup not if    ( set default )
          pop "changes into a " morphname @ strcat "." strcat morphmessage !
          "Using default message '" target @ name strcat " " strcat morphmessage @ strcat "'." strcat tell 
          break
        else
          morphmessage !
          break
        then 
      then
    repeat 

    target @ "desc#" morph-prop getpropstr atoi tint !  (delete old desc if any)
    begin
      tint @ not if break then
      target @ "desc#/" tint @ intostr strcat morph-prop remove_prop
      tint @ 1 - tint !
    repeat

    target @ "_regmorphs" reglist @ 0 addprop  (set list of registered morphs)
    target @ "message" morph-prop morphmessage @ 0 addprop (set message)
    target @ "name" morph-prop morphname @ 0 addprop (set name)
    target @ "sex" morph-prop target @ "sex" getpropstr 0 addprop  (set sex)
    target @ "species" morph-prop target @ "species_prop" getpropstr dup if target @ swap getpropstr else pop target @ "species" getpropstr then 0 addprop  (set species)
    target @ "say" morph-prop target @ SAYPROP getpropstr 0 addprop  (set SAYPROP)
    target @ "osay" morph-prop target @ OSAYPROP getpropstr 0 addprop  (set OSAYPROP)
    target @ "scent" morph-prop target @ SCENTPROP getpropstr 0 addprop  (set _scent)

    target @ desc strip "{" instr 1 = if ( desc is in MPI )
      target @ desc "redesc" instr if    ( standard description )
        target @ "retempdesc#" "0" 0 addprop (nullify temp list)
        target @ "redesc#" getpropstr atoi tint !  (get # of lines )
        target @ "desc#" morph-prop tint @ intostr 0 addprop  (set # of lines )
        begin			(copy existing proplist )
          tint @ not if break then
          target @ "desc#/" tint @ intostr strcat morph-prop target @ "redesc#/" tint @ intostr strcat getpropstr 0 addprop
          tint @ 1 - tint !
        repeat
      else     (convert to stringlist & save)
        target @ target @ "retempdesc" desc-snapshot
 (       target @ DESCPROP setdesc )
      then
    else     (convert to stringlist & save)
      target @ target @ "retempdesc" desc-snapshot
 (     target @ DESCPROP setdesc )
    then
 
    target @ "retempdesc#" getpropstr atoi dup tint !
    if 
      target @ "desc#" morph-prop tint @ intostr 0 addprop (set # of lines)
      begin
        tint @ not if break then
        target @ "desc#/" tint @ intostr strcat morph-prop 
        target @ "retempdesc#/" tint @ intostr strcat getpropstr 0 addprop 
        tint @ 1 - tint !
      repeat
    else
      pop
    then

    target @ "retempdesc#" remove_prop   (erase temp propdir )
    "Morph created.  Use MORPH " morphcmd @ toupper strcat " to change to it or QMORPH " strcat morphcmd @ toupper strcat " to change to it without printing the message." strcat tell
    exit
  then

  tstr !
  target @ "_regmorphs" getpropstr toupper " " tstr @ toupper strcat " " strcat instr
  not if	(not a valid morph)
    "'" tstr @ strcat "' is not a valid morph command.  MORPH #HELP for more info." strcat tell
    exit
  then

  tstr @ morphcmd !
  target @ "scent" morph-prop getpropstr dup if target @ swap SCENTPROP swap 0 addprop else pop then  (set scent)
  target @ "sex" target @ "sex" morph-prop getpropstr 0 addprop (set sex)
  target @ "gender" target @ "sex" morph-prop getpropstr 0 addprop (set sex)
  target @ "species" target @ "species" morph-prop getpropstr 0 addprop (set species)
  target @ "species_prop" remove_prop
  target @ SAYPROP target @ "say" morph-prop getpropstr 0 addprop (set say)
  target @ OSAYPROP target @ "osay" morph-prop getpropstr 0 addprop (set osay)

  target @ "redesc#" getpropstr atoi tint !  (delete old desc)
  begin
    tint @ not if break then
    target @ "redesc#/" tint @ intostr strcat remove_prop
    tint @ 1 - tint !
  repeat

  1 tint !			( copy in new desc )
  begin
    tint @ target @ "desc#" morph-prop getpropstr atoi > if break then
    target @ "redesc#/" tint @ intostr strcat target @ "desc#/" tint @ intostr strcat morph-prop getpropstr 0 addprop
    tint @ 1 + tint !
  repeat

  target @ "redesc#" tint @ 1 - intostr 0 addprop (set # lines)
  target @ "descprop" morph-prop getpropstr dup if
    target @ swap setdesc
  else
    pop target @ DESCPROP setdesc
  then

  target @ "_lastmorph" target @ "name" morph-prop getpropstr 0 addprop (set last)

  command @ "qmorph" stringcmp if	(not a quiet morph, print message)
    loc @ #-1 target @ name " " strcat target @ "message" morph-prop getpropstr strcat notify_except
  else
    ">> You quietly morph to '" target @ "name" morph-prop getpropstr strcat "'." strcat tell
  then
;
.
c
q

