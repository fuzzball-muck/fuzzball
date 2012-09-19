@program cmd-sweep
1 10000 d
1 i
( CMD-SWEEP   by Tygryss, IMiR of XR     Written 2/5/91        )
( Sends home all players in a room who are sleeping, and who   )
(  are not owners of the room.  It also sends their contents   )
(  that they do not own home when it homes them.               )
  
(
  _sweep              message shown when you sweep the room.
  _swept or _osweep   message shown if you are swept.
  _sweep_player       message shown if you sweep an individual player.
  _sweep_to           where players swept in this room go to. ie: room #1234
  _sweep_authorized   players authorized to sweep this room.  #12 #432 #190
  _sweep_immune       players immune to general sweeps in this room.
  _public?            if 'yes' means this room in sweepable by anyone.
  _sweep_public?      if 'yes' means this room in sweepable by anyone.
  _sweep_immune?      If yes, means this THING doesn't home from inventory
                       when the player is swept.
  @/sweepable?        If 'yes', player it is set on can be swept by anyone,
                       they are swept home, always.
)
$include $lib/strings
  
$def LOGOUT_SAFE_TIME 180
$def getlinkfix dup program? if owner else getlink then

: doenvprop[ ref:obj str:prop -- str:result ]
    obj @ prop @ envprop if
        prop @ "[sweep]"  0 parseprop
    else
        pop ""
    then
;
  
: show-list
   dup not if pop exit then
   me @ over 2 + rotate notify
   1 - show-list
;
: show-help
"__Propname_________ _where_ _What_it_does____________________________________"
" _sweep              you     message shown when you sweep a room.            "
" _swept or _osweep   you     message shown when you are swept.               "
" _sweep_player       you     message shown if you sweep an individual player."
" _sweep_to           room    dbref of where players swept in the room go to. "
" _sweep_authorized   room    space seperated dbrefs of players allowed to    "
"                               sweep the room.  ex.  #1234 #465 #12 #8       "
" _sweep_immune       room    space seperated dbrefs of players who are immune"
"                               to being swept from here.  ex. #567 #12 #8    "
" _sweep_public?      room    if 'yes' means this room in sweepable by anyone."
" _sweep_immune?      item    If 'yes', means this THING doesn't get sent home"
"                               when the player holding it is swept.          "
" @/sweepable?        player  If 'yes', player is sweepable by anyone, and    "
"                               gets sent home always when swept.                    "
"_____________________________________________________________________________"
"                                                                             "
" _sweep, _swept, and _sweep_player messages can also be set on a room, as the"
"   default messages if a player sweeping there does not have them set.       "
" different formats for specific player sweeps can be had by setting props    "
"   with the prefix '_sweep_', containing the format, then specifying them at "
"   sweep time.  ie: if you have a _sweep_nasty format set, you can use it by "
"   typing 'sweep <player>=nasty'                                             "
22 show-list
;
  
: home-object (objectdbref -- )
   #-3 moveto
;
  
