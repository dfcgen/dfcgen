/* DFCGEN Global Definitions and Support Functions (System)

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/fdsys.c,v $
 * $Revision: 1.3 $
 * $Date: 2000-08-17 12:45:28 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $

 */

#include <dir.h>                     /* definition of MAXPATH, MAXEXT, ... */
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "DFCWIN.H"
#include "FDREG.H"
#include "FDFLTRSP.H"


#define FORMAT_NOTEPOSX                 "%.*lG"

#define FORMAT_FILE_DOUBLE_READ         "%lG"
#define FORMAT_FILE_DOUBLE_WRITE        "%.15lG"  /* save with max. precision */
#define FORMAT_FILE_SHORT               "%hd"
#define FORMAT_FILE_UINT                "%04X"
#define FORMAT_FILE_INT                 "%d"
#define FORMAT_FILE_ROOT_WRITE          "%.15lG+j %.15lG"
#define FORMAT_FILE_ROOT_READ           "%lG+j%lG"
#define FORMAT_FILE_COLOR               "%08lX"

#define MAX_SAVE_WINDOWS    99

#define OPTDESK_MONCURVECOMMENT         "x=1.0"
#define OPTDESK_MONCURVECOMPOS          1.0   /* position of comment in world x */

#define SIZE_COMMENT                    1024 /* max. size of curve comment (includes \0 */


/* prototypes of local functions */

/* at first new getting functions for user def diag */
static BOOL fnUserSin(double *, double *, void *);
static BOOL fnUserCos(double *, double *, void *);
static BOOL fnUserTan(double *, double *, void *);
static BOOL fnUserCos2(double *, double *, void *);
static BOOL fnUserSqr(double *, double *, void *);
static BOOL fnUserSinh(double *, double *, void *);
static BOOL fnUserCosh(double *, double *, void *);
static BOOL fnUserTanh(double *, double *, void *);
static BOOL fnUserExp(double *, double *, void *);
static BOOL fnUserSi(double *, double *, void *);
static BOOL fnUserSi2(double *, double *, void *);
static BOOL fnUserHamming(double *, double *, void *);
static BOOL fnUserHanning(double *, double *, void *);
static BOOL fnUserBlackman(double *, double *, void *);
static BOOL fnUserBessel(double *, double *, void *);  /* Bessel function of zero order, first art */
static BOOL fnUserTriangle(double *, double *, void *);
static BOOL fnUserRectangle(double *, double *, void *);
static BOOL fnUserCheby(double *, double *, void *);
static BOOL fnUserComplElliptic1(double *x, double *y, void *pDummy);
static BOOL fnUserEllipticIntegr1(double *, double *, void *);
static BOOL fnUserJacobiSN(double *, double *, void *);
static BOOL fnUserJacobiCN(double *, double *, void *);
static BOOL fnUserJacobiDN(double *x, double *y, void *pModul);
static BOOL fnUserSineIntegr(double *x, double *y, void *pDummy);



static void UpdatePrjStat(HWND hDlg);
static BOOL SetMinMaxRange(HWND hwndDlg, int iAxisDesc, tDiagAxis *lpAxis,
                           BOOL bLogAxis);
static void PresetOptAxisCommonDlgItems(HWND, tDiagAxis *, int);
static void ShowRangeWins(HWND hwndDlg, BOOL bShow);
static pMarker InsertNewNote(HWND hwndDiag);
static void FillNoteDialogWin(HWND hDlg, tDiag *pDiag, pMarker pNote);
static BOOL FillNotePositionBox(HWND hDlg, tDiag *pDiag);
static void NextColorRect(HDC dc, int aCol[], RECT *pRect);
static void GotoNextPureColor(HDC dc, int aCol[], RECT *pRect, int Idx, int Direction, int Step, int ColMax);
static BOOL OptDeskGetSampleCurveY(double *x, double *y, void *pAppData);
static void DesktopCfgPrintf(FILE *f);
static void AppIdentPrintf(FILE *f);
static void AppKeyPrintf(FILE *f, char *szKey);
static void StringPrintf(FILE *f, char *szKey, char *szString);
static char *EnumKey(char *szKey, int Num);
static void PolyDataPrintf(FILE *f, PolyDat *p, BOOL bRootsValid);
static void AxisDataPrintf(FILE *f, char *szKey, tDiagAxis *pAxis);
static void RectPosPrintf(FILE *f, RECT *p);
static void WindowPosPrintf(FILE *f, char *szWinKey, HWND win);
static void AppKeyScanf(FILE *f, char *szKey);
static void KeyScanf(FILE *f, char *szKey);
static void ScanToNextTerm(FILE *f);
static void StringScanf(char szBuf[], int SizeBuf, FILE *f);
static void fdscanf(FILE *f, char *format, int nArgs, ...);
static BOOL PolyDataScanf(FILE *f, PolyDat *p, BOOL bScanRoots);
static void KeyDoubleScanf(FILE *f, char *szKey, double *d);
static void KeyUnsignedScanf(FILE *f, char *szKey, unsigned *u);
static void KeyIntScanf(FILE *f, char *szKey, int *i);
static BOOL BoolFlagScanf(FILE *f, char *szKey);
static void AxisDataScanf(FILE *f, char *szKey, tDiagAxis *pAxis);
static void RectPosScanf(FILE *f, RECT *p);
static void WindowPosScanf(FILE *f, char *szWinKey, WINDOWPLACEMENT *wpl);
static BOOL DesktopCfgScanf(FILE *f);
static int AppIdentScanf(FILE *f, char *szSerNo, char *szUser,
                         char *szCompany, unsigned *pInstCode);
static int ReadPrjFile(FILE *f, BOOL bLoadProject);





/* public variables */

tFilter MainFilter =                  /* definition of current dig. filter */
{
    NOTDEF,
    LINFIRDLG,
    /* use double brackets because union initialization !!! */
    {{0, RECT_LP, RECT_WIN, 0.0, 0.0}},                 /* definition data */
    1,                   /* input frequency unit index (mHz, Hz, kHz, ...) */
    { NOFTR, 1, 0, 0.0, 0.0 },            /* frequency transformation data */
    /* ----------------- * end of dialog definition part in this structure */
    {0, NULL, NULL},               /* numerator polynomial data (z-Domain) */
    {0, NULL, NULL},               /* nominator polynomial data (z-Domain) */
    1.0,                     /* transfer function multiplicator (not used) */
    0.0,                                             /* sampling frequency */
    "",                    /* short description of project, project number */
    NULL,                  /* user filter description (malloc mem space !) */
    0                                                             /* flags */
};

HWND hwndFDesk;                              /* main window with main menu */
HACCEL haccMainMenu;                              /* accelerators resource */
HWND hwndDiagBox;                             /* diagram box window handle */
HWND hwndStatus;                                          /* status window */
HWND hwndPosition;                  /* coordinate of mouse cursor in curve */
HWND hwndFilterCoeff = HWND_NULL;            /* filter coefficients window */
HWND hwndFilterRoots = HWND_NULL;                   /* filter roots window */


/* desktop variables/options */
int cxyBmpSamples = DEFAULT_CXYBMP_SAMPLES; /* extension of bitmap in discret diagrams (1 to 64) */
int cxyBmpRoots = DEFAULT_CXYBMP_ROOTS;
int nWidthCurvePen = DEFAULT_WIDTHCURVE;

/*-------------------*/

COLORREF alInstColor[SIZE_COLOR_ARR] =     /* all installable colors */
{
    DEF_COLOR_SCALE_VALS,         /* 0 : scale numbers in diagrams */
    DEF_COLOR_SCALE_LINES,        /* 1 : scale lines in diagrams */
    DEF_COLOR_CURVE,              /* 2 : curve in diagrams */
    DEF_COLOR_DIAG_FRAME,         /* 3 : frame of diag */
    DEF_COLOR_DIAG_NOTES,         /* 4 : note text in diag */
    DEF_COLOR_DIAG_NOTE_FRAME,    /* 5 : frame of note in diag */
    DEF_COLOR_AXIS_NAME,          /* 6 : axis name and unit */
};


unsigned int uDeskOpt = 0;

/* other defaults */
char szPrjFileName[MAXPATH] = "";

/* user installable output precisions */
int anPrec[] =
{
    DEFAULT_PRECISION, /* 0 : roots output precision in roots list box (note: that's not
                              the calculating precision !) */
    DEFAULT_PRECISION, /* 1 : any coeff output precision */
    DEFAULT_PRECISION, /* 2 : all dialog frequency values (sampling, cutoff, bandwidth) */
    DEFAULT_PRECISION, /* 3 : gain output in dialog windows */
    DEFAULT_PRECISION, /* 4 : ripple/min. attenuation for elliptic filters */
    DEFAULT_PRECISION  /* 5 : other params (Kaiser Alpha, module of Cauer-Filters etc.) */
};


double aMathLimits[] =
{
    DEFAULT_NULLCOEFF,   /* 0 (IMATHLIMIT_NULLCOEFF) : set coeff to null less than */
    DEFAULT_NULLROOT,    /* 1 (IMATHLIMIT_NULLROOT) : display root as zero */
    DEFAULT_ERRROOTS,    /* 2 (IMATHLIMIT_ERR_ROOTS) : error calculating roots of polynopmials */
    DEFAULT_ERRSI,       /* 3 (IMATHLIMIT_ERRSI) : sine integral */
    DEFAULT_ERRBESSEL,   /* 4 (IMATHLIMIT_ERRBESSEL) : Bessel function of first kind */
    DEFAULT_ERRELLIPTIC, /* 5 (IMATHLIMIT_ERRELLIPTIC) : elliptic integral */
    DEFAULT_ERRJACOBISN, /* 6 (IMATHLIMIT_ERRJACOBISN) : Jacobian function SN(x) */
    DEFAULT_ERRKAISER,   /* 7 (IMATHLIMIT_ERRKAISER) : Kaiser window function */
    DEFAULT_ERRPHASE     /* 8 (IMATHLIMIT_ERRPHASE) : phase error if calc by integration */
};


tUnitDesc FrequencyUnits[] =
{
    {"mHz", 1.0e-3},
    {"Hz", 1.0},
    {"kHz", 1.0e3},
    {"MHz", 1.0e6},
    {"GHz", 1.0e9},
    {NULL, 0.0}
};


tUnitDesc TimeUnits[] =
{
    {"ps", 1.0e-12},
    {"ns", 1.0e-9},
    {"µs", 1.0e-6},
    {"ms", 1.0e-3},
    {"s", 1.0},
    {NULL, 0.0}
};




/* locale typedefs */

typedef enum                                    /* types of user functions */
{
    USER_FN_PARAMETER_INT,                            /* has int parameter */
    USER_FN_PARAMETER_DBL,                         /* has double parameter */
} tUserFnParam;

/* user function table in correspondence with the names table */
typedef struct
{
    char *MinVal;
    char *MaxVal;
    int IdParamName;
    tUserFnParam Type;
} tUserFnParamDesc;

typedef struct {
    FUNCGETY FnGetY;
    tUserFnParamDesc *ParamDesc;
} tUserFnDesc;



/******************* locale variables *******************/


static COLORREF const aDefaultColors[SIZE_COLOR_ARR] =     /* all default colors */
{
    DEF_COLOR_SCALE_VALS,         /* 0 : scale numbers in diagrams */
    DEF_COLOR_SCALE_LINES,        /* 1 : scale lines in diagrams */
    DEF_COLOR_CURVE,              /* 2 : curve in diagrams */
    DEF_COLOR_DIAG_FRAME,         /* 3 : frame of diag */
    DEF_COLOR_DIAG_NOTES,         /* 4 : note text in diag */
    DEF_COLOR_DIAG_NOTE_FRAME,    /* 5 : frame of note in diag */
    DEF_COLOR_AXIS_NAME,          /* 6 : axis name and unit */
};



/* axis data to use in diagram option dialog */
static tAxisDesc AxisDesc[] =
{
    {STRING_AXISNAME_FREQUENCY, MIN_F_INPUT, MAX_F_INPUT},
    {STRING_AXISNAME_TIME, MIN_T_INPUT, MAX_T_INPUT},
    {STRING_AXISNAME_X, MINMATHFUNCX_INPUT, MAXMATHFUNCX_INPUT},
    {STRING_AXISNAME_MAGNITUDE, MIN_FRESP_INPUT, MAX_FRESP_INPUT},
    {STRING_AXISNAME_ATTENUATION, MIN_ATTENUAT_INPUT, MAX_ATTENUAT_INPUT},
    {STRING_AXISNAME_IMPULS_RESP, MIN_IRESP_INPUT, MAX_IRESP_INPUT},
    {STRING_AXISNAME_STEP_RESP, MIN_STEPRESP_INPUT, MAX_STEPRESP_INPUT},
    {STRING_AXISNAME_PHASE, MIN_PHASE_INPUT, MAX_PHASE_INPUT},
    {STRING_AXISNAME_PHASEDELAY, MIN_DELAY_INPUT, MAX_DELAY_INPUT},
    {STRING_AXISNAME_GROUPDELAY, MIN_GROUPDELAY_INPUT, MAX_GROUPDELAY_INPUT},
    {STRING_AXISNAME_APPROXCHARAC, MIN_APPROXCHARAC_INPUT, MAX_APPROXCHARAC_INPUT},
    {STRING_AXISNAME_Y, WORLD_MIN, WORLD_MAX}
};



static tUserFnParamDesc FuncParamDesc[] =
{
    {"1", "64", STRING_PARAM_ORDER, USER_FN_PARAMETER_INT},
    {"0.0", "0.9999", STRING_PARAM_ELLIPTIC_MODUL, USER_FN_PARAMETER_DBL},
};

static tUserFnDesc aUserFn[] =
{                             /* first functuion should'nt have parameters */
    { fnUserSin,       NULL},
    { fnUserCos,       NULL},
    { fnUserTan,       NULL},
    { fnUserCos2,      NULL},
    { fnUserSqr,       NULL},
    { fnUserExp,       NULL},
    { fnUserSinh,      NULL},
    { fnUserCosh,      NULL},
    { fnUserTanh,      NULL},
    { fnUserSi,        NULL},
    { fnUserSi2,       NULL},
    { fnUserHamming,   NULL},
    { fnUserHanning,   NULL},
    { fnUserBlackman,  NULL},
    { fnUserTriangle,  NULL},
    { fnUserRectangle, NULL},
    { fnUserSineIntegr,NULL},
    { fnUserBessel,    &FuncParamDesc[0]},
    { fnUserCheby,     &FuncParamDesc[0]},
    { fnUserComplElliptic1, NULL},
    { fnUserEllipticIntegr1, &FuncParamDesc[1]},
    { fnUserJacobiSN, &FuncParamDesc[1]},
    { fnUserJacobiCN, &FuncParamDesc[1]},
    { fnUserJacobiDN, &FuncParamDesc[1]}
};

/* in correspondence to user function table */
static int anUserFnNames[] =
{
    STRING_USERFN_SIN,
    STRING_USERFN_COS,
    STRING_USERFN_TAN,
    STRING_USERFN_COS2,
    STRING_USERFN_SQR,
    STRING_USERFN_EXP,
    STRING_USERFN_SINH,
    STRING_USERFN_COSH,
    STRING_USERFN_TANH,
    STRING_USERFN_SI,
    STRING_USERFN_SI2,
    STRING_USERFN_HAMMING,
    STRING_USERFN_HANNING,
    STRING_USERFN_BLACKMAN,
    STRING_USERFN_TRIANGLE,
    STRING_USERFN_RECTANGLE,
    STRING_USERFN_INTEGRSIN,
    STRING_USERFN_BESSEL,
    STRING_USERFN_CHEBY,
    STRING_USERFN_COMPL_ELLIPTIC1,
    STRING_USERFN_ELLIPTIC_INTEGR1,
    STRING_USERFN_JACOBI_SN,
    STRING_USERFN_JACOBI_CN,
    STRING_USERFN_JACOBI_DN
};



static tUnitDesc FMagnitudeLogUnits[] =
{
    {"mdB", 1.0e-3},
    {"dB", 1.0},
    {NULL, 0.0}
};



static tUnitDesc PhaseUnits[] =
{
    {"°", 1.0},
    {NULL, 0.0}
};




