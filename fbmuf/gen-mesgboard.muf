@prog gen-mesgboard
1 99999 d
1 i
( MUFmessageBoard v3.0    Copyright 5/31/91 by Garth Minette )
(                                           foxen@netcom.com )
( gui code by Revar/Naiya                                    )
( A program for storing and displaying multi-line messages   )
  
( This code may be freely distributed, and code from it may   
  used in other non-similar programs, but the author's name
  must be credited.                                          )
  
(To install, simply create an item to stand for the board and
 to hold all its properties.  Then create actions on that item
 for the 'read', 'write', 'editmsg', and 'erase' functions,
 linking them all to this program.  Optional features are
 'protect' which prevents messages from auto-deletion if that
 function is enabled, [set the property '_expire' on the board
 object to the number of days messages will live] and 'boardgui'
 which enables a gui interface for browsing and posting if
 the MUCK server has MCP built into it [like FuzzBall 6 and
 Proto] and if the user is using a MCP capable client like
 Trebuchet. )
  
$def VERSION "MessageBoard v3.0"
$ifdef __version>Muck2.2fb5.00
$def GUI 1
$endif
$include $lib/strings
$include $lib/props
$include $lib/match
$include $lib/lmgr
$include $lib/mesg
$include $lib/mesgbox
$include $lib/edit
$include $lib/editor
  
$def .sedit_std EDITOR
$def STRtolower tolower
$def DAYOFFSET 7800
 
$ifdef GUI
$include $lib/gui
$def tell descrcon swap connotify
$def }join } array_make "" array_join
lvar messData
lvar curMess
lvar messDlog
lvar messKeys
$endif
lvar bbsobj
 
  
( ***** Misc. Object ***** )
  
: get-day ( -- dayint)
    systime dup 86400 % time 60 * + 60 * + - - 86400 /
$ifdef DAYOFFSET
    DAYOFFSET -
$endif
;
  
  
$define showrange EDITdisplay $enddef
  
  
( ***** Message Board Object -- MBRD *****
    Display  [parm base dbref -- err]
    Add      [parm base dbref -- err]
    Kill     [parm base dbref -- err]
)
  
