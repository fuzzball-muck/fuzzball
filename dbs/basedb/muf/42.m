( MUFmessageBoard v0.80   Copyright 5/31/91 by Garth Minette )
(                                           foxen@netcom.com )
( A program for storing and displaying multi-line messages   )
  
( This code may be freely distributed, and code from it may   
  used in other non-similar programs, but the author's name
  must be credited.                                          )
 
( CHANGES: This version modified by Jessy @ FurryMUCK. In
    func interface, 'trigger @ name' replaced with 'cmd @', so 
    that command aliases may be used in place of a separate 
    exit object for each command.                            )
 
$version 2.7
  
$def VERSION "MessageBoard v2.7"
 
$include $lib/edit
$include $lib/editor
$include $lib/lmgr
$include $lib/match
$include $lib/mesg
$include $lib/mesgbox
$include $lib/props
($include $lib/strings)
 
$def DAYOFFSET 7800
 
( ***** Misc. Object ***** )
 
: get-day ( -- dayint)
    systime dup 86400 % time 60 * + 60 * + - - 86400 /
$ifdef DAYOFFSET
    DAYOFFSET -
$endif
;
 
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
      " " split " " split swap atoi swap
    else
      "" 0 rot
    then
    " " split swap atoi dbref swap
    " " split swap atoi swap
;
 
: MBRDreparseinfo (keywords protect? poster day subject -- infostr)
    rot owner rot rot
    swap intostr " " strcat swap strcat
    swap int intostr " " strcat swap strcat
    swap intostr " " strcat swap strcat
    swap ";" " " subst tolower " " strcat
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
    MBOX-message EDITdisplay
    0 (No error.)
;
 
: MBRDdisplay (parmstr base dbref -- err)
    rot tolower -3 rotate (lowercase parmstr)
  
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
            MBOX-message EDITdisplay
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
    0 EDITOR pop
;
 
: MBRDadd (parmstr base dbref -- err)
    rot "=" split strip swap strip
    dup not if
        "What is the subject of this post?"
        me @ swap notify pop read strip
    then
    swap
    dup not if
        "What keywords fit this post? (ie: art, building, place, personal)"
        me @ swap notify pop read strip
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
  
    EDITOR pop dup not if
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
    read strip dup if swap then pop
  
    5 rotate
    me @ "Current keywords: \"" 3 pick strcat "\"" strcat notify
    "Enter new keywords, or press space and return to keep old ones."
    me @ swap notify
    read strip dup if swap then pop
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
 
( ***** Interface Object *****
)
$def basename "msgs"
 
: handle-errs
    dup not if pop me @ "Done." notify exit then
    dup 1 = if pop me @ "Should be a numeric parameter." notify exit then
    dup 2 = if pop me @ "Invalid message number." notify exit then
    dup 3 = if pop me @ "Permission denied." notify exit then
    dup 4 = if pop me @ "Cancelling post." notify exit then
    dup 5 = if pop me @ "Cancelling edit." notify exit then
    dup 6 = if pop me @ "No more messages." notify exit then
;
 
: get-bbsobj (default -- bbsdbref)
    dup "_bbsloc" getpropstr
    dup not if pop exit then
    dup number? not if pop exit then
    atoi dbref
    dup ok? not if pop exit then
    over owner over .controls
    if swap then pop
;
 
: MBRD-showhelp ( -- )
{
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
}tell
;
 
lvar bbsobj
: interface
    preempt
    "me" match me !
    dup strip "#help" stringcmp not if
        pop MBRD-showhelp
        exit
    then
    trigger @ get-bbsobj bbsobj !  ( Natasha@HLM 28 May 2002 )
    trigger @ exit? if
        basename bbsobj @ MBRD-checkinit
        (* 
           Patch: replace...
               
              trigger @ name
 
           with...
              
              cmd @
           
           ...so alias names can be used rather than a
           separate exit object for each command
        *)
        
        command @
        
        (* end patch *)
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
        pop basename bbsobj @ MBRDdisplay
        handle-errs exit
    then
    basename bbsobj @ MBRD-checkinit
    basename bbsobj @ MBRDdisplay
    handle-errs exit
;
