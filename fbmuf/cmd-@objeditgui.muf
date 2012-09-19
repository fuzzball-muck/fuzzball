@prog cmd-@objeditgui
1 99999 d
1 i
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
 
$include $lib/case
$include $lib/lmgr
$include $lib/match
$include $lib/optionsinfo
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
    extradata @ "dlogid" [] var! dlogid
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    extradata @ "opts_id" [] var! opts_id
    extradata @ "optionsinfo" [] var! optionsinfo
    optionsinfo @ "@name" [] "value" [] var! username
    targ @ player? if
        optionsinfo @ "@password" []
        dup not if pop 0 array_make_dict then
        "value" [] dup not if
            dlogid @ if
                pop ""
            else
                "Please enter your current password to verify namechange."
                "bold,red,bg_black" textattr
                me @ swap notify
                pop read
                opts_id @ "@password" "value" 4 pick
                optionsinfo_set_indexed
            then
        then
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
    catch
        optname @ "Could not set %s property: %s" fmtstring
        exit
    endcatch
    optname @ "%s property set." fmtstring .tell
    ""
;
 
 
: mlev_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    0 try
        targ @
        val @ intostr
        set
    catch
        val @
        "Could not set Mucker/Priority level to %i: %s." fmtstring
        exit
    endcatch
    val @
    "Set Mucker/Priority level to %i."
    fmtstring .tell
    ""
