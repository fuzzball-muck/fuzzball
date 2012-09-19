@prog lib-cases
1 99999 d
1 i
(
Okay, here's an evil idea for you programmers:
Fringe came up with a truly demented way to get case statements
using just $defines, so we made it into a library.  The library
basically makes the following defines:
--------
$define case    begin dup       $enddef
$define when    if pop          $enddef
$define end     break then dup  $enddef
$define default pop 1 if        $enddef
$define endcase pop pop 1 until $enddef
--------
And what it lets you do is something like this:
--------
$include $lib/case
<data> case
  <test> when <effect> end
  <test> when <effect> end
  <test> when <effect> end
  <test> when <effect> end
  default <otherwise>  end
endcase
--------
This will compile to the following:
--------
<data> begin
  dup <test> if pop <effect> break then
  dup <test> if pop <effect> break then
  dup <test> if pop <effect> break then
  dup <test> if pop <effect> break then
  dup pop 1  if <otherwise>  break then
  dup pop pop
1 until
--------
The default clause is optional and thats why the wierd 'dup pop pop'
is at the end.  The <otherwise> clause is passed the value that failed
to match in any of the tests.  The <tests> can be as complex as you wish,
and so can the <effect> and <otherwise> statements.
Here's an example using real code:
--------
read case
  "#help" over stringcmp not swap "#h" stringcmp not or when do-help end
  "#help2" over stringcmp not swap "#h2" stringcmp not or when do-help2 end
  "#list" stringcmp not when do-list end
  default pop give-error end
endcase
--------
Enjoy!
        - Foxen
)
: main "" pop ;
.
c
q
@register lib-cases=lib/case
@register #me lib-cases=tmp/prog1
@set $tmp/prog1=V
@set $tmp/prog1=/_/de:A scroll containing a spell called lib-cases
@set $tmp/prog1=/_defs/case:begin dup
@set $tmp/prog1=/_defs/default:pop 1 if
@set $tmp/prog1=/_defs/end:break then dup
@set $tmp/prog1=/_defs/endcase:pop pop 1 until
@set $tmp/prog1=/_defs/when:if pop
