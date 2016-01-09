(RIDE frontend  Ver 3.7FB by Riss)
(Props used:)
(RIDE/ontaur        - REF-list of dbref of riders [taur])
(RIDE/reqlist       - REF-list of dbrefs that cam come [taur])
(RIDE/tauring       - Flag to enable _arrive engine [taur])
(RIDE/onwho         - dbref of carrier [rider])
 
var taur
var rider
var namelist
var mode
var mess
 
$define globalprop prog $enddef
$include $lib/reflist    (the reflist routines)
 
: tellhelp
"RIDE 3.7FB by Riss" .tell
"Command                 Function" .tell
"Handup | carry <name>   Enables you to carry the named character." .tell
"Hopon | ride <name>     Accepts the offer to be carried by <name>." .tell
"Hopoff | dismount       Leave the ride." .tell
"Dropoff | doff <name>   Drop the named player from your ride." .tell
"Carrywho | cwho         Shows who is being carried by you." .tell
"Ridewho | rwho          Shows who you are riding." .tell
"Rideend | rend          Disables riding and cleans up." .tell
" " .tell
"Example: Riss wants to carry Lynx. Riss would: HANDUP LYNX. This would" .tell
"notify Lynx that Riss offers to carry him. He can accept the offer" .tell
"with: HOPON RISS. When Riss moves to another location, Lynx will move" .tell
"with him. Lynx can leave the ride at any time, cleanly by a: HOPOFF," .tell
"or simply by moving away from Riss by using any normal exit." .tell
"RIDE does check Locks on exits passed through, and will not allow" .tell
"someone who is locked out from entering." .tell
" " .tell
"Page #mail Riss with comments and such." .tell
" Enter RIDE #HELP1 for other setup information!" .tell
;
 
: help1
"RIDE Custom setups - RIDE can be made to display custom messages for" .tell
" most functions. You can set your own custom messages in your RIDE/" .tell
" props directory. You may have as many different modes of messages" .tell
" as you like. Set each group in a different sub directory." .tell
"MESSAGE PROP NAMES: ('taur' refers to carrier, 'rider' to rider.)" .tell
"_HANDUP:      Message taur sees when using handup command." .tell
"_OOHANDUP:    Message rider sees when taur offers to carry." .tell
 
