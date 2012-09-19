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
  
$include $lib/props
$include $lib/lmgr
  
: MSG-destroy (base dbref -- )
    over over swap "#/i" strcat remove_prop
    LMGR-deletelist
;
  
: MSG-setinfo (infostr base dbref -- )
    swap "#/i" strcat rot setpropstr
;
  
: MSG-create ({strrange} infostr base dbref -- )
    over over MSG-destroy
    rot 3 pick 3 pick MSG-setinfo
    ({strrange} base dbref)
    1 rot rot LMGR-putrange
;
  
: MSG-count (base dbref -- count)
    LMGR-getcount
;
  
: MSG-message (base dbref -- {strrange})
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
    swap "/i" strcat safeclear
;
  
: MSG-oldinfo (base dbref -- infostr)
    swap "/i" strcat getpropstr
;
  
: MSG-newinfo (base dbref -- infostr)
    swap "#/i" strcat getpropstr
;
  
: convert-info (base dbref -- )
    over over MSG-oldinfo
    3 pick 3 pick MSG-setinfo
    MSG-clearoldinfo
;
  
: MSG-info (base dbref -- infostr)
    over over MSG-newinfo
    dup if rot rot pop pop exit then
    pop over over MSG-oldinfo
    dup if rot rot convert-info exit then
    pop pop pop ""
;
  
: MSG-item (itemnum base dbref -- itemstr)
    LMGR-getelem
;
  
: MSG-setitem (itemstr itemnum base dbref -- )
    LMGR-putelem
;
  
: MSG-insitem (itemstr itemnum base dbref -- )
    1 -4 rotate LMGR-insertrange
;
  
  
: MSG-append (itemstr base dbref -- )
    over over LMGR-getcount 1 +
    rot rot LMGR-putelem
;
  
: MSG-delitem (itemnum base dbref -- )
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
