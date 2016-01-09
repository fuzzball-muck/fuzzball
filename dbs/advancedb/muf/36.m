( cmd-banish.muf by Natasha@HLM
  Area banishing program like Riss's.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$include $lib/match
$include $lib/reflist
$include $lib/strings
 
$def prop_arrive "_arrive/banish"
$def prop_banish "_banish/banish"
$def prop_haven "_banish/haven"
$def prop_boot_msg "_banish/msg"
$def prop_boot_omsg "_banish/omsg"
$def str_boot_msg "The Great Furry Paw of Banishment[tm] sweeps down and drives you home."
$def str_boot_omsg "The Great Furry Paw of Banishment[tm] sweeps down and drives %N home."
 
: rtn-bootwhere  ( dbWho -- dbWhere )
    pop #-3
;
 
: rtn-bootfrom  ( dbWho dbRoom -- )
    over " " notify  ( dbWho dbRoom )
    dup prop_boot_msg getpropstr  ( dbWho dbRoom strMsg )
    dup not if pop str_boot_msg then  ( dbWho dbRoom strMsg )
    3 pick swap notify  ( dbWho dbRoom )
 
    dup prop_boot_omsg getpropstr  ( dbWho dbRoom strMsg )
    dup not if pop str_boot_omsg then  ( dbWho dbRoom strMsg )
    "%N" 4 pick name subst 3 pick swap pronoun_sub  ( dbWho dbRoom strMsg )
    3 pick dup location swap 1  ( dbWho dbRoom strMsg dbWhere dbWho intOne )
    4 rotate notify_exclude  ( dbWho dbRoom )
 
    pop  ( dbWho )
    dup rtn-bootwhere moveto  (  )
;
 
: rtn-banish  ( dbWho -- )
    dup "*{%d}*" fmtstring var! whopat  ( dbWho )
    dup location  ( dbWho dbRoom )
 
    ( Don't sweep from home. We can stop potential loops this way. )
    over getlink over dbcmp if pop pop exit then  ( dbWho dbRoom )
 
    ( Which environment? )
    dup 1 array_make swap  ( dbWho aryEnv dbRoom )
    begin dup #0 dbcmp not while  ( dbWho aryEnv dbRoom )
        location  ( dbWho aryEnv dbRoom' )
        dup rot array_appenditem swap  ( dbWho aryEnv dbRoom' )
    repeat pop  ( dbWho aryEnv )
 
    ( Banished from any? )
    dup prop_banish whopat @ array_filter_prop  ( dbWho aryEnv aryBanning )
    dup if  ( dbWho aryEnv aryBanning )
        swap pop  ( dbWho aryBanning )
        dup array_first pop array_getitem  ( dbWho dbBanning )
        rtn-bootfrom exit  (  )
    else pop then  ( dbWho aryEnv )
 
    ( Are any havens? )
    dup prop_haven "?*" array_filter_prop  ( dbWho aryEnv aryHavens )
    dup if  ( dbWho aryEnv aryHavens )
        ( Yup. Are we unbanished from any? )
        dup prop_haven whopat @ array_filter_prop swap array_diff  ( dbWho aryEnv aryBlockingHavens )
        dup if
            swap pop  ( dbWho aryBanning )
            dup array_first pop array_getitem  ( dbWho dbBanning )
            rtn-bootfrom exit  (  )
        else pop then  ( dbWho aryEnv )
    else pop then  ( dbWho aryEnv )
 
    pop pop  (  )
;
 
: rtn-banishlist  ( dbWhere -- )
    0 var! showed
    dup prop_banish REF-list  ( dbWhere strBanished )
    dup if
        over "Banished from %D: %s" fmtstring .tell  ( dbWhere )
        1 showed !
    else pop then  ( dbWhere )
    dup prop_haven REF-list  ( dbWhere strHavened )
    dup if
        over "Allowed in %D: %s" fmtstring .tell  ( dbWhere )
        1 showed !
    else pop then  ( dbWhere )
    showed @ not if "No one is banished from %D." fmtstring .tell else pop then  (  )
;
 
: rtn-cmd[ str:where str:who addr:what -- ]
    ( Banish from where? )
    where @ dup if atoi dbref else pop loc @ then  ( dbWhere )
 
    ( Can I@ banish there? )
    me @ over controls not if  ( dbWhere )
        "You can't banish people from %D." fmtstring .tell  (  )
        exit  (  )
    then  ( dbWhere )
 
    ( Banish whom? )
    who @ dup not if pop rtn-banishlist exit then  ( dbWhere strY )
    .noisy_pmatch dup ok? not if pop pop exit then  ( dbWhere dbWho )
 
    what @ execute  (  )
;
 
: rtn-isthere?  ( dbThere dbWho -- bool )
    location 1 begin while  ( dbThere db )
        over over dbcmp if pop pop 1 exit then  ( dbThere db )
    1 try location 1 catch 0 endcatch repeat  ( dbThere dbZero }  Go up rooms until we can no longer go up. )
    pop pop 0  ( bool )
;
 
: do-banish  ( dbWhere dbWho -- )
    over prop_banish 3 pick REF-add  ( dbWhere dbWho )
 
    ( Boot? )
    over over rtn-isthere? if dup rtn-banish then  ( dbWhere dbWho )
 
    "You banish %D from %D." fmtstring .tell  (  )
;
 
: do-unbanish  ( dbWhere dbWho -- )
    over prop_banish 3 pick REF-delete  ( dbWhere dbWho )
    "You unbanish %D from %D." fmtstring .tell  (  )
;
 
: do-haven  ( dbWhere dbWho -- )
    over prop_haven 3 pick REF-add  ( dbWhere dbWho )
    "You add %D to %D's haven list." fmtstring .tell  (  )
;
 
: do-unhaven  ( dbWhere dbWho -- )
    over prop_haven 3 pick REF-delete  ( dbWhere dbWho )
 
    ( Boot? )
    over over rtn-isthere? if dup rtn-banish then  ( dbWhere dbWho )
 
    "You remove %D from %D's haven list." fmtstring .tell  (  )
;
 
$define dict_commands {
    "banish" 'do-banish
    "unbanish" 'do-unbanish
    "haven" 'do-haven
    "unhaven" 'do-unhaven
}dict $enddef
 
: main  ( str -- )
    " banish unbanish haven unhaven " command @ tolower " %s " fmtstring instr not if pop me @ rtn-banish exit then  ( str )
 
    STRparse pop  ( strX strY )
    over "help" stringcmp not if pop pop .showhelp exit then  ( strX strY )
 
    dict_commands command @ tolower array_getitem  ( strX strY ? )
    dup address? if rtn-cmd exit then  ( strX strY ? )
    pop pop pop  (  )
 
    command @ "I don't know what '%s' means." fmtstring .tell
;
