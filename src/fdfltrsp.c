/* DFCGEN functions to calculate phase-, magnitude-,
   attenuation-, group delay-, phase delay-, impulse and step response

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/fdfltrsp.c,v $
 * $Revision: 1.2 $
 * $Date: 2000-08-17 12:45:20 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $

 */

#include "DFCWIN.H"
#include "FDFLTRSP.H"


/* locale functions */

static struct complex GetCplxTransfer(double dOmega, PolyDat *poly);
static double ModAngle180(double dAngle);
static BOOL Angle(double, PolyDat *, double *);
static BOOL GetPhase(double omega, tFilter *pFlt, double *dPhase);
static double GetGroupDelayForPhase(double frequ);
static BOOL GetGroupDelayOfPoly(double dOmega, PolyDat *poly, double *lpGr, double *pMag2);
static void IncPtrCircular(double **, double *, int);
static void DecPtrCircular(double **, double *, int);
static void ProcessNextInput(tFilter *, TimeRespStruc *p);
static double GetNextDiscretInput(double dTime, tFilterInput SigType);
static int TimeRespInit(double dTimeStart, double dTimeEnd,
                        tFilterInput SigType, TimeRespStruc **pTimeRespStrucPtr);


/* returns the real and imag part of polynomial (passed vector of filter
   coefficients) in z-domain at omega = 2*Pi*f/f0
 * T(z) = a[0] + a[1]*z^-1 + a[2]*z^-2 + ... a[n]*z^-n
 * T(z) = (...((a[n]*z^-1 + a[n-1])*z^-1 + a[n-2])*z^-1 + ... a[1])*z^-1 + a[0]
 * z^-1 = exp(-j*omega), means this is a rotation of an vector by -omega
 */

static struct complex GetCplxTransfer(double dOmega, PolyDat *poly)
{
    int i = poly->order;
    double dOldReal = 0.0;
    struct complex Tr = {0.0, 0.0};
    double dSinOmega = sin(dOmega);
    double dCosOmega = cos(dOmega);
    double *pCoeff = poly->coeff;

    for (pCoeff = &poly->coeff[poly->order]; i >= 0; i--, pCoeff--)
    {
        Tr.x = dOldReal*dCosOmega + Tr.y*dSinOmega + *pCoeff;
        Tr.y = Tr.y * dCosOmega - dOldReal*dSinOmega;
        dOldReal = Tr.x;
    } /* for */

    return Tr;
} /* GetCplxTransfer() */


/* returns the magnitude related with the passed vector of filter
   coefficients in z-domain at the point omega = 2*Pi*f/f0 */
double Magnitude(double dOmega, PolyDat *poly)
{
    struct complex T = GetCplxTransfer(dOmega, poly);
    return hypot(T.x, T.y);
} /* Magnitude() */




/* returns the frequency response values at current frequency 'frequ' */
#pragma argsused
BOOL FrequencyResponse(double *frequ, double *dG, void *pAppData)
{
    double dOmega = 2.0*M_PI * *frequ/MainFilter.f0;

    return ProtectedDiv(Magnitude(dOmega, &MainFilter.a),
                        Magnitude(dOmega, &MainFilter.b), dG);
} /* FrequencyResponse() */


/* returns the attenuation at current frequency in dB */
BOOL Attenuation(double *frequ, double *dGdB, void *pAppData)
{
    if (!FrequencyResponse(frequ, dGdB, pAppData)) return FALSE;
    *dGdB = -20.0*log10(*dGdB);
    return TRUE;
} /* Attenuation() */


/* calculates the discrimination (characteristic) function D(f), means
   |T(f)|^2 = 1/(1 + |D(f)|^2) and therfore
   |D(f)|^2 = (1/|T(f)|^2 - 1)
   |D(f)| = sqrt |1/|T(f)|^2 - 1|
 */
BOOL ApproxCharacResponse(double *frequ, double *pD, void *pAppData)
{
    double T;
    BOOL bValid = FrequencyResponse(frequ, &T, pAppData);

    if (bValid)                            /* finite frequency response ? */
    {
        bValid = ProtectedDiv(1.0, T*T, pD);
        if (bValid) *pD = sqrt(fabs(1.0 / *pD - 1.0));
    } /* if */
    else *pD = 1.0;

    return bValid;
} /* ApproxCharacResponse() */



/* returns the angle (in rad) related with the passed vector of filter
   coefficients in z-domain T(z) at the point omega = 2*Pi*f/f0
   in [-PI+0, +PI] interval
 */
