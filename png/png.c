/*
 * png.c --
 *
 *  PNG photo image type, Tcl/Tk package
 *
 * Copyright (c) 2002 Andreas Kupries <andreas_kupries@users.sourceforge.net>
 *
 * This Tk image format handler reads and writes PNG files in the standard
 * PNG file format.  ("PNG" should be the format name.)  It can also read
 * and write strings containing base64-encoded PNG data.
 *
 * Author : Jan Nijtmans
 * Date   : 2/13/97
 * Original implementation : Joel Crisp
 *
 * The following format options are available:
 *
 * Read  PNG image: "png -matte <bool> -alpha <float>"
 * Write PNG image: None yet.
 *
 * -matte <bool>:  If set to false, a matte (alpha) channel is ignored
 *                 during reading. Default is true.
 * -alpha <float>: An additional alpha filtering for the overall image, which
 *                 allows the background on which the image is displayed to show through.
 *                 This usually also has the effect of desaturating the image.
 *                 The alphaValue must be between 0.0 and 1.0. 
 *                 Specifying an alpha value, overrides the setting of the matte flag,
 *                 i.e. reading a file which has no alpha channel (Greyscale, RGB) will
 *                 add an alpha channel to the image independent of the matte flag setting.
 *
 * $Id: png.c 361 2013-10-02 18:12:59Z obermeier $
 */

/*
 * Generic initialization code, parameterized via CPACKAGE and PACKAGE.
 */

#include "pngtcl.h"
#include <string.h>
#include <stdlib.h>

static int SetupPngLibrary(Tcl_Interp *interp);

#define MORE_INITIALIZATION \
    if (SetupPngLibrary (interp) != TCL_OK) { return TCL_ERROR; }

#include "init.c"



#define COMPRESS_THRESHOLD 1024

typedef struct png_text_struct_compat
{
   png_text compat;
   png_size_t itxt_length; /* length of the itxt string */
   png_charp lang;         /* language code, 0-79 characters
                              or a NULL pointer */
   png_charp lang_key;     /* keyword translated UTF-8 string, 0 or more
                              chars or a NULL pointer */
} png_text_compat;

typedef struct cleanup_info {
    Tcl_Interp *interp;
    jmp_buf jmpbuf;
} cleanup_info;

/*
 * Prototypes for local procedures defined in this file:
 */

static int CommonMatchPNG(tkimg_MFile *handle, int *widthPtr,
        int *heightPtr);

static int CommonReadPNG(png_structp png_ptr,
        Tcl_Interp* interp, Tcl_Obj *format,
        Tk_PhotoHandle imageHandle, int destX, int destY, int width,
        int height, int srcX, int srcY);

static int CommonWritePNG(Tcl_Interp *interp, png_structp png_ptr,
        png_infop info_ptr, Tcl_Obj *format,
        Tk_PhotoImageBlock *blockPtr);

static void tk_png_error(png_structp, png_const_charp);

static void tk_png_warning(png_structp, png_const_charp);

/*
 * These functions are used for all Input/Output.
 */

static void     tk_png_read(png_structp, png_bytep,
        png_size_t);

static void     tk_png_write(png_structp, png_bytep,
        png_size_t);

static void     tk_png_flush(png_structp);

static int ParseFormatOpts (interp, format, matte, alpha)
    Tcl_Interp *interp;
    Tcl_Obj *format;
    int *matte;
    double *alpha;
{
    static const char *const pngOptions[] = {"-matte", "-alpha", NULL};
    int objc, length, i, index;
    Tcl_Obj **objv;
    const char *matteStr;
    const char *alphaStr;

    *matte = 1;
    *alpha = -1.0;

    if (tkimg_ListObjGetElements(interp, format, &objc, &objv) != TCL_OK)
        return TCL_ERROR;
    if (objc) {
        matteStr = "1";
        alphaStr = "-1.0";
        for (i=1; i<objc; i++) {
            if (Tcl_GetIndexFromObj(interp, objv[i], (CONST84 char *CONST86 *)pngOptions,
                    "format option", 0, &index) != TCL_OK) {
                return TCL_ERROR;
            }
            if (++i >= objc) {
                Tcl_AppendResult(interp, "No value for option \"",
                        Tcl_GetStringFromObj (objv[--i], (int *) NULL),
                        "\"", (char *) NULL);
                return TCL_ERROR;
            }
            switch(index) {
                case 0:
                    matteStr = Tcl_GetStringFromObj(objv[i], (int *) NULL);
                    break;
                case 1:
                    alphaStr = Tcl_GetStringFromObj(objv[i], (int *) NULL);
                    break;
            }
        }

        length = strlen (matteStr);
        if (!strncmp (matteStr, "1", length) || \
            !strncmp (matteStr, "true", length) || \
            !strncmp (matteStr, "on", length)) {
            *matte = 1;
        } else if (!strncmp (matteStr, "0", length) || \
            !strncmp (matteStr, "false", length) || \
            !strncmp (matteStr, "off", length)) {
            *matte = 0;
        } else {
            Tcl_AppendResult(interp, "invalid alpha (matte) mode \"", matteStr,
                              "\": should be 1 or 0, on or off, true or false",
                              (char *) NULL);
            return TCL_ERROR;
        }
        if (strcmp (alphaStr, "-1.0")) {
            *alpha = atof (alphaStr);
            if (*alpha < 0.0 ) *alpha = 0.0;
            if (*alpha > 1.0 ) *alpha = 1.0;
        }
    }
    return TCL_OK;
}

