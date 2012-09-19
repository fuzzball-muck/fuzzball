" Vim syntax file
" Language:    MUF
" Maintainer:  Revar Desmera <revar@belfry.com>
" Last Change: Wed 29 Jun 2005 17:15:27 PST
" Filenames:   *.muf
" URL:	       http://www.belfry.com/fuzzball/muf.vim

" $Id: muf.vim,v 1.1 2007/08/28 09:46:45 revar Exp $

" 2005-06-29:
" Revar Desmera <revar@belfry.com>
" Initial revision
"


" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
    syntax clear
elseif exists("b:current_syntax")
    finish
endif

" Synchronization method
syn sync ccomment maxlines=200

" I use gmuf, so I set this to case ignore
syn case ignore

" Some special, non-FORTH keywords
syn keyword mufTodo contained TODO FIXME WORK BUG
syn match mufTodo contained 'Copyright\(\s([Cc])\)\=\(\s[0-9]\{2,4}\)\='

" Characters allowed in keywords
" I don't know if 128-255 are allowed in ANS-FORHT
if version >= 600
    setlocal iskeyword=!,@,33-35,%,$,38-64,A-Z,91-96,a-z,123-126,128-255
else
    set iskeyword=!,@,33-35,%,$,38-64,A-Z,91-96,a-z,123-126,128-255
endif


" Keywords

" basic mathematical and logical operators
syn keyword mufOperators + - * / % ++ --
syn keyword mufOperators < <= = > >=
syn keyword mufOperators AND OR XOR NOT
syn keyword mufOperators BITAND BITOR BITSHIFT BITXOR
syn keyword mufOperators SQRT FABS FMOD MODF FLOOR ROUND CEIL
syn keyword mufOperators ACOS ASIN ATAN ATAN2 COS SIN TAN
syn keyword mufOperators DIFF3 DIST3D EXP POW LOG LOG10
syn keyword mufOperators POLAR_TO_XYZ XYZ_TO_POLAR ABS SIGN
syn keyword mufOperators CLEAR CLEAR_ERROR ERROR? ERROR_BIT ERROR_NAME
syn keyword mufOperators ERROR_NUM ERROR_STR IS_SET? SET_ERROR
syn keyword mufOperators ERR_DIVZERO? ERR_NAN? ERR_IMAGINARY?
syn keyword mufOperators ERR_FBOUNDS? ERR_IBOUNDS?
syn keyword mufOperators EPSILON PI INF FRAND
syn keyword mufOperators RANDOM SRAND GETSEET SETSEED GAUSSIAN

" array operators
syn keyword mufArrayOps ARRAY_APPENDITEM ARRAY_COMPARE ARRAY_COUNT
syn keyword mufArrayOps ARRAY_CUT ARRAY_DELITEM ARRAY_DELRANGE
syn keyword mufArrayOps ARRAY_DIFF ARRAY_EXCLUDEVAL ARRAY_EXPLODE
syn keyword mufArrayOps ARRAY_EXTRACT ARRAY_FINDVAL ARRAY_FIRST
syn keyword mufArrayOps ARRAY_GETITEM ARRAY_GETRANGE ARRAY_INSERTITEM
syn keyword mufArrayOps ARRAY_INSERTRANGE ARRAY_INTERPRET ARRAY_INTERSECT
syn keyword mufArrayOps ARRAY_JOIN ARRAY_KEYS ARRAY_LAST
syn keyword mufArrayOps ARRAY_MAKE ARRAY_MAKE_DICT ARRAY_MATCHKEY
syn keyword mufArrayOps ARRAY_MATCHVAL ARRAY_NDIFF ARRAY_NESTED_DEL
syn keyword mufArrayOps ARRAY_NESTED_GET ARRAY_NESTED_SET ARRAY_NEXT
syn keyword mufArrayOps ARRAY_NINTERSECT ARRAY_NOTIFY ARRAY_NUNION
syn keyword mufArrayOps ARRAY_PREV ARRAY_REVERSE ARRAY_SETITEM
syn keyword mufArrayOps ARRAY_SETRANGE ARRAY_SORT ARRAY_SORT_INDEXED
syn keyword mufArrayOps ARRAY_UNION ARRAY_VALS
syn keyword mufArrayOps ARRAY_DIFF ARRAY_UNION ARRAY_INTERSECT
syn match mufArrayOps '\[]'
syn match mufArrayOps '->\[]'
syn match mufArrayOps '\[]<-'
syn match mufArrayOps '\[\.\.]'

