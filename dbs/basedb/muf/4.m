( ***** Stack based range handling object -- SRNG ****                 
         offset is how many stack items are between range and parms    
         pos is the position within the range you wish to deal with.   
         num is the number of range items to deal with.                
  
  A 'range' is defines as a set of related items on the stack with an
  integer 'count' of them on the top.  ie:  "bat" "cat" "dog" 3
  
   sr-extractrng[     {rng} ... offset num pos -- {rng'} ... {subrng} ]
     pulls a subrange out of a range buried in the stack, removing them.
  
   sr-copyrng   [     {rng} ... offset num pos -- {rng} ... {subrng}  ]
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
 
$doccmd @list __PROG__=!@1-37
 
: catranges ( {rng1} {rng2} -- {rng} )
    dup 2 + rotate +
;
 
: copyrange ( {rng} ... offset num pos -- {rng} ... {subrng} )
    1 - var! pos
    var! num
    array_make var! stuff
    array_make var! range
    var subrng
 
    (avoid rangecheck errors to duplicate pre FB6 behaviour.) 
    num @ 0 <= range @ array_count pos @ < or if
        { }list subrng !
    else
        (limit operations to the actual size of the range [pre FB6 compat])
        pos @ num @ + range @ array_count - dup 0 > if
            num @ swap - num !
        else 
            pop
        then        
        range @ pos @ dup num @ + 1 -
        array_getrange subrng !
    then
    range @ array_vals
    stuff @ array_vals pop
    subrng @ array_vals
;
 
: extractrange ( {rng} ... offset num pos -- {rng'} ... {subrng} )
    1 - var! pos
    var! num
    array_make var! stuff
    array_make var! range
    var subrng
 
    (limit operations to the actual size of the range [pre FB6 compat])
    pos @ num @ + range @ array_count - dup 0 > if
        num @ swap - num !
    else 
        pop
    then
 
    (avoid rangecheck errors to duplicate pre FB6 behaviour.) 
    num @ 0 <= range @ array_count pos @ < or if
        { }list subrng !
    else
        range @ pos @ dup num @ + 1 -
        array_getrange subrng !
        range @ pos @ dup num @ + 1 -
        array_delrange range !
    then
    range @ array_vals
    stuff @ array_vals pop
    subrng @ array_vals
;
 
: swapranges ( {rng1} {rng2} -- {rng2} {rng1} )
    array_make var! tmp
    array_make var! tmp2
    tmp @ array_vals
    tmp2 @ array_vals
;
 
: deleterange  ( {rng} ... offset num pos -- {rng'} )
    extractrange popn
;
 
: insertrange  ( {rng1} ... {rng2} offset pos -- {rng} ... )
    1 - var! pos
    var! offset
    array_make var! newrng
    offset @ array_make var! stuff
    array_make var! range
 
    (limit operations to the actual size of the range [pre FB6 compat])
    pos @ range @ array_count - dup 0 > if
        pos @ swap - pos !
    else 
        pop
    then
    range @ pos @ newrng @ array_insertrange
    array_vals
    stuff @ array_vals pop
;
 
: filterrange ( {rng} funcaddr -- {rng'} {filtrdrng} )
    var! cb
    { }list var! outrng
    array_make var! range
    range @ foreach
        dup cb @ execute if
            outrng @ dup array_count array_setitem outrng !
            range @ swap array_delitem range !
        else
            pop
        then
    repeat
    range @ array_vals
    outrng @ array_vals
;
 
public catranges
public copyrange
public deleterange
public extractrange
public filterrange
public insertrange
public swapranges
 
$pubdef sr-catrng       __PROG__ "catranges" call
$pubdef sr-copyrng      __PROG__ "copyrange" call
$pubdef sr-deleterng    __PROG__ "deleterange" call
$pubdef sr-extractrng   __PROG__ "extractrange" call
$pubdef sr-filterrng    __PROG__ "filterrange" call
$pubdef sr-insertrng    __PROG__ "insertrange" call
$pubdef sr-poprng       popn
$pubdef sr-swaprng      __PROG__ "swapranges" call
 
$pubdef SRNGcat         __PROG__ "catranges" call
$pubdef SRNGcopy        __PROG__ "copyrange" call
$pubdef SRNGdelete      __PROG__ "deleterange" call
$pubdef SRNGextract     __PROG__ "extractrange" call
$pubdef SRNGfilter      __PROG__ "filterrange" call
$pubdef SRNGinsert      __PROG__ "insertrange" call
$pubdef SRNGpop         popn
$pubdef SRNGswap        __PROG__ "swapranges" call
