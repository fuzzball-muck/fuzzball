( lib-timestr.muf
  MUF versions of MPI's {stimestr}, {timestr}, and {ltimestr}.
 
  Public domain.
 
  Version history:
  1.1, 16 March 2003: added rtimestr
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>, Revar
$version 1.1
$lib-version 1.1
$note MUF versions of MPI's {stimestr}, {timestr}, and {ltimestr}.
 
 
: ltimestr[ int:amt -- str:result ]
    0 array_make var! result  ( arr )
    var unit
 
    1 "%i sec%s"
    60 "%i min%s"
    3600 "%i hour%s"
    86400 "%i day%s"
 
    4 begin dup while  ( intN strN..int1 str1 intN )
        swap unit ! swap  ( intN strN..int2 str2 intN int1 )
        amt @ over >= if  ( intN strN..int2 str2 intN int1 )
            amt @ over /  ( intN strN..int2 str2 intN int1 intHowMany1s )
            dup 1 = if "" else "s" then swap unit @ fmtstring  ( intN strN..int2 str2 intN int1 strHowMany1s )
            result @ array_appenditem result !  ( intN strN..int2 str2 intN int1 )
            amt @ swap % amt !  ( intN strN..int2 str2 intN )
        else pop then  ( intN strN..int2 str2 intN )
    1 - repeat pop  (  )
    result @ ", " array_join  ( str )
;
 
: rtimestr  ( int -- str }  Show a human-readable 'ago' message. Algorithm taken from SC-Track Roundup issue tracking Python software. )
    dup case  ( intSince )
        63072000 (730 days) >= when 31536000 (365 days) / "%i years ago"  fmtstring end
        31536000 (365 days) >= when pop "A year ago" end
        5184000  ( 60 days) >= when 2592000  ( 30 days) / "%i months ago" fmtstring end
        2505600  ( 29 days) >= when pop "A month ago" end
        1209600  ( 14 days) >= when 604800   (  7 days) / "%i weeks ago"  fmtstring end
        604800   (  7 days) >= when pop "A week ago" end
        172800   (  2 days) >= when 86400   (  1 day ) / "%i days ago"   fmtstring end
        46800    (13 hours) >= when pop "Yesterday" end
        7200     ( 2 hours) >= when 3600    ( 2 hours) / "%i hours ago"  fmtstring end
        6300  (1 3/4 hours) >= when pop "1 3/4 hours ago" end
        5400  (1 1/2 hours) >= when pop "1 1/2 hours ago" end
        4500  (1 1/4 hours) >= when pop "1 1/4 hours ago" end
        3600     ( 1 hour ) >= when pop "An hour ago" end
        2700  ( 45 minutes) >= when pop "3/4 hour ago" end
        1800  ( 30 minutes) >= when pop "1/2 an hour ago" end
        900   ( 15 minutes) >= when pop "1/4 hour ago" end
        120   (  2 minutes) >= when 60 / "%i minutes ago" fmtstring end
        ( Where Roundup would say '1 minute ago' we go ahead and say 'Just now.' )
        default pop pop "Just now" end
    endcase  ( str )
;
 
: stimestr (i -- s}  from standard fbmuf 3who )
    dup 0 < if pop 0 then
    dup 86400 > if
        86400 / intostr "d" strcat
    else dup 3600 > if
            3600 / intostr "h" strcat
        else dup 60 > if
                60 / intostr "m" strcat
            else
                intostr "s" strcat
            then
        then
    then
    "    " swap strcat
    dup strlen 4 - strcut swap pop
;
 
: mtimestr (i -- s}  from standard fbmuf 3who )
    dup 0 < if pop 0 then
    "" over 86400 > if
        over 86400 / intostr "d " strcat strcat
        swap 86400 % swap
    then
    over 3600 / intostr
    "00" swap strcat
    dup strlen 2 - strcut
    swap pop strcat ":" strcat
    swap 3600 % 60 / intostr
    "00" swap strcat
    dup strlen 2 - strcut
    swap pop strcat
;
 
PUBLIC ltimestr $libdef ltimestr
PUBLIC mtimestr $libdef mtimestr
PUBLIC stimestr $libdef stimestr
PUBLIC rtimestr $libdef rtimestr

$pubdef timestr mtimestr
