afltestimgs.7z contains a series of AFL (American Fuzzy Lop) fuzzed
images in different file formats.
To execute the tests, first unpack afltestimgs.7z.
This generates 10 directories containing more than 20000 test images.

To see the usage message:
> tclsh RunAflTests.tcl --help

To execute the test suite using the Img extension and all test images:
> tclsh RunAflTests.tcl --img all

Using option --proc different ways of reading the images can be selected:
--proc 1: Uses "image create photo -file $fileName"
--proc 2: Uses "set ph [image create photo] ; $ph read $fileName" 

