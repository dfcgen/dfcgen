/* DFCGEN math./tech. design functions for digital filters

 * supports design of various digital filters, e.g. Lin. FIR,
   Standard IIR (Bessel, Butterworth, Chebyshev and Chebyshev Inverse, Cauer)
   and various special filter functions (fitting filter, integrator, hilbert
   transformer)

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/fdfltdef.c,v $
 * $Revision: 1.4 $
 * $Date: 2001-10-06 20:01:05 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $
   Revision 1.3  2000/08/17 12:45:18  ralf
   DFCGEN now is shareware (no registration or license dialogs).
   Directory with examples added.


 */

#include "DFCWIN.H"
#include "FDFLTRSP.H"

#define POW10(val)    exp(M_LN10*(val))


/* prototypes of local functions */
static double Sn(double k, double x);  /* Jacobian function sn(x) */
static double GetDiscrimination(double att);
static BOOL CalcRootsBreakProc(long);
static int GetLowpassCutoff(tFilter *pFlt, double dMinFc, double dMaxFc,
                            double *dCutoff);
static void DomainTransfRoots(tFilter *);
static void TransfRootBP(struct complex *, struct complex *, double, double);
static void FTransfRootsSDomain(tTransform, double, double,
                                int *, struct complex *, int *, struct complex *);
static void BilinearRoot(double, struct complex *);
static UINT NormFilterTransfer(tFilter *);
static BOOL CheckPolynomial(PolyDat *);
static void FactorCplxArr(int cNum, struct complex pCplxNum[], double fac);



/* get mem space for a new filter structure */
BOOL MallocFilter(tFilter *f)
{
    if (!MallocPolySpace(&f->a)) return FALSE;
    if (!MallocPolySpace(&f->b))
    {
        FreePolySpace(&f->a);
        return FALSE;
    } /* if */

    return TRUE;
} /* MallocFilter */


void FreeFilter(tFilter *f)
{
    FreePolySpace(&f->a);
    FreePolySpace(&f->b);
} /* FreeFilter */


static BOOL CheckPolynomial(PolyDat *poly)
{
    int i, k;

    int degree = poly->order;

    if (fabs(poly->coeff[degree]) < aMathLimits[IMATHLIMIT_NULLCOEFF])
    {
        MessageAckUsr(hwndFDesk, ERROR_COEFFN_BAD);
        for (i = degree; i >= 0; i--)
            if (fabs(poly->coeff[i]) < aMathLimits[IMATHLIMIT_NULLCOEFF]) degree--;
            else break;      /* break for loop if first valid coeff found */
    } /* if */

    if (degree < 0) return FALSE;

    if (fabs(poly->coeff[0]) < aMathLimits[IMATHLIMIT_NULLCOEFF])
    {
        MessageAckUsr(hwndFDesk, ERROR_COEFF0_BAD);

        /* search first valid coeff */
        for (i = 0; (i <= degree) && (fabs(poly->coeff[i]) < aMathLimits[IMATHLIMIT_NULLCOEFF]);
             i++);

        if (i > degree) return FALSE;

        /* shift down */
        degree -= i;
        for (k = 0; k <= degree; k++) poly->coeff[k] = poly->coeff[k+i];
    } /* if */

    if (degree != poly->order)                 /* destroy redundant memory */
    {
        poly->coeff = (double *)REALLOC(poly->coeff,
                               max (1, 1+degree*sizeof(poly->coeff[0])));
        poly->root  = (struct complex *)REALLOC(poly->root,
                                  max (1, degree*sizeof(poly->root[0])));
    } /* if */

    poly->order = degree;
    return ((poly->root != NULL) && (poly->coeff != NULL));       
} /* CheckPolynomial() */


/* Checks the possibility of implementation and returns FALSE if not possible */
BOOL CheckImplementation(tFilter *lpFilter)
{
    return CheckPolynomial(&lpFilter->a) && CheckPolynomial(&lpFilter->b);
} /* CheckImplementation() */



BOOL SystemStabil(tFilter *Filter)
{
    int i;
    struct complex *r = Filter->b.root;
    for (i = 0; i < Filter->b.order; i++, r++)
        if (r->x * r->x + r->y * r->y > 1.0) return FALSE;

    return TRUE;
}



BOOL RoundCoefficients(PolyDat *polynom)
{
    int i;

    for (i=0; i <= polynom->order; i++)
        polynom->coeff[i] = ROUND(polynom->coeff[i]);

    return TRUE;   /* it's all ok */
}


BOOL DeletePolyCoeff(int iCoeff, PolyDat *poly)
{
    int i;
    if ((poly->order < 1) || (iCoeff > poly->order)) return FALSE;

    for (i = iCoeff; i < poly->order; i++) poly->coeff[i] = poly->coeff[i+1];
    poly->order--;
    poly->coeff = (double *)REALLOC(poly->coeff,
                               max (1, 1+poly->order*sizeof(poly->coeff[0])));
    poly->root  = (struct complex *)REALLOC(poly->root,
                                  max (1, poly->order*sizeof(poly->root[0])));
    return (poly->coeff != NULL) && (poly->root != NULL);
}



BOOL MultPolynomUp(PolyDat *polynom, /* polynomial to multiplicate with fact*x^expo */
                   double fact,         /* real factor */
                   unsigned expo)       /* exponent (shift count) */
{
    int i;

    for (i=0; i<=polynom->order; i++)
    {
        polynom->coeff[i+expo] = fact*polynom->coeff[i];
        if (i < expo) polynom->coeff[i] = 0.0;
    }

    polynom->order += expo;
    return TRUE;
}


BOOL NormGainTo(PolyDat *lpPolynom, tFilter *lpFilter, double frequency,
                double dDestGain)
{
    double dRealGain;
    int i;
    double factor;
    double dOmegaT0 = 2.0*M_PI*frequency/lpFilter->f0;

    if (!ProtectedDiv(Magnitude(dOmegaT0, &lpFilter->a),
                      Magnitude(dOmegaT0, &lpFilter->b),
                      &dRealGain)) return FALSE;
    if (!ProtectedDiv(dDestGain, dRealGain, &factor)) return FALSE;
    if (lpPolynom == &lpFilter->b)
        if (!ProtectedDiv(1.0, factor, &factor)) return FALSE;

    for (i=0; i <= lpPolynom->order; i++) lpPolynom->coeff[i] *=  factor;

    return CheckImplementation(lpFilter);
} /* NormGainTo */



