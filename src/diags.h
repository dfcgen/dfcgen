/* 2D diagram plot function header file *
 * running with Windows 3.1             *
 * author : Ralf Hoppe                  *
 * version : 0.01                       */

#ifndef __DIAGS_H

#ifndef WINVER
#define WINVER                 0x030a   /* only for windows 3.1 */
#endif

/* generating control */
#define STRICT

#include <stdlib.h>       /* supports exit values (i.e. EXIT_FAILURE ... ) */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <values.h>
#include <windowsx.h>

/* public macros */
#define HRGN_NULL               ((HRGN) 0)

/* global macros */
#define ROUND(x)               floor((x) + 0.5)

/* internal limits of PaintDiag, ... */
#define WORLD_MIN_LOG   ((double)MINFLOAT)
#define WORLD_RATIO_LOG (2*DBL_EPSILON)   /* min of |dWorldMax/dWorldMin| */

#define WORLD_MIN       (-(double)(MAXDOUBLE/32))
#define WORLD_MAX       ((double)(MAXDOUBLE/32))
#define WORLD_NULL      (2*DBL_EPSILON)
#define WORLD_DIFF      (16*DBL_EPSILON)   /* min of |dWorldMax-dWorldMin| */


typedef int                    COORDINATE;  /* set to long for WINDOWS NT */


typedef struct {
    char   *szUnit;      /* NULL if the last entry in a list array */
    double dUnitFactor;  /* multiplier for this unit */
} tUnitDesc;


typedef struct {
    unsigned AxisFlags;
    double dWorldMin;           /* fill before passing to PaintDiag */
    double dWorldMax;           /* (if Y_AUTORANGE this is the abs. limit)  */
    double dDevToWorldRatio;    /* delta dev coord to world coord ratio (set by PaintDiag) */
    COORDINATE nScreenMin, nScreenMax;/* fill before passing to PaintDiag */
    short  sPrecision;          /* digits after point */
    tUnitDesc *lpUnitList;      /* pointer to list of possible units for this
                                   axis or NULL if no list exist */
    short iCurrUnit;            /* index of current unit in lpUnitList[] */
    int nIdAxisName;            /* Name of Axis */
} tDiagAxis;                    /* definition for one axis */

/* values of axis flags */
#define AXIS_LOG        0x01       /* default is linear axis (user inst.) */
#define AXIS_DOTLINES   0x02       /* dotted scale lines */


struct tagMarker {
    unsigned uOpt;                                              /* options */
    char *szTxt;                 /* marker text (NULL if no mem allocated) */
    double x;                    /* x world coordinate for marker position */
    RECT   rcNote;           /* (size of) rect relativ to x,y marker point */
    HRGN   hrgnHotSpot;     /* region of hot spot on curve (0 if not def.) */
    struct tagMarker *pNextMarker;        /* pointer to next marker struct */
};

typedef struct tagMarker tMarker;
typedef struct tagMarker * pMarker;

/* public definitions of internal metrics */
#define NOTE_BORDER_X          (GetSystemMetrics(SM_CXFRAME)+1)
#define NOTE_BORDER_Y          (GetSystemMetrics(SM_CYFRAME)+1)
#define CX_HOTSPOT(pDiag)      (max(GetSystemMetrics(SM_CXDOUBLECLK)+2, \
                                    pDiag->cxySmplCurve))
#define CY_HOTSPOT(pDiag)      (max(GetSystemMetrics(SM_CYDOUBLECLK)+2, \
                                    pDiag->cxySmplCurve))

/* marker options (derivated by Windows defined DT_xxx internals) */
#define MARKER_RIGHT        DT_LEFT
#define MARKER_BOTTOM       DT_TOP
#define MARKER_LEFT         DT_RIGHT
#define MARKER_HCENTER      DT_CENTER
#define MARKER_TOP          DT_BOTTOM
#define MARKER_VCENTER      DT_VCENTER
#define MARKER_POSAUTO      0x1000  /* pos. of rect is auto, but not size */
#define MARKER_HIDE         0x2000             /* hide this single marker */
#define MARKER_FRAME        0x4000                   /* frame marker text */
#define MARKER_OPAQUE       0x8000     /* opaque or transparent rectangle */

