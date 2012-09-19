@prog Lib-Debug.muf
1 9999 d
i
$def HEADER "Lib-Debug.muf -- Syvel @ FurryFaire -- 18 Aug 1998"
$def VERSION "$Id: lib-debug.muf,v 1.1 2007/08/28 09:22:10 revar Exp $"
( Debugging Library, making figuring out why your program is breaking or
  doing unexpected things a little less spammy.

  NEW! Now Fuzzball 6.0 aware! Showvar and Showstack will properly convert
  arrays and dictionaries to human readable format, and identify floats.

  Functions provided:
    debugon       [   -- ] Set DEBUG flag on calling program.
    debugoff      [   -- ] Remove DEBUG flag on calling program.
    showstack     [ i -- ] Show i stack items to program owner.
    comment       [ s -- ] Show s comment to program owner.
    showvar       [ v -- ] show var or lvar v to program owner.
    pause_prog    [   -- ] NEW! Simply pauses the program and waits for you.
                           Enter any line <space works> to continue.

 [ I swiped the docs below from Mystique's library. --Syvel ]
 SHOWVAR setup:

 @set yourprogram=_Reg/Vars/xx:variable name
 @set yourprogram=_Reg/LVars/xx:variable name

 Showvar can distinguish the variables and produce names, but it can't read
 your program to get them.  The way they're counted is, normal VAR's start
 from 4 and go up <ones you define> from the beginning of your program.
 LVAR's start at 0 and go up, so:  Example fake program, showing variables.


 var myvariable            @set yourprogram=_Reg/Vars/4:myvariable
 var myname                @set yourprogram=_Reg/Vars/5:myname
 ...
 lvar temp                 @set yourprogram=_Reg/LVars/0:temp
 lvar cookies              @set yourprogram=_Reg/LVars/1:cookies
 ...

 If you don't setup registrations on your program for your variables, you'll
 get User Var #xx and User LVar #xx.
)
(
    Lib-Debug.muf -- Syvel @ FurryFaire -- 18 Aug 1998
    Copyright [c] 1998 Lawrence Cotnam Jr.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    [at your option] any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
)
$include $lib/strings
$include $lib/case

lvar CALL-OWN

: pause_prog ( -- )
	caller owner "-pausing-" notify read pop
;

: leftfitstr ( str pad width - s )
  3 pick strlen over >= if swap pop strcut pop else
    3 pick -3 rotate STRfillfield strcat
  then
;

: rightfitstr ( str pad width - s )
  3 pick strlen over >= if swap pop strcut pop else
    3 pick -3 rotate STRfillfield swap strcat
  then
;

: centfitstr ( str pad width - s )
  3 pick strlen over >= if swap pop strcut pop else
    3 pick -3 rotate
    STRfillfield dup strlen 2 / strcut rot strcat swap strcat
  then
;

: tostring ( ? -- s ; convert anything to a string )
    dup case
        int? when
            intostr
        end
        string? when
            dup pop
        end
        dbref? when
            intostr "#" swap strcat
        end
        address? when
            intostr
        end
        lock? when
            dup pop
        end
$ifndef __version<Muck2.2fb6.00
		array? when
			pop "(array)" (**TODO**)
		end
		dictionary? when
			pop "(dictionary)" (**TODO**)
		end
		float? when
			ftostr
		end
$endif
        default
            pop intostr
        end
    endcase
;

$ifndef __version<Muck2.2fb6.00
: parse_element ( ? -- s ; parse out elements of an array for display )
    dup case
        int? when
            intostr
        end
        string? when
			"\"" strcat "\"" swap strcat
        end
        dbref? when
            intostr "#" swap strcat
        end
        address? when
            intostr
        end
        lock? when
            0 pop
        end
		dictionary? when
			dup array_count intostr "{" strcat
			over array_first
			begin
				while
				( arr str idx )
				dup parse_element ":" strcat 4 pick
				( arr str idx str arr )
				3 pick array_getitem parse_element strcat " " strcat
				( arr str idx str )
				rot swap strcat
				( arr idx str ) 3 pick rot array_next
			repeat
			pop swap pop dup strlen -- strcut pop "}" strcat
		end
		array? when
			dup array_count intostr "{" strcat over array_count 0
			begin
				( a s i1 i2 )
				4 pick over array_getitem
				( a s i1 i2 e )
				parse_element over intostr ":" strcat swap strcat 4 rotate
				swap strcat " " strcat -3 rotate
				( a s i1 i2 )
				++ over over > not
			until
			pop pop swap pop dup strlen -- strcut pop "}" strcat
		end
		float? when
			ftostr
		end
        default
            pop intostr
        end
    endcase
;
$endif


: showstack ( i -- ; show i stack items. )
	caller owner dup CALL-OWN !
	"D-Bug> ___" notify
	dup depth > if pop depth then
	begin
		dup while
		dup 1 + pick
		dup case
			int? when
				intostr " (int)" strcat
			end
			string? when
				"\" (string)" strcat "\"" swap strcat
			end
			dbref? when
				intostr "#" swap strcat " (ref)" strcat
			end
			address? when
				intostr " (adr)" strcat
			end
			lock? when
				"\" (lock)" strcat "\"" swap strcat
			end
$ifndef __version<Muck2.2fb6.00
			dictionary? when
				parse_element " (dictionary)" strcat
			end
			array? when
				parse_element " (array)" strcat
			end
			float? when
				ftostr " (float)" strcat
			end
$endif
			default
				pop intostr " (var)" strcat
			end
		endcase
		"D-Bug> " 3 pick intostr " " 2 leftfitstr strcat "..." strcat
		swap strcat CALL-OWN @ swap notify
		1 -
	repeat pop
	CALL-OWN @ "D-Bug> ~~~" notify
;

: comment ( s -- ; show comment s )
  caller owner swap "D-Bug> " swap strcat notify
;

: debugon
  caller "D" set
;

: debugoff
  caller "!D" set
;

: parse-var ( i -- s ; return proper name for variable i )
    case
        0 = when
            "ME(0)"
        end
        1 = when
            "LOC(1)"
        end
        2 = when
            "TRIG(2)"
        end
        3 = when
            "COMMAND(3)"
        end
        default
            caller "_reg/vars/" 3 pick intostr strcat getpropstr dup if
                "(" strcat swap intostr strcat ")" strcat
            else
                pop "User Var " swap intostr strcat
            then
        end
    endcase
;

: parse-lvar ( i -- s ; return proper name for localvar i )
    intostr caller "_reg/lvars/" 3 pick strcat getpropstr dup if
        "(" strcat swap strcat ")" strcat
    else
        pop "User LVar " swap strcat
    then
;

: showvar ( v -- ; show variables or localvars from caller )
    "D-Bug> "
    over @ 3 pick intostr atoi variable @
    tostring swap tostring stringcmp if
        over intostr atoi parse-lvar
    else
        over intostr atoi parse-var
    then strcat "=" strcat
    swap @
    dup case
        int? when
            intostr " (int)" strcat
        end
        string? when
            " (str)" strcat
        end
        dbref? when
            intostr "#" swap strcat " (ref)" strcat
        end
        address? when
            intostr " (adr)" strcat
        end
        lock? when
            " (lock)" strcat
        end
$ifndef __version<Muck2.2fb6.00
		dictionary? when
			parse_element " (dictionary)" strcat
		end
		array? when
			parse_element " (array)" strcat
		end
		float? when
			ftostr " (float)" strcat
		end
$endif
        default
            pop intostr " (var)" strcat
        end
    endcase strcat caller owner swap notify
;
public showstack
public comment
public debugon
public debugoff
public showvar
public pause_prog
.
c
q
@set Lib-Debug=/_defs/comment:"$lib/Debug" match "comment" call
@set Lib-Debug=/_defs/debugoff:"$lib/Debug" match "debugoff" call
@set Lib-Debug=/_defs/debugon:"$lib/Debug" match "debugon" call
@set Lib-Debug=/_defs/showstack:"$lib/Debug" match "showstack" call
@set Lib-Debug=/_defs/showvar:"$lib/Debug" match "showvar" call
@set Lib-Debug=/_defs/pause_prog:"$lib/Debug" match "pause_prog" call
