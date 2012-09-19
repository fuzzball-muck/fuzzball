@prog lib-gui
1 99999 d
1 i
(
  GUI_GENERATE[ int:Dscr list:DlogSpec -- dict:Handlers str:DlogID ]
     Given a nested list that describes a dialog, sends all MCP commands
     neccesary to build that dialog, and returns a dictionary of callbacks
     to be called by that dialog's events, and the dialogID of the new
     dialog.  The dialog description list is built like:
         {SIMPLE_DLOG "Dialog Title"
             {LABEL "HelloLblCtrl"
                 "text" "Hello World!"
                 "sticky" "ew"
             }CTRL
             {BUTTON "OkayBtnCtrl"
                 "text" "Done"
                 "width" 6
                 "|buttonpress" 'done-callback
             }CTRL
         }DLOG
     Each control has a controlID [which may be blank, for anonymous controls]
     followed by either name-value pairs, or child controls, in the case of
     containers. [frame and notebook] ie:
         {SIMPLE_DLOG "Buttons!"
             {FRAME "ButtonFr"
                 "text" "Press one"
                 "visible" 1
                 {BUTTON "b1"
                     "text" "One"
                     "newline" 0
                     "|buttonpress" 'first-callback
                 }
                 {BUTTON "b2"
                     "text" "Two"
                     "|buttonpress" 'second-callback
                 }
             }CTRL
         }DLOG
     When an option name starts with a "|", then it is a callback, and
     the value is expected to be the function address to call when the
     named event is received for that control.  For events sent for
     automatically created controls, [ie: for the window close button]
     you can specify the callback by giving the Dialog itself a callback
     argument built from the virtual controlID and the event type. ie:
         {SIMPLE_DLOG "Hello"
             {LABEL ""
                 "text" "Hello World!"
                 "sticky" "ew"
             }CTRL
             "|_closed|buttonpress" 'done-callback
         }DLOG
     The {NOTEBOOK control is special in that it requires the child
     controls to be grouped in named panes.  You can do this like:
         {NOTEBOOK "nb"
             {PANE "pane1" "First Pane"
                 {BUTTON "firstbtn"
                     ...
                 }CTRL
             }PANE
             {PANE "pane2" "Second Pane"
                 {BUTTON "secondbtn"
                     ...
                 }CTRL
             }PANE
         }CTRL
     The {MENU control is different from most controls, as you don't
     specify any layout information for it.  Instead, you just add in
     {BUTTON controls for menu commands, {CHECKBOXes and {RADIOs for
     the correspending menu item types, and {HRULEs for line separators.
     You can also nest {MENUs to get cascading menus.  If you create a
     menu item with an & in the display name, then the next character
     will be underlined, and you can select the menu with that letter
     when the menu is displayed.  You must end a {MENU block with the
     end token }MENU.  A menu declaration may look like:
         {MENU "filemenu"
             text "&File"
             {MENU "cascade"
                 "text" "&Cascading menu"
                 {RADIO "mrad1"
                     "text" "Radio button &1"
                    "selvalue" 1
                    "valname" "menuradioval"
                 }CTRL
                 {RADIO "mrad2"
                     "text" "Radio button &2"
                    "selvalue" 2
                    "valname" "menuradioval"
                 }CTRL
                 {RADIO "mrad3"
                     "text" "Radio button &3"
                    "selvalue" 3
                    "valname" "menuradioval"
                    "value" 2
                 }CTRL
             }MENU
             {CHECKBOX "mcb1"
                 "text" "Checkbox"
                 "value" 1
                 "report" 1
                 "|buttonpress" 'mcb1-clicked-cb
             }CTRL
             {HRULE "" }CTRL
             {BUTTON "exit"
                 "text" "E&xit"
                 "dismiss" 1
                 "|buttonpress" 'exit-cb
             }CTRL
         }MENU
  
     The supported controls are:
         {LABEL       A static test label.
         {HRULE       A horizontal line
         {VRULE       A vertical line
         {BUTTON      A pushbutton
         {CHECKBOX    A boolean on/off checkbox
         {EDIT        A single line text entry field
         {MULTIEDIT   A multi-line text entry field
         {COMBOBOX    An optionally editable text field with pulldown defaults.
         {SPINNER     An integer entry field with up/down buttons.
         {SCALE       A floating point slider control.
         {LISTBOX     A control for selecting one or more options.
         {TREE        A control for selecting one of a heirarchy of options.
         {FRAME       A box to put other controls in, with optional captionr
                        and outline.
         {NOTEBOOK    A notebook container, to organize controls into related
                        panes.
         {MENU        A menu or sub-menu.  Can contain labels, buttons,
                        checkboxes, and other menus.
  
     All controls support some special layout options.  These are:
         "sticky"     The sides of the cell to which this control will
                        stick.  Contains one or more of N, S, E, and W.
         "newline"    If false, next control will be to the right.  If true,
                        then it will be at the start of the next row.
         "colskip"    Number of columns to skip before placing this control.
         "colspan"    Number of columns this control's cell will span across.
         "rowspan"    Number of rows this control's cell will span across.
         "column"     Column number to put this control into.  First is 0.
         "row"        Row number to put this control into.  First is 0.
         "toppad"     Number of pixels to pad above the current row.
         "leftpad"    Number of pixels to pad to the left of the current col.
         "vweight"    Specifies the expansion ratio for the current row.
         "hweight"    Specifies the expansion ratio for the current column.
  
     Note that "row" and "column" are not usually necessary, if you create
     the controls from left to right, top to bottom.  You can simply keep
     laying out the next control in the next logical position.  When you
     specify the "newline" option to 1, the next position is automatically
     the first column of the next row.  If you don't specify "newline" or
     it is 0, then the next logical position is the column to the right of
     the current control's rightmost column.  <as modified by "colspan".>
     MENU controls do not require layout options.
  
  
  GUI_DLOG_REGISTER[ str:dlogid dict:handlers -- ]
     This registers a set of event handler callbacks for a dialog.  These
     will be called by GUI_EVENT_PROCESS if an event comes in for that dialog,
     that is listed among those events it has a handler for.  The handlers
     dictionary argument is keyed by controlID and event/error type, and the
     values are the addresses of the functions to call.  ie:
         {
             "OkayBtn|buttonpress"   'okaybtn-callback
             "CancelBtn|buttonpress" 'cancel-callback
             "_closed|buttonpress"   'cancel-callback
             "BodyTxt|EOUTOFRANGE"   'error-handler-cb
             " default"              'handle-other-events-cb
         }dict
     A special key of " default" will be used if it exists, when a gui event
     comes in that does not match one of the more specific handlers.
     Gui callbacks are expected to have the signature:
       [ dict:Context str:DlogID str:CtrlID str:GuiEventType -- int:ExitRequested ]
     Error handler callbacks are expected to have the signature:
       [ dict:Context str:DlogID str:CtrlID str:ErrText str:ErrCode -- int:ExitRequested ]
     The Context dictionaries will have the following entries:
         "descr"       the descriptor that the dialog was for.
         "dlogid"      the DlogID of the dialog that generated the event.
         "id"          the ID of the control that generated the event.
         "dismissed"   true if the dialog was dismissed, 0 otherwise.
         "event"       the type of gui event.  ie: buttonpress, menuselect.
         "data"        array of extra gui-event specific data.
         "values"      the dictionary of control values, keyed by control name.
         "statedata"   data set with EVENT_STATEDATA_SET.
         "errcode"     error code field for error events only.
         "errtext"     error text for error events only.
  
 
  GUI_DLOG_DEREGISTER[ str:dlogid -- ]
     Deregisters all callback handlers for the given dialog, making them no
     longer exist for purposes of GUI_EVENT_PROCESS.
 
 
  GUI_DLOG_STATEDATA_SET[ str:eventid any:data -- ]
     This sets context statedata for a specific GUI dialog.  When a GUI event
     is processed and the associated callback is called, this data will be
     passed to the callback as the "statedata" item in the context dictionary.
  
 
  EVENT_REGISTER[ str:eventid addr:handler -- ]
     This registers a callback for a specific event type.  This callback
     will be called by GUI_EVENT_PROCESS if an event comes in that matches.
     A special eventid of " default" will let you specify a callback for all
     other events that you don't have a handler for.
     Miscellaneous event callbacks have the signature:
     [ dict:Context str:EventType -- int:ExitRequested ]
     The Context dictionary will have the following entries:
         "statedata"   data set with EVENT_STATEDATA_SET.
         "context"     event context info returned by EVENT_WAITFOR.
  
       
  EVENT_DEREGISTER[ str:dlogid -- ]
     Deregisters the callback for the given event type, making it no longer
     exist for purposes of GUI_EVENT_PROCESS.
 
     
  EVENT_STATEDATA_SET[ str:eventid any:data -- ]
     This sets context statedata for a specific event type.  When an event
     is processed and the associated callback is called, this data will be
     passed to the callback as the "statedata" item in the context dictionary.
  
 
  GUI_EVENT_PROCESS[ -- dict:Args str:Event ]
     This waits for and processes events as they come in.  If there are
     callbacks available for each event, as registered by GUI_DLOG_REGISTER
     or EVENT_REGISTER, they will be called.
  
     Gui callbacks are expected to have the signature:
       [ dict:Context str:DlogID str:CtrlID str:GuiEventType -- int:ExitRequested ]
     If the callback returns true, then GUI_EVENT_PROCESS will exit.
 
     Gui error handler callbacks are expected to have the signature:
       [ dict:Context str:DlogID str:CtrlID str:ErrText str:ErrCode -- int:ExitRequested ]
     If the callback returns true, then GUI_EVENT_PROCESS will exit.
     If an error event is received that we don't have a handler for,
     then this will ABORT the program, with the given error message.
  
     Miscellaneous event callbacks have the signature:
       [ dict:Context str:EventType -- int:ExitRequested ]
     If the callback returns true, then GUI_EVENT_PROCESS will exit.
 
     You must do a GUI_DLOG_DEREGISTER when you use the GUI_DLOG_CLOSE
     primitive in a callback, to let GUI_EVENT_PROCESS know that it no longer
     needs to worry about that dialog.
     
     GUI_EVENT_PROCESS will return when all dialogs have been dismissed or
     deregistered in a callback, or when one of the callbacks returns true.
     The values returned are:
         1. The dictionary of context data returned by EVENT_WAIT for the
             event that triggered the exiting.
         2. The event string returned by EVENT_WAIT.
  
  
  GUI_MESSAGEBOX[ str:title str:text list:buttons dict:opts -- str:button ]
     Creates a dialog box with the given title and caption text, containing
     one or more buttons with the given names.  The options dictionary is
     for more obscure options for the dialog.  The currently supported
     options are:
  
        "default" to specify which button should be the default button.  
  
     The MUF program will pause and wait for one of the buttons to be pressed.
     When a button is pressed, this call will return with a string that is
     the name that was displayed on the button that was pressed.
)
 
$author Revar Desmera <revar@belfry.com>
$lib-version 6.003
$version 6.003
$note This is a MUF library to make it much simpler to create GUI dialogs.
 
 
: list_parse[ list:spec -- dict:args list:ctrls ]
    ""      var! key
    { }dict var! args
    { }dict var! ctrls
  
    spec @
    foreach
        swap pop
        key @ if
            args @ key @ array_setitem args !
            "" key !
        else
            dup string? if
                key !
            else
                dup array? if
                    ctrls @ dup array_count array_setitem ctrls !
                else
                    "Bad dialog description format"
                    abort
                then
            then
        then
    repeat
    key @ if
        "Bad dialog description format: option is missing its value"
        abort
    then
    args @ ctrls @
;
  
   
: gui_generate_ctrl[ str:dlogid str:pane list:ctrlspec -- dictHandlers ]
    ctrlspec @ 0 []
    var! type
 
    ctrlspec @ 1 []
    var! id
  
    type @ string? not if
        "Bad control type: Expected a string"
        abort
    then
    
    id @ string? not if
        "Bad control ID: Expected a string"
        abort
    then
    
    { }list var! panes
    { }list var! names
  
    { "value" "" }dict var! args
    {            }dict var! ctrls
    {            }dict var! handlers
 
    type @ C_NOTEBOOK stringcmp not
    var! multipane
  
    ctrlspec @ array_count 2 > if
        ctrlspec @ 2 9999 [..] list_parse
        ctrls ! args !
    then
  
    ctrls @ array_count if
        id @ not if
            "Cannot embed controls in anonymous container"
            abort
        then
    then
  
    args @
    foreach
        over "|" 1 strncmp not if
            id @ not if
                "Cannot assign handlers to anonymous controls"
                abort
            then
            args @ 3 pick array_delitem args !
            swap 1 strcut swap pop
            id @ "|" strcat swap strcat
            over address? not if
                "Handlers can only take address arguments."
                abort
            then
            handlers @ swap array_setitem handlers !
        else
            pop pop
        then
    repeat
    
    multipane @ if
        ctrls @ foreach
            swap pop
            dup 0 [] "notebook_pane" stringcmp if
                "Notebooks can only contain panes."
                abort
            else
                dup 1 []
                dup string? not if
                    "Bad pane ID: Expected a string"
                    abort
                then
                panes @ dup array_count array_setitem panes !
                
                2 []
                dup string? not if
                    "Bad pane name: Expected a string"
                    abort
                then
                names @ dup array_count array_setitem names !
            then
        repeat
        panes @ args @ "panes" array_setitem args !
        names @ args @ "names" array_setitem args !
    then
 
    pane @ args @ "pane" array_setitem args !
    dlogid @ type @ id @ args @ GUI_CTRL_CREATE
  
    multipane @ if
        ctrls @ foreach
            swap pop
            var newpane dup 1 [] newpane !
            3 9999 [..] list_parse
            swap array_count if
                "Bad dialog description format: expected control array"
                abort
            then
            foreach
                swap pop
                dlogid @ newpane @ rot gui_generate_ctrl
                0 handlers @ array_setrange handlers !
            repeat
        repeat
    else
        ctrls @ foreach
            swap pop
            dlogid @ id @ rot gui_generate_ctrl
            0 handlers @ array_setrange handlers !
        repeat
    then
    handlers @
;
  
  
: gui_generate_simple[ int:dscr str:dlogtype str:title list:dlogspec --
                       dict:handlers str:dlogid ]
  
    { }dict var! handlers
 
    dlogspec @ list_parse swap
    dup var! args
    foreach
        over "|" 1 strncmp not if
            args @ 3 pick array_delitem args !
            swap 1 strcut swap pop
            over address? not if
                "Handlers can only take address artguments."
                abort
            then
            handlers @ swap array_setitem handlers !
        else
            pop pop
        then
    repeat
    
    dscr @ dlogtype @ title @ args @ GUI_DLOG_CREATE
    var dlogid dlogid !
  
    foreach
        swap pop
        dlogid @ "" rot gui_generate_ctrl
        0 handlers @ array_setrange handlers !
    repeat
  
    handlers @ dlogid @
;
  
  
: gui_generate_paned[ int:dscr str:dlogtype str:title list:dlogspec --
                      dict:handlers str:dlogid ]
    { }list var! panes
    { }list var! names
  
    dlogspec @ list_parse
    var! ctrls
    var! args
    { }dict var! handlers
  
    args @
    foreach
        over "|" 1 strncmp not if
            args @ 3 pick array_delitem args !
            swap 1 strcut swap pop
            over address? not if
                "Handlers can only take address arguments."
                abort
            then
            handlers @ swap array_setitem handlers !
        else
            pop pop
        then
    repeat
    
    ctrls @ foreach
        swap pop
        dup 0 [] "notebook_pane" stringcmp if
            "This dialog type can only directly contain panes."
            abort
        else
            dup 1 [] (pane)
            panes @ array_appenditem panes !
            swap 2 [] (name)
            names @ array_appenditem names !
        then
    repeat
  
    panes @ args @ "panes" array_setitem args !
    names @ args @ "names" array_setitem args !
  
    dscr @ dlogtype @ title @ args @ GUI_DLOG_CREATE
    var dlogid dlogid !
  
    ctrls @ foreach
        swap pop
        var newpane dup 1 [] newpane !
        3 9999 [..] list_parse
        swap array_count if
            "Bad dialog description format: expected control array"
            abort
        then
        foreach
            swap pop
            dlogid @ newpane @ rot gui_generate_ctrl
            0 handlers @ array_setrange handlers !
        repeat
    repeat
  
    handlers @ dlogid @
;
  
  
: gui_generate[ int:Dscr list:DlogSpec -- dict:Handlers str:DlogID ]
    DlogSpec @ 0 [] var! type
    DlogSpec @ 1 [] var! title
  
    Dscr @ type @ title @
    DlogSpec @ 2 9999 [..]
 
    type @ D_TABBED stringcmp not
    type @ D_HELPER stringcmp not or if
        gui_generate_paned
    else
        gui_generate_simple
    then
;
PUBLIC gui_generate
 
(--------------------------------------------------------------)
( Gui Dispatcher                                               )
(--------------------------------------------------------------)
: dispatch ( ... strValue dictDests -- ... intSuccess)
    dup rot []
    dup not if
        pop " default" []
        dup not if
            pop 0 exit
        else
            execute
        then
    else
        swap pop
        execute
    then
    1
;
  
: gui_dict_add (dict1 dict2 -- dict3)
    ( adds all items from dict2 to dict1, removing those items with false values )
    foreach
        dup not if
            pop array_delitem
        else
            rot rot array_setitem
        then
    repeat
;
 
 
lvar GuiHandlers
lvar OtherHandlers
lvar GuiStateData
lvar OtherStateData 
 
: gui_dlog_register[ str:dlogid dict:handlers -- ]
    GuiHandlers @ if
        handlers @ GuiHandlers @
        "GUI." dlogid @ strcat
        array_setitem GuiHandlers !
    else
        { "GUI." dlogid @ strcat handlers @ }dict GuiHandlers !
    then
;
PUBLIC gui_dlog_register
 
 
: gui_dlog_deregister[ str:dlogid -- ]
    GuiHandlers @ if
        GuiHandlers @
        "GUI." dlogid @ strcat
        array_delitem GuiHandlers !
    else
        { }dict GuiHandlers !
    then
    GuiStateData @ if
        GuiStateData @
        "GUI." dlogid @ strcat
        array_delitem GuiStateData !
    else
        { }dict GuiStateData !
    then
;
PUBLIC gui_dlog_deregister
 
 
: gui_dlogs_registered[ -- dict:GuiHandlers ]
    GuiHandlers @ if
        GuiHandlers @
    else
        { }dict dup GuiHandlers !
    then
;
PUBLIC gui_dlogs_registered
 
 
: gui_dlog_statedata_set[ str:dlogid any:data -- ]
    GuiStateData @ if
        data @ GuiStateData @
        "GUI." dlogid @ strcat
        array_setitem GuiStateData !
    else
        { "GUI." dlogid @ strcat data @ }dict GuiStateData !
    then
;
PUBLIC gui_dlog_statedata_set
 
 
: event_register[ str:eventid addr:callback -- ]
    OtherHandlers @ if
        callback @ OtherHandlers @ eventid @ array_setitem OtherHandlers !
    else
        { eventid @ callback @ }dict OtherHandlers !
    then
;
PUBLIC event_register
 
 
: event_deregister[ str:eventid -- ]
    OtherHandlers @ if
        OtherHandlers @ eventid @ array_delitem OtherHandlers !
    else
        { }dict OtherHandlers !
    then
    OtherStateData @ if
        OtherStateData @ eventid @ array_delitem OtherStateData !
    else
        { }dict OtherStateData !
    then
;
PUBLIC event_deregister
 
 
: events_registered[ -- dict:EventHandlers ]
    OtherHandlers @ if
        OtherHandlers @
    else
        { }dict dup OtherHandlers !
    then
;
PUBLIC events_registered
 
 
: event_statedata_set[ str:eventid any:data -- ]
    OtherStateData @ if
        data @ OtherStateData @ eventid @ array_setitem OtherStateData !
    else
        { eventid @ data @ }dict OtherStateData !
    then
;
PUBLIC event_statedata_set
 
 
: gui_event_process[ -- dict:Context str:Event ]
    GuiHandlers @ not if { }dict GuiHandlers ! then
    OtherHandlers @ not if { }dict OtherHandlers ! then
    begin
        GuiHandlers @ array_keys array_make
        OtherHandlers @ array_keys array_make
        array_union
 
        EVENT_WAITFOR
        var! event
        var! args
        event @ "GUI." 4 strncmp not if
            GuiHandlers @ event @ []
            var! dests
            dests @ not if
                (If no callbacks for this dialog, ignore it.)
                continue
            then
            
            args @ "descr"     [] var! dscr
            args @ "dlogid"    [] var! dlogid
            args @ "id"        [] var! id
            args @ "event"     [] var! guievent
            args @ "dismissed" [] var! dismiss
            args @ "errcode"   [] var! errcode
            args @ "errtext"   [] var! errtext
 
            id @ not if "" id ! then
            GuiStateData @
            dup not if pop { }list then
            event @ []
            args @ "statedata" ->[] args !
 
            errcode @ if
                args @ dlogid @ id @ errtext @ errcode @
                { id @ "|" errcode @ }join
                dests @ dispatch
                not if
                    5 popn
                    { errcode @ ": " errtext @ }join
                    abort
                else
                    if
                        (The callback wants us to exit.)
                        args @ event @
                        break
                    then
                then
            else
                args @ dlogid @ id @ guievent @
                { id @ "|" guievent @ }join
                dests @ dispatch
                not if
                    4 popn
                else
                    if
                        (The callback wants us to exit.)
                        args @ event @
                        break
                    then
                then
                dismiss @ if
                    (The dialog was dismissed.  Deregister it.)
                    dlogid @ gui_dlog_deregister
                    dlogid @ gui_dlog_close
                then
            then
 
            GuiHandlers @ array_count not if
                (No more dialogs left.  Time to exit)
                args @ event @
                break
            then
        else
            { "context" args @ }dict args !
 
            OtherStateData @
            dup not if pop { }list then
            event @ []
            args @ "statedata" ->[] args !
 
            args @ event @ dup OtherHandlers @ dispatch
            not if
                2 popn
            else
                if
                    (The callback wants us to exit.)
                    args @ event @
                    break
                then
            then
        then
    repeat
;
PUBLIC gui_event_process
 
: _gui_messagebox_cb[ int:dscr str:dlogid str:ctrlid str:event -- int:exit ]
    1
;
 
: GUI_MESSAGEBOX[ str:title str:text list:buttons dict:opts -- str:buttonpressed ]
    0 var! maxlen
    buttons @
    foreach
        strlen dup maxlen @ > if
            maxlen !
        else
            pop
        then
        pop
    repeat
    maxlen @ 2 + maxlen !
    maxlen @ 8 * 50 + buttons @ array_count * var! estwidth
    estwidth @ 300 < if 300 estwidth ! then
    
    { }dict var! buttonmap
 
    { D_SIMPLE title @
        { C_LABEL ""
            "value" text @
            "maxwidth" estwidth @
            }list
        { C_HRULE "" }list
        { C_FRAME "_bfr"
            "sticky" "sew"
            buttons @
            foreach
                var! btext
                var! bnum
                "_btn" bnum @ intostr strcat
                var! bname
                btext @ buttonmap @ bname @ array_setitem
                buttonmap !
                { C_BUTTON bname @
                    "text" btext @
                    "width" maxlen @
                    "newline" 0
                    "sticky" ""
                    "hweight" 1
                    opts @ "default" [] dup if
                        btext @ strcmp not if
                            "default" 1
                        then
                    else pop
                    then
                    "|buttonpress" '_gui_messagebox_cb
                    }list
            repeat
            }list
    }list
    DESCR swap GUI_GENERATE
    swap pop
    dup GUI_DLOG_SHOW
    "GUI." swap strcat
    1 array_make event_waitfor
    pop "id" []
    buttonmap @ swap array_getitem
;
PUBLIC gui_messagebox
 
$pubdef GUI_DLOGS_REGISTERED   "$lib/gui" match "gui_dlogs_registered"   call
$pubdef GUI_DLOG_REGISTER      "$lib/gui" match "gui_dlog_register"      call
$pubdef GUI_DLOG_DEREGISTER    "$lib/gui" match "gui_dlog_deregister"    call
$pubdef GUI_DLOG_STATEDATA_SET "$lib/gui" match "gui_dlog_statedata_set" call
 
$pubdef EVENTS_REGISTERED      "$lib/gui" match "events_registered"   call
$pubdef EVENT_REGISTER         "$lib/gui" match "event_register"      call
$pubdef EVENT_DEREGISTER       "$lib/gui" match "event_deregister"    call
$pubdef EVENT_STATEDATA_SET    "$lib/gui" match "event_statedata_set" call
 
$pubdef GUI_GENERATE           "$lib/gui" match "gui_generate"      call
$pubdef GUI_EVENT_PROCESS      "$lib/gui" match "gui_event_process" call
$pubdef GUI_MESSAGEBOX         "$lib/gui" match "gui_messagebox"    call
 
$pubdef {SIMPLE_DLOG  { D_SIMPLE
$pubdef {TABBED_DLOG  { D_TABBED
$pubdef {HELPER_DLOG  { D_HELPER
 
$pubdef {DATUM     { C_DATUM
$pubdef {LABEL     { C_LABEL
$pubdef {IMAGE     { C_IMAGE
$pubdef {BUTTON    { C_BUTTON
$pubdef {CHECKBOX  { C_CHECKBOX
$pubdef {RADIO     { C_RADIOBTN
$pubdef {PASSWORD  { C_PASSWORD
$pubdef {EDIT      { C_EDIT
$pubdef {MULTIEDIT { C_MULTIEDIT
$pubdef {COMBOBOX  { C_COMBOBOX
$pubdef {LISTBOX   { C_LISTBOX
$pubdef {TREE      { C_TREE
$pubdef {SCALE     { C_SCALE
$pubdef {SPINNER   { C_SPINNER
 
$pubdef {HRULE     { C_HRULE
$pubdef {VRULE     { C_VRULE
 
$pubdef {FRAME     { C_FRAME
$pubdef {MENU      { C_MENU
$pubdef {NOTEBOOK  { C_NOTEBOOK
$pubdef {PANE      { "notebook_pane"
 
$pubdef }DLOG      }list
$pubdef }PANE      }list
$pubdef }MENU      }list
$pubdef }CTRL      }list
.
c
q
@register lib-gui=lib/gui
@register #me lib-gui=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=S
@set $tmp/prog1=H
@set $tmp/prog1=V
@set $tmp/prog1=Z
@set $tmp/prog1=3

