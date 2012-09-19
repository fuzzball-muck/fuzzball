@program cmd-page
1 99999 d
1 i
( MUFpage    Copyright 4/15/91 by Fuzzball Software              )
(                                 foxen@netcom.com               )
(                                                                )
( This code is released under the GNU Public Licence.            )
(                                                                )
  
( CONFIGURATION )
  
( Restrict guests from paging anyone but a wizard )
$def RESTRICTWIZ
( Allow multi-player summons )
( $def MULTISUMMON )
( Restrict guests from multi-page )
$def RESTRICTMULT
  
( Encrypt lastpage[d/dgroup/er/ers] props )
( $def ENCRYPTPROPS )
  
( Encryption key for lastpaged props and #mail, where applicable )
$def KEY "encryption key"
  
( The action of #mail, PAGEMAIL or LIBMAIL or NONE )
$define MAILTYPE LIBMAIL $enddef
  
( END OF CONFIGURATION )
  
$ifdef MAILTYPE=LIBMAIL
$include $lib/mail
$else
$ifndef MAILTYPE=NONE
$ifndef MAILTYPE=PAGEMAIL
$echo "Warning: Invalid or no mailtype assigned, assuming none."
$def MAILTYPE NONE
$endif
$endif
$endif
  
$def VERSION "MUFpage v3.00 by Revar"
$def UPDATED "Updated 3/16/00"
  
$def descr_idle descrcon conidle
  
: oproploc ( dbref -- dbref' )
    dup "_proploc" getpropstr
    dup if
        dup "#" 1 strncmp not if
            1 strcut swap pop
        then
        atoi dbref
        dup ok? if
            dup owner 3 pick
            dbcmp if swap then
        then
        pop
    else pop
    then
;
  
: myproploc ( -- dbref)
    me @ oproploc
;
  
: tell (string -- )
    me @ swap notify
;
  
: fillspace
    swap strlen -
    "                                        " ( 40 spaces )
    dup strcat ( 80 spaces now )
    swap strcut pop
;
  
$def strip-leadspaces striplead
$def strip-trailspaces striptail
  
( mail encryption stuff )
  
  
( Crypto ver 1: Very stupid encryption system. )
: transpose (char -- char')
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz 1234567890_"
    over instr dup if
        swap pop 1 -
        "wG8D kBQzWm4gbRXHOqaZiJPtUTN2pu6M0VjFlK3sdS9oYe5A_7IE1cnLvfyCrhx"
        swap strcut 1 strcut pop swap pop
    else
        pop
    then
;
  
: encrypt (string -- string')
    "" swap begin
        dup while
        100 strcut "" rot
        begin
            dup while
            1 strcut swap transpose
            rot swap strcat swap
        repeat pop
        rot swap strcat swap
    repeat pop
;
  
  
( Crypto ver 2:  better encryption. But slower. )
: asc (stringchar -- int)
    dup if
      " 1234567890-=!@#$%&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZXCVBNM<>?\"`~\\|^"
      swap instr 1 - exit
    then pop 0
;
  
: chr (int -- strchar)
    " 1234567890-=!@#$%&*()_+qwertyuiop[]QWERTYUIOP{}asdfghjkl;'ASDFGHJKL:zxcvbnm,./ZXCVBNM<>?\"`~\\|^"
    swap strcut 1 strcut pop swap pop
;
  
: cypher (key chars -- chars')
    1 strcut asc swap asc
    over 89 > over 89 > or if chr swap chr strcat swap pop exit then
    dup 10 / 10 *
    4 pick 10 + rot 10 % - 10 %
    rot dup 10 / 10 *
    5 rotate 10 + rot 10 % - 10 %
    4 rotate + chr -3 rotate + chr strcat
;
  
: crypt2 (key string -- string')
    swap 9 % 100 + "" rot
    begin dup while
        2 strcut 4 pick rot cypher
        rot swap strcat swap
    repeat pop swap pop
;
  
  
( Crypto ver 3: Faster inserver FB5 encryption )
  
: encrypt3 (key string -- string')
    swap intostr KEY over strcat strcat strencrypt
;
  
: decrypt3 (key string -- string')
    swap intostr KEY over strcat strcat strdecrypt
;
  
( Encryption dispatchers )
  
$ifdef ENCRYPTPROPS 
: pencrypt ( i s -- s )
    dup not if swap pop exit then
$ifdef __version>Muck2.2fb5.46
    encrypt3 "!" swap strcat
$else
    crypt2 "*" swap strcat
$endif
;
$else
    $def pencrypt swap pop
$endif
  
: pdecrypt ( i s -- s )
    dup not if swap pop exit then
    dup 1 strcut pop "*!" swap instr
    dup 1 = if pop 1 strcut swap pop crypt2 exit then
$ifdef __version>Muck2.2fb5.46
    dup 2 = if pop 1 strcut swap pop decrypt3 exit then
$endif
    pop swap pop
;
  
$ifdef MAILTYPE=PAGEMAIL
: mencrypt ( i s -- s )
    dup not if swap pop exit then
$ifdef __version>Muck2.2fb5.46
    encrypt3 "E" swap strcat
$else
    crypt2 "D" swap strcat
$endif
;
  
: mdecrypt ( i s -- s )
    dup not if swap pop exit then
    dup 1 strcut pop "CDE" swap instr
    dup 1 = if pop 1 strcut swap pop swap pop encrypt exit then
    dup 2 = if pop 1 strcut swap pop crypt2 exit then
$ifdef __version>Muck2.2fb5.46
    dup 3 = if pop 1 strcut swap pop decrypt3 exit then
$endif
    pop swap pop
;
$endif
  
(
Gazer's Sort routines
  
  Shell Sort
  
  Takes  [ x1 x2 x3 ... xn n -- x1' x2' x3' ... xn' n ]
)
  
: CmpStrCaseInsensAsc  stringcmp 0 > ;
  
: SortIncLoop  ( <strings*n> n cmp inc --- <strings*n> n )
    begin
      dup 0 > while                 ( while inc > 0 )
      dup 1 +                       (   for i := inc + 1 to n )
      begin
        dup 5 pick <= while           ( for i := inc + 1 to n )
        over over swap -              (   j := i - inc )
        begin
          dup 0 > while                 ( while j > 0 )
          dup 5 + pick                  ( get A[j] )
          over 5 pick + 6 + pick        ( get A[j+inc] )
          6 pick execute if             ( do comparison )
            dup 5 + pick                ( swap: get A[j] )
            over 5 pick + 6 + pick      (   get A[j+inc] )
            3 pick 6 + put              (   put into A[j] )
            over 5 pick + 5 + put       (   put into A[j+inc] )
            3 pick -                    ( j := j - inc )
          else
            break then                  ( break out if we don't swap )
        repeat pop
        1 +                             (   while j > 0 )
      repeat pop
      2 /
    repeat pop pop
;
  
: Sort  ( <strings*n> n )
    'CmpStrCaseInsensAsc
    over 2 / SortIncLoop
;
  
( End Gazer's Sort routines )
  
  
: sort-stringwords (str -- str')
    strip
    dup " " instr if
        " " explode sort
        begin dup 1 > while
            1 - swap " " strcat rot strcat swap
        repeat pop
        strip
    then
;
  
  
: fake_format? (default string -- string' TRUE )
               (            or -- default FALSE )
    "%n" me @ name subst
    dup "%n" instr not if
        "%n " swap strcat
    then
  
    dup  "%n whispers, \"%m\"" stringcmp not
    over "%n whispers \"%m\"" stringcmp not or
    over "%n shouts, \"%m\"" stringcmp not or
    over "%n shouts \"%m\"" stringcmp not or
    over "%n %m" stringcmp not or if
        pop 0
    else
        swap pop 1
    then
;
  
  
( *** routines to get and set properties *** )
  
: setpropstr (dbref propname value -- )
    dup not if
        pop 0 setprop
    else
        0 addprop
    then
;
  
: envprop envpropstr swap pop ;
  
: search-prop (propname -- str)
    myproploc over getpropstr
    dup not if
        me @ location
        rot envprop
    then
    swap pop
;
  
: getprop (playerdbref propname -- str)
    over oproploc over getpropstr
    dup not if
        pop swap over envprop
        dup not if
            pop trigger @ swap getpropstr
        else swap pop
        then
    else rot rot pop pop
    then
;
  
  
( *** BEGIN PERSONAL PROPS *** )
  
  
: getignorestr (playerdbref -- ignorestr)
    trigger @ getlink "ignore#"
    rot int intostr
    strcat getpropstr
;
  
: setignorestr (ignorestr playerdbref -- )
    int intostr trigger @
    getlink "ignore#" rot
    strcat rot setpropstr
;
  
: getprioritystr (playerdbref -- prioritystr)
    trigger @ getlink "priority#"
    rot int intostr
    strcat getpropstr
;
  
: setprioritystr (prioritystr playerdbref -- )
    int intostr trigger @
    getlink "priority#" rot
    strcat rot setpropstr
;
  
: getlastpager (playerdbref -- string)
    dup int swap oproploc "_page/lastpager" getpropstr pdecrypt
;
  
: setlastpager (string playerdbref -- )
    dup oproploc swap int rot pencrypt
  
    "_page/lastpager" swap setpropstr
;
  
: getlastpagers (playerdbref -- string)
    dup int swap oproploc "_page/lastpagers" getpropstr pdecrypt
;
  
: setlastpagers (string playerdbref -- )
    dup oproploc swap int rot pencrypt
  
    "_page/lastpagers" swap setpropstr
;
  
: getlastpaged (playerdbref -- string)
    dup int swap oproploc "_page/lastpaged" getpropstr pdecrypt
;
  
: setlastpaged (string playerdbref -- )
    dup oproploc swap int rot pencrypt
  
    "_page/lastpaged" swap setpropstr
;
  
: getlastpagedgroup (playerdbref -- string)
    dup int swap oproploc "_page/lastpagedgroup" getpropstr pdecrypt
;
  
: setlastpagedgroup (string playerdbref -- )
    dup oproploc swap int rot pencrypt
  
    "_page/lastpagedgroup" swap setpropstr
;
  
: set_page_standard (valstr -- )
    myproploc "_page/standard?" rot setpropstr
;
  
: page_standard? (playerdbref -- bool)
    oproploc "_page/standard?" getpropstr
    dup "yes" stringcmp not if pop 2 exit then
    "prepend" stringcmp not if 1 exit then
    0
;
  
: set_page_echo (valstr -- )
    myproploc "_page/echo?" rot setpropstr
;
  
: page_echo? ( -- bool)
    myproploc "_page/echo?" getpropstr
    "no" stringcmp not not
;
  
: set_page_inform (valstr -- )
    myproploc "_page/inform?" rot setpropstr
;
  
: page_inform? (playerdbref -- bool)
    oproploc "_page/inform?" getpropstr
    "yes" stringcmp not
;
  
  
: get-curr-format ( -- formatname )
    myproploc "_page/curr_format" getpropstr
    dup not if pop "page" then
;
  
: set-curr-format ( formatname -- )
    myproploc "_page/curr_format" rot setpropstr
;
  
: set-format-prop ( playerdbref formatname format -- )
    rot oproploc rot "_page/formats/f-" swap strcat rot setpropstr
;
  
: get-format-prop ( playerdbref formatname -- format )
    "_page/formats/f-" swap strcat over swap getprop
    dup not if pop "_page/formats/f-page" getprop else swap pop then
    dup not if pop "You page, \"%m\" to %n." then
;
  
: set-oformat-prop ( playerdbref formatname format -- )
    rot oproploc rot "_page/formats/o-" swap strcat rot setpropstr
;
  
: get-oformat-prop ( playerdbref formatname -- format )
    "_page/formats/o-" swap strcat over swap getprop
    dup not if pop "_page/formats/o-page" getprop else swap pop then
    "%n pages, \"%m\" to %t." swap dup if fake_format? then pop
;
  
: get_opose ( -- oposeformat)
    myproploc "_page/formats/o-pose" over swap getprop
    dup not if pop "_page/formats/o-page" getprop else swap pop then
    "In a page-pose to %t, %n %m" swap dup if fake_format? then pop
;
  
  
: set-standard (stdformat playerdbref -- )
    oproploc "_page/stdf" rot setpropstr
;
  
: get-standard (playerdbref -- stdformat)
    oproploc "_page/stdf" getpropstr
    dup not if pop "%n pages: %m" "_page/stdf" trigger @ swap getpropstr dup if swap then pop then
    "<loc>" "%l" subst
;
  
  
: set-prepend (prepformat playerdbref -- )
    oproploc "_page/prepf" rot setpropstr
;
  
: get-prepend (playerdbref -- prepformat)
    oproploc "_page/prepf" getpropstr
    dup not if pop "%n pages: " "_page/prepf" trigger @ swap getpropstr dup if swap then pop then
    "<loc>" "%l" subst
;
  
  
  
$ifdef MAILTYPE=PAGEMAIL
: get-forward (playerdbref -- string)
    oproploc "_page/forward" getpropstr
;
  
: set-forward (string -- )
    myproploc "_page/forward" rot setpropstr
;
  
  
: mail-count (playerdbref -- count)
    oproploc dup "_page/mail#" getpropstr atoi
    swap "page-mail#" getpropstr atoi +
;
  
: mail-get   (playerdbref -- message)
    dup dup mail-count swap oproploc
    "_page/mail" "#/" strcat 3 pick intostr strcat
    over over getpropstr dup not if
        pop dup "#/" rinstr 1 - strcut
        2 strcut swap pop strcat
        over over getpropstr
    then
    -5 rotate 0 setprop
    1 - intostr swap oproploc
    "_page/mail" "#" strcat rot setpropstr
;
  
  
: mail-add   (playerdbref message -- )
    over mail-count 1 + intostr
    3 pick oproploc "_page/mail" "#" strcat 3 pick setpropstr
    rot oproploc "_page/mail" "#/" strcat rot strcat rot setpropstr
;
  
: mail-erase-loop (proploc count -- proploc count)
    dup not if exit then
    over mail-get dup " " split pop
    1 strcut swap pop atoi dbref
    me @ dbcmp not if
        rot rot 1 - mail-erase-loop
    else
        pop exit
    then
    over 4 rotate mail-add
;
  
: mail-erase (playerdbref -- erased?)
    dup mail-count mail-erase-loop swap pop
;
  
$endif
  
: get-lastversion ( -- versionstr)
    myproploc "_page/lastversion" getpropstr dup if exit then
    pop myproploc "_page_lastversion" getpropstr
;
  
: verstr-parse (versionstr -- versionint)
    " " split swap pop dup not if pop 0 exit then
    " " split pop dup not if pop 0 exit then
    1 strcut swap pop "." split
    atoi swap atoi 100 * +
;
  
: set-lastversion (versionstr -- )
    myproploc "_page/lastversion" rot setpropstr
;
  
  
: get-multimax (playerdbref -- int)
    oproploc "_page/multimax" getpropstr
    atoi dup not if pop 8888 then
;
  
: set-multimax (int playerdbref -- )
    oproploc "_page/multimax"
    rot intostr setpropstr
;
  
  
: get-sleepmsg (dbref -- string)
    oproploc "_page/sleepmsg" getpropstr
;
  
: set-sleepmsg (string dbref -- )
    oproploc "_page/sleepmsg" rot setpropstr
;
  
: get-havenmsg (dbref -- string)
    oproploc "_page/havenmsg" getpropstr
;
  
: set-havenmsg (string dbref -- )
    oproploc "_page/havenmsg" rot setpropstr
;
  
: get-ignoremsg (dbref -- string)
    oproploc "_page/ignoremsg" getpropstr
;
  
: set-ignoremsg (string dbref -- )
    oproploc "_page/ignoremsg" rot setpropstr
;
  
: get-idlemsg (dbref -- string)
    oproploc "_page/idlemsg" getpropstr
;
  
: set-idlemsg (string dbref -- )
    oproploc "_page/idlemsg" rot setpropstr
;
  
: get-idletime (dbref -- int)
    oproploc "_page/idletime" getpropval
    dup not if pop 600 then
;
  
: set-idletime (int dbref -- )
    oproploc "_page/idletime" rot setprop
;
  
: get-awaymsg (dbref -- string)
    oproploc "_page/awaymsg" getpropstr
;
  
: set-awaymsg (string dbref -- )
    oproploc "_page/awaymsg" rot setpropstr
;
  
  
( change proploc )
  
  
: update-prop (dbref oldprop newprop -- )
    3 pick 3 pick getpropstr
    dup not if pop pop pop pop exit then
    4 pick 4 rotate 0 setprop
    setpropstr
;
  
$ifdef MAILTYPE=PAGEMAIL
: update-mail (dbref -- )
    dup dup "page-mail#" getpropstr atoi
    begin
        dup while
        over "page-mail#/"
        3 pick intostr strcat
        over over getpropstr not if
            pop "page-mail"
            4 pick intostr strcat
        then
        "_page/mail#/" 4 pick intostr strcat update-prop
        1 -
    repeat pop pop
    "page-mail#" "_page/mail#" update-prop
;
$endif
  
: update-aliases (dbref -- )
    dup dup "Aliases" getpropstr
    begin
        dup while
        " " split swap
        3 pick "Alias-" 3 pick strcat
        "_page/alias/a-" 4 rotate strcat
        update-prop
    repeat pop pop
    "Aliases" "_page/aliases" update-prop
;
  
: do-update-props (versionint -- )
    dup 300 < if
        dup 235 <= if
            myproploc
            dup "lastpager"         "_page/lastpager"      update-prop
            dup "lastpagers"        "_page/lastpagers"     update-prop
            dup "lastpaged"         "_page/lastpaged"      update-prop
            dup "lastpagedgroup"    "_page/lastpagedgroup" update-prop
            dup "_page_standard?"   "_page/standard?"      update-prop
            dup "_page_echo?"       "_page/echo?"          update-prop
            dup "_page_inform?"     "_page/inform?"        update-prop
            dup "_page_curr_format" "_page/curr_format"    update-prop
            dup "_page"             "_page/formats/f-page" update-prop
            dup "_opage"            "_page/formats/o-page" update-prop
            dup "_pose"             "_page/formats/f-pose" update-prop
            dup "_opose"            "_page/formats/o-pose" update-prop
            dup "_page_lastversion" "_page/lastversion"    update-prop
            dup "_page_prepf"       "_page/prepf"          update-prop
            dup "_page_stdf"        "_page/stdf"           update-prop
            dup "_page_sleepmsg"    "_page/sleepmsg"       update-prop
            dup "_page_havenmsg"    "_page/havenmsg"       update-prop
            dup "_page_ignoremsg"   "_page/ignoremsg"      update-prop
$ifdef MAILTYPE=PAGEMAIL
            dup "_page_forward"     "_page/forward"        update-prop
            dup update-mail
$endif
            pop
        then
        dup 240 <= if
            myproploc update-aliases
        then
    then
    pop
;
  
: update-galiases ( -- )
    trigger @ getlink dup dup "GlobalAliases" getpropstr
    trigger @ getlink "_page/galiases" getpropstr " " strcat over strcat
    strip sort-stringwords trigger @ getlink "GlobalAliases" rot setprop
    begin
        dup while
        " " split swap
        3 pick "AliasGlobal-" 3 pick strcat
        "_page/galias/g-" 4 pick strcat
        update-prop
        3 pick "GlobalOwn-" 3 pick strcat
        "_page/galiasown/g-" 4 rotate strcat
        update-prop
    repeat pop pop
    "GlobalAliases" "_page/galiases" update-prop
;
  
  
( change proploc )
  
  
: move-prop (dbref newdbref str -- )
    3 pick over getprop
    4 rotate 3 pick 0 setprop
    setprop
;
  
$ifdef MAILTYPE=PAGEMAIL
: move-mail (dbref newdbref count -- )
    begin
        dup while
        3 pick 3 pick "_page/mail#/"
        4 pick intostr strcat
        3 pick over getpropstr not if
            pop "_page/mail"
            4 pick intostr strcat
        then
        move-prop
        1 -
    repeat pop pop pop
;
$endif
  
: move-aliases (dbref newdbref aliases -- )
    begin
        dup while
        " " split swap
        4 pick 4 pick "_page/alias/a-"
        4 rotate strcat
        move-prop
    repeat pop pop pop
;
  
: do-proplock-set (str -- )
    strip match dup not if
        "page #proploc: I don't know what object you mean!"
        tell pop exit
    then dup #-2 dbcmp if
        "page #proploc: I don't know _which_ object you mean!"
        tell pop exit
    then dup owner me @ dbcmp not if
        "page #proploc: You don't own that object!"
        tell pop exit
    then myproploc swap
    dup int intostr me @ "_proploc" rot setpropstr
    over over "_page/lastpager"  move-prop
    over over "_page/lastpagers" move-prop
    over over "_page/lastpaged"  move-prop
    over over "_page/lastpagedgroup" move-prop
    over over "_page/standard?"     move-prop
    over over "_page/echo?"       move-prop
    over over "_page/inform?"     move-prop
    over over "_page/curr_format" move-prop
    over over "_page/formats/f-page"  move-prop
    over over "_page/formats/o-page"  move-prop
    over over "_page/formats/f-pose"  move-prop
    over over "_page/formats/o-pose"  move-prop
    over over "_page/lastversion"   move-prop
    over over "_page/prepf"      move-prop
    over over "_page/stdf"       move-prop
    over over "_page/away"       move-prop
    over over "_page/sleepmsg"   move-prop
    over over "_page/havenmsg"   move-prop
    over over "_page/ignoremsg"  move-prop
    over over "_page/idlemsg"  move-prop
    over over "_page/idletime"  move-prop
    over over "_page/awaymsg"  move-prop
$ifdef MAILTYPE=PAGEMAIL
    over over "_page/forward"    move-prop
    over "_page/mail#" getpropstr atoi
    3 pick 3 pick rot move-mail
    over over "_page/mail#"  move-prop
$endif
    over "_page/aliases" getpropstr
    3 pick 3 pick rot move-aliases
    over over "_page/aliases" move-prop
  
    "Properties now stored on \""
    swap name strcat "\"" strcat tell
;
  
  
( *** END PERSONAL PROPS *** )
  
  
: get-g-aliases ( -- aliasesstr)
    trigger @ getlink "_page/galiases" getpropstr
;
  
  
: set-g-aliases (aliasesstr -- )
    sort-stringwords
    trigger @ getlink "_page/galiases" rot setpropstr
;
  
  
: set-p-aliases (aliasesstr -- )
    sort-stringwords
    myproploc "_page/aliases" rot setpropstr
;
  
  
: get-p-aliases ( -- aliasesstr)
    myproploc "_page/aliases" getpropstr dup if exit then
    pop trigger @ getlink me @ int intostr
    "Aliases" strcat getpropstr
    dup set-p-aliases
    trigger @ getlink me @ int intostr
    "Aliases" strcat 0 setprop
;
  
  
: set-personal-alias (aliasname aliasstr -- )
    swap tolower dup strlen
    10 > if 10 strcut pop then
    swap get-p-aliases
    " " swap over strcat strcat
    over if
        dup 4 pick " " swap over strcat strcat
        instr not if
            " " strcat 3 pick strcat
        then
        "Personal alias set." tell
    else
        3 pick " " swap over strcat strcat
        split " " swap strcat strcat strip
        "Personal alias cleared." tell
    then
    strip set-p-aliases
  
    "_page/alias/a-" rot strcat
    myproploc swap rot setpropstr
;
  
  
: get-personal-alias (aliasname playerdbref -- aliasstr)
    over over oproploc "_page/alias/a-" rot strcat getpropstr
    dup if rot rot pop pop exit then
    pop over over int intostr "Alias" swap strcat
    "-" strcat swap strcat
    trigger @ getlink swap over over getpropstr
    dup not if pop pop pop pop pop "" exit then
    rot rot 0 setprop
    swap pop over swap set-personal-alias
;
  
  
: get-global-alias (aliasname -- aliasstr)
    trigger @ getlink "_page/galias/g-"
    rot strcat getpropstr
;
  
  
: set-global-alias (aliasname aliasstr -- )
    over get-global-alias
    me @ "w" flag? not and
    me @ trigger @ getlink owner dbcmp not and
    "_page/galiasown/g-" 4 pick strcat
    trigger @ getlink swap getpropstr
    me @ int intostr stringcmp and if
        "Permission denied." tell
        pop pop exit
    then
    (aliasname aliasstr)
    dup not if
        "_page/galiasown/g-" 3 pick strcat
        trigger @ getlink swap 0 setprop
    then
    (aliasname aliasstr)
  
    swap tolower dup strlen
    10 > if 10 strcut pop then
    swap get-g-aliases
    " " swap over strcat strcat
    over if
        dup 4 pick " " swap over strcat strcat
        instr not if " " strcat 3 pick strcat then
        "Global alias set." tell
    else
        3 pick " " swap over strcat strcat
        split " " swap strcat strcat strip
        "Global alias cleared." tell
    then
    strip set-g-aliases
  
    "_page/galiasown/g-" 3 pick strcat
    trigger @ getlink swap
    me @ int intostr setpropstr
  
    "_page/galias/g-" rot strcat
    trigger @ getlink swap rot setpropstr
;
  
  
: get-alias (aliasname playerdbref -- aliasstr)
    over swap get-personal-alias
    dup not if
        pop get-global-alias
    else swap pop
    then
;
  
( Line 888. )
( *** END PROPS ON PROG *** )

  
  
  
: getday ( -- int)
  
    systime dup 86400 % 86400 + time 60 * + 60 * + - 86400 % - 86400 /
  
  
;
  
  
: setday ( int -- )
    #0 "day" "" 4 pick addprop
    trigger @ getlink "day" rot "" swap addprop
;
  
  
  
  
: gettime ( -- int )
    time 60 * + 60 * +
;
  
: get-timestr ( -- timestr)
    "%I:%M%p" systime timefmt tolower
    dup "0" 1 strncmp not if
        1 strcut swap pop
    then
;
  
  
: get-timestr24h ( -- timestr)
    "%H:%M" systime timefmt
;
  
  
( *** end of routines for getting and setting properties *** )
  
( alias listing stuff )
  
: list-personal-aliases ( - )
    "  Personal Aliases List" tell
    "Alias Name -- Alias Expansion" tell
    "----------    --------------------------------------------------" tell
    me @ get-p-aliases sort-stringwords
    begin
        dup while
        " " split swap dup 4 pick get-personal-alias
        " -- " swap strcat over 10 fillspace swap strcat
        strcat tell
    repeat pop pop
;
  
  
: list-global-aliases ( - )
    "   Global Aliases List" tell
    "Alias Name -- Alias Expansion" tell
    "----------    --------------------------------------------------" tell
    get-g-aliases sort-stringwords
    begin
        dup while
        " " split swap dup get-global-alias
        " -- " swap strcat over 10 fillspace swap strcat
        strcat tell
    repeat pop
;
  
: list-matching-aliases (namestr -- )
  foreground "Aliases containing the name \"" over strcat "\"" strcat tell
  "Alias Name -- Alias Expansion" tell
  "----------    --------------------------------------------------" tell
  tolower get-g-aliases " " strcat get-p-aliases strcat sort-stringwords
    begin
        dup while
        " " split swap dup me @ get-alias
        " -- " swap strcat over 10 fillspace swap strcat strcat
        dup " " swap over strcat strcat tolower
        4 pick " " swap over strcat strcat
        instr not if pop else tell then
    repeat pop
;
  
  
( misc simple routines )
  
  
: single-space (s -- s') (strips all multiple spaces down to a single space)
    begin
        dup "  " instr while
        " " "  " subst
    repeat
;
  
  
: comma-format (string -- formattedstring)
    strip single-space
    ", " " " subst
    dup ", " rinstr dup if
        1 - strcut 2 strcut
        swap pop " and "
        swap strcat strcat
    else pop
    then
;
  
  
: stringmatch? (str cmpstr #charsmin-- bool)
    3 pick strlen > if pop pop 0 exit then swap stringpfx
;
  
  
( simple player matching )
  
  
: player-match? (playername -- [dbref] succ?)
  
    me @ pennies "lookup_cost" sysparm atoi < me @ "W" flag? not and if
        "You've run out of " "pennies" sysparm strcat " to page with!" strcat
        tell pop -1 exit
    then
    "*" swap strcat match dup if 1 else pop 0 then
;
  
  
  
: partial-match ( playername -- [dbref] succ? )
    dup strlen 3 < if pop 0 exit then
    part_pmatch dup ok? if 1 else pop 0 then
;
  
  
: cullto5words (string -- string')
    single-space strip
    " " explode_array
    0 4 [..] " " array_join
;
  
: update-lastpagers (fullname playerdbref -- )
    dup getlastpagers strip
    " " swap over strcat strcat
    " " 4 rotate over strcat strcat
    over tolower over tolower instr not if
        1 strcut swap pop strcat
        cullto5words swap setlastpagers
    else
        pop pop pop
    then
;
  
( Does anyone actually read these comments? )
( Probably not.  *gryn* )
( FEEP! )
  
: do-feep ( -- )
    "Welcome to page #feep!  I'm thinking of a number between 1 and 1024. "
    "Try to guess it!  To play, just enter numbers, and I'll tell you higher "
    "or lower.  To quit at any time, type any non number like 'quit' or 'end'."
    "  Enjoy!"  strcat strcat strcat tell 0 random 1024 % 1 +
    begin
        swap 1 + swap
        read dup number? not if
            "Quitting the page #feep game!  Bye!"
            tell pop pop pop exit
        then
        atoi over over = if
            "You guessed it in " 4 pick intostr strcat
            " guesses! Game over." strcat tell
            pop pop pop exit
        then
        over over > if pop "Higher." tell continue then
        over over < if pop "Lower." tell then
    repeat
;
  
( remember stuff )
  
  
: extract-player (playername string -- string')
    single-space " " explode dup 2 + rotate
    "" swap
    begin
        3 pick while
        4 rotate dup if
            over over stringcmp not if pop
            else
                rot dup if " " strcat then
                swap strcat swap
            then
        else pop
        then
        rot 1 - rot rot
    repeat pop swap pop
;
  
: remember-pager (playerdbref -- )
    me @ name over setlastpager
    me @ name over update-lastpagers
    me @ getlastpaged
    over name swap extract-player
    swap setlastpagedgroup
;
  
  
: remember-pagee (player[s] -- player[s])
    dup not if        (is a player specified?)
        pop me @      (if not, use last player paged...)
        getlastpaged
    else
        single-space  (...otherwise, use the player given...)
    then
;
  
  
( ignore stuff )
  
  
: ignored?       (playerdbref -- ignored?)
    getignorestr
    me @ int intostr
    " " strcat " #" swap
    strcat instr
;
  
  
: ignoring?       (playerdbref -- ignored?)
    int intostr " " strcat
    me @ getignorestr
    " #" rot strcat instr
;
  
  
: ignore-dbref (dbref -- )
    int intostr " " strcat
    " #" swap strcat
    me @ getignorestr
    swap over over instr not
    if strcat else pop then
    me @ setignorestr
;
  
  
: unignore-dbref (dbref -- )
    int intostr " " strcat
    " #" swap strcat
    me @ getignorestr
    swap split strcat
    me @ setignorestr
;
  
  
  
: check-ignored-dbref (dbref -- player?)
    dup player? not if
        unignore-dbref 0
    else
        pop 1
    then
;
  
  
: list-ignored ( -- string)
    "" me @ getignorestr
    strip single-space
    begin
        dup while
        " " split swap 1 strcut
        swap pop atoi dbref
        dup check-ignored-dbref if
            name " " strcat
            rot strcat swap
            else pop
        then
    repeat pop sort-stringwords " " strcat
    comma-format
;
        
    
( priority stuff )
  
: priority?   (playerdbref -- priority?)
    getprioritystr
    me @ int intostr
    " " strcat " #" swap
    strcat instr
;
  
: priority-dbref (dbref -- )
    int intostr " " strcat
    " #" swap strcat
    me @ getprioritystr
    swap over over instr not
    if strcat else pop then
    me @ setprioritystr
;
  
  
: unpriority-dbref (dbref -- )
    int intostr " " strcat
    " #" swap strcat
    me @ getprioritystr
    swap split strcat
    me @ setprioritystr
;
  
  
  
: check-priority-dbref (dbref -- player?)
    dup player? not if
        unpriority-dbref 0
    else
        pop 1
    then
;
  
  
: list-priority ( -- string)
    "" me @ getprioritystr
    strip single-space
    begin
        dup while
        " " split swap 1 strcut
        swap pop atoi dbref
        dup check-priority-dbref if
            name " " strcat
            rot strcat swap
        else pop
        then
    repeat pop sort-stringwords " " strcat
    comma-format
;
        
( page stuff )
  
  
: havened?  (playerdbref -- haven?)
    "haven" flag?
;
  
  
: pagepose? (string -- bool)
    dup strlen 1 > if
        2 strcut pop
        dup ":" 1 strncmp not if
          1 strcut swap pop
          " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,':*"
          swap instr
        else pop 0
        then
    else pop 0
    then
;
  
  
: page-me-inform (message -- )
    page_echo? if         (does sender not want to see the echo?)
        tell (if not, show constructed string to sender)
    else
        pop              (else, pop the string off the stack)
        "Your message has been sent."
        tell
    then
;
  
  
: page-them-inform (message dbref format to -- )
    3 pick name "(^| )%s([., ]|$)" fmtstring "\\1you\\2" 0 regsub -4 rotate
    over page_standard? dup 1 = if
        pop over get-prepend
        over over strlen strcut pop
        stringcmp if
            over get-prepend
            " " strcat swap strcat
        then
    else
        2 = if
            get_opose over stringcmp if
                pop dup get-standard
            else
                pop dup get-standard
                "%n %m" "%m" subst
            then
        then
    then
    3 pick " " split pop
    1 strcut strlen 3 <
    over not if swap pop " " swap then
    ".,?!:' " rot instr and
    if "%n%m" "%n %m" subst then
  
    me @ name "%n" subst (do name substitution for %n in format string)
    me @ location
    name "%l" subst      (do location of sender sub for %l in format string)
    4 rotate "%t" subst  (subst in the to line for %t)
  
    dup "%w" instr if
        get-timestr
        "%w" subst
    then
    dup "%W" instr if
        get-timestr24h
        "%W" subst
    then
  
  
    "%%m" "%m" subst
    "%%m" "%M" subst     (keep %m from being pronoun_subbed)
    me @ swap pronoun_sub (do pronoun subs for %o, %p, %r, %s in format str)
                          (using sender's pronoun subs)
  
  
    rot "%m" subst       (do message sub for %m in format string)
    notify               (show constructed string to receiver)
;
  
  
  
( mail stuff  )
  
  
$ifdef MAILTYPE=PAGEMAIL
: mail-unparse-mesg (mesgstr -- player time mesg)
    ( "#dbref day@hh:mm:ss Cencryptedmesg" )
    " " split swap
    dup "#" 1 strncmp not if
        1 strcut swap pop
        atoi dbref dup player? if name else pop "<Toad>" then swap
  
        "@" split swap
        atoi getday swap -
        dup not if
            pop "Today at "
        else dup 1 = if
            pop "Yesterday at "
            else
                intostr " days ago at " strcat
            then
        then
        swap " " split rot rot
  
        ":" split swap atoi
        dup 11 > if 12 - "PM" else "AM" then
        rot swap strcat swap
        dup not if pop 12 then
        intostr ":" strcat swap strcat strcat
  
        swap
        me @ int swap mdecrypt
    else
        swap 3 strcut swap pop ") -- " split
        swap ":" split swap atoi
        dup 11 > if 12 - "PM" else "AM" then
        rot swap strcat swap
        dup not if pop 12 then
        intostr ":" strcat swap strcat
        "Unknown day at " swap strcat swap
    then
;
  
: mail-read ( -- )
    me @ mail-count 0 > if
        me @ mail-get mail-read
        mail-unparse-mesg
        ") -- " swap strcat strcat
        " (" swap strcat strcat
        tell
    then
;
  
  
: mail-send (message player -- )
    dup mail-count 40 < over "truewizard" flag? or not if
        name "'s mail box is full." strcat tell pop
    else
        dup "You sense that you have new mail from " me @ name strcat
        ".  Use 'page #mail' to read." strcat notify
        "#" me @ int intostr strcat " " strcat
        getday intostr strcat "@" strcat
        time intostr ":" strcat
        swap dup intostr ":" strcat swap 10 < if "0" swap strcat then strcat
        swap dup intostr swap 10 < if "0" swap strcat then strcat
        strcat
        (message player string)
  
        " " strcat over int 4 rotate mencrypt
  
        strcat mail-add
        ( "#dbref day@hh:mm:ss Cencryptedmesg" )
    then
;
$endif
  
  
  
  
( player getting stuff )
  
  
: get-playerdbrefs  (playersstr -- dbref_range unrecstr)
    0 " " "" 4 rotate
    begin
        dup while
        " " split swap
        dup not if pop continue then
        dup "(" 1 strncmp not if
            " " strcat swap strcat
            ")" split swap pop strip
            continue
        then
        dup "#" 1 strncmp not if
            dup 1 strcut swap pop
            dup number? if
                atoi dbref dup ok? if
                    dup player? if
                        swap pop 5 rotate 1 +
                        -5 rotate -5 rotate
                        continue
                    else pop
                    then
                else pop
                then
            else pop
            then
        then
        dup "*" 1 strncmp not if
            1 strcut swap pop
            4 pick " " 3 pick over tolower strcat strcat instr if
                pop continue
            then
            4 rotate over tolower strcat " " strcat -4 rotate
            dup me @ get-alias dup not if
                pop "\"*" swap strcat
                "\" " strcat rot
                swap strcat swap continue
            then
            swap pop " " strcat
            swap strcat single-space
            continue
        then
        dup player-match? dup -1 = if
            pop pop pop pop
            strip exit
        then
        0 > if
            swap pop 5 rotate
            1 + -5 rotate -5 rotate
        else
            dup me @ get-alias dup if
                tolower
                5 pick " " 4 pick over strcat strcat instr if
                    pop pop continue
                then
                5 rotate rot strcat " " strcat -4 rotate
                " " strcat swap strcat single-space
            else pop
  
                dup partial-match
  
                if
                    swap pop 5 rotate 1 +
                    -5 rotate -5 rotate
                else
                    "\"" swap strcat
                    "\" " strcat rot
                    swap strcat swap
                then
            then
        then
    repeat pop swap pop sort-stringwords
;
  
  
: refs2names  (dbrefrange count nullstr -- dbrefrange namestr)
    begin
        over while
        3 pick 3 + rotate dup -5 rotate
        name strcat " " strcat
        swap 1 - swap
    repeat swap pop sort-stringwords
;
  
  
: remove-sleepers (dbrefrange count nullstr -- dbrefrange sleeperstr)
    begin
        over while
        3 pick 3 + rotate dup awake? if
            -4 rotate
        else
            dup get-sleepmsg dup if
                "Sleeping message for "
                rot name strcat ": " strcat
                swap strcat tell
            else
                pop name " " strcat strcat
            then
            rot 1 - rot rot
        then
        swap 1 - swap
    repeat swap pop sort-stringwords
;
  
  
$ifdef MAILTYPE=PAGEMAIL
: remove-non-erasees (dbrefrange count nullstr -- dbrefrange non-erasestr)
    begin
        over while
        3 pick 3 + rotate dup mail-erase if
            -4 rotate
        else
            name " " strcat strcat
            rot 1 - rot rot
        then
        swap 1 - swap
    repeat swap pop sort-stringwords
;
$endif
  
  
: remove-nopagers (dbrefrange count nullstr -- dbrefrange nopagestr)
    begin
        over while
        3 pick 3 + rotate dup havened? not over priority? or if
            -4 rotate
        else
            dup page_inform? if
                dup "You sense that " me @ name strcat
                " tried to page you, but you are set havened."
                strcat notify
            then
            dup get-havenmsg dup if
                "Haven message for "
                rot name strcat ": " strcat
                swap strcat tell
            else
                pop name " " strcat strcat
            then
            rot 1 - rot rot
        then
        swap 1 - swap
    repeat swap pop sort-stringwords
;
  
  
: remove-ignoring (dbrefrange count nullstr -- dbrefrange ignoringstr)
    begin
        over while
        3 pick 3 + rotate dup ignored? not if
            -4 rotate
        else
            dup page_inform? if
                dup me @ name
                " tried to page you, but you are ignoring them."
                strcat notify
            then
            dup get-ignoremsg dup if
                "Ignore message for "
                rot name strcat ": " strcat
                swap strcat tell
            else
                pop name " " strcat strcat
            then
            rot 1 - rot rot
        then
        swap 1 - swap
    repeat swap pop sort-stringwords
;
  
  
: remove-maxers (dbrefrange count count nullstr -- dbrefrange ignoringstr)
    begin
        over while
        4 pick 4 + rotate dup get-multimax 5 pick < not over priority? or if
            -5 rotate
        else
            dup page_inform? if
                dup me @ name
                " tried to include you in too large of a multi-page."
                strcat notify
            then
            name " " strcat strcat
            4 rotate 1 - -4 rotate
        then
        swap 1 - swap
    repeat swap pop swap pop sort-stringwords
;
  
  
  
: remove-nonwiz (dbrefrange count nullstr -- dbrefrange sleeperstr)
    begin
        over while
        3 pick 3 + rotate dup "wizard" flag? if
            -4 rotate
        else
            name " " strcat strcat
            rot 1 - rot rot
        then
        swap 1 - swap
    repeat swap pop sort-stringwords
;
  
  
  
: list-ignored-pagees (dbrefrange count nullstr -- dbrefrange ignoringstr)
    begin
        over while
        3 pick 3 + rotate dup ignoring? not if
            -4 rotate
        else
            dup -5 rotate
            name " " strcat strcat
        then
        swap 1 - swap
    repeat swap pop sort-stringwords
;
  
  
: do-getplayers (players -- dbrefrange)
    strip single-space
    remember-pagee
    get-playerdbrefs
    dup if
        comma-format dup " " instr
        "I don't recognize the player"
        swap if "s" strcat then
        " named " strcat swap strcat
        tell
    else pop
    then
;
  
  
: do-sleepers (dbrefrange -- dbrefrange')
    dup "" remove-sleepers
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently asleep." strcat
        strcat tell
$ifndef MAILTYPE=NONE
        "You can leave mail with 'page #mail <plyrs>=<msg>'" tell
$endif
    else pop
    then
;
  
  
  
$ifdef MAILTYPE=PAGEMAIL
: do-erasees (dbrefrange -- dbrefrange')
    dup "" remove-non-erasees
    dup if
        comma-format
        " didn't have any messages from you."
        strcat tell
    else pop
    then
;
$endif
  
  
  
: do-nopagers (dbrefrange -- dbrefrange')
    dup "" remove-nopagers
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently not accepting pages."
        strcat strcat tell
    else pop
    then
;
  
  
: do-ignoring (dbrefrange -- dbrefrange')
    dup "" remove-ignoring
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently ignoring you."
        strcat strcat tell
    else pop
    then
;
  
  
: do-nonwiz (dbrefrange -- dbrefrange')
    dup "" remove-nonwiz
    dup if
        comma-format dup " " instr if
          " are not wizards."
        else
          " is not a wizard."
        then
        strcat tell
    else pop
    then
;
  
  
: do-maxers ( dbrefrange -- dbrefrange' )
    dup dup "" remove-maxers
    dup if
        comma-format dup " " instr
        if " don't " else " doesn't " then
        "want to be included in multi-pages to that many people."
        strcat strcat tell
    else pop
    then
;
  
  
: do-list-ignored-pagees (dbrefrange -- dbrefrange')
    dup "" list-ignored-pagees
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently ignored by you."
        strcat strcat tell
    else pop
    then
;
  
(********* ADDED BY RISS *********)
: list-ineditor (lists folks in I mode)
    begin
        over while
        3 pick 3 + rotate dup "I" flag? not if
            -4 rotate
        else
            dup -5 rotate
            name " " strcat strcat
        then
        swap 1 - swap
    repeat swap pop sort-stringwords
;
: do-interactive (added by riss)
    dup "" list-ineditor
    dup if
        comma-format dup " " instr if
            " are editing a program or file and might not respond quickly."
        else
            " is editing a program or file and might not respond quickly."
        then
        strcat tell
    else pop
    then
;
(******* END ADDED BY RISS *******)
  
: away? (dbref -- bool)
    oproploc "_page/away" getpropstr
;
  
: idle-length (dbref -- int)
    dup player? if
        descriptors dup not if pop -1 exit then
        1 - swap descr_idle
        begin
            over
        while
            swap 1 - swap
            rot descr_idle
            over over > if swap then pop
        repeat
        swap pop
    else
        timestamps pop swap pop swap pop
        systime swap -
    then
;
  
: idle? (dbref -- bool)
    dup idle-length
    swap get-idletime >=
;
  
: do-list-away (refrange -- refrange')
    dup "" begin over while swap 1 - swap
        over 4 + pick dup away? not over "I" flag? or if pop continue then
        dup get-awaymsg dup if
            "Away message for "
            rot name strcat ": " strcat
            swap strcat tell
        else
            pop name " " strcat strcat
        then
    repeat swap pop sort-stringwords
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently away and may not get back to you soon." strcat
        strcat tell
    else pop
    then
;
  
: do-list-idle (refrange -- refrange')
    dup ""
    begin
        over
    while
        swap 1 - swap
  
        over 4 + pick dup idle? not over away? or over "I" flag? or
        if pop continue then
  
        dup get-idlemsg dup if
            "Idle message for "
            3 pick name strcat ": " strcat
            swap strcat
        else
            pop dup name
            " is currently %i idle and may not get back to you soon."
            strcat
        then
  
        swap idle-length
        dup 3600 > if
            3600 / " hour"
        else
            dup 60 > if 60 / " minute" else " second" then
        then
        over 1 = not if "s" strcat then
        swap intostr swap strcat
        "%i" subst tell
    repeat
    swap pop sort-stringwords
    dup if
        comma-format dup " " instr
        if " are " else " is " then
        "currently idle and may not get back to you soon." strcat
        strcat tell
    else pop
    then
;
  
: do-warn-away ()
    me @ away? if
        "You are currently marked as being away." tell
    then
;
  
  
: get-valid-pagees (players -- dbrefrange players')
    do-getplayers
    do-sleepers
$ifdef RESTRICTWIZ
    me @ name tolower "guest" 5 strncmp not if do-nonwiz then
$endif
    do-nopagers
    do-ignoring
    do-maxers
    do-interactive (ADDED BY RISS *******)
    do-list-away
    do-list-idle
    do-list-ignored-pagees
    do-warn-away
    dup "" refs2names
;
  
  
( each stuff )
  
  
: page-toeach (dbrefrange to message -- )
    begin
        3 pick while
        3 pick 3 + rotate over swap
        (refrange to mesg mesg dbref)
        dup remember-pager
        get-curr-format
        me @ swap get-oformat-prop
        (refrange to mesg mesg dbref format)
        5 pick page-them-inform
        rot 1 - rot rot
    repeat pop pop pop
;
  
  
: summon-toeach (dbrefrange -- )
    begin
        dup while
        dup 1 + rotate
        dup remember-pager
        "You sense that " me @ name strcat
        " is looking for you in " strcat
        me @ location name strcat
        over me @ location owner dbcmp if
          me @ location intostr
          "(#" swap strcat ")" strcat strcat
        then
        "." strcat notify
        1 -
    repeat pop
;
  
  
  
$ifdef MAILTYPE=PAGEMAIL
: mail-toeach (dbrefrange message -- )
    begin
        over while
        over 2 + rotate
        over swap mail-send
        swap 1 - swap
    repeat pop pop
;
  
: mail-do-forwards (dbrefrange message -- )
    begin
        over while
        swap 1 - swap
        over 3 + rotate
        dup get-forward dup if
            do-getplayers dup if
                dup "" remove-ignoring pop
                dup 2 + rotate name
                "(Orig. to " swap strcat
                ") " strcat
                over 3 + pick strcat
            else pop 1 3 pick
            then
        else pop 1 3 pick
        then
        mail-toeach
    repeat pop pop
;
  
: check-each (dbrefrange -- )
    begin
        dup while
        dup 1 + rotate
        dup name " has " strcat
        over mail-count
        dup not if
          pop "no messages" strcat
        else
          dup 1 = if
            pop "1 message" strcat
          else
            intostr strcat
            " messages" strcat
          then
        then
        " waiting." strcat
        over mail-count if
            "  Oldest is dated " strcat swap
            oproploc dup "_page/mail#/1" getpropstr
            dup not if
                swap "_page/mail"
                "1" strcat getpropstr
            then
            swap pop mail-unparse-mesg
            pop swap pop strcat "." strcat
        else swap pop
        then
        tell
        1 -
    repeat pop
;
$endif
  
  
  
: ignore-each (dbrefrange -- )
    begin
        dup while
        swap ignore-dbref
        1 -
    repeat pop
;
  
  
: unignore-each (dbrefrange -- )
    begin
        dup while
        swap unignore-dbref
        1 -
    repeat pop
;
  
  
: priority-each (dbrefrange -- )
    begin
        dup while
        swap priority-dbref
        1 -
    repeat pop
;
  
  
: unpriority-each (dbrefrange -- )
    begin
        dup while
        swap unpriority-dbref
        1 -
    repeat pop
;
  
  
( multi stuff )
  
  
: multi-page (message player -- )
    get-valid-pagees
    dup if
        (message dbrefrange playerstr)
        dup me @ setlastpaged comma-format
        (message dbrefrange playerstr)
        over 3 + rotate
        (dbrefrange playerstr message)
        dup me @ get-curr-format
        (derefrange plyrstr mesg mesg formatname)
        get-format-prop
        (derefrange plyrstr mesg mesg format)
        over " " split pop
        1 strcut strlen 3 <
        over not if swap pop " " swap then
        ".,?!:' " rot instr and
        if "%i%m" "%i %m" subst then
        (derefrange plyrstr mesg mesg format)
        4 pick "%n" subst
        (derefrange plyrstr mesg mesg format)
        dup "%w" instr if
            get-timestr
            "%w" subst
        then
        dup "%W" instr if
            get-timestr24h
            "%W" subst
        then
        me @ name "%i" subst
        (derefrange plyrstr mesg mesg format)
        swap "%m" subst
        (derefrange plyrstr mesg format)
        page-me-inform page-toeach
  
        me @ havened? if
            "You are currently set haven."
            tell
        then
    else pop pop pop
    then
;
  
  
: multi-summon (player -- )
    get-valid-pagees
    dup if
        dup me @ setlastpaged comma-format
        "You sent your summons to "
        swap strcat "." strcat
        page-me-inform summon-toeach
  
        me @ havened? if
            "You are currently set haven."
            tell
        then
    else pop pop
    then
;
  
  
: multi-ping (player -- )
    get-valid-pagees
    dup if
        dup me @ setlastpaged
        comma-format
        "You can page to "
        swap strcat "." strcat
        page-me-inform popn
  
        me @ havened? if
            "You are currently set haven."
            tell
        then
    else pop pop
    then
;
  
  
  
$ifdef MAILTYPE=PAGEMAIL
: multi-mail (mesg names -- )
    do-getplayers
    do-ignoring
    dup "" refs2names
    ( mesg {dbref_range} names )
    dup if
        dup me @ setlastpaged
        over 3 + rotate dup pagepose? if
            1 strcut swap pop
            dup " " split pop
            1 strcut strlen 3 <
            over not if swap pop " " swap then
            ".?!,': " rot instr and
            not if " " swap strcat then
            me @ name swap strcat
        then
        swap comma-format
        "You mail \"" 3 pick strcat
        "\" to " strcat over strcat "." strcat tell
        dup " " instr if
            "(to " swap strcat ")" strcat strcat
        else pop
        then
        mail-do-forwards
  
        me @ havened? if
            "You are currently set haven."
            tell
        then
    then
;
  
: multi-check
    do-getplayers
    dup if
        check-each
    then
;
  
: multi-erase (player -- )
    do-getplayers
    do-erasees
    dup "" refs2names
    dup if
        comma-format
        "You erased your last message to "
        swap strcat "." strcat
        page-me-inform popn
    else pop pop
    then
;
$endif
  
  
  
: multi-ignore (players -- )
    do-getplayers
    dup "" refs2names
    comma-format
    "Adding " swap strcat
    " to your ignore list."
    strcat tell ignore-each
;
  
  
: multi-unignore (players -- )
    do-getplayers
    dup "" refs2names
    comma-format
    "Removing " swap strcat
    " from your ignore list."
    strcat tell unignore-each
;
  
  
  
: multi-priority (players -- )
    do-getplayers
    dup "" refs2names
    comma-format
    "Adding " swap strcat
    " to your priority list."
    strcat tell priority-each
;
  
  
: multi-unpriority (players -- )
    do-getplayers
    dup "" refs2names
    comma-format
    "Removing " swap strcat
    " from your priority list."
    strcat tell unpriority-each
;
  
  
  
(  _______
  {__|__  \
  ___|__}_/
)
  
( help stuff )
  
  
$def }tell }list { me @ }list array_notify
  
: show-changes
{
VERSION "   Changes" strcat
"---------------------------------------------------------------------------"
"v3.01  5/20/02  Added %W for 24 hour time in message formats."
"v3.00  3/16/00  Optimized the code somewhat for FBMUCK 6."
"v2.51  7/ 3/96  Added %i sub for idle messages to give idle time.  Added"
"                 #idletime option."
"v2.50  4/28/96  Added config options, interactive, idle and away warning,"
"                 fixed various bugs, and made the code more FBized."
"v2.40  7/13/92  Modded to use propdirs and assume FB server."
"v2.35  3/31/92  Made page-posing more intelligent with regards to spacing."
"v2.34  2/ 5/92  Make lastpaged/r/group encrypted.  Improved encryptions."
"                 Added partial name matching for last five pagers."
"v2.32  1/22/92  Added #lookup <player> to list aliases w/ them in them."
"v2.31 10/31/91  Summoning now gives room# if pagee owns room pager is in."
"v2.30 10/12/91  Added #priority for letting players page you despite haven."
"v2.29 10/11/91  Added #sleepmsg, #haven and #ignore messages."
"-- Type 'page #help' to see more info on each command.  \"feeps 4-ever!\" --"
}tell
;
  
(  old changes:
"v2.26 10/10/91  Fixed #multimax probs, and made #mail remember last paged."
"v2.25  9/ 6/91  Fixed #proploc page-mail copying problem.  Added #multimax."
"v2.23  8/21/91  Added #erase for erasing messages mistakenly #mailed."
"v2.22  6/20/91  Added #inform.  Various bugfixes and security fixes."
"v2.20  6/17/91  Made #proploc work with p-aliases.  Added 'page &<alias>'"
"                 Fixed aliases to work with dbrefs and ignore stuff in parens"
"v2.18  6/14/91  Made it sort all multiple name outputs alphabetically."
"v2.17  6/12/91  Added sorting to alias listing."
"v2.16  6/11/91  Made small formatting fixes.  Moved p-aliases to player"
"v2.15  5/27/91  Made paging of multiple ignored players list on one line."
"v2.14  5/21/91  Added %w oformat sub for time.  Made all functions that"
"                 take player arguments work with page-again feature. Added"
"                 #time to tell the current time.  Helpful with %w's"
"v2.11  5/20/91  Added #proploc and made #ignore work on page-mail."
"v2.09  5/16/91  Added #check to see if a player has page-mail waiting."
"v2.08  5/16/91  Made page-mail use encryption, and disallowed multi-page"
"                 usage by the Guest character.  Added update notification."
"v2.05  5/ 9/91  Added %t substitution for #oformats to list all paged to."
"v2.04  5/ 9/91  Added #forward, and day stamping in page-mail."
"v2.02  5/ 1/91  Added #credits and fixed a problem with paging when broke."
"v2.00  4/27/91  Removed #pose, #opose, #page, #opage and replaced them with"
"                 #format,  #oformat and 'page !<format> <plyrs>=<msg>'."
)
  
  
: show-credits
{
VERSION "   " strcat UPDATED strcat "   Credits" strcat
"-------------------------------------------------------------------------"
"The following people, through questions, comments, or suggestions gave me"
"the ideas for the following features:  (in alphabetical order)"
"  Ashtoreth:    disallowing Guest multi-paging, #inform"
"  auzzie:       #ignore, formats, #haven, #ping, #help, #credits"
"  Bruce:        #mail"
"  Chris:        informing when you are haven, or page an ignored player"
"  ChupChup:     #echo, #standard, using /lib/cpp"
"  darkfox:      various coding ideas, %w subs, and being a kooshball target"
"  Erych:        encryption of page-mail"
"  Fre'ta:       #away, #idle, config stuff, MOSSmail interface."
"  fur:          Made all player arg commands work with page-again"
"  Gazer:        The shell sort routines. (he wrote the code)"
"  Jack_Salem:   #erasing of mistakenly sent page-mail"
"  Karrejanshi:  showing room numbers in summons when pagee owns room."
"  Lunatic:      single line messages for multiple people."
"  Lynn_Onyx:    page #mail security loophole fix.  #priority"
"  Miller:       #check"
"  Platypus_Bob: #prepending formats, #standard formats"
"  Sabachka:     24 hour time format substitutions."
"  Siegfried:    disallowing Guest use of #commands.  dbrefs in aliases."
"  Snooze:       debugging help with paging without pennies"
"  tk:           global and personal multi-person aliases"
"  Tugrik:       multiple selectable formats"
"And for help with the code itself"
"  Fre'ta:       Configurability, strencrypt/strdecrypt, #idle, #away"
"  Riss:         Interactive notification"
"And this leaves only multi-player paging, #version, #changes, #hints,"
"#index and page-posing as completely my own ideas that no-one else"
"suggested I add into it.  Oh yes... and #feep."
}tell
;
  
  
: show-index
{
VERSION "   " strcat UPDATED strcat "   Index" strcat
"----------------------------------------------------------------"

$ifndef MAILTYPE=NONE
"Aliases            2,A               Multi-paging          1    "
"Away               4                 Multimax              2    "
"Changes            1                 Oformats              3,A,B"
"Echo               3                 Page format           A,B  "
"Erase              1                 Pose format           A,B  "
"Formatted          3                 Paging                1    "
"Formats            3,A               Pinging               2    "
"Forwarding         3                 Posing                1,B  "
"Global aliases     2,A               Prepend               3,B  "
"Haven              4                 Proploc               3,B  "
"Help               1,2,3             Repaging              1    "
"Hints              1,A,B             Replying              1    "
"Idle               4                 Sleepmsg              4    "
"Ignoring           2                 Standard              3    "
"Inform             3                 Summoning             1    "
"Mailing            1                 Version               1    "
"Mail-checking      3                 Who                   1    "

$else

"Aliases            2,A               Oformats              3,A,B"
"Away               4                 Page format           A,B  "
"Changes            1                 Pose format           A,B  "
"Echo               3                 Paging                1    "
"Formatted          3                 Pinging               2    "
"Formats            3,A               Posing                1,B  "
"Global aliases     2,A               Prepend               3,B  "
"Haven              4                 Proploc               3,B  "
"Help               1,2,3             Repaging              1    "
"Hints              1,A,B             Replying              1    "
"Idle               4                 Sleepmsg              4    "
"Ignoring           2                 Standard              3    "
"Inform             3                 Summoning             1    "
"Multi-paging       1                 Version               1    "
"Multimax           2                 Who                   1    "
$endif

"--  1 = page #help      2 = page #help2     3 = page #help3   --"
"--  4 = page #help4     A = page #hints     B = page #hints2  --"
}tell
;
  
  
: show-help
{
VERSION "   " strcat UPDATED strcat "   Page1" strcat
"--------------------------------------------------------------------------"
"To give your location to another player:     'page <player>'"
"To send a message to another player:         'page <player> = <message>'"
"To send a pose style page to a player:       'page <player> = :<pose>'"
"To page multiple people:                     'page <plyr> <plyr> [= <msg>]'"
"To send another mesg to the last players:    'page = <message>'"
"To send your loc to the last players paged:  'page'"
"To send a message in a different format:     'page !<fmt> <plyrs> = <msg>'"
"To reply to a page sent to you:              'page #r [= <message>]'"
"To reply to all the people in a multi-page:  'page #R [= <message>]'"
  
$ifndef MAILTYPE=NONE
"To leave a mail message for someone:         'page #mail <players>=<mesg>'"
"To read all mail messages left for you:      'page #mail'"
"To erase a message you sent to a player:     'page #erase <players>'"
$endif
  
"To list who you last paged, who last"
"  paged you, and who you are ignoring:       'page #who'"
"To display what version this program is:     'page #version'"
"To display the latest program changes:       'page #changes'"
"To show who all helped with this program:    'page #credits'"
"To display an index of commands:             'page #index'"
"To display the next help screen:             'page #help2'"
"-- Words in <> are parameters.  Parameters in [] are optional. --"
}tell
;
  
  
: show-help2
{
VERSION "  " strcat UPDATED strcat "  Page2" strcat
"------------------------------------------------------------------------"
"To test if you can page a player:          'page #ping <players>'"
"To refuse pages from specific players:     'page #ignore <players>'"
"To set the mesg all ignored players see:   'page #ignore [<plyrs>]=<mesg>'"
"To accept pages from a player again:       'page #!ignore <player>'"
"To let players page you despite haven:     'page #priority <players>'"
"To remove players from your priority list: 'page #!priority <players>'"
"To page a group of people in an alias:     'page *<aliasname> = <message>'"
"To set a personal page alias:              'page #alias <alias>=<players>'"
"To clear a personal page alias:            'page #alias <alias>='"
"To list who is in an alias:                'page #alias <alias>'"
"To list all your personal aliases:         'page #alias'"
"To set an alias to the players last paged: 'page &<aliasname>'"
"To make an alias that everyone can use:    'page #global <alias>=<players>'"
"To clear a global page alias:              'page #global <alias>='"
"To list all the global aliases:            'page #global'"
"To list all aliases with a player in them: 'page #lookup <playername>'"
"To see the time (useful with %w subs):     'page #time'"
"To set the max# of plyrs in a page to you: 'page #multimax <max#players>'"
"To see your multimax setting:              'page #multimax'"
"To display the third help page:            'page #help3'"
}tell
;
  
  
  
: show-help3
{
VERSION "   " strcat UPDATED strcat "   Page3" strcat
"--------------------------------------------------------------------------"
"To turn on echoing of your message:          'page #echo'"
"To turn off echoing of your message:         'page #!echo'"
"To be informed when a page to you fails:     'page #inform'"
"To be turn off failed-page informing:        'page #!inform'"
"To see another player's formatted pages:     'page #formatted'"
"To prepend a format string to other's pages: 'page #prepend'"
"To set your prepended format string:         'page #prepend <formatstr>'"
"To force other's pages to a standard format: 'page #standard'"
"To set the standard format you receive in:   'page #standard <formatstr>'"
"To set a format that you see when paging:    'page #format <fmtname>=<fmt>'"
"To set a format that others receive:         'page #oformat <fmtname>=<fmt>'"
  
$ifndef MAILTYPE=NONE
"To forward mail to another player:           'page #forward <players>'"
"To stop forwarding mail:                     'page #forward #'"
"To see who mail to you is forwarded to:      'page #forward'"
"To see if mail is waiting for a player:      'page #check [players]'"
$endif
  
"To use an object for storing page props on:  'page #proploc <object>'"
"To display the last page of help:            'page #help4'"
}tell
;
  
  
: show-help4
{
VERSION "   " strcat UPDATED strcat "   Page4" strcat
"--------------------------------------------------------------------------"
"To haven yourself so you are unpagable:      'page #haven'"
"To set your 'havened' message:               'page #haven <message>'"
"To clear your 'havened' message:             'page #haven #clear'"
"To unhaven yourself so you can be paged:     'page #!haven'"
"To mark yourself away:                       'page #away'"
"To set your away flag and message:           'page #away <message>'"
"To clear your away message:                  'page #away #clear'"
"To reset your away flag:                     'page #!away'"
"To set the your 'Sleeping' message:          'page #sleepmsg <message>'"
"To clear your 'Sleeping' message:            'page #sleepmsg #clear'"
"To set your idle message:                    'page #idlemsg <message>'"
"To clear your idle message:                  'page #idlemsg #clear'"
"To view what your current idle timeout is:   'page #idletime'"
"To set how long your idle timeout is:        'page #idletime <minutes>'"
"To turn off your idle messages:              'page #idletime #off'"
}tell
;
  
  
: show-hints
{
VERSION "   " strcat UPDATED strcat "   Hints1" strcat
"--------------------------------------------------------------------------"
"All page commands can be used abbreviated to unique identifiers."
"  For example: 'page #gl' is the same as 'page #global'"
"If you page to a name it doesn't recognize, it will check to see if it is"
"  a personal alias.  If it isn't, it checks to see if it is a global alias."
"  For example: If there is a global alias 'tyg' defined as 'Tygryss', then"
"  'page tyg=test' will page 'test' to Tygryss."
"In format strings, %n will be replaced by the name of the player(s) receiv-"
"  ing the page.  %m will be replaced by the message.  %i will be replaced"
"  by your name.  %w gets replaced by the time.  %W gets replaced by 24 hour"
"  time.  These messages are what are shown to you when you page to someone."
"In oformat strings, %n will be replaced by your name, %m by the message,"
"  and %l by your location.  %t will be replaced with the names of all the"
"  people in a multi-page.  %w will be replaced with the current time."
"  %W gets replaced by the current time in 24 hour format.  These messages"
"  are what is shown to the player you are paging."
"If you have a #prepend or #standard format with a %w, it shows you the time"
"  when a player paged you."
"Use 'page #hints2' to show the next hints screen."
}tell
;
  
  
: show-hints2
{
VERSION "   " strcat UPDATED strcat "   Hints2" strcat
"--------------------------------------------------------------------------"
"There are two standard formats with page: the 'page' format, and the 'pose'"
"  format.  There are matching #oformats to go with them as well."
"If you really dislike having your pages that begin with colon's parsed as"
"  page-poses, then you can 'page #oformat pose=%n pages: :%m'"
"  or alternately, you can simply use 'page ! <players>=<mesg>'"
"One good way to have all the pages to you beeped and hilighted is to do:"
"  'page #prepend ##page>' and then set up the this trigger in tinyfugue:"
"  '/def -p15 -fg -t\"##page> *\" = /beep 3%;/echo %e[7m%-1%e[0m'"
"  If you want bold hilites instead, use '%e[1m' instead of '%e[7m'"
"  This only works if you have version 1.5.0 or later of tinyfugue and a"
"  vt100 terminal type."
"TinyTalk users, to make your pages always beep, use 'page #standard'"
"  Then all pages to you will be in standard page format."
"You can specify another object to store the properties used by the page"
"  program on.  To do this, type 'page #proploc <object>' where <object>"
"  is either the name (if its in the room) or dbref of the object to use."
"  #proploc will automatically copy all the page props to the new object."
}tell
;
  
  
  
  
: show-who-info ( -- )
    "You last paged to "
    me @ getlastpaged comma-format
    dup not if pop "no one" then
    strcat "." strcat tell
  
    "The last six people to page you were "
    me @ getlastpagers comma-format
    dup not if pop "no one" then
    strcat " (who paged last)." strcat tell
  
    me @ getlastpagedgroup comma-format
    dup if
        "The last group page also included "
        swap strcat "." strcat tell
    else pop
    then
  
    "You are receiving pages in "
    me @ page_standard?
    dup 1 = if pop "prepended"
    else
        2 = if "forced standard"
        else   "regular formatted"
        then
    then
    strcat " form." strcat tell
  
    me @ get-multimax dup 888 < if
        "You accept pages including up to "
        over intostr strcat swap 1 >
        if " people." else " player." then strcat tell
    else pop
    then
    
    "You are ignoring "
    list-ignored dup not
    if pop "no one" then
    strcat "." strcat tell
  
    "You are giving priority to "
    list-priority dup not
    if pop "no one" then
    strcat "." strcat tell
  
    me @ "haven" flag? if
        "You are currently set haven, so no one can page you."
        tell
    then
;
  
  
  
: page-main
    preempt strip
    dup "&" 1 strncmp not if
        1 strcut swap pop
        "=" strcat me @
        getlastpaged strcat
        "#alias " swap strcat
    then
    dup "#R" 2 strncmp not if
        2 strcut swap pop
        me @ getlastpagedgroup
        " " strcat swap strcat
        "#r" swap strcat
    then
    dup "#r" 2 strncmp not if
        2 strcut swap pop
        me @ getlastpager
        " " strcat swap strcat
    then
    dup "#" 1 strncmp not if   (if it begins with #, then it is a command)
        dup "#who" 2 stringmatch? if
            pop show-who-info exit
        then
        dup "#version" 2 stringmatch? if
            pop VERSION "  " strcat UPDATED strcat
            tell exit
        then
        dup "#changes" 2 stringmatch? if
            pop show-changes exit
        then
        dup "#credits" 3 stringmatch? if
            pop show-credits exit
        then
        dup "#index" 3 stringmatch? if
            pop show-index exit
        then
        dup "#help" 2 stringmatch? if
            pop show-help exit
        then
        dup  "#help2" stringcmp not
        over "#hel2" stringcmp not or
        over "#he2" stringcmp not or
        over "#h2" stringcmp not or if
            pop show-help2 exit
        then
        dup  "#help3" stringcmp not
        over "#hel3" stringcmp not or
        over "#he3" stringcmp not or
        over "#h3" stringcmp not or if
            pop show-help3 exit
        then
        dup  "#help4" stringcmp not
        over "#hel4" stringcmp not or
        over "#he4" stringcmp not or
        over "#h4" stringcmp not or if
            pop show-help4 exit
        then
        dup "#hints" 3 stringmatch? if
            pop show-hints exit
        then
        dup  "#hints2" stringcmp not
        over "#hint2" stringcmp not or
        over "#hin2" stringcmp not or
        over "#hi2" stringcmp not or if
            pop show-hints2 exit
        then
  
        me @ name tolower "guest" 5 strncmp not if
            pop "Permission denied." tell exit
        then
  
        dup "#feep" 5 stringmatch? if
            do-feep pop exit
        then
        dup "#!haven" 3 stringmatch? if
            pop me @ "!haven" set
            "Haven bit reset."
            tell exit
        then
        dup "#!away" 3 stringmatch? if
            pop me @ oproploc "_page/away" 0 setprop
            "Away flag reset."
            tell exit
        then
        dup "#echo" 2 stringmatch? if
            pop "" set_page_echo
            "Pages now echoed." tell exit
        then
        dup "#!echo" 3 stringmatch? if
            pop "no" set_page_echo
            "Pages now not echoed." tell exit
        then
        dup "#inform" 3 stringmatch? if
            pop "yes" set_page_inform
            "You will now be informed of ignored page attempts."
            tell exit
        then
        dup "#!inform" 4 stringmatch? if
            pop "" set_page_inform
            "You will no longer be informed of ignored page attempts."
            tell exit
        then
        dup " " instr if
            " " split swap
  
$ifndef MAILTYPE=NONE
            dup "#mail" 2 stringmatch? if
                pop strip dup "=" instr if
$ifdef MAILTYPE=PAGEMAIL
                    "=" split strip swap
                    multi-mail exit
$else
                    "page #mail" swap QUICKsend exit
$endif
                else
$ifdef MAILTYPE=PAGEMAIL
                    "page: #mail format: 'page #mail <players>=<message>'"
                    tell pop exit
$else
                    "page #mail" swap QUICKread exit
$endif
                then
            then
            dup "#check" 3 stringmatch? if
$ifdef MAILTYPE=PAGEMAIL
                pop multi-check exit
$else
                pop "page #check" swap QUICKcheck exit
$endif
            then
$endif
  
            dup "#haven" 3 stringmatch? if
                pop strip dup
                "#clear" stringcmp not if pop "" then
                me @ set-havenmsg
                me @ "haven" set
                "Haven message and haven bit are now set." tell exit
            then
            dup "#away" 3 stringmatch? if
                pop strip dup
                "#clear" stringcmp not if pop "" then
                me @ set-awaymsg
                me @ oproploc "_page/away" "yes" setprop
                "Away message and away flag are now set." tell exit
            then
            dup "#sleepmsg" 3 stringmatch? if
                pop strip dup
                "#clear" stringcmp not if pop "" then
                me @ set-sleepmsg
                "Sleep message is set." tell exit
            then
            dup "#idlemsg" 3 stringmatch? if
                pop strip dup
                "#clear" stringcmp not if pop "" then
                me @ set-idlemsg
                "Idle message is set." tell exit
            then
            dup "#idletime" 6 stringmatch? if
                pop strip dup
                "#off" stringcmp not if
                    pop 88888888
                else
                    dup number? if atoi else pop -1 then
                then
                dup 0 > if
                    60 * me @ set-idletime
                    "Idle timeout is set." tell
                else
                    pop "page: #idletime: timeout must be a positive number."
                    tell
                then
                exit
            then
            dup "#ignore" 2 stringmatch? if
                pop strip dup "=" instr if
                    "=" split strip
                    swap strip swap
                    me @ set-ignoremsg
                    "Ignore message is set." tell
                    dup not if pop exit then
                then
                single-space multi-ignore exit
            then
            dup "#!ignore" 3 stringmatch? if
                pop strip single-space
                multi-unignore exit
            then
            dup "#priority" 2 stringmatch? if
                pop strip single-space
                multi-priority exit
            then
            dup "#!priority" 3 stringmatch? if
                pop strip single-space
                multi-unpriority exit
            then
            dup "#format" 2 stringmatch? if
                pop dup "=" instr if
                    "=" split strip swap
                    strip single-space
                    "_" " " subst
                    me @ swap rot
                    set-format-prop
                    "Format set." tell
                else
                    strip dup
                    me @ swap get-format-prop
                    swap "' set to \"" strcat
                    swap strcat "\"" strcat
                    "Format '" swap strcat tell
                then exit
            then
            dup "#oformat" 3 stringmatch? if
                pop dup "=" instr if
                    "=" split strip swap
                    strip single-space
                    "_" " " subst
                    me @ swap rot
                    set-oformat-prop
                    "Oformat set." tell
                else
                    strip dup
                    me @ swap get-oformat-prop
                    swap "' set to \"" strcat
                    swap strcat "\"" strcat
                    "Oformat '" swap strcat tell
                then exit
            then
            dup "#alias" 2 stringmatch? if
                pop dup "=" instr if
                    "=" split single-space
                    strip swap
                    strip single-space
                    dup not if
                        "page: #alias: Alias name cannot be null"
                        tell pop pop exit
                    then
                    "_" " " subst swap
                    set-personal-alias
                else
                    strip dup me @
                    get-alias "Alias \"" rot
                    strcat "\" expands to \""
                    strcat swap strcat "\""
                    strcat tell
                then exit
            then
            dup "#global" 2 stringmatch? if
                pop "=" split strip single-space
                swap strip single-space
                dup not if
                    "page: #global: Alias name cannot be null"
                    tell pop pop exit
                then
                "_" " " subst swap
                set-global-alias exit
            then
            dup "#lookup" 3 stringmatch? if
                pop single-space strip
                list-matching-aliases
                "Done." tell exit
            then
  
$ifndef MAILTYPE=NONE
            dup "#forward" 4 stringmatch? if
                pop single-space
$ifdef MAILTYPE=PAGEMAIL
                dup "#" strcmp not if
                    pop "" "Page-mail forwarding cleared."
                else
                    "Page-mail forwarding set."
                then tell set-forward exit
$else
                "page #forward" swap QUICKforward exit
$endif
            then
            dup "#erase" 4 stringmatch? if
                pop strip single-space
$ifdef MAILTYPE=PAGEMAIL
                multi-erase exit
$else
                "page #erase" swap QUICKerase exit
$endif
            then
$endif
  
            dup "#multimax" 3 stringmatch? if
                pop strip atoi
                me @ set-multimax
                "Multi-max set." tell exit
            then
            dup "#standard" 3 stringmatch? if
                pop me @ set-standard
                "yes" set_page_standard
                "Page standard format set."
                tell exit
            then
            dup "#prepended" 3 stringmatch? if
                pop me @ set-prepend
                "prepend" set_page_standard
                "Page prepend format set."
                tell exit
            then
            dup "#ping" 3 stringmatch? if
                pop strip
                multi-ping exit
            then
  
            dup "#proploc" 4 stringmatch? if
                pop do-proplock-set exit
            then
  
  
        else
  
$ifndef MAILTYPE=NONE
            dup "#mail" 2 stringmatch? if
$ifdef MAILTYPE=PAGEMAIL
                pop mail-read "Done." tell exit
$else
                pop "page #mail" "" QUICKread exit
$endif
            then
            dup "#check" 3 stringmatch? if
$ifdef MAILTYPE=PAGEMAIL
                pop me @ name multi-check exit
$else
                pop "page #check" me @ name QUICKcheck exit
$endif
            then
$endif
  
            dup "#haven" 3 stringmatch? if
                pop me @ "haven" set
                "Haven bit set." tell
                "Your haven message is \""
                me @ get-havenmsg strcat
                "\"" strcat tell exit
            then
            dup "#away" 3 stringmatch? if
                pop me @ "_page/away" "yes" setprop
                "Away flag set." tell
                "Your away message is \""
                me @ get-awaymsg strcat
                "\"" strcat tell exit
            then
            dup "#sleepmsg" 3 stringmatch? if
                pop "Your sleep message is \""
                me @ get-sleepmsg strcat
                "\"" strcat tell exit
            then
            dup "#idlemsg" 3 stringmatch? if
                pop "Your idle message is \""
                me @ get-idlemsg strcat
                "\"" strcat tell exit
            then
            dup "#idletime" 6 stringmatch? if
                pop "Your idle timeout is "
                me @ get-idletime 60 / intostr strcat
                " minutes." strcat tell exit
            then
            dup "#ignore" 2 stringmatch? if
                "You are currently ignoring "
                list-ignored dup not
                if pop "no one" then
                strcat "." strcat
                tell pop "Your ignore message is \""
                me @ get-ignoremsg strcat "\"" strcat
                tell exit
            then
            dup "#!ignore" 3 stringmatch? if
                "" me @ setignorestr
                "You are now ignoring no one."
                tell pop exit
            then
            dup "#priority" 2 stringmatch? if
                "You are currently prioritizing "
                list-priority dup not
                if pop "no one" then
                strcat "." strcat
                tell pop exit
            then
            dup "#!priority" 3 stringmatch? if
                "" me @ setprioritystr
                "You are now prioritizing no one."
                tell pop exit
            then
            dup "#time" 2 stringmatch? if
                pop "The time is: "
                get-timestr strcat
                tell exit
            then
            dup "#alias" 2 stringmatch? if
                list-personal-aliases
                "Done." tell exit
            then
            dup "#global" 2 stringmatch? if
                list-global-aliases
                "Done." tell exit
            then
            dup "#lookup" 3 stringmatch? if
                "Syntax: page #lookup <name>"
                tell exit
            then
            dup "#formatted" 3 stringmatch? if
                pop "" set_page_standard
                "Pages now received in formatted form."
                tell exit
            then
            dup "#multimax" 3 stringmatch? if
                pop me @ get-multimax
                "You currently accept pages including up to "
                over intostr strcat swap 1 >
                if " people." else " player." then strcat
                tell exit
            then
            dup "#oformat" 3 stringmatch? if
                "Bad #oformat syntax.  Type 'page #help3' for more help."
                tell pop exit
            then
  
$ifndef MAILTYPE=NONE
            dup "#forward" 4 stringmatch? if
$ifdef MAILTYPE=PAGEMAIL
                pop me @ get-forward comma-format
                dup if
                    "You currently forward mail to "
                    swap strcat "." strcat
                else
                    pop "You aren't currently forwarding mail."
                then tell exit
$else
                pop "page #forward" "" QUICKforward exit
$endif
            then
$endif
  
            dup "#standard" 3 stringmatch? if
                pop "yes" set_page_standard
                "Pages now received in the standard form '"
                me @ get-standard strcat "'" strcat
                tell exit
            then
            dup "#prepended" 3 stringmatch? if
                pop "prepend" set_page_standard
                "Pages now received prepended with '"
                me @ get-prepend strcat "'" strcat
                tell exit
            then
            dup "#setup" 3 stringmatch? if
                me @ "wizard" flag? not if
                    "Permision denied." tell exit
                then
                trigger @ "_page/formats/f-page"
                "You page, \"%m\" to %n."  setpropstr
                trigger @ "_page/formats/o-page"
                "%n pages, \"%m\" to %t." setpropstr
                trigger @ "_page/formats/f-pose"
                "You page-pose, \"%i %m\" to %n"  setpropstr
                trigger @ "_page/formats/o-pose"
                "In a page-pose to %t, %n %m" setpropstr
                update-galiases
                "Setup done." tell pop exit
            then
  
            dup "#proploc" 4 stringmatch? if
                pop "Syntax: page #proploc <object>" tell exit
            then
  
        then
        "page: Syntax error: " swap strcat tell
        "Type \"page #help\" for help." tell exit
    then
    dup "=" instr not if
        strip single-space
  
  
$ifdef RESTRICTMULT
        me @ name tolower "guest" 5 strncmp not if
            dup " " instr if
                " " split pop
                "Guests are not allowed to use multi-page." tell
            then
        then
$endif
  
$ifndef MULTISUMMON
        dup " " instr if
            "You cannot send a summons to more than one player at a time." tell
            "Perhaps you forget to put the = in the the page." tell
            pop exit
        then
$endif
  
        multi-summon      (do a summons page)
    else
        "=" split
        strip
        dup pagepose? if
            1 strcut swap pop
            "pose" set-curr-format
        else
            "page" set-curr-format
        then
        swap strip single-space
        dup "!" 1 strncmp not if
            " " split swap
            1 strcut swap pop
            dup not if pop "page" then
            set-curr-format
        then
  
  
$ifdef RESTRICTMULT
        me @ name tolower "guest" 5 strncmp not if
            dup " " instr if
                " " split pop
                "Guests are not allowed to use multi-page." tell
            then
        then
$endif
  
  
        multi-page        (do a message page)
    then
;
  
  
: main
  
    getday setday
  
  
    get-lastversion dup if verstr-parse do-update-props else pop then
  
    page-main
  
  
$ifndef MAILTYPE=NONE
$ifdef MAILTYPE=LIBMAIL
$def mail-count MAILnew
$endif
    me @ mail-count 0 > if
        "You have " me @ mail-count intostr strcat
        " mail messages waiting.  Use 'page #mail' to read."
        strcat tell
    then
$endif
  
  
    get-lastversion dup VERSION strcmp if
        dup if
"Page has been upgraded.  Type 'page #changes' to see the latest mods." tell
            "You last used " over strcat tell
        then
        VERSION set-lastversion
    then pop
;  
.
c
q
@action page=#0=tmp/page
@link $tmp/page=cmd-page
page #setup