/* calculation of cutoff frequency Wc in radians of passed filter with
   transfer function T(s) by coefficients !!!
 * T(s) = A(s)/B(s)   T(0)^2/|T(jfg)|^2 = 2
 * roots of transfer function are not used
 * not destroys pFlt->b.coeff[] array
 * requires pFlt->b.order >= pFlt->a.order and pFlt->b.order > 0
   (possibility : pFlt->a.order == 0)
 * return values are CALC_OK, CALC_BREAK, CALC_NO_MEM
 */
static int GetLowpassCutoff(tFilter *pFlt, double dMinFc, double dMaxFc,
                            double *dCutoff)
{
    int i, k;
    int Status = CALC_OK;
    double dFac;                                           /* 0.5 * T(0)^2 */
    double *Coeff;                               /* pointer to coeff array */
    double *Bx2 = (double *)MALLOC((1+pFlt->b.order)*sizeof(Bx2[0]));
    if (Bx2 == NULL) return CALC_NO_MEM;


    dFac = pFlt->a.coeff[0]/pFlt->b.coeff[0];
    dFac = dFac*dFac*0.5;

    /* calc |B(x)|^2 with x=s^2 in Bx2[] */
    for (i = 0; i <= pFlt->b.order; i++) Bx2[i] = 0.0;            /* clear */

    /* build power of real part */
    for (i = 0, Coeff = pFlt->b.coeff; i <= pFlt->b.order/2; i++) 
        for (k = 0; k <= pFlt->b.order/2; k++)
            Bx2[i+k] += dFac * Coeff[2*i] * Coeff[2*k] * (ODD(i+k)?-1:+1);

    /* build power of imag part */
    for (i = 0; i <= (pFlt->b.order-1)/2; i++) 
        for (k = 0; k <= (pFlt->b.order-1)/2; k++)
            Bx2[i+k+1] += dFac * Coeff[2*i+1] * Coeff[2*k+1] * (ODD(i+k)?-1:+1);

    /* calc |A(x)|^2 with x=s^2 and accumulate in Bx2[] */
    for (i = 0, Coeff = pFlt->a.coeff; i <= pFlt->a.order/2; i++) /* sqr real part */
        for (k = 0; k <= pFlt->a.order/2; k++)
            Bx2[i+k] -= Coeff[2*i] * Coeff[2*k] * (ODD(i+k)?-1:+1);

    /* build power of imag part only if a.order > 0 */
    if (pFlt->a.order > 0)
    {
        for (i = 0; i <= (pFlt->a.order-1)/2; i++) /* sqr imag part */
            for (k = 0; k <= (pFlt->a.order-1)/2; k++)
                Bx2[i+k+1] -= Coeff[2*i+1] * Coeff[2*k+1] * (ODD(i+k)?-1:+1);
    } /* if */


    /* search real roots of polynomial Bx2[]
     * search only in range results in x=w^2
     */

    if (SearchRealRootOfPoly(pFlt->b.order, Bx2, aMathLimits[IMATHLIMIT_ERRROOTS],
                             dCutoff, dMinFc*dMinFc, dMaxFc*dMaxFc,
                             CalcRootsBreakProc))
        *dCutoff = sqrt(*dCutoff);
    else Status = CALC_BREAK;

    FREE(Bx2);
    return Status;
} /* GetLowpassCutoff */



/* Bilinear Transformation of a single complex root r from S-domain into
 * Z-domain
 * Because the real part of r is less than zero, no division by zero is
 * possible
 * r := (2*f0 + r) / (2*f0 - r)
 */
static void BilinearRoot(double dF0, struct complex *r)
{
    struct complex num, den;
    dF0 *= 2.0;
    num.x = dF0 + r->x;
    den.x = dF0 - r->x;
    num.y = r->y;
    den.y = - r->y;
    *r = CplxDiv(num, den);
}


/* - transforms roots (poles, zeros) in S-domain into roots in Z-domain
 * - roots space must be enough to perform transformation (caller must alloc
     a.order = b.order space for numerator polynomial)
 * - the order a.order will be corrected to b.order
 * - the denominator order (b.order) must be greather/equal like numerator order (a.order)
 */

static void DomainTransfRoots(tFilter *lpFlt)
{
    int i;
    struct complex *r;
    for (i = 0, r = lpFlt->b.root; i < lpFlt->b.order; i++, r++) /* poles */
        BilinearRoot(lpFlt->f0, r);
    for (i = 0, r = lpFlt->a.root; i < lpFlt->a.order; i++, r++) /* zeros */
        BilinearRoot(lpFlt->f0, r);

    for (i = 0; i < lpFlt->b.order - lpFlt->a.order; i++, r++)
    {
        r->x = -1.0;
        r->y =  0.0;
    } /* for */

    lpFlt->a.order = lpFlt->b.order;        /* correct the numerator order */
}


/* transformation of digital frequency into analogue to perform correction
 * of frequency's
 *                       f := f0/Pi * sin(2Pi*f/f0)/(1+cos(2Pi*f/f0))
 *
 * because any frequency can be transformed via biliniear transformation
 *
 * s  := 2*f0 * (z-1)/(z+1) = 2*f0 * (exp(s/f0) - 1)/(exp(s/f0) + 1)
 * jw := 2*f0 * (exp(jw/f0) - 1)/(exp(jw/f0) + 1)  with w=2Pi*f
 */

static double BilinearCorrectF(double dF, double dSampleF)
{
    dF = 2.0*M_PI*dF/dSampleF;
    return dSampleF*M_1_PI*sin(dF)/(1.0+cos(dF));
}


/* transforms single root *lpSrcRoot into two new bandpass roots *lpSrcRoot and
 * lpDestRoot2
 * Q = Quality, Wm = Center Frequency (rad) = 2*M_PI*Fm
 * s := s/2/Q +/- sqrt((s/2/Q)^2 - Wm^2)
 * sqrt means the complex variation
 */
static void TransfRootBP(struct complex *lpSrcRoot, struct complex *lpDestRoot2,
                         double dCenterOmega, double dQ)

{
    struct complex CplxTherm1,                                 /* is s/2/Q */
                   CplxTherm2; /* the complex root sqrt((s/2/Q)^2 - Wm*Wm) */

    dCenterOmega *= dCenterOmega;                                /* square */

    CplxTherm1.x = 0.5/dQ;                                        /* 1/2/Q */
    CplxTherm1.y = 0.0;
    CplxTherm1 = CplxMult(CplxTherm1, *lpSrcRoot);         /* therm1 ready */

    CplxTherm2 = CplxMult(CplxTherm1, CplxTherm1);       /* complex square */
    CplxTherm2.x -= dCenterOmega;
    CplxTherm2 = CplxRoot(CplxTherm2);                     /* therm2 ready */

    lpDestRoot2->x = CplxTherm1.x + CplxTherm2.x;
    lpDestRoot2->y = CplxTherm1.y + CplxTherm2.y;
    lpSrcRoot->x = CplxTherm1.x - CplxTherm2.x;
    lpSrcRoot->y = CplxTherm1.y - CplxTherm2.y;
}


