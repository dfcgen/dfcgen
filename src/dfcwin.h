/* Digital Filter Coefficients Generator for Windows 3.1X 
 *                             
 * Author  : Ralf Hoppe
 
 * Compile Options:
           a) Model: LARGE
           b) Float Calculation: Emulation
           c) Prolog/Epilog: explicite functions exportable
           d) Calling conventions: C

 * Compile Hints:
           a) Do not use the option: AUTOMATIC FAR DATA, since this
              generates code that is errorneous if more than one instance
              of DFCGEN runs  
 *         
 */

#ifndef __DFCWIN_H

#include "FDGEN.H"       /* generating control */

#if GENBORSTYLE
#include "BWCC.H"       /* Borland DLL (BWCC.DLL) prototypes & defines */
                        /* Do not use old version from BCW/TCW 3.1 */

/* 'BWCCDefWindowProc' is the default function for Borland dialogs internal
 * uses shaded Borland dialog style background for window background
 * do use only for self defined user controls inserted in Borland dialogs
 */
#define FDWINDEFWINDOWPROC  BWCCDefWindowProc

#define FDWINMESSAGEBOX     BWCCMessageBox

#else                       /* windows standard styles */

#define FDWINDEFWINDOWPROC  DefWindowProc
#define FDWINMESSAGEBOX     MessageBox
#endif


#include <stdlib.h>     /* supports exit values (i.e. EXIT_FAILURE ... ) */
#include <stdio.h>
#include <string.h>
#include "DFCTLS.H"
#include "DFCDLG.H"
#include "FDTYPES.H"
#include "FDPUBLIC.H"
#include "FDERROR.H"
#include "FDHELP.H"
#include "DIAGS.H"
#include "FDMATH.H"
#include "FDUTIL.H"
#include "FDSYS.H"



/* Windows compatibility defines and castings */
#define MIN_WINDOWS_VERSION     0x03U           /* running with 3.1 upward */
#define MIN_WINDOWS_RELEASE     0x0AU

#define DUMMY_WPARAM            ((WPARAM) 0)
#define DUMMY_LPARAM            ((LPARAM) 0)


#define WM_UPDATEMENU           (WM_USER+64)     /* update MDI menu items */
#define UPDATE_MENU()  PostMessage(hwndFDesk, WM_UPDATEMENU, DUMMY_WPARAM, DUMMY_LPARAM)



/* Macro's */
#define GET_WM_SCROLL_POS(wParam, lParam)       (int)LOWORD(lParam)
#define GET_WM_COMMAND_IDCTL(wParam, lParam)    (UINT)wParam
#define GET_WM_COMMAND_NOTIFY(wParam, lParam)   (UINT)HIWORD(lParam)
#define GET_WM_COMMAND_HWNDCTL(wParam, lParam)  (HWND)LOWORD(lParam)

#define OFS_DIAG_DATA           0  /* offset of diag data pointer in window extra memory */
#define GETPDIAG(hwnd)          ((tDiag *)(LPSTR) GetWindowLong(hwnd, \
                                    OFS_DIAG_DATA))
#define GETDIAGTYPE(hwnd)       (tFdDiags) GetWindowWord(hwnd, \
                                    OFS_DIAG_DATA +sizeof(LONG))
#define GETPAINTRECT(hwnd, pos) (int)GetWindowWord(hwnd, \
                                    OFS_DIAG_DATA+sizeof(LONG)+sizeof(WORD) \
                                    +(pos)*sizeof(int))
#define SETPDIAG(hwnd, p)       SetWindowLong(hwnd, OFS_DIAG_DATA, \
                                    (LONG)(LPSTR)(p))
#define SETDIAGTYPE(hwnd, type) SetWindowWord(hwnd, OFS_DIAG_DATA \
                                    +sizeof(LONG), (WORD)(type))
#define SETPAINTRECT(hwnd, pos, val)    SetWindowWord(hwnd, OFS_DIAG_DATA \
                                            +sizeof(LONG)+sizeof(WORD) \
                                            +(pos)*sizeof(int), (WORD)(val))



/* prototypes of import/export functions (public) */