tDiagDesc Diags[NO_OF_DIAGS] = /* description of all diag types (public !) */
{
    {                                           /* frequency response diag */
        AXIS_FREQUENCY,
        AXIS_MAGNITUDE,
        STRING_FRESP_DIAG,
        MAKEINTRESOURCE(IDICO_DIAG),
        {
            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, DEFAULT_WIDTHCURVE, 0, NULL,
            (FUNCGETYINIT) NULL, FrequencyResponse, (FUNCGETYEND) NULL,
            {
                AXIS_DOTLINES, DEFAULT_F_START, DEFAULT_F_END, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, FrequencyUnits, (short) 1,
                STRING_AXIS_FREQUENCY
            },
            {
                AXIS_DOTLINES, 0.0, 2.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, NULL, (short) 0, STRING_AXIS_MAGNITUDE
            }
        }
    },

    {
        AXIS_FREQUENCY, AXIS_ATTENUATION, STRING_ATTENUATION_DIAG, MAKEINTRESOURCE(IDICO_ATT),
        {
            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, DEFAULT_WIDTHCURVE, 0, NULL,
            NULL, Attenuation, NULL,
            {
                AXIS_DOTLINES, DEFAULT_F_START, DEFAULT_F_END, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, FrequencyUnits, (short) 1,
                STRING_AXIS_FREQUENCY
            },
            {
                AXIS_DOTLINES, -20.0, 20.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, FMagnitudeLogUnits, (short) 1,
                STRING_AXIS_ATTENUATION
            }
        }
    },

    {                                               /* phase response diag */
        AXIS_FREQUENCY, AXIS_PHASE, STRING_PHASE_DIAG, MAKEINTRESOURCE(IDICO_PHASE),
        {
            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, DEFAULT_WIDTHCURVE, 0, NULL,
            PhaseRespInit, PhaseResponse, PhaseRespEnd,
            {
                AXIS_DOTLINES, DEFAULT_F_START, DEFAULT_F_END, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, FrequencyUnits, (short) 1,
                STRING_AXIS_FREQUENCY
            },
            {
                AXIS_DOTLINES, 0.0, 360.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, PhaseUnits, (short) 0, STRING_AXIS_PHASE
            },
        }
    },

    {                                                  /* phase delay diag */
        AXIS_FREQUENCY, AXIS_PHASEDELAY, STRING_PHASEDELAY_DIAG, MAKEINTRESOURCE(IDICO_DELAY),
        {
            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, DEFAULT_WIDTHCURVE, 0, NULL,
            PhaseDelayInit, PhaseDelay, PhaseDelayEnd,
            {
                AXIS_DOTLINES, DEFAULT_F_START, DEFAULT_F_END, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, FrequencyUnits, (short) 1,
                STRING_AXIS_FREQUENCY
            },
            {
                AXIS_DOTLINES, 0.0, 1.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, TimeUnits, (short) 4,
                STRING_AXIS_PHASEDELAY
            }
        }
    },

    {                                                 /* group delay diag */
        AXIS_FREQUENCY, AXIS_GROUPDELAY, STRING_GROUPDELAY_DIAG, MAKEINTRESOURCE(IDICO_GRP),
        {
            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, DEFAULT_WIDTHCURVE, 0, NULL,
            NULL, GroupDelay, NULL,
            {
                AXIS_DOTLINES, DEFAULT_F_START, DEFAULT_F_END, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, FrequencyUnits, (short) 1,
                STRING_AXIS_FREQUENCY
            },
            {
                AXIS_DOTLINES, 0.0, 1.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, TimeUnits, (short) 4,
                STRING_AXIS_GROUPDELAY
            }
        }
    },

    {                                             /* impulse response diag */
        AXIS_TIME, AXIS_IMPULS_RESP, STRING_IRESP_DIAG, MAKEINTRESOURCE(IDICO_IMPULS),
        {
            HRGN_NULL, (DIAG_YAUTORANGE | DIAG_DISCRET), NULL,
            MAKEINTRESOURCE(IDBMP_POINT), DEFAULT_CXYBMP_SAMPLES, 0, NULL,
            ImpulseRespInit, TimeResponse, TimeRespEnd,
            {
                AXIS_DOTLINES, DEFAULT_TIME_START, DEFAULT_TIME_END, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, TimeUnits, (short) 4, STRING_AXIS_TIME
            },
            {
                AXIS_DOTLINES, 0.0, 1.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, NULL, (short) 0, STRING_AXIS_IMPULS_RESP
            }
        }
    },

    {                                                /* step response diag */
        AXIS_TIME, AXIS_STEP_RESP, STRING_STEPRESP_DIAG, MAKEINTRESOURCE(IDICO_STEP),
        {
            HRGN_NULL, (DIAG_YAUTORANGE | DIAG_DISCRET), NULL,
            MAKEINTRESOURCE(IDBMP_POINT), DEFAULT_CXYBMP_SAMPLES, 0, NULL,
            StepRespInit, TimeResponse, TimeRespEnd,
            {
                AXIS_DOTLINES, DEFAULT_TIME_START, DEFAULT_TIME_END, 0.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, TimeUnits, (short) 4, STRING_AXIS_TIME
            },
            {
                AXIS_DOTLINES, 0.0, 1.0, 0.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, NULL, (short) 0, STRING_AXIS_STEP_RESP
            }
        }
    },

    {
        AXIS_FREQUENCY, AXIS_APPROXCHARAC, STRING_APPROXRESP_DIAG,
        MAKEINTRESOURCE(IDICO_APPROX),
        {
            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, DEFAULT_WIDTHCURVE, 0, NULL,
            (FUNCGETYINIT) NULL, ApproxCharacResponse, (FUNCGETYEND) NULL,
            {
                AXIS_DOTLINES, DEFAULT_F_START, DEFAULT_F_END, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, FrequencyUnits, (short) 1,
                STRING_AXIS_FREQUENCY
            },
            {
                AXIS_DOTLINES, 0.0, 2.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, NULL, (short) 0, STRING_AXIS_APPROXCHARAC
            }
        }
    },


    {                                      /* user selected math. function */
        AXIS_X, AXIS_Y, STRING_USERDEF_DIAG, MAKEINTRESOURCE(IDICO_MATH),
        {
            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, DEFAULT_WIDTHCURVE, 0, NULL,
            NULL, fnUserSin, NULL,
            {
                AXIS_DOTLINES, -1.0, 1.0, 1.0, (COORDINATE)0, (COORDINATE)639,
                (short) DEFAULT_PRECISION, NULL, (short) 1, STRING_AXIS_X
            },
            {
                AXIS_DOTLINES, -1.0, 1.0, 1.0, (COORDINATE)0, (COORDINATE)479,
                (short) DEFAULT_PRECISION, NULL, (short) 1, STRING_AXIS_Y
            }
        }
    }
};





/*********************** IMPLEMENTATION ***********************************/


void UpdateMainWinTitle()
{
    char szBuf[256] = {'\0'};

    (void)GetRCVerStringInfo("InternalName", szBuf, DIM(szBuf)); /* app name */

    if (MainFilter.f_type != NOTDEF)      /* filter defined (exist) ? */
    {
        char *pEnd = szBuf+lstrlen(szBuf);

        if (lstrlen(MainFilter.szPrjName) > 0)    /* cat project name */
            wsprintf(pEnd, "  \xBB%s\xAB", MainFilter.szPrjName);
        else
            wsprintf(pEnd, "  %s", szPrjFileName);  /* file path/name */
    } /* if */

    SetWindowText(hwndFDesk, szBuf);
} /* UpdateMainWinTitle */



/* project dialog utility (updates status text in IDD_PROJECT_STATUS) */

static void UpdatePrjStat(HWND hDlg)
{
    static struct tStrStatus
    {
        unsigned *pFlags;
        unsigned Mask;
        int nIdSetStr;
        int nIdClrStr;
    } PrjStat[] =
    {
        {&MainFilter.uFlags, FILTER_TYPE_INVALID, STRING_STATUS_DEF_CHANGED, 0},
        {&MainFilter.uFlags, FILTER_SAVED, STRING_STATUS_SAVED, STRING_STATUS_NOT_SAVED}
    };

    int nIdStr, i;
    char szStatus[256];
    char szOpt[128];
    struct tStrStatus *pStat;

    szStatus[0] = '\0';
    for (i=0, pStat=PrjStat; i<DIM(PrjStat); i++, pStat++)
    {
        nIdStr = (*pStat->pFlags & pStat->Mask) ? pStat->nIdSetStr : pStat->nIdClrStr;
        if (nIdStr != 0)
        {
            if (szStatus[0] != '\0') (void)lstrcat(szStatus, ", ");
            LoadString(GetWindowInstance(hDlg), nIdStr, szOpt, DIM(szOpt));
            (void)lstrcat(szStatus, szOpt);
        } /* if */
    } /* for */

    SetDlgItemText(hDlg, IDD_PROJECT_STATUS, szStatus);
} /* UpdatePrjStat() */




#pragma argsused  /* disable "Parameter is never used" Warning */
BOOL CALLBACK ProjectDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG :
            UpdatePrjStat(hDlg);
            SetDlgItemText(hDlg, IDD_PROJECT_FILE, szPrjFileName);
            SetDlgItemText(hDlg, IDD_PROJECT_NAME, MainFilter.szPrjName);

            SendDlgItemMessage(hDlg, IDD_PROJECT_DESC, EM_FMTLINES,
                               FALSE, DUMMY_LPARAM);
            SendDlgItemMessage(hDlg, IDD_PROJECT_DESC, EM_LIMITTEXT,
                               SIZE_PRJDESC-1, DUMMY_LPARAM);
            SendDlgItemMessage(hDlg, IDD_PROJECT_NAME, EM_LIMITTEXT,
                               SIZE_PRJNAME-1, DUMMY_LPARAM);

            if (MainFilter.szPrjDesc != NULL)
                SetDlgItemText(hDlg, IDD_PROJECT_DESC, MainFilter.szPrjDesc);
	        return TRUE; /* WM_INITDIALOG */


	    case WM_COMMAND :
        {
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_PROJECT_DELETE :
                    SetDlgItemText(hDlg, IDD_PROJECT_NAME, "");
                    SetDlgItemText(hDlg, IDD_PROJECT_DESC, "");
                    SetDlgFocus(hDlg, IDD_PROJECT_NAME);
                    return TRUE;

//                case IDD_PROJECT_DESC :
//                case IDD_PROJECT_NAME :
//                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_CHANGE)
//                        UpdatePrjStat(hDlg);
//                    break;


                case IDOK :
                {
                    int nText = (int)       /* type cast from long to int */
                        SendDlgItemMessage(hDlg, IDD_PROJECT_DESC,
                                           WM_GETTEXTLENGTH, DUMMY_WPARAM,
                                           DUMMY_LPARAM);
                                                         
                    if (MainFilter.szPrjDesc != NULL) FREE((void *)MainFilter.szPrjDesc);
                    if (nText > 0)
                    {
                        MainFilter.szPrjDesc = (char *)MallocErr(hDlg, (nText+1)*sizeof(char));
                        if (MainFilter.szPrjDesc != NULL)
                            GetDlgItemText(hDlg, IDD_PROJECT_DESC,
                                           MainFilter.szPrjDesc, nText+1);
                    } /* if */
                    else MainFilter.szPrjDesc = NULL;

                    GetDlgItemText(hDlg, IDD_PROJECT_NAME, MainFilter.szPrjName,
                                   DIM(MainFilter.szPrjName));

                    MainFilter.uFlags &= ~FILTER_SAVED;
                    UpdateMainWinTitle();
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                } /* IDOK */

                case IDCANCEL :
                    EndDialog(hDlg, FALSE);
                    return TRUE;

                case IDHELP :
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpProjectInfoDlg);
                    return TRUE;

            } /* switch */
		} /* case */
    } /* switch */
    return FALSE;
}



static BOOL SetMinMaxRange(HWND hwndDlg, int iAxisDesc, tDiagAxis *lpAxis,
                           BOOL bLogAxis)
{
    double dMinVal, dMaxVal, dDummy;

    if (!CheckDlgItemFloat(hwndDlg, IDD_OPTAXISDLG_FROM,
                           IDD_OPTAXISDLG_FROM_TEXT,
                           AxisDesc[iAxisDesc].MinLimitInput,
                           AxisDesc[iAxisDesc].MaxLimitInput,
                           &dMinVal)) return FALSE;

    if (!CheckDlgItemFloat(hwndDlg, IDD_OPTAXISDLG_TO,
                           IDD_OPTAXISDLG_TO_TEXT, dMinVal,
                           AxisDesc[iAxisDesc].MaxLimitInput,
                           &dMaxVal)) return FALSE;

    if (bLogAxis)
        if (!CheckDlgItemFloat(hwndDlg, IDD_OPTAXISDLG_FROM,
                               IDD_OPTAXISDLG_FROM_TEXT,
                               WORLD_MIN_LOG, dMaxVal,
                               &dDummy)) return FALSE;

    lpAxis->dWorldMin = dMinVal;
    lpAxis->dWorldMax = dMaxVal;
    if (lpAxis->lpUnitList != NULL)
    {
        double dFactor =
            lpAxis->lpUnitList[GetDlgListCurSel(hwndDlg, IDD_OPTAXISDLG_UNITBOX)].dUnitFactor;
        lpAxis->dWorldMin *= dFactor;
        lpAxis->dWorldMax *= dFactor;
    } /* if */

    return TRUE;
}



static void PresetOptAxisCommonDlgItems(HWND hOptAxisDlg, tDiagAxis *pAxis,
                                        int iAxisDesc)
{
    HWND hwndMDIChild;
    char szClassName[40];

    SetWindowTextId(hOptAxisDlg, AxisDesc[iAxisDesc].nIddAxisName);
    SetDlgItemInt(hOptAxisDlg, IDD_OPTAXISDLG_PRECISION,
                  (int)pAxis->sPrecision, TRUE);

    /* show min/max range with current precision */

    SetDlgItemFloatUnit(hOptAxisDlg, IDD_OPTAXISDLG_FROM, pAxis->dWorldMin,
                        (int)pAxis->sPrecision, pAxis->iCurrUnit,
                        pAxis->lpUnitList);
    SetDlgItemFloatUnit(hOptAxisDlg, IDD_OPTAXISDLG_TO, pAxis->dWorldMax,
                        (int)pAxis->sPrecision, pAxis->iCurrUnit,
                        pAxis->lpUnitList);

    CheckDlgButton(hOptAxisDlg, IDD_OPTAXISDLG_LOG, pAxis->AxisFlags & AXIS_LOG);
    CheckDlgButton(hOptAxisDlg, IDD_OPTAXISDLG_DOTLINES, pAxis->AxisFlags & AXIS_DOTLINES);

    DrawComboAxisUnits(hOptAxisDlg, IDD_OPTAXISDLG_UNITBOX,
                       IDD_OPTAXISDLG_UNIT_TEXT, pAxis->iCurrUnit,
                       pAxis->lpUnitList);

    EnableDlgItem(hOptAxisDlg, IDD_OPTAXISDLG_SETALL, FALSE);

    hwndMDIChild = GetFirstChild(hwndDiagBox);
    while (hwndMDIChild)
    {
        GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
        if (!lstrcmp(szClassName, DIAG_CLASS))
        {
            if (((Diags[GETDIAGTYPE(hwndMDIChild)].nIdAxisX == iAxisDesc) ||
                 (Diags[GETDIAGTYPE(hwndMDIChild)].nIdAxisY == iAxisDesc)) &&
                (pAxis != &GETPDIAG(hwndMDIChild)->X) && /* check not the own */
                (pAxis != &GETPDIAG(hwndMDIChild)->Y))
            {
                EnableDlgItem(hOptAxisDlg, IDD_OPTAXISDLG_SETALL, TRUE);
                break;                                    /* any found */
            } /* if */
        } /* if */

        hwndMDIChild = GetNextSibling(hwndMDIChild);       /* next */
    } /* while */
} /* PresetOptAxisCommonDlgItems */





/* passed parameter in LPARAM is :
   HWND of active MDI-Child, which is related to this diag
 */