/* - Frequency Transformation of roots inside the S-domain
 * - passed frequency is in rad, what means this is w=2Pi*f */
static void FTransfRootsSDomain(tTransform Ft,
                                double dOmegaC,  /* cutoff or/and center f */
                                double dQ,                /* Q of BP or BS */
                                int *lpOrdNum, struct complex *lpRootNum,
                                int *lpOrdNom, struct complex *lpRootNom)
{
    int i;

    switch(Ft)  /* frequency transformation of roots */
    {
        case BANDSTOP :
        case HIGHPASS :                           /* LP->HP transformation */
        {
            struct complex CplxOmegaC;
            struct complex *lpRoot;

            CplxOmegaC.y = 0.0;
            CplxOmegaC.x = dOmegaC*dOmegaC;
            for (i=0, lpRoot=lpRootNom; i < *lpOrdNom; i++, lpRoot++)
                *lpRoot = CplxDiv(CplxOmegaC, *lpRoot);
            for (i = 0, lpRoot=lpRootNum; i < *lpOrdNum; i++, lpRoot++)
                *lpRoot = CplxDiv(CplxOmegaC, *lpRoot);
            for (i = 0; i < *lpOrdNom - *lpOrdNum; i++, lpRoot++)
                lpRoot->x = lpRoot->y = 0.0;
            *lpOrdNum = *lpOrdNom;                  /* new numerator order */
            if (Ft == HIGHPASS) break;             /* bandstop fall trough */
        }

        case BANDPASS :                           /* LP->BP transformation */
            for (i = 0; i < *lpOrdNom; i++, lpRootNom++)
                TransfRootBP(lpRootNom, lpRootNom + *lpOrdNom, dOmegaC, dQ);

            for (i = 0; i < *lpOrdNum; i++, lpRootNum++)
                TransfRootBP(lpRootNum, lpRootNum + *lpOrdNum, dOmegaC, dQ);

            for (i=0, lpRootNum += *lpOrdNum; i < *lpOrdNom - *lpOrdNum; i++, lpRootNum++)
                lpRootNum->x = lpRootNum->y = 0.0;

            *lpOrdNum += *lpOrdNom;        /* new numerator order */
            *lpOrdNom *= 2;
            break;  /* BANDPASS */
    } /* switch */
}


/* modify coefficients
 * norm b[0] to one and then set all numerator coefficients to realize
 * a passband attenuation of 0dB (transfer ratio 1.0)
 * returns IDSTRNULL if ok
 * else returns ERROR_FILTER_IMPLEMENT
 */
static UINT NormFilterTransfer(tFilter *lpFlt)
{
    int i;
    double *lpCoeff;
    double dFrequNorm = 0.0;

    switch(lpFlt->FTr.FTransf)         /* norm at center f, 0 or infinite */
    {
        case BANDPASS :                          /* LP->BP transformation */
            dFrequNorm = lpFlt->FTr.dCenter;  /* arithmetic center frequ. */

            /* if center frequency is geometric use the following expression 
               f = sqrt((B/2)^2 + fc^2) getting the linear (arithmetic)
               corrected center frequency
             */
            if (lpFlt->FTr.uFlags & CENTER_GEOMETRIC) /* center geometric */
                dFrequNorm = hypot(lpFlt->FTr.dCenter, 0.5*lpFlt->FTr.dCutFBw);

            break; /* BANDPASS */

        case HIGHPASS :                          /* LP->HP transformation */
            dFrequNorm = lpFlt->f0*0.5-DBL_EPSILON;  /* norm at Nyquist f */
            break; /* HIGHPASS */
    } /* switch */

    /* norm b[0] to 1.0 */
    for(i = lpFlt->b.order, lpCoeff = lpFlt->b.coeff+lpFlt->b.order;
        i >= 0; i--, lpCoeff--)
    {
        if (!ProtectedDiv(*lpCoeff, lpFlt->b.coeff[0], lpCoeff))
            return ERROR_FILTER_IMPLEMENT;
    } /* for */

    /* norm gain to 1.0 */
    if (!NormGainTo(&lpFlt->a, lpFlt, dFrequNorm, 1.0)) return ERROR_FILTER_IMPLEMENT;
    return IDSTRNULL;
} /* NormFilterTransfer */



/* Definition of lin. FIR-filters *
 * returns the error string id or null if ok *
 * IDSTROUTOFMEMORY is only a pseudo string identifier (do not use directly)
 */