" Property operators
syn keyword mufPropOps ADDPROP ARRAY_FILTER_FLAGS ARRAY_FILTER_PROP
syn keyword mufPropOps ARRAY_GET_PROPDIRS ARRAY_GET_PROPLIST ARRAY_GET_PROPVALS
syn keyword mufPropOps ARRAY_GET_REFLIST ARRAY_PUT_PROPLIST ARRAY_PUT_PROPVALS
syn keyword mufPropOps ARRAY_PUT_REFLIST BLESSED? BLESSPROP
syn keyword mufPropOps ENVPROP ENVPROPSTR GETPROP
syn keyword mufPropOps GETPROPFVAL GETPROPSTR GETPROPVAL
syn keyword mufPropOps NEXTPROP PARSEPROP PARSEPROPEX
syn keyword mufPropOps PROPDIR? REFLIST_ADD REFLIST_DEL
syn keyword mufPropOps REFLIST_FIND REMOVE_PROP SETPROP
syn keyword mufPropOps UNBLESSPROP 

" DB operators
syn keyword mufDBOps ADDPENNIES ARRAY_GET_IGNORELIST CALLER
syn keyword mufDBOps CHECKPASSWORD CONTENTS CONTENTS_ARRAY
syn keyword mufDBOps CONTROLS COPYOBJ COPYPLAYER
syn keyword mufDBOps DBCMP DBREF DBTOP DESC DROP EXIT?
syn keyword mufDBOps EXITS EXITS_ARRAY FAIL FINDNEXT FLAG? GETLINK
syn keyword mufDBOps GETLINKS GETLINKS_ARRAY IGNORE_ADD
syn keyword mufDBOps IGNORE_DEL IGNORING? LOCATION
syn keyword mufDBOps MATCH MLEVEL MOVEPENNIES MOVETO NAME NEWEXIT
syn keyword mufDBOps NEWOBJECT NEWPASSWORD NEWPLAYER
syn keyword mufDBOps NEWPROGRAM NEWROOM NEXT
syn keyword mufDBOps NEXTENTRANCE NEXTOWNED OBJMEM
syn keyword mufDBOps ODROP OFAIL OK?  OSUCC OWNER PART_PMATCH
syn keyword mufDBOps PENNIES PLAYER? PMATCH PROG PROGRAM? RECYCLE
syn keyword mufDBOps RMATCH ROOM? SET SETDESC SETDROP SETFAIL
syn keyword mufDBOps SETLINK SETLINKS_ARRAY SETNAME
syn keyword mufDBOps SETODROP SETOFAIL SETOSUCC
syn keyword mufDBOps SETOWN SETSUCC SETSYSPARM STATS SUCC SYSPARM
syn keyword mufDBOps SYSPARM_ARRAY THING? TIMESTAMPS
syn keyword mufDBOps TOADPLAYER TRIG TRUENAME UNPARSEOBJ

" Time operators
syn keyword mufTimeOps SYSTIME SYSTIME_PRECISE TIME TIMEFMT TIMESPLIT
syn keyword mufTimeOps DATE GMTOFFSET SLEEP
 
" Connection operators
syn keyword mufConnOps AWAKE? CONBOOT CONCOUNT CONDBREF
syn keyword mufConnOps CONDESCR CONHOST CONIDLE CONNOTIFY
syn keyword mufConnOps CONTIME CONUSER DESCR DESCR_ARRAY
syn keyword mufConnOps DESCR_SETUSER DESCRBOOT DESCRBUFSIZE DESCRCON
syn keyword mufConnOps DESCRDBREF DESCRFLUSH DESCRHOST DESCRIDLE
syn keyword mufConnOps DESCRIPTORS DESCRLEASTIDLE DESCRMOSTIDLE DESCRNOTIFY
syn keyword mufConnOps DESCRSECURE? DESCRTIME DESCRUSER FIRSTDESCR
syn keyword mufConnOps LASTDESCR NEXTDESCR ONLINE ONLINE_ARRAY

" MUF Event operators
syn keyword mufEventOps EVENT_COUNT EVENT_EXISTS EVENT_SEND
syn keyword mufEventOps EVENT_WAIT EVENT_WAITFOR
syn keyword mufEventOps TIMER_START TIMER_STOP WATCHPID 

" MCP operators
syn keyword mufMCPOps MCP_BIND MCP_REGISTER MCP_REGISTER_EVENT
syn keyword mufMCPOps MCP_SEND MCP_SUPPORTS 