: home-objects (playerdbref objdbref -- )
   dup not if pop pop exit then   (exit if done with the player's contents)
   over over owner dbcmp not if   (if object owned by the player, ignore it)
      dup "_sweep_immune?"        (if not owned by sweepee, and *not* set)
      getpropstr 1 strcut pop     (_sweep_immune?:yes then sweep is okay)
      "y" stringcmp if
         dup next swap            (else get next object in players contents)
         home-object              (& home the current object)
      else next
      then
   else next
   then
   home-objects                   (go on to next object in player's contents)
;
  
: home-player (dbref -- )
   dup name " " strcat over "_osweep" doenvprop
   dup not if pop over "_swept" doenvprop then
   dup not if pop trigger @ "_swept" doenvprop then
   dup not if pop "is sent home." then
   strcat me @ swap pronoun_sub
   over location #-1 rot notify_except
  
   dup me @ location "_sweep_to" doenvprop
   dup "#" 1 strncmp not if 1 strcut swap pop then
   atoi dbref me @ location owner
   over owner dbcmp not
   ( d d dswto i )
   3 pick "@/sweepable?" getpropstr "yes" strcmp not or
   if
      pop dup getlinkfix
   then moveto                       (& move player home)
pop exit
   dup contents home-objects         (& home objects carried)
;
  
: authorized? (dbref -- bool)
   dup location "_sweep_authorized" doenvprop " " strcat
   swap intostr " " strcat "#" swap strcat instr
;
  
: immune? (dbref -- bool)
   dup location "_sweep_immune" doenvprop " " strcat
   swap intostr " " strcat "#" swap strcat instr
;
  
: awake-or-recent? (dbref -- bool )
   dup awake? if pop 1 exit then
   timestamps pop rot rot pop pop
   systime swap - LOGOUT_SAFE_TIME <=
;
  
: home-players (sweep? #swept dbref -- #swept)
   dup not if pop swap pop exit then   (exit if done with room's contents)
   dup player? not if                  (if not a player object, ignore it)
      next
   else
      dup awake-or-recent? if      (if player is awake or recently off, ignore)
         next
      else
         dup dup location owner dbcmp if    (if owner of room, ignore)
            next
         else
            dup location over getlinkfix dbcmp if    (if already home, ignore)
               next
            else
               dup immune? if          (if immune in this room, ignore)
                  next
               else
                  swap 1 + swap        (inc count of swept players)
                  3 pick if
                     dup next swap     (else get next obj in room)
                     home-player
                  else
                     next
                  then
               then
            then
         then
      then
   then
   home-players               (go on to next object in rooms contents)
;
  
: sweep-room
   "me" match me !
   dup not if
      me @ dup location owner dbcmp not
      me @ "wizard" flag? not and
      me @ authorized? not and
      me @ location "_public?" doenvprop "yes" stringcmp and
      me @ location "_sweep_public?" doenvprop "yes" stringcmp and
      if
            pop "Permission denied."
            me @ swap notify exit
      then
      me @ location contents 0 0 rot home-players (count num to be swept)
      0 > if
         me @ "_sweep" doenvprop
         dup not if pop me @ location "_sweep" doenvprop then
         dup not if pop trigger @ "_sweep" doenvprop then
         dup not if pop
            "pulls out a big fuzzy broom and sweeps the room clean of sleepers."
         then
         me @ swap pronoun_sub
         me @ name " " strcat swap strcat
         me @ location #-1 rot notify_except
         me @ location contents 1 0 rot home-players   (sweep the room)
         dup 1 = if
            pop me @ "1 player sent home." notify
         else
            intostr " players sent home."
            strcat me @ swap notify
         then
      else
         me @ "No one sent home." notify
      then
   else (sweep <object>)
  
      "=" .split .stripspaces swap .stripspaces
      dup "#help" stringcmp not if
         pop show-help exit
      then
      dup "*" 1 strncmp not
      me @ "W" flag? not and
      if 1 strcut swap pop then
      match dup not if
         pop "I don't see that here."
         me @ swap notify exit
      then
      dup #-2 dbcmp if
         pop "I don't know which one you mean!"
         me @ swap notify exit
      then
      dup room? over exit? or if
         pop "I can't home that!"
         me @ swap notify exit
      then
      dup location me @ location dbcmp not if
         me @ over owner dbcmp not
         me @ 3 pick location owner dbcmp not and
         me @ "w" flag? not and if
            "I don't see that here!"
            me @ swap notify pop exit
         then
      then
      me @ over owner dbcmp not
      me @ 3 pick location owner dbcmp not and
      me @ "w" flag? not and
      over thing? if
         over location "_public?" getpropstr tolower "y" 1 strncmp 
         3 pick location "_sweep_public?" getpropstr tolower "y" 1 strncmp and
         3 pick "_puppet?" getpropstr
         4 pick "Z" flag? or        (Puppet or zombie)
         not or                     (p/z or _public)
         3 pick owner "W" flag? or  (thing owned by wizard ?)
         AND                        (with the rest of it all)
      then
      over player? if
         over "@/sweepable?" getpropstr
         "yes" stringcmp and
      then
      me @ #1026 dbcmp not and   (give Revar permissions.   Used only for
                                  sweeping spammers out of public rooms.)
      me @ authorized? not and if
         pop "Permission denied."
         me @ swap notify exit
      then
      dup player? if
         dup location over getlinkfix dbcmp if
            "They are already home!"
            me @ swap notify
         else
            over not if swap pop "player" swap then
            me @ "_sweep_" 4 rotate strcat doenvprop
            dup not if pop me @ location "_sweep_player" doenvprop then
            dup not if pop "sweeps %n off to %p home." then
            over swap pronoun_sub
            me @ name " " strcat swap strcat
            over location #-1 rot notify_except
            home-player
         then
      else
         dup location over getlinkfix dbcmp if
            me @ "That's already home!" notify
         else
            dup name " swept home by " strcat
            me @ name strcat "." strcat
            over location dup room? if
               me @ 3 pick notify_except
            else pop
            then
            me @ swap notify
            home-object
         then
      then
   then
;
.
c
q
@action sweep;swee=#0=tmp/exit1
@link $tmp/exit1=cmd-sweep

