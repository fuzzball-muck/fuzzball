@prog gen-desc-docs
1 99999 d
1 i
Quick Usage Summary of $desc (gen-desc v2.0)
August 2, 1992 by Conrad Wong
 
How to use:
 
    $desc is a program that you can use to make more complex messages on
descriptions, success, fail, and drop messages.  To do so, do this:
 
    @desc object = @$desc (description here)
  (or @succ, @fail, @drop)
 
    The description is a normal text message, but may include special
'tokens' which are translated by the $desc program to normal text.  For
example, you might write this:
 
    @desc me = @$desc A cute little fox with %sub[color] eyes.
    @set me = color:green
 
    Then when someone looked at you, they would see:
 
    A cute little fox with green eyes.
 
    All tokens are of the form %token-name[parameters] and you may nest
tokens, so that you may have descriptions which look like this:
 
    @desc me = @$desc %sub[%sub[morph]  %sub[clothes]
 
    Which would take the property name that you had stored in the property
'morph' and substitute the contents of that property.  It would also take
whatever you had in 'clothes' and add that into the description.
 
    You should also bear in mind that lists, when mentioned below, are
created by doing this:
 
    lsedit object = listname
    (first line of the list)
    (second line of the list)
        ....
    (last line of the list)
    .end
 
    The lsedit program is very useful for editing lists.
 
Tokens:
 
%sub[<match,>property]
Example: %sub[color]
         %sub[me,species]
         %sub[here,night]
         %sub[#321,small]
 
    %sub[] is used to take the contents of a property.  It normally takes
the property from the object that is looked at (or which has the @succ or
@fail message) but can be set to use properties from other objects as shown
above.
 
%rand[<match,>listname]
Example: %rand[buttons]
         %rand[here,songs]
 
    %rand[] takes one of the lines in a list at random.  As with %sub[], it
normally takes this from the object that is looked at, but can take
properties from other objects.  %rand[] differs in that it expects the name
given to be the name of a list created with lsedit, not a property name.
 
%time[<match,>listname]
  Table: one item in list -- full day
         two items in list -- midnight to noon, noon to midnight
         three items -- midnight to 8 AM, 8 AM to 4 PM, 4 PM to midnight
         four items -- midnight to 6 AM, 6 AM to noon, noon to 6 PM,
                       6 PM to midnight
         six items -- midnight to 4 AM, 4 AM to 8 AM, 8 AM to noon,
                      noon to 4 PM, 4 PM to 8 PM, 8 PM to midnight
%date[<match,>listname]
  Table: one item in list -- full year
         two items in list -- January to July, August to December
         three items in list -- Jan. to Apr., May to Aug., Sept. to Dec.
         four items in list -- winter, spring, summer, fall
         twelve items in list -- monthly
 
    %time[] selects one of the lines in a list, based on the time of the
day.  That is, the first line corresponds to just after midnight, the last
line is before midnight, and all the lines in between are mapped over the
day.  %date[] is similar, but it does this over the year so that you can
have descriptions which vary with the season.
 
%concat[<match,>listname]
 
    Rather than taking one item, %concat[] takes each of the items in the
list and strings them together into your description.  However no spaces are
inserted between elements so you must include that, or commas, if you want
something to be put in between list elements.
 
%select[<match,>listname,value]
Example: %select[here,weather,%sub[me,climate]]
 
    This selects one item in a list, given the value.  This can be useful in
such cases as where you have a number on some object and want to give a
description keyed to it.  Also, if there is no entry in the list that
corresponds, it will take the closest entry less than or equal to that
number given.  (for instance, you could set only some entries in a list)
    %select[] was written to allow people to write descriptions that would
vary with the size of the people looking.  This is generally defined as an
implementation-specific macro such as %+size[listname]
 
%if[condition,true<,false>]
Example: %if[%sub[me,fly],You fly up into the air,You can't fly!]
         %if[%sub[light],In the flickering glow you see a trapdoor.]
 
    If the condition given (i.e. a substituted property) is not a null
string (blank), or FALSE, then it will return the "true" message.
Otherwise, it will return either a blank or a "false" message if one is
provided.
    Note that you may combine substitution of properties with %yes[] and
%no[] which are described below.
 
%yes[parameter]
%no[parameter]
Example: %yes[%sub[me,fly]]
 
    If the parameter given is either 'Yes' or 'No' for %yes[] and %no[]
respectively, then it will return YES.  Otherwise, it will return a blank
string.  These are non-case-sensitive so you may use 'yes', 'Yes', or 'YES',
or even 'y'.
    These may be useful with %if[] where you may require that a property be
set specifically to 'yes' or 'no'.
 
%@progname[<arguments>]
%#prognum[<arguments>]
Example: %@$foo[bar]
         %#321[baz]
 
    These allow you to call programs, which must take one string parameter
(whatever is given as <arguments>) and will return one string.  This will
then be inserted into your description like any other token would.
 
%+<macroname>[<arguments>]
Example: %+size[listname]
 
    This looks for a macro on the object, the current room, or environments
thereof up to #0.  A macro is set as a property named _macros/macroname so
for instance, you might do this:
 
    @set #0 = _macros/size:%select[%0,%sub[me,size]]
 
    In a macro definition (which is simply inserted like another token) %0,
%1, %2, and so on may be used for arguments, which are separated with
commas.  So %0 here refers to the first argument given, and all other
arguments would be ignored.
 
%prog#num[arg1,arg2,arg3,...]
%prog$name[arg1,arg2,arg3,...]
Example: %prog#321[foo,bar]
         %prog$baz[gum]
 
    These are like %$prog[] and %#prog[] but they split the arguments so you
can call programs that need several different arguments.
 
%char[num]
Example: \\[
         \\,
         %%token[parameter]
 
    Escaped characters such as \c (i.e. any character prefaced with \) and
%% (which is translated to %) will be translated into %char[num] which
should not be used directly.  This allows you to substitute special
characters which would otherwise be interpreted by $desc.
 
%otell[<room,>message]
Example: %otell[stares out the window at the passing fishes.]
 
    This sends a message to everyone else which can be useful if you want to
simulate a randomly changing osucc or ofail.  You would leave the @osucc
blank and instead, use $desc to create the @osucc message.  The message will
be preceded by the name of the person looking.  You may use this to display
a message in some other room as long as the owner of the object also owns
that room.
    Note: you cannot use %otell[] on a message that is on a player, so you
cannot make %otell[] to simulate @odescs on players.  Please do not abuse
%otell[].
 
%prand[listname<,mins>]
Example: %prand[fishtype]
 
    This chooses one of the elements at random, based on both the trigger's
dbref (object number) and the current time, within a period.  So you may
have it cycle through all the objects in the number of minutes you want, and
it will be like a random message that will not change if you look at it
immediately.
    %prand[] defaults to a cycle time of 1 minute through each element in
the list.
 
%list[listname]
Example: %list[desc]
 
    This prints out the first part of the description before the %list[],
then prints each element of the list on its own line, then resumes printing
the rest of the description.  This can be useful when you want to print
pictures or lists of items.
    Note that %list[] cannot be used as an argument so something like
%sub[%list[]] will simply look for a property named "%list[]"
 
%call[prognum<,arguments>]
Example: %call[#321,foobarbaz]
 
    This prints out the first part of the description before the %call[],
then calls the program, which may do whatever it wishes including printing
out messages and taking input from the user.  (programs that need to take
input cannot be called from descriptions) It then resumes with the rest of
the description.
    Like %list[], you may not use %call[] as an argument to another token.
 
%nl[]
 
    This simply prints whatever comes before this and starts the rest of the
description on a new line.
    Like %list[], you may not use %nl[] as an argument to another token.
 
Proplocs and Property Searches:
 
    Normally a property is looked for on the trigger object, i.e. where the
@description, @succ, or @fail is set.  With many commands you can specify
another object such as the looker or the current room.
 
    The program can normally only read properties on the looker, or anything
that the owner of the trigger object owns.
 
    The program will automatically search environments for properties where
appropiate.  So you may set a list on a parent room and each of the rooms
inside will be able to use that list.
 
    You may set a _proploc:(object number without the #) property on objects
and the $desc program will look on the 'property location' given for the
property asked for _after_ it checks the environment.  You may even create
chains of _proploc properties so that several proplocs can inherit from
one.
 
    However you can only use a proploc owned by someone else if it is set
_proploc_ok?:yes
.
q
@register gen-desc-docs=desc-docs
@prog gen-desc-2
1 99999 d
1 i
(
gen-desc v2.0 written on May 11, 1992
 
All tokens matched in form %token[parameter,parameter,...]
Escaped characters i.e. \[ and %% converted to %char[num]
 
Valid tokens evaluated on first pass:
        %sub[<match,>property]
        %rand[<match,>listname]
        %time[<match,>listname]
        %date[<match,>listname]
        %concat[<match,>listname]
        %select[<match,>listname,value]
        %if[condition,true<,false>]
        %yes[<match>,property]
        %no[<match>,property]
        %me[]
        %here[]
        %@progname[<arguments>]
        %#prognum[<arguments>]
        %+macroname[<arguments>]  { set a property _macros/macroname
                                    with the definition, and use %0, %1,
                                    etc. to reach the arguments }
        %prog#num[arg1,arg2,arg3,...]
        %prog$name[arg1,arg2,arg3,...]
        %char[num]
        %otell[<room,>message]     { does not work on players }
        %prand[listname<,mins>] -- based on dbref of trigger and time to
                                   minutes, with optional number of minutes
                                   per entry to use
 
Valid tokens evaluated after first pass:
        %list[listname]
        %call[prognum<,arguments>]
        %nl[]
 
To Do:
        %timed[cycletime,offset,listname]
        pagination?
)
 
$include $lib/lmgr
$include $lib/strings
$include $lib/props
$include $lib/edit
lvar lbracket   ( count of left brackets found )
lvar search     ( current place to search )
 
( get-legal-prop -- if it is OK to get the property from that dbref, then
                    do so; otherwise, return error instead )
: get-legal-prop ( d s -- s' )
        over owner me @ owner dbcmp
        3 pick owner trigger @ owner dbcmp
        4 pick "_proploc_ok?" getpropstr .yes?
        or or if
                getpropstr
        else
                "** CAN'T READ " swap strcat
                " ON #" strcat intostr swap strcat
                " **" strcat
                .tell
        then
;
public get-legal-prop
 
( count-char -- counts the number of the specified character in the string
                and returns the number of occurences)
: count-char ( s c -- i )
        0 rot rot                                       ( 0 s c )
        begin over over instr dup while                 ( i s c pos )
                rot swap strcut swap pop                ( i c sr )
                rot 1 +                                 ( c sr i+1 )
                swap rot                                ( i+1 sr c )
        repeat
        pop pop pop                                     ( i )
;
 
( count-brackets -- updates the number of unmatched left brackets found )
: count-brackets ( s -- )
        dup "[" count-char swap "]" count-char          ( il ir )
        - lbracket @ +
        lbracket !
;
 
( cut-next-delimiter -- finds the first occurence of the delimiter that has
                        no unmatched brackets on its left, and splits the
                        string around that delimiter)
: cut-next-delimiter ( s c -- sl sr )
        "" rot rot                                      ( "" s c )
        0 lbracket !
        begin
                over over instr dup if                  ( sl sr c pos )
                        rot swap 1 - strcut 1 strcut swap pop
                                                        ( sl c sm sr )
                        over count-brackets
                        lbracket @ not if
                            rot pop                     ( sl sm sr )
                            rot rot strcat swap         ( sl+sm sr )
                            exit
                        else
                            swap 3 pick strcat          ( sl c sr sm+c )
                            4 rotate swap strcat        ( c sr sl+sm+c )
                            swap rot                    ( sl+sm+c sr c )
                            dup "]" strcmp not if
                                lbracket @ 1 - lbracket !
                            then
                            0                           ( sl+sm+c sr c 0 )
                        then
                else
                        pop pop strcat ""               ( sl+sr "" )
                        exit
                then
        until
        pop
;
 
( get-next-token -- given a string with a token, tries to find a token
                    matching the standard %token[args] form.  If successful,
                    then the token string returned will be "%token[".  If
                    no "]" exists, it will be assumed at the far right and
                    sr will be "" )
: get-next-token ( s -- sl sr args token 1 or s 0 )
        dup "%" instr dup if                            ( s pos )
                1 - strcut dup "[" instr dup if         ( sl sr pos )
                        strcut "]" cut-next-delimiter   ( sl token args sr )
                        swap rot 1                      ( sl sr args token 1 )
                else
                        pop strcat 0
                then
        else
                pop 0
        then
;
 
( split-args -- given a string of arguments separated by commas, splits the
                arguments in such a way that each argument will contain a
                matched set of brackets. )
: split-args ( args -- a[1] a[2] ... a[n] n )
        1
        begin swap "," cut-next-delimiter dup while     ( ... n a[n] a[n+1] )
                rot swap                                ( ... a[n] n a[n+1] )
                dup if
                        swap 1 +                        ( ... a[n] a[n+1] n+1 )
                else
                        swap
                then
        repeat
        pop swap
;
public split-args
 
( format-print -- given a string, breaks it into a number of lines formatted
                  to fit the given screen width )
: format-print ( s -- )
        dup not if
                pop exit
        then
        trigger @ swap pronoun_sub
        "_screen_width" dup me @ swap .locate-prop dup ok? if
                getpropstr atoi
        else
                pop pop 78
        then
        swap                                            ( width str )
        1 rot 0 swap 1 1 EDITfmt_rng                    ( s1 s2 ... sn n )
        EDITdisplay
;
public format-print
 
( wipe-list -- deletes a list of strings on the top of the stack )
: wipe-list ( s[1] ... s[n] n -- )
        begin dup while
                swap pop 1 -
        repeat
        pop
;
public wipe-list
 
( select-item -- given a string, a list of matching strings, and functions,
                 pick out the function that goes with the string, if any )
: select-item ( s a[1] f[1] a[2] f[2] ... a[n] f[n] n -- s f[n] )
        dup 2 * 2 + rotate                              ( ... n s )
        begin over while
                swap 1 - swap                           ( ... n-1 s )
                4 rotate 4 rotate                       ( ... n s a[n] f[n] )
                rot rot over swap smatch if             ( ... n f[n] s )
                        begin 3 pick while
                                4 rotate pop 4 rotate pop
                                3 rotate 1 - -3 rotate
                        repeat                          ( 0 f[n] s )
                        pop swap pop exit               ( f[n] )
                else
                        swap pop                        ( ... n s )
                then
        repeat
        pop pop 0
;
 
lvar debug
: eval-loop ( s -- s' )
        dup not if
            exit
        then
        "" swap                                         ( "" s )
        begin get-next-token while                      ( sl sm sr args token )
                5 rotate 5 rotate strcat -4 rotate      ( sl' sr args token )
                me @ "_debug" getpropstr 1 strcut pop "y" stringcmp not if
                        me @ trigger @ owner dbcmp
                        me @ "W" flag? or if
                                over over swap strcat "]" strcat
                                debug !
                        then
                then
                dup
                "%sub\\[" "do-sub"
                "%rand\\[" "do-rand"
                "%time\\[" "do-time"
                "%date\\[" "do-date"
                "%concat\\[" "do-concat"
                "%select\\[" "do-select"
                "%if\\[" "do-if"
                "%yes\\[" "do-yes"
                "%no\\[" "do-no"
                "%me\\[" "do-me"
                "%here\\[" "do-here"
                "%@*\\[" "do-program"
                "%prog#*\\[" "do-progmultargs"
                "%prog$*\\[" "do-progmultargs"
                "%\\+*\\[" "do-macro"
                "%char\\[" "do-char"
                "%otell\\[" "do-otell"
                "%prand\\[" "do-prand"
                18 select-item                          ( sl' sr args token f )
                dup if
                        "$desc-lib" match swap call     ( sl' sr result )
                        eval-loop
                        me @ "_debug" getpropstr 1 strcut pop "y" stringcmp not if
                                me @ trigger @ owner dbcmp
                                me @ "W" flag? or if
                                        debug @ " = " strcat over strcat
                                        .tell
                                then
                        then
                else
                        pop swap strcat "]" strcat      ( sl' sr token+args )
                        1 strcut rot                    ( sl' "%" rest sr )
                        strcat swap                     ( sl' sr' "%" )
                then
                rot swap strcat swap                    ( sl'+result sr )
        repeat
        strcat                                          ( s' )
;
public eval-loop
 
: second-pass ( s -- s' )
        dup not if
            exit
        then
        "" swap                                         ( "" s )
        begin get-next-token while                      ( sl sm sr args token )
                5 rotate 5 rotate swap strcat -4 rotate ( sl' sr args token )
                dup
                "%list\\[" "do-list"
                "%call\\[" "do-call"
                "%nl\\[" "do-newline"
                3 select-item                           ( sl' sr args token f )
                dup if
                        "$desc-lib" match swap call     ( sl' sr result )
                        eval-loop
                else
                        pop swap strcat "]" strcat      ( sl' sr token+args )
                        1 strcut rot                    ( sl' "%" rest sr )
                        strcat swap                     ( sl' sr' "%" )
                then
                rot swap strcat swap                    ( sl'+result sr )
        repeat
        strcat                                          ( s' )
;
( escape-chars -- convert all occurences of %% to %char[num] and occurences
                  of \char to %char[num] so that they can be handled without
                  errors or mixing with normal parameters. )
: escape-chars ( s -- s' )
        "%char[" "%" .asc intostr strcat "]" strcat      ( s "%char[num]" )
        "%%" subst                                      ( s1 )
        begin dup "\\" instr dup while                  ( s1 pos )
              1 - strcut 1 strcut swap pop 1 strcut     ( sl c sr )
              swap .asc intostr                          ( sl sr num )
              "%char[" swap strcat "]" strcat           ( sl sr "%char[num]" )
              swap strcat strcat                        ( s' )
        repeat
        pop
;
 
: gen-desc ( s -- )
        escape-chars
        eval-loop
        second-pass
        dup if
                format-print
        else
                pop
        then
;
public gen-desc
.
c
q
@register gen-desc-2=desc
@set $desc=W
@set $desc=L
@set $desc=/_/de:@$desc %list[doc]
@set $desc=/_defs/.eval-loop:"$desc" match "eval-loop" call
@set $desc=/_defs/.format-print:"$desc" match "format-print" call
@set $desc=/_defs/.gen-desc:"$desc" match "gen-desc" call
@set $desc=/_defs/.get-legal-prop:"$desc" match "get-legal-prop" call
@set $desc=/_defs/.split-args:"$desc" match "split-args" call
@set $desc=/_defs/.wipe-list:"$desc" match "wipe-list" call
@set $desc=/_docs:@list $desc-docs
@prog gen-desc-lib
1 99999 d
1 i
(
gen-desc library v2.0 written on May 11, 1992
 
Contains all the tokens and macro subroutines which are implemented in $desc.
)
$include $lib/lmgr
$include $lib/props
$include $lib/strings
$include $desc
 
: do-sub1 ( name obj -- value )
        dup ok? if
                swap .get-legal-prop             ( ... value )
        else
                pop pop ""
        then
;
 
: do-sub ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args                                 ( ... args n )
        dup 1 = if
                pop .eval-loop                          ( ... name )
                dup trigger @ swap .locate-prop         ( ... name obj )
                do-sub1
        else
            dup 2 = if
                pop .eval-loop swap .eval-loop          ( ... name source )
                match over .locate-prop                 ( ... name obj )
                do-sub1
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %SUB **"
            then
        then
;
public do-sub
 
: do-rand1 ( list obj -- result )
        dup ok? if
               over over .lmgr-getcount        ( list obj num )
               random 1003 / swap % 1 + -3 rotate  ( pos list obj )
               .lmgr-getelem                   ( result )
        else
               pop pop ""
        then
;
 
: do-rand ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
                pop .eval-loop                             ( list )
                dup "#" strcat trigger @ swap .locate-prop ( list obj )
                do-rand1
        else
            2 = if
                .eval-loop swap .eval-loop                 ( list name )
                match swap dup "#" strcat               ( obj list list# )
                rot swap .locate-prop                   ( list obj' )
                do-rand1
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %RAND **"
            then
        then
;
public do-rand
 
: do-time1
        dup ok? if
                over over .lmgr-getcount        ( list obj cnt )
                systime 14400 - 86400 % * 86400 / ( list obj pos )
                -3 rotate .lmgr-getelem         ( result )
        else
                pop pop ""
        then
;
 
: do-time ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
                pop .eval-loop                             ( list )
                dup "#" strcat trigger @ swap .locate-prop ( list obj )
                do-time1
        else
            2 = if
                .eval-loop swap .eval-loop                 ( list name )
                match swap dup "#" strcat               ( obj list list# )
                rot swap .locate-prop                   ( list obj' )
                do-time1
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %TIME **"
            then
        then
;
public do-time
 
: do-date1
        dup ok? if
                over over .lmgr-getcount        ( list obj cnt )
                systime 14400 - 86400 / 365 % *
                365 / 1 +                       ( list obj pos )
                -3 rotate .lmgr-getelem         ( result )
        else
                pop pop ""
        then
;
 
: do-date ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
                pop .eval-loop                             ( list )
                dup "#" strcat trigger @ swap .locate-prop ( list obj )
                do-date1
        else
            2 = if
                .eval-loop swap .eval-loop                 ( list name )
                match swap dup "#" strcat               ( obj list list# )
                rot swap .locate-prop                   ( list obj' )
                do-date1
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %DATE **"
            then
        then
;
public do-date
 
: do-concat1 ( list obj -- result )
        dup ok? if
                .lmgr-fullrange .lmgr-getrange          ( s1 ... sn n )
                begin dup 1 > while
                    rot rot strcat swap 1 -             ( s1 ... sn-1 n-1 )
                repeat
                pop
        else
                pop pop ""
        then
;
 
: do-concat ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
                pop .eval-loop                             ( list )
                dup "#" strcat trigger @ swap .locate-prop ( list obj )
                do-concat1
        else
            2 = if
                .eval-loop swap .eval-loop                 ( list name )
                match swap dup "#" strcat               ( obj list list# )
                rot swap .locate-prop                   ( list obj' )
                do-concat1
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %CONCAT **"
            then
        then
;
public do-concat
 
: do-select1 ( pos list obj -- result )
        dup ok? if
                rot atoi -3 rotate               ( pos list obj )
                0 -4 rotate "" -5 rotate          ( lst idx pos list obj )
                begin 4 pick 4 pick < 
                      6 pick not if
                          5 pick 5 pick 100 + <
                      else
                          0
                      then or while
                        4 rotate 1 + -4 rotate   ( lst idx+1 pos list obj )
                        4 pick 3 pick 3 pick .lmgr-getelem
                        dup if                   ( lst idx+1 pos list obj x )
                                6 rotate pop     ( idx+1 pos list obj x )
                                -5 rotate        ( x idx+1 pos list obj )
                        else
                                pop
                        then
                repeat
                pop pop pop pop
        else
                pop pop ""
        then
;
 
: do-select ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
            pop pop "** TOO FEW ARGUMENTS FOR %SELECT **"
        else
            dup 2 = if
                pop .eval-loop swap .eval-loop             ( pos list )
                dup "#" strcat trigger @ swap .locate-prop ( pos list obj )
                do-select1
            else
                3 = if
                    .eval-loop swap .eval-loop rot .eval-loop  ( pos list name )
                    match swap dup "#" strcat            ( pos obj list list# )
                    rot swap .locate-prop                ( pos list obj' )
                    do-select1
                else
                    .wipe-list
                    "** TOO MANY ARGUMENTS FOR %SELECT **"
                then
            then
        then
;
public do-select
 
: do-if ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
            pop pop "** TOO FEW ARGUMENTS FOR %IF **"
        else
            dup 2 = if
                pop swap .eval-loop                     ( true cond )
                .no? if                                 ( true )
                    pop ""
                then
                .eval-loop
            else
                dup 3 = if
                    pop rot .eval-loop                  ( true false cond )
                    .no? if                             ( true false )
                        swap pop
                    else
                        pop
                    then
                    .eval-loop
                else
                    .wipe-list
                    "** TOO MANY ARGUMENTS FOR %IF **"
                then
            then
        then
;
public do-if
 
: do-yes ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        1 = if
              .eval-loop .yes? if
                  "YES"
              else
                  ""
              then
        else
              .wipe-list
              "** TOO MANY ARGUMENT FOR %YES**"
        then  
;
public do-yes
 
: do-no ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        1 = if
              .eval-loop .no? if
                  "YES"
              else
                  ""
              then
        else
              .wipe-list
              "** TOO MANY ARGUMENT FOR %NO **"
        then  
;
public do-no
 
: do-me ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        1 = if
              pop me @ intostr "#" swap strcat
        else
              .wipe-list
              "** TOO MANY ARGUMENT FOR %ME **"
        then  
;
public do-me
 
: do-here ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        1 = if
              pop loc @ intostr "#" swap strcat
        else
              .wipe-list
              "** TOO MANY ARGUMENT FOR %HERE **"
        then  
;
public do-here
 
lvar mme
lvar mloc
lvar mtrig
lvar mdepth
: do-program ( s[left] s[right] args token -- s[left] s[right] result )
    me @ mme !
    loc @ mloc !
    trigger @ mtrig !
    depth mdepth !
    swap .eval-loop swap                                   ( ... args token )
    dup strlen 1 - strcut pop 2 strcut swap pop 1 strcut over "$" strcmp if
        strcat "#" swap
    then
    strcat match
    dup ok? not if
        pop pop "** NO SUCH OBJECT IN %PROG **" exit
    then
    dup program? not if
        pop pop "** OBJECT NOT A PROGRAM IN %PROG **" exit
    then
    call
    mme @ me !
    mloc @ loc !
    mtrig @ trigger !
    depth mdepth @ - 2 +                               ( ... results i )
    dup 1 = if
        pop
        dup int? if
            intostr
        then
        .eval-loop
    else
        dup 0 = if
            pop ""
        else
            dup 0 < if
                pop "" "" "** STACK UNDERFLOW IN %PROG **"
            else
                .wipe-list
                "** TOO MANY ARGUMENTS RETURNED IN %PROG **"
            then
        then
    then
;
public do-program
 
: do-progmultargs ( s[left] s[right] args token -- s[left] s[right] result )
    me @ mme !
    loc @ mloc !
    trigger @ mtrig !
    depth mdepth !
    5 strcut dup strlen 1 - strcut pop swap pop match
    dup ok? not if
        pop pop "** NO SUCH OBJECT IN %PROG **" exit
    then
    dup program? not if
        pop pop "** OBJECT NOT A PROGRAM IN %PROG **" exit
    then                                               ( ... args dbref )
    swap .split-args dup                               ( ... dbref a1-an n n )
    begin dup while
        dup 2 + rotate .eval-loop over 2 + 0 swap - rotate
        1 -
    repeat
    pop 1 + rotate                                     ( ... a1'-an' n dbref)
    call
    mme @ me !
    mloc @ loc !
    mtrig @ trigger !
    depth mdepth @ - 2 +                               ( ... results i )
    dup 1 = if
        pop
        dup int? if
            intostr
        then
        .eval-loop
    else
        dup 0 = if
            pop ""
        else
            dup 0 < if
                pop "" "" "** STACK UNDERFLOW IN %PROG **"
            else
                .wipe-list
                "** TOO MANY ARGUMENTS RETURNED IN %PROG **"
            then
        then
    then
;
public do-progmultargs
 
: do-char ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        1 = if
            .eval-loop atoi .chr
        else
            .wipe-list
            "** TOO MANY ARGUMENTS FOR %CHAR **"
        then
;
public do-char
 
( do-list -- prints out whatever was before the list, then prints each item
             of the list on its own line, then resumes with the rest )
: do-list1 ( s[left] s[right] list obj -- "" s[right] "" )
        dup ok? not if
            pop pop "" exit
        then
        4 rotate .format-print "" -4 rotate              ( "" sr list obj )
        .lmgr-fullrange .lmgr-getrange                  ( "" sr s1 ... sn n )
        begin dup while
                dup 1 + rotate                          ( "" sr s2...sn n s1 )
                .gen-desc                               ( "" sr s2...sn n )
                1 -                                     ( "" sr s2...sn n-1 )
        repeat
        pop ""
;
 
: do-list ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
                pop .eval-loop                             ( list )
                dup "#" strcat trigger @ swap .locate-prop ( list obj )
                do-list1
        else
            2 = if
                .eval-loop swap .eval-loop                 ( list name )
                match swap dup "#" strcat               ( obj list list# )
                rot swap .locate-prop                   ( list obj' )
                do-list1
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %LIST **"
            then
        then
;
public do-list
 
( do-call -- prints out whatever was before the list, then calls the
             program given, with whatever arguments were used.  Only one
             argument is permitted.  The rest of the description is then
             printed after the program's output. )
: do-call1 ( s[left] s[right] prognum args -- "" s[right] "" )
        over ok? not if
            pop pop "** NO SUCH OBJECT IN %CALL **" exit
        then
        over program? not if
            pop pop "** OBJECT NOT A PROGRAM IN %CALL **" exit
        then
        4 rotate .format-print "" -4 rotate              ( "" sr prognum args )
        me @ mme !  loc @ mloc !  trigger @ mtrig !  depth mdepth !
        swap call
        mme @ me !  mloc @ loc !  mtrig @ trigger !
        depth mdepth @ - 2 +                            ( ... results i )
        dup 0 = if
            pop ""
        else
            dup 0 < if
                pop "" "" "** STACK UNDERFLOW IN %CALL **"
            else
                .wipe-list (we throw any returned results on the floor)
            then
        then
;
 
: do-call ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
                pop .eval-loop match ""                 ( prognum "" )
                do-call1
        else
            dup 1 > if
                begin dup 2 > while                     ( ...args a1 a2 n )
                    rot rot "," swap strcat strcat      ( ...args n a1,a2 )
                    swap 1 -                            ( ... args a1,a2 n-1 )
                repeat                                  ( name arg 2 )
                pop swap .eval-loop match swap .eval-loop  ( prognum arg )
                do-call1
            else
                pop "** MUST SUPPLY PROGRAM NUMBER FOR %CALL **"
            then
        then
;
public do-call
 
: do-macro ( s[left] s[right] args token -- s[left] s[right] result )
    2 strcut dup strlen 1 - strcut pop
         swap pop "_macros/" swap strcat                ( ... args name )
    dup trigger @ swap .locate-prop                     ( ... args name' obj )
    dup ok? not if
        pop swap pop 8 strcut swap pop "** " swap strcat
        " NOT FOUND IN %MACRO **" strcat exit           ( ... error )
    then
    swap getpropstr                                     ( ... args def )
    swap .split-args dup 2 + rotate                     ( ... a1-an n def )
    begin over while
        over intostr "%" swap strcat                    ( ... a1-an n def %n )
        rot 1 - -3 rotate                               ( ... a1-an n-1 def %n )
        4 rotate swap subst                             ( ... a1-an-1 n-1 def' )
    repeat
    swap pop .eval-loop                                 ( ... def' )
;
public do-macro
 
: do-otell ( s[left] s[right] args token -- s[left] s[right] result )
        trigger @ player? if
            pop pop "** CANNOT USE %OTELL ON PLAYERS **" exit
        then
        pop .split-args
        dup 1 = if
            pop .eval-loop me @ name " " strcat swap strcat .otell ""
        else
            2 = if
                .eval-loop swap .eval-loop match      ( ... msg db )
                dup ok? not if
                    pop pop "** TRIED TO %OTELL TO INVALID ROOM **" exit
                then
                dup room? not if
                    pop pop "** TRIED TO %OTELL TO INVALID ROOM **" exit
                then
                dup owner trigger @ owner dbcmp not if
                    pop pop "** TRIED TO %OTELL TO FOREIGN ROOM **" exit
                then
                swap #-1 swap
                me @ name " " strcat swap strcat notify_except ""
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %OTELL **"
            then
        then
;
public do-otell
 
: do-newline ( s[left] s[right] args token -- s[left] s[right] result )
        pop pop                          ( s[left] s[right] )
        swap dup not if pop " " then     ( s[right] s[left] )
        .format-print                    ( s[right] )
        "" swap                          ( "" s[right] )
        ""                               ( "" s[right] "" )
;
public do-newline
 
: do-prand1 ( dividend list obj -- result )
        dup ok? if
               over over .lmgr-getcount        ( dividend list obj num )
               trigger @ int 10003 * 29 / systime 60 / + 5 rotate /
                                               ( list obj num rand )
               swap % 1 + -3 rotate            ( pos list obj )
               .lmgr-getelem                   ( result )
        else
               pop pop ""
        then
;
 
: do-prand ( s[left] s[right] args token -- s[left] s[right] result )
        pop .split-args
        dup 1 = if
                pop .eval-loop                             ( list )
                dup "#" strcat trigger @ swap .locate-prop ( list obj )
                1 -3 rotate                                ( dividend list obj )
                do-prand1
        else
            2 = if
                .eval-loop atoi swap                       ( dividend list )
                dup "#" strcat trigger @ swap .locate-prop ( dividend list obj )
                do-prand1
            else
                .wipe-list
                "** TOO MANY ARGUMENTS FOR %PRAND **"
            then
        then
;
public do-prand
 
.
c
q
@register gen-desc-lib=desc-lib
@set $desc-lib=L
@set $desc-lib=2
