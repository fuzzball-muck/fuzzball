@program terraform-mover
1 19999 d
i
( TerraForm - planet generation system  )
( Terrain generator... room maintenence )
( By Triggur of Brazilian Dreams        )
( {C} 1994 by Kevin Kelm, pferd@netcom.com )
( All rights reserved. )
( DO MODIFY WITHOUT EXPLICIT PERMISSION! )
 
( V1.00 - Original )
( V1.01 - Added code to pull terrain types from map hierarchies )
( V1.02 - Added code to autosave/restore exits )
( V1.03 - Fixed wrap at north and south of world )
( V1.04 - Converted 2D -> 3D, oceans, skies, space )
( ---- RELEASED ---- )
( V1.06 - Diddled the order of looking for player's location )
( V1.08 - '!' terrain type now can use local/global fail/ofail set with
          @editterrain )
( V1.09 - added support for 'inside' rooms and building )
( V1.10 - added support for terrain library rooms )
( V1.11 - made the room destroyer sleep for 1 second before starting so that
          Riss's RIDE and other programs can finish what they need to do
          before the room goes poof ) 
( V1.12 - modified: now only player positions are tracked with @/occupants ) 
( V1.15 - added on-the-spot sanity checking to make sure that the dbref
          stored in the @/rooms propdir is valid.  It might NOT be valid,
          for instance, if someone has @rec'd the room in question.  Note
          that if it has since become ANOTHER spot on the planet, this will
          NOT be detected... hence weirdness could result.  But at least it
          doesn't crash. )
( V1.16 - added skylinks )
( ------ RELEASE V1.18 ------ )
( V1.19 - reduced the time delay in the destructor to 0 )
( V1.20 - Juggled code around to handle destruction of rooms in light of the
          fact that forced objects not set Z can use exits, but don't trigger
          _depart.  Added code to end of program to ensure destruction when
          a terraform exit is used to get out of a world and/or when the
          _depart is called.  It's STILL possible for rooms to be left when
          an X,!Z object teleports out of the world using non-TF exits tho.
          This is not currently a solvable problem. )
    
$define PARSELIMIT 50 $enddef  ( max # of substitutions in text generator ) 
 
$define DEBUGON prog "d" set $enddef 
$define DEBUGOFF prog "!d" set $enddef 
 
lvar env
lvar cx
lvar cy
lvar cz
lvar lx
lvar ly
lvar lz
lvar lastroom
lvar nextroom
lvar terrain
lvar altitude
 
lvar val1
lvar val2
lvar val3
lvar val4
lvar val5
 
lvar temp
lvar cttstop
lvar x1
lvar y1
lvar x2
lvar y2
lvar mapdbref
lvar mapwide
lvar maptall
lvar terrains 
 
: calculate_sky_link ( -- bool ) 
( skylink entries are in the format:  x1 y1 x2 y2 dbref )
( where x1,y1 = upper left corner, x2,y2=lower right corner, )
( dbref = dbref of the primary map room of the destination world.)
( NOTE: You MUST store the lower-resolution )
( maps at the bottom of the list and the higher resolution maps at )
( the top! The dbref MUST be a room and this room MUST be )
( a world's primary environment room! )
  env @ "./skymaps#" getpropstr atoi cttstop !
  1 temp !
  begin
    temp @ cttstop @ > if ( out of maps )
      break
    then
    env @ "./skymaps#/" temp @ intostr strcat getpropstr
    dup ":" instr dup if 1 - strcut pop else pop then
    strip " " explode pop
    atoi x1 !
    atoi y1 !
    atoi x2 !
    atoi y2 !
    atoi dbref mapdbref !
 
    cx @ x1 @ < not cx @ x2 @ > not and   ( is current x value in range? )
    cy @ y1 @ < not cy @ y2 @ > not and   ( is current y value in range? )
    and if  ( match! )
( get fractions into this rectangle and translate to fractions across other
  world, flipped horizontally. )
      mapdbref @ "./maxx" getpropstr atoi mapwide !
      mapdbref @ "./maxy" getpropstr atoi maptall !
      mapdbref @ "./maxz" getpropstr atoi
       cz @ env @ "./maxz" getpropstr atoi - 1 - - cz ! ( move into other sky )
      cz @ 0 < if
        0 cz !
      then 
      x1 @ x2 @ = if   ( map into destination planet width {flipped} )
        0 cx !
      else
        mapwide @ cx @ x1 @ - mapwide @ * x2 @ x1 @ - 1 + / - cx !
      then
      y1 @ y2 @ = if   ( map into destination planet 'height' )
        0 cy !
      else
        cy @ y1 @ - 1 + maptall @ * y2 @ y1 @ - 1 + / cy !
      then
      1 exit
    then
   
    temp @ 1 + temp !
  repeat
DEBUGOFF
  0 exit   ( no error message if we can't find one... just fail. )
;
 
: calculate_terrain_type ( -- bool ) 
( map entries are in the format:  x1 y1 x2 y2 dbref )
( where x1,y1 = upper left corner, x2,y2=lower right corner, )
( dbref = dbref that contains mapping data.  It is broken up )
( like this so that Revar's caching system can still do its  )
( job & save memory.  NOTE: You MUST store the lower-resolution )
( maps at the bottom of the list and the higher resolution maps at )
( the top! The dbref MUST be a room and this room MUST have )
( the world's primary environment room as a parent or ancestor! )
 
  env @ "./maps#" getpropstr atoi cttstop !
  1 temp !
  begin
    temp @ cttstop @ > if ( out of maps )
      break
    then
    env @ "./maps#/" temp @ intostr strcat getpropstr
    dup ":" instr dup if 1 - strcut pop else pop then
    strip " " explode pop
    atoi x1 !
    atoi y1 !
    atoi x2 !
    atoi y2 !
    atoi dbref mapdbref !
    mapdbref @ "./terrains" getpropstr dup if
      atoi dbref terrains !
    else
      pop mapdbref @ terrains !
    then
 
    cx @ x1 @ < not cx @ x2 @ > not and   ( is current x value in range? )
    cy @ y1 @ < not cy @ y2 @ > not and   ( is current y value in range? )
    and if  ( possible match! )
      mapdbref @ "./map#" getpropstr atoi 1 + maptall ! ( get map dimensions )
      mapdbref @ "./map#/1" getpropstr strlen 1 + mapwide !
      cy @ y1 @ - maptall @ 1 - * y2 @ y1 @ - 1 + / 1 + ( get map line )
      mapdbref @ swap "./map#/" swap intostr strcat getpropstr
      cx @ x1 @ - mapwide @ 1 - * x2 @ x1 @ - 1 + /  ( get map column )
      strcut swap pop 1 strcut pop      ( get the map character! )
      dup terrains @ swap "./" swap strcat "/name" strcat getpropstr if
        terrain ! 1 exit                ( terrain type known ... done! )
      then
    then
   
    temp @ 1 + temp !
  repeat
( TODO: ENHANCE THIS ERROR TO PRINT LOCATION )
  me @ "ERROR: Could not calculate terrain type.  Notify " env @ owner name
  strcat " immediately!" strcat notify
  0 exit
;
 
: gettoken ( s -- s1 s2 token )
  dup "%" instring dup not if  ( none left... return )
    pop "" exit
  then
 
  1 - strcut 1 strcut swap pop  ( snip off prefix, % )
  dup "%" instring 1 - strcut 1 strcut ( snip off suffix, % )
  swap pop swap ( reverse )
  val1 @ 137 + temp !  ( rotate to next string number )
  val2 @ val1 !
  val3 @ val2 !
  val4 @ val3 !
  val5 @ val4 !
  temp @ val5 !
  dup not if  ( null token )
    pop strcat gettoken  ( put prefix, suffix back together and try again )
  then
  exit
;
  
lvar propnum
lvar propname
lvar limit
 
: parsetext ( s -- s )
( description parsing tokens are stored on the maproom in a wizpropsubdir )
( with the same letter as the terrain type for that location.  Example: )
( for the terrain type 'j' and the token 'roomnames', the propdir name )
( should be  @/j/roomnames )
 
  PARSELIMIT limit !
  begin
    limit @ not if  (exceeded max count)
      exit
    then
    gettoken dup not if ( no more tokens... return )
      pop break
    then
    "./" swap strcat propname !
    mapdbref @ propname @ "#" strcat getpropstr atoi val1 @ swap % 1 + propnum !
    mapdbref @ propname @ "#/" propnum @ intostr strcat strcat
    getpropstr ( get the line )
    swap strcat strcat ( join with suffix and prefix )
 
    limit @ 1 - limit !
  repeat
  exit
; 
 
lvar count
: itemcount ( -- i )
  0 count !
  lastroom @ contents
  begin
    dup #-1 dbcmp if
      pop break
    then
    count @ 1 + count !
    next
  repeat
  count @
  exit
;
 
lvar numexits
lvar exitdir
lvar exitnum
 
: addexits ( d -- )
  "@/exits/" cx @ intostr strcat "/" strcat cy @ intostr strcat "/" strcat
   cz @ intostr strcat "#" strcat
   exitdir !
   env @ exitdir @ getpropstr atoi dup numexits !
   exitdir @ "/" strcat exitdir !
   begin
     numexits @ not if ( no more exits to add )
       break 
     then
 
     env @ exitdir @ numexits @ intostr strcat getpropstr ( get the raw data )
( string format: names:link:succ:osucc:fail:ofail:drop:odrop:lock:desc ) 
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch names )
     dup ":" instr 1 - strcut 1 strcut swap pop swap atoi dbref swap ( link )
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch succ )
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch osucc )
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch fail )
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch ofail )
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch drop )
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch odrop )
     dup ":" instr 1 - strcut 1 strcut swap pop  ( fetch lock )
