@prog test-floatmath
1 99999 d
1 i
: simple_test[ -- ]
    pi 3.141592653589793238 = not if "Pi != 3.141592653589793238" abort then
    pi "%.15f" fmtstring "3.141592653589793" strcmp if "Pi fmtstring is wrong." abort then
 
    pi 1 + 4.141592653589793238 = not if "Pi+1 != 4.141592653589793238" abort then
    pi 1.0 + 4.141592653589793238 = not if "Pi+1.0 != 4.141592653589793238" abort then
    pi ++ 4.141592653589793238 = not if "Pi++ != 4.141592653589793238" abort then
    pi 1.0 + "%.15f" fmtstring "4.141592653589793" strcmp if "Pi+1.0 fmtstring is wrong." abort then
 
    pi 1 - 2.141592653589793238 = not if "Pi-1 != 2.141592653589793238" abort then
    pi 1.0 - 2.141592653589793238 = not if "Pi-1.0 != 2.141592653589793238" abort then
    pi -- 2.141592653589793238 = not if "Pi-- != 2.141592653589793238" abort then
    pi 1.0 - "%.15f" fmtstring "2.141592653589793" strcmp if "Pi-1.0 fmtstring is wrong." abort then
 
    pi pi + pi 2 * = not if "Pi+Pi != Pi*2" abort then
    pi pi - 0.0 = not if "Pi-Pi != 0.0" abort then
    pi pi / 1.0 = not if "Pi/Pi != 1.0" abort then
 
    1.0 3 / "%.15f" fmtstring "0.333333333333333" strcmp if "1.0/3 fmtstring is wrong." abort then
    1.0 3 / 9.0 * 3.0 = not if "1.0/3*9.0 != 3.0" abort then
 
    2.0 sqrt 1.41421356237309504880 - fabs epsilon < not if "sqrt(2.0) != 1.414..." abort then
    2.0 0.5 pow 1.41421356237309504880 - fabs epsilon < not if "pow(2.0,0.5) != 1.414..." abort then
    1.5 3.0 pow 1.5 dup dup * * - fabs epsilon < not if "pow(1.5,3.0) != 1.5*1.5*1.5" abort then
 
    1.0 epsilon + 1.0 = if "1.0+Epsilon == 1.0" abort then
    1.0 epsilon 10 / + 1.0 = not if "1.0+(Epsilon/10) != 1.0" abort then
    epsilon log10 -15 > if "log10(Epsilon) > -15" abort then
;
 
 
: trig_test[ -- ]
    0.0 sin 0.0 = not if "sin(0.0) != 0.0" abort then
    pi sin fabs epsilon < not if "sin(Pi) != 0.0" abort then
    pi 2 / sin 1.0 - fabs epsilon < not if "sin(Pi/2) != 1.0" abort then
    pi 1.5 * sin -1.0 - fabs epsilon < not if "sin(1.5Pi) != -1.0" abort then
 
    0.0 cos 1.0 = not if "cos(0.0) != 1.0" abort then
    pi cos -1.0 - fabs epsilon < not if "cos(Pi) != -1.0" abort then
    pi 2 / cos fabs epsilon < not if "cos(Pi/2) != 0.0" abort then
    pi 1.5 * cos fabs epsilon < not if "cos(1.5Pi) != 0.0" abort then
 
    0.0 tan fabs epsilon < not if "tan(0.0) != 0.0" abort then
    pi tan fabs epsilon < not if "tan(Pi) != 0.0" abort then
 
    pi 1.5 * tan pi 1.5 * sin pi 1.5 * cos / - fabs epsilon < not if "tan(1.5Pi) != sin(1.5Pi)/cos(1.5Pi)" abort then
    inf atan pi 2 / - fabs epsilon < not if "atan(INF) != Pi/2" abort then
    1.0 asin pi 2 / - fabs epsilon < not if "asin(1.0) != Pi/2" abort then
    0.0 acos pi 2 / - fabs epsilon < not if "acos(0.0) != Pi/2" abort then
;
 
 
: properties_test[ -- ]
    prog "_testprop" pi setprop
    prog "_testprop" getprop pi = not if "Property Pi != Pi" abort then
;
 
 
: main[ str:args -- ]
    simple_test
    trig_test
    properties_test
;
 
.
c
q
@register #me test-floatmath=tmp/prog1
@set $tmp/prog1=3
@propset $tmp/prog1=float:/_testprop:3.141592653589793

