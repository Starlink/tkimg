/*
 * tkimg.c --
 *
 *  Generic interface to XML parsers.
 *
 * Copyright (c) 2002 Andreas Kupries <andreas_kupries@users.sourceforge.net>
 *
 * Zveno Pty Ltd makes this software and associated documentation
 * available free of charge for any purpose.  You may make copies
 * of the software but you must include all of this notice on any copy.
 *
 * Zveno Pty Ltd does not warrant that this software is error free
 * or fit for any purpose.  Zveno Pty Ltd disclaims any liability for
 * all claims, expenses, losses, damages and costs any user may incur
 * as a result of using, copying or modifying the software.
 */

#include "tkimg.h"
#include "tkimgUuid.h"

MODULE_SCOPE const TkimgStubs tkimgStubs;

/*
 * Declarations for externally visible functions.
 */

#ifdef ALLOW_B64
static int tob64(void *clientData, Tcl_Interp *interp,
	int argc, Tcl_Obj *const objv[]);
static int fromb64(void *clientData, Tcl_Interp *interp,
	int argc, Tcl_Obj *const objv[]);
#endif

/*
 *----------------------------------------------------------------------------
 *
 * Tkimg_Init --
 *
 *  Initialisation routine for loadable module
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Creates commands in the interpreter,
 *  loads xml package.
 *
 *----------------------------------------------------------------------------
 */

#ifndef STRINGIFY
#  define STRINGIFY(x) STRINGIFY1(x)
#  define STRINGIFY1(x) #x
#endif

int Tkimg_Init(
	Tcl_Interp *interp /* Interpreter to initialise. */
) {
	Tcl_CmdInfo info;

	if (!Tcl_InitStubs(interp, "8.3", 0)) {
		return TCL_ERROR;
	}
	if (!Tk_InitStubs(interp, "8.3", 0)) {
		return TCL_ERROR;
	}
	TkimgInitUtilities(interp);
#ifdef ALLOW_B64 /* Undocumented feature */
	Tcl_CreateObjCommand(interp, "img_to_base64", tob64, NULL, NULL);
	Tcl_CreateObjCommand(interp, "img_from_base64", fromb64, NULL, NULL);
#endif

	if (Tcl_PkgProvideEx(interp, PACKAGE_TCLNAME, PACKAGE_VERSION,
			(void *)&tkimgStubs) != TCL_OK
	) {
		return TCL_ERROR;
	}
	if (Tcl_GetCommandInfo(interp, "::tcl::build-info", &info)) {
	    Tcl_CreateObjCommand(interp, "::tkimg::build-info",
	    info.objProc, (void *)(PACKAGE_VERSION
		    "+" STRINGIFY(TKIMG_VERSION_UUID)
#ifdef ALLOW_B64 /* Undocumented feature */
		    ".b64"
#endif
#if defined(__clang__) && defined(__clang_major__)
		    ".clang-" STRINGIFY(__clang_major__)
#if __clang_minor__ < 10
		    "0"
#endif
		    STRINGIFY(__clang_minor__)
#endif
#if defined(__cplusplus) && !defined(__OBJC__)
		    ".cplusplus"
#endif
#ifndef NDEBUG
		    ".debug"
#endif
#ifdef DEBUG_LOCAL
		    ".debug-local"
#endif
#if !defined(__clang__) && !defined(__INTEL_COMPILER) && defined(__GNUC__)
		    ".gcc-" STRINGIFY(__GNUC__)
#if __GNUC_MINOR__ < 10
		    "0"
#endif
		    STRINGIFY(__GNUC_MINOR__)
#endif
#ifdef __INTEL_COMPILER
		    ".icc-" STRINGIFY(__INTEL_COMPILER)
#endif
#ifdef TCL_MEM_DEBUG
		    ".memdebug"
#endif
#if defined(_MSC_VER)
		    ".msvc-" STRINGIFY(_MSC_VER)
#endif
#ifdef USE_NMAKE
		    ".nmake"
#endif
#ifndef TCL_CFG_OPTIMIZED
		    ".no-optimize"
#endif
#ifdef __OBJC__
		    ".objective-c"
#if defined(__cplusplus)
		    "plusplus"
#endif
#endif
#ifdef TCL_CFG_PROFILED
		    ".profile"
#endif
#ifdef PURIFY
		    ".purify"
#endif
#ifdef STATIC_BUILD
		    ".static"
#endif
	    ), NULL);
	}

	return TCL_OK;
}