( desc is left )
     10 rotate nextroom @ swap newexit exitnum ! ( create exit )
     exitnum @ int 1 < if
       me @ "ERROR: COULD NOT CREATE EXIT.  PLEASE NOTIFY " env @ owner name
       strcat "!!!" strcat notify
     then
     exitnum @ mapdbref @ owner setown ( chown to owner of region )
     exitnum @ swap setdesc
     exitnum @ swap setlockstr pop
     exitnum @ swap setodrop
     exitnum @ swap setdrop
     exitnum @ swap setofail
     exitnum @ swap setfail
     exitnum @ swap setosucc
     exitnum @ swap setsucc
     exitnum @ swap setlink
 
     numexits @ 1 - numexits !
  repeat
  exit
;
 
lvar exprop
lvar exnum
lvar exstr
 
: save_exits ( -- )
  "@/exits/" cx @ intostr strcat "/" strcat cy @ intostr strcat
  "/" strcat cz @ intostr strcat
  "#" strcat exprop !
 
  env @ exprop @ remove_prop
  1 exnum !
  lastroom @ exits
  begin
    dup #-1 dbcmp if
      break
    then
    dup name ":" strcat exstr ! 
    dup getlink exstr @ swap int intostr strcat ":" strcat exstr !
    dup succ exstr @ swap strcat ":" strcat exstr !
    dup osucc exstr @ swap strcat ":" strcat exstr !
    dup fail exstr @ swap strcat ":" strcat exstr !
    dup ofail exstr @ swap strcat ":" strcat exstr !
    dup drop exstr @ swap strcat ":" strcat exstr !
    dup odrop exstr @ swap strcat ":" strcat exstr !
    dup getlockstr dup "*UNLOCKED*" stringcmp not if
      pop ""
    then
    exstr @ swap strcat ":" strcat exstr !
    dup desc exstr @ swap strcat exstr !
    env @ exprop @ "/" strcat exnum @ intostr strcat exstr @ 0 addprop
 
    exnum @ 1 + exnum !
    next
  repeat
  exnum @ 1 = not if
    env @ exprop @ exnum @ 1 - intostr 0 addprop
  then
  exit
