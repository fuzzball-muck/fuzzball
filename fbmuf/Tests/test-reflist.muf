@program test-reflist
1 9999 d
1 i
$def rlget prog "foo" getprop
$def rladd prog "foo" reflist_add
$def rldel prog "foo" reflist_del
$def rlfind prog "foo" reflist_find
 
: test-reflist[ str:arg -- ]
    prog "foo" remove_prop
    #1 rladd
    rlget "#1" strcmp if "Initial add failed." abort then
    #2 rladd
    rlget "#1 #2" strcmp if "Append failed." abort then
    #3 rladd
    rlget "#1 #2 #3" strcmp if "Append failed." abort then
    #2 rladd
    rlget "#1 #3 #2" strcmp if "Re-add from middle failed." abort then
    #2 rladd
    rlget "#1 #3 #2" strcmp if "Re-add from end failed." abort then
    #11 rladd
    rlget "#1 #3 #2 #11" strcmp if "Add with similar digits failed." abort then
    #1 rladd
    rlget "#3 #2 #11 #1" strcmp if "Re-add from start failed." abort then
    #1 rlfind 4 = not if "Find at start failed." abort then
    #2 rlfind 2 = not if "Find at end failed." abort then
    #3 rlfind 1 = not if "Find in middle failed." abort then
    #11 rlfind 3 = not if "Find in middle with similar digits failed." abort then
    #4 rlfind 0 = not if "Finding nonexistent item gave unexpected result." abort then
    #2 rldel
    rlget "#3 #11 #1" strcmp if "Del from middle failed." abort then
    #3 rldel
    rlget "#11 #1" strcmp if "Del from start failed." abort then
    #1 rldel
    rlget "#11" strcmp if "Del from end failed." abort then
    #11 rldel
    rlget if "Last del failed." abort then
;
.
c
q
