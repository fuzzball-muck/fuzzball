( ***** Stack based range handling object -- SRNG ****                 
         offset is how many stack items are between range and parms    
         pos is the position within the range you wish to deal with.   
         num is the number of range items to deal with.                
  
  A 'range' is defines as a set of related items on the stack with an
  integer 'count' of them on the top.  ie:  "bat" "cat" "dog" 3
  
   sr-extractrng[     {rng} ... offset num pos -- {rng'} ... {subrng} ]
     pulls a subrange out of a range buried in the stack, removing them.
  
   sr-copyrng   [     {rng} ... offset num pos -- {rng} ... {rng}     ]
     copies a subrange out of a range buried in the stack.
  
   sr-deleterng [     {rng} ... offset num pos -- {rng'}              ]
     deletes a subrange from a range buried on the stack.
  
   sr-insertrng [ {rng1} ... {rng2} offset pos -- {rng}               ]
     inserts a subrange into the middle of a buried range on the stack.
  
   sr-filterrng [               {rng} funcaddr -- {rng'} {filtrdrng}  ]
     Takes the given range and tests each item with the given filter
     function address.  The function takes a single data value and
     returns an integer.  If the integer is non-zero, it pulls that
     data item out of the range and puts it into the filtered range.
     The data items can be of any type.
  
   sr-catrng    [                {rng1} {rng2} -- {rng}               ]
     concatenates two ranges into one range.
  
   sr-poprng    [                        {rng} --                     ]
     removes a range from the stack.  Also defined as 'popn'.
  
   sr-swaprng   [                {rng1} {rng2} -- {rng2} {rng1}       ]
     takes two ranges on the stack and swaps them.
  
)
  
: catranges ( {rng1} {rng2} -- {rng} )
    dup 2 + rotate +
;
  
  
: popoffn ({rng} -- )
    dup not if pop exit then
    begin
      swap pop 1 -
    dup not until pop
;
  
  
: copy-loop ( {rng} ... {rng2} offset num pos -- {rng} ... {rng} )
    4 pick 4 pick + 5 + pick over < 3 pick 1 < or if pop pop pop exit then
    ( {rng} ... {rng2} offset num pos )
    4 pick 4 pick + 5 + pick
    ( {rng} ... {rng2} offset num pos xc)
    5 pick + 4 pick + 6 + over - pick
    ( {rng} ... {rng2} offset num pos xn)
    5 rotate 1 + -5 rotate -5 rotate
    ( {rng} ... {rng2} offset num pos)
    1 + swap 1 - swap
    'copy-loop jmp
;
  
: copyrange ( {rng} ... offset num pos -- {rng} ... {rng} )
    0 -4 rotate copy-loop
;
  
  
: extract-loop ( {rng} ... {rng2} offset num pos -- {rng} ... {rng} )
    4 pick 4 pick + 6 + dup pick 3 pick <
    4 pick 1 < or if pop pop pop pop exit then
    ( {rng} ... {rng2} offset num pos rot)
    dup pick over + 1 + 3 pick - rotate
    ( {rng} ... {rng2} offset num pos rot xn)
    swap dup 1 + rotate 1 - swap -1 * rotate
    ( {rng} ... {rng2} offset num pos xn)
    -5 rotate 4 rotate 1 + -4 rotate
    ( {rng} ... {rng2} offset num pos)
    swap 1 - swap
    'extract-loop jmp
;
  
: extractrange ( {rng} ... offset num pos -- {rng'} ... {subrng} )
    0 -4 rotate extract-loop
;
  
  
: swapranges ( {rng1} {rng2} -- {rng2} {rng1} )
    dup 1 + 520 1 extractrange
    dup 2 + pick over + 4 + 3 + rotate pop
;
  
: deleterange  ( {rng} ... offset num pos -- {rng'} )
    extractrange popoffn
;
  
: insertrange  ( {rng1} ... {rng2} offset pos-- {rng} )
    3 pick not if pop pop pop exit then
    ( {rng1} ... {rng2} offset pos )
    rot 1 - rot rot 4 rotate
    ( {rng1} ... {rng2} offset pos elem )
    4 pick 5 + 4 pick + dup 2 + dup rotate
    ( {rng1} ... {rng2} offset pos elem rot rots rng1c )
    1 + dup rot -1 * rotate +
    ( {rng1} ... {rng2} offset pos elem rot )
    3 pick - -1 * rotate
    ( {rng1} ... {rng2} offset pos )
    'insertrange jmp
;
  
  
: filterrange-loop ( {rng} {rng2} funcaddr cnt-- {rng'} {rng2'} )
   3 pick 4 + pick over
   <= if pop pop pop exit then      (If done, then clean up and exit)
   3 pick over + 4 + pick           (get the datum from the old range)
   3 pick execute if                (check to see if datum is to be filtered)
      3 pick over + 4 + rotate         (get data from old range)
      -4 rotate                        (out it in the new one)
      rot 1 + rot rot                  (Increment the new range counter)
      3 pick 4 + pick 1 -              (decrememnt the old range counter)
      4 pick 4 + put                   (and put it back)
   then
   1 + 'filterrange-loop jmp        (repeat until done.)
;
: filterrange ( {rng} funcaddr -- {rng'} {filtrdrng} )
   0 swap 0 filterrange-loop
;
public catranges
public popoffn
public extractrange
public swapranges
public copyrange
public deleterange
public insertrange
public filterrange
