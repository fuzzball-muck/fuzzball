@prog mcp-extern.muf
1 99999 d
1 i
( mcp-extern v1.0  Copyright Nov.2001 by Revar.  Released under the LGPL.
  
  A useful program to allow scripts outside the muck to send event messages
  to the muck, to perform certain actions.  All actions should be authenticated
  with an AUTHTOKEN in the auth argument.  Currently, the following messages
  are implemented:
  
      org-fuzzball-extern-wall auth mesg
              @walls the value of mesg to everyone logged into the muck.
              The mesg may be multiple lines long.  Each line is prepended
              with a '# ' sequence.
  
  The program outside the muck should send MCP messages to the muck via
  the normal telnet port.  It doesn't need to log in as a player.  The
  utility 'netcat' or 'nc' is useful for sending this message.  The MCP
  message sequence should look something like:
  
    #$#mcp authentication-key: "XYZ123" version: "2.1" to: "2.1"
    #$#mcp-negotiate-can XYZ123 package: "mcp-negotiate" min-version: "2.0" max-version: "2.0"
    #$#mcp-negotiate-can XYZ123 package: "org-fuzzball-extern" min-version: "1.0" max-version: "1.0"
    #$#mcp-negotiate-end XYZ123
    #$#org-fuzzball-extern-wall XYZ123 auth: "AUTHTOKEN" mesg*: "" _data-tag: "XYZZY"
    #$#* XYZZY mesg: This is the message to @wall.
    #$#* XYZZY mesg: It can be multiple lines long.
    #$#: XYZZY
  
  NOTE:  XYZZY and XYZ123 should be two different alphanumeric strings with
         no spaces that you have made up that nobody else can easily guess.
  
  NOTE: AUTHTOKEN should match the AUTHTOKEN defined below.
)
  
  
$def AUTHTOKEN "NON-TRIVIAL_PASSWORD"

$def ALLOWED_HOSTS "localhost" "127.0.0.1"
  
: check_auth[ int:dscr dict:args -- int:is_ok ]
    { ALLOWED_HOSTS }list dscr @ descrhost 
    array_findval not if 0 exit then

    args @ "auth" [] 0 []
    AUTHTOKEN strcmp not
;
   
: do_wall[ int:dscr dict:args -- ]
    online_array { }list array_union var! whoall
    { }list var! out
    args @ "mesg" []
    foreach swap pop
        "# " swap strcat
        out @ array_appenditem out !
    repeat
    out @ whoall @ array_notify
;
   
: main[ str:args -- ]
    "org-fuzzball-extern" 1.0 1.0 MCP_REGISTER_EVENT
    begin
        {
            "MCP.org-fuzzball-extern-wall" 'do_wall
        }dict
        dup array_keys array_make
        event_waitfor var! event var! ctx
        event @ [] dup if
            ctx @ "descr" []
            ctx @ "args" []
            over over check_auth if
                rot execute
            else
                ( If not authorized, silently drop. )
                pop pop pop
            then
        else
            pop
        then
    repeat
;
.
c
q
@register #me mcp-extern.muf=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=A
@set $tmp/prog1=3
