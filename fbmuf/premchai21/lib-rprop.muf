@prog lib-rprop.muf
1 9999 d
i
$ifdef __fuzzball__
  $pragma comment_recurse
$endif

( lib-rprop.muf
  Copyright (C) 2003, 2004 Drake Wilson
 
  This program is free software; you may redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundtaion; either version 2 of the license, or,
  at your option, any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
)
 
( This program requires a wizbit, unfortunately.  Otherwise its
  whole purpose would be for naught. )
 
( This library is for storing program-specific properties on objects
  under a wizpropped root depending on the program dbref.  The object
  owner can _clear_ the tree, but can't tamper with the properties any
  other way.  Clearing is performed by invoking the program directly
  from a wizard-owned exit, giving the object and program in question,
  separated by =.
 
  Usage:
 
  All the ordinary property operations with an 'r' inserted before the
  first 'prop', 'reflist', or 'bless' part of the name will access
  rprops rather than ordinary properties.  (Exception: blessrprop and
  unblessrprop are used rather than rblessprop and unrblessprop.)
  ~ and @ properties underneath the rprop root will still be dispermitted
  unless the caller is at least level 4.
 
  The caller must be at least level MIN_MLEV, default 2, to access
  rprops at all.  This is installer-customizable.
)
 
$include $lib/prem
 
  (The following macros are installer-customizable.)
$def MIN_MLEV 2
$def RPROP_FMT "~rprop/%d"    (No trailing slash.)
  (End customizable section.)
$def gname dup ok? if name else pop "<garbage>" then
 
$version 1.012
$lib-version 1.002
$author Drake Wilson
 
( This is the primary part of it --
  permissions checking and name transforms. )
( Buggo: this should use lib-guid now that it exists, to prevent
  recycled programs from having their rprops stolen by new programs
  with the same dbref.  That would make @rclear a bit trickier, though. )
 
: -rcdperms ( dict iabrt -- dict )
  var! iabrt
  var str
  caller "W" flag? if exit then
  { swap foreach swap str !
      str @ "/%s" fmtstring "*/[@~]*" smatch if
        iabrt @ if "Permission denied." abort else pop then
      else str @ swap then repeat }dict
;
 
: -rcperms ( s -- )    ( string must have leading / )
  var! str
  caller mlevel MIN_MLEV < if
    MIN_MLEV "Mucker level %i required." fmtstring abort then
  caller "W" flag? not if
    str @ "*/[~@]*" smatch if
      "Permission denied." abort then then
;
 
: -rprop ( s -- s )
  "/%s" fmtstring dup var! str -rcperms
  caller RPROP_FMT fmtstring str @ strcat
;
 
: -disrprop ( s -- s )
  var! str caller RPROP_FMT fmtstring var! pfx
  str @ dup pfx @ stringpfx if pop
    str @ pfx @ strlen ++ strcut swap pop then
  ( "^/+" "/" 0 regsub "/+$" "/" 0 regsub )
;
 
( And the definitions of all the individual functions. )
( Buggo: should nextrprop, array_get_rpropdirs, and array_get_rpropvals
  really require M3?  It seems like the primary purpose of the restriction
  is to keep M<=2 from scouring other people's objects for valuable data,
  but that can't happen here since it's all stuff the program would have
  set itself anyway... )
 
$def -mii  caller mlevel 2 < if "Mucker level 2 required." abort then
$def -miii caller mlevel 3 < if "Mucker level 3 required." abort then
$def -miv  caller "W" flag? not if "Mucker level 4 required." abort then
 
