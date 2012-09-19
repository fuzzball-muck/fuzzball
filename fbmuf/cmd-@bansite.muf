@prog cmd-@bansite
1 9999 d
1 i
$def BannedPropdir "@banned/"
$def BannedPropdirLoc #0
  
: site-is-banned? (iDescr -- iTrue?)
    conhost BannedPropdir swap strcat
    BannedPropdirLoc BannedPropdir
    begin
        over swap nextprop dup
    while
        3 pick over smatch if
            pop pop pop 1 exit
        then
    repeat
    pop pop pop 0
;
  
: boot-banned-connects ( -- )
    me @ descriptors
    begin
        dup
    while
        1 - swap descrcon dup
        site-is-banned? if
            dup "Sorry, but connections from that site are not allowed."
            connotify conboot
        else pop
        then
    repeat
;
  
: list-banned-sites ( -- )
    me @ "Banned sites:" notify
    BannedPropdirLoc BannedPropdir
    begin
        over swap nextprop dup
    while
        "  " over strcat
        me @ swap notify
    repeat
    pop pop
    me @ "Done." notify
;
  
$def .tell me @ swap notify
: show-help ( -- )
    "@bansite v1.0  Written 11/11/93 by Foxen" .tell
    "----------------------------------------------------------------" .tell
    "@bansite SITENAME    Prevents logins from the given site." .tell
    "@bansite !SITENAME   Re-allows logins from the given site." .tell
    "@bansite #list       Lists all the sites that are banned." .tell
    "@bansite #help       Displays this screen." .tell
    "----------------------------------------------------------------" .tell
    "NOTE: Sitenames can be wildcard patterns.  ie: *.uloser.edu" .tell
;
  
: main
    preempt
    command @ "Queued event." stringcmp not if
        pop boot-banned-connects exit
    then
    me @ "wizard" flag? not if
        me @ "Permission denied." notify
        pop exit
    then
    strip
    dup "#list" stringcmp not if
        pop list-banned-sites exit
    then
    dup "#help" stringcmp not if
        pop show-help exit
    then
    dup "!" 1 strncmp not if
        1 strcut swap pop
        BannedPropdirLoc BannedPropdir rot strcat
        over over getpropstr if
            remove_prop "Site unbanned."
        else
            pop pop "That site was not banned."
        then
        me @ swap notify
    else
        BannedPropdirLoc BannedPropdir rot strcat
        over over getpropstr if
            pop pop "That site is already banned."
        else
            "banned" 0 addprop "Site banned."
        then
        me @ swap notify
    then
;
.
c
q
@register #me cmd-@bansite=tmp/prog1
@register #prop #0:_connect/ cmd-@bansite=bansite
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=3
@action @bansite=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