BOOL CALLBACK OptAxisXDlgProc(HWND hwndOptWin,
                              UINT msg,         /* message */
                              WPARAM wParam,
                              LPARAM lParam)
{
    static HWND hwndDiag;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            tDiag *pDiag;              /* pointer to activ MDI-child diag */
            hwndDiag = (HWND)lParam;
            pDiag = GETPDIAG(hwndDiag);

            PresetOptAxisCommonDlgItems(hwndOptWin, &pDiag->X,
                                        Diags[GETDIAGTYPE(hwndDiag)].nIdAxisX);

            EnableDlgItem(hwndOptWin, IDD_OPTAXISDLG_CONST_SAMPLES, FALSE);

            if (pDiag->nDiagOpt & DIAG_DISCRET)
            {
                pDiag->nDiagOpt &= ~DIAG_CONST_SAMPLES; /* not possible ! */
                EnableDlgItem(hwndOptWin, IDD_OPTAXISDLG_CONST_BTN, FALSE);
            } /* if */

            if (pDiag->nDiagOpt & DIAG_CONST_SAMPLES)
            {
                SetDlgItemInt(hwndOptWin, IDD_OPTAXISDLG_CONST_SAMPLES,
                              pDiag->nConstSmpl, TRUE);
                CheckDlgButton(hwndOptWin, IDD_OPTAXISDLG_CONST_BTN, TRUE);
                EnableDlgItem(hwndOptWin, IDD_OPTAXISDLG_CONST_SAMPLES, TRUE);
            } /* if */

	        return TRUE;
        } /* WM_INIT_DIALOG */

	    case WM_COMMAND :
        {
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_OPTAXISDLG_CONST_BTN :
                    if (IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_CONST_BTN))
                    {
                        EnableDlgItem(hwndOptWin,
                                      IDD_OPTAXISDLG_CONST_SAMPLES, TRUE);
                        SetDlgFocus(hwndOptWin, IDD_OPTAXISDLG_CONST_SAMPLES);
                    } /* if */
                    else EnableDlgItem(hwndOptWin,
                                       IDD_OPTAXISDLG_CONST_SAMPLES, FALSE);
                    return TRUE;


                case IDD_OPTAXISDLG_PRECISION:
                    if (ChangeIncDecInt(hwndOptWin, wParam, lParam,
                                        MIN_PRECISION, MAX_PRECISION,
                                        DEFAULT_PRECISION)) return TRUE;
                    break; /* IDD_OPTAXISDLG_PRECISION */


                case IDOK :
                {
                    int nPrecision, nSamples;
                    char szClassName[40];
                    BOOL bLog = IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_LOG);
                    BOOL bConst = IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_CONST_BTN);
                    tDiag *pDiag = GETPDIAG(hwndDiag);
                    tDiagAxis *pAxis = &pDiag->X;
                    int iAxisDesc = Diags[GETDIAGTYPE(hwndDiag)].nIdAxisX;

                    if (bConst)
                        if (!CheckDlgItemInt(hwndOptWin, IDD_OPTAXISDLG_CONST_SAMPLES,
                                             IDD_OPTAXISDLG_CONST_BTN,
                                             2, MAXINT, &nSamples))
                            return TRUE;

                    if (!CheckDlgItemInt(hwndOptWin, IDD_OPTAXISDLG_PRECISION,
                                         IDD_OPTAXISDLG_PREC_TEXT,
                                         MIN_PRECISION, MAX_PRECISION,
                                         &nPrecision))
                        return TRUE;

                    if (!SetMinMaxRange(hwndOptWin, iAxisDesc, pAxis, bLog))
                        return TRUE;

                    /* all numeric inputs ok up to this point, update data
                       structures */
                    UPDATEFLAG(pDiag->nDiagOpt, DIAG_CONST_SAMPLES, bConst);
                    if (bConst) pDiag->nConstSmpl = nSamples;

                    UPDATEFLAG(pAxis->AxisFlags, AXIS_LOG, bLog);
                    UPDATEFLAG(pAxis->AxisFlags, AXIS_DOTLINES,
                               IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_DOTLINES));
                    pAxis->sPrecision = nPrecision;

                    pAxis->iCurrUnit = GetDlgListCurSel(hwndOptWin, IDD_OPTAXISDLG_UNITBOX);

                    InvalidateRect(hwndDiag, NULL, TRUE);

                    if (IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_SETALL))
                    {
                        HWND hwndMDIChild = GetFirstChild(hwndDiagBox);
                        while (hwndMDIChild)
                        {
                            GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
                            if (!lstrcmp(szClassName, DIAG_CLASS))
                            {
                                tDiag *pDiag = GETPDIAG(hwndMDIChild);

                                if (Diags[GETDIAGTYPE(hwndMDIChild)].nIdAxisX
                                    == iAxisDesc)                /* found */
                                {
                                    pDiag->X = *pAxis;  /* copy axis desc */
                                    UPDATEFLAG(pDiag->nDiagOpt, DIAG_CONST_SAMPLES, bConst);
                                    if (bConst)
                                        pDiag->nConstSmpl = nSamples;

                                    InvalidateRect(hwndMDIChild, NULL, TRUE);
                                } /* if */

                                if (Diags[GETDIAGTYPE(hwndMDIChild)].nIdAxisY
                                    == iAxisDesc)                /* found */
                                {
                                    pDiag->Y = *pAxis;     /* copy y-axis */
                                    InvalidateRect(hwndMDIChild, NULL, TRUE);
                                } /* if */
                            } /* if */
                            hwndMDIChild = GetNextSibling(hwndMDIChild);
                        } /* while */
                    } /* if */

                    EndDialog(hwndOptWin, TRUE);
                    return TRUE;
                } /* case IDOK */

                case IDCANCEL :
                    EndDialog(hwndOptWin, FALSE);
                    return TRUE;

                case IDHELP:
                    WinHelp(hwndOptWin, HELP_FNAME, HELP_CONTEXT, HelpOptAxisX);
                    return TRUE;
            } /* switch */
        } /* case WM_COMMAND */
    } /* switch */
    return FALSE;
}



static void ShowRangeWins(HWND hwndDlg, BOOL bShow)
{
    EnableDlgItem(hwndDlg, IDD_OPTAXISDLG_TO, bShow);
    EnableDlgItem(hwndDlg, IDD_OPTAXISDLG_FROM, bShow);
    EnableDlgItem(hwndDlg, IDD_OPTAXISDLG_TO_TEXT, bShow);
    EnableDlgItem(hwndDlg, IDD_OPTAXISDLG_FROM_TEXT, bShow);
}




/* passed parameter in LPARAM is :
     HWND of aktiv MDI-Child, which related to a diag
 */

BOOL CALLBACK OptAxisYDlgProc(HWND hwndOptWin,
                              UINT msg,         /* message */
                              WPARAM wParam,
                              LPARAM lParam)
{
    static HWND hwndDiag;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            tDiag *pDiag;      /* pointer to activ MDI-child diag */
            hwndDiag = (HWND)lParam;
            pDiag = GETPDIAG(hwndDiag);

            PresetOptAxisCommonDlgItems(hwndOptWin, &pDiag->Y,
                                        Diags[GETDIAGTYPE(hwndDiag)].nIdAxisY);

            if (pDiag->nDiagOpt & DIAG_YAUTORANGE)
            {
                CheckDlgButton(hwndOptWin, IDD_OPTAXISDLG_AUTOSCALE, TRUE);
                ShowRangeWins(hwndOptWin, FALSE);
                SetDlgFocus(hwndOptWin, IDD_OPTAXISDLG_PRECISION);
                return FALSE;            /* have had set the focus */
            } /* if */
	        return TRUE;
        } /* WM_INIT_DIALOG */


	    case WM_COMMAND :
        {
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_OPTAXISDLG_AUTOSCALE :
                    if (IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_AUTOSCALE))
                        ShowRangeWins(hwndOptWin, FALSE);
                    else
                    {
                        ShowRangeWins(hwndOptWin, TRUE);
                        SetDlgFocus(hwndOptWin, IDD_OPTAXISDLG_FROM);
                    }
                    return TRUE;


                case IDD_OPTAXISDLG_PRECISION:
                    if (ChangeIncDecInt(hwndOptWin, wParam, lParam,
                                        MIN_PRECISION, MAX_PRECISION,
                                        DEFAULT_PRECISION)) return TRUE;
                    break; /* IDD_OPTAXISDLG_PRECISION */


               case IDOK :
                {
                    int nPrecision;
                    char szClassName[40];
                    tDiag *pDiag = GETPDIAG(hwndDiag);
                    tDiagAxis *pAxis = &pDiag->Y;
                    int iAxisDesc = Diags[GETDIAGTYPE(hwndDiag)].nIdAxisY;
                    BOOL bLog = IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_LOG);

                    if (!CheckDlgItemInt(hwndOptWin, IDD_OPTAXISDLG_PRECISION,
                                         IDD_OPTAXISDLG_PREC_TEXT,
                                         MIN_PRECISION, MAX_PRECISION,
                                         &nPrecision))
                        return TRUE;

                    if (!IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_AUTOSCALE))
                        if (!SetMinMaxRange(hwndOptWin, iAxisDesc, pAxis, bLog))
                            return TRUE;

                    UPDATEFLAG(pDiag->nDiagOpt, DIAG_YAUTORANGE,
                               IsDlgButtonChecked(hwndOptWin,
                                                  IDD_OPTAXISDLG_AUTOSCALE));

                    UPDATEFLAG(pAxis->AxisFlags, AXIS_LOG, bLog);
                    UPDATEFLAG(pAxis->AxisFlags, AXIS_DOTLINES,
                               IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_DOTLINES));
                    pAxis->sPrecision = nPrecision;

                    pAxis->iCurrUnit = GetDlgListCurSel(hwndOptWin, IDD_OPTAXISDLG_UNITBOX);

                    InvalidateRect(hwndDiag, NULL, TRUE);

                    if (IsDlgButtonChecked(hwndOptWin, IDD_OPTAXISDLG_SETALL))
                    {
                        HWND hwndMDIChild = GetFirstChild(hwndDiagBox);
                        while (hwndMDIChild)
                        {
                            GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
                            if (!lstrcmp(szClassName, DIAG_CLASS))
                            {
                                if (Diags[GETDIAGTYPE(hwndMDIChild)].nIdAxisX
                                    == iAxisDesc)                 /* found */
                                {
                                    GETPDIAG(hwndMDIChild)->X = *pAxis;
                                    InvalidateRect(hwndMDIChild, NULL, TRUE);
                                } /* if */

                                if (Diags[GETDIAGTYPE(hwndMDIChild)].nIdAxisY
                                    == iAxisDesc)                 /* found */
                                {
                                    GETPDIAG(hwndMDIChild)->Y = *pAxis;
                                    UPDATEFLAG(GETPDIAG(hwndMDIChild)->nDiagOpt,
                                               DIAG_YAUTORANGE,
                                               pDiag->nDiagOpt & DIAG_YAUTORANGE);

                                    InvalidateRect(hwndMDIChild, NULL, TRUE);
                                } /* if */
                            } /* if */

                            hwndMDIChild = GetNextSibling(hwndMDIChild);
                        } /* while */
                    } /* if */

                    EndDialog(hwndOptWin, TRUE);
                    return TRUE;
                } /* case IDOK */

                case IDCANCEL :
                    EndDialog(hwndOptWin, FALSE);
                    return TRUE;

                case IDHELP:
                    WinHelp(hwndOptWin, HELP_FNAME, HELP_CONTEXT, HelpOptAxisY);
                    return TRUE;
            } /* switch */
        } /* case WM_COMMAND */
    } /* switch */

    return FALSE;
}



/********************** axis note dialog section **************************/

static tRadioBtnDesc aNotePosHorz[] =
{
    {MARKER_LEFT, IDD_NOTE_HPOS_LEFT},
    {MARKER_RIGHT, IDD_NOTE_HPOS_RIGHT},
    {MARKER_HCENTER, IDD_NOTE_HPOS_CENTER},
    {0, 0}           /* last number */
};

static tRadioBtnDesc aNotePosVert[] =
{
    {MARKER_TOP, IDD_NOTE_VPOS_TOP},
    {MARKER_BOTTOM, IDD_NOTE_VPOS_BOTTOM},
    {MARKER_VCENTER, IDD_NOTE_VPOS_CENTER},
    {0, 0}           /* last flag */
};



/* if pNote is NULL there is no comment to edit, all dialog items will be
   cleared then (or set to their default values)
 */
static void FillNoteDialogWin(HWND hDlg, tDiag *pDiag, pMarker pNote)
{
    double YWorld;
    unsigned uOpt = MARKER_DEFAULT;

    if (pNote != NULL) uOpt = pNote->uOpt;

    CheckSelRadioBtn(hDlg, uOpt & MARKER_HPOS_MASK, aNotePosHorz);
    CheckSelRadioBtn(hDlg, uOpt & MARKER_VPOS_MASK, aNotePosVert);

    CheckDlgButton(hDlg, IDD_NOTE_OPT_HIDE, uOpt & MARKER_HIDE);
    CheckDlgButton(hDlg, IDD_NOTE_OPT_FRAME, uOpt & MARKER_FRAME);
    CheckDlgButton(hDlg, IDD_NOTE_OPT_OPAQUE, uOpt & MARKER_OPAQUE);
    CheckDlgButton(hDlg, IDD_NOTE_OPT_POSAUTO, uOpt & MARKER_POSAUTO);

    SetDlgItemText(hDlg, IDD_NOTE, "");
    SetDlgItemText(hDlg, IDD_NOTE_POSY_TEXT, "");

    if (pNote == NULL) SetDlgItemText(hDlg, IDD_NOTEBOX_POSX, "");
    else
    {
        if (pNote->szTxt != NULL)                       /* exist a text ? */
            SetDlgItemText(hDlg, IDD_NOTE, pNote->szTxt);

        if (GetWorldYByX(pDiag, &pNote->x, 0, &YWorld)) /* exist pt */
            SetDlgItemFloat(hDlg, IDD_NOTE_POSY_TEXT,
                            YWorld/GetScaleFactor(&pDiag->Y),
                            pDiag->Y.sPrecision);

        SetDlgItemFloatUnit(hDlg, IDD_NOTEBOX_POSX, pNote->x,
                            pDiag->X.sPrecision,
                            GetDlgListCurSel(hDlg, IDD_NOTEPOSX_UNITBOX),
                            pDiag->X.lpUnitList);
    } /* else */
} /* FillNoteDialogWin */


/* returns FALSE at mem space allocation errors
 */
static BOOL FillNotePositionBox(HWND hDlg, tDiag *pDiag)
{
    pMarker pN = pDiag->pM;  /* pointer to first note i.e. comment */

    SendDlgItemMessage(hDlg, IDD_NOTEBOX_POSX, CB_RESETCONTENT,
                       DUMMY_WPARAM, DUMMY_LPARAM);

    while (pN != NULL)
    {                         /* fill combo box with x coordinates */
        char szCursorPos[80];
        short iUnit = pDiag->X.iCurrUnit;  /* save current setting */

        pDiag->X.iCurrUnit = /* can be CB_ERR, but isn't a problem */
            GetDlgListCurSel(hDlg, IDD_NOTEPOSX_UNITBOX);

        sprintf(szCursorPos, FORMAT_NOTEPOSX, pDiag->X.sPrecision,
                pN->x/GetScaleFactor(&pDiag->X));

        pDiag->X.iCurrUnit = iUnit;           /* restore old value */

        if (SendDlgItemMessage(hDlg, IDD_NOTEBOX_POSX, CB_ADDSTRING,
                               DUMMY_WPARAM,        /* add to list */
                               (LPARAM)(LPSTR)szCursorPos) 
            < 0)
        {
            ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
            EndDialog(hDlg, FALSE);
            return FALSE;
        } /* if */

        pN = pN->pNextMarker;                  /* ptr to next note */
    } /* while */

    return TRUE;
} /* FillNotePositionBox() */


/* mallocs memory space for a new comment in diag
 * inserts this new comment in note chain
 * returns pointer to this new comment (equals to pM) or NULL if memory error
 */
static pMarker InsertNewNote(HWND hwndDiag)
{
    TEXTMETRIC tm;
    HDC dc;
    pMarker pNewNote, pFirstNote;
    tDiag *pDiag = GETPDIAG(hwndDiag);

    pNewNote = MALLOC(sizeof(tMarker));
    if (pNewNote == NULL) return NULL;               /* not enough memory */

    pFirstNote = pDiag->pM;        /* ptr to first comment struct or NULL */
    pDiag->pM = pNewNote;                           /* insert new comment */
    pNewNote->pNextMarker = pFirstNote;
    pNewNote->szTxt = NULL;                  /* no text assigned/malloced */

    dc = GetDC(hwndDiag);
    GetTextMetrics(dc, &tm);
    ReleaseDC(hwndDiag, dc);

    /* set default start size of text rect to 10 chars/line and two lines */
    SetRectEmpty(&pNewNote->rcNote);                        /* clear rect */
    pNewNote->rcNote.right = 10*tm.tmAveCharWidth;
    pNewNote->rcNote.bottom = 2*(NOTE_BORDER_Y+tm.tmHeight+tm.tmExternalLeading+1);
    pNewNote->uOpt = 0;                          /* reset all option bits */
    pNewNote->hrgnHotSpot = HRGN_NULL;
    return pNewNote;
} /* InsertNewNote() */



/* lParam is a pointer to the dialog create struct 'CREATEAXISNOTEDLGSTRUCT'
 * note : GetParent() for a dialog box returns the desktop window handle
 */

