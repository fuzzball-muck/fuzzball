@prog cmd-newwa
1 99999 d
1 i
( WhereAre v6.03   Copyright 2002 by Revar revar@belfry.com )
( Released under the terms of the LGPL.                     )
 
$author Revar Desmera <revar@belfry.com>
$version 6.03
$note Released under the terms of the LGPL.
 
$def WA_PROP  "_whereare"
$def DIR_PROP "_wherearedir"
$def WF_PROP  "_prefs/con_announce_list"
$def WF_hide_prop "/_prefs/@con_announce_hide"
$def WF_show_prop "/_prefs/@con_announce_show"
$def WF_hideall_prop "/_prefs/con_announce_hideall?"
$def IDLE_TIME 180 (seconds)
 
$def NAMENEVER_PROP     "_prefs/whereare/namenever"
$def NAMEOK_PROP        "_prefs/whereare/nameok"
$def WADEFAULT_PROP     "_prefs/whereare/defaultopts"
$def WHEREISOK_PROP     "whereis/_ok?"
$def WHEREISUNFIND_PROP "whereis/_unfindable"
 
$def MASTER_FORMAT "%3[cnt]~ %3[act]~%3[wfl]~ %1[adult]~%-25.25[locname]~  "
$def OLD_FORMAT    "%-34.34[locname]~ %3[cnt]~%1[adult]~"
 
$def LASTPUBLIC "yes" (use lastpublic timestamp for idleness determination )
$def LASTPUBLIC_PROP "~lastpublic"
 
$iflib $adultlock
  $include $adultlock
  $def AGE_PROP1    "_Banish/force-age?"
  $def AGE_PROP2    "_Banish/@force-age?"
  $def WAADULT_PROP "_prefs/whereare/adult"
 
: Adultmaybe?[ ref:obj -- bool:hasprop ]
    obj @ AGE_PROP1 envpropstr swap pop
    obj @ AGE_PROP2 envpropstr swap pop
    OR
;
 
: checkage?[ ref:obj -- bool:isadult ]
    obj @ Adultmaybe? not if 1 EXIT then
    me @  WAADULT_PROP getpropstr not if 0 EXIT then
    obj @ pass-this-room?   (from adultlock)
;
$endif
 
 
: oktoname?[ ref:who dict:opts -- bool:isok ]
    opts @ "quell" [] not if
        me @ "w" flag? if 1 exit then
    then
    who @ ok? not if 0 exit then
    who @ namenever_prop getpropstr if 0 EXIT then
    who @ "H" flag? if 0 EXIT then
    who @ nameok_prop getpropstr if 1 EXIT then
    who @ whereisok_prop getpropstr 1 strcut pop "y" stringcmp
      if 0 exit then  (stringcmp reverse logic)
    who @ "_proploc" getpropstr atoi     (d i)
      dup if dbref else pop who @ then
      dup ok? not if pop 0 exit then
      whereisunfind_prop getpropstr 1 strcut pop "y" stringcmp
;
 
: atleast[ idx:index dict:roomentry dict:ctx -- int:keep ]
    ctx @ "mincnt" [] var! min
    ctx @ "sortcol" [] var! field
    roomentry @ "cnt" [] min @ >=
;
 
: array_filter[ arr:arr any:context addr:func -- arr:kept arr:discards ]
    { }dict var! keep
    { }dict var! discard
    arr @ foreach
        over over context @ func @ execute if
            keep @ rot array_setitem keep !
        else
            discard @ rot array_setitem discard !
        then
    repeat
    keep @ discard @
;
 
