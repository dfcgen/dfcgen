
#ifndef __FDTYPES_H

#include "FDMATH.H"
#include "DIAGS.H"

typedef enum {RECT_WIN, HAMMING_WIN, HANNING_WIN, BLACKMAN_WIN,
              KAISER_WIN} tDataWin;
typedef enum {NOFTR, HIGHPASS, BANDPASS, BANDSTOP} tTransform;
typedef enum {BILINEAR, EULER_FORWARD, EULER_BACKWARD} t_S_Z_Transf;
typedef enum {USER_IN_FUNCTION, UNIT_STEP, DIRAC_IMPULSE} tFilterInput;

typedef enum        /* general filter types (important for implementation) */
{
    NOTDEF = 0,                         /* flag (no filter defined/loaded) */
    LINFIR = 1,
    IIR = 2,
    FIR = 3
} tGenType;


typedef enum                                  /* filter definition dialogs */
{
    LINFIRDLG = 1,   /* same code like LINFIR */
    STDIIRDLG = 2,
    PREDEFDLG = 3,
    MISCDLG = 4,
} tFltDlg;


typedef enum   /* Lin. FIR-Filter dialog subtypes */
{
    RECT_LP,  /* rectangular lowpass */
    COS_LP,  /* cosinus lowpass */
    COS2_LP,  /* square cosinus lowpass */
    GAUSS_LP,  /* gaussian lowpass */
    SQR_LP
} tLinFIRSubType;

typedef enum          /* standard IIR-filter approximation dialog subtypes */
{             /* don't change the enum-values, because used as table index */
    BUTTERWORTH = 0,       /* power filters (max. flat magnitude response) */
    CHEBY1 = 1,                  /* T1 case (design by ripple attenuation) */
    CHEBY2 = 2,            /* T2 case (design by stopband min. attenuation */
    CAUER1 = 3,               /* Cauer filter design by ripple attenuation */
    CAUER2 = 4,        /* Cauer filter design by stopband min. attenuation */
    BESSEL = 5                                    /* max. flat group delay */
} tStandard;


typedef enum
{
    HILBERT_TRANSF90 = 0,                           /* hilbert transformer */
    INTEGR_FSDEV = 1,        /* integrator (deriation by Fourier sequence) */
    DIFF_FSDEV = 2,      /* differantiator (deriation by Fourier sequence) */
    COMB = 3,                                /* comb filter with parameter */
    AVG_FIR = 4,                  /* moving average filter (non-recursive) */
    AVG_IIR = 5,                      /* moving average filter (recursive) */
    AVG_EXP = 6                              /* exponential average filter */
} tMiscDigSys;


/* frequency transform data user input in dialog */
typedef struct
{
    tTransform FTransf;                   /* frequency transformation type */
    short iDefaultUnit;                  /* default unit index in combobox */
    unsigned uFlags;                            /* special flags see below */
    double dCenter;                    /* center frequency (only BP or BS) */
    double dCutFBw;        /* bandwidth (BP, BS) or cuttoff (HP) frequency */
} tTransformData;

#define CENTER_GEOMETRIC        0x01   /* center frequency (BP, BS) is geometric
                                          average of cutoff frequencys
                                          sqrt(fg1*fg2) = fm, against the normal
                                          case (fg1+fg2)/2 = fm (arithmetic
                                          average) */


typedef struct                        /* special LIN FIR Dialog parameters */
{
    int            nOrder;    /* design order before checking implementation, i.e.
                               * order of denominator poly after frequency or
                               * domain transformation */
    tLinFIRSubType SubType;    /* filter (curve) subtype in design phase */
    tDataWin       DataWin;    /* type of window used to clip the impulse response */
    double         dCutoff;    /* Target cutoff frequency of response lowpass in
                                * design phase (equal to arithmetric center
                                * frequency of BP, BS) */
    double         dAlpha;     /* only if Kaiser-Window as DesignDataWin */
} tLinFIR;


typedef struct
{
    int nOrder; /* design order before checking implementation, i.e. order of
                   denominator poly after frequency or domain transformation */
    tStandard SubType;           /* filter (curve) subtype in design phase */
    double dCutoff;   /* Target cutoff frequency of LP in design phase (equal
                         to geometric center f for BP) */
    double dRippleAtt; /* ripple attenuation for elliptic filters (CHEBY1, CAUER) */
    double dMinAtt; /* min attenuation for elliptic filters (CHEBY2, CAUER) */
    double dModuleAngle;  /* module angle for Cauer filters 1.0° ... 80.0° */
    t_S_Z_Transf SToZTransf; /* type of domain transform (not implemented) */
} tStdIIR;



/* misc predefined digital systems with parameters (max. 2) */

#define MAXMISCSYSPARAM 2

