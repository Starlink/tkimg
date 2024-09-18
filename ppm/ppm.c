/* STARTHEADER
 *
 * File :       ppm.c
 *
 * Author :     Paul Obermeier (paul@poSoft.de)
 *
 * Date :       2001 / 01 / 22
 *
 * Copyright :  (C) 2001-2019 Paul Obermeier
 *
 * Description :
 *
 * A photo image handler for the PPM/PGM image file formats.
 *
 * The following image types are supported:
 *
 * Grayscale  (PGM): 8-bit and 16-bit, 1 channel per pixel.
 * True-color (PPM): 8-bit and 16-bit, 3 channels per pixel.
 *
 * Both types can be stored as pure ASCII or as binary files.
 *
 * List of currently supported features:
 *
 * Type              |     Read      |     Write     |
 *                   | -file | -data | -file | -data |
 * -----------------------------------------------
 * PGM  8-bit ASCII  | Yes   | Yes   | No    | No    |
 * PGM  8-bit BINARY | Yes   | Yes   | No    | No    |
 * PGM 16-bit ASCII  | Yes   | Yes   | No    | No    |
 * PGM 16-bit BINARY | Yes   | Yes   | No    | No    |
 * PPM  8-bit ASCII  | Yes   | Yes   | Yes   | Yes   |
 * PPM  8-bit BINARY | Yes   | Yes   | Yes   | Yes   |
 * PPM 16-bit ASCII  | Yes   | Yes   | No    | No    |
 * PPM 16-bit BINARY | Yes   | Yes   | No    | No    |
 *
 * The following format options are available:
 *
 * Read  image: "ppm -verbose <bool> -gamma <float>
 *                   -min <float> -max <float> -scanorder <string>"
 * Write image: "ppm -ascii <bool>"
 *
 * -verbose <bool>:     If set to true, additional information about the file
 *                      format is printed to stdout. Default is false.
 * -gamma <float>:      Specify a gamma correction to be applied when mapping
 *                      the input data to 8-bit image values.
 *                      Default is 1.0.
 * -min <float>:        Specify the minimum pixel value to be used for mapping
 *                      the input data to 8-bit image values.
 *                      Default is the minimum value found in the image data.
 * -max <float>:        Specify the maximum pixel value to be used for mapping
 *                      the input data to 8-bit image values.
 *                      Default is the maximum value found in the image data.
 * -scanorder <string>: Specify the scanline order of the input image. Convention
 *                      is storing scan lines from top to bottom.
 *                      Possible values: "TopDown" or "BottomUp".
 * -ascii <bool>:       If set to true, file is written in PPM ASCII format (P3).
 *                      Default is false, i.e. write in binary format (P6).
 *
 * Notes:
 *
 * - Part of this code was taken from Tk's tkImgPPM.c:
 *
 *  >> tkImgPPM.c --
 *  >>
 *  >>  A photo image file handler for PPM (Portable PixMap) files.
 *  >>
 *  >> Copyright (c) 1994 The Australian National University.
 *  >> Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *  >>
 *  >> See the file "license.terms" for information on usage and redistribution
 *  >> of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *  >>
 *  >> Author: Paul Mackerras (paulus@cs.anu.edu.au),
 *  >>     Department of Computer Science,
 *  >>     Australian National University.
 *
 * ENDHEADER
 *
 */

#include <stdlib.h>
#include <float.h>
#include <math.h>

/*
 * Generic initialization code, parameterized via CPACKAGE and PACKAGE.
 */

#include "init.c"


/*
#define DEBUG_LOCAL
*/

/*
 * Define PGM and PPM, i.e. gray images and color images.
 */

#define PGM 1
#define PPM 2

/* Some general defines and typedefs. */
#define TRUE  1
#define FALSE 0
#define BOTTOM_UP   0
#define TOP_DOWN    1

#define strIntel    "Intel"
#define strMotorola "Motorola"
#define strTopDown  "TopDown"
#define strBottomUp "BottomUp"

typedef unsigned char Boln;     /* Boolean value: TRUE or FALSE */
typedef unsigned char UByte;    /* Unsigned  8 bit integer */
typedef char  Byte;             /* Signed    8 bit integer */
typedef unsigned short UShort;  /* Unsigned 16 bit integer */
typedef short Short;            /* Signed   16 bit integer */
typedef int UInt;               /* Unsigned 32 bit integer */
typedef int Int;                /* Signed   32 bit integer */
typedef float Float;            /* IEEE     32 bit floating point */
typedef double Double;          /* IEEE     64 bit floating point */

typedef struct {
    Float minVal;
    Float maxVal;
    Float gamma;
    Boln  verbose;
    Boln  writeAscii;
    Int   scanOrder;
} FMTOPT;

