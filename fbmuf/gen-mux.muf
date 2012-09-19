@prog multiplexer
1 99999 d
1 i
( MULTIPLEXER v.2.0fb                          by Mercy Sha'ali Veh )
(                                                                   )
( Given a list of @<program> <argument>s, it will apply the         )
( programs in order.                                                )
(                                                                   )
$include $lib/strings
  
lvar mme lvar mloc lvar mtrig lvar msplit lvar mdepth
lvar variables lvar var1 lvar var2 lvar var3 lvar var4
lvar var5 lvar var6 lvar var7 lvar var8 lvar var9 lvar var10
lvar var11 lvar var12 lvar var13 lvar var14 lvar var15 
  
: set-variables
  dup not if
    pop exit then 
  dup variables int + localvar rot swap !
  1 - set-variables 
;
: get-variables
  dup variables @ = if  
    pop exit then
  dup variables int + localvar @ 
  swap 1 + get-variables
; 
: popall
  depth mdepth @ >= if
    pop popall then 
;
 
: main
  mme @ me ! mloc @ loc ! mtrig @ trigger ! 
  1 get-variables 
  msplit @ 
  dup not if
    exit then
  " @" .split msplit !
  " " .split swap 
  dup number? not if
    dup "$" instr 1 = if
        1 strcut swap pop 
        "_reg/" over strcat #0 swap getpropstr
        dup not if
            pop "$" swap strcat " is not a registered program." strcat .tell
            exit 
        else swap pop atoi dbref then
    else " is not a program." strcat .tell exit then
  else atoi dbref then
  dup ok? if 
    dup program? if
         dup owner me @ dbcmp over "L" flag? or if
             over not variables @ and if 
                  swap pop then 
             call else
             intostr "#" swap strcat " is not LINK_OK." strcat .tell exit then
    else
         intostr "#" swap strcat " is not a program." strcat .tell exit then
  else
    intostr "#" swap strcat " is not OK." strcat .tell exit then
  popall main
;
  
: multiplexer
  me @ mme !
  loc @ mloc !
  trigger @ mtrig !
  depth mdepth !
  dup int? if
    dup 15 <= if 
         dup variables ! set-variables else
         pop "Internal error: excessive stack given to MUX." .tell exit then 
    then 
  "@" .split swap .tell msplit !
  main 
  popall
;
.
c
q
@register multiplexer=mux
@register #me multiplexer=tmp/prog1
@set $tmp/prog1=L
@set $tmp/prog1=/_/de:A scroll containing a spell called multiplexer