/* window or dialog callback functions */
LRESULT CALLBACK FDDeskWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GeneralDiagProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AxisNoteDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ImpWinDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FTransformDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OptAxisXDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OptAxisYDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OptMathDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK OptDeskDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK MathFuncSelectProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK CoeffDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ModifyCoeffDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FactorCoeffDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK NormCoeffDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK RootsDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AbortDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ProjectDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK HelpAboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK HelpLicenseDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK NoLicenseStartupDlgProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam);


/* from FDFLTDLG.C */
BOOL FilterDefinitionDlg(HWND, tFltDlg, BOOL);



/* digital filter design functions from */
int  DefineLinFirFilter(tFilter *, tFilter *);
int  DefineStdIIRFilter(HWND, tFilter *, tFilter *);
int  SetPredefFilter(HINSTANCE, tFilter *, tFilter *);
int DefineMiscDigSys(tFilter *pSrcFlt, tFilter *pDestFlt);


void DisplayNewFilterData(void);
BOOL SystemStabil(tFilter *);

BOOL CheckImplementation(tFilter *);
BOOL RoundCoefficients(PolyDat *);
BOOL ShiftPolyCoeffsDown(PolyDat *);
BOOL MultPolynomUp(PolyDat *, double, unsigned);
BOOL NormGainTo(PolyDat *, tFilter *, double, double);
BOOL MallocFilter(tFilter *);
void FreeFilter(tFilter *);


/* from FDDIAG.C */
void UpdateMDIMenuItems(void);
void UpdateDiags(tFdDiags);
void FreeDiag(tDiag *);
HWND CreateDiagWin(DIAGCREATESTRUCT *dg);



/* user input bound limits */
#define MAXORDER     (0xFFFF/sizeof(struct complex))  /* max. 64k segment allocation */

/* clipping bounds in internal calculations */
#define GRDELAY_INVALID_MAG     (1.0E-14)    /* no correct calc possible ! */
#define PHASE_INVALID_MAG       (1.0E-14)
#define MIN_TIME_RESP           WORLD_MIN                  /* from DIAGS.H */
#define MAX_TIME_RESP           WORLD_MAX
#define MIN_TIME_SEC            0.0         /* time is beginning from zero */
#define MAX_TIME_SEC            WORLD_MAX
#define MIN_F_HZ                0.0           /* frequency starts at zero */
#define MAX_F_HZ                WORLD_MAX
#define MIN_MAGNITUDE           0.0
#define MAX_MAGNITUDE           WORLD_MAX
#define MIN_GROUPDELAY          (-1.0E14)
#define MAX_GROUPDELAY          (-MIN_GROUPDELAY)
#define MIN_PHASEDELAY          0.0            /* all values are positive */
#define MAX_PHASEDELAY          WORLD_MAX

/* calculating precisions (error limits)
 * all definitions shouldn't exceed MIN_MATHLIMIT and (2*DBL_EPSILON)
 */
#define DEFAULT_NULLCOEFF       (1.0E-11) /* set coeff to null less than */
#define DEFAULT_NULLROOT        (1.0E-11) /* display root as zero less than */
#define DEFAULT_ERRROOTS        (1.0E-12) /* calculating bound of roots */
#define DEFAULT_ERRSI           (1.0E-5)  /* iteration error limit Si(x) */
#define DEFAULT_ERRJACOBISN     (1.0E-4)  /* iteration error limit sn(x) */
#define DEFAULT_ERRBESSEL       (1.0E-5)  /* iteration error limit I(x) */
#define DEFAULT_ERRELLIPTIC     (1.0E-5)  /* integration error limit F(k, x) */
#define DEFAULT_ERRKAISER       (1.0E-5)  /* Kaiser window function */
#define DEFAULT_ERRPHASE        (1.0E-5)  /* phase if calc by integration */

/***** dialog input limits */

/* input bounds for calculating precisions 
 * definitions shouldn't exceed (2*DBL_EPSILON)
 */
#define MIN_MATHLIMIT           (4.0*DBL_EPSILON)
#define MAX_MATHLIMIT           (0.01)

