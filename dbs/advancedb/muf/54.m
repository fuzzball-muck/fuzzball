( cmd-@mpi+ 1.2 by Natasha@HLM
 
  This program {C} 1998-2002 Natasha O'Brien, and is released under
  the GNU General Public License. https://www.gnu.org/copyleft/gpl.html
  for the license agreement.
 
    __version history
  1.0, circa 1998:  first version
  1.1, 28 May 2000: Modified to support Jaffa's '@mpi #null' syntax for quiet
                    parsing; commented, better spacing.
  1.2, 28 May 2002: Added Fuzzball support.
  1.3, 6 June 2002: Added wizards-only @wmpi for executing blessed MPI.
)
( If you're using a GlowMuck, uncomment this. )
($def Glow)
 
lvar v_loud?
: main  ( str -- }  Parses str for MPI and displays the result. )
 
    ( The program uses two display sections with the parsing in the middle,
      so if the code gets an error, we already displayed what they typed, so
      if the user typoed or something, they can see. )
 
    command @ "do" stringcmp not var! v_do?
 
    ( Should we be quiet? )
    dup if dup "#null " stringpfx else 0 then if
        ( Strip out the '#null ' if it was there. )
        6 strcut swap pop  ( str )
        0  ( str boolLoud? )
    else
        command @ tolower "q" instr v_do? @ or not  ( str boolLoud? )
    then  ( str boolLoud? )
    ( Remember for when we need to Result:. )
    dup v_loud? !  ( str boolLoud? )
    if  ( str )
        "Command: \"" over strcat "\"" strcat .tell  ( str )
    then  ( str )
 
    me @ "_temp/mpi" rot setprop  (  )
    command @ "@wmpi" strcmp if 0 else me @ "wizard" flag? then if  (  )
        me @ "_temp/mpi" blessprop  (  )
    then  (  )
    me @ "_temp/mpi"  ( db str )
$def parsempi \parseprop
    ( Parse! )
    "(" command @ strcat ")" strcat 0  ( db strMpi strHow intDelay )
    parsempi  ( str' )
 
    v_do? @ if
        dup if
            me @ over force
        else
            "Nothing to do." .tell
        then
    then  ( str' )
 
    ( Display the result if we didn't get an error. )
    v_loud? @ if  ( str' )
        " Result: \"" over strcat "\"" strcat .tell  (  )
    then  ( str' )
 
    pop  (  )
 
;
