/*
 * tifftcl.c --
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
 *
 * $Id: tifftcl.c,v 1.2 2002/12/05 07:08:42 andreas_kupries Exp $
 *
 */

#include "tcl.h"
#include "tifftcl.h"

#define TCL_DOES_STUBS \
    (TCL_MAJOR_VERSION > 8 || TCL_MAJOR_VERSION == 8 && (TCL_MINOR_VERSION > 1 || \
    (TCL_MINOR_VERSION == 1 && TCL_RELEASE_LEVEL == TCL_FINAL_RELEASE)))

/*
 * Declarations for externally visible functions.
 */

#undef TCL_STORAGE_CLASS
#ifdef BUILD_tifftcl
# define TCL_STORAGE_CLASS DLLEXPORT
#else
# ifdef USE_TIFFTCL_STUBS
#  define TCL_STORAGE_CLASS
# else
#  define TCL_STORAGE_CLASS DLLIMPORT
# endif
#endif

EXTERN int Tifftcl_Init     _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN int Tifftcl_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));

#undef  TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

/*
 * Prototypes for procedures defined later in this file:
 */

/*
 *----------------------------------------------------------------------------
 *
 * Tifftcl_Init --
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

int
Tifftcl_Init (interp)
      Tcl_Interp *interp; /* Interpreter to initialise. */
{
#if TCL_DOES_STUBS
  extern TifftclStubs tifftclStubs;
#endif

#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
#endif

#if TCL_DOES_STUBS
  if (Tcl_PkgProvideEx(interp, PACKAGE_NAME, TIFFTCL_VERSION,
		       (ClientData) &tifftclStubs) != TCL_OK) {
    return TCL_ERROR;
  }
#else
  if (Tcl_PkgProvide(interp, PACKAGE_NAME, TIFFTCL_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
#endif

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------------
 *
 * Tifftcl_SafeInit --
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

int
Tifftcl_SafeInit (interp)
      Tcl_Interp *interp; /* Interpreter to initialise. */
{
    return Tifftcl_Init(interp);
}

/*
 *----------------------------------------------------------------------------
 *
 * Tifftcl_XXX --
 *
 *  Wrappers around the zlib functionality.
 *
 * Results:
 *  Depends on function.
 *
 * Side effects:
 *  Depends on function.
 *
 *----------------------------------------------------------------------------
 */

/*
 * No wrappers are required. Due to intelligent definition of the stub
 * table using the function names of the libz sources the stub table
 * contains jumps to the actual functionality.
 */