/* basic definitions */
#define MIN_INTEGR_STEPS_INPUT 50
#define MAX_INTEGR_STEPS_INPUT MAXINT
#define MIN_CUTOFF_INPUT       ((double)MINFLOAT) /* min. cutoff frequency inp */
#define MIN_SAMPLE_INPUT       ((double)MINFLOAT) /* min. sampling frequency */
#define MIN_CENTER_INPUT       ((double)MINFLOAT) /* min. center frequency inp */
#define MIN_COEFF_INPUT        (-(double)MAXFLOAT)
#define MAX_COEFF_INPUT        ((double)MAXFLOAT)
#define KAISER_ALPHA_MIN       2.0
#define KAISER_ALPHA_MAX       10.0
#define MIN_ATTENUAT_INPUT     (20.0*FLT_MIN_10_EXP)
#define MAX_ATTENUAT_INPUT     (20.0*FLT_MAX_10_EXP)

/* next three defines correponds to the rc-script file definition
   for IIR filter popup dialog */
#define MIN_MODANGLE_INPUT     1   /* sin(1) = 0.0175 min. module angle for Cauer */
#define MAX_MODANGLE_INPUT     80  /* sin(80) = 0.985 max. module angle for Cauer */
#define DEFAULT_MODANGLE       45

/* derived definitions */
#define MIN_RIPPLE_INPUT       (1.0/MAX_ATTENUAT_INPUT)     /* 0dB */
#define MAX_RIPPLE_INPUT       (10*log10(2.0))            /* < 3dB */
#define MIN_STOPBAND_ATT_INPUT (10*log10(2.0))            /* > 3dB */
#define MAX_STOPBAND_ATT_INPUT MAX_ATTENUAT_INPUT


/* precisions at sprintf() outputs */
#define MIN_PRECISION          1
#define MAX_PRECISION          DBL_DIG /* greather values not possible */
#define DEFAULT_PRECISION      6


/* diag axis input bounds */
#define MAX_F_INPUT            (1.0e12)   /* max. input for frequency axis */
#define MIN_F_INPUT            MIN_F_HZ   /* min. input for frequency axis */
#define MIN_T_INPUT            MIN_TIME_SEC    /* min. input for time axis */
#define MAX_T_INPUT            MAX_TIME_SEC    /* max. input for time axis */
#define MIN_FRESP_INPUT        0.0         /* min. frequency response axis */
#define MAX_FRESP_INPUT        (+1.0e20)   /* max. frequency response axis */
#define MIN_APPROXCHARAC_INPUT MIN_FRESP_INPUT
#define MAX_APPROXCHARAC_INPUT MAX_FRESP_INPUT
#define MIN_IRESP_INPUT        MIN_TIME_RESP
#define MAX_IRESP_INPUT        MAX_TIME_RESP
#define MIN_STEPRESP_INPUT     MIN_TIME_RESP
#define MAX_STEPRESP_INPUT     MAX_TIME_RESP
#define MIN_PHASE_INPUT        (-180.0*MAXORDER)
#define MAX_PHASE_INPUT        (180.0*MAXORDER)
#define MIN_DELAY_INPUT        (-1.0e20)
#define MAX_DELAY_INPUT        (MAXORDER/MIN_SAMPLE_INPUT)
#define MIN_GROUPDELAY_INPUT   MIN_GROUPDELAY
#define MAX_GROUPDELAY_INPUT   MAX_GROUPDELAY

#define MAXMATHFUNCX_INPUT     1000.0
#define MINMATHFUNCX_INPUT     -(MAXMATHFUNCX_INPUT)

/* generating defines */
#define DEFAULT_F_START         0.0
#define DEFAULT_F_END           1.0E3  /* 1 kHz */
#define DEFAULT_TIME_START      0.0
#define DEFAULT_TIME_END        1.0    /* 1 sec. */

/* desktop options (user install) in var 'uDeskOpt' */
#define DOPT_PHASE_INTEGRATE    0x01  /* calcs the phase by integration */

#define DOPT_PHASE360           0x02  /* shows the phase at 0 ... 360 deg */
                                      /* else from -180 to 180 (standard) */
#define DOPT_PHASE180           0x00
#define DOPT_PHASE_MASK         0x03  /* mask of two phase bits */

