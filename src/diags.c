/* 2D diagram plot function PaintDiag() (not rentrant!) *
 * running on Windows 3.1x  *
 * author  : Ralf Hoppe      *
 */

#include "DIAGS.H"


#ifndef ODD
#define ODD(number)            ((((number)>>1)<<1) != number)
#endif

#define POWER10(val)           exp(M_LN10*(val))
#define DIM(array)             (sizeof(array)/sizeof(array[0]))
#define CLIPVAL(val, bound_min, bound_max)                              \
                               {if ((val)<(bound_min)) val = bound_min; \
                                if ((val)>(bound_max)) val = bound_max;}
#define LINEHEIGHT(cyChar)     ((cyChar)+(cyChar)/4)

#define MAXSCALEPOINTS 16                   /* max number of scale points */
#define NUMCHARSCALESTR 16       /* max. number of chars for scale string */
#define FORMAT_DOUBLE "%.*lG"                 /* any double output format */


typedef struct {
    char szScaleStr[NUMCHARSCALESTR];
    int cStrLen;
    COORDINATE nLinePos;
    RECT rectStringBox;
} tScaleStrDesc;



/* prototypes of local functions */
static void SetDevToWorldRatio(tDiagAxis *);
static BOOL CheckWorldRanges(tDiagAxis *);
static BOOL InWorldRange(tDiag *pDiag, double WorldX, double WorldY);
static int FloatFormatCharMax(short precision);
static char  *GetScaleName(HINSTANCE, tDiagAxis *);
static int GetScaleStrings(HDC, tDiagAxis *, tScaleStrDesc *, int, BOOL, int *);
static int GetScaleStringsLog(HDC, tDiagAxis *, tScaleStrDesc *, int, BOOL, int *);
static int GetScaleStringsLin(HDC, tDiagAxis *, tScaleStrDesc *, int, BOOL, int *);
static BOOL SetScaleStrOvr(HDC, tDiagAxis *, double, tScaleStrDesc *, int,
                           BOOL, int *, int *);
static double GetNextHumanVal(double);
static void ExchangeInt(int *, int *);
static void ExchangeCoordinate(COORDINATE *, COORDINATE *);
static void EndGetting(tDiag *p);
static BOOL ChkBreakCond(tDiag *p, FUNCUSERBREAK fnUserBreak);
static BOOL DummyBreakProc(void);
static void FrameRgnSolid(HDC dc, HRGN hrgnFrame, COLORREF Color);
static void FrameRectSolid(HDC dc, LPRECT rcRect, COLORREF Color);
static void FillRgnSolid(HDC dc, HRGN hrgnFill, COLORREF Color);



static void ExchangeInt(int *lpPar1, int *lpPar2)
{
    int nHelp = *lpPar1;
    *lpPar1 = *lpPar2;
    *lpPar2 = nHelp;
}


static void ExchangeCoordinate(COORDINATE *lpPar1, COORDINATE *lpPar2)
{
    COORDINATE nHelp = *lpPar1;
    *lpPar1 = *lpPar2;
    *lpPar2 = nHelp;
}


static void EndGetting(tDiag *p)
{
    if (p->EndGetY != NULL) p->EndGetY(p->pAppData);           /* free allocated mem */
} /* EndGetting() */


static BOOL ChkBreakCond(tDiag *p, FUNCUSERBREAK fnUserBreak)
{
    if (fnUserBreak())
    {
        EndGetting(p);
        return TRUE;
    } /* if */

    return FALSE;
} /* ChkBreakCond() */



double GetWorldXY(tDiagAxis *diax, double dExactScreenCoord)
{
    double dArg = (dExactScreenCoord-diax->nScreenMin)/diax->dDevToWorldRatio;

    /* at first check the equation :  WorldMin < World < WorldMax */

    if (diax->AxisFlags & AXIS_LOG)                         /* log. axis ! */
    {
        /* WorldMin < WorldMin*10^((Screen - ScreenMin)/DeltaRatio) < WorldMax
           for WorldMin :   0 < Screen - ScreenMin
           for WorldMax :   (Screen - ScreenMin)/DeltaRatio < log(WorldMax/WorldMin)
        */
        CLIPVAL(dArg, 0.0, log10(diax->dWorldMax/diax->dWorldMin));
        return diax->dWorldMin*POWER10(dArg);
    } /* if */

    return diax->dWorldMin + dArg;
} /* GetWorldXY() */



COORDINATE GetDevXY(tDiagAxis *diax, double dWorldCoord)
{
    if (diax->AxisFlags & AXIS_LOG)
        return ROUND(diax->nScreenMin+diax->dDevToWorldRatio*
                     log10(dWorldCoord/diax->dWorldMin));

    return ROUND((dWorldCoord-diax->dWorldMin)*diax->dDevToWorldRatio+
                 diax->nScreenMin);
} /* GetDevXY */


static void SetDevToWorldRatio(tDiagAxis *lpAxis)
{
    if (lpAxis->AxisFlags & AXIS_LOG)
        lpAxis->dDevToWorldRatio = (lpAxis->nScreenMax-lpAxis->nScreenMin)/
                                log10(lpAxis->dWorldMax/lpAxis->dWorldMin);
    else
        lpAxis->dDevToWorldRatio = (lpAxis->nScreenMax-lpAxis->nScreenMin)/
                                (lpAxis->dWorldMax-lpAxis->dWorldMin);
} /* SetDevToWorldRatio */


