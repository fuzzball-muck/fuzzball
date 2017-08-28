( lib-away.muf by Natasha@HLM
  A library for showing when and why players are away.
 
 
  away-cmd  { strY strZ -- }
  Performs an #away command, such as if the user entered '#away strY=strZ'.
 
  back-cmd  { strY strZ -- }
  Performs a #back command, such as if the user entered '#back strY=strZ',
  although the strings don't actually matter much.
 
  away-away  { db -- str }
  Returns db's away message if db is away, or the empty string if not.
 
  away-away?  { db -- bool }
  Returns true if db is away, or 0 if not.
 
  away-message  { db -- str }
  Returns db's current away message, whether or not db is away.
 
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters MUCK.
  "@view $box/mit" for license information.
 
  Version history
  1.001, 16 March 2003: don't use .ltimestr macro, use ltimestr in
    $lib/timestr. Put $pubdefs this program uses before it $includes
    itself.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.001
$lib-version 1.0
$note A library for showing when and why players are away.
$doccmd @list $lib/away=1-29
 
$include $lib/strings
$include $lib/timestr
$iflib $lib/alias $include $lib/alias $endif
$iflib $lib/wf $include $lib/wf $endif
 
$pubdef away-awayprop "_prefs/away/away"
$pubdef away-awayatprop "_prefs/away/awayat"
$pubdef away-msgprop "_prefs/away/message"
$pubdef away-msgpropdir "_prefs/away/msg/m%s"
 
$pubdef away-away? away-awayprop getpropval
$pubdef away-message away-msgprop getpropstr
$pubdef away-away away-away? if away-message else "" then
 
$include $lib/away
 
 
: do-set  ( strY strZ -- )
    me @ 3 pick away-msgpropdir fmtstring  ( strY strZ dbMe strProp )
    3 pick if  ( strY strZ dbMe strProp )
        3 pick setprop  ( strY strZ )
    else
        remove_prop  ( strY strZ )
    then  ( strY strZ )
 
    swap  ( strZ strY )
    dup if "Away message '%s' " else "%sDefault away message " then  ( strZ strY strYMsg )
    3 pick if "set to '%s'." else "%sremoved." then strcat  ( strZ strY strMsg )
    fmtstring .tell  (  )
;
 
 
: do-list  ( strY strZ -- )
    pop pop  (  )
    "" away-msgpropdir fmtstring dup strlen 1 - strcut pop  ( strMessagedir )
    me @ swap array_get_propvals  ( dictMessages )
 
    "Your away messages:" .tell
    foreach  ( strKey strValue )
        swap 1 strcut swap pop "%-10s: %s" fmtstring .tell  (  )
    repeat  (  )
    "Done." .tell
;
 
 
: do-away  ( strY strZ -- )
    ( Set a message too? )
    dup if  ( strY strZ )
        over swap do-set  ( strY )
    else pop then  ( strY )
 
    ( Do I have a preset for that? )
    me @ over away-msgpropdir fmtstring getpropstr  ( strY strMsg )
    dup if swap then pop  ( strMsg' }  If strMsg=="", this is really strY. )
 
$iflib $lib/wf
    dup if dup "went away (%s)" fmtstring else "went away" then  ( strMsg' strAnnc )
    me @ swap wf-announce  ( strMsg' )
$endif
 
    me @ away-msgprop 3 pick dup if  ( strMsg' dbMe strMsgprop strMsg' )
        setprop  ( stMsg' )
        "You're now away. Your message is '%s'." fmtstring  ( strMsg" )
    else
        pop remove_prop  ( strMsg' )
        pop "You're now away."  ( strMsg" )
    then  ( strMsg" )
    me @ away-awayprop 1 setprop  ( strMsg" )
    me @ away-awayatprop systime setprop  ( strMsg" )
    .tell  (  )
;
 
 
: do-back  ( strY strZ -- )
    pop pop  (  )
 
    me @ away-away? not if
        "You aren't away." .tell  (  )
        exit  (  )
    then  (  )
 
    me @ away-awayprop remove_prop  (  )
    me @ away-awayatprop getpropval  ( int )
    dup if
        "Welcome back! You were gone for "  ( int str )
        systime rot - ltimestr strcat "." strcat  ( str )
    else "Welcome back!" then .tell  (  )
 
$iflib $lib/wf
    me @ "came back" wf-announce
$endif
;
 
 
: rtn-see  ( arrDb -- )
    foreach swap pop  ( db )
        dup away-away? if  ( db )
            dup away-message dup not if  ( db strMsg )
                pop "" "%D has been away %s.%s"  ( db strMsg strFmt )
            else
                "%D has been away %s: %s"  ( db strMsg strFmt )
            then  ( db strMsg strFmt )
            rot swap  ( strMsg db strFmt )
            systime 3 pick away-awayatprop getpropval - mtimestr  ( strMsg db strFmt strTime )
            -3 rotate  ( strMsg strTime db strFmt )
            fmtstring .tell  (  )
        else
            dup awake? if "" else "asleep but " then  ( db str )
            swap "%D is %snot away." fmtstring .tell  (  )
        then  (  )
    repeat  (  )
;
: do-see  ( strY strZ -- )
    pop  ( strY )
$iflib $lib/alias
    me @ swap alias-expand  ( arrDb )
$else
    .noisy_pmatch dup if 1 array_make else pop exit then  ( arrDb )
$endif
    rtn-see  (  )
;
 
 
: do-help pop pop .showhelp ;
 
$define dict_commands {
    "away" 'do-away
    "back" 'do-back
    "set"  'do-set
    "list" 'do-list
    "help" 'do-help
    "see"  'do-see
}dict $enddef
 
: main  ( str -- )
    STRparse  ( strX strY strZ )
 
    rot dup not if  ( strY strZ strX )
        pop  ( strY strZ )
 
        ( It's not if there's a strZ {msgname=message}. )
        over over not and if  ( strY strZ )
$iflib $lib/alias
            ( Look for aliases. )
            me @ 3 pick alias-expand-quiet  ( strY strZ arrDb arrUnknown )
 
            ( Were there as many or more aliases recognized than not? )
            over array_count over array_count >= if  ( strY strZ arrDb arrUnknown )
                ( Yeah, there were. )
                dup if alias-tell-unknown else pop then  ( strY strZ arrDb )
                rtn-see  ( strY strZ )
                pop pop exit  (  )
            else pop pop then  ( strY strZ )
$else
            ( Is it a player name? )
            over pmatch dup if  ( strY strZ db )
                ( Yes, show. )
                1 array_make rtn-see  ( strY strZ )
                pop pop exit  (  )
            then pop  ( strY strZ )
$endif
        then  ( strY strZ )
        command @  ( strY strZ strCmd )
    then  ( strY strZ strCmd }  If strX is given, it's strCmd. )
 
    dict_commands over array_getitem dup address? if  ( strY strZ strCmd adr )
        swap pop execute  (  )
    else  ( strY strZ strCmd adr )
        pop command @ swap "I don't know what you mean by '%s'. Try '%s #help'." fmtstring .tell  ( strY strZ )
        pop pop  (  )
    then  (  )
;
 
PUBLIC do-away
PUBLIC do-back
$pubdef away-cmd "$lib/away" match "do-away" call
$pubdef back-cmd "$lib/away" match "do-back" call
