TinyMUCK2.2fb4.0 MUF documentation.
===================================

  There are now four levels of MUCKERs in fb4.0.  Level zero is a non-mucker.
    They cannot use the editor, and MUF programs owned by them run as if
    they were level 1 MUCKERs.
      Level one MUCKER's are apprentices.  Their powers are restricted as they
    cannot get information about any object that is not in the same room they
    are.  ie:  OWNER, NAME, LOCATION, etc all fail if the object isn't in the
    same room as the player.  Level one MUCKER programs always run as if they
    are set SETUID.  NOTIFY, NOTIFY_EXCEPT, and NOTIFY_EXCLUDE will refuse to
    send messages to rooms the user is not in.  Level one programs also cannot
    use ADDPENNIES.
      Level two MUCKERs are also called Journeymen.  Their permissions are
    equivalent to the permissions for a normal MUCKER under older versions
    of the server.
      Level three MUCKERs are referred to as Masters.  They can use the con-
    nection info primitives (ie: CONDBREF, ONLINE, etc.), read the EXITS list
    of any room, use NEXTOBJ on objects, can use NEWROOM, NEWOBJECT, NEWEXIT,
    and COPYOBJ without limitations, can use QUEUE and KILL, and can override
    the permissions restrictions of MOVETO.  You only give a player MUCKER
    level 3 if they are very trusted.
      A player who is wizbitted is effectively Mucker Level 4.  MUCKER level
    four is required for the RECYCLE primitive, the CONHOST primitive, the
    FORCE primitive, and the SETOWN primitive.  ML4 also allows overriding
    of permissions of the SET* primitives, and property permissions.  Props
    not listed by NEXTPROP with ML3 are listed with ML4.
      The MUCKER level permissions that a program runs at is the lesser of
    it's own MUCKER level and the MUCKER level of it's owner.
      If it is owned by a player who is MUCKER level 2, and it is MUCKER
    level 3, then it runs at Muckr level 2.  The one exception to this is
    programs owned by a Wizard player.  They run at Mucker level 2 if the
    program itself is not wizbit, and at Mucker level 4 if the program IS
    set wizbit.
      Mucker level is referred to in flags lists by M# where the # is the
    Mucker level.  Level zero objects don't show a flag for it.  Example:
    Revar(#37PM3)
      In verbose flags lists, Mucker levels greater than zero are shown
    by MUCKER# where # is the mucker level.
      To set a level on a player or program, use the level number as the
    flag name.  MUCKER is the same as 2, and !MUCKER is the same as 0.
    Example:  @set Revar=2
      A player may set the MUCKER level on a program they own to any level
    lower than or equal to their own level, and a wizard may set a program
    or player to any MUCKER level.  A program cannot be set to Mucker Level
    Zero, since it has no meaning for programs. (You expect them to use the
    MUF editor or something?)
      When a program is created, it is automatically set to the same MUCKER
    level as the creating player.  When a program is loaded from the db, if
    it is Mucker Level 0, it is upgraded to Mucker Level 2.



Compiler directives for MUF:
    $define <defname> <definition> $enddef
      Basically the same as C's #define <defname> <definition>
    $undef <defname>
      About the same as C's #undef <defname>
    $echo <string>
      Echos the given string to the screen of the person compiling the program.
      Runs at compile-time.
    __version
      A pre$defined macro that contains the current server version.
      Currently "Muck2.2fb3.4"
    $ifdef <condition> <compile this if true> $else <compile if false> $endif
    $ifndef <condition> <compile this if true> $else <compile if false> $endif
      where <condition> is either a $defined name, or a test that consists of
      a $defined name, a comparator (=, <, or >) and a test value, all in one
      word without space.  The $else clause is optional.  Compiler directives
      are nestable also.  Some examples:
      $ifndef __version>Muck2.2fb3.5 $define envprop .envprop $enddef $endif
      $define ploc $ifdef proplocs .proploc $else $endif $enddef
    $include <dbref|progreg>
      Sets a bunch of $defines from the properties in the /_defs/ propdir.
      For example, if object #345 had the following properties:
        /_defs/desc: "_/de" getpropstr
        /_defs/setpropstr: dup if 0 addprop else pop remove_prop then
        /_defs/setpropval: dup if "" swap addprop else pop remove_prop then
        /_defs/setprop: dup int? if setpropval else setpropstr then
      then if a program contained '$include #345' in it, then all subsequent
      references to 'desc', 'setpropstr', 'setpropval', and 'setprop' would
      be expanded to the string values of their respective programs. ie:
      'desc' would be replaced throughout the program with '"_/de" getpropstr'
  
  You can now escape a token in MUF so that it will be interpreted literally.
    ie:  \.pmatch will try to compile '.pmatch' without expanding it as a
    macro.  This lets you make special things with $defines such as:
    $define addprop over over or if \addprop else pop pop remove_prop $enddef
    so that all the 'addprop's in the program will be expanded to the
    definition, but the 'addprop' in the definition will not try to expand
    recursively.  It will call the actual addprop.
  
  

Multitasking:
  There are now 3 modes that a program can be in when running:  foreground,
    background, and preempt.  A program running in the foreground lets other
    users and programs have timeslices (ie multitasking), but blocks input
    from the program user.  Background mode allows the user to also go on and
    do other things and issue other commands, but will not allow the program
    to do READs.  Preempt mode means that the program runs to completion
    without multitasking, taking full control of the interpreter and not
    letting other users or progs have timeslices, but imposes an instruction
    count limit unless the program is a wizard program.
  Programs run by @locks, @descs, @succs, @fails, and @drops default to the
    preempt mode when they run.  Programs run by actions linked to them
    default to running in foreground mode.  QUEUEd program events, such as
    those set up by _listen, _connect, _disconnect, etc, and those QUEUEd by
    other programs default to running in background mode. (NOTE: these
    programs cannot be changed out of background mode)



New primitives:
  PUBLIC <functionname>  Declares a function to be public for execution by
                          other programs.  This is a compile-time directive,
                          not a run-time primitive.  To call a public
                          function, put the dbref of the program on the
                          stack, then put a string, containing the function
                          name, on the stack, then use CALL.  For example:
                          #888 "functionname" CALL
  
  LVAR <varname>   This declares a variable as a local variable, that is
                    local to a specific program.  If another program calls
                    this program, the values of the local variables will not
                    be changed in the calling program, even if the called
                    program changes them.
  
  LOCALVAR  (i -- l)    Takes an integer and returns the respective local
                          variable.  Similar to the 'variable' primitive.


  ABORT    (s -- )  Halts the interpreter with an error of the string given.

  SETLINK   (d d -- )   Takes an exit dbref and a destination dbref and sets
                         the exit link to that destination.  Note that if the
                         exit is already linked, it must be unlinked by doing
                         a setlink with #-1 as a destination.
  
  SETOWN    (d d -- )   Sets the ownership of the first object to the player
                         given in the second dbref. (wizbit only)
  
  NEWROOM   (d s -- d)  Takes dbref of the location, and the name in a string.
                          Returns the dbref of the room created.  Owner is
                          runner of the program. (Mucker Level 3)
  
  NEWOBJECT (d s -- d)  Takes location and name and returns new thing's dbref.
                          (Mucker Level 3)
  
  NEWEXIT   (d s -- d)  Takes location and name and returns new exit's dbref.
                          (Requires Mucker Level 3)
  
  RECYCLE   (d -- )    Recycles the given object d.  Will not recycle
                        players, the global environment, the player
                        starting room, or any currently running program.
                        (Can recycle objets owned by uid if running with
                        Mucker Level 3 permissions.  Can recycle other
                        people's items with wizbit)

  STATS (d -- 7 ints)  Takes a dbref and returns 7 integers, giving, in order,
                        The total number of objects owned by the given dbref,
                        the number of rooms owned, the number of exits owned,
                        the number of things owned, number of programs owned,
                        number of players, and number of garbage items owned.
                        If the given dbref was #-1, then the stats are the
                        totals for the entire database. (Needs Mucker Level 3)



  AWAKE?    (d -- i)    Returns whether the given player (dbref) is connected.
                         It returns how many times they are connected.
  
  ONLINE    ( -- d d..d i)  Returns the dbrefs of all the players logged in,
                         with the count of them on the top of the stack.
                         (Requires Mucker Level 3)
  
  CONCOUNT  ( -- i)     Returns how many connections to the server there are.
                         (Requires Mucker Level 3)
  
  CONDBREF  (i -- d)    Returns the dbref of the player connected to this
                         connection.  (Requires Mucker Level 3)
  
  CONTIME   (i -- i)    Returns how many seconds the given connection has been
                         connected to the server.  (Requires Mucker Level 3)
  
  CONIDLE   (i -- i)    Returns how many seconds the connection has been idle.
                         (Requires Mucker Level 3)
  
  CONDESCR  ( i -- i )  Takes a connection number and returns the descriptor
                         number associated with it.  (Requires Mucker Level 3)
  
  DESCRCON (i -- i)  Takes a descriptor and returns the associated connection
                      number, or 0 if no match was found.

  DESCRIPTORS (d -- ix...i1 i) Takes a player dbref, or #-1, and returns the
                                range of descriptor numbers associated with
                                that dbref (or all for #-1) with their count
                                on top.

  CONHOST   (i -- s)    Returns the hostname of the connection. (wizbit only)
  
  CONBOOT   (i -- )    Takes a connection number and disconnects that
                        connection from the server.  Basically @boot for
                        a specific connection. (wizbit only)

  CONNOTIFY (i s -- )  Sends a string to a specific connection to the
                        server.  (Requires Mucker Level 3)
                         


  INT?      (? -- i)    Tells whether the top stack item is an integer.
  
  STRING?   (? -- i)    Tells whether the top stack item is a string.
  
  DBREF?    (? -- i)    Tells whether the top stack item is a dbref.
  


  NEXTPROP  (d s -- s)  This takes a dbref and a string that is the name of a
                         property and returns the next property name on that
                         dbref, or returns a null string if that was the last.
                         To *start* the search, give it a propdir, or a blank
                         string. For example, '#10 "/" NEXTPROP'  or 
                         '#28 "/letters/" NEXTPROP'   A blank string is the
                         same as "/".  (Requires Mucker Level 3)
  
  The NEXTPROP primitive will skip over property names that the program
    and user would not have permissions to read.  Wizbit programs can see all
    of the properties on an object.  Otherwise, nextprop will skip over the
    props that are not readable by the uid the program is running under.


  PROPDIR?  (d s -- i)  Takes a dbref and a property name, and returns whether
                         that property is a propdir that contains other props.
                         (Requires Mucker Level 3)

  ENVPROPSTR (s d -- s d )  Takes a starting object dbref and a property name
                             and searches down the environment tree from that
                             object for a property with the given name.
                             If the property isn't found, it returns #-1 and
                             a null string.  If the property is found, it will
                             return the dbref of the object it was found on,
                             and the string value it contained.
  


  NOTIFY_EXCLUDE (d dn ... d1 n s -- )  Displays the message s to all the
                             players (or _listening objects), excluding the
                             n given players, in the given room.  Example:
                             #0 #1 #23 #7 3 "Hi!" notify_exclude
                             would send "Hi!" to everyone in room #0 except
                             for players (or objects) #1, #7, and #23.

  NOTIFY_EXCEPT has been removed and replaced by a $define to NOTIFY_EXCLUDE.
    It is effectively $define notify_except 1 swap notify_exclude $enddef

  NOTIFY and NOTIFY_EXCLUDE will not trigger _listen's that would run the
    current program.

  
  
  SYSTIME   ( -- i)     Returns the number of seconds since 1/1/79 00:00 GMT
  
  TIMEFMT   (s i -- s)  Takes a format string and a SYSTIME integer and returns
                         a string formatted with the time.  The format string
                         is ascii text with formatting commands:
                           %% -- "%"
                           %a -- abbreviated weekday name.
                           %A -- full weekday name.
                           %b -- abbreviated month name.
                           %B -- full month name.
                           %C -- "%A %B %e, %Y"
                           %c -- "%x %X"
                           %D -- "%m/%d/%y"
                           %d -- month day, "01" - "31"
                           %e -- month day, " 1" - "31"
                           %h -- "%b"
                           %H -- hour, "00" - "23"
                           %I -- hour, "01" - "12"
                           %j -- year day, "001" - "366"
                           %k -- hour, " 0" - "23"
                           %l -- hour, " 1" - "12"
                           %M -- minute, "00" - "59"
                           %m -- month, "01" - "12"
                           %p -- "AM" or "PM"
                           %R -- "%H:%M"
                           %r -- "%I:%M:%S %p"
                           %S -- seconds, "00" - "59"
                           %T -- "%H:%M:%S"
                           %U -- week number of the year. "00" - "52"
                           %w -- week day number, "0" - "6"
                           %W -- week# of year, starting on a monday,
                                   "00" - "52"
                           %X -- "%H:%M:%S"
                           %x -- "%m/%d/%y"
                           %y -- year, "00" - "99"
                           %Y -- year, "1900" - "2155"
                           %Z -- Time zone.  "GMT", "EDT", "PST", etc.
 
  TIMESPLIT (i -- i i i i i i i i)  Takes a systime integer, and splits it up
                         into second, minute, hour, dayofmonth, month, year,
                         dayofweek, and dayofyear.  1 == sunday for dayofweek.
                         Dayofyear is a number from 1 to 366.
  
  TIMESTAMPS ( d -- i i i i )   Takes the dbref of an object, and returns
                                 the four timestamp ints in the order of
                                 Created, Modified, Lastused, and Usecount
                                 where the three times are in systime
                                 notation.
  
  SLEEP   (i -- )     makes the program pause here for 'i' seconds.

  QUEUE     (i d s -- i) Takes a time in seconds, a program's dbref, and a
                          parameter string.  It will execute the given program
                          with the given string as the only string on the
                          stack, after a delay of the given number of second.
			  Returns pid of the queued process.  QUEUE'd
			  programs cannot use the READ primitive.  They are
			  writeonly.  When a QUEUEd program executes, it does
			  so with me @ set to the player who was running the
			  program that QUEUEd it, a loc @ of #-1, and a
			  trigger @ of #-1.
                          (Requires Mucker Level 3)

  FORK  ( -- i)  This primitive forks off a BACKGROUND (muf) process from
                   the currently running program.  It returns the pid of
                   the child process to the parent process, and returns a
                   0 to the child.  This does NOT do a UNIX fork.  It only
                   copies the current stack frame and puts the background
                   muf process on the time queue. (Requires Mucker Level 3)

  KILL       (i -- i)  Attempts to kill the given process number.  Returns 1
                        if the process existed, and 0 if it didn't.
                        (Requires Mucker Level 3)

  ISPID?  (i -- i)  Takes a process id and checks to see if an event with that
                     pid is in the timequeue.  It returns 1 if it is, and 0 if
                     it is not.  NOTE: since the program that is running is not
                     on the timequeue WHILE it is executing, but only when it
                     is swapped out letting other programs run, 'pid ispid?'
                     will always return 0.
  


  TOUPPER    (s -- s)  Takes a string and returns it with all the letters
                        in uppercase.
  
  TOLOWER    (s -- s)  Takes a string and returns it with all the letters
                        in lowercase.
  
  STRIPLEAD  (s -- s)  Strips leading spaces from the given string.

  STRIPTAIL  (s -- s)  Strips trailing spaces from the given string.

  STRIP      (s -- s)  This is a built in $define.  It is read as
                        "striplead striptail"

  SMATCH  ( s s -- i )  Takes a string and a string pattern to check against.
                         Returns true if the string fits the pattern.  In the
                         pattern string, a '?' matches any single character,
                         '*' matches any number of characters. Word matching
                         can be done with '{word1|word2|etc}'.  If multiple
                         characters are in [], it will match a single
                         character that is in that set of characters.
                         '[aeiou]' will match a single character if it is a
                         vowel, for example.  To search for one of these
                         special chars, put a \ in front of it to escape it.
                         It is not case sensitive.  Example pattern:
                         "{Foxen|Lynx|Fiera} *t[iy]ckle*\?"  Will match any
                         string starting with 'Foxen', 'Lynx', or 'Fiera',
                         that contains either 'tickle' or 'tyckle' and ends
                         with a '?'.
  


  SETLOCKSTR (d s -- i)  Tries to set the lock on the given object to the
                          lock expression given in the string.  If it was a
                          success, then it will return a 1, otherwise, if
                          the lock expression was bad, it returns a 0.  To
                          unlock an object, set its lock to a null string.
  
  GETLOCKSTR ( d -- s )  Returns the lock expression for the given object
                          in the form of a string.  Returns "*UNLOCKED*" if
                          the object doesn't have a lock set.
  
  LOCKED? (d d -- i)  Takes, in order, the dbref of the object to test the
                        lock on, and the dbref of the player to test the lock
                        against.  It tests the lock, running programs as
                        necessary, and returns a integer of 0 if it is not
                        locked against them, or 1 if it is.

  UNPARSEOBJ ( d -- s )  Returns the name-and-flag string for an object.
                          For example: "One(#1PW)"
  


  DBTOP     ( -- d)     Returns the dbref of the first object beyond the top
                          object of the database.
  
  DEPTH     ( -- i)     Returns how many items are on the stack.
  
  VERSION   ( -- s)     Returns the version of this code in a string.
                          "Muck2.2fb4.0" is the current version.
  
  PROG      ( -- d)     Returns the dbref of the currently running program.
  
  TRIG      ( -- d)     Returns the dbref of the original trigger.
  
  CALLER    ( -- d)     Returns the dbref of the program that called this one,
                          or the dbref of the trigger, if this wasn't called
                          by a program.
  


  BITOR     (i i -- i)  Does a logical bitwise or.
  
  BITXOR    (i i -- i)  Does a logical bitwise exclusive or.
  
  BITAND    (i i -- i)  Does a logical bitwise and.
  
  BITSHIFT  (i i -- i)  Shifts the first integer by the second integer's
                          number of bit positions.  Same as the C << operator.
                          If the second integer is negative, its like >>.
  



  FORCE     (d s -- )  Forces player d to do action s as if they were
                        @forced.  (wizbit only)
                         
  PREEMPT   ( -- )     Prevents a program from being swapped out to do
                        multitasking.  Needed in some cases to protect
                        crutial data from being changed while it is being
                        worked on.  A program will remain in preempt mode
                        until it's execution is completed.  Basically what
                        this command does is to turn off multitasking, but
                        then you have a limit on how many instructions you
                        can run without needing either to pause with a SLEEP,
                        or have a wizbit on the program.

  FOREGROUND ( -- )  To turn on multitasking, you can issue a foreground
                      command.  While a program is in foreground mode, the
                      server will be multitasking and handling multiple
                      programs at once, and input from other users, but it
                      will be blocking any input from the user of the program
                      until the program finishes.  You cannot foreground a
                      program once it is running in the background. A program
                      will stay in foreground mode until it finishes running
                      or until you change the mode.

  BACKGROUND ( -- )  Another way to turn on multitasking is to use the
                      background command.  Programs in the background let
                      the program user go on and be able to do other things
                      while waiting for the program to finish.  You cannot
                      use the READ command in a background program.  Once a
                      program is put into background mode, you cannot set
                      it into foreground or preempt mode.  A program will
                      remain in the background until it finishes execution.




  BEGIN   ( -- )      Marks the beginning of begin-until or begin-repeat loops

  UNTIL   (i -- )     If the value on top of the stack is false, then it jumps
                        execution back to the instruction afer the matching
                        BEGIN statement.  (BEGIN-UNTIL, BEGIN-REPEAT, and
                        IF-ELSE-THEN's can all be nested as much as you want.)
                        If the value is true, it exits the loop, and executes
                        the next instruction, following the UNTIL.  Marks the
                        end of the current loop.

  REPEAT  ( -- )      Jumps execution to the instruction after the BEGIN in a
                        BEGIN-REPEAT loop.  Marks the end of the current loop.
  
  WHILE      (i -- )   If the value on top of the stack is false, then this
                        causes execution to jump to the instruction after the
                        UNTIL or REPEAT for the current loop.  If the value is
                        true, however, execution falls through to the instr-
                        uction after the WHILE.

  BREAK      ( -- )    Breaks out of the innermost loop.  Jumps execution to
                        the instruction after the UNTIL or REPEAT for the
                        current loop.

  CONTINUE   ( -- )    Jumps execution to the beginning of the current loop.

    The BEGIN statement marks the beginning of a loop.
    Either the UNTIL or the REPEAT statement marks the end of the loop.
      REPEAT will do an unconditional jump to the statement after the BEGIN
        statement.
      UNTIL checks to see if the value on the stack is false.  If it is, it
        jumps execution to the statement after the BEGIN statement, otherwise,
        it falls through on execution to the statement after the UNTIL.
    Within a loop, even within IF-ELSE-THEN structures within the loop
      structure, you can place WHILE, CONTINUE, or BREAK statements.  There
      is no limit as to how many, or in what combinations these instructions
      are used.
      A WHILE statement checks to see if the value on the stack is false.
        If it is, execution jumps to the first statement after the end of
        the loop.  If the value was true, execution falls through to the
        statement after the WHILE.
      The CONTINUE statement forces execution to jump to the beginning of
        the loop, after the BEGIN.
      The BREAK statement forces execution to jump to the end of the loop,
        at the statement after the REPEAT or UNTIL, effectively exiting the
        loop.
    Note: You can nest loops complexly, but WHILE, BREAK, and CONTINUE
      statements only refer to the innermost loop structure.

    Example of a complex loop structure:
      101 begin                       (BEGIN the outer loop)
        dup while 1 -                 (This WHILE, ...)
        dup not if break then         (this BREAK, and..)
        dup 2 % not if continue then  (this CONTINUE refer to the outer loop)
        dup 10 % not if
          15 begin                    (BEGIN inner loop)
            dup while 1 -             (This WHILE, and.. )
            dup 5 % not if break then (... this BREAK, refer to inner loop)
          repeat                      (This REPEAT statement ends inner loop.)
        then
        dup 7 % not if continue then  (This CONTINUE, and...)
        dup 3 % not if dup 9 % while then (this WHILE refer to the outer loop)
        dup intostr me @ swap notify
      dup 1 = until pop               (This UNTIL ends the outer loop)
  
  


  On dbload, if a program is set ABODE (AUTOSTART), *AND* it is owned by
    a wizard, then it will be placed in the timequeue with a delay of 0 and
    a string parm of "Startup".  Autostart programs run with the location
    NOTHING (#-1) rather than the location of the owner of the program.

  You can now check the Interactive flag on a player in MUF to see if they
    are in READ mode or @edit'ing.
  
  MOVETO will now run programs in the @desc and @succ/@fail of a room when
    moving a player.
  
  When a message is notify_except'ed or notify_exclud'ed to a room, and
    LISTENERS and LISTENERS_ENV are defined, then it will run ALL the
    programs referred to in all the _listen properties down the environment
    tree.  Also, the muf NOTIFY primitive was changed to run the _listen
    program on an object or player if a message is sent to them that way.
  
  Removed DESC, SUCC, FAIL, DROP, OSUCC, OFAIL, ODROP, SETDESC, SETSUCC,
    SETFAIL, SETDROP, SETOSUCC, SETOFAIL, and SETODROP and replaced them with
    $defines that would set/get the appropriate information in/from properties.
    For example, DESC is now translated to '"_/de" getpropstr' and SETDESC is
    translated to '"_/de" swap 0 addprop'.  One result of this is that if you
    tried to escape one of these command names with a backslash in a macro
    definition, it wouldn't work.  ie:  \SETDESC would get a compile error.
  
  There is a COMMAND variable, similar to ME, LOC, and TRIGGER, except that
    it contains a string.  The string contains the command the user typed
    that triggered the the program, without the command line arguments.  ie:
    if there was an exit named "abracadabra;foo bar;frozzboz" that was linked
    to the program, and the user typed in "foo bar baz", then the program
    would run with "baz" on the stack, and "foo bar" in the global COMMAND
    variable.
  
  Made ROOM?, PLAYER?, THING?, EXIT?, and PROGRAM? primitives return false
  if the object passed to it is invalid, instead of aborting interpreting.
  
  INSTRING and RINSTRING are now inserver defines that let you do case
    insensitive versions of their instr and rinstr counterparts.

Programs are now compiled when they are run or called instead of when
  the databate is loaded.  They are compiled with the uid of the owner
  of the program.
  
If a program has the HAVEN flag set on it (HARDUID) then it runs with
  the uid and permissions of the owner of the trigger of the program.
  If the program is a timequeue event (with trigger of #-1), then it
  will run with the permissions and uid of the program owner as in SETUID.
  
A room or player may have a "_connect" property set that contains the
  dbref of a progran to run when a player connects.  The program must be
  either link_ok or must be owned by the player connecting.  When the
  program is run, the string on the stack will be "Connect", the "loc @"
  will be the location of the connecting player, the "me @" will be the
  connecting player, and the "trigger @" (and "trig") will be #-1.  All
  programs referred to by _connect properties on the player, and on rooms
  down the environment tree from the player, will be QUEUEd up to run.
  When a player desconnects, programs referred to by _disconnect properties
  will be run in a similar manner.
  (connect and disconnect _actions_ are also implemented.)
  
Programs refered to by props in _depart/_arrive/_connect/_disconnect propdirs
  will now be all queued up, eliminating the need for a dispatcher program.
  An example would be _connect/announce:1234  That would queue up program
  #1234 when a player connects.  The name ("announce") is not important, and
  can be anything you want, but they are executed in alphabetic order.

Programs set BUILDER (BLOCKED) run in preempt mode, regardless of the mode
  of the program.  ie: a foreground program, while running in a program set
  BLOCKED, will run pre-empt, with the multitasking effectively shut off.

Programs set HARDUID and SETUID, that are owned by a Wizard, run at the same
  mucker level as the calling program.  This is useful for writing libraries.

Tabs sent to the server are now interpreted as single spaces to help solve
  a problem with tabbed comments in MUF programs being uploaded.