/* PPM file header structure */
typedef struct {
    Int   width;
    Int   height;
    Int   maxVal;
    Boln  isAscii;
} PPMHEADER;

/* Structure to hold information about the PPM file being processed. */
typedef struct {
    PPMHEADER th;
    UByte  *pixbuf;
    UShort *ushortBuf;
    UByte  *ubyteBuf;
} PPMFILE;

#define OUT Tcl_WriteChars (outChan, str, -1)
static void printImgInfo (int width, int height, int maxVal, int isAscii, int nChans,
                          FMTOPT *opts, const char *filename, const char *msg)
{
    Tcl_Channel outChan;
    char str[256];

    outChan = Tcl_GetStdChannel (TCL_STDOUT);
    if (!outChan) {
        return;
    }
    tkimg_snprintf (str, 256, "%s %s\n", msg, filename);                                        OUT;
    tkimg_snprintf (str, 256, "\tSize in pixel    : %d x %d\n", width, height);                 OUT;
    tkimg_snprintf (str, 256, "\tMaximum value    : %d\n", maxVal);                             OUT;
    tkimg_snprintf (str, 256, "\tNo. of channels  : %d\n", nChans);                             OUT;
    tkimg_snprintf (str, 256, "\tGamma correction : %f\n", opts->gamma);                        OUT;
    tkimg_snprintf (str, 256, "\tMinimum map value: %f\n", opts->minVal);                       OUT;
    tkimg_snprintf (str, 256, "\tMaximum map value: %f\n", opts->maxVal);                       OUT;
    tkimg_snprintf (str, 256, "\tVertical encoding: %s\n", opts->scanOrder == TOP_DOWN?
                                                           strTopDown: strBottomUp);            OUT;
    tkimg_snprintf (str, 256, "\tAscii format     : %s\n", isAscii?  "Yes": "No");              OUT;
    tkimg_snprintf (str, 256, "\tHost byte order  : %s\n", tkimg_IsIntel ()?  strIntel: strMotorola); OUT;
    Tcl_Flush (outChan);
}
#undef OUT

static void ppmClose (PPMFILE *tf)
{
    if (tf->pixbuf)    ckfree ((char *)tf->pixbuf);
    if (tf->ushortBuf) ckfree ((char *)tf->ushortBuf);
    if (tf->ubyteBuf)  ckfree ((char *)tf->ubyteBuf);
    return;
}

#define UCHAR(c) ((unsigned char) (c))

static int getNextVal (Tcl_Interp *interp, tkimg_MFile *handle, UInt *val)
{
    char c, buf[TCL_INTEGER_SPACE];
    UInt i;

    /* First skip leading whitespaces. */
    while (tkimg_Read2(handle, &c, 1) == 1) {
        if (!isspace(UCHAR(c))) {
            break;
        }
    }

    buf[0] = c;
    i = 1;
    while (tkimg_Read2(handle, &c, 1) == 1 && i < TCL_INTEGER_SPACE) {
        if (isspace(UCHAR(c))) {
            buf[i] = '\0';
            sscanf (buf, "%u", val);
            return TRUE;
        }
        buf[i++] = c;
    }
    Tcl_AppendResult (interp, "cannot read next ASCII value", (char *) NULL);
    return FALSE;
}

static Boln readUShortRow (Tcl_Interp *interp, tkimg_MFile *handle, UShort *pixels,
                           Int nShorts, char *buf, Boln swapBytes, Boln isAscii)
{
    UShort *mPtr = pixels;
    char   *bufPtr = buf;
    UInt   i, val;

    #ifdef DEBUG_LOCAL
        printf ("Reading %d UShorts\n", nShorts);
    #endif
    if (isAscii) {
        for (i=0; i<nShorts; i++) {
            if (!getNextVal (interp, handle, &val)) {
                return FALSE;
            }
            pixels[i] = (UShort) val;
        }
        return TRUE;
    }

    if (2 * nShorts != tkimg_Read2(handle, buf, 2 * nShorts)) {
        return FALSE;
    }

    if (swapBytes) {
        for (i=0; i<nShorts; i++) {
            ((char *)mPtr)[0] = bufPtr[1];
            ((char *)mPtr)[1] = bufPtr[0];
            mPtr++;
            bufPtr += 2;
        }
    } else {
        for (i=0; i<nShorts; i++) {
            ((char *)mPtr)[0] = bufPtr[0];
            ((char *)mPtr)[1] = bufPtr[1];
            mPtr++;
            bufPtr += 2;
        }
    }
    return TRUE;
}