int DefineLinFirFilter(tFilter *lpSrcFlt, tFilter *OldFilter)
{
    int i;
    double (*win_func)(double);             /* pointer to window function */
    double *Vec;                            /* ptr to coefficients vector */
    double dOmega;
    double dTransferRatio;
    double dOrder;                                    /* order of polynom */
    double dOrder2;
    UINT nIdStrErrMsg = IDSTRNULL;
    tLinFIR *lpPar = &lpSrcFlt->DefDlg.FIRDat;
    double dArithCenterF = 0.0;

    switch (lpSrcFlt->FTr.FTransf)
    {
	case HIGHPASS:       /* use the same cutoff frequency for lowpass */ 
	    dOmega = lpSrcFlt->FTr.dCutFBw;
	    break; /* cutoff F */

        case BANDPASS:
	case BANDSTOP:
	    dOmega = 0.5 * lpSrcFlt->FTr.dCutFBw;  /* lowpass design with */
	    break;                                      /* half bandwidth */

	default:   /* NOFTR and others */
	    dOmega = lpPar->dCutoff;
	    break;
    } /* switch */

    dOmega *= 2.0*M_PI/lpSrcFlt->f0;                    /* normed radians */

    lpSrcFlt->a.order = lpPar->nOrder;        /* set order of polynomials */
    lpSrcFlt->b.order = 0;
    dOrder = (double)(lpSrcFlt->a.order);
    dOrder2 = dOrder*0.5;

    /* memory alloc for coeff and root space in nominator and
     * denominator polynomial */

    if (!MallocFilter(lpSrcFlt)) return IDSTROUTOFMEMORY;
    Vec = lpSrcFlt->a.coeff;
    lpSrcFlt->b.coeff[0] = 1.0;
    lpSrcFlt->factor = 1.0;                                   /* not used */
    lpSrcFlt->uFlags = 0;                              /* clear all flags */
    lpSrcFlt->f_type = LINFIR;


    switch (lpPar->DataWin)                            /* set window func */
    {                                    /* kaiser win has extra handling */
        case HAMMING_WIN : win_func = hamming; break;
        case HANNING_WIN : win_func = hanning; break;
        case BLACKMAN_WIN: win_func = blackman; break;
        default :          win_func = rectangle; break;
    }

    for (i=0; i <= lpSrcFlt->a.order; i++)    /* compute the coefficients */
    {
        switch (lpPar->SubType)
        {
            case GAUSS_LP  :
                 Vec[i] = exp(-dOmega*dOmega*(i-dOrder2)*(i-dOrder2)*0.5/M_LN2);
                 break;

            case COS_LP    :
                 Vec[i] = cos(2.0*dOmega*(i-dOrder2))/
                          (1.0-16.0*dOmega*dOmega*(i-dOrder2)*(i-dOrder2)/M_PI/M_PI);
                 break;

            case COS2_LP   :
            {
                 double C = 1.0/acos(sqrt(M_SQRT_2));
                 Vec[i] = si(M_PI_2*C*dOmega*(i-dOrder2))/
                          (1.0-C*C*dOmega*dOmega*(i-dOrder2)*(i-dOrder2)*0.25);
                 break;
            }

            case SQR_LP   :
                 Vec[i] = exp(-dOmega*fabs(i-dOrder2)/sqrt(M_SQRT2-1.0));
                 break;

            default :                         /* ideal rectangular lowpass */
                 Vec[i] = si(dOmega*(i-dOrder2));
                 break;
        } /* switch */

        if (lpPar->DataWin != KAISER_WIN)                     /* windowing */
            Vec[i] *= (*win_func)((double)i/dOrder);
        else
            Vec[i] *= kaiser(lpPar->dAlpha, i/dOrder, aMathLimits[IMATHLIMIT_ERRKAISER]);
    } /* for */

    /* linear frequency transformation */
    switch(lpSrcFlt->FTr.FTransf)
    {
        case BANDSTOP :          /* at first LP->BP, then BP->BS transform */
        case BANDPASS :                           /* LP->BP transformation */

            /* if center frequency is geometric use the expression
               f = sqrt((B/2)^2 + fc^2) getting the linear (arithmetic)
               corrected center frequency
             */

            if (lpSrcFlt->FTr.uFlags & CENTER_GEOMETRIC) /* correction of center f */
                dArithCenterF = hypot(lpSrcFlt->FTr.dCenter,
                                      0.5*lpSrcFlt->FTr.dCutFBw);
            else dArithCenterF = lpSrcFlt->FTr.dCenter;

            for (i=0; i <= lpSrcFlt->a.order; i++)  /* modify coefficients */
                Vec[i] *= 2.0*cos(2.0*M_PI*(i-lpSrcFlt->a.order/2)*
                          dArithCenterF/lpSrcFlt->f0);
            if (lpSrcFlt->FTr.FTransf == BANDPASS) break;

            /* bandstop fall through */
            if (!NormGainTo(&lpSrcFlt->a, lpSrcFlt, dArithCenterF, 1.0)) break;

        case HIGHPASS :                           /* LP->HP transformation */
            dOmega = 2.0*M_PI*dArithCenterF/lpSrcFlt->f0;
            (void)ProtectedDiv(Magnitude(dOmega, &lpSrcFlt->a),
                               Magnitude(dOmega, &lpSrcFlt->b),
                               &dTransferRatio);

            for (i=0; i <= lpSrcFlt->a.order; i++) /* modify coefficients */
                if (!ProtectedDiv(Vec[i], -dTransferRatio, &Vec[i])) break;

            if (i <= lpSrcFlt->a.order) nIdStrErrMsg = ERROR_FILTER_IMPLEMENT;
            else Vec[lpSrcFlt->a.order/2] += 1.0;
            break; /* HIGHPASS */
    } /* switch */

    if (nIdStrErrMsg == IDSTRNULL) nIdStrErrMsg = NormFilterTransfer(lpSrcFlt);
    if (nIdStrErrMsg == IDSTRNULL) 
    {
        if (OldFilter->f_type != NOTDEF)
            FreeFilter(OldFilter);     /* release memory of target filter */
        *OldFilter = *lpSrcFlt;         /* set temp to current filter def */
    } /* if */
    else FreeFilter(lpSrcFlt);   /* on error */

    return nIdStrErrMsg;
}


/* Definition of Standard IIR-Filters */

#pragma argsused  /* disable "Parameter is never used" Warning */
static BOOL CalcRootsBreakProc(long trys)
{
    return ChkAbort();
}


/* multiply a array of complex values by a common factor */
static void FactorCplxArr(int cNum, struct complex pCplxNum[], double fac)
{
    while (cNum-- > 0)
    {
        pCplxNum->x *= fac;
        pCplxNum->y *= fac;
        ++pCplxNum;
    } /* while */
} /* FactorCplxArr */



static double Sn(double k, double x)
{
    return JacobiSN(k, x, aMathLimits[IMATHLIMIT_ERRJACOBISN]);
} /* Sn() */


/* calculates value of discrimination function (characteristic function
   of the filter) from the passed attenuation
 */
static double GetDiscrimination(double att)
{
    return sqrt(POW10(0.1*att) - 1.0);
} /* GetDiscrimination() */



/* returns the error string id or IDSTRNULL (=0) if ok *
 * IDSTROUTOFMEMORY is only a pseudo string identifier (do not use to
   access string ressource in rc-script)
 */
