                    Description of the tests
                    ========================

testFull.tcl:  Read and write full images
testFrom.tcl:  Read and write images with option "-from"
testTo.tcl:    Read and write images with option "-to"
testSmall.tcl: Read and write images with sizes from 1x1 to 4x4

Each test performs the following operations:

For each image format "fmt":
  1. Draw the test canvas, store it in a photo image
     and write it to a file in format "fmt".

  2. Read the image from file in different ways and display it.


The following ways to read image data are tested:
Read from file 1:         image create photo -file $fileName
Read from file 2:         set ph [image create photo] ; $ph read $fileName
Read as binary 1:         image create photo -data $imgData
Read as binary 2:         set ph [image create photo] ; $ph put $imgData
Read as uuencoded string: set ph [image create photo] ; $ph put $imgData

The following ways to write image data are tested:
Write to file:             $ph write $fileName -format $fmt
Write to uuencoded string: $ph data -format $fmt

ImgPut.tcl: Create an image using the standard Tk method and using the
            img::raw format handler.

ReadXbm.tcl: Display images from a XBM string using the "image create bitmap"
             and "image create photo -format XBM" commands. 
             Both image columns should be identical.

WriteGif.tcl: Create images using 255 and 256 different colors and try to
              write them as GIF files. The 256 color image generates an error.