static BOOL Angle(double dOmega, PolyDat *poly, double *pAngle)
{
    struct complex T = GetCplxTransfer(dOmega, poly);
    *pAngle = atan2(T.y, T.x);
    return (hypot(T.y, T.x) > PHASE_INVALID_MAG);
} /* Angle() */



/* normation of passed angle to -M_PI < dAngle <= M_PI
 * this matches atan2() algorithm
 */
static double ModAngle180(double dAngle)
{
    dAngle = fmod(dAngle, 2.0*M_PI);                 /* multiples of 2*Pi */
    if (dAngle > M_PI) dAngle -= 2.0*M_PI; /* norm to -M_PI < Phi <= M_PI */
    if (dAngle <= -M_PI) dAngle += 2.0*M_PI;
    return dAngle;
} /* ModAngle180 */



/* returns phase ([-PI+0, +PI] interval) at circle frequency 'Omega'
 */
static BOOL GetPhase(double Omega, tFilter *pFlt, double *pPhase)
{
    double dPoleAngle, dZeroAngle;

    if (!Angle(Omega, &pFlt->b, &dPoleAngle)) return FALSE;
    if (!Angle(Omega, &pFlt->a, &dZeroAngle)) return FALSE;
    *pPhase = ModAngle180(dPoleAngle - dZeroAngle); /* norm to [-M_PI+0, M_PI] */
    return TRUE;
} /* GetPhase() */



/* initialization of phase response (only required if flag
   DOPT_PHASE_INTEGRATE) returns 1 because this is the "all ok" flag
 * The variable 'LastTgr' is not used correct (only used if
   DOPT_PHASE_INTEGRATE is set) because there may be a reentrant call
   of 'PhaseResponse()'. But there is no effcetive way to solve this
   problem in current version of DFCGEN. In most cases no problem
   occurs since undifined magnitude response points of the first and
   second caller are different from view of the processing time.
 */

static double LastTgr = 0.0;

#pragma argsused
int PhaseRespInit(double dStart, double dEnd, void **pPhaseDataPtr)
{
    if (uDeskOpt & DOPT_PHASE_INTEGRATE)
    {
        double Mag;
        double *pPhase0;
        struct complex Num = GetCplxTransfer(0.0, &MainFilter.a); 
        struct complex Den = GetCplxTransfer(0.0, &MainFilter.b);

        if ((pPhase0 = MALLOC(sizeof(*pPhase0))) == NULL) return -1;

        *pPhase0 = 0.0;
        *pPhaseDataPtr = pPhase0;

        if (ProtectedDiv(Num.x, Den.x, &Mag)) /* DC magnitude too small ? */
            if (Mag < -PHASE_INVALID_MAG) *pPhase0 = M_PI;

        LastTgr = 0.0;
    } /* if */

    return 1;
} /* PhaseRespInit() */

/* transformation of group delay getting function to MATHFUNCDIM2 type
 * returns group delay   
 */
static double GetGroupDelayForPhase(double frequ)
{
    double Tgr;

    if (GroupDelay(&frequ, &Tgr, NULL))            /* valid group delay ? */
        LastTgr = Tgr;
    else Tgr = LastTgr;                          /* else return old value */

    return Tgr;
} /* GetGroupDelayForPhase() */



/* returns the phase (in deg) for current frequency */
BOOL PhaseResponse(double *frequ, double *pPhase, void *pPhase0)
{
    if (uDeskOpt & DOPT_PHASE_INTEGRATE)
        /* calculation by integration of group delay time Tgr
         * Tgr(f) = dB(w)/dw = 1/(2*Pi)*dB(f)/df
         * B(f) = 2*Pi*Integr(Tgr(f))
         */

        *pPhase =
            2.0*M_PI*Integrate(0.0, *frequ, aMathLimits[IMATHLIMIT_ERRPHASE],
                               GetGroupDelayForPhase) + *((double *)pPhase0);
    else                             
    {                                    /* limited phase values desired ? */
        if (!GetPhase(2.0*M_PI * *frequ/MainFilter.f0, &MainFilter, pPhase))
            return FALSE;

        if ((uDeskOpt & DOPT_PHASE360) && (*pPhase < 0.0))
            *pPhase += 2.0*M_PI;
    } /* if */

    *pPhase *= 180.0/M_PI;
    return TRUE;
} /* PhaseResponse() */


/* free the memory for initial phase at f=0
 */
void PhaseRespEnd(void *pPhase0)
{
    if (uDeskOpt & DOPT_PHASE_INTEGRATE) FREE(pPhase0);
} /* PhaseRespEnd() */



/* calculates group delay of a single Z-polynomial
 * returns also square of magnitude related to this polynomial in *pMag2
 */