" GUI operators
syn keyword mufGUIOps GUI-OPTIONS GUI_AVAILABLE GUI_CTRL_COMMAND
syn keyword mufGUIOps GUI_CTRL_CREATE GUI_DLOG_CLOSE GUI_DLOG_CREATE
syn keyword mufGUIOps GUI_DLOG_HELPER GUI_DLOG_SHOW GUI_DLOG_SIMPLE
syn keyword mufGUIOps GUI_DLOG_TABBED GUI_VALUE_GET GUI_VALUE_SET
syn keyword mufGUIOps GUI_VALUES_GET

" GUI constants
syn keyword mufGUIConsts C_BUTTON C_CHECKBOX C_COMBOBOX C_DATUM
syn keyword mufGUIConsts C_EDIT C_FRAME C_HRULE C_IMAGE
syn keyword mufGUIConsts C_LABEL C_LISTBOX C_MENU C_MULTIEDIT
syn keyword mufGUIConsts C_NOTEBOOK C_RADIOBTN C_SCALE C_SPINNER
syn keyword mufGUIConsts C_VRULE

" Misc operators
syn keyword mufMiscOps FORCE FORCE_LEVEL VERSION

" WORK: should this be a different hilite type?
syn keyword mufOperators FTOSTR STRTOF FTOSTRC FLOAT INT DBREF

" stack manipulations
syn keyword mufStack POP DUP OVER SWAP ROT PICK PUT REVERSE
syn keyword mufStack POPN DUPN LDUP ROTATE LREVERSE
syn keyword mufStack { } }CAT }DICT }JOIN }LIST }TELL
syn keyword mufStack { } }CAT }DICT }JOIN }LIST }TELL

" Variables and stuff
syn keyword mufVariable @ ! LOCALVAR VARIABLE ME LOC COMMAND TRIGGER

" conditionals
syn keyword mufCond IF ELSE THEN
syn keyword mufCond TRY ABORT CATCH CATCH_DETAILED ENDCATCH

" iterations
syn keyword mufLoop BEGIN FOR FOREACH WHILE BREAK CONTINUE REPEAT UNTIL

" new words
syn region mufArgsList start=+\[\>+ end=+]+
syn match mufColonDef '\<:\s\+[^ \t]\+\(\[\>.*]\)\=\>' contains=mufArgsList
syn keyword mufEndOfColonDef ;
syn keyword mufDeclare VAR LVAR VAR! PUBLIC WIZCALL

" Compiler directives
syn match mufInclude '\$include\s\+\k\+'

syn match mufDirective '\$def\s.*\>'
syn match mufDirective '\$define\s\+[^ \t]\+\>'
syn match mufDirective '\$enddef\>'
syn match mufDirective '\$undef\s\+[^ \t]\+\>'
syn match mufDirective '\$cleardefs\s\+[^ \t]\+\>'

syn match mufDirective '\$author\s.*\>'
syn match mufDirective '\$note\s.*\>'
syn match mufDirective '\$version\s\+[^ \t]\+\>' contains=mufFloat
syn match mufDirective '\$lib-version\s\+[^ \t]\+\>' contains=mufFloat

syn match mufDirective '\$pragma\s\+[^ \t]\+\>'
syn match mufDirective '\$echo\s\+[^ \t]\+\>'
syn match mufDirective '\$abort\s\+[^ \t]\+\>'

syn match mufDirective '\$ifdef\s\+[^ \t]\+\>'
syn match mufDirective '\$ifcancall\s\+[^ \t]\+\>'
syn match mufDirective '\$iflib\s\+[^ \t]\+\>'
syn match mufDirective '\$iflibver\s\+[^ \t]\+\>' contains=mufFloat
syn match mufDirective '\$ifver\s\+[^ \t]\+\>' contains=mufFloat
syn match mufDirective '\$else\>'
syn match mufDirective '\$endif\>'

syn match mufDirective '\$ifndef\s\+[^ \t]\+\>'
syn match mufDirective '\$ifncancall\s\+[^ \t]\+\>'
syn match mufDirective '\$ifnlib\s\+[^ \t]\+\>'
syn match mufDirective '\$ifnlibver\s\+[^ \t]\+\>' contains=mufFloat
syn match mufDirective '\$ifnver\s\+[^ \t]\+\>' contains=mufFloat

syn match mufDirective '\$libdef\s\+[^ \t]\+\>'
syn match mufDirective '\$pubdef\s\+[^ \t]\+\>'

" debugging
syn keyword mufDebug DEBUG_ON DEBUG_OFF DEBUG_LINE DEBUGGER_BREAK

