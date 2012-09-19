@prog lib-optionsmenu
1 99999 d
1 i
( $lib/optionsmenu  Copyright 10/18/2002 by Revar <revar@belfry.com> )
( Released under the GNU LGPL.                                       )
(                                                                    )
( A MUF library to automatically create menus to edit a set of       )
( option values.                                                     )
(                                                                    )
( v1.000 -- 10/18/2002 -- Revar <revar@belfry.com>                   )
(     Initial revision.                                              )
( v1.001 --  8/18/2003 -- Revar <revar@belfry.com>                   )
(     Added opts_id to callback data.                                )
( v1.002 --  3/06/2004 -- Revar <revar@belfry.com>                   )
(     Changed regexps to PCRE style regexps to match the server.     )

$author Revar Desmera <revar@belfry.com>
$version 1.002
$lib-version 1.001
$note Released under the LGPL.

(

    MENU_OPTIONS_PROCESS[ ref:user  any:caller_ctx  addr:saveall_cb
                        str:title arr:optionsinfo -- ]

        Generates a menu or set of menus that will edit a set of options
        that are described by the optionsinfo array.  The menu[s] will be
        created with the given title root, and displayed to the given user.
        The caller program may provide the address to a saveall_cb callback
        function that will be called when the user saves the changes.
        If saveall_cb is 0, this call will not be made.  The caller program
        also provides a caller_ctx context data item that will be passed
        to any callback routines that are called.  This can be whatever
        single stack item the caller program wants to pass, including an
        array or dictionary.

        MENU_OPTIONS_PROCESS will wait for user input, process it, and will
        only return when the user exits the menus.

        The format of the data in the optionsinfo array is a list of
        optioninfo description dictionaries.  Each optioninfo description
        can have the following fields:

            "group"    The name of the option group to group this option with.
                        This field is required.
            "name"     The name of this option, used to refer to its data.
                        This field is required.
            "type"     The type of this option.  May be one of "label",
                        "string", "password", "multistring", "boolean",
                        "integer", "float", "dbref", "dbreflist", "timespan",
                        "option" or "custom".  This field is required.
            "label"    The short text to label this option with.  Required.
            "help"     The human readable long help text for this option.
            "value"    The actual current value of this option.  Required.
            "minval"   For numeric options, the minimum legal value.
                        If not given, assumes 0.
            "maxval"   For numeric options, the maximum legal value.
                        If not given, assumes 99999999.
            "digits"   For float options, the number of significant digits.
                        If not given, assumes 0.
            "resolution" For float options, this specifies the smallest change
                        that can be made to the value.
            "options"  For option options, gives list of predefined values.
                        This is required for options of type option.
            "editable" For option options, if true, user can give arbitrary
                        values that aren't in the predefined value set.
                        If not given, assumes not editable.
            "objtype"  For dbref options, lists the allowed object types.
                        This is a list array of strings of valid types, and
                        can contain "any", "bad", "garbage", "player", "room",
                        "exit", "program", "thing", "zombie", and "vehicle".
                        If "thing" is given, "zombie" and "vehicle" are ok.
                        If not given, assumes any object type is ok.
            "vfy_cb"   Optional address of function to call back to verify
                        the value of this option before saving it.
            "save_cb"  Optional address of function to call back when saving
                        the value of this option via Apply or Done.

        When the menus are shown, the options will be re-ordered by group
        name.  The order of the options within each group will be determined
        by the order that they were given in the optionsinfo list.

        An optionsinfo list may look like:

            {
                {
                    "group"  "Self Destruct"
                    "grouplabel" "Settings for Armageddon."
                    "name"   "self_destruct_label"
                    "type"   "label"
                    "value"  "This is the emergency self destruct system, with convenient access for all protagonists."
                }dict
                {
                    "group"  "Self Destruct"
                    "name"   "destruct_armed"
                    "type"   "boolean"
                    "label"  "Arm self destruct system"
                    "help"   "This arms the self destruct system so that the countdown will be initiated if the big red button is pushed."
                    "value"  0
                }dict
                {
                    "group"   "Self Destruct"
                    "name"    "destruct_delay"
                    "type"    "timespan"
                    "label"   "Delay before self destruct"
                    "help"    "This specifies how long will elapse between initiation of the self destruct sequence, and when things go boom."
                    "minval"  60
                    "maxval"  86400
                    "save_cb" 'delay_save_callback
                    "vfy_cb"  'delay_verify_callback
                    "value"   300
                }dict
                {
                    "group"  "Self Destruct"
                    "name"   "destruct_password"
                    "type"   "string"
                    "label"  "Self destruct password"
                    "help"   "This is the password to the self destruct system."
                    "value"  "1A2B3"
                }dict
                {
                    "group"  "Self Destruct"
                    "name"   "destruct_warning"
                    "type"   "string"
                    "label"  "Self destruct initiation warning"
                    "help"   "This is the warning message to announce when the self destruct sequence is initiated."
                    "value"  "WARNING: SELF DESTRUCT SEQENCE INITIATED!"
                }dict
                {
                    "group"  "Minions"
                    "grouplabel" "Settings for evil minions."
                    "name"   "minion_count"
                    "type"   "integer"
                    "label"  "Number of minions"
                    "help"   "This specifies how many minions you control."
                    "minval" 0
                    "maxval" 99999
                    "value"  25
                }dict
                {
                    "group"  "Minions"
                    "name"   "minion_type"
                    "type"   "option"
                    "label"  "Type of minions"
                    "help"   "This defines the type of minions that you control."
                    "options" { "Ninjas" "Double Agents" "Triple Agents" "Zombies" "Drones" }list
                    "editable" 1
                    "value"  "Ninjas"
                }dict
                {
                    "group"   "Secret Base"
                    "grouplabel" "Options for your secret base."
                    "name"    "base_location"
                    "type"    "dbref"
                    "label"   "Secret Base Location"
                    "help"    "This is the room where your secret base is located."
                    "objtype" { "room" }list
                    "value"   #1234
                }dict
                {
                    "group"    "Secret Base"
                    "name"     "secret_base_value"
                    "type"     "float"
                    "label"    "Value of Secret Base in Millions"
                    "help"     "This is how much your secret base cost to build, in Millions of dollars."
                    "minval"   0.0
                    "maxval"   10000.0
                    "digits"   2
                    "value"    27.35
                }dict
                {
                    "group"   "Secret Base"
                    "name"    "secret_base_desc"
                    "type"    "multistring"
                    "label"   "Description of Secret Base"
                    "help"    "This is the description of your secret base.  It can be multiple lines long."
                    "value"   { "This is the" "base description." }list
                }dict
            }list

        The save_cb and vfy_cb fields above should contain the address of a
        function that has the following signature:
            [ dict:extradata str:optname any:value -- str:errs ]

        The extradata dictionary contains the following extra data:
            "user"         The user the menu was displayed to.
            "opts_id"      The optionsinfo identifier.
            "context"      The caller's context data.
            "optionsinfo"  Information on all the other options.

        The save_cb callback will only be called if the associated value
        has changed [or is a custom option] and Apply or Done were clicked.
        The vfy_cb callbacks will always be called when Apply or Done are
        clicked, regardless of whether the value has changed or not.

        The saveall_cb argument to MENU_OPTIONS_PROCESS is a callback
        function address that is called after all the option specific
        save_cb callbacks have been done.  This callback will only be
        called if all options passed their verifications.  It has the
        signature:
            [ dict:extradata dict:optionsinfo -- str:errs ]
        The extradata arg is the same as for the save_cb and vfy_cb callbacks.
        
        The context extradata argument will contain whatever you passed to
        the MENU_OPTIONS_PROCESS function in the caller_ctx argument.
        
        The optionsinfo argument given to callbacks will contain the
        optionsinfo array you provided to MENU_OPTIONS_PROCESS, re-indexed
        by option name, with updated "value", "oldvalue", and "changed"
        fields in each option description.  The old value of the each
        option will be in the "oldvalue" field.  The new value will be
        in the "value" field.  If the value changed, the "changed" field
        will be set to 1.

        ie:  The value of the optionsinfo argument may look like this:
            {
                "base_location" {
                    "group"   "Secret Base"
                    "name"    "base_location"
                    "type"    "dbref"
                    "label"   "Secret Base Location"
                    "help"    "This is the room where your secret base is located."
                    "objtype" { "room" }list
                    "value"    #1234
                    "oldvalue" #1234
                    "changed"  0
                }dict
                "secret_base_desc" {
                    "group"   "Secret Base"
                    "name"    "secret_base_desc"
                    "type"    "multistring"
                    "label"   "Description of Secret Base"
                    "help"    "This is the description of your secret base.  It can be multiple lines long."
                    "value"    { "This is the" "new base description." }list
                    "oldvalue" { "This is the" "old base description." }list
                    "changed"  1
                }dict
                ... etc ...
            }dict
        
        If the save was successful, the save and verify callback functions
        should return with a null string.  If there was an error, then the
        human readable error messages should be returned in a single string.

)