static BOOL GetGroupDelayOfPoly(double dOmega, PolyDat *poly, double *lpGr, double *pMag2)
{
    int i;
    double dReal = 0.0;
    double dImag = 0.0;
    double dDevReal = 0.0;
    double dDevImag = 0.0;
    double dCosOmegaN;
    double dSinOmegaN;
    double *pCoeff;

    for (i = 0, pCoeff = poly->coeff; i <= poly->order; i++, pCoeff++)
    {
        dCosOmegaN = cos(i*dOmega);
        dSinOmegaN = sin(i*dOmega);
        dReal += *pCoeff * dCosOmegaN;
        dImag += *pCoeff * dSinOmegaN;
        dDevReal += *pCoeff * i * dSinOmegaN;
        dDevImag += *pCoeff * i * dCosOmegaN;
    } /* for */

    *pMag2 = dReal*dReal + dImag*dImag;           /* magnitude of poly */
    return ProtectedDiv(dReal*dDevImag + dImag*dDevReal, *pMag2, lpGr);
} /* GetGroupDelayOfPoly */


#pragma argsused
BOOL GroupDelay(double *frequ, double *dTg, void *pAppData)
{
    double dSqrMagZero, dSqrMagPole;
    double dOmega = 2.0*M_PI * *frequ/MainFilter.f0;
    double dTg1, dTg2;

    if (!GetGroupDelayOfPoly(dOmega, &MainFilter.a, &dTg1, &dSqrMagZero)) return FALSE;
    if (!GetGroupDelayOfPoly(dOmega, &MainFilter.b, &dTg2, &dSqrMagPole)) return FALSE;
    *dTg = (dTg1 - dTg2)/MainFilter.f0;
    if (!ProtectedDiv(dSqrMagZero, dSqrMagPole, &dSqrMagZero)) return FALSE;

    return ((*dTg > MIN_GROUPDELAY) &&
            (*dTg < MAX_GROUPDELAY) &&
            (sqrt(dSqrMagZero) > GRDELAY_INVALID_MAG)); /* valid magnitude ? */
} /* GroupDelay() */



/* phase delay = Tp(f) = B(f) / (2*Pi*f) = B(f)/Omega
 */

#pragma argsused
int PhaseDelayInit(double dStart, double dEnd, void **pPhaseDataPtr)
{
    if (uDeskOpt & DOPT_PHASE_INTEGRATE)   /* monotonous phase required ? */
        return PhaseRespInit(dStart, dEnd, pPhaseDataPtr);

    return 1; /* else return <ok> */
} /* PhaseDelayInit() */


#pragma argsused
BOOL PhaseDelay(double *frequ, double *pTd, void *pPhase0)
{
    double dW = *frequ * 2.0*M_PI;

    if (uDeskOpt & DOPT_PHASE_INTEGRATE) 
    {                        /* calc phase via integration of group delay */
        *pTd =
            2.0*M_PI*Integrate(0.0, *frequ, aMathLimits[IMATHLIMIT_ERRPHASE],
                               GetGroupDelayForPhase) + *((double *)pPhase0);
    } /* if */
    else                                /* limited phase values desired ? */
        if (!GetPhase(dW/MainFilter.f0, &MainFilter, pTd)) return FALSE;  /* in [-M_PI+0, M_PI] */

    if (*pTd < 0.0)                     /* transform into positive values */
        *pTd -= 2.0*M_PI*floor(*pTd/2.0/M_PI);

    if (!ProtectedDiv(*pTd, dW, pTd)) return FALSE;
    return ((*pTd >= MIN_PHASEDELAY) && (*pTd <= MAX_PHASEDELAY));
} /* PhaseDelay */


/* free the memory for initial phase at f=0
 */
void PhaseDelayEnd(void *pPhase0)
{
    if (uDeskOpt & DOPT_PHASE_INTEGRATE) FREE(pPhase0);
} /* PhaseDelayEnd() */




/* time-domain signal responses support */

static void IncPtrCircular(double **p, double *pBuffer, int order)
{

    if (*p >= pBuffer+order) *p = pBuffer;
    else ++(*p);
}

static void DecPtrCircular(double **p, double *pBuffer, int order)
{
    if (*p <= pBuffer) *p = pBuffer+order;
    else --(*p);
}


static double GetNextDiscretInput(double dTime, tFilterInput SigType)
{
    switch (SigType)
    {
        case USER_IN_FUNCTION :
        case UNIT_STEP :     return 1.0;
        case DIRAC_IMPULSE : return (dTime == 0.0) ? 1.0 : 0.0;
    } /* switch */
    return 0.0;
}

