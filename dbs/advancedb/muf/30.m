( lib-table.muf by Natasha@HLM
  Library for tabular output {whodoing, laston, whospecies, etc}.
 
 
  table-process[ dict:processors list:data -- list:results ]
 
  For each item of list:data, executes each address value of dict:processors,
  placing the result in a dictionary as a value with the same key. These
  dictionaries are held in list:results.
  
  For example, say dict:processors is:
 
    { "name": 'rtn-getname, "species": 'rtn-getspecies }
 
  and list:data contains the dbrefs:
 
    { #71, #208 }
 
  list:results might contain:
 
    { { "name": "Natasha", "species": "fox"     },
      { "name": "Lumi",    "species": "snowmeow"} }
 
 
  table-format[ str:linefmt dict:headers list:results -- list:output ]
 
  Format results from table-process into strings. str:linefmt should be a
  string suitable for array_fmtstrings. In the above example, it might be:
 
    "%-16.16[name]s  %-16.16[species]s"
 
  dict:headers contains a set of strings to place in the line format, to be
  printed as a header. If it's empty, it's not used. In the above example,
  it might be:
 
    { "name": "Name", "species": "Species" }
 
  The example results would be output from table-format as:
 
    { "Name              Species         ",
      "Natasha           fox             ",
      "Lumi              snowmeow        " }
 
 
  table-table-sort[ str:linefmt dict:headers dict:processors list:data str:sorthow -- ]
 
  Processes, formats, and outputs the given data as explained above,
  sorted by the output data with key sorthow.
 
 
  table-table[ str:linefmt dict:headers dict:processors list:data -- ]
 
  Processes, formats, and outputs the given data as explained above,
  in the natural order of the data list.
 
 
  Copyright 2002-2003 Natasha O'Brien. Copyright 2002-2003 Here Lie Monsters.
  "@view $box/mit" for license information.
)
$author Natasha O'Brien <mufden@mufden.fuzzball.org>
$version 1.1
$lib-version 1.1
$note Library for tabular output (whodoing, laston, whospecies, etc).
$doccmd @list $lib/table=1-51
 
: table-process[ dict:processors list:data -- list:results ]
    data @ 0 array_make  ( arrData arrResults )
    swap foreach swap pop  ( arrResults ?Datum )
        ( Make a dictResult containing everything in processors, only with the results of each adr for values. )
        0 array_make_dict  ( arrResults ?Datum dictResult )
        processors @ foreach  ( arrResults ?Datum dictResult strKey adrValue )
            4 pick swap execute  ( arrResults ?Datum dictResult strKey strValue )
            -3 rotate array_setitem  ( arrResults ?Datum dictResult )
        repeat  ( arrResults ?Datum dictResult )
        rot array_appenditem  ( ?Datum arrResults )
        swap pop  ( arrResults )
    repeat  ( arrResults )
;
 
 
: table-format[ str:linefmt dict:headers list:results -- list:output ]
    headers @ if headers @ results @ array_appenditem else results @ then  ( arrResults )
    linefmt @ array_fmtstrings  ( arrOutput )
    array_reverse  ( arrOutput )
;
 
 
: table-table-sort[ str:linefmt dict:headers dict:processors list:data str:sorthow -- ]
    linefmt @ headers @  ( strLinefmt arrHeaders )
    processors @ data @ table-process  ( strLinefmt arrHeaders arrResults )
    sorthow @ if SORTTYPE_NOCASE_DESCEND sorthow @ array_sort_indexed then  ( strLinefmt arrHeaders arrResults' )
    table-format  ( arrOutput )
    me @ 1 array_make array_notify  (  )
;
 
 
PUBLIC table-process $libdef table-process
PUBLIC table-format $libdef table-format
PUBLIC table-table-sort $libdef table-table-sort

$pubdef table-table "" table-table-sort