$def MENU_TITLEBAR_COLOR     "bold" textattr
$def MENU_TITLE_COLOR        "bold,green,bg_black" textattr
$def MENU_SELECTOR_COLOR     "bold,cyan,bg_black" textattr
$def MENU_ITEM_COLOR         "bold" textattr
$def MENU_INSTRUCTION_COLOR  "bold,yellow,bg_black" textattr
$def MENU_ERROR_COLOR        "bold,red,bg_black" textattr

$include $lib/case
$include $lib/editor
$include $lib/optionsinfo
$include $lib/optionsmisc

lvar caller_ctx 
lvar save_fn 
lvar main_title

: menu_title_notify[ str:title ref:who -- ]
    "=======================================" dup strcat
    dup strlen
    title @ ansi_strlen 4 + -
    dup 2 < if pop 2 then
    strcut pop
    dup strlen 2 / strcut
    "( " strcat MENU_TITLEBAR_COLOR swap
    " )" swap strcat MENU_TITLEBAR_COLOR
    title @ swap strcat strcat
    who @ swap notify
;

: menu_generic_help[ int:opts_id str:optname -- ]
    opts_id @ optionsinfo_get  var! optionsinfo
    optionsinfo @ optname @ [] var! item
    me @ var! who

    who @ "---------------------------------------" dup strcat notify
    item @ "name"  [] "Name: %s" fmtstring who @ swap notify
    item @ "label" [] "Label: %s" fmtstring who @ swap notify
    item @ "type"  [] "Type:  %s" fmtstring who @ swap notify

    item @ "value" []
    item @ "type" [] "timespan" stringcmp not if
        optmisc_timespan2str
    then
    item @ "type" [] "dbref" stringcmp not if
        who @ swap optmisc_dbref_unparse
    then
    item @ "type" [] "dbreflist" stringcmp not if
        { }list var! reflistset
        foreach swap pop
            who @ swap optmisc_dbref_unparse
            reflistset @ array_appenditem reflistset !
        repeat
        reflistset @ "\r" array_join
    then
    item @ "type" [] "boolean" stringcmp not if
        if "yes" else "no" then
    then
    item @ "type" [] "multistring" stringcmp not if
        pop "[Multi-line list]"
    then
    item @ "type" [] "password" stringcmp not if
        pop "[Password]"
    then
    item @ "type" [] "timespan" stringcmp not if
        "" var! spanstr
        dup 86400 / dup if
            swap 86400 % swap
            "%id " fmtstring
            spanstr @ swap strcat spanstr !
        else pop
        then
        dup 3600 /
        swap 3600 % swap
        dup 60 /
        swap 60 %
        swap rot
        "%02i:%02i:%02i" fmtstring
        spanstr @ swap strcat strip
    then
    item @ "value"  [] "Value:  %s" fmtstring who @ swap notify

    item @ "help"   [] dup not if pop "" then who @ swap notify
    who @ "---------------------------------------" dup strcat notify
    who @ "<Type 'c' to continue>" notify
    read pop
