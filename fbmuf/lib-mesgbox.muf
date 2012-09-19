@prog lib-mesgbox
1 99999 d
1 i
( **** Message Box Object -- MBOX- ****
    MBOX-badref?  [refnum base dbref -- bad?]
      Returns whether or not the given message number exists.
  
    MBOX-ref2prop [refnum base dbref -- mbase dbref]
      Returns the name and dbref of the specific message number in the given
      message box.
  
    MBOX-ref2num  [refnum base dbref -- msgnum]
      Returns the absolute message number of a message referred to by the
      given reference number.  See MBOX-num2ref for an explanation of those
      terms.

    MBOX-num2ref  [msgnum base dbref -- refnum]
      Returns the reference number of the message that has the given absolute
      message number.  The reference number is the position number of the
      message in the message list.  The absolute message number is the number
      that the message was given when it was created.  The absolute reference
      number of a message will never change.  If the message with the given
      message number no longer exists, then the reference to the first message
      after the given message number is returned.

    MBOX-create   [base dbref -- ]
      Creates a new message box with no messages in it.
  
    MBOX-count    [base dbref -- count]
      Returns the number of messages contained in the given message box.
  
    MBOX-destroy  [base dbref -- ]
      Destroys the message box and all of it's contents.
  
    MBOX-append   [{strrange} infostr base dbref -- refnum]
      Creates a new message with the given message items and info string
      and appends it at the end of the message box. Returns the message's
      number.
  
    MBOX-insmesg  [{strrange} infostr refnum base dbref -- refnum]
      Creates a new message with the given message items and info string
      and inserts it before the given message number in the message box.
      Returns the message's number.
  
    MBOX-delmesg  [refnum base dbref -- ]
      Delete the given message number in the message box.  It moves the
      rest of the messages after it up in the message box.
  
    MBOX-setmesg  [{strrange} infostr refnum base dbref -- ]
      Sets the given message number in the given message box to contain the
      given message items and info string.
  
    MBOX-msginfo  [refnum base dbref -- infostr]
      Returns the info string of the given message number in the message box.
  
    MBOX-setinfo  [refnum base dbref -- ]
      Sets the info string for the given message number in the message box.
  
    MBOX-message  [refnum base dbref -- {strrange}]
      Returns the contents of the given message number in the message box
      as a range of strings.
  
)
  
$include $lib/mesg
  
: MBOX-count (base dbref -- count)
    MSG-count
;
  
: MBOX-ref2num (refnum base dbref -- mesgnum)
    MSG-item atoi
;
  
lvar n2r_num
lvar n2r_cnt
: MBOX-num2ref (mesgnum base dbref -- refnum)
    rot n2r_num !
    over over MBOX-count n2r_cnt !
    1 begin
	dup n2r_cnt @ <= while
	dup 4 pick 4 pick MBOX-ref2num
	n2r_num @ >= if
	    rot rot pop pop exit
	then
	1 +
    repeat
    pop pop pop 0
;

