@prog lib-bolding.muf
1 99999 d
1 i
$lib-version    1.04
$def LIBDATE    "Jul 29 2002"
$def LIBNAME    "lib-cmdtable"
$author                 Vash@Anime/Sarusa@FM
(
  -= SUMMARY =-
        This only works on FB6. You can check for it by:
                $ifdef __version>Muck2.2fb6
                        do bolding stuff
                $endif
 
        This library includes support for a certain sort of bolding, in which
        anything bracketed by * is converted to bold, and anything bracketed by _
        is converted to underline, by the following rules:
          - The */_ must be preceeded by a space [or be first on the line].
          - The */_ must be followed by a letter or number.
          - There must be a matching */_ on the line to turn it off.
 
        All the bolding and underlining is done by on the RECEIVER side only. The
        person talking/paging/shouting/whatever does not have to do anything
        special, other than use the *bold* formatting as they might anyhow.
        
        Someone who has bolding turned on would see:
                Vash says, "Knives, you <bold>monster</bold>!"
        [where the <bold> </unbold> is shifting to another ANSI color code], and
        anyone who has bolding off would still see:
                Vash says, "Knives, you *monster*!"
 
        This code is formatted for 4-space tab stops. And I tend to format like
        Python [i.e., watch the indenting].
 
  -= LEGALESE =-
        Use this anywhere you want, without asking, just leave this header, please.
        And if you're feeling helpful, you could document your changes down below
        and change the VERSION so people know they're not using a stock version.
 
  -= INSTALLING =-
        This library needs to be set M3.
 
  -= USE =-
        '$include $lib/bolding' in your MUF code.
         [or '$include #dbref-of-the-lib' if it's not global on your MUCK]
 
        You then have access to the following common functions:
        
        BOLD-supported-q[ -- bool:supported ]
                This will be true on FB6+, false otherwise <FB5>. It tells you
                whether the MUCK can even do color.
 
        BOLD-filter[ dbref:who str:message -- str:formatted_message ]
                Filters 'message' based on 'who's settings, which may mean it comes
                back totally unchanged. This will not notify, just return the
                appropriate string.
 
        BOLD-notify[ dbref:who str:message -- ]
                Filters 'message' based on 'who's settings, then notifies.
 
                Note: If calling program is M1, this will prepend the name, just
                like a normal 'notify'.
 
        BOLD-notify-list[ arr:who str:message -- ]
                For each 'who' in the array, filters 'message' appropriately, and
                then notifies them. This doesn't make any sense under FB5, which
                has no arrays.
 
                Note: If calling program is M1, this will prepend the name, just
                like a normal 'notify'.
 
        BOLD-cmd-parse1[ str:cmdarg -- bool:handled ]
                This requires the calling program be at least M3.
                Attempts to parse user input - if it returns handled true, then it
                was caught and handled. See BOLD-cmd-parse2 for details.
        
        BOLD-cmd-parse2[ str:argument str:cmd -- bool:handled ]
                This requires the calling program be at least M3.
                Attempts to parse user input which has already been split up into
                a one word 'cmd' <like '#bold'> and 'argument' <like 'bold,green'>
 
                If handled is true, it handled the input, otherwise it's up to
                you to keep trying to handle it.
 
                It handles the following:
                        #bold                   - turn bolding settings
                        #bold on                - turn bolding on
                        #bold off               - turn bolding off
                        #bold help              - BOLD-cmd-help
                        #bold help2         - advanced help
                        #bold trebuchet - trebuchet specific help
                        #bold-code <attr>  - set bold on code
                        #under-code <attr> - set underline on code
                        #italic-code <attr> - set italic on code <no longer>
                        #off_code <attr>        - set 'turn off' code
 
                <attr> is the same as in 'man textattr' or 'mpi attr'. These will
                set the appropriate user props.
 
                Example, assuming we're split into cmd and arg already:
                        cmd @ "#help" stringcmp not if
                                pop do_help exit then
                        arg @ cmd @ BOLD-cmd-parse2 if 
                                exit then <was handled>
                        "Unrecognized command '%s'." cmd @ "%s" subst .tell
 
        BOLD-cmd-help[ -- ]
                This will show the user #help for the BOLD-cmd-parse* commands. The
                idea is that you can incorporate it into your own #help screens.
                'cmd' is the name of the program/trigger being run. 
 
                Make sure you also call BOLD-cmd-parse1 or BOLD-cmd-parse2 from inside
                your command parsing section, or this does you no good at all!
 
                For example <in a really stupid way>:
                        userinput @ "#help bold" stringcmp not if
                                BOLD-cmd-help exit then 
 
        BOLD-get-settings [ -- str:status ]
                Returns a string that summarizes the current BOLD settings for
                'me @' - from the same routine used to print the status for #bold?
 
 
        Less commonly useful functions:
 
        BOLD-possible-q[ dbref:who -- bool:possible ]
                Returns true if char 'who' can even see color at all. Even if the
                muck supports color, the char might not be set up for it, so
                you would normally call BOLD-active-q instead.
 
        BOLD-active-q[ dbref:who -- bool:active ]
                Returns true if char 'who' has bolding turned on AND they can
                even see color at all.
 
        BOLD-set-active[ dbref:who bool:active -- ]
                Calling program must be m3 to do this. Does not change the
                BOLD-possible-q setting.
                This is still VERY rude to do without the user requesting it.
 
        BOLD-filter-test[ str:message -- str:formatted_message ]
                Filters 'message' with generic codes for 'me @' assuming it can be
                done at all. You should use this only for test messages - do NOT
                override people's settings normally. Like this:
                        "If you can see color at all, *this* should look different."
                        BOLD-filter-test .tell
 
  -= VERSIONS =- 
        V0.01   Jul 17 2002 First cut - won't compile under FB5 yet
        V0.02   Jul 17 2002 Add BOLD-cmd-help*, still not FB5 compliant
        V0.03   Jul 17 2002 Add trebuchet help, /, escape char in smatch
        V0.04   Jul 19 2002 Get rid of italics, too many problems
        V1.00   Jul 19 2002 Make it sane under FB5, release.
        V1.01   Jul 19 2002 BOLD-notify-list won't tell zombies if owner in list
        V1.02   Jul 26 2002 Make 'C' flag? check on owner instead of puppet
        V1.03   Jul 26 2002 Convert 'active' flag to 'off' flag, default more sense
        V1.04   Jul 26 2002 Get rid of FB5 support. Not worth it.
        
)
 
$define VERSION { "---- " LIBNAME " " prog "_lib-version" getpropstr " - "
                                        prog "_author" getpropstr " - " LIBDATE " ----" }join
$enddef
 
$def boldchar "*"
($def italicchar "/")
$def underchar "_"
 
$def uval_off                   "_prefs/bolding/off" (0=on, 1=off)
$def ustr_bold_code             "_prefs/bolding/bold_code"
($def ustr_italic_code  "_prefs/bolding/italic_code")
$def ustr_under_code    "_prefs/bolding/under_code"
$def ustr_off_code              "_prefs/bolding/off_code"
 
$def get_bold_code ustr_bold_code getpropstr dup not if pop "1" then
($def get_italic_code ustr_italic_code getpropstr dup not if pop "2" then)
$def get_under_code ustr_under_code getpropstr dup not if pop "4" then
$def get_off_code ustr_off_code getpropstr dup not if pop "0" then
 
 
($def bold-active?   uval_active getprop 0 >=)
$def uval_active                "_prefs/bolding/active"  (0/1=on, -1=off)
(use the long way for a week, then revert to simple check)
($def bold-active?   uval_off getprop not)
: bold-active?[ dbref:who -- bool:active? ]
 
        ( this entire section is temporary )
        who @ uval_active getprop dup if
                who @ uval_active remove_prop
                -1 = if
                        who @ uval_off 1 setprop
                else
                        who @ uval_off remove_prop
                then
        else pop then
 
        who @ uval_off getprop not
;
                
 
 
$ifdef __version>Muck2.2fb6
$def COLOR
$def bold-possible? ( dbref:who -- bool:yesno ) owner "c" flag? 
  ( May be HTML support later, so don't hardcode ANSI except here )
$def colorstart "\[["
$def colorend   "m"
$else
$error This only works on FB6+ MUCKs
$endif
 
( -----------------------------------------------------------
  Worker procedures, not visible outside
  ----------------------------------------------------------- )
 
(- check caller M level - mlevel is 0-4, 4 is wizbit -)
: mlevel[ dbref:what -- int:mlevel ]
        what @ "W" flag? if 4 else
                what @ "M" flag? 2 * what @ "N" flag? +
        then
;
 
(- bold/underline filter for ansi -)
: bold_filter_specific[ (str:msg) str:astart str:aend str:sep -- (str:msg) ] 
 
        "" swap ("" "this _goes_ boom" )
        begin ( sdone sdoing )
                dup " " sep @ strcat instr dup while
                strcut ( "" "this " "_goes_ boom" )
                ( next needs to be a letter )
                dup "\\" sep @ strcat "[a-z0-9]*" strcat smatch not if
                        strcat 0 break then
                dup sep @ rinstr 2 < if ( no closing _ found )
                        strcat 0 break then ( "" "this " "_goes_ boom" )
                1 strcut swap pop ( "" "this "goes_ boom" )
                dup sep @ instr ( "" "this " "goes_ boom" 4 )
                -- strcut 1 strcut swap pop swap ( "" "this " " boom" "goes" )
 
                colorstart astart @ colorend strcat strcat swap strcat ( underline on! )
                colorstart aend @ colorend strcat strcat strcat
                                        ( "" "this " " boom" "<u>goes</u>" )
                rot swap strcat rot swap strcat swap ( "this <u> goes</u>" " boom" )
        repeat pop strcat
;
 
: bold_filter[ (str:msg) dbref:who -- (str:msg) ] 
        " " swap strcat ( add a space on front naughty, but saves special cases )
 
        who @ get_bold_code
        who @ get_off_code
        boldchar bold_filter_specific
 
        who @ get_under_code
        who @ get_off_code
        underchar bold_filter_specific
 
(       who @ get_italic_code
        who @ get_off_code
        italicchar bold_filter_specific )
 
        1 strcut swap pop ( get rid of space on front )
;
 
: do_filter_test[ str:message -- str:formatted_message ]
        message @
        me @ bold-possible? if 
                prog bold_filter
        then
;
 
 
: do_help[ -- ]
        "\r- This program uses the Global Bolding library ($lib/bolding, available" .tell
        "  for any program to use, though not all do ). By default it's on if you" .tell
        "  have color enabled ('@set me=c'). To turn it off:" .tell
        "    #bold off    - turn it off" .tell
        "    #bold on     - turn it back on" .tell
(       "\r- If it is on, it will take any words bracketed by *, _, or /, and turn" .tell
        "  them into bold, underline, and italic color codes respectively. If it is" .tell
        "  off, you will just see the characters unchanged. It only affects YOU." .tell
        "\r- If it is on, it will take any words bracketed by * or _, and turn" .tell
        "  them into bold and underline color codes respectively. If it is off," .tell
        "  you will just see the characters unchanged. It only affects YOU." .tell
        "\r- It will try to avoid eating valid * _ or /'s with the following rules:" .tell )
        "    * The */_ must be preceeded by a space (or be first on the line)." .tell
        "    * The */_ must be followed by a letter or number." .tell
        "    * There must be a matching */_ on the line to turn it off." .tell
        "\r- See '#bold trebuchet' for Trebuchet specific help!" .tell
        "- See '#bold help2' for information on changing the codes, or reasons" .tell
        "  why it might not be working for you." .tell
;
 
: do_help2[ -- ]
        "\r- To have bolding work, your terminal program/client must support ANSI," .tell
        "  you must be in MUCK Color mode ('@set me=c'), your client must not be" .tell
        "  overriding the color settings completely, and you must have bolding" .tell
        "  enabled ('#bold on')." .tell
        "    If the first three are true, *this should look different*." 
                do_filter_test .tell
        "\r- If you wish to change the color codes, use the following:" .tell
        "    #bold-code <attribute>" .tell
(       "    #italic-code <attribute>" .tell)
        "    #under-code <attribute>" .tell
        "  See 'man textattr' or 'mpi attr' for a list of valid <attribute>s. You" .tell
        "  can combine the codes with ',' like this:" .tell
        "        #bold-code bold,green" .tell
(       "\r- Keep in mind that many clients don't actually support italic or" .tell
        "  underline, so just pick color combos that you like." .tell )
        "\r- Keep in mind that many clients don't actually support underline so" .tell
        "  just pick color combos that you like." .tell
(       "- Trebuchet uses flash as italic on, so do '#italic-code flash'." .tell)
        "\r- You can set the '#off-code', but it usually should be left as 'reset'." .tell
;
 
: do_help_trebuchet[ -- ]
(       "\r- Trebuchet uses the ANSI 'flash' code to turn on italics. This is not" .tell
        "  used by default, because that would look VERY bad for people whose" .tell
        "  clients would actually start flashing text. Use this to make /italic/" .tell
        "  actually show up as italic:" .tell
        "     #italic-code flash" .tell )
        "\r- If Trebuchet is overriding the MUCK color codes with its own, go to" .tell
        "  Option->Preferences->Display and turn on " .tell
        "    'ANSI colors override hilite colors'" .tell
;
 
(- return settings string for 'me @' -)
: get_settings_string[ -- str:settings ]
        me @ bold-possible? not if
                "You do not have color turned on ('@set me=c')."
        else me @ bold-active? not if
                "You have bolding turned off ('#bold on')."
        else 
(               "Global Bolding is on: *bold* _underline_ /italic/"     me @ bold_filter)
                "Global Bolding is on: *bold* _underline_"      me @ bold_filter
        then then
; 
 
(- show bold settings -)
: show_settings[ -- ]
        "-- " get_settings_string strcat .tell
;
 
(- Turn off bolding -)
: do_unbold[ str:arg -- ]
        me @ uval_off 1 setprop
        "-- #bold mode turned off." .tell
        show_settings
;
 
(- Turn on bolding -)
: do_bold[ str:arg -- ]
        arg @ not if show_settings exit then
        arg @ "off" stringcmp not if arg @ do_unbold exit then
        arg @ "help" stringcmp not if do_help exit then
        arg @ "help2" stringcmp not if do_help2 exit then
        arg @ "treb" stringpfx if do_help_trebuchet exit then
        arg @ "on" stringcmp if
                "-- Unknown command: '#bold %s'" arg @ "%s" subst .tell
                exit then
 
        me @ uval_off remove_prop
        "-- #bold mode turned on." .tell show_settings
;
 
(- Set various bold related values -)
: do_setcode[ str:sarg str:sprop -- ]
        me @ bold-possible? not if
                "-- You are not set up to receive color from the MUCK." .tell
                "   '@set me=c' to do so ('@set me=!c' to turn off again)" .tell
                exit then
        0 try ( try getting the attr )
                sarg @ "@@@" swap textattr
                dup "@@@" instr 1 - strcut pop ( get rid of back stuff )
                ";" colorstart subst "" colorend subst ( ^[31m^[1m -> ;31;1 )
                1 strcut swap pop
                dup strip if
                        me @ sprop @ rot setprop
                else pop then
                show_settings
        catch
                pop "-- Error trying to understand '%s' as an attribute." 
                        sarg @ "%s" subst .tell
                "   See 'mpi attr' or 'man textattr' for a list, and separate them" .tell
                "   with a ',' if you want more than one (like 'bold,green')." .tell
        endcatch
;
 
 
( -----------------------------------------------------------
  Exported routines for FB6
  ----------------------------------------------------------- )
: BOLD-supported-q[ -- ]
        1
;
 
: BOLD-possible-q[ dbref:who -- bool:possible ]
        who @ bold-possible?
;
 
: BOLD-active-q[ dbref:who -- bool:active ]
        who @ bold-active? if
                who @ bold-possible?
        else 0 then
;
 
: BOLD-set-active[ dbref:who bool:active -- ]
        caller mlevel 3 < if
                "BOLD-set-active was called by non-M3 program." abort then
        who @ uval_off active @ if 0 else 1 then setprop
;
 
: BOLD-filter[ dbref:who str:message -- str:formatted_message ]
        message @ who @ bold-possible? if who @ bold-active? if 
                 who @ bold_filter
        then then
;          
 
: BOLD-filter-test[ str:message -- str:formatted_message ]
        message @ do_filter_test
;
 
: BOLD-notify[ dbref:who str:message -- ]
        who @ message @ BOLD-filter
        caller mlevel 1 <= if   ( add name of user to M1 notify )
                me @ name " " strcat swap strcat then
        who @ swap notify
;
 
: BOLD-notify-list[ arr:wholist str:message -- ] 
        wholist @ foreach
                swap pop ( get rid of array index )
                dup ok? not if pop continue then
                dup "z" flag? if ( zombie whose owner is in the room, ignore )
                        wholist @ over owner array_findval if
                                pop continue then
                then
                message @ BOLD-notify
        repeat
;
 
: BOLD-cmd-parse2[ str:argument str:cmd -- bool:handled ]
        caller mlevel 3 < if
                "BOLD-cmd-parse was called by non-M3 program." abort then
 
        argument @
        cmd @ "#bold" stringcmp not if do_bold 1 exit then
        cmd @ "#bold-code" stringcmp not if ustr_bold_code do_setcode 1 exit then
(       cmd @ "#italic-code" stringcmp not if ustr_italic_code do_setcode 1 exit then )
        cmd @ "#under-code" stringcmp not if ustr_under_code do_setcode 1 exit then
        cmd @ "#off-code" stringcmp not if ustr_off_code do_setcode 1 exit then
        pop 0   ( wasn't handled )
;
        
: BOLD-cmd-parse1[ str:cmdarg -- bool:handled ]
        cmdarg @ 
        dup " " instr dup if
                strcut swap striptail   ( cut into cmd and argument )
        else 
                pop "" swap ( was no argument, just put a blank there )
        then
        BOLD-cmd-parse2
;
 
: BOLD-cmd-help2[ -- ]
        do_help2
;
 
: BOLD-cmd-help[ -- ]
        do_help
;
 
: BOLD-get-settings[ -- str:settings ]
        get_settings_string
;
 
( -----------------------------------------------------------
  Test routine
  ----------------------------------------------------------- )
 
 
( For testing purposes! )
: main ( str:arg -- )
        pop 
        VERSION .tell
        "-- Testing the bolding library." .tell
        BOLD-get-settings .tell
        "-- Testing notify:" .tell
        me @ "  I should see this once." BOLD-notify
        "-- Testing notify-list:" .tell
        { me @ me @ "realdog" match }LIST "  I should see this twice." BOLD-notify-list
;
 
 
(These three will be renamed to ? instead of '-q' when compile bug fixed)
PUBLIC BOLD-active-q
PUBLIC BOLD-possible-q
PUBLIC BOLD-supported-q
 
PUBLIC BOLD-cmd-parse2
PUBLIC BOLD-cmd-parse1
PUBLIC BOLD-cmd-help2
PUBLIC BOLD-cmd-help
PUBLIC BOLD-filter
PUBLIC BOLD-filter-test
PUBLIC BOLD-get-settings
PUBLIC BOLD-notify
PUBLIC BOLD-notify-list
PUBLIC BOLD-set-active
 
(
@set $lib/bolding=_docs:@list $lib/bolding=1-137
@set $lib/bolding=_defs/BOLD-active-q: "$lib/bolding" match "BOLD-active-q" call
@set $lib/bolding=_defs/BOLD-cmd-parse2: "$lib/bolding" match "BOLD-cmd-parse2" call
@set $lib/bolding=_defs/BOLD-cmd-parse1: "$lib/bolding" match "BOLD-cmd-parse1" call
@set $lib/bolding=_defs/BOLD-cmd-help2: "$lib/bolding" match "BOLD-cmd-help2" call
@set $lib/bolding=_defs/BOLD-cmd-help: "$lib/bolding" match "BOLD-cmd-help" call
@set $lib/bolding=_defs/BOLD-filter: "$lib/bolding" match "BOLD-filter" call
@set $lib/bolding=_defs/BOLD-filter-test: "$lib/bolding" match "BOLD-filter-test" call
@set $lib/bolding=_defs/BOLD-get-settings: "$lib/bolding" match "BOLD-get-settings" call
@set $lib/bolding=_defs/BOLD-notify: "$lib/bolding" match "BOLD-notify" call
@set $lib/bolding=_defs/BOLD-notify-list: "$lib/bolding" match "BOLD-notify-list" call
@set $lib/bolding=_defs/BOLD-possible-q: "$lib/bolding" match "BOLD-possible-q" call
@set $lib/bolding=_defs/BOLD-set-active: "$lib/bolding" match "BOLD-set-active" call
@set $lib/bolding=_defs/BOLD-supported-q: "$lib/bolding" match "BOLD-supported-q" call
)
 
.
c
q
@register lib-bolding.muf=lib/bolding
@register #me lib-bolding.muf=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_defs/BOLD-active-q: "$lib/bolding" match "BOLD-active-q" call
@propset $tmp/prog1=str:/_defs/BOLD-cmd-help: "$lib/bolding" match "BOLD-cmd-help" call
@propset $tmp/prog1=str:/_defs/BOLD-cmd-help2: "$lib/bolding" match "BOLD-cmd-help2" call
@propset $tmp/prog1=str:/_defs/BOLD-cmd-parse1: "$lib/bolding" match "BOLD-cmd-parse1" call
@propset $tmp/prog1=str:/_defs/BOLD-cmd-parse2: "$lib/bolding" match "BOLD-cmd-parse2" call
@propset $tmp/prog1=str:/_defs/BOLD-filter: "$lib/bolding" match "BOLD-filter" call
@propset $tmp/prog1=str:/_defs/BOLD-filter-test: "$lib/bolding" match "BOLD-filter-test" call
@propset $tmp/prog1=str:/_defs/BOLD-get-settings: "$lib/bolding" match "BOLD-get-settings" call
@propset $tmp/prog1=str:/_defs/BOLD-notify: "$lib/bolding" match "BOLD-notify" call
@propset $tmp/prog1=str:/_defs/BOLD-notify-list: "$lib/bolding" match "BOLD-notify-list" call
@propset $tmp/prog1=str:/_defs/BOLD-possible-q: "$lib/bolding" match "BOLD-possible-q" call
@propset $tmp/prog1=str:/_defs/BOLD-set-active: "$lib/bolding" match "BOLD-set-active" call
@propset $tmp/prog1=str:/_defs/BOLD-supported-q: "$lib/bolding" match "BOLD-supported-q" call
@propset $tmp/prog1=str:/_docs:@list $lib/bolding=1-137

