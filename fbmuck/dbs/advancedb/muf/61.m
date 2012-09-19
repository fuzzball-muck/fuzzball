( cmd-programs.muf by Natasha@HLM
  A Fuzzball 6 program lister.
 
  Copyright 2003 Natasha O'Brien. Copyright 2003 Here Lie Monsters.
  "@view $box/mit" for license information.
)
 
$include $lib/strings
 
: main  ( str -- )
    STRparse pop  ( strX strY )
 
    var! vsearch  ( strX )
    #-1 var! vowner
    "FV" var! vflags  ( strX )
 
    dup if  ( strX )
        "mine" stringcmp if
            me @ vowner !
            "F" vflags !
        then  (  )
    else pop then  (  )
 
    " Dbref Name                     Author               Owner      Modified @view" "bold" textattr .tell
 
    background  (  )
    #-1 begin  ( dbStart )
        vowner @ "*" vflags @ findnext  ( db )
        dup ok?  ( db boolOK? )
    while  ( db )
        dup  ( db db )
        dup "_note" getpropstr dup if "\r       " swap strcat then  ( db db strNote )
 
        ( Wait, are we really listing this object? )
        vsearch @ if
            over over "%s%D" fmtstring vsearch @ instring not if  ( db db strNote )
                pop pop continue  ( db )
            then  ( db db strNote )
        then swap  ( db strNote db )
 
        dup "_docs" getpropstr if "\[[1;32myes\[[0m" else "\[[1;31mno\[[0m" then swap  ( db strNote strDocs db )
        dup timestamps pop pop swap pop "%D" swap timefmt swap  ( db strNote strDocs strModified db )
        dup owner swap  ( db strNote strDocs strModified dbOwner db )
        dup "_author" getpropstr swap  ( db strNote strDocs strModified dbOwner strAuthor db )
        dup  ( db strNote strDocs strModified dbOwner strAuthor db db )
        "%6.6d %-24.24D %-20.20s %-10.10D %8.8s %-4.4s%s" fmtstring .tell
    repeat pop  (  )
 
    "Done." .tell
;
