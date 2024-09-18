# Some constants
set modeFile 0x01
set modeBin  0x02
set modeUU   0x04
set modeFileStr "File IO"
set modeBinStr  "Binary IO"
set modeUUStr   "UUencoded IO"

# The list of file formats to be tested.
# First entry specifies the file extension used to create the image filenames.
# Second entry specifies the image format name as used by the Img extension.
# Third entry specifies optional format options.

set fmtList [list \
        [list ".bmp"   "bmp"  ""] \
        [list ".gif"   "gif"  ""] \
        [list ".ico"   "ico"  ""] \
        [list ".jpg"   "jpeg" ""] \
        [list ".pcx"   "pcx"  ""] \
        [list ".png"   "png"  ""] \
        [list ".ppm"   "ppm"  ""] \
        [list ".raw"   "raw"  "-useheader true -nomap true -nchan 3"] \
        [list ".rgb"   "sgi"  ""] \
        [list ".ras"   "sun"  ""] \
        [list ".tga"   "tga"  ""] \
        [list ".tif"   "tiff" ""] \
        [list ".xbm"   "xbm"  ""] \
        [list ".xpm"   "xpm"  ""] ]


# Load image data directly from a file into a photo image.
# Uses commands: image create photo -file "fileName"
proc readPhotoFile1 { name fmt } {
    PN "File read 1: "

    set sTime [clock clicks -milliseconds]
    set retVal [catch {image create photo -file $name} ph]
    if { $retVal != 0 } {
        P "\n\tWarning: Cannot detect image file format. Trying again with -format."
        P "\tError message: $ph"
        set retVal [catch {image create photo -file $name -format $fmt} ph]
        if { $retVal != 0 } {
            P "\tERROR: Cannot read image file with format option $fmt"
            P "\tError message: $ph"
            return ""
        }
    }
    set eTime [clock clicks -milliseconds]
    PN "[format "%.2f " [expr ($eTime - $sTime) / 1.0E3]]"
    return $ph
}

# Load image data directly from a file into a photo image.
# Uses commands: set ph [image create photo] ; $ph read "fileName"
# args maybe "-from ..." and/or "-to ..." option.
proc readPhotoFile2 { name fmt width height args } {
    PN "File read 2: "

    set sTime [clock clicks -milliseconds]
    if { $width < 0 && $height < 0 } {
        set ph [image create photo]
    } else {
        set ph [image create photo -width $width -height $height]
    }
    set retVal [catch {eval {$ph read $name} $args} errMsg]
    if { $retVal != 0 } {
        P "\n\tWarning: Cannot detect image file format. Trying again with -format."
        P "\tError message: $errMsg"
        set retVal [catch {eval {$ph read $name -format $fmt} $args} errMsg]
        if { $retVal != 0 } {
            P "\tERROR: Cannot read image file with format option $fmt"
            P "\tError message: $errMsg"
            return ""
        }
    }
    set eTime [clock clicks -milliseconds]
    PN "[format "%.2f " [expr ($eTime - $sTime) / 1.0E3]]"
    return $ph
}

# Load binary image data from a variable into a photo image.
# Uses commands: image create photo -data $imgData
proc readPhotoBinary1 { name fmt args } {
    PN "Binary read 1: "

    set sTime [clock clicks -milliseconds]
    set retVal [catch {open $name r} fp]
    if { $retVal != 0 } {
        P "\n\tERROR: Cannot open image file $name for binary reading."
        return ""
    }
    fconfigure $fp -translation binary
    set imgData [read $fp [file size $name]]
    close $fp

    set retVal [catch {image create photo -data $imgData} ph]
    if { $retVal != 0 } {
        P "\n\tWarning: Cannot detect image file format. Trying again with -format."
        P "\tError message: $ph"
        set retVal [catch {image create photo -data $imgData -format $fmt} ph]
        if { $retVal != 0 } {
            P "\tERROR: Cannot create photo from binary image data."
            P "\tError message: $ph"
            return ""
        }
    }
    set eTime [clock clicks -milliseconds]
    PN "[format "%.2f " [expr ($eTime - $sTime) / 1.0E3]]"
    return $ph
}

# Load binary image data from a variable into a photo image.
# Uses commands: set ph [image create photo] ; $ph put $imgData
# args maybe "-to ..." option.
proc readPhotoBinary2 { name fmt width height args } {
    PN "Binary read 2: "

    set sTime [clock clicks -milliseconds]
    set retVal [catch {open $name r} fp]
    if { $retVal != 0 } {
        P "\n\tERROR: Cannot open image file $name for binary reading."
        return ""
    }
    fconfigure $fp -translation binary
    set imgData [read $fp [file size $name]]
    close $fp

    if { $width < 0 && $height < 0 } {
        set ph [image create photo]
    } else {
        set ph [image create photo -width $width -height $height]
    }
    set retVal [catch {eval {$ph put $imgData} $args} errMsg]
    if { $retVal != 0 } {
        P "\n\tWarning: Cannot detect image file format. Trying again with -format."
        P "\tError message: $errMsg"
        set retVal [catch {eval {$ph put $imgData -format $fmt} $args} errMsg]
        if { $retVal != 0 } {
            P "\tERROR: Cannot create photo from binary image data."
            P "\tError message: $errMsg"
            return ""
        }
    }
    set eTime [clock clicks -milliseconds]
    PN "[format "%.2f " [expr ($eTime - $sTime) / 1.0E3]]"
    return $ph
}

