@prog lib-optionsgui
1 99999 d
1 i
( $lib/optionsgui  Copyright 7/22/2002 by Revar <revar@belfry.com> )
( Released under the GNU LGPL.                                     )
(                                                                  )
( A MUF library to automatically create dialogs to edit a set of   )
( option values.                                                   )
(                                                                  )
( v1.000 --  7/22/2002 -- Revar <revar@belfry.com>                 )
(     Initial relase.                                              )
( v1.005 --  7/30/2002 -- Revar <revar@belfry.com>                 )
(     Changed callback arguments.                                  )
( v1.100 --  8/02/2002 -- Revar <revar@belfry.com>                 )
(     Majorly reworked interface.  Many bugfixes.                  )
( v1.101 --  8/13/2002 -- Revar <revar@belfry.com>                 )
(     Bigfix for the help dialogs sent to other descriptors.       )
( v1.102 --  8/20/2002 -- Revar <revar@belfry.com>                 )
(     Made dbref options use combobox.                             )
( v1.103 --  8/29/2002 -- Revar <revar@belfry.com>                 )
(     Made dbref options update combobox with matched object name. )
( v1.104 --  8/18/2003 -- Revar <revar@belfry.com>                 )
(     Made fallback to $lib/optionsmenu in textmode.               )
(     Added opts_id to callback data.                              )
(     Split optionsinfo calls into $lib/optionsinfo.               )

$author Revar Desmera <revar@belfry.com>
$version 1.104
$lib-version 1.102
$note Released under the LGPL.