/* checks world coordinate is in world coordinates range */
static BOOL InWorldRange(tDiag *pDiag, double WorldX, double WorldY)
{
    return ((WorldX >= pDiag->X.dWorldMin) &&
            (WorldX <= pDiag->X.dWorldMax) &&
            (WorldY >= pDiag->Y.dWorldMin) &&
            (WorldY <= pDiag->Y.dWorldMax));            /* out of range ? */
} /* InWorldRange */



/* checks axis bounds against some predefined limits
 * if the range (min, max) doesn't match with these limits the function
   changes the range automatically and returns FALSE
 * if all bounds in range it returns TRUE
 */
static BOOL CheckWorldRanges(tDiagAxis *lpAxis)
{
    double dMin = lpAxis->dWorldMin;
    double dMax = lpAxis->dWorldMax;

    if (lpAxis->AxisFlags & AXIS_LOG)                 /* logarithmic axis */
    {
        CLIPVAL(dMin, WORLD_MIN_LOG, WORLD_MAX);     /* clip */
        CLIPVAL(dMax, WORLD_MIN_LOG, WORLD_MAX); /* absolute */

        if (dMin/dMax < WORLD_RATIO_LOG) dMin = dMax*WORLD_RATIO_LOG;
        if (dMax/dMin < 1.0 + WORLD_RATIO_LOG)
            dMax = dMin * (1.0 + WORLD_RATIO_LOG);
    } /* if */
    else                                                   /* linear axis */
    {
        double MinDiff = WORLD_DIFF*GetScaleFactor(lpAxis);

        CLIPVAL(dMin, WORLD_MIN, WORLD_MAX);/* clip to upper */
        CLIPVAL(dMax, WORLD_MIN, WORLD_MAX); /* and lower bound */

        if (dMax-dMin < MinDiff) dMax = dMin + MinDiff;
    } /* else */


    if ((dMin != lpAxis->dWorldMin) || (dMax != lpAxis->dWorldMax))
    {
        lpAxis->dWorldMin = dMin;
        lpAxis->dWorldMax = dMax;
        return FALSE;
    } /* if */

    return TRUE;
} /* CheckWorldRanges() */



/* returns TRUE if a y-value exist in range [x-x_delta/2, x+x_delta/2]
 * if x_delta is too small at discret diags extends the width x_delta
   to satisfy a minimum difference
 * in discret diags the value x can be modified to match with the
   sample points (distance/frequency)
 */ 
BOOL GetWorldYByX(tDiag *pDiag, double *x, double x_delta, double *py)
{
    int cSamples = 1;

    if ((pDiag->nDiagOpt & DIAG_DISCRET) &&
        (x_delta < fabs(*x)*FLT_EPSILON))

        x_delta = fabs(*x)*FLT_EPSILON;

    if (pDiag->InitGetY != NULL)
        cSamples = pDiag->InitGetY(*x - 0.5*x_delta, *x + 0.5*x_delta,
                                   &pDiag->pAppData);

    switch (cSamples)
    {
        case -1 : break;    /* error in 'InitGetY' (don't call 'EndGetY') */

        default : if (!pDiag->GetNextWorldY(x, py, pDiag->pAppData))
                      cSamples = -1;              
                                                      /* and fall through */
        case 0  : EndGetting(pDiag);            /* 0 = no samples in area */
                  break; /* 0, default */
    } /* switch */

    return (cSamples > 0);                          /* y in world exist ? */
} /* GetWorldYByX */


/* returns the current factor for displaying the correct values in user unit */
double GetScaleFactor(tDiagAxis *pAxis)
{
    double dOptUnitFactor = 1.0;
    if (pAxis->lpUnitList != NULL)
        dOptUnitFactor = pAxis->lpUnitList[pAxis->iCurrUnit].dUnitFactor;
    return dOptUnitFactor;
} /* GetScaleFactor */


/* returns a pointer to the scale/unit name */
static char *GetScaleName(HINSTANCE hInst, tDiagAxis *lpAxis)
{
    static char szName[40];
    LoadString(hInst, lpAxis->nIdAxisName, szName, DIM(szName));
    if (lpAxis->lpUnitList != NULL)                     /* definition made ? */
    {
        lstrcat(szName, " [");
        lstrcat(szName, lpAxis->lpUnitList[lpAxis->iCurrUnit].szUnit);
        lstrcat(szName, "]");
    } /* if */
    return szName;
} /* GetScaleName() */


static double GetNextHumanVal(double dVal)
{
    static double adLinDelta[] = {0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0, 10.0};
    int i;
    int nExp;
    double dMantissa;

    dVal = log10(fabs(dVal));
    nExp = (int) dVal;

    dMantissa = POWER10(dVal-nExp);
    /* and now search the next mantissa in table which is greather */
    for (i=0; i < DIM(adLinDelta); i++)
        if (adLinDelta[i] >= dMantissa) break;

    dVal = adLinDelta[i] * pow10(nExp);
    return dVal;
}

/* returns FALSE if the new string overwrite the previous string
   at axis scaling 
 */
