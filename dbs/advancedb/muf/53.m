( cmd-meetme.muf by Natasha@HLM
  A simple, free mjoin/msummon program for Fuzzball 6.
 
  Invitations are stored on the inviter. For example, if A msummons B without
  a prior invitation, A's _meetme/summon/#B property is set to the current
  time. When X mjoins Y, meetme will look for a prior invitation in Y's
  _meetme/join/#X property; if it's there, X will be moved to Y immediately.
  Otherwise notification takes place.
 
  There's currently no "meet" command to let the invited party choose whether
  to summon or join. Sorry.
 
  v1.1 7 July 2002: added $lib/ignore support.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for licensing information.
)
$include $lib/strings
$include $lib/match
$iflib $lib/ignore $include $lib/ignore $endif
$iflib $lib/alias  $include $lib/alias  $endif
 
$def str_program "meetme" (which $lib/ignore list)
$def prop_invites "_meetme/%s/%d"
$def int_expire 300
 
: rtn-invited  ( dbInviter dbInvited strMode -- bool }  Has dbInviter summoned dbInvited? )
    prop_invites fmtstring  ( dbInviter strProp )
    over over getpropval dup if 300 + systime > else pop 0 then  ( dbInviter strProp bool )
    dup -4 rotate if pop pop else remove_prop then  ( bool )
;
 
: rtn-meet  ( db dbMe strMode -- )
    "summon" strcmp if (So strMode is NOT summon, so dbMe is dbMove.) swap then  ( dbMove dbStill )
    location moveto  (  )
;
 
: rtn-join-summon  ( strDone strAsked strAsk strMode strNotMode db -- )
    ( Has db summoned me@? )
    me @  ( strDone strAsked strAsk strMode strNotMode db dbMe )
    rot prop_invites fmtstring  ( strDone strAsked strAsk strMode db strProp )
    over over getpropval  ( strDone strAsked strAsk strMode db strProp intWhen )
    3 pick rot remove_prop  ( strDone strAsked strAsk strMode db intWhen )
    systime int_expire - < if  ( strDone strAsked strAsk strMode db )
        ( No. I'm asking to join db. )
 
        ( Is db ignoring me@? )
        dup me @ str_program ignores? if  ( strDone strAsked strAsk strMode db )
            "%D is ignoring you." fmtstring .tell  ( strDone strAsked strAsk strMode )
            pop pop pop pop exit  (  )
        then  ( strDone strAsked strAsk strMode db )
        ( Am I@ ignoring db? )
        me @ over str_program ignores? if  ( strDone strAsked strAsk strMode db )
            "You are ignoring %D." fmtstring .tell  ( strDone strAsked strAsk strMode )
            pop pop pop pop exit  (  )
        then  ( strDone strAsked strAsk strMode db )
 
        ( Record. )
        me @ over  ( strDone strAsked strAsk strMode db dbMe db )
        4 rotate prop_invites fmtstring  ( strDone strAsked strAsk db dbMe strProp )
        systime setprop  ( strDone strAsked strAsk db )
 
        ( Notify. )
        dup  ( strDone strAsked strAsk db db )
        rot fmtstring .tell  ( strDone strAsked db )
        me @ dup dup  ( strDone strAsked db dbMe dbMe dbMe )
        5 rotate fmtstring pronoun_sub  ( strDone db strMsg )
        over swap notify  ( strDone db )
 
        swap pop  ( db )
    else  ( strDone strAsked strAsk strMode db )
        ( Yes. )
        dup me @  ( strDone strAsked strAsk db db dbMe )
        4 rotate rtn-meet  ( strDone strAsked strAsk db )
        -4 rotate pop pop  ( db strDone )
        .tell  ( db )
    then  ( db )
    pop  (  )
;
 
: do-join  ( db -- )
    "Joined." "%D has asked to join you. Type 'msummon %D' to summon %%o." "You ask to join %D." "join" "summon" 6 rotate rtn-join-summon  (  )
;
: do-summon  ( db -- )
    "Summoned." "%D has asked you to join %%o. Type 'mjoin %D' to do so." "You ask %D to join you." "summon" "join" 6 rotate rtn-join-summon  (  )
;
 
: rtn-cancel-decline  ( strOsucc strSucc strNoInvite strMode db -- )
    me @ over  ( strOsucc strSucc strNoInvite strMode db dbMe db )
    4 rotate "cancel" stringcmp if (I@ am declining, so db is the inviter.) swap then  ( strOsucc strSucc strNoInvite db dbInviter dbInvited )
    over over "join" rtn-invited not if  ( strOsucc strSucc strNoInvite db dbInviter dbInvited )
        over over "summon" rtn-invited not if  ( strOsucc strSucc strNoInvite db dbInviter dbInvited )
            pop pop  ( strOsucc strSucc strNoInvite db )
            swap  ( strOsucc strSucc db strNoInvite )
            fmtstring .tell  ( strOsucc strSucc )
            pop pop exit  (  )
        else "join" "summon" then  ( strOsucc strSucc strNoInvite db dbInviter dbInvited strNotJs strJs )
    else "summon" "join" then  ( strOsucc strSucc strNoInvite db dbInviter dbInvited strNotJs strJs )
    6 rotate pop  ( strOsucc strSucc db dbInviter dbInvited strNotJs strJs )
 
    4 rotate 4 rotate rot  ( strOsucc strSucc db strNotJs dbInviter dbInvited strJs )
    prop_invites fmtstring  ( strOsucc strSucc db strNotJs dbInviter strProp )
    remove_prop  ( strOsucc strSucc db strNotJs )
 
    over over over  ( strOsucc strSucc db strNotJs db strNotJs db )
    6 rotate  ( strOsucc db strNotJs db strNotJs db strSucc )
    fmtstring pronoun_sub  ( strOsucc db strNotJs strMsg )
    .tell  ( strOsucc db strNotJs )
 
    me @ swap over  ( strOsucc db dbMe strNotJs dbMe )
    5 rotate  ( db dbMe strNotJs dbMe strOsucc )
    fmtstring pronoun_sub  ( db strMsg )
    notify  (  )
;
: do-cancel  ( db -- )
    "%D cancels %%p invitation to %s %%o." "You cancel your invitation for %D to %s you." "You have no outstanding invitation to %D to cancel." "cancel" 5 rotate  ( strNoInvite strMode db )
    rtn-cancel-decline
;
: do-decline  ( db -- )
    "%D declines your invitation to %s you." "You decline %D's invitation to %s %%o." "%D has tendered no invitation to decline." "decline" 5 rotate  ( strNoInvite strMode db )
    rtn-cancel-decline
;
 
: do-help pop .showhelp ;
 
: do-ignore   str_program cmd-ignore-add ;
: do-unignore str_program cmd-ignore-del ;
 
$define dict_commands {
    "meetme"   "help"
    ("meet"     'do-meet)
    "mjoin"    'do-join
    "msummon"  'do-summon
    "mdecline" 'do-decline
    "mcancel"  'do-cancel
}dict $enddef
 
$define dict_hashcmds {
    "help"     'do-help
    "ignore"   'do-ignore
    "unignore" 'do-unignore
    "!ignore"  'do-unignore
}dict $enddef
 
: main  ( str -- )
    ( What? )
    STRparse pop  ( strX strY )
    over if  ( strX strY )
        swap dict_hashcmds over array_getitem  ( strY strX ? )
        dup address? if  ( strY strX adr )
            swap pop execute  (  )
            exit  (  )
        then  ( strY strX ? )
        pop  ( strY strX )
        "I don't know what you mean by '#%s'." fmtstring .tell  ( strY )
        pop exit  (  )
    then  ( strX strY )
    swap pop  ( strArg )
    command @ tolower  ( strArg strCmd )
 
    dict_commands over array_getitem  ( strArg strCmd ? )
    dup address? not if  ( strArg strCmd ? )
        pop pop pop .showhelp exit  (  )
    then  ( strArg strCmd adr )
    swap pop  ( strArg adr )
 
    ( All other commands take a player argument, so who is strArg? )
$iflib $lib/alias
    me @ rot alias-expand  ( adr arrDb )
    foreach swap pop  ( adr db )
        over execute  ( adr )
    repeat  ( adr )
    pop  (  )
$else
    swap .noisy_pmatch dup ok? not if pop pop exit then  ( adr dbArg )
    swap  ( dbArg adr )
    execute  (  )
$endif
;
