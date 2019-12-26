( hand.muf -- by Wog
  This is my take on a hand proggie. Link an action called
  'hand;qhand' to this program.
 
--- Change History ----------------------------------
v 1.0  02/24/00
    Assignment of version number to programs.
v 1.1hlm  13 June 2002  Natasha@HLM
    Default everyone to handable.
v 1.2hlm  27 June 2002  Natasha@HLM
    @conlock yourself against receiving objects.
v 1.3hlm  20 October 2002  Natasha@HLM
    Match more sanely with $lib/match.
    Honor global ignores.
v 1.4hlm  1 December 2002  Natasha@HLM
    Messages include object's name.
v 1.5hlm  26 January 2003  Natasha@HLM
    Try getting messages from object first.
    Let throw throw to someone not in the room, with attendant message handling.
    Use names, not %Ns in parse.
v 1.6 25 December 2019 HopeIslandCoder@HIM
    Fix blank hand messages if default props are not set.
--- Distrubution Information ------------------------
Copyright {C} Charles "Wog" Reiss <car@cs.brown.edu>
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or {at your option} any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
For a copy of the GPL:
     a> see: https://www.gnu.org/copyleft/gpl.html
     b> write to: the Free Software Foundation, Inc., 
         59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
)
$include $lib/strings
$include $lib/match  (Match better 20 Oct 2002 Natasha@HLM)
$def handOkProp "_hand/hand_ok"
 
var object
var target
var throw?
var quiet?
var verb
 
: getmessage ( s -- str ) ( Check for objects' messages too
                            Natasha@HLM 26 January 2003

                            And handle empty prop case -HopeIslandCoder
                           )
    command @ "_prefs/hand/%s/%s" fmtstring var! prop
    
    object @ prop @ getpropstr dup if exit then pop
    me @     prop @ getpropstr dup if exit then pop
    prog     prop @ getpropstr  ( str )
    
    ( Do a reasonable default if there's no string at this point )
    dup strlen not if
      pop
      "[m]N " command @ strcat "s [o] to [t]N." strcat
    then
    
    dup "[m]N" instring not if "[m]N " swap strcat then  ( Prepend thrower's name if not present. )
    "%" "[t]" subst target @ name "%N" subst target @ swap pronoun_sub  ( Replace %N with real name Natasha@HLM 26 January 2003 )
    "%" "[m]" subst me @     name "%N" subst me @     swap pronoun_sub  ( Replace %N with real name Natasha@HLM 26 January 2003 )
    object @ name "[o]" subst
;
 
: handOk? ( p -- i )
    handOkProp getpropstr .no? not  ( Inverse permissions }  Natasha@HLM 13 June 2002 )
;
 
: checkPrems ( -- 1 / 0 )
    me @ handOk? not if 
        "You are set to not be able to hand/be handed things." .tell
        "To allow yourself to hand/be handed things type 'hand #ok'" .tell
        0 exit
    then
 
    object @ thing? object @ program? or not if  ( Added check for thingness }  Natasha@HLM 1 December 2002 )
        object @ "%D isn't a thing you can hand someone." fmtstring .tell 0 exit  (  )
    then  (  )
    object @ location me @ dbcmp not if
        object @ "You must be holding the %D to hand it to someone." fmtstring .tell 0 exit  ( Changed msg to include object name }  Natasha@HLM 1 December 2002 )
    then 
    target @ location me @ location dbcmp not throw? @ if not then if  ( Target must be here iff not throwing Natasha@HLM 26 January 2003 )
        target @ throw? @ if "%D is already here." else "%D isn't here to whom to hand anything." then fmtstring .tell 0 exit  ( Changed msg to include target name Natasha@HLM 1 December 2002; special message if throwing, since this block has different meaning Natasha@HLM 26 January 2003 )
    then
    target @ player? target @ "Z" flag? 
    (target @ "_puppet?" getpropstr .yes? or) or not if  ( Removed check for _puppet? as we don't use it Natasha@HLM 3 Jan 2004 )
        target @ "%D isn't something to which you can hand anything." fmtstring .tell 0 exit  ( Changed msg to include target name }  Natasha@HLM 1 December 2002 )
    then
    target @ handOk? not if
        target @ "%D isn't accepting handed things from anyone." fmtstring .tell  ( Changed msg to include target name }  Natasha@HLM 1 December 2002 )
        0 exit
    then
 
    ( Check conlock. }  Natasha@HLM 27 June 2002 )
    target @  ( db )
    dup player? not if dup owner 2 else 1 then  ( dbN..db1 intN }  Check zombie owner's too Natasha@HLM 3 Jan 2004 )
    array_make foreach swap pop  ( db )
        "_/clk" getprop dup lock? if  ( lock )
            me @ swap testlock not if  (  )
                target @ "%D isn't accepting handed things from you." fmtstring .tell  ( Changed msg to include target name }  Natasha@HLM 1 December 2002 )
                0 exit
            then  (  )
        else pop then  (  )
    repeat  (  )
    ( Check global ignore. }  Natasha@HLM 20 October 2002 )
    me @ target @ ignoring? if
        target @ "You are ignoring %D." fmtstring .tell
        0 exit
    then
 
    1
