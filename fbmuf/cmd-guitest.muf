@prog cmd-guitest
1 99999 d
1 i
$include $lib/gui
$def tell .tell
$def }join } array_make "" array_join
 
$def URL1 "http://www.belfry.com/pics/revhead.gif"
$def URL2 "http://www.belfry.com/pics/foxen.gif"
$def URL3 "http://www.belfry.com/pics/revar_sm.gif"
$def URL4 "http://www.belfry.com/pics/belfry.jpg"
  
: generic_handler[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ GUI_VALUES_GET var! vals
    context @ "descr" [] var! dscr
    
    { ctrlid @ " sent " event @ " event!" }join tell 
  
    vals @ foreach
        swap " = " strcat swap "\r" array_join strcat
        "\r    "  "\r" subst
        tell
    repeat
    0
;
 
: change_image[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ "imageurl" GUI_VALUE_GET var! newurl
    dlogid @ "varimg" newurl @ GUI_VALUE_SET
    0
;
 
: listbox_add[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ "listbox_add_data" GUI_VALUE_GET var! data
    dlogid @ "lbox" GUI_VALUE_GET 0 [] var! pos
    dlogid @ "lbox" "insert" {
        "values" data @
        "before" pos @
    }dict
    GUI_CTRL_COMMAND
    0
;
 
: listbox_del[ dict:context str:dlogid str:ctrlid str:event -- int:exit ]
    dlogid @ "lbox" GUI_VALUE_GET var! items
    dlogid @ "lbox" "delete" { "items" items @ }dict GUI_CTRL_COMMAND
    0
;
 
: gen_reader_dlog[ -- dict:Handlers str:DlogId ]
    {SIMPLE_DLOG "GUI Controls Test Dialog"
        "resizable" "both"
        {MENU "filemenu"
            "text" "&File"
            {CHECKBOX "menucbox"
                "text" "&Checkbox item"
                "value" 1
            }CTRL
            {MENU "menucascade"
                "text" "Cascading && expanding &menu"
                {RADIO "menurad1_1"
                    "text" "Radio button &1"
                    "selvalue" 1
                    "valname" "menuradval1"
                }CTRL
                {RADIO "menurad1_2"
                    "text" "Radio button &2"
                    "selvalue" 2
                    "value" 2
                    "valname" "menuradval1"
                }CTRL
                {RADIO "menurad1_3"
                    "text" "Radio button &3"
                    "selvalue" 3
                    "valname" "menuradval1"
                }CTRL
                {HRULE "" }CTRL
                {RADIO "menurad2_1"
                    "text" "Radio button &A"
                    "selvalue" "A"
                    "valname" "menuradval2"
                }CTRL
                {RADIO "menurad2_2"
                    "text" "Radio button &B"
                    "selvalue" "B"
                    "valname" "menuradval2"
                }CTRL
                {RADIO "menurad2_3"
                    "text" "Radio button &C"
                    "selvalue" "C"
                    "valname" "menuradval2"
                    "value" "A"
                }CTRL
            }MENU
            {HRULE "" }CTRL
            {BUTTON "menubutton"
                "text" "E&xit"
                "dismiss" 1
                "bindkey" "<Key-Escape>"
            }CTRL
        }MENU
        {MENU "editmenu"
            "text" "&Edit"
            {BUTTON "cut"
                "text" "&Cut"
                "dismiss" 0
            }CTRL
            {BUTTON "copy"
                "text" "Cop&y"
                "dismiss" 0
            }CTRL
            {BUTTON "paste"
                "text" "&Paste"
                "dismiss" 0
            }CTRL
        }MENU
        {DATUM "datum"
            "value" "Hidden"
        }CTRL
        {NOTEBOOK "nb"
            "hweight" 1
            "vweight" 1
            "sticky" "nsew"
            {PANE "img" "Images"
                {LABEL ""
                    "value" "The images below are deliberately cropped."
                    "maxwidth" 600
                    "colspan" 3
                    "newline" 1
                }CTRL
                {IMAGE "pic1"
                    "value" URL1
                    "width" 118
                    "height" 121
                    "sticky" "n"
                    "newline" 0
                    "vweight" 1
                    "hweight" 1
                }CTRL
                {IMAGE "varimg"
                    "value" URL2
                    "width" 200
                    "height" 300
                    "newline" 0
                    "rowspan" 2
                    "sticky" "nsew"
                }CTRL
                {IMAGE "revgif"
                    "value" URL3
                    "width" 200
                    "height" 300
                    "newline" 1
                    "rowspan" 2
                    "sticky" "nsew"
                    "leftpad" 0
                }CTRL
                {IMAGE ""
                    "value" URL1
                    "width" 118
                    "height" 121
                    "sticky" "se"
                    "toppad" 0
                    "vweight" 1
                    "hweight" 1
                }CTRL
                {COMBOBOX "imageurl"
                    "value" URL2
                    "editable" 0
                    "options" {
                        URL1
                        URL2
                        URL3
                        URL4
                    }list
                    "colspan" 2
                    "sticky" "ew"
                    "report" 1
                    "|valchanged"  'change_image
                    "|buttonpress" 'change_image
                    "newline" 0
                }CTRL
            }PANE
            {PANE "buttons" "Buttons, etc."
                {FRAME "bfr"
                    "newline" 0
                    "visible" 1
                    "text" "Buttons"
                    "sticky" "nsew"
                    {BUTTON "Btn1"
                        "text" "&One"
                        "width" 8
                        "sticky" "n"
                        "dismiss" 0
                        "|buttonpress" 'generic_handler
                    }CTRL
                    {BUTTON "Btn2"
                        "text" "&Two"
                        "width" 8
                        "sticky" "n"
                        "dismiss" 0
                        "|buttonpress" 'generic_handler
                    }CTRL
                }CTRL
                {FRAME "cbfr"
                    "text" "Checkboxes"
                    "visible" 1
                    "newline" 0
                    "sticky" "nsew"
                    {CHECKBOX "cb1"
                        "text" "Checkbox o&ne"
                        "value" 0
                    }CTRL
                    {CHECKBOX "cb2"
                        "text" "Checkbox T&wo"
                        "value" 1
                    }CTRL
                    {CHECKBOX "cb3"
                        "text" "Checkbox T&hree"
                        "value" 0
                    }CTRL
                }CTRL
                {FRAME "radfr"
                    "text" "Radio Buttons"
                    "visible" 1
                    "sticky" "nsew"
                    {RADIO "rad1"
                        "text" "Rad&io button one"
                        "valname" "radval"
                        "value" 2
                        "selvalue" 1
                    }CTRL
                    {RADIO "rad2"
                        "text" "&Radio button two"
                        "valname" "radval"
                        "selvalue" 2
                    }CTRL
                    {RADIO "rad3"
                        "text" "R&adio button three"
                        "valname" "radval"
                        "selvalue" 3
                    }CTRL
                }CTRL
            }PANE
            {PANE "listbox" "Listbox"
                {LABEL ""
                    "value" "Listbox"
                    "sticky" "w"
                    "newline" 1
                }CTRL
                {LISTBOX "lbox"
                    "value" "0"
                    "sticky" "nswe"
                    "options" {
                        "Revar       Writing Gui Programs in MUF"
                        "Fre'ta      Scripting in Trebuchet"
                        "Points      Floating point error checking in MUF"
                    }list
                    "font" "fixed"
                    "selectmode" "extended" ( can be single, multiple, or extended)
                    "report"   1
                    "height"   5
                    "width"   60
                    "newline"  1
                    "colspan"  3
                    "toppad"   0
                }CTRL
                {EDIT "listbox_add_data"
                    "value" "Sample"
                    "sticky" "ew"
                    "width"  30
                    "hweight" 1
                    "newline" 0
                }CTRL
                {BUTTON "ListAdd"
                    "text" "&Add"
                    "width" 8
                    "sticky" "w"
                    "dismiss" 0
                    "newline" 0
                    "|buttonpress" 'listbox_add
                }CTRL
                {BUTTON "ListDel"
                    "text" "&Delete"
                    "width" 8
                    "sticky" "w"
                    "dismiss" 0
                    "|buttonpress" 'listbox_del
                }CTRL
            }PANE
            {PANE "multi" "Text Editing"
                {EDIT "edit"
                    "text" "Edit control"
                    "value" "Sample"
                    "sticky" "ew"
                    "width"  30
                    "hweight" 1
                    "newline" 0
                }CTRL
                {COMBOBOX "combo"
                    "text" "Combobox"
                    "value" "Example"
                    "editable" 1
                    "width"   12
                    "options" "Zero\rOne\rTwo\rThree\rFour\rFive\rSix\rSeven\rEight\rNine"
                    "hweight" 1
                    "sticky" "ew"
                }CTRL
                {LABEL ""
                    "value" "Multiedit"
                    "sticky" "w"
                }CTRL
                {MULTIEDIT "medit"
                    "value" "This is a test\rof multi-line\rediting."
                    "width"   60
                    "height"  10
                    "readonly" 0
                    "toppad"   0
                    "colspan"  4
                    "sticky"   "nsew"
                    "vweight"  1
                }CTRL
            }PANE
            {PANE "numbers" "Numbers"
                {LABEL "lblspin1"
                    "value" "Spinner one"
                    "newline" 0
                    "sticky" "w"
                }CTRL
                {SPINNER "spin1"
                    "value"   50
                    "minval"   0
                    "maxval" 100
                    "width"    4
                    "newline"  0
                    "sticky" "w"
                    "hweight"  1
                }CTRL
                {VRULE ""
                    "newline" 0
                    "rowspan" 3
                }CTRL
                {SCALE "scale1"
                    "text" "Scale one"
                    "value"       50.0
                    "minval"       0.0
                    "maxval"     100.0
                    "length"     100
                    "interval"    50.0
                    "resolution"   1.0
                    "digits"    3
                    "orient" "vert"
                    "rowspan"   3
                    "sticky"  "w"
                    "hweight" 100
                }CTRL
                {HRULE ""
                    "colspan" 2
                }CTRL
                {SCALE "scale2"
                    "text" "Scale two"
                    "value"       50.0
                    "minval"       0.0
                    "maxval"     100.0
                    "length"     150
                    "interval"    50.0
                    "resolution"   1.0
                    "digits"       3
                    "orient"    "horiz"
                    "colspan"      2
                }CTRL
            }PANE
        }CTRL
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
  
: gui_test[ str:arg -- ]
    DESCR GUI_AVAILABLE 0.0 > if
        preempt (For timing purposes only.)
        gen_reader_dlog swap gui_dlog_register
        background
        gui_event_process
        pop pop
        "Done." tell
    else
        ( Put in old-style config system here. )
		"Gui not supported!" tell
    then
;
.
c
q
@register #me cmd-guitest=tmp/prog1
@set $tmp/prog1=W
@set $tmp/prog1=L
@set $tmp/prog1=2
@propset $tmp/prog1=str:/_/de:A scroll containing a spell called cmd-guitest
