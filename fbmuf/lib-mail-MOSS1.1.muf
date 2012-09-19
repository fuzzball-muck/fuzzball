@name MOSS1.0.muf=MOSS1.1.muf
@prog MOSS1.1.muf
1 10000 d
i
(
Mail Organization and Storage System <MOSS> 1.1 my Fre'ta @ FurryMUCK, etc etc
Copyright 1996 by Fre'ta <Steven Lang>
 
This code may be freely distributed!
If you make a change, please make some notice about it, like in the version
string, and give credit where credit is due.  Also, if you think it is
something that might benefit <I know I mispelled that> others, please tell
me about it!  I'm trying to design this to suit everything from the small
little garage MUCK to the big ones.
 
To install, just put this text into a program, and link a action to it!
Thats all!  If you would like page #mail that uses the mailer, check for
the newest cmd-page, and enable LIBMAIL for the mail.
 
The security of this program is pretty good, it's impossible for a mortal to
forge mail <Since it's written on wizard only properties> and it's a pain
for wizzes to forge mail if encryption is on.  Reading someone elses mail is
just as hard.  Of course theres nothing to stop a wiz from copying the
encryption code out of this, but it's still a pain.
 
A wizbitted program can send a quick-mail message using the quick-mail
interface, however it will show up with the 'you quick-mail' stuff.  Perhaps
in the future I will add code to allow programs to send messages and take
care of any displaying themselves.
)
 
