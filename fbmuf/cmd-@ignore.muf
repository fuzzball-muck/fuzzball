@program cmd-@ignore.muf
1 9999 d
i
( cmd-@ignore 10 Sep 2002 by Steve <muf@quantumrain.com> )
 
$author Steve <muf@quantumrain.com>
$version 1.0
 
( -- Options -- )
 
( Change this line to whatever you use to determine if someone is a guest )
 
$def Guest? ( d -- i ) name "Guest" stringpfx
 
( -- No more options -- )
 
: StripDuplicates[ str:Line str:ToRemove -- str:Line ]
    ToRemove @ ToRemove @ strcat var! ToRemoveTwice
 
    Line @ begin dup ToRemoveTwice @ instr while ToRemove @ ToRemoveTwice @ subst repeat
;
 
: StrWnk[ str:Line -- str:Line ]
    Line @ 1 strcut tolower swap toupper swap strcat
;
 
: Array_Commas[ arr:Array str:And -- str:Text ]
    Array @ array_count 2 > if
        Array @ array_vals array_make array_reverse dup 0 []
        swap 0 array_delitem array_reverse
        ", " array_join " " And @ " " strcat strcat strcat swap strcat
    else
        Array @ " " And @ " " strcat strcat array_join
    then
;
 
: Array_Quote[ arr:Array -- arr:Array ]
    Array @ dictionary? if
        { Array @ foreach "\"%~\"" fmtstring repeat }dict
    else
        { Array @ foreach "\"%~\"" fmtstring swap pop repeat }list
    then
;
 
: Array_StrWnk[ arr:Array -- arr:Array ]
    Array @ dictionary? if
        { Array @ foreach "%~" fmtstring StrWnk repeat }dict
    else
        { Array @ foreach "%~" fmtstring StrWnk swap pop repeat }list
    then
;
 
: Array_Filter[ arr:Array any:Context addr:Func -- arr:Accept arr:Reject ]
    { }dict var! Accept
    { }dict var! Reject
 
    Array @ foreach
        over over Context @ Func @ execute if
            Accept @ rot ->[] Accept !
        else
            Reject @ rot ->[] Reject !
        then
    repeat
 
    Accept @ Reject @
;
 
: AppInfoMsgArray[ arr:Array str:Msg -- ]
    Array @ if
        { {
            Array @ array_count 1 = if
                    "is"    "is"
                    "isn't"    "isn't"
                    "s"        ""
                    "es"    ""
                    "ies"    ""
                else
                    "is"    "are"
                    "isn't"    "aren't"
                    "s"        "s"
                    "es"    "es"
                    "ies"    "ies"
            then
 
            "array"        Array @ array_strwnk array_quote dup "and" array_commas
            "arrayor"    rot "or" array_commas
        }dict }list
 
        Msg @ array_fmtstrings me @ 1 array_make array_notify
    then
;
 
: AppParsePlayersSub[ str:Text any:Context -- any:Data ]
    Text @ pmatch dup not if pop Text @ part_pmatch then
;
 
: AppParseAddRemoveList[ str:Args any:Context addr:MatchFunc -- list:Add list:Remove list:Unknown ]
    { }list var! Add
    { }list var! Remove
    { }list var! Unknown
 
    Args @ strip dup if
        " " StripDuplicates " " explode_array foreach swap pop
            dup "!" stringpfx if 1 strcut swap pop 1 else 0 then var! Removing? var! Text
 
            Text @ Context @ MatchFunc @ execute dup if
                Removing? @ if
                    Remove @ array_appenditem Remove !
                else
                    Add @ array_appenditem Add !
                then
            else pop Text @ Unknown @ array_appenditem Unknown ! then
        repeat
    else pop then
 
    Add @ Remove @ Unknown @
;
 
: AppParseAddRemovePlayers[ str:Args -- list:Add list:Remove ]
    Args @ 0 'AppParsePlayersSub AppParseAddRemoveList
    "I don't recognise the player%[s]s named %[array]s." AppInfoMsgArray
;
 
