@prog lib-edit
1 99999 d
1 i
( Stack Based String Range Editing Routines
start, end, pos, and dest are all with reference to the start of the range
that is towards the bottom of the stack.  A 1 means the first item of the
range; the item deepest in the stack.  offset is the number of stack items
between the top of the string range and the bottom parameter.
  
    EDITsearch  [            {rng} ... offset string start -- {rng} ... pos ]
      Searches a range of strings for the first occurence of a substring. This
      is case sensitive, and returns the line number of the first occurence
  
    EDITreplace [ {rng} ... offset oldstr newstr start end -- {rng'} ...    ]
      Searches the range of strings for all occurences of a case sensitive
      substring, and replaces them with new text.
  
    EDITmove    [          {rng} ... offset dest start end -- {rng'} ...    ]
      Moves text within a string range from one line to another location,
      deleting the original.
  
    EDITcopy    [          {rng} ... offset dest start end -- {rng'} ...    ]
      Copies text within a string range from one line to another, inserting it
      in the new location.
  
    EDITlist    [         {rng} ... offset nums? start end -- {rng} ...     ]
      Lists the given set of lines within a string range, with an int telling
      it to prepending each line with a number and a colon.  Ie:
      "8: line eight."
  
    EDITleft    [               {rng} ... offset start end -- {rng'} ...    ]
      Left justify all the given lines within a string range.
  
    EDITcenter  [          {rng} ... offset cols start end -- {rng'} ...    ]
      Center justify all the given lines within a string range.
  
    EDITright   [          {rng} ... offset cols start end -- {rng'} ...    ]
      Right justify all the given lines within a string range.
  
    EDITindent  [          {rng} ... offset cols start end -- {rng'} ...    ]
      Indents all the given lines in a string range by COLS spaces.  if COLS
      is a negative integer, it undents by that many spaces.  It will never
      undent past left justification.
  
    EDITfmt_rng [          {rng} ... offset cols start end -- {rng'} ...    ]
      Formats the given subrange in the string range to COLS columns.  This
      is similar to the UNIX fmt command, in that it splits long lines and
      joins short lines.  A line that contains only spaces is considered a
      paragraph delimiter, and is not joined.
  
    EDITjoin_rng [              {rng} ... offset start end -- {rng'} ...    ]
      Joins all the given lines in the string range together, and returns the
      string range that results.
  
  
  
    EDITshuffle [                                    {rng} -- {rng'}    ]
      Take a range of items on the stack and randomize their order.
  
    EDITsort    [          {rng} ascending? CaseSensitive? -- {rng'}    ]
      Alphabetically sorts strings with integers telling it whether to sort
      in ascending or decending order, and if it should be case sensitive.
  
    EDITjoin    [                                    {rng} -- string    ]
      Join a range of strings on the stack into one string.
  
    EDITdisplay [                                    {rng} --           ]
      displays the range of strings on the stack to the user.
  
    EDITsplit   [     string splitchars rmargin wrapmargin -- {rng}     ]
      splits a string up into several lines in a range.  The criterion
      for where to split each line are as follows:  It splits at the last
      split character it can find between the rmargin and the wrapmargin.
      If it cannot find one, then it splits at the rmargin.
  
    EDITformat  [      {rng} splitchars rmargin wrapmargin -- {rng'}    ]
      Takes a range and formats it similarly to the way that the UNIX fmt
      command would, splitting long lines, and joining short ones.
)
  
$include $lib/strings
$include $lib/stackrng
$define SRNGextract sr-extractrng $enddef
$define SRNGinsert  sr-insertrng  $enddef
$define SRNGcopy    sr-copyrng    $enddef
  
  
: EDITforeach ( {str_rng} ... offset 'function data start end -- {str_rng'} )
              ( 'function must be addr of a [string data -- string] function)
    5 pick 6 + pick dup 4 pick <
    4 pick 4 pick > or if
        pop pop pop pop pop pop exit
    then
    6 pick + 7 + 3 pick - dup 1 +
    rotate 5 pick 7 pick execute
    swap -1 * rotate
    swap 1 + swap EDITforeach
;
  
  
  
: EDITsearch  ( {rng} ... offset string start -- {rng} pos )
    dup 4 pick 5 + pick > if pop pop pop 0 exit then
    3 pick 5 + dup pick + over - pick 3 pick
    instr if rot rot pop pop exit then
    1 + EDITsearch
;
  
  
: EDITreplace ( {rng} ... offset oldstr newstr start end -- {rng'} )
    over 6 pick 7 + pick > 3 pick 3 pick > or if
        pop pop pop pop pop exit
    then
    5 pick 7 + dup pick + 3 pick - dup 1 + rotate
    5 pick 7 pick subst swap -1 * rotate
    swap 1 + swap EDITreplace
;
  
  
: EDITmove    ( {rng} ... offset dest start end -- {rng'} )
    3 pick over > if
        rot over 4 pick - 1 + - rot rot
    else
        3 pick 3 pick >= if pop pop pop pop exit then
    then
    over - 1 + swap 4 pick 2 + rot rot SRNGextract
    ( {rng'} ... offset dest {subrng} )
    dup 3 + rotate over 3 + rotate
    ( {rng'} ... {rng2} offset dest )
    SRNGinsert
;
  
  
: EDITcopy    ( {rng} ... offset dest start end -- {rng'} )
    over - 1 + swap 4 pick 2 + rot rot SRNGcopy
    dup 3 + rotate over 3 + rotate
    SRNGinsert
;
  
  
  
: EDITsort    ( {rng} ascending?  CaseSensitive? -- {rng'} )
    if
        if SORTTYPE_CASE_ASCEND
        else SORTTYPE_CASE_DESCEND
        then
    else
        if SORTTYPE_NOCASE_ASCEND
        else SORTTYPE_NOCASE_DESCEND
        then
    then
	var! sorttype
	array_make sorttype @ array_sort array_vals
;
  
  
: EDITjoin ( {rng} -- string )
    dup 2 < if pop exit then
    rot STRsts rot STRsls
    over dup strlen 
    dup if
      1 - strcut pop
      ".!?" swap instr if "  " else " " then
    else
      pop pop " "
    then
    swap strcat strcat swap
    1 - EDITjoin
;
  
  
: EDITsplit-splitloop (string splitchars last -- string string)
    over not if
        swap pop
        dup not if pop dup strlen then
        strcut exit
    then
    swap 1 strcut rot rot 4 pick swap rinstr
    over over < if swap then pop
    EDITsplit-splitloop
;
  
: EDITsplit-split (string splitchars rmargin wrapmargin --
                   excess splitchars rmargin wrapmargin string)
    4 rotate 3 pick strcut swap 3 pick strcut
    (splitchars rmargin wrapmargin excess str wrap)
    6 pick 0 EDITsplit-splitloop
    rot rot strcat rot rot swap strcat 
    (splitchars rmargin wrapmargin str excess)
    -5 rotate
;
  
: EDITsplit-loop ({rng} string splitchars rmargin wrapwargin -- {rng})
    4 pick strlen 3 pick < if
        pop pop pop
        dup if swap 1 +
        else pop
        then exit
    then
    EDITsplit-split -6 rotate 5 rotate 1 + -5 rotate
    EDITsplit-loop
;
  
: EDITsplit   ( string splitchars rmargin wrapmargin -- {rng} )
    0 -5 rotate EDITsplit-loop
;
  
  
: EDITformat-loop  ( {rng} splitchars rmargin wrapmargin {rng2} -- {rng'} )
    dup 5 + pick not if
        dup 3 + dup rotate pop dup rotate pop
        dup rotate pop dup rotate pop pop exit
    then
    dup 4 + 1 1 SRNGextract pop
    ( {rng} splitchars rmargin wrapmargin {rng2} string )
    dup STRblank? not if
        over 6 + dup pick swap dup pick swap 1 - pick
        EDITsplit dup 2 + rotate + 1 - swap
        ( {rng} splitchars rmargin wrapmargin {rng2} string )
        over 6 + pick dup if
            3 pick + 6 + pick
            dup STRblank?
        else pop "" 1
        then
        ( {rng} splitchars rmargin wrapmargin {rng2} string nocat? )
        if pop swap 1 +
        else 2 EDITjoin over 6 + pick 3 pick + 5 + put
        then
        ( {rng} splitchars rmargin wrapmargin {rng2} )
    else
        pop "  " swap 1 +
    then
    EDITformat-loop
;
  
: EDITformat  ( {rng} splitchars rmargin wrapmargin -- {rng'} )
    0 EDITformat-loop
;
  
  
: EDITfmt_rng ( {str_rng} ... offset cols start end -- {str_rng'} ... )
    over - 1 + over swap
    ({rng} ... off cols start start cnt )
    5 pick 3 + swap rot SRNGextract
    ({rng'} ... off cols start {srng})
    "- " over 4 + rotate dup 20 - EDITformat
    ({rng'} ... off start {srng})
    dup 3 + rotate over 3 + rotate
    SRNGinsert
;
  
  
: EDITshuffle-innerloop ( {rng} shuffles loop -- {rng'} )
    dup not if pop exit then
    4 rotate 4 pick            ( {rng} shuffles loop item cnt )
    random 256 / swap %        ( {rng} shuffles loop item rnd )
    4 + -1 * rotate            ( {rng} shuffles loop )
    1 - EDITshuffle-innerloop
;
  
: EDITshuffle-outerloop ( {rng} shuffles -- {rng'} )
    dup not if pop exit then
    over EDITshuffle-innerloop
    1 - EDITshuffle-outerloop
;
  
: EDITshuffle ( {rng} -- {rng'} )
    8 EDITshuffle-outerloop
;
  
  
: EDITlist    ( {rng} ... offset nums? start end -- {rng} ... )
    over over >
    3 pick 6 pick 7 + pick > or if
        pop pop pop pop exit
    then
    4 pick 6 + dup pick + 3 pick - pick
    4 pick if
        "  " 4 pick intostr strcat
        dup strlen 3 - strcut
        swap pop ": " strcat
        swap strcat
    then
    dup not if pop " " then
    me @ swap notify
    swap 1 + swap EDITlist
;
    
  
  
: EDITdisplay ( {str_rng} -- )
    dup if
        dup 1 + rotate me @ swap notify
        1 - EDITdisplay exit
    then pop
;
  
  
: EDITleft-func (string null -- string )
    pop STRsls
;
  
: EDITleft ( {strrng} ... offset start end -- {strrng'} ... )
    'EDITleft-func "" -4 rotate -4 rotate EDITforeach
;
  
  
: EDITcenter-func (string cols -- string )
    swap STRstrip dup strlen
    dup 4 pick >= if
        pop swap pop exit
    then
    rot swap - 2 /
    "                                        "
    dup strcat dup strcat
    swap strcut pop swap strcat
;
  
: EDITcenter ( {strrng} ... offset cols start end -- {strrng'} ... )
    'EDITcenter-func -4 rotate EDITforeach
;
  
  
: EDITright-func (string cols -- string )
    swap STRstrip dup strlen
    dup 4 pick >= if
        pop swap pop exit
    then
    rot swap -
    "                                        "
    dup strcat dup strcat
    swap strcut pop swap strcat
;
  
: EDITright ( {strrng} ... offset cols start end -- {strrng'} ... )
    'EDITright-func -4 rotate EDITforeach
;
  
  
: EDITindent-func (str cols -- str)
    swap dup strlen swap STRsls
    dup strlen rot swap - rot +
    dup 1 < if pop exit then
    "                                        "
    dup strcat dup strcat
    swap strcut pop swap strcat
;
  
: EDITindent ( {str_rng} ... offset cols start end -- {str_rng'} ... )
    'EDITindent-func -4 rotate EDITforeach
;
  
 
: EDITjoin_rng ( {str_rng} ... offset start end -- {str_rng'} ... )
    over - 1 + over
    ({rng} ... off start cnt start )
    4 pick 2 + rot rot SRNGextract
    ({rng'} ... off start {srng})
    EDITjoin 1 4 rotate 4 rotate
    SRNGinsert
;
PUBLIC EDITsearch
PUBLIC EDITreplace
PUBLIC EDITmove
PUBLIC EDITcopy
PUBLIC EDITlist
PUBLIC EDITleft
PUBLIC EDITcenter
PUBLIC EDITright
PUBLIC EDITindent
PUBLIC EDITfmt_rng
PUBLIC EDITjoin_rng
  
PUBLIC EDITshuffle
PUBLIC EDITsort
PUBLIC EDITjoin
PUBLIC EDITdisplay
PUBLIC EDITsplit
PUBLIC EDITformat
.
c
q
@register lib-edit=lib/edit
@register #me lib-edit=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called lib-edit
@set $tmp/prog1=/_defs/EDITcenter:"$lib/edit" match "EDITcenter" call
@set $tmp/prog1=/_defs/EDITcopy:"$lib/edit" match "EDITcopy" call
@set $tmp/prog1=/_defs/EDITdisplay:"$lib/edit" match "EDITdisplay" call
@set $tmp/prog1=/_defs/EDITfmt_rng:"$lib/edit" match "EDITfmt_rng" call
@set $tmp/prog1=/_defs/EDITformat:"$lib/edit" match "EDITformat" call
@set $tmp/prog1=/_defs/EDITindent:"$lib/edit" match "EDITindent" call
@set $tmp/prog1=/_defs/EDITjoin:"$lib/edit" match "EDITjoin" call
@set $tmp/prog1=/_defs/EDITjoin_rng:"$lib/edit" match "EDITjoin_rng" call
@set $tmp/prog1=/_defs/EDITleft:"$lib/edit" match "EDITleft" call
@set $tmp/prog1=/_defs/EDITlist:"$lib/edit" match "EDITlist" call
@set $tmp/prog1=/_defs/EDITmove:"$lib/edit" match "EDITmove" call
@set $tmp/prog1=/_defs/EDITreplace:"$lib/edit" match "EDITreplace" call
@set $tmp/prog1=/_defs/EDITright:"$lib/edit" match "EDITright" call
@set $tmp/prog1=/_defs/EDITsearch:"$lib/edit" match "EDITsearch" call
@set $tmp/prog1=/_defs/EDITshuffle:"$lib/edit" match "EDITshuffle" call
@set $tmp/prog1=/_defs/EDITsort:"$lib/edit" match "EDITsort" call
@set $tmp/prog1=/_defs/EDITsplit:"$lib/edit" match "EDITsplit" call
@set $tmp/prog1=/_docs:@list $lib/edit=1-80
