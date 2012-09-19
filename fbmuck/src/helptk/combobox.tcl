#############################################################################
# ComboBox, by Garth Minette.  Released into the public domain 11/19/97.
# This is a Windows-95 style combo-box for TCL/Tk 8.0 or better.
# Features a scrollable pulldown list, and an optionally editable text field.
#############################################################################

#############################################################################
# TO DO:
#    Moving mouse over dropdown listbox actively selects item under mouse.
#    Add command hook for when entry value is changed.
#############################################################################


global gdmComboBoxModuleLoaded
if {![info exists gdmComboBoxModuleLoaded]} {
set gdmComboBoxModuleLoaded true

proc gdm:ComboBox:Dispell {wname {clicked {}}} {
    global gdmComboBox
    set toplev $gdmComboBox(toplevel)
    if {!$gdmComboBox(ismapped)} {
        return ""
    }

    # $wname.dropbtn config -relief "raised"
    wm transient $toplev {}
	if {$clicked == {}} {
		focus $wname.entry
	} else {
		focus $clicked
	}
    destroy $toplev

    bind [winfo toplevel $wname] <ButtonPress-1> $gdmComboBox(basebind)
    bind $wname.dropbtn <ButtonPress-1> "$wname.dropbtn config -relief \"sunken\" ; gdm:ComboBox:Drop $wname ; break"
	if {[winfo class $wname] == "ComboBox"} {
        bind $wname.entry <ButtonPress-1> {}
	}

    set gdmComboBox(basebind) {}
    set gdmComboBox(ismapped) 0
    set gdmComboBox(currmaster) {}
    set gdmComboBox(listwidget) {}
    set gdmComboBox(toplevel) {}
}

proc gdm:ComboBox:Keypress {wname keycode key} {
    global gdmComboBox
    if {$keycode == "Up"} {
        gdm:ComboBox:ArrowKey $wname -1
        return
    }
    if {$keycode == "Down"} {
        gdm:ComboBox:ArrowKey $wname 1
        return
    }
    if {![regexp -nocase -- {[ -~]} $key]} {
        if {[info exists gdmComboBox($wname,typedchars)]} {
            unset gdmComboBox($wname,typedchars)
        }
        if {[info exists gdmComboBox($wname,keytimer)]} {
            after cancel $gdmComboBox($wname,keytimer)
            unset gdmComboBox($wname,keytimer)
        }
        return
    }
    set mylist $gdmComboBox($wname,list)
    set lastpos $gdmComboBox($wname,selected)
    set lastval $gdmComboBox($wname,selectval)
    set curval [$wname.entry get]

    if {$curval != $lastval} {
        set lastpos {}
        set lastval {}
        set cnt 0
        foreach item $mylist {
            if {[string tolower $item] == [string tolower $curval]} {
                set lastpos $cnt
                set lastval [lindex $mylist $cnt]
                set gdmComboBox($wname,selected) $cnt
                set gdmComboBox($wname,selectval) $lastval
                break
            }
            incr cnt
        }
    }

    if {[info exists gdmComboBox($wname,keytimer)]} {
        after cancel $gdmComboBox($wname,keytimer)
        unset gdmComboBox($wname,keytimer)
    }
    set gdmComboBox($wname,keytimer) [
        after 750 "
            if {\[info exists gdmComboBox($wname,typedchars)\]} {
                unset gdmComboBox($wname,typedchars)
            }
        "
    ]
    set chars {}
    if {[info exists gdmComboBox($wname,typedchars)]} {
        set chars $gdmComboBox($wname,typedchars)
    }
    if {$lastpos == {}} {
        set lastpos 0
    }
    append chars $key
    set chars [string tolower $chars]

    set llen [llength $mylist]
    set slen [string length $chars]
    incr slen -1
    for {set newpos 0} {$newpos < $llen} {incr newpos} {
        set item [lindex $mylist $newpos]
        set posstr [string tolower [string range $item 0 $slen]]
        set test  [string compare $posstr $chars]
        if  {$test == 0} {
            break
        }
    }
    if {$newpos >= $llen} {
        set newpos $lastpos
    }
    set gdmComboBox($wname,typedchars) $chars

    $wname.entry delete 0 end
    $wname.entry insert end [lindex $mylist $newpos]
    $wname.entry selection range 0 end

    set gdmComboBox($wname,selected) $newpos
    set gdmComboBox($wname,selectval) [lindex $mylist $newpos]
    eval "$gdmComboBox($wname,changecmd)"
}

proc gdm:ComboBox:ArrowKey {wname dir} {
    global gdmComboBox
    set mylist $gdmComboBox($wname,list)
    set lastpos $gdmComboBox($wname,selected)
    set lastval $gdmComboBox($wname,selectval)
    set curval [$wname.entry get]
    if {[info exists gdmComboBox($wname,typedchars)]} {
        unset gdmComboBox($wname,typedchars)
    }
    if {[info exists gdmComboBox($wname,keytimer)]} {
        after cancel $gdmComboBox($wname,keytimer)
        unset gdmComboBox($wname,keytimer)
    }

    if {$curval != $lastval} {
        set lastpos {}
        set lastval {}
        set cnt 0
        foreach item $mylist {
            if {$item == $curval} {
                set lastpos $cnt
                set lastval [lindex $mylist $cnt]
                set gdmComboBox($wname,selected) $cnt
                set gdmComboBox($wname,selectval) $lastval
                break
            }
            incr cnt
        }
    }
    if {$lastpos == {}} {
        if {$dir > 0} {
            $wname.entry delete 0 end
            $wname.entry insert end [lindex $mylist 0]
            $wname.entry selection range 0 end

            set gdmComboBox($wname,selected) 0
            set gdmComboBox($wname,selectval) [lindex $mylist 0]
            eval "$gdmComboBox($wname,changecmd)"
        }
        return
    }
    set newpos [expr {$lastpos + $dir}]
    set llen [llength $mylist]
    if {$newpos >= $llen} {
        set newpos [expr {$llen - 1}]
    }
    if {$newpos < 0} {
        set newpos 0
    }
    $wname.entry delete 0 end
    $wname.entry insert end [lindex $mylist $newpos]
    $wname.entry selection range 0 end

    set gdmComboBox($wname,selected) $newpos
    set gdmComboBox($wname,selectval) [lindex $mylist $newpos]
    eval "$gdmComboBox($wname,changecmd)"
}

proc gdm:ComboBox:Motion {wname} {
    global gdmComboBox
    set toplev $gdmComboBox(toplevel)

    set x [winfo pointerx $toplev.list]
    set y [winfo pointery $toplev.list]
    if {[winfo containing $x $y] == "$wname.dropbtn"} {
        set dstate "sunken"
    } else {
        set dstate "raised"
    }
    if {[$wname.dropbtn cget -relief] != "$dstate"} {
        $wname.dropbtn config -relief "$dstate"
    }
    incr x -[winfo rootx $toplev.list]
    incr y -[winfo rooty $toplev.list]
    if {$y < -1} {
        $toplev.list yview scroll -1 unit
    } elseif {$y > [winfo height $toplev.list]} {
        $toplev.list yview scroll 1 unit
    }
    if {$x >= 0 && $x < [winfo width $toplev.list]} {
        $toplev.list selection clear 0 end
        $toplev.list selection set @$x,$y
    }
}

proc gdm:ComboBox:Select {wname} {
    global gdmComboBox
    set toplev $gdmComboBox(toplevel)
    set cursel [$toplev.list curselection]
    if {$cursel != {} && $cursel >= 0} {
        $wname.entry delete 0 end
        set gdmComboBox($wname,selected) $cursel
        set curtext [$toplev.list get $cursel]
        set gdmComboBox($wname,selectval) $curtext
        $wname.entry insert end $curtext
        $wname.entry selection range 0 end
        eval "$gdmComboBox($wname,changecmd)"
    }
    after 10 "gdm:ComboBox:Dispell $wname"
    return ""
}

proc gdm:ComboBox:FocusOut {toplev wname} {
    switch -glob -- [focus] {
        $toplev* { return }
        $widget* { return }
        default  {
            after 10 "gdm:ComboBox:Dispell $wname"
        }
    }
}

proc gdm:ComboBox:Drop {wname} {
    global gdmComboBox
	set wtop [winfo toplevel $wname]
    set toplev .gdmcombodropdown

    if {$gdmComboBox(ismapped)} {
        after 10 "gdm:ComboBox:Dispell $wname"
        return ""
    }
    if {[llength $gdmComboBox($wname,list)] == 0} {
        $wname.dropbtn config -relief "raised"
        return ""
    }

    bind $wname.dropbtn <ButtonPress-1> "gdm:ComboBox:Dispell $wname ; break"
	if {[winfo class $wname] == "ComboBox"} {
        bind $wname.entry <ButtonPress-1> {gdm:ComboBox:Dispell [gdm:ComboBox:Find %W] ; break}
	}

    toplevel $toplev -cursor top_left_arrow
    wm overrideredirect $toplev 1
    wm transient $toplev $wtop

    set listbg [$wname.entry cget -background]
    listbox $toplev.list -exportselection 0 -background $listbg \
        -highlightthickness 0 -relief solid -borderwidth 1

    foreach item $gdmComboBox($wname,list) {
        $toplev.list insert end $item
    }
    set itemh [expr {[lindex [$toplev.list bbox 0] 3] + 3}]
    if {[string tolower $gdmComboBox($wname,selectval)] != [string tolower [$wname.entry get]]} {
        set gdmComboBox($wname,selected) {}
    }
    if {$gdmComboBox($wname,selected) == {}} {
        set curtext [$wname.entry get]
        set cnt 0
        foreach item [$toplev.list get 0 end] {
            if {[string tolower $item] == [string tolower $curtext]} {
                set gdmComboBox($wname,selected) $cnt
                break
            }
            incr cnt
        }
    }

    $toplev.list selection clear 0 end
    if {$gdmComboBox($wname,selected) != {}} {
        $toplev.list selection set $gdmComboBox($wname,selected)
        $toplev.list activate $gdmComboBox($wname,selected)
        $toplev.list see $gdmComboBox($wname,selected)
    } else {
        $toplev.list activate -1
        $toplev.list see 0
    }

    set scrollflag 0
    set x [winfo rootx ${wname}]
    set y [expr {[winfo rooty ${wname}] + [winfo height ${wname}]}]
    set w [expr {[winfo width ${wname}] - 2}]
    if {$itemh != ""} {
        set lcnt [$toplev.list size]
        set h [expr {($itemh * $lcnt) + 2}]
        if {$h > 200} {
            set h [expr {(int(200 / $itemh) * $itemh) + 2}]
            set scrollflag 1
        }
    } else {
        set h 20
        set scrollflag 1
    }
	if {$y + $h > [winfo screenheight $wname]} {
		set y [expr {$y - ($h + [winfo height $wname])}]
	}
    wm geometry $toplev ${w}x${h}+$x+$y

    bind $toplev <FocusOut> "gdm:ComboBox:FocusOut $toplev $wname"
    bind $toplev <Key-Escape> "gdm:ComboBox:FocusOut $toplev $wname"
    bind $toplev.list <ButtonRelease-1> "gdm:ComboBox:Select $wname ; break"
    bind $toplev.list <Key-Return> "gdm:ComboBox:Select $wname ; break"
    bind $toplev.list <B1-Motion> "gdm:ComboBox:Motion $wname ; break"

    set gdmComboBox(basebind) [bind $wtop <ButtonPress-1>]
	bind $wtop <ButtonPress-1> "+gdm:ComboBox:Dispell $wname %W"

    grid rowconfig $toplev 0 -weight 1
    grid columnconfig $toplev 0 -weight 1
    grid $toplev.list -row 0 -column 0 -sticky nsew
    if {$scrollflag} {
        $toplev.list configure -yscrollcommand [list $toplev.scroll set]
        scrollbar $toplev.scroll -orient vertical -width 10 -borderwidth 1 \
            -command [list $toplev.list yview]
        grid $toplev.scroll -row 0 -column 1 -sticky nse
    }

    #update idletasks
    focus $toplev.list
    # grab $toplev
    
    set gdmComboBox(currmaster) $wname
    set gdmComboBox(ismapped) 1
    set gdmComboBox(listwidget) $toplev.list
    set gdmComboBox(toplevel) $toplev

    set motioncmd "
        if {\$gdmComboBox(ismapped) &&
            !\[string match \"$wname*\" \[winfo containing %X %Y\]\]} {
            focus $toplev.list
            event generate $toplev.list <B1-Motion> -when head
        }
        break
    "
    bind $wname.entry <B1-Motion> $motioncmd
    bind $wname.dropbtn <B1-Motion> $motioncmd

    set releasecmd "
        if {\$gdmComboBox(ismapped) &&
            \[winfo containing %X %Y\] == \"$toplev.list\"} {
            focus $toplev.list
            event generate $toplev.list <ButtonRelease-1> -when head
        }
        $wname.dropbtn config -relief \"raised\"
        break
    "
    bind $wname.entry <ButtonRelease-1> $releasecmd
    bind $wname.dropbtn <ButtonRelease-1> $releasecmd
}

proc gdm:ComboBox:list {wname opt args} {
    global gdmComboBox
    set entries $gdmComboBox($wname,list)
    switch -exact -- $opt {
        size {
            return [llength $entries]
        }
        get {
            return [eval "lrange $entries $args"]
        }
        insert {
            set gdmComboBox($wname,list) [eval "linsert [list $entries] $args"]
            set gdmComboBox($wname,selected) {}
        }
        delete {
            set gdmComboBox($wname,list) [eval "lreplace [list $entries] $args"]
            set gdmComboBox($wname,selected) {}
        }
    }
    if {$gdmComboBox(ismapped)} {
        return [eval "$gdmComboBox(listwidget) $opt $args"]
    }
    return ""
}

proc gdm:ComboBox:Proxy {wname opt args} {
    global gdmComboBox
    switch -exact -- $opt {
        entry { eval "$wname.entry $args"}
        dropbtn { eval "$wname.dropbtn $args"}
        list { eval "gdm:ComboBox:list $wname $args" }
        all {
            eval "$wname.entry $args"
            eval "$wname.dropbtn $args"
            eval "gdm:ComboBox:list $wname $args"
            eval "$wname $args"
        }
        frame -
        default { eval "$wname $args"}
    }
}

proc gdm:ComboBox:Get {wname} {
    return [$wname.entry get]
}

proc gdm:ComboBox:Set {wname text} {
    $wname.entry delete 0 end
    $wname.entry insert end $text
    $wname.entry selection range 0 end
}


proc gdm:ComboBox:Find {wname} {
	while {$wname != "."} {
		set class [winfo class $wname]
		if {$class == "ComboBox" || $class == "ComboBoxEditable"} {
			return $wname
		}
		set wname [winfo parent $wname]
	}
	return ""
}


image create bitmap decrimg -data \
{#define decr_width 7
#define decr_height 4
static unsigned char decr_bits[] = {
   0x7f, 0x3e, 0x1c, 0x08};}

proc gdm:ComboBox:New {wname args} {
    global gdmComboBox gdmBitMaps

    set frame ${wname}
    set entry $frame.entry
    set dropbtn $frame.dropbtn
    set decrimg "decrimg"

	set comboclass "ComboBoxEditable"
    set comboargs {}
    set frameargs {}
    set entryargs {}
    set listargs {}
    set dropbtnargs {}
    set allargs {}

    foreach {opt val} $args {
        switch -glob -- $opt {
            -dropbtn* { lappend dropbtnargs -[string range $opt 8 end] $val }
            -frame*   { lappend frameargs   -[string range $opt 6 end] $val }
            -entry*   { lappend entryargs   -[string range $opt 6 end] $val }
            -list*    { lappend listargs    -[string range $opt 5 end] $val }
            default   { lappend comboargs $opt $val }
        }
    }

    set gdmComboBox($wname,changecmd) {}
    foreach {opt val} $comboargs {
        switch -glob -- $opt {
			-editable {
				if {$val == "1" || $val == "true"} {
					set comboclass "ComboBoxEditable"
				} elseif {$val == "0" || $val == "false"} {
					set comboclass "ComboBox"
				}
			}
            -changecommand { set gdmComboBox($wname,changecmd) $val }
            -bg -
            -background { lappend allargs -background $val -highlightbackground $val }
            -fg -
            -foreground { lappend allargs -foreground $val }
            default { error "gdm:ComboBox: invalid option $opt" }
        }
    }

    eval "frame $frame -class $comboclass -relief sunken -bd 2 $allargs $frameargs"
    eval "entry $entry -relief flat -borderwidth 0 $allargs $entryargs"
    eval "button $dropbtn -width 11 -highlightthickness 0 -takefocus 0 \
              -image $decrimg $allargs $dropbtnargs"

	bindtags $frame  [list [winfo toplevel $wname] $frame $wname $comboclass all]
	bindtags $entry  [list [winfo toplevel $wname] $entry $wname $comboclass Entry all]
	bindtags $dropbtn [list [winfo toplevel $wname] $dropbtn $wname Button $comboclass all]

    if {$comboclass == "ComboBox"} {
        $entry configure -takefocus 1 -highlightthickness 1 -cursor left_ptr -insertwidth 0 -insertontime 0
    }
    bind $dropbtn <ButtonPress-1> "$dropbtn config -relief \"sunken\" ; gdm:ComboBox:Drop $frame ; break"

    grid columnconfig $frame 0 -weight 1
    grid rowconfig $frame 0 -weight 1
    grid $entry   -in $frame -column 0 -row 0 -sticky nsew
    grid $dropbtn -in $frame -column 1 -row 0 -sticky nsew
    
    set gdmComboBox($wname,list) {}
    set gdmComboBox($wname,selected) {}
    set gdmComboBox(ismapped) 0
    set gdmComboBox(currmaster) {}
    set gdmComboBox(listwidget) {}
    set gdmComboBox(toplevel) {}
    set gdmComboBox($wname,selectval) {}
    return $wname
}

proc gdm:ComboBox {opt args} {
    switch -exact -- $opt {
        sub -
        subwid -
        subwidget { return [eval "gdm:ComboBox:Proxy $args"] }
        list { return [eval "gdm:ComboBox:list $args"] }
        new { return [eval "gdm:ComboBox:New $args"] }
        set { return [eval "gdm:ComboBox:Set $args"] }
        get { return [eval "gdm:ComboBox:Get $args"] }
        default { error "gdm:ComboBox: option \"$opt\" should be subwidget, get, set, or new." }
    }
}


foreach comboclass [list ComboBox ComboBoxEditable] {
	bind $comboclass <Shift-Key-Tab> {focus [Notebook:nextfocus %W -1]}
	bind $comboclass <Key-Tab>     {focus [Notebook:nextfocus %W 1]}
	bind $comboclass <Alt-Key>     {continue}
	#bind $comboclass <Shift-Key>   {continue}
	bind $comboclass <Control-Key> {continue}
	bind $comboclass <Key-Escape>  {continue}
}
bind ComboBox <ButtonPress-1>    {gdm:ComboBox:Drop [gdm:ComboBox:Find %W] ; break}
bind ComboBox <Key>              {gdm:ComboBox:Keypress [gdm:ComboBox:Find %W] %K %A ; break}
bind ComboBoxEditable <Key-Up>   {gdm:ComboBox:ArrowKey [gdm:ComboBox:Find %W] -1 ; break}
bind ComboBoxEditable <Key-Down> {gdm:ComboBox:ArrowKey [gdm:ComboBox:Find %W] 1 ; break}
bind ComboBoxEditable <Key>      {after 10 "$gdmComboBox([gdm:ComboBox:Find %W],changecmd)"; continue}




if {0} {
    global footext onet twot
    label .fool -textvariable footext
    pack .fool

    gdm:ComboBox new .chkbx -entrytextvariable onet -changecommand {global onet footext; set footext $onet}
    foreach item {One Two Three Four Five Six Seven Eight Nine Ten Eleven Twelve Thirteen Fourteen Fifteen} {
        gdm:ComboBox list .chkbx insert end $item
    }
    pack .chkbx

    gdm:ComboBox new .cb -bg #c0c000 -entrytextvariable twot -editable 0 -changecommand {global twot footext; set footext $twot}
    foreach item {Aye Bee Cee Dee Eee Eff Jee Ach Eiy Jay Kay Ell} {
        gdm:ComboBox list .cb insert end $item
    }
    pack .cb
}

}

