#ifndef __FDPUBLIC_H

#include "FDGEN.H"
#include "FDTYPES.H"

/* external declarations for public variables */


extern HWND hwndDiagBox;                                    /* erase MDI ! */
extern HWND hwndFDesk;                          /* filter designer desktop */
extern HACCEL haccMainMenu;                       /* accelerators resource */
extern HWND hwndStatus;
extern HWND hwndPosition;           /* coordinate of mouse cursor in curve */
extern HWND hwndFilterCoeff;       /* handle of filter coefficients window */
extern HWND hwndFilterRoots;              /* handle of filter roots window */
extern tFilter MainFilter;                            /* the target filter */
extern tDiagDesc Diags[];                       /* description of diagrams */
extern tUnitDesc FrequencyUnits[];                                /* units */
extern tUnitDesc TimeUnits[];


/* user inst. options and parameters */
extern COLORREF alInstColor[];                   /* all installable colors */

extern int anPrec[];                  /* all installable output precisions */
#define IOUTPRECDLG_ROOTS        0
#define IOUTPRECDLG_COEFFS       1
#define IOUTPRECDLG_FREQU        2            /* frequency in dialog boxes */
#define IOUTPRECDLG_GAIN         3
#define IOUTPRECDLG_ATTENUATION  4
#define IOUTPRECDLG_OTHER        5


extern double aMathLimits[];
#define IMATHLIMIT_NULLCOEFF     0
#define IMATHLIMIT_NULLROOT      1
#define IMATHLIMIT_ERRROOTS      2
#define IMATHLIMIT_ERRSI         3
#define IMATHLIMIT_ERRBESSEL     4
#define IMATHLIMIT_ERRELLIPTIC   5
#define IMATHLIMIT_ERRJACOBISN   6
#define IMATHLIMIT_ERRKAISER     7
#define IMATHLIMIT_ERRPHASE      8


extern unsigned uDeskOpt;                              /* desktop options */
extern int cxyBmpSamples;       /* width/height of discret points in diag */
extern int cxyBmpRoots;    /* width/height of roots bitmap in root dialog */
extern int nWidthCurvePen;       /* widt of curve pen in continuous diags */
extern char szPrjFileName[];


#define __FDPUBLIC_H
#endif
