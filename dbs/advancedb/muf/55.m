( cmd-@muf by Natasha@HLM
  Runs MUF code you enter. A MUF version of @mpi.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.0
$note Runs MUF code you enter. A MUF version of @mpi.
 
: rtn-pretty  ( arr -- str }  Return a prettily formatted string displaying all the contents of arr. )
    "" swap  ( str arr )
    dup dictionary? var! isDict  ( str arr )
 
    foreach  ( str ?key ?val )
        ( Format a special type? )
        dup string? if  ( str ?key strVal )
            "\\\"" "\"" subst
            "\"%s\"" fmtstring  ( str ?key strVal )
        then  ( str ?key ?val )
        dup dbref? if "%d" fmtstring then
        dup array? if rtn-pretty "{%s}" fmtstring then  ( str ?key ?val )
 
        swap isDict @ if
            ", %~:%~"
        else
            pop ", %~"
        then fmtstring  ( str -str )
 
        strcat  ( str )
    repeat  ( str )
    2 strcut swap pop
;
 
: main  ( str -- )
    0 var! debug
    striplead dup "#" stringpfx if  ( str )
        1 strcut swap pop  ( str )
        " " split swap  ( str strCommand )
        "debug" stringcmp if
            pop .showhelp exit  (  )
        else
            1 debug !  ( str )
        then  ( str )
    then  ( str )
 
    ( The user must be a player, and must have an Mlevel. )
    me @ player? me @ mlevel and not if  ( str )
        "@muf is only available to users with a MUCKer bit." .tell
        pop exit  (  )
    then  ( str )
 
    ( Make a new program and set its contents. )
    random intostr "@@muf.muf" strcat newprogram  ( str dbProg )
    swap var! programtext  ( dbProg )
 
    0 try  ( dbProg )
 
        dup programtext @  ( dbProg dbProg str )
        ": main %s ;" fmtstring 1 array_make program_setlines  ( dbProg )
 
        ( Program saved. Compile it. )
        dup 1 compile if  ( dbProg )
 
            ( Yay, compiled! Set it D if desired. )
            debug @ if dup "d" set then
 
            ( Run. )
            depth var! howDeep
            "" var! resultStr
            0 var! error
            0 try  ( dbProg )
                dup call  ( dbProg ... )
            catch_detailed  ( dbProg dictErr )
                dup "program" array_getitem 3 pick = if  ( dbProg dictErr )
                    "Error: %[instr]s: %[error]s"  ( dbProg dictErr strMsg )
                else
                    "Error: %[instr]s on line %[line]i of %[program]D(%[program]d): %[error]s"  ( dbProg dictErr strMsg )
                then  ( dbProg dictErr strMsg )
                swap 1 array_make swap array_fmtstrings 0 array_getitem  ( dbProg strMsg )
                .tell  ( dbProg )
 
                1 error !  ( dbProg )
            endcatch  ( dbProg [...] )
 
            error @ not if  ( dbProg ... )
                depth howDeep @ - array_make rtn-pretty  ( dbProg str )
                "Result: " swap strcat  ( dbProg strMsg )
                .tell  ( dbProg )
            then  ( dbProg )
 
        else  ( dbProg )
            ( Boo, didn't compile. )
            "There were compile errors, shown above." .tell
        then  ( dbProg )
 
    catch  ( dbProg strExc )
        "@muf encountered an error running your program: %s" fmtstring .tell
    endcatch  ( dbProg )
 
    recycle  (  )
;