static BOOL SetScaleStrOvr(HDC hdc, tDiagAxis *diax, double dCurrCoord,
                           tScaleStrDesc lpaDesc[], int nMinDistance,
                           BOOL bVSpacing, int *cxMaxStr, int *cEntrys)
{
    tScaleStrDesc *npStrDesc;  /* tmp pointer to current scale string desc */
    SIZE cxcyStr;               /* scale string width/height in pixel */
    RECT rectDummy;              /* dummy parameter for intersect */
    BOOL bOverlapped = FALSE;

    nMinDistance = nMinDistance/2+1;
    npStrDesc = &lpaDesc[*cEntrys];     /* adr of curr description */
    npStrDesc->nLinePos = GetDevXY(diax, dCurrCoord);
    if ((dCurrCoord >= diax->dWorldMin-DBL_EPSILON) &&
        (dCurrCoord <= diax->dWorldMax+DBL_EPSILON))
    {
        if (fabs(dCurrCoord) < WORLD_NULL) dCurrCoord = 0.0;
        sprintf(npStrDesc->szScaleStr, FORMAT_DOUBLE, (int)(diax->sPrecision),
                dCurrCoord/GetScaleFactor(diax));
        npStrDesc->cStrLen = lstrlen(npStrDesc->szScaleStr);
        SetRectEmpty(&npStrDesc->rectStringBox);

        GetTextExtentPoint(hdc, npStrDesc->szScaleStr,     /* only win 3.1 */
                           npStrDesc->cStrLen, &cxcyStr);
        *cxMaxStr = max(*cxMaxStr, cxcyStr.cx);
        npStrDesc->rectStringBox.left = npStrDesc->nLinePos;
        if (bVSpacing)
            ExchangeInt(&npStrDesc->rectStringBox.left, &npStrDesc->rectStringBox.top);

        npStrDesc->rectStringBox.bottom = npStrDesc->rectStringBox.top+cxcyStr.cy;
        npStrDesc->rectStringBox.right = npStrDesc->rectStringBox.left+cxcyStr.cx;
        InflateRect(&npStrDesc->rectStringBox, nMinDistance, nMinDistance);

        if (*cEntrys > 0)  /* check free space to last scale strings */
            bOverlapped = IntersectRect(&rectDummy,
                                        &lpaDesc[(*cEntrys)-1].rectStringBox,
                                        &npStrDesc->rectStringBox);
        ++(*cEntrys);
    } /* if */

    return bOverlapped;
}


static int GetScaleStringsLog(HDC hdc, tDiagAxis *diax, tScaleStrDesc lpaDesc[],
                              int nMinDistance, BOOL bVSpacing, int *cxMaxStr)
{
    static int cPossible[9] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    static double DecScale[9][MAXSCALEPOINTS] =
    {
        {1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 8.0},
        {1.0, 1.25, 1.75, 2.5, 3.0, 4.0, 5.5, 7.5},
        {1.0, 1.5, 2.0, 2.75, 3.75, 5.0, 7.0},
        {1.0, 1.5, 2.25, 3.5, 5.0, 7.5},
        {1.0, 1.5, 2.5, 4.0, 6.0},
        {1.0, 1.75, 3.0, 6.0},
        {1.0, 2.0, 5.0},
        {1.0, 3.0},
        {1.0}
    };

    double dCurrCoord;     /* current world coordinate for virtual scaling */
    int iScale = 0;        /* start with index 0 */
    int iCoord, nExp;
    int cEntrys;                 /* (current) number of entrys in string tab */
    BOOL bOverlapped;            /* TRUE if two scale strings to near */


    do
    {
        nExp = (int)floor(log10(fabs(diax->dWorldMin))); /* reset exponent */
        iCoord = 0;       /* start with first coordinate */
        cEntrys = 0;      /* next try */
        *cxMaxStr = 0;
        bOverlapped = FALSE;

        dCurrCoord = -MAXDOUBLE;

        /* check the distance between strings and decrement cTargetScaleStr
           if only two of all strings to near (overlapp) */

        while ((dCurrCoord <= diax->dWorldMax) && !bOverlapped &&
               (cEntrys < MAXSCALEPOINTS))
        {
            dCurrCoord = DecScale[iScale][iCoord] * pow10(nExp);
            bOverlapped = SetScaleStrOvr(hdc, diax, dCurrCoord, lpaDesc,
                                         nMinDistance, bVSpacing, cxMaxStr, &cEntrys);
            if (++iCoord == cPossible[iScale])            /* next decade */
            {
                ++nExp;
                iCoord = 0;      /* start with first value in new decade */
            } /* if */
        } /* while */

        ++iScale;
    } /* do */
    while (bOverlapped && (iScale < DIM(cPossible)));

    if ((cEntrys <= 0) ||        /* true number of string entrys in array */
        (iScale > DIM(cPossible)) ||          /* no more scaling variants */
        bOverlapped) return 0;

    return cEntrys;
}


static int GetScaleStringsLin(HDC hdc, tDiagAxis *diax, tScaleStrDesc lpaDesc[],
                            int nMinDistance, BOOL bVSpacing, int *cxMaxStr)
{
    double dDeltaC; /* distance between two scale points in world coordinates */
    double dCurrCoord;     /* current world coordinate for virtual scaling */
    int cTargetScaleStr = MAXSCALEPOINTS;/* target number of scale strings */
    int cEntrys;               /* (current) number of entrys in string tab */
    BOOL bOverlapped;                 /* TRUE if two scale strings to near */

    do
    {
        dDeltaC = GetNextHumanVal((diax->dWorldMax-diax->dWorldMin)/cTargetScaleStr);
        dCurrCoord = floor(diax->dWorldMin/dDeltaC) * dDeltaC;

        /* check valid values if linear scaling of logarithmic diags
         * this can be a problem because 'floor' truncates an the current
           coordinate may be less then min. world value
         */
        if (diax->AxisFlags & AXIS_LOG)
            if (dCurrCoord < diax->dWorldMin) dCurrCoord += dDeltaC;

        cEntrys = 0;
        *cxMaxStr = 0;
        bOverlapped = FALSE;

        /* check the distance between strings and decrement cTargetScaleStr
           if only two of all strings to near (overlapp) */

        while ((dCurrCoord <= diax->dWorldMax) && !bOverlapped &&
               (cEntrys < MAXSCALEPOINTS))
        {
            bOverlapped = SetScaleStrOvr(hdc, diax, dCurrCoord, lpaDesc,
                                         nMinDistance, bVSpacing, cxMaxStr,
                                         &cEntrys);
            dCurrCoord += dDeltaC;
        } /* while */

        --cTargetScaleStr;
    } /* do */
    while (bOverlapped && (cTargetScaleStr > 0));

    if ((cEntrys <= 0) ||         /* true number of string entrys in array */
        (cTargetScaleStr < 0) ||  /* target number of scale strings */
        bOverlapped) return 0;

    return cEntrys;
}