: increment_entry[ arr:arr idx:idx -- arr:arr' ]
    arr @ idx @ [] ++
    arr @ idx @ ->[]
;
 
: append_name[ arr:arr idx:idx ref:who dict:opts -- arr:arr' ]
    who @ opts @ oktoname? if
        arr @ idx @ []
        opts @ "format" [] "col" stringcmp not if
            dup if
                dup "\r" rsplit dup if swap then pop
                strip strlen who @ name strlen +
                37 > if
                    "\r" strcat
                    " " "%40s" fmtstring strcat
                else
                    " " strcat
                then
            then
        else
            dup if " " strcat then
        then
        who @ name strcat
        arr @ idx @ ->[]
    else
        arr @
    then
;
 
: is_watched_For[ ref:who -- bool:watched ]
    me @ WF_PROP who @ reflist_find not if 0 exit then
    who @ WF_hideall_prop getpropstr "yes" stringcmp not if
        who @ WF_show_prop me @ reflist_find if 1 exit then
        0 exit
    then
    who @ WF_hide_prop me @ reflist_find if 0 exit then
    who @ "D" flag? if 0 exit then
    1
;
 
: get_master_format[ dict:opts -- str:fmt ]
    opts @ "style" [] "old" stringcmp not if
        OLD_FORMAT
    else
        MASTER_FORMAT
    then
;
 
: show_whereare[ dict:opts -- ]
    opts @ "mincnt"  [] var! min
    opts @ "sortcol" [] var! sorton
    opts @ "optcol"  [] var! optfield
    opts @ "showall" [] var! showall
    0 var! nonpublic
 
    { }dict var! walist
    online_array { }list array_union
    foreach swap pop var! who
        who @ location var! wholoc
        0 nonpublic !
        wholoc @ WA_PROP getpropstr not if
            showall @ not
            me @ "wizard" flag? not
            or if
                continue
            then
            1 nonpublic !
        then
$iflib $adultlock
        wholoc @ checkage? not if continue then
$endif
        walist @ wholoc @ int []
        dup not if
            pop {
                "loc"   wholoc @
                "locname" wholoc @ name nonpublic @ if "[%.23s]" fmtstring then
                "note"  wholoc @ WA_PROP getpropstr strip
                "dir"   wholoc @ DIR_PROP getpropstr strip
$iflib $adultlock
                "adult" wholoc @ Adultmaybe? if "*" else " " then
$else
                "adult" " "
$endif 
                "act"     0
                "cnt"     0
                "wfl"     0
                "names"   ""
                "wfnames" ""
            }dict
        then
 
        "cnt" increment_entry
 
$ifdef LASTPUBLIC
        who @ LASTPUBLIC_PROP getpropstr atoi
        dup not if pop who @ LASTPUBLIC_PROP getpropval then
        systime swap -
$else
        who @ descrleastidle descridle
$endif
        IDLE_TIME < if
            "act" increment_entry
        then
 
        who @ is_watched_for if
            "wfl" increment_entry
            "wfnames" who @ opts @ append_name
        then
        "names" who @ opts @ append_name
      
        walist @ wholoc @ int ->[] walist !
    repeat
    {
        "cnt"     "Tot"
        "wfl"     "WF"
        "act"     "Act"
        "adult"   ""
$iflib $adultlock
        "loc"     "Room Name [*=Adult area]"
$else
        "loc"     "Room Name"
$endif
        "note"    "Comments"
        "names"   "Names"
        "wfnames" "Watched For"
        "dir"     "Directions"
    }dict var! labels
    walist @ array_vals array_make
 
    opts @ 'atleast array_filter pop array_vals array_make
    
    { "wfl" "act" "cnt" }list dup sorton @
    array_excludeval array_extract array_vals array_make
    sorton @ swap array_appenditem
    foreach swap pop
        opts @ "ascend" [] if
            SORTTYPE_NOCASE_ASCEND
        else
            SORTTYPE_NOCASE_DESCEND
        then
        swap array_sort_indexed
    repeat
 
    opts @ get_master_format
    "%-38.38[OPT]~" strcat var! labelfmt
 
    opts @ "format" [] "2ln" stringcmp not
    optfield @ "note" stringcmp and if
        opts @ get_master_format
        "%-38.38[note]~\r            LABEL: %[OPT]~" strcat
        labels @ optfield @ [] "LABEL" subst
 
        labelfmt @ optfield @ "OPT" subst labelfmt !
    else
        opts @ get_master_format "%[OPT]~" strcat
    then
 
    { labels @ }list labelfmt @
    optfield @ "OPT" subst
    array_fmtstrings
    { me @ }list array_notify
 
    optfield @ "OPT" subst
    array_fmtstrings
    { me @ }list array_notify
    me @ IDLE_TIME
$ifdef LASTPUBLIC
    "-- Tot=Total Awake, Act=Visibly Active in last %isecs, WF=In WatchFor --"
$else
    "-- Tot = Total Awake, Act = Active [idle<%isec], WF = In WatchFor --"
$endif
    fmtstring notify
;
 
: set_default[ str:args -- ]
    me @ WADEFAULT_PROP args @ strip setprop
    args @ strip dup if
        "Setting default options to: " swap strcat
    else
        pop "Resetting default options."
    then
    me @ swap notify
;
 
: set_wa_prop[ str:val -- ]
    me @ dup location controls not if
        "Permission denied." .tell
        exit
    then
    val @ strip val !
    val @ ansi_strlen 38 > if
        "The given comment is more than 38 characters in length." .tell
        exit
    then
    me @ location wa_prop val @ setprop
    val @ if
        "This room will now be shown in the whereare listings." .tell
    else
        "This room will no longer be shown in whereare listings." .tell
    then
;
 
: set_wadir_prop[ str:val -- ]
    me @ dup location controls not if
        "Permission denied." .tell
        exit
    then
    val @ strip val !
    val @ ansi_strlen 38 > if
        "The given directions are more than 38 characters in length." .tell
        exit
    then
    me @ location dir_prop val @ setprop
    val @ if
        "Whereare directions set." .tell
    else
        "Whereare directions cleared." .tell
    then
;
 
: set_whereis[ -- ]
    me @ nameok_prop remove_prop
    me @ namenever_prop remove_prop
    "WhereAre will now show your name #name or #wf, based on your whereis setting." .tell
;
 
: set_always[ -- ]
    me @ nameok_prop "yes" setprop
    me @ namenever_prop remove_prop
    "WhereAre will now always show your name in a WhereAre #name or #wf list." .tell
;
 
: set_never[ -- ]
    me @ namenever_prop "yes" setprop
    me @ nameok_prop remove_prop
    "WhereAre will now never show your name in a WhereAre #name or #wf list." .tell
;
 
$iflib $adultlock
: set_adult[ bool:adult -- ]
    adult @ if
        me @ WAADULT_PROP "yes" setprop
        me @ "WhereAre will now list adult rooms." notify
    else
        me @ WAADULT_PROP remove_prop
        me @ "WhereAre will now no longer list adult rooms." notify
    then
;
$endif
 
: show_usage_long[ -- ]
    {
        " "
        "WhereAre v6.03   Copyright 2003 by foxen@belfry.com"
        "--------------------------------------------------------------------------"
        "Only one of the following optional fields can be shown at a time:"
        "  #comments      Show the descriptive comment for each room. [default]"
        "  #names         Show the names of findable people in each room."
        "  #wfnames       Same as #names, but only lists those in your watchfor."
        "  #directions    Show the directions to each room."
        " "
        "Only one of the following sorting options can be used at a time:"
        "  #bycount       Sort by number of awake people in each room. [default]"
        "  #byactive      Sort by number of people idle less than 3 minutes."
        "  #bywfl         Sort by number of people from your watchfor list."
        "  #byroom        Sort by the name of the room."
        " "
        "Only one of the following format options can be used at a time:"
        "  #columnar      Show #names or #wf all in the same last column. [default]"
        "  #twoline       Show #names or #wf on a second line, similar to old WA."
        "  #oneline       Show #names or #wf in the last column, all in one line."
        " "
        "The following specify other display options:"
        me @ "w" flag? if
            "  #quell         Lets wizards see #names and #wf as a normal player would."
            "  #all           Lets wizards see all rooms, even those not public."
        then
        "  #reversed      Sort display in reversed order."
        "  #old           Show results like old 'whereare'.  Implies #twoline."
        "  #new           Resets #old.  Implied #columnar."
        "  NUMBER         Min awake user count req'd to list room [default=2]."
        " "
        "The following commands can only appear as the first option:"
        "  #default OPTS  Sets your default options to OPTS.  Uses entire line."
        "  #reset         Resets your default options back to system default."
$iflib $adultlock
        compare-my-age if
            "  #adult         Specifies you wish to see adult rooms in the future."
            "  #prude         Specifies you do NOT wish to see adult rooms. [default]"
        then
$endif
        "  #never         Never show your name when others use #name or #wf."
        "  #always        Always show your name when others use #name or #wf."
        "  #whereis       Maybe show name in #name or #wf, depending on whereis. [def]"
        "  #set COMMENT   List current room in future WA listing with the given COMMENT."
        "  #set           Use without COMMENT to remove this room from future listings."
        "  #setdir DIRS   Sets the 'Directions' shown to get to this room in listings."
        "  #help          Shows this help message."
        " "
    }tell
;
 
 
: show_usage[ -- ]
    {
        "I didn't understand that!  Please see 'whereare #help' for help."
    }tell
;
 
 
: stringminpfx[ str:pattern str:val int:minlen -- bool:truefalse ]
    pattern @ val @ stringpfx
    val @ strlen minlen @ >= and
;
 
 
: parse_args[ dict:opts str:args int:ignore -- arr:opts 0 | 1 ]
    1 var! firstopt
    begin
        args @ striplead " " split args !
        dup while
        
        dup "{#|#by}" smatch if pop show_usage 1 exit then
        
        firstopt @ if
            0 firstopt !
            ignore @ if
                dup "#help"     swap stringpfx if pop continue then
                dup "#default"  swap 3 stringminpfx if pop continue then
                dup "#reset"    swap stringpfx if pop continue then
$iflib $adultlock
                dup "#adult"    swap stringpfx if pop continue then
                dup "#prude"    swap stringpfx if pop continue then
$endif
                dup "#never"    swap stringpfx over strlen 2 > and if pop continue then
                dup "#always"   swap stringpfx if pop continue then
                dup "#whereis"  swap stringpfx if pop continue then
                dup "#set"      swap stringpfx if pop continue then
                dup "#setdir"   swap stringpfx if pop continue then
            else
                dup "#help"     swap stringpfx if pop show_usage_long 1 exit then
                dup "#default"  swap 3 stringminpfx if pop args @ set_default 1 exit then
                dup "#reset"    swap stringpfx if pop "" set_default 1 exit then
$iflib $adultlock
                compare-my-age if
                    dup "#adult"    swap stringpfx if pop 1 set_adult 1 exit then
                    dup "#prude"    swap stringpfx if pop 0 set_adult 1 exit then
                then
$endif
                dup "#never"    swap stringpfx over strlen 2 > and if pop set_never 1 exit then
                dup "#always"   swap stringpfx if pop set_always 1 exit then
                dup "#whereis"  swap stringpfx if pop set_whereis 1 exit then
                dup "#set"      swap stringpfx if pop args @ set_wa_prop 1 exit then
                dup "#setdir"   swap stringpfx if pop args @ set_wadir_prop 1 exit then
            then
        then
        
        dup "#names"      swap stringpfx if pop "names"   opts @ "optcol" ->[] opts ! continue then
        dup "#comments"   swap stringpfx if pop "#notes" then
        dup "#notes"      swap stringpfx if pop "notes"   opts @ "optcol" ->[] opts ! continue then
        dup "#wfnames"    swap stringpfx if pop "wfnames" opts @ "optcol" ->[] opts ! continue then
        dup "#directions" swap stringpfx if pop "dir"     opts @ "optcol" ->[] opts ! continue then
        
        dup "#bycount"    swap stringpfx if pop "cnt" opts @ "sortcol" ->[] opts ! continue then
        dup "#byactive"   swap stringpfx if pop "act" opts @ "sortcol" ->[] opts ! continue then
        dup "#bywfl"      swap stringpfx if pop "#bywatchfor" then
        dup "#bywatchfor" swap stringpfx if pop "wfl" opts @ "sortcol" ->[] opts ! 1 opts @ "mincnt" ->[] opts ! continue then
        dup "#byroom"     swap stringpfx if pop "locname" opts @ "sortcol" ->[] 1 swap "ascend" ->[] opts ! continue then
 
        dup "#old"        swap stringpfx if pop "old" opts @ "style" ->[] "2ln" swap "format" ->[] opts ! continue then
        dup "#new"        swap stringpfx if pop "new" opts @ "style" ->[] "col" swap "format" ->[] opts ! continue then
 
        dup "#columnar"   swap stringpfx if pop "col" opts @ "format" ->[] opts ! continue then
        dup "#twoline"    swap stringpfx if pop "2ln" opts @ "format" ->[] opts ! continue then
        dup "#oneline"    swap stringpfx if pop "1ln" opts @ "format" ->[] opts ! continue then
 
        dup "#quell"      swap stringpfx if pop "yes" opts @ "quell"  ->[] opts ! continue then
        dup "#all"        swap stringpfx if pop 1 opts @ "showall"  ->[] opts ! continue then
        dup "#reversed"   swap stringpfx if pop opts @ "ascend" [] not opts @ "ascend" ->[] opts ! continue then
 
        dup number? if atoi opts @ "mincnt" ->[] opts ! continue then
        ignore @ not if
            pop show_usage 1 exit
        then
    repeat pop
    opts @ 0
;
 
: main[ str:args -- ]
    {
        "mincnt"  2
        "sortcol" "cnt"
        "optcol"  "note"
        "format"  "col"
        "quell"   ""
        "ascend"  0
        "style"   "new"
        "showall" 0
    }dict
    me @ WADEFAULT_PROP getpropstr 1 parse_args if exit then
    args @ 0 parse_args if exit then
    show_whereare
;
 
.
c
q
@register #me cmd-newwa=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3
@action WhereAre;wa=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
@propset $tmp/exit1=str:/_/de:wa #help for info.

