( cmd-@objeditgui  Copyright 7/28/2002 by revar@belfry.com )
( Released under the terms of the GNU LGPL.                )
 
$author Revar Desmera <revar@belfry.com>
$version 1.003
 
( A program to let you easily create options that can be   )
( edited via a GUI dialog.                                 )
 
$def LIST_PROPDIR   "%s#"
$def DESCMPI        "{eval:{list:%s}}"
$def DESCMPI_NTFY   "{look-notify:{eval:{list:%s}}}"
$def LOOK_NOTIFY    "{look-notify}"
 
$def DESC_PROP      "_/de"
$def DESC_LIST      "_redesc"
$def OLD_DESC_LIST  "redesc"
 
$def SUCC_PROP      "_/sc"
$def SUCC_LIST      "_/succlist"
$def OSUCC_PROP     "_/osc"
$def OSUCC_LIST     "_/osucclist"
 
$def FAIL_PROP      "_/fl"
$def FAIL_LIST      "_/faillist"
$def OFAIL_PROP     "_/ofl"
$def OFAIL_LIST     "_/ofaillist"
 
$def DROP_PROP      "_/dr"
$def DROP_LIST      "_/droplist"
$def ODROP_PROP     "_/odr"
$def ODROP_LIST     "_/odroplist"
 
$def FLIGHT_PROP    "_flight?"
$def HAND_PROP      "_hand/hand_ok"
$def SCENT_PROP     "_scent"
$def SCENT_LIST     "_scentlist"
$def SMELL_NOTIFY   "_smell_notify"
 
$def SWEEP_PROP     "_sweep"
$def SWEEP_LIST     "_sweeplist"
$def SWEEPPLYR_PROP "_sweep_player"
$def SWEEPPLYR_LIST "_sweep_player_list"
$def SWEPT_PROP     "_swept"
$def SWEPT_LIST     "_sweptlist"
 
$def SAY_PROP       "_say/def/say"
$def OSAY_PROP      "_say/def/osay"
$def NORM_SAY_PROP  "_say/normal"
$def ADHOC_SAY_PROP "_say/adhoc"
 
$def SPECIES_PROP   "species"
$def ALLOW_CUSTOM_SPECIES 1 (change to 0 if custom species not allowed )
: generate_species_predefineds[ -- list:species ]
    { "Alicorn" "Alpaca" "Antelope" "Arctic Fox" "Bat" "Bear" "Beaver" "Binturong" "Bobcat" "Buffalo" "Cat" "Centaur" "Coyote" "Crocodile" "Dog" "Donkey" "Dragon" "Duck" "Eagle" "Elephant" "Ermine" "Falcon" "Fennic" "Flying Squirrel" "Fox" "Frog" "Fruit Bat" "Gazelle" "Gerbil" "Hawk" "Hedgehog" "Horse" "Human" "Hyena" "Ibis" "Jackal" "Jaguar" "Kangaroo" "Koala" "Lemur" "Lion" "Lizard" "Lynx" "Marten" "Mink" "Mongoose" "Monkey" "Mountain Lion" "Mouse" "Mule" "Numbat" "Opossum" "Owl" "Panda" "Panther" "Penguin" "Pig" "Porcupine" "Quail" "Rabbit" "Raccoon" "Rat" "Red Panda" "Seal" "Silver Fox" "Skunk" "Sloth" "Squirrel" "Tiger" "Thylacine" "Unicorn" "Vixen" "Vampire Bat" "Weasel" "Wolf" "Xerus" "Yak" "Zebra" }list
    SORTTYPE_CASEINSENS array_sort
;
 
$def GENDER_PROP "sex"
$def ALLOW_CUSTOM_GENDERS 1 (change to 0 if custom genders not allowed )
: generate_gender_predefineds[ -- list:species ]
    { "Female" "Male" "Neuter" "Herm" "Hermaphrodite" }list
    "_pronouns/"
    begin
        #0 swap nextprop dup
    while
        dup tolower "" "_pronouns/" subst
        1 strcut swap toupper swap strcat
        rot array_appenditem swap
    repeat
    pop
;
 
 
( -------------------------------------------------------------------------- )
 
$include $lib/lmgr
$include $lib/match
$include $lib/optionsgui
 
$def yesprop? getpropstr 1 strcut pop "y" stringcmp not
 
