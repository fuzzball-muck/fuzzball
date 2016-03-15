#!/usr/bin/wish

package require opt

source ./tree.tcl
source ./combobox.tcl

global HelpData
global document_file
set document_file ""


proc init {args} {
}
init


proc item_num {item pos} {
	set num [lindex $item $pos]
	set num [string trimleft [string range $num 0 2] 0]
	if {$num == ""} {
		set num 0
	}
	return $num
}


proc mk_main_window {base} {
    toplevel $base
    wm withdraw .mw
    wm protocol $base WM_DELETE_WINDOW exit
	wm title $base "Helpfile Meta-Editor"

	image create photo idir -data {
	R0lGODdhEAAQAKIAAP////j4+Hh4eLi4uPj4AAAAAP///////ywAAAAAEAAQ
	AAADbQi63P4wLhSBooqhqxAoQ0ODgqEbIDg0NISDohsgGLqsggUSNEhDQ4M0
	JAUoMTSEQ0NDOFIggTQ0NEhDQ4IFEkM4NDSEQyMFKEFDgzQ0NEhSIIFDQ0M4
	NDSCBRI0SENDgzQkBSi6zIUQWLrMgAkAOw==
	}

	image create photo ifile -data {
	R0lGODdhEAAQAKIAAP///3h4ePj4+Li4uAAAAP///////////ywAAAAAEAAQ
	AAADaQgYuiuCIgAQKLoagSIAECi6OoIhABAog6I7REgAEIIiREJIIkOAAIGi
	y0MIGiJIuiKDBACBostDCBoiSLoigwQAgaLLQwgaIki6IoMEAIGiy0MIGii6
	PISggaLLQwgaOLo8hKBIuryAkAA7
	}

	image create photo hfile -data {
	R0lGODlhEAAQAMIAAAAAALi4uHh4ePj4+P////j4+Pj4+Pj4+CH5BAEKAAcA
	LAAAAAAQABAAAANJeLLM91CNSeeKstIwRBQDoQoBYEKgCBAEsAFoOLjzG6vT
	OpX3bNIk2CNVm7GCPZ1LxBvKVi2bUzRCTqvWDLapCHi/YO5vTBYmAAA7
	}

	image create photo dtfile -data {
	R0lGODlhEAAQAMIAAAAAALi4uHh4ePj4+P///wAAAAAAAAAAACH5BAEKAAcA
	LAAAAAAQABAAAANJeLLM91CNSeeKstIwRBQDoQoBYELguAHoYIJmXLZAl9Yk
	+8ATjs87V2+YoxEBIqAEuegldUtASlgMilTKKRYqCHi/YGVsTNYlAAA7
	}

	image create photo dfile -data {
	R0lGODlhEAAQAMIAAAAAALi4uHh4ePj4+P///wAAAAAAAAAAACH5BAEKAAcA
	LAAAAAAQABAAAANJeLLM91CNSeeKstIwRBQDoQoBYEKgaIIbgIYD0LGk+6Tx
	BIj1q+otXy5GEJWEuwXseIOtKkwJbxTFjXrN66Qa6Hq/UZN4PH4kAAA7
	}

    frame $base.treefr -relief sunk -borderwidth 2
    Tree:create $base.treefr.tree -width 250 -height 350 -yscrollcommand "$base.treefr.scroll set"
    scrollbar $base.treefr.scroll -orient vertical -command "$base.treefr.tree yview"

    pack $base.treefr.tree -side left -fill both -expand 1
    pack $base.treefr.scroll -side left -fill y

    button $base.newfile   -text "New File"    -command "new_filegroup $base"
    button $base.newsect   -text "New Section" -command "new_section $base"
    button $base.newtopic  -text "New Topic"   -command "new_topic $base"
    button $base.edit      -text "Edit"        -command "edit_item $base"
    button $base.massmove  -text "Mass Move"   -command "launch_massmove $base"
    button $base.upsect    -text "^^"          -command "move_item $base -sect"
    button $base.up        -text "^"           -command "move_item $base -1"
    button $base.dn        -text "v"           -command "move_item $base +1"
    button $base.dnsect    -text "vv"          -command "move_item $base +sect"
    button $base.save      -text "Save"        -command "save_helpfile"

    pack $base.treefr -side left -fill both -expand 1
    pack $base.newfile -side top -fill x -padx 10 -pady 5
    pack $base.newsect -side top -fill x -padx 10 -pady 5
    pack $base.newtopic -side top -fill x -padx 10 -pady 5
    pack $base.edit -side top -fill x -padx 10 -pady 5
    pack $base.massmove -side top -fill x -padx 10 -pady 5
    pack $base.save -side bottom -fill x -padx 10 -pady 5
    pack $base.upsect -side left -fill x -padx 10 -pady 5 -anchor nw
    pack $base.up -side left -fill x -padx 10 -pady 5 -anchor nw
    pack $base.dnsect -side right -fill x -padx 10 -pady 5 -anchor ne
    pack $base.dn -side right -fill x -padx 10 -pady 5 -anchor ne
}


