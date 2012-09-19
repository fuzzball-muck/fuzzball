@prog game-chess.muf
1 99999 d
1 i
( An MCP-aware game of chess.                              )
 
lvar piecenames
 
$def }join }list "" array_join
$def CHESS_PKGNAME "org-fuzzball-chess"
 
 
 
( Game Data management -----------------------------)
 
 
: gamedata_get[ str:game str:key -- any:val ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    getprop
;
 
: gamedata_set[ str:game str:key any:val -- ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    val @ setprop
;
 
: gamedata_del[ str:game str:key -- ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    remove_prop
;
 
: gamedata_listget[ str:game str:key -- list:val ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    array_get_proplist
;
 
: gamedata_listset[ str:game str:key list:val -- ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    over over "#" strcat remove_prop
    val @ array_put_proplist
;
 
: gamedata_listdel[ str:game str:key -- ]
    game @ key @ { }list gamedata_listset
    prog { "_gamedata/game-" game @ "/" key @ "#" }join
    remove_prop
;
 
: gamedata_reflistget[ str:game str:key -- list:val ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    array_get_reflist
;
 
: gamedata_reflistset[ str:game str:key list:val -- ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    val @ array_put_reflist
;
 
: gamedata_reflistdel[ str:game str:key -- ]
    prog { "_gamedata/game-" game @ "/" key @ }join
    remove_prop
;
 
: gamedata_clear[ str:game -- ]
    prog { "_gamedata/game-" game @ "/" }join
    begin
        over swap nextprop
        dup while
        over over remove_prop
    repeat
    pop pop
;
 
 
 
( Player Data management -----------------------------)
 
 
: playerdata_get[ ref:who str:game str:key -- any:val ]
    game @ { "player" who @ "/" key @ }join gamedata_get
;
 
: playerdata_set[ ref:who str:game str:key any:val -- ]
    game @ { "player" who @ "/" key @ }join val @ gamedata_set
;
 
: playerdata_del[ ref:who str:game str:key -- ]
    game @ { "player" who @ "/" key @ }join gamedata_del
;
 
: playerdata_clear[ ref:who str:game -- ]
    prog { "_gamedata/game-" game @ "/player" who @ }join
    remove_prop
;
 
 
 
( Team management -----------------------------)
 
 
: teams_get[ str:game -- list:teams ]
    game @ "teams" gamedata_listget
    dup not if pop { }list then
;
 
: teams_set[ list:teams str:game -- ]
    game @ "teams" teams @ gamedata_listset
;
 
: teams_clear[ str:game -- ]
    game @ "teams" gamedata_clear
;
 
 
 
 
: team_players_get[ str:game str:team -- list:players ]
    game @ { "team-" team @ tolower }join
    gamedata_reflistget
    dup not if pop { }list then
;
 
: team_players_set[ list:plist str:game str:team -- ]
    game @ teams_get
    dup not if pop { }list then
    { team @ }list swap array_union
    game @ teams_set
    game @ { "team-" team @ tolower }join
    plist @ gamedata_reflistset
;
 
: team_players_clear[ str:game str:team -- ]
    game @ teams_get
    { team @ }list swap array_diff
    game @ teams_set
    game @ { "team-" team @ tolower }join
    gamedata_reflistdel
;
 
: team_player_add[ ref:who str:game str:team -- ]
    game @ team @ team_players_get
    dup not if pop { }list then
    { who @ }list swap array_union
    game @ team @ team_players_set 
;
 
: team_player_del[ ref:who str:game str:team -- ]
    game @ team @ team_players_get
    { who @ }list swap array_diff dup
    array_count if
        game @ team @ team_players_set
    else
        pop game @ team @ team_players_clear
    then
;
 
: player_notify[ ref:who str:game str:text -- ]
    who @ text @ notify
    who @ descr_array
    foreach var! dscr pop
        dscr @ CHESS_PKGNAME mcp_supports 0.0 > if
            dscr @ CHESS_PKGNAME "info" {
                "game" game @
                "info" text @
            }dict mcp_send
        else
            dscr @ text @ descrnotify
        then
    repeat
;
 
: team_notify[ str:game str:team str:text -- ]
    game @ team @ team_players_get
    foreach swap pop
        game @ text @ player_notify
    repeat
;
 
 
 
( Player management -----------------------------)
 
 
: players_get[ str:game -- list:plist ]
    game @ "players" gamedata_reflistget
    dup not if pop { }list then
;
 
: players_set[ list:plist str:game -- ]
    game @ "players" plist @ gamedata_reflistset
;
 
: players_init[ str:game -- ]
    { }list game @ players_set
;
 
: players_clear[ str:game -- ]
    game @ "players" gamedata_reflistdel
;
 
: players_notify[ str:game str:mesg -- ]
    game @ players_get
    foreach var! who pop
        who @ game @ mesg @ player_notify
    repeat
;
 
: player_add[ ref:who str:game -- ]
    game @ players_get
    { who @ }list array_union
    game @ players_set
    who @ game @ "team" "" playerdata_set
;
 
: player_team_get[ ref:who str:game -- str:team ]
    who @ game @ "team" playerdata_get
    dup not if pop "" then
;
 
: player_team_set[ ref:who str:game str:team -- ]
    who @ game @ player_team_get dup if
        who @ game @ rot team_player_del
    else pop
    then
    who @ game @ "team" team @ playerdata_set
    who @ game @ team @ team_player_add
;
 
: player_del[ ref:who str:game -- ]
    game @ players_get
    { who @ }list swap array_diff
    game @ players_set
 
    who @ game @ player_team_get
    who @ game @ rot team_player_del
    
    who @ game @ playerdata_clear
;
 
 
 
( Board Square management -----------------------------)
 
 
: board_get[ str:game -- str:board ]
    game @ "board" gamedata_get
    dup not if
        pop "........"
        dup strcat
        dup strcat
        dup strcat
    then
;
 
: board_set[ str:board str:game -- ]
    game @ "board" board @ gamedata_set
;
 
: board_del[ str:game -- ]
    game @ "board" gamedata_del
;
 
: board_setsquare[ str:board int:x int:y str:piece -- str:board' ]
    board @
    8 y @ - 8 * x @ -- +
    strcut 1 strcut swap pop
    piece @ dup not if pop "." then
    swap strcat strcat
;
 
: board_getsquare[ str:board int:x int:y -- str:piece ]
    board @
    8 y @ - 8 * x @ + 1 midstr strip
    dup "." strcmp not if pop "" then
;
 
: board_findpiece[ str:board str:piece -- int:x int:y ]
    board @ piece @ instr
    -- dup
    8 % ++ 8 rot
    8 / -
;
 
 
: square_set[ str:game int:x int:y str:piece -- ]
    game @ board_get
    x @ y @ piece @ board_setsquare
    game @ board_set
;
 
: square_get[ str:game int:x int:y -- str:piece ]
    game @ board_get
    x @ y @ board_getsquare
;
 
 
 
 
( board position encoding. -----------------------)
 
: encode_pos[ int:x int:y -- str:pos ]
    "ABCDEFGH" x @ 1 midstr
    "12345678" y @ 1 midstr strcat
;
 
: decode_pos[ str:pos -- int:x int:y ]
    pos @ strip toupper
    dup strlen 2 = not if
        "Invalid board position." abort
    then
    1 strcut
    dup number? not if swap then
    "12345678" swap instr dup not if
        "Invalid board position." abort
    then
    swap
    "ABCDEFGH" swap instr dup not if
        "Invalid board position." abort
    then
    swap
; 
 
: normalize_pos ( str:pos -- str:pos' )
    decode_pos encode_pos
;
 
 
 
( Piece management -----------------------------)
 
 
: piece_is_white[ str:token -- bool:iswhite ]
    token @ if "PNBRQK" token @ instr not not else 0 then
;
 
: piece_name[ str:token -- str:name ]
    ( token @ piece_is_white if "White " else "Black " then )
    piecenames @ token @ toupper [] ( strcat )
;
 
: abs ( int:val -- int:val' )
    dup 0 < if -1 * then
;
 
: sign ( int:val -- int:val' )
    dup abs /
;
 
: piece_move_nocheck[ str:board str:from str:to -- str:board' ]
    board @ from @ decode_pos
    board_getsquare var! piece
    
    board @
    from @ decode_pos ""      board_setsquare
    to   @ decode_pos piece @ board_setsquare
;
 
: piece_check_intervening[ str:board str:from str:to -- int:isok ]
    from @ decode_pos var! y     var! x
    to   @ decode_pos var! desty var! destx
    begin
        destx @ x @ - sign var! dx
        desty @ y @ - sign var! dy
        x @ dx @ + x !
        y @ dy @ + y !
        x @ destx @ =
        y @ desty @ = and if
            1 break
        then
        board @ x @ y @ board_getsquare if
            0 break
        then
    repeat
;
 
: piece_capture_allowed[ str:board str:from str:to -- int:isok ]
    board @ from @ decode_pos board_getsquare       var! piece
    board @ to   @ decode_pos board_getsquare strip var! target
    
    to   @ decode_pos
    from @ decode_pos rot swap - rot rot -
    var! dx var! dy
    
    dx @ not dy @ not and if 0 exit then
    piece @ "B" stringcmp not if
        dx @ abs dy @ abs = not if 0 exit then
        board @ from @ to @ piece_check_intervening not if 0 exit then
    then
    piece @ "R" stringcmp not if
        dx @ dy @ and if 0 exit then
        board @ from @ to @ piece_check_intervening not if 0 exit then
    then
    piece @ "Q" stringcmp not if
        dx @ dy @ and
        dx @ abs dy @ abs = not and if 0 exit then
        board @ from @ to @ piece_check_intervening not if 0 exit then
    then
    piece @ "K" stringcmp not if
        dx @ abs 1 <= dy @ abs 1 <= and
        not if 0 exit then
    then
    piece @ "N" stringcmp not if
        dx @ abs 1 = dy @ abs 2 = and
        dx @ abs 2 = dy @ abs 1 = and
        or not if 0 exit then
    then
    piece @ "P" stringcmp not if
        dy @ not if
            piece @ piece_is_white if
                from @ "?4" smatch not if 0 exit then
            else
                from @ "?5" smatch not if 0 exit then
            then
        else
            piece @ piece_is_white if
                dy @ 1 = not if 0 exit then
            else
                dy @ -1 = not if 0 exit then
            then
        then
        dx @ abs 1 = not if 0 exit then
        target @ not if 0 exit then
        piece  @ piece_is_white not
        target @ piece_is_white not
        = if 0 exit then
    then
    target @ if
        piece  @ piece_is_white not
        target @ piece_is_white not = if
            0 exit
        then
    then
    ( FIXME: Need test for if en-passante pawn moved before )
    1
;
 
: piece_in_check[ str:board str:pos -- int:in_check ]
    board @ pos @ decode_pos board_getsquare
    dup if piece_is_white else pop -1 then
    var! targwhite
    1 8 1 for var! y
        1 8 1 for var! x
            board @ x @ y @ board_getsquare
            dup if
                piece_is_white targwhite @ = not
                targwhite @ -1 = or if
                    board @ x @ y @ encode_pos pos @
                    piece_capture_allowed if
                        1 exit
                    then
                then
            else pop
            then
        repeat
    repeat
    0
;
 
 
: piece_move_leaves_in_check[ str:board str:from str:to str:kingpos -- int:incheck ]
    board @ from @ to @ piece_move_nocheck
    kingpos @ from @ stringcmp not if to @ kingpos ! then
    kingpos @ piece_in_check
;
 
 
: piece_move_allowed[ str:game str:board str:from str:to -- int:isok ]
    board @ from @ decode_pos board_getsquare       var! piece
    board @ to   @ decode_pos board_getsquare strip var! target
    
    to   @ decode_pos
    from @ decode_pos rot swap - rot rot -
    var! dx var! dy
    
    dx @ not dy @ not and if 0 exit then
    piece @ "B" stringcmp not if
        dx @ abs dy @ abs = not if 0 exit then
        board @ from @ to @ piece_check_intervening not if 0 exit then
    then
    piece @ "R" stringcmp not if
        dx @ dy @ and if 0 exit then
        board @ from @ to @ piece_check_intervening not if 0 exit then
    then
    piece @ "Q" stringcmp not if
        dx @ dy @ and
        dx @ abs dy @ abs = not and if 0 exit then
        board @ from @ to @ piece_check_intervening not if 0 exit then
    then
    piece @ "K" stringcmp not if
        target @ "R" stringcmp not
        from @ "E1" stringcmp not
        to   @ "A1" stringcmp not
        to   @ "H1" stringcmp not  or and
        from @ "E8" stringcmp not
        to   @ "A8" stringcmp not
        to   @ "H8" stringcmp not  or and
        or and if
            to @ 1 strcut swap
            "A" stringcmp not if
                board @ from @ "D" 4 pick strcat from @
                piece_move_leaves_in_check
                board @ from @ "C" 5 pick strcat from @
                piece_move_leaves_in_check or
                board @ from @ "E" 5 rotate strcat from @
                piece_move_leaves_in_check or
                if 0 exit then
            else
                board @ from @ "F" 4 pick strcat from @
                piece_move_leaves_in_check
                board @ from @ "G" 5 pick strcat from @
                piece_move_leaves_in_check or
                board @ from @ "E" 5 rotate strcat from @
                piece_move_leaves_in_check or
                if 0 exit then
            then
            game @ from @ "moved" strcat
            gamedata_get if 0 exit then
            game @ to @ "moved" strcat
            gamedata_get if 0 exit then
        else
            dx @ abs 1 <= dy @ abs 1 <= and
            not if 0 exit then
        then
    then
    piece @ "N" stringcmp not if
        dx @ abs 1 = dy @ abs 2 = and
        dx @ abs 2 = dy @ abs 1 = and
        or not if 0 exit then
    then
    piece @ "P" stringcmp not if
        dy @ not if
            piece @ piece_is_white if
                from @ "?4" smatch not if 0 exit then
            else
                from @ "?5" smatch not if 0 exit then
            then
        else
            piece @ piece_is_white if
                dy @ 2 = if
                    from @ "?2" smatch not if 0 exit then
                else
                    dy @ 1 = not if 0 exit then
                then
            else
                dy @ -2 = if
                    from @ "?7" smatch not if 0 exit then
                else
                    dy @ -1 = not if 0 exit then
                then
            then
        then
        dx @ abs 1 > if 0 exit then
        dx @ abs 1 = if
            target @ not if 0 exit then
            piece  @ piece_is_white not
            target @ piece_is_white not
            = if 0 exit then
        then
    then
    target @ if
        piece  @ piece_is_white not
        target @ piece_is_white not = if
            (castling)
            piece  @ "K" stringcmp not
            target @ "R" stringcmp not and
            from @ "E1" stringcmp not
            to   @ "A1" stringcmp not
            to   @ "H1" stringcmp not  or and
            from @ "E8" stringcmp not
            to   @ "A8" stringcmp not
            to   @ "H8" stringcmp not  or and
            or and not if 0 exit then
        then
    then
    board @ from @ to @
    board @ piece @ piece_is_white
    if "K" else "k" then
    board_findpiece encode_pos
    piece_move_leaves_in_check
    if 0 exit then
    (
    FIXME: Need test for if en-passante pawn moved before
    )
    1
;
 
 
: piece_in_checkmate[ str:game str:board str:pos -- int:mated ]
    pos @ decode_pos var! row var! col
    board @ col @ row @ board_getsquare var! targpiece
    targpiece @ piece_is_white var! iswhite
    
    ( is king in check to start with? )
    board @ pos @ piece_in_check not if 0 exit then
    
    ( can king move out of check? )
    -1 1 1 for var! dy
        -1 1 1 for var! dx
            dx @ dy @ or not if continue then
            col @ dx @ +
            row @ dy @ +
            over 1 < 3 pick 8 > or
            over 1 < 3 pick 8 > or or not if
                encode_pos var! tmpdest
                game @ board @ pos @ tmpdest @ piece_move_allowed if
                    board @ pos @ tmpdest @ tmpdest @
                    piece_move_leaves_in_check not if 0 exit then
                then
            else
                pop pop
            then
        repeat
    repeat
    
    ( What threats are there? )
    { }list var! threats
    { }list var! mypieces
    1 8 1 for var! y
        1 8 1 for var! x
            x @ y @ encode_pos var! tmppos
            board @ x @ y @ board_getsquare
            piece_is_white iswhite @ = if
                tmppos @ mypieces @
                array_appenditem mypieces !
                continue
            then
            board @ tmppos @ pos @
            piece_capture_allowed if
                tmppos @ threats @
                array_appenditem threats !
            then
        repeat
    repeat
    
    ( Can we take the threat and leave us out of check? )
    threats @ foreach var! threat pop
        mypieces @ foreach var! piece pop
            board @ piece @ threat @
            piece_capture_allowed if
                board @ piece @ threat @ pos @
                piece_move_leaves_in_check not if 0 exit then
            then
        repeat
    repeat
    
    ( Can we block the threat and leave us out of check? )
    threats @ foreach threat ! pop
        "RBQ" threat @ instr not if continue then
        threat @ decode_pos var! threatrow var! threatcol
        col @ threatcol @ - sign dx !
        row @ threatrow @ - sign dy !
        mypieces @ foreach piece ! pop
            begin
                threatcol @ dx @ + threatcol !
                threatrow @ dy @ + threatrow !
                threatcol @ col @ =
                threatrow @ row @ = or not
            while
                threatcol @ threatrow @ encode_pos var! targ
                
                game @ board @ piece @ targ @
                piece_move_allowed if
                    board @ piece @ targ @ pos @
                    piece_move_leaves_in_check not if 0 exit then
                then
            repeat
        repeat
    repeat
    
 
    ( We're in checkmate. )
    1
;
 
 
 
 
 
( Board management -----------------------------) 
 
: board_init[ str:game -- ]
    {
        { "R" "N" "B" "Q" "K" "B" "N" "R" }list
        { "P" "P" "P" "P" "P" "P" "P" "P" }list
        { ""  ""  ""  ""  ""  ""  ""  ""  }list
        { ""  ""  ""  ""  ""  ""  ""  ""  }list
        { ""  ""  ""  ""  ""  ""  ""  ""  }list
        { ""  ""  ""  ""  ""  ""  ""  ""  }list
        { "p" "p" "p" "p" "p" "p" "p" "p" }list
        { "r" "n" "b" "q" "k" "b" "n" "r" }list
    }list
    foreach swap ++ var! y
        foreach swap ++ var! x var! piece
            game @ x @ y @ piece @ square_set
        repeat
    repeat
;
  
: board_show_mcp[ int:dscr str:game -- ]
    dscr @ CHESS_PKGNAME mcp_supports 0.0 > if
        dscr @ CHESS_PKGNAME "setboard"
        {
            "object" game @
            "board" game @ board_get
            "sequence" game @ "sequence" gamedata_get
            "color"
                dscr @ descrdbref game @ player_team_get
            "whiteplayer"
                game @ "white" team_players_get
                dup if 0 [] name else pop "" then
            "blackplayer"
                game @ "black" team_players_get
                dup if 0 [] name else pop "" then
            "turn"
                game @ "turn" gamedata_get
                dup not if pop "white" then
        }dict mcp_send
    then
;
 
 
: board_show_ansi[ int:dscr str:game -- ]
    ( Squares are red and black, alternating. )
    ( Pieces are white and green. )
    ( Just-moved piece is underlined? )
    dscr @ {
        "White: "
        game @ "white" team_players_get
        dup if 0 [] name else pop "" then
 
        "    Black: "
        game @ "black" team_players_get
        dup if 0 [] name else pop "" then
    }join descrnotify
    
    0 var! squarecolor
    dscr @ "  A  B  C  D  E  F  G  H  " descrnotify
    1 8 1 for 9 swap - var! y
        y @ 2 % squarecolor !
        y @ intostr "" strcat
        var! line
        1 8 1 for var! x
            game @ x @ y @ square_get var! piece
            line @ piece @ toupper
            dup not if pop " " then
            " " swap over strcat strcat
            { "bold"
                piece @ piece_is_white if "white" else "green" then
                squarecolor @ if "bg_black" else "bg_red" then
            }list "," array_join
            textattr
            strcat line !
            squarecolor @ not squarecolor !
        repeat
        line @ y @ intostr strcat line !
        dscr @ line @ descrnotify
    repeat
    dscr @ "  A  B  C  D  E  F  G  H  " descrnotify
;
 
( Plain text mode looks like the following:
   ------------------------
8 | R [N] B [Q] K [B] N [R]|
7 |[P] P [P] P [P] P [P] P |
6 |   [ ]   [ ]   [ ]   [ ]|
5 |[ ]   [ ]   [ ]   [ ]   |
4 |   [ ]   [ ]   [ ]   [ ]|
3 |[ ]   [ ]   [ ]   [ ]   |
2 | p [p] p [p] p [p] p [p]|
1 |[r] n [b] q [k] b [n] r |
   ------------------------
    A  B  C  D  E  F  G  H
)
 
: board_show_ascii[ int:dscr str:game -- ]
    0 var! squarecolor
    dscr @ {
        "White: "
        game @ "white" team_players_get
        dup if 0 [] name else pop "" then
 
        "    Black: "
        game @ "black" team_players_get
        dup if 0 [] name else pop "" then
    }join descrnotify
    
    dscr @ "   A  B  C  D  E  F  G  H  " descrnotify
    dscr @ "  ------------------------ " descrnotify
    1 8 1 for 9 swap - var! y
        y @ 2 % squarecolor !
        y @ intostr "|" strcat
        var! line
        1 8 1 for var! x
            game @ x @ y @ square_get var! piece
            line @
            squarecolor @ if " " " " " " else "[" " " "]" then
            piece @ not if swap piece ! else swap pop then
            piece @ swap strcat strcat
            strcat line !
            squarecolor @ not squarecolor !
        repeat
        dscr @ line @ "|" strcat
        y @ intostr strcat descrnotify
    repeat
    dscr @ "  ------------------------ " descrnotify
    dscr @ "   A  B  C  D  E  F  G  H  " descrnotify
;
 
: board_show[ ref:who str:game int:do_mcp -- ]
    who @ descr_array
    foreach var! dscr pop
        dscr @ CHESS_PKGNAME mcp_supports 0.0 > if
            do_mcp @ if
                dscr @ game @ board_show_mcp
            then
        else
            who @ "C" flag? if
                dscr @ game @ board_show_ansi
            else
                dscr @ game @ board_show_ascii
            then
        then
    repeat
;
 
: board_show_toall[ str:game int:do_mcp -- ]
    game @ players_get
    foreach var! who pop
        who @ game @ do_mcp @ board_show
    repeat
;
 
 
: piece_move[ int:except ref:who str:game str:from str:to -- ]
    game @ board_get var! board
    game @ board @ from @ to @
    piece_move_allowed not if
        who @ game @ "That move would be illegal!" player_notify
        who @ descr_array
        foreach swap pop
            game @ board_show_mcp
        repeat
    else
        0 try
            board @ from @ decode_pos board_getsquare var! piece
            board @ to @   decode_pos board_getsquare var! takenpiece
        catch
            who @ swap notify
            exit
        endcatch
 
        0 var! castling
        "" var! rto
        "" var! newto
        from @ "E8" stringcmp not if
            to @ "A8" stringcmp not if
                1 castling ! "D8" rto ! "C8" newto !
            else
                to @ "H8" stringcmp not if
                    1 castling ! "F8" rto ! "G8" newto !
                then
            then
        else
            from @ "E1" stringcmp not if
                to @ "A1" stringcmp not if
                    1 castling ! "D1" rto ! "C1" newto !
                else
                    to @ "H1" stringcmp not if
                        1 castling ! "F1" rto ! "G1" newto !
                    then
                then
            then
        then
 
        game @ "sequence" over over gamedata_get ++ gamedata_set
 
        game @ "turn" over over gamedata_get
        dup not if pop "" then
        "white" stringcmp if "white" else "black" then
        gamedata_set
 
        from @ toupper "[AEH][18]" smatch if
            game @ from @ "moved" strcat 1 gamedata_set
        then
 
        {
            who @ name " moves " from @ toupper
            " to " to @ toupper "."
        }join var! movemesg
 
        game @ players_get
        foreach var! player pop
            player @ descr_array
            foreach var! dscr pop
                dscr @ except @ = not if
                    dscr @ CHESS_PKGNAME mcp_supports 0.0 > if
                        dscr @ CHESS_PKGNAME "move"
                        {
                            "game" game @
                            "sequence" game @ "sequence" gamedata_get
                            "from" from @
                            "to" to @
                            "whiteplayer"
                                game @ "white" team_players_get
                                dup if 0 [] name else pop "" then
                            "blackplayer"
                                game @ "black" team_players_get
                                dup if 0 [] name else pop "" then
                            "turn" game @ "turn" gamedata_get
                        }dict mcp_send
                    then
                then
            repeat
        repeat
        castling @ if
            {
                who @ name " castles on the "
                to @ toupper "C*" smatch
                if "queen" else "king" then 
                " side."
            }join movemesg !
     
            0 try
                game @ to @ decode_pos         square_get var! rook
                game @ to @ decode_pos ""      square_set
                game @ rto @ decode_pos rook @ square_set
                newto @ to !
            catch
                who @ swap notify
                exit
            endcatch
        else
            piece @ "p" strcmp not if
                to @ "*1" smatch if
                    "q" piece !
                then
            else
                piece @ "P" strcmp not if
                    to @ "*8" smatch if
                        "Q" piece !
                    then
                then
            then
        then
 
        0 try
            game @ from @ decode_pos ""      square_set
            game @ to @   decode_pos piece @ square_set
        catch
            who @ swap notify
            exit
        endcatch
 
        game @ board_get board !
        game @ {
            "Chess> match "
            game @
            ": "
            movemesg @
            
            castling @ not
            takenpiece @ striplead and if
                "  "
                piece @ piece_name
                " takes "
                takenpiece @ piece_name
                "."
            then
            
            piece @ piece_is_white
            if "k" else "K" then
            board @ swap board_findpiece encode_pos var! kingpos
 
            board @ kingpos @
            piece_in_check if "  Check!" then
             
            game @ board @ kingpos @
            piece_in_checkmate if pop "  Checkmate!" then
        }join players_notify
 
        game @ 0 board_show_toall
    then
;
 
 
 
( Game management -----------------------------)
 
 
: games_get ( -- list:games )
    prog "_games" array_get_proplist
    { }list array_union (make list items unique)
    SORTTYPE_NOCASE_ASCEND array_sort (sort them)
;
 
: games_set ( list:games -- )
    { }list array_union (make list items unique)
    SORTTYPE_NOCASE_ASCEND array_sort (sort them)
    prog "_games#" remove_prop
    prog "_games" rot array_put_proplist
;
 
: game_exists[ str:game -- int:exists? ]
    game @ "lasttouched" gamedata_get not not
;
 
: game_init[ str:game -- ]
    game @ game_exists not if
        game @ games_get array_appenditem games_set
        game @ board_init
        game @ players_init
        game @ "turn"     "white" gamedata_set
        game @ "sequence" 1       gamedata_set
    then
;
 
: game_del[ str:game -- ]
    game @ game_exists if
        game @ players_clear
        game @ board_del
        game @ gamedata_clear
    then
    { game @ }list games_get array_diff games_set
;
 
: game_join[ ref:who str:game -- ]
    game @ game_init
    who @ game @ player_add
;
 
: game_leave[ ref:who str:game -- ]
    who @ game @ player_del
    game @ players_get array_count not if
        game @ game_del
    then
;
 
: games_list[ ref:who -- ]
    who @ "Chess matches currently in progress:" notify
    games_get foreach var! game pop
        who @ {
            game @ " %5s -- " fmtstring
            { "white" "black" }list
            foreach swap pop
                dup toupper ": " strcat swap
                game @ swap team_players_get
                { swap foreach name swap pop repeat }list
                ", " array_join "  " strcat
            repeat
        }join notify
    repeat
    who @ "Done." notify
; 
 
: game_touch[ str:game -- ]
    game @ "lasttouched" systime gamedata_set
;
 
: game_last_touched[ str:game -- int:systime ]
    game @ "lasttouched" gamedata_get
    dup not if pop 0 then
;
 
 
 
 
( Chess bell management --------------------------------)
 
 
: bell_del[ ref:who -- ]
    prog { "_belllist/" who @ int 10000 / intostr }join
    over over array_get_reflist
    { who @ }list swap array_diff
    array_put_reflist
;
 
: bell_add[ ref:who -- ]
    prog { "_belllist/" who @ int 10000 / intostr }join
    over over array_get_reflist
    { who @ }list array_union
    array_put_reflist
;
 
: bell_ring[ str:mesg -- ]
    0 dbtop int 10000 / 1 for
        prog { "_belllist/" 4 rotate intostr }join
        array_get_reflist
        foreach swap pop
            mesg @ notify
        repeat
    repeat
;
 
 
( Server initialization and startup --------------------)
 
 
: server_cleanup[ int:when str:event -- ]
    games_get foreach var! game pop
        systime game @ game_last_touched -
        7200 > if
            game @ game_del
        then
    repeat
 
    prog "_challenges/"
    begin
        over swap nextprop
        dup while
        
        over over "/" strcat
        begin
            over swap nextprop
            dup while
            
            over over "/timestamp" strcat getprop
            systime swap - 900 > if
                over over remove_prop
            then
        repeat
        pop pop
    repeat
    pop pop
 
    600 "cleanup" timer_start
;
 
: server_mcp_refresh[ arr:context str:event -- ]
    context @ "args" [] var! data
    context @ "descr" [] dup var! dscr descrdbref var! who
    data @ "game" [] 0 [] strip var! game
    
    game @ game_exists not if
        who @ "That chess match does not exist!" notify
        exit
    then
    who @ game @ 1 board_show
    game @ game_touch
;
 
: server_mcp_move[ arr:context str:event -- ]
    context @ "args" [] var! data
    context @ "descr" [] dup var! dscr descrdbref var! who
    data @ "game" [] 0 [] strip var! game
    0 try
        data @ "from" [] 0 [] normalize_pos var! from
        data @ "to"   [] 0 [] normalize_pos var! to
    catch
        who @ swap notify
    endcatch
    game @ game_exists not if
        who @ "That chess match does not exist!" notify
        exit
    then
    who @ game @ player_team_get
    dup "observer" stringcmp not if
        who @ "Observer's aren't allowed to move pieces!" notify
        pop exit
    then
    game @ "turn" gamedata_get
    dup not if pop "" then
    stringcmp if
        who @ "It's not your turn to move!" notify
        exit
    then
    game @ game_touch
    dscr @ who @ game @ from @ to @ piece_move
;
 
: server_mcp_leave[ arr:context str:event -- ]
    context @ "args" [] var! data
    context @ "descr" [] descrdbref var! who
    data @ "game" [] 0 [] strip var! game
    game @ game_exists not if
        who @ "That chess match does not exist!" notify
        exit
    then
    game @ game_touch
    who @ game @ game_leave
;
 
 
: server_join[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "player" [] var! who
    data @ "game" [] strip var! game
    data @ "team" [] strip var! team
 
    game @ not if
        who @ "You must stecify a match name!" notify
        exit
    then
    {
        "white" "white"
        "whit"  over
        "whi"   over
        "wh"    over
        "w"     over
        "black" "black"
        "blac"  over
        "bla"   over
        "bl"    over
        "b"     over
        "observer" "observers"
        "observe" over
        "observ" over
        "obser" over
        "obse"  over
        "obs"   over
        "ob"    over
        "o"     over
        "random" "random"
        "rando" over
        "rand"  over
        "ran"   over
        "ra"    over
        "r"     over
        "?"     over
        ""      over
    }dict team @ tolower []
    dup string? not if
        who @ "You must choose a team of white, black, or observer!" notify
        exit
    then
    team !
    team @ "random" stringcmp not if
        { "black" "white" }list
        foreach swap pop
            game @ over team_players_get array_count not if
                random 256 / 2 % if
                    team ! break
                then
            then
            pop
        repeat
        team @ "random" stringcmp not if
            who @ "Both sides have already been chosen!" notify
            exit
        then
    then
    team @ "observers" stringcmp if
        game @ team @ team_players_get dup array_count if
            who @ {
                "That side was already chosen by "
                4 rotate 0 [] name "!"
            }join notify
            exit
        else pop
        then
    then
    who @ game @ game_join
    who @ game @ team @ player_team_set
    who @ game @ 1 board_show
    game @ game_touch
    team @ "observers" strcmp not if
        " as an observer."
    else
        { " as the player for " team @ "." }join
    then
    var! ending
    game @ { who @ name " has joined chess match " game @ ending @ }join players_notify
;
 
: server_challenge[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "player" [] var! who
    data @ "challengee" [] var! challengee
    data @ "team" [] strip var! team
 
    1 9999 1 for
        intostr
        dup game_exists while
        pop
    repeat
    var! game
 
    {
        "white" "white"
        "whit"  over
        "whi"   over
        "wh"    over
        "w"     over
        "black" "black"
        "blac"  over
        "bla"   over
        "bl"    over
        "b"     over
        "observer" "observers"
        "observe" over
        "observ" over
        "obser" over
        "obse"  over
        "obs"   over
        "ob"    over
        "o"     over
        "random" "random"
        "rando" over
        "rand"  over
        "ran"   over
        "ra"    over
        "r"     over
        "?"     over
        ""      over
    }dict team @ tolower []
    dup string? not if
        who @ "You must choose a team of white, black, or observer!" notify
        exit
    then
    team !
    team @ "random" stringcmp not if
        { "black" "white" }list
        random 256 / over array_count % []
        team !
    then
    challengee @ player? not if
        who @ "I don't know who you mean!" notify
        exit
    then
 
    who @ game @ game_join
    who @ game @ team @ player_team_set
    who @ game @ 1 board_show
    game @ game_touch
 
    prog { "_challenges/" who @ "/" challengee @ "/game" }join
    game @ setprop
    prog { "_challenges/" who @ "/" challengee @ "/timestamp" }join
    systime setprop
 
    who @ "_chess/recentmatch" game @ setprop
    who @ {
        "You challenge " challengee @ name " to a game of chess!"
    }join notify
 
    challengee @ "_chess/recentchallenger" who @ setprop
    challengee @ {
        "-- " who @ name " challenges you to a game of chess!  "
        "Type 'chess accept " who @ name "' to start the game."
    }join notify
;
 
: server_accept[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "player" [] var! who
    data @ "challenger" [] var! challenger
 
    challenger @ player? not if
        who @ "I don't know who you mean!" notify
        exit
    then
 
    prog { "_challenges/" challenger @ "/" who @ "/game" }join
    getprop var! game
 
    prog { "_challenges/" challenger @ "/" who @ "/game" }join
    remove_prop
    prog { "_challenges/" challenger @ "/" who @ "/timestamp" }join
    remove_prop
    
    game @ not if
        who @ {
            "There are no outstanding challenges to you from "
            challenger @ name "."
        }join notify
        exit
    then
 
    { "black" "white" }list
    foreach swap pop
        game @ over team_players_get array_count not if
            var! team
            break
        then
        pop
    repeat
 
    who @ game @ game_join
    who @ game @ team @ player_team_set
    who @ game @ 1 board_show
    game @ game_touch
 
    who @ "_chess/recentmatch" game @ setprop
    who @ {
        "You accept " challenger @ name "'s offer of a game of chess.  "
        "The game has been started as match " game @ ".  "
        "You are playing " team @ ".  "
        "Type 'chess help' for chess command help."
    }join notify
 
    team @ tolower "w" 1 strncmp not
    if "black" else "white" then var! oteam
    challenger @ {
        "-- " who @ name " accepts your offer of a game of chess.  "
        "The game has been started as match " game @ ".  "
        "You are playing " oteam @ ".  "
        "Type 'chess help' for chess command help."
    }join notify
;
 
: server_list[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "player" [] var! who
    who @ games_list
;
 
: server_register[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "player" [] var! who
    who @ bell_add
    who @ "You register yourself with the chess bell." notify
;
 
: server_deregister[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "player" [] var! who
    who @ bell_del
    who @ "You deregister yourself from the chess bell." notify
;
 
: server_ring[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "player" [] var! who
    context @ "data" [] var! data
    data @ "mesg" [] strip var! mesg
    {
        "-- " who @ name " is looking for someone to play chess with."
        mesg @ if "  \"" mesg @ "\"" then
    }join
    bell_ring
;
 
: server_leave[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "player" [] var! who
    data @ "game" [] strip var! game
    game @ game_exists not if
        who @ "That chess match does not exist!" notify
        exit
    then
    game @ game_touch
    who @ game @ game_leave
    game @ { who @ name " has left chess match " game @ "." }join players_notify
;
 
: server_start[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "player" [] var! who
    data @ "game" [] strip var! game
    game @ game_exists not if
        who @ "That chess match does not exist!" notify
        exit
    then
    game @ "white" team_players_get not if
        who @ "That chess match has no player for white yet!" notify
        exit
    then
    game @ "black" team_players_get not if
        who @ "That chess match has no player for black yet!" notify
        exit
    then
    game @ 1 board_show_toall
    game @ { who @ name " has started chess match " game @ "." }join players_notify
;
 
: server_view[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "descr" [] var! who
    data @ "game" [] strip var! game
    game @ game_exists not if
        who @ descrdbref "That chess match does not exist!" notify
        exit
    then
    game @ game_touch
    who @ descrdbref "C" flag? if
        who @ game @ board_show_ansi
    else
        who @ game @ board_show_ascii
    then
;
 
: server_move[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "descr" [] dup descrdbref var! who var! dscr
    data @ "game" [] strip var! game
    0 try
        data @ "from" [] normalize_pos var! from
        data @ "to"   [] normalize_pos var! to
    catch
        who @ swap notify
        exit
    endcatch
    game @ game_exists not if
        who @ "That chess match does not exist!" notify
        exit
    then
    who @ game @ player_team_get
    dup "observer" stringcmp not if
        who @ "Observer's aren't allowed to move pieces!" notify
        pop exit
    then
    game @ "turn" gamedata_get
    dup not if pop "" then
    stringcmp if
        who @ "It's not your turn to move!" notify
        exit
    then
    game @ game_touch
    -1 who @ game @ from @ to @ piece_move
;
 
: server_reset[ arr:context str:event -- ]
    context @ "caller_prog" [] prog dbcmp not if exit then
    context @ "data" [] var! data
    context @ "descr" [] dup descrdbref var! who var! dscr
    data @ "game" [] strip var! game
    game @ game_exists not if
        who @ "That chess match does not exist!" notify
        exit
    then
    who @ game @ player_team_get
    dup "observer" stringcmp not if
        who @ "Observer's aren't allowed to reset the game!" notify
        pop exit
    then
    game @ game_touch
    game @ board_init
    game @ 1 board_show_toall
    game @ { who @ name " has reset chess match " game @ "." }join
    players_notify
;
 
: server_event_loop[ -- ]
    1 "cleanup" timer_start
    {
        "MCP." CHESS_PKGNAME strcat "-move"    strcat 'server_mcp_move
        "MCP." CHESS_PKGNAME strcat "-leave"   strcat 'server_mcp_leave
        "MCP." CHESS_PKGNAME strcat "-refresh" strcat 'server_mcp_refresh
        "TIMER.cleanup" 'server_cleanup
        "USER.leave" 'server_leave
        "USER.challenge" 'server_challenge
        "USER.accept" 'server_accept
        "USER.start" 'server_start
        "USER.reset" 'server_reset
        "USER.join" 'server_join
        "USER.list" 'server_list
        "USER.view" 'server_view
        "USER.move" 'server_move
        "USER.deregister" 'server_deregister
        "USER.register" 'server_register
        "USER.ring" 'server_ring
    }dict var! handlers
    begin
        handlers @ array_keys array_make
        event_waitfor var! event var! ctx
        handlers @ foreach var! addr var! key
            event @ key @ smatch if
                ( 0 try )
(prog "d" set)
                    ctx @ event @ addr @ execute break
(prog "!d" set)
                ( catch
                    me @ swap notify
                endcatch )
            then
        repeat
    repeat
;
 
: server_init[ -- ]
    prog "_serverpid" getpropval
    dup ispid? and if
        exit
    then
    prog "_serverpid" pid setprop
    
    {
        ""  "BAD_PIECE"
        "P" "Pawn"
        "N" "Knight"
        "B" "Bishop"
        "R" "Rook"
        "Q" "Queen"
        "K" "King"
    }dict piecenames !
    CHESS_PKGNAME 1.0 1.0 mcp_register_event
    server_event_loop
;
 
 
 
 
( Client code --------------------)
 
 
: client_server_send[ any:context str:type -- ]
    prog "_serverpid" getpropval
    dup dup ispid? and not if
        pop fork dup not if
            pop server_init exit
        then
    then
    type @ context @ event_send
;
 
: client_game_join[ str:args -- ]
    args @ "=" split strip var! color
    strip
    dup if
        me @ "_chess/recentmatch" 3 pick setprop
    then
    var! game
    {
        "game" game @
        "team" color @
    }dict "join" client_server_send
;
 
: client_game_observe[ str:args -- ]
    args @ strip
    dup if
        me @ "_chess/recentmatch" 3 pick setprop
    then
    var! game
    {
        "game" game @
        "team" "observer"
    }dict "join" client_server_send
;
 
: client_game_challenge[ str:args -- ]
    args @ "=" split strip var! color
    strip dup not if pop #-1 else pmatch then
    var! challengee
    {
        "challengee" challengee @
        "team" color @
    }dict "challenge" client_server_send
;
 
: client_game_accept[ str:args -- ]
    args @ strip dup not if
        pop me @ "_chess/recentchallenger" getprop
        dup not if pop #-1 then
    else
        pmatch
    then
    var! challenger
    {
        "challenger" challenger @
    }dict "accept" client_server_send
;
 
: client_game_move[ str:args -- ]
    args @ "=" split
    strip "-" split strip var! to strip var! from
    strip
    dup not if
        pop me @ "_chess/recentmatch" getpropstr
    else
        me @ "_chess/recentmatch" 3 pick setprop
    then
    var! game
    {
        "game" game @
        "from" from @
        "to"   to @
    }dict "move" client_server_send
;
 
: client_game_leave[ str:args -- ]
    args @ strip
    dup not if
        pop me @ "_chess/recentmatch" getpropstr
    else
        me @ "_chess/recentmatch" 3 pick setprop
    then
    var! game
    {
        "game" game @
    }dict "leave" client_server_send
;
 
: client_game_reset[ str:args -- ]
    args @ strip
    dup not if
        pop me @ "_chess/recentmatch" getpropstr
    else
        me @ "_chess/recentmatch" 3 pick setprop
    then
    var! game
    {
        "game" game @
    }dict "reset" client_server_send
;
 
: client_game_view[ str:args -- ]
    args @ strip
    dup not if
        pop me @ "_chess/recentmatch" getpropstr
    else
        me @ "_chess/recentmatch" 3 pick setprop
    then
    var! game
    {
        "game" game @
    }dict "view" client_server_send
;
 
: client_game_list[ str:args -- ]
    {
    }dict "list" client_server_send
;
 
: client_game_register[ str:args -- ]
    {
    }dict "register" client_server_send
;
 
: client_game_deregister[ str:args -- ]
    {
    }dict "deregister" client_server_send
;
 
: client_game_ring[ str:args -- ]
    {
        "mesg" args @ strip
    }dict "ring" client_server_send
;
 
: client_usage[ str:args -- ]
    {
    " "
    "Usage: " command @ strcat " OPTION [ARGS...]" strcat
    "OPTIONS:"
    "    list                      Lists the existing chess matches."
    "    challenge PLAYER[=COLOR]  Challenge a player to a game.  Challenger's"
    "                                COLOR is random if not given, and can be"
    "                                specified as black or white."
    "    accept [PLAYER]           Accept a challenge from [PLAYER]."
    "    observe [MATCHNAME]       Lets you observe the given match."
    "    leave [MATCHNAME]         Leaves the match."
    "    move [MATCHNAME]=FROM-TO  Where FROM and TO are positions like A3 or G8."
    "    reset [MATCHNAME]         Resets the chess game to starting state."
    "    register                  Register yourself with the chess bell list."
    "    deregister                Remove yourself from the chess bell list."
    "    ring [MESSAGE]            Ring the chess bell."
    " "
    "If optional [PLAYER] is not given, it defaults to the most recent challenger."
    "If optional [MATCHNAME] is not given, it defaults to the last one used."
    " "
    "Users running Trebuchet or other clients supporting the fuzzball-chess"
    "package, will get a nice GUI chess board.  Other users will see a text"
    "version of the chess board.  For those who have their Color flag set,"
    "a much nicer ANSI colored text board is shown."
    " "
    }list
    { me @ }list array_notify
;
 
: main[ str:args -- ]
    command @ "Queued Event." stringcmp not if
        fork not if server_init then exit
    then
    
    {
        "join" 'client_game_join
        "joi" over
        "jo" over
        "j" over
        
        "leave" 'client_game_leave
        
        "view" 'client_game_view
        "vie" over
        "vi" over
        "v" over
        
        "move" 'client_game_move
        "mov" over
        "mo" over
        "m" over
        
        "reset" 'client_game_reset
 
        "list" 'client_game_list
        "lis" over
        "li" over
        "l" over
        
        "challenge" 'client_game_challenge
        "challeng" over
        "challen" over
        "challe" over
        "chall" over
        "chal" over
        "cha" over
        "ch" over
        "c" over
        
        "accept" 'client_game_accept
        "accep" over
        "acce" over
        "acc" over
        "ac" over
        "a" over
        
        "observe" 'client_game_observe
        "observ" over
        "obser" over
        "obse" over
        "obs" over
        "ob" over
        "o" over
        
        "register" 'client_game_register
        "registe" over
        "regist" over
        "regis" over
        "regi" over
        "reg" over
        "re" over
        
        "deregister" 'client_game_deregister
        "deregiste" over
        "deregist" over
        "deregis" over
        "deregi" over
        "dereg" over
        "dere" over
        "der" over
        "de" over
        "d" over
        
        "ring" 'client_game_ring
        "rin" over
        "ri" over
        "r" over
        
        "#help" 'client_usage
        "#hel" over
        "#he" over
        "#h" over
        "help" over
        "hel" over
        "he" over
        "h" over
        "" over
        
        " default" 'client_usage
    }dict var! cmdlineopts
    args @ striplead " " split swap
    cmdlineopts @ swap []
    dup not if pop cmdlineopts @ " default" [] then
    execute
;
.
c
q
@register #me game-chess.muf=tmp/prog1
@set $tmp/prog1=A
@set $tmp/prog1=3