BOOL CALLBACK AxisNoteDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                              LPARAM lParam)
{
    static pMarker pNote; /* pointer to note struct (NULL if not in list) */
    static HWND hwndDiag;
    static tDiag *pDiag;

    switch(msg)
    {
        case WM_INITDIALOG :
        {
            tDiagAxis *pAxisX;
            pMarker pN;             /* pointer to first note i.e. comment */
            double x = ((CREATEAXISNOTEDLGSTRUCT *)lParam)->PosX;
            int i = 0;

            hwndDiag = ((CREATEAXISNOTEDLGSTRUCT *)lParam)->hwndDiag;
            pDiag = GETPDIAG(hwndDiag);
            pN = pDiag->pM;
            pAxisX = &pDiag->X;

            SendDlgItemMessage(hDlg, IDD_NOTE, EM_FMTLINES, FALSE, DUMMY_LPARAM);
            SendDlgItemMessage(hDlg, IDD_NOTE, EM_LIMITTEXT, SIZE_COMMENT-1, DUMMY_LPARAM);

            DrawComboAxisUnits(hDlg, IDD_NOTEPOSX_UNITBOX,
                               IDD_NOTEPOSX_UNIT_TEXT, pAxisX->iCurrUnit,
                               pAxisX->lpUnitList);

            if (!FillNotePositionBox(hDlg, pDiag)) return TRUE; /* if Memory Error */

            /* check passed note coordinate match with defined notes
               inside marker chain
             */
            pNote = NULL;                        /* no correspondent note */

            while (pN != NULL)
            {                         
                if ((x < pAxisX->dWorldMax) && (x > pAxisX->dWorldMin))
                {
                    if (abs(GetDevXY(pAxisX, x) - GetDevXY(pAxisX, pN->x)) <=
                        CY_HOTSPOT(pDiag)/2) /* correspondence note found */
                    {
                        PostMessage(GetDlgItem(hDlg, IDD_NOTEBOX_POSX),
                                    CB_SETCURSEL, i, DUMMY_LPARAM);
                        pNote = pN;       /* set pointer to selected note */
                    } /* if */
                } /* if */

                ++i;                              /* next combo box index */
                pN = pN->pNextMarker;                 /* ptr to next note */
            } /* while */

            SetDlgItemTextId(hDlg, IDD_NOTE_AXISNAME_X, pAxisX->nIdAxisName);
            SetDlgItemTextId(hDlg, IDD_NOTE_AXISNAME_Y, pDiag->Y.nIdAxisName);

            FillNoteDialogWin(hDlg, pDiag, pNote);
                         
            if (pNote == NULL)                       /* unknown comment ? */
                SetDlgItemFloatUnit(hDlg, IDD_NOTEBOX_POSX, x,
                                    pAxisX->sPrecision, pAxisX->iCurrUnit,
                                    pAxisX->lpUnitList);
            return TRUE;
        } /* WM_INITDIALOG */

        case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_NOTEBOX_POSX :
                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == CBN_SELCHANGE)
                    {
                        int iNote = GetDlgListCurSel(hDlg, IDD_NOTEBOX_POSX);

                        if (iNote != CB_ERR)    /* if no comment selected */
                        {
                            int i = 0;
                            pMarker pN = pDiag->pM;

                            pNote = NULL; 

                            while (pN != NULL)
                            {          /* search pointer to selected note */
                                if (i++ == iNote) /* next list item index */
                                {
                                    pNote = pN;
                                    FillNoteDialogWin(hDlg, pDiag, pN);
                                    return TRUE;
                                } /* if */

                                pN = pN->pNextMarker; /* ptr to next note */
                            } /* while */
                        } /* if NOT CB_ERR */
                        return TRUE;
                    } /* if CBN_SELCHANGE */

                    break; /* IDD_NOTEBOX_POSX */


                case IDD_NOTEPOSX_UNITBOX :
                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == CBN_SELCHANGE)
                        if (!FillNotePositionBox(hDlg, pDiag)) return TRUE;
                    break; /* IDD_NOTEPOSX_UNITBOX */

                case IDCLOSE :                            /* Close button */
                case IDCANCEL :                             /* Escape Key */
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDHELP :
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpEditCommentsDlg);
                    return TRUE;

                case IDNEW :                    /* clear all dialog items */
                    pNote = NULL;
                    FillNoteDialogWin(hDlg, pDiag, NULL);
                    SetDlgFocus(hDlg, IDD_NOTEBOX_POSX);
                    return TRUE; /* IDNEW */

                case IDCLEAR :  /* delete a single note (ident by 'pNote') */
                    if (pNote != NULL)
                    {
                        tMarker FirstNoteDummy;
                        pMarker pN = &FirstNoteDummy;
                        int iNote = 0;

                        FirstNoteDummy.pNextMarker = pDiag->pM;

                        while (pN != pNote)
                        {
                            if (pN->pNextMarker == pNote)
                            {                    /* note found in chain ? */
                                pN->pNextMarker = pNote->pNextMarker;

                                if (pNote->szTxt != NULL) FREE(pNote->szTxt);
                                if (pNote->hrgnHotSpot != HRGN_NULL)
                                    DeleteRgn(pNote->hrgnHotSpot);
                                FREE(pNote);

                                SendDlgItemMessage(hDlg, IDD_NOTEBOX_POSX,
                                                   CB_DELETESTRING, iNote,
                                                   DUMMY_LPARAM);
                                pNote = NULL;
                                break; /* break while loop */
                            } /* if */

                            ++iNote;
                            pN = pN->pNextMarker;
                        } /* while */

                        pDiag->pM = FirstNoteDummy.pNextMarker;
                    } /* if */

                    FillNoteDialogWin(hDlg, pDiag, pNote);
                    InvalidateRect(hwndDiag, NULL, TRUE);
                    return TRUE; /* IDCLEAR */

                case IDOK :
                {
                    double XWorld, YWorld;
                    int nCount;

                    int iAxisDescX = Diags[GETDIAGTYPE(hwndDiag)].nIdAxisX;

                    if (!CheckDlgItemFloat(hDlg, IDD_NOTEBOX_POSX,
                                           IDD_NOTE_AXISNAME_X,
                                           AxisDesc[iAxisDescX].MinLimitInput,
                                           AxisDesc[iAxisDescX].MaxLimitInput,
                                           &XWorld))
                        return TRUE;

                    if (pDiag->X.lpUnitList != NULL)
                        XWorld *=
                            pDiag->X.lpUnitList
                            [
                                 GetDlgListCurSel(hDlg, IDD_NOTEPOSX_UNITBOX)
                            ].dUnitFactor;

                    if (!GetWorldYByX(pDiag, &XWorld, 0, &YWorld))
                    {                              /* exist curve point ? */
                        (void)HandleDlgError(hDlg, IDD_NOTEBOX_POSX, ERROR_NOTEATX_SING);
                        return TRUE;
                    } /* if */

                    if (pNote == NULL)   /* current note isn't in chain ? */
                    {
                        pNote = InsertNewNote(hwndDiag);    /* create new */
                        if (pNote == NULL)
                        {
                            ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
                            return TRUE;
                        } /* if */
                    } /* if */


                    if (pNote->szTxt != NULL) FREE(pNote->szTxt);

                    pNote->x = XWorld;                       /* set X position */

                    /* get note from dialog and store */
                    nCount = GetWindowTextLength(GetDlgItem(hDlg, IDD_NOTE));
                    pNote->szTxt = MallocErr(hDlg, (nCount+1)*sizeof(pNote->szTxt[0]));

                    /* update flags */
                    pNote->uOpt &= ~MARKER_POS_MASK; /* clear all bits */
                    pNote->uOpt |= GetSelRadioBtn(hDlg, aNotePosHorz);
                    pNote->uOpt |= GetSelRadioBtn(hDlg, aNotePosVert);

                    UPDATEFLAG(pNote->uOpt, MARKER_HIDE,
                               IsDlgButtonChecked(hDlg, IDD_NOTE_OPT_HIDE));

                    UPDATEFLAG(pNote->uOpt, MARKER_FRAME,
                               IsDlgButtonChecked(hDlg, IDD_NOTE_OPT_FRAME));

                    UPDATEFLAG(pNote->uOpt, MARKER_OPAQUE,
                               IsDlgButtonChecked(hDlg, IDD_NOTE_OPT_OPAQUE));

                    UPDATEFLAG(pNote->uOpt, MARKER_POSAUTO,
                               IsDlgButtonChecked(hDlg, IDD_NOTE_OPT_POSAUTO));

                    if (pNote->szTxt != NULL)
                        GetDlgItemText(hDlg, IDD_NOTE, pNote->szTxt, nCount+1);

                    if (FillNotePositionBox(hDlg, pDiag))
                    { 
                        FillNoteDialogWin(hDlg, pDiag, pNote);
                        InvalidateRect(hwndDiag, NULL, TRUE);
                    } /* if */

                    return TRUE;
                } /* IDOK */
            }
            break; /* WM_COMMAND */
    } /* switch */

    return FALSE;
} /* AxisNoteDlgProc */





/* lParam is the handle to math function diag window */
BOOL CALLBACK MathFuncSelectProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int iFunc;
    static tDiag *pDiag;
    static HWND hwndActivChild;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            hwndActivChild = (HWND)lParam;
            pDiag = GETPDIAG(hwndActivChild);

            /* search the math func description */
            for (iFunc = 0; (iFunc < DIM(aUserFn)) &&
                            (aUserFn[iFunc].FnGetY !=
                             GETPDIAG(hwndActivChild)->GetNextWorldY);
                 iFunc++);

            (void)DrawBoxIdStrings(hDlg, IDD_USERFN_BOX, iFunc,
                                   DIM(anUserFnNames), anUserFnNames);

            if (aUserFn[iFunc].ParamDesc != NULL)   /* with parameter ? */
            {
                ShowDlgItem(hDlg, IDD_USERFN_PARA, TRUE);
                ShowDlgItem(hDlg, IDD_USERFN_PARA_TEXT, TRUE);
                SetDlgItemTextId(hDlg, IDD_USERFN_PARA_TEXT,
                                 aUserFn[iFunc].ParamDesc->IdParamName);
                switch (aUserFn[iFunc].ParamDesc->Type)
                {
                    case USER_FN_PARAMETER_INT :
                        SetDlgItemInt(hDlg, IDD_USERFN_PARA,
                                      *(int *)(pDiag->pAppData), TRUE);
                        break;

                    case USER_FN_PARAMETER_DBL :
                        SetDlgItemFloat(hDlg, IDD_USERFN_PARA,
                                        *(double *)(pDiag->pAppData), 8);
                        break;
                } /* switch */
            } /* if */

            return TRUE;
        } /* WM_INITDIALOG */

	case WM_COMMAND :
        {
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_USERFN_BOX :
                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == CBN_SELCHANGE)
                    {
                        BOOL bParamExist;    /* functions with order etc. */

                        iFunc = GetDlgListCurSel(hDlg, IDD_USERFN_BOX);
                        bParamExist = aUserFn[iFunc].ParamDesc != NULL;
                        if (bParamExist)              /* with parameter ? */
                            SetDlgItemTextId(hDlg, IDD_USERFN_PARA_TEXT,
                                             aUserFn[iFunc].ParamDesc->IdParamName);

                        ShowDlgItem(hDlg, IDD_USERFN_PARA, bParamExist);
                        ShowDlgItem(hDlg, IDD_USERFN_PARA_TEXT, bParamExist);
                        return TRUE;
                    } /* if */
                    break;

                case IDOK :
                {
                    void *pData = NULL;

                    /* check such functions that have a parameter */
                    if (aUserFn[iFunc].ParamDesc != NULL)   /* with parameter ? */
                    {
                        tUserFnParamDesc *pParam = aUserFn[iFunc].ParamDesc;

                        switch (pParam->Type)
                        {
                            case USER_FN_PARAMETER_INT :
                                pData = MallocErr(hDlg, sizeof(int));
                                if (pData == NULL) return TRUE;
                                if (!CheckDlgItemInt(hDlg, IDD_USERFN_PARA,
                                                     IDD_USERFN_PARA_TEXT,
                                                     atoi(pParam->MinVal),
                                                     atoi(pParam->MaxVal),
                                                     (int *)(pData)))
                                {
                                    FREE(pData);   /* release mem if error */
                                    return TRUE;
                                } /* if */

                                break;

                            case USER_FN_PARAMETER_DBL :
                                pData = MallocErr(hDlg, sizeof(double));
                                if (pData == NULL) return TRUE;
                                if (!CheckDlgItemFloat(hDlg, IDD_USERFN_PARA,
                                                       IDD_USERFN_PARA_TEXT,
                                                       atof(pParam->MinVal),
                                                       atof(pParam->MaxVal),
                                                       (double *)(pData)))
                                {
                                    FREE(pData);   /* release mem if error */
                                    return TRUE;
                                } /* if */

                                break;
                         } /* switch */
                    } /* if */

                    if (pDiag->pAppData != NULL) FREE(pDiag->pAppData);      /* release memory */
                    pDiag->pAppData = pData;
                    pDiag->GetNextWorldY = aUserFn[iFunc].FnGetY;
                    InvalidateRect(hwndActivChild, NULL, TRUE); /* repaint */
                    EndDialog(hDlg, TRUE);
                    return TRUE;
                } /* IDOK */

                case IDCANCEL :
                    EndDialog(hDlg, FALSE);
                    return TRUE;

                case IDHELP:
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpOptMathFunc);
                    return TRUE;

            } /* switch */
            break;
        } /* case WM_COMMAND */
    } /* switch */
    return FALSE;
}



/* Tech. or Math. options dialog */


BOOL CALLBACK OptMathDlgProc(HWND hwndOpt,
                              UINT msg,         /* message */
                              WPARAM wParam,
                              LPARAM lParam)
{
    static struct
    {
        int nIddEdit;
        int nIddStaticName;
        int iMathLimitArr;           /* index inside aMathLimits var */
        double dDefault;
    } const aLimitDesc[DIM(aMathLimits)] = /* description of all precision edit fields */
    {
        {IDD_OPTTECH_NULLCOEFF, IDD_OPTTECH_NULLCOEFF_TEXT,
         IMATHLIMIT_NULLCOEFF, DEFAULT_NULLCOEFF},
        {IDD_OPTTECH_NULLROOT, IDD_OPTTECH_NULLROOT_TEXT,
         IMATHLIMIT_NULLROOT, DEFAULT_NULLROOT},
        {IDD_OPTTECH_ERRPHASE, IDD_OPTTECH_ERRPHASE_TEXT,
         IMATHLIMIT_ERRPHASE, DEFAULT_ERRPHASE},
        {IDD_OPTTECH_ERRROOTS, IDD_OPTTECH_ERRROOTS_TEXT,
         IMATHLIMIT_ERRROOTS, DEFAULT_ERRROOTS},
        {IDD_OPTTECH_ERRSI, IDD_OPTTECH_ERRSI_TEXT,
         IMATHLIMIT_ERRSI, DEFAULT_ERRSI},
        {IDD_OPTTECH_ERRBESSEL, IDD_OPTTECH_ERRBESSEL_TEXT,
         IMATHLIMIT_ERRBESSEL, DEFAULT_ERRBESSEL},
        {IDD_OPTTECH_ERRELLIPTIC, IDD_OPTTECH_ERRELLIPTIC_TEXT,
         IMATHLIMIT_ERRELLIPTIC, DEFAULT_ERRELLIPTIC},
        {IDD_OPTTECH_ERRJACOBISN, IDD_OPTTECH_ERRJACOBISN_TEXT,
         IMATHLIMIT_ERRJACOBISN, DEFAULT_ERRJACOBISN},
        {IDD_OPTTECH_ERRKAISER, IDD_OPTTECH_ERRKAISER_TEXT,
         IMATHLIMIT_ERRKAISER, DEFAULT_ERRKAISER}
    };


    int i;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            for (i = 0; i < DIM(aLimitDesc); i++)    /* set all */
                SetDlgItemFloat(hwndOpt, aLimitDesc[i].nIddEdit,
                                aMathLimits[aLimitDesc[i].iMathLimitArr],
                                MAX_PRECISION);

            #if GENFPEOPT
            ShowDlgItem(hwndOpt, IDD_OPTTECH_IGNFPE, GetWinFlags() & WF_80x87);
            CheckDlgButton(hwndOpt, IDD_OPTTECH_IGNFPE, uDeskOpt & DOPT_IGNFPE);
            #endif
            return TRUE;
        } /* WM_INITDIALOG */


	case WM_COMMAND :
        {
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDOK :
                {
                    double Val;

                    for (i = 0; i < DIM(aLimitDesc); i++)    /* check all */
                        if (!CheckDlgItemFloat(hwndOpt, aLimitDesc[i].nIddEdit,
                                             aLimitDesc[i].nIddStaticName,
                                             MIN_MATHLIMIT, MAX_MATHLIMIT,
                                             &Val)) return FALSE;

                    for (i = 0; i < DIM(aLimitDesc); i++)    /* get all */
                        (void)CheckDlgItemFloat(hwndOpt, aLimitDesc[i].nIddEdit,
                                              aLimitDesc[i].nIddStaticName,
                                              MIN_MATHLIMIT, MAX_MATHLIMIT,
                                              &aMathLimits[aLimitDesc[i].iMathLimitArr]);

                    #if GENFPEOPT
                    UPDATEFLAG(uDeskOpt, DOPT_IGNFPE,
                               IsDlgButtonChecked(hwndOpt, IDD_OPTTECH_IGNFPE));
                    InitErrorHandler();
                    #endif

                    EndDialog(hwndOpt, TRUE);
                    return TRUE;                                  /* IDOK */
                } /* IDOK */

                case IDCANCEL :
                    EndDialog(hwndOpt, FALSE);
                    return TRUE;


                case IDD_OPTTECH_SETSTD:        /* back to default values */
                    for (i = 0; i < DIM(aLimitDesc); i++)      /* set all */
                        SetDlgItemFloat(hwndOpt, aLimitDesc[i].nIddEdit,
                                        aLimitDesc[i].dDefault, MAX_PRECISION);
                    return TRUE; /* IDD_OPTTECH_SETSTD */


                case IDHELP :
                    WinHelp(hwndOpt, HELP_FNAME, HELP_CONTEXT, HelpOptMathDlg);
                    return TRUE; /* IDHELP */


                default :
                    for (i = 0; i < DIM(aLimitDesc); i++)    /* search */
                    {
                        if (wParam == aLimitDesc[i].nIddEdit)
                        {
                            if ((GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_MOUSE_DEC)
                                || (GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_MOUSE_INC))
                            {
                                double dVal;

                                /* wParam contains the ID of dialog element */
                                switch (GetDlgItemFloat(hwndOpt, wParam, MIN_MATHLIMIT,
                                                        MAX_MATHLIMIT, &dVal))
                                {
                                    case CVT_ERR_EMPTY : dVal = aLimitDesc[i].dDefault; break;

                                    case CVT_ERR_OVERFLOW : dVal = MAX_MATHLIMIT; break;

                                    case CVT_ERR_UNDERFLOW : dVal = MIN_MATHLIMIT; break;

                                    case CVT_ERR_SYNTAX : return TRUE; /* do nothing */

                                    case CVT_OK :

                                        switch (GET_WM_COMMAND_NOTIFY(wParam, lParam))
                                        {
                                            case EN_MOUSE_DEC:
                                                if ((dVal/=10.0) < MIN_MATHLIMIT) return TRUE;
                                                break;

                                            case EN_MOUSE_INC:
                                                if ((dVal*=10.0) > MAX_MATHLIMIT) return TRUE;
                                                break;
                                        } /* switch */

                                        break; /* CVT_OK */
                                } /* switch */

                                SetDlgItemFloat(hwndOpt, wParam, dVal, MAX_PRECISION);
                                return TRUE;
                            } /* if */

                            break; /* for loop */
                        } /* if */
                    } /* for */

                    break; /* default */

            } /* switch */
            break;
        } /* case WM_COMMAND */
    } /* switch */
    return FALSE;
}