/*
 *
 */

static int
SetupPngLibrary (interp)
    Tcl_Interp *interp;
{
    if (Pngtcl_InitStubs(interp, PNGTCL_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static void
tk_png_error(png_ptr, error_msg)
    png_structp png_ptr;
    png_const_charp error_msg;
{
    cleanup_info *info = (cleanup_info *) png_get_error_ptr(png_ptr);
    Tcl_AppendResult(info->interp, error_msg, (char *) NULL);
    longjmp(info->jmpbuf,1);
}

static void
tk_png_warning(png_ptr, error_msg)
    png_structp png_ptr;
    png_const_charp error_msg;
{
    return;
}

static void
tk_png_read(png_ptr, data, length)
    png_structp png_ptr;
    png_bytep data;
    png_size_t length;
{
    if (tkimg_Read((tkimg_MFile *) png_get_progressive_ptr(png_ptr),
            (char *) data, (size_t) length) != (int) length) {
        png_error(png_ptr, "Read Error");
    }
}

static void
tk_png_write(png_ptr, data, length)
    png_structp png_ptr;
    png_bytep data;
    png_size_t length;
{
    if (tkimg_Write((tkimg_MFile *) png_get_progressive_ptr(png_ptr),
            (char *) data, (size_t) length) != (int) length) {
        png_error(png_ptr, "Write Error");
    }
}

static void
tk_png_flush(png_ptr)
    png_structp png_ptr;
{
}

static int ChnMatch(
    Tcl_Channel chan,
    const char *fileName,
    Tcl_Obj *format,
    int *widthPtr,
    int *heightPtr,
    Tcl_Interp *interp
) {
    tkimg_MFile handle;

    handle.data = (char *) chan;
    handle.state = IMG_CHAN;

    return CommonMatchPNG(&handle, widthPtr, heightPtr);
}

static int
ObjMatch(
    Tcl_Obj *data,
    Tcl_Obj *format,
    int *widthPtr,
    int *heightPtr,
    Tcl_Interp *interp
) {
    tkimg_MFile handle;

    if (!tkimg_ReadInit(data, '\211', &handle)) {
        return 0;
    }
    return CommonMatchPNG(&handle, widthPtr, heightPtr);
}

static int
CommonMatchPNG(handle, widthPtr, heightPtr)
    tkimg_MFile *handle;
    int *widthPtr, *heightPtr;
{
    unsigned char buf[8];

    if ((tkimg_Read(handle, (char *) buf, 8) != 8)
            || (strncmp("\211\120\116\107\15\12\32\12", (char *) buf, 8) != 0)
            || (tkimg_Read(handle, (char *) buf, 8) != 8)
            || (strncmp("\111\110\104\122", (char *) buf+4, 4) != 0)
            || (tkimg_Read(handle, (char *) buf, 8) != 8)) {
        return 0;
    }
    *widthPtr = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];
    *heightPtr = (buf[4]<<24) + (buf[5]<<16) + (buf[6]<<8) + buf[7];
    return 1;
}

static int
ChnRead(interp, chan, fileName, format, imageHandle,
        destX, destY, width, height, srcX, srcY)
    Tcl_Interp *interp;
    Tcl_Channel chan;
    const char *fileName;
    Tcl_Obj *format;
    Tk_PhotoHandle imageHandle;
    int destX, destY;
    int width, height;
    int srcX, srcY;
{
    png_structp png_ptr;
    tkimg_MFile handle;
    cleanup_info cleanup;

    handle.data = (char *) chan;
    handle.state = IMG_CHAN;

    cleanup.interp = interp;

    png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,
            (png_voidp) &cleanup,tk_png_error,tk_png_warning);
    if (!png_ptr) return(0);

    png_set_read_fn(png_ptr, (png_voidp) &handle, tk_png_read);

    return CommonReadPNG(png_ptr, interp, format, imageHandle, destX, destY,
            width, height, srcX, srcY);
}

static int
ObjRead (interp, dataObj, format, imageHandle,
        destX, destY, width, height, srcX, srcY)
    Tcl_Interp *interp;
    Tcl_Obj *dataObj;
    Tcl_Obj *format;
    Tk_PhotoHandle imageHandle;
    int destX, destY;
    int width, height;
    int srcX, srcY;
{
    png_structp png_ptr;
    tkimg_MFile handle;
    cleanup_info cleanup;

    cleanup.interp = interp;

    png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,
            (png_voidp) &cleanup,tk_png_error,tk_png_warning);
    if (!png_ptr) return TCL_ERROR;

    tkimg_ReadInit(dataObj,'\211',&handle);

    png_set_read_fn(png_ptr,(png_voidp) &handle, tk_png_read);

    return CommonReadPNG(png_ptr, interp, format, imageHandle, destX, destY,
            width, height, srcX, srcY);
}

