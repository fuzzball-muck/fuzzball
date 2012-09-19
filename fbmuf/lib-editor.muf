@prog lib-editor
1 99999 d
1 i
( This code is hereby stated as public domain, for use in any program
  It might be needed for, under the condition that the original author's
  name is kept in the header, and full credit is given online in any
  programs written with it.
                - 6/25/1991 by Foxen                 foxen@belfry.com    )
 
( To use these routines from a program of yours, just do a:
  $include $lib/editor
 
EDITOR takes a bunch of strings on the stack, with a count on top, and
  returns a set of strings with the count, and a string on top containing
  the command used to exit. [so you can have abort or end.]  The format is:
      {strs} count -- {strs'} count' exitcmdstr
 
EDITORloop is more complex. it takes the strings and count [hereafter
  to be referred to as a range], a string containing the space
  seperated names of the commands it should return on [in addition to
  abort and end], the position in the range that you want it to set the
  insertion point at, and the command you want it to first execute
  [usually ".i"].
    It lets the user edit the range until the user enters a .command that
  you have listed in the mask, or until they enter .end, or .abort.
    It returns the new range, the mask, the insertion point that the user
  was at when it returned, and the arguments that were given to the
  command that made the routine return, and the name of the command
  that made it return.  The format of a .command is:
      .cmd <startline> <endline> =<argstr>
  So, overall, the input/output of the function is:
      {rng} mask curpos cmdstr --
                      {rng'} mask curpos argstr startline endline exitcmd
 
EDITORparse takes and returns arguments identically to .sedit_loop,
  but only executes the command you pass it without doing any more
  editing on the range, before returning.
 
EDITORheader takes and returns no arguments, but prints out a standard
  message about you entering the editor.  EDITOR calls this automatically.
)
 
$version 6.002
$lib-version 6.001
$author foxen@belfry.com

$include $lib/strings
$include $lib/gui
$include $lib/stackrng
 

$define SRNGcat     sr-catrng     $enddef
$define SRNGpop     sr-poprng     $enddef
$define SRNGextract sr-extractrng $enddef
$define SRNGswap    sr-swaprng    $enddef
$define SRNGcopy    sr-copyrng    $enddef
$define SRNGdelete  sr-deleterng  $enddef
$define SRNGinsert  sr-insertrng  $enddef
 
 
 
 
$include $lib/edit
 
( ***** Stack based string range editor -- EDITOR *****
 1  EDITOR      [ {string_range} -- {string_range'} ]
 2  EDITORloop  [ {str_rng} maskstr currline cmdstr --
                  {str_rng'} mask currline arg3str arg1ing arg2int cmdname ]
 3  EDITORparse [ {string_range} command -- {string_range'} mask curr 1     or
                  {str_rng'} mask currline arg3str arg1ing arg2int cmdname 0 ]
)
 
: EDITORerror ( errnum -- )
    dup       1 = if pop "Invalid line reference."
    else dup  2 = if pop "Error: Line referred to is before first line."
    else dup  3 = if pop "Error: Line referred to is after last line."
    else dup  4 = if pop "Error: 1st line ref is after 2nd reference."
    else dup  5 = if pop "Warning: First line reference ignored."
    else dup  6 = if pop "Warning: Second line reference ignored."
    else dup  7 = if pop "Warning: String argument ignored."
    else dup  8 = if pop "Error: Unknown command.  Enter '.h' for help."
    else dup  9 = if pop "Error: Command needs string parameter."
    else dup 10 = if pop "Error: Must have pattern to search for."
    else dup 11 = if pop "Error: Must have a destination line reference."
    else dup 12 = if pop "Error: Columns parameter invalid."
    else pop "Unknown error."
    then then then then then
    then then then then then
    then then
    "< ** " swap strcat " ** >" strcat
    me @ swap notify
;
 
 
: EDITORargument ( endline currentline string -- linenum )
    dup "$" 1 strncmp not if
        1 strcut swap pop
        swap pop over 1 + swap
    else dup "^" 1 strncmp not if
        1 strcut swap pop
        swap pop 1 swap
    else dup "." 1 strncmp not if
        1 strcut swap pop
    then then then
 
    dup not if pop swap pop exit then
 
    dup "+" 1 strncmp not if 1 strcut swap pop atoi +
    else dup "-" 1 strncmp not if 1 strcut swap pop atoi -
    else dup number? if atoi swap pop
    else pop pop pop 1 EDITORerror 0
    then then then
 
    (max line)
    dup 1 < if pop 1
    else
        swap 1 + over over
        > if swap then pop
    then
;
 
 
: EDITORmesg  (int1 int2 int3 string -- )
    swap intostr "%3" subst
    swap intostr "%2" subst
    swap intostr "%1" subst
    me @ swap notify
;
 
 
lvar origlist
lvar guilist
lvar guisaved
 
: setguiuse ( ref:player bool:usegui -- )
    "_prefs/lsedit/usegui" swap setprop
;
 
: save-callback[ dict:ctx str:dlogid str:ctrlid str:event -- int:exit ]
    ctx @ "values" [] var! guivalues
    guivalues @ "body" []
    array_vals over not over and if
        -- swap pop
    then
    array_make
    { }list swap
    foreach swap pop
        dup not if pop " " then
        swap array_appenditem
    repeat
    guilist !
    guivalues @ "usegui" [] 0 [] atoi me @ swap setguiuse
    1 guisaved !
    dlogid @ gui_dlog_deregister
    dlogid @ gui_dlog_close
    1
;
 
: usegui? ( ref:player -- bool:usegui )
    "_prefs/lsedit/usegui" getpropval
    descr GUI_AVAILABLE 1.0 >= and
;
 
: gui-make-editor-dlog[ arr:strList -- dict:Handlers str:dlogid ]
    {SIMPLE_DLOG "Post Message"
        "resizable" "xy"
        {MULTIEDIT "body"
            "value" strList @
            "width" 80
            "height" 20
            "colspan" 1
            "sticky" "nsew"
            "hweight" 1
            "vweight" 1
            }CTRL
        {FRAME "bfr"
            "sticky" "ew"
            "colspan" 1
            {CHECKBOX "usegui"
                "text" "Use &GUI editor instead of command line in future edits"
                "value" me @ usegui?
                "sticky" "w"
                "hweight" 1
                "newline" 0
                }CTRL
            {BUTTON "SaveBtn"
                "text" "&Save"
                "width" 8
                "sticky" "e"
                "newline" 0
                "dismiss" 1
                "|buttonpress" 'save-callback
                }CTRL
            {BUTTON "CancelBtn"
                "text" "&Cancel"
                "width" 8
                "sticky" "e"
                "dismiss" 1
                }CTRL
        }CTRL
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
 
: do-gui-edit[ arr:text -- arr:text int:didsave int:supported ]
    descr GUI_AVAILABLE 1.0 >= if
        "\[[1m< Opening GUI editor >" .tell
        " " .tell
        caller name "*lsedit*" smatch if
            background
        then
        0 guisaved !
        text @ guilist !
        text @ gui-make-editor-dlog swap gui_dlog_register
        gui_event_process pop pop
        guilist @
        guisaved @
        1
    else
        "\[[1m< GUI editing not supported by user's MUD client >" .tell
        0 0
    then
;
 
 
: EDITORparse ( {str_rng} mask currline cmdstr --
                {str_rng'} mask currline arg1 arg2 exitcmd 1   or
                {str_rng'} mask currline 0                        )
    dup not if pop read then
    dup not if pop " " then
    dup "\"" 1 strncmp not if
        1 strcut swap pop
        "< In the editor > You say, \"" over strcat "\"" strcat
        me @ swap notify me @ name " says, \"" strcat
        swap strcat "\"" strcat
        me @ location me @ rot notify_except
        1 exit
    then
    dup ":" 1 strncmp not if
        1 strcut swap pop
        me @ name " " strcat swap strcat
        "< In the editor > " over strcat
        me @ swap notify
        me @ location me @
        rot notify_except
        1 exit
    then
    dup ".\"" 2 strncmp not over ".:" 2 strncmp not or
    over ".." 2 strncmp not or if 1 strcut swap pop 1 else 0 then
    over "." 1 strncmp or if
        ( {str_rng} mask currlinenum string )
        4 pick 5 + 3 pick - -1 * rotate
        1 + rot 1 + rot rot 1 exit
    then
    " " STRsplit swap 1 strcut swap pop swap
    "=" STRsplit swap STRstrip
    dup STRstrip if
        " " STRsplit STRstrip swap STRstrip
        7 pick 6 pick rot EDITORargument
        ( {str_rng} mask currline cmd arg3str arg2str arg1int )
        dup not if
            pop pop pop pop 1 exit
        then
        swap dup if
            ( {str_rng} mask currline cmd arg3str arg1int arg2str )
            7 pick 6 pick rot EDITORargument dup not if
                pop pop pop pop 1 exit
            then
            over over > if
                4 EDITORerror
                pop pop pop pop 1 exit
            then
        else pop 0
        then
    else pop 0 0
    then
 
    ( {strrng} mask currline cmdstr arg3str args1int arg2int )
    4 rotate dup " " swap over strcat strcat
    7 pick " " swap over strcat strcat STRsinglespace swap instr if
        0 exit
    then
 
    ( {strrng} mask currline arg3str args1int arg2int cmdstr )
    dup "i" stringcmp not if
        pop if 6 EDITORerror then
        dup not if pop over then
        swap if 7 EDITORerror then
        dup 0 0 "< Insert at line %1 >" EDITORmesg
        swap pop 1 exit
    then
    dup "del" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop over not if swap pop 3 pick swap then
        dup not if pop dup then
        rot if 7 EDITORerror then
        over 3 put over - 1 +
        over over 0
        "< deleting %2 lines starting at line %1  (Now current line) >"
        EDITORmesg
        2 swap rot SRNGdelete
        1 exit
    then
    dup "l" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot if 7 EDITORerror then
        over not over not and if pop pop 1 4 pick then
        dup not if pop dup then
        over over swap - 1 + 3 pick
        -4 rotate -4 rotate
        ( {strrng} mask currline cnt frst start end )
        4 0 4 rotate 4 rotate EDITlist
        0 "< listed %1 lines starting at line %2 >"
        EDITORmesg 1 exit
    then
    dup "p" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot if 7 EDITORerror then
        over not over not and if pop pop 1 4 pick then
        dup not if pop dup then
        over over swap - 1 + 3 pick
        -4 rotate -4 rotate
        ( {strrng} mask currline cnt frst start end )
        4 1 4 rotate 4 rotate EDITlist
        0 "< listed %1 lines starting at line %2 >"
        EDITORmesg 1 exit
    then
    dup "find" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop if 6 EDITORerror then
        dup not if pop over then
        over not if 10 EDITORerror pop pop 1 exit then
        2 rot rot EDITsearch dup if
            dup 0 0 "< Found.  Going to line %1 >"
            EDITORmesg swap pop
            2 over dup 1 EDITlist
        else
            pop 0 0 0
            "< pattern not found >"
            EDITORmesg
        then
        1 exit
    then
    dup "move" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop 3 pick not if 11 EDITORerror pop pop pop 1 exit then
        over not if pop pop over dup then
        dup not if pop dup then
        rot 6 pick 5 pick rot STRstrip EDITORargument
        dup not if pop pop pop 1 exit then rot rot
        ( {strrng} mask currline arg3i arg1i arg2i )
 
        over over swap - 1 + 3 pick 5 pick
        "< Moved %1 lines from line %2 to line %3.  (dest now curr line) >"
        EDITORmesg 3 pick 4 put
        2 -4 rotate EDITmove 1 exit
    then
    dup "copy" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop 3 pick not if 11 EDITORerror pop pop pop 1 exit then
        over not if pop pop over dup then
        dup not if pop dup then
        rot 6 pick 5 pick rot STRstrip EDITORargument
        dup not if pop pop pop 1 exit then rot rot
 
        over over swap - 1 + 3 pick 5 pick
        "< Copied %1 lines from line %2 to line %3.  (now current line) >"
        EDITORmesg 3 pick 4 put
        2 -4 rotate EDITcopy 1 exit
    then
    dup "repl" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop over not over not and if pop pop 1 5 pick then
        dup not if pop dup then
        rot dup not if
            9 EDITORerror
            pop pop pop 1 exit
        then
        ( {strrng} mask currline args1int arg2int arg3str )
        1 strcut swap STRsplit -4 rotate
        dup not if
            pop pop pop pop
            10 EDITORerror 1 exit
        then
        -4 rotate 3 -5 rotate
        7 5 pick 4 pick EDITsearch dup if
            dup 0 0 "< Replaced.  Going to line %1 >"
            EDITORmesg dup 7 put
            rot pop swap over -6 rotate
            ({rng} mask currline first 3 oldtext newtext first arg2int)
            EDITreplace ({rng} mask currline first)
            2 1 rot dup ({rng} mask currline 2 1 first first) EDITlist
        else
            pop pop pop pop pop pop
            0 0 0 "< pattern not found >"
            EDITORmesg
        then
        1 exit
    then
    dup "join" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot if 7 EDITORerror then
        over not if pop pop dup dup 1 + then
        dup not if pop dup 1 + then over 3 put
        over over swap - 1 + 3 pick 0
        "< Joining %1 lines starting at line %2. (Now current line) >"
        EDITORmesg 2 rot rot EDITjoin_rng
        1 exit
    then
    dup "split" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop if 6 EDITORerror then
        dup not if pop over then
        over not if 10 EDITORerror pop pop 1 exit then
        ( {strrng} mask currline arg3str args1int )
        5 pick 6 + over - dup 1 + rotate
        ( {rng} mask currline ars3str arg1int rot str)
        dup 5 pick instr dup if
            5 rotate strlen + 1 - strcut
            ( {rng} mask currline arg1int rot strb stre )
            rot 1 + -1 * swap over rotate
            1 + rotate dup 0 0
            "< Split line %1.  (Now current line) >"
            EDITORmesg swap pop
            rot 1 + rot rot
        else
            pop swap -1 * rotate
            0 0 0 "< Text not found.  Line not split. >"
            EDITORmesg pop pop
        then 1 exit
    then
    dup "gui" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        6 array_make var! tmp_store
        array_make do-gui-edit if
            var! tmp_didsave array_vals
            tmp_store @ array_vals pop
            0 0 0 "< Editor exited. >"
            EDITORmesg
            pop tmp_didsave @ if "end" else "abort" then
            0 exit
        else
            pop array_vals
            tmp_store @ array_vals pop
            1 exit
        then
    then
    dup "edit" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop if 6 EDITORerror then
        dup not if pop over then
        swap if 7 EDITORerror then
        ( {strrng} mask currline args1int )
        4 pick 4 + over - dup 2 + rotate
        ( {rng} mask currline arg1int rot str)
        rot dup 4 put 0 0 "< Editing line %1. >" EDITORmesg
        ( {rng} mask currline rot str)
        "##edit> " swap strcat me @ swap notify
        read swap -1 * rotate 1 exit
    then
    dup "left" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot if 7 EDITORerror then
        over not if pop pop dup dup then
        dup not if pop dup then
        over over swap - 1 + 3 pick 0
        "< Left justifying %1 lines starting at line %2. >"
        EDITORmesg 2 rot rot EDITleft
        1 exit
    then
    dup "center" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot dup not if pop "72" then
        dup number? not if
            pop pop pop 12 EDITORerror 1 exit
        then atoi
        dup 160 > if pop 160 then
        dup 40 < if pop 40 then rot rot
        over not if pop pop over dup then
        dup not if pop dup then
        over over swap - 1 + 3 pick 5 pick
        "< Centered %1 lines starting at line %2 for screenwidth %3. >"
        EDITORmesg 2 -4 rotate EDITcenter 1 exit
    then
    dup "right" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot dup not if pop "72" then
        dup number? not if
            pop pop pop 12 EDITORerror 1 exit
        then atoi
        dup 160 > if pop 160 then
        dup 40 < if pop 40 then rot rot
        over not if pop pop over dup then
        dup not if pop dup then
        over over swap - 1 + 3 pick 5 pick
        "< Right justified %1 lines starting at line %2 to column %3. >"
        EDITORmesg 2 -4 rotate EDITright 1 exit
    then
    dup "format" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot STRstrip dup not if pop "72" then
        dup number? not if
            pop pop pop 12 EDITORerror 1 exit
        then atoi
        dup 160 > if pop 160 then
        dup 40 < if pop 40 then rot rot
        over not if pop pop over dup then
        dup not if pop dup then over 4 put
        over over swap - 1 + 3 pick 5 pick
"< Formatted %1 lines starting at line %2 (Now curr line) to %3 columns. >"
        EDITORmesg 2 -4 rotate EDITfmt_rng 1 exit
    then
    dup "indent" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        pop rot dup not if pop "2" then
        dup number? not if
            pop pop pop 12 EDITORerror 1 exit
        then atoi
        dup 80 > if pop 80 then
        dup -80 < if pop -80 then rot rot
        over not if pop pop over dup then
        dup not if pop dup then
        over over swap - 1 + 3 pick 5 pick
        "< Indented %1 lines starting at line %2, %3 columns. >"
        EDITORmesg 2 -4 rotate EDITindent 1 exit
    then
    dup "shuffle" stringcmp not over "sort" stringcmp not or if  ( Added Natasha@HLM 22 December 2002 )
        "shuffle" stringcmp if SORTTYPE_CASEINSENS else SORTTYPE_SHUFFLE then var! sorthow  ( {strrng} mask currline arg3str args1int arg2int )
        rot if 7 EDITORerror then  ( {strrng} mask currline args1int arg2int )
 
        ( Square away the arguments. )
        var! line2  ( {strrng} mask currline args1int )
        dup not if pop 1    then var! line1  ( {strrng} mask currline )
        var! currline var! mask  ( {strrng} )
        array_make  ( arrStr )
        line2 @ not if dup array_count line2 ! then  ( arrStr )
 
        ( Extract a list of the lines to shuffle. )
        { line1 @ -- begin dup line2 @ < while dup ++ repeat }list  ( arrStr arrLines }  -- and < because arrays are 0-based where lists are 1-based. )
        over swap array_extract array_vals array_make  ( arrStr arrStrsToShuffle )
        swap line1 @ -- line2 @ -- array_delrange swap  ( arrStr arrStrsShuffled )
 
        sorthow @ array_sort  ( arrStr arrStrsShuffled )
 
  ( Put the shuffled lines back. )
        line1 @ -- swap array_insertrange  ( arrStr' )
 
        ( Get ready to return. )
        array_vals  ( {strrng'} )
        line1 @ line2 @ 0  ( {strrng'} int int int )
        sorthow @ SORTTYPE_SHUFFLE = if "shuffl" else "sort" then "< Lines %%1 through %%2 %sed. >" fmtstring EDITORmesg  ( {strrng'} )
        mask @ currline @ 1 exit  ( {strrng'} mask currline 1 )
    then
    dup "end" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        0 0 0 "< Editor exited. >"
        EDITORmesg
        0 exit
    then
    dup "abort" stringcmp not if
        ( {strrng} mask currline arg3str args1int arg2int cmdstr )
        0 0 0 "< Edit aborted. >"
        EDITORmesg
        pop pop pop pop pop pop SRNGpop
        0 "" 1 "" 1 1 "abort" 0 exit
    then
    dup "h" stringcmp not if
{
"          MUFedit Help Screen.  Arguments in [] are optional."
"    Any line not starting with a '.' is inserted at the current line."
"Lines starting with '..', '.\"' , or '.:' are added with the '.' removed."
"-------  st = start line   en = end line   de = destination line  -------"
" .end                    Exits the editor with the changes intact."
" .abort                  Aborts the edit."
" .h                      Displays this help screen."
" .i [st]                 Changes the current line for insertion."
" .l [st [en]]            Lists the line(s) given. (if none, lists all.)"
" .p [st [en]]            Like .l, except that it prints line numbers too."
" .del [st [en]]          Deletes the given lines, or the current one."
" .copy [st [en]]=de      Copies the given range of lines to the dest."
" .move [st [en]]=de      Moves the given range of lines to the dest."
" .find [st]=text         Searches for the given text starting at line start."
" .repl [st [en]]=/old/new  Replaces old text with new in the given lines."
" .join [st [en]]         Joins together the lines given in the range."
" .split [st]=text        Splits given line into 2 lines.  Splits after text"
" .sort [st en]           Sorts the lines of the list case-insensitively."  ( Added Natasha@HLM 22 December 2002 )
" .shuffle [st en]        Shuffles the lines of the list."  ( Added Natasha@HLM 22 December 2002 )
" .left [st [en]]         Aligns all the text to the left side of the screen."
" .center [st [en]]=cols  Centers the given lines for cols screenwidth."
" .right [st [en]]=col    Right justifies to column col."
" .indent [st [en]]=cols  Indents or undents text by cols characters"
" .format [st [en]]=cols  Formats text nicely to cols columns."
"---- Example line refs:  $ = last line, . = curr line, ^ = first line. ----"
"12 15 (lines 12 to 15)    5 $ (line 5 to last line)    ^+3 6 (lines 4 to 6)"
".+2 $-3 (curr line + 2 to last line - 3)     5 +3 (line 5 to curr line + 3)"
}
        EDITdisplay
        pop pop pop pop 1 exit
    then
    pop pop pop pop 8 EDITORerror 1
;
 
 
: EDITORloop ( {rng} mask currpos cmdstr --
              {rng'} mask currpos arg3str arg1int arg2int exitcmd )
    EDITORparse if "" EDITORloop then
;
 
: EDITORheader ( -- )
    {
        "\[[1m< Entering editor.  Type '.h' on a line by itself for help. >"
        "\[[1m< '.end' will exit the editor.   '.abort' aborts the edit.  >"
        "\[[1m< Poses and says will pose and say as usual.  To start a    >"
        "\[[1m< line with : or \" just preceed it with a period  ('.')     >"
        descr gui_available 1.0 >= if
            "\[[1m< If your client supports MCP-GUI, '.gui' will launch a GUI >"
            "\[[1m< text editor in another window.                            >"
        then
    }list { me @ }list array_notify
;
 
: EDITOR ( {str_rng} -- {str_rnd'} exitcmdstr )
    me @ usegui? not if
        EDITORheader
        "" 1 ".i $" EDITORloop -6 rotate pop pop pop pop pop
    else
        "" 1 ".gui" EDITORloop -6 rotate pop pop pop pop pop
    then
;
 
PUBLIC EDITOR
PUBLIC EDITORloop
PUBLIC EDITORparse
PUBLIC EDITORheader
.
c
q
@register lib-editor=lib/editor
@register #me lib-editor=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_defs/editor:"$lib/editor" match "EDITOR" call
@propset $tmp/prog1=str:/_defs/editorheader:"$lib/editor" match "EDITORheader" call
@propset $tmp/prog1=str:/_defs/editorloop:"$lib/editor" match "EDITORloop" call
@propset $tmp/prog1=str:/_defs/editorparse:"$lib/editor" match "EDITORparse" call
@propset $tmp/prog1=str:/_defs/EDITORprop:"$lib/editor" match "EDITORprop" call
@propset $tmp/prog1=str:/_docs:@list $lib/editor=2-57
@propset $tmp/prog1=str:/_lib-version:2.0.1