proc massmove:move {mw w fnum snum} {
	global HelpData

	set targsect [gdm:ComboBox get $w.cb]
	set targsnum -1
	foreach filenum $HelpData(files) {
		foreach sectnum $HelpData(sections,$filenum) {
			if {$targsect == $HelpData(section,$sectnum)} {
				set targsnum $sectnum
				set targfnum $filenum
				break
			}
		}
	}

	set eorders [$w.lb curselection]
	set enums {}
	foreach eorder $eorders {
		lappend enums [lindex $HelpData(sectionentries,$snum) $eorder]
	}
	foreach eorder [lsort -integer -decreasing $eorders] {
		$w.lb delete $eorder
	}
	foreach enum $enums {
		if {$enum != ""} {
			set eorder [lsearch -exact $HelpData(sectionentries,$snum) $enum]
			set HelpData(sectionentries,$snum) [lreplace $HelpData(sectionentries,$snum) $eorder $eorder]
			set HelpData(sectionentries,$targsnum) [linsert $HelpData(sectionentries,$targsnum) end $enum]
		}
	}
	tree_del_section $mw.treefr.tree $fnum $snum
	tree_del_section $mw.treefr.tree $targfnum $targsnum
	tree_add_section $mw.treefr.tree $fnum $snum
	tree_add_section $mw.treefr.tree $targfnum $targsnum
}


proc massmove_dlog {mainwin filenum sectnum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]

	set section [lindex [split $HelpData(section,$sectnum) "|"] 0]
	set sectorder [lsearch -exact $HelpData(sections,$filenum) $sectnum]

	set w ".massmovedlog"
	toplevel $w
	wm title $w "Mass Move - $section"

	listbox $w.lb -selectmode multiple -yscrollcommand "$w.sb set"
	scrollbar $w.sb -command "$w.lb yview"
	gdm:ComboBox new $w.cb -editable 0 -entrywidth 40
	button $w.move -text "Move" -command "massmove:move $mainwin $w $filenum $sectnum"
	button $w.done -text "Done" -command "destroy $w"

	pack $w.lb -side left -expand 1 -fill both
	pack $w.sb -side left -expand 1 -fill y
	pack $w.cb -side top -expand 1 -fill x -padx 19 -pady 10
	pack $w.move -side top -expand 1 -fill x -padx 10
	pack $w.done -side bottom -expand 1 -fill x -padx 10 -pady 10

	foreach entrynum $HelpData(sectionentries,$sectnum) {
		set topic [lindex [split $HelpData(entrytopics,$entrynum) "|"] 0]
		$w.lb insert end $topic
	}
	foreach fnum $HelpData(files) {
		foreach snum $HelpData(sections,$fnum) {
			gdm:ComboBox list $w.cb insert end $HelpData(section,$snum)
		}
	}
	gdm:ComboBox set $w.cb $HelpData(section,$snum)

	tkwait window $w
}


proc entryedit:destroy {w code} {
	global entryedit_result

	set entryedit_result(code) $code
	set entryedit_result(topics) [$w.topics get]
	set entryedit_result(xref) [$w.xref get]
	set entryedit_result(data) [$w.data get 1.0 "end-1c"]
	destroy $w
}