static int
CommonReadPNG(png_ptr, interp, format, imageHandle, destX, destY,
        width, height, srcX, srcY)
    png_structp png_ptr;
    Tcl_Interp *interp;
    Tcl_Obj *format;
    Tk_PhotoHandle imageHandle;
    int destX, destY;
    int width, height;
    int srcX, srcY;
{
    png_infop info_ptr;
    png_infop end_info;
    char **png_data = NULL;
    Tk_PhotoImageBlock block;
    unsigned int I;
    png_uint_32 info_width, info_height;
    int bit_depth, color_type, interlace_type;
    int intent;
    int result = TCL_OK;
    int matte;
    double alpha;
    int useAlpha = 0;
    int addAlpha = 0;

    if (ParseFormatOpts(interp, format, &matte, &alpha) != TCL_OK) {
        return TCL_ERROR;
    }

    info_ptr=png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr,NULL,NULL);
        return(TCL_ERROR);
    }

    end_info=png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
        return(TCL_ERROR);
    }

    if (setjmp((((cleanup_info *) png_get_error_ptr(png_ptr))->jmpbuf))) {
        if (png_data) {
            ckfree((char *)png_data);
        }
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return TCL_ERROR;
    }

    png_read_info(png_ptr,info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &info_width, &info_height, &bit_depth,
        &color_type, &interlace_type, (int *) NULL, (int *) NULL);

    if ((srcX + width) > (int) info_width) {
        width = info_width - srcX;
    }
    if ((srcY + height) > (int) info_height) {
        height = info_height - srcY;
    }
    if ((width <= 0) || (height <= 0)
        || (srcX >= (int) info_width)
        || (srcY >= (int) info_height)) {
        png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
        return TCL_OK;
    }

    if (tkimg_PhotoExpand(interp, imageHandle, destX + width, destY + height) == TCL_ERROR) {
        png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
        return TCL_ERROR;
    }

    Tk_PhotoGetImage(imageHandle, &block);

    if (png_set_strip_16 != NULL) {
        png_set_strip_16(png_ptr);
    } else if (bit_depth == 16) {
        block.offset[1] = 2;
        block.offset[2] = 4;
    }

    if (png_set_expand != NULL) {
        png_set_expand(png_ptr);
    }

    if ((color_type & PNG_COLOR_MASK_COLOR) == 0) {
        /* grayscale image */
        block.offset[1] = 0;
        block.offset[2] = 0;
    }
    block.width = width;
    block.height = height;

    if ((color_type & PNG_COLOR_MASK_ALPHA)
        || png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        /* Image has an alpha channel.
           Check, if we want to use the alpha channel at all (matte == true)
           and then, if the alpha multiply value should be applied (alpha >= 0.0) */
        if (!matte) {
            png_set_strip_alpha (png_ptr);
            block.pixelSize--;
            block.pitch = block.pixelSize * width;
            block.offset[3] = 0;
        } else {
            block.offset[3] = block.pixelSize - 1;
            if ( alpha >= 0.0) {
                useAlpha = 1;
            }
        }
    } else {
        /* Image has no alpha channel.
           If a valid alpha multiply has been specified, add an alpha channel to the image.
           The matte flag is ignored. */
        if ( alpha >= 0.0) {
            addAlpha = 1;
            png_set_add_alpha(png_ptr, (unsigned int)(alpha*255), PNG_FILLER_AFTER);
        } else {
            block.offset[3] = 0;
        }
    }

    /* Note: png_read_update_info may only be called once per info_ptr !! */
    png_read_update_info(png_ptr,info_ptr);
    block.pixelSize = png_get_channels(png_ptr, info_ptr);
    block.pitch = png_get_rowbytes(png_ptr, info_ptr);
    if (addAlpha) {
        block.offset[3] = block.pixelSize - 1;
    }

    if (png_get_sRGB && png_get_sRGB(png_ptr, info_ptr, &intent)) {
        png_set_sRGB(png_ptr, info_ptr, intent);
    } else if (png_get_gAMA) {
        double gamma;
        if (!png_get_gAMA(png_ptr, info_ptr, &gamma)) {
            gamma = 0.45455;
        }
        png_set_gamma(png_ptr, 1.0, gamma);
    }

    png_data= (char **) ckalloc(sizeof(char *) * info_height +
            info_height * block.pitch);

    for(I=0;I<info_height;I++) {
        png_data[I]= ((char *) png_data) + (sizeof(char *) * info_height +
                I * block.pitch);
    }

    png_read_image(png_ptr,(png_bytepp) png_data);

    block.pixelPtr=(unsigned char *) (png_data[srcY]+srcX*block.pixelSize);

    if (useAlpha) {
        unsigned char * alphaPtr = block.pixelPtr + block.offset[3];
        for(I=0;I<height*width;I++) {
            *alphaPtr = alpha * *alphaPtr;
            alphaPtr += block.offset[3] + 1 ;
        }
    }

    if (tkimg_PhotoPutBlock(interp, imageHandle, &block, destX, destY, width, height,
            block.offset[3]? TK_PHOTO_COMPOSITE_OVERLAY: TK_PHOTO_COMPOSITE_SET) == TCL_ERROR) {
        result = TCL_ERROR;
    }

    ckfree((char *) png_data);
    png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
    return result;
}