#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL OptDeskGetSampleCurveY(double *x, double *y, void *pAppData)
{
    *y = exp(-fabs(*x/5))*cos(*x);
    return TRUE;
} /* OptDeskGetSampleCurveY() */



#define IDXRED    0
#define IDXBLUE   1
#define IDXGREEN  2



static void NextColorRect(HDC dc, int aCol[], RECT *pRect)
{
    int nHeight = pRect->bottom - pRect->top;

    COLORREF rgbColor = RGB((BYTE)min(255, aCol[IDXRED]),
                            (BYTE)min(255, aCol[IDXGREEN]),
                            (BYTE)min(255, aCol[IDXBLUE]));

    HBRUSH hbr = CreateSolidBrush(GetNearestColor(dc, rgbColor));
    FillRect(dc, pRect, hbr);
    DeleteBrush(hbr);
    OffsetRect(pRect, 0, nHeight);
} /* NextColorRect() */



static void GotoNextPureColor(HDC dc, int aCol[], RECT *pRect, int Idx, int Direction, int Step, int ColMax)
{
    do
    {
        NextColorRect(dc, aCol, pRect);
        aCol[Idx] += Step*Direction;
    } /* */
    while (aCol[Idx] % ColMax != 0);

} /* GotoNextPureColor() */


/* desktop options dialog function */
BOOL CALLBACK OptDeskDlgProc(HWND hwndOpt, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static struct
    {
        int nIddEdit;
        int nIddStaticName;
        int iPrec;           /* index inside anPrec var */
    } const aPrecDesc[] =     /* description of all precision edit fields */
    {
        {IDD_OPTDESK_OUTPREC_ROOT, IDD_OPTDESK_OUTPREC_ROOT_TEXT, IOUTPRECDLG_ROOTS},
        {IDD_OPTDESK_OUTPREC_COEFF, IDD_OPTDESK_OUTPREC_COEFF_TEXT, IOUTPRECDLG_COEFFS},
        {IDD_OPTDESK_OUTPREC_FREQU, IDD_OPTDESK_OUTPREC_FREQU_TEXT, IOUTPRECDLG_FREQU},
        {IDD_OPTDESK_OUTPREC_GAIN, IDD_OPTDESK_OUTPREC_GAIN_TEXT, IOUTPRECDLG_GAIN},
        {IDD_OPTDESK_OUTPREC_ATT, IDD_OPTDESK_OUTPREC_ATT_TEXT, IOUTPRECDLG_ATTENUATION},
        {IDD_OPTDESK_OUTPREC_OTHER, IDD_OPTDESK_OUTPREC_OTHER_TEXT, IOUTPRECDLG_OTHER}
    };


    static tRadioBtnDesc aBtnPhaseOpt[] =
    {
        {DOPT_PHASE180,  IDD_OPTDESK_PHASE180},
        {DOPT_PHASE360,  IDD_OPTDESK_PHASE360},
        {DOPT_PHASE_INTEGRATE, IDD_OPTDESK_PHASE_EXT},
        {0, 0}
    };

    static int IdColorName[DIM(alInstColor)] =
    {
        STRING_DIAGITEM_SCALEVALS,  /* 0 : scale numbers in diagrams */
        STRING_DIAGITEM_SCALELINES, /* 1 : scale lines in diagrams */
        STRING_DIAGITEM_CURVE,      /* 2 : curve in diagrams */
        STRING_DIAGITEM_FRAME,      /* 3 : frame of diag */
        STRING_DIAGITEM_NOTES,      /* 4 : note text in diag */
        STRING_DIAGITEM_NOTEFRAME,  /* 5 : frame of note in diag */
        STRING_DIAGITEM_AXISNAME    /* 6 : axis name and unit */
    };


    static COLORREF aColor[DIM(alInstColor)];     /* all diag colors */

    int i;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            RECT rcColRect;

            GetClientRect(GetDlgItem(hwndOpt, IDD_OPTDESK_COLORRECT), &rcColRect);
            CheckSelRadioBtn(hwndOpt, uDeskOpt & DOPT_PHASE_MASK, aBtnPhaseOpt);
            SetDlgItemInt(hwndOpt, IDD_OPTDESK_PIXMAP_SIZE, cxyBmpSamples, TRUE);
            SetDlgItemInt(hwndOpt, IDD_OPTDESK_ROOTS_BMPSIZE, cxyBmpRoots, TRUE);
            SetDlgItemInt(hwndOpt, IDD_OPTDESK_CURVE_SIZE, nWidthCurvePen, TRUE);

            for (i = 0; i < DIM(aPrecDesc); i++) /* set all precision vals */
                SetDlgItemInt(hwndOpt, aPrecDesc[i].nIddEdit,
                              anPrec[aPrecDesc[i].iPrec], TRUE);

            memcpy(aColor, alInstColor, sizeof(aColor));
            DrawBoxIdStrings(hwndOpt, IDD_OPTDESK_COLORNAMES, 0, DIM(IdColorName), IdColorName);
            return TRUE;
        } /* WM_INITDIALOG */


        case WM_DRAWITEM :
        {
            LPDRAWITEMSTRUCT lpdi = (LPDRAWITEMSTRUCT)lParam;

            if (lpdi->itemAction & ODA_DRAWENTIRE)
            {
                switch (lpdi->CtlID)
                {
                    case IDD_OPTDESK_COLORRECT:
                    {
                        /* circular color way based on physical color triangle
                           in RGB (red, green, blue) 

                           RGB(255, 255, 0)    yellow 
                           RGB(0, 255, 0)      green 
                           RGB(0, 255, 255)    cyan (türkis) 
                           RGB(0, 0, 255)      blue 
                           RGB(255, 0, 255)    magenta (violett) 
                           RGB(255, 0, 0)      red 

                        and special colors (based on luminance, no true colors)

                           RGB(255, 255, 255)  white 
                           RGB(128, 128, 128)  gray 
                           RGB(0, 0, 0)        black
                        */

                        int aColors[3] = {256, 256, 256};
                        int ColMax, ColDelta, cyColBar;
                        int i;
                        int k;   /* number of luminosity levels, means rots in color triangle */
                        int j;     /* number of color steps moving from one color to the next */ 
                        HBRUSH hbr;
                        RECT rcHorBar;

                        CopyRect(&rcHorBar, &lpdi->rcItem);            /* make a working copy */

                        if (GetDeviceCaps(lpdi->hDC, NUMCOLORS) < 256) k = 2; /* 8, 16, 256 (with palette the return value in general is 20 ) colors */
                        else k = 3;                /* three luminosity levels with all colors */

                        j = GetDeviceCaps(lpdi->hDC, NUMCOLORS)/(6*k+2);       /* preliminary */
                        cyColBar = (rcHorBar.bottom - rcHorBar.top + 1)/(j*(6L*k+2L)); /* height of color strip */

                        if (cyColBar < GetSystemMetrics(SM_CXDOUBLECLK))
                            cyColBar = GetSystemMetrics(SM_CXDOUBLECLK);
                        if (cyColBar < 1) cyColBar = 1;                      /* minimum width */

                        ColMax = aColors[0];   /* should be 256 -> start from white to yellow */
                        j = (rcHorBar.bottom - rcHorBar.top + 1)/cyColBar/(6*k+2);
                        if (j < 1) j = 1;                       /* less than 1 isn't possible */
                        ColDelta = ColMax/j;       /* there should be no remainder (fraction) */

                        rcHorBar.bottom = rcHorBar.top + cyColBar;  /* first color strip rect */
                        GotoNextPureColor(lpdi->hDC, aColors, &rcHorBar, IDXBLUE, -1, ColDelta, ColMax);

                        do                       /* do it for three or less luminosity levels */
                        {
                            for (i = 0; i < 6; i++)     /* the circular color way in triangle */
                                GotoNextPureColor(lpdi->hDC, aColors, &rcHorBar, i%3,
                                                  ODD(i) ? +1 : -1, ColDelta, ColMax);

                            ColMax /= 2;                            /* prepare next lum level */
                            ColDelta = ColMax/j;

                            for (i = 0; i < DIM(aColors); i++) aColors[i] /= 2;
                        } /* do */
                        while (--k);

                        do
                        {
                            aColors[IDXGREEN] -= ColDelta;
                            aColors[IDXRED] -= ColDelta;
                            NextColorRect(lpdi->hDC, aColors, &rcHorBar);
                        } /* */
                        while (aColors[IDXRED] > 0);

                        if (rcHorBar.top < lpdi->rcItem.bottom) /* not complete filled ? */
                        {
                            RECT rcItem;
                            GetWindowRect(lpdi->hwndItem, &rcItem);
                            ScreenToClientRect(hwndOpt, &rcItem);
                            MoveWindow(lpdi->hwndItem, rcItem.left, rcItem.top,
                                       rcItem.right-rcItem.left, rcHorBar.top, TRUE);
                        } /* if */

                        hbr = SelectBrush(lpdi->hDC, GetStockBrush(NULL_BRUSH));
                        Rectangle(lpdi->hDC, lpdi->rcItem.left, lpdi->rcItem.top,
                                  lpdi->rcItem.right, rcHorBar.top);
                        SelectBrush(lpdi->hDC, hbr);
                        break;
                    } /* IDD_OPTDESK_COLORRECT */


                    case IDD_OPTDESK_COLORMONITOR:
                    {
                        tMarker Comment =
                        {
                            MARKER_POSAUTO|MARKER_RIGHT|MARKER_VCENTER|MARKER_FRAME|MARKER_OPAQUE,
                            OPTDESK_MONCURVECOMMENT, OPTDESK_MONCURVECOMPOS, {0, 0, 0, 0}, HRGN_NULL, NULL
                        };

                        tDiag MonCurve =
                        {
                            HRGN_NULL, DIAG_YAUTORANGE, NULL, NULL, 1, 0, NULL,
                            NULL, OptDeskGetSampleCurveY, NULL,
                            {AXIS_DOTLINES, -15.0, 15.0, 0.0, 0, 639, 2, NULL, 0, STRING_AXIS_X},
                            {AXIS_DOTLINES, -1.6, 1.6, 0.0, 0, 479, 2, NULL, 0, IDSTRNULL}
                        };

                        RECT rcDiag;
                        SIZE cxyTxt;

                        GetTextExtentPoint(lpdi->hDC, Comment.szTxt,
                                           lstrlen(Comment.szTxt), &cxyTxt);
                        Comment.rcNote.bottom = Comment.rcNote.top + cxyTxt.cy+2*NOTE_BORDER_Y;
                        Comment.rcNote.right = Comment.rcNote.left + cxyTxt.cx+2*NOTE_BORDER_X;

                        GetClientRect(lpdi->hwndItem, &rcDiag);
                        MonCurve.X.nScreenMin = rcDiag.left;
                        MonCurve.Y.nScreenMin = rcDiag.top;
                        MonCurve.X.nScreenMax = rcDiag.right;
                        MonCurve.Y.nScreenMax = rcDiag.bottom;
                        MonCurve.pM = &Comment;

                        MonCurve.cxySmplCurve = GetDlgItemInt(hwndOpt, IDD_OPTDESK_CURVE_SIZE, NULL, FALSE);
                        if (MonCurve.cxySmplCurve < MIN_WIDTHCURVE) MonCurve.cxySmplCurve = 1;
                        else                                 /* paint with current line width */
                            if (MonCurve.cxySmplCurve > MAX_WIDTHCURVE)
                                MonCurve.cxySmplCurve = MAX_WIDTHCURVE;

                        if (!PaintDiag(GetWindowInstance(hwndOpt), lpdi->hDC,
                                       UserBreak, &MonCurve, aColor)) /* don't call MessageAckUsr() */
                            StatusMessage(ERROR_DIAGRAM);   /* because recurion possible */

                        /* free GDI ressources */
                        if (MonCurve.hrgnCurve != HRGN_NULL) DeleteRgn(MonCurve.hrgnCurve);
                        if (Comment.hrgnHotSpot != HRGN_NULL) DeleteRgn(Comment.hrgnHotSpot);
                        MonCurve.hrgnCurve = Comment.hrgnHotSpot = HRGN_NULL;
                        break; 
                    } /* IDD_OPTDESK_COLORMONITOR */
                } /* switch */
            } /* if */

            break;
        } /* WM_DRAWITEM */



        case WM_COMMAND :
        {
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDOK :
                {
                    int TmpPrec, TmpCxyRoots, TmpCxySamples, TmpWidthCurve;

                    if (!CheckDlgItemInt(
                             hwndOpt, IDD_OPTDESK_PIXMAP_SIZE,
                             IDD_OPTDESK_PIXMAP_TEXT, MIN_CXYBMP_SAMPLES,
                             MAX_CXYBMP_SAMPLES, &TmpCxySamples))
                       
                        break; /* if error */

                    if (!CheckDlgItemInt(hwndOpt, IDD_OPTDESK_ROOTS_BMPSIZE,
                                         IDD_OPTDESK_ROOTS_BMPSIZE_TEXT,
                                         MIN_CXYBMP_ROOTS, MAX_CXYBMP_ROOTS,
					                     &TmpCxyRoots))
                        break; /* if error */

                    if (!CheckDlgItemInt(
                             hwndOpt, IDD_OPTDESK_CURVE_SIZE,
                             IDD_OPTDESK_CURVE_TEXT, MIN_WIDTHCURVE,
                             MAX_WIDTHCURVE, &TmpWidthCurve))

                        break; /* if error */

                    for (i = 0; i < DIM(aPrecDesc); i++)    /* check all */
                        if (!CheckDlgItemInt(hwndOpt, aPrecDesc[i].nIddEdit,
                                             aPrecDesc[i].nIddStaticName,
                                             MIN_PRECISION, MAX_PRECISION,
                                             &TmpPrec)) return FALSE;

                    for (i = 0; i < DIM(aPrecDesc); i++)    /* check all */
                        (void)CheckDlgItemInt(hwndOpt, aPrecDesc[i].nIddEdit,
                                              aPrecDesc[i].nIddStaticName,
                                              MIN_PRECISION, MAX_PRECISION,
                                              &anPrec[aPrecDesc[i].iPrec]);

                    cxyBmpSamples = TmpCxySamples;
                    cxyBmpRoots = TmpCxyRoots;
                    nWidthCurvePen = TmpWidthCurve;

                    uDeskOpt &= ~DOPT_PHASE_MASK;
                    uDeskOpt |= GetSelRadioBtn(hwndOpt, aBtnPhaseOpt);
                    memcpy(alInstColor, aColor, sizeof(alInstColor));
                    EndDialog(hwndOpt, TRUE);
                    return TRUE;
                } /* IDOK */


                case IDCANCEL :
                    EndDialog(hwndOpt, FALSE);
                    return TRUE;

                case IDHELP :
                    WinHelp(hwndOpt, HELP_FNAME, HELP_CONTEXT, HelpOptDesktopDlg);
                    return TRUE;


                case IDD_OPTDESK_PIXMAP_SIZE:
                    if (ChangeIncDecInt(hwndOpt, wParam, lParam,
                                        MIN_CXYBMP_SAMPLES, MAX_CXYBMP_SAMPLES,
                                        DEFAULT_CXYBMP_SAMPLES)) return TRUE;
                    break; /* IDD_OPTDESK_PIXMAP_SIZE */

                case IDD_OPTDESK_CURVE_SIZE:
                    if (ChangeIncDecInt(hwndOpt, wParam, lParam,
                                        MIN_WIDTHCURVE, MAX_WIDTHCURVE,
                                        DEFAULT_WIDTHCURVE) ||
                        (GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_CHANGE))
                    {
                        InvalidateRect(GetDlgItem(hwndOpt, IDD_OPTDESK_COLORMONITOR), NULL, TRUE);
                        return TRUE;
                    } /* if */

                    break; /* IDD_OPTDESK_CURVE_SIZE */


                case IDD_OPTDESK_ROOTS_BMPSIZE:
                    if (ChangeIncDecInt(hwndOpt, wParam, lParam,
                                        MIN_CXYBMP_ROOTS, MAX_CXYBMP_ROOTS,
                                        DEFAULT_CXYBMP_ROOTS)) return TRUE;

                    break; /* IDD_OPTDESK_ROOTS_BMPSIZE */


                case IDD_OPTDESK_COLORRECT:  /* if BN_CLICKED */
                {
                    HWND hwndColorBar = GetDlgItem(hwndOpt, IDD_OPTDESK_COLORRECT);
                    HDC dc = GetDC(hwndColorBar);
                    DWORD dwPoint = GetMessagePos();            /* returns screen coordinates */
                    POINT ptClick = MAKEPOINT(dwPoint);

                    SetDlgFocus(hwndOpt, IDD_OPTDESK_COLORNAMES); /* shift focus to combo box */ 
                    ScreenToClient(hwndColorBar, &ptClick); /* translate to window coordinates */

                    aColor[GetDlgListCurSel(hwndOpt, IDD_OPTDESK_COLORNAMES)] =
                        GetPixel(dc, ptClick.x, ptClick.y);         /* save the choosed color */

                    ReleaseDC(hwndColorBar, dc);
                    InvalidateRect(GetDlgItem(hwndOpt, IDD_OPTDESK_COLORMONITOR), NULL, TRUE);
                    return TRUE;
                } /* IDD_OPTDESK_COLORRECT */

                case IDD_OPTDESK_GETSTDCOLORS:
                    memcpy(aColor, aDefaultColors, sizeof(aColor));
                    InvalidateRect(GetDlgItem(hwndOpt, IDD_OPTDESK_COLORMONITOR), NULL, TRUE);
                    return TRUE;


                default:
                    for (i = 0; i < DIM(aPrecDesc); i++)    /* check all */
                    {
                        if (wParam == aPrecDesc[i].nIddEdit)
                            if (ChangeIncDecInt(hwndOpt, wParam, lParam,
                                                MIN_PRECISION, MAX_PRECISION,
                                                DEFAULT_PRECISION))
                                return TRUE;
                    } /* for */

                    break; /* default */
            } /* switch */

            break;
        } /* case WM_COMMAND */
    } /* switch */
    return FALSE;
}