;
  
lvar tx
lvar ty
lvar tz
 
: main ( s -- )
  "Depart" stringcmp not if   ( clean up behind player )
 
( WARNING: THE FOLLOWING LINE _MAY_ POSE SOME PROBLEMS IN FILLING UP THE
  TIME QUEUE.  IF THIS HAPPENS, COMMENT IT OUT, BUT BE AWARE THAT IT WILL
  BREAK RISS' RIDE PROGRAM AND ANY OTHER PROGRAM THAT JUST _EXPECTS_ THE
  ROOM THAT WAS JUST LEFT TO STILL EXIST :)
    0 sleep 
    preempt  ( we do NOT want to be interrupted in here )
    loc @ ok? not if  ( doublecall, V1.0 leftover )
      exit
    then
    loc @ lastroom !
    lastroom @ "~x" getpropstr not if  ( not really a generated room )
      exit
    then
    lastroom @ "@/env" getpropstr atoi dbref env !
    lastroom @ location me @ location location dbcmp not if ( exited world )
      env @ "@/occupants/" me @ int intostr strcat remove_prop
    then
    itemcount not lastroom @ ".noflush" getpropstr not and if  ( zap it )
      lastroom @ "~x" getpropstr atoi cx !
      lastroom @ "~y" getpropstr atoi cy !
      lastroom @ "~z" getpropstr atoi cz !
      save_exits
      env @ "@/rooms/" cx @ intostr strcat "/" strcat
      cy @ intostr strcat "/" strcat cz @ intostr strcat remove_prop
      lastroom @ recycle
    then
    exit
  then
 
  preempt  ( we do NOT want to be interrupted in here )
