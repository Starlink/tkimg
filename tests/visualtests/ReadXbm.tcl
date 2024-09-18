package require Tk
package require Img
puts "Using Tcl [info patchlevel], Tk [package version Tk], Img [package version Img]"

proc _appendToFile {} {
    return {
        #define appendToFile_width 32
        #define appendToFile_height 32
        static char appendToFile_bits[] = {
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00,
          0xc0, 0xff, 0x3f, 0x00,
          0xc0, 0xff, 0x3f, 0x00,
          0xc0, 0x00, 0xf0, 0x00,
          0xc0, 0x00, 0xf0, 0x00,
          0xc0, 0x00, 0x30, 0x03,
          0xc0, 0x00, 0x30, 0x03,
          0xc0, 0x00, 0xf0, 0x0f,
          0xc0, 0x00, 0xf0, 0x0f,
          0xc0, 0x30, 0x00, 0x0c,
          0xc0, 0x30, 0x00, 0x0c,
          0xc0, 0xf0, 0x00, 0x0c,
          0xc0, 0xf0, 0x00, 0x0c,
          0xfc, 0xff, 0x03, 0x0c,
          0xfc, 0xff, 0x03, 0x0c,
          0xfc, 0xff, 0x03, 0x0c,
          0xfc, 0xff, 0x03, 0x0c,
          0xc0, 0xf0, 0x00, 0x0c,
          0xc0, 0xf0, 0x00, 0x0c,
          0xc0, 0x30, 0x00, 0x0c,
          0xc0, 0x30, 0x00, 0x0c,
          0xc0, 0x00, 0x00, 0x0c,
          0xc0, 0x00, 0x00, 0x0c,
          0xc0, 0x00, 0x00, 0x0c,
          0xc0, 0x00, 0x00, 0x0c,
          0xc0, 0xff, 0xff, 0x0f,
          0xc0, 0xff, 0xff, 0x0f,
          0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00};
    }
}

proc CreateErrorImage { width height } {
    set phImg [image create photo -width $width -height $height]
    set scanline [lrepeat $width "#A00000"]
    lset scanline [expr { $width/2 + 0 }] "#FFFF00"
    lset scanline [expr { $width/2 + 1 }] "#FFFF00"
    set data [list]
    lappend data $scanline
    for { set y 0 } { $y < $height } { incr y } {
        $phImg put $data -to 0 $y
    }
    return $phImg
}

proc appendToFile { type args } {
    puts "appendToFile $type $args"
    if { $type eq "bitmap" } {
        set catchVal [catch {image create bitmap -data [_appendToFile] {*}$args} img]
    } else {
        set catchVal [catch {image create photo -data [_appendToFile] -format [list "xbm" {*}$args]} img]
    }
    if { $catchVal == 0 } {
        return $img
    } else {
        puts "    Error: $img"
        return [CreateErrorImage 32 32]
    }
}

set labelBg "lightgreen"
set typeList [list  "bitmap" "photo"]

pack [frame .top]
pack [text .top.t -height 3]
.top.t insert end \
"Display images from a XBM string using the \"image create bitmap\"
and \"image create photo -format XBM\" commands.
Both image columns should be identical."

pack [frame .cont]
foreach type $typeList {
    frame .cont.$type
}
pack {*}[winfo children .cont] -side left

foreach type $typeList {
    puts "Column $type:"
    label .cont.${type}.l -text $type -anchor nw

    label .cont.${type}.l1 -background $labelBg
    .cont.${type}.l1 configure -image [appendToFile $type]

    label .cont.${type}.l2 -background $labelBg
    .cont.${type}.l2 configure -image [appendToFile $type -foreground "red"]

    label .cont.${type}.l3 -background $labelBg
    .cont.${type}.l3 configure -image [appendToFile $type -background "yellow"]

    label .cont.${type}.l4 -background $labelBg
    .cont.${type}.l4 configure -image [appendToFile $type -foreground "#FF00FF" -background "#FFFFFF"]

    label .cont.${type}.l5 -background $labelBg
    if { $type eq "bitmap" } {
        puts "   Ignoring appendToFile bitmap -background \"NoColor\""
        .cont.${type}.l5 configure -image [CreateErrorImage 32 32]
    } else {
        .cont.${type}.l5 configure -image [appendToFile $type -background "NoColor"]
    }

    label .cont.${type}.l6 -background $labelBg
    .cont.${type}.l6 configure -image [appendToFile $type -background]

    label .cont.${type}.l7 -background $labelBg
    .cont.${type}.l7 configure -image [appendToFile $type -invalidoption "#FF00FF"]

    pack {*}[winfo children .cont.${type}] -pady 5
    puts ""
}

bind . <Escape> exit