tcl::OptProc entryedit {
    {-data   {} "The body of the entry's data."}
    {-topics {} "The topics of the entry."}
    {-xref   {} "The topics that this entry cross references."}
} {
	global entryedit_result

	set w ".entryedit"

	toplevel $w
	wm title $w "Entry editor"
	label $w.topicsl -text "Topics"
	entry $w.topics -width 60
	$w.topics insert end $topics
	label $w.xrefl -text "Also see"
	entry $w.xref -width 60
	$w.xref insert end $xref
	text $w.data -width 80 -height 12
	$w.data insert end $data
	button $w.ok -text "Ok" -width 6 -highlightthickness 1 -command "entryedit:destroy $w ok"
	button $w.cancel -text "Cancel" -width 6 -highlightthickness 1 -command "entryedit:destroy $w cancel"

	grid rowconfig $w 0 -minsize 5
	grid rowconfig $w 2 -minsize 5
	grid rowconfig $w 4 -minsize 5
	grid rowconfig $w 5 -weight 1
	grid rowconfig $w 6 -minsize 5
	grid rowconfig $w 8 -minsize 5
	grid columnconf $w 0 -minsize 5
	grid columnconf $w 2 -minsize 5
	grid columnconf $w 3 -weight 1
	grid columnconf $w 4 -minsize 5

	grid $w.topicsl -row 1 -column 1
	grid $w.topics  -row 1 -column 3
	grid $w.xrefl   -row 3 -column 1
	grid $w.xref    -row 3 -column 3
	grid $w.data    -row 5 -column 1 -columnspan 3
	grid $w.ok      -row 7 -column 1
	grid $w.cancel  -row 7 -column 3

	focus $w.topics
	tkwait window $w
	if {[info exists entryedit_result(code)]} {
		set code $entryedit_result(code)
		set topics $entryedit_result(topics)
		set xref $entryedit_result(xref)
		set data $entryedit_result(data)
	} else {
		set code "cancel"
	}
	return [list $code $topics $data $xref]
}


tcl::OptProc requestor {
    {-value  {} "The original text to edit."}
    {-title  {} "The title of the requestor dialog window."}
    {caption {} "The caption to show in the requestor dialog."}
} {
	global requestor_result
	global requestor_value
	set requestor_value $value
	set w ".requestor"
	if {$title == ""} {
		set title "Request"
	}

	toplevel $w
	wm title $w $title
	wm resizable $w 0 0
	label $w.l -text $caption
	entry $w.e -width 40
	$w.e insert end $value
	bind $w.e <Key-Return> "$w.ok invoke; break"
	bind $w.e <Key-Escape> "$w.cancel invoke; break"
	button $w.ok -text "Ok" -width 6 -default active -highlightthickness 1 -command "set requestor_result ok; set requestor_value \[$w.e get\]; destroy $w"
	button $w.cancel -text "Cancel" -width 6 -highlightthickness 1 -command "set requestor_result cancel; destroy $w"

	pack $w.l -side top -expand 1 -fill x -padx 10 -pady 5 -anchor nw
	pack $w.e -side top -expand 1 -fill x -padx 10 -pady 5
	pack $w.ok -side left -padx 10 -pady 5
	pack $w.cancel -side right -padx 10 -pady 5

	focus $w.e
	tkwait window $w
	return [list $requestor_result $requestor_value]
}


proc new_filegroup {w} {
	global HelpData

	set reqres [requestor -title "New File" "What name do you want to give\nthe new file grouping?"]
	if {[lindex $reqres 0] == "ok"} {
		set plainfile [lindex $reqres 1]
		set fnum [incr HelpData(filecount)]
		set HelpData(file,$fnum) $plainfile
		lappend HelpData(files) $fnum
		set HelpData(sections,$fnum) {}
		tree_add_file $w.treefr.tree $fnum
		tree_sel_file $w.treefr.tree $fnum
	}
}


proc new_section {w} {
	global HelpData

	set item [Tree:getselection $w.treefr.tree]
	if {[llength $item] >= 1} {
		set forder [item_num $item 0]
		set fnum [lindex $HelpData(files) $forder]
	} else {
		set fnum [lindex $HelpData(files) end]
	}
	set reqres [requestor -title "New Section" "What name do you want to\ngive the new section?"]
	if {[lindex $reqres 0] == "ok"} {
		set fullname [lindex $reqres 1]
		regsub -all -- " " $fullname "" short
		set reqres [requestor -value $short -title "New Section" "What one-word shortened name do you\nwant to give the new section?"]
		if {[lindex $reqres 0] == "ok"} {
			set shortname [lindex $reqres 1]
			set section "$fullname|$shortname"
			set snum [incr HelpData(sectioncount)]
			set HelpData(section,$snum) $section
			lappend HelpData(sections,$fnum) $snum
			set HelpData(sectionentries,$snum) {}
			tree_add_section $w.treefr.tree $fnum $snum
			tree_sel_section $w.treefr.tree $fnum $snum
		}
	}
}