int DefineStdIIRFilter(HWND hDlg, tFilter *pTmpFlt, tFilter *pDestFlt)
{
    int i;
    struct complex *r;
    double dPartOfPi;
    double dTargetCutoffLP;   /* target cutoff frequency of lowpass in rad */
    double dCurrentCutoffLP;  /* true cutoff frequency of designed lowpass */
    double dQ = 0.0;                                      /* Q of BP or BS */
    UINT nIdStrErrMsg = IDSTRNULL;                             /* no error */
    tStdIIR *lpDef = &pTmpFlt->DefDlg.StdIIRDat;

    /* memory alloc for coeff and root space in numerator and
     * denominator polynomial */
    pTmpFlt->a.order = pTmpFlt->b.order = lpDef->nOrder;      /* for alloc */
    if (!MallocFilter(pTmpFlt)) return IDSTROUTOFMEMORY; /* memory space ? */

    pTmpFlt->factor = 1.0;                                /* everytime 1.0 */
    pTmpFlt->uFlags = 0;                                    /* clear flags */
    pTmpFlt->f_type = IIR;

    switch(pTmpFlt->FTr.FTransf)        /* modify denominator order before */
    {                                          /* frequency transformation */
        case BANDPASS :
        case BANDSTOP :
        {
            double dFu, dFo;

            pTmpFlt->b.order /= 2;      /* design lowpass with half order */

            /* if center frequency is geometric, the bandwidth B may be exceed
               half of center frequency fc because fc^2 = fo^2 * fu^2
               and fu = sqrt((B/2)^2 + fc^2) - B/2
               and fo = sqrt((B/2)^2 + fc^2) + B/2, means the virtual linear
               center frequency is sqrt((B/2)^2 + fc^2)
             * if linear the expression fc = 1/2 (fo + fu) is valid, and
               this means B = fo - fu 
             */
            if (pTmpFlt->FTr.uFlags & CENTER_GEOMETRIC)    /* arith avg ? */
            {
                dFu = hypot(pTmpFlt->FTr.dCenter, 0.5*pTmpFlt->FTr.dCutFBw);
                dFo = dFu + 0.5*pTmpFlt->FTr.dCutFBw;    /* lin. Fc + B/2 */
                dFu -= 0.5*pTmpFlt->FTr.dCutFBw;         /* lin. Fc - B/2 */
            } /* if */
            else
            {
                dFo = pTmpFlt->FTr.dCenter + 0.5*pTmpFlt->FTr.dCutFBw;
                dFu = pTmpFlt->FTr.dCenter - 0.5*pTmpFlt->FTr.dCutFBw;
            } /* else */

            dFu = BilinearCorrectF(dFu, pTmpFlt->f0);
            dFo = BilinearCorrectF(dFo, pTmpFlt->f0);

            dTargetCutoffLP = sqrt(dFu*dFo);/* geometric center frequency */
            dQ = dTargetCutoffLP/(dFo - dFu);           /* quality = fc/B */
            break;
        } /* BANDPASS, BANDSTOP */

        case HIGHPASS :
            dTargetCutoffLP = BilinearCorrectF(pTmpFlt->FTr.dCutFBw,
                                               pTmpFlt->f0);
            break; /* HIGHPASS */

        case NOFTR : /* LOWPASS */
            dTargetCutoffLP = BilinearCorrectF(lpDef->dCutoff, pTmpFlt->f0);
            break; /* NOFTR */
    } /* switch */

    /* reset order in S-domain for Bessel, Butterworth, Chebyshev I lowpass
     * from this point pTmpFlt->b.order has the correct system degree
     */
    pTmpFlt->a.order = 0;

    /* set design cutoff frequency to standard value wc=1.0 (cutoff
       frequency in radians) for Butterworth, Chebyshev and Chebyshev
       Invers lowpass
     */
    dCurrentCutoffLP = 1.0;
    dTargetCutoffLP *= 2.0*M_PI;                /* transform into radians */

    dPartOfPi = M_PI_2/pTmpFlt->b.order;

    switch (lpDef->SubType)       /* define lowpass in S-domain (LAPLACE) */
    {
        case BUTTERWORTH :             /* flat magnitude response filters */
            for (i=0, r=pTmpFlt->b.root; i<pTmpFlt->b.order; i++, r++)
            {
                r->x = -sin((2.0*i+1)*dPartOfPi); /* circle of roots */
                r->y = -cos((2.0*i+1)*dPartOfPi);
            } /* for */
            break; /* BUTTERWORTH */

        case CHEBY1 :       /* best passband approximation ripple filters */
        {                /* design with respect to cutoff frequency = 1.0 */
            double dInvDelta = 1.0/GetDiscrimination(lpDef->dRippleAtt);
            double dRealFactor = -1.0/ChebyshevInv(pTmpFlt->b.order, dInvDelta)
                                 * sinh(arsinh(dInvDelta)/pTmpFlt->b.order);
            double dImagFactor = 1.0/ChebyshevInv(pTmpFlt->b.order, dInvDelta)
                                 * cosh(arsinh(dInvDelta)/pTmpFlt->b.order);
            for (i=0, r=pTmpFlt->b.root; i<pTmpFlt->b.order; i++, r++)
            {
                r->x = dRealFactor*sin((2.0*i+1)*dPartOfPi);  /* elliptic */
                r->y = dImagFactor*cos((2.0*i+1)*dPartOfPi);
            } /* for */
            break;
        } /* CHEBY1 */


        case CHEBY2 :
        {
            struct complex numerator;
            double dArg, dSinArg, dCosArg;
            double dStopMin = POW10(lpDef->dMinAtt*0.05);
            double dSigEllipt = sinh(arsinh(dStopMin)/pTmpFlt->b.order);
            double dOmegEllipt = cosh(arsinh(dStopMin)/pTmpFlt->b.order);
            numerator.y = 0.0;                                       /* imag zero */
            numerator.x = ChebyshevInv(pTmpFlt->b.order, dStopMin);

            for (i=0, r=pTmpFlt->b.root; i<pTmpFlt->b.order; i++, r++)
            {
                dArg = (2.0*i+1)*dPartOfPi;
                dSinArg = sin(dArg);
                dCosArg = cos(dArg);
                r->x = - dSigEllipt*dSinArg;
                r->y = dOmegEllipt*dCosArg;
                *r = CplxDiv(numerator, *r);
            } /* for */

            pTmpFlt->a.order = pTmpFlt->b.order; /* overwrite default */

            /* define zeros */
            for (i=0, r=pTmpFlt->a.root; i<pTmpFlt->a.order; i++, r++)
            {
                r->x = 0.0;                              /* all at jw-axis */
                r->y = numerator.x/cos((2.0*i+1)*dPartOfPi);
            } /* for */

            break;
        } /* CHEBY2 */


        case CAUER1 : /* input is max. ripple attenuation and module angle */
        case CAUER2 :      /* input is min. stopband att. and module angle */
        {
            int idx_offset;
            double d2PiF, dPow2PiF, dPreMultPolyB, Delta;
            double ApproxErr; /* corresponds to Cauers variable +/-L (max. error) */
            double *pCoeff;
            struct complex *pRootsPow2A, *pRootsPow2B;
            PolyDat PolyBx2; /* B(x)^2 means square of denominator poly in x=s^2 */
            double dIntK;             /* complete elliptic integral */

            /* the module corresponds to kappa^2 in Cauers original book and
               is the so called module = sin(Teta) */
            double dModule = sin(lpDef->dModuleAngle/180.0*M_PI);

            /* malloc mem space for calculation of characteristic function */
            PolyBx2.order = pTmpFlt->b.order;
            if (!MallocPolySpace(&PolyBx2))
            {
                nIdStrErrMsg = IDSTROUTOFMEMORY;   /* out of memory space */
                break;
            } /* if */

            WorkingMessage(STRING_WAIT_CAUER_FILTER);

            pTmpFlt->a.order = PolyBx2.order =
                               (pTmpFlt->b.order / 2) * 2;  /* set to even */

            dIntK = EllIntegr_K(dModule, aMathLimits[IMATHLIMIT_ERRELLIPTIC]);

            /* prepare roots calculation */
            idx_offset = ODD(pTmpFlt->b.order) ? 2 : 1;
            dPreMultPolyB = ApproxErr = 1.0;
            r = pTmpFlt->a.root;                /* roots of numerator poly */
            pRootsPow2A = pTmpFlt->b.root;   /* roots of A(x)^2 with x=s^2 */
            pRootsPow2B = PolyBx2.root;          /* ptr to roots of B(x)^2 */
                                                    /* calc all roots in x */
            for (i = 0; i < pTmpFlt->a.order; i += 2)
            {
                d2PiF = sqrt(dModule)*
                        Sn(dModule, (i+idx_offset)*dIntK/pTmpFlt->b.order);
                dPow2PiF = Sn(dModule, (i+1)*dIntK/pTmpFlt->b.order);
                ApproxErr *= dPow2PiF*dPow2PiF*dModule;

                dPow2PiF = d2PiF * d2PiF;

                /* roots of numerator poly of transfer function T(s) are
                   defined by denominator roots of characteristic
                   polynomial D(s) 
                 */
                r->x = (r+1)->x = 0.0;      
                r->y = 1.0/d2PiF;
                (r+1)->y = -r->y;

                /* next are the numerator roots of A(x)^2, if A(s) is the
                   numerator polynomial of characteristic function D(s) */
                pRootsPow2A->x = (pRootsPow2A+1)->x = -dPow2PiF;
                pRootsPow2A->y = (pRootsPow2A+1)->y = 0.0;

                /* next are the numerator roots of A(x)^2, if A(s) is the
                   numerator polynomial of characteristic function D(s) */
                pRootsPow2B->x = (pRootsPow2B+1)->x = -1.0/dPow2PiF;
                pRootsPow2B->y = (pRootsPow2B+1)->y = 0.0;

                /* correct product pre-multiplier */
                dPreMultPolyB *= dPow2PiF*dPow2PiF; 

                r += 2;                                   /* to next roots */
                pRootsPow2A += 2; pRootsPow2B += 2; 
            } /* for */


            if (ODD(pTmpFlt->b.order))
            {                          /* define last & single root at 0,0 */
                pTmpFlt->b.root[pTmpFlt->b.order-1].x =     /* for A(x)^2) */
                pTmpFlt->b.root[pTmpFlt->b.order-1].y = 0.0;  
                ApproxErr *= sqrt(dModule);     
            } /* if */

            if (lpDef->SubType == CAUER1)  /* design by ripple attenuation */
            {
                Delta = GetDiscrimination(lpDef->dRippleAtt)/ApproxErr;
                if (Delta/ApproxErr < GetDiscrimination(MIN_STOPBAND_ATT_INPUT))
                    nIdStrErrMsg = ERROR_CAUER_MINATTLT3DB;
            } /* if */

            if (lpDef->SubType == CAUER2) /* design by stopband (min.) attenuation */
            {
                Delta = ApproxErr*GetDiscrimination(lpDef->dMinAtt);
                if (Delta*ApproxErr > GetDiscrimination(MAX_RIPPLE_INPUT))
                    nIdStrErrMsg = ERROR_CAUER_RIPPLEGT3DB;
            } /* if */


            Delta *= Delta;                               /* preparation */

            /* calc denominator polynomial of T(s)*T(-s)
             * with T(s)*T(-s) = 1/(1+D(s)*D(-s)) =
                                   B(s)*B(-s)/(B(s)*B(-s) + A(s)*A(-s))
             * note : D(s) = A(s)/B(s)
             * and if even order : A(s) = A(-s), B(s) = B(-s)
             *     if odd order  : A(s) = -A(-s), B(s) = B(-s) 
             * T(s)*T(-s) = B(s)*B(-s)/(B(s)*B(-s) + A(s)*A(-s))
             */

            if (!RootsToCoeffs(&pTmpFlt->b) ||        /* coeffs of A(x)^2 */
                  !RootsToCoeffs(&PolyBx2))       /* and coeffs of B(x)^2 */

                nIdStrErrMsg = IDSTROUTOFMEMORY;   /* out of memory space */


            if (nIdStrErrMsg != IDSTRNULL)
            {
                FreePolySpace(&PolyBx2);
                break; /* CAUER1, CAUER2 */
            } /* if */


            if (ODD(pTmpFlt->b.order))
            {
                PolyBx2.coeff[pTmpFlt->b.order] = 0.0;
                Delta = -Delta;             /* prepare A(s)*A(-s)=-A(x)^2 */
            } /* if */

            /* multiply the numerator poly of characteristic
               function, means A(s)*A(-s), with Alpha (d2PiF)
             * add Alpha*A(x)^2 and B(x)^2
             */
            for (i = 0, pCoeff = pTmpFlt->b.coeff; i <= pTmpFlt->b.order; i++, pCoeff++)
                *pCoeff = *pCoeff*Delta + dPreMultPolyB*PolyBx2.coeff[i];

            FreePolySpace(&PolyBx2);


            InitAbortDlg(hDlg, STRING_ABORT_CAUER);

            /* calc roots of A(x)^2+B(x)^2 with x=s^2, this are the roots
               of square denominator polynomial of transfer function */
            switch(GetPolynomialRoots(&pTmpFlt->b, aMathLimits[IMATHLIMIT_ERRROOTS],
                   CalcRootsBreakProc))
            {
                case CALC_BREAK  :
                    nIdStrErrMsg = ERROR_CAUER_USR_BREAK;   /* User Break */
                    break;

                case CALC_OK     :
                    /* calc complex root of all roots x[i] = w[i]^2 and
                       use such roots with neg. real part to define stabil
                       systems
                     */
                    for (i = 0, r = pTmpFlt->b.root; i < pTmpFlt->b.order; i++, r++)
                    {
                        *r = CplxRoot(*r);
                        if (r->x > 0.0)
                        {
                            r->x = -r->x;
                            r->y = -r->y;
                        } /* if */
                    } /* for */

                    /* calc coefficients by roots to get lowpass cutoff */
                    if (RootsToCoeffs(&pTmpFlt->b) &&       
                        RootsToCoeffs(&pTmpFlt->a))
                    {
                        /* search cutoff frequency for this Cauer filter 
                         * transition area from passband into stopband lies
                           between w[n-1] and 1/w[n-1], where
                           w[i] = sqrt(k)*sn(k,i/n*B)
                         * k is the module of the elliptic integral and B
                           the complete elliptic integral
                         * n = filter order 
                         */
                        dPow2PiF = sqrt(dModule) * Sn(dModule, (double)(pTmpFlt->b.order-1)*
                                                               dIntK/pTmpFlt->b.order);

                        switch(GetLowpassCutoff(pTmpFlt, dPow2PiF, 1.0/dPow2PiF,
                                                &dCurrentCutoffLP))
                        {                   
                            case CALC_BREAK  :
                                nIdStrErrMsg = ERROR_CAUER_USR_BREAK; 
                                break;

                            case CALC_OK     :
                                nIdStrErrMsg = IDSTRNULL;
                                break; /* CALC_OK */
                        } /* switch */
                    } /* if */

                    break; /* CALC_OK */
            } /* switch */

            EndAbortDlg();
            break;
        } /* CAUER */

        case BESSEL :
        {
            WorkingMessage(STRING_WAIT_BESSEL_FILTER);

            nIdStrErrMsg = IDSTROUTOFMEMORY;
            pTmpFlt->a.coeff[0] = 1.0;

            /* get B(s) = Bessel polynomial */
            if (GetBesselPoly(pTmpFlt->b.order, pTmpFlt->b.coeff)) 
            {
                /* search current cutoff frequency for this Bessel polynomial
                 * search only the pos. results !
                 */
                InitAbortDlg(hDlg, STRING_ABORT_BESSEL);

                switch(GetLowpassCutoff(pTmpFlt, 0.0, 10*pTmpFlt->b.order,
                                        &dCurrentCutoffLP))
                {                   /* define roots of Bessel poly */
                    case CALC_BREAK  :
                        nIdStrErrMsg = ERROR_BESSEL_USR_BREAK; /* User Break */
                        break;

                    case CALC_OK     :

                        /* calc the roots of nominator polynomial for
                           possible frequency transformation
                         */
                        switch(GetPolynomialRoots(&pTmpFlt->b,
                                                  aMathLimits[IMATHLIMIT_ERRROOTS],
                                                  CalcRootsBreakProc))
                        {                   /* define roots of Bessel poly */
                            case CALC_OK :
                                nIdStrErrMsg = IDSTRNULL;
                                break;

                            case CALC_BREAK  :
                                nIdStrErrMsg = ERROR_BESSEL_USR_BREAK; /* User Break */
                                break; /* CALC_BREAK */
                        } /* switch */

                        break; /* CALC_OK */

                } /* switch */

                EndAbortDlg();
            } /* if */
            break;
        } /* case BESSEL */
    } /* switch */

    if (nIdStrErrMsg == IDSTRNULL)
    {
        /* normation of roots into target lowpass frequency band */
        FactorCplxArr(pTmpFlt->b.order, pTmpFlt->b.root, dTargetCutoffLP/dCurrentCutoffLP);
        FactorCplxArr(pTmpFlt->a.order, pTmpFlt->a.root, dTargetCutoffLP/dCurrentCutoffLP);

        /* frequency transformation */
        FTransfRootsSDomain(pTmpFlt->FTr.FTransf, dTargetCutoffLP, dQ,
                            &pTmpFlt->a.order, pTmpFlt->a.root,
                            &pTmpFlt->b.order, pTmpFlt->b.root);
        DomainTransfRoots(pTmpFlt);   /* S to Z domain transform of roots */
        if (!(RootsToCoeffs(&pTmpFlt->a) &&       /* calc coeffs by roots */
              RootsToCoeffs(&pTmpFlt->b)))   nIdStrErrMsg = IDSTROUTOFMEMORY;
    } /* if */

    if (nIdStrErrMsg == IDSTRNULL)
    {
        for (i = 0; i <= pTmpFlt->a.order/2; i++)  /* sort to poly in z^-1 */
        {
            SwapDouble(&pTmpFlt->a.coeff[i], &pTmpFlt->a.coeff[pTmpFlt->a.order-i]);
            SwapDouble(&pTmpFlt->b.coeff[i], &pTmpFlt->b.coeff[pTmpFlt->a.order-i]);
        } /* for */

        nIdStrErrMsg = NormFilterTransfer(pTmpFlt);
    } /* if */

    if (nIdStrErrMsg != IDSTRNULL) FreeFilter(pTmpFlt); /* an error occurs */
    else
    {
        if (pDestFlt->f_type != NOTDEF)            /* was memory allocated */
            FreeFilter(pDestFlt);       /* release memory of target filter */
        *pDestFlt = *pTmpFlt;        /* set temp to current filter def */
    } /* else */

    WorkingMessage(0);
    return nIdStrErrMsg;
}


