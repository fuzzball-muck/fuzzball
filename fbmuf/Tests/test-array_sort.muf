@program test-array_sort_base
1 9999 d
1 i
: test-array_sort[ str:arg -- ]
    {
        "Silvia"
        "Karen"
        "Jaqueline"
        "Anupatra"
        "Revar"
        "Foxenlings"
        "Tygryss"
        "Tygling"
        "Menolly"
        "Ravellia"
        "littlefox"
        1.3
        ""
        7
        
        -3
        "Foxen"
        "Fiera"
        "Lynx"
    }list var! mylist

    mylist @ SORTTYPE_CASE_ASCEND array_sort    " " array_join
    "-3 1.3 7  Anupatra Fiera Foxen Foxenlings Jaqueline Karen Lynx Menolly Ravellia Revar Silvia Tygling Tygryss littlefox"
    strcmp if
        "SORTTYPE_CASE_ASCEND ordering failed." abort
    then

    mylist @ SORTTYPE_NOCASE_ASCEND array_sort  " " array_join
    "-3 1.3 7  Anupatra Fiera Foxen Foxenlings Jaqueline Karen littlefox Lynx Menolly Ravellia Revar Silvia Tygling Tygryss"
    strcmp if
        "SORTTYPE_NOCASE_ASCEND ordering failed." abort
    then

    mylist @ SORTTYPE_CASE_DESCEND array_sort   " " array_join
    "littlefox Tygryss Tygling Silvia Revar Ravellia Menolly Lynx Karen Jaqueline Foxenlings Foxen Fiera Anupatra  7 1.3 -3"
    strcmp if
        "SORTTYPE_CASE_DESCEND ordering failed." abort
    then

    mylist @ SORTTYPE_NOCASE_DESCEND array_sort " " array_join
    "Tygryss Tygling Silvia Revar Ravellia Menolly Lynx littlefox Karen Jaqueline Foxenlings Foxen Fiera Anupatra  7 1.3 -3"
    strcmp if
        "SORTTYPE_NOCASE_DESCEND ordering failed." abort
    then
;
.
c
q
