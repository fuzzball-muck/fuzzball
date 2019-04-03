( lib-ignore by Natasha@HLM
  Easily $includable, program specific #ignore function.
 
  cmd-ignore-add  { str strProgram -- }
  Parses str as a space-delimited list of names, adding the valid and not-
  already-#ignored ones to the active user's strProgram #ignore list. Emits
  appropriate messages for each name in the list.
 
  cmd-ignore-del  { str strProgram -- }
  Parses str as a space-delimited list of names, adding the valid and #ignored
  ones to the active user's strProgram #ignore list. Emits appropriate
  messages for each name in the list.
 
  ignores?  { db dbWhom strProgram -- bool }
  Returns true if db has dbWhom on her strProgram #ignore list.
 
  array_get_ignorers  { arrDb db strProgram -- arrDb' }
  Returns only the dbrefs of players who are strProgram #ignoring db.
 
  array_filter_ignorers  { arrDb db strProgram -- arrDb' }
  Returns only the dbrefs of players who are *not* strProgram #ignoring db.
 
  notify_exclude_ignorers  { dbRoom dbN..db1 intN str strProgram --  }
  Notifies str to everyone in dbRoom except those in the list and those who
  have strProgram #ignored me@.
 
  ignore-add  { db dbWhom strProgram -- }
  Adds dbWhom to db's strProgram #ignore list. If dbWhom is already #ignored,
  puts him at the end of the list {which doesn't really mean anything}.
  
  ignore-del  { db dbWhom strProgram -- }
  Removes dbWhom from db's strProgram #ignore list. If dbWhom is not #ignored,
  fails silently.
 
  ignore-list  { db strProgram -- dbN..db1 intN }
  Returns the list of who is in db's strProgram #ignore list.
 
  ignore-listify  { str -- str' }
  Turns the given program code into an ignore reflist property. You don't need
  to call this yourself if you're just using the above functions.
 
 
  __Version history
  1.1 19 October 2002: rtn-ignores? is an in-library call, and honors in-server
                       ignores.
  1.2 12 January 2003: notify_exclude_ignorers uses a real notify_exclude.
  1.3  3 January 2004: Added array_get_ignorers, fixed backwards description of
                       array_filter_ignorers.
 
 
  Copyright 2002-2003 Natasha O'Brien. Copyright 2002-2003 Here Lie Monsters.
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files {the "Software"}, to
  deal in the Software without restriction, including without limitation the
  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
)
 
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.003
$lib-version 1.003
$note Easily $includable, program specific #ignore function.
$doccmd @list $lib/ignore=1-59
 
$include $lib/reflist
$include $lib/match
$include $lib/ignore
 
: rtn-add reflist_add ;
: rtn-del reflist_del ;
 
: rtn-add-del[ str:names str:ignlist str:succmsg str:failmsg int:failifign adr:fn -- ]
    me @ ignlist @ ignore-listify  ( dbMe strProp )
    names @ " " explode_array foreach swap pop  ( dbMe strProp strName )
        .noisy_pmatch dup ok? if  ( dbMe strProp db )
            3 dupn reflist_find not not failifign @ = if  ( dbMe strProp db )
                failmsg @ fmtstring .tell  ( dbMe strProp )
            else  ( dbMe strProp db )
                3 dupn fn @ execute  ( dbMe strProp db )
                succmsg @ fmtstring .tell  ( dbMe strProp )
            then  ( dbMe strProp )
        else pop then  ( dbMe strProp )
    repeat pop pop  (  )
;
 
( strNames strProgram -- }  Central commands for #ignoring and #!ignoring people. )
: cmd-add  ( strNames strProgram )
    over if
        "Ignored %D." "You are already ignoring %D." 1 'rtn-add rtn-add-del
    else
        swap pop  ( strProgram )
        me @ swap ignore-listify REF-list "You are ignoring: %s" fmtstring .tell  (  )
    then
;
: cmd-del "Unignored %D." "You are already not ignoring %D." 0 'rtn-del rtn-add-del ;
 
: rtn-array-filter  ( arrDb db strProgram -- arrDb' }  Removes anyone strProgram #ignoring db from arrDb. )
    3 pick -4 rotate  ( arrDb arrDb db strProgram )
    ignore-listify swap "*{%d}*" fmtstring  ( arrDb arrDb strProp strSmatch )
    array_filter_prop  ( arrDb arrIgnoring )
    swap array_diff  ( arrDb' )
;
 
: rtn-notify-exclude  ( dbRoom dbN..db1 intN str strProgram -- }  Notify str to everyone in dbRoom except those listed and those strProgram #ignoring me@. )
    var! program var! mesg  ( dbRoom dbN..db1 intN )
    array_make over  ( dbRoom arrDb dbRoom )
 
    (dup mesg @ notify  { dbRoom arrDb dbRoom )
 
    contents_array  ( dbRoom arrDb arrContents )
    dup me @ program @ rtn-array-filter  ( dbRoom arrDb arrContents arrOkContents )
    swap array_diff  ( dbRoom arrDb arrIgnorers )
    array_union  ( dbRoom arrExcept )
    array_vals mesg @  ( dbRoom dbN..db1 intN str )
    notify_exclude  (  )
;
 
: rtn-ignores?  ( dbAlice dbBob strProgram -- bool }  Returns true if dbAlice is ignoring dbBob for the purposes of strProgram. )
    3 pick 3 pick ignoring? if  ( dbAlice dbBob strProgram )
        pop pop pop 1  ( bool )
    else
        ignore-listify swap reflist_find  ( bool )
    then  ( bool )
;
 
: main "Go away! Nyeh!" abort ;
 
PUBLIC cmd-add
PUBLIC cmd-del
PUBLIC rtn-array-filter
PUBLIC rtn-notify-exclude
PUBLIC rtn-ignores?

$pubdef array_filter_ignorers __PROG__ "rtn-array-filter" call
$pubdef array_get_ignorers ignore-listify swap "*{%d}*" fmtstring array_filter_prop
$pubdef cmd-ignore-add __PROG__ "cmd-add" call
$pubdef cmd-ignore-del __PROG__ "cmd-del" call
$pubdef ignore-add ignore-listify swap reflist_add
$pubdef ignore-del ignore-listify swap reflist_del
$pubdef ignore-list ignore-listify array_get_reflist array_vals
$pubdef ignore-listify "_prefs/%s/ignore" fmtstring
$pubdef ignores? __PROG__ "rtn-ignores?" call
$pubdef notify_except_ignorers 1 -3 rotate notify_exclude_ignorers
$pubdef notify_exclude_ignorers __PROG__ "rtn-notify-exclude" call