/*
 *----------------------------------------------------------------------------
 *
 * Tkimg_SafeInit --
 *
 *  Initialisation routine for loadable module in a safe interpreter.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Creates commands in the interpreter,
 *  loads xml package.
 *
 *----------------------------------------------------------------------------
 */

int Tkimg_SafeInit(
	Tcl_Interp *interp /* Interpreter to initialise. */
) {
	return Tkimg_Init(interp);
}

/*
 *-------------------------------------------------------------------------
 * tob64 --
 *  This function converts the contents of a file into a base-64
 *  encoded string.
 *
 * Results:
 *  none
 *
 * Side effects:
 *  none
 *
 *-------------------------------------------------------------------------
 */

#ifdef ALLOW_B64
int tob64(
	void *clientData,
	Tcl_Interp *interp,
	int argc,
	Tcl_Obj *const objv[]
) {
	Tcl_DString dstring;
	tkimg_MFile handle;
	Tcl_Channel chan;
	char buffer[1024];
	size_t len;

	if (argc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "filename");
		return TCL_ERROR;
	}

	chan = tkimg_OpenFileChannel(interp, Tcl_GetStringFromObj(objv[1], &len), 0);
	if (!chan) {
		return TCL_ERROR;
	}

	Tcl_DStringInit(&dstring);
	tkimg_WriteInit(&dstring, &handle);

	while ((len = Tcl_Read(chan, buffer, 1024)) == 1024) {
		tkimg_Write(&handle, buffer, 1024);
	}
	if (len + 1 > 1) {
		tkimg_Write(&handle, buffer, len);
	}
	if ((Tcl_Close(interp, chan) == TCL_ERROR) || (len < 0)) {
		Tcl_DStringFree(&dstring);
		Tcl_AppendResult(interp, Tcl_GetStringFromObj(objv[1], &len),
			": ", Tcl_PosixError(interp), (char *)NULL);
		return TCL_ERROR;
	}
	tkimg_Putc(IMG_DONE, &handle);

	Tcl_DStringResult(interp, &dstring);
	return TCL_OK;
}

/*
 *-------------------------------------------------------------------------
 * fromb64 --
 *  This function converts a base-64 encoded string into binary form,
 *  which is written to a file.
 *
 * Results:
 *  none
 *
 * Side effects:
 *  none
 *
 *-------------------------------------------------------------------------
 */

int fromb64(
	void *clientData,
	Tcl_Interp *interp,
	int argc,
	Tcl_Obj *const objv[]
) {
	tkimg_MFile handle;
	Tcl_Channel chan;
	char buffer[1024];
	size_t len;

	if (argc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "filename data");
		return TCL_ERROR;
	}

	chan = tkimg_OpenFileChannel(interp, Tcl_GetStringFromObj(objv[1], &len), 0644);
	if (!chan) {
		return TCL_ERROR;
	}

	handle.data = Tcl_GetStringFromObj(objv[2], &handle.length);
	handle.state = 0;

	while ((len = tkimg_Read(&handle, buffer, 1024)) == 1024) {
		if (Tcl_Write(chan, buffer, 1024) != 1024) {
			goto writeerror;
		}
	}
	if (len + 1 > 1) {
		if ((size_t)Tcl_Write(chan, buffer, len) != len) {
			goto writeerror;
		}
	}
	if (Tcl_Close(interp, chan) == TCL_ERROR) {
		return TCL_ERROR;
	}
	return TCL_OK;

writeerror:
	Tcl_AppendResult(interp, Tcl_GetStringFromObj(objv[1], &len), ": ",
		Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
}
#endif