( Config stuff..  If you don't want an option, comment out the line that )
( defines it, otherwise leave it defined, and add a value if needed )
( If it is something real long, a function would be recommended to save )
( space when compiled, however a function or a define will work )
 
( Define this to the maximum number mails can be forwarded to )
$def MAXFORWARD 5
( Define this to the max number someone can send a mail to )
$def MULTIMAX 30
( Define this to the maximum number of new messages a user can have )
$def INBOXSIZE 40
( Define this to the maximum number of saved messages before they get )
( marked to delete )
$def MAXSAVED 40
( Define this to how long until saved messages expire in seconds )
( 1 day is 86400 seconds, a week is 604800 seconds )
( A value like 86400 30 * for 30 days is a valid value )
$def EXPIRETIME 604800
( Define this to how long players have to do something when their saved )
( mailbox is full or a message is expired )
$def GRACETIME 86400
( If you want to limit message sizes, define this to either LINE for )
( limiting it to so many lines, or CHARACTER to limiting it to so many )
( characters )
$define MAILLIMIT CHARACTER $enddef
( And how many to limit it to )
$def MAILLIMITCOUNT 4096
( Maximum recursion for forward and alias evaluation before it gives up )
$def RECURSIONLIMIT 9
( Define this if you want messages to be encrypted )
$def ENCRYPTION
( Define this to a unique string with lotsa different chars - It's used to )
( generate the encryption keys, and can contain any kind of character the )
( MUCK would normally accept as input )
$def KEY "Put the string here"
( A program to call to convert any other form of mail to MOSS format )
( This program should take a dbref and an address, calling the address as )
( to from subject {mesg} sent read type )
( to is a space seperated list of dbrefs in int format, from is the dbref )
( it's from, subject is the string subject, {mesg} is a string range, sent )
( is a 'systime' of when it was sent, read is a 'systime of when it was read )
( and type is 0 for new or 1 for saved )
( For an example program, look for MOSS-convpmail the same place you found )
( this )
( $def CONVERT "$mailconvert" match )
( Just a few commands that determin if a player is a guest - Change to fit )
( your MUCK if needed )
( This needs to be defined, if you don't want to check guests just leave it )
( as 'pop 0' )
$def ISGUEST? name "Guest" stringpfx
( The prefix for alias props )
$def ALIASPROP "_page/alias/"
( Same, for the global aliases )
$def GLOBALALIASPROP "_page/galias/"
( Where to find global aliases, this should be set to a dbref for best results )
$def GLOBALALIASDBREF "page" match dup ok? if getlink then
( Converts a dbref to an ignored prop name )
$def IGNOREPROP "ignore#" swap int intostr strcat
( Where to find ignored props, again this should be set to a dbref )
$def IGNOREDBREF "page" match dup ok? if getlink then
( Prefix for page props, _page_ for older vers, _page/ for newer vers )
$def PAGEPROP "_page/"
 
( No user servicable parts beyond this point )
 
$def vernum 3
$def shortver "MOSS 1.1 by Fre'ta"
$def VERSION "Mail Organization and Storage System (MOSS) 1.1 by Fre'ta"
$def msgprep "** "
$def qmodemask 15
$def qheadermask 4
$def qnotheadermask 3
$def qpromptmask 2
$def qsavemask 1
 
( a *mesg* in a comment refers to to from subject {s}, being the compressed )
( list of dbrefs to send to, the person it's from, the subject, )
( and the message itself )
 
lvar lcount
lvar lines
lvar linelen
lvar wraplen
lvar editor
lvar lastkeynum
lvar lastkey
lvar runmode
( 0=update event, 1=interactive, 2=one-shot, 16+=quick )
( qflags: XXX )
(        / \/ )
( Display   \ )
( Header     Display mode )
( prompt mode 00=No prompt and delete, 01=no prompt and save, 10=prompt )
 
: poprange ( {?} --  Pops a range - Also known as POPN )
  begin dup while swap pop 1 - repeat pop
;
 
: pageproploc ( dbref -- dbref' )
  dup "_proploc" getpropstr
  dup if
    dup "#" 1 strncmp not if
      1 strcut swap pop
    then
    atoi dbref
    dup ok? if
      dup owner 3 pick
      dbcmp if swap then
    else swap
    then pop
  else pop
  then
;
 
: askme2 ( -- s  Reads from the user )
  lines @ lcount !
  begin
    read
    dup strip "\"" 1 strncmp not if
      strip 1 strcut swap pop "<In the mailer> You say, \"" over
      strcat "\"" strcat me @ swap notify
      me @ name " says, \"" strcat swap strcat "\"" strcat
      me @ location swap me @ swap 1 swap notify_exclude continue
    then
    dup strip ":" 1 strncmp not if
      strip 1 strcut swap pop dup 1 strcut pop dup if
        ". ,':!?-" swap instr not if " " swap strcat then
      then me @ name swap strcat
      dup me @ location swap me @ swap 1 swap notify_exclude
      "<In the mailer> " swap strcat me @ swap notify continue
    then
  dup until
;
 
: askme ( -- s  Reads from the user and swallows .getlock )
  begin askme2 dup ".getlock" strcmp until
;
 
: askmeraw ( -- s  Reads from the yser and swallows .getlock, " and : allowed )
  begin read dup ".getlock" strcmp until
;
 
: wrapsplit ( s1 i -- line1 line2  Splits a line at length, preserving words )
  over strlen over >= over and if
    over " " instr if
      strcut swap dup " " rinstr dup if
        1 - strcut 1 strcut swap pop rot strcat
      else
        pop strcat dup " " instr 1 - strcut 1 strcut swap pop
      then exit then then
  pop ""
;
 
: tellme ( s --  Tells the user )
  wraplen @ wrapsplit dup if
    swap tellme tellme exit
  then pop
  lcount @ dup 1 = if
    pop me @ "_prefs/mail/expert" getpropstr if
      " --- More ---"
    else
      " --- More [Yes/No/Continuous] ---"
    then me @ swap notify askme
    tolower dup "n" 1 strncmp not if
      pop pop -2 lcount ! exit
    then
    "c" 1 strncmp not if -1 dup lcount ! else 1 then
  then
  dup -2 > if
    me @ rot notify
  else
    swap pop
  then
  dup 1 > if
    1 - lcount !
  else
    pop
  then
;
 
: claimlock ( d -- i  Tries to lock player d's mailbox, on falure returns the )
          (         PID with the current lock )
  dup "@mail/lock" getprop dup if
    dup ispid? if
      over "@mail/lock/time" getprop #0 "_sys/startuptime" getprop > if
        over me @ dbcmp if
          "Attempting to claim lock from process " over intostr strcat tellme
          over "@mail/lock/owner" getprop ".getlock" force 0 sleep
          pop "@mail/lock" getprop dup if
            "Unable to claim lock." tellme
          else
            pop me @ claimlock
          then
          exit
        else
          swap pop exit
        then
      else
        pop "Removing stale lock." tellme
      then
    else
      pop "Removing stale lock." tellme
    then
    dup "@mail/lock" remove_prop
  else pop then
  dup "@mail/lock" pid setprop
  dup "@mail/lock/time" systime setprop
  "@mail/lock/owner" me @ setprop 0
;
 
: killlock ( i -- i  Kills the process associated with the lock, returning )
           (         0 upon success or the PID of the locking process if a )
           (         different process has the lock )
  me @ "@mail/lock" getprop over over = if
    me @ "@mail/lock/owner" getprop "Killing process so " me @ name
    " can access their mail" strcat strcat notify
    pop kill pop me @ "@mail/lock" remove_prop
    me @ claimlock
  else
    swap pop
    dup not if pop me @ claimlock then
  then
;
 
: clearlock ( d --  Clears the lock for d )
  dup "@mail/lock" getprop pid = if "@mail/lock" remove_prop else pop then
;
 
: getmylock ( -- i  Claims my lock, or returns 0 on falure )
  me @ claimlock begin dup while
    "Process " over intostr strcat
    " has a lock on your mailbox, kill the process?" strcat tellme askme
    tolower "y" 1 strncmp not if killlock else
      "Mail command cancelled." me @ swap notify pop 0 exit
    then
  repeat pop 1
;
 
: instrcount ( s1 s2 --  Returns the number of occurences of s2 in s1 )
  dup not if pop pop 0 exit then
  swap dup strlen rot dup strlen 4 rotate rot "" swap subst strlen
  rot swap - swap /
;
 
: cmdnum ( s1 s2 --  Returns the index of s2 in a space seperated list s1 )
  " " swap tolower strip strcat swap " " swap tolower strip strcat swap
  over over instr rot swap strcut rot instr if
    pop 0 exit
  then
  " " instrcount
;
 
: initwrap ( --  Initializes word-wrap )
  me @ "_prefs/mail/wrap" getpropstr dup if
    atoi dup linelen ! wraplen !
  else
    pop 0 wraplen ! 78 linelen !
  then
;
 
: initpaging ( --  Initializes paging )
  me @ "_prefs/mail/paging" getpropstr atoi dup
  lines ! lcount !
;
 
: getqflags ( -- i )
  0
  me @ "_prefs/mail/qreadmode" getpropstr dup if
    "delete save prompt" swap cmdnum dup if 1 - then
    +
  else pop then
  me @ "_prefs/mail/qheader" getpropstr dup if
    "abbreviated full" swap cmdnum dup if 1 - 4 * then
    +
  else pop then
;
 
: init ( --  Initializes stuff )
  "me" match me !
  me @ player? not if
    runmode @ if
      "Sorry, only players may send mail." me @ swap notify 0
    else -1 then runmode ! exit
  then
  me @ ISGUEST? if
    runmode @ if
      "Sorry, guests are not allowed to use mail." me @ swap notify 0
    else -1 then runmode ! exit
  then
  0 lastkeynum !
  runmode @ if
    me @ "_prefs/mail/lastver" getpropstr atoi dup not if
      pop "Welcome to " VERSION strcat tellme
      "Setting up defaults in _prefs/mail..." tellme
      me @ "_prefs/mail/qpre" "On %d, %n said:" setprop
      me @ "_prefs/mail/qpfx" "> " setprop
      me @ "_prefs/mail/qpost" " " setprop
      me @ "_prefs/mail/repwrap" "78" setprop
      me @ "_prefs/mail/lastver" vernum intostr setprop
      "Use 'mail #options' to change the default settings." tellme
    else
      dup vernum = not if
        "Mail has been upgraded to " SHORTVER strcat
        ", type 'mail #changes' for details." strcat tellme
        dup 1 = if
          pop 2 me @ "_prefs/mail/repwrap" "78" setprop
        then
        dup 2 = if
          pop 3 me @ "_prefs/mail/qflags" over over getprop
          rot rot remove_prop
          atoi dup 3 bitand dup if
            1 = if "save" else "prompt" then
            me @ "_prefs/mail/qreadmode" rot setprop
          else pop then
          4 bitand if
            me @ "_prefs/mail/qheader" "full" setprop
          then
        then
        me @ "_prefs/mail/lastver" rot intostr setprop
      else pop then
    then
  then
  initpaging initwrap
  "$lib/editor" match editor !
  runmode @ 16 = if
    runmode @ getqflags + runmode !
  then
;
 
: showmenu ( s long short --  Displays a menu prompt )
  me @ "_prefs/mail/expert" getpropstr tolower "y" 1 strncmp not if
    ": [" swap strcat "]" strcat swap pop strcat me @ swap notify
  else
    pop swap ": Please select a command by using at least the first letter."
    strcat me @ swap notify
    "Menu commands available: " swap strcat " H>elp" strcat me @ swap notify
  then
;
 
: getname ( d -- s  Gets the name checking for invalid name )
  dup ok? not if
    pop "*Toaded*" exit
  then dup player? not if
    pop "*Toaded*" exit
  then name
;
 
: makespaces ( i -- s  Makes spaces )
  dup 20 > if
    20 - makespaces 20 swap
  else "" then
  swap "                    " swap strcut pop strcat
;
 
: strpad ( s i -- s  Pads s to i characters )
  over strlen - makespaces strcat
;
 
: ignored ( d -- i  If d is ignored, notify and return 1 )
  IGNOREDBREF over IGNOREPROP getpropstr " " swap over strcat strcat
  " #" me @ int intostr " " strcat strcat instr not if
    pop 0 exit then
  dup PAGEPROP "ignoremsg" strcat getpropstr dup if
    "Ignore message from " 3 pick name strcat ": " strcat swap
  else
    pop dup name " is ignoring you"
  then
  strcat tellme
  dup PAGEPROP "inform?" strcat getpropstr "yes" strcmp not if
    me @ dup name swap " tried to mail you, but you are ignoring %o"
    pronoun_sub strcat notify 1 exit
  then
  pop 1
;
 
: read_alias ( s -- s  Reads alias named s )
  me @ pageproploc over ALIASPROP swap strcat getpropstr dup if
    swap pop exit then
  pop GLOBALALIASDBREF dup ok? not if pop pop "" exit then
  GLOBALALIASPROP rot strcat getpropstr
;
 
: refmatch ( s -- d  Matches against a dref )
  dup "#" 1 strncmp if pop #-1 exit then
  1 strcut swap pop dup number? not if pop #-1 exit then
  atoi dbref
;
 
: realname2ref ( s i i -- {d}  Converts a name of a player or alias to )
               (               dbrefs, checking for recursion )
               (               The first i is the quiet flag, 0=quiet )
  1 + dup RECURSIONLIMIT > if
    "Alias recursion limit reached, ignored." tellme pop pop pop 0 exit then
  3 pick not if pop pop pop 0 exit then
  rot over if
    dup "*" stringpfx not if
      "*" over strcat match dup ok? not if
        pop dup refmatch dup ok? if dup player? not if pop #-1 then then
      then
      dup ok? if
        swap pop swap pop dup ISGUEST? if
          pop if "Sorry, you may not send mail to guests." tellme then
          0 exit
        then swap pop dup ignored if pop 0 else 1 then exit
      then pop
    else
      1 strcut swap pop
    then
    over if
      dup read_alias dup not if
        pop rot if
          "Unknown player: " swap strcat tellme
        else pop then
        pop 0 exit then
      swap pop
    then
  then
  " " swap strcat
  begin dup " (" instr dup while
    strcut
    dup ")" instr dup if
      strcut swap pop strcat
    else
      pop pop
    then
  repeat pop
  strip begin dup "  " instr while " " "  " subst repeat
  " " explode 0 begin over while
    rot 3 pick 3 pick + dup 5 + pick swap 4 + pick realname2ref
    dup 2 + pick over + over 2 + put
    begin dup while
      swap over dup 4 + pick over dup 5 + pick swap - + + 2 + 0 swap -
      rotate 1 -
    repeat
    pop swap 1 - swap
  repeat
  swap pop dup 2 + rotate pop dup 2 + rotate pop
;
 
: name2ref ( s -- {d}  Converts a name of a player or alias to dbrefs )
  strip dup not if pop 0 exit then 1 -1 realname2ref
;
 
: quietname2ref ( s -- {d}  Converts a name to dbrefs quietly )
  strip dup not if pop 0 exit then 0 -1 realname2ref
;
 
: refduppurge ( {d} -- {d}  Purges duplicate dbrefs )
  dup 2 + 0 swap - 0 swap rotate begin dup while
    1 - swap
    over begin dup while
      1 - over over 5 + pick dbcmp if
        rot 1 - rot rot dup 4 + rotate pop
      then
    repeat pop
    over dup 4 + pick + 3 + 0 swap - rotate
    dup dup 3 + pick 1 + swap 2 + put
  repeat pop
;
 
: refcompress ( {d} -- s  Turns a range of dbrefs to a string of dbrefs )
  "" swap begin dup while
    1 - rot int intostr " " swap strcat rot swap strcat swap
  repeat pop strip
;
 
: nextcompref ( s -- s d  Gets the next dbref in a compressed list )
  dup " " instr dup not if pop "" swap atoi dbref exit then
  strcut swap striptail atoi dbref
;
 
: getrecpt ( s -- s s  Gets the real recepient/s of a forwarded mail and
                       returns everyone who can recieve it and everyone to
                       send it to )
  0 swap begin dup while
    nextcompref dup ok? if dup player? not if pop #-1 then then
    dup ok? not if pop continue then
    rot 1 + rot
  repeat pop
  dup not if
    pop "" "" exit
  then
  refduppurge refcompress dup
  0 swap begin
    dup while
    nextcompref dup 1
    0 begin 1 +
      dup RECURSIONLIMIT > if
        pop poprange 1 break
      then
      0 0 begin
        over 4 + pick over > while
        over 5 + pick "_prefs/mail/forward" getpropstr quietname2ref
        dup if
          dup 2 + rotate over 3 + rotate rot + swap
          over over + 5 + rotate pop
          over 4 + pick 1 - 3 pick 4 + put
        else
          pop 1 +
        then
      repeat pop
      dup not if
        pop pop dup 2 + rotate pop break
      then
      dup 2 + rotate over 3 + rotate rot + swap
$ifdef MAXFORWARD
      over MAXFORWARD > if
        pop poprange 1 break
      then
$endif
    repeat
    dup 2 + rotate over 3 + rotate rot + swap
  repeat pop
  refduppurge refcompress
;
 
: expand ( s -- s  Expands a compressed reflist to a list of names )
  "" begin
    over while
    swap nextcompref getname
    over 4 pick and if
      ", " swap strcat
    else
      3 pick if
        " and " swap strcat
    then then
    rot swap strcat
  repeat
  swap pop
;
 
: getkey2 ( d1 d2 -- s  Gets the key for mail from d1 to d2 )
  int swap int + dup lastkeynum @ = if
    pop lastkey @ exit
  then
  dup lastkeynum !
  "" lastkey !
  begin
    KEY dup strlen rot swap over over % 4 rotate swap strcut swap pop 1 strcut
    pop lastkey @ strcat lastkey ! 2 / / dup while
  repeat
  pop lastkey @
;
 
: getkey1 ( d1 d2 i -- s  Gets the key for mail from d1 to d2 with a length )
          (               of i, necessary due to a bug in encryption )
  rot rot getkey2
  begin over over strlen > while dup strcat
  repeat swap strcut pop
;
 
$ifdef __version<Muck2.2fb5.46 $undef ENCRYPTION $endif
 
: writeprop ( d1 d2 p s --  Writes a line s from d1 to d2 to prop p )
$ifdef ENCRYPTION
$ifdef __version<Muck2.2fb5.48
  4 rotate 4 pick 3 pick strlen getkey1 strencrypt "A" swap strcat
$else
  4 rotate 4 pick getkey2 strencrypt "B" swap strcat
$endif
$else
  4 rotate pop " " swap strcat
$endif
  setprop
;
 
: getmailprop ( d1 d2 p -- s  Reads a prop in message from d1 to d2 )
  over swap getpropstr dup not if rot rot pop pop exit then
  dup 1 strcut pop
  " AB" swap instr dup if
    dup 1 = if
      pop 1 strcut swap pop rot rot pop pop exit
    then
    dup 2 = if
      pop 1 strcut swap pop rot rot 3 pick strlen 2 - getkey1 strdecrypt exit
    then
    dup 3 = if
      pop 1 strcut swap pop rot rot getkey2 strdecrypt exit
    then
  then
  pop rot rot pop pop
;
 
: getpropref ( d p -- d  Reads a dbref prop )
  over swap getmailprop atoi dbref
;
 
: getpropint ( d p -- i  Reads an int prop )
  over swap getmailprop atoi
;
 
: writepropref ( d p d -- d  Writes a dbref prop )
  3 pick rot rot int intostr writeprop
;
 
: writepropint ( d p i -- d  Writes an int prop )
  3 pick rot rot intostr writeprop
;
 
: unreadcount ( d -- i  Returns number of unread messages )
  "@mail/newidx" getpropstr strlen 3 + 4 /
;
 
: savedcount ( d -- i  Returns number of read messages )
  "@mail/savedidx" getpropstr strlen 3 + 4 /
;
 
: indexcount ( i -- i  Returns how many messages user has in mode i )
  me @ swap if savedcount else unreadcount then
;
 
: writemesg ( *mesg* d1 d2 p --  Write a message *mesg* from player d1 to d2 )
            (                    to prop p )
  "#" strcat over over 6 pick intostr setprop "/" strcat
  0 5 rotate
  begin dup while
    swap 1 + swap 1 - 5 pick 5 pick 5 pick 5 pick intostr strcat
    4 pick 9 + rotate writeprop
  repeat pop pop 3 pick 3 pick 3 pick "subject" strcat 7 rotate writeprop
  over over "from" strcat 6 rotate writepropref
  "to" strcat 4 rotate writeprop
;
 
: writetrans ( *mesg* ? d1 d2 p -- *mesg* ?  Like writemesg but doesn't )
             (                               clobber *mesg*, and ignores )
             (                               the stack item after the message )
  "#" strcat over over 7 pick intostr setprop "/" strcat
  0 6 pick
  begin dup while
    swap 1 + swap 1 - 5 pick 5 pick 5 pick 5 pick intostr strcat
    4 pick 11 + pick writeprop
  repeat pop pop
  3 pick 3 pick 3 pick "subject" strcat 8 pick 9 + pick writeprop
  over over "from" strcat 7 pick 9 + pick writepropref
  "to" strcat 5 pick 8 + pick writeprop
;
 
: getmesg ( prop d -- *mesg* time  Reads a message in prop p on the user )
  swap "#" strcat over over "/from" strcat getpropref
  over "/to" strcat over 5 pick rot getmailprop -4 rotate
  rot rot 3 pick
  over "/subject" strcat over 5 pick rot getmailprop -4 rotate
  3 pick 3 pick getpropstr atoi rot "/" strcat rot 4 rotate 4 pick
  1 swap begin dup while
    5 pick 3 pick intostr strcat 5 pick 5 pick rot getmailprop
    -7 rotate 1 - swap 1 + swap
  repeat pop pop swap pop swap "time" strcat getpropint
;
 
: getmesgprop ( d num mode -- prop  Gets the prop for message num )
  rot swap if "savedidx" else "newidx" then "@mail/" dup rot strcat rot swap
  getpropstr rot 1 - 4 * strcut swap pop 4 strcut pop
  striptail dup if strcat else pop pop "" then
;
 
: reindex ( d num mode newmode --  Reindexes a message as new mode )
          (                        Here newmode 2 means deleted )
  dup 2 = if pop "deleted" else if "savedidx" else "newidx" then then
  "@mail/" swap strcat "@mail/" rot if "savedidx" else "newidx" then strcat
  4 pick over getpropstr 4 rotate 1 - 4 * strcut 4 strcut over not if
    pop pop pop pop pop pop exit then
  rot swap strcat 5 pick 4 rotate rot setprop
  3 pick 3 pick getpropstr "   " over strlen 3 bitand 4 swap - 3 bitand
  strcut pop strcat swap strcat setprop
;
 
: mytimestr ( i -- s  Returns a formatted time string )
  systime gmtoffset + over gmtoffset + - 86400 /
  dup not if
    pop "Today at"
  else 1 = if
    "Yesterday at"
  else
    "%a %b %e"
  then
  then over timefmt "%l:%M:%S%p" rot timefmt strip " " swap strcat strcat
;
 
: dumpmesg ( prop --  Dumps a message to the user )
  "#" strcat me @ over getpropstr atoi swap "/" strcat
  me @ over "from" strcat getpropref swap me @ over "time" strcat getpropint
  "From: " 4 pick getname strcat 40 strpad
  swap mytimestr strcat tellme
  over over me @ swap "to" strcat getmailprop expand
  "To  : " swap strcat tellme
  "Size: " 4 pick intostr strcat tellme
  over over me @ swap "subject" strcat getmailprop dup if
    "Subj: " swap strcat tellme
  else pop then
  " " tellme
  rot 0 begin over lcount @ -2 > and while
    1 + swap 1 - swap 4 pick me @ 5 pick 4 pick intostr strcat getmailprop
    tellme
  repeat pop pop pop pop
;
 
: quickdump ( prop --  Dumps a message in abbreviated format )
  "#" strcat me @ over getpropstr atoi swap "/" strcat
  me @ over "from" strcat getpropref swap me @ over "time" strcat getpropint
  3 pick getname " (" strcat swap mytimestr strcat ") -- " strcat
  3 pick me @ 4 pick "to" strcat getmailprop dup " " instr if
  "(to " swap expand strcat ")" strcat else pop "" then
  4 pick me @ 5 pick "subject" strcat getmailprop
  dup "Page-mail" strcmp over "Quick-mail" strcmp and not over not or
  7 pick 2 < and if
    pop 4 rotate me @ 5 rotate "1" strcat getmailprop swap strcat
    strcat tellme pop exit
  then
  swap strcat strcat tellme
  rot 0 begin over over > while 1 +
    4 pick me @ 5 pick 4 pick intostr strcat getmailprop tellme
  repeat pop pop pop pop
;
 
: delmesg ( d prop --  Deletes a message prop on d )
  "#" strcat remove_prop
;
 
: delmail ( d num mode --  Deletes a message and fill in the hole on d )
  3 pick dup 4 pick 4 pick getmesgprop delmesg 2 reindex
;
 
: markread ( prop --  Marks a mailprop as read )
  "#/read" strcat me @ over getpropstr if pop exit then
  me @ swap systime writepropint
;
 
: markdel ( prop --  Marks a mailprop as deleted )
  me @ swap "#/deleted" strcat "yes" setprop
;
 
: savemail ( num --  Saves a new mail )
  me @ over 0 getmesgprop me @ swap "#/read" strcat systime writepropint
  me @ swap 0 1 reindex
;
 
$ifndef CONVERT
$def checkmailbox
$else
: convertcall ( s d s {s} i i i --  Writes a message with the parameters )
              (                     of *mesg* sent read type )
  "sds{s}i3" checkargs
  if "@mail/savedidx" else "@mail/newidx" then me @ over getpropstr
  me @ "@mail/deleted" getpropstr dup if
    4 strcut me @ "@mail/deleted" rot setprop striptail
  else
    pop me @ dup savedcount swap unreadcount + intostr
  then
  swap dup strlen 3 bitand "   " 4 rot - 3 bitand strcut pop
  3 pick strcat strcat me @ 4 rotate rot setprop
  "@mail/" swap strcat me @ over "#/read" strcat 4 rotate dup if writepropint
  else pop pop pop then
  me @ over "#/time" strcat 4 rotate writepropint
  over 4 + pick me @ rot writemesg
;
 
: checkmailbox ( --  Makes sure player's mailbox is all converted )
  me @ "@mail/propver" getprop not if
    "Please wait, converting your mailbox..." tellme
    me @ 'convertcall CONVERT call
    me @ "@mail/propver" 1 setprop
  then
;
$endif
 
: getnewmesg ( d -- i  Initilizes a new message on <player> and returns the )
             (         number )
  dup "@mail/newidx" getpropstr dup strlen dup 4 / swap 3 bitand if 1 + then
  dup 512 >= if
    pop pop name "'s mailbox is full." strcat tellme 0 exit
  then
$ifdef INBOXSIZE
  3 pick "Truewizard" flag? not me @ "W" flag? not and if
    INBOXSIZE over <= if
      pop pop name "'s mailbox is full." strcat tellme 0 exit
    then
  then
$endif
  3 pick "@mail/deleted" getpropstr dup if
    4 strcut 5 pick "@mail/deleted" rot setprop striptail
  else
    pop 3 pick savedcount over + intostr
  then
  4 pick "@mail/" 3 pick strcat "#/time" strcat systime writepropint
  rot dup strlen 3 bitand "   " 4 rot - 3 bitand strcut pop strcat swap strcat
  rot "@mail/newidx" rot setprop 1 +
;
 
: deliver ( *mesg* --  Delivers a message )
  dup 4 + pick getrecpt swap expand "Sent to " swap strcat "." strcat tellme
  begin
    nextcompref
    dup "You sense you have new mail from " me @ name strcat "." strcat notify
    me @ swap dup getnewmesg dup if
      over swap 0 getmesgprop 4 pick if
        writetrans
      else
        4 rotate pop writemesg exit
      then
    else
      "Your mailbox is full, mail lost" notify
      pop dup not if
        pop poprange pop pop pop exit
      then
    then
  repeat
;
 
: editmail ( *mesg* i --  Edits and delivers a message from the user )
           (              i is the initial insertion point )
  "<      Entering message editor.  To send your message enter '.send'     >"
  tellme
  "<   To abort this message enter '.abort'  For further help enter '.h'   >"
  tellme
  "<      If you would like to say or pose, use \" to say or : to pose      >"
  tellme
  "< To start a line with a . \" or :, use a . before it as in .. .\" or .:  >"
  tellme
  me @ "@mail/postponed#" getpropstr not if
  "<     If you would like to postpone this message, enter '.postpone'     >"
    tellme
  then
  "end send abort postpone getlock" swap ".i"
  begin
    lines @ lcount !
    begin editor @ "editorparse" call while askme repeat
    tolower dup "getlock" strcmp if
      dup "postpone" strcmp not if
        me @ "@mail/postponed#" getpropstr if
          "< Only one message may be postponed >" tellme pop
        else
          "< Message postponed, use resume to continue it >" tellme
          pop pop pop pop pop pop
          dup 4 + pick "" begin over while
            swap nextcompref name " " strcat rot swap strcat
          repeat swap pop over 4 + put
          me @ dup "@mail/postponed" writemesg exit
        then
      else
        "abort" strcmp if
          pop pop pop
$ifdef MAILLIMIT
          me @ "W" flag? not if
$ifdef MAILLIMIT=LINE
            MAILLIMITCOUNT 4 pick < if
              "Unable to send, message exceeds line limit of "
              MAILLIMITCOUNT intostr strcat " lines" strcat tellme
              ".i" continue
            then
$else $ifdef MAILLIMIT=CHARACTER
            MAILLIMITCOUNT 0 5 pick begin dup while
              dup 6 + pick strlen rot + swap 1 -
            repeat pop < if
              "Unable to send, message exceeds size limit of "
              MAILLIMITCOUNT intostr strcat " characters" strcat
              tellme ".i" continue
            then
$else
$echo "Please define MAILLIMIT as either CHARACTER or LINE or undefine it"
$endif $endif
          then
$endif
          pop pop break
        else
        "Are you sure you want to abort, and lose the message?" tellme
        askme 1 strcut pop tolower "y" strcmp not if
          "< Message aborted >" tellme
          pop pop pop pop pop begin dup while 1 - swap pop repeat
          pop pop pop pop exit
        then
        "< Abort cancelled >" tellme
    then then then
    pop pop pop ""
  repeat
  deliver
;
 
: resume ( s --  Resume a postponed message )
  pop me @ "@mail/postponed#" getpropstr not if
    "No postponed message." tellme exit
  then
  "@mail/postponed" me @ getmesg me @ "@mail/postponed" delmesg pop
  dup 4 + pick name2ref dup not if
    "Nobody left to mail." tellme
    pop poprange pop pop pop exit
  then
$ifdef MULTIMAX
  me @ "W" flag? not if
    dup MULTIMAX > if
      "Messages must be limited to a maximum of " MULTIMAX
      intostr strcat " recipients." strcat tellme
      poprange poprange pop pop pop exit
    then
  then
$endif
  refcompress dup 3 pick 5 + put
  expand "Resuming message to " swap strcat " about " strcat
  over 3 + pick strcat tellme dup 1 + editmail
;
 
: my_pronoun_sub ( *mesg* time s -- *mesg time s  Pronoun sub with %t, %d )
                 (                                %j and %w added )
  "%%&" "%%" subst "%T" "%t" subst
  "%l:%M:%S%p" 3 pick timefmt "%T" subst
  "%D" "%d" subst "%a %b %e" 3 pick timefmt "%D" subst
  "%W" "%w" subst 3 pick 6 + pick expand "%W" subst
  "%J" "%j" subst 3 pick 4 + pick dup not if pop "something" then "%J" subst
  "%%" "%%&" subst 3 pick 5 + pick swap pronoun_sub
;
 
: reply ( p --  Reply to a message in prop p )
  me @ getmesg over if
    me @ "_prefs/mail/repwrap" getpropstr atoi dup if
      3 pick begin dup while
        over over 5 + pick swap wrapsplit dup if
          3 pick 5 + put -5 3 pick - rotate 4 rotate 1 + -4 rotate
        else pop pop 1 - then
      repeat pop
    then pop
    me @ "_prefs/mail/qpfx" getpropstr my_pronoun_sub
    0 swap
    begin over 5 pick < while
      over 5 + pick over swap strcat 3 pick 5 + put swap 1 + swap
    repeat pop pop
    me @ "_prefs/mail/qpre" getpropstr my_pronoun_sub
    3 pick -3 swap - rotate swap 1 + swap
    me @ "_prefs/mail/qpost" getpropstr my_pronoun_sub
    rot 1 + rot
  then pop
  dup if
    "Include original message in reply?" tellme askme
    tolower "y" 1 strncmp if
      poprange 0
    then
  then
  dup 4 + pick me @ int intostr strcmp if
    "Reply to all recipients?" tellme askme
    tolower "y" 1 strncmp not if
      dup 4 + pick " " swap over strcat strcat
      over 4 + pick int intostr " " swap over strcat strcat " " swap subst
      " " me @ int intostr " " swap over strcat strcat subst
      over 4 + pick int intostr strcat strip
    else dup 3 + pick int intostr then
  else dup 3 + pick int intostr then
  "" begin over while swap nextcompref rot swap
    dup ok? if dup player? not if pop #-1 then then
    dup ok? if
      dup ignored not if
        int intostr " " strcat strcat
      else pop then
    else pop then
  repeat swap pop dup not if
    "Nobody left to mail." tellme pop poprange pop pop pop exit
  then
$ifdef MULTIMAX
  strip me @ "W" flag? not if
    dup " " instrcount 1 + MULTIMAX > if
      "Messages must be limited to a maximum of " MULTIMAX
      intostr strcat " recipients." strcat tellme
      pop poprange pop pop pop exit
    then
  then
$endif
  over 4 + put
  dup 2 + pick dup "Re: " 4 strncmp if
    dup not if pop "Re: Your mail" else "Re: " swap strcat then
    over 2 + put
  else pop then
  me @ over 3 + put
  dup 1 + editmail
;
 
: forward ( p --  Forwards a message )
  "Who do you wish to forward to?" tellme askme name2ref
$ifdef MULTIMAX
  me @ "W" flag? not if
    dup MULTIMAX > if
      "Messages must be limited to a maximum of " MULTIMAX
      intostr strcat " recipients." strcat tellme
      poprange pop exit
    then
  then
$endif
  dup not if "Nobody to mail to." tellme pop pop exit then
  refcompress swap
  me @ getmesg "%a %b %e %T %Z" swap timefmt
  "----- Forwarded message -----" 3 pick -3 swap - rotate swap 1 + swap
  over 4 + pick getname "From: " swap strcat 40 strpad swap strcat
  over -1 swap - rotate dup 5 + rotate expand "To  : " swap strcat
  over -1 swap - rotate dup 4 + pick "Subj: " swap strcat
  over -1 swap - rotate " " over -1 swap - rotate 4 +
  dup 2 + pick "      " swap strcat dup dup strlen
  6 - strcut swap pop " (fwd)" strcmp if
    " (fwd)" strcat
  then 6 strcut swap pop over 2 + put me @ over 3 + put
  1 editmail
;
 
: index ( i --  Displays an index for mode i )
  runmode @ 1 = if
    " " tellme
  then
  dup if "@mail/savedidx" else "@mail/newidx" then
  me @ swap getpropstr dup not if
    pop if
      "No saved mail"
    else
      "No new mail"
    then tellme " " tellme exit
  then
  "@mail/" swap
  rot me @ "_prefs/mail/expert" getpropstr tolower "y" 1 strncmp
  swap if
    if
      "Saved mail: (-=deleted, *=expired, +=mailbox overflow)"
    else
      "Saved mail:"
    then
  else
    if
      "New mail: (-=deleted)"
    else
      "New mail:"
    then
  then tellme
  " #   Date   From             Lines Subject" tellme
  0 swap begin dup while
    4 strcut rot rot striptail swap 1 +
    4 pick rot strcat me @ over "#/from" strcat getpropref
    3 pick dup intostr swap 10 / dup not if swap " " strcat swap then
    10 / not if " " strcat then " " strcat me @ 4 pick "#/deleted" strcat
    getprop if
      "-"
    else
      me @ 4 pick "#/purged" strcat getprop if
        "+"
      else
        me @ 4 pick "#/expired" strcat getprop if
          "*"
        else
          " "
        then
      then
    then swap strcat
    me @ 4 pick "#/time" strcat getpropint "%b %e" swap timefmt strcat
    " " strcat over getname 17 strpad
    strcat me @ 4 pick "#" strcat getpropstr dup strlen "      " swap strcut
    swap pop strcat strcat
    swap me @ 4 rotate "#/subject" strcat getmailprop strcat
    dup strlen linelen @ swap over >= if
      4 - strcut pop "..." strcat
    else pop then tellme swap
  repeat pop pop pop
  runmode @ 1 = if
    " " tellme
  else
    "*Done*" tellme
  then
;
 
: index_eraseable ( d -- {i}  Displays an index of eraseable messages on )
                  (           d and returns a range of erasable message )
                  (           numbers )
  0 swap over over "@mail/newidx" getpropstr
  begin dup while swap 1 + swap
    4 strcut "@mail/" rot striptail strcat
    4 pick over "#/from" strcat getpropref
    me @ dbcmp not if
      pop continue
    then
    5 pick dup not if
      " #   Date   Lines Subject" tellme
    then
    1 + dup 6 put
    intostr " " swap strcat 5 strpad "%b %e " 6 pick
    4 pick "#/time" strcat getpropint timefmt strcat
    5 pick 3 pick "#" strcat getpropstr 6 strpad strcat
    me @ 6 pick 4 rotate "#/subject" strcat getmailprop strcat
    dup strlen linelen @ swap over >=
    if 4 - strcut pop "..." strcat else pop then
    tellme
    over 5 pick -4 swap - rotate
  repeat pop pop pop
;
 
: newmail ( s --  Reads a message from the user and delivers it )
  dup "=" instr dup if
    1 - strcut 1 strcut swap pop
  else
    pop ""
  then
  swap dup not if
    pop "Who do you want to send this mail to?" tellme askme
  then name2ref dup not if
    "Nobody to send to." tellme pop pop 0 exit
  then
$ifdef MULTIMAX
  me @ "W" flag? not if
    dup MULTIMAX > if
      "Messages must be limited to a maximum of " MULTIMAX
      intostr strcat " recipients." strcat tellme
      poprange pop 0 exit
    then
  then
$endif
  refduppurge refcompress
  me @ rot dup not if
    pop "What is the subject of this mail?" tellme askme
  then
  0 1 editmail
;
 
: tell_underline ( s --  Underlines s and displays it )
  shortver 40 strpad swap strcat tellme
"------------------------------------------------------------------------------"
  tellme
;
 
: masterFEEP ( --  Why don't you try it and find out? :)
  "You reach out for a FEEP and just as you almost have it, an old lynx "
  "steps in your way and says, \"Halt!  I am the keeper of the feep.  "
  "You may not pass here!\"  He mumbles some things and then continues, "
  "\"Oh yeah, You may not pass here without a challenge!  You must pass "
  "the challenge of MasterFEEP!\"" strcat strcat strcat strcat tellme
  "He sets out a game board with 12 lines of holes on it, and a hidden "
  "area.  He explains, \"To see the FEEP, you must guess my five colors.  "
  "Every time you guess, I will tell you if you have a correct color, a "
  "correct color in the wrong place, or the wrong color, but I won't tell "
  "you wich ones are which!  Each guess must be made on a seperate row on "
  "the board.  If you run out of room..\" He chuckles a little, sending "
  "shivers up your spine, \"Well..  Don't run out of room.  If however you "
  "are a coward, you may at any time 'quit'.  So, on with the challenge!\""
  strcat strcat strcat strcat strcat strcat strcat tellme
  "The keeper rummages around, then announces, \"I have made my selection!"
  "  Now it is your turn..  Guess!\"" strcat tellme
  "He hands you a small box, and surveying its contents you find colored "
  "pegs, 6 colors - Red, yellow, green, blue, white, and black."
  strcat tellme 12
  random dup 6 % swap 6 / dup 6 % swap 6 / dup 6 % swap 6 / dup 6 % swap 6 /
  6 % begin
    askme tolower dup "q" 1 strncmp not if
      7 poprange
      "The keeper shakes his fist at you and yells, \"Coward!\""
      tellme exit
    then
    " " explode dup 5 = not if poprange
      "The keeper looks at you oddly trying to figure out what color you "
      "are referring to." strcat tellme continue
    then
    pop "" 0 begin dup 5 < while 1 +
      "red yellow green blue white black" over 3 + pick cmdnum
      dup 0 = if
        "The keeper looks at you oddly trying to figure out what color you "
        "are referring to." strcat tellme
        8 poprange 1 break
      then
      "red white yellow blue green black" dup
      4 pick 5 + pick instr 1 - strcut swap pop dup " " instr dup if
      1 - strcut then pop 4 rotate 4 pick 5 < if ", " rot swap else "and "
      rot then strcat strcat rot rot 1 - over 2 + put
    repeat 1 = if continue then
    "You look over the colored pegs, selecting 5, you place down " swap
    ".  You look up at the keeper to see what he does." strcat strcat tellme
    0 5 begin dup while dup while 1 -
      12 pick 8 pick over = if pop -1 -2 8 put rot 1 + else rot then rot
    repeat 5 begin dup while 1 -
      5 begin dup while 1 - 
        over over + 6 + pick 6 pick = if
          -2 over 4 pick + 6 + put rot 1 + rot rot break
        then
      repeat pop 4 rotate pop
    repeat pop over 5 < while
    dup if
      over if
        "Nodding quickly, the keeper marks " rot intostr strcat
        " of them as in the right place and " strcat swap intostr strcat
        " as in the wrong place." strcat tellme
      else
        "The keeper watches the colors you place.  After a brief pause "
        "he marks " strcat swap intostr strcat " as the correct color, but "
        "in the wrong place." strcat strcat tellme pop
    then else
      pop dup if
        "As you slip the colored pegs into place, the keeper eyes them "
        "and marks " strcat swap intostr strcat " as in the right place."
        strcat tellme
      else
        pop "Your heart sinks as the keeper chuckles "
        "briefly and doesn't mark any of them as correct." strcat tellme
    then then 5 poprange
    6 rotate 1 - dup not if 1 break then -6 rotate
  repeat if
    6 poprange "The keeper starts with a soft chuckle, then "
    "starts laughing loudly.  He commands, \"You are nothing to the FEEP!  "
    "You do not deserve to see the FEEP!  Off with you!\"  As you hurry "
    "away, his laughs echo through the cave of the FEEP."
  else
    12 poprange "The keeper stands up suddenly and exclaims, \"Impossible!  "
    "Nobody can defeat the keeper of the FEEP!\"  He searches for words, "
    "then just motions you to follow him.  Weakly he says, \"You must tell "
    "none of what you see here.\"  He leads you into the cave of the FEEP, "
    "and shows you a ball of fluff with adhesive feet, glued on eyes, a "
    "elastic wristband, and a little tag that reads FEEP." strcat strcat
  then strcat strcat strcat tellme
;
 
: do_help ( s --  Displays help )
  pop runmode @ 1 = if "" else command @ " #" strcat then
  "Main help" tell_underline
  runmode @ 1 = not if
  command @ 40 strpad "Enter the interactive mailer" strcat tellme
  then
  dup "Mail [<players>][=subject]" strcat 40 strpad "Send mail" strcat tellme
  dup "Resume" strcat 40 strpad "Resume a .postponed message" strcat tellme
  runmode @ 1 = if
  "<number>" 40 strpad "Read a message" strcat tellme
  else
  dup "Read [new/saved] <number>" strcat 40 strpad "Read a message" strcat
  tellme
  then
  "Delete [new/saved] <number>" 40 strpad "Delete a message" strcat tellme
  dup "Index [new/saved]" strcat 40 strpad "View an index of messages"
  strcat tellme
  runmode @ 1 = if
  "Saved" 40 strpad "Toggle to saved messages" strcat tellme
  "New" 40 strpad "Toggle to new messages" strcat tellme
  then
  dup "Check [<players>]" strcat 40 strpad
  "Check a recipient for unread messages" strcat tellme
  dup "Unmail [<player>]" strcat 40 strpad
  "Erase or edit a message to someone else" strcat tellme
  dup "Options [option value]" strcat 40 strpad "Set options" strcat tellme
  dup "Version" strcat 40 strpad "Display version number" strcat tellme
  dup "Changes" strcat 40 strpad "Display changes log" strcat tellme
  runmode @ 1 = if
    "At any prompt you may say with \" or pose with :" tellme
  else
    "For help on quick-mail commands, type qmail #help" tellme
  then
  pop
;
 
: quick_help ( s --  Displays help for quick mail )
  pop "Quick-mail help" tell_underline
  "Qmail Players=Message" 40 strpad "Send a quick 1-line mail" strcat tellme
  "Qmail [new/saved/all]" 40 strpad "Read mail, defaults to new" strcat tellme
  "Qmail #Scan [new/saved]" 40 strpad "Display a list of new or saved mail"
  strcat tellme
  "Qmail #Erase Players" 40 strpad "Erase last mail to players" strcat tellme
  "Qmail #Forward" 40 strpad "View forwarding of incoming mail" strcat tellme
  "Qmail #Forward Players" 40 strpad
  "Set forwarding of incoming mail" strcat tellme
  "Qmail #Forward #" 40 strpad "Stop forwarding mail" strcat tellme
  "Qmail #Check Players" 40 strpad "Check for new mail on players"
  strcat tellme
  "Qmail #Full" 40 strpad "Display quick-mail with full headers" strcat tellme
  "Qmail #Abbreviated" 40 strpad "Display quick-mail with short header"
  strcat tellme
  "Qmail #Prompt" 40 strpad "Prompt after each message" strcat tellme
  "Qmail #Delete" 40 strpad "Don't prompt and delete mail" strcat tellme
  "Qmail #Save" 40 strpad "Don't prompt and save mail" strcat tellme
;
 
: read_help ( i --  Display read help, i<2 for regular read prompt, or )
            (       >1 for quick-read )
  dup 2 < if "Read help" else "Quick-read help" then tell_underline
  "Again" 40 strpad "Read the message again" strcat tellme
  dup 3 = not if
    "Next" 40 strpad over if "Read the next message" else
      "Save the message and read the next" then strcat tellme
  then
  dup 1 bitand not if "Save" 40 strpad "Save the message" strcat tellme then
  "Reply" 40 strpad "Send a response to the sender" strcat tellme
  "Forward" 40 strpad "Send the message to someone else" strcat tellme
  "Delete" 40 strpad "Delete the message" strcat tellme
  dup 1 = if "Back" 40 strpad "Back out of the read" strcat tellme then
  dup 2 = if "Quit" 40 strpad "Save current message and quit" strcat tellme
  then dup 3 = if "Quit" 40 strpad "Quit reading messages" strcat tellme then
  dup 2 < if
    "Undelete" 40 strpad "Undelete current message" strcat tellme
  then
  pop
;
 
: option_help ( --  Display help for the options menu )
  "Option help" tell_underline
  "Menus [verbose/terse]" 40 strpad "Toggle between terse and full menus"
  strcat tellme
  "Wrap [length/no]" 40 strpad "Set wrap line length or turn wrap off"
  strcat tellme
  "Paging [length/no]" 40 strpad "Set page length or turn paging off"
  strcat tellme
  "Forwarding [person(s)]" 40 strpad "Set automatic message forwarding"
  strcat tellme
  "Prepend [string]" 40 strpad "Set the line shown before quoted text"
  strcat tellme
  "Prefix [string]" 40 strpad "Set the string to mark quoted text"
  strcat tellme
  "Postpend [string]" 40 strpad "Set the line shown after quoted text"
  strcat tellme
  "Quote [length/no]" 40 strpad "Set wrap length for quoted text"
  strcat tellme
  "Display [full/abbreviated]" 40 strpad "Set quick-mail header display"
  strcat tellme
  "Prompt [yes/save/delete]" 40 strpad "Enable prompting or set action"
  strcat tellme
  "Settings" 40 strpad "Display current settings" strcat tellme
  "Back" 40 strpad "Back out of options menu" strcat tellme
  " " tellme
  "Any string shown with quoting may use regular pronoun substitutions as well"
  tellme
  "as %t for the time the message was sent, %d for the date, and %j for the"
  tellme
  "subject, and %w for who the message was to"
  tellme
;
 
: do_changes ( -- )
  "Changes" tell_underline
  "v1.1   7/ 2/1996  Fixed assorted bugs and punctuation, and made qmail" tellme
  "                   setting props make sense.  Added #version and #changes." tellme
  "v1.0   7/ 1/1996  Initial release." tellme
  "v0.9   3/24/1996  Initial beta release." tellme
;
 
: do_display ( num mode --  Displays a message )
  getmylock not if
    pop pop exit
  then
  swap over indexcount
  over < over 1 < or if
    "Invalid message number." tellme pop me @ clearlock exit
  then
  me @ over 4 pick getmesgprop dup dumpmesg
  begin
    lines @ lcount !
    " " tellme 3 pick if
      "Saved mail" "A>gain N>ext R>eply F>orward D>elete B>ack" "ANRFDB"
    else
      "New mail" "A>gain N>ext&save S>ave R>eply F>orward D>elete"
      "ANSRFD"
    then me @ 5 pick "#/deleted" strcat getprop if
      swap " U>ndelete" strcat swap "U" strcat " undelete" else ""
    then -4 rotate showmenu askme " " tellme
    "save next again delete back reply forward quit help" rot strcat swap
    cmdnum 4 pick if
      dup 5 = over 8 = or if pop break then
    else
      dup 1 = over 8 = or if
        pop over savemail "Message saved." tellme break
      then
    then
    dup 2 = if
      pop pop over not if
        dup savemail 1 -
      then
      over indexcount over <= if
        over if
          "Last message."
        else
          "Last message saved."
        then
        tellme 0 break
      then
      over if
        "Message saved." tellme
      then " " tellme
      1 + me @ over 4 pick getmesgprop dup dumpmesg continue
    then
    dup 3 = if pop dup dumpmesg continue then
    dup 4 = if
      pop markdel over not if dup savemail 1 - then
      over indexcount over <= if
        "Last message deleted." tellme 0 break
      then
      " " "Message deleted." tellme tellme
      1 + me @ over 4 pick getmesgprop dup
      dumpmesg continue
    then
    dup 6 = if pop dup reply continue then
    dup 7 = if pop dup forward continue then
    dup 9 = if pop 3 pick read_help continue then
    dup 10 = if pop me @ over "#/deleted" strcat remove_prop
      "Deletion mark removed." tellme continue then
    pop "Unknown command, try <H>elp." tellme
  repeat
  pop pop pop me @ clearlock
;
 
: parse2nums ( i s -- i i  Parses for [new/saved] num )
  tolower strip dup " " instr dup if
    strcut striplead swap striptail swap
  else pop "" then
  dup number? over and not if swap then
  dup number? over and not if
    pop pop pop -1 exit
  then atoi swap dup if
    dup strlen over "new" 3 pick strncmp not if
      pop pop 0
    else
      over "saved" 3 pick strncmp not if
        pop pop 1
      else
        pop pop pop pop -1 exit
      then
    then
    rot pop
  else
    pop swap
  then swap dup 0 <= if
    pop pop -1
  then
;
 
: do_read ( s --  Read a message )
  parse2nums dup 0 < if
    "I don't know what message you want to read." tellme pop exit
  then
  swap do_display
;
 
: do_check ( s --  Check for mail )
  strip dup not if
    pop "Who's mail would you like to check?" tellme
    askme
  then
  name2ref dup not if pop exit then
  begin dup while 1 - swap
    dup name " has " strcat swap unreadcount dup not
    if "no" else dup intostr then " unread " strcat
    swap 1 = if "message" else "messages" then
    strcat strcat " waiting." strcat tellme
  repeat pop
;
 
: do_index ( i s --  Displays an index )
  dup not if pop index exit then
  swap pop dup strlen swap
  dup "new" 4 pick strncmp not if
    pop pop 0
  else
    dup "saved" 4 pick strncmp not if
      pop pop 1
    else
      "I don't know what you want indexed." tellme pop pop exit
    then
  then
  index
;
 
: do_erase2 ( d --  Erases messages to a player )
  "Checking " over name strcat " for messages from you..." strcat tellme
  " " tellme
  dup index_eraseable
  dup not if
    "No messages found." tellme pop pop exit
  then
  " " tellme
  begin
    "Which message would you like to unmail?" tellme
    askme2 "quit abort .getlock" over cmdnum dup 3 = if
      3 pick 4 + pick name " accessing mailbox, aborting." strcat tellme
    else
      dup if "Aborted." tellme then
    then
    if
      pop begin dup while 1 - swap pop repeat pop pop exit
    then
    atoi over over < over 1 < or if "Invalid number." tellme pop continue then
  break repeat
  1 + pick swap
  begin dup while 1 - rot pop repeat pop
  "Unmail message to " 3 pick name " about " strcat strcat
  me @ 4 pick over 5 pick 0 getmesgprop "#/subject" strcat
  getmailprop strcat tellme
  1 begin not if "Invalid choice." tellme then
    "[Yes/No/Edit]" tellme askme2
    "no quit abort yes edit .getlock" swap cmdnum
  dup until
  dup 4 < if
    "Message not unmailed." tellme pop pop pop exit
  then
  dup 6 = if
    pop pop name " accessing mailbox, aborting." strcat tellme exit
  then
  dup 4 = if
    pop 0 delmail "Message unmailed." tellme exit
  then
  dup 5 = if
    pop over over 0 getmesgprop 3 pick getmesg
    pop dup 6 + pick int intostr over 4 + put
    dup 6 + rotate dup 3 pick 7 + rotate 0 delmail clearlock
    dup 1 + editmail exit
  then
  pop pop pop "Internal error." tellme
;
 
: do_erase ( s --  Handles locks for erase )
  strip dup not if
    pop "Unmail a message you sent to whom?" tellme askme
  then
  "*" swap strcat match dup not if pop "Unknown player." tellme exit then
  dup claimlock if
    "Unable to lock " swap name "'s mailbox." strcat strcat tellme exit
  then
  dup do_erase2 clearlock
;
 
: set_yesno_option ( input inverse prop prompt yesstr nostr ynalias --  )
  "yes no " swap strcat striptail dup 8 rotate cmdnum 5 rotate over if
    pop swap pop
  else
    tellme pop begin dup askme cmdnum dup not while
      pop "[Yes/No]" tellme
    repeat swap pop
  then
  1 bitand dup if swap else rot then pop swap tellme
  rot bitxor if
    me @ swap "yes" setprop
  else
    me @ swap remove_prop
  then
;
 
: set_nonum_option ( input prop prompt1 prompt2 enpre enpost dis --  )
  7 rotate strip dup dup number? "yes no" rot cmdnum or 7 rotate swap if
  pop else
    tellme pop begin
      askme strip dup dup number? "yes no" rot cmdnum or not while
      pop "[Yes/No]" tellme
    repeat
  then
  dup tolower "y" 1 strncmp 6 rotate swap if
  pop else
    tellme pop begin askme strip dup number? over atoi 2 > and not while
      pop "Number greater then 2 expected." tellme
    repeat
  then
  dup number? if
    swap pop rot over strcat rot strcat
    swap rot me @ swap rot setprop
  else
    pop swap pop swap pop me @ rot remove_prop
  then tellme
;
 
: set_nostring_option ( input prop prompt1 prompt2 en dis ynalias --  )
  "yes no " swap strcat striptail 7 rotate 6 rotate over if
  pop else
    tellme pop begin askme over over cmdnum not while
      "[Yes/No]" tellme
    repeat
  then
  swap over cmdnum 5 rotate over if
    swap 1 bitand if
      tellme pop askmeraw
    else pop pop "" then
  else pop pop then
  dup if
    swap pop swap over strcat
  else
    rot pop swap
  then
  tellme me @ rot rot setprop
;
 
: yes_or_no ( s1 s2 s3 -- i  Returns 1 on yes, using s3 as aliases for )
            (                yes/no and s2 as a prompt )
  "yes no " swap strcat dup 4 rotate tolower cmdnum dup not if
    pop swap tellme
    begin
      dup askme tolower cmdnum dup not while
      pop "[Yes/No]" tellme
    repeat
  else
    swap pop
  then
  swap pop 1 bitand
;
 
: setoption ( s --  Sets a menu option )
  striplead dup " " instr dup if
    strcut striplead swap striptail swap
  else
    pop ""
  then
  "wrap menus paging forward prepend prefix postpend quote display prompt"
  rot cmdnum dup not if
    pop pop "Unknown option." tellme
  else
    dup 1 = if
      pop "_prefs/mail/wrap" "Enable word-wrap?"
      "How many characters per line?" "Word-wrap enabled at " " characters."
      "Word-wrap disabled." set_nonum_option
      initwrap exit
    then
    dup 2 = if
      pop 1 "_prefs/mail/expert" "Verbose menus?" "Menus will be verbose."
      "Menus will be terse." "verbose terse" set_yesno_option exit
    then
    dup 3 = if
      pop "_prefs/mail/paging" "Enable paging?" "How many lines per page?"
      "Paging enabled at " " lines." "Paging disabled." set_nonum_option
      initpaging exit
    then
    dup 4 = if
      pop "_prefs/mail/forward" "Forward incoming mail?"
      "Who would you like to forward mail to?" "Mail will be forwarded to "
      "Mail will not be forwarded." "on off" set_nostring_option exit
    then
    dup 5 = if
      pop "_prefs/mail/qpre" "Prepend a string before quoted text?"
      "What would you like to prepend quoted text with?"
      "Quot prepend set to " "Quote prepend cleared" "on off"
      set_nostring_option exit
    then
    dup 6 = if
      pop "_prefs/mail/qpfx" "Prefix quoted text?"
      "What would you like to prefix quoted text with?"
      "Quot prefix set to " "Quote prefix cleared." "on off"
      set_nostring_option exit
    then
    dup 7 = if
      pop "_prefs/mail/qpost" "Postpend a string after quoted text?"
      "What would you like to postpend quoted text with?"
      "Quot postpend set to " "Quote postpend cleared." "on off"
      set_nostring_option exit
    then
    dup 8 = if
      pop "_prefs/mail/repwrap" "Automatically wrap quoted text?"
      "Wrap after how many characters? (Not including the prefix)"
      "Quoted text will be wrapped at " " characters."
      "Quoted text will not be wrapped." set_nonum_option exit
    then
    dup 9 = if
      pop "Display abbreviated headers?" "abbreviated full" yes_or_no if
        "Quick-mail headers will be abbreviated."
        me @ "_prefs/mail/qheader" remove_prop
      else
        "Quick-mail headers will be full."
        me @ "_prefs/mail/qheader" "full" setprop
      then tellme
    then
    dup 10 = if
      pop "delete save prompt" swap cmdnum
      dup not if
        pop "" "Prompt after quick-mail messages?" "prompt" yes_or_no
        if 3 else
          "" "Save messages after reading?" "save delete" yes_or_no
          1 +
        then
      then
      dup 3 = if
        "Quick-mail messages will be prompted for."
        me @ "_prefs/mail/qreadmode" "prompt" setprop
      else dup 2 = if
        "Quick-mail messages will be saved without asking."
        me @ "_prefs/mail/qreadmode" "save" setprop
      else dup 1 = if
        "Quick-mail messages will be deleted without asking."
        me @ "_prefs/mail/qreadmode" remove_prop
      else pop ""
      then then then tellme exit
    then
    pop pop
  then
;
 
: display_options ( --  Displays options menu )
  " " tellme
  me @ "_prefs/mail/expert" getpropstr if
  "<M>enus are terse" else "<M>enus are verbose" then tellme
  wraplen @ if
  "Word <W>rapping is enabled after column " wraplen @ intostr strcat
  else "<W>ord wrapping is disabled" then tellme
  lines @ if
  "<PA>ging is enabled at " lines @ intostr strcat " lines per page" strcat
  else "<PA>ging is disabled" then tellme
  me @ "_prefs/mail/forward" getpropstr dup if
    "<F>orwarding of recieved mail is enabled and sent to " swap strcat
  else pop "<F>orwarding of recieved mail is disabled"
  then tellme
  me @ "_prefs/mail/qpre" getpropstr dup if
    "Quoted text will be <PREP>ended with " swap strcat
  else
    pop "Quoted text will not be <PREP>ended"
  then tellme
  me @ "_prefs/mail/qpfx" getpropstr dup if
    "Quoted text will be <PREF>ixed with " swap strcat
  else
    "Quoted text will not be <PREF>ixed"
  then tellme
  me @ "_prefs/mail/qpost" getpropstr dup if
    "Quoted text will be <PO>stpended with " swap strcat
  else
    "Quoted text will not be <PO>stpended"
  then tellme
  me @ "_prefs/mail/repwrap" getpropstr dup if
    "<Q>uoted text will be wrapped to " swap strcat " characters"
    strcat
  else
    pop "<Q>uoted text will not be wrapped"
  then tellme
  getqflags dup qheadermask bitand if
    "Quick-mail will <D>isplay messagws with a full header"
  else
    "Quick-mail will <D>isplay messages with an abbreviated header"
  then tellme
  dup qpromptmask bitand if
    pop "Quick-mail will <PRO>mpt for action with each message"
  else
    "Quick-mail will not <PRO>mpt and will "
    swap qsavemask bitand if
      "save messages"
    else
      "delete messages"
    then strcat
  then tellme
  " " tellme
;
 
: do_options ( s --  Options menu )
  dup if setoption exit then pop display_options
  begin
    "Options" "S>ettings B>ack" "SB" showmenu askme
    dup dup " " instr dup if strcut pop strip else pop then
    "menus wrap paging forward prepend prefix postpend quote display prompt"
    " settings help back" strcat
    swap cmdnum dup 12 > if
      pop pop exit
    then
    dup 11 = if pop pop display_options continue then
    dup 12 = if pop pop " " tellme option_help " " tellme continue then
    " " tellme
    if setoption else pop "Unknown command, try <H>elp." tellme then
    " " tellme
  repeat
;
 
: do_delete ( i s --  Marks a message as deleted )
  striplead dup not if
    pop "Which message would you like to delete?" tellme
    runmode @ 1 = if
      askme
    else
      pop exit
    then
  then
  parse2nums dup 0 <= if
    pop "Invalid message number." tellme exit
  then
  over indexcount over < if
    "Invalid message number." tellme pop pop exit
  then
  me @ swap rot getmesgprop "#/deleted" strcat me @ over getprop if
    "Message already marked as deleted." tellme pop exit
  then
  me @ swap "yes" setprop "Message marked as deleted." tellme
;
 
: do_command ( i s -- i  Processes a menu or command argument and returns )
            (            false if it was an invalid command )
  strip dup not if pop 0 exit then
  dup " " instr dup if strcut else pop "" then
  swap striptail
"resume mail index check erase unmail options help delete version changes feep "
  runmode @ 1 > if
    "read" strcat
  then swap cmdnum
  dup 1 = if pop resume pop 1 exit then
  dup 2 = if pop newmail pop 1 exit then
  dup 3 = if pop do_index 2 exit then
  dup 4 = if pop do_check pop 1 exit then
  dup 5 = over 6 = or if pop do_erase pop 1 exit then
  dup 7 = if pop do_options pop 1 exit then
  dup 8 = if pop do_help pop 1 exit then
  dup 9 = if pop do_delete 1 exit then
  dup 10 = if pop pop pop VERSION tellme 1 exit then
  dup 11 = if pop pop pop do_changes 1 exit then
  dup 13 = if
    runmode @ 1 > if
      pop do_read 1 exit
    then
  then
  if masterFEEP pop pop 1 else pop pop 0 then
;
 
: do_menu ( --  Displays a menu )
  me @ unreadcount if 0 else
    me @ savedcount if 1 "No new mail, switching to saved." tellme
    else 0
  then then
  begin
    dup index
    begin
      "Mail"
      over if
        "#> D>elete I>ndex C>heck M>ail R>esume U>nmail N>ew O>ptions Q>uit"
        "#ICMRFUNOQ"
      else
        "#> D>elete I>ndex C>heck M>ail R>esume U>nmail S>aved O>ptions Q>uit"
        "#ICMRUSOQ"
      then showmenu askme
      dup strip if
        "saved new" over cmdnum 1 - 3 pick = if
          pop not break
        then
        dup tolower dup strlen "quit" swap strncmp not if
          pop pop exit
        then
        strip dup not if
          pop "Unknown command, try <H>elp." tellme continue
        then
        dup number? if
          atoi over do_display break
        then
        over swap do_command dup 2 = if pop continue then not while
      else pop then
      "Unknown command, try <H>elp." tellme
    repeat
  repeat pop
;
 
: quick_display ( i1 i2 i3 --  Displays a range of messages from i1 to i2 )
                (              In mode i3 )
  dup not if
    getmylock not if
      "Skipping mail check." tellme
      pop pop pop exit
    then
  then
  over not if
    if "No saved mail." else "No new mail." me @ clearlock then
    tellme pop pop exit
  then
  rot rot begin over over <= while
    me @ 3 pick 5 pick getmesgprop
    dup runmode @ qheadermask bitand if dumpmesg else quickdump then
    begin
      runmode @ qpromptmask bitand if
        lines @ lcount ! " " tellme
        4 pick dup if
          "Saved mail" "A>gain N>ext R>eply F>orward D>elete Q>uit"
          "ANRFDQ"
        else
          "New mail" "A>gain S>ave R>eply F>orward D>elete Q>uit&save"
          "ASRFDQ"
        then showmenu askme " " tellme
        "save again delete back reply forward quit next help" swap cmdnum
        swap if
          dup 8 = if pop pop swap 1 + swap break then
          dup 7 = if pop pop pop pop not if me @ clearlock then exit then
        else
          dup 1 = over 7 = or if
            "Message saved." tellme
            4 pick savemail
            1 = if pop 1 - break then
            pop pop pop not if me @ clearlock then exit
          then
        then
        dup 2 = if pop
          dup runmode @ qheadermask bitand if dumpmesg else quickdump then
          continue
        then
        dup 3 = if
          pop me @ 4 pick 6 pick delmail "Message deleted." tellme
          pop 1 - break
        then
        dup 5 = if pop dup reply continue then
        dup 6 = if pop dup forward continue then
        dup 9 = if pop 4 pick 2 + read_help continue then
        pop "Unknown command, try <H>elp." tellme
      else
        lcount @ -2 = if
          pop pop pop not if me @ clearlock then exit
        then
        pop runmode @ qsavemask bitand if
          4 pick if
            swap 1 + swap
          else
            1 - over savemail
          then
        else
          1 - me @ 3 pick 5 pick delmail
        then break
      then
    repeat
  repeat
  pop pop not if me @ clearlock then
;
 
: quick_mail ( s s --  Quick-mail a user )
  dup "=" instr dup not if
    pop pop "Usage: " swap strcat " Player(s)=Message" strcat tellme exit
  then
  1 - strcut 1 strcut swap pop striplead swap striptail
  over not if
    pop pop "Usage: " swap strcat " Player(s)=Message" strcat tellme exit
  then
  swap dup ":" stringpfx if
    1 strcut swap pop ". ,':!?-" over 1 strcut pop instr not if
      " " swap strcat
    then me @ name swap strcat
  then swap
  name2ref dup not if
    pop pop pop "Nobody to send mail to" tellme exit
  then
$ifdef MULTIMAX
  me @ "W" flag? not if
    dup MULTIMAX > if
      "Messages must be limited to a maximum of " MULTIMAX
      intostr strcat " recipients" strcat tellme
      poprange pop 0 exit
    then
  then
$endif
  refcompress dup getrecpt 4 rotate me @ swap "Quick-mail" swap 1 6 rotate
  expand "You quick-mail \"" 4 pick strcat "\" to " strcat swap strcat tellme
  5 rotate
  begin
    nextcompref dup "You sense you have new mail from "
    me @ name strcat "." strcat notify me @ swap dup getnewmesg
    dup if
      over swap 0 getmesgprop 4 pick if
        writetrans
      else
        4 rotate pop writemesg break
      then
    else
      pop "Your mailbox is full, mail lost." notify
      pop dup not if
        pop poprange pop pop pop break
      then
    then
  repeat
  pop
;
 
: quick_read ( s1 s2 --  Reads mail )
  strip dup not if pop "new" else then
  dup number? if "new " swap strcat then
  dup " " instr dup if
    strcut striplead swap striptail
  else
    pop "" swap
  then
  "new saved all" swap cmdnum dup not if
    pop pop "Usage: " swap strcat " [new/saved/all] [#]" strcat tellme exit
  then
  over over 3 = and if
    pop pop pop
    "You may not specify a specific message when reading all mail." tellme
    exit
  then
  0 rot rot
  over if
    1 - swap dup number? not if
      pop pop pop pop "Invalid message number." tellme exit
    then
    atoi over me @ swap if savedcount else unreadcount then
    over < over 1 < or if
      pop pop pop pop "Invalid message number." tellme exit
    then dup rot 1
  else
    swap pop
    dup 2 bitand if
      1 me @ savedcount 1 1 5 rotate
    then
    dup 1 bitand if
      1 me @ unreadcount 0 1 5 rotate
    then
    pop
  then
  begin
    dup pop while quick_display
  repeat pop
  "*Done*" tellme
;
 
: quick_erase ( s1 s2 --  Erase last message to a user )
  dup not if
    pop "Usage: " swap strcat " Player(s)" strcat tellme exit
  then
  name2ref dup not if
    pop pop "Nobody to erase messages to." tellme exit
  then
  begin dup while
    1 - swap dup claimlock if
      "Unable to access " swap name "'s mailbox, try again later."
      strcat strcat tellme continue
    then
    dup unreadcount begin dup while
      over dup 3 pick 0 getmesgprop "#/from" strcat getpropref
      me @ dbcmp if
        break
      then 1 -
    repeat
    dup if
      over swap 0 delmail
      "Last message to " over name " erased." strcat strcat tellme
    else
      pop dup name " didn't have any mail from you." strcat tellme
    then
    clearlock
  repeat pop pop
;
 
: quick_forward ( s1 s2 --  Sets mail forwarding )
  strip dup not if
    pop me @ "_prefs/mail/forward" getpropstr dup if
      "Recieved mail is currently forwarded to " swap strcat tellme
    else
      "Recieved mail is not currently forwarded." tellme
    then
  else
    dup "#" 1 strncmp not if
      pop me @ "_prefs/mail/forward" remove_prop
      "Mail forwarding cleared." tellme
    else
      "Mail will be forwarded to " over strcat tellme
      me @ "_prefs/mail/forward" rot setprop
    then
  then
  pop
;
 
: quick_check ( s1 s2 --  Checks player for mail )
  dup if
    swap pop do_check
  else
    pop "Usage: " swap strcat " <players>" strcat tellme
  then
;
 
: qmail ( s1 s2 i --  Call qmail function i with s1 as the base command and )
        (             s2 as the parameters )
  dup 1 = if pop quick_mail exit then
  dup 2 = if pop quick_read exit then
  dup 3 = if pop quick_erase exit then
  dup 4 = if pop quick_forward exit then
  dup 5 = if pop quick_check exit then
  dup 6 = if pop 0 swap do_index exit then
  pop pop pop
;
 
: qparse
  strip dup "#" 1 strncmp if
    command @ swap dup "=" instr if
      1
    else
      2
    then
  else
    1 strcut swap pop dup " " instr dup if
      strcut strip swap strip
    else pop "" swap then
    command @ " #" strcat over strcat rot rot
"mail read erase forward check scan help full abbreviated prompt delete save er"
    swap cmdnum
  then
  dup not if
    pop "Unknown quick-mail command.  Try " command @ " #help"
    strcat strcat me @ swap notify
  else
    dup 7 < if
      qmail
    else
      dup 7 = if quick_help exit then
      dup 8 = if
        pop me @ "_prefs/mail/qheader" "full" setprop
        "Messages will be displayed with a full header." tellme exit
      then
      dup 9 = if
        pop me @ "_prefs/mail/qheader" remove_prop
        "Messages will be displayed with an abbreviared header." tellme exit
      then
      dup 10 = if
        pop me @ "_prefs/mail/qreadmode" "prompt" setprop
        "A prompt will be displayed after each message." tellme exit
      then
      dup 11 = if
        pop me @ "_prefs/mail/qreadmode" remove_prop
        "Messages will be deleted without asking." tellme exit
      then
      dup 12 = if
        pop me @ "_prefs/mail/qreadmode" "save" setprop
        "Messages will be saved without asking." tellme exit
      then
      dup 12 > if
        pop pop exit
      then
    then
  then
;
 
: deinit ( --  Deinitializes stuff )
  0 0 me @ unreadcount 1 1 me @ savedcount begin
    dup not if
      pop while pop continue
    then
    me @ over 4 pick getmesgprop "#/deleted" strcat me @ swap getpropstr if
      3 pick 3 pick or not if
        1 3 put me @ claimlock if
          pop pop break
        then
      then
      me @ over 4 pick delmail
    then 1 -
  repeat pop me @ clearlock
$ifdef MAXSAVED
  me @ "TrueWizard" flag? not if
    me @ savedcount 0 swap dup 0 0 rot begin dup while
      me @ over 1 getmesgprop "#/" strcat
      me @ over "expired" strcat getprop if
        rot 1 + rot rot pop
      else
        me @ swap "purged" strcat getprop if
          rot 1 + rot rot
        then
      then 1 -
    repeat pop
    over over + 4 pick swap - MAXSAVED - dup 0 > if
      rot over + rot rot 1 begin
        over while
        me @ over 1 getmesgprop "#/" strcat
        me @ over "purged" strcat getprop me @ 3 pick "expired" strcat
        getprop or not if
          me @ swap "purged" strcat systime writepropint swap 1 - swap
          1 6 put
        else pop then 1 +
      repeat pop pop
    else
      dup 0 < 4 pick and if
        0 swap - dup 4 pick < if
          rot over - rot rot 3 pick swap
        else
          pop 0 swap rot 0 swap
        then
        1 begin over while
          me @ over 1 getmesgprop "#/purged" strcat
          me @ over getprop if
            4 pick if
              pop rot 1 - rot rot
            else
              me @ swap remove_prop swap 1 - swap
            then
          else pop then 1 +
        repeat pop pop
      else pop then
    then
    pop swap pop dup rot and if
      "Your saved mailbox is full, if you don't clean out " swap intostr
      strcat " messages, they will be automatically purged." strcat
      msgprep swap strcat tellme
    else pop then
  then
$endif
;
 
: maintenence
  0 runmode ! init runmode @ 0 < if exit then
  me @ unreadcount dup if
    "You have " over 1 = if "a new message" else
    over intostr " new messages" strcat then
    " waiting, use 'mail' or 'qmail' to read " strcat strcat
    swap 1 = if "it." else "them." then strcat tellme
  else pop then
  0 0 me @ savedcount begin dup while
    me @ over 1 getmesgprop "#/" strcat
$ifdef MAXSAVED
    me @ over "purged" strcat getpropint dup if
      systime swap - GRACETIME > if
        pop me @ over 1 delmail swap 1 + swap 1 - continue
      then
    else pop then
$endif
    me @ over "read" strcat getpropint dup not if
      pop me @ over "read" strcat systime writepropint systime
    then
$ifdef EXPIRETIME
    over "expired" strcat me @ swap getpropint dup if
      systime swap - GRACETIME > if
        pop pop me @ over 1 delmail swap 1 + swap 1 - continue
      then pop
    else pop
      systime swap - EXPIRETIME > if
        me @ over "expired" strcat systime writepropint 4 rotate 1 +
        -4 rotate
      then
    then
$else
    pop
$endif
    pop 1 -
  repeat pop
  dup if
    msgprep swap intostr strcat " messages deleted." strcat
    over if
      " and " strcat swap intostr strcat
      " messages are expired and will be deleted." strcat
    else swap pop then
    tellme
  else
    pop dup if
      msgprep swap intostr strcat " messages are expired and will be deleted."
      strcat tellme
    else pop then
  then
;
 
: qcall ( s1 s2 i --  Calls a qmail function from an external program )
  "s2i" checkargs
  16 runmode ! init runmode @ not if pop pop pop exit then
  checkmailbox qmail deinit
;
 
: mane
  command @ 1 strcut tolower swap toupper swap strcat command !
  command @ "Q" 1 strncmp command @ "@q" 2 strncmp and not if
    16 runmode !
  else
    strip dup not if 1 else 2 then runmode !
  then
  init runmode @ dup not if
    pop pop exit
  then checkmailbox 15 > if qparse deinit exit then
  strip dup "#" 1 strncmp if
    dup not if
      pop do_menu
    else
      newmail
    then
  else
    1 strcut swap pop 0 swap do_command not if "Unknown option" tellme then
  then
  "*Done*" tellme deinit
  me @ clearlock
;
PUBLIC maintenence
PUBLIC unreadcount
PUBLIC savedcount
PUBLIC qcall
.
c
q
@reg MOSS1.1.muf=lib/mail
@set $lib/mail=!d
@set $lib/mail=!z
@set $lib/mail=!l
@set $lib/mail=w
@set $lib/mail=_defs/MAILnew:"$lib/mail" match "unreadcount" call
@set $lib/mail=_defs/MAILsaved:"$lib/mail" match "savedcount" call
@set $lib/mail=_defs/QUICKsend:1 "$lib/mail" match "qcall" call
@set $lib/mail=_defs/QUICKread:2 "$lib/mail" match "qcall" call
@set $lib/mail=_defs/QUICKerase:3 "$lib/mail" match "qcall" call
@set $lib/mail=_defs/QUICKforward:4 "$lib/mail" match "qcall" call
@set $lib/mail=_defs/QUICKcheck:5 "$lib/mail" match "qcall" call
@set $lib/mail=_defs/QUICKscan:6 "$lib/mail" match "qcall" call
@prog MOSS-mailwarn
1 9999 d
i
: mane "$lib/mail" match "maintenence" call ;
.
c
q
@set MOSS-mailwarn=!d
@set MOSS-mailwarn=!z
@set MOSS-mailwarn=l
@set MOSS-mailwarn=w
@propset #0=dbref:_connect/MOSS:MOSS-mailwarn
whisper me=MOSS 1.1 installed.  Don't forget to read and set the configuration parameters at the beginning of the program.
whisper me=If you want to use MOSS 1.1 for page #mail, you will need to get cmd-page 2.50 or newer from the FB-MUF archive.  Edit the config options at the beginning of page, changing MAILTYPE to LIBMAIL.
whisper me=To use MOSS 1.1 as a standalone program, or as a compliment to the LIBMAIL option of page, create an action called qmail;mail and link it to $lib/mail.