/************* application data file I/O functions */

/* writes an application key inside brackets, like Windows .INI files */
static void AppKeyPrintf(FILE *f, char *szKey)
{
    fprintf(f, "\n[%s]\n", szKey);
} /* AppKeyPrintf */


static void StringPrintf(FILE *f, char *szKey, char *szString)
{
    char *p = szString;

    if (szKey != NULL) fprintf(f, "%s=", szKey);

    if (p != NULL)
    {
        while (*p != '\0')
        {
            switch (*p)
            {
                case '\n': fputs("\\\\", f); /* substitute by two backslashes */

                case '[':
                case ']':
                case '\r': break;             /* erase special characters */

                default: putc(*p, f); break;
            } /* switch */

            ++p;
        } /* while */
    } /* if */

    putc('\n', f);
} /* StringPrintf() */


/* appends number 'Num' to 'szKey' and returns passed pointer */
static char *EnumKey(char *szKey, int Num)
{
    static char szEnumKey[40];

    sprintf(szEnumKey, "%s%d", szKey, Num);
    return szEnumKey;
} /* EnumKey */


/* writing numerator or denominator polynomial data into filter
   file */
static void PolyDataPrintf(FILE *f, PolyDat *p, BOOL bRootsValid)
{
    int i;

    fprintf(f, IOKEY_ORDER "=" FORMAT_FILE_INT "\n", p->order);

    for (i=0; i <= p->order; i++)
        fprintf(f, "%s=" FORMAT_FILE_DOUBLE_WRITE "\n",
                EnumKey(IOKEY_COEFF, i), p->coeff[i]);

    if (bRootsValid)
    {
        for (i=0; i < p->order; i++)
        fprintf(f, "%s=" FORMAT_FILE_ROOT_WRITE "\n",
                EnumKey(IOKEY_ROOT, i), p->root[i].x, p->root[i].y);
    } /* if */

    putc('\n', f);
} /* PolyDataPrintf */


/* writes all significant diag data into passed file by passed keyword */
static void AxisDataPrintf(FILE *f, char *szKey, tDiagAxis *pAxis)
{
    fprintf(f, "%s=" FORMAT_FILE_UINT "," FORMAT_FILE_SHORT ","
               FORMAT_FILE_SHORT "," FORMAT_FILE_DOUBLE_WRITE ","
               FORMAT_FILE_DOUBLE_WRITE "\n",
            szKey, pAxis->AxisFlags, pAxis->sPrecision, pAxis->iCurrUnit,
            pAxis->dWorldMin, pAxis->dWorldMax);
} /* AxisDataPrintf */


/* writes top left pos and bottom right pos of rectangle into passed
   file
 */
static void RectPosPrintf(FILE *f, RECT *p)
{
    fprintf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT "," FORMAT_FILE_INT "," FORMAT_FILE_INT,   
            p->left, p->top, p->right, p->bottom);
} /* RectPosPrintf */



static void WindowPosPrintf(FILE *f, char *szWinKey, HWND win)
{
    WINDOWPLACEMENT wpl =
    {
        sizeof(WINDOWPLACEMENT), WPF_SETMINPOSITION, SW_HIDE,
        {0, 0}, {0, 0}, {0, 0, 0, 0}
    };

    fprintf(f, "%s=", szWinKey);

    if (win != HWND_NULL)
    {
        wpl.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(win, &wpl);   /* don't works correct if window */
                             /* hidden, then returns SW_NORMAL in showCmd */
        if (!IsWindowVisible(win)) wpl.showCmd = SW_HIDE;    /* overwrite */
    } /* if */

    fprintf(f, FORMAT_FILE_UINT "," FORMAT_FILE_UINT ","
               FORMAT_FILE_INT "," FORMAT_FILE_INT ","
               FORMAT_FILE_INT "," FORMAT_FILE_INT ",",
               wpl.flags, wpl.showCmd,
               wpl.ptMinPosition.x, wpl.ptMinPosition.y, /* ptMinPosition */
               wpl.ptMaxPosition.x, wpl.ptMaxPosition.y  /* ptMaxPosition */
            ); /* end of fprintf */

    RectPosPrintf(f, &wpl.rcNormalPosition);

    putc('\n', f);
} /* WindowPosPrintf */



/* writes application ini file */
BOOL WriteIniFile(char *szFileName)
{
    FILE *f = fopen(szFileName, "wt");  /* open in text mode (not binary) */
    if (f == NULL)  return FALSE;   /* use '\n' to write CR LF into file  */

    AppIdentPrintf(f);              /* write application information data */
    DesktopCfgPrintf(f);

    return (fclose(f) == 0);
} /* WriteIniFile() */


/* returns the contents of a version ressource string */
static void GetAppResStr(char *rcName, char *pDestStr, int nLen)
{
    if (!GetRCVerStringInfo(rcName, pDestStr, nLen))
        lstrcpy(pDestStr, "<unknown>");
} /* GetAppResStr() */



/* Writes application identification data to passed file */
static void AppIdentPrintf(FILE *f)
{
    WORD StartupSecCrypt;

    char szBuf[128];
    WORD StartupSec = 1;

    AppKeyPrintf(f, IOAPPKEY_APPLICATION);
    GetAppResStr("InternalName", szBuf, DIM(szBuf));
    StringPrintf(f, IOKEY_APPNAME, szBuf);
    GetAppResStr("FileVersion", szBuf, DIM(szBuf));
    StringPrintf(f, IOKEY_VERSION, szBuf);

    MemCpyCrypt(&StartupSecCrypt, &StartupSec, sizeof(StartupSecCrypt));

    fprintf(f, IOKEY_SETUPCODE "=%X\n", StartupSecCrypt);
    StringPrintf(f, IOKEY_SERNO, "00000-00000");  /* freeware code */
    StringPrintf(f, IOKEY_USRNAME, "");
    StringPrintf(f, IOKEY_COMPANY, "");
} /* AppIdentPrintf() */



/* writes desktop config (ini) data to passed file */
static void DesktopCfgPrintf(FILE *f)
{
    int i;

    AppKeyPrintf(f, IOAPPKEY_DESKTOP);
    fprintf(f, IOKEY_DESKOPT "=" FORMAT_FILE_UINT "\n"
               IOKEY_CXY_SAMPLES "=" FORMAT_FILE_INT "\n"
               IOKEY_CXY_ROOTS "=" FORMAT_FILE_INT "\n"
               IOKEY_WIDTH_CURVE "=" FORMAT_FILE_INT "\n",
               uDeskOpt, cxyBmpSamples, cxyBmpRoots, nWidthCurvePen);

    fprintf(f, IOKEY_COLORS "=" FORMAT_FILE_COLOR, alInstColor[0]);
    for (i = 1; i < DIM(alInstColor); i++)       /* all installable colors */
        fprintf(f, "," FORMAT_FILE_COLOR, alInstColor[i]);

    fprintf(f, "\n" IOKEY_PRECISION "=" FORMAT_FILE_INT, anPrec[0]);
    for (i = 1; i < DIM(anPrec); i++)                  /* output precision */
        fprintf(f, "," FORMAT_FILE_INT, anPrec[i]);

    fprintf(f, "\n" IOKEY_MATHLIMITS "=" FORMAT_FILE_DOUBLE_WRITE, aMathLimits[0]);
    for (i = 1; i < DIM(aMathLimits); i++)     /* calculation error limits */
        fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE, aMathLimits[i]);
    putc('\n', f);

    WindowPosPrintf(f, IOKEY_WIN_DESKTOP, hwndFDesk);       /* main window */
} /* DesktopCfgPrintf() */



/* saving the filter data to passed file (existing or non existing)
 */
BOOL WriteProjectDataFile(char *szFileName)
{
    FILE *f;
    HWND hwndMDIChild, hNextChild;
    int i;

    /* open file in text mode (see O_TEXT in FCNTL.H), since usage of
       '\n' writes true CR LF into file
     * but reading the file via C-Library functions as getc() eleminates
       the CR and returns only the LF respective '\n' in the string 
     */

    if ((f = fopen(szFileName, "wt")) == NULL)  return FALSE;

    AppIdentPrintf(f);              /* write application information data */

    /* project data (name, (length of) description, status flags) */
    AppKeyPrintf(f, IOAPPKEY_PROJECT);
    StringPrintf(f, IOKEY_PRJNAME, MainFilter.szPrjName);
    StringPrintf(f, IOKEY_PRJDESC, MainFilter.szPrjDesc);

    fprintf(f, IOKEY_PRJFLAGS "=" FORMAT_FILE_UINT "\n", MainFilter.uFlags);

    /* basic filter data */
    AppKeyPrintf(f, IOAPPKEY_BASIC);

    fprintf(f, IOKEY_SAMPLING_FREQU "=" FORMAT_FILE_DOUBLE_WRITE "Hz\n"
               IOKEY_TRANSFER_FACTOR "=" FORMAT_FILE_DOUBLE_WRITE "\n",
            MainFilter.f0, MainFilter.factor);

    AppKeyPrintf(f, IOAPPKEY_NUMERATOR);                     /* numerator */
    PolyDataPrintf(f, &MainFilter.a, MainFilter.uFlags & FILTER_ROOTS_VALID);

    AppKeyPrintf(f, IOAPPKEY_DENOMINATOR);                 /* denominator */
    PolyDataPrintf(f, &MainFilter.b, MainFilter.uFlags & FILTER_ROOTS_VALID);

    AppKeyPrintf(f, IOAPPKEY_DESIGN);                      /* design data */
    fprintf(f, IOKEY_FTYPE "=" FORMAT_FILE_INT "," FORMAT_FILE_INT "\n",
            (int)MainFilter.f_type, (int)MainFilter.FltDlg);
                             
    fputs(IOKEY_DIALOG "=", f);          /* filter definition dialog data */

    switch (MainFilter.FltDlg)
    {
        case LINFIRDLG :
            fprintf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT ","
                       FORMAT_FILE_INT "," FORMAT_FILE_DOUBLE_WRITE,
                    MainFilter.DefDlg.FIRDat.nOrder, 
                    (int)MainFilter.DefDlg.FIRDat.SubType, 
                    (int)MainFilter.DefDlg.FIRDat.DataWin,
                    MainFilter.DefDlg.FIRDat.dCutoff);

            if (MainFilter.DefDlg.FIRDat.DataWin == KAISER_WIN)
                fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE, MainFilter.DefDlg.FIRDat.dAlpha);
            break; /* LINFIRDLG */

        case STDIIRDLG :
            fprintf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT "," FORMAT_FILE_INT "," 
                    FORMAT_FILE_DOUBLE_WRITE,
                    MainFilter.DefDlg.StdIIRDat.nOrder,
                    (int)MainFilter.DefDlg.StdIIRDat.SubType,
                    (int)MainFilter.DefDlg.StdIIRDat.SToZTransf,
                    MainFilter.DefDlg.StdIIRDat.dCutoff);

                   switch (MainFilter.DefDlg.StdIIRDat.SubType)
                   {
                       case CAUER1 :
                           fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE,
                                   MainFilter.DefDlg.StdIIRDat.dModuleAngle);
                                                       /* and fall through */
                       case CHEBY1 :
                           fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE,
                                   MainFilter.DefDlg.StdIIRDat.dRippleAtt);
                           break; /* CHEBY1, CAUER1 */

                       case CAUER2 :
                           fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE,
                                   MainFilter.DefDlg.StdIIRDat.dModuleAngle);
                                                       /* and fall through */
                       case CHEBY2 :
                           fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE,
                                   MainFilter.DefDlg.StdIIRDat.dMinAtt);
                           break; /* CHEBY2, CAUER2 */
                   } /* switch */

            break; /* STDIIRDLG */

        case PREDEFDLG :
            fprintf(f, FORMAT_FILE_INT, MainFilter.DefDlg.PredefSub);
            break; /* PREDEFDLG */

        case MISCDLG :      /* optional parameters not supported up today */
            fprintf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT,
                    MainFilter.DefDlg.MiscFltDat.nOrder,
                    (int)MainFilter.DefDlg.MiscFltDat.SubType);
            break; /* MISCDLG */
    } /* switch */

    fprintf(f, "," FORMAT_FILE_SHORT "\n", MainFilter.iInputUnitF);

    /* frequency transformation dialog data */
    switch (MainFilter.FltDlg)
    {
        case LINFIRDLG :
        case STDIIRDLG :
            fprintf(f, IOKEY_FTRANSFORM "=" FORMAT_FILE_INT ","
                       FORMAT_FILE_SHORT "," FORMAT_FILE_UINT,
                    (int)MainFilter.FTr.FTransf, MainFilter.FTr.iDefaultUnit,
                    MainFilter.FTr.uFlags);

            switch (MainFilter.FTr.FTransf)
            {
                case BANDPASS :
                case BANDSTOP :        /* center frequency (only BP or BS) */
                    fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE, MainFilter.FTr.dCenter);
                                                       /* and fall through */

                case HIGHPASS :    /* bandwidth (BP, BS) or cuttoff (HP) frequency */
                    fprintf(f, "," FORMAT_FILE_DOUBLE_WRITE, MainFilter.FTr.dCutFBw);
                    break; /* ALL */
            } /* switch */

            putc('\n', f);

            break; /* ALL */
    } /* switch */


    DesktopCfgPrintf(f);                   /* store desktop configuration */
    WindowPosPrintf(f, IOKEY_WIN_COEFFS, hwndFilterCoeff); /* coefficients */
    WindowPosPrintf(f, IOKEY_WIN_ROOTS, hwndFilterRoots);     /* roots win */

    i = 0;     
    hwndMDIChild = hNextChild = GetFirstChild(hwndDiagBox);
    while (hNextChild && (i < MAX_SAVE_WINDOWS))
    {                                             /* count number of diags */
        char szClassName[40];

        GetClassName(hNextChild, szClassName, DIM(szClassName));
        if (!lstrcmp(szClassName, DIAG_CLASS))
            if (GETDIAGTYPE(hNextChild) != DIAG_USER_DEF) ++i;

        hwndMDIChild = hNextChild;                     /* save last handle */
        hNextChild = GetNextSibling(hNextChild);                   /* next */
    } /* while */

    fprintf(f, IOKEY_DIAGCOUNT "=" FORMAT_FILE_INT "\n", i);

    i = 0;          /* save all diag data with correct Z-order */
    while (hwndMDIChild && (i < MAX_SAVE_WINDOWS))
    {                           
        char szClassName[40];

        GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
        if (!lstrcmp(szClassName, DIAG_CLASS))
        {
            if (GETDIAGTYPE(hwndMDIChild) != DIAG_USER_DEF)
            {
                int cNote = 0;
                tDiag *pDiag = GETPDIAG(hwndMDIChild);
                pMarker p = pDiag->pM;

                AppKeyPrintf(f, EnumKey(IOAPPKEY_DIAGS, i));
                WindowPosPrintf(f, IOKEY_DIAGPOS, hwndMDIChild);

                fprintf(f, IOKEY_DIAGTYPE "=" FORMAT_FILE_INT "\n"
                           IOKEY_DIAGSTYLE "=" FORMAT_FILE_UINT "\n",
                           (int)GETDIAGTYPE(hwndMDIChild), pDiag->nDiagOpt);

                if (pDiag->nDiagOpt & DIAG_CONST_SAMPLES)
                    fprintf(f, IOKEY_XPOINTS "=" FORMAT_FILE_INT "\n",
                            pDiag->nConstSmpl);

                AxisDataPrintf(f, IOKEY_AXIS_X, &pDiag->X);
                AxisDataPrintf(f, IOKEY_AXIS_Y, &pDiag->Y);

                while (p != NULL)           /* count all comments in diag */
                {
                    ++cNote;
                    p = p->pNextMarker;               /* ptr to next note */
                } /* while */

                fprintf(f, IOKEY_NOTE_CNT "=" FORMAT_FILE_INT "\n", cNote);

                cNote = 0;                      /* start new and save all */
                p = pDiag->pM;

                while (p != NULL)             /* for all comments in diag */
                {
                    fprintf(f, "%s=" FORMAT_FILE_UINT "," FORMAT_FILE_DOUBLE_WRITE ",",
                            EnumKey(IOKEY_NOTE, cNote), p->uOpt, p->x);
                    RectPosPrintf(f, &p->rcNote);
                    putc(',', f);
                    StringPrintf(f, NULL, p->szTxt);
                    ++cNote;
                    p = p->pNextMarker;               /* ptr to next note */
                } /* while */

                i++; /* next diagram */
            } /* if */
        } /* if */

        hwndMDIChild = GetPrevSibling(hwndMDIChild);   /* previous diagram */
    } /* while */

    return (fclose(f) == 0);
} /* WriteProjectDataFile() */