/* derivated constants */
#define MARKER_HPOS_MASK    (MARKER_LEFT|MARKER_HCENTER|MARKER_RIGHT)
#define MARKER_VPOS_MASK    (MARKER_TOP|MARKER_VCENTER|MARKER_BOTTOM)
#define MARKER_POS_MASK     (MARKER_VPOS_MASK|MARKER_HPOS_MASK)

#define MARKER_DEFAULT      (MARKER_LEFT|MARKER_TOP|MARKER_FRAME| \
                             MARKER_OPAQUE|MARKER_POSAUTO)     /* default */


typedef int    (*FUNCGETYINIT)(double dStartX, double dEndX, void **pAppDataPtr);
typedef BOOL   (*FUNCGETY)(double *x, double *y, void *pAppData);
typedef void   (*FUNCGETYEND)(void *pAppData);
typedef BOOL   (*FUNCUSERBREAK)(void);


typedef struct {
    HRGN   hrgnCurve;    /* region handle to curve area (create by PaintDiag) */
    unsigned nDiagOpt;                           /* Options of the diagram */
    pMarker pM;                    /* marking points in diag (can be NULL) */
    LPCSTR szCurveBmp;            /* only for discret diagrams (NULL else) */
    int cxySmplCurve; /* height/width of samples (bitmap) in discret diags */
                              /* width of curve pen in continuous 2D diags */
    int nConstSmpl;      /* number of x samples in analogue diags, if Flag */
                                              /* DIAG_CONST_SAMPLES is set */
    void *pAppData;    /* ptr to application data passed via GetNextWorldY */
    FUNCGETYINIT InitGetY;  /* should return number of discret samples in
                               passed interval [dStartX, dEndX] 
                             * if continous diag should return a value
                               greather/equal 1 
                             * if an error occurs should return a negative
                               number, e.g. -1 */

    FUNCGETY GetNextWorldY;    /* pointer to function computes y (and x if
                                * discret diag) in real world, returns FALSE
                                * if no valid value exist in real world */
    FUNCGETYEND EndGetY;                 /* ptr to end of computation func */
    tDiagAxis X;                                      /* x axis definition */
    tDiagAxis Y;                                      /* y axis definition */
} tDiag;                                   /* complete diagram description */



/* values of diagram options */
#define DIAG_YAUTORANGE      0x01   /* set if the y-axis will be auto scaled */
#define DIAG_DISCRET         0x02   /* discret samples */
#define DIAG_NOSCALE         0x04   /* only curve will be paint */
#define DIAG_CONST_SAMPLES   0x10   /* number of x samples is constant,
                                     * independent of window size
                                     * (not implemented) */




BOOL   PaintDiag(HINSTANCE hInst, HDC dc, FUNCUSERBREAK fnUserBreak,
                 tDiag *lpDiag, COLORREF aColors[]);

#define IDIAGCOL_SCALENUM       0  /* scaling numbers */
#define IDIAGCOL_SCALELINE      1  /* dotted scale lines */
#define IDIAGCOL_CURVE          2  /* curve */
#define IDIAGCOL_FRAME          3  /* frame of diag */
#define IDIAGCOL_NOTETEXT       4 /* comments/notes text */
#define IDIAGCOL_NOTEFRAME      5 /* comments/notes frame */
#define IDIAGCOL_AXISNAME       6  /* axis name and unit */

#define SIZE_COLOR_ARR          7
    
double GetScaleFactor(tDiagAxis *);
double GetWorldXY(tDiagAxis *, double);
BOOL GetWorldYByX(tDiag *pDiag, double *x, double x_delta, double *py);
COORDINATE GetDevXY(tDiagAxis *, double);



#define __DIAGS_H
#endif