# Load uuencoded image data from a variable into a photo image.
# Uses commands: set ph [image create photo] ; $ph put $imgData
proc readPhotoString { str fmt width height args } {
    PN "String read: "

    set sTime [clock clicks -milliseconds]
    if { $width < 0 && $height < 0 } {
        set ph [image create photo]
    } else {
        set ph [image create photo -width $width -height $height]
    }
    set retVal [catch {eval {$ph put $str} $args} errMsg]
    if { $retVal != 0 } {
        P "\n\tWarning: Cannot detect image string format. Trying again with -format."
        P "\tError message: $errMsg"
        set retVal [catch {eval {$ph put $str -format $fmt} $args} errMsg]
        if { $retVal != 0 } {
            P "\tERROR: Cannot read image string with format option: $fmt"
            P "\tError message: $errMsg"
            return ""
        }
    }
    set eTime [clock clicks -milliseconds]
    PN "[format "%.2f " [expr ($eTime - $sTime) / 1.0E3]]"
    return $ph
}

proc writePhotoFile { ph name fmt del args } {
    PN "File write: "

    set sTime [clock clicks -milliseconds]
    set retVal [catch {eval {$ph write $name -format $fmt} $args} str]
    set eTime [clock clicks -milliseconds]

    if { $retVal != 0 } {
        P "\n\tERROR: Cannot write image file $name (Format: $fmt)"
        P "\tError message: $str"
        return ""
    }
    if { $del } {
        image delete $ph
    }
    PN "[format "%.2f " [expr ($eTime - $sTime) / 1.0E3]]"
    return $str
}

proc writePhotoString { ph fmt del args } {
    PN "String write: "

    set sTime [clock clicks -milliseconds]
    set retVal [catch {eval {$ph data -format $fmt} $args} str]
    set eTime [clock clicks -milliseconds]
    if { $retVal != 0 } {
        P "\n\tERROR: Cannot write image to string (Format: $fmt)"
        P "\tError message: $str"
        return ""
    }
    if { $del } {
        image delete $ph
    }
    PN "[format "%.2f " [expr ($eTime - $sTime) / 1.0E3]]"
    return $str
}

proc createErrImg {} {
    set retVal [catch {image create photo -data [unsupportedImg]} errImg]
    if { $retVal != 0 } {
        P "FATAL ERROR: Cannot load uuencode GIF image into canvas."
        P "             Test will be cancelled."
        exit 1
    }
    return $errImg
}

proc getCanvasPhoto { canvId } {
    PN "Canvas photo: "
    set sTime [clock clicks -milliseconds]
    set retVal [catch {image create photo -format window -data $canvId} ph]
    set eTime [clock clicks -milliseconds]
    if { $retVal != 0 } {
        P "\n\tFATAL ERROR: Cannot create photo from canvas window."
        P "\tError message: $ph"
        exit 1
    }
    P "[format "%.2f secs" [expr ($eTime - $sTime) / 1.0E3]]"
    return $ph
}

proc delayedUpdate {} {
    update
    after 200
}

proc drawInfo { canvId x y color xsize } {
    set ysize 10
    $canvId create rectangle $x $y [expr $x + $xsize] [expr $y + $ysize] -fill $color
    delayedUpdate
}

proc drawTestCanvas { imgVersion} {
    set tw .0Win
    toplevel $tw
    wm title $tw "Canvas window"
    wm geometry $tw "+0+30"

    set canvId $tw.c
    # Canvas size must not exceed 256 pixels, as the ICO format
    # does not support larger images.
    set width  250
    set height 230
    canvas $canvId -bg gray -width $width -height $height -borderwidth 0 -highlightthickness 0
    pack $canvId

    P "Drawing color rectangles into canvas .."
    $canvId create rectangle 1 1 [expr $width - 1] [expr $height - 1] -outline black
    $canvId create rectangle 3 3 [expr $width - 3] [expr $height - 3] -outline green -width 2
    delayedUpdate

    drawInfo $canvId 10  10 black   [expr $width - 20]
    drawInfo $canvId 10  30 white   [expr $width - 20]
    drawInfo $canvId 10  50 red     [expr $width - 20]
    drawInfo $canvId 10  70 green   [expr $width - 20]
    drawInfo $canvId 10  90 blue    [expr $width - 20]
    drawInfo $canvId 10 110 cyan    [expr $width - 20]
    drawInfo $canvId 10 130 magenta [expr $width - 20]
    drawInfo $canvId 10 150 yellow  [expr $width - 20]

    update
    return $canvId
}
