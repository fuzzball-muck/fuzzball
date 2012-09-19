@prog cmd-@tunegui
1 99999 d
1 i
( cmd-@tunegui  Copyright 7/23/2002 by Revar <revar@belfry.com> )
$author Revar Desmera <revar@belfry.com>
$version 1.010
$note Released under the GNU LGPL.
 
$include $lib/case
$include $lib/optionsgui
 
 
: val2str[ any:val str:type -- str:result ]
    type @ case
        "string"   stringcmp not when val @ end
        "integer"  stringcmp not when val @ intostr end
        "timespan" stringcmp not when val @ intostr "s" strcat end
        "float"    stringcmp not when val @ ftostr end
        "dbref"    stringcmp not when val @ "%d" fmtstring end
        "boolean"  stringcmp not when val @ if "yes" else "no" then end
        default val @ "%~" fmtstring end
    endcase
;
 
: tunegui_save_cb[ dict:extradata dict:optionsinfo -- str:errs ]
    "" var! errs
    optionsinfo @ foreach var! ctrlinfo var! ctrlname
        ctrlinfo @ "type"     [] var! ctrltype
        ctrlinfo @ "label"    [] var! ctrllbl
        ctrlinfo @ "value"    [] ctrltype @ val2str var! ctrlval
        ctrlinfo @ "changed" [] if
            0 try
                ctrlname @ ctrlval @ setsysparm
            catch pop
                errs @
                dup if "\r" strcat then
                ctrllbl @ swap "%sBad value for '%s'." fmtstring
                errs !
            endcatch
        then
    repeat
    errs @
;
 
 
: main[ str:args -- ]
    me @ "wizard" flag? not
$ifdef __muckname=FurryMUCK
    me @ #1026 dbcmp not and
$endif
    if
        "You pull out a harmonica and play a tune, but the harmonica gets all sticky."
        .tell exit
    then
    descr gui_available 0.0 = if
        "Your client doesn't support the MCP-GUI package.  Try using @tune instead."
        .tell exit
    then
 
    "" sysparm_array
    SORTTYPE_CASEINSENS "type"  array_sort_indexed
    SORTTYPE_CASEINSENS "group" array_sort_indexed
    var! parms
 
    descr 0 'tunegui_save_cb "Tunable System Parameters" parms @
    gui_options_process
;
.
c
q
@register #me cmd-@tunegui=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3