: DisplayIgnores[ ref:Who -- ]
    Who @ array_get_ignorelist dup not if
        pop "no one"
    else Array_StrWnk Array_Quote "and" Array_Commas then
 
    me @ dup Who @ = if
        "You are currently ignoring    "
    else
        Who @ name " is currently ignoring " strcat
    then rot "." strcat strcat notify
;
 
: ReallyIgnoring?[ ref:Player ref:Who -- int:Result ]
    Player @ array_get_ignorelist Who @ array_findval if 1 else 0 then
;
 
: Ignore_ProcessOptionsClearSub[ idx:Idx ref:Who ref:Player -- int:Result ]
    Player @ Who @ ReallyIgnoring? if
        Player @ Who @ ignore_del 1
    else 0 then
;
 
: Ignore_ProcessOptionsSetSub[ idx:Idx ref:Who ref:Player -- int:Result ]
    Player @ Who @ ReallyIgnoring? if 0 else
        Player @ Who @ ignore_add 1
    then
;
 
: ProcessIgnores[ ref:Who str:List -- ]
    List @ AppParseAddRemovePlayers
 
    Who @ 'Ignore_ProcessOptionsClearSub Array_Filter rot
 
    dup Who @ array_findval if
        { Who @ }list swap array_diff
 
        me @ Who @ = if
            Who @ "You stick your fingers in your ears and shout \"Lalala!\" as loud as you can, but you just can't seem to be able to ignore yourself!" notify
        else
            me @ "You can't make a player ignore themselves!" notify
        then
    then
 
    Who @ 'Ignore_ProcessOptionsSetSub Array_Filter
 
    me @ Who @ = if
        "%[array]s %[is]s already on your ignore list." AppInfoMsgArray
        "Adding %[array]s to your ignore list." AppInfoMsgArray
        "%[array]s %[is]sn't on your ignore list." AppInfoMsgArray
        "Removing %[array]s from your ignore list." AppInfoMsgArray
    else
        { "%[array]s %[is]s already on " Who @ name "'s ignore list." }join AppInfoMsgArray
        { "Adding %[array]s to " Who @ name "'s ignore list." }join AppInfoMsgArray
        { "%[array]s %[is]sn't on " Who @ name "'s ignore list." }join AppInfoMsgArray
        { "Removing %[array]s from " Who @ name "'s ignore list." }join AppInfoMsgArray
    then
;
 
: ProcessCommands[ ref:Who str:List -- ]
    List @ strip if
        Who @ List @ ProcessIgnores
    else
        Who @ DisplayIgnores
    then
;
 
: DisplayHelp[ -- ]
    {
        "@Ignore v1.0                                                        Help Page"
        "-----------------------------------------------------------------------------"
        "Instructions:"
        "  @ignore may be used to manage your list of ignored players.  Any player"
        "  who you have placed on your ignore list you will NOT hear any messages"
        "  (says/poses/pages/etc...) from at all."
        " "
        "Commands:"
        "  @ignore                View your list of currently ignored players."
        "  @ignore <player>       Add <player> to your list of ignored players."
        "  @ignore !<player>      Remove <player> from your list of ignored players."
        
        me @ "WIZARD" flag? if
            " "
            "  @ignore <plyr>=        View <plyr>'s list of currently ignored players."
            "  @ignore <plyr>=<who>   Add <who> to <plyr>'s list of ignored players."
            "  @ignore <plyr>=!<who>  Remove <who> from <plyr>'s list of ignored players."
        then
    }tell
;
 
: Main[ str:Args -- ]
    "me" match me ! me @ Guest? if
        me @ "Permission denied." notify
    else
        "ignore_support" sysparm "yes" strcmp if
            me @ "Ignore not supported." notify
        else
            Args @ "#help" strcmp if
                Args @ "=" instr if
                    me @ "WIZARD" flag? if
                        Args @ "=" split swap strip dup pmatch dup if swap pop swap ProcessCommands else
                            pop me @ "I don't recognise the player named \"" rot "\"." notify pop
                        then
                    else me @ "Permission denied." notify then
                else me @ Args @ ProcessCommands then
            else DisplayHelp then
        then
    then
;
.
c
q
@set cmd-@ignore.muf=M3