/* functions for loading saved filter data */

/* reads white spaces up to file end or any other character
 * if any file I/O error occurs check this later by ferror()
 */
static void ScanToNextTerm(FILE *f)
{
    char c;

    do                                 /* kill all white spaces in stream */
        c = getc(f);                     /* returns EOF if any read error */ 
    while (isspace(c));

    ungetc(c, f);
} /* ScanToNextTerm */



/* scans the application line (see also windows INI-Files) and
   checks matching with passed key
 * sets f->flags to _F_ERR (see STDIO.H) indicating an file I/O or matching
   error, check this later by function ferror()
 * BNF format : {WhiteSpace} '[' keyword ']' {WhiteSpace}
 */
static void AppKeyScanf(FILE *f, char *szKey)
{
    char c, szScanKey[256];
    BOOL bMustScan = FALSE;
    int i=0;
    int max = lstrlen(szKey);

    szScanKey[0] = '\0'; /* UAE safety */

    do
    {
        switch(c = getc(f))
        {
            case EOF : return;
            case '[' : bMustScan = TRUE; 
            case ']' : break;
            default  : if(bMustScan) szScanKey[i++] = c; break;
        } /* switch */
    } /* do */
    while((c != ']') && (i < DIM(szScanKey)) && (i <= max));

    szScanKey[i] = '\0';
    ScanToNextTerm(f);
    if (lstrcmp(szKey, szScanKey) != 0) f->flags |= _F_ERR;
} /* AppKeyScanf */



/* scans the keyword line (see also windows INI-Files) and
   checks matching with passed key
 * sets f->flags to _F_ERR (see STDIO.H) indicating an file I/O or matching
   error, check this later by function ferror()
 * BNF format : {WhiteSpace} keyword '='
 */
static void KeyScanf(FILE *f, char *szKey)
{
    char szScanKey[256];
    char c = ' ';
    int i=0;
    int max = lstrlen(szKey);

    ScanToNextTerm(f); /* scan white spaces */
    szScanKey[0] = '\0'; /* UAE safety */

    do
    {
        c = getc(f);
        if (c == '=') break; /* don't store the '=' */
        szScanKey[i++] = c;
    } /* do */
    while((c != EOF) && (i < DIM(szScanKey)) && (i <= max));

    szScanKey[i] = '\0';
    if (lstrcmp(szKey, szScanKey) != 0) f->flags |= _F_ERR;
} /* KeyScanf */


/* reads 'nChars' characters from stream or up to '\0' or '['
 */
static void StringScanf(char szBuf[], int nChars, FILE *f)
{
    int i = 0;
    char c = ' ';

    while ((i < nChars) && (c != '[') && (c != EOF) && (c != '\n'))
    {
        c = getc(f);            /* never returns '\r' (deleted from file) */
        switch (c)
        {
            case '\\':
                if ((i > 0) && (szBuf[i-1] == '\\'))
                {
                    szBuf[i-1] = '\r';       /* DOS/Windows specific case */
                    szBuf[i++] = '\n';
                    break; /* case '\\' */
                } /* if */

            default: szBuf[i++] = c; break;

            case '[' : c = ungetc(c, f);  /* put last char back to stream */
            case EOF :                                    /* fall through */
            case '\n': szBuf[i++] = '\0'; break;
        } /* switch */
    } /* while */

    if (i >= nChars) szBuf[i] = '\0';      /* limited ? set end of string */
} /* StringScanf() */



static void KeyDoubleScanf(FILE *f, char *szKey, double *d)
{
    KeyScanf(f, szKey);
    fdscanf(f, FORMAT_FILE_DOUBLE_READ, 1, d);
} /* KeyDoubleScanf */



static void KeyIntScanf(FILE *f, char *szKey, int *i)
{
    KeyScanf(f, szKey);
    fdscanf(f, FORMAT_FILE_INT, 1, i);
} /* KeyIntScanf */



static void KeyUnsignedScanf(FILE *f, char *szKey, unsigned *u)
{
    KeyScanf(f, szKey);
    fdscanf(f, FORMAT_FILE_UINT, 1, u);
} /* KeyUnsignedScanf */



/* reading numerator or denominator polynomial data from filter
   file */
static BOOL PolyDataScanf(FILE *f, PolyDat *p, BOOL bScanRoots)
{
    int i;


    KeyIntScanf(f, IOKEY_ORDER, &p->order);         /* order for malloc() */
    if (ferror(f)) return FALSE;            /* syntax or file I/O error */
    if (!MallocPolySpace(p)) return FALSE;
                                                   /* ok, degree is valid */
    for (i=0; i <= p->order; i++)                     /* all coefficients */
        KeyDoubleScanf(f, EnumKey(IOKEY_COEFF, i), &p->coeff[i]); /* scan */

    if (bScanRoots)
    {
        for (i=0; i < p->order; i++)           /* scan all roots if known */
        {
            KeyScanf(f, EnumKey(IOKEY_ROOT, i));
            fdscanf(f, FORMAT_FILE_ROOT_READ, 2, &p->root[i].x, &p->root[i].y);
        } /* for */
    } /* if */

    return !ferror(f);
} /* PolyDataScanf*/


/* works similiary like fscanf, but checks format errors and sets
   ferror() TRUE if any error occurs
 */
static void fdscanf(FILE *f, char *format, int nArgs, ...)
{
   va_list  p;
   int nScanCnt;

   va_start(p, nArgs);
   nScanCnt = vfscanf(f, format, p);
   va_end(p);

   if (nScanCnt != nArgs) f->flags |= _F_ERR;    /* set global error flag */
} /* fdscanf */


static void AxisDataScanf(FILE *f, char *szKey, tDiagAxis *pAxis)
{
    KeyScanf(f, szKey);
    fdscanf(f, FORMAT_FILE_UINT "," FORMAT_FILE_SHORT ","
               FORMAT_FILE_SHORT "," FORMAT_FILE_DOUBLE_READ ","
               FORMAT_FILE_DOUBLE_READ,
            5, &pAxis->AxisFlags, &pAxis->sPrecision, &pAxis->iCurrUnit,
            &pAxis->dWorldMin, &pAxis->dWorldMax);
} /* AxisDataScanf */


/* scans top left pos and bottom right pos of rectangle from passed
   file
 */
static void RectPosScanf(FILE *f, RECT *p)
{
    fdscanf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT "," FORMAT_FILE_INT ","
               FORMAT_FILE_INT, 4,   
            &p->left, &p->top, &p->right, &p->bottom);
} /* RectPosScanff */



static void WindowPosScanf(FILE *f, char *szWinKey, WINDOWPLACEMENT *wpl)
{
    KeyScanf(f, szWinKey);
    wpl->length = sizeof(WINDOWPLACEMENT);

    fdscanf(f, FORMAT_FILE_UINT "," FORMAT_FILE_UINT ","
               FORMAT_FILE_INT "," FORMAT_FILE_INT ","
               FORMAT_FILE_INT "," FORMAT_FILE_INT ",",
               6, &wpl->flags, &wpl->showCmd,
               &wpl->ptMinPosition.x, &wpl->ptMinPosition.y, /* ptMinPosition */
               &wpl->ptMaxPosition.x, &wpl->ptMaxPosition.y  /* ptMaxPosition */
            ); /* end of fdscanf */

    RectPosScanf(f, &wpl->rcNormalPosition);
} /* WindowPosScanf */


/* scans the keywords true/false at key equation
 * not used up today because only writing flags as complete byte
 */
static BOOL BoolFlagScanf(FILE *f, char *szKey)
{
    char szFlgStr[8];

    KeyScanf(f, szKey);
    StringScanf(szFlgStr, DIM(szFlgStr)-1, f);
    return lstrcmpi(szFlgStr, "false");
} /* BoolFlagScanf */



/* reads application ini file */
int ReadIniFile(char *szFileName)
{
    unsigned DummyInstCode;
    char szCompany[SIZE_LICENSE_COMPANY+1] = {'\0'};
    char szSerNo[SIZE_LICENSE_SERNO+1] = {'\0'};
    char szUsr[SIZE_LICENSE_SERNO+1] = {'\0'};

    int nErr = IDSTRNULL;
    FILE *f = fopen(szFileName, "rt");          /* open file in text mode */

    if (f == NULL)  return ERROR_FILEREAD;

                 /* read crypted startup delay (5sec are crypted to DEE4) */
    nErr = AppIdentScanf(f, szSerNo, szUsr, szCompany, &DummyInstCode);

    if (nErr == IDSTRNULL)                       /* scan appl. info first */
    {
        if (!DesktopCfgScanf(f)) nErr = ERROR_FILEREAD; /* scan desktop layout */
    } /* if */

    if (fclose(f) != 0) nErr = ERROR_FILEREAD;
    UpdateMDIMenuItems();              /* if error loading desktop layout */
    return nErr;
} /* ReadIniFile() */



/* scans application identification data */
static int AppIdentScanf(FILE *f, char *szSerNo, char *szUser,
                         char *szCompany, unsigned *pInstCode)
{
    char szBuf1[128], szBuf2[128];

    AppKeyScanf(f, IOAPPKEY_APPLICATION);           /* application */

    KeyScanf(f, IOKEY_APPNAME);
    StringScanf(szBuf1, DIM(szBuf1)-1, f);
    GetAppResStr("InternalName", szBuf2, DIM(szBuf2));
    if (lstrcmp(szBuf1, szBuf2)) return ERROR_APPMISMATCH;

    KeyScanf(f, IOKEY_VERSION);                     /* version management */
    StringScanf(szBuf1, DIM(szBuf1)-1, f);
    if (GetRCVerStringInfo("FileVersion", szBuf2, DIM(szBuf1)))
    {
        int nVer = atoi(szBuf1);

        if (ferror(f)) return ERROR_FILEREAD;     /* file version unknown */
        if (nVer > atoi(szBuf2))                            /* too less ? */
           return ERROR_FILEREAD_VERSION;         /* upward compatibility */
    } /* if */                                  /* else no check possible */


    if (ferror(f)) return ERROR_FILEREAD;
    KeyUnsignedScanf(f, IOKEY_SETUPCODE, pInstCode);
    if (ferror(f))
    {
        *pInstCode = UINT_MAX/2;
        return ERROR_FILEREAD;
    } /* if */

    KeyScanf(f, IOKEY_SERNO);
    StringScanf(szSerNo, SIZE_LICENSE_SERNO, f);
    KeyScanf(f, IOKEY_USRNAME);
    StringScanf(szUser, SIZE_LICENSE_USER, f);
    KeyScanf(f, IOKEY_COMPANY);
    StringScanf(szCompany, SIZE_LICENSE_COMPANY, f);

    if (ferror(f)) return ERROR_FILEREAD;

    return IDSTRNULL;
} /* AppIdentScanf() */