proc new_topic {w} {
	global HelpData
	set item [Tree:getselection $w.treefr.tree]
	if {[llength $item] >= 2} {
		set forder [item_num $item 0]
		set fnum [lindex $HelpData(files) $forder]
		set sorder [item_num $item 1]
		set snum [lindex $HelpData(sections,$fnum) $sorder]
	} else {
		set fnum [lindex $HelpData(files) end]
		if {![info exists HelpData(sections,$fnum)]} {
			bell
			return
		}
		set snum [lindex $HelpData(sections,$fnum) end]
	}
	set reqres [entryedit]
	if {[lindex $reqres 0] == "ok"} {
		set topics [lindex $reqres 1]
		set data [lindex $reqres 2]
		set xref [lindex $reqres 3]
		set enum [incr HelpData(entrycount)]
		set HelpData(entrytopics,$enum) $topics
		set HelpData(entrytype,$enum) "data"
		set HelpData(entrydata,$enum) $data
		set HelpData(entryxref,$enum) $xref
		lappend HelpData(sectionentries,$snum) $enum
		tree_add_entry $w.treefr.tree $fnum $snum $enum
		tree_sel_entry $w.treefr.tree $fnum $snum $enum
	}
}


proc edit_item {w} {
	global HelpData

	set item [Tree:getselection $w.treefr.tree]
	switch -exact -- [llength $item] {
		1 {
			set forder [item_num $item 0]
			set fnum [lindex $HelpData(files) $forder]
			set reqres [requestor -value $HelpData(file,$fnum) -title "Edit File Grouping" "What name do you want to\ngive the file grouping?"]
			if {[lindex $reqres 0] == "ok"} {
				tree_del_file $w.treefr.tree $fnum
				set plainfile [lindex $reqres 1]
				set HelpData(file,$fnum) $plainfile
				tree_add_file $w.treefr.tree $fnum
				tree_sel_file $w.treefr.tree $fnum
			}
		}
		2 {
			set forder [item_num $item 0]
			set fnum [lindex $HelpData(files) $forder]
			set sorder [item_num $item 1]
			set snum [lindex $HelpData(sections,$fnum) $sorder]
			set reqres [requestor -value $HelpData(section,$snum) -title "Edit Section" "What name do you want\nto give the section?"]
			if {[lindex $reqres 0] == "ok"} {
				tree_del_section $w.treefr.tree $fnum $snum
				set section [lindex $reqres 1]
				set HelpData(section,$snum) $section
				tree_add_section $w.treefr.tree $fnum $snum
				tree_sel_section $w.treefr.tree $fnum $snum
			}
		}
		3 {
			set forder [item_num $item 0]
			set fnum [lindex $HelpData(files) $forder]
			set sorder [item_num $item 1]
			set snum [lindex $HelpData(sections,$fnum) $sorder]
			set eorder [item_num $item 2]
			set enum [lindex $HelpData(sectionentries,$snum) $eorder]
			set reqres [entryedit -data $HelpData(entrydata,$enum) -xref $HelpData(entryxref,$enum) -topics $HelpData(entrytopics,$enum)]
			if {[lindex $reqres 0] == "ok"} {
				tree_del_entry $w.treefr.tree $fnum $snum $enum
				set topics [lindex $reqres 1]
				set data [lindex $reqres 2]
				set xref [lindex $reqres 3]
				set HelpData(entrytopics,$enum) $topics
				set HelpData(entrytype,$enum) "data"
				set HelpData(entrydata,$enum) $data
				set HelpData(entryxref,$enum) $xref
				tree_add_entry $w.treefr.tree $fnum $snum $enum
				tree_sel_entry $w.treefr.tree $fnum $snum $enum
			}
		}
	}
}