static Boln readUByteRow (Tcl_Interp *interp, tkimg_MFile *handle, UByte *pixels,
                          Int nBytes, char *buf, Boln swapBytes, Boln isAscii)
{
    UByte *mPtr = pixels;
    char  *bufPtr = buf;
    UInt  i, val;

    #ifdef DEBUG_LOCAL
        printf ("Reading %d UBytes\n", nBytes);
    #endif
    if (isAscii) {
        for (i=0; i<nBytes; i++) {
            if (!getNextVal (interp, handle, &val)) {
                return FALSE;
            }
            pixels[i] = (UByte) val;
        }
        return TRUE;
    }

    if (nBytes != tkimg_Read2(handle, buf, nBytes)) {
        return FALSE;
    }

    for (i=0; i<nBytes; i++) {
        ((char *)mPtr)[0] = bufPtr[0];
        mPtr++;
        bufPtr += 1;
    }
    return TRUE;
}

static Boln readUShortFile (Tcl_Interp *interp, tkimg_MFile *handle, UShort *buf, Int width, Int height,
                            Int nchan, Boln swapBytes, Boln isAscii, Boln verbose,
                            Double minVals[], Double maxVals[])
{
    Int    x, y, c;
    UShort *bufPtr = buf;
    char   *line;

    #ifdef DEBUG_LOCAL
        printf ("readUShortFile: Width=%d Height=%d nchan=%d swapBytes=%s\n",
                 width, height, nchan, swapBytes? "yes": "no");
    #endif
    for (c=0; c<nchan; c++) {
        minVals[c] =  DBL_MAX;
        maxVals[c] = -DBL_MAX;
    }
    line = attemptckalloc (sizeof (UShort) * nchan * width);
    if (line == NULL) {
        Tcl_AppendResult (interp, "Unable to allocate memory for image data.", (char *) NULL);
        return FALSE;
    }

    for (y=0; y<height; y++) {
        if (!readUShortRow (interp, handle, bufPtr, nchan * width, line, swapBytes, isAscii)) {
            return FALSE;
        }
        for (x=0; x<width; x++) {
            for (c=0; c<nchan; c++) {
                if (*bufPtr > maxVals[c]) maxVals[c] = *bufPtr;
                if (*bufPtr < minVals[c]) minVals[c] = *bufPtr;
                bufPtr++;
            }
        }
    }
    if (verbose) {
        printf ("\tMinimum pixel values :");
        for (c=0; c<nchan; c++) {
            printf (" %u", (UShort)minVals[c]);
        }
        printf ("\n");
        printf ("\tMaximum pixel values :");
        for (c=0; c<nchan; c++) {
            printf (" %u", (UShort)maxVals[c]);
        }
        printf ("\n");
        fflush (stdout);
    }
    ckfree (line);
    return TRUE;
}

static Boln readUByteFile (Tcl_Interp *interp, tkimg_MFile *handle, UByte *buf, Int width, Int height,
                           Int nchan, Boln swapBytes, Boln isAscii, Boln verbose,
                           Double minVals[], Double maxVals[])
{
    Int   x, y, c;
    UByte *bufPtr = buf;
    char  *line;

    #ifdef DEBUG_LOCAL
        printf ("readUByteFile: Width=%d Height=%d nchan=%d swapBytes=%s\n",
                 width, height, nchan, swapBytes? "yes": "no");
    #endif
    for (c=0; c<nchan; c++) {
        minVals[c] =  DBL_MAX;
        maxVals[c] = -DBL_MAX;
    }
    line = attemptckalloc (sizeof (UByte) * nchan * width);
    if (line == NULL) {
        Tcl_AppendResult (interp, "Unable to allocate memory for image data.", (char *) NULL);
        return FALSE;
    }

    for (y=0; y<height; y++) {
        if (!readUByteRow (interp, handle, bufPtr, nchan * width, line, swapBytes, isAscii)) {
            return FALSE;
        }
        for (x=0; x<width; x++) {
            for (c=0; c<nchan; c++) {
                if (*bufPtr > maxVals[c]) maxVals[c] = *bufPtr;
                if (*bufPtr < minVals[c]) minVals[c] = *bufPtr;
                bufPtr++;
            }
        }
    }
    if (verbose) {
        printf ("\tMinimum pixel values :");
        for (c=0; c<nchan; c++) {
            printf (" %u", (UByte)minVals[c]);
        }
        printf ("\n");
        printf ("\tMaximum pixel values :");
        for (c=0; c<nchan; c++) {
            printf (" %u", (UByte)maxVals[c]);
        }
        printf ("\n");
        fflush (stdout);
    }
    ckfree (line);
    return TRUE;
}

