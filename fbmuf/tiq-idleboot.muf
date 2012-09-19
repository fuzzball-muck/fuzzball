@prog tiq-idleboot
1 9999 d
1 i
( tiq-idleboot -- obsolete idle disconnection MUF program      )
(                                                              )
( Part of the fbmuck MUF program archive.                      )
(                                                              )
( Please note, this program is obsolete.  Please see the @tune )
( parameters 'maxidle', 'idle_boot_mesg', and 'idleboot' for a )
( more-easily configurable version of the same functionality.  )
( If you wish to use this version, please set this program     )
( A [Autostart] to automatically start when the MUCK is        )
( started.                                                     )
(                                                              )
( Requirements: M4 [Wizard]   due to use of conboot primitive. )
(               A [Autostart] to automatically start at boot.  )

$echo "tiq-idleboot is obsolete -- please see comments for information!"
$def maxidle 120
$def discon-mesg "Auto-disconnected for inactivity."

: idleboot
  concount begin
    dup while
    dup conidle maxidle 60 * > if
      dup condbref "wizard" flag? not if
$ifdef discon-mesg
        dup discon-mesg connotify
$endif
        dup conboot
      then
    then
    1 -
  repeat
  pop
;

: main
  background
  begin
    600 sleep
    idleboot
  0 until
;
.
c
q
@set tiq-idleboot=W
whisper me=------------------------------------------------------------
whisper me=Please note, this program is obsolete.  Please see the @tune
whisper me=parameters 'maxidle', 'idle_boot_mesg', and 'idleboot' for a
whisper me=more-easily configurable version of the same functionality.
whisper me=If you wish to use this version, please type
whisper me=@set tiq-idleboot=A
whisper me=to set this program to auto-start when the MUCK is started.
whisper me=Please see the program comments for more information.
whisper me=------------------------------------------------------------