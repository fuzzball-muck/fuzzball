(RIDE ENGINE 3.7FB By Riss)
$include $lib/REFLIST
$def globalprop #102
var target
var mess
var mode
 
: taurREF-first     ( -- d)
     me @ "RIDE/ontaur" REF-first
;
: taurREF-next      (d -- d')
     me @ "RIDE/ontaur" rot REF-next
;
: taurREF-delete    (d -- )
     me @ "RIDE/ontaur" rot REF-delete
;
: taurREF-list      ( -- s)
     me @ "RIDE/ontaur" REF-list
;
: taurREF-add       (d -- )
     me @ "RIDE/ontaur" rot REF-add
;
: porz?   (d -- b  Player or zombie   ***** 3.7)
dup player? if pop 1 exit then
dup thing? if "Z" flag? exit then
pop 0
;
: Anyonehome?       ( -- b True if first player in Ref-list)
     taurRef-first
     porz?            (***** 3.7)
     
;
: getonwho     (d -- d)
     "RIDE/onwho" getpropstr atoi dbref
;
 
: onwho?            (d -- b        True if on you  **** 3.5)
     dup                                (d d  For second check)
     getonwho
     me @ dbcmp     (d b)
     swap intostr                       (b s)
     "RIDE/_check/" swap strcat
     globalprop swap getpropstr         (b s')
     atoi dbref me @ dbcmp
(    dup not if "RIDE: Possible security fault." .tell then)
     AND       (both checks)
;
 
: getatrig          (d-- d'       **** 3.5)
     "RIDE/theta" getpropstr atoi dbref
;
: setatrig               ( --   records triggerdbref on taur   **** 3.5)
     prog trig dbcmp if  (caused by RIDE?)
          me @ getonwho getatrig
     else
          trig
     then
 
     me @
     "RIDE/theta" rot intostr 0 addprop
;
 
 
: atoldloc?         (d -- b        True if at old location)
     location me @ "RIDE/oldloc" getpropstr atoi dbref dbcmp
;
 
: setoldloc         ( -- set taur location to here)
     me @ "RIDE/oldloc" me @ location intostr 0 addprop
;
: getoldloc         ( -- d)
     me @ "RIDE/oldloc" getpropstr atoi dbref
;
 
: dororcheck   (d -- b   Rider on rider check  one time)
     dup       (d d)
     getonwho getatrig   (get the trig used by the taur)
     dup       (d d' d') 
     exit? if
          locked? EXIT
     then
     (nuts with it.. need recursion)
     pop pop 
     0         (<- free ride)
;
 
: lockedout?        (d -- b  True if locked out)
     OWNER               (***** 3.7 for ZOMBIES!)
     dup  (D D)
     dup  (D D D)
     prog owner dbcmp swap    (D b  D)
     "W" flag? or   (D b)
     me @ "W" flag? or (D B)
     me @ prog owner dbcmp or (D B')
     me @ name "Riss" stringcmp not or (D B")
     loc @ "_yesride" getpropstr "yes" stringcmp not or
     if pop 0 EXIT then (not locked out)
     (D)
     loc @ owner me @ dbcmp   (I own room?)
     if pop 0 EXIT then
     (D)
     loc @ "_noride" getpropstr "yes" stringcmp not
     if pop 1 EXIT then       (room set to _noride)
     (D)
     trig exit?     (D b)
     if trig locked? EXIT then     (trig WAS an exit)
     (D ok.. must have been a program moveto of some sort)
     trig prog dbcmp     (did RIDE do the moveto?)
     if dororcheck EXIT then  (do rider on rider check)
     (D driveto or objexit maybe?)
 trig mlevel 3 = trig "W" flag? or
     if pop 0 EXIT then (free pass if lev 3 moveto ***** 3.7)
     pop  (heck with the rider.. check the room)
     loc @ "J" flag? not if 1 EXIT then      (no J flag)
     loc @ "vehicle_ok?" .envprop "yes" stringcmp 
     loc @ "vehicle_ok"  .envprop "yes" stringcmp AND
     loc @ "_vehicle_ok?" .envprop "yes" stringcmp AND
     loc @ "_vehicle_ok"  .envprop "yes" stringcmp AND
     (exits true if stuff from driveto.muf not found)
;
               
 
: listlocked?       ( -- b)
     taurREF-first
     dup
     porz? not if   (if first dbref no good then cancel ***** 3.7)
          pop 1 exit
     then                (d)
     0 swap              (b d)
     BEGIN               (b d)
          dup            (b d d)
          porz? WHILE    (b d        ***** 3.7)
          dup            (b d d)
          lockedout?     (b d b)
          rot            (d b b)
          or             (d b)
          swap           (b d)
          taurREF-next        (b d')
     REPEAT
     pop                 (b)
;
 
: getmsg  (s -- s')
     mess ! me @ "RIDE/_mode" getpropstr mode !
     me @ "RIDE/" mode @ strcat "/" strcat mess @ strcat
     getpropstr
     dup not if     (no good, try global)
          pop globalprop
          "RIDE/" mode @ strcat "/" strcat mess @ strcat
          getpropstr
          dup not if     (again no good)
               pop
               globalprop
               "RIDE/RIDE/" mess @ strcat
               getpropstr
          then
     then
;
 
: tellnotonwho      (d --   of player not on taur)
     dup            (Tells taur who is not on them)
     name "RIDE: " swap strcat " " strcat "_notonwho" getmsg
     strcat pronoun_sub .tell
;
: tellnotawake      (d -- )
     dup            (Tells taur player fell asleep)
     name "RIDE: " swap strcat " " strcat "_notawake" getmsg
     strcat pronoun_sub .tell
;
: tellnotatoldloc   (d -- )
     dup            (Tells taur that rider moved off)
     name "RIDE: " swap strcat " " strcat "_notatoldloc" getmsg
     strcat pronoun_sub .tell
;
: telllocked        (d -- )
     dup            (Tells taur that rider was locked out)
     name "RIDE: " swap strcat " " strcat "_locked" getmsg
     strcat pronoun_sub .tell
;
: tellridergone     (d -- )
     me @           (for pronounsub. Tells rider they moved with taur)
     me @ name " " strcat "_ridermsg" getmsg strcat pronoun_sub  (d s)
     notify
;
: tellnewroom       ( -- )
     me @           (for pronounsub. Tells room who did what with who)
     me @ name " " strcat "_newroom" getmsg strcat
     taurREF-list "%l" subst       (string, reflist, %l = string)
     pronoun_sub me @ location     (place)
     #0 rot notify_except
;
: telloldroom       ( -- )
     me @           (for pronounsub. Tells room who did what with who)
     me @ name " " strcat "_oldroom" getmsg strcat
     taurREF-list "%l" subst       (string, reflist, %l = string)
     pronoun_sub getoldloc #0 rot
     notify_except
;
: resetlookstat
     me @ "RIDE/lookstat" remove_prop
;
 
: setlookstat       ( -- set the lookstat prop for the Taur)
     me @ dup name " " strcat
     "_lstatTAUR" getmsg strcat
     taurREF-list "%l" subst pronoun_sub (setup by the dup)
     me @ "RIDE/lookstat" rot 0 addprop
;
 
: setriderlookstat (d -- sets the riders prop)
     dup            (d d)
     name " " strcat     (d s)
     "_lstatRIDER" getmsg strcat
     me @ name "%n" subst me @ swap pronoun_sub (d s)
     "RIDE/lookstat" swap 0 addprop
;
: yankrider    (d -- d')
     dup taurREF-next swap taurREF-delete
;
: resetrider   (d -- )
     dup
     "RIDE/onwho" "no_one" 0 addprop
     "RIDE/lookstat" remove_prop
;
: resettaur    ( -- )
     me @ "RIDE/tauring" "NO" 0 addprop
     resetlookstat
;
 
: allaboard
     BEGIN
          me @ "RIDE/reqlist" REF-first
          dup
          target ! porz? WHILE      ( ***** 3.7)
          target @ onwho?
          if
               target @ taurREF-add
          else
               "RIDE: "
               target @ name strcat 
               " did not accept your offer to be carried." strcat .tell
          then
          me @ "RIDE/reqlist" target @ REF-delete
     REPEAT
;
 
 
: moveriders        ( -- MAIN)
     SETATRIG
     allaboard
     taurREF-first
     dup 
     porz? not if    (***** 3.7)
          "RIDE: You are not carring anyone." .tell
          resettaur
          setoldloc
          EXIT
     then
     me @
     "RIDE/_ridercheck"
     getpropstr
     "YES"
     stringcmp not
     if        (ridercheck wanted!)
          listlocked?
          if        (opps.. someone is locked!)
               me @
               getoldloc
               MOVETO
"RIDE: One or more of your riders were locked out of your destination."
               .tell
               EXIT      (the whole shebang!)
          then
     then
 
 
     BEGIN                    (d)
          dup porz? WHILE       (***** 3.7)
          dup onwho? not if   (onwho? true if on you)
               dup  tellnotonwho   (d --     NOT on you)
               dup  yankrider      (d -- d'  pull from your list and get next)
               CONTINUE
          then
          dup OWNER awake? not if  (awake true if awake ***** 3.7)
               dup  tellnotawake   (d --     tells taur player not awake)
               dup  resetrider     (d --     Reset them)
               dup  yankrider      (d -- d'  pull from list)
               CONTINUE
          then
          dup atoldloc? not if     (atoldloc? true if at old location)
               dup  tellnotatoldloc     (d --     bailed out)
               dup  resetrider     (d --)
               dup  yankrider      (d -- d')
               CONTINUE
          then
          dup lockedout? if        (lockedout? true if Locked out)
               dup telllocked (d -- tell cant come with)
               dup resetrider      (d -- )
               dup yankrider       (d -- d')
               CONTINUE
          then
          (OK.... move them...)
          dup tellridergone        (d -- tells rider they moving)
          dup setriderlookstat     (D -- sets the riders lookstat)
          dup loc @ MOVETO         (ta da!)
               
          taurREF-next   (d -- d')
     REPEAT
     anyonehome?
     if
          tellnewroom
          telloldroom
          setlookstat
     else
          "RIDE: No one came with you!" .tell
          resettaur
     then
 
     setoldloc           (set location to here)
;
: MAINSWITCH
     me @ "RIDE/TAURING" getpropstr "YES" stringcmp
     if EXIT then
          MOVERIDERS
;
