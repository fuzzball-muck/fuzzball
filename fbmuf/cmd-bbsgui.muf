@prog cmd-bbsgui
1 99999 d
1 i
$include $lib/gui
$def tell descrcon swap connotify
$def }join } array_make "" array_join
  
: generic_handler[ int:dscr str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ GUI_VALUES_GET var! vals
    
    { ctrlid @ " sent " event @ " event!" }join dscr @ tell 
  
    vals @ foreach
        swap "=" strcat dscr @ tell
        foreach
            "    " swap strcat dscr @ tell
            pop
        repeat
    repeat
    0
;
  
: postyes_callback[ int:dscr str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ GUI_VALUES_GET var! vals
 
    "This is where I would post the message." dscr @ tell
    vals @ foreach
        swap "=" strcat dscr @ tell
        foreach
            "    " swap strcat dscr @ tell
            pop
        repeat
    repeat
    0
;
    
: gui_cancelwrite_cb[ int:dscr str:dlogid str:ctrlid str:event -- int:exit ]
    "Cancel new message"
    "Are you sure you want to cancel this new message?"
    { "Yes" "No" }list
    gui_messagebox "Yes" strcmp not if
        dlogid @ gui_dlog_close
        dlogid @ gui_dlog_deregister
    then
    0
;
  
: gen_writer_dlog[ -- dict:Handlers str:dlogid ]
    {SIMPLE_DLOG "Post Message"
        {LABEL ""
            "value" "Subject"
            "newline" 0
            }CTRL
        {EDIT "subj"
            "value" "This is a subject"
            "sticky" "ew"
            }CTRL
        {LABEL ""
            "value" "Keywords"
            "newline" 0
            }CTRL
        {EDIT "keywd"
            "value" "Default keywords"
            "sticky" "ew"
            "hweight" 1
            }CTRL
        {MULTIEDIT "body"
            "value" ""
            "width" 80
            "height" 20
            "colspan" 2
            }CTRL
        {FRAME "bfr"
            "sticky" "ew"
            "colspan" 2
            {BUTTON "PostBtn"
                "text" "Post"
                "width" 8
                "sticky" "e"
                "hweight" 1
                "newline" 0
                "|buttonpress" 'generic_handler
                }CTRL
            {BUTTON "CancelBtn"
                "text" "Cancel"
                "width" 8
                "sticky" "e"
                "dismiss" 0
                "|buttonpress" 'gui_cancelwrite_cb
                }CTRL
        }CTRL
        ( "|_closed|buttonpress" 'gui_cancelwrite_cb )
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
  
: gui_write_new_cb[ int:dscr str:dlogid str:ctrlid str:event -- int:exit ]
    gen_writer_dlog swap gui_dlog_register
    0
;
  
: gui_protectmsg_cb[ int:dscr str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ "msgs" GUI_VALUE_GET "" array_join atoi
    var! msgnum
    "Message protection"
    {
        "Are you sure you want to toggle message #" msgnum @
        "'s protection flag?"
    }join
    { "Yes" "No" }list
    gui_messagebox
    "Yes" strcmp not if
        {
            "Message " msgnum @
            " would have it's protection flag toggled here."
        }join
        dscr @ tell
    then
    0
;
 
: gui_deletemsg_cb[ int:dscr str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ "msgs" GUI_VALUE_GET "" array_join atoi
    var! msgnum
    "Delete message?"
    { "Are you sure you want to delete message #" msgnum @ "?" }join
    { "Yes" "No" }list
    gui_messagebox
    "Yes" strcmp not if
        msgnum @ "Message %i would be deleted here." fmtstring
        dscr @ tell
    then
    0
;
  
: gen_reader_dlog[ -- dict:Handlers str:DlogId ]
    {SIMPLE_DLOG "Read Messages"
        {LISTBOX "msgs"
            "value" "0"
            "sticky" "nswe"
            "options" {
                "Revar       Writing Gui Programs in MUF"
                "Fre'ta      Scripting in Trebuchet"
                "Points      Floating point error checking in MUF"
            }list
            "font" "fixed"
            "report" 1
            "height" 5
            "newline" 0
            }CTRL
        {FRAME "bfr"
            "sticky" "nsew"
            {BUTTON "WriteBtn"
                "text" "Write New"
                "width" 8
                "sticky" "n"
                "dismiss" 0
                "|buttonpress" 'gui_write_new_cb
                }CTRL
            {BUTTON "DelBtn"
                "text" "Delete"
                "width" 8
                "sticky" "n"
                "dismiss" 0
                "|buttonpress" 'gui_deletemsg_cb
                }CTRL
            {BUTTON "ProtectBtn"
                "text" "Protect"
                "width" 8
                "sticky" "n"
                "vweight" 1
                "dismiss" 0
                "|buttonpress" 'gui_protectmsg_cb
                }CTRL
            }CTRL
        {FRAME "header"
            "sticky" "ew"
            "colspan" 2
            {LABEL "from"
                "value" "Revar"
                "sticky" "w"
                "width" 16
                "newline" 0
                }CTRL
            {LABEL "subj"
                "value" "This is a subject."
                "sticky" "w"
                "newline" 0
                "hweight" 1
                }CTRL
            {LABEL "date"
                "value" "Fri Dec 24 15:52:30 PST 1999"
                "sticky" "e"
                "hweight" 1
                }CTRL
            }CTRL
        {MULTIEDIT "body"
            "value" ""
            "width" 80
            "height" 20
            "readonly" 1
            "hweight" 1
            "toppad" 0
            "colspan" 2
            }CTRL
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
  
: gui_test[ str:arg -- ]
    DESCR GUI_AVAILABLE 0.0 > if
        background
        gen_reader_dlog swap gui_dlog_register
        gui_event_process
        pop pop
    else
        ( Put in old-style config system here. )
        DESCR descrcon "Gui not supported!" connotify
    then
;
.
c
q
@register #me cmd-bbsgui=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=3


