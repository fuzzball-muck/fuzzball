@program con-multiguest
1 9999 d
1 i
(IMPORTANT NOTE:  Guests must be able to use @name and @password.)
(This program will force them to rename and repasswd themselves, so players..)
(...logging in won't be able to rename or repassword the chars, themselves.)
  
$def GuestName "Guest"  (Name of the main Guest char)
$def GuestPassWD "guest"  (Normal Guest Passwrod)
  
(Important Note 2:  There must be one fewer guest characters, including..)
("Guest" itself, than the defined NumGuests.  This lets the guests have...)
(...their names shuffled around.  There MUST be one guest named Guest.)
(ie:  if NumGuests is 4, then Guest, Guest1, Guest2, and Guest3 should exist)
  
$def NumGuests 9  (Max guests allowed connected + 1)
$def Wizard #1  (Change this to a wizard's dbref.  Preferably not #1)
  
(This next $def has the password creation scheme for the guests.)
(You'll want to change this to be something unique.)
(Don't make this just return "guest" or else the guests will be...)
(...able to rename and repassword themselves.)
$def PassWdMake (i -- s) 3 + 9 * intostr "TimE" swap strcat "tWINe" strcat
  
(takes guest# and returns that guests passwd)
: get-guest-num-passwd (i -- s)
    dup not if pop GuestPassWD exit then
    PassWdMake
;
  
(takes guest# and returns that guests name)
: get-guest-num-name (i -- s)
    dup if intostr else pop "" then
    GuestName swap strcat
;
  
(takes guest# and returns that guests dbref)
: get-guest-num-dbref (i -- d)
    me @ 1 addpennies
    get-guest-num-name .pmatch
;
  
(Finds first existing awake guest char)
: get-next-free-guest-num ( -- i)
    1 begin
        dup NumGuests <= while
        dup get-guest-num-dbref
        dup ok? if
            awake? not if break then
        else pop
        then
        1 +
    repeat
    dup NumGuests > if pop 0 exit then
;
  
(Finds the guest that was renamed to Guest)
: get-unclaimed-guest-num ( -- i)
    1 begin
        dup NumGuests <= while
        dup get-guest-num-dbref
        ok? not if break then
        1 +
    repeat
;
  
  
(Renames a guest from one guest number to another)
: rename-guest (orignum newnum -- )
    "@name me=" swap get-guest-num-name strcat
    " " strcat over get-guest-num-passwd strcat
    swap get-guest-num-dbref swap force
;
  
(Repasswords a guest from one guest number to another)
: repassword-guest (i i -- )
    "@password " rot
    get-guest-num-passwd strcat
    "=" strcat over
    get-guest-num-passwd strcat
    swap get-guest-num-dbref swap force
;
  
(Does a rename and a repassword of the guest.)
(These should be done together)
: redo-guest (i i -- )
    over over rename-guest
    repassword-guest
;
  
(Boot all connections of this [me @] guest)
: bootme ( -- )
    me @ descriptors
    begin
        dup while 1 - swap
        preempt
        descrcon conboot
        background
    repeat pop
;
  
(Boots all but oldest connection to this [me @] guest)
(This is used to prevent more than one guest logging in simultaneously)
: boot-extras ( -- )
    me @ descriptors
    begin
        dup 1 > while 1 - swap
        preempt descrcon
        "## I'm sorry, but you connected at the same time as someone else."
        over swap connotify
        dup "## Please log in as guest again." connotify
        conboot background
    repeat pop pop
;
  
(This shuffles the guests around, so Guest can be renamed to the lowest...)
(... numbered guest not currently logged in.)
: shuffle-guests ( -- )
    get-next-free-guest-num
    get-unclaimed-guest-num
    over over < if
        redo-guest
    else
        pop pop
    then
;
  
(rename this Guest to the available guest number.)
(Also, inform the player who they are logged in as.)
: setup-me-guest ( -- )
    get-unclaimed-guest-num
    0 over redo-guest
  
    get-guest-num-name
    "You are now connected to "
    swap strcat me @ swap notify
;
  
  
(Rename the next unused guest to Guest, so the next person can log in)
: prepare-next-guest ( -- )
    get-next-free-guest-num
    dup 0 redo-guest
;
  
  
(shuffles the guests around when Guest logs in.)
: guest-login (run this when the guest logs in)
    pop
    me @ name tolower GuestName tolower dup strlen strncmp if
        "Sorry, but you can't use this program."
        me @ swap notify pop exit
    then
  
    get-next-free-guest-num
    not if
        "Too many guest players connected.  Please try again, later."
        me @ swap notify bootme exit
    then
  
    boot-extras
    shuffle-guests
    setup-me-guest
    prepare-next-guest
;
  
(When this program is run from an action, make sure...)
(...that the guests passwords are all correct.)
: init-repass-guests ( -- )
    0 begin
        dup NumGuests <= while
        "@newpassword " over get-guest-num-name strcat
        "=" strcat over get-guest-num-passwd strcat
        Wizard swap force
        1 +
    repeat
    pop
;
  
: main
    command @ "Queued Event*" smatch
    if guest-login else init-repass-guests then
;
.
c
q
@register con-multiguest=con/multiguest
@set con-multiguest=w
@action mgs=me
@link mgs=con-multiguest
mgs
@recycle mgs
@prog con-callmultiguest
1 9999 d
1 i
( Have all the guest chars call this program from a _connect propqueue )
(This calls the !Link_OK multi-guest program, to keep the password...)
(...creation subroutines secret)
: go "$con/multiguest" match call ;
.
c
q
@set con-callmultiguest=L
@set con-callmultiguest=S
