To Do list
----------

Current Planned Tasks for fb6.00 (9/30/96):
	[updated 06/12/2005 by Winged with STATUS info]

    Add error trapping to MUF.  ie: ERRCATCH-ERRHANDLE-ERREND blocks.
STATUS: Done TRY/CATCH/ENDCATCH blocks.
    Save out changed objects to temp file on prop-loaded queue ejection.
STATUS: ?What does this refer to?
    Eliminate DBSAVE lockups by caching changes in memory, and saving in BG.
STATUS: Worked around with the magic of fork()/vfork()
    Add threadsafe DB_LOCKING
STATUS: Not a chance in hell -- unless we use db4 or such.

    Make free frames pool dynamic in size, based on use timestamp.
STATUS: Unknown

    Add MUF prim to return how a MUF program instace was instantiated.
STATUS: We have the initial strings on the stack, but that's not
		technically "safe".  Still needs work.
    Work with Points and Fre'ta to improve inserver Vehicle handling.
STATUS: What do they want to do with it?
    Perhaps change MOVETO to do conlock checking?
STATUS: Should be done.

    Investigate inserver player command macros.
STATUS: Not done.  Can be imitated with {force:me,"string\nstring\nstring"}.
    Error msgs in MUF interpreter need cleanup.
STATUS: Most seem to be better, but still need full audit.
    Make Netmuck simpler to compile/install.  Especially for doing upgrades.
STATUS: If we could figure out a consistent way to do so, it'd be
	possible.  At least we have 'make install', which will install
	the binaries and shared help files.
    Investigate forbidding X flags on Wizbitted objects.
STATUS: Obsoleted via BLESS.
    Allow exiting from the MUF editor without saving.
STATUS: Undone.
    Allow some way to execute commands from inside the muf editor. ie: allow
      poses, says, pages, etc from inside the editor.
STATUS: MCP-GUI allows this, but the editor itself needs a lot of work.

    Add DESCR* prims to replace/supplement CON* prims.
STATUS: Done.
    Add keyhole optimizer to MUF.
STATUS: Done.
    Add standardized ~sys/cmd/@teleport/osucc type mesgs for inserver cmds.
STATUS: Not done.
    Add lock checking for inserver commands.
STATUS: Give me a list of inserver commands to add locks for, and I'll do it.

    Add NEWPLAYER and TOADPLAYER primitives to MUF.
STATUS: Done.  TOADPLAYER is a SCARY_MUF_PRIMITIVE.
    Add {moveto} to MPI.
STATUS: Not done.  Would this need blessed permissions?
    Add {cutlist} primitive for MPI.  Returns all items BUT the given sublist.
STATUS: Not done.  We need to integrate MUF and MPI array handling.

    Document what command @ and stack are in 'man [dis]connect', etc.
STATUS: Not done.
    Document running order for @succ, etc & _arrive, _listen, _depart etc.
STATUS: Not done.

    Make MPI use allocated strings instead of BUFFER_LEN argument buffers.
STATUS: Done.
    Improve prop deletion schemes for protected props.
STATUS: ?what does this refer to?
    Add process listing/management primitives.
STATUS: We have KILL, and WAITFOR_PID, but still not any good arrays.
    Clean up the struct specific fields of objects.
STATUS: Not done.  We also need to make sure that we're checking any
		reference of those fields against NULL, to avoid crashes.

    WAITPROP (d s -- s)  Waits for deletion or change of given property.
STATUS: Not done, propose an event class of PROP.*
+   TRIGDESCR ( -- i) relates the descriptor that started this program.
STATUS: Done. (DESCR prim)

    Implement following MUF prims: Make @tunable format str for list formats.
~	GETLISTRANGE (i i d s -- {s})
~	SETLISTRANGE ({s} i d s -- )
    CLEARLISTRANGE (i i d s -- )
	MOVELISTRANGE (i i i d s -- )
STATUS: ?what does this refer to?  Is it superseded by array primitives?

    Add a way to let programs edit and compile programs.
STATUS: Done.
    Make Properties store privs and ownership.
STATUS: Not done.  This would bloat the database, including essentially a
		boolexp and dbref with every property.  As well, toading would
		suddenly become a nightmare as all properties are loaded and
		ownership checked.

+   Make MUF ARRAYs that are dynamically allocated.
STATUS: Done, but not completely safely.




{tell}s > prepending may be broken.  Look into it.  Tell Timur when fixed.
STATUS: ?what does this refer to?

Risseth (Yesterday at 8:45:43PM) -- I found something that I want fixed..  
 Parseprop strips spaces from the beginning and end of a string AFTER parsing 
 it..  It shouldn't, only before.
STATUS: Not done (to my knowledge).

Kimi okays. A way for _listen to know if an @message/@omessage triggered them,
 first thing. This can be done in the command variable, she thinks. Also, a way
 for _listen to know if it was another program's notify or notify_exclude that
 triggered the _listen, hopefully being able to know WHICH program, as well, as
 say/pose muf's would make the former useless.
STATUS: Not done, but it should be possible.  It'll just take a bit of deep
		hacking.

Current:
    Document the _sys/ properties somewhere.
STATUS: Not done.
-   Add .rsplit to $lib/strings
STATUS: Unknown
    Make Huh? sysparm message exec for MPI, with dbref 0 perms.
