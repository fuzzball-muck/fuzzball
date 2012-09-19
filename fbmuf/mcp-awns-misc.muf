@prog mcp-awns-misc
1 99999 d
1 i
( mcp-awns-misc   copyright 9/21/2002 by Revar Desmera                 )
(                                                                      )
( Support daemon for the following miscellaneous MCP packages:         )
(                                                                      )
(   dns-com-awns-ping     1.0                                          )
(       Allows a client to determine its ping latency.                 )
(                                                                      )
(   dns-com-awns-timezone 1.0                                          )
(       Allows a client to specify its timezone.                       )
(                                                                      )
(   org-fuzzball-timezone 1.0                                          )
(       Allows a client to specify its timezone more accurately.       )
 
$author Revar Desmera <revar@belfry.com>
$version 1.001
$lib-version 1.0
$note Released under the LGPL.
 
 
$def PKG_PING     "dns-com-awns-ping"
$def PKG_TIMEZONE "dns-com-awns-timezone"
$def PKG_FBTZONE  "org-fuzzball-timezone"
 
 
: timezone_to_offset[ str:zone -- int:offset ]
    zone @ "[+-][0-9][0-9][0-9][0-9]" smatch if
        zone @ 1 strcut 2 strcut
        atoi 60 * swap atoi 3600 * +
        swap "1" strcat atoi *
        exit
    then
 
    zone @ "GMT[+-][0-9]" smatch if
        zone @ 3 strcut swap pop
        atoi 3600 *
        exit
    then
 
    zone @ "GMT[+-][0-9][0-9]" smatch if
        zone @ 3 strcut swap pop
        atoi 3600 *
        exit
    then
 
    {
        "HST"  -10 3600 *
        "AKST"  -9 3600 *
        "AKDT"  -8 3600 *
        "YST"   -8 3600 *
        "PST"   -8 3600 *
        "PDT"   -7 3600 *
        "MST"   -7 3600 *
        "MDT"   -6 3600 *
        "CST"   -6 3600 *
        "CDT"   -5 3600 *
        "EST"   -5 3600 *
        "EDT"   -4 3600 *
        "AST"   -4 3600 *
        "NST"   -3.5 3600 * int
        "ADT"   -3 3600 *
        "NDT"   -2.5 3600 * int
        "GMT"    0
        "UT"     0
    }dict zone @ []
    exit
;
 
 
: awns-timezone[ int:dscr dict:args -- ]
    args @ "timezone" [] 0 []
    timezone_to_offset
    dscr @ descrdbref "_tzoffset" rot setprop
;
 
   
: fb-timezone[ int:dscr dict:args -- ]
    args @ "tzoffset" [] 0 [] atoi
    dscr @ descrdbref "_tzoffset" rot setprop
;
 
 
: awns-ping[ int:dscr dict:args -- ]
    args @ "id" [] 0 [] var! id
    dscr @ PKG_PING "reply" {
        "id" id @
    }dict mcp_send
;
 
   
lvar pings_pending
 
: generate-id[ int:dscr  -- str:id ]
    begin
        srand abs intostr var! id
        pings_pending @
        dup not if pop { }dict dup pings_pending ! then
        { dscr @ "-" id @ }join
        [] not
    until
    id @
;
 
: server-pinguser[ int:dscr dict:args -- ]
    args @ "data" [] var! targdscr
    args @ "caller_pid" [] var! clientpid
    clientpid @ not if exit then
    targdscr @ PKG_PING mcp_supports not if
        clientpid @ "pinguser" -1 event_send
        exit
    then
    targdscr @ generate-id var! id
    targdscr @ PKG_PING "" {
        "id" id @
    }dict mcp_send
    systime_precise var! stime
    {
        "clientpid" clientpid @
        "starttime" stime @
    }dict
    pings_pending @
    dup not if pop { }dict then
        { targdscr @ "-" id @ }join
    ->[] pings_pending !
;
 
: awns-ping-reply[ int:dscr dict:args -- ]
    systime_precise var! etime
    args @ "id" [] 0 [] var! id
    pings_pending @
    dup not if pop { }dict dup pings_pending ! then
    { dscr @ "-" id @ }join [] var! context
    context @ if
        pings_pending @ { dscr @ "-" id @ }join array_delitem pings_pending !
        context @ "starttime" [] var! stime
        etime @ stime @ -
        context @ "clientpid" []
        dup ispid? if
            "pinguser" rot event_send
        else pop pop
        then
    then
;
 
   
: server_init_if_needed[ -- ]
    prog "_serverpid" getprop dup if
        ispid? if exit then
    else pop
    then
    fork dup if
        ( The parent branch, AKA the client )
        prog "_serverpid" rot setprop
    else
        ( The child branch, AKA the server. )
        pop
        PKG_PING     1.0 1.0 MCP_REGISTER_EVENT
        PKG_TIMEZONE 1.0 1.0 MCP_REGISTER_EVENT
        PKG_FBTZONE  1.0 1.0 MCP_REGISTER_EVENT
        begin
            {
                { "MCP." PKG_PING          }join 'awns-ping
                { "MCP." PKG_PING "-reply" }join 'awns-ping-reply
                { "MCP." PKG_TIMEZONE      }join 'awns-timezone
                { "MCP." PKG_FBTZONE       }join 'fb-timezone
                { "USER.pinguser"          }join 'server-pinguser
            }dict
            dup array_keys array_make
            event_waitfor var! event var! ctx
            event @ [] dup if
                ctx @ "descr" []
                ctx @ "args" [] dup not if
                    pop ctx @
                then
                rot execute
            else
                pop
            then
        repeat
    then
;
 
: client_server_send[ str:type any:context -- ]
    server_init_if_needed
    prog "_serverpid" getprop
    type @ context @ event_send
;
 
 
: client_server_wait_for_reply[ str:type int:timeout -- any:context int:success ]
    server_init_if_needed
    prog "_serverpid" getprop var! serverpid
    timeout @ if timeout @ "_cs_timeout" timer_start then
    begin
        {
            { "USER." type @ }join
            "TIMER._cs_timeout"
        }list
        event_waitfor
        dup "TIMER._cs_timeout" strcmp not if
            pop 0
            exit
        else
            pop dup "caller_pid" []
            serverpid @ = if
                1 break
            else
                pop
            then
        then
    repeat
    pop "data" []
;
 
 
: ping-client[ int:dscr -- flt:secs ]
    "pinguser" dscr @
    client_server_send
    "pinguser" 30
    client_server_wait_for_reply
    dup not if pop inf then
;
public ping-client
$libdef ping-client
 
: main[ str:args -- ]
    server_init_if_needed
    command @ "ping" stringcmp not if
        args @ pmatch dup not if
            "I don't know who you mean!" .tell exit
        then
        dup awake? not if
            name " is not awake." strcat .tell exit
        then
        dup descrleastidle
        ping-client
        dup inf = if
            pop "%D did not respond to the ping." fmtstring .tell
        else
            dup -1 = if
                pop "%D does not have a client that supports pinging." fmtstring .tell
            else
                1000 *
                swap "%D has a %.0f ms ping time." fmtstring .tell
            then
        then
        descr descrflush
        exit
    then
;
 
.
c
q
@register #me mcp-awns-misc=tmp/prog1
@register #me mcp-awns-misc=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=A
@set $tmp/prog1=3

