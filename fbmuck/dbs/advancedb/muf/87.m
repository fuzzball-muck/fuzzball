( con-bootme.muf by Aerowolf@HLM and Natasha@HLM )
: main
    "me" match awake? 1 > if
        0 sleep
        "You have more than one connection. Type @bootme to drop the old ones." .tell
    then
;
