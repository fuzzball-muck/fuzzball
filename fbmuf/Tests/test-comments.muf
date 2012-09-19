@prog test-comments
1 99999 d
1 i
( Testing ( strict comments.  By default, this should be the fallback style. )
 
$pragma comment_strict
( Original style comments ( where the first end paren ends it )
: test1
  "comment_strict"
;
 
$pragma comment_recurse
( Recursive ( new style ) comments. )
: test2
    "comment_recurse"
;
 
$pragma comment_loose
( recursive ( new ) coment style )
( strict (old style )
( recursive ( new ) style )
: test3
    "comment_loose"
;
 
: main[ str:args -- ]
    test1 "comment_strict"  strcmp if "pragma comment_strict failed"  abort then
    test2 "comment_recurse" strcmp if "pragma comment_recurse failed" abort then
    test3 "comment_loose"   strcmp if "pragma comment_loose failed"   abort then
;
 
.
c
q
@register #me test-comments=tmp/prog1
@set $tmp/prog1=3
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called test-comments