(

    GUI_OPTIONS_GENERATE[ int:dscr  any:caller_ctx  addr:saveall_cb
                        str:title arr:optionsinfo -- int:opts_id ]

        Generates a dialog or set of dialogs that will edit a set of options
        that are described by the optionsinfo array.  The dialog[s] will be
        created with the given title root, and displayed to the given dscr
        descriptor connection.  The caller program may provide the address
        to a saveall_cb callback function that will be called when the user
        saves the changes.  If saveall_cb is 0, this call will not be made.
        The caller program also provides a caller_ctx context data item that
        will be passed to any callback routines that are called.  This can
        be whatever single stack item the caller program wants to pass,
        including an array or dictionary.

        This function will return the opts_id of the current options set.
        You must call GUI_OPTIONS_FREE with this opts_id after all the
        dialogs have been closed.

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
            "create_cb" For custom options, this is the address of the
                        function to call to get the controls description
                        used to created the controls needed by this custom
                        option.
            "vfy_cb"   Optional address of function to call back to verify
                        the value of this option before saving it.
            "save_cb"  Optional address of function to call back when saving
                        the value of this option via Apply or Done.

        When the dialog is created, the options will be re-ordered by group
        name.  The order of the options within each group will be determined
        by the order that they were given in the optionsinfo list.

        An optionsinfo list may look like:

            {
                {
                    "group"  "Self Destruct"
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
                {
                    "group"     "Laboratory"
                    "name"      "lab_ctrls"
                    "type"      "custom"
                    "create_cb" 'lab_ctrls_create
                    "save_cb"   'lab_ctrls_save
                    "vfy_cb"    'lab_ctrls_verify
                    "value"     0
                }dict
            }list

        The create_cb field above should contain the address of a function
        that has the following signature:
            [ int:dscr any:context -- arr:ctrlsdescr ]
        The return value should be a valid $lib/gui control set description
        array with all neccesary callbacks, etc.

        The save_cb and vfy_cb fields above should contain the address of a
        function that has the following signature:
            [ dict:extradata str:optname any:value -- str:errs ]

        The extradata dictionary contains the following extra data:
            "descr"        The descriptor the dialog was displayed to.
            "dlogid"       The dialog's ID.
            "opts_id"      The optionsinfo identifier.
            "context"      The caller's context data.
            "optionsinfo"  Information on all the other options.

        The save_cb callback will only be called if the associated value
        has changed [or is a custom option] and Apply or Done were clicked.
        The vfy_cb callbacks will always be called when Apply or Done are
        clicked, regardless of whether the value has changed or not.

        The saveall_cb argument to GUI_OPTIONS_PROCESS is a callback function
        address that is called after all the option specific save_cb call-
        backs have been done.  This callback will only be called if all
        options passed their verifications.  It has the signature:
            [ dict:extradata dict:optionsinfo -- str:errs ]
        The extradata arg is the same as for the save_cb and vfy_cb callbacks.
        
        The context extradata argument will contain whatever you passed to
        the GUI_OPTIONS_PROCESS function in the caller_ctx argument.
        
        The optionsinfo argument given to callbacks will contain the
        optionsinfo array you provided to GUI_OPTION_PROCESS, re-indexed
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



    GUI_OPTIONS_VALUE_SET[ int:opts_id str:optname any:value -- ]
        Sets the value of the given option within the gui option editor
        specified by opts_id.  This change will be shown to the user in
        any gui option editor dialogs she may have up at the moment.



    GUI_OPTIONS_FREE[ int:opts_id -- ]
        Frees up all internal memory associated with the gui options editor.



    GUI_OPTIONS_PROCESS[ int:dscr  any:caller_ctx  addr:saveall_cb
                        str:title  arr:optionsinfo -- ]

        GUI_OPTIONS_PROCESS will call GUI_OPTIONS_GENERATE, go into the
        background, wait for the user to dismiss the dialogs it creates,
        then call GUI_OPTIONS_FREE and return.

)


$include $lib/case
$include $lib/gui
$include $lib/optionsinfo
$include $lib/optionsmisc
$include $lib/optionsmenu

$def DEFAULT_CAPTION "Edit the following data and click on 'Done' or 'Apply' to commit the changes."

( ------------------------------------------------------------- )


: gui_dlog_generic_help[ int:dscr dict:item -- ]
    dscr @ descrdbref var! who

    {SIMPLE_DLOG item @ "name" [] "Help for %s" fmtstring
        {LABEL "optnamel"
            "value"   "Name:"
            "newline" 0
            "sticky"  "w"
        }CTRL
        {LABEL "optname"
            "value"  item @ "name" []
            "newline" 1
            "sticky"  "w"
            "toppad"  0
        }CTRL
        {LABEL "optlbll"
            "value"   "Label:"
            "newline" 0
            "sticky"  "w"
        }CTRL
        {LABEL "optlbl"
            "value"   item @ "label" []
            "newline" 1
            "maxwidth" 400
            "sticky"  "w"
            "toppad"  0
        }CTRL
        {LABEL "opttypel"
            "value"   "Type:"
            "newline" 0
            "sticky"  "w"
        }CTRL
        {LABEL "opttype"
            "value"   item @ "type" []
            "newline" 1
            "sticky"  "w"
            "toppad" 0
        }CTRL
        {LABEL "optvall"
            "value"   "Value:"
            "newline" 0
            "sticky"  "w"
        }CTRL
        {LABEL "optval"
            "value"   item @ "value" []
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
                item @ "type" [] "password" stringcmp not if
                    pop ""
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
            "newline"  1
            "sticky"   "w"
            "maxwidth" 400
            "hweight"  1
            "toppad"   0
        }CTRL
        {LABEL "opthelp"
            "value"   item @ "help" [] dup not if pop "" then
            "colspan"  2
            "newline"  1
            "maxwidth" 500
            "sticky"   "w"
        }CTRL
        {HRULE "hrule1"
            "sticky"  "ew"
            "colspan" 2
            "newline" 1
        }CTRL
        {FRAME "btnfr"
            "sticky"  "e"
            "colspan" 2
            {BUTTON "done"
                "text"    "&Done"
                "width"   6
                "sticky"  "e"
                "dismiss" 1
            }CTRL
        }CTRL
    }DLOG
    dscr @ swap GUI_GENERATE
    dup GUI_DLOG_SHOW
    swap gui_dlog_register
;


    
: gui_dlog_generic_cancel_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ gui_dlog_deregister
    0
;


: gui_dlog_generic_help_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    context @ "descr" [] var! dscr
    context @ "values" [] var! vals
    context @ "statedata" [] var! statedata
    statedata @ "title" [] var! title
    statedata @ "opts_id" [] var! opts_id

    ctrlid @ "name_" "helpbtn_" subst
    vals @ swap [] 0 [] var! ctrlname

    dscr @
    opts_id @ ctrlname @ optioninfo_get
    gui_dlog_generic_help
    0
;


: gui_dlog_generic_save_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    context @ "descr" [] var! dscr
    context @ "values" [] var! vals
    context @ "statedata" [] var! statedata
    statedata @ "opts_id" [] var! opts_id
    statedata @ "save_cb" [] var! save_fn
    statedata @ "caller_ctx" [] var! caller_ctx

    opts_id @ optionsinfo_get  var! optionsinfo

    0 var! stackdepth
    0 var! ctrlval
    0 var! minmaxchk
    "" var! errs
    dscr @ descrdbref var! who

    vals @ "ctrl_cnt" [] 0 [] atoi
    0 swap -- 1 for var! cnt
        0 minmaxchk !
        vals @ cnt @ "name_%03i" fmtstring []
        dup if
            0 [] var! ctrlname
            optionsinfo @ ctrlname @ [] var! iteminfo
            iteminfo @ "type" [] var! ctrltype
            iteminfo @ "label" [] var! ctrllbl
            ctrltype @ case
                "password" stringcmp not when
                    vals @ cnt @ "value_%03i" fmtstring [] 0 [] ctrlval !
                end
                "string" stringcmp not when
                    vals @ cnt @ "value_%03i" fmtstring [] 0 [] ctrlval !
                end
                "multistring" stringcmp not when
                    vals @ cnt @ "value_%03i" fmtstring [] ctrlval !
                end
                "timespan" stringcmp not when
                    vals @ cnt @ "value_days_%03i" fmtstring [] 0 [] atoi 24 *
                    vals @ cnt @ "value_hrs_%03i"  fmtstring [] 0 [] atoi + 60 *
                    vals @ cnt @ "value_mins_%03i" fmtstring [] 0 [] atoi + 60 *
                    vals @ cnt @ "value_secs_%03i" fmtstring [] 0 [] atoi +
                    ctrlval !
                    1 minmaxchk !
                end
                "integer" stringcmp not when
                    vals @ cnt @ "value_%03i" fmtstring [] 0 [] atoi ctrlval !
                    1 minmaxchk !
                end
                "float" stringcmp not when
                    vals @ cnt @ "value_%03i" fmtstring [] 0 [] strtof ctrlval !
                    1 minmaxchk !
                end
                "option" stringcmp not when
                    vals @ cnt @ "value_%03i" fmtstring [] 0 [] ctrlval !
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
                    vals @ cnt @ "value_%03i" fmtstring [] 0 []
                    who @ optmisc_dbref_parse ctrlval !
                    who @ ctrlval @ iteminfo @
                    optmisc_objtype_check
                    dup if errs ! else pop then
                    1 minmaxchk !
                end
                "dbreflist" stringcmp not when
                    { }list ctrlval !
                    vals @ cnt @ "value_%03i" fmtstring [] 0 []
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
                    vals @ cnt @ "value_%03i" fmtstring [] 0 [] atoi
                    if 1 else 0 then ctrlval !
                end
                default
                    ctrlname @ ctrltype @ "Invalid control type '%s' for '%s'!" fmtstring abort
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
                    dup not if pop 99999999 then
                    > if
                        {
                            errs @ dup not if pop then
                            ctrllbl @ "Value for '%s' is too high." fmtstring
                        }list "\r" array_join
                        errs !
                    then
                then
            then
    
            iteminfo @ "value" [] var! oldctrlval
            oldctrlval @ iteminfo @ "oldvalue" ->[] iteminfo !
            ctrlval    @ iteminfo @ "value"    ->[] iteminfo !
    
            oldctrlval @ ctrlval @ optmisc_equalvals? if 0 else 1 then
            iteminfo @ "changed" ->[] iteminfo !
    
            iteminfo @ optionsinfo @ ctrlname @ ->[] optionsinfo !
    
            ( check verification callback function. )
            iteminfo @ "vfy_cb" [] var! vfy_fn
            vfy_fn @ address? if
                depth stackdepth !
                {
                    "descr"       dscr @
                    "opts_id"     opts_id @
                    "dlogid"      dlogid @
                    "context"     caller_ctx @
                    "optionsinfo" optionsinfo @
                }dict
                ctrlname @
                iteminfo @ "value" []
                vfy_fn @ execute
                depth stackdepth dup ++ @ = not if
                    depth stackdepth @ > if
                        ctrlname @
                        "Verify callback for '%s' returned extra garbage on the stack."
                    else
                        ctrlname @
                        "Verify callback for '%s' was supposed to return a string."
                    then
                    fmtstring abort
                then
                dup string? not if
                    ctrlname @
                    "Verify callback for '%s' was supposed to return a string."
                    fmtstring abort
                then
                dup if
                    (If we have an error, don't bother continuing. )
                    errs ! break
                else pop
                then
            then
            errs @ if break then
            ctrltype @ "dbref" stringcmp not if
                dlogid @ cnt @ "value_%03i" fmtstring
                who @ iteminfo @ "value" [] optmisc_dbref_unparse
                GUI_VALUE_SET
                (
                dlogid @ cnt @ "value_%03i" fmtstring
                "cursor" { "position" "end" }dict
                GUI_CTRL_COMMAND
                )
            then
        else pop
        then
    repeat

    errs @ if
        ( if any verification failed, tell the user and keep dlog up. )
        dlogid @ "errtext" errs @ gui_value_set
    else
        ( call individual save callbacks )
        optionsinfo @
        foreach iteminfo ! ctrlname !
            iteminfo @ "changed" []
            iteminfo @ "type" [] "custom" stringcmp not or
            if
                iteminfo @ "save_cb" [] var! itemsave_fn
                itemsave_fn @ address? if
                    depth stackdepth !
                    {
                        "descr"       dscr @
                        "opts_id"     opts_id @
                        "dlogid"      dlogid @
                        "context"     caller_ctx @
                        "optionsinfo" optionsinfo @
                    }dict
                    ctrlname @
                    iteminfo @ "value" []
                    itemsave_fn @ execute
                    depth stackdepth dup ++ @ = not if
                        depth stackdepth @ > if
                            ctrlname @
                            "Save callback for '%s' returned extra garbage on the stack."
                        else
                            ctrlname @
                            "Save callback for '%s' was supposed to return a string."
                        then
                        fmtstring abort
                    then
                    dup string? not if
                        ctrlname @
                        "Save callback for '%s' was supposed to return a string."
                        fmtstring abort
                    then
                    dup if
                        (If we have an error, don't bother continuing. )
                        errs ! break
                    else
                        ( If no errors, remember changes )
                        opts_id @ ctrlname @ "oldvalue" iteminfo @ "oldvalue" []
                        optionsinfo_set_indexed
                        opts_id @ ctrlname @    "value" iteminfo @    "value" []
                        optionsinfo_set_indexed
                    then
                then
            then
        repeat
    
        ( call save callback function. )
        save_fn @ address? if
            depth stackdepth !
            {
                "descr"       dscr @
                "opts_id"     opts_id @
                "dlogid"      dlogid @
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
            ( if save failed, tell the user and keep dlog up. )
            dlogid @ "errtext" errs @ gui_value_set
        else
            ( remember changes )
            optionsinfo @
            foreach iteminfo ! ctrlname !
                opts_id @ ctrlname @ "oldvalue" iteminfo @ "oldvalue" []
                optionsinfo_set_indexed
                opts_id @ ctrlname @    "value" iteminfo @    "value" []
                optionsinfo_set_indexed
            repeat
            
            ctrlid @ "apply" stringcmp if
                dlogid @ gui_dlog_deregister
                dlogid @ gui_dlog_close
            else
                dlogid @ "errtext" DEFAULT_CAPTION gui_value_set
            then
        then
    then

    dlogid @
    {
        "opts_id" opts_id @
        "caller_ctx" caller_ctx @
        "save_cb" save_fn @
    }dict GUI_DLOG_STATEDATA_SET

    0
;


( ------------------------------------------------------------- )


: singlegroup_ctrls_generate[ int:dscr int:startnum any:context int:opts_id str:group -- arr:ctrlsdesc int:optcount ]
    var maxval
    var minval
    5 var! toppad
    dscr @ descrdbref var! who

    {
        opts_id @ group @ optionsinfo_get_group_opts var! groupopts
        groupopts @
        startnum @ var! cnt
        foreach var! optname pop
            opts_id @ optname @ optioninfo_get var! item
            item @ "type" [] "label" stringcmp not if
                {LABEL ""
                    "value"   item @ "value" []
                    "maxwidth" 500
                    "newline" 1
                    "sticky"  "w"
                    "colspan" 3
                }CTRL
            else
                {DATUM cnt @ "name_%03i" fmtstring
                    "value" item @ "name" []
                }CTRL
                {IMAGE cnt @ "helpbtn_%03i" fmtstring
                    "value"   "http://www.belfry.com/pics/info.gif"
                    "width"   9
                    "height"  9
                    "sticky"  "w"
                    "toppad"  toppad @
                    "hweight" 0
                    "newline" 0
                    "report"  1
                    "|buttonpress" 'gui_dlog_generic_help_cb
                }CTRL
                item @ "type" []
                case
                    "password" stringcmp not when
                        {LABEL ""
                            "value"   item @ "label" []
                            "newline" 0
                            "sticky"  "w"
                        }CTRL
                        {PASSWORD cnt @ "value_%03i" fmtstring
                            "value" item @ "value" []
                            "sticky" "ew"
                            "width"  item @ "width" [] dup not if pop 40 then
                            "hweight" 100
                            "newline" 1
                            "toppad"  toppad @
                        }CTRL
                    end
                    "string" stringcmp not when
                        {LABEL ""
                            "value"   item @ "label" []
                            "newline" 0
                            "sticky"  "w"
                        }CTRL
                        {EDIT cnt @ "value_%03i" fmtstring
                            "value" item @ "value" []
                            "sticky" "ew"
                            "width"  item @ "width" [] dup not if pop 40 then
                            "hweight" 100
                            "newline" 1
                            "toppad"  toppad @
                        }CTRL
                    end
                    "multistring" stringcmp not when
                        {LABEL ""
                            "value"   item @ "label" []
                            "newline" 1
                            "colspan" 2
                            "hweight" 1
                            "sticky"  "w"
                            "toppad"  toppad @
                        }CTRL
                        {FRAME cnt @ "filler%003i" fmtstring
                            "newline" 0
                        }CTRL
                        {MULTIEDIT cnt @ "value_%03i" fmtstring
                            "value" item @ "value" []
                            "sticky" "nsew"
                            "width"   item @ "width"  [] dup not if pop 60 then
                            "height"  item @ "height" [] dup not if pop  5 then
                            "colspan" 2
                            "hweight" 1
                            "vweight" 1
                            "newline" 1
                            "toppad"  toppad @
                        }CTRL
                    end
                    "custom" stringcmp not when
                        {LABEL ""
                            "value"   item @ "label" []
                            "newline" 1
                            "colspan" 2
                            "hweight" 1
                            "sticky"  "w"
                            "toppad"  toppad @
                        }CTRL
                        {FRAME cnt @ "filler%003i" fmtstring
                            "newline" 0
                        }CTRL
                        {FRAME cnt @ "framer%003i" fmtstring
                            "newline" 0
                            "colspan" 2
                            "hweight" 1
                            "vweight" 1
                            "newline" 1
                            "sticky" "nsew"
                            item @ "create_cb" [] address? if
                                dscr @ context @
                                item @ "create_cb" [] execute
                            else
                                item @ "name" []
                                "Bad create_cb callback for option '%s'."
                                fmtstring abort
                            then
                        }CTRL
                    end
                    "integer" stringcmp not when
                        {LABEL ""
                            "value"  item @ "label" []
                            "newline" 0
                            "sticky" "w"
                        }CTRL
                        {SPINNER cnt @ "value_%03i" fmtstring
                            item @ "maxval" [] dup not if pop 999999999 then
                            var! smaxval
                            item @ "width"  [] dup not if
                                pop smaxval @ float log10 fabs 2 + int
                            then
                            var! swidth
                            "value" item @ "value" []
                            "maxval" smaxval @
                            "minval" item @ "minval" [] dup not if pop 0 then
                            "sticky" "w"
                            "width"  swidth @
                            "hweight" 100
                            "newline" 1
                            "toppad"  toppad @
                        }CTRL
                    end
                    "float" stringcmp not when
                        {LABEL ""
                            "value"  item @ "label" []
                            "newline" 0
                            "sticky" "w"
                        }CTRL
                        {SCALE cnt @ "value_%03i" fmtstring
                            "value"    item @ "value" []
                            "digits"   item @ "digits" []
                            "maxval"   item @ "maxval" [] dup not if pop 100.0 then
                            "minval"   item @ "minval" [] dup not if pop 0.0 then
                            "resolution" item @ "resolution" [] dup not if pop 0.1 then
                            "interval" 6 pick 5 pick - 4.0 /
                            "sticky"   "ew"
                            "length"    200
                            "hweight"  100
                            "newline"  1
                            "toppad"   toppad @
                        }CTRL
                    end
                    "dbref" stringcmp not when
                        {LABEL ""
                            "value"  item @ "label" []
                            "newline" 0
                            "sticky" "w"
                        }CTRL
                        {COMBOBOX cnt @ "value_%03i" fmtstring
                            "value"   who @ item @ "value" [] optmisc_dbref_unparse
                            "options" who @ item @ optmisc_dbref_option_list
                            "editable" 1
                            "sticky" "w"
                            "width"  30
                            "hweight" 100
                            "newline" 1
                            "toppad"  toppad @
                        }CTRL
                    end
                    "dbreflist" stringcmp not when
                        {LABEL ""
                            "value"  item @ "label" []
                            "newline" 0
                            "sticky" "w"
                        }CTRL
                        {COMBOBOX cnt @ "value_%03i" fmtstring
                            "value"  item @ "value" []
                                { }list var! reflistset
                                foreach swap pop
                                    who @ swap optmisc_dbref_unparse
                                    reflistset @ array_appenditem reflistset !
                                repeat
                                reflistset @ ";" array_join
                            "options" who @ item @ optmisc_dbref_option_list
                            "editable" 1
                            "sticky" "w"
                            "width"  30
                            "hweight" 100
                            "newline" 1
                            "toppad"  toppad @
                        }CTRL
                    end
                    "option" stringcmp not when
                        {LABEL ""
                            "value"   item @ "label" []
                            "newline" 0
                            "sticky"  "w"
                        }CTRL
                        {COMBOBOX cnt @ "value_%03i" fmtstring
                            "value"    item @ "value" []
                            "editable" item @ "editable" []
                            "sorted"   item @ "sorted" []
                            "options"  item @ "options" []
                            "sticky"   "w"
                            item @ "editable" [] if
                                "width"    30
                            then
                            "hweight"   100
                            "newline"   1
                            "toppad"    toppad @
                        }CTRL
                    end
                    "boolean" stringcmp not when
                        {CHECKBOX cnt @ "value_%03i" fmtstring
                            "text"  item @ "label" []
                            "value" item @ "value" []
                            "sticky" "w"
                            "colspan" 2
                            "newline" 1
                            "hweight" 1
                            "toppad"  toppad @
                        }CTRL
                    end
                    "timespan" stringcmp not when
                        {LABEL ""
                            "value"  item @ "label" []
                            "newline" 0
                            "sticky" "w"
                        }CTRL
                        {FRAME cnt @ "timefr_%03i" fmtstring
                            "sticky"  "w"
                            "newline" 1
                            "hweight" 100
                            "toppad"  toppad @
                            item @ "maxval" [] dup not if pop 999999999 then maxval !
                            item @ "minval" [] dup not if pop 0 then minval !
                            maxval @ 86400 >= if
                                {SPINNER cnt @ "value_days_%03i" fmtstring
                                    "value" item @ "value" [] 86400 /
                                    "maxval"  maxval @ 86400 /
                                    "minval"  minval @ 86400 /
                                    "sticky" "w"
                                    "width"   4
                                    "hweight" 0
                                    "newline" 0
                                }CTRL
                                {LABEL ""
                                    "value" "Days"
                                    "newline" 0
                                    "leftpad" 2
                                    "sticky" "w"
                                }CTRL
                            then
                            maxval @ 3600 >= if
                                {SPINNER cnt @ "value_hrs_%03i" fmtstring
                                    "value" item @ "value" [] 3600 / 24 %
                                    "maxval" maxval @ 3600 / dup 23 > if pop 23 then
                                    maxval @ minval @ - 86400 <
                                    maxval @ 86400 % minval @ 86400 % > and if
                                        "minval" minval @ 3600 /
                                    then
                                    "sticky" "w"
                                    "width"   2
                                    "hweight" 0
                                    "newline" 0
                                }CTRL
                                {LABEL ""
                                    "value" "Hrs"
                                    "newline" 0
                                    "leftpad" 2
                                    "sticky" "w"
                                }CTRL
                            then
                            maxval @ 60 >= if
                                {SPINNER cnt @ "value_mins_%03i" fmtstring
                                    "value" item @ "value" [] 60 / 60 %
                                    "maxval"  maxval @ 60 / dup 59 > if pop 59 then
                                    maxval @ minval @ - 3600 <
                                    maxval @ 3600 % minval @ 3600 % > and if
                                        "minval" minval @ 60 /
                                    then
                                    "sticky" "w"
                                    "width"   2
                                    "hweight" 0
                                    "newline" 0
                                }CTRL
                                {LABEL ""
                                    "value" "Min"
                                    "newline" 0
                                    "leftpad" 2
                                    "sticky" "w"
                                }CTRL
                            then
                            {SPINNER cnt @ "value_secs_%03i" fmtstring
                                "value" item @ "value" [] 60 %
                                "maxval"  maxval @ dup 59 > if pop 59 then
                                maxval @ minval @ - 60 <
                                maxval @ 60 % minval @ 60 % > and if
                                    "minval" minval @
                                then
                                "sticky" "w"
                                "width"   2
                                "hweight" 1
                                "newline" 0
                            }CTRL
                            {LABEL ""
                                "value" "Sec"
                                "newline" 1
                                "leftpad" 2
                                "sticky" "w"
                                "hweight" 0
                            }CTRL
                        }CTRL
                    end
                    default
                        item @ "name" []
                        item @ "type" []
                        "Unknown option type '%s' for option '%s'!"
                        fmtstring abort
                    end
                endcase
            then
            cnt ++
        repeat
    }list
    groupopts @ array_count
;


( ------------------------------------------------------------- )


: gui_options_generate_singlegroup[ int:dscr any:caller_ctx addr:save_cb str:title str:opts_id str:group -- dict:Handlers str:DlogId ]
    {SIMPLE_DLOG title @
        "|_closed|buttonpress" 'gui_dlog_generic_cancel_cb
        "resizable" "x"
        {LABEL "errtext"
            "value"   DEFAULT_CAPTION
            "colspan" 3
            "newline" 1
            "sticky"  "w"
        }CTRL
        dscr @ 0 caller_ctx @ opts_id @ group @
        singlegroup_ctrls_generate var! optcnt
        array_vals pop
        {DATUM "ctrl_cnt"
            "value" optcnt @
        }CTRL
        {HRULE "hrule1"
            "sticky"  "ew"
            "colspan" 3
        }CTRL
        {FRAME "btnfr"
            "sticky"  "ew"
            "colspan" 3
            {BUTTON "done"
                "text"    "&Done"
                "sticky"  "e"
                "width"   6
                "hweight" 1
                "newline" 0
                "dismiss" 0
                "|buttonpress" 'gui_dlog_generic_save_cb
            }CTRL
            {BUTTON "apply"
                "text"    "&Apply"
                "sticky"  "e"
                "width"   6
                "hweight" 0
                "newline" 0
                "dismiss" 0
                "|buttonpress" 'gui_dlog_generic_save_cb
            }CTRL
            {BUTTON "cancel"
                "text"    "&Cancel"
                "width"   6
                "sticky"  "e"
                "|buttonpress" 'gui_dlog_generic_cancel_cb
            }CTRL
        }CTRL
    }DLOG
    dscr @ swap GUI_GENERATE var! dlogid var! handlers
    dlogid @ GUI_DLOG_SHOW

    dlogid @
    {
        "title" title @
        "opts_id" opts_id @
        "caller_ctx" caller_ctx @
        "save_cb" save_cb @
    }dict GUI_DLOG_STATEDATA_SET

    handlers @ dlogid @
;


( ------------------------------------------------------------- )


: multigroup_notebook[ int:dscr any:caller_ctx addr:save_cb str:title str:opts_id -- dict:Handlers str:DlogId ]
    0 var! total_count
    opts_id @ optionsinfo_getgroups var! groups
    
    {SIMPLE_DLOG title @
        "|_closed|buttonpress" 'gui_dlog_generic_cancel_cb
        "resizable" "x"
        {LABEL "errtext"
            "value"   DEFAULT_CAPTION
            "colspan" 3
            "newline" 1
            "sticky"  "w"
        }CTRL
        {NOTEBOOK "notebook1"
            groups @
            foreach var! group "group_%03i" fmtstring var! groupid
                {PANE groupid @ group @
                    dscr @ total_count @ caller_ctx @ opts_id @ group @
                    singlegroup_ctrls_generate
                    total_count @ + total_count !
                    array_vals pop
                }PANE
            repeat
        }CTRL
        {DATUM "ctrl_cnt"
            "value" total_count @
        }CTRL
        {HRULE "hrule1"
            "sticky"  "ew"
            "colspan" 3
        }CTRL
        {FRAME "btnfr"
            "sticky"  "ew"
            "colspan" 3
            {BUTTON "done"
                "text"    "&Done"
                "sticky"  "e"
                "width"   6
                "hweight" 1
                "newline" 0
                "dismiss" 0
                "|buttonpress" 'gui_dlog_generic_save_cb
            }CTRL
            {BUTTON "apply"
                "text"    "&Apply"
                "sticky"  "e"
                "width"   6
                "hweight" 0
                "newline" 0
                "dismiss" 0
                "|buttonpress" 'gui_dlog_generic_save_cb
            }CTRL
            {BUTTON "cancel"
                "text"    "&Cancel"
                "width"   6
                "sticky"  "e"
                "|buttonpress" 'gui_dlog_generic_cancel_cb
            }CTRL
        }CTRL
    }DLOG
    dscr @ swap GUI_GENERATE var! dlogid var! handlers
    dlogid @ GUI_DLOG_SHOW
    dlogid @ handlers @ GUI_DLOG_REGISTER

    dlogid @
    {
        "title" title @
        "opts_id" opts_id @
        "caller_ctx" caller_ctx @
        "save_cb" save_cb @
    }dict GUI_DLOG_STATEDATA_SET

    handlers @ dlogid @
;


( ------------------------------------------------------------- )


: open_group_dlog_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    context @ "descr" [] var! dscr
    context @ "values" [] var! vals
    context @ "statedata" [] var! statedata
    statedata @ "title" [] var! title
    statedata @ "opts_id" [] var! opts_id
    statedata @ "caller_ctx" [] var! caller_ctx
    statedata @ "save_cb" [] var! save_cb

    ctrlid @ "group_" "groupbtn_" subst
    vals @ swap [] 0 [] var! group

    { "%s: %s" title @ group @ } reverse fmtstring title !

    dscr @ caller_ctx @ save_cb @ title @ opts_id @ group @
    gui_options_generate_singlegroup
    swap gui_dlog_register
    0
;


: gui_options_generate_multigroup[ int:dscr any:caller_ctx addr:save_cb str:title arr:opts_id -- ]
    0 var! colnum
    4 var! maxcols

    opts_id @ optionsinfo_getgroups var! groups

    {SIMPLE_DLOG title @
        {LABEL "errtext"
            "value"   "Select the option group you wish to edit."
            "colspan" maxcols @
            "newline" 1
            "sticky"  "w"
        }CTRL
        groups @
        foreach var! group var! cnt
            {DATUM cnt @ "group_%03i" fmtstring
                "value" group @
            }CTRL
            {BUTTON cnt @ "groupbtn_%03i" fmtstring
                "text"    group @
                "dismiss" 0
                "sticky"  "nsew"
                "width"   10
                "hweight" 1
                "toppad"  2
                "leftpad" 2
                "newline"
                    colnum ++ colnum @
                    maxcols @ % not if
                        1 0 colnum !
                    else 0
                    then
                "|buttonpress" 'open_group_dlog_cb
            }CTRL
        repeat
        colnum @ maxcols @ % if
        {FRAME "filler1"
            "newline" 1
            "toppad"  2
            "leftpad" 2
        }CTRL
        then
        {HRULE "hrule1"
            "sticky"  "ew"
            "colspan" maxcols @
        }CTRL
        {FRAME "btnfr"
            "sticky"  "ew"
            "colspan" maxcols @
            {BUTTON "done"
                "text"    "&Done"
                "sticky"  "e"
                "width"   6
                "default" 1
                "hweight" 1
            }CTRL
        }CTRL
    }DLOG
    dscr @ swap GUI_GENERATE var! dlogid var! handlers
    dlogid @ GUI_DLOG_SHOW
    dlogid @ handlers @ GUI_DLOG_REGISTER

    dlogid @
    {
        "title" title @
        "opts_id" opts_id @
        "caller_ctx" caller_ctx @
        "save_cb" save_cb @
    }dict GUI_DLOG_STATEDATA_SET
;


: gui_options_generate[ int:dscr any:caller_ctx addr:save_cb str:title arr:optionsinfo -- int:opts_id ]
    { }dict var! groups
    optionsinfo @ optionsinfo_new var! opts_id
    opts_id @ optionsinfo_getgroups var! groups
    groups @ array_count 1 > if
        groups @ array_vals array_make "   " array_join strlen 80 <
        if
            dscr @ caller_ctx @ save_cb @ title @ opts_id @
            multigroup_notebook
        else
            dscr @ caller_ctx @ save_cb @ title @ opts_id @
            gui_options_generate_multigroup
        then
    else
        dscr @ caller_ctx @ save_cb @ title @ opts_id @ groups @ 0 []
        gui_options_generate_singlegroup
        swap gui_dlog_register
    then
    opts_id @
;
PUBLIC gui_options_generate


: gui_options_value_set[ int:opts_id str:optname any:value -- ]
    opts_id @ optname @ "value" value @ optionsinfo_set_indexed
    ( FIXME: Must implement updating of dialogs. )
;
PUBLIC gui_options_value_set


: gui_options_free[ int:opts_id -- ]
    opts_id @ optionsinfo_del
;
PUBLIC gui_options_free


: gui_options_process[ int:dscr any:caller_ctx addr:save_cb str:title arr:optionsinfo -- ]
    dscr @ GUI_AVAILABLE 0.0 > if
        dscr @ caller_ctx @ save_cb @ title @ optionsinfo @
        gui_options_generate var! opts_id
        background
        gui_event_process
        opts_id @ gui_options_free
    else
        dscr @ descrdbref me @ dbcmp if
            caller_ctx @ save_cb @ title @ optionsinfo @
            menu_options_process
        else
            "Textmode option menu fallback will only work for the triggering player."
            abort
        then
    then
;
PUBLIC gui_options_process


$pubdef GUI_OPTIONS_GENERATE  "$lib/optionsgui" match "gui_options_generate" call
$pubdef GUI_OPTIONS_VALUE_SET  "$lib/optionsgui" match "gui_options_value_set" call
$pubdef GUI_OPTIONS_FREE  "$lib/optionsgui" match "gui_options_free" call
$pubdef GUI_OPTIONS_PROCESS  "$lib/optionsgui" match "gui_options_process" call

.
c
q
@register lib-optionsgui=lib/optionsgui
@register #me lib-optionsgui=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3

