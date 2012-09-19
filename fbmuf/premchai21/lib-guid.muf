@program lib-guid.muf
1 9999 d
i
$ifdef __fuzzball__
  $pragma comment_recurse
$endif

( lib-guid.muf
  Copyright (C) 2004 Drake Wilson

  This program is free software; you may redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundtaion; either version 2 of the license, or,
  at your option, any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
)

( This attempts to resolve the problem of programs not having a good
  way to distinguish between two objects that have the same dbref but
  are referenced at different times and have different identities.

  An object that gets recycled will not have the same GUID as the object
  that gets created afterwards with the same dbref, and two objects with
  different dbrefs will not have the same GUID.
 
  Interface:
    objguid ( d -- s )
      Obtain the GUID of an existing object.
    guidobj ( s -- d )
      Obtain the object associated with a given GUID, or #-1.
)

$author Drake Wilson
$version 1.004
$lib-version 1.001

( Buggo?  This takes linear time and stack space. )
 
: str++ ( s -- s )
  dup not if pop " " exit then
  dup strlen -- strcut ctoi ++
  dup 127 = if pop 32 swap str++ swap then
  itoc strcat
;
 
: next-guid ( -- s )
  prog "~last_guid" getpropstr
  str++
  prog "~last_guid" 3 pick setprop
;
 
: objguid ( d -- s )
  dup ok? not if "Invalid object." abort then
  dup var! obj "~guid" getpropstr var! str
  str @ if str @ exit then
  next-guid obj @ int "%i!%s" fmtstring
  obj @ "~guid" 3 pick setprop
;
public objguid $libdef objguid
 
: guidobj ( s -- d )
  dup string? not if "Not a string." abort then
  dup var! str atoi dbref var! obj
  obj @ ok? not if #-1 exit then
  obj @ "~guid" getpropstr str @ strcmp if #-1 exit then
  obj @
;
public guidobj $libdef guidobj
.
c
q
@set lib-guid.muf=w
@set lib-guid.muf=b
@set lib-guid.muf=v
@set lib-guid.muf=l
@set lib-guid.muf=_docs:@list $lib/guid=19-32
@register lib-guid.muf=lib/guid
