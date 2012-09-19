@prog lib-optionsinfo
1 99999 d
1 i
( $lib/optionsinfo Copyright 8/13/2003 by Revar <revar@belfry.com> )
( Released under the GNU LGPL.                                     )
(                                                                  )
( A MUF library to provide common internal support for gui and     )
( menu driven editing of option values.                            )
(                                                                  )
( v1.000 --  8/13/2003 -- Revar <revar@belfry.com>                 )
(     Initial relase.                                              )

$author Revar Desmera <revar@belfry.com>
$version 1.000
$lib-version 1.000
$note Released under the LGPL.

(
    Docs go here.
)

( ------------------------------------------------------------- )


lvar opts_info_arr
lvar opts_grouplabels_arr
lvar opts_groupsorder_arr
lvar opts_grouporder_arr
lvar opts_info_topnum

: optionsinfo_get[ int:id -- arr:optionsinfo ]
    opts_info_arr @ not if { }dict opts_info_arr ! then

    opts_info_arr @
    dup not if pop { }dict then
    id @ []
    dup not if pop { }dict then
;


: optionsinfo_getgroups[ int:id -- list:groups ]
    opts_groupsorder_arr @ not if { }dict opts_groupsorder_arr ! then

    opts_groupsorder_arr @ id @ []
    dup not if pop { }list then
;


: optionsinfo_getgrouplabel[ int:id str:group -- str:title ]
    opts_grouplabels_arr @ not if { }dict opts_grouplabels_arr ! then

    opts_grouplabels_arr @ id @ []
    dup not if pop "" then
    group @ []
    dup not if pop "" then
;


: optionsinfo_get_group_opts[ int:id str:group -- list:optnames ]
    opts_grouporder_arr @ not if { }dict opts_grouporder_arr ! then

    opts_grouporder_arr @ id @ []
    dup not if pop { }list then
    group @ []
    dup not if pop { }list then
;


: optionsinfo_set[ int:id arr:optionsinfo -- ]
    opts_info_arr @ not if { }dict opts_info_arr ! then
    opts_groupsorder_arr @ not if { }dict opts_groupsorder_arr ! then
    opts_grouplabels_arr @ not if { }dict opts_grouplabels_arr ! then
    opts_grouporder_arr @ not if { }dict opts_grouporder_arr ! then

    { }dict var! reindexed
    { }list var! groups
    { }dict var! grouporder
    { }dict var! grouplabels

    optionsinfo @ foreach var! iteminfo pop
        iteminfo @ "name"  [] var! optname
        iteminfo @ "group" [] var! group
        iteminfo @ "grouplabel" [] var! grouplabel

        groups @ group @ array_findval not if
            group @ groups @ array_appenditem groups !
        then
        grouplabel @ if
            grouplabel @ grouplabels @ group @ ->[] grouplabels !
        then

        grouporder @ group @ []
        dup not if pop { }list then
        optname @ swap array_appenditem
        grouporder @ group @ ->[] grouporder !

        iteminfo @ reindexed @ optname @ ->[] reindexed !
    repeat

    opts_info_arr @
    reindexed @ swap id @ ->[]
    opts_info_arr !

    groups @ opts_groupsorder_arr @ id @ ->[] opts_groupsorder_arr !
    grouporder @ opts_grouporder_arr @ id @ ->[] opts_grouporder_arr !
    grouplabels @ opts_grouplabels_arr @ id @ ->[] opts_grouplabels_arr !
;


: optionsinfo_new[ arr:optionsinfo -- int:id ]
    opts_info_topnum dup ++ @ var! id
    id @ optionsinfo @ optionsinfo_set
    id @
;


: optioninfo_get[ int:id str:optname -- arr:optinfo ]
    opts_info_arr @ not if { }dict opts_info_arr ! then
    opts_info_arr @ id @ []
    dup not if pop { }dict then
    optname @ []
    dup not if pop { }dict then
;


: optioninfo_set[ int:id str:optname arr:optinfo -- ]
    opts_info_arr @ not if { }dict opts_info_arr ! then
    opts_info_arr @ id @ [] var! optionsinfo
    optionsinfo @ not if { }dict optionsinfo ! then
    optinfo @ optionsinfo @ optname @ ->[] optionsinfo !
    optionsinfo @ opts_info_arr @ id @ ->[] opts_info_arr !
;


: optionsinfo_set_indexed[ int:id str:optname str:key str:val -- ]
    id @ optname @ optioninfo_get var! optinfo
    val @ optinfo @ key @ ->[] optinfo !
    id @ optname @ optinfo @ optioninfo_set
;


: optionsinfo_del[ int:id -- ]
    opts_info_arr @ not if { }dict opts_info_arr ! then
    opts_groupsorder_arr @ not if { }dict opts_groupsorder_arr ! then
    opts_grouporder_arr @ not if { }dict opts_grouporder_arr ! then

    opts_info_arr @ id @ array_delitem opts_info_arr !
    opts_groupsorder_arr @ id @ array_delitem opts_groupsorder_arr !
    opts_grouporder_arr @ id @ array_delitem opts_grouporder_arr !
;

public optionsinfo_get
public optionsinfo_getgroups
public optionsinfo_getgrouplabel
public optionsinfo_get_group_opts
public optionsinfo_set
public optionsinfo_new
public optioninfo_get
public optioninfo_set
public optionsinfo_set_indexed
public optionsinfo_del

$libdef optionsinfo_get
$libdef optionsinfo_getgroups
$libdef optionsinfo_getgrouplabel
$libdef optionsinfo_get_group_opts
$libdef optionsinfo_set
$libdef optionsinfo_new
$libdef optioninfo_get
$libdef optioninfo_set
$libdef optionsinfo_set_indexed
$libdef optionsinfo_del
.
c
q
@register lib-optionsinfo=lib/optionsinfo
@register #me lib-optionsinfo=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=3