"_OHANDUP:     Message the rest of the room sees." .tell
"_HOPON:       Message the rider sees when hopping on." .tell
"_OOHOPON:     Message the taur sees confirming the rider hopped on." .tell
"_OHOPON:      What the rest of the room sees." .tell
"_XHOPON:      The fail message to rider when they cant get on the taur." .tell
"_OOXHOPON:    The fail message to the taur." .tell
"_OXHOPON:     What the rest of the room sees." .tell
"_HOPOFF:      Message to the rider when they hopoff." .tell
"_OOHOPOFF:    Message to the taur when the rider hops off." .tell
"_OHOPOFF:     What the rest of the room sees." .tell
"_DROPOFF:     Message to the taur when they drop a rider." .tell
"_OODROPOFF:   Message to the rider when they are dropped by the taur." .tell
"_ODROPOFF:    Ditto the rest of the room." .tell
"Enter RIDE #HELP2 for next screen." .tell
;
: help2
"In all the messages, %n will substitute to the taur's name, along with" .tell
" the gender substitutions %o, %p, and %s. The substitution for the" .tell
" rider's name is %l. Any message prop beginning with _O will have the" .tell
" name of the actioner appended to the front automatically." .tell
"You create the messages in a subdirectory of RIDE/ named for the mode" .tell
" you want to call it. Examples:" .tell
"@set me=ride/carry/_handup:You offer to carry %l." .tell
"@set me=ride/carry/_oohandup:offers to carry you in %p hand." .tell
"@set me=ride/carry/_ohandup:offers to carry %l in %p hand." .tell
"And so on.. You would then set your RIDE/_MODE prop to CARRY to use" .tell
" the messages from that directory. @set me=ride/_mode:carry " .tell
"If you do not provide messages, or a _mode, or a bad directory name" .tell
" for _mode, then the default RIDE messages will be used." .tell
"There are 4 build in modes. RIDE, HAND, WALK, and FLY." .tell
" RIDE is the default if your mode is not set, and is used for riding" .tell
" on ones back. HAND is holding hands to show around. WALK is just" .tell
"walking with, and FLY is used for flying type messages. Feel free to" .tell
"use these, or customize your own." .tell
"------------" .tell
"There's more! Those are just the messages for the actions, there are" .tell
" also the messages for the movements themselves!..." .tell
"Enter RIDE #HELP3 for those." .tell
;
: help3
"Messages used by the RIDE engine - Different substitutions apply here." .tell
"_RIDERMSG - What the rider sees when moved. Taur's name prepended and" .tell
" pronoun subs refer to the taur." .tell
"_NEWROOM - Tells the room entered by the taur and riders what's going" .tell
" on. %l is the list of riders. Taur's name prepended w/ pronoun subs." .tell
"_OLDROOM - Tells the room just left. Like _NEWROOM." .tell
"There is no specific message for the taur, as you see the _NEWROOM" .tell
" message. There are other error messages, they begin with RIDE: and" .tell
" are not alterable. Set the props in the subdirectory you named in" .tell
" your _MODE property." .tell
" -------" .tell
"One last prop... RIDE does check exits passed through for locks against" .tell
" your riders. If they are locked out, they fall off. If in your RIDE/" .tell
" directory, you @set me=ride/_ridercheck:YES" .tell
" then your riders will be checked just after you move to a new place," .tell
" and if any are locked out, you will automatically be yoyo'ed back" .tell
" to the place you just left, and get a warning message." .tell
" ------- " .tell
"Enter RIDE #help4 for more version change info." .tell
;
: help4
"Version 3.5 ---" .tell
"Worked on the lock checking routines to help them work with folks using" .tell
"the Driveto.muf program for transportation. Should work now. And beefed" .tell
"lock checking up some. You may now set 2 new properties on ROOMS:" .tell
"_yesride:yes will allow riders in to a room NO MATTER IF OTHER LOCKS" .tell
"TRY TO KEEP THEM OUT." .tell
"_noride:yes will lock out all riders from a room." .tell
"Version 3.6 ---" .tell
"Minor Lock change... You can now carry riders in to a room if you own" .tell
" it, even if you passed through an exit to which the riders are locked" .tell
" out. This allows you to not have to unlock exits to say, your front" .tell
" door, every time you want to carry someone in. This also bypasses _noride"
.tell
"LookStatus Added - A way to display your RIDE status when someone looks" .tell
" at you. Add this to the end of your @6800 desc: %sub[RIDE/LOOKSTAT]" .tell
"The property RIDE/LOOKSTAT will be set on you to display either who" .tell
" you are carrying or who you are riding. This message comes from:" .tell
"_LSTATTAUR: is carrying %l with %o.    <- for the 'taur  -or-" .tell
"_LSTATRIDER: is being carryed by %n.   <- for the rider." .tell
"These props can be set in the same _mode directory with the other" .tell
" custom messages. Pronoun subs refer to the taur, and name is prepended." .tell
" %l is the list of riders. %n is the taur's name." .tell
"RIDE/LOOKSTAT will _NOT_ be set until after the first move by the 'taur" .tell
"If RIDE/LOOKSTAT gets stuck showing something wrong, do a RIDEEND." .tell
"Ver 3.7 -Zombies should work." .tell
;
: PorZ?   (d -- b   Returns True if Player or Zombie  ***** 3.7)
dup player? if pop 1 exit then
dup thing? if "Z" flag? exit then
pop 0
;
 
: issamestr?   (s s -- i  is same string?)
     stringcmp not
;
: setoldloc         ( -- set taur location to here)
     me @ "RIDE/oldloc" me @ location intostr 0 addprop