proc move_item {w direction} {
	global HelpData

	set item [Tree:getselection $w.treefr.tree]
	if {$direction == "-1" || $direction == "+1"} {
		switch -exact -- [llength $item] {
			1 {
				set forder [item_num $item 0]
				set fnum [lindex $HelpData(files) $forder]
				if {[string index $direction 0] == "+"} {
					set neworder [expr {$forder+1}]
				} else {
					set neworder [expr {$forder-1}]
				}
				if {$neworder != $forder} {
					set HelpData(files) [lreplace $HelpData(files) $forder $forder]
					set HelpData(files) [linsert $HelpData(files) $neworder $fnum]
					tree_refresh $w.treefr.tree
					tree_sel_file $w.treefr.tree $fnum
				}
			}
			2 {
				set forder [item_num $item 0]
				set fnum [lindex $HelpData(files) $forder]
				set sorder [item_num $item 1]
				set snum [lindex $HelpData(sections,$fnum) $sorder]
				if {[string index $direction 0] == "+"} {
					set neworder [expr {$sorder+1}]
				} else {
					set neworder [expr {$sorder-1}]
				}
				if {$neworder != $sorder} {
					tree_del_file $w.treefr.tree $fnum
					set HelpData(sections,$fnum) [lreplace $HelpData(sections,$fnum) $sorder $sorder]
					set HelpData(sections,$fnum) [linsert $HelpData(sections,$fnum) $neworder $snum]
					tree_add_file $w.treefr.tree $fnum
					tree_sel_section $w.treefr.tree $fnum $snum
				}
			}
			3 {
				set forder [item_num $item 0]
				set fnum [lindex $HelpData(files) $forder]
				set sorder [item_num $item 1]
				set snum [lindex $HelpData(sections,$fnum) $sorder]
				set eorder [item_num $item 2]
				set enum [lindex $HelpData(sectionentries,$snum) $eorder]
				if {[string index $direction 0] == "+"} {
					set neworder [expr {$eorder+1}]
				} else {
					set neworder [expr {$eorder-1}]
				}
				if {$neworder != $eorder} {
					tree_del_section $w.treefr.tree $fnum $snum
					set HelpData(sectionentries,$snum) [lreplace $HelpData(sectionentries,$snum) $eorder $eorder]
					set HelpData(sectionentries,$snum) [linsert $HelpData(sectionentries,$snum) $neworder $enum]
					tree_add_section $w.treefr.tree $fnum $snum
					tree_sel_entry $w.treefr.tree $fnum $snum $enum
				}
			}
		}
	} elseif {$direction == "-sect" || $direction == "+sect"} {
		switch -exact -- [llength $item] {
			1 {
				bell
			}
			2 {
				set forder [item_num $item 0]
				set fnum [lindex $HelpData(files) $forder]
				set sorder [item_num $item 1]
				set snum [lindex $HelpData(sections,$fnum) $sorder]
				if {[string index $direction 0] == "+"} {
					set newforder [expr {$forder+1}]
					if {$newforder >= [llength $HelpData(files)]} {
						set newforder [expr {[llength $HelpData(files)] - 1}]
					}
				} else {
					set newforder [expr {$forder-1}]
					if {$newforder < 0} {
						set newforder 0
					}
				}
				set newfnum [lindex $HelpData(files) $newforder]
				if {$newfnum != $fnum} {
					tree_del_file $w.treefr.tree $fnum
					tree_del_file $w.treefr.tree $newfnum
					set HelpData(sections,$fnum) [lreplace $HelpData(sections,$fnum) $sorder $sorder]
					set HelpData(sections,$newfnum) [linsert $HelpData(sections,$newfnum) end $snum]
					tree_add_file $w.treefr.tree $fnum
					tree_add_file $w.treefr.tree $newfnum
					tree_sel_section $w.treefr.tree $fnum $snum
				}
			}
			3 {
				set forder [item_num $item 0]
				set fnum [lindex $HelpData(files) $forder]
				set sorder [item_num $item 1]
				set snum [lindex $HelpData(sections,$fnum) $sorder]
				set eorder [item_num $item 2]
				set enum [lindex $HelpData(sectionentries,$snum) $eorder]
				if {[string index $direction 0] == "+"} {
					set newsorder [expr {$sorder+1}]
					if {$newsorder >= [llength $HelpData(sections,$fnum)]} {
						set newsorder [expr {[llength $HelpData(sections,$fnum)] - 1}]
					}
				} else {
					set newsorder [expr {$sorder-1}]
					if {$newsorder < 0} {
						set newsorder 0
					}
				}
				set newsnum [lindex $HelpData(sections,$fnum) $newsorder]
				if {$newsnum != $snum} {
					tree_del_section $w.treefr.tree $fnum $snum
					tree_del_section $w.treefr.tree $fnum $newsnum
					set HelpData(sectionentries,$snum) [lreplace $HelpData(sectionentries,$snum) $eorder $eorder]
					set HelpData(sectionentries,$newsnum) [linsert $HelpData(sectionentries,$newsnum) end $enum]
					tree_add_section $w.treefr.tree $fnum $snum
					tree_add_section $w.treefr.tree $fnum $newsnum
					tree_sel_entry $w.treefr.tree $fnum $newsnum $enum
				}
			}
		}
	}
	update idletasks
}


