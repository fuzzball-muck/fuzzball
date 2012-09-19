( cmd-ansi.muf for Fuzzball 6 by Natasha@HLM
 
  Copyright 2002 Natasha@HLM. Copyright 2002 Here Lie Monsters.
  "@view $box/mit for license information.
)
 
: do-core  ( strOther strMsgdir strFlag -- )
    me @ swap set  ( strOther strMsgdir )
 
    prog swap array_get_proplist  ( strOther arr )
    random over array_count %  ( strOther arr intKey )
    array_getitem  ( strOther str )
    swap command @ " '%s %s' to revert." fmtstring strcat
    .tell
;
: do-on  "off" "_msg/on" "c"  do-core ;
: do-off "on" "_msg/off" "!c" do-core ;
 
$define dict_commands {
    "on"  'do-on
    "off" 'do-off
}dict $enddef
 
: do-pattern  ( str -- }  With the given string yielding special codes. )
    {
    "\[[1;30m>\[[0;37m>\[[1;37m>    0    1    2     3     4      5     6     7   \[[1m\[[31mA\[[32mN\[[33mS\[[35mI\[[37m guide"  ( strHead )
    " \[[30mblack \[[31mred \[[32mgreen \[[33myellow \[[34mblue \[[35mmagenta \[[36mcyan \[[37white "  ( strHead str )
    4 rotate dup if textattr else pop then  ( strHead str )
    dup "\[[1;30m>\[[0;37m>\[[1;37m> \[[0m" swap strcat swap "\[[1;30m>\[[0;37m>\[[1;37m> " swap strcat  ( strHead strBold strDim )
    "\[[1;30m>\[[0;37m>\[[1;37m> ansi [on|off]    toggle your viewing of color"
    "\[[1;30m>\[[0;37m>\[[1;37m> ansi <color>     see pattern with <color> for background"
    }list { me @ }list array_notify
;
 
: main  ( str -- )
    strip tolower
    dup "#" stringpfx if 1 strcut swap pop then  ( str )
 
    ( Is it a command? )
    0 try  ( str )
        dict_commands over array_getitem  ( str adr )
        execute exit  ( str )
    catch pop endcatch  ( str )
 
    ( No, not a command. Is it a background color code? )
    0 try  ( str )
        {
            ""        ""
            "black"   "bg_black"
            "red"     "bg_red"
            "yellow"  "bg_yellow"
            "green"   "bg_green"
            "cyan"    "bg_cyan"
            "blue"    "bg_blue"
            "magenta" "bg_magenta"
            "white"   "bg_white"
        }dict over array_getitem  ( str strColor )
        do-pattern exit  ( str )
    catch pop endcatch  ( str )
 
    ( No. Is it nothing? )
    "I don't know what you mean by '%s'." fmtstring .tell  (  )
;