;
 
 
: flag_save[ dict:extradata str:optname any:val -- str:errs ]
    extradata @ "context" [] var! context
    context @ "target" [] var! targ
    0 try
        targ @
        optname @ 1 strcut pop
        val @ not if "!" swap strcat then
        set
    catch
        optname @
        val @ if "set" else "reset" then
        "Could not %s %s flag: %s" fmtstring
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
 
 
: options_data[ ref:who dbref:obj -- arr:data ]
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
                        "help"    "When someone uses an exit, one of several things may happen, depending on the type of object that is the exit's destination.\r\rIf it is a room, then the user is moved to that room through the exit.\r\rIf it is a player, and that player is set Jump_OK, the user is moved through to exit to the room the destination player is in.\r\rIf it is a thing, then that thing is moved into the user's inventory.\r\rIf it is a MUF program, then that program is run, with any remaining arguments on the line passed as arguments to the program.\r\rIf it is an exit, then this is a 'meta-link', and that remote exit is triggered as if the user were at the destination exit's location.  Exits triggered this way will move Things to the exit's location, instead of into the user's inventory.  An exit may meta-link to multiple exits, to trigger all of them.  You can specify multiple meta-link destinations by seperating them with semicolons (';'s)."
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
 
        obj @ player?
        obj @ thing? or not if
            {
                "group"  "Flags"
                "grouplabel" "Flag settings [A to K]."
                obj @ case
                    program? when
                        "name" "Autostart"
                        "label" "Autostart: Program automatically runs at server start"
                        "help" "When a program is set Autostart, and it is owned by a wizard, it will automatically be run at server startup time, or when the program is recompiled."
                    end
                    exit? when
                        "name" "Abate"
                        "label" "Abate: Exit matching priority is slightly lowered"
                        "help" "An exit with the ABATE flag is a lower priority then one without the ABATE flag, making it lower then the M0 exit priority."
                    end
                    room? when
                        "name" "Abode"
                        "label" "Abode: Room allows other player to set their home to it"
                        "help" "A room with the ABODE flag is available for other players to @link their home to."
                    end
                    ( WORK: figure out and add Thing case. )
                endcase
                "type"     "boolean"
                "value"    obj @ "a" flag?
                "save_cb"  'flag_save
            }dict
        then

        obj @ exit?
        obj @ thing? or not
        obj @ player? not
        who @ "w" flag? or and if
            {
                "group"  "Flags"
                "grouplabel" "Flag settings [A to K]."
                obj @ case
                    program? when
                        "name" "Bound"
                        "label" "Bound: Program is forced to run in preempt mode"
                        "help" "When a program is set Bound, it will automatically be run in preempt mode by default."
                    end
                    room? when
                        "name" "Block"
                        "label" "Block: Prevent player from using personal actions here"
                        "help"  "A room with the Block flag prevents players in it from using personal actions to exit the room."
                    end
                    player? when
                        "name" "Builder"
                        "label" "Builder: Player can create rooms, exits and things"
                        "help" "A player with the Builder flag can use @action, @open, @create and @dig to build."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "b" flag?
                "save_cb" 'flag_save
            }dict
        then

        obj @ program? not if
            {
                "group"  "Flags"
                "grouplabel" "Flag settings [A to K]."
                obj @ case
                    player? when
                        "name" "Color"
                        "label" "Color: Player wishes to see ANSI color codes"
                        "help" "A player with the Color flag will allow ANSI color codes to be sent to them."
                    end
                    default pop
                        "name" "Chown_OK"
                        "label" "Chown_OK: Other players can @Chown this object to themselves"
                        "help" "The Chown_OK flag allows other players to change the ownership of this object to themselves using the @chown command."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "c" flag?
                "save_cb" 'flag_save
            }dict
        then

        obj @ player? not
        who @ "w" flag? or if
            {
                "group"  "Flags"
                "grouplabel" "Flag settings [A to K]."
                obj @ case
                    player? when
                        "name" "Dark"
                        "label" "Dark: Player is invisible and moves silently"
                        "help" "A player with the Dark flag will not show up in the room contents or in the WHO list, and they suppress has arrived/has left message."
                    end
                    program? when
                        "name" "Debug"
                        "label" "Debug: MUF program runs with instruction tracing"
                        "help" "The Debug flag on a program causes it to print out a trace of every instruction it executes, along with what data is on the stack."
                    end
                    exit? when
                        "name" "Dark"
                        "label" "Dark: Exit suppresses arrive/leave messages"
                        "help" "An exit with the Dark flag will suppress the automatic has arrived/has left message."
                    end
                    room? when
                        "name" "Dark"
                        "label" "Dark: Room's contents are invisible"
                        "help" "A room with the Dark flag will not show a contents list, and it will suppress the automatic has arrived/has left messages."
                    end
                    thing? when
                        "name" "Dark"
                        "label" "Dark: Thing is invisible"
                        "help" "A Thing object with the Dark flag will not be publicly listed in the contents of the room, player, or container it is in."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "d" flag?
                "save_cb" 'flag_save
            }dict
        then

        {
            "group"  "Flags"
            "grouplabel" "Flag settings [A to K]."
            obj @ case
                player? when
                    "name"  "Haven"
                    "label" "Haven: Player won't accept pages"
                    "help" "A player with the Haven flag will not receive pages attempts sent to them."
                end
                program? when
                    "name" "HardUID"
                    "label" "HardUID: Run program with permissions of trigger owner"
                    "help" "The HardUID flag, in the absence of the SetUID flag, causes a program to run with the permissions of the trigger owner.  If the SetUID flag is also set, though, and the program is owned by a wizard, it will run with the permissions of the calling program."
                end
                thing? when
                    "name" "Hide"
                    "label" "Hide: Don't show container contents"
                    "help" "The Hide flag prevents the contents of Things from being displayed."
                end
                exit? when
                    "name" "Haven"
                    "label" "Haven: Allow exit to match with arguments when not linked to program"
                    "help" "The Haven flag allows exits to match user commands entered with trailing arguments, even if the exit isn't linked to a MUF program.  This lets you make pure MPI commands that accept {&arg}, without having to link to a $donothing program."
                end
                room? when
                    "name" "Haven"
                    "label" "Haven: Prevent players from being 'kill'ed here"
                    "help" "The Haven flag allows exits to match user commands entered with trailing arguments, even if the exit isn't linked to a MUF program.  This lets you make pure MPI commands that accept {&arg}, without having to link to a $donothing program."
                end
            endcase
            "type"     "boolean"
            "value"    obj @ "h" flag?
            "save_cb" 'flag_save
        }dict

        obj @ exit?
        obj @ program? or not if
            {
                "group"  "Flags"
                "grouplabel" "Flag settings [A to K]."
                obj @ case
                    player? when
                        "name"  "Jump_OK"
                        "label" "Jump_OK: Player can throw/recieve things, and be moved"
                        "help"  "Players with Jump_OK set will allow unprivileged MUF programs to move them to another room, using the MOVETO primitive.  It will also allow throwable things to be thrown into or out of their inventory."
                    end
                    thing? when
                        "name"  "Jump_OK"
                        "label" "Jump_OK: Thing can be thrown"
                        "help"  "Select this to allow this Thing to be thrown."
                    end
                    room? when
                        "name"  "Jump_OK"
                        "label" "Jump_OK: Allow unprivileged MUF programs to move players/things here"
                        "help"  "Rooms with Jump_OK set will allow unprivileged MUF programs to move players and things into or out of it, using the MOVETO primitive."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "j" flag?
                "save_cb" 'flag_save
            }dict
        then

        obj @ player? if
            {
                "group"  "Flags"
                "grouplabel" "Flag settings [A to K]."
                obj @ case
                    player? when
                        "name"  "Kill_OK"
                        "label" "Kill_OK: Player allows others to 'kill' them"
                        "help"  "Players with Kill_OK set will allow other players to use the kill command on them successfully.  Bith the killer and killee must be set Kill_OK, and the room must not be set Haven."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "k" flag?
                "save_cb" 'flag_save
            }dict
        then


        {
            "group"   "More Flags"
            "grouplabel" "More flag settings [L to Z]."
            "name"    "Link_OK"
            "label"   "Link_OK: allows others to @link exits to this"
            "help"    "Objects with the Link_OK flag set allow other players to @link an exit to them."
            "type"    "boolean"
            "value"    obj @ "l" flag?
            "save_cb" 'flag_save
        }dict

        {
            "group"   "More Flags"
            "name"    "Quell"
            "label"   "Quell: Temporarily nullify Wizard flag"
            "help"    "Objects with the Wizard flag can use the Quell flag to temporarily disable the effects of their Wizard flag.  The Quell flag has no meaning when the Wizard flag is not set."
            "type"    "boolean"
            "value"   obj @ "q" flag?
            "save_cb" 'flag_save
        }dict

        obj @ exit? if
            obj @ location thing?
        else
            1
        then
        if
            {
                "group"  "More Flags"
                obj @ case
                    room? when
                        "name"  "Sticky"
                        "label" "Sticky: Send contents to drop-to when last player leaves"
                        "help"  "Rooms with a drop-to set and the Sticky flag will send their contents to the drop-to destination when the last player leaves the room."
                    end
                    exit? when
                        "name"  "Sticky"
                        "label" "Sticky: When exit is triggered, don't send Thing home"
                        "help"  "Exits on Thing objects will normally send that Thing home when they are triggered.  The Sticky flag prevents that."
                    end
                    player? when
                        "name"  "Silent"
                        "label" "Silent: Player doesn't wish to see object dbrefs"
                        "help"  "Players with the Silent flag set will not be shown dbrefs on object they control."
                    end
                    thing? when
                        "name"  "Sticky"
                        "label" "Sticky: Send Thing home when dropped"
                        "help"  "Thing objects with the Sticky flag will be send home when they are dropped."
                    end
                    program? when
                        "name"  "SetUID"
                        "label" "SetUID: Run program with program owner's permissions"
                        "help"  "Programs with the SetUID flag (and no HardUID flag) will be run with the permissions of owner of the program.  If SetUID is used in conjunction with HardUID, and the program is owned by a Wizard, then the program will be run with the permissions of the calling program. (This is useful for libraries.)"
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "s" flag?
                "save_cb" 'flag_save
            }dict
        then

        {
            "group"  "More Flags"
            obj @ case
                room? when
                    "name"  "Vehicle"
                    "label" "Vehicle: Prevent vehicles from entering this room"
                    "help"  "Rooms with the Vehicle flag will not allow Vehicle things to move into them."
                end
                exit? when
                    "name"  "Vehicle"
                    "label" "Vehicle: Prevent vehicles from using this exit"
                    "help"  "Exits with the Vehicle flag will not allow Vehicle things to move through them."
                end
                player? when
                    "name"  "Vehicle"
                    "label" "Vehicle: Player is disallowed from setting Vehicle flag"
                    "help"  "Players with the Vehicle flag are not allowed to set Vehicle flags on Things."
                end
                thing? when
                    "name"  "Vehicle"
                    "label" "Vehicle: Thing is a Vehicle that can be entered by players"
                    "help"  "Thing objects with the Vehicle flag can be entered by players.  To enter a vehicle, you can either use a MUF program to teleport you to it via MOVETO, you can get a wizard to @teleport you into it, or else you an use an action that is both attached and linked to the vehicle to enter it.  This means that you can only enter a vehicle from the same room that it is in, and you cannot use far links to enter it.  This prevents the use of vehicles to get around locks.  Inside the vehicle, you will see it's @idesc, instead of it's @desc, and you will not be shown it's @succ or @fail.  Objects dropped in a vehicle will not go away to the their homes, as a vehicle cannot have a dropto set in it."
                end
                program? when
                    "name"  "Viewable"
                    "label" "Viewable: Allow program to be @list-ed by anyone"
                    "help"  "Programs with the Viewable flag will allow anyone to @list them."
                end
            endcase
            "type"     "boolean"
            "value"    obj @ "v" flag?
            "save_cb" 'flag_save
        }dict

        obj @ player? if
            who @ "w" flag?
        else
            obj @ thing?
            obj @ exit? or not
        then
        if
            {
                "group"  "More Flags"
                obj @ case
                    room? when
                        "name"  "Wizard"
                        "label" "Wizard: Room is root of Realm environment"
                        "help"  "Rooms with the Wizard flag are the root environment rooms for Realms.  Realms are areas of the Muck where an otherwise non-wizard player can have nearly wizardly control to alter or move objects.  The owner of a Wizard flagged room is the Realm controller."
                    end
                    player? when
                        "name"  "Wizard"
                        "label" "Wizard: Player is a Wizard, with elevated privileges"
                        "help"  "Players with the Wizard flag "
                    end
                    program? when
                        "name"  "Wizard"
                        "label" "Wizard: Run program with elevated Wizard permissions"
                        "help"  "Programs with the Wizard flag run with elevated Wizardly permissions, allowing them to do pretty much anything allowed in MUF."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "w" flag?
                "save_cb" 'flag_save
            }dict
        then

        obj @ thing?
        obj @ player? or
        obj @ exit?
        who @ "w" flag? and
        or if
            {
                "group"  "More Flags"
                obj @ case
                    exit? when
                        "name"  "Xpress"
                        "label" "Xpress: Exit can take args without delimiting space"
                        "help"  "Exits with the Xpress flag can be invoked without having to have a space between the exit name and the arguments.  (ie: using '+foo' to run the exit named '+' with the argument 'foo')"
                    end
                    player? when
                        "name"  "XForcible"
                        "label" "XForcible: Player can be @forced by those who pass its @flock"
                        "help"  "Players with the XForcible flag can be @forced by other players who pass a test against the @flock force-lock."
                    end
                    thing? when
                        "name"  "XForcible"
                        "label" "XForcible: Thing can be @forced by those who pass its @flock"
                        "help"  "Thing objects with the XForcible flag can be @forced by players who pass a test against the @flock force-lock."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "x" flag?
                "save_cb" 'flag_save
            }dict
        then


        obj @ player? not
        who @ "w" flag? or if
            {
                "group"  "More Flags"
                obj @ case
                    room? when
                        "name"  "Zombie"
                        "label" "Zombie: Prevent vehicles from entering this room"
                        "help"  "Rooms with the Zombie flag will not allow Zombie things to move into them."
                    end
                    exit? when
                        "name"  "Zombie"
                        "label" "Zombie: Prevent vehicles from using this exit"
                        "help"  "Exits with the Zombie flag will not allow Zombie things to move through them."
                    end
                    player? when
                        "name"  "Zombie"
                        "label" "Zombie: Player is disallowed from setting Zombie flag"
                        "help"  "Players with the Zombie flag are not allowed to set Zombie flags on Things."
                    end
                    thing? when
                        "name"  "Zombie"
                        "label" "Zombie: Thing is a Zombie that echoes what it hears to it's owner"
                        "help"  "Thing objects with the Zombie flag can be entered by players.  To enter a vehicle, you can either use a MUF program to teleport you to it via MOVETO, you can get a wizard to @teleport you into it, or else you an use an action that is both attached and linked to the vehicle to enter it.  This means that you can only enter a vehicle from the same room that it is in, and you cannot use far links to enter it.  This prevents the use of vehicles to get around locks.  Inside the vehicle, you will see it's @idesc, instead of it's @desc, and you will not be shown it's @succ or @fail.  Objects dropped in a vehicle will not go away to the their homes, as a vehicle cannot have a dropto set in it."
                    end
                    program? when
                        "name"  "Zombie"
                        "label" "Zombie: Run program in the MUF debugger"
                        "help"  "Programs with the Zombie flag will run in the MUF debugger when triggered by their owner or a wizard."
                    end
                endcase
                "type"     "boolean"
                "value"    obj @ "z" flag?
                "save_cb" 'flag_save
            }dict
        then
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
    me @ targ @ options_data
    GUI_OPTIONS_PROCESS
;
.
c
q
@register #me cmd-@objeditgui=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=3
@action @objeditgui;@objeditgu;@objeditg;@objedit;@objedi;@objed=#0=tmp/exit1
@link $tmp/exit1=$tmp/prog1

