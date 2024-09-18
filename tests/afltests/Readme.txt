afltestimgs.7z contains a series of AFL (American Fuzzy Lop) fuzzed
images in different file formats.
To execute the tests, first unpack afltestimgs.7z.
This generates 10 directories containing more than 20000 test images.

To execute the test suite using the Img extension and all test images:
> tclsh RunAflTests.tcl --img all

To see the usage message:
> tclsh RunAflTests.tcl --help