( -------------------------------------------------------------------------- )
 
 
: multiline_get[ dbref:obj str:prop str:proplist str:oldproplist -- list:val ]
    obj @ prop @ getpropstr
    dup proplist @ "{list:%s}" fmtstring instring if
        pop obj @ proplist @ array_get_proplist exit
    then
    oldproplist @ if
        dup oldproplist @ "{list:%s}" fmtstring instring if
            pop obj @ oldproplist @ array_get_proplist exit
        then
    then
    1 array_make
;
 
 
: multiline_set[ list:val bool:looknotify dbref:obj str:prop str:proplist -- ]
    val @ string? if
        { val @ }list val !
    then
    val @ foreach var! line var! key
        (remove look notify just in case it's there)
        line @ "" LOOK_NOTIFY subst
        proplist @ DESCMPI fmtstring
        proplist @ DESCMPI_NTFY fmtstring subst
        val @ key @ ->[] val !
    repeat
    val @ array_count 2 < if
        proplist @ obj @ lmgr-deletelist
        obj @ prop @ val @ 0 []
        looknotify @ if LOOK_NOTIFY strcat then
        setprop
    else
        proplist @ obj @ lmgr-deletelist
        obj @ proplist @ val @ array_put_proplist
        obj @ prop @
        proplist @ DESCMPI fmtstring
        looknotify @ if LOOK_NOTIFY strcat then
        setprop
    then
;
 
 
( -------------------------------------------------------------------------- )
 
 
: user_mesg_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
 
    {
        "@success"  { SUCC_PROP  SUCC_LIST  "@succ set."  }list
        "@osuccess" { OSUCC_PROP OSUCC_LIST "@osucc set." }list
        "@fail"     { FAIL_PROP  FAIL_LIST  "@fail set."  }list
        "@ofail"    { OFAIL_PROP OFAIL_LIST "@ofail set." }list
        "@drop"     { DROP_PROP  DROP_LIST  "@drop set."  }list
        "@odrop"    { ODROP_PROP ODROP_LIST "@odrop set." }list
        SCENT_PROP  { SCENT_PROP SCENT_LIST "Scent message set."  }list
        SWEEP_PROP  { SWEEP_PROP SWEEP_LIST "Sweep message set."  }list
        SWEPT_PROP  { SWEPT_PROP SWEPT_LIST "Swept message set."  }list
        SWEEPPLYR_PROP { SWEEPPLYR_PROP SWEEPPLYR_LIST "Sweep player message set."  }list
    }dict
    optname @ [] dup not if
        pop optname @
        "Unkown option '%s'." fmtstring abort
    then
    array_vals pop
    var! text
    var! listname
    var! propname
 
    val @ 0 targ @ propname @ listname @ multiline_set
    text @ .tell
    ""
;
 
 
: user_desc_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    extradata @ "optionsinfo" [] var! optionsinfo
    context @ "target" [] var! targ
    optionsinfo @ "@desc" [] "value" [] var! userdesc
    optionsinfo @ LOOK_NOTIFY [] "value" [] var! looknotify
 
    userdesc @ looknotify @ targ @ DESC_PROP DESC_LIST multiline_set
    "@desc set." .tell
    ""
;
 
 
: setlink_vfy[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
 
    val @ array? not if { val @ }list val ! then
    val @ array_count 1 > if
        targ @ exit? not if
            "Only exits may have multiple destinations." exit
        else
            val @
            foreach swap pop
                exit? not if
                    "If an exit has multiple destinations, they must all be to exits objects." exit
                then
            repeat
            "At this time, setting metalinks isn't implemented." exit
        then
    then
    val @ array_count 1 < if
        targ @ player? if
            "Players must have a home!" exit
        then
        targ @ thing? if
            "Things must have a home!" exit
        then
        { #-1 }list val !
    then
    val @ 0 [] ok? not if
        targ @ case
            room? when
                val @ 0 [] dup #-1 dbcmp not
                swap #-3 dbcmp not and if
                    "Bad drop-to destination." exit
                then
            end
            exit? when
                val @ 0 [] dup #-1 dbcmp not
                swap #-3 dbcmp not and if
                    "Bad exit destination." exit
                then
            end
            player? when
                "Bad player home." exit
            end
            thing? when
                "Bad object home." exit
            end
        endcase
    then
    ""
;
 
 
: setlink_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
 
    val @ not if { #-1 }list val ! then
    val @ array? not if { val @ }list val ! then
    val @ 0 [] val !
 
    0 try
        targ @ exit? if
            targ @ getlink if
                targ @ #-1 setlink
            then
        then
        targ @ val @ setlink
    catch pop
        "Linking failed for unknown reasons." exit
    endcatch
    "@link set." .tell
    ""
;
 
 
: name_vfy[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    extradata @ "optionsinfo" [] var! optionsinfo
    optionsinfo @ "@name" [] "value" [] var! username
    targ @ player? if
        optionsinfo @ "@password" []
        dup not if pop 0 array_make_dict then
        "value" [] dup not if pop "" then
        var! oldpass
    then
 
    optionsinfo @ "@name" [] "changed" [] not if
        "" exit
    then
 
    targ @ player? if
        targ @ oldpass @ checkpassword not if
            "Password incorrect!" exit
        then
        username @ pmatch if
            "That name is already taken." exit
        then
        username @ pname-ok? not if
            "Bad player name." exit
        then
    then
    username @ name-ok? not if
        "Bad object name." exit
    then
    ""
;
 
 
: name_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    extradata @ "optionsinfo" [] var! optionsinfo
    optionsinfo @ "@name" [] "value" [] var! username
    targ @ player? if
        optionsinfo @ "@password" []
        dup not if pop 0 array_make_dict then
        "value" [] dup not if pop "" then
        var! oldpass
    then
 
    0 try
        targ @ player? if
            targ @
            username @ " " strcat
            oldpass @ strcat
            setname
        else
            targ @
            username @
            setname
        then
        "Name changed." .tell
    catch pop
        "Could not set name for unknown reasons." exit
    endcatch
    ""
;
 
 
: password_vfy[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    extradata @ "optionsinfo" [] var! optionsinfo
    context @ "target" [] var! targ
    optionsinfo @ "@password"     [] "value" [] var! oldpass
    optionsinfo @ "user_newpass"  [] "value" [] var! newpass1
    optionsinfo @ "user_newpass2" [] "value" [] var! newpass2
    newpass1 @ newpass2 @ strcmp if
        "You didn't enter the same new password into both the new password and the new password verify fields." exit
    then
    newpass1 @ if
        targ @ oldpass @ checkpassword not if
            "Password incorrect!" exit
        then
    then
    ""
;
 
 
: password_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    extradata @ "optionsinfo" [] var! optionsinfo
    context @ "target" [] var! targ
    optionsinfo @ "user_newpass"  [] "value" [] var! newpass1
    targ @ newpass1 @ newpassword
    "Password changed." .tell
    ""
;
 
 
: stringprop_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    0 try
        targ @ optname @ val @ setprop
    catch pop
        optname @ "Could not set %s property for unknown reasons." fmtstring
        exit
    endcatch
    optname @ "%s property set." fmtstring .tell
    ""
;
 
 
: boolprop_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    0 try
        targ @ optname @ val @ if "yes" else "no" then setprop
    catch pop
        optname @ "Could not set %s property for unknown reasons." fmtstring
        exit
    endcatch
    optname @ "%s property set." fmtstring .tell
    ""
;
 
 
: flag_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    0 try
        targ @ optname @ val @ if "%s" else "!%s" then fmtstring set
    catch pop
        optname @
        val @ if "set" else "reset" then
        "Could not %s %s flag for unknown reasons." fmtstring
        exit
    endcatch
    val @ if "set" else "reset" then
    optname @
    "%s flag %s." fmtstring .tell
    ""
;
 
 
: saveall[ dict:extradata dict:optionsinfo -- str:errs ]
    ""
;
 
 
( -------------------------------------------------------------------------- )
 
 
: options_data[ dbref:obj -- arr:data ]
    {
        { (Name group)
            "group"  "Basics"
            "grouplabel" "Name, Password, Gender, Species, Home, etc."
            "name"   "@name"
            "type"   "string"
            obj @ player? if
                "label"  "Character name"
                "help"   "To change names, edit this field, and enter your password in the 'Current Password' field, then apply the changes."
            else
                "label"  "Object name"
                "help"   "To change the object name, edit this field, then apply the changes."
            then
            "width" 40
            "vfy_cb"  'name_vfy
            "save_cb" 'name_save
            "value"   obj @ name
        }dict
        obj @ player? if
            {
                "group"  "Basics"
                "name"   "@password"
                "type"   "password"
                "label"  "Password (for name change)"
                "help"   "To change name or password, you must enter your current password here for verification."
                "width" 40
                "value"  ""
            }dict
            (
            {
                "group"  "Basics"
                "name"   "user_newpass"
                "type"   "password"
                "label"  "New Password"
                "help"   "To change your login password, enter your new password in this field, re-enter it in the 'Verify New Password' field, and enter your old password in the 'Current Password' field, then apply the changes."
                "value"  ""
            }dict
            {
                "group"  "Basics"
                "name"   "user_newpass2"
                "type"   "password"
                "label"  "Verify New Password"
                "help"   "To change your login password, enter your new password in the 'New Password' field, re-enter it in this field, and enter your old password in the 'Current Password' field, then apply the changes."
                "vfy_cb" 'password_vfy
                "value"  ""
            }dict
            )
            {
                "group"  "Basics"
                "name"   GENDER_PROP
                "type"   "option"
                "label"  "Character gender"
                "help"   "Select the gender that your character will be.  You can change this later."
                "editable" ALLOW_CUSTOM_GENDERS
                "options" generate_gender_predefineds
                "save_cb" 'stringprop_save
                "value"  obj @ GENDER_PROP getpropstr
            }dict
            {
                "group"  "Basics"
                "name"   SPECIES_PROP
                "type"   "option"
                "label"  "Character species"
                "help"   "Select the species that your character will resemble.  You can change this later."
                "editable" ALLOW_CUSTOM_SPECIES
                "options" generate_species_predefineds
                "save_cb" 'stringprop_save
                "value"  obj @ SPECIES_PROP getpropstr
            }dict
            {
                "group"  "Basics"
                "name"   FLIGHT_PROP
                "type"   "boolean"
                "label"  "Character can fly"
                "help"   "If your character can fly, you should select this."
                "save_cb" 'boolprop_save
                "value"  obj @ FLIGHT_PROP yesprop?
            }dict
            {
                "group"  "Basics"
                "name"   "Jump_OK"
                "type"   "boolean"
                "label"  "Character can throw things"
                "help"   "Select this to allow your character to throw, and receive thrown things."
                "save_cb" 'flag_save
                "value"  obj @ "jump_ok" flag?
            }dict
            {
                "group"  "Basics"
                "name"   HAND_PROP
                "type"   "boolean"
                "label"  "Character can hand/be handed things"
                "help"   "If this is set, your character can hand things to and be handed things by other players."
                "save_cb" 'boolprop_save
                "value"  obj @ HAND_PROP yesprop?
            }dict
        then
        obj @ program? not if
            {
                "group"  "Basics"
                "name"   "@link"
                obj @ case
                    player? when
                        "type"    "dbref"
                        "label"   "Character's home room"
                        "help"    "This lets you select the home room for this player.  A player's home is where they return to when swept, or when they type 'home'."
                        "objtype" { "room" }list
                        "linkable" 1
                        "value"   obj @ getlink
                    end
                    room?   when
                        "type"    "dbref"
                        "label"   "Room's drop-to destination"
                        "help"    "This lets you select where things dropped in this room will be sent to."
                        "objtype" { "room" "nothing" "home" }list
                        "minval"  -3
                        "linkable" 1
                        "value"   obj @ getlink
                    end
                    thing?  when
                        "type"    "dbref"
                        "label"   "Thing's home location"
                        "help"    "This lets you select the home for this thing.  A thing's home is where it returns to when swept, or when the player holding it type 'home'."
                        "objtype" { "player" "room" "thing" }list
                        "linkable" 1
                        "value"   obj @ getlink
                    end
                    exit?   when
                        "type"    "dbreflist"
                        "label"   "Exit's destination(s)"
                        "help"    "When someone uses an exit, one of several things may happen, depending on the type of object that is the exit's destination.\r\rIf it is a room, then the user is moved to that room through the exit.\r\rIf it is a player, and that player is set Jump_OK, the user is moved through to exit to the room the destination player is in.\r\rIf it is a thing, then that thing is moved into the user's inventory.\r\rIf it is a MUF program, then that program is run, with any remaining arguments on the line passed as arguments to the program.\r\rIf it is an exit, then this is a 'meta-link', and that remote exit is triggered as if the user were at the destination exit's location.  Exits triggered this way will move Things to the exit's location, instead of into the user's inventory.  An exit may meta-link to multiple exits, to trigger all of them.  You can specify multiple meta-link destinations by separating them with semicolons (';'s)."
                        "objtype" { "player" "room" "thing" "program" "exit" "home" "nothing" }list
                        "linkable" 1
                        "minval"  -3
                        "value"   obj @ getlinks_array
                    end
                endcase
                "vfy_cb" 'setlink_vfy
                "save_cb" 'setlink_save
            }dict
        then
 
        { (Description group)
            "group"  "Desc"
            "grouplabel" "Description and Look-notify."
            "name"   "@desc"
            "type"   "multistring"
            "label"  "Description"
            "width"  80
            "height" 10
            "help"   "To change the description, edit this field, then apply the changes."
            "save_cb" 'user_desc_save
            "value"  obj @ DESC_PROP DESC_LIST OLD_DESC_LIST multiline_get
        }dict
        {
            "group"  "Desc"
            "name"   LOOK_NOTIFY
            "type"   "boolean"
            "label"  "Tell me when someone looks at me"
            obj @ me @ dbcmp if
                "help"   "If this is enabled, when someone looks at you, you'll be told about it."
            else
                "help"   "If this is enabled, when someone looks at this object, you'll be told about it."
            then
            "save_cb" 'user_desc_save
            "value"   me @ desc "{look-notify" instring if 1 else 0 then
        }dict
 
        obj @ player? if
            { (Say group)
                "group"  "Say"
                "grouplabel" "Speech formatting and options."
                "name"   SAY_PROP
                "type"   "string"
                "label"  "When you speak, you see:     You"
                "width"  40
                "help"   "When you use the say command, this is the way your speech will look to yourself."
                "save_cb" 'stringprop_save
                "value"  obj @ SAY_PROP getpropstr
            }dict
            {
                "group"  "Say"
                "name"   OSAY_PROP
                "type"   "string"
                "label"  obj @ "When you speak, others see:  %D" fmtstring
                "width"  40
                "help"   "When you use the say command, this is the way your speech will look to others in the same room."
                "save_cb" 'stringprop_save
                "value"  obj @ OSAY_PROP getpropstr
            }dict
            {
                "group"  "Say"
                "name"   NORM_SAY_PROP
                "type"   "boolean"
                "label"  "Force other's speech to normal format"
                "help"   "If you have this set, other people's speech will always appear to you in the normal\r    XXX says, \"YYY\"\rformat, regardless of whether they are using split or custom formats."
                "save_cb" 'boolprop_save
                "value"  obj @ NORM_SAY_PROP yesprop?
            }dict
            {
                "group"  "Say"
                "name"   ADHOC_SAY_PROP
                "type"   "boolean"
                "label"  "Enable ad-hoc verbs in your speech"
                "help"   "If you have this set, you can say things with ad-hoc verbs.  For example:\r    \"Hullo there,,squeaks,,How are you?\ror\r    \"Yeeargh!!,,noisily %n shouts,,"
                "save_cb" 'boolprop_save
                "value"  obj @ ADHOC_SAY_PROP yesprop?
            }dict
 
            { (Sweep group)
                "group"  "Sweep"
                "grouplabel" "Sweep messages and options."
                "name"   SWEEP_PROP
                "type"   "multistring"
                "label"  obj @ "When you sweep the room, it says:  %D..." fmtstring
                "help"   "When you sweep the room of one or more idle characters, it says this message, prepended with your name.  This message cam process MPI code."
                "width"  80
                "height" 3
                "value"  obj @ SWEEP_PROP SWEEP_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
            {
                "group"  "Sweep"
                "name"   SWEEPPLYR_PROP
                "type"   "multistring"
                "label"  obj @ "When you sweep a player by name, it says:  %D..." fmtstring
                "help"   "When you sweep a specific player from the room, it says this message, prepended with your name, and with pronoun substitutions honored.  This message cam process MPI code."
                "width"  80
                "height" 3
                "value"  obj @ SWEEPPLYR_PROP SWEEPPLYR_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
            {
                "group"  "Sweep"
                "name"   SWEPT_PROP
                "type"   "multistring"
                "label"  obj @ "When you get swept, it says:  %D..." fmtstring
                "help"   "When you are swept out of the room by someone else, it says this message, prepended with your name, and with pronoun substitutions honored.  This message cam process MPI code."
                "width"  80
                "height" 3
                "value"  obj @ SWEPT_PROP SWEPT_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
        then
 
        { (smell group)
            "group"  "Smell"
            "grouplabel" "Smell messages and options."
            "name"   SCENT_PROP
            "type"   "multistring"
            obj @ player? if
                "label"  "When smelled, your scent is"
                "help"   "When someone smells you, this is what they will see."
            else
                "label"  "When smelled, the scent is"
                "help"   "When someone smells this object, this is what they will see."
            then
            "width"  80
            "height"  5
            "value"  obj @ SCENT_PROP SCENT_LIST "" multiline_get
            "save_cb" 'user_mesg_save
        }dict
        {
            "group"  "Smell"
            "name"   SMELL_NOTIFY
            "type"   "string"
            obj @ player? if
                "label"  "Smell notification message"
                "help"   "When someone smells you, if you have this set, you will see this message with all %n's replaced by the smeller's name, and all other pronoun substitutions honored."
            else
                "label"  "Smell notification message"
                "help"   "When someone smells this item, and you have this set on it, you will see this message with all %n's replaced by the smeller's name, and all other pronoun substitutions honored."
            then
            "width"  40
            "save_cb" 'stringprop_save
            "value"  obj @ SMELL_NOTIFY getpropstr
        }dict
 
        (Only programs don't use Succ/Fail/Drop/Osucc/Ofail/Odrop)
        obj @ program? not if
            "This message may contain MPI code and pronoun substitutions.  Valid pronoun substitutions are:\r    %a for his/hers/its\r    %s for he/she/it\r    %o for him/her/it\r    %p for his/her/its\r    %r for himself/herself/itself\r    %n for the player's name\rYou can use the capitalized substitution to get the capitalized pronoun.  ie: %S for He/She/It."
            var! pronouns_help
            { (Success group)
                "group"  "Succ"
                "grouplabel" "Success messages."
                "name"   "@success"
                "type"   "multistring"
                obj @ case
                    player? when "label" "Message someone sees when they rob you" end
                    room?   when "label" "Success message someone sees when looking at this room" end
                    thing?  when "label" "Message someone sees when they pick this thing up" end
                    exit?   when "label" "Message someone sees when they pass through this exit" end
                endcase
                obj @ case
                    player? when "help" "If someone tries to rob you, and they succeed, passing your @lock test, they will see this message.  This message may contain MPI code." end
                    room?   when "help" "When someone looks at this room, the room's @lock will be checked.  If the looking player passes that test, they will see this message after the room's description.  This message may contain MPI code." end
                    thing?  when "help" "If someone tries to pick up this object, and they pass the @lock test, they will be shown this message as they pick this thing up.  This message may contain MPI code." end
                    exit?   when "help" "If someone tries to go through this exit, and they pass the @lock test, they will be shown this message right before they pass through the exit.  This message may contail MPI code." end
                endcase
                "width"  80
                "height" 5
                "value"  obj @ SUCC_PROP SUCC_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
            {
                "group"  "Succ"
                "name"   "@osuccess"
                "type"   "multistring"
                obj @ case
                    player? when "label" "Message others see when someone robs you" end
                    room?   when "label" "Success message others see when someone looks at this room" end
                    thing?  when "label" "Message others see when someone picks this thing up" end
                    exit?   when "label" "Message others see when someone passes through this exit" end
                endcase
                obj @ case
                    player? when "help" "If someone tries to rob you, and they succeed, passing your @lock test, everyone in the room except them will see this message with the robber's name prepended.  " pronouns_help @ strcat end
                    room?   when "help" "When someone looks at this room, the room's @lock will be checked.  If the looking player passes that test, everyone in the room except them will see this message with the looker's name prepended.  " pronouns_help @ strcat end
                    thing?  when "help" "If someone tries to pick up this object, and they pass the @lock test, everyone in the room except them will be shown this message with the player's name prepended, as they pick this thing up.  " pronouns_help @ strcat end
                    exit?   when "help" "If someone tries to go through this exit, and they pass the @lock test, everyone in the room except them will be shown this message with the player's name prepended, right before they pass through the exit.  " pronouns_help @ strcat end
                endcase
                "width"  80
                "height" 5
                "value"  obj @ OSUCC_PROP OSUCC_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
            { (Fail group)
                "group"  "Fail"
                "grouplabel" "Fail messages."
                "name"   "@fail"
                "type"   "multistring"
                obj @ case
                    player? when "label" "Message someone sees when they fail to rob you" end
                    room?   when "label" "Fail message someone sees when looking at this room" end
                    thing?  when "label" "Message someone sees when they fail to pick this thing up" end
                    exit?   when "label" "Message someone sees when they fail to pass through this exit" end
                endcase
                obj @ case
                    player? when "help" "If someone tries to rob you, and they fail your @lock test, they will see this message.  This message may contain MPI code." end
                    room?   when "help" "When someone looks at this room, the room's @lock will be checked.  If the looking player fails that test, they will see this message after the room's description.  This message may contain MPI code." end
                    thing?  when "help" "If someone tries to pick up this object, and they fail the @lock test, they will be shown this message.  This message may contain MPI code." end
                    exit?   when "help" "If someone tries to go through this exit, and they fail the @lock test, they will be shown this message.  This message may contail MPI code." end
                endcase
                "width"  80
                "height" 5
                "value"  obj @ FAIL_PROP FAIL_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
            {
                "group"  "Fail"
                "name"   "@ofail"
                "type"   "multistring"
                obj @ case
                    player? when "label" "Message others see when someone fails to rob you" end
                    room?   when "label" "Fail message others see when someone looks at this room" end
                    thing?  when "label" "Message others see when someone fails to pick this thing up" end
                    exit?   when "label" "Message others see when someone fails to pass through this exit" end
                endcase
                obj @ case
                    player? when "help" "If someone tries to rob you, and they fail your @lock test, everyone in the room except them will see this message with the robber's name prepended.  " pronouns_help @ strcat end
                    room?   when "help" "When someone looks at this room, the room's @lock will be checked.  If the looking player fails that test, everyone in the room except them will see this message with the looker's name prepended.  " pronouns_help @ strcat end
                    thing?  when "help" "If someone tries to pick up this object, and they fail the @lock test, everyone in the room except them will be shown this message with the player's name prepended.  " pronouns_help @ strcat end
                    exit?   when "help" "If someone tries to go through this exit, and they fail the @lock test, everyone in the room except them will be shown this message with the player's name prepended.  " pronouns_help @ strcat end
                endcase
                "width"  80
                "height" 5
                "value"  obj @ OFAIL_PROP OFAIL_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
            { (Drop group)
                "group"  "Drop"
                "grouplabel" "Drop messages."
                "name"   "@drop"
                "type"   "multistring"
                obj @ case
                    player? when "label" "Message someone sees when they kill you" end
                    room?   when "label" "Message someone sees when they drop something in this room" end
                    thing?  when "label" "Message someone sees when they drop this thing" end
                    exit?   when "label" "Message someone sees after passing through this exit" end
                endcase
                obj @ case
                    player? when "help" "If someone kills you, they will see this message.  This message may contain MPI code." end
                    room?   when "help" "If someone drops something in this room, they will see this message after the dropped item's drop message.  This message may contain MPI code." end
                    thing?  when "help" "If someone drops this object, they will be shown this message.  This message may contain MPI code." end
                    exit?   when "help" "If someone passes through this exit, they will be shown this message after they arrive at the destination.  This message may contail MPI code." end
                endcase
                "width"  80
                "height" 5
                "value"  obj @ DROP_PROP DROP_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
            {
                "group"  "Drop"
                "name"   "@odrop"
                "type"   "multistring"
                obj @ case
                    player? when "label" "Message others see when someone kills you" end
                    room?   when "label" "Message others see when someone drops something in this room" end
                    thing?  when "label" "Message others see when someone drops this thing" end
                    exit?   when "label" "Message others see at destination when someone uses this exit" end
                endcase
                obj @ case
                    player? when "help" "If someone kills you, everyone in the room except them will see this message with the robber's name prepended.  " pronouns_help @ strcat end
                    room?   when "help" "If someone drops something in this room, everyone in the room except them will see this message with the looker's name prepended, after the dropped item's odrop message.  " pronouns_help @ strcat end
                    thing?  when "help" "If someone drops this object, everyone in the room except them will be shown this message with the player's name prepended.  " pronouns_help @ strcat end
                    exit?   when "help" "If someone passes through this exit, everyone in the destination room except them will be shown this message with the player's name prepended.  " pronouns_help @ strcat end
                endcase
                "width"  80
                "height" 5
                "value"  obj @ ODROP_PROP ODROP_LIST "" multiline_get
                "save_cb" 'user_mesg_save
            }dict
        then
 
        ( Flags.  Commented out for now.
        {
            "group"  "Flags"
            "grouplabel" "Flag settings [A to K]."
            "name"   "flag_a"
            "type"   "boolean"
            "label"  "Abode"
            "help"   "Sets the state of the abode flag."
            "value"   me @ "a" flag?
        }dict
        {
            "group"  "Flags"
            "name"   "flag_b"
            "type"   "boolean"
            "label"  "Builder"
            "help"   "Sets the state of the builder flag."
            "value"   me @ "b" flag?
        }dict
        {
            "group"  "Flags"
            "name"   "flag_c"
            "type"   "boolean"
            "label"  "Chown_ok"
            "help"   "Sets the state of the Chown_ok flag."
            "value"   me @ "c" flag?
        }dict
        {
            "group"  "Flags"
            "name"   "flag_d"
            "type"   "boolean"
            "label"  "Dark"
            "help"   "Sets the state of the Dark flag."
            "value"   me @ "d" flag?
        }dict
        {
            "group"  "Flags"
            "name"   "flag_h"
            "type"   "boolean"
            "label"  "Haven"
            "help"   "Sets the state of the Haven flag."
            "value"   me @ "h" flag?
        }dict
        {
            "group"  "Flags"
            "name"   "flag_j"
            "type"   "boolean"
            "label"  "Jump_ok"
            "help"   "Sets the state of the Jump_ok flag."
            "value"   me @ "j" flag?
        }dict
        {
            "group"  "Flags"
            "name"   "flag_k"
            "type"   "boolean"
            "label"  "Kill_ok"
            "help"   "Sets the state of the Kill_ok flag."
            "value"   me @ "k" flag?
        }dict
        {
            "group"  "More Flags"
            "grouplabel" "More flag settings [L to Z]."
            "name"   "flag_l"
            "type"   "boolean"
            "label"  "Link_ok"
            "help"   "Sets the state of the Link_ok flag."
            "value"   me @ "l" flag?
        }dict
        {
            "group"  "More Flags"
            "name"   "flag_q"
            "type"   "boolean"
            "label"  "Quell"
            "help"   "Sets the state of the Quell flag."
            "value"   me @ "q" flag?
        }dict
        {
            "group"  "More Flags"
            "name"   "flag_s"
            "type"   "boolean"
            "label"  "Sticky"
            "help"   "Sets the state of the Sticky flag."
            "value"   me @ "s" flag?
        }dict
        {
            "group"  "More Flags"
            "name"   "flag_v"
            "type"   "boolean"
            "label"  "Vehicle"
            "help"   "Sets the state of the Vehicle flag."
            "value"   me @ "v" flag?
        }dict
        {
            "group"  "More Flags"
            "name"   "flag_w"
            "type"   "boolean"
            "label"  "Wizard"
            "help"   "Sets the state of the Wizard flag."
            "value"   me @ "w" flag?
        }dict
        {
            "group"  "More Flags"
            "name"   "flag_x"
            "type"   "boolean"
            "label"  "Xforcible"
            "help"   "Sets the state of the Xforcible flag."
            "value"   me @ "x" flag?
        }dict
        {
            "group"  "More Flags"
            "name"   "flag_z"
            "type"   "boolean"
            "label"  "Zombie"
            "help"   "Sets the state of the Zombie flag."
            "value"   me @ "z" flag?
        }dict
        )
 
    }list
;
 
 
( -------------------------------------------------------------------------- )
 
 
: main[ str:args -- ]
    "me" match me !
 
    args @ .match_controlled
    dup not if pop exit then
    var! targ
 
    {
        "target" targ @
    }dict var! context
 
    descr context @ 'saveall
    targ @ dup "Object Editor: %D(%d)" fmtstring
    targ @ options_data
    GUI_OPTIONS_PROCESS
;
