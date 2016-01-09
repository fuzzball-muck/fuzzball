( cmd-@doing  ver 1.2
    version 1.0 created by Whitefire.
    version 1.1 modified by Kimi.
       clears doing on logout.
    version 1.2 modified by Foxen.
       doesn't clear doing unless you logged out for longer than 20 minutes.
)
  
: add-doing (s -- )
    dup strlen 40 >
    if
        40 strcut strlen
        "Warning: " swap dup 1 >
        if
            intostr strcat " characters" strcat
        else
            intostr strcat " character" strcat
        then
        " lost." strcat .tell
    then
    me @ "_/do" rot 0 addprop
    me @ "Set." notify
;
  
: cmd-@doing ( s -- )
    strip
    dup not
    if
        pop
        me @ "You are curently doing: " me @ "_/do" getpropstr strcat notify 
        exit
    then
    dup tolower "#c" 2 strncmp not
    if
        pop
        me @ "_/do" remove_prop
        me @ "Cleared." notify
    else
        add-doing
    then
;
  
: clear-doing  ( -- )
    me @ "_/do" remove_prop
;
  
: main ( s -- )
    command @ "Queued event." stringcmp not
    if
        pop
        me @ awake? not  (awake? returns number of connections player has)
        if  (player disconected.  has 0 connections)
            me @ "@/doingpid" "" pid addprop  (remember the pid, fluke)
            20 60 * sleep  (sleep 20 minutes)
            me @ "@/doingpid" remove_prop (clear the pid)
            clear-doing
            exit
        else  (player connected.  >=1 connections)
            me @ "@/doingpid" getpropval
            dup if  (there's a logout process to nuke)
                dup ispid? if  (just make SURE that it's still there)
                    kill pop  (kill it if it is.)
                then
                me @ "@/doingpid" remove_prop (clear the logout pid)
            then
        then
        exit
    then
    cmd-@doing
;