static int
ChnWrite (interp, filename, format, blockPtr)
    Tcl_Interp *interp;
    const char *filename;
    Tcl_Obj *format;
    Tk_PhotoImageBlock *blockPtr;
{
    png_structp png_ptr;
    png_infop info_ptr;
    tkimg_MFile handle;
    int result;
    cleanup_info cleanup;
    Tcl_Channel chan = (Tcl_Channel) NULL;

    chan = tkimg_OpenFileChannel(interp, filename, 0644);
    if (!chan) {
        return TCL_ERROR;
    }

    handle.data = (char *) chan;
    handle.state = IMG_CHAN;

    cleanup.interp = interp;

    png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING,
            (png_voidp) &cleanup,tk_png_error,tk_png_warning);
    if (!png_ptr) {
        Tcl_Close(NULL, chan);
        return TCL_ERROR;
    }

    info_ptr=png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr,NULL);
        Tcl_Close(NULL, chan);
        return TCL_ERROR;
    }

    png_set_write_fn(png_ptr,(png_voidp) &handle, tk_png_write, tk_png_flush);

    result = CommonWritePNG(interp, png_ptr, info_ptr, format, blockPtr);
    Tcl_Close(NULL, chan);
    return result;
}

static int StringWrite(
    Tcl_Interp *interp,
    Tcl_Obj *format,
    Tk_PhotoImageBlock *blockPtr
) {
    png_structp png_ptr;
    png_infop info_ptr;
    tkimg_MFile handle;
    int result;
    cleanup_info cleanup;
    Tcl_DString data;

    Tcl_DStringInit(&data);
    cleanup.interp = interp;

    png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING,
            (png_voidp) &cleanup, tk_png_error, tk_png_warning);
    if (!png_ptr) {
        return TCL_ERROR;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr,NULL);
        return TCL_ERROR;
    }

    png_set_write_fn(png_ptr, (png_voidp) &handle, tk_png_write, tk_png_flush);

    tkimg_WriteInit(&data, &handle);

    result = CommonWritePNG(interp, png_ptr, info_ptr, format, blockPtr);
    tkimg_Putc(IMG_DONE, &handle);
    if (result == TCL_OK) {
        Tcl_DStringResult(interp, &data);
    } else {
        Tcl_DStringFree(&data);
    }
    return result;
}

