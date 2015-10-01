@prog cmd-optguitest
1 99999 d
1 i
$include $lib/optionsgui
 
: destruct_arm_save[ dict:extradata str:name str:val -- str:errs ]
    val @ if "ARMED" else "disarmed" then
    "The self destruct has been %s." fmtstring .tell
    ""
;
 
: options_data[ -- arr:data ]
    {
        {
            "group"  "Self Destruct"
            "name"   "destruct_armed"
            "type"   "boolean"
            "label"  "Arm self destruct system"
            "help"   "This arms the self destruct system so that the countdown will be initiated if the big red button is pushed."
            "value"  0
            "save_cb" 'destruct_arm_save
        }dict
        {
            "group"  "Self Destruct"
            "name"   "destruct_delay"
            "type"   "timespan"
            "label"  "Delay before self destruct"
            "help"   "This specifies how long will elapse between initiation of the self destruct sequence, and when things go boom."
            "minval" 60
            "maxval" 86400
            "value"  300
        }dict
        {
            "group"  "Self Destruct"
            "name"   "destruct_password"
            "type"   "password"
            "label"  "Self destruct password"
            "help"   "This is the password to activate the self destruct system."
            "value"  "1A2B3"
        }dict
        {
            "group"  "Self Destruct"
            "name"   "destruct_warning"
            "type"   "string"
            "label"  "Self destruct initiation warning"
            "help"   "This is the warning message to announce when the self destruct sequence is initiated."
            "value"  "WARNING: SELF DESTRUCT SEQENCE INITIATED!"
        }dict
        {
            "group"  "Minions"
            "name"   "minion_count"
            "type"   "integer"
            "label"  "Number of minions"
            "help"   "This specifies how many minions you control."
            "minval" 0
            "maxval" 99999
            "value"  25
        }dict
        {
            "group"  "Minions"
            "name"   "minion_type"
            "type"   "option"
            "label"  "Type of minions"
            "help"   "This defined the type of minions that you control."
            "editable" 1
            "options" { "Double Agents" "Drones" "Foxes" "Jackals" "Ninjas" "Zombies" }list
            "value"  "Ninjas"
        }dict
        {
            "group"   "Secret Base"
            "name"    "base_location"
            "type"    "dbref"
            "label"   "Secret Base Location"
            "help"    "This is the room where your secret base is located."
            "objtype" { "room" }list
            "value"   #0
        }dict
        {
            "group"    "Secret Base"
            "name"     "secret_base_pi"
            "type"     "float"
            "label"    "Value of Pi near your Secret Base"
            "help"     "This is the value of Pi around your secret base.  Warning: altering fundamental contants of the universe isn't recommended.  Care should be exercised."
            "minval"   3.0
            "maxval"   4.0
            "digits"   4
            "resolution" 0.001
            "value"    pi
        }dict
        {
            "group"   "Secret Base"
            "name"    "secret_base_desc"
            "type"    "multistring"
            "label"   "Description of Secret Base"
            "help"    "This is the description of your secret base.  It can be multiple lines long."
            "value"   { "This is the" "base description." }list
        }dict
    }list
;
 
: saveall[ dict:extradata dict:optionsinfo -- str:errs ]
    optionsinfo @ foreach var! optinfo pop
        optinfo @ "changed" [] if
            {
                "Setting %s to '%~'."
                optinfo @ "name" []
                optinfo @ "value" []
            } reverse fmtstring
            .tell
        then
    repeat
    ""
;
 
: main[ str:Args -- ]
   descr 0 'saveall "Test options" options_data
   GUI_OPTIONS_PROCESS
;
.
c
q
@register #me cmd-optguitest=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=3

