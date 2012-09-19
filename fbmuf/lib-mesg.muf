@prog lib-mesg
1 99999 d
1 i
( **** Message Object -- MSG- ****
  A message is a set of elements with a count and an information string,
    stored in properties.
  
  base is a string containing the name of the message.
  itemnum is the number of an item within a message.
  itemstr is a single item's string.
  infostr is the messages information string.
  {strrange} is a string range that is all the elements of the message
    with an integer count.
  
    MSG-destroy [base dbref -- ]
      Clears and removes the message.
  
    MSG-create  [{strrange} infostr base dbref -- ]
      Create a new message with the given items and info string on
      the given object with the given name.
  
    MSG-count   [base dbref -- count]
      Returns how many items are in the given message.
  
    MSG-info    [base dbref -- infostr]
      Returns the information string for the given message.
  
    MSG-setinfo [infostr base dbref -- ]
      Sets the information string on an existing message.
  
    MSG-message [base dbref -- {strrange}]
      Returns the items of a message as a range on the stack.
  
    MSG-item    [itemnum base dbref -- itemstr]
      Returns the given message item from the message.
  
    MSG-setitem [itemstr itemnum base dbref -- ]
      Sets the specified message item to the given string.
  
    MSG-insitem [itemstr itemnum base dbref -- ]
      Inserts a new message item into the message at the given point.
  
    MSG-append  [itemstr base dbref -- ]
      Appends a message item to the given message.
  
    MSG-delitem [itemnum base dbref -- ]
      Deletes the specified message item from the given message.
  
  
Message data type:
  Base#         Count of Message Items
  Base#/X       Message Items
  Base#/i       Info String
)
  
( $def the following if your muck uses the new style propdirized lists )
( ie:  listname#/1:line one    listename#/2:line two    etc. )
$def SLASHED_LISTS
  
$include $lib/props
$include $lib/lmgr
  
: MSG-destroy (base dbref -- )
$ifdef SLASHED_LISTS
    over over swap "#/i" strcat remove_prop
$else
    swap "/" strcat
    over over "i" strcat remove_prop
    swap
$endif
    LMGR-deletelist
;
  
: MSG-setinfo (infostr base dbref -- )
    swap
$ifdef SLASHED_LISTS
    "#" strcat
$endif
    "/i" strcat rot setpropstr
;
  
: MSG-create ({strrange} infostr base dbref -- )
    over over MSG-destroy
    rot 3 pick 3 pick MSG-setinfo
    ({strrange} base dbref)
    1 rot rot
$ifndef SLASHED_LISTS
    swap "/" strcat swap
$endif
    LMGR-putrange
;
  
: MSG-count (base dbref -- count)
$ifndef SLASHED_LISTS
    swap "/" strcat swap
    over over LMGR-getcount
    dup if
        rot rot pop pop exit
    else pop
    then
$endif
    LMGR-getcount
;
  
: MSG-message (base dbref -- {strrange})
$ifndef SLASHED_LISTS
    swap "/" strcat swap
    over over lmgr-getlist
    dup if
        dup 2 + rotate pop
        dup 2 + rotate pop
        exit
    else pop
    then
$endif
    LMGR-getlist
;
  
: safeclear (d s -- )
    over over propdir? if
        over over "" -1 addprop
        "" 0 addprop
    else
        remove_prop
    then
;
  
: MSG-clearoldinfo (base dbref -- )
    swap
$ifdef SLASHED_LISTS
    over over
    "#/i" strcat safeclear
$else
    "/i" strcat safeclear
$endif
;
  
: MSG-oldinfo (base dbref -- infostr)
    swap "/i" strcat getpropstr
;
  
: MSG-newinfo (base dbref -- infostr)
    swap "#/i" strcat getpropstr
;
  
: convert-info (base dbref value -- )
    3 pick 3 pick MSG-clearoldinfo
    rot rot MSG-setinfo
;
  
: MSG-info (base dbref -- infostr)
$ifdef SLASHED_LISTS
    over over MSG-newinfo
    dup if rot rot pop pop exit then
    pop over over MSG-oldinfo
    dup if dup -4 rotate convert-info exit then
$else
    over over MSG-oldinfo
    dup if rot rot pop pop exit then
    pop over over MSG-newinfo
    dup if dup -4 rotate convert-info exit then
$endif
    pop pop pop ""
;
  
: MSG-item (itemnum base dbref -- itemstr)
$ifndef SLASHED_LISTS
    3 pick 3 pick "/" strcat 3 pick
    LMGR-getelem
    dup if
        -4 rotate pop pop pop exit
    else pop
    then
$endif
    LMGR-getelem
;
  
: MSG-setitem (itemstr itemnum base dbref -- )
$ifndef SLASHED_LISTS
    swap "/" strcat swap
$endif
    LMGR-putelem
;
  
: MSG-insitem (itemstr itemnum base dbref -- )
$ifndef SLASHED_LISTS
    swap "/" strcat swap
$endif
    1 -4 rotate LMGR-insertrange
;
  
  
: MSG-append (itemstr base dbref -- )
$ifndef SLASHED_LISTS
    swap "/" strcat swap
$endif
    over over LMGR-getcount 1 +
    rot rot LMGR-putelem
;
  
: MSG-delitem (itemnum base dbref -- )
$ifndef SLASHED_LISTS
    swap "/" strcat swap
$endif
    1 -4 rotate LMGR-deleterange
;
  
PUBLIC MSG-destroy
PUBLIC MSG-create
PUBLIC MSG-count
PUBLIC MSG-info
PUBLIC MSG-setinfo
PUBLIC MSG-message
PUBLIC MSG-item
PUBLIC MSG-setitem
PUBLIC MSG-insitem
PUBLIC MSG-append
PUBLIC MSG-delitem
.
c
q
@register lib-mesg=lib/mesg
@register #me lib-mesg=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called lib-mesg
@set $tmp/prog1=/_defs/MSG-append:"$lib/mesg" match "MSG-append" call
@set $tmp/prog1=/_defs/MSG-count:"$lib/mesg" match "MSG-count" call
@set $tmp/prog1=/_defs/MSG-create:"$lib/mesg" match "MSG-create" call
@set $tmp/prog1=/_defs/MSG-delitem:"$lib/mesg" match "MSG-delitem" call
@set $tmp/prog1=/_defs/MSG-destroy:"$lib/mesg" match "MSG-destroy" call
@set $tmp/prog1=/_defs/MSG-info:"$lib/mesg" match "MSG-info" call
@set $tmp/prog1=/_defs/MSG-insitem:"$lib/mesg" match "MSG-insitem" call
@set $tmp/prog1=/_defs/MSG-item:"$lib/mesg" match "MSG-item" call
@set $tmp/prog1=/_defs/MSG-message:"$lib/mesg" match "MSG-message" call
@set $tmp/prog1=/_defs/MSG-setinfo:"$lib/mesg" match "MSG-setinfo" call
@set $tmp/prog1=/_defs/MSG-setitem:"$lib/mesg" match "MSG-setitem" call
@set $tmp/prog1=/_docs:@list $lib/mesg=1-50