static int GetScaleStrings(HDC hdc, tDiagAxis *diax, tScaleStrDesc lpaDesc[],
                           int nMinDistance, BOOL bVSpacing, int *cxMaxStr)
{
    int cStr;
    if (diax->AxisFlags & AXIS_LOG)
    {
        cStr = GetScaleStringsLog(hdc, diax, lpaDesc, nMinDistance, bVSpacing,
                                  cxMaxStr);   /* log scaling success ? */
        if (cStr > 1) return cStr;             /* else try lin. scaling */
    } /* if */

    return GetScaleStringsLin(hdc, diax, lpaDesc, nMinDistance, bVSpacing,
                              cxMaxStr);
} /* GetScaleStrings */



/* returns the max. count of chars in G format at passed precision
 */
static int FloatFormatCharMax(short precision)
{
    /* E format: [-] d.dddd E [+|-] ddd
     * F format: [-] dddd.dddd

     * if count of characters in F format exceeds the precision the E format
       will be used from printf
     * or if the count of leading zeros excceeds 4.
     */  
    return max(precision+2, 7+min(precision, 2));
}


static BOOL DummyBreakProc()
{
    return FALSE;
}


static void FrameRectSolid(HDC dc, LPRECT pRc, COLORREF Color)
{

    HBRUSH hbrFrame = CreateSolidBrush(GetNearestColor(dc, Color));
    FrameRect(dc, pRc, hbrFrame);
    DeleteBrush(hbrFrame);
} /* FrameRectSolid() */


static void FrameRgnSolid(HDC dc, HRGN hrgnFrame, COLORREF Color)
{
    HBRUSH hbrFrame = CreateSolidBrush(GetNearestColor(dc, Color));
    FrameRgn(dc, hrgnFrame, hbrFrame, 1, 1);
    DeleteBrush(hbrFrame);
} /* FrameRgnSolid() */




static void FillRgnSolid(HDC dc, HRGN hrgnFill, COLORREF Color)
{
             /* set correct color without brush dithering technique */
    HBRUSH hbrFill = CreateSolidBrush(GetNearestColor(dc, Color));
    FillRgn(dc, hrgnFill, hbrFill);
    DeleteBrush(hbrFill);
} /* FillRgnSolid */


BOOL PaintDiag(HINSTANCE hInst, HDC dc, FUNCUSERBREAK fnUserBreak,
               tDiag *lpDiag, COLORREF aColors[])
/*
    DeltaRatio = (ScreenMax-ScreenMin)/(WorldMax-WorldMin)      (I)
    Screen     = DeltaRatio*(World-WorldMin) + ScreenMin        (II)
    World      = (Screen - ScreenMin) / DeltaRatio + WorldMin   (III)

    if LOGAXIS (logarithmic axis) then World := log(World) before using
    of equation I,II,III what means :
    DeltaRatio = (ScreenMax-ScreenMin)/(log(WorldMax)-log(WorldMin))
               = (ScreenMax-ScreenMin)/(log(WorldMax/WorldMin))
    Screen     = DeltaRatio*(log(World) - log(WorldMin)) + ScreenMin
               = DeltaRatio*log(World/WorldMin) + ScreenMin
    World      = 10^(log(WorldMin) + (Screen - ScreenMin)/DeltaRatio))
               = WorldMin*10^((Screen - ScreenMin)/DeltaRatio)
*/