proc launch_massmove {w} {
	global HelpData
	set item [Tree:getselection $w.treefr.tree]
	if {[llength $item] >= 2} {
		set forder [item_num $item 0]
		set fnum [lindex $HelpData(files) $forder]
		set sorder [item_num $item 1]
		set snum [lindex $HelpData(sections,$fnum) $sorder]
		massmove_dlog $w $fnum $snum
	} else {
		bell
	}
	return
}



#  ~@TEXT                    TEXT in HTMLfile
#  ~!TEXT                    TEXT in Helpfile
#  ~<TEXT                    TEXT in Textfile, TEXT in helpfile.
#  ~#TEXT                    TEXT in Textfile, ~TEXT in helpfile.
#  ~~file FILENAME           Textfile is FILENAME from here on.
#  ~~sectlist                Insert HTML & Helpfile section index.
#  ~~section SECTIONNAME     Change section name
#  ~~secttopics              Insert HTML & Helpfile index for each section.
#  ~~index                   Insert HTML & Helpfile alphabetical topic index.
#  ~~alsosee TOPIC,TOPIC...  Gives a list of reated crossreferenced entries.
#  ~~code                    The lines following are source code.
#  ~~endcode                 Marks the end of the source code segment.

proc read_helpfile {filename} {
	global HelpData

	array unset HelpData

	set fileadded 0
	set plainfile ""
	set filenum 0

	set sectadded 0
	set section ""
	set sectionnum 0

	set topics ""
	set mode "data"
	set data ""
	set alsosee ""
	set entrynum 0

	set HelpData(title) "Untitled (needs ~~title line)"
	set HelpData(author) "No author (needs ~~author line)"
	set HelpData(doccmd) "No doccmd (needs ~~doccmd line)"

	set f [open $filename "r"]
	while {![eof $f]} {
		gets $f line
		if {[string match -nocase "~*" $line] &&
		    ![string match -nocase "~~code" $line] &&
			![string match -nocase "~~endcode" $line] &&
			![string match -nocase "~~alsosee *" $line]
		} {
			if {$mode == "data" && $topics != ""} {
				set mode "meta"
				set HelpData(entrytopics,$entrynum) $topics
				set HelpData(entrytype,$entrynum) "data"
				set HelpData(entrydata,$entrynum) $data
				set HelpData(entryxref,$entrynum) $alsosee
				lappend HelpData(sectionentries,$sectionnum) $entrynum
				set data ""
				set topics ""
				set alsosee ""
				incr entrynum
			}
			if {[string match -nocase "~~file *" $line]} {
				set mode "meta"
				set plainfile [string trim [string range $line 7 end]]
				incr filenum
				set HelpData(file,$filenum) $plainfile
				lappend HelpData(files) $filenum
				set HelpData(sections,$filenum) {}
				set fileadded 1
			} elseif {[string match -nocase "~~section *" $line]} {
				set mode "meta"
				set section [string trim [string range $line 10 end]]
				incr sectionnum
				set HelpData(section,$sectionnum) $section
				lappend HelpData(sections,$filenum) $sectionnum
				set HelpData(sectionentries,$sectionnum) {}
				set sectadded 1
			} elseif {[string match -nocase "~~title *" $line]} {
				set mode "meta"
				set title [string trim [string range $line 8 end]]
				set HelpData(title) $title
			} elseif {[string match -nocase "~~author *" $line]} {
				set mode "meta"
				set author [string trim [string range $line 9 end]]
				set HelpData(author) $author
			} elseif {[string match -nocase "~~doccmd *" $line]} {
				set mode "meta"
				set doccmd [string trim [string range $line 9 end]]
				set HelpData(doccmd) $doccmd
			} else {
				if {!$sectadded && [info exists HelpData(sectionentries,$sectionnum)]} {
					set HelpData(section,$sectionnum) $section
					lappend HelpData(sections,$filenum) $sectionnum
					set sectadded 1
				}
				if {!$fileadded && [info exists HelpData(sections,$filenum)]} {
					set HelpData(file,$filenum) $plainfile
					lappend HelpData(files) $filenum
					set fileadded 1
				}
			}
		} else {
			if {$mode != "data"} {
				set mode "data"
				set topics [string toupper $line]
			} else {
				if {[string match -nocase "~~alsosee *" $line]} {
					set alsosee [split [string range $line 10 end] ","]
				} else {
					if {$data != ""} {
						append data "\n"
					}
					append data $line
				}
			}
		}
	}
	close $f
	if {$data != ""} {
		if {$mode == "data"} {
			set HelpData(entrytopics,$entrynum) $topics
			set HelpData(entrytype,$entrynum) "data"
			set HelpData(entrydata,$entrynum) $data
			set HelpData(entryxref,$entrynum) $alsosee
			lappend HelpData(sectionentries,$sectionnum) $entrynum
			incr entrynum
		}
	}
	set HelpData(entrycount) $entrynum
	set HelpData(sectioncount) $sectionnum
	set HelpData(filecount) $filenum
}



