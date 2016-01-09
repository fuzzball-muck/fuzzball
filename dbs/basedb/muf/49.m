( CMD-SWEEP   by Tygryss, IMiR of XR     Orig Written 2/5/91   )
( Sends home all players in a room who are sleeping, and who   )
(  are not owners of the room.  It also sends their contents   )
(  that they do not own home when it homes them.               )
  
(
    _sweep/sweep        message shown when you sweep the room.
    _sweep/swept        message shown if you are swept.
    _sweep/fmt/         propdir where sweep-player formats are stored.
    _sweep/fmt/std      message shown if you sweep an individual player.
    _sweep/to           where players swept in this room go to. ie: room #1234
    _sweep/authorized   players authorized to sweep this room.  #12 #432 #190
    _sweep/immune       players immune to general sweeps in this room.
    _sweep/public?      if 'yes' means this room in sweepable by anyone.
    _sweep/immune?      If yes, means this THING doesn't home from inventory
                         when the player is swept.
)
  
$define DEFAULT_SWEEP_MESG
  "pulls out a big fuzzy broom and sweeps the room clean of sleepers."
$enddef
  
$include $lib/strings
$include $lib/props
$include $lib/edit
$include $lib/match
  
: show-help
"__Propname_________ _where_ _What_it_does____________________________________"
" _sweep/sweep        you     message shown when you sweep a room.            "
" _sweep/swept        you     message shown when you are swept.               "
" _sweep/fmt/         you     propdir where sweep-player formats are kept.    "
" _sweep/fmt/std      you     default message shown if you sweep one player.  "
" _sweep/to           room    dbref of where players swept in the room go to. "
" _sweep/authorized   room    space seperated dbrefs of players allowed to    "
"                               sweep the room.  ex.  #1234 #465 #12 #8       "
" _sweep/immune       room    space seperated dbrefs of players who are immune"
"                               to being swept from here.  ex. #567 #12 #8    "
" _sweep/public?      room    if 'yes' means this room in sweepable by anyone."
" _sweep/immune?      item    If 'yes', means this THING doesn't get sent home"
"                               when the player holding it is swept.          "
"_____________________________________________________________________________"
"                                                                             "
" sweep, swept, and fmt/* messages can also be set on a room, as the default  "
"   messages if a player sweeping there does not have them set.               "
" different formats for specific player sweeps can be had by setting props    "
"   with the prefix '_swee/fmt/', containing the format, then specifying them "
"   at sweep time.  ie: if you have a _sweep/fmt/nasty format set, you can    "
"   use it by typing 'sweep <player>=nasty'                                   "
20 EDITdisplay
;
  
  
: home-object (objectdbref -- )
    #-3 moveto
;
  
: home-objects (playerdbref objdbref -- )
    dup not if pop pop exit then  (exit if done with the player's contents)
    over over owner dbcmp not if  (if object owned by the player, ignore it)
      dup "_sweep/immune?"          (if not owned by sweepee, and *not* set)
      getpropstr 1 strcut pop       (_sweep_immune?:yes then sweep is okay)
      "y" stringcmp if
        dup next swap             (else get next object in players contents)
        home-object               (& home the current object)
      else next
      then
    else next
    then
    home-objects                  (go on to next object in player's contents)
;
  
: home-player (dbref -- )
    dup name " " strcat
    over "_sweep/swept" .envprop
    dup not if pop trigger @ "_sweep/swept" .envprop then
    dup not if pop "is sent home." then
    strcat me @ swap pronoun_sub
    over location #-1 rot notify_except
    dup me @ location "_sweep/to" .envprop
    dup "#" 1 strncmp not if 1 strcut swap pop then
    dup atoi dbref me @ location owner
    over owner dbcmp not rot not or if
        pop dup getlink
    then
    moveto                               (& move player home)
    dup contents home-objects            (& home objects carried)
;
  
: authorized? (dbref -- bool)
    dup location "_sweep/authorized" .envprop " " strcat
    swap intostr " " strcat "#" swap strcat instr
;
  
: immune? (dbref -- bool)
    dup location "_sweep/immune" .envprop " " strcat
    swap intostr " " strcat "#" swap strcat instr
;
  
: home-players (sweep? #swept dbref -- #swept)
    dup not if pop swap pop exit then  (exit if done with room's contents)
    dup player? not if            (if not a player object, ignore it)
      next
    else
      dup awake? if               (if the player is awake, ignore them)
        next
      else
        dup dup location owner dbcmp if        (if owner of room, ignore)
          next
        else
          dup location over getlink dbcmp if   (if already home, ignore)
            next
          else
            dup immune? if                     (if immune in this room, ignore)
              next
            else
              swap 1 + swap                    (inc count of swept players)
              3 pick if
                dup next swap                   (else get next obj in room)
                home-player
              else
                next
              then
            then
          then
        then
      then
    then
    home-players                  (go on to next object in rooms contents)
;
  
: sweep-room
    "me" match me !
    caller exit? not if pop exit then
    dup not if
      me @ dup location .controls not
      me @ authorized? not and
      me @ location "_sweep/public?" .envprop
      "yes" stringcmp and if
          pop "Permission denied."
          me @ swap notify exit
      then
      me @ location contents 0 0 rot home-players (count num to be swept)
      0 > if
        me @ "_sweep/sweep" .envprop
        dup not if pop trigger @ "_sweep/sweep" .envprop then
        dup not if pop
          DEFAULT_SWEEP_MESG
        then
        me @ swap pronoun_sub
        me @ name " " strcat swap strcat
        me @ location #-1 rot notify_except
        me @ location contents 1 0 rot home-players  (sweep the room)
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
        me @ over .controls not
        me @ 3 pick location .controls not and if
          "I don't see that here!"
          me @ swap notify pop exit
        then
      then
      me @ over .controls not
      over "Z" flag?
      3 pick "_listen" getpropstr or
      3 pick "_listen" propdir? or
      3 pick "_olisten" getpropstr or
      3 pick "_olisten" propdir? or
      3 pick thing? and
      3 pick location "_sweep/public?"
      getpropstr "yes" stringcmp not and not and
      me @ 3 pick location .controls not and
      me @ authorized? not and if
        pop "Permission denied."
        me @ swap notify exit
      then
      dup player? if
        dup location over getlink dbcmp if
          "They are already home!"
          me @ swap notify
        else
          over not if swap pop "player" swap then
          me @ "_sweep/fmt/" 4 rotate strcat .envprop
          dup not if pop me @ location "_sweep/fmt/std" .envprop then
          dup not if pop "sweeps %n off to %p home." then
          over swap pronoun_sub
          me @ name " " strcat swap strcat
          over location #-1 rot notify_except
          home-player
        then
      else
        dup location over getlink dbcmp if
          me @ "That's already home!" notify
        else
          dup name " sent home." strcat
          me @ swap notify
          home-object
        then
      then
    then
;
