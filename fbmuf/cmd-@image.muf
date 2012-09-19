@prog cmd-@image
1 99999 d
1 i
$include $lib/match
$include $lib/strings
  
$def IMAGE_PACKAGE "dns-org-fuzzball-image"
$def }tell }list { me @ }list array_notify
  
: list-imagers[ -- ]
    0 var! count
    preempt
    descr IMAGE_PACKAGE mcp_supports
    0.0 > if
        { }list var! names
        { }list var! targrefs
        { }list var! urls
        me @ location contents
        begin
            dup while
            dup room? if next continue then
            count @ 1 + dup count ! 150 <= while
            dup "_/image" getpropstr dup if
                urls @ array_append urls !
                dup targrefs @ array_append targrefs !
                dup name names @ array_append names !
            else
                pop
            then
            next
        repeat pop
        descr IMAGE_PACKAGE "viewable"
        {
            "names" names @
            "refs"  targrefs @
            "urls"  urls @
        }dict
        mcp_send
    else
        "(@imaged objects)" .tell
        me @ location contents
        begin
            dup while
            dup room? if next continue then
            count @ 1 + dup count ! 50 > if
                "(Too many objects in this room.  Skipping the remainder.)"
                .tell break
            then
            dup "_/image" getpropstr if
                dup name .tell
            then
            next
        repeat pop
        "(Done.)" .tell
    then
;
  
  
: cmd-@image[ str:cmdline -- ]
    cmdline @ tolower "#help" stringcmp not if
        {
        "@Image ver. 2.0                        Copyright 7/10/1994 by Revar"
        "-------------------------------------------------------------------"
        "@image <object>            To see where to find a gif of the object"
        "@image <obj>=<URL>         To specify where one can find a gif of  "
        "                             that object.  The URL is the WWW URL  "
        "                             format for specifying where on the net"
        "                             a file is.                            "
        "@image <obj>=clear         To clear the image reference.           "
        "-------------------------------------------------------------------"
        "URLs have the following format:   type://machine.name/path/file"
        "If I wanted to show that people can find an image of Revar on"
        "www.belfry.com, via the web, I'd just do:"
        "    @image Revar=http://www.belfry.com/pics/revar-cw3.jpg"
        "  "
        "Those of you who have used the web should find URLs familiar."
        }tell
        exit
    then
  
    cmdline @ not if
        list-imagers
        exit
    then
  
    cmdline @ "=" split strip
    var! newurl
  
    strip
    dup not if
        "Usage: @image <object>    or    @image <object>=<URL>" .tell
        pop exit
    then
    .noisy_match
    var! targref
  
    targref @ not if
        exit
    then
  
    newurl @ not if
        cmdline @ "=" instr if
            ( @image <object>= )
            "me" match targref @ controls not if
                "Permission denied." .tell
                exit
            then
  
            targref @ "_/image" "" 0 addprop
            "Image unset." .tell
            exit
        then
  
        ( @image <object> )
        targref @ "_/image" getpropstr
        " " "\r" subst
        var! currurl
  
        currurl @ not if
            "No image available." .tell
            exit
        then
  
        descr IMAGE_PACKAGE mcp_supports
        0.0 > if
            descr IMAGE_PACKAGE "view"
            {
                "name" targref @ name
                "ref"  targref @
                "url"  currurl @
            }dict
            mcp_send
        else
            "(@image) " currurl @ strcat .tell
        then
    else
        ( @image <object>=<URL> )
        "me" match targref @ controls not if
            "Permission denied." .tell
            exit
        then
  
        newurl @ "://" split strip swap strip
        dup "file" stringcmp
        over "http" stringcmp and
        over "ftp" stringcmp and if
            pop pop
            "Unknown URL service type.  The acceptable types are ftp, http, and file." .tell
            "Example:  http://www.furry.com/pics/revar-cw3.jpg" .tell
            exit
        then
        "://" strcat swap
        "/" split swap
        dup "*[^-:%.a-z0-9_]*" smatch if
            "Invalid character in machine name.  Valid chars are a-z, 0-9, _, period, colon, and -." .tell
            pop pop pop exit
        then
        "/" strcat swap
        strcat strcat
        targref @ "_/image" rot 0 addprop
        "Image set." .tell
    then
;
.
c
q
@register #me cmd-@image=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=3
@action @image;@imag;@ima;@im=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1