static void ProcessNextInput(tFilter *Filt, TimeRespStruc *p)
{
    int i;
    double dOut = 0.0;
    double *pD;
    double dIn = GetNextDiscretInput(p->dCurrTime, p->TimeSigType);

    *p->pCurrIn = dIn;
    pD = p->pCurrIn;
    DecPtrCircular(&p->pCurrIn, p->pInputBuffer, Filt->a.order);

    for (i = 0; i <= Filt->a.order; i++)
    {
        dOut += *pD * Filt->a.coeff[i];
        IncPtrCircular(&pD, p->pInputBuffer, Filt->a.order);
    } /* for */

    pD = p->pLastOut;
    for (i = 1; i <= Filt->b.order; i++)
    {
        dOut -= *pD * Filt->b.coeff[i];
        IncPtrCircular(&pD, p->pOutputBuffer, Filt->b.order);
    } /* for */

    dOut = dOut/Filt->b.coeff[0]*Filt->factor;
    DecPtrCircular(&p->pLastOut, p->pOutputBuffer, Filt->b.order);
    *p->pLastOut = dOut;
}

int ImpulseRespInit(double dTimeStart, double dTimeEnd, void **pTimeRespStrucPtr)
{
    return TimeRespInit(dTimeStart, dTimeEnd, DIRAC_IMPULSE, (TimeRespStruc **)pTimeRespStrucPtr);
}


int StepRespInit(double dTimeStart, double dTimeEnd, void **pTimeRespStrucPtr)
{
    return TimeRespInit(dTimeStart, dTimeEnd, UNIT_STEP, (TimeRespStruc **)pTimeRespStrucPtr);
}



/* returns number of discret samples in interval [dTimeStart, dTimeEnd]
 * if an error occurs returns -1, then don't call 'TimeRespEnd()'
 */
static int TimeRespInit(double dTimeStart, double dTimeEnd,
                        tFilterInput SigType, TimeRespStruc **pTimeRespStrucPtr)
{
    int i, cSamples = 0;
    double dTime, dT0 = 1.0/MainFilter.f0;
    TimeRespStruc *p;

    if ((p = MALLOC(sizeof(TimeRespStruc))) == NULL) return -1;
    *pTimeRespStrucPtr = p; /* update application data with ptr to struct */

    p->pInputBuffer = (double *)MALLOC((1+MainFilter.a.order)*sizeof(double));
    p->pOutputBuffer = (double *)MALLOC((1+MainFilter.b.order)*sizeof(double));
    if ((p->pInputBuffer == NULL) || (p->pOutputBuffer == NULL)) return -1;

    if (dT0 <= 2*DBL_EPSILON) dT0 = 2*DBL_EPSILON;
    p->pLastOut = p->pOutputBuffer;
    p->pCurrIn = p->pInputBuffer;
    p->TimeSigType = SigType;                   /* set passed signal type */

    /* clear input and output sample buffer */
    for (i=0; i<=MainFilter.a.order; i++) p->pInputBuffer[i] = 0.0;
    for (i=0; i<=MainFilter.b.order; i++) p->pOutputBuffer[i] = 0.0;

    p->dCurrTime = 0.0;
    while (p->dCurrTime < dTimeStart)     /* process samples up to dTimeStart */
    {
        if (UserBreak()) return 0;                     /* break while loop */
        ProcessNextInput(&MainFilter, p);
        p->dCurrTime += dT0;
    } /* while */


    dTime = p->dCurrTime;

    while (dTime <= dTimeEnd)       /* count num of samples up to dTimeEnd */
    {
        if (UserBreak()) return 0;                     /* break while loop */
        if (++cSamples == MAXINT) return MAXINT-1;
        dTime += dT0;
    } /* while */

    return cSamples;
} /* TimeRespInit */


#pragma argsused
BOOL TimeResponse(double *dTime, double *dResponseVal, void *pTimeRespStruc)
{
    TimeRespStruc *p = (TimeRespStruc *)pTimeRespStruc;

    *dTime = p->dCurrTime;
    ProcessNextInput(&MainFilter, p);
    p->dCurrTime += 1.0/MainFilter.f0;
    *dResponseVal = *p->pLastOut;
    return TRUE;
}

void TimeRespEnd(void *pTimeRespStruc)
{
    FREE(((TimeRespStruc *)pTimeRespStruc)->pInputBuffer);
    FREE(((TimeRespStruc *)pTimeRespStruc)->pOutputBuffer);
    FREE(pTimeRespStruc);
}


