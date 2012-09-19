@prog lib-install.muf
1 19999 d
i
( Common MUF Interface -- Installation/Upgrade Component )
( Designed 3/98 by Triggur )

( V1.0  : {04/04/98} Inception - Triggur )
( V1.1  : {01/21/02} Wizbit check fix - Nightwind )
( V1.2  : {06/03/04} Wizbit check REAL fix, and bootstrap fix - Winged )
( V1.21 : {06/18/04} Make the program compile again - Schneelocke )


$define THISVERSION "1.3" $enddef 
$ifndef __version<Muck2.2fb6.01
$version 1.003
$lib-version 1.000
$endif

$define THISCMD "@install;@uninstall" $enddef
$define THISLIB "install" $enddef
$define THISREG "install" $enddef

$define GLOBALENV #0 $enddef
$define NOTHING #-1 $enddef
$define AMBIGUOUS #-2 $enddef
$define HOME #-3 $enddef

lvar tstr1
lvar tstr2
lvar tstr3
lvar tdb1
lvar tdb2
lvar tint1

lvar ltstr1
lvar ltstr2
lvar ltstr3
lvar ltstr4
lvar ltdb1
lvar ltdb2
lvar ltint1

( ------------------------------------------------------------------- )
( WIZ-CHECK: Check to see if wizard permissions exist                 )
( ------------------------------------------------------------------- )

: WIZ-CHECK ( -- X )
  ( If the person calling the program is a wizard, let it run )
  me @ "wizard" flag? ( -- i1 ) ( person running has a wizard flag )
  ( If the prog calling the program runs with wiz perms, let it run )
  caller "wizard" flag? ( -- i1 i2 ) ( caller has a wizard flag )
  caller owner "truewizard" flag? and ( -- i1 i2 )
    ( caller has wiz flag AND owner of caller is a wizard )
  dup if ( if caller has wizard flag... )
    caller "setuid" flag? ( -- i1 i2 ics )
      ( does the caller have a setuid flag? )
    caller "harduid" flag? ( -- i1 i2 ics ich )
      ( does the caller have a harduid flag? )
    and ( -- i1 i2 icsh ) not ( if caller both SH, caller has no wizbit )
    and ( -- i1 iwizperm )
  then
  or ( -- i )  ( one of the two has a wizbit flag )
  not if
    "ERROR: This function may only be called by a wizard!" me @ swap notify
    exit
  then 
;

: FUNCTION-WIZ-CHECK ( -- X )
  WIZ-CHECK 1
;

: INSTALL-WIZ-CHECK ( -- X )
  WIZ-CHECK 1
;

: UNINSTALL-WIZ-CHECK ( -- X )
  WIZ-CHECK 1
;