proc write_helpfile {filename} {
	global HelpData

	set f [open $filename "w"]
	puts $f "~~title $HelpData(title)"
	puts $f "~~author $HelpData(author)"
	puts $f "~~doccmd $HelpData(doccmd)"
	puts $f "~~sectlist"
	puts $f "~~secttopics"
	puts $f "~~index"
	foreach filenum $HelpData(files) {
		puts -nonewline $f "~~file "
		puts $f $HelpData(file,$filenum)
		foreach sectnum $HelpData(sections,$filenum) {
			puts $f "~"
			puts -nonewline $f "~~section "
			puts $f $HelpData(section,$sectnum)
			puts $f "~"
			puts $f "~"
			foreach entrynum $HelpData(sectionentries,$sectnum) {
				set xref $HelpData(entryxref,$entrynum)
				puts $f $HelpData(entrytopics,$entrynum)
				puts $f $HelpData(entrydata,$entrynum)
				if {$xref != ""} {
					puts -nonewline $f "~~alsosee "
					puts $f [join $xref ","]
				}
				puts $f "~"
				puts $f "~"
			}
		}
	}
	close $f
}


proc tree_del_file {w filenum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	Tree:delitem $w [list $plainfile]
}


proc tree_del_section {w filenum sectionnum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	set section [lindex [split $HelpData(section,$sectionnum) "|"] 0]
	set sectorder [lsearch -exact $HelpData(sections,$filenum) $sectionnum]
	set section [format "%03d %s" $sectorder $section]

	Tree:delitem $w [list $plainfile $section]
}


proc tree_del_entry {w filenum sectionnum entrynum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	set section [lindex [split $HelpData(section,$sectionnum) "|"] 0]
	set sectorder [lsearch -exact $HelpData(sections,$filenum) $sectionnum]
	set section [format "%03d %s" $sectorder $section]

	set topic [lindex [split $HelpData(entrytopics,$entrynum) "|"] 0]
	set topicorder [lsearch -exact $HelpData(sectionentries,$sectionnum) $entrynum]
	set topic [format "%03d %s" $topicorder $topic]

	Tree:delitem $w [list $plainfile $section $topic]
}


proc tree_add_file {w filenum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	Tree:newitem $w [list $plainfile] -image idir

	if {[info exists HelpData(sections,$filenum)]} {
	    foreach sectnum $HelpData(sections,$filenum) {
			tree_add_section $w $filenum $sectnum
		}
	}
}