;
 
: getmsg  (s -- s')
     mess ! taur @ "RIDE/_mode" getpropstr mode ! taur @
     "RIDE/" mode @ strcat "/" strcat mess @ strcat
     getpropstr
     dup not if     (no good, try global)
          pop globalprop
          "RIDE/" mode @ strcat "/" strcat mess @ strcat
          getpropstr
          dup not if     (again no good)
               pop globalprop mess @ over "RIDE/_mode" getpropstr "RIDE/%s/%s" fmtstring getpropstr
          then
     then
;
 
 
: makesubs     (s -- s)
     rider @ name "%l" subst  (s)
     taur @ name "%n" subst
     taur @ swap pronoun_sub  (s)
;
: telltaur     (s)
     taur @ swap notify
;
: tellrider    (s)
     rider @ swap notify
;
: tellroom     (s)
     loc @ taur @ rider @ 2 5 pick notify_exclude
;
: checkin ( --    **** 3.5)
     prog "RIDE/_check/" rider @ intostr strcat
     taur @ intostr 0 addprop
;
: checkout ( --   **** 3.5)
     prog
     "RIDE/_check/" rider @ intostr strcat
     remove_prop
;
 
: handup  (USED BY TAUR - takes the param as the name of the player)
     me @ taur !
     dup not if tellhelp EXIT then
     match     (playername to dbref)
     dup porz? not  (is not a player here?)
                    (***** 3.7)
     if
          "RIDE: That is not a character here." .tell
          exit
     then
     dup rider !    (save it the rider dbref in here)
     dup me @ dbcmp
     if
          "RIDE: You want to ride yourself? Kinda silly no?" .tell
          exit
     then
     me @ "RIDE/reqlist" rot REF-add
     me @ "RIDE/tauring" "YES" 0 addprop
     "_HANDUP" getmsg makesubs telltaur
     "_OOHANDUP" getmsg "%n " swap strcat makesubs tellrider
     "_OHANDUP" getmsg "%n " swap strcat makesubs tellroom
     setoldloc      (init this)
;
: hopon   (RUN BY RIDER)
     me @ rider ! dup not
     if tellhelp EXIT then
 
     match
     dup porz? not
          (***** 3.7) 
     if
          "RIDE: That is not a character here." .tell
          exit
     then
     dup
     taur !    (Is the taur looking to carry you?)
     "RIDE/reqlist" me @ REF-inlist?
     if        (YES.. ok)
          me @           (set our ridingon prop)
          "RIDE/onwho" taur @ intostr 1 addprop
          "_HOPON" getmsg makesubs tellrider
          "_OOHOPON" getmsg "%l " swap strcat makesubs telltaur
          "_OHOPON" getmsg "%l " swap strcat makesubs tellroom
          CHECKIN
     else
          "_XHOPON" getmsg makesubs tellrider
          "_OOXHOPON" getmsg "%l " swap strcat makesubs telltaur
          "_OXHOPON" getmsg "%l " swap strcat makesubs tellroom
     then
;
: hopoff        (run by rider, does not take a param)
     me @ dup rider ! "RIDE/onwho" getpropstr     (are we on someone?)
     atoi dbref dup taur !         (save it here)
     porz?                           (***** 3.7)
     if
          taur @ "RIDE/ontaur" me @ REF-inlist?
          if        (YES.. ok)
               "_HOPOFF" getmsg makesubs tellrider
               "_OOHOPOFF" getmsg "%l " swap strcat makesubs telltaur
               "_OHOPOFF" getmsg "%l " swap strcat makesubs tellroom
          else
               "RIDE: You decide not to go." .tell
          then
     else
          "RIDE: Already off." .tell
     then
     me @ "RIDE/onwho" "0" 1 addprop
     me @ "RIDE/lookstat" remove_prop
     CHECKOUT
;
: carrywho     (run by taur.. shows the REF-list)
     "RIDE: You carry: " namelist !
     me @ "RIDE/ontaur" REF-first
     BEGIN
          dup porz? WHILE                         (***** 3.7)
          dup name " " strcat
          namelist @ swap strcat namelist !
          me @ "RIDE/ontaur" rot
          REF-next
     REPEAT
     pop
     me @
     "RIDE/reqlist"
     REF-first                (d)
     BEGIN
          dup porz? WHILE     (d       ****** 3.7)
          dup "RIDE/onwho" getpropstr atoi dbref  (d d')
          me @ dbcmp          (d b)
          if                  (d)
               dup name " " strcat
               namelist @ swap strcat namelist !
          then
          me @ "RIDE/reqlist" rot
          REF-next
     REPEAT
     pop
     namelist @
     .tell
;
: ridewho      (run by rider)
     me @
     "RIDE/onwho"
     getpropstr     (are we on someone?)
     atoi dbref dup taur !         (save it here)
     porz?                       (***** 3.7)
     if
          "RIDE: You are being carried by "
          taur @ name strcat "." strcat .tell
     else
          "RIDE: You are not being carried." .tell
     then
;
: rideend           (run by taur.  clean up and stop.)
(need onwho check and rider cleanup)
     me @           (pull list)
     "RIDE/ontaur" remove_prop
     me @ "RIDE/reqlist" remove_prop
     me @           (flag off)
     "RIDE/tauring" "NO" 0 addprop
     me @ "RIDE/lookstat" remove_prop
     "RIDE: Ride over." .tell
;
     
: dropoff (USED BY TAUR - takes the param as the name of the player)
 
     me @ taur !    (im the taur)
 
     dup not if tellhelp EXIT then
     match     (playername to dbref)
     dup porz? not  (is not a player here?)
               (***** 3.7)
     if
          "RIDE: That is not a character here." .tell
          exit
     then
     rider !   (save it the rider dbref in here)
 
     taur @ "RIDE/ontaur" rider @ REF-inlist?
     taur @ "RIDE/reqlist" rider @ REF-inlist?
     or
     if rider @ "RIDE/onwho" getpropstr atoi dbref taur @ dbcmp
          if rider @ "RIDE/onwho" "0" 1 addprop
               rider @ "RIDE/lookstat" remove_prop
               CHECKOUT
               taur @ "RIDE/ontaur" rider @ REF-delete
     "_DROPOFF" getmsg makesubs telltaur
     "_OODROPOFF" getmsg "%n " swap strcat makesubs tellrider
     "_ODROPOFF" getmsg "%n " swap strcat makesubs tellroom
          else
               "RIDE: That player is not set to you." .tell
          then
     else
          "RIDE: That player is not in your carry list." .tell
     then
     taur @ "RIDE/reqlist" rider @ REF-delete
     
;    
 
: ridecom           (MAIN)
     strip          (clean the param if any)
     dup "#help" issamestr? if tellhelp exit then
     dup "#help1" issamestr? if help1 exit then
     dup "#help2" issamestr? if help2 exit then
     dup "#help3" issamestr? if help3 exit then
     dup "#help4" issamestr? if help4 exit then
 
     command @      (get the command that started this mess....)
     dup "handup" issamestr? if pop handup exit then
     dup "carry" issamestr? if pop handup exit then
     dup "hopon" issamestr? if pop hopon exit then
     dup "ride" issamestr? if pop hopon exit then
     dup "hopoff" issamestr? if hopoff exit then
     dup "dismount" issamestr? if hopoff exit then
     dup "carrywho" issamestr? if carrywho exit then
     dup "cwho" issamestr? if carrywho exit then
     dup "ridewho" issamestr? if ridewho exit then
     dup "rwho" issamestr? if ridewho exit then
     dup "rideend" issamestr? if rideend exit then
     dup "rend" issamestr? if rideend exit then
     dup "dropoff" issamestr? if pop dropoff exit then
     dup "doff" issamestr? if pop dropoff exit then
     "RIDE: HUH?" 
     .tell     (should never get here)
;