static int ParseFormatOpts(
    Tcl_Interp *interp,
    Tcl_Obj *format,
    FMTOPT *opts
) {
    static const char *const ppmOptions[] = {
         "-verbose", "-min", "-max", "-gamma", "-scanorder", "-ascii", NULL
    };
    int objc, i, index;
    char *optionStr;
    Tcl_Obj **objv;
    int boolVal;
    double doubleVal;

    /* Initialize options with default values. */
    opts->verbose    = 0;
    opts->minVal     = 0.0;
    opts->maxVal     = 0.0;
    opts->gamma      = 1.0;
    opts->scanOrder  = TOP_DOWN;
    opts->writeAscii = 0;

    if (tkimg_ListObjGetElements (interp, format, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc) {
        for (i=1; i<objc; i++) {
            if (Tcl_GetIndexFromObj(interp, objv[i], (const char * const *)ppmOptions,
                    "format option", 0, &index) != TCL_OK) {
                return TCL_ERROR;
            }
            if (++i >= objc) {
                Tcl_AppendResult (interp, "No value for option \"",
                        Tcl_GetStringFromObj (objv[--i], (int *) NULL),
                        "\"", (char *) NULL);
                return TCL_ERROR;
            }
            optionStr = Tcl_GetStringFromObj(objv[i], (int *) NULL);
            switch(index) {
                case 0:
                    if (Tcl_GetBoolean(interp, optionStr, &boolVal) == TCL_ERROR) {
                        Tcl_AppendResult (interp, "Invalid verbose mode \"", optionStr,
                                          "\": should be 1 or 0, on or off, true or false",
                                          (char *) NULL);
                        return TCL_ERROR;
                    }
                    opts->verbose = boolVal;
                    break;
                case 1:
                    if (Tcl_GetDouble(interp, optionStr, &doubleVal) == TCL_ERROR) {
                        Tcl_AppendResult (interp, "Invalid minimum map value \"", optionStr,
                                          "\": Must be greater than or equal to zero.", (char *) NULL);
                        return TCL_ERROR;
                    }
                    if (doubleVal >= 0.0) {
                        opts->minVal = doubleVal;
                    }
                    break;
                case 2:
                    if (Tcl_GetDouble(interp, optionStr, &doubleVal) == TCL_ERROR) {
                        Tcl_AppendResult (interp, "Invalid maximum map value \"", optionStr,
                                          "\": Must be greater than or equal to zero.", (char *) NULL);
                        return TCL_ERROR;
                    }
                    if (doubleVal >= 0.0) {
                        opts->maxVal = doubleVal;
                    }
                    break;
                case 3:
                    if (Tcl_GetDouble(interp, optionStr, &doubleVal) == TCL_ERROR) {
                        Tcl_AppendResult (interp, "Invalid gamma value \"", optionStr,
                                          "\": Must be greater than or equal to zero.", (char *) NULL);
                        return TCL_ERROR;
                    }
                    if (doubleVal >= 0.0) {
                        opts->gamma = doubleVal;
                    }
                    break;
                case 4:
                    if (!strncmp (optionStr, strTopDown, strlen (strTopDown))) {
                        opts->scanOrder = TOP_DOWN;
                    } else if (!strncmp (optionStr, strBottomUp, strlen (strBottomUp))) {
                        opts->scanOrder = BOTTOM_UP;
                    } else {
                        Tcl_AppendResult (interp, "invalid scanline order \"", optionStr,
                                          "\": should be TopDown or BottomUp",
                                          (char *) NULL);
                        return TCL_ERROR;
                    }
                    break;
                case 5:
                    if (Tcl_GetBoolean(interp, optionStr, &boolVal) == TCL_ERROR) {
                        Tcl_AppendResult (interp, "Invalid ascii mode \"", optionStr,
                                          "\": should be 1 or 0, on or off, true or false",
                                          (char *) NULL);
                        return TCL_ERROR;
                    }
                    opts->writeAscii = boolVal;
                    break;
            }
        }
    }
    return TCL_OK;
}

/*
 * Prototypes for local procedures defined in this file:
 */

static int CommonMatch (tkimg_MFile *handle, int *widthPtr,
                int *heightPtr, int *maxIntensityPtr);
static int CommonRead (Tcl_Interp *interp, tkimg_MFile *handle,
                const char *filename, Tcl_Obj *format,
                Tk_PhotoHandle imageHandle, int destX, int destY,
                int width, int height, int srcX, int srcY);
static int CommonWrite (Tcl_Interp *interp,
                const char *filename, Tcl_Obj *format,
                tkimg_MFile *handle, Tk_PhotoImageBlock *blockPtr);
static int ReadPPMFileHeader (tkimg_MFile *handle, int *widthPtr,
                int *heightPtr, int *maxIntensityPtr, Boln *isAsciiPtr);


/*
 *----------------------------------------------------------------------
 *
 * ChnMatch --
 *
 *      This procedure is invoked by the photo image type to see if
 *      a file contains image data in PPM format.
 *
 * Results:
 *      The return value is >0 if the first characters in file "f" look
 *      like PPM data, and 0 otherwise.
 *
 * Side effects:
 *      The access position in f may change.
 *
 *----------------------------------------------------------------------
 */

static int ChnMatch(
    Tcl_Channel chan,           /* The image file, open for reading. */
    const char *filename,       /* The name of the image file. */
    Tcl_Obj *format,            /* User-specified format object, or NULL. */
    int *widthPtr,              /* The dimensions of the image are */
    int *heightPtr,             /* returned here if the file is a valid
                                 * PPM file. */
    Tcl_Interp *interp          /* Interpreter to use for reporting errors. */
) {
    tkimg_MFile handle;
    int   dummy;

    handle.data = (char *) chan;
    handle.state = IMG_CHAN;

    return CommonMatch(&handle, widthPtr, heightPtr, &dummy);
}

static int ObjMatch(
    Tcl_Obj *data,
    Tcl_Obj *format,
    int *widthPtr,
    int *heightPtr,
    Tcl_Interp *interp
) {
    tkimg_MFile handle;
    int   dummy;

    tkimg_ReadInit(data, 'P', &handle);
    return CommonMatch(&handle, widthPtr, heightPtr, &dummy);
}

static int CommonMatch(
    tkimg_MFile *handle,
    int *widthPtr,
    int *heightPtr,
    int *maxIntensityPtr
) {
    Boln dummy;
    return ReadPPMFileHeader(handle, widthPtr, heightPtr, maxIntensityPtr, &dummy);
}


/*
 *----------------------------------------------------------------------
 *
 * ChnRead --
 *
 *      This procedure is called by the photo image type to read
 *      PPM format data from a file and write it into a given
 *      photo image.
 *
 * Results:
 *      A standard TCL completion code.  If TCL_ERROR is returned
 *      then an error message is left in the interp's result.
 *
 * Side effects:
 *      The access position in file f is changed, and new data is
 *      added to the image given by imageHandle.
 *
 *----------------------------------------------------------------------
 */

static int ChnRead(
    Tcl_Interp *interp,         /* Interpreter to use for reporting errors. */
    Tcl_Channel chan,           /* The image file, open for reading. */
    const char *filename,       /* The name of the image file. */
    Tcl_Obj *format,            /* User-specified format string, or NULL. */
    Tk_PhotoHandle imageHandle, /* The photo image to write into. */
    int destX, int destY,       /* Coordinates of top-left pixel in
                                 * photo image to be written to. */
    int width, int height,      /* Dimensions of block of photo image to
                                 * be written to. */
    int srcX, int srcY          /* Coordinates of top-left pixel to be used
                                 * in image being read. */
) {
    tkimg_MFile handle;

    handle.data = (char *) chan;
    handle.state = IMG_CHAN;

    return CommonRead (interp, &handle, filename, format, imageHandle,
                       destX, destY, width, height, srcX, srcY);
}

static int ObjRead(
    Tcl_Interp *interp,
    Tcl_Obj *data,
    Tcl_Obj *format,
    Tk_PhotoHandle imageHandle,
    int destX, int destY,
    int width, int height,
    int srcX, int srcY
) {
    tkimg_MFile handle;

    tkimg_ReadInit (data, 'P', &handle);
    return CommonRead (interp, &handle, "InlineData", format, imageHandle,
                       destX, destY, width, height, srcX, srcY);
}

static int CommonRead(
    Tcl_Interp *interp,         /* Interpreter to use for reporting errors. */
    tkimg_MFile *handle,                /* The image file, open for reading. */
    const char *filename,       /* The name of the image file. */
    Tcl_Obj *format,            /* User-specified format string, or NULL. */
    Tk_PhotoHandle imageHandle, /* The photo image to write into. */
    int destX, int destY,       /* Coordinates of top-left pixel in
                                 * photo image to be written to. */
    int width, int height,      /* Dimensions of block of photo image to
                                 * be written to. */
    int srcX, int srcY          /* Coordinates of top-left pixel to be used
                                 * in image being read. */
) {
    Int fileWidth, fileHeight, maxIntensity;
    Int x, y, c;
    int type;
    Tk_PhotoImageBlock block;
    FMTOPT opts;
    PPMFILE tf;
    Boln swapBytes, isAscii;
    int stopY, outY;
    int bytesPerPixel;
    Double minVals[IMG_MAX_CHANNELS], maxVals[IMG_MAX_CHANNELS];
    UByte  *pixbufPtr;
    UShort *ushortBufPtr;
    UByte  *ubyteBufPtr;
    Double gtable[IMG_GAMMA_TABLE_SIZE];

    memset (&tf, 0, sizeof (PPMFILE));

    swapBytes = tkimg_IsIntel ();

    if (ParseFormatOpts (interp, format, &opts) != TCL_OK) {
        return TCL_ERROR;
    }

    type = ReadPPMFileHeader (handle, &fileWidth, &fileHeight, &maxIntensity, &isAscii);
    if (type == 0) {
        Tcl_AppendResult(interp, "couldn't read PPM header from file \"",
                          filename, "\"", (char *)NULL);
        return TCL_ERROR;
    }

    if ((fileWidth <= 0) || (fileHeight <= 0)) {
        Tcl_AppendResult(interp, "PPM image file \"", filename,
                          "\" has dimension(s) <= 0", (char *) NULL);
        return TCL_ERROR;
    }
    if ((maxIntensity <= 0) || (maxIntensity >= 65536)) {
        char buffer[TCL_INTEGER_SPACE];

        tkimg_snprintf(buffer, TCL_INTEGER_SPACE, "%d", maxIntensity);
        Tcl_AppendResult(interp, "PPM image file \"", filename,
                          "\" has bad maximum intensity value ", buffer,
                          (char *) NULL);
        return TCL_ERROR;
    }

    bytesPerPixel = maxIntensity >= 256? 2: 1;

    tkimg_CreateGammaTable (opts.gamma, gtable);

    if (opts.verbose) {
        printImgInfo (fileWidth, fileHeight, maxIntensity, isAscii, type==PGM? 1: 3,
                      &opts, filename, "Reading image:");
    }

    if ((srcX + width) > fileWidth) {
        width = fileWidth - srcX;
    }
    if ((srcY + height) > fileHeight) {
        height = fileHeight - srcY;
    }
    if ((width <= 0) || (height <= 0)
        || (srcX >= fileWidth) || (srcY >= fileHeight)) {
        Tcl_AppendResult(interp, "Width or height are negative", (char *) NULL);
        return TCL_ERROR;
    }

    if (type == PGM) {
        block.pixelSize = 1;
        block.offset[1] = 0;
        block.offset[2] = 0;
    } else {
        block.pixelSize = 3;
        block.offset[1] = 1;
        block.offset[2] = 2;
    }
    block.offset[3] = block.offset[0] = 0;
    block.width = width;
    block.height = 1;
    block.pitch = block.pixelSize * fileWidth;
    tf.pixbuf = (UByte *) attemptckalloc (fileWidth * block.pixelSize);
    if (tf.pixbuf == NULL) {
        Tcl_AppendResult (interp, "Unable to allocate memory for image data.", (char *) NULL);
        return TCL_ERROR;
    }
    block.pixelPtr = tf.pixbuf + srcX * block.pixelSize;

    switch (bytesPerPixel) {
        case 2: {
            tf.ushortBuf = (UShort *)attemptckalloc (fileWidth*fileHeight*block.pixelSize*sizeof (UShort));
            if (tf.ushortBuf == NULL) {
                Tcl_AppendResult (interp, "Unable to allocate memory for image data.", (char *) NULL);
                ppmClose (&tf);
                return TCL_ERROR;
            }
            if (!readUShortFile(interp, handle, tf.ushortBuf, fileWidth, fileHeight, block.pixelSize,
                                 swapBytes, isAscii, opts.verbose, minVals, maxVals)) {
                ppmClose (&tf);
                return TCL_ERROR;
            }
            break;
        }
        case 1: {
            tf.ubyteBuf = (UByte *)attemptckalloc (fileWidth*fileHeight*block.pixelSize*sizeof (UByte));
            if (tf.ubyteBuf == NULL) {
                Tcl_AppendResult (interp, "Unable to allocate memory for image data.", (char *) NULL);
                ppmClose (&tf);
                return TCL_ERROR;
            }
            if (!readUByteFile (interp, handle, tf.ubyteBuf, fileWidth, fileHeight, block.pixelSize,
                        swapBytes, isAscii, opts.verbose, minVals, maxVals)) {
                ppmClose (&tf);
                return TCL_ERROR;
            }
            break;
        }
    }

    if (opts.minVal != 0.0 || opts.maxVal != 0.0) {
        for (c=0; c<block.pixelSize; c++) {
            minVals[c] = opts.minVal;
            maxVals[c] = opts.maxVal;
        }
    }
    switch (bytesPerPixel) {
        case 2: {
            tkimg_RemapUShortValues (
                tf.ushortBuf, fileWidth, fileHeight,
                block.pixelSize, minVals, maxVals, -1.0f, 0
            );
            break;
        }
    }

    if (tkimg_PhotoExpand(interp, imageHandle, destX + width, destY + height) == TCL_ERROR) {
        ppmClose (&tf);
        return TCL_ERROR;
    }

    stopY = srcY + height;
    outY = destY;

    for (y=0; y<stopY; y++) {
        pixbufPtr = tf.pixbuf;
        switch (bytesPerPixel) {
            case 2: {
                if (opts.scanOrder == BOTTOM_UP) {
                    ushortBufPtr = tf.ushortBuf + (fileHeight -1 - y) * fileWidth * block.pixelSize;
                } else {
                    ushortBufPtr = tf.ushortBuf + y * fileWidth * block.pixelSize;
                }
                tkimg_UShortToUByte (fileWidth * block.pixelSize, ushortBufPtr,
                                     opts.gamma != 1.0? gtable: NULL, pixbufPtr);
                ushortBufPtr += fileWidth * block.pixelSize;
                break;
            }
            case 1: {
                if (opts.scanOrder == BOTTOM_UP) {
                    ubyteBufPtr = tf.ubyteBuf + (fileHeight -1 - y) * fileWidth * block.pixelSize;
                } else {
                    ubyteBufPtr = tf.ubyteBuf + y * fileWidth * block.pixelSize;
                }
                for (x=0; x<fileWidth * block.pixelSize; x++) {
                    pixbufPtr[x] = ubyteBufPtr[x];
                }
                ubyteBufPtr += fileWidth * block.pixelSize;
                break;
            }
        }
        if (y >= srcY) {
            if (tkimg_PhotoPutBlock(interp, imageHandle, &block, destX, outY,
                                width, 1,
                                block.offset[3]?
                                TK_PHOTO_COMPOSITE_SET:
                                TK_PHOTO_COMPOSITE_OVERLAY) == TCL_ERROR) {
                ppmClose (&tf);
                return TCL_ERROR;
            }
            outY++;
        }
    }
    ppmClose (&tf);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * ChnWrite --
 *
 *      This procedure is invoked to write image data to a file in PPM
 *      format.
 *
 * Results:
 *      A standard TCL completion code.  If TCL_ERROR is returned
 *      then an error message is left in the interp's result.
 *
 * Side effects:
 *      Data is written to the file given by "filename".
 *
 *----------------------------------------------------------------------
 */

static int ChnWrite(
    Tcl_Interp *interp,
    const char *filename,
    Tcl_Obj *format,
    Tk_PhotoImageBlock *blockPtr
) {
    Tcl_Channel chan;
    tkimg_MFile handle;
    int result;

    chan = tkimg_OpenFileChannel (interp, filename, 0644);
    if (!chan) {
        return TCL_ERROR;
    }

    handle.data = (char *) chan;
    handle.state = IMG_CHAN;

    result = CommonWrite (interp, filename, format, &handle, blockPtr);
    if (Tcl_Close(interp, chan) == TCL_ERROR) {
        return TCL_ERROR;
    }
    return result;
}

static int StringWrite(
    Tcl_Interp *interp,
    Tcl_Obj *format,
    Tk_PhotoImageBlock *blockPtr
) {
    tkimg_MFile handle;
    int result;
    Tcl_DString data;

    Tcl_DStringInit(&data);
    tkimg_WriteInit (&data, &handle);
    result = CommonWrite (interp, "InlineData", format, &handle, blockPtr);
    tkimg_Putc(IMG_DONE, &handle);

    if (result == TCL_OK) {
        Tcl_DStringResult(interp, &data);
    } else {
        Tcl_DStringFree(&data);
    }
    return result;
}

static int writeAsciiRow (tkimg_MFile *handle, const unsigned char *scanline, int nBytes)
{
    int i;
    char buf[TCL_INTEGER_SPACE];

    for (i=0; i<nBytes; i++) {
        tkimg_snprintf (buf, TCL_INTEGER_SPACE, "%d\n", scanline[i]);
        if (tkimg_Write2(handle, buf, strlen(buf)) != strlen(buf)) {
            return i;
        }
    }
    return nBytes;
}

static int CommonWrite(
    Tcl_Interp *interp,
    const char *filename,
    Tcl_Obj *format,
    tkimg_MFile *handle,
    Tk_PhotoImageBlock *blockPtr
) {
    int w, h;
    int redOff, greenOff, blueOff, nBytes;
    unsigned char *scanline, *scanlinePtr;
    unsigned char *pixelPtr, *pixLinePtr;
    char header[16 + TCL_INTEGER_SPACE * 2];
    FMTOPT opts;

    if (ParseFormatOpts (interp, format, &opts) != TCL_OK) {
        return TCL_ERROR;
    }

    tkimg_snprintf(header, 16 + TCL_INTEGER_SPACE * 2,
                  "P%d\n%d %d\n255\n", opts.writeAscii? 3: 6,
                   blockPtr->width, blockPtr->height);
    if (tkimg_Write2(handle, header, strlen(header)) != strlen(header)) {
        goto writeerror;
    }

    pixLinePtr = blockPtr->pixelPtr + blockPtr->offset[0];
    redOff     = 0;
    greenOff   = blockPtr->offset[1] - blockPtr->offset[0];
    blueOff    = blockPtr->offset[2] - blockPtr->offset[0];

    nBytes = blockPtr->width * 3; /* Only RGB images allowed. */
    scanline = (unsigned char *) attemptckalloc((unsigned) nBytes);
    if (scanline == NULL) {
        Tcl_AppendResult (interp, "Unable to allocate memory for image data.", (char *) NULL);
        goto writeerror;
    }
    for (h = blockPtr->height; h > 0; h--) {
        pixelPtr = pixLinePtr;
        scanlinePtr = scanline;
        for (w = blockPtr->width; w > 0; w--) {
            *(scanlinePtr++) = pixelPtr[redOff];
            *(scanlinePtr++) = pixelPtr[greenOff];
            *(scanlinePtr++) = pixelPtr[blueOff];
            pixelPtr += blockPtr->pixelSize;
        }
        if (opts.writeAscii) {
            if (writeAsciiRow(handle, scanline, nBytes) != nBytes) {
                goto writeerror;
            }
        } else {
            if (tkimg_Write2(handle, (char *) scanline, nBytes) != nBytes) {
                goto writeerror;
            }
        }
        pixLinePtr += blockPtr->pitch;
    }
    ckfree ((char *) scanline);
    return TCL_OK;

 writeerror:
    Tcl_AppendResult(interp, "Error writing \"", filename, "\": ",
                      (char *) NULL);
    return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * ReadPPMFileHeader --
 *
 *      This procedure reads the PPM header from the beginning of a
 *      PPM file and returns information from the header.
 *
 * Results:
 *      The return value is PGM if file "f" appears to start with
 *      a valid PGM header, PPM if "f" appears to start with a valid
 *      PPM header, and 0 otherwise.  If the header is valid,
 *      then *widthPtr and *heightPtr are modified to hold the
 *      dimensions of the image and *maxIntensityPtr is modified to
 *      hold the value of a "fully on" intensity value.
 *
 * Side effects:
 *      The access position in f advances.
 *
 *----------------------------------------------------------------------
 */

static int
ReadPPMFileHeader(
    tkimg_MFile *handle,        /* Image file to read the header from */
    int *widthPtr, int *heightPtr,  /* The dimensions of the image are
                                     * returned here. */
    int *maxIntensityPtr,       /* The maximum intensity value for
                                 * the image is stored here. */
    Boln *isAsciiPtr
) {
#define BUFFER_SIZE 1000
    char buffer[BUFFER_SIZE];
    int i, numFields;
    int type = 0;
    char c;

    /*
     * Read 4 space-separated fields from the file, ignoring
     * comments (any line that starts with "#").
     */

    if (tkimg_Read2(handle, &c, 1) != 1) {
        return 0;
    }
    i = 0;
    for (numFields = 0; numFields < 4; numFields++) {
        /*
         * Skip comments and white space.
         */

        while (1) {
            while (isspace(UCHAR(c))) {
                if (tkimg_Read2(handle, &c, 1) != 1) {
                    return 0;
                }
            }
            if (c != '#') {
                break;
            }
            do {
                if (tkimg_Read2(handle, &c, 1) != 1) {
                    return 0;
                }
            } while (c != '\n');
        }

        /*
         * Read a field (everything up to the next white space).
         */

        while (!isspace(UCHAR(c))) {
            if (i < (BUFFER_SIZE-2)) {
                buffer[i] = c;
                i++;
            }
            if (tkimg_Read2(handle, &c, 1) != 1) {
                goto done;
            }
        }
        if (i < (BUFFER_SIZE-1)) {
            buffer[i] = ' ';
            i++;
        }
    }
    done:
    buffer[i] = 0;

    /*
     * Parse the fields, which are: id, width, height, maxIntensity.
     */

    *isAsciiPtr = 0;
    if (strncmp(buffer, "P6 ", 3) == 0) {
        type = PPM;
    } else if (strncmp(buffer, "P3 ", 3) == 0) {
        type = PPM;
        *isAsciiPtr = 1;
    } else if (strncmp(buffer, "P5 ", 3) == 0) {
        type = PGM;
    } else if (strncmp(buffer, "P2 ", 3) == 0) {
        type = PGM;
        *isAsciiPtr = 1;
    } else {
        return 0;
    }
    if (sscanf(buffer+3, "%d %d %d", widthPtr, heightPtr, maxIntensityPtr)
            != 3) {
        return 0;
    }
    return type;
}