{
    tDiagAxis *lpAxisX = &lpDiag->X;
    tDiagAxis *lpAxisY = &lpDiag->Y;        /* pointer to axis description */
    tScaleStrDesc aStrDescX[MAXSCALEPOINTS];    /* array for scale strings */
    tScaleStrDesc aStrDescY[MAXSCALEPOINTS];    /* array for scale strings */
    COORDINATE nX;                                 /* the count variable X */
    int cSamples;              /* to open the "world value get" - function */
    BOOL bIsOutSide;
    double dDeltaPix, dXPix;
    double XWorld, YWorld;             /* world coordinates in calculation */
    int cEntrysX, cEntrysY;          /* item counter of scale string array */
    TEXTMETRIC tm;             /* textmetric description in device context */
    int cyChar;                   /* height of a character in current font */
    pMarker pNote;                /* pointer to current painting diag note */
    int cxMaxScaleStr;            /* max pixel width of axis scale strings */
    COORDINATE x1, x2;                 /* diag start/end coordinate x-axis */
    HPEN hpenOld, hp;
    RECT rcPaintSector;


    if (fnUserBreak == NULL) fnUserBreak = DummyBreakProc;
    if (lpDiag->hrgnCurve != HRGN_NULL)
    {
        DeleteRgn(lpDiag->hrgnCurve);    /* destroy old region define */
        lpDiag->hrgnCurve = HRGN_NULL;
    } /* if */

    (void)CheckWorldRanges(lpAxisY);
    (void)CheckWorldRanges(lpAxisX);

    GetTextMetrics(dc, &tm);
    cyChar = tm.tmHeight+tm.tmExternalLeading;

    x2 = lpAxisX->nScreenMax;   /* save max and min position for any paint */
    x1 = lpAxisX->nScreenMin;

//    lpAxisX->nScreenMax -= tm.tmAveCharWidth*(4+lpAxisX->sPrecision)/2 + 1;
    lpAxisX->nScreenMax -= tm.tmAveCharWidth*FloatFormatCharMax(lpAxisX->sPrecision)/2+1;

    lpAxisY->nScreenMin += LINEHEIGHT(cyChar); /* (possible) top scale num */
    if (lstrlen(GetScaleName(hInst, lpAxisY)) > 0)    /* scalename exist ? */
        lpAxisY->nScreenMin += LINEHEIGHT(cyChar); /* add scalename height */

    lpAxisY->nScreenMax -= LINEHEIGHT(cyChar);          /* x-scale numbers */
    if (lstrlen(GetScaleName(hInst, lpAxisX)) > 0)
        lpAxisY->nScreenMax -= LINEHEIGHT(cyChar);   /* x-axis name height */

    if (lpAxisY->nScreenMin >= lpAxisY->nScreenMax) return TRUE;

    /* calculate for X only temp, changes later */
    SetDevToWorldRatio(lpAxisX);    /* calculate ratio screen to world corrd's */

    bIsOutSide = lpDiag->nDiagOpt & DIAG_YAUTORANGE;   /* flag auto range */

    /* search max/min value if AUTO ranging of y-axis */
    while (bIsOutSide)       /* doit max twice (two loops) if log. y-axis */
    {                                         /* if auto-range in options */
        double YMin = WORLD_MAX;
        double YMax = WORLD_MIN;

        cSamples = 1;
        if (lpDiag->InitGetY != NULL)
            cSamples = lpDiag->InitGetY(lpAxisX->dWorldMin,
                                        lpAxisX->dWorldMax, &lpDiag->pAppData);
        if (cSamples < 0) return FALSE;                          /* error */

        if (lpDiag->nDiagOpt & DIAG_DISCRET)        /* discret sample diag */
        {
            for (nX = 0; nX < cSamples; nX++)
            {
                XWorld = GetWorldXY(lpAxisX, nX);
                if (lpDiag->GetNextWorldY(&XWorld, &YWorld, lpDiag->pAppData))
                {                                /* only if y value exist */
                    YMin = min(YMin, YWorld);
                    YMax = max(YMax, YWorld);
                } /* if */

                if (ChkBreakCond(lpDiag, fnUserBreak))
                    return TRUE;  /* chk user break */
            } /* for */
        } /* if DIAG_DISCRET */

        else                                         /* continous 2D diag */
        {
            if (lpDiag->nDiagOpt & DIAG_CONST_SAMPLES)
                dDeltaPix = (double)(lpAxisX->nScreenMax - lpAxisX->nScreenMin)/
                            lpDiag->nConstSmpl;
            else dDeltaPix = 1.0;                      /* steps of one pixel */

            for (dXPix = lpAxisX->nScreenMin; dXPix <= lpAxisX->nScreenMax;
                 dXPix += dDeltaPix)
            {
                XWorld = GetWorldXY(lpAxisX, dXPix);
                if (lpDiag->GetNextWorldY(&XWorld, &YWorld, lpDiag->pAppData))
                {
                    YMin = min(YMin, YWorld);
                    YMax = max(YMax, YWorld);
                } /* if */

                if (ChkBreakCond(lpDiag, fnUserBreak))
                    return TRUE;  /* chk user break */
            } /* for */
        } /* else */


        EndGetting(lpDiag);

        if (YMin < YMax)                            /* any valid values ? */
        {
            lpAxisY->dWorldMin = YMin;
            lpAxisY->dWorldMax = YMax;
        } /* if */

        bIsOutSide = !CheckWorldRanges(lpAxisY);/* TRUE if bounds changed */

        if (lpAxisY->AxisFlags & AXIS_LOG)
        {
            if (bIsOutSide) lpAxisY->AxisFlags &= ~AXIS_LOG;/* change lin */
        } /* if */
        else bIsOutSide = FALSE;
    } /* while */

    /* Attention : changing the y-min and max values for equal handling
                   of X- and Y-axis */
    ExchangeCoordinate(&lpAxisY->nScreenMin, &lpAxisY->nScreenMax);
    SetDevToWorldRatio(lpAxisY);    /* calculate ratio screen to world corrd's */

    cEntrysY = GetScaleStrings(dc, lpAxisY, aStrDescY, tm.tmHeight, TRUE, &cxMaxScaleStr);

    lpAxisX->nScreenMin += 2*tm.tmAveCharWidth + cxMaxScaleStr;
    if (lpAxisX->nScreenMax <= lpAxisX->nScreenMin) return TRUE;

    /* calculate true x-ratio (new) */
    SetDevToWorldRatio(lpAxisX);    /* calculate ratio screen to world corrd's */

    lpDiag->hrgnCurve =
        CreateRectRgn(lpAxisX->nScreenMin, lpAxisY->nScreenMax,
                      lpAxisX->nScreenMax, lpAxisY->nScreenMin);

    /**** from this point painting begins */

    SaveDC(dc);           /* because changing much device context objects */
    SetBkMode(dc, TRANSPARENT);
    SetROP2(dc, R2_COPYPEN);

    if (!(lpDiag->nDiagOpt & DIAG_NOSCALE))         /* scaling required ? */
    {
        /* at first draw the axis names and current units in top/right area
           of diag
         */
        int cLen;
        char szBuffer[256];

        SetTextColor(dc, aColors[IDIAGCOL_AXISNAME]); /* axis name & unit */
        SetTextAlign(dc, TA_NOUPDATECP|TA_LEFT|TA_BOTTOM);
        cLen = wsprintf(szBuffer, "%s", GetScaleName(hInst, lpAxisY));
        TextOut(dc, x1+tm.tmAveCharWidth, lpAxisY->nScreenMax-tm.tmHeight, szBuffer, cLen);

        cLen = wsprintf(szBuffer, "%s", GetScaleName(hInst, lpAxisX));
        SetTextAlign(dc, TA_NOUPDATECP|TA_RIGHT|TA_TOP);
        TextOut(dc, x2-tm.tmAveCharWidth,
                lpAxisY->nScreenMin+LINEHEIGHT(cyChar), szBuffer, cLen);

        /* calc axis scaling and draw all axis strings */

        SetTextColor(dc, aColors[IDIAGCOL_SCALENUM]);       /* scale numbers */
        SetTextAlign(dc, TA_RIGHT|TA_BASELINE);

        for (nX = 0; nX < cEntrysY; nX++)                 /* x-axis scale */
            TextOut(dc, lpAxisX->nScreenMin-tm.tmAveCharWidth,
                    aStrDescY[nX].nLinePos, aStrDescY[nX].szScaleStr,
                    aStrDescY[nX].cStrLen);

        cEntrysX = GetScaleStrings(dc, lpAxisX, aStrDescX, 2*tm.tmAveCharWidth,
                                   FALSE, &cxMaxScaleStr);

        SetTextAlign(dc, TA_CENTER|TA_BOTTOM);

        for (nX = 0; nX < cEntrysX; nX++)   /* paint x axis scale strings */
            TextOut(dc, aStrDescX[nX].nLinePos,
                    lpAxisY->nScreenMin+LINEHEIGHT(cyChar),
                    aStrDescX[nX].szScaleStr, aStrDescX[nX].cStrLen);
    } /* if !DIAG_NOSCALE */

    FrameRgnSolid(dc, lpDiag->hrgnCurve, aColors[IDIAGCOL_FRAME]); /* frame */

    GetClipBox(dc, &rcPaintSector);
    if (!RectInRegion(lpDiag->hrgnCurve, &rcPaintSector))
    {                                       /* no curve painting required */
        RestoreDC(dc, -1);                          /* restore for return */
        return TRUE;
    } /* if */

    IntersectClipRect(dc, lpAxisX->nScreenMin, lpAxisY->nScreenMax,
                          lpAxisX->nScreenMax, lpAxisY->nScreenMin);

    /**** search and display marker text (notes) in diag */

    SetTextColor(dc, aColors[IDIAGCOL_NOTETEXT]);
    SetTextAlign(dc, TA_LEFT|TA_TOP); /* because 'DrawText' is also based on TA_ settings */

    hp = CreatePen(PS_INSIDEFRAME, 1, aColors[IDIAGCOL_NOTEFRAME]);
    hpenOld = SelectPen(dc, hp);                      /* line to hot spot */

    pNote = lpDiag->pM;

    while (pNote != NULL)
    {
        if (pNote->hrgnHotSpot != HRGN_NULL) DeleteRgn(pNote->hrgnHotSpot);
        pNote->hrgnHotSpot = HRGN_NULL;

        if (!(pNote->uOpt & MARKER_HIDE) &&                /* hide note ? */
            (pNote->x > lpAxisX->dWorldMin) &&      /* in X world range ? */
            (pNote->x < lpAxisX->dWorldMax))
        {
            if (GetWorldYByX(lpDiag, &pNote->x, 0, &YWorld))
            {                                          /* related Y exist */
                if ((YWorld > lpAxisY->dWorldMin) &&
                    (YWorld < lpAxisY->dWorldMax))   /* outside Y range ? */
                {                                       
                    x1 = GetDevXY(lpAxisX, pNote->x);   /* hot spot point */
                    x2 = GetDevXY(lpAxisY, YWorld);         /* coordinate */

                    if (PtInRegion(lpDiag->hrgnCurve, x1, x2)) /* inside? */
                    {
                        COORDINATE x_offs = CX_HOTSPOT(lpDiag)/2;
                        COORDINATE y_offs = CY_HOTSPOT(lpDiag)/2;

                        pNote->hrgnHotSpot =
                            CreateRectRgn(x1-x_offs+1, x2-y_offs+1,
                                          x1+x_offs, x2+y_offs);

                        FrameRgnSolid(dc, pNote->hrgnHotSpot, aColors[IDIAGCOL_NOTEFRAME]);
                        ExcludeClipRect(dc, x1-x_offs+1, x2-y_offs+1,
                                        x1+x_offs, x2+y_offs);

                        if (pNote->szTxt != NULL)
                        {
                            if (lstrlen(pNote->szTxt) > 0)
                            {             /* display comment text in rect */
                                RECT rcDest;              /* working rect */

                                if (pNote->uOpt & MARKER_POSAUTO)
                                {                        
                                    OffsetRect(&pNote->rcNote, /* del offset */
                                               -pNote->rcNote.left,
                                               -pNote->rcNote.top);

                                    x_offs = tm.tmAveCharWidth;
                                    y_offs = cyChar/2;

                                    /* at first horizontal alignment */
                                    if (pNote->uOpt & MARKER_LEFT)
                                        x_offs = -(pNote->rcNote.right+x_offs);
                                    else
                                        if (pNote->uOpt & MARKER_HCENTER)
                                            x_offs = -pNote->rcNote.right/2;

                                    /* now calc vertical alignment */
                                    if (pNote->uOpt & MARKER_TOP)
                                        y_offs = -(pNote->rcNote.bottom+y_offs);
                                    else
                                        if (pNote->uOpt & MARKER_VCENTER)
                                            y_offs = -pNote->rcNote.bottom/2;

                                    OffsetRect(&pNote->rcNote, x_offs, y_offs);
                                } /* if */

                                CopyRect(&rcDest, &pNote->rcNote);
                                OffsetRect(&rcDest, x1, x2); /* translate x,y */

                                if (RectInRegion(lpDiag->hrgnCurve, &rcDest))
                                {
                                    if (pNote->uOpt & MARKER_FRAME)
                                        FrameRectSolid(dc, &rcDest, aColors[IDIAGCOL_NOTEFRAME]);

                                    InflateRect(&rcDest, -NOTE_BORDER_X,
                                                -NOTE_BORDER_Y);

                                    if ((rcDest.top < rcDest.bottom) &&
                                        (rcDest.left < rcDest.right))
                                    {
                                        UINT uFormat = (pNote->uOpt & MARKER_POS_MASK) | DT_NOPREFIX | DT_WORDBREAK;
                                        if (strchr(pNote->szTxt, '\n') == NULL)
                                            uFormat |= DT_SINGLELINE;
                                        DrawText(dc, pNote->szTxt, -1, &rcDest, uFormat);
                                    } /* if */

                                    if (pNote->uOpt & MARKER_OPAQUE)
                                    {
                                        ExcludeClipRect(dc, rcDest.left-NOTE_BORDER_X,
                                                        rcDest.top-NOTE_BORDER_Y,
                                                        rcDest.right+NOTE_BORDER_X,
                                                        rcDest.bottom+NOTE_BORDER_Y);
                                        MoveTo(dc, x1, x2);
                                        LineTo(dc, (rcDest.right+rcDest.left)/2,
                                               (rcDest.top+rcDest.bottom)/2);
                                    } /* if */
                                } /* if RectInRegion */
                            } /* if length of text non zero */
                        } /* if any text exist */

                    } /* if comment coordinate in curve region */
                } /* if comment coordinate in world range limit */
            } /* if y world value exist for x coordinate */
        } /* if not hide the marker*/

        pNote = pNote->pNextMarker;                /* pointer to next note */
    } /* while */

    DeletePen(SelectPen(dc, hpenOld));  /* select old and free current pen */

    /****** start of painting dotted scaling lines */

    if (!(lpDiag->nDiagOpt & DIAG_NOSCALE))
    {
        int nLen;
        LOGPEN lopnInDC;

        hp = CreatePen(PS_DOT, 1, aColors[IDIAGCOL_SCALELINE]);      
        hpenOld = SelectPen(dc, hp); /* select pen for dotted scale lines */

        /* because dotted lines can end some pixel after LineTo(x2, y2)
           end parameter x2, y2 you must clip explicit
         */
        if (lpAxisY->AxisFlags & AXIS_DOTLINES) 
            for (nX = 0; nX < cEntrysY; nX++)     /* all horizontal lines */
            {                          
                MoveTo(dc, lpAxisX->nScreenMin, aStrDescY[nX].nLinePos); 
                LineTo(dc, lpAxisX->nScreenMax, aStrDescY[nX].nLinePos);
            } /* for */

        if (lpAxisX->AxisFlags & AXIS_DOTLINES)
            for (nX = 0; nX < cEntrysX; nX++)       /* all vertical lines */
            {
                MoveTo(dc, aStrDescX[nX].nLinePos, lpAxisY->nScreenMin); 
                LineTo(dc, aStrDescX[nX].nLinePos, lpAxisY->nScreenMax);
            } /* for */

        GetObject(hp, sizeof(lopnInDC), &lopnInDC);      /* get pen infos */
        lopnInDC.lopnStyle = PS_INSIDEFRAME;   /* create pen based on old */
        lopnInDC.lopnColor = aColors[IDIAGCOL_FRAME];
        lopnInDC.lopnWidth.x = max(3, cyChar/8);
        if (!ODD(lopnInDC.lopnWidth.x)) ++lopnInDC.lopnWidth.x;
        hp = CreatePenIndirect(&lopnInDC);
        DeletePen(SelectPen(dc, hp));                  /* get pen into DC */

        nLen = tm.tmAveCharWidth/2;

        for (nX = 0; nX < cEntrysY; nX++)           /* all y scale points */
        {
            MoveTo(dc, lpAxisX->nScreenMin, aStrDescY[nX].nLinePos);
            LineTo(dc, lpAxisX->nScreenMin+nLen, aStrDescY[nX].nLinePos);
            MoveTo(dc, lpAxisX->nScreenMax, aStrDescY[nX].nLinePos);
            LineTo(dc, lpAxisX->nScreenMax-nLen, aStrDescY[nX].nLinePos);
        } /* for */

        for (nX = 0; nX < cEntrysX; nX++)           /* all x scale points */
        {
            MoveTo(dc, aStrDescX[nX].nLinePos, lpAxisY->nScreenMin); 
            LineTo(dc, aStrDescX[nX].nLinePos, lpAxisY->nScreenMin-nLen);
            MoveTo(dc, aStrDescX[nX].nLinePos, lpAxisY->nScreenMax);
            LineTo(dc, aStrDescX[nX].nLinePos, lpAxisY->nScreenMax+nLen);
        } /* for */

        DeletePen(SelectPen(dc, hpenOld));/* select old pen before delete */
    } /* if DIAG_NOSCALE */
                                                 /* and overwrite scaling */
    FrameRgnSolid(dc, lpDiag->hrgnCurve, aColors[IDIAGCOL_FRAME]);

    /******** start of curve/samples painting */

    cSamples = 1;
    if (lpDiag->InitGetY != NULL)              /* init function defined ? */
        cSamples = lpDiag->InitGetY(lpAxisX->dWorldMin, lpAxisX->dWorldMax,
                                    &lpDiag->pAppData);

    if (cSamples < 0)
    {
        RestoreDC(dc, -1);                    /* restore for return */
        return FALSE;                         /* memory or other errors ? */
    } /* if */

    if (lpDiag->nDiagOpt & DIAG_DISCRET)
    {
        HDC hdcMem;
        BITMAP bmPoint;

        HBRUSH hbrOld = SelectBrush(dc, CreateSolidBrush(aColors[IDIAGCOL_CURVE])); /* color of dest bitmap */
        int cxy2 = lpDiag->cxySmplCurve/2;
        HBITMAP hbmp = LoadBitmap(hInst, lpDiag->szCurveBmp);

        hdcMem = CreateCompatibleDC(dc);          /* create memory device */
        SelectBitmap(hdcMem, hbmp);
        SetMapMode(hdcMem, GetMapMode(dc));
        GetObject(hbmp, sizeof(BITMAP), &bmPoint);
        SetStretchBltMode(dc, BLACKONWHITE);

        for (nX = 0; nX < cSamples; nX++)
        {
            if (fnUserBreak()) break;                   /* break for loop */

            if (lpDiag->GetNextWorldY(&XWorld, &YWorld, lpDiag->pAppData))
            {                                            /* y or x exist ? */
                if (InWorldRange(lpDiag, XWorld, YWorld))/* out of range ? */
                {
                    COORDINATE nY = GetDevXY(lpAxisY, YWorld) - cxy2;
                    COORDINATE nHeight = lpDiag->cxySmplCurve;

                    if (YWorld < 0.0)
                    {
                        nHeight = -nHeight;
                        nY += 2*cxy2;
                    } /* if */

                    StretchBlt(dc, GetDevXY(lpAxisX, XWorld) - cxy2, nY,
                               lpDiag->cxySmplCurve, nHeight,
                               hdcMem, 0, 0, bmPoint.bmWidth,
                               bmPoint.bmHeight, 0xB8074Al);
                   /* for raster operation code 0xB8074AL see Ch.Petzold
                      Chapter 13 or Windows reference manual */
                } /* if */
            } /* if */
        } /* for */

        DeleteDC(hdcMem);      /* at first delete context, then bitmap !!! */
        DeleteBitmap(hbmp);
        DeleteBrush(SelectBrush(dc, hbrOld));
    } /* if */
    else         /* continous 2D diags */
    {
        BOOL bFirstValidVal = TRUE, bWasOutSide = TRUE;

        hpenOld = SelectPen(dc, CreatePen(PS_SOLID, lpDiag->cxySmplCurve,
                                          aColors[IDIAGCOL_CURVE]));

        if (lpDiag->nDiagOpt & DIAG_CONST_SAMPLES)
            dDeltaPix = (double)(lpAxisX->nScreenMax - lpAxisX->nScreenMin)/
                        (lpDiag->nConstSmpl - 1);
        else dDeltaPix = 1.0;                        /* steps of one pixel */

        for (dXPix = lpAxisX->nScreenMin; dXPix < lpAxisX->nScreenMax+2; dXPix += dDeltaPix)
        {
            XWorld = GetWorldXY(lpAxisX, dXPix);
            if (lpDiag->GetNextWorldY(&XWorld, &YWorld, lpDiag->pAppData))
            {                                        /* if valid value */
                bIsOutSide = FALSE;
                if (YWorld < lpAxisY->dWorldMin)     /* out of y range */
                {
                    bIsOutSide = TRUE;              /* store outsiding */
                    YWorld = lpAxisY->dWorldMin;     /* set min. value */
                } /* if */
                else
                {
                    if (YWorld > lpAxisY->dWorldMax)
                    {
                        bIsOutSide = TRUE;
                        YWorld = lpAxisY->dWorldMax;
                    } /* if */
                } /* /else */

                if ((bWasOutSide && bIsOutSide) || /* set cursor to border */
                    bFirstValidVal)             /* first valid world value */
                { 
                    MoveTo(dc, ROUND(dXPix), GetDevXY(lpAxisY, YWorld));
                    bFirstValidVal = FALSE;
                } /* if */
                else                     /* line from/to border or inside */
                {
                    LineTo(dc, ROUND(dXPix), GetDevXY(lpAxisY, YWorld));
                }

                bWasOutSide = bIsOutSide;
            } /* if */

            if (fnUserBreak()) break;                   /* break for loop */
        } /* for */

        DeletePen(SelectPen(dc, hpenOld));
    } /* else */

    EndGetting(lpDiag);                        /* at last overwrite curve */
    FrameRgnSolid(dc, lpDiag->hrgnCurve, aColors[IDIAGCOL_FRAME]);  

    RestoreDC(dc, -1);
    return TRUE;
} /* PaintDiag */