static int
CommonWritePNG(interp, png_ptr, info_ptr, format, blockPtr)
    Tcl_Interp *interp;
    png_structp png_ptr;
    png_infop info_ptr;
    Tcl_Obj *format;
    Tk_PhotoImageBlock *blockPtr;
{
    int greenOffset, blueOffset, alphaOffset;
    int tagcount = 0;
    Tcl_Obj **tags = (Tcl_Obj **) NULL;
    int I, pass, number_passes, color_type;
    int newPixelSize;
    png_bytep row_pointers = (png_bytep) NULL;

    if (tkimg_ListObjGetElements(interp, format, &tagcount, &tags) != TCL_OK) {
        return TCL_ERROR;
    }
    tagcount = (tagcount > 1) ? (tagcount - 1) / 2: 0;

    if (setjmp((((cleanup_info *) png_get_error_ptr(png_ptr))->jmpbuf))) {
        if (row_pointers) {
            ckfree((char *) row_pointers);
        }
        png_destroy_write_struct(&png_ptr,&info_ptr);
        return TCL_ERROR;
    }
    greenOffset = blockPtr->offset[1] - blockPtr->offset[0];
    blueOffset = blockPtr->offset[2] - blockPtr->offset[0];
    alphaOffset = blockPtr->offset[0];
    if (alphaOffset < blockPtr->offset[2]) {
        alphaOffset = blockPtr->offset[2];
    }
    if (++alphaOffset < blockPtr->pixelSize) {
        alphaOffset -= blockPtr->offset[0];
    } else {
        alphaOffset = 0;
    }

    if (greenOffset || blueOffset) {
        color_type = PNG_COLOR_TYPE_RGB;
        newPixelSize = 3;
    } else {
        color_type = PNG_COLOR_TYPE_GRAY;
        newPixelSize = 1;
    }
    if (alphaOffset) {
        color_type |= PNG_COLOR_MASK_ALPHA;
        newPixelSize++;
#if 0 /* The function png_set_filler doesn't seem to work; don't known why :-( */
    } else if ((blockPtr->pixelSize==4) && (newPixelSize == 3)
            && (png_set_filler != NULL)) {
        /*
         * The set_filler() function doesn't need to be called
         * because the code below can handle all necessary
         * re-allocation of memory. Only it is more economically
         * to let the PNG library do that, which is only
         * possible with v0.95 and higher.
         */
        png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
        newPixelSize++;
#endif
    }

    png_set_IHDR(png_ptr, info_ptr, blockPtr->width, blockPtr->height, 8,
            color_type, PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_BASE,
            PNG_FILTER_TYPE_BASE);

    if (png_set_gAMA) {
        png_set_gAMA(png_ptr, info_ptr, 1.0);
    }

    if (tagcount > 0) {
        png_text_compat text;
        for(I=0;I<tagcount;I++) {
            int length;
            memset(&text, 0, sizeof(png_text_compat));
            text.compat.key = Tcl_GetStringFromObj(tags[2*I+1], (int *) NULL);
            text.compat.text = Tcl_GetStringFromObj(tags[2*I+2], &length);
            text.compat.text_length = length;
            if (text.compat.text_length>COMPRESS_THRESHOLD) {
                text.compat.compression = PNG_TEXT_COMPRESSION_zTXt;
            } else {
                text.compat.compression = PNG_TEXT_COMPRESSION_NONE;
            }
            png_set_text(png_ptr, info_ptr, &text.compat, 1);
        }
    }
    png_write_info(png_ptr,info_ptr);

    number_passes = png_set_interlace_handling(png_ptr);

    if (blockPtr->pixelSize != newPixelSize) {
        int J, oldPixelSize;
        png_bytep src, dst;
        oldPixelSize = blockPtr->pixelSize;
        row_pointers = (png_bytep)
                ckalloc(blockPtr->width * newPixelSize);
        for (pass = 0; pass < number_passes; pass++) {
            for(I=0; I<blockPtr->height; I++) {
                src = (png_bytep) blockPtr->pixelPtr
                        + I * blockPtr->pitch + blockPtr->offset[0];
                dst = row_pointers;
                for (J = blockPtr->width; J > 0; J--) {
                    memcpy(dst, src, newPixelSize);
                    src += oldPixelSize;
                    dst += newPixelSize;
                }
                png_write_row(png_ptr, row_pointers);
            }
        }
        ckfree((char *) row_pointers);
        row_pointers = NULL;
    } else {
        for (pass = 0; pass < number_passes; pass++) {
            for(I=0;I<blockPtr->height;I++) {
                png_write_row(png_ptr, (png_bytep) blockPtr->pixelPtr
                        + I * blockPtr->pitch + blockPtr->offset[0]);
            }
        }
    }
    png_write_end(png_ptr,NULL);
    png_destroy_write_struct(&png_ptr,&info_ptr);

    return(TCL_OK);
}