( ------------------------------------------------------------------- )
( add {or update} a global command {WIZ ONLY}                         )
( in: d: dbref of program to link the global exit to                  )
(     s: name of the global exit to create                            )
( uses: tstr1, tdb1, tstr2                                            )
( ------------------------------------------------------------------- )
: add-global-command ( d s -- )
  tstr1 ! tdb1 !

  FUNCTION-WIZ-CHECK

  tstr1 @ ( -- s )
  ";" instr ( -- i ) 
  dup 0 = ( -- i b ) if ( -- i ) ( if 0, this is a single command )
    pop ( -- )
    tstr1 @ tstr2 ! ( -- ) ( single command, set tstr2 to it )
  else ( -- i ) ( else is a multiple command )
    tstr1 @ swap ( -- s i )
    strcut ( -- s2 s1 ) swap pop tstr2 ! ( -- )
  then ( -- )                        ( tstr2 = first command in list )

  GLOBALENV tstr2 @ rmatch ( -- d ) int dup 0 < if ( -- i )
    ( if less than 0, check to see if #-2 or #-3 before creating )
    dup -1 < if ( -- i )
      dup -3 = if ( -- i )
        me @ "INSTALL ERROR: cannot name an exit 'home', install aborted!"
        notify
        exit
      then ( -- i )
      dup -2 = if ( -- i )
        me @ "ERROR: Multiple items of the name '" tstr2 @ "' found in #0,"
        notify
        me @ "please fix before reattempting installation.  ABORTED." notify
        exit
      then ( -- i )
    then ( -- i )
    pop ( -- )
    me @ "Installing global '" tstr1 @ strcat "'..." strcat notify
    GLOBALENV tstr1 @ newexit
  then

  GLOBALENV tstr2 @ rmatch ( -- d ) ( either already found or created )
  name tstr1 @ stringcmp if ( -- )  ( if not the same, update command list )
    GLOBALENV tstr2 @ rmatch dup tstr1 @ setname
  then ( -- )

  GLOBALENV tstr2 @ rmatch getlink tdb1 @ dbcmp not if (update destination )
    me @ "INSTALL: Updating link on '" tstr2 @ strcat
    "' from #" strcat GLOBALENV tstr2 @ rmatch getlink intostr strcat
    " to #" strcat tdb1 @ int intostr strcat "." strcat notify
    GLOBALENV tstr2 @ rmatch dup #-1 setlink
    tdb1 @ setlink
  then
;

( ------------------------------------------------------------------- )
( remove a global command from #0 {WIZ ONLY}                          )
( ------------------------------------------------------------------- )
: remove-global-command ( s -- )
  tstr1 !

  FUNCTION-WIZ-CHECK

  tstr1 @ ";" instr dup 0 if
    tstr1 @ swap 1 - strcut pop tstr2 !
  else
    pop tstr1 @ tstr2 !
  then                               ( tstr2 = first command in list )

  #0 tstr2 @ rmatch int 0 < not if  (delete it only if it exists, of course)
    me @ "Removing global '" tstr1 @ strcat "'..." strcat notify
    #0 tstr2 @ rmatch recycle
  then
;


( ------------------------------------------------------------------- )
( add {or update} a global library entry {WIZ ONLY}                   )
( THIS COMMAND MUST BE RUN BEFORE ANY CALLS TO export-* !!!           )
( ------------------------------------------------------------------- )
: add-global-library ( d s -- )
  tstr1 ! tdb1 !

  FUNCTION-WIZ-CHECK

  me @ "Registering library '" tstr1 @ strcat "'..." strcat notify
  #0 "_reg/lib/" tstr1 @ strcat tdb1 @ setprop

  caller "_lib-name" tstr1 @ 0 addprop
;

( ------------------------------------------------------------------- )
( Remove a global library entry {WIZ ONLY}                            )
( THIS COMMAND CAN ONLY BE RUN AFTER ANY CALLS TO LIBRARY FUNCTIONS!  )
( ------------------------------------------------------------------- )
: remove-global-library ( s -- )
  tstr1 !

  FUNCTION-WIZ-CHECK

  me @ "Deregistering library '" tstr1 @ strcat "'..." strcat notify
  #0 "_reg/lib/" tstr1 @ strcat remove_prop
;

( ------------------------------------------------------------------- )
( add {or update} a global registry entry {WIZ ONLY}                  )
( ------------------------------------------------------------------- )
: add-global-registry ( d s -- )
  tstr1 ! tdb1 !

  FUNCTION-WIZ-CHECK

  me @ "Registering '" tstr1 @ strcat "'..." strcat notify
  #0 "_reg/" tstr1 @ strcat tdb1 @ setprop
;

( ------------------------------------------------------------------- )
( remove a global registry entry {WIZ ONLY}                           )
( ------------------------------------------------------------------- )
: remove-global-registry ( s -- )
  tstr1 !

  FUNCTION-WIZ-CHECK

  me @ "Deregistering '" tstr1 @ strcat "'..." strcat notify
  #0 "_reg/" tstr1 @ strcat remove_prop
;

( ------------------------------------------------------------------- )
( Create the _defs text to export a library function {WIZ ONLY}       )
( ------------------------------------------------------------------- )
: export-function ( d s -- )
  tstr1 ! tdb1 !

  FUNCTION-WIZ-CHECK
( ALSO CAUSING PROBLEMS?
  tdb1 @ "_lib-name" getpropstr "" strcmp not if
    me @ "$LIB/INSTALL: CONFIGURATION ERROR: export-function cannot "
         "be called until after export-global-library is called!" strcat
    notify
    exit
  then
)
  me @ "Exporting " tdb1 @ name strcat " function '" strcat tstr1 @ strcat
      "'..." strcat notify

  tdb1 @ "_defs/" tstr1 @ strcat "\"$lib/" tdb1 @ "_lib-name" getpropstr
      strcat  "\" match \"" strcat tstr1 @ strcat "\" call" strcat 0 addprop
;

( ------------------------------------------------------------------- )
( Create the _defs text to export a library macro {WIZ ONLY}          )
( ------------------------------------------------------------------- )
: export-macro ( d s s -- )
  tstr2 ! tstr1 ! tdb1 !

  FUNCTION-WIZ-CHECK

  me @ "Exporting " tdb1 @ name strcat " macro '" strcat tstr1 @ strcat
      "'..." strcat notify

  tdb1 @ "_defs/" tstr1 @ strcat tstr2 @ 0 addprop
;

( ------------------------------------------------------------------- )
( declare the version of a library {WIZ ONLY}                         )
( ------------------------------------------------------------------- )
: set-library-version ( d s -- )
  tstr1 ! tdb1 !

  FUNCTION-WIZ-CHECK

  me @ "Setting library " tdb1 @ name strcat " version to '" strcat
      tstr1 @ strcat "'." strcat notify
  tdb1 @ "_lib-version" tstr1 @ 0 addprop
;

( ------------------------------------------------------------------- )
( fetch the version of a library                                      )
( ------------------------------------------------------------------- )
: get-library-version ( d -- s )
  tdb1 !

  tdb1 @ "_lib-version" getpropstr
;

( ------------------------------------------------------------------- )
( NAME:  perform-install                                              )
( Function: call a MUF program's do-install word                      )
( input: s1 --                                                        )
(    s1: string describing the library to install                     )
( output: --                                                          )
( vars:  ltstr1, ltstr2, ltstr3, ltstr4, ltdb1                        )
( ------------------------------------------------------------------- )

: perform-install ( s -- )
          ltstr1 ! ( -- )
  NOTHING ltdb1  ! ( -- )

  FUNCTION-WIZ-CHECK ( -- )

  ltstr1 @ match ( -- d )
  dup intostr atoi ( -- d i ) -1 > if ( -- d )
    dup program? if ( -- d )
      ( We'll assume that we want to do based on the matched argument )
      dup ltdb1 ! ( -- d )
      name ltstr1 ! ( -- )
    else
      pop ( it matched something other than a program, we don't want )
    then
  else
    pop ( it's NOTHING, AMBIGUOUS, or HOME, none of which we want )
  then

  ( sanity check just to be sure )
  ltstr1 @ string? not if
    "perform-install: ltstr1 is not a string!" abort
  then
  ltdb1 @ dbref? not if
    "perform-install: ltdb1 is not a dbref!" abort
  then

  ( if it's NOTHING, we still need to find the program... ) 
  ltdb1 @ NOTHING dbcmp not if ( -- )
    GLOBALENV ( -- d )
    "_ver/" ltstr1 @ "/prog" strcat strcat ( -- d s )
    dup ltstr4 ! ( -- d s )   ( ...save propname for later use... )
    getprop ( -- x )
    dup string? not if ( -- x )
      dup int?   ( -- x i1 )
      swap       ( -- i1 x )
      dup dbref? ( -- i1 x i2 )
      rot        ( -- x i2 i1 )
      or         ( -- x i ) ( x is either an int or dbref, can be used )
      not if ( -- x )
        pop
        "ERROR: #0=" ltstr4 @ strcat 
        " is not a usable dbref -- cannot continue." strcat
        me @ swap notify
        exit
      then
      intostr ( -- s ) 
    then      
    dup "" strcmp not ( -- s i ) if ( ...if string is empty... )
      pop ( get rid of the empty string on the stack )
      me @ "ERROR: Unknown program.  Use MUF name or dbref." notify
      exit
    then ( -- s )
    ( we have a dbref )
    atoi dbref ( -- d ) dup ( -- d d )
    program? not if ( -- d )
      "ERROR: #0=" ltstr4 @ " points to #" strcat strcat ( -- d s )
      swap intostr strcat ( -- s)
      ", which is not a program!" strcat ( -- s )
      me @ swap notify ( -- )
      exit 
    then ( -- d )
    ( it's a dbref to a program!  yippee! )
    dup name ( -- d s )
    ltstr1 ! ( -- d )
    ltdb1 ! ( -- )
  then ( -- )

  NOTHING ltdb1 @ dbcmp if ( -- )
    "ERROR: Should not be able to get here -- ltdb1 is #-1!"
    me @ swap notify
    exit
  then

  ( Get the current version of the program to be installed... )
  GLOBALENV "_ver/" ltstr1 @ strcat "/vers" strcat getpropstr ltstr2 ! ( -- )

  ( BUGBUG: if ltstr1 doesn't match what the library will eventually be  )
  ( entered as, ltstr2 gets to be empty.  This might be a bit strange if )
  ( the library enters itself as another name?                           )
   
  ltstr2 @ ltdb1 @ "do-install" call      ( call the program's installer )

  ltstr3 !
  ltstr3 @ string? not if
    me @ "ERROR: program returned non-string on top of stack." notify
  then

  ltstr3 @ "" stringcmp not if
    me @ "WARNING: Installation not completed." notify
    exit
  then

  ltstr3 @ ltstr2 @ stringcmp not if
    me @ "@INSTALL: " ltstr1 @ strcat " reports version " strcat ltstr2 @ strcat
        " already installed." strcat notify
  else
    me @ "@INSTALL: Upgraded " ltstr1 @ strcat " to version " strcat ltstr3 @
        strcat "." strcat notify
  then

  #0 "_ver/" ltstr1 @ strcat "/prog" strcat ltdb1 @ setprop
  #0 "_ver/" ltstr1 @ strcat "/vers" strcat ltstr3 @ 0 addprop
;

( ------------------------------------------------------------------- )
( call a MUF program's do-uninstall function                          )
( ------------------------------------------------------------------- )
: perform-uninstall ( s -- )
  ltstr1 !

  FUNCTION-WIZ-CHECK
 
  ltstr1 @ 1 strcut pop "#" stringcmp not if (is a dbref)
    ltstr1 @ 1 strcut swap pop atoi dbref dup ltdb1 ! name ltstr1 !
  else
    #0 "_ver/" ltstr1 @ strcat "/prog" strcat getpropstr dup "" strcmp not if
      me @ "ERROR: Unknown program.  Use MUF name or dbref." notify
      pop exit
    then
    int dbref name ltstr1 !
  then

  ltdb1 @ #-1 dbcmp if
    #0 "_ver/" ltstr1 @ strcat "/prog" strcat getprop dbref ltdb1 !
  then
  #0 "_ver/" ltstr1 @ strcat "/vers" strcat getpropstr ltstr2 !

  ltdb1 @ program? not if
    me @ "@INSTALL ERROR: Dbref #" ltdb1 @ intostr strcat " is not a program."
        strcat notify
    exit
  then

  ltstr2 @ ltdb1 @ "do-uninstall" call ltstr3 ! (call program's uninstaller )

  ltstr3 @ "" stringcmp not if
    me @ "WARNING: Installation not completed." notify
    exit
  then

  #0 "_ver/" ltstr1 @ strcat remove_prop

  me @ "@UNINSTALL: Removed " ltstr1 @ strcat " from the system." strcat notify
;

( ------------------------------------------------------------------- )
( PUBLIC: Perform installation/upgrade of this command                )
( ------------------------------------------------------------------- )
: do-install ( s -- s )

  INSTALL-WIZ-CHECK

  prog "W" flag? not if 
    me @ "ADMIN: Do @SET #" prog intostr strcat "=W" strcat notify
    me @ "ADMIN: Then re-run " command @ strcat " #" strcat prog intostr strcat
        notify
    "" exit
  then

  prog "L" set  ( make it publically linkable )

  prog THISCMD add-global-command
  prog THISLIB add-global-library 
  prog THISREG add-global-registry 

  prog "add-global-command" export-function
  prog "add-global-library" export-function
  prog "add-global-registry" export-function
  prog "export-function" export-function
  prog "export-macro" export-function
  prog "set-library-version" export-function
  prog "get-library-version" export-function
  prog "remove-global-command" export-function
  prog "remove-global-library" export-function
  prog "remove-global-registry" export-function

  prog "INSTALL-WIZ-CHECK" export-function
  prog "UNINSTALL-WIZ-CHECK" export-function
  prog "FUNCTION-WIZ-CHECK" export-function

  prog THISVERSION set-library-version

  THISVERSION 
; 

( ------------------------------------------------------------------- )
( PUBLIC: Perform uninstallation of this command                      )
( ------------------------------------------------------------------- )
: do-uninstall ( s -- s )
  pop

  UNINSTALL-WIZ-CHECK

  prog THISCMD remove-global-command
  prog THISLIB remove-global-library 
  prog THISREG remove-global-registry 

  me @ "The program can now be removed entirely by typing @rec #" prog intostr
      strcat notify

  VERSION 
; 

: main ( s -- i )

  command @ "@install" stringcmp not if
    perform-install
    0 exit
  then

  command @ "@uninstall" stringcmp not if
    perform-uninstall
    0 exit
  then

  me @ "USAGE:  @install #<prog/lib dbref>" notify
  0 exit
;

PUBLIC add-global-command
PUBLIC add-global-library
PUBLIC add-global-registry
PUBLIC export-function
PUBLIC export-macro
PUBLIC set-library-version
PUBLIC get-library-version
PUBLIC remove-global-command
PUBLIC remove-global-library
PUBLIC remove-global-registry
PUBLIC INSTALL-WIZ-CHECK
PUBLIC UNINSTALL-WIZ-CHECK
PUBLIC FUNCTION-WIZ-CHECK

PUBLIC do-install
PUBLIC do-uninstall
.
c
q
@set lib-install.muf=w
@action @install;@uninstall=me=tmp/exit1
@link $tmp/exit1=lib-install.muf
@install lib-install.muf