: addrprop 3 pick -rprop 3 put addprop ;
: array_filter_rprop swap -rprop swap array_filter_prop ;
: array_get_rpropdirs -miii -rprop array_get_propdirs 0 -rcdperms ;
: array_get_rproplist -rprop array_get_proplist ;
: array_get_rpropvals -miii -rprop array_get_propvals 0 -rcdperms ;
: array_get_rreflist -rprop array_get_reflist ;
: array_put_rproplist swap -rprop swap array_put_proplist ;
: array_put_rpropvals swap -rprop swap 1 -rcdperms array_put_propvals ;
: array_put_rreflist swap -rprop swap array_put_reflist ;
: rblessed? -mii -rprop blessed? ;
: blessrprop -miv -rprop blessprop ;
: envrprop -rprop envprop ;
: envrpropstr -rprop envpropstr ;
: getrprop -rprop getprop ;
: getrpropfval -rprop getpropfval ;
: getrpropstr -rprop getpropstr ;
: getrpropval -rprop getpropval ;
: nextrprop -miii -rprop nextprop -disrprop ;
: parserprop -miii 3 pick -rprop 3 put parseprop ;
: parserpropex -miii 3 pick -rprop 3 put parsepropex ;
: rpropdir? -mii -rprop propdir? ;
: rreflist_add swap -rprop swap reflist_add ;
: rreflist_del swap -rprop swap reflist_del ;
: rreflist_find swap -rprop swap reflist_find ;
: remove_rprop -rprop remove_prop ;
: setrprop swap -rprop swap setprop ;
: unblessrprop -miv -rprop unblessprop ;
 
( Blargh. )
 
public addrprop public array_filter_rprop public array_get_rpropdirs
public array_get_rproplist public array_get_rpropvals
public array_get_rreflist
public array_put_rproplist public array_put_rpropvals
public array_put_rreflist public rblessed? public blessrprop
public envrprop public envrpropstr public getrprop public getrpropfval
public getrpropstr public getrpropval public nextrprop public parserprop
public parserpropex public rpropdir? public rreflist_add
public rreflist_del public rreflist_find public remove_rprop
public setrprop public unblessrprop 
 
$libdef addrprop
$libdef array_filter_rprop
$libdef array_get_rpropdirs
$libdef array_get_rproplist
$libdef array_get_rpropvals
$libdef array_get_rreflist
$libdef array_put_rproplist
$libdef array_put_rpropvals
$libdef array_put_rreflist
$libdef rblessed?
$libdef blessrprop
$libdef envrprop
$libdef envrpropstr
$libdef getrprop
$libdef getrpropfval
$libdef getrpropstr
$libdef getrpropval
$libdef nextrprop
$libdef parserpropex
$libdef rpropdir?
$libdef rreflist_add
$libdef rreflist_del
$libdef rreflist_find
$libdef remove_rprop
$libdef setrprop
$libdef parserprop
$libdef unblessrprop
 
( And the clearing program. )
 
: clearrprop
  var! arg
  caller trig = caller exit? and not if
    "rprop clear must be called directly from its trigger." abort then
  trig owner "truewizard" flag? not if
    "rprop clear trigger must be owned by a wizard." abort then
 
  arg @ "=" split var! pgm var! obj
  pgm @ not obj @ not or if
    command @ "Usage: %s <object>=<program>" fmtstring .tell exit then
  obj @ M_CONTROLLED M_NOISY bitor prem-match dup obj ! ok? not if exit then
  pgm @ "^#[0-9]+$" 0 regexp pop if pgm @ stod pgm !
    else pgm @ M_NOISY prem-match dup pgm ! ok? not if exit then then
 
  pgm @ RPROP_FMT fmtstring var! dir
  obj @ dir @ propdir? not if
    obj @ dup gname pgm @ dup gname
      "There are no rprops of %s(%d) on %s(%d)." fmtstring .tell exit then
  obj @ dir @ remove_prop
  obj @ dup gname pgm @ dup gname
    "Removed rprops of %s(%d) from %s(%d)." fmtstring .tell
;
.
c
q
@set lib-rprop.muf=l
@set lib-rprop.muf=w
@set lib-rprop.muf=m3
@set lib-rprop.muf=v
@set lib-rprop.muf=_docs:@list $lib/rprop=22-40
@register lib-rprop.muf=lib/rprop
whisp me=You may wish to perform the following commands:
whisp me=@action @rclear=#0=tmp/act
whisp me=@link $tmp/act=$lib/rprop