( get this player's current location, if any )
  trigger @ "@/env" getpropstr atoi dbref env !
  env @ #0 dbcmp if
    me @ "ERROR: EXIT #" trigger @ int intostr strcat
    " DOES NOT HAVE 'ENV' PROP SET TO ENV ROOM!!!" strcat notify
    exit
  then
 
  0 cx ! 0 cy ! 0 cz !
 
( load location from location if possible, else bump )
   me @ location  "~x" getpropstr atoi cx !
   me @ location  "~y" getpropstr atoi cy !
   me @ location  "~z" getpropstr atoi cz !
 
  cx @ 0 = cy @ 0 = cz @ 0 = and and if  ( probably teleported in.  naughty. )
    env @ "@/occupants/" me @ int intostr strcat getpropstr
    dup if "," explode pop else pop "" "" "" then atoi cx ! atoi cy ! atoi cz !
  then  
  
( get the dbref of the [existing] room at that location )
  env @ "@/rooms/" cx @ intostr strcat "/" strcat cy @ intostr strcat
  "/" strcat cz @ intostr strcat getpropstr atoi dbref lastroom !
 
  cx @ lx !
  cy @ ly !
  cz @ lz !
 
  cx @ 0 = cy @ 0 = cz @ 0 = and and if  ( entering the world fresh )
    trigger @ "@/x" getpropstr atoi cx !
    trigger @ "@/y" getpropstr atoi cy !
    trigger @ "@/z" getpropstr atoi cz !
  else
    trigger @ "@/dx" getpropstr atoi cx @ + cx !
    trigger @ "@/dy" getpropstr atoi cy @ + cy !
    trigger @ "@/dz" getpropstr atoi cz @ + cz !
  then
 
  cx @ lx @ = cy @ ly @ = cz @ lz @ = and and if ( get absolute )
    trigger @ "@/x" getpropstr atoi cx !
    trigger @ "@/y" getpropstr atoi cy !
    trigger @ "@/z" getpropstr atoi cz !
  then
 
  cx @ 1 < if  ( wrap around the world )
    cx @ env @ "./maxx" getpropstr atoi + cx !
  then
  cx @ env @ "./maxx" getpropstr atoi > if
    cx @ env @ "./maxx" getpropstr atoi - cx !
  then
  cy @ 1 < if  ( wrap north... complex )
    1 cy @ - cy ! ( move back )
    cx @ env @ "./maxx" getpropstr atoi 2 / +
    env @ "./maxx" getpropstr atoi % 
    dup not if pop env @ "./maxx" getpropstr atoi then cx !
  then
  cy @ env @ "./maxy" getpropstr atoi > if  ( wrap south... complex )
    env @ "./maxy" getpropstr atoi 2 * cy @ - cy ! ( move back )
    cx @ env @ "./maxx" getpropstr atoi 2 / +
    env @ "./maxx" getpropstr atoi %
    dup not if pop env @ "./maxx" getpropstr atoi then cx !
  then
 
( calculate description string numbers )
  cx @ cy @ + dup 0 < if 0 swap - then val1 !
  cy @ cx @ - dup 0 < if 0 swap - then val2 !
  cx @ cy @ * cz @ + dup 0 < if 0 swap - then val3 !
  cx @ 7 * cy @ cz @ + + dup 0 < if 0 swap - then val4 !
  cx @ cy @ 3 * cz @ - + dup 0 < if 0 swap - then val5 !
 
( get the dbref of the new room )
  env @ "@/rooms/" cx @ intostr strcat "/" strcat cy @ intostr strcat
  "/" strcat cz @ intostr strcat getpropstr atoi dbref nextroom !
 
( sanity check: Is the stored dbref valid? ) 
  nextroom @ dup #0 dbcmp not if
     dup room? not if        ( the old room reference is bad )
      pop #0 nextroom !
    else dup "@/env" getpropstr atoi dbref env @ dbcmp not if ( ref is bad )
      pop #0 nextroom !
    else   ( ref is ok, use it )
      pop
    then then 
  then   
 
( if it doesnt exist yet, create it, else get maproom/terrain/etc )
  nextroom @ #0 dbcmp not if    ( hunt down the mapdbref/terrains )
    nextroom @ location mapdbref !
    mapdbref @ "./terrains" getpropstr dup if
      atoi dbref terrains !
    else
      pop mapdbref @ terrains !
    then
  else
    calculate_terrain_type not if  ( failed )
      exit
    then
 
 loc @ ".inside" getpropstr not loc @ owner me @ dbcmp not or if
( if @inside and this player owns this room, can go any dir, else check )
( see if they can move in that direction )
    terrain @ "!" stringcmp not if  ( boundary character... fail )
      me @ terrains @ "./!/fail" getpropstr dup not if
        env @ fail dup not if
          "You can't go that way."
        else
          pop
        then
      else
        pop
      then
      notify   ( deliver fail )
      loc @ me @ me @ name " " strcat
      terrains @ "./!/ofail" getpropstr dup not if
        env @ ofail dup not if
          exit
        then
      else
        pop
      then
      notify_except
      exit
    then
 
( see if they're trying to go too high )
    env @ "./maxz" getpropstr atoi cz @ < if  ( yup )
      calculate_sky_link not if
        me @ env @ fail notify
        exit
      then
      me @ "You cross over into the gravitational well of " mapdbref @ name
      strcat "." strcat notify
( we're about to warp over to another planet... start the engines! :)
      trigger @ "@/env" getpropstr temp !  (save old props if any )
      trigger @ "@/x" getpropstr lx !
      trigger @ "@/y" getpropstr ly !
      trigger @ "@/z" getpropstr lz !
      trigger @ "@/dx" getpropstr tx !
      trigger @ "@/dy" getpropstr ty !
      trigger @ "@/dz" getpropstr tz !
    
      trigger @ "@/env" mapdbref @ intostr 0 addprop (set new props)
      trigger @ "@/x" cx @ intostr 0 addprop
      trigger @ "@/y" cy @ intostr 0 addprop
      trigger @ "@/z" cz @ intostr 0 addprop
      trigger @ "@/dx" remove_prop
      trigger @ "@/dy" remove_prop
      trigger @ "@/dz" remove_prop
      prog "" swap call        ( call ourselves for new world )
      temp @ if                 ( restore old props if any )
        trigger @ "@/env" temp @ 0 addprop
      else
        trigger @ "@/env" remove_prop
      then
      tx @ if
        trigger @ "@/dx" tx @ 0 addprop
      then
      ty @ if
        trigger @ "@/dy" ty @ 0 addprop
      then
      tz @ if
        trigger @ "@/dz" tz @ 0 addprop
      then
      lx @ if
        trigger @ "@/x" lx @ 0 addprop
      else
        trigger @ "@/x" remove_prop
      then
      ly @ if
        trigger @ "@/y" ly @ 0 addprop
      else
        trigger @ "@/y" remove_prop
      then
      lz @ if
        trigger @ "@/z" lz @ 0 addprop
      else
        trigger @ "@/z" remove_prop
      then
      me @ location loc @ dbcmp not if  ( remove player from this tfbd )
        env @ "@/occupants/" me @ intostr strcat remove_prop
      then
DEBUGOFF
      exit   ( halt processing in THIS world... player's gone. )
    then
    cz @ 0 > if
      env @ "./flyprop" getpropstr dup if
        me @ swap getpropstr not if  ( player doesnt have prop! )
          me @ env @ fail notify
          exit
        then
    else pop then
    then
 
( see if they're trying to go too low )
    mapdbref @ "./" terrain @ strcat "/minz" strcat getpropstr atoi cz @ > if
      me @ env @ fail notify
      exit
    then
    cz @ 0 < if
      mapdbref @ "./" terrain @ strcat "/underprop" strcat getpropstr dup if
        me @ swap getpropstr not if  ( player doesnt have prop! )
          me @ env @ fail notify
          exit
        then
      else pop then
    then
  then
 
( the new room's name and desc prototypes come from the maproom itself )
( UNLESS there is a redirection prop. )
    terrains @
    cz @ not if  ( get name ) 
      terrains @ "./" terrain @ strcat "/name" strcat getpropstr
    else cz @ 0 > if  ( get a sky name )
      env @ "./skymap#" getpropstr atoi temp !
      begin
        temp @ not if break then
        env @ "./skymap#/" temp @ intostr strcat getpropstr " " explode pop
        atoi cz @ > not swap atoi cz @ < not and if  ( in range! )
          temp @ altitude !
          env @ "./skyname/" temp @ intostr strcat getpropstr
          break
        then
        temp @ 1 - temp !
      repeat
    else  ( get 'under' name )
      terrains @ "./" terrain @ strcat "/undername" strcat getpropstr
    then then
 
    parsetext dup not if pop "a room" then newroom nextroom !
    nextroom @ int 1 < if  ( failed )
      me @ "ERROR: Could not create room!  Please notify " env @ owner name
      strcat "." strcat notify
      exit
    then 
 
    loc @ ".inside" getpropstr cz @ 0 = not and if
      nextroom @ ".inside" "y" 0 addprop
      nextroom @ ".noflush" "y" 0 addprop
    then
    nextroom @ mapdbref @ owner setown  ( chown the room to owner of the map )
    nextroom @
    cz @ not if  ( get desc ) 
      terrains @ "./" terrain @ strcat "/desc" strcat getpropstr
    else cz @ 0 > if  ( get a sky desc )
      env @ "./skydesc/" altitude @ intostr strcat getpropstr
    else  ( get 'under' desc )
      terrains @ "./" terrain @ strcat "/underdesc" strcat getpropstr
    then then
    parsetext setdesc ( describe the room )
    nextroom @ "~x" cx @ intostr 0 addprop ( tell the room its location )
    nextroom @ "~y" cy @ intostr 0 addprop
    nextroom @ "~z" cz @ intostr 0 addprop
    nextroom @ "~listen" env @ "./_listen" getpropstr 0 addprop
    env @ "@/rooms/" cx @ intostr strcat "/" strcat cy @ intostr strcat
    "/" strcat cz @ intostr strcat 
    nextroom @ env @ succ setsucc
    nextroom @ "~terrain" terrain @ 0 addprop (tell room its terrain )
    nextroom @ "@/env" env @ int intostr 0 addprop  (tell room its env)
    nextroom @ int intostr 0 addprop ( tell envroom about the new room )
 
    nextroom @ addexits
  then
 
 ( move the player to the next room, update envroom props )
  cz @ not lz @ not and loc @ ".inside" getpropstr or if ( use terrestrial verb )
    me @ trigger @ "succ" getpropstr ( print succ )
    terrains @ "./" loc @ "~terrain" getpropstr strcat "/verb" strcat
    getpropstr "%v" subst me @ swap pronoun_sub notify
 
     loc @ me @ trigger @ "osucc" getpropstr ( print osucc )
    terrains @ "./" loc @ "~terrain" getpropstr strcat "/overb" strcat
    getpropstr "%v" subst me @ name " " strcat swap strcat
    me @ swap pronoun_sub notify_except
 
    nextroom @ me @ trigger @ "odrop" getpropstr ( print odrop ) 
    terrains @ "./" loc @ "~terrain" getpropstr strcat "/overb" strcat
    getpropstr "%v" subst me @ name " " strcat swap strcat
    me @ swap pronoun_sub notify_except
  else cz @ 0 < lz @ 0 < or if ( use under verb )
    me @ trigger @ "succ" getpropstr ( print succ )
    terrains @ "./" loc @ "~terrain" getpropstr strcat "/underverb" strcat
    getpropstr "%v" subst me @ swap pronoun_sub notify
 
    loc @ me @ trigger @ "osucc" getpropstr ( print osucc )
    terrains @ "./" loc @ "~terrain" getpropstr strcat "/underoverb" strcat
    getpropstr "%v" subst me @ name " " strcat swap strcat
    me @ swap pronoun_sub notify_except
 
    nextroom @ me @ trigger @ "odrop" getpropstr ( print odrop ) 
    terrains @ "./" loc @ "~terrain" getpropstr strcat "/underoverb"
    strcat getpropstr "%v" subst me @ name " " strcat swap strcat
    me @ swap pronoun_sub notify_except
  else  ( use "fly/flies" verb )
    me @ trigger @ "succ" getpropstr ( print succ )
    "fly" "%v" subst me @ swap pronoun_sub notify
 
    loc @ me @ trigger @ "osucc" getpropstr ( print osucc )
    "flies" "%v" subst me @ name " " strcat swap strcat    
    me @ swap pronoun_sub notify_except
 
    nextroom @ me @ trigger @ "odrop" getpropstr ( print odrop )
    "flies" "%v" subst me @ name " " strcat swap strcat    
    me @ swap pronoun_sub notify_except
  then then
 
  me @ nextroom @ moveto
 
  me @ player? if 
    env @ "@/occupants/" me @ int intostr strcat cx @ intostr "," strcat
    cy @ intostr strcat "," strcat cz @ intostr strcat 0 addprop
  then
 
( how ugly.  since objects set X and !Z can use exits but dont trigger _depart,
  we have to call the destructor code here explicitly. :P )
  "Depart" main 
  exit
;
.
c
q