proc tree_add_section {w filenum sectionnum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	set section [lindex [split $HelpData(section,$sectionnum) "|"] 0]
	set sectorder [lsearch -exact $HelpData(sections,$filenum) $sectionnum]
	set section [format "%03d %s" $sectorder $section]

	Tree:newitem $w [list $plainfile $section] -image idir

	if {[info exists HelpData(sectionentries,$sectionnum)]} {
	    foreach entrynum $HelpData(sectionentries,$sectionnum) {
			tree_add_entry $w $filenum $sectionnum $entrynum
		}
	}
}


proc tree_add_entry {w filenum sectionnum entrynum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	set section [lindex [split $HelpData(section,$sectionnum) "|"] 0]
	set sectorder [lsearch -exact $HelpData(sections,$filenum) $sectionnum]
	set section [format "%03d %s" $sectorder $section]

	set topic [lindex [split $HelpData(entrytopics,$entrynum) "|"] 0]
	set topicorder [lsearch -exact $HelpData(sectionentries,$sectionnum) $entrynum]
	set topic [format "%03d %s" $topicorder $topic]

	Tree:newitem $w [list $plainfile $section $topic] -image ifile
}


proc tree_sel_file {w filenum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	Tree:setselection $w [list $plainfile]
}


proc tree_sel_section {w filenum sectionnum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	set section [lindex [split $HelpData(section,$sectionnum) "|"] 0]
	set sectorder [lsearch -exact $HelpData(sections,$filenum) $sectionnum]
	set section [format "%03d %s" $sectorder $section]

	Tree:open $w [list $plainfile]
	Tree:setselection $w [list $plainfile $section]
}


proc tree_sel_entry {w filenum sectionnum entrynum} {
	global HelpData

	set plainfile $HelpData(file,$filenum)
	set fileorder [lsearch -exact $HelpData(files) $filenum]
	set plainfile [format "%03d %s" $fileorder $plainfile]

	set section [lindex [split $HelpData(section,$sectionnum) "|"] 0]
	set sectorder [lsearch -exact $HelpData(sections,$filenum) $sectionnum]
	set section [format "%03d %s" $sectorder $section]

	set topic [lindex [split $HelpData(entrytopics,$entrynum) "|"] 0]
	set topicorder [lsearch -exact $HelpData(sectionentries,$sectionnum) $entrynum]
	set topic [format "%03d %s" $topicorder $topic]

	Tree:open $w [list $plainfile]
	Tree:open $w [list $plainfile $section]
	Tree:setselection $w [list $plainfile $section $topic]
}


proc tree_refresh {w} {
	global HelpData

	foreach child [Tree:children $w {}] {
		Tree:delitem $w $child
	}
	foreach filenum $HelpData(files) {
		tree_add_file $w $filenum
	}
}


proc open_helpfile {{file ""}} {
	global document_file
	if {$file == ""} {
		set filetypes {
			{{Raw Document Files}         {.raw}   TEXT}
			{{All Files}                  *        }
		}
		if {$document_file == ""} {
			set initfile ""
			set initdir [pwd]
		} else {
			set initfile [file tail $document_file]
			set initdir [file dirname $document_file]
		}
        set file [tk_getOpenFile -filetypes $filetypes -title "Open File" \
					-defaultextension ".raw" -initialdir $initdir \
					-initialfile $initfile]
        if {$file == ""} {
            return
		}
	}
	set document_file $file
	read_helpfile $file
	tree_refresh .mw.treefr.tree
}


proc save_helpfile {{file ""}} {
	global document_file
	if {$file == {}} {
		set filetypes {
			{{Raw Document Files}         {.raw}   TEXT}
			{{All Files}                  *        }
		}
		if {$document_file == ""} {
			set initfile "Unknown.raw"
			set initdir [pwd]
		} else {
			set initfile [file tail $document_file]
			set initdir [file dirname $document_file]
		}
        set file [tk_getSaveFile -filetypes $filetypes -title "Save File" \
					-defaultextension ".raw" -initialdir $initdir \
					-initialfile $initfile]
        if {$file == ""} {
            return
        }
	}
	set document_file $file
	write_helpfile $file
}


proc main {argv} {
    wm withdraw .
    mk_main_window .mw
    wm deiconify .mw
    update idletasks
	tkwait visibility .mw
	open_helpfile [lindex $argv 0]
}
main $argv