#define DOPT_SHOWCOEFF          0x10
#define DOPT_SHOWROOTS          0x20
#define DOPT_IGNFPE             0x80  /* ignore FPE errors */


/* desktop options */
#define DEFAULT_CXYBMP_SAMPLES  8
#define MAX_CXYBMP_SAMPLES      64
#define MIN_CXYBMP_SAMPLES      2
#define DEFAULT_CXYBMP_ROOTS    6
#define MAX_CXYBMP_ROOTS        64
#define MIN_CXYBMP_ROOTS        2
#define MAX_WIDTHCURVE          64
#define MIN_WIDTHCURVE          1
#define DEFAULT_WIDTHCURVE      1


/* default colors */
#define DEF_COLOR_SCALE_VALS   BLACK         /* scale strings in diagrams */
#define DEF_COLOR_SCALE_LINES  BLACK         /* dashed scale lines in diagrams */
#define DEF_COLOR_CURVE        BLACK         /* curve in diagrams */
#define DEF_COLOR_DIAG_FRAME   BLACK         /* frame of diag */
#define DEF_COLOR_DIAG_NOTES   BLACK         /* comments in diag */
#define DEF_COLOR_DIAG_NOTE_FRAME BLACK      /* comment frame */
#define DEF_COLOR_AXIS_NAME    BLACK         /* axis name and unit */


/* file I/O */
#define FILTER_FILE_EXT         "dfc"
#define INI_FILE_EXT            "ini"

#define IOAPPKEY_APPLICATION    "Application"
#define IOAPPKEY_BASIC          "BasicFilterData"
#define IOAPPKEY_NUMERATOR      "Numerator"
#define IOAPPKEY_DENOMINATOR    "Denominator"
#define IOAPPKEY_PROJECT        "Project"
#define IOAPPKEY_DESIGN         "Design"
#define IOAPPKEY_DESKTOP        "Desktop"
#define IOAPPKEY_DIAGS          "DiagWin"

#define IOKEY_APPNAME           "AppName"
#define IOKEY_VERSION           "Version"
#define IOKEY_SERNO             "SerNo"  /* from registration */
#define IOKEY_USRNAME           "User"
#define IOKEY_COMPANY           "Company"
#define IOKEY_SETUPCODE         "InstallCode"
#define IOKEY_PRJNAME           "Name"
#define IOKEY_PRJDESC           "Description"
#define IOKEY_PRJFLAGS          "Status"
#define IOKEY_TRANSFER_FACTOR   "Factor"
#define IOKEY_SAMPLING_FREQU    "SampleFrequency"
#define IOKEY_ORDER             "Order"
#define IOKEY_COEFF             "Coeff"
#define IOKEY_ROOT              "Root"
 
#define IOKEY_FTYPE             "FilterType"
#define IOKEY_DIALOG            "DialogData"
#define IOKEY_FTRANSFORM        "FrequTransformation"
#define IOKEY_DESKOPT           "Options"
#define IOKEY_CXY_SAMPLES       "SampleCxyPixel"
#define IOKEY_WIDTH_CURVE       "WidthCurvePixel"
#define IOKEY_CXY_ROOTS         "RootsCxyPixel"
#define IOKEY_COLORS            "Colors"
#define IOKEY_PRECISION         "Precisions"
#define IOKEY_MATHLIMITS        "MathLimits"
#define IOKEY_WIN_DESKTOP       "DesktopWin"
#define IOKEY_WIN_COEFFS        "CoeffsWin"
#define IOKEY_WIN_ROOTS         "RootsWin"
#define IOKEY_DIAGCOUNT         "NumOfDiag"

/* for all diags */
#define IOKEY_DIAGPOS           "Position"
#define IOKEY_DIAGTYPE          "Type"
#define IOKEY_DIAGSTYLE         "Style"
#define IOKEY_XPOINTS           "NumPoints"
#define IOKEY_NOTE_CNT          "NumComments"
#define IOKEY_NOTE              "Comment"
#define IOKEY_AXIS_X            "AxisX"
#define IOKEY_AXIS_Y            "AxisY"

#define __DFCWIN_H
#endif /* ifdef __DFCWIN_H */