/* Definition of predefined FIR/IIR time discret systems loaded from
   resources file 
 * returns the error string id IDSTRNULL if ok, else returns any other id
 * the return value IDSTROUTOFMEMORY is only a pseudo string identifier
   (do not use)
 */

int SetPredefFilter(HINSTANCE hinst, tFilter *lpSrcFlt, tFilter *lpFlt)
{
    int i;

    LPSTR p = LoadRCdata(hinst, lpSrcFlt->DefDlg.PredefSub);

    if (p == NULL) return ERROR_LOAD_FILTER_DATA;
    p += lstrlen(p)+1;                         /* skip filter description */
    p += lstrlen(p)+1;                         /* skip filter name */

    lpSrcFlt->f_type = (tGenType)(*((int *)p)); /* set type (FIR, IIR,..) */
    p += sizeof(int);

    /* memory alloc for coeff and root space in numerator and
     * denominator polynomial */
    lpSrcFlt->a.order = *((int *)p);
    p += sizeof(int);
    lpSrcFlt->b.order = *((int *)p);;    /* for alloc */
    p += sizeof(int);
    if (!MallocFilter(lpSrcFlt))
    {
        FreeRCdata();
        return IDSTROUTOFMEMORY;                        /* memory space ? */
    } /* if */

    lpSrcFlt->uFlags = 0;                                  /* clear flags */

    for (i=0; i<=lpSrcFlt->a.order; i++)
        lpSrcFlt->a.coeff[i] = strtod((char *)p, (char **)(&p));

    for (i=0; i<=lpSrcFlt->b.order; i++)
        lpSrcFlt->b.coeff[i] = strtod((char *)p, (char **)(&p));

    lpSrcFlt->factor = strtod((char *)p, (char **)(&p)); /* read pre-multiplier */

    FreeRCdata();

    if (lpFlt->f_type != NOTDEF)                  /* was memory allocated */
        FreeFilter(lpFlt);             /* release memory of target filter */
    *lpFlt = *lpSrcFlt;                 /* set temp to current filter def */

    return IDSTRNULL;
} /* SetPredefFilter */