;


: menu_set_value[ int:opts_id str:optname str:optval -- str:errs ]
    me @ var! who
    opts_id @ optionsinfo_get  var! optionsinfo
    optionsinfo @ optname @ [] var! iteminfo

    0 var! stackdepth
    0 var! ctrlval
    0 var! minmaxchk
    "" var! errs

    iteminfo @ "type" [] var! ctrltype
    iteminfo @ "label" [] var! ctrllbl
    ctrltype @ case
        "password" stringcmp not when
            optval @ ctrlval !
        end
        "string" stringcmp not when
            optval @ ctrlval !
        end
        "multistring" stringcmp not when
            optval @ ctrlval !
        end
        "timespan" stringcmp not when
            0 var! days
            0 var! hours
            0 var! mins
            0 var! secs
            optval @
            "^ *(([0-9]+)d)? *(([0-9]+):)?(([0-9]+):)?([0-9]+) *$"
            REG_ICASE regexp array_count if
                dup 2 [] atoi days !
                dup 4 [] atoi hours !
                dup 6 [] atoi mins !
                dup 7 [] atoi secs !
            else
                optval @
                "^ *([0-9]+) *([dhms]) *$"
                REG_ICASE regexp array_count if
                    dup 1 []
                    over 2 [] "d" stringcmp not if 86400 * then
                    over 2 [] "h" stringcmp not if 3600 * then
                    over 2 [] "m" stringcmp not if 60 * then
                    secs !
                else
                    {
                        errs @ dup not if pop then
                        ctrllbl @ "Value for '%s' is invalid." fmtstring
                    }list "\r" array_join
                    errs !
                then
            then
            days @ 86400 * hours @ 3600 * + mins @ 60 * + secs @ +
            ctrlval !
            1 minmaxchk !
        end
        "integer" stringcmp not when
            optval @ atoi ctrlval !
            1 minmaxchk !
        end
        "float" stringcmp not when
            optval @ strtof ctrlval !
            1 minmaxchk !
        end
        "option" stringcmp not when
            optval @ ctrlval !
            iteminfo @ "editable" [] not if
                iteminfo @ "options" []
                ctrlval @ array_findval not if
                    {
                        errs @ dup not if pop then
                        ctrllbl @ "Value for '%s' is invalid." fmtstring
                    }list "\r" array_join
                    errs !
                then
            then
        end
        "dbref" stringcmp not when
            optval @
            who @ optmisc_dbref_parse ctrlval !
            who @ ctrlval @ iteminfo @
            optmisc_objtype_check
            dup if errs ! else pop then
            1 minmaxchk !
        end
        "dbreflist" stringcmp not when
            { }list ctrlval !
            optval @
            ";" explode_array
            foreach swap pop
                strip who @ optmisc_dbref_parse
                who @ over iteminfo @
                optmisc_objtype_check
                dup if errs ! pop break else pop then
                ctrlval @ array_appenditem ctrlval !
            repeat
        end
        "boolean" stringcmp not when
            optval @ tolower "{true|yes|on|t|y}" smatch if
                1 ctrlval !
            else 
                optval @ tolower "{false|no|off|f|n}" smatch if
                    0 ctrlval !
                else 
                    optval @ number? if
                        optval @ atoi
                        if 1 else 0 then
                        ctrlval !
                    else
                        {
                            errs @ dup not if pop then
                            ctrllbl @ "Value for '%s' is invalid." fmtstring
                        }list "\r" array_join
                        errs !
                    then
                then
            then
        end
        default
            optname @ ctrltype @ "Invalid control type '%s' for '%s'!" fmtstring abort
        end
    endcase

    ( check min/max constraints. )
    minmaxchk @ if
        ctrlval @ iteminfo @ "minval" [] < if
            {
                errs @ dup not if pop then
                ctrllbl @ "Value for '%s' is too low." fmtstring
            }list "\r" array_join
            errs !
        else
            ctrlval @
            iteminfo @ "maxval" []
            dup if
                > if
                    {
                        errs @ dup not if pop then
                        ctrllbl @ "Value for '%s' is too high." fmtstring
                    }list "\r" array_join
                    errs !
                then
            else pop pop
            then
        then
    then

    iteminfo @ "value" [] var! oldctrlval
    oldctrlval @ iteminfo @ "oldvalue" ->[] iteminfo !
    ctrlval    @ iteminfo @ "value"    ->[] iteminfo !

    oldctrlval @ ctrlval @ optmisc_equalvals? if 0 else 1 then
    iteminfo @ "changed" ->[] iteminfo !

    iteminfo @ optionsinfo @ optname @ ->[] optionsinfo !

    ( check verification callback function. )
    iteminfo @ "vfy_cb" [] var! vfy_fn
    vfy_fn @ address? if
        depth stackdepth !
        {
            "opts_id"     opts_id @
            "context"     caller_ctx @
            "optionsinfo" optionsinfo @
        }dict
        optname @
        iteminfo @ "value" []
        vfy_fn @ execute
        depth stackdepth dup ++ @ = not if
            depth stackdepth @ > if
                optname @
                "Verify callback for '%s' returned extra garbage on the stack."
            else
                optname @
                "Verify callback for '%s' was supposed to return a string."
            then
            fmtstring abort
        then
        dup string? not if
            optname @
            "Verify callback for '%s' was supposed to return a string."
            fmtstring abort
        then
        dup if
            (If we have an error, don't bother continuing. )
            dup errs ! exit
        else pop
        then
    then

    errs @ if
        ( if any verification failed, exit with errors )
        errs @ exit
    else
        ( call individual save callback )
        iteminfo @ "changed" []
        iteminfo @ "type" [] "custom" stringcmp not or
        if
            iteminfo @ "save_cb" [] var! itemsave_fn
            itemsave_fn @ address? if
                depth stackdepth !
                {
                    "opts_id"     opts_id @
                    "context"     caller_ctx @
                    "optionsinfo" optionsinfo @
                }dict
                optname @
                iteminfo @ "value" []
                itemsave_fn @ execute
                depth stackdepth dup ++ @ = not if
                    depth stackdepth @ > if
                        optname @
                        "Save callback for '%s' returned extra garbage on the stack."
                    else
                        optname @
                        "Save callback for '%s' was supposed to return a string."
                    then
                    fmtstring abort
                then
                dup string? not if
                    optname @
                    "Save callback for '%s' was supposed to return a string."
                    fmtstring abort
                then
                dup if
                    (If we have an error, exit. )
                    dup errs ! exit
                else
                    ( If no errors, remember changes )
                    opts_id @ optname @ "oldvalue" iteminfo @ "oldvalue" []
                    optionsinfo_set_indexed
                    opts_id @ optname @    "value" iteminfo @    "value" []
                    optionsinfo_set_indexed
                then
            then
        then
    
        ( call save callback function. )
        save_fn @ address? if
            depth stackdepth !
            {
                "opts_id"     opts_id @
                "context"     caller_ctx @
                "optionsinfo" optionsinfo @
            }dict
            optionsinfo @
            save_fn @ execute
            depth stackdepth dup ++ @ = not if
                depth stackdepth @ > if
                    "Save callback returned extra garbage on the stack."
                else
                    "Save callback was supposed to return a string."
                then
                abort
            then
            dup string? not if
                "Save callback was supposed to return a string."
                abort
            then
            errs !
        then

        errs @ if
            ( if save failed, return error. )
            errs @ exit
        else
            ( remember changes )
            optionsinfo @
            foreach iteminfo ! optname !
                opts_id @ optname @ "oldvalue" iteminfo @ "oldvalue" []
                optionsinfo_set_indexed
                opts_id @ optname @    "value" iteminfo @    "value" []
                optionsinfo_set_indexed
            repeat
        then
    then
    ""
;


( ------------------------------------------------------------- )


: menu_edit_item[ int:opts_id str:optname -- ]
    me @ var! who

    begin
        opts_id @ optname @ optioninfo_get var! item
        item @ "type" []
        case
            "password" stringcmp not when
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            "string" stringcmp not when
                item @ "value" []
                "Current value: " MENU_INSTRUCTION_COLOR swap strcat
                who @ swap notify
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            "multistring" stringcmp not when
                "Current value: " MENU_INSTRUCTION_COLOR
                who @ swap notify
                item @ "value" []
                { who @ }list array_notify
                who @ "< Now entering the list editor >" MENU_INSTRUCTION_COLOR notify
                item @ "value" [] array_vals editor
                dup "abort" stringcmp not if
                    pop exit
                else
                    pop array_make
                    opts_id @ optname @ rot
                    menu_set_value
                then
            end
            "custom" stringcmp not when
                "Custom item type not supported as yet in menus."
                abort
                (
                    item @ "create_cb" [] address? if
                        dscr @ caller_ctx @
                        item @ "create_cb" [] execute
                    else
                        item @ "name" []
                        "Bad create_cb callback for option '%s'."
                        fmtstring abort
                    then
                )
            end
            "integer" stringcmp not when
                item @ "value" []
                "Current value: " MENU_INSTRUCTION_COLOR swap intostr strcat
                who @ swap notify
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            "float" stringcmp not when
                item @ "value" []
                "Current value: " MENU_INSTRUCTION_COLOR swap ftostr strcat
                who @ swap notify
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            "dbref" stringcmp not when
                item @ "value" []
                who @ swap optmisc_dbref_unparse
                "Current value: " MENU_INSTRUCTION_COLOR swap strcat
                who @ swap notify
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            "dbreflist" stringcmp not when
                { }list var! reflistset
                "Current value: " MENU_INSTRUCTION_COLOR
                who @ swap notify
                item @ "value" []
                foreach swap pop
                    who @ swap optmisc_dbref_unparse
                    who @ over notify
                    reflistset @ array_appenditem reflistset !
                repeat
                reflistset @
                who @ "< Now entering the list editor >" MENU_INSTRUCTION_COLOR notify
                array_vals editor
                dup "abort" stringcmp not if
                    who @ "< aborting changes >" notify
                    pop exit
                else
                    pop array_make
                    opts_id @ optname @ rot
                    menu_set_value
                then
            end
            "option" stringcmp not when
                item @ "value" []
                "Current value: " MENU_INSTRUCTION_COLOR swap strcat
                who @ swap notify
                item @ "options" [] ", " array_join
                item @ "editable" [] if
                    "Some common values are: "
                    MENU_INSTRUCTION_COLOR
                    swap strcat
                    who @ swap notify
                else
                    "Valid options are: "
                    MENU_INSTRUCTION_COLOR
                    swap strcat
                    who @ swap notify
                then
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            "boolean" stringcmp not when
                item @ "value" []
                if "yes" else "no" then
                "Current value: " MENU_INSTRUCTION_COLOR swap strcat
                who @ swap notify
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            "timespan" stringcmp not when
                "" var! spanstr
                item @ "value" []
                dup 86400 / dup if
                    swap 86400 % swap
                    "%id " fmtstring
                    spanstr @ swap strcat spanstr !
                else pop
                then
                dup 3600 /
                swap 3600 % swap
                dup 60 /
                swap 60 %
                swap rot
                "%02i:%02i:%02i" fmtstring
                spanstr @ swap strcat strip
                "Current value: " MENU_INSTRUCTION_COLOR swap strcat
                who @ swap notify
                who @ "Please enter the new value." MENU_INSTRUCTION_COLOR notify
                read
                opts_id @ optname @ rot
                menu_set_value
            end
            default
                item @ "name" []
                item @ "type" []
                "Unknown option type '%s' for option '%s'!"
                fmtstring abort
            end
        endcase
        dup not if pop break then
        MENU_ERROR_COLOR
        who @ swap notify
    repeat
;


: menu_single_group[ int:opts_id str:group -- int:quit? ]
    me @ var! who
    { }list var! itemnames
    opts_id @ group @ optionsinfo_get_group_opts var! groupopts

    begin
        0 var! cnt
        who @ " " notify
        {
            main_title @
            group @ dup not if pop then
        }list " - " array_join var! grouptitle

        grouptitle @ MENU_TITLE_COLOR
        who @ menu_title_notify

        groupopts @
        foreach var! optname pop
            opts_id @ optname @ optioninfo_get var! item
            item @ "name" [] itemnames @ array_appenditem itemnames !
            item @ "type" [] "label" stringcmp not if
                who @
                item @ "value" []
                MENU_ITEM_COLOR
                notify
            else
                item @ "type" []
                case
                    "password" stringcmp not when
                        "[HIDDEN]"
                    end
                    "string" stringcmp not when
                        item @ "value" []
                        dup strlen
                        item @ "label" [] strlen + 5 + 78 > if
                            pop "[string]"
                        then
                    end
                    "multistring" stringcmp not when
                        "[multi-line]"
                    end
                    "custom" stringcmp not when
                        "Custom item type not supported as yet in menus."
                        abort
                        (
                            item @ "create_cb" [] address? if
                                dscr @ caller_ctx @
                                item @ "create_cb" [] execute
                            else
                                item @ "name" []
                                "Bad create_cb callback for option '%s'."
                                fmtstring abort
                            then
                        )
                    end
                    "integer" stringcmp not when
                        item @ "value" [] intostr
                    end
                    "float" stringcmp not when
                        item @ "value" [] ftostr
                    end
                    "dbref" stringcmp not when
                        who @ item @ "value" [] optmisc_dbref_unparse
                    end
                    "dbreflist" stringcmp not when
                        item @ "value" []
                        { }list var! reflistset
                        foreach swap pop
                            who @ swap optmisc_dbref_unparse
                            reflistset @ array_appenditem reflistset !
                        repeat
                        reflistset @ ";" array_join
                    end
                    "option" stringcmp not when
                        item @ "value" []
                    end
                    "boolean" stringcmp not when
                        item @ "value" []
                        if "yes" else "no" then
                    end
                    "timespan" stringcmp not when
                        "" var! spanstr
                        item @ "value" []
                        dup 86400 / dup if
                            swap 86400 % swap
                            "%id " fmtstring
                            spanstr @ swap strcat spanstr !
                        else pop
                        then
                        dup 3600 /
                        swap 3600 % swap
                        dup 60 /
                        swap 60 %
                        swap rot
                        "%02i:%02i:%02i" fmtstring
                        spanstr @ swap strcat strip
                    end
                    default
                        item @ "name" []
                        item @ "type" []
                        "Unknown option type '%s' for option '%s'!"
                        fmtstring abort
                    end
                endcase

                item @ "label" [] MENU_ITEM_COLOR
                cnt @ ++
                "%3i" fmtstring MENU_SELECTOR_COLOR
                "%s) %s: %s" fmtstring
                who @ swap notify

            then
            cnt ++
        repeat

        "Please enter an item number, '" MENU_INSTRUCTION_COLOR
        "B" MENU_SELECTOR_COLOR strcat
        "' to go back a menu, or '" MENU_INSTRUCTION_COLOR strcat
        "Q" MENU_SELECTOR_COLOR strcat
        "' to quit." MENU_INSTRUCTION_COLOR strcat
        who @ menu_title_notify

        read tolower
        dup number? if
            atoi dup itemnames @ array_count >
            over 1 < or if
                who @ "That's not a valid menu item!" MENU_ERROR_COLOR notify
            else
                itemnames @ swap -- []
                opts_id @ swap
                menu_edit_item
            then
        else
            dup "b" stringcmp not if pop 0 exit then
            dup "q" stringcmp not if pop 1 exit then
            pop "Invalid selection." MENU_ERROR_COLOR
            who @ swap notify
        then
    repeat
;


: menu_multiple_groups[ arr:opts_id str:basegroup -- int:quit? ]
    me @ var! who
    opts_id @ optionsinfo_getgroups var! groups

    begin
        who @ " " notify
        {
            main_title @
            basegroup @ dup not if pop then
        }list " - " array_join var! grouptitle

        grouptitle @ MENU_TITLE_COLOR
        who @ menu_title_notify

        groups @
        foreach var! group var! cnt
            opts_id @ group @ optionsinfo_getgrouplabel
            dup if group @ ": " strcat swap strcat then
            MENU_ITEM_COLOR
            cnt @ ++
            "%3i" fmtstring MENU_SELECTOR_COLOR
            "%s) %s" fmtstring
            who @ swap notify
        repeat

        "Please enter an item number" MENU_INSTRUCTION_COLOR
        basegroup @ if
            ", '" MENU_INSTRUCTION_COLOR strcat
            "B" MENU_SELECTOR_COLOR strcat
            "' to go back a menu" MENU_INSTRUCTION_COLOR strcat
        then
        ", or '" MENU_INSTRUCTION_COLOR strcat
        "Q" MENU_SELECTOR_COLOR strcat
        "' to quit." MENU_INSTRUCTION_COLOR strcat
        who @ menu_title_notify

        read tolower
        dup number? if
            atoi dup groups @ array_count >
            over 1 < or if
                who @ "That's not a valid menu item!" MENU_ERROR_COLOR notify
            else
                opts_id @ groups @ rot -- []
                menu_single_group
                if 1 exit then
            then
        else
            dup "b" stringcmp not if pop 0 exit then
            dup "q" stringcmp not if pop 1 exit then
            pop "Invalid selection." MENU_ERROR_COLOR
            who @ swap notify
        then
    repeat
;


( ------------------------------------------------------------- )


: menu_options_process[ any:context addr:save_cb str:title arr:optionsinfo -- ]
    context @ caller_ctx !
    save_cb @ save_fn !
    title @ main_title !
    read_wants_blanks
    optionsinfo @ optionsinfo_new var! opts_id
    opts_id @ optionsinfo_getgroups var! groups
    groups @ array_count 1 > if
        opts_id @ ""
        menu_multiple_groups pop
    else
        opts_id @ groups @ 0 []
        menu_single_group pop
    then
    me @ "< Done. >" MENU_INSTRUCTION_COLOR notify
;
PUBLIC menu_options_process
$libdef MENU_OPTIONS_PROCESS
.
c
q
@register lib-optionsmenu=lib/optionsmenu
@register #me lib-optionsmenu=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3