typedef struct
{
    tMiscDigSys SubType;                /* subtype type of digital system */
    int nOrder;                        /* definition characteristic order */
    double adParam[MAXMISCSYSPARAM];        /* special params */
    short iUnit[MAXMISCSYSPARAM];          /* only if unit combobox exist */
} tVarSys;



/* all supported digital systems design data struct */
typedef union
{
    tLinFIR    FIRDat;                /* dialog parameters for FIR-Filters */
    tStdIIR    StdIIRDat;    /* dialog parameters for standard IIR-Filters */
    tVarSys   MiscFltDat;    /* dialog parameters for misc FIR/IIR-Filters */
    int PredefSub;  /* predefined special filters (resource ID in RC-file) */
} tDesignData;



#define SIZE_PRJNAME            64
#define SIZE_PRJDESC            1024 /* max. size of project description string */


/* filter description struct */
typedef struct
{
    tGenType f_type;   /* general filter type (filled by filter def. func) */
    /* -----------------    * next items will be filled by dialog function */
    tFltDlg  FltDlg;                      /* filter definition dialog type */
    tDesignData DefDlg;   /* remaining filter definition dialog parameters */
    short iInputUnitF;   /* input frequency unit index (mHz, Hz, kHz, ...) */
    tTransformData FTr;                   /* frequency transformation data */
    /* ----------------- * end of dialog definition part in this structure */
    PolyDat a;                          /* polynomial zero data (z-Domain) */
    PolyDat b;                          /* polynomial pole data (z-Domain) */
    double factor;           /* transfer function multiplicator (not used) */
    double f0;           /* sampling frequency (only discret time filters) */
    char szPrjName[SIZE_PRJNAME];                /* project name or number */
    char *szPrjDesc;       /* user filter description (malloc mem space !) */
    unsigned uFlags;                            /* special flags see below */
} tFilter;

/* filter desc flags */
#define FILTER_SAVED            0x01
#define FILTER_TYPE_INVALID     0x02
#define FILTER_ROOTS_VALID      0x04


/* diagram Id's to identify the type of diag (use as index for diag info
   table) */
typedef enum
{
    DIAG_F_RESPONSE  = 0,             /* frequency response diagram linear */
    DIAG_ATTENUATION = 1,                           /* attenuation diagram */
    DIAG_PHASE       = 2,                                 /* phase diagram */
    DIAG_PHASE_DELAY = 3,                           /* phase delay diagram */
    DIAG_GROUP_DELAY = 4,                           /* group delay diagram */
    DIAG_I_RESPONSE  = 5,                      /* impulse response diagram */
    DIAG_STEP_RESP   = 6,                         /* step response diagram */
    DIAG_APPROXCHARAC_RESP = 7,          /* characteristic filter function */
    DIAG_USER_DEF    = 8,                        /* user defined functions */
} tFdDiags;


#define NO_OF_DIAGS             9                  /* number of diag kinds */



/* axis Id's to identify the type of (use as index for axis info
   table) */
typedef enum
{
	AXIS_FREQUENCY    = 0,                                /* frequency (x) */
	AXIS_TIME         = 1,                                     /* time (x) */
	AXIS_X            = 2,            /* general x-axis for user def. diag */
	AXIS_MAGNITUDE    = 3,                           /* lin. magnitude (y) */
	AXIS_ATTENUATION  = 4,                              /* attenuation (y) */
	AXIS_IMPULS_RESP  = 5,                  /* discret impuls response (y) */
	AXIS_STEP_RESP    = 6,                    /* discret step response (y) */
	AXIS_PHASE        = 7,                           /* phase response (y) */
	AXIS_PHASEDELAY   = 8,                              /* phase delay (y) */
	AXIS_GROUPDELAY   = 9,                              /* group delay (y) */
    AXIS_APPROXCHARAC = 10,                 /* characteristic function (y) */
    AXIS_Y            = 11,           /* general y-axis for user def. diag */
} tFdAxis;


/* complete filter designer axis dialog description */
typedef struct {
    int    nIddAxisName;         /* ID of the title string in string table */
    double MinLimitInput;
    double MaxLimitInput;
} tAxisDesc;


/* complete diagram type description */
typedef struct {
    tFdAxis  nIdAxisX;                           /* type (index) of x-axis */
    tFdAxis  nIdAxisY;                           /* type (index) of y-axis */
    int nIddDiagName;            /* ID of the title string in string table */
    LPCSTR szIconName;
    tDiag DefaultDiag;                                       /* start data */
} tDiagDesc;


/* passed if creating a new diag */
typedef struct {
    tDiag *lpDiag;
    tFdDiags type;
} DIAGCREATESTRUCT;


/* passed if creating the axis notes dialog */
typedef struct {
    HWND hwndDiag; /* this is the diag window, where are the notes located */
    double PosX;           /* position of click point in world coordinates */
} CREATEAXISNOTEDLGSTRUCT;/* set 'PosX' to MAXDOUBLE isn't a note selected */

#define __FDTYPES_H
#endif