;
 
: doHand ( -- )
    command @ "throw" instr throw? ! ( Throwing something? Natasha@HLM 26 January 2003 )
    checkPrems not if exit then 
    throw? @ if "throw" else "hand" then verb !
    command @ "q" instring quiet? !
 
    ( User's message. )
    throw? @ quiet? @ or if
        "from" getmessage
        throw? @ quiet? @ not and if .alltell else .tell then
    then
    ( Recipient's message. )
    "to" getmessage
    quiet? @ if
        target @ swap notify
    else
        target @ location 0 rot notify_exclude
    then
 
    object @ target @ moveto
;
 
: goHandOk ( -- )
    me @ handOkProp remove_prop  ( Inverse permissions }  Natasha@HLM 13 June 2002 )
;
 
: goNotHandOk ( -- )
    me @ handOkProp "no" setprop  ( Inverse permissions }  Natasha@HLM 13 June 2002 )
;
 
: doHelp ( -- )
    { "hand [by Wog]"
    "-------------------------------------------"
    "Options: "
    "  hand #ok"
    "           Allow people to hand things to you."
    "  hand #!ok"
    "           Disallow people to hand things to you."
    "  hand <object> to <player>"
    "           Hands <object> to <player>."
    "  qhand <object> to <player>"
    "           Quietly hands <object> to <player>."
    "  hand #props"
    "           Properties used by hand."
    }list foreach swap pop .tell repeat
;
 
: doProps ( -- )
    { "hand [by Wog]"
    "------------------------------------------------------------"
    "Properties: "
    "[on you] [optional, if not present defaults used]"
    "  _hand/format   -- what you see when you hand something"
    "                     to someone"
    "  _hand/oformat  -- what every else sees on a non-quiet hand"
    "  _hand/qformat  -- what you see on a quiet hand"
    "  _hand/qoformat -- what the reciever of the object sees on a quiet hand."
    "---------------------------------------------------------------------"
    "[o] will be replaced with the name of the object being handed."
    "[m]<letter> will run pronoun subs you for the person who hands"
    "   the object, like %<letter>..."
    "[t]<letter> will run pronoun subs for the person who recieves"
    "   the object, like %<letter>..."
    "(type 'man pronoun_sub' for information on pronoun subsitutions.)"
    "Do note that omessages will be prefixed with the name of the handing"
    "  player."
    "----------------------------------------------------------------------"
    "Example: setting the _hand/oformat prop to "
    " 'hands [o] to [t]N with [m]p greatest care.'"
    "would show everyone: "
    "%N hands Object to Wog with %p greatest care." me @ swap pronoun_sub
    "if you were handing 'Object' to Wog."
    "----------------------------------------------------------------------"
    }list foreach swap pop .tell repeat
;
 
: main ( -- )
    dup "#he" stringpfx if pop doHelp exit then
    dup "#pr" stringpfx if pop doProps exit then
    dup "#ok" stringpfx if pop goHandOk 
        "You can now hand/be handed things." .tell exit
    then
    dup "#!o" stringpfx if pop goNotHandOk 
        "You can now not hand/be handed things." .tell exit
    then
    dup " to " instr not if "What? Type 'hand #help' for help."
        .tell exit
    then
    " to " .split
    (object name, target name)
    dup match  ( strObject strTarget dbTarget }  Begin hand to zombies Natasha@HLM 3 Jan 2004 )
    dup ok? if dup thing? over "z" flag? and else 0 then if  ( strObject strTarget dbTarget )
        swap pop  ( strObject dbTarget )
    else  ( strObject strTarget dbTarget )
        pop .noisy_pmatch
    then  ( strObject dbTarget )
    target !  (Begin match better 20 Oct 2002 Natasha@HLM)
    .noisy_match object !
    object @ ok? not if exit then
    target @ ok? not if exit then  (End match better 20 Oct 2002 Natasha@HLM)
    doHand
;
