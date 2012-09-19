@prog test-strings
1 99999 d
1 i
( Tests:
    STRCAT, INSTR, INSTRING, RINSTR, RINSTRING, SPLIT, RSPLIT,
    TOUPPER, TOLOWER, MIDSTR, STRCUT, STRLEN, STRIP, STRIPLEAD,
    STRIPTAIL, ANSI_STRIP, ANSI_STRLEN, SUBST
)
: main[ str:args -- ]
    "aBcD" "" strcat "aBcD" strcmp if "STRCAT failed. (1)" abort then
    "aBcD" "eFgH" strcat "aBcDeFgH" strcmp if "STRCAT failed. (2)" abort then
    "" "eFgH" strcat "eFgH" strcmp if "STRCAT failed. (3)" abort then
 
    "abcdefghabcdefgh" "abc" instr 1 = not if "INSTR failed." abort then
    "abcdefghabcdefgh" "Abc" instr 0 = not if "INSTR failed. (2)" abort then
    "abcdefghabcdefgh" "def" instr 4 = not if "INSTR failed. (3)" abort then
    "abcdefghabcdefgh" "fgh" instr 6 = not if "INSTR failed. (4)" abort then
    "abcdefghabcdefgh" "dEf" instr 0 = not if "INSTR failed. (5)" abort then
    "abcdefghabcdefgh" "dEf" instring 4 = not if "INSTRING failed." abort then
 
    "abcdefghabcdefgh" "abc" rinstr  9 = not if "RINSTR failed." abort then
    "abcdefghabcdefgh" "Abc" rinstr  0 = not if "RINSTR failed. (2)" abort then
    "abcdefghabcdefgh" "def" rinstr 12 = not if "RINSTR failed. (3)" abort then
    "abcdefghabcdefgh" "fgh" rinstr 14 = not if "RINSTR failed. (4)" abort then
    "abcdefghabcdefgh" "dEf" rinstr  0 = not if "RINSTR failed. (5)" abort then
    "abcdefghabcdefgh" "dEf" rinstring 12 = not if "RINSTRING failed." abort then
 
    "abcdefghabcdefgh" "dEf" split
    "" strcmp if "SPLIT failed. (1)" abort then
    "abcdefghabcdefgh" strcmp if "SPLIT failed. (2)" abort then
 
    "abcdefghabcdefgh" "def" split
    "ghabcdefgh" strcmp if "SPLIT failed. (3)" abort then
    "abc" strcmp if "SPLIT failed. (4)" abort then
 
    "abcdefghabcdefgh" "abc" split
    "defghabcdefgh" strcmp if "SPLIT failed. (6)" abort then
    "" strcmp if "SPLIT failed. (5)" abort then
 
    "abcdefghabcdefgh" "dEf" rsplit
    "" strcmp if "RSPLIT failed. (1)" abort then
    "abcdefghabcdefgh" strcmp if "RSPLIT failed. (2)" abort then
 
    "abcdefghabcdefgh" "def" rsplit
    "gh" strcmp if "RSPLIT failed. (3)" abort then
    "abcdefghabc" strcmp if "RSPLIT failed. (4)" abort then
 
    "abcdefghabcdefgh" "fgh" rsplit
    "" strcmp if "RSPLIT failed. (5)" abort then
    "abcdefghabcde" strcmp if "RSPLIT failed. (6)" abort then
 
    "aBCdEFgh" toupper "ABCDEFGH" strcmp if "TOUPPER failed." abort then
    "aBCdEFgh" tolower "abcdefgh" strcmp if "TOLOWER failed." abort then
 
    "abcdefgh" 2 4 midstr "bcde" strcmp if "MIDSTR failed. (1)" abort then
    "abcdefgh" 1 2 midstr "ab" strcmp if "MIDSTR failed. (2)" abort then
    "abcdefgh" 7 2 midstr "gh" strcmp if "MIDSTR failed. (3)" abort then
    "abcdefgh" 7 3 midstr "gh" strcmp if "MIDSTR failed. (4)" abort then
    "abcdefgh" 1 30 midstr "abcdefgh" strcmp if "MIDSTR failed. (5)" abort then
 
    "abcdefgh" 0 strcut
    "abcdefgh" strcmp if "STRCUT failed. (1)" abort then
    "" strcmp if "STRCUT failed. (2)" abort then
 
    "abcdefgh" 1 strcut
    "bcdefgh" strcmp if "STRCUT failed. (3)" abort then
    "a" strcmp if "STRCUT failed. (4)" abort then
 
    "abcdefgh" 7 strcut
    "h" strcmp if "STRCUT failed. (5)" abort then
    "abcdefg" strcmp if "STRCUT failed. (6)" abort then
 
    "abcdefgh" 8 strcut
    "" strcmp if "STRCUT failed. (7)" abort then
    "abcdefgh" strcmp if "STRCUT failed. (8)" abort then
 
    "abcdefgh" 9 strcut
    "" strcmp if "STRCUT failed. (9)" abort then
    "abcdefgh" strcmp if "STRCUT failed. (10)" abort then
 
    "" strlen 0 = not if "STRLEN failed. (1)" abort then
    "abcdefgh" strlen 8 = not if "STRLEN failed. (1)" abort then
 
    "aBCdEFgh" "xyz" "def" subst "aBCdEFgh" strcmp if "SUBST failed. (1)" abort then
    "aBCdEFgh" "xyz" "dEF" subst "aBCxyzgh" strcmp if "SUBST failed. (2)" abort then
    "aBCdEFgh" "" "dEF" subst "aBCgh" strcmp if "SUBST failed. (3)" abort then
    "aBCdEFgh" "stuvwxyz" "dEF" subst "aBCstuvwxyzgh" strcmp if "SUBST failed. (4)" abort then
    "aBCdEFgh" "" "aBC" subst "dEFgh" strcmp if "SUBST failed. (5)" abort then
    "aBCdEFgh" "" "Fgh" subst "aBCdE" strcmp if "SUBST failed. (6)" abort then
 
    "" ansi_strlen 0 = not if "ANSI_STRLEN failed. (1)" abort then
    "\[[1m" ansi_strlen 0 = not if "ANSI_STRLEN failed. (2)" abort then
    "\[[1mabcdefgh" ansi_strlen 8 = not if "ANSI_STRLEN failed. (3)" abort then
    "abc\[[1mdefgh" ansi_strlen 8 = not if "ANSI_STRLEN failed. (4)" abort then
    "abcdefgh\[[1m" ansi_strlen 8 = not if "ANSI_STRLEN failed. (5)" abort then
 
    "aBCdefGH" strip "aBCdefGH" strcmp if "STRIP failed. (1)" abort then
    "  aBCdefGH  " strip "aBCdefGH" strcmp if "STRIP failed. (2)" abort then
 
    "aBCdefGH" striplead "aBCdefGH" strcmp if "STRIPLEAD failed. (1)" abort then
    "  aBCdefGH  " striplead "aBCdefGH  " strcmp if "STRIPLEAD failed. (2)" abort then
 
    "aBCdefGH" striptail "aBCdefGH" strcmp if "STRIPTAIL failed. (1)" abort then
    "  aBCdefGH  " striptail "  aBCdefGH" strcmp if "STRIPTAIL failed. (2)" abort then
 
    "aB\[[1mCdefGH" ansi_strip "aBCdefGH" strcmp if "ANSI_STRIP failed. (1)" abort then
    "\[[1maBCdefGH" ansi_strip "aBCdefGH" strcmp if "ANSI_STRIP failed. (2)" abort then
    "aBCdefGH\[[1m" ansi_strip "aBCdefGH" strcmp if "ANSI_STRIP failed. (3)" abort then
;
 
.
c
q
@register #me test-strings=tmp/prog1
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called test-strings