: MBRDparseinfo (refnum base dbref -- keywords protect? poster day subject)
    (format: player# day# subject$)
    (new:    $topicword safe? player# day# subject$)
    MBOX-msginfo
    dup "$" 1 strncmp not if
      1 strcut swap pop
      " " STRsplit " " STRsplit swap atoi swap
    else
      "" 0 rot
    then
    " " STRsplit swap atoi dbref swap
    " " STRsplit swap atoi swap
;
  
: MBRDreparseinfo (keywords protect? poster day subject -- infostr)
    rot owner rot rot
    swap intostr " " strcat swap strcat
    swap int intostr " " strcat swap strcat
    swap intostr " " strcat swap strcat
    swap ";" " " subst STRtolower " " strcat
    swap strcat "$" swap strcat
;
  
: MBRDsetinfo (refnum base dbref keywords protect? poster day subject -- )
    rot owner rot rot
    MBRDreparseinfo -4 rotate MBOX-setinfo
;
  
lvar tmp
: MBRDperms? (refnum base dbref -- bool)
    me @ owner tmp !
    MBRDparseinfo pop pop rot rot pop pop
    tmp @ dbcmp tmp @ "Wizard" flag? or
    tmp @ trigger @ getlink owner dbcmp or
    tmp @ trigger @ location owner dbcmp or
;
  
  
: MBRDlastread (dbref -- lastreadmesgnum)
    "_bbsread/" swap intostr strcat
    me @ owner swap getpropval
;
  
  
: MBRDset_lastread (lastreadnum dbref -- )
    "_bbsread/" swap intostr strcat
    me @ owner swap rot "" swap addprop
;
  
  
: MBRDdisplay-expire? (refnum base dbref -- bool)
    dup "_expire" getpropstr atoi
    dup not if pop pop pop pop 0 exit then
    -4 rotate MBRDparseinfo
    pop -4 rotate pop swap pop
    if pop pop 0 exit then
    get-day swap - <
;
  
  
  
: MBRDdisplay-header (topicstr refnum base dbref -- )
    3 pick 3 pick 3 pick MBRDparseinfo
    (topicstr refnum base dbref keywords protect? poster day subject)
    5 rotate 9 rotate dup if
        (If keyword is a negative number, don't display mesgs older than that)
        dup number? over atoi 0 < and if
            4 pick get-day - over atoi < if
                pop pop pop
                pop pop pop
                pop pop pop
                exit
            then
            pop pop
        else
            (If keyword is 'new', don't display messages older than 2 days)
            dup "new" stringcmp not if
                get-day 5 pick - 3 >= if
                    pop pop pop
                    pop pop pop
                    pop pop pop
                    exit
                then
                pop pop
            else
                (If keyword isn't in the keywords of the mesg don't display)
                instr not if
                    pop pop pop pop
                    pop pop pop exit
                then
            then
        then
    else
        pop pop
    then
    7 pick intostr 5 rotate if "} " else ") " then strcat
    4 rotate
    dup ok? if
        dup player? if name
        else pop "(Toaded Player)"
        then
    else pop "(Toaded Player)"
    then
    strcat "  " strcat
    rot get-day swap - 
    dup 7 < if
        dup not if pop "Today"
        else dup 1 = if pop "Yesterday"
            else intostr " days ago" strcat
            then
        then
    else 7 / dup 1 = if pop "Last week"
        else intostr " weeks ago" strcat
        then
    then
    " -- " strcat strcat swap
    strcat me @ swap notify
    pop pop pop
;
  
  
: MBRDdisplay-loop (topic base dbref lcv  -- )
    3 pick 3 pick MBOX-count swap
    begin
        over over <  if pop pop pop pop pop break then
        5 pick over 6 pick 6 pick
        3 pick 3 pick 3 pick MBRDdisplay-expire? if
          MBOX-delmesg pop
          swap 1 - swap
        else
          MBRDdisplay-header
          1 +
        then
    repeat
  
    "Use 'read <mesgnum>'to list a message.  Use 'read <keyword>' to list"
    me @ swap notify
    "messages with a keyword.  Use 'read -' to read the next message."
    me @ swap notify
;
  
  
: MBRDdisplay_next (base dbref -- err)
    (find the next message reference number)
    dup MBRDlastread 1 +
    3 pick 3 pick MBOX-num2ref
  
    (Was that the last message?)
    dup not if
       pop pop pop
       6 (No more messages to read.) exit
    then
    rot rot
  
    (remember that we've read this message)
    3 pick 3 pick 3 pick MBOX-ref2num
    over MBRDset_lastread
  
    (display the message)
    "" 4 pick 4 pick 4 pick MBRDdisplay-header
    MBOX-message showrange
    0 (No error.)
;
  
: MBRDdisplay (parmstr base dbref -- err)
    rot STRtolower -3 rotate (lowercase parmstr)
  
    begin (Not a loop.  Used for fake case, to provide breaks)
  
        (case "-":)
            (read next message after last read mesg)
        3 pick "-" strcmp not if
            rot pop
            MBRDdisplay_next
            exit
            break (Yes, I know the break is overkill)
        then
  
        (case "-recent":)
            (read all messages after last read mesg)
        3 pick "-recent" stringcmp not if
            rot pop
            begin
                over over MBRDdisplay_next
                0 sleep
            until
            pop pop
            break
        then
  
        (case "recent":)
            (display headers of messages after last read message)
        3 pick "recent" stringcmp not if
            (find refnum of message after last message read)
            rot pop "" rot rot
            dup MBRDlastread 1 +
            3 pick 3 pick MBOX-num2ref
  
            dup if
                MBRDdisplay-loop (topic base dbref lcv  -- )
            else
                6 exit (No more messages.)
            then
            break
        then
  
        (case <number>:)
            (Read a single message referred to by number)
        3 pick number?
        4 pick atoi 0 >= and if
            rot atoi rot rot
  
            (check to see if that reference is valid)
            3 pick 3 pick 3 pick MBOX-badref? if pop pop pop 2 exit then
  
            (remember that we've read this message)
            3 pick 3 pick 3 pick MBOX-ref2num
            over MBRDset_lastread
  
            (display the message)
            "" 4 pick 4 pick 4 pick MBRDdisplay-header
            MBOX-message showrange
            me @ "  " notify
            break
        then
  
        (default:)
            (display headers of messages, by topic or other criteria)
        1 MBRDdisplay-loop
  
    1 until (Not a loop.  Used for fake case.  breaks jump to after this line)
  
    0 (no error)
;
  
  
: MBRDreadlines ( -- {str_rng})
    0 .sedit_std pop
;
  
: MBRDadd (parmstr base dbref -- err)
    rot "=" STRsplit STRstrip swap STRstrip
    dup not if
        "What is the subject of this post?"
        me @ swap notify pop read STRstrip
    then
    swap
    dup not if
        "What keywords fit this post? (ie: art, building, place, personal)"
        me @ swap notify pop read STRstrip
    then
    0 me @ owner get-day 5 rotate MBRDreparseinfo rot rot
  
    ( infostr base dbref )
    MBRDreadlines
  
    dup if
        (Stamp the name and time onto the message)
        "  " over 2 + 0 swap - rotate 1 +
        "From: " me @ name strcat
        me @ player? not if
            (if it's a puppet, then include the owner's name too)
            " (" strcat
            me @ owner name strcat
            ")" strcat
        then
        "  " strcat "%X %x %Z" systime timefmt strcat
        over 2 + 0 swap - rotate 1 +
  
        ( store post )
        dup 4 + rotate
        over 4 + rotate
        3 pick 4 + rotate
        MBOX-append
        0 (no error)
    else
        pop pop pop pop 4 (post cancelled)
    then
;
  
: MBRDkill (parmstr base dbref -- err)
    rot dup number? not if pop pop pop 1 exit then
    atoi rot rot
    3 pick 3 pick 3 pick
    MBOX-badref? if pop pop pop 2 exit then
    3 pick 3 pick 3 pick MBRDperms? not if
        pop pop pop 3 exit  (not owner of mesgboard or poster)
    then
    MBOX-delmesg
    0 (no error)
;
  
: MBRDprotect (parmstr base dbref -- err)
    rot dup number? not if pop pop pop 1 exit then
    atoi rot rot
    3 pick 3 pick 3 pick MBOX-badref? if pop pop pop 2 exit then
    me @ "Wizard" flag?
    me @ trigger @ getlink owner dbcmp or
    me @ trigger @ location owner dbcmp or not if
        pop pop pop 3 exit  (not owner of mesgboard or poster)
    then
    3 pick 3 pick 3 pick MBRDparseinfo
    4 rotate not -4 rotate MBRDsetinfo
    0 (no error)
;
  
  
  
lvar fromline
  
: MBRDedit (parmstr base dbref -- err)
    "" fromline !
    rot dup number? not if pop pop pop 1 exit then
    atoi rot rot
    3 pick 3 pick 3 pick MBOX-badref? if pop pop pop 2 exit then
    3 pick 3 pick 3 pick MBRDperms? not if
        pop pop pop 3 exit  (not owner of mesgboard or poster)
    then
    3 pick 3 pick 3 pick MBOX-message
  
    (Strip headers, if they are there)
    begin
        dup 1 + pick "  " strcmp not if
            dup 1 + rotate pop 1 - break
        then
        dup 1 + pick "From: " 6 strncmp not if
            dup 1 + rotate fromline ! 1 - continue
        then
        dup 1 + pick "Edited by: " 11 strncmp not if
            dup 1 + rotate pop 1 - continue
        then
        break
    repeat
  
    .sedit_std pop dup not if
        pop pop pop pop 5 (no error) exit
    then
  
    (Stamp the name and time onto the message)
    "  " over 2 + 0 swap - rotate 1 +
    "Edited by: " me @ name strcat
    me @ player? not if
        (if it's a puppet, then include the owner's name too)
        " (" strcat
        me @ owner name strcat
        ")" strcat
    then
    "  " strcat "%X %x %Z" systime timefmt strcat
    over 2 + 0 swap - rotate 1 +
  
    (Resave the From header, if there was one)
    fromline @ if
        fromline @ over 2 + 0 swap - rotate 1 +
    then
  
    dup 4 + rotate over 4 + rotate 3 pick 4 + rotate
    3 pick 3 pick 3 pick MBRDparseinfo
  
    me @ "Current subject: \"" 3 pick strcat "\"" strcat notify
    "Enter new subject, or press space and return to keep old one."
    me @ swap notify
    read STRstrip dup if swap then pop
  
    5 rotate
    me @ "Current keywords: \"" 3 pick strcat "\"" strcat notify
    "Enter new keywords, or press space and return to keep old ones."
    me @ swap notify
    read STRstrip dup if swap then pop
    -5 rotate
  
    swap pop get-day swap MBRDreparseinfo
    -4 rotate MBOX-setmesg
    0 (no error)
;
  
  
: MBRD-checkinit (basename dbref -- )
    (If MBOX doesn't exist yet, create it.)
    over over MBOX-count not if
        MBOX-create
    else
        pop pop
    then
;
 
$def basename "msgs"
: get-bbsobj (default -- bbsdbref)
    dup "_bbsloc" getpropstr
    dup not if pop exit then
    dup number? not if pop exit then
    atoi dbref
    dup ok? not if pop exit then
    over owner over .controls
    if swap then pop
;
 
$ifdef GUI
: msg_index_gen[ int:doUpdate -- ]
    messData @ array_count var! oldMessCount
    0 array_make messData !
    preempt
    messKeys @ basename bbsobj @ over over MBOX-count 1
    begin
        over over < if pop pop pop pop pop break then
        5 pick over 6 pick 6 pick
        3 pick 3 pick 3 pick MBRDdisplay-expire? if
            MBOX-delmesg pop
            swap 1 - swap
        else
            3 pick 3 pick 3 pick MBRDparseinfo
            (topicstr refnum base dbref keywords protect? poster day subject)
            5 rotate 9 rotate dup if
                (If keyword is a negative number, don't display mesgs older than that)
                dup number? over atoi 0 < and if
                    4 pick get-day - over atoi < if
                        pop pop pop pop pop pop pop pop pop
                        1 + continue
                    then
                        pop pop
                else
                    (If keyword is 'new', don't display messages older than 2 days)
                    dup "new" stringcmp not if
                        get-day 5 pick - 3 >= if
                            pop pop pop pop pop pop pop pop pop
                            1 + continue
                        then
                        pop pop
                    else
                        (If keyword isn't in the keywords of the mesg don't display)
                        instr not if
                            pop pop pop pop pop pop pop
                            1 + continue
                        then
                    then
                then
            else
                pop pop
            then
            rot (get poster)
            dup ok? if
                dup player? if name
                else pop "(Toaded player)"
                then
            else pop "(Toaded player)"
            then
            rot get-day swap -
            dup 7 < if
                dup not if pop "Today"
                else dup 1 = if pop "Yesterday"
                    else intostr " days ago" strcat
                    then
                then
            else 7 / dup 1 = if pop "Last week"
                else intostr " weeks ago" strcat
                then
            then
            "author" -3 rotate
            "date" swap
            "subject" -6 rotate
            "refnum" 12 pick
            "protect" 10 pick
            "itmstr" 10 pick
            7 pick
            4 pick if "+" else "-" then
            11 pick "%-20s %1s %-14s : %s" fmtstring
            6 array_make_dict
            messData @ array_appenditem messData !
            pop pop pop pop
            1 +
        then
    repeat
    foreground
    doUpdate @ if
        oldMessCount @ 0 > if (nuke old values)
            messDlog @ "msgs" "delete"
            { "items" { oldMessCount @ -- begin
                dup while
                dup --
            repeat
            }list }dict GUI_CTRL_COMMAND
        then
        messDlog @ "msgs" "insert"
        { "values" { messData @ foreach
            swap pop (discard index)
            "itmstr" []
        repeat }list }dict GUI_CTRL_COMMAND
    then
;
 
$def get_gui_val [] array_vals pop
 
: gui_postwrite_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ GUI_VALUES_GET var! vals
    context @ "descr" [] var! dscr
 
    "Post new message"
    "Are you sure this is ready for posting?"
    { "Yes" "No" }list
    { "default" "No" }dict
    gui_messagebox "Yes" strcmp not if
        preempt
        basename bbsobj @ vals @ "subj" get_gui_val
        vals @ "keywd" get_gui_val
        0 me @ owner get-day 5 rotate MBRDreparseinfo rot rot
  
        ( infostr base dbref )
        vals @ "body" [] array_vals
  
        (Stamp the name and time onto the message)
        "  " over 2 + 0 swap - rotate 1 +
        "From: " me @ name strcat
        me @ player? not if
            (if it's a puppet, then include the owner's name too)
            " (" strcat
            me @ owner name strcat
            ")" strcat
        then
        "  " strcat "%X %x %Z" systime timefmt strcat
        over 2 + 0 swap - rotate 1 +
  
        ( store post )
        dup 4 + rotate
        over 4 + rotate
        3 pick 4 + rotate
        MBOX-append
        foreground
        dlogid @ gui_dlog_close
        dlogid @ gui_dlog_deregister
        1 msg_index_gen
    then
    0
;
    
: gui_cancelwrite_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    "Cancel new message"
    "Are you sure you want to cancel this new message?"
    { "Yes" "No" }list
    { "default" "No" }dict
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
  "dismiss" 0
                "|buttonpress" 'gui_postwrite_cb
                }CTRL
            {BUTTON "CancelBtn"
                "text" "Cancel"
                "width" 8
                "sticky" "e"
                "dismiss" 0
                "|buttonpress" 'gui_cancelwrite_cb
                }CTRL
        }CTRL
        "|_closed|buttonpress" 'gui_cancelwrite_cb
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
  
: gui_write_new_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    gen_writer_dlog swap gui_dlog_register
    0
;
  
: gui_protectmsg_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    messData @ curMess @ [] var! mItem
    mItem @ "refnum" [] var! refNum
    "Message protection"
    {
        "Are you sure you want to " mItem @ "protect" []
        if "unprotect" else "protect" then
        " message #" curMess @ ++ "?"
    }join
    { "Yes" "No" }list
    { "default" "Yes" }dict
    gui_messagebox
    "Yes" strcmp not if
        me @ "Wizard" flag?
        me @ trigger @ getlink owner dbcmp or
        me @ trigger @ location owner dbcmp or not if
     "Protect failed."
            "You don't have permission to alter the protection state of messages. [not owner of the board]"
            { "Okay" }list
            { "default" "Okay" }dict
            gui_messagebox pop
        else
            preempt
            refNum @ basename bbsobj @ MBOX-badref? if
                "Bad objref" refNum intostr strcat DESCR tell
            else
                refnum @ basename bbsobj @ 3 pick 3 pick 3 pick MBRDparseinfo
                4 rotate not -4 rotate MBRDsetinfo
                foreground
                mItem @ "protect" [] not dup mItem @ "protect" array_setitem
                "Operation successful"
                "Message " rot if "protected" else "unprotected" then strcat
                { "Okay" }list
                { "default" "Okay" }dict
                gui_messagebox pop
                1 msg_index_gen
            then
        then
    then
    0
;
 
: gui_deletemsg_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    messData @ curMess @ [] var! mItem
    mItem @ "refnum" [] var! refNum
    "Delete message?"
    { "Are you sure you want to delete message " curMess @ ++ ":'" mItem @ "subject" [] "'?" }join
    { "Yes" "No" }list
    { "default" "No" }dict
    gui_messagebox
    "Yes" strcmp not if
        preempt
        refNum @ basename bbsobj @ MBOX-badref? if
            "Bad objref" refNum intostr strcat DESCR tell
        else
            refNum @ basename bbsobj @ MBRDperms? not if
                (not owner of mesgboard or poster)
                foreground
                "Delete failed"
                "You don't have permission to delete this message [not owner/poster or protected]"
                { "Okay" }list
                { "default" "Okay" }dict
                gui_messagebox pop
            else
                refNum @ basename bbsobj @ MBOX-delmesg
                foreground
                "Operation successful"
                "Message deleted"
                { "Okay" }list
                { "default" "Okay" }dict
                gui_messagebox pop
                1 msg_index_gen
            then
        then
    then
    0
;
 
: gui_listchange ( -- )
    messData @ array_count curMess @ > if
        messData @ curMess @ [] var! mItem
        mItem @ "refnum" [] var! refNum
        preempt
        refNum @ basename bbsobj @ MBOX-badref? if
            "Bad objref" mItem intostr strcat DESCR tell
        else
            messDlog @ "subj" mItem @ "subject" [] GUI_VALUE_SET
            messDlog @ "date" mItem @ "date" [] GUI_VALUE_SET
            messDlog @ "from" mItem @ "author" [] GUI_VALUE_SET
            messDlog @ "body" refNum @ basename bbsobj @ MBOX-message array_make GUI_VALUE_SET
        then
        foreground
    else
        messDlog @ "subj" "-" GUI_VALUE_SET
        messDlog @ "date" "-" GUI_VALUE_SET
        messDlog @ "from" "-" GUI_VALUE_SET
        messDlog @ "body" " " GUI_VALUE_SET    
    then
;
 
: gui_listchange_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ "msgs" GUI_VALUE_GET "" array_join atoi curMess !
    messData @ array_count curMess @ < if
        "How did we get to an item that doesn't exist?" DESCR tell
    else
        gui_listchange
    then
    0
;
 
: gui_keywdchange_cb[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ "keylst" GUI_VALUE_GET "" array_join messKeys !
    "New keywords : " messKeys @ strcat DESCR tell
    1 msg_index_gen
    0
;
 
: gen_reader_dlog[ -- dict:Handlers str:DlogId ]
 
    {SIMPLE_DLOG "Read Messages"
        {FRAME "keyframe"
               "colspan" 2
               "sticky" "ew"
               "newline" 1
            {BUTTON "keyact"
                    "text" "Set"
                    "newline" 0
                    "dismiss" 0
                    "sticky" "ew"
                    "|buttonpress" 'gui_keywdchange_cb
            }CTRL
            {LABEL ""
                   "value" "Keywords"
                   "sticky" "ew"
                   "newline" 0
            }CTRL
            {EDIT  "keylst"
                   "value" messKeys @
                   "sticky" "ew"
                   "newline" 0
                   ("report" 1
                   "|valchanged" 'gui_keywdchange_cb)
            }CTRL
        }CTRL
        {LISTBOX "msgs"
            "value" "0"
            "sticky" "nswe"
            "options" { 
                messData @ foreach
                    swap pop (discard index)
                    "itmstr" []
                repeat
                }list
            "font" "fixed"
            "report" 1
            "height" 5
            "newline" 0
            "|valchanged" 'gui_listchange_cb
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
                "value" ""
                "sticky" "w"
                "width" 16
                "newline" 0
                }CTRL
            {LABEL "subj"
                "value" ""
                "sticky" "w"
                "newline" 0
                "hweight" 1
                }CTRL
            {LABEL "date"
                "value" ""
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
 
: gui-update[ dict:context str:event -- int:exitReq ]
    1 msg_index_gen
    "Message list updated." DESCR tell
    120 "update" timer_start    
    0
;
 
: input-handler[ dict:context str:event -- int:exitReq ]
    (parse things like say, pose, page events in here so someone)
    (can browse while still being social...)
    read
    dup "\"" stringpfx if
      1 strcut swap pop
      dup "You say, \"%s\"" fmtstring .tell
      me @ name "%s says, \"%s\"" fmtstring .otell
      0 exit
    then
    dup "say " stringpfx if
      4 strcut swap pop
      dup "You say, \"%s\"" fmtstring .tell
      me @ name "%s says, \"%s\"" fmtstring .otell
      0 exit
    then
    dup ":" stringpfx if
      1 strcut swap pop
      me @ name "%s %s" fmtstring loc @ swap 0 swap notify_exclude
      0 exit
    then
    dup "pose " stringpfx if
      5 strcut swap pop
      me @ name "%s %s" fmtstring loc @ swap 0 swap notify_exclude
      0 exit
    then
    (dup "page " stringpfx if
    then)
    0
;
$endif  
  
( ***** Interface Object *****
)
  
: handle-errs
    dup not if pop me @ "Done." notify exit then
    dup 1 = if pop me @ "Should be a numeric parameter." notify exit then
    dup 2 = if pop me @ "Invalid message number." notify exit then
    dup 3 = if pop me @ "Permission denied." notify exit then
    dup 4 = if pop me @ "Cancelling post." notify exit then
    dup 5 = if pop me @ "Cancelling edit." notify exit then
    dup 6 = if pop me @ "No more messages." notify exit then
;
  
  
  
: MBRD-showhelp ( -- )
VERSION " by Foxen/Revar.  Capitalized words are user supplied args." strcat
"-----------------------------------------------------------------------------"
"read #help             Shows this help screen."
"read                   Show the headers of all posted messages."
"read new               Show headers of all mesgs less than 2 days old."
"read recent            Show headers of all mesgs after last read mesg."
"read KEYWORD           Show headers of all mesgs with matching KEYWORD."
"read -DAYS             Show headers of all mesgs fewer than DAYS old."
"read MESGNUM           Read the mesg referred to by the given mesg number."
"read -                 Read the next mesg, after the last one you read."
"read -recent           Read all mesgs after last read mesg, in one long dump."
"write                  Post a message.  Prompts for subject and keywords."
"write SUBJECT          Post a mesg with given SUBJECT.  Prompts for keywords."
"write SUBJECT=KEYWRDS  Post a message with given SUBJECT and KEYWRDS."
"erase MESGNUM          Lets message owner erase a previously written mesg."
"editmesg MESGNUM       Lets message owner edit a previously written mesg."
"protect MESGNUM        Lets a wizard protect a mesg from auto-expiration."
$ifdef GUI
"boardgui               Start the graphical interface if client supports it."
18
$else
17
$endif
showrange
;
  
  
: interface
    "me" match me !
 
    preempt
    dup strip "#help" stringcmp not if
        pop MBRD-showhelp
        exit
    then
    trigger @ exit? if
        trigger @ location
        get-bbsobj bbsobj !
        basename bbsobj @ MBRD-checkinit
        command @
        dup "write" instring if
            pop basename bbsobj @ MBRDadd
            handle-errs
            me @ location me @ me @ name
            " finishes writing on the bulletin board." strcat
            notify_except
            exit
        then
        dup "erase" instring if
            pop basename bbsobj @ MBRDkill
            handle-errs exit
        then
        dup "edit" instring if
            pop basename bbsobj @ MBRDedit
            handle-errs
            me @ location me @ me @ name
            " finishes editing a message on the bulletin board." strcat
            notify_except
            exit
        then
        dup "protect" instring if
            pop basename bbsobj @ MBRDprotect
            handle-errs exit
        then
$ifdef GUI
        dup "boardgui" instring if
            foreground
            pop messKeys !
            DESCR GUI_AVAILABLE 0.0 > if
                0 curMess !
                0 array_make messData !
                0 msg_index_gen
                gen_reader_dlog dup rot gui_dlog_register
                messDlog !
                gui_listchange
                "READ" 'input-handler EVENT_REGISTER
                "TIMER.update" 'gui-update EVENT_REGISTER
                120 "update" timer_start
                gui_event_process
                pop pop
                me @ location me @ me @ name " finishes browsing the bulletin board." strcat notify_except
                "Done." DESCR tell exit
            else
                "GUI not available with this client." DESCR tell exit
            then
        then
$endif
        pop basename bbsobj @ MBRDdisplay
        handle-errs exit
    then
    trigger @ get-bbsobj bbsobj !
    basename bbsobj @ MBRD-checkinit
    basename bbsobj @ MBRDdisplay
    handle-errs exit
;
.
c
q
@register gen-mesgboard=mesgboard
@register #me gen-mesgboard=tmp/prog1
@register #me gen-mesgboard=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3


