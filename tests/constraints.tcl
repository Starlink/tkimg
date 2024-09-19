namespace eval imgtest {

    namespace export imageInit imageFinish imageCleanup imageNames
    variable ImageNames

    proc imageInit {} {
        variable ImageNames
        if {![info exists ImageNames]} {
            set ImageNames [lsort [image names]]
        }
        imageCleanup
        if {[lsort [image names]] ne $ImageNames} {
            return -code error "IMAGE NAMES mismatch: [image names] != $ImageNames"
        }
    }

    proc imageFinish {} {
        variable ImageNames
        if {[lsort [image names]] ne $ImageNames} {
            return -code error "images remaining: [image names] != $ImageNames"
        }
        imageCleanup
    }

    proc imageCleanup {} {
        variable ImageNames
        foreach img [image names] {
            if {$img ni $ImageNames} {image delete $img}
        }
    }

    proc imageNames {} {
        variable ImageNames
        set r {}
        foreach img [image names] {
            if {$img ni $ImageNames} {lappend r $img}
        }
        return $r
    }

    proc imageCompare { phImg1 phImg2 } {
        set w1 [image width  $phImg1]
        set h1 [image height $phImg1]
        set w2 [image width  $phImg2]
        set h2 [image height $phImg2]
        if { $w1 != $w2 && $h1 != $h2 } {
            return 0
        }
        for { set y 0 } { $y < $h1 } { incr y } {
            for { set x 0 } { $x < $w1 } { incr x } {
                set left  [$phImg1 get $x $y]
                set right [$phImg2 get $x $y]

                set dr [expr { [lindex $right 0] - [lindex $left 0] }]
                if { $dr != 0 } { return 0 }

                set dg [expr { [lindex $right 1] - [lindex $left 1] }]
                if { $dg != 0 } { return 0 }

                set db [expr { [lindex $right 2] - [lindex $left 2] }]
                if { $db != 0 } { return 0 }
            }
        }
        return 1
    }
}

namespace import -force imgtest::*
