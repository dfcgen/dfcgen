#ifndef __FDGEN_H

#define WINVER                 0x030A
#define DEBUG                  0
#define LIB   /* version management static functions (no DLL) -> see VER.H */
#define STRICT
#define GENBORSTYLE            1  /* generate with Borland style dialog */
#define GENFPEOPT              0  /* user defined FPE error handling */

/* define FDLANG_ENGLISH       1 */
#define FDLANG_GERMAN          1

#include <WINDOWSX.H>   /* includes #define <windows.h> + message crackers */
#include <VER.H>


#define __FDGEN_H
#endif

