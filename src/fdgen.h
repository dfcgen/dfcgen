#ifndef __FDGEN_H

#define WINVER                 0x030A
#define DEBUG                  FALSE
#define LIB   /* version management static functions (no DLL) -> see VER.H */
#define STRICT
#define GENBORSTYLE            TRUE  /* generate with Borland style dialog */
#define GENFPEOPT              FALSE    /* user defined FPE error handling */

/* #define LANG_ENGLISH           1 */
#define LANG_GERMAN         1

#include <WINDOWSX.H>   /* includes #define <windows.h> + message crackers */
#include <VER.H>


#define __FDGEN_H
#endif