STATUS: Not done.  I propose a change to this request: #0 perms, with a
		secondary @tune to state whether to evaluate blessed or not.
		Perhaps superseded, though, by HUH? M3 exit-in-environment
		patch.

    Change compiler to bytecode style.  Use fn ptrs.
STATUS: Not Done.  Probably not needed.  Would like to change it
		to be a lot more like MPI's layout.

+   Make server force ZOMBIES to look whenever it moves them.  All cases.
STATUS: Unknown, but I think it's done.
+   Make dropto treat zombies like players for triggering.
STATUS: Unknown -- I'm not sure what this refers to. (oh, wait... now I
		know.  If an object is a THING, and there are no players, and
		there's a dropto, all THINGs get swept to the dropto.  If the
		THING is a Zombie, though, it should be treated like an interactive
		player.)
?   Make dropto not drop zombies.
STATUS: As in, zombies are treated as players for purposes of determining
		if they should be dropto'ed, and as long as they are there (and
		set ZOMBIE) they prevent dropto from occurring?

?   Document the CONTROLS and SYSPARM muf primitives.
STATUS: Unknown.
+   Write mpi-intro docs. (mostly done)
STATUS: 2/3rds Done.  We need advanced MPI docs.

?   Make pose strip starting space for ',:;!?. or space.
STATUS: Not sure, will check.  Not really needed, with MUF.

    Make building quotas.
STATUS: Not done.
    Make _verb/ and _overb/ propqueues.
STATUS: ?What does this refer to?  When a player spouts a verb that's
		not recognized by the server, it runs those propqueues?
    Implement control Zones.
STATUS: Is this like Realms Control? -- or like the Group object type
		that I proposed on fbmuck-devel?
    Implement inserver 'look player=object' command, with apropriate lock.
STATUS: Unknown.
    Implement {case:} to act as case statement.
STATUS: Not done.

    Add PROPMATCH (dObj sPropdir sString -- sPropname) exitname style match
STATUS: Not done.
    Add NEXT_ENTRANCE (dLinkedto dLastobj -- dNextobj) to scan inlinks.
STATUS: Backported from ProtoMUCK, I believe.
    Add PCREATE (s s -- d)
STATUS: Done.
	Add TOADPLAYER (d d -- )
STATUS: Done -- SCARY_MUF_PRIMITIVES
	Add NEWPASSWD (d s s -- i) prims.
STATUS: Unknown.
    LOCKEDPROP?
STATUS: Relies on bloated database, as referenced above.
    PARSEMPISTR
STATUS: Not done.  Not sure what the MPI perms should be -- trig's? prog's?
		uid's?
    Add SEED prim to set te random number seed.
STATUS: Every frame has its own seed, so I think it's done.


  
KNOWN BUGS:
+   SETLINK should allow linking exits and rooms to #-3
STATUS: I can see linking exits to #-3. Rooms linked to #-3 would just send
		their dropped stuff to the object's home?
?   Make recursion limit in boolexp parsing.  ie no (((...((((( to crash with.
STATUS: Unknown
+   Make internal MUF addresses use dbref+index instead of memory pointer.
STATUS: Undone.


PROBLEMS:
?   Need to make FORCE prim log forces to status file.
STATUS: Done.
    Make it impossible to trigger an _listen in a prog that was itself
    triggered by an _listen propqueue.
STATUS: Not done, but should be fairly easy.
?   M1, M2, M3, &c should be be flag aliases for 1, 2, 3, &c.
STATUS: Not done, but is just part of command-line parsing.
    Have Timequeues store command @ variable value.
STATUS: Unknown.
    Need a CON_PRIMS_MLEVEL @tune that sets the MLevel CON* and ONLINE run at.
STATUS: Not done.  Would also need to apply to DESCR*.
    Finish the documentation, especially MUF.manual and MUF-tutorial.
STATUS: Not done.


PRIMITIVES:
    MATCH_EXIT (s -- d)
STATUS: Why MATCH_EXIT, when we could have MATCH_TYPE?
    RMATCH_EXIT (d s -- d)
STATUS: Why RMATCH_EXIT, when we could have RMATCH_TYPE?
    OMATCH (d s -- d) match from remote object's POV.  Use player for me&here.
STATUS: Wouldn't this be RMATCH, following the above naming scheme?


ENHANCEMENTS:
    Let smatch() return the unknown parameters.
STATUS: Not done.  Would be similar to what \1, \2, etc is for regsub.
-   Add in Anon funcs. : Fn "cat" "dog" "bat" 3 ': strcmp > ; .sort ;
STATUS: Not done.
-   Make ticked macros and prims compile to anony funcs.
STATUS: Not done.


COMMANDS:
    @unlink <obj>[=<newdest>]  to unlink and relink in one.
STATUS: Not done.  Need to re-examine, and especially look at metalinks.


CODE CLEANUP:
    Make system only props that are protected.
STATUS: @sys/* exist, but can still be overwritten by wizards.
	Move passwords into sysprops.
STATUS: Not done.  (Pennies are in sysprops, though.)
    Make properties use skiplists. (?)
STATUS: Not done.  Not needed.
    Make PUBLIC declarations use faster structure.
STATUS: Not done.