: MBOX-ref2prop (refnum base dbref -- base' dbref)
    rot 3 pick 3 pick MBOX-ref2num intostr
    rot "/" strcat swap strcat swap
;
  
: MBOX-delmesg (refnum base dbref -- )
    3 pick 3 pick 3 pick MBOX-ref2prop
    MSG-destroy MSG-delitem
;
  
: MBOX-destroy-loop (base dbref cnt -- )
    dup not if pop pop pop exit then
    dup 4 pick 4 pick MBOX-delmesg
    1 - MBOX-destroy-loop
;
  
: MBOX-destroy (base dbref -- )  
    over over over over MBOX-count
    MBOX-destroy-loop
    MSG-destroy
;
  
: MBOX-create ( base dbref -- )
    over over MBOX-destroy
    0 "0" 4 rotate 4 rotate MSG-create
;
  
  
: MBOX-append ({strrange} infostr base dbref -- refnum)
    over over MSG-info
    ({strrange} infostr base dbref next)
    dup atoi 1 + intostr 4 pick 4 pick MSG-setinfo
    ({strrange} infostr base dbref next)
    3 pick 3 pick MSG-append
    ({strrange} infostr base dbref)
    over over MSG-count
    ({strrange} infostr base dbref ref)
    dup 6 pick 6 + -1 * rotate
    (ref {strrange} infostr base dbref ref)
    rot rot MBOX-ref2prop
    (ref {strrange} infostr mbase dbref)
    MSG-create
    (ref)
;
  
  
: MBOX-insmesg ({strrange} infostr refnum base dbref -- refnum)
    over over MSG-info
    dup atoi 1 + intostr
    4 pick 4 pick MSG-setinfo
    ({strrange} infostr refnum base dbref next)
    3 pick 3 pick MBOX-count
    ({strrange} infostr refnum base dbref next cnt)
    5 rotate over over > if swap then pop swap
    ({strrange} infostr base dbref refnum next)
    dup rot 5 pick 5 pick MSG-insitem
    ({strrange} infostr base dbref next)
    atoi rot rot MBOX-ref2prop
    ({strrange} infostr mbase dbref)
    MSG-create
;
  
  
: MBOX-setmesg ({strrange} infostr refnum base dbref -- )
    MBOX-ref2prop MSG-create
;
  
  
: MBOX-msginfo (refnum base dbref -- infostr)
    MBOX-ref2prop MSG-info
;
  
  
: MBOX-setinfo (infostr refnum base dbref -- )
    MBOX-ref2prop MSG-setinfo
;
  
  
: MBOX-message (refnum base dbref -- {strrange})
    MBOX-ref2prop MSG-message
;
  
: MBOX-badref? (refnum base dbref -- bad?)
    MBOX-count over < swap 1 < or
;
  
PUBLIC MBOX-ref2prop
PUBLIC MBOX-ref2num
PUBLIC MBOX-num2ref
PUBLIC MBOX-create
PUBLIC MBOX-count
PUBLIC MBOX-destroy
PUBLIC MBOX-append
PUBLIC MBOX-insmesg
PUBLIC MBOX-delmesg
PUBLIC MBOX-setmesg
PUBLIC MBOX-msginfo
PUBLIC MBOX-setinfo
PUBLIC MBOX-message
PUBLIC MBOX-badref?
.
c
q
@register lib-mesgbox=lib/mesgbox
@register #me lib-mesgbox=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called lib-mesgbox
@set $tmp/prog1=/_defs/MBOX-append:"$lib/mesgbox" match "MBOX-append" call
@set $tmp/prog1=/_defs/MBOX-badref?:"$lib/mesgbox" match "MBOX-badref?" call
@set $tmp/prog1=/_defs/MBOX-count:"$lib/mesgbox" match "MBOX-count" call
@set $tmp/prog1=/_defs/MBOX-create:"$lib/mesgbox" match "MBOX-create" call
@set $tmp/prog1=/_defs/MBOX-delmesg:"$lib/mesgbox" match "MBOX-delmesg" call
@set $tmp/prog1=/_defs/MBOX-destroy:"$lib/mesgbox" match "MBOX-destroy" call
@set $tmp/prog1=/_defs/MBOX-insmesg:"$lib/mesgbox" match "MBOX-insmesg" call
@set $tmp/prog1=/_defs/MBOX-message:"$lib/mesgbox" match "MBOX-message" call
@set $tmp/prog1=/_defs/MBOX-msginfo:"$lib/mesgbox" match "MBOX-msginfo" call
@set $tmp/prog1=/_defs/MBOX-ref2prop:"$lib/mesgbox" match "MBOX-ref2prop" call
@set $tmp/prog1=/_defs/MBOX-ref2num:"$lib/mesgbox" match "MBOX-ref2num" call
@set $tmp/prog1=/_defs/MBOX-num2ref:"$lib/mesgbox" match "MBOX-num2ref" call
@set $tmp/prog1=/_defs/MBOX-setinfo:"$lib/mesgbox" match "MBOX-setinfo" call
@set $tmp/prog1=/_defs/MBOX-setmesg:"$lib/mesgbox" match "MBOX-setmesg" call
@set $tmp/prog1=/_docs:@list $lib/mesgbox=1-58
@set $tmp/prog1=/_version:1.2
