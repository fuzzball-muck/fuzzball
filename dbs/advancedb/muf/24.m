( lib-alias by Natasha@HLM
  Library for general #alias feature for multiple programs.
 
  alias-expand  { dbPlayer strNames -- arrDb }
  Expands the given space-delimited list of names into an array of players'
  dbrefs, expanding aliases where necessary. Unrecognized names are displayed
  to the user.
 
  alias-expand-quiet  { dbPlayer strNames -- arrDb arrUnknown }
  As alias-expand, but unrecognized names are returned as an array of strings
  {arrUnknown}.
 
  alias-tell-unknown  { arrUnknown -- }
  Tell the user the strings in arrUnknown are unrecognized aliases. You should
  probably only use this on an array you got from alias-expand-quiet.
 
  alias-get  { db str -- str' }
  Expands the given db's given alias. Does not expand global aliases.
 
  cmd-alias  { strY strZ [db] -- }
  Decodes command arguments strY and strZ as from $lib/strings' STRparse,
  setting the alias strY to the names in strZ. If db is given, aliases
  are stored and read from that object; otherwise, "me @" is used. If
  both strY and strZ are null, lists all the object's aliases. If strY
  is null but not strZ, lists all the object's aliases that contain strY.
 
  alias-global-registry  { -- db }
  Returns the dbref of the global alias registry, suitable for use with
  alias-get and cmd-alias.
 
  pmatch { str -- db }  or  .pmatch { str -- db }
  Match the given string as a name or alias, returning the first resulting
  player. The alias may still expand to more than one player, but only the
  first will be returned.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
 
  Version history:
  1.0: cleaned up for submission to Akari's repository
  1.1, 27 March 2003: Added pmatch and .pmatch aliases. Don't keep
    expanding recursive aliases. Don't barf on empty strings.
 
  Lib-version history:
  1.0: original
  1.1, 27 March 2003: added pmatch and .pmatch
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.1
$lib-version 1.1
$note Library for general #alias feature for multiple programs.
$doccmd @list $lib/alias=1-47
 
$include $lib/strings
$pubdef prop_aliases "_prefs/alias/a%s"
$pubdef alias-get prop_aliases fmtstring getpropstr
$include $lib/alias
 
$def prop_aliasesdir "_prefs/alias/"
$def prop_globalowner "_aliasowner/%s"
 
: rtn-expand-quiet  ( dbPlayer strAlias -- arrDb arrUnknown }  Given a player and a space-delimited string of names and aliases, return an array of the referred players' dbrefs and an array of the unrecognized names. )
    dup not if pop pop 0 array_make 0 array_make exit then  ( dbPlayer strAlias )
    0 array_make var! unknownNames  ( dbPlayer strAlias )
    " " explode_array  ( dbPlayer arrAliases )
    0 array_make swap  ( dbPlayer arrDb arrAliases )
    6 var! recurses  ( dbPlayer arrDb arrAliases )
    begin dup recurses @ and recurses -- while  ( dbPlayer arrDb arrAliases )
        0 array_make swap  ( dbPlayer arrDb arrAliases' arrAliases )
        foreach swap pop  ( dbPlayer arrDb arrAliases' strAlias )
            ( Empty string? )
            dup not if pop continue then  ( dbPlayer arrDb arrAliases' strAlias )
 
            ( Player name? )
            dup \pmatch dup ok? if  ( dbPlayer arrDb arrAliases' strAlias db )
                swap pop  ( dbPlayer arrDb arrAliases' db )
                rot array_appenditem  ( dbPlayer arrAliases' arrDb )
                swap continue  ( dbPlayer arrDb arrAliases' )
            then pop  ( dbPlayer arrDb arrAliases' strAlias )
 
            ( Alias? )
            4 pick over prop_aliases fmtstring getpropstr  ( dbPlayer arrDb arrAliases' strAlias strAlias' )
            dup not if pop alias-global-registry over prop_aliases fmtstring getpropstr then  ( dbPlayer arrDb arrAliases' strAlias strAlias' )
            dup if  ( dbPlayer arrDb arrAliases' strAlias strAlias' )
                swap pop  ( dbPlayer arrDb arrAliases' strAlias' )
                " " explode_array array_union  ( dbPlayer arrDb arrAliases' )
                continue
            then pop  ( dbPlayer arrDb arrAliases' strAlias )
 
            ( Partial player name? )
            dup part_pmatch dup ok? if  ( dbPlayer arrDb arrAliases' strAlias db )
                swap pop rot array_appenditem swap continue  ( dbPlayer arrDb arrAliases' )
            then pop  ( dbPlayer arrDb arrAliases' strAlias )
 
            ( No recognizable name. )
            unknownNames @ array_appenditem unknownNames !  ( dbPlayer arrDb arrAliases' )
        repeat  ( dbPlayer arrDb arrAliases' )
    repeat pop swap pop  ( arrDb )
    unknownNames @  ( arrDb arrUnknown )
;
 
: rtn-tell-unknown  ( arrUnknown -- )
    dup if
        dup array_last pop array_cut array_vals pop  ( arrUnknown strLastName )
        over if  ( arrUnknown strLastName )
            swap ", " array_join  ( strLastName strNames )
            "I don't recognize the names: %s and %s." fmtstring .tell  (  )
        else
            "I don't recognize the name '%s'." fmtstring .tell  ( arrEmpty )
            pop  (  )
        then  (  )
    else pop then  (  )
;
 
: rtn-expand rtn-expand-quiet rtn-tell-unknown ;
 
: cmd-set  ( strY strZ [db] -- )
    dup dbref? not if me @ then var! proploc
 
    over not if  ( strY strZ )
        swap pop  ( strZ )
        dup if  ( strZ )
            dup "Aliases containing '%s':" fmtstring .tell  ( strZ )
            tolower  ( strZ )
            proploc @ prop_aliasesdir array_get_propvals foreach  ( strZ strProp strValue )
                dup tolower 4 pick instr if  ( strZ strProp strValue )
                    swap 1 strcut swap pop "    %s: %s" fmtstring .tell  ( strZ )
                else pop pop then  ( strZ )
            repeat pop  (  )
        else
            pop "Aliases:" .tell  (  )
            proploc @ prop_aliasesdir array_get_propvals foreach  ( strProp strValue )
                swap 1 strcut swap pop "    %s: %s" fmtstring .tell  (  )
            repeat  (  )
        then
        "Done." .tell exit  (  )
    then  ( strY strZ )
 
    ( Global? )
    proploc @ alias-global-registry dbcmp  ( strY strZ boolGlobal )
    dup var! globalp  ( strY strZ boolGlobal )
    if  ( strY strZ )
        ( Yes; do I@ have permission to do that? )
        alias-global-registry 3 pick prop_globalowner fmtstring getprop  ( strY strZ ?Owner )
        dup dbref? if me @ dbcmp else pop 1 then  ( strY strZ boolControls }  If no owner, everyone controls it. )
        me @ "wizard" flag? or not if  ( strY strZ )
            pop "You don't control the global alias '%s'." fmtstring .tell  (  )
            exit  (  )
        then  ( strY strZ )
 
    then  ( strY strZ )
 
    proploc @ 3 pick prop_aliases fmtstring rot  ( strY dbMe strProp strZ )
    dup if
        setprop  ( strY )
        "Alias '%s' set."  ( strY strMsg )
        globalp @ if  ( strY strMsg )
            alias-global-registry 3 pick prop_globalowner fmtstring me @ setprop  ( strY strMsg )
        then  ( strY strMsg )
    else
        pop remove_prop
        "Alias '%s' cleared."
        globalp @ if  ( strY strMsg )
            alias-global-registry 3 pick prop_globalowner fmtstring remove_prop  ( strY strMsg )
        then  ( strY strMsg )
    then  ( strY strMsg )
    fmtstring .tell  (  )
;
 
: do-help pop pop .showhelp ;
: do-global alias-global-registry cmd-set ;
 
$def dict_commands { "" 'cmd-set "global" 'do-global "help" 'do-help }dict
 
: main ( str -- )
    STRparse  ( strX strY strZ )
 
    rot dict_commands over array_getitem dup address? if  ( strY strZ strX ? )
        swap pop execute  (  )
    else  ( strY strZ strX ? )
        pop "I don't know what you mean by '#%s'." .tell  ( strY strZ )
        pop pop  (  )
    then  (  )
;
PUBLIC rtn-expand
PUBLIC rtn-expand-quiet
PUBLIC rtn-tell-unknown
PUBLIC cmd-set
 
$pubdef ref_lib_alias "$lib/alias" match
$pubdef alias-expand ref_lib_alias "rtn-expand" call
$pubdef alias-expand-quiet ref_lib_alias "rtn-expand-quiet" call
$pubdef alias-tell-unknown ref_lib_alias "rtn-tell-unknown" call
$pubdef cmd-alias ref_lib_alias "cmd-set" call
$pubdef pmatch me @ swap alias-expand-quiet pop dup if 0 array_getitem else pop #-1 then
$pubdef .pmatch pmatch
