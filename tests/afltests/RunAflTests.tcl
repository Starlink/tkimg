package require Tk

set useImg      false
set useVerbose  false
set useReadProc 1

set dirNames [lsort -dictionary [glob -nocomplain -types d "\[a-z\]*"]]
set logDir "_Logs"

proc PrintUsageAndExit { progName } {
    puts "$progName ?Options? DirName1 ?DirNameN?"
    puts ""
    puts "Run test suite using image files located in specified directories."
    puts "If \"DirName\" is \"all\", then all tests are run."
    puts ""
    puts "The following directories are available:"
    puts "$::dirNames"
    puts ""
    puts "By default, only the image parsers of Tk are used."
    puts "The results of the tests are written into directory \"$::logDir\"."
    puts ""
    puts "Options:"
    puts "--help   : Print this help message and exit."
    puts "--verbose: Print each file being checked onto stdout."
    puts "--img    : Load Img extension on startup."
    puts "--proc   : Specify read procedure. Default: 1"
    puts "           1: Uses \"image create photo -file \$fileName\""
    puts "           2: Uses \"set ph \[image create photo\] \; \$ph read \$fileName\""
    exit 1
}

proc GetReadProc { readProcNum } {
    switch -exact $readProcNum {
        1 { set readProc "ReadPhotoFile1" }
        2 { set readProc "ReadPhotoFile2" }
        default { puts "Unknown read proc $readProcNum" ; exit 1 }
    }
    return $readProc
}

# Load image data directly from a file into a photo image.
# Uses command: image create photo -file "fileName"
proc ReadPhotoFile1 { fileName } {
    set phImg ""
    set err   ""
    set sTime [clock clicks -milliseconds]

    set retVal [catch { set phImg [image create photo -file $fileName] } err]

    set eTime [clock clicks -milliseconds]
    return [list $retVal $phImg $err [expr {($eTime - $sTime) / 1.0E3}]]
}

# Load image data directly from a file into a photo image.
# Uses commands: set ph [image create photo] ; $ph read "fileName"
proc ReadPhotoFile2 { fileName } {
    set phImg ""
    set err   ""
    set sTime [clock clicks -milliseconds]

    set phImg [image create photo]
    set retVal [catch { $phImg read $fileName } err]
    if { $retVal != 0 } {
        image delete $phImg
        set phImg ""
    }

    set eTime [clock clicks -milliseconds]
    return [list $retVal $phImg $err [expr {($eTime - $sTime) / 1.0E3}]]
}

set curArg 0
while { $curArg < $argc } {
    set curParam [lindex $argv $curArg]
    if { [string first "--" $curParam] == 0 } {
        set curParam [string range $curParam 1 end]
    }
    if { $curParam eq "-img" } {
        set useImg true
    } elseif { $curParam eq "-verbose" } {
        set useVerbose true
    } elseif { $curParam eq "-help" } {
        PrintUsageAndExit $argv0
    } elseif { $curParam eq "-proc" } {
        incr curArg
        set useReadProc [lindex $argv $curArg]
    } else {
        set testDir [lindex $argv $curArg]
        if { [lsearch $dirNames $testDir] >= 0 || $testDir eq "all" } {
            lappend testDirs $testDir
        } else {
            puts "Ignoring test directory $testDir"
        }
    }
    incr curArg
}

if { ! [info exists testDirs] } {
    PrintUsageAndExit $argv0
}
if { [lsearch $testDirs "all"] >= 0 } {
    set testDirs $dirNames
}

set haveTk87  false
set haveImg15 false

if { [string first "8.7" [package version Tk]] >= 0 } {
    set haveTk87 true
}

puts -nonewline "Using [expr 8 * $tcl_platform(pointerSize)]-bit Tcl [info patchlevel], Tk [package version Tk]"
if { $useImg } {
    package require Img
    puts ", Img [package version Img] "
    if { [string first "1.5" [package version Img]] >= 0 } {
        set haveImg15 true
    }
} else {
    puts ""
}
puts "Using read proc \#$useReadProc : [GetReadProc $useReadProc]"
puts ""

if { ! [file isdirectory $logDir] } {
    file mkdir $logDir
}

set countTotal 0
foreach testDir $testDirs {
    foreach testCase [list "edges-only" "full"] {
        if { $useImg } {
            set logFileName [format "%s/%s_%s-%s.csv" $logDir $testDir $testCase "Img"]
        } else {
            set logFileName [format "%s/%s_%s-%s.csv" $logDir $testDir $testCase "Tk"]
        }
        set fp [open "$logFileName" "w"]
        puts "Starting test case ${testDir}/${testCase} ..."
        set countAll  0
        set countFail 0
        set countOk   0

        set pathName [file join $testDir $testCase "images"]
        if { ! [file isdirectory $pathName] } {
            puts stderr "Error: Directory $pathName does not exist."
            continue
        }
        set fileNames [lsort -dictionary [glob [file join $pathName "*"]]]
        foreach fileName $fileNames {
            if { $useVerbose } {
                puts "Checking file $fileName"
            }
            set readProc [GetReadProc $useReadProc]
            lassign [$readProc $fileName] retVal phImg err secs
            if { $retVal == 0 } {
                puts $fp "$fileName;[image width $phImg];[image height $phImg];"
                image delete $phImg
                incr countOk
            } else {
                puts $fp "$fileName;-1;-1;$err"
                incr countFail
            }
	    incr countAll
        }
        close $fp
        puts "Log written to file $logFileName"
        puts "$countAll files checked: $countOk files correct. $countFail files incorrect."
        puts ""
        incr countTotal $countAll
    }
}

puts ""
puts "Total number of checked files: $countTotal"

exit 0
