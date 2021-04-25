#!/usr/bin/wish

global	share
set	share "/usr/share"

proc replaced {product dev} {
    global share

    set xqx [file join $share foo2xqx firmware ]

    #puts "$product $dev"
    switch $product {
    "HP LaserJet 1018" 	{ exec usb_printerid $dev }
    "HP LaserJet 1020" 	{ exec usb_printerid $dev }
    "HP LaserJet P1005"	{ exec cp [file join $xqx sihpP1005.dl] $dev }
    "HP LaserJet P1007"	{ exec cp [file join $xqx sihpP1005.dl] $dev }
    "HP LaserJet P1006"	{ exec cp [file join $xqx sihpP1006.dl] $dev }
    "HP LaserJet P1008"	{ exec cp [file join $xqx sihpP1006.dl] $dev }
    "none" 		{ exec usb_printerid $dev }
    }
}

proc devput {dev str reply re} {
    upvar $reply r
    set fp [open $dev "w+"]
    fconfigure $fp -buffering line -eofchar \x0c
    puts $fp "\033%-12345X@PJL\n@PJL $str\n\033%-12345Z"
    while {1} {
	gets $fp r
	if [regexp ".* $str.*" $r] {
	    break
	}
    }
    while {1} {
	gets $fp r
	# puts $r
	if [regexp "\"\?\"" $r] {
	    break
	}
	if [regexp "$re" $r] {
	    break
	}
    }
    close $fp
}

proc devreset {dev} {
    set fp [open $dev "w+"]
    fconfigure $fp -buffering line -eofchar \x0c
    puts $fp "\033%-12345X@PJL\n@PJL ECHO\n\033%-12345Z"
    close $fp
}

proc code2str {code} {
    switch -regexp $code {
	10001			{ return "Idle" }
	10002			{ return "Offline" }
	10003			{ return "Warming up" }
	10004			{ return "Busy (self-test)" }
	10005			{ return "Busy (reset)" }
	10006			{ return "Low toner" }
	10023			{ return "Printing" }
	30119			{ return "Media jam" }
	41[0-9][0-9][0-9]	{ return "Out of paper" }
	40021			{ return "Door open" }
	40022			{ return "Media jam" }
	40038			{ return "Low toner" }
	40600			{ return "No toner" }
    }
    return "Unknown"
}

proc do_hdr {f n} {
    frame $f.sf$n
    label $f.sf$n.label1 -text "Device Status" \
	-font "*adobe-helvetica-bold-r-normal--*-140-*" 
	# -relief solid
    pack $f.sf$n.label1 -side top -fill y -expand 1
    grid $f.sf$n -row 0 -column 0

    label $f.config$n -text "Replaced\nThe Paper?" \
	-font "*adobe-helvetica-bold-r-normal--*-140-*" 
	# -relief solid
    grid $f.config$n -row 0 -column 1
}

proc do_one {f n file product serial replace} {
    frame $f.sf$n

    if { $product == "" } {
	set prodsn $file
	set product "none"
    } else {
	set prodsn [concat $product "SN: " $serial]
    }
    label $f.sf$n.label1 -text "$prodsn"
    pack $f.sf$n.label1 -side top -fill y -expand 1

    devput $file "INFO STATUS" code .
    set str [code2str $code]
    label $f.sf$n.label2 -text "Status: $code ($str)"
    pack $f.sf$n.label2 -side top -fill y -expand 1

    devput $file "INFO PAGECOUNT" pagecount .
    label $f.sf$n.label3 -text "Page Count: $pagecount"
    pack $f.sf$n.label3 -side top -fill y -expand 1

    set re "xxx"
    switch -regexp $product {
    "HP LaserJet P1.*" { set re "PercentRemaining" }
    "HP LaserJet Pro.*" { set re "PercentLifeRemaining" }
    }

    devput $file "INFO SUPPLIES" perlife "$re"
    if { $perlife == "\"?\"" } {
	label $f.sf$n.label4 -text "Toner: PercentLifeRemaining = ???"
    } else {
	label $f.sf$n.label4 -text "Toner: $perlife%"
    }
    pack $f.sf$n.label4 -side top -fill y -expand 1

    grid $f.sf$n -row $n -column 0 -pady 5

    if { $replace == 1 } { set state "normal" } else { set state "disabled" }
    button $f.config$n -text "test" -image icon -state $state \
	    -command "replaced {$product} $file"
    grid $f.config$n -row $n -column 1
    $f.balloon bind $f.config$n -balloonmsg "Replaced Paper"

    devreset $file
}

proc main {w} {
    global share

    image create photo icon -file [file join $share foo2zjs hplj1020_icon.gif]

    frame $w.frame
    tixBalloon $w.frame.balloon

    set n 0
    set old 1
    set pwd [pwd]
    
    foreach file [lsort [glob -nocomplain /sys/class/usb/lp*/device]] {
	set old 0
	regsub /.*usb/(lp\[^/]*)/.* $file {\1} lp
	cd $file
	cd ..
	# puts [pwd]
	if { [file exists "product"] == 0 } {
	    continue
	}
	set fp [open "product" "r"]
	gets $fp product
	close $fp
	set fp [open "serial" "r"]
	gets $fp serial
	close $fp
	cd $pwd

	#puts $product
	switch -regexp $product {
	    "HP LaserJet 1018" { set replace 1 }
	    "HP LaserJet 1020" { set replace 1 }
	    "HP LaserJet P1005" { set replace 1 }
	    "HP LaserJet P1006" { set replace 1 }
	    "HP LaserJet P1007" { set replace 1 }
	    "HP LaserJet P1008" { set replace 1 }
	    "HP LaserJet Professional M12a" { set replace 0 }
	    "HP LaserJet Professional M12w" { set replace 0 }
	    "HP LaserJet Professional P1102" { set replace 0 }
	    "HP LaserJet Professional P1102w" { set replace 0 }
	    "CLP-310 Series" { set product [concat "Samsung" $product]
				set replace 0 }
	    default { continue }
	}
	set file /dev/usb/$lp

	if {$n == 0} {
	    do_hdr $w.frame 0
	    incr n
	}

	do_one $w.frame $n $file $product $serial $replace
	incr n
    }
    if {$old == 1} {
	foreach file [lsort [glob -nocomplain /dev/usb/lp?]] {
	    if {$n == 0} {
		do_hdr $w.frame 0
		incr n
	    }

	    do_one $w.frame $n $file "" "" 1
	    incr n
	}
    }
    if {$n == 0} {
	label $w.frame.label -text "No HP LaserJet 1018/1020/P100x"
	pack $w.frame.label
    }

    pack $w.frame -expand 1
}

wm title . "HP LaserJet 1018/1020/P100x GUI"

package require Tix

main ""
