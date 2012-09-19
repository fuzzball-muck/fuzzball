#!/bin/csh -ef

#ChupChup (rearl@belch.berkeley.edu) foisted this on me. -- NF

if ("x$1" == "x") then
	echo "Usage: $0 file"
	exit 1;
endif

set file = "`basename $1 .m`"
set out = "$file.muf"

if (! -r $file.m) then
	echo "File not found: $file.m"
	exit 1;
endif

( echo "@edit $file" ; echo "1 5000 d" ; echo "i" ; muv < $file.m ; \
echo "." ; echo "c" ; echo "q" ) > $out

