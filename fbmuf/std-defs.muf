@edit cmd-@register
def addpropstr dup not if pop remove_prop else 0 addprop then
def confirm ( s -- i ) ( displays query string and waits for user input-- returns 1 if user said "y" ) me @ swap notify read 1 strcut pop "y" stringcmp not
def debug-off ( -- ) prog "!D" set ( set a program !debug from this line )
def debug-on ( -- ) prog "D" set ( set a program debug from this line )
def debug-line ( -- ) prog "!d" over "d" set set (shows a single debug line)
def no? ( s -- i ) (checks that string starts with "n", is "0", or is blank) dup if 1 strcut "n" stringcmp not if 1 else "0" strcmp not then else pop 1 then
def yes? ( s -- i ) ( checks that string starts with y ) 1 strcut pop "y" stringcmp not
def tell me @ swap notify
def otell loc @ me @ rot notify_except ( s -- ) ( Display string to everyone in my room except me )
def pmatch pmatch (This is inserver as of 6.0)
q