static BOOL DesktopCfgScanf(FILE *f)
{
    int i;
    WINDOWPLACEMENT wpl;

    AppKeyScanf(f, IOAPPKEY_DESKTOP);

    KeyUnsignedScanf(f, IOKEY_DESKOPT, &uDeskOpt);
    KeyIntScanf(f, IOKEY_CXY_SAMPLES, &cxyBmpSamples);
    KeyIntScanf(f, IOKEY_CXY_ROOTS, &cxyBmpRoots);
    KeyIntScanf(f, IOKEY_WIDTH_CURVE, &nWidthCurvePen);

    if (ferror(f)) return FALSE;

    CheckMenuItem(GetMenu(hwndFDesk), IDM_SHOWCOEFF,
                  MF_BYCOMMAND | ((uDeskOpt & DOPT_SHOWCOEFF) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(GetMenu(hwndFDesk), IDM_SHOWROOTS,
                  MF_BYCOMMAND | ((uDeskOpt & DOPT_SHOWROOTS) ? MF_CHECKED : MF_UNCHECKED));

    KeyScanf(f, IOKEY_COLORS);
    fdscanf(f, FORMAT_FILE_COLOR, 1, &alInstColor[0]);
    for (i = 1; i < DIM(alInstColor); i++)   /* all installable colors */
        fdscanf(f, "," FORMAT_FILE_COLOR, 1, &alInstColor[i]);

    KeyScanf(f, IOKEY_PRECISION);
    fdscanf(f, FORMAT_FILE_INT, 1, &anPrec[0]);
    for (i = 1; i < DIM(anPrec); i++)              /* output precision */
        fdscanf(f, "," FORMAT_FILE_INT, 1, &anPrec[i]);

    KeyScanf(f, IOKEY_MATHLIMITS);
    fdscanf(f, FORMAT_FILE_DOUBLE_READ, 1, &aMathLimits[0]);
    for (i = 1; i < DIM(aMathLimits); i++)              /* math calc limits */
        fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1, &aMathLimits[i]);

    WindowPosScanf(f, IOKEY_WIN_DESKTOP, &wpl);        /* main window */
    if (ferror(f)) return FALSE;
    SetWindowPlacement(hwndFDesk, &wpl);

    return (!ferror(f));
} /* DesktopCfgScanf() */




/* reads the filter data into variable MainFilter and (optional) desktop
   data from the specified file (path/name passed in variable szFileName) 
 * You can also access the same data by the Windows function (example)
       GetPrivateProfileString(IOAPPKEY_DESKTOP, IOKEY_CXY_ROOTS, "ERROR",
                               szData, DIM(szData), szFileName);
 */
int ReadProjectDataFile(char *szFileName, BOOL bLoadProject)
{
    FILE *f;
    int nErr = IDSTRNULL;
                                                /* open file in text mode */
    if ((f = fopen(szFileName, "rt")) == NULL)  return ERROR_FILEREAD_OPEN;

    nErr = ReadPrjFile(f, bLoadProject);
    if (fclose(f) != 0) nErr = ERROR_FILEREAD_CLOSE;
    UpdateMDIMenuItems();              /* if error loading desktop layout */
    return nErr;
} /* ReadProjectDataFile() */





static int ReadPrjFile(FILE *f, BOOL bLoadProject)
{
    tFilter TmpFlt;
    char szBuf[max(SIZE_PRJDESC,SIZE_COMMENT)];
    int nData;
    unsigned DummyInstCode;

    nData = AppIdentScanf(f, szBuf, szBuf, szBuf, &DummyInstCode);
    if (nData != IDSTRNULL) return nData;  /* don't check license errors */

    /* project data (name, (length of) description, status flags) */
    AppKeyScanf(f, IOAPPKEY_PROJECT);
    KeyScanf(f, IOKEY_PRJNAME);
    StringScanf(TmpFlt.szPrjName, DIM(TmpFlt.szPrjName)-1, f);
    KeyScanf(f, IOKEY_PRJDESC);
    TmpFlt.szPrjDesc = NULL;        /* no memory malloced for description */

    if (ferror(f)) return ERROR_FILEREAD;    /* incorrect data for malloc */
    StringScanf(szBuf, DIM(szBuf)-1, f);
    if ((nData = lstrlen(szBuf)) > 0)
    {
        TmpFlt.szPrjDesc = MALLOC(nData+1); /* nData is the string length! */
        if (TmpFlt.szPrjDesc == NULL) return ERROR_FILEREAD_MEM; /* memory error */
        lstrcpy(TmpFlt.szPrjDesc, szBuf);
    } /* if */

    KeyScanf(f, IOKEY_PRJFLAGS);
    fdscanf(f, FORMAT_FILE_UINT, 1, &TmpFlt.uFlags);
    if (ferror(f)) return ERROR_FILEREAD;            /* incorrect flags */

    /* basic filter data */
    AppKeyScanf(f, IOAPPKEY_BASIC);

    KeyDoubleScanf(f, IOKEY_SAMPLING_FREQU, &TmpFlt.f0); /* sampling frequency */
    StringScanf(szBuf, 2, f);                                /* read "Hz" */
    KeyDoubleScanf(f, IOKEY_TRANSFER_FACTOR, &TmpFlt.factor);  /* T(f) premultiplier */

    AppKeyScanf(f, IOAPPKEY_NUMERATOR);                      /* numerator */
    if (!PolyDataScanf(f, &TmpFlt.a, TmpFlt.uFlags & FILTER_ROOTS_VALID))
        return ERROR_FILEREAD;

    AppKeyScanf(f, IOAPPKEY_DENOMINATOR);                  /* denominator */
    if (!PolyDataScanf(f, &TmpFlt.b, TmpFlt.uFlags & FILTER_ROOTS_VALID))
        return ERROR_FILEREAD;

    AppKeyScanf(f, IOAPPKEY_DESIGN);                       /* design data */
    KeyScanf(f, IOKEY_FTYPE);
    fdscanf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT, 2,
            (int *)&TmpFlt.f_type, (int *)&TmpFlt.FltDlg);

    KeyScanf(f, IOKEY_DIALOG);
    switch (TmpFlt.FltDlg)
    {
        case LINFIRDLG :
            fdscanf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT "," FORMAT_FILE_INT ","
                   FORMAT_FILE_DOUBLE_READ, 4,
                   &TmpFlt.DefDlg.FIRDat.nOrder,
                   (int *)&TmpFlt.DefDlg.FIRDat.SubType,
                    (int *)&TmpFlt.DefDlg.FIRDat.DataWin,
                    &TmpFlt.DefDlg.FIRDat.dCutoff);

            if (TmpFlt.DefDlg.FIRDat.DataWin == KAISER_WIN)
                fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1, &TmpFlt.DefDlg.FIRDat.dAlpha);
            break; /* LINFIRDLG */

        case STDIIRDLG :
            fdscanf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT "," FORMAT_FILE_INT
                      "," FORMAT_FILE_DOUBLE_READ, 4,
                   &TmpFlt.DefDlg.StdIIRDat.nOrder,
                   (int *)&TmpFlt.DefDlg.StdIIRDat.SubType,
                   (int *)&TmpFlt.DefDlg.StdIIRDat.SToZTransf,
                   &TmpFlt.DefDlg.StdIIRDat.dCutoff);

                   switch (TmpFlt.DefDlg.StdIIRDat.SubType)
                   {
                       case CAUER1 :
                           fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1,
                                  &TmpFlt.DefDlg.StdIIRDat.dModuleAngle);
                                                      /* and fall through */
                       case CHEBY1 :
                           fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1,
                                  &TmpFlt.DefDlg.StdIIRDat.dRippleAtt);
                           break; /* CHEBY1, CAUER1 */

                       case CAUER2 :
                           fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1,
                                  &TmpFlt.DefDlg.StdIIRDat.dModuleAngle);
                                                      /* and fall through */
                       case CHEBY2 :
                           fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1,
                                  &TmpFlt.DefDlg.StdIIRDat.dMinAtt);
                           break; /* CHEBY2, CAUER2 */
                   } /* switch */

            break; /* STDIIRDLG */

        case PREDEFDLG :
            fdscanf(f, FORMAT_FILE_INT, 1, &TmpFlt.DefDlg.PredefSub);
            break; /* PREDEFDLG */

        case MISCDLG :      /* optional parameters not supported up today */
            fdscanf(f, FORMAT_FILE_INT "," FORMAT_FILE_INT, 2,
                   &TmpFlt.DefDlg.MiscFltDat.nOrder,
                   (int *)&TmpFlt.DefDlg.MiscFltDat.SubType);
            break; /* MISCDLG */
    } /* switch */



    fdscanf(f, "," FORMAT_FILE_SHORT, 1, &TmpFlt.iInputUnitF);

    /* frequency transformation dialog data */
    switch (TmpFlt.FltDlg)
    {
        case LINFIRDLG :
        case STDIIRDLG :
            KeyScanf(f, IOKEY_FTRANSFORM);
            fdscanf(f, FORMAT_FILE_INT "," FORMAT_FILE_SHORT "," FORMAT_FILE_UINT,
                    3, (int *)&TmpFlt.FTr.FTransf, &TmpFlt.FTr.iDefaultUnit,
                    &TmpFlt.FTr.uFlags);

            switch (TmpFlt.FTr.FTransf)
            {
                case BANDPASS :
                case BANDSTOP :       /* center frequency (only BP or BS) */
                    fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1, &TmpFlt.FTr.dCenter);
                                                      /* and fall through */

                case HIGHPASS :      /* bandwidth (BP, BS) or cutoff (HP) */
                    fdscanf(f, "," FORMAT_FILE_DOUBLE_READ, 1, &TmpFlt.FTr.dCutFBw);
                    break; /* ALL */
            } /* switch */

            break; /* ALL */
    } /* switch */

    if (ferror(f)) return ERROR_FILEREAD;

    /* at this point all data correct ! */
    if (MainFilter.f_type != NOTDEF)
    {
        FreeFilter(&MainFilter);    /* frees only coeffs and roots space */
        if (MainFilter.szPrjDesc != NULL) FREE((void *)MainFilter.szPrjDesc);
    } /* if */

    memcpy(&MainFilter, &TmpFlt, sizeof(MainFilter));


    if (bLoadProject)  /********************* desktop and windows section */
    {
        int cDiags, i, cNotes, k;
        WINDOWPLACEMENT wpl;
        HWND hwndMDIChild = GetFirstChild(hwndDiagBox);

        while (hwndMDIChild)            /* destroy all MDI client windows */
        {
            char szClassName[40];
            HWND hwndNext = GetNextSibling(hwndMDIChild); /* get next MDI */
                                /* client in chain before destroy current */
            GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
            if (!lstrcmp(szClassName, DIAG_CLASS))
                SendMessage(hwndDiagBox, WM_MDIDESTROY,
                            (WPARAM)hwndMDIChild, DUMMY_LPARAM);

            hwndMDIChild = hwndNext; /* restore next client window handle */
        } /* while */


        if (!DesktopCfgScanf(f)) return ERROR_FILEREAD_INCOMPLETE;

        WindowPosScanf(f, IOKEY_WIN_COEFFS, &wpl); /* coeff dialog window */
        if (ferror(f)) return ERROR_FILEREAD_INCOMPLETE;

        if (hwndFilterCoeff == HWND_NULL)                /* must create ? */
            hwndFilterCoeff = FDWinCreateModelessDlg(MAKEINTRESOURCE(COEFFDLG),
                                                     hwndFDesk, CoeffDlgProc);
        SetWindowPlacement(hwndFilterCoeff, &wpl);
        ShowWindow(hwndFilterCoeff, (uDeskOpt & DOPT_SHOWCOEFF) ? SW_SHOWNORMAL:SW_HIDE);

        WindowPosScanf(f, IOKEY_WIN_ROOTS, &wpl);  /* roots dialog window */
        if (ferror(f)) return ERROR_FILEREAD_INCOMPLETE;
        if (hwndFilterRoots == HWND_NULL)
            hwndFilterRoots = FDWinCreateModelessDlg(MAKEINTRESOURCE(ROOTSDLG),
                                                     hwndFDesk, RootsDlgProc);
        SetWindowPlacement(hwndFilterRoots, &wpl);
        ShowWindow(hwndFilterRoots, (uDeskOpt & DOPT_SHOWROOTS) ? SW_SHOWNORMAL:SW_HIDE);

        KeyIntScanf(f, IOKEY_DIAGCOUNT, &cDiags);      /* number of diags */

        for (i = 0; i < cDiags; i++)
        {                        /* read all diag data and create windows */
            DIAGCREATESTRUCT dg;
            tDiag *pDiag;

            dg.lpDiag = pDiag = MALLOC(sizeof(tDiag));
            if (pDiag == NULL) return ERROR_FILEREAD_INCMEM; /* memory space error ? */

            AppKeyScanf(f, EnumKey(IOAPPKEY_DIAGS, i));
            WindowPosScanf(f, IOKEY_DIAGPOS, &wpl);
            KeyIntScanf(f, IOKEY_DIAGTYPE, (int *)(&dg.type));
            *pDiag = Diags[dg.type].DefaultDiag;     /* init with default */
            KeyUnsignedScanf(f, IOKEY_DIAGSTYLE, &pDiag->nDiagOpt);

            if (pDiag->nDiagOpt & DIAG_CONST_SAMPLES)
                KeyIntScanf(f, IOKEY_XPOINTS, &pDiag->nConstSmpl);

            AxisDataScanf(f, IOKEY_AXIS_X, &pDiag->X);
            AxisDataScanf(f, IOKEY_AXIS_Y, &pDiag->Y);

            if (ferror(f)) return ERROR_FILEREAD_INCOMPLETE;
            hwndMDIChild = CreateDiagWin(&dg);
            SetWindowPlacement(hwndMDIChild, &wpl);     /* set pos./style */

            KeyIntScanf(f, IOKEY_NOTE_CNT, &cNotes);   /* num of comments */

            for (k = 0; k < cNotes; k++)    /* create all notes from file */
            {
                pMarker p;
                int len;

                if (ferror(f)) return ERROR_FILEREAD_INCOMPLETE;
                p = InsertNewNote(hwndMDIChild);            /* create new */
                if (p == NULL) return ERROR_FILEREAD_INCMEM;    /* not enough memory */

                KeyScanf(f, EnumKey(IOKEY_NOTE, k));
                fdscanf(f, FORMAT_FILE_UINT "," FORMAT_FILE_DOUBLE_READ ",", 2,
                        &p->uOpt, &p->x);
                RectPosScanf(f, &p->rcNote);

                fdscanf(f, ",", 0);
                p->szTxt = NULL;
                StringScanf(szBuf, DIM(szBuf)-1, f);   /* get the comment */
                if ((len = lstrlen(szBuf)) > 0)
                {
                    p->szTxt = MALLOC(len);      /* get memory for string */
                    if (p->szTxt == NULL) return ERROR_FILEREAD_INCMEM;   /* not enough mem */
                    lstrcpy(p->szTxt, szBuf);
                } /* if */
            } /* for */
        } /* for */
    } /* if (bLoadPrj) */

    return IDSTRNULL;
} /* ReadPrjFile() */



/************* getting functions for user def diag */
#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserSin(double *x, double *y, void *pUnusedData)
{
    *y = sin(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserCos(double *x, double *y, void *pUnusedData)
{
    *y = cos(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserTan(double *x, double *y, void *pUnusedData)
{
    return ProtectedDiv(sin(*x), cos(*x), y);
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserCos2(double *x, double *y, void *pUnusedData)
{
    *y = cos(*x)*cos(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserSqr(double *x, double *y, void *pUnusedData)
{
    *y = *x * *x;
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserSinh(double *x, double *y, void *pUnusedData)
{
    *y = sinh(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserCosh(double *x, double *y, void *pUnusedData)
{
    *y = cosh(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserTanh(double *x, double *y, void *pUnusedData)
{
    return ProtectedDiv(sinh(*x), cosh(*x), y);
} /* fnUserTanh() */


#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserExp(double *x, double *y, void *pUnusedData)
{
    *y = exp(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserSi(double *x, double *y, void *pUnusedData)
{
    *y = si(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserSi2(double *x, double *y, void *pUnusedData)
{
    *y = si2(*x);
    return TRUE;
}

static BOOL fnUserBessel(double *x, double *y, void *pDegree)  /* Bessel function of zero order, first art */
{
    *y = bessel(*(int *)(pDegree), *x, aMathLimits[IMATHLIMIT_ERRBESSEL]);
    return TRUE;
} /* fnUserBessel() */


#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserSineIntegr(double *x, double *y, void *pUnusedData)
{
    *y = SineIntegral(*x, aMathLimits[IMATHLIMIT_ERRSI]);
    return TRUE;
} /* fnUserSineIntegr() */


static BOOL fnUserCheby(double *x, double *y, void *pDegree)
{
    *y = Chebyshev(*(int *)(pDegree), *x);
    return TRUE;
}



#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserComplElliptic1(double *x, double *y, void *pDummy)
{
    if (fabs(*x) >= 1.0) return FALSE;   /* modules greater than or equal */
                                               /* unity (1.0) not allowed */
    *y = EllIntegr_K(*x, aMathLimits[IMATHLIMIT_ERRELLIPTIC]);
    return TRUE;
} /* fnUserComplElliptic1() */



static BOOL fnUserEllipticIntegr1(double *x, double *y, void *pModul)
{
    *y = EllIntegr_F(*(double *)(pModul), *x, aMathLimits[IMATHLIMIT_ERRELLIPTIC]);
    return TRUE;
} /* fnUserEllipticIntegr1 */


static BOOL fnUserJacobiSN(double *x, double *y, void *pModul)
{
    *y = JacobiSN(*(double *)(pModul), *x, aMathLimits[IMATHLIMIT_ERRJACOBISN]);
    return TRUE;
} /* fnUserJacobiSN */


static BOOL fnUserJacobiCN(double *x, double *y, void *pModul)
{
    *y = JacobiCN(*(double *)(pModul), *x, aMathLimits[IMATHLIMIT_ERRJACOBISN]);
    return TRUE;
} /* fnUserJacobiCN */


static BOOL fnUserJacobiDN(double *x, double *y, void *pModul)
{
    *y = JacobiDN(*(double *)(pModul), *x, aMathLimits[IMATHLIMIT_ERRJACOBISN]);
    return TRUE;
} /* fnUserJacobiDN */


/* next are window functions original defined between 0<=x<=1 */
#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserHamming(double *x, double *y, void *pUnusedData)
{
    *y = hamming(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserHanning(double *x, double *y, void *pUnusedData)
{
    *y = hanning(*x);
    return TRUE;
}


#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserBlackman(double *x, double *y, void *pUnusedData)
{
    *y = blackman(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserTriangle(double *x, double *y, void *pUnusedData)
{
    *y = triangle(*x);
    return TRUE;
}

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL fnUserRectangle(double *x, double *y, void *pUnusedData)   /* returns 1  if x between 0 and +1.0 */
{
    *y = rectangle(*x);
    return TRUE;
}
