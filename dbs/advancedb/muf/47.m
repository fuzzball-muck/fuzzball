( cmd-ignore by Natasha@HLM
  Adds to, removes from, and displays your in-server ignore list.
 
  Copyright 2002 Natasha O'Brien. Copyright 2002 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.0
$note Adds to, removes from, and displays your in-server ignore list.
 
$include $lib/match
: main  ( str -- )
    ( To whom are we doing things? )
    strip dup if  ( str )
 
        dup "#" stringpfx if  ( str )
            pop .showhelp exit  (  )
        then  ( str )
 
        " " explode_array  ( arrNames )
        0 array_make dup rot  ( arrAdd arrDel arrNames )
        foreach swap pop  ( arrAdd arrDel strName )
            dup not if pop continue then  ( arrAdd arrDel strName )
 
            dup "!" stringpfx if 1 else 0 then strcut  ( arrAdd arrDel strDel? strName )
            .noisy_pmatch dup ok? not if pop pop continue then  ( arrAdd arrDel strDel? db )
 
            ( Add to which list? )
            swap if  ( arrAdd arrDel db )
                swap array_appenditem  ( arrAdd arrDel )
            else
                rot array_appenditem swap  ( arrAdd arrDel )
            then  ( arrAdd arrDel )
        repeat  ( arrAdd arrDel )
 
        ( Anyone to act upon? )
        over over or not if  ( arrAdd arrDel )
            "No one to ignore. Try 'ignore #help' for help." .tell
        then  ( arrAdd arrDel )
 
        ( Are we actually 'adding to the unignore list?' )
        command @ tolower "un" stringpfx if swap then  ( arrAdd arrDel )
 
        foreach swap pop  ( arrAdd db )
            me @ over ignore_del  ( arrAdd db )
            "%D unignored." fmtstring .tell  ( arrAdd )
        repeat  ( arrAdd )
 
        foreach swap pop  ( db )
            me @ over ignore_add  ( db )
            "%D ignored." fmtstring .tell  (  )
        repeat  (  )
 
    else
        ( View ignore list. )
        "" me @ array_get_ignorelist foreach swap pop  ( strList db )
            name  ( strList strDb )
            ", " swap strcat strcat  ( strList )
        repeat  ( strList )
        2 strcut swap pop  ( strList )
        "You are globally ignoring: " swap strcat .tell  (  )
    then  (  )
;