" basic character operations
syn keyword mufCharOps ITOC CTOI

" char-number conversion
syn keyword mufConversion FTOSTR STRTOF STRTOFC INTOSTR ATOI STOD
syn keyword mufConversion DPL F>D HLD HOLD NUMBER S>D SIGN >NUMBER

" interptreter, wordbook, compiler
syn match mufProgFlow '\<\'[^ \t]\+\>'
syn keyword mufProgFlow EXIT CALL EXECUTE JMP CANCALL?
syn keyword mufProcess BG_MODE FG_MODE PR_MODE SETMODE MODE
syn keyword mufProcess BACKGROUND FOREGROUND PREEMPT
syn keyword mufProcess KILL FORK QUEUE INTERP UNCOMPILE
syn keyword mufProcess PID ISPID? GETPIDS GETPIDINFO 
syn keyword mufProgram INSTANCES COMPILED?
syn keyword mufProgram PROGRAM_GETLINES PROGRAM_SETLINES

" I/O
syn keyword mufIOOps NOTIFY NOTIFY_EXCEPT NOTIFY_EXCLUDE USERLOG
syn keyword mufIOOps READ TREAD READ_WANTS_BLANKS

" numbers
syn match mufInteger '\<-\=[0-9]*[0-9]\+\>'
syn match mufFloat '\<-\=\d*[.]\=\d\+\>'
syn match mufFloat '\<-\=\d*[.]\=\d\+[Ee]\d\+\>'

" Strings
syn region mufString start=+\<"+ skip=+\\"+ end=+"+ end=+$+
syn keyword mufStrOps ANSI_MIDSTR ANSI_STRCUT ANSI_STRIP ANSI_STRLEN
syn keyword mufStrOps ARRAY_FMTSTRINGS EXPLODE EXPLODE_ARRAY FMTSTRING
syn keyword mufStrOps INSTR INSTRING MIDSTR PRONOUN_SUB REGEXP REGSUB
syn keyword mufStrOps RINSTR RINSTRING RSPLIT NAME-OK? NUMBER? PNAME-OK?
syn keyword mufStrOps SMATCH SPLIT STRCAT STRCMP STRCUT STRINGCMP
syn keyword mufStrOps STRDECRYPT STRENCRYPT STRINGPFX STRLEN STRNCMP
syn keyword mufStrOps STRIP STRIPLEAD STRIPTAIL SUBST TEXTATTR
syn keyword mufStrOps TOKENSPLIT TOLOWER TOUPPER

" Types tests
syn keyword mufTypeCheck ADDRESS? ARRAY? DBREF? DICTIONARY?
syn keyword mufTypeCheck FLOAT? INT? LOCK? STRING?

" Comments
syn match mufComment '([^)]*)' contains=mufTodo
syn region mufComment start='(' end=')' contains=mufTodo

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_muf_syn_inits")
    if version < 508
	let did_muf_syn_inits = 1
	command -nargs=+ HiLink hi link <args>
    else
	command -nargs=+ HiLink hi def link <args>
    endif

    " The default methods for highlighting. Can be overriden later.
    HiLink mufTodo Todo
    HiLink mufOperators Operator
    HiLink mufMath Number
    HiLink mufInteger Number
    HiLink mufFloat Float
    HiLink mufStack Special
    HiLink mufVariable Identifier
    HiLink mufCond Conditional
    HiLink mufLoop Repeat
    HiLink mufColonDef Function
    HiLink mufEndOfColonDef Function
    HiLink mufArgsList Comment
    HiLink mufDeclare Identifier
    HiLink mufDirective Include
    HiLink mufDebug Debug
    HiLink mufCharOps Character
    HiLink mufConversion String
    HiLink mufArrayOps Statement
    HiLink mufPropOps Statement
    HiLink mufDBOps Statement
    HiLink mufTimeOps Statement
    HiLink mufConnOps Statement
    HiLink mufEventOps Statement
    HiLink mufMCPOps Statement
    HiLink mufGUIOps Statement
    HiLink mufGUIConsts Identifier
    HiLink mufMiscOps Statement
    HiLink mufProgFlow Statement
    HiLink mufProcess Statement
    HiLink mufProgram Statement
    HiLink mufIOOps Statement
    HiLink mufString String
    HiLink mufStrOps Statement
    HiLink mufTypeCheck Statement
    HiLink mufComment Comment
    HiLink mufInclude Include

    delcommand HiLink
endif

let b:current_syntax = "muf"

" vim:ts=8:sw=4:nocindent:smartindent:
