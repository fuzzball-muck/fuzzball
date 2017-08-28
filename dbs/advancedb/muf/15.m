( Lib-MPI   version 1.0  Created 2/7/94 by Foxen
A library to facilitate parsing of MPI within the standard @desc, @succ, etc
messages.  These are drop in replacements for the DESC, SUCC, etc primitives.
 
PARSE_DESC  [d -- s]  Like DESC, but parses embedded MPI and returns result.
PARSE_IDESC [d -- s]  Like IDESC, but parses embedded MPI and returns result.
PARSE_SUCC  [d -- s]  Like SUCC, but parses embedded MPI and returns result.
PARSE_OSUCC [d -- s]  Like OSUCC, but parses embedded MPI and returns result.
PARSE_FAIL  [d -- s]  Like FAIL, but parses embedded MPI and returns result.
PARSE_OFAIL [d -- s]  Like OFAIL, but parses embedded MPI and returns result.
PARSE_DROP  [d -- s]  Like DROP, but parses embedded MPI and returns result.
PARSE_ODROP [d -- s]  Like ODROP, but parses embedded MPI and returns result.
)

$doccmd @list $lib/mpi=1-14
 
: parse_desc  "_/de"  "(@Desc)"  1 parseprop ;
: parse_idesc "_/ide" "(@Idesc)" 1 parseprop ;
: parse_succ  "_/sc"  "(@Succ)"  1 parseprop ;
: parse_osucc "_/osc" "(@Osucc)" 0 parseprop ;
: parse_fail  "_/fl"  "(@Fail)"  1 parseprop ;
: parse_ofail "_/ofl" "(@Ofail)" 0 parseprop ;
: parse_drop  "_/dr"  "(@Drop)"  1 parseprop ;
: parse_odrop "_/odr" "(@Odrop)" 0 parseprop ;

PUBLIC parse_desc $libdef parse_desc
PUBLIC parse_idesc $libdef parse_idesc
PUBLIC parse_succ $libdef parse_succ
PUBLIC parse_osucc $libdef parse_osucc
PUBLIC parse_fail $libdef parse_fail
PUBLIC parse_ofail $libdef parse_ofail
PUBLIC parse_drop $libdef parse_drop
PUBLIC parse_odrop $Libdef parse_odrop