/* Definition of misc. FIR/IIR time discret systems 
 * returns the error string id or null if ok 
 * IDSTROUTOFMEMORY is only a pseudo string identifier (do not use)
 */

int DefineMiscDigSys(tFilter *lpSrcFlt, tFilter *OldFilter)
{
    int i, ic;
    double *Vec;                            /* ptr to coefficients vector */
    UINT nIdStrErrMsg = IDSTRNULL;
    tVarSys *lpPar = &lpSrcFlt->DefDlg.MiscFltDat;     /* for fast access */

    /* set correct numerator an denominator polynomial order for
       memory allocation */

    lpSrcFlt->a.order = lpPar->nOrder;

    switch (lpPar->SubType)              /* check which filter type is it */
    {
        case AVG_EXP :
            lpSrcFlt->a.order = 0;           /* T(z) = 1/(N - (N-1)*z^-1) */
                                                      /* and fall through */
        case AVG_IIR :
            lpSrcFlt->b.order = 1;    /* denominator polynomial: 1 - z^-1 */
            break; /* AVG_IIR */

        case HILBERT_TRANSF90 :                    /* hilbert transformer */
        case INTEGR_FSDEV:  /* integrator (deriation by Fourier sequence) */
        case DIFF_FSDEV:/* differantiator (deriation by Fourier sequence) */
        case COMB :                         /* comb filter with parameter */
        case AVG_FIR :
            lpSrcFlt->b.order = 0;/* default denominator polynomial order */
            break; /* HILBERT_TRANSF90, INTEGR_FSDEV, DIFF_FSDEV, COMB, AVG_FIR */
    } /* switch */

    /* memory alloc for coeff and root space in nominator and
     * denominator polynomial */

    if (!MallocFilter(lpSrcFlt)) return IDSTROUTOFMEMORY;

    /* default settings (lin. FIR, a[0]=b[0]=1, ...) */
    lpSrcFlt->factor = 1.0;                                   /* not used */
    lpSrcFlt->uFlags = 0;                              /* clear all flags */
    lpSrcFlt->b.coeff[0] = lpSrcFlt->a.coeff[0] = 1.0;  /* default coeffs */
    ic = lpPar->nOrder/2;                                 /* center index */
    lpSrcFlt->f_type = LINFIR;          /* default type of digital system */
    Vec = lpSrcFlt->a.coeff;

    switch (lpPar->SubType)
    {
        case HILBERT_TRANSF90 :                    /* hilbert transformer */
            Vec[ic] = 0.0;                           /* c[0] = a[N/2] = 0 */

            for (i = 1; i <= ic; i++)  /* i means index of Fourier coeffs */
            {
                if (ODD(i))
                {
                    Vec[ic-i] = -M_2_PI/i;  
                    Vec[ic+i] = -Vec[ic-i];
                } /* if */
                else Vec[ic-i] = Vec[ic+i] = 0.0;
            } /* for */

            break; /* HILBERT_TRANSF90 */

        case INTEGR_FSDEV:  /* integrator (deriation by Fourier sequence) */

            WorkingMessage(STRING_WAIT_INTEGRATOR); /* integration time ! */

            Vec[ic] = 0.5;                          /* center coefficient */

            for (i = 1; i <= ic; i++)
            {
                Vec[ic-i] = 0.5 - M_1_PI*SineIntegral(i*M_PI, aMathLimits[IMATHLIMIT_ERRSI]);
                Vec[ic+i] = 1.0 - Vec[ic-i]; /* 0.5 + M_1_PI*SineIntegral(i*M_PI) */
            } /* for */                  /* not antimetric -> non lin. FIR */

            lpSrcFlt->f_type = FIR;                  /* correction of type */

            if (!NormGainTo(&lpSrcFlt->a, lpSrcFlt, 0.0, 1.0)) /* norm to 1 */
                nIdStrErrMsg = ERROR_FILTER_IMPLEMENT;

            WorkingMessage(0);
            break; /* INTEGR_FSDEV */

        case DIFF_FSDEV:/* differantiator (derivation by Fourier sequence) */
            Vec[ic] = 0.0;                           /* c[0] = a[N/2] = 0 */

            for (i = 1; i <= ic; i++)
            {
                Vec[ic-i] = -M_1_PI/i;
                if (!ODD(i)) Vec[ic-i] *= -1;              /* change sign */
                Vec[ic+i] = -Vec[ic-i];    /* because antimetric lin. FIR */
            } /* for */
            break; /* DIFF_FSDEV */


        case AVG_FIR :                           /* moving average filter */
            for (i = 0; i <= lpSrcFlt->a.order; i++)
                Vec[i] = 1.0/(lpSrcFlt->a.order+1);
            break; /* AVG_FIR */

        case AVG_IIR:       /* moving average filter (IIR implementation) */
            lpSrcFlt->b.coeff[1] = -1.0;          /* polynomial: 1 - z^-1 */
            for (i = 1; i < lpSrcFlt->a.order; i++) Vec[i] = 0.0;
            Vec[0] = 1.0/lpSrcFlt->a.order;
            Vec[lpSrcFlt->a.order] = -Vec[0];  /* poly: 1/N*(1 - z^-n) */
            lpSrcFlt->f_type = IIR;                 /* correction of type */
            break; /* AVG_IIR */

        case COMB :                                        /* comb filter */
            for (i = 1; i < lpSrcFlt->a.order; i++) Vec[i] = 0.0;
            Vec[0] = 0.5;
            Vec[lpSrcFlt->a.order] = -0.5;  /* polynomial: 0.5*(1 - z^-n) */
            break; /* COMB */

        case AVG_EXP :                      /* exponential average filter */
            lpSrcFlt->b.coeff[0] = lpPar->nOrder;   /* 1/(N - (N-1)*z^-1) */
            lpSrcFlt->b.coeff[1] = 1-lpPar->nOrder;
            lpSrcFlt->f_type = IIR;                 /* correction of type */
            break; /* AVG_EXP */
    } /* switch */

    if (nIdStrErrMsg == IDSTRNULL)
        if (!CheckImplementation(lpSrcFlt))
            nIdStrErrMsg = ERROR_FILTER_IMPLEMENT;

    if (nIdStrErrMsg != IDSTRNULL)   /* release allocated memory on error */
        FreeFilter(lpSrcFlt);
                                
    else                                                 /* definition ok */
    {
        if (OldFilter->f_type != NOTDEF)
            FreeFilter(OldFilter);        /* release memory of old filter */
        *OldFilter = *lpSrcFlt;         /* set temp to current filter def */
    } /* else */

    return nIdStrErrMsg;
} /* DefineMiscDigSys() */

