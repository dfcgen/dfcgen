/* math. support functions for filter designer :
 * windowing functions, operations with polynomials (roots calculation, ...)
 * copyright (c) Ralf Hoppe 1994-1998
 */

#include <SIGNAL.H>
#include <STDLIB.H>
#include <FLOAT.H>
#include <VALUES.H>
#include "FDMATH.H"
#include "FDERROR.H"

#ifndef MALLOC
#define MALLOC(size)           malloc(size)
#endif

#ifndef FREE
#define FREE(ptr)              free(ptr)
#endif

#ifndef REALLOC
#define REALLOC(ptr)           realloc(ptr)
#endif


#define EXTENDED               long double  /* machine dependent max. prec */

#if sizeof(EXTENDED) == sizeof(double)
#define FABS(x)                 fabs(x)       /* standard double functions */
#define SQRT(x)                 sqrt(x)
#define MAXEXT                  MAXDOUBLE
#else
#define FABS(x)                 fabsl(x)          /* long double functions */
#define SQRT(x)                 sqrtl(x)
#define MAXEXT                  LDBL_MAX
#endif



/* if iteration of bessel calculation overflows use approx. for infinite x */
#define BESSEL_OVERFLOW (2.0/DBL_EPSILON)

/* if iteration of Si(x) calculation overflows use simpson integal for calc */
#define SI_OVERFLOW     (2.0/DBL_EPSILON)


                              
/* prototypes of local functions */
static void (*OpenLocalMathErr(void))(int sig, int type);
static void LocalMathErrHandler(int sig, int type);
static BOOL WasLocalMathErr(void);
static void CloseLocalMathErr(FNSIGNALHANDLER fnErrHandler);
static BOOL IsOutOfTol(double MaxErr, double AbsErr, double ResEstimate);


static void GetCorrOfFunctions(int, EXTENDED, EXTENDED, EXTENDED *, EXTENDED *,
                               EXTENDED *, EXTENDED *);
static void GetRootsPoly2(EXTENDED, EXTENDED, struct complex *);
static double DiffPoly(double , int, int, double *);
static double GetMaxAbsOfPolyRoots(int, double *);
static double EllipticIntegrArgFunc1(double phi);
static double IntegrSimpson(double start, double stop, int n, MATHFUNCDIM2 func);
static double IntegrTrapez(double start, double stop, int n, MATHFUNCDIM2 func);
static double IntegrRect(double start, double stop, int n, MATHFUNCDIM2 func);
static double BesselIntegrArgFunc(double phi);
static BOOL ExtDiv(EXTENDED, EXTENDED, EXTENDED *);
static double skalar(int n, double v1[], double v2[]);



/* local variables */
volatile static BOOL bLocalMathErr;   /* local error variable for floating point errors */


/* math. error handling */

static void (*OpenLocalMathErr())()
{
    bLocalMathErr = FALSE;
    return signal(SIGFPE, LocalMathErrHandler);
} /* OpenLocalMathErr() */



#pragma argsused
static void LocalMathErrHandler(int sig, int type)
{
    bLocalMathErr = TRUE;
    (void)signal(SIGFPE, LocalMathErrHandler);
} /* LocalMathErrHandler() */


static BOOL WasLocalMathErr()
{
    return bLocalMathErr;
} /* WasLocalMathErr() */


static void CloseLocalMathErr(void (*fnOldErrHandler) ())
{
    (void)signal(SIGFPE, fnOldErrHandler);
} /* CloseLocalMathErr() */


/* returns TRUE if passed error is out of tolerance
 */
static BOOL IsOutOfTol(double MaxErr, double AbsErr, double dValue)
{
    double RelErr;

    AbsErr = fabs(AbsErr);

//    if (!ProtectedDiv(AbsErr, fabs(dValue), &RelErr)) RelErr = MaxErr;
//    return (AbsErr > MaxErr) && (RelErr >= MaxErr);
    if (!ProtectedDiv(AbsErr, fabs(dValue), &RelErr)) RelErr = AbsErr;
    return RelErr >= MaxErr;
} /* IsOutOfTol() */



/* various math functions */

double si(double x)
{
    double dResult;

    if (fabs(x) > 2.0*DBL_EPSILON) dResult = sin(x)/x;
    else
        if (!ProtectedDiv(sin(x), x, &dResult)) dResult = 1.0;

    return dResult;
} /* si() */


double si2(double x)
{
  return(si(x)*si(x));
}

double arsinh(double x)       /* area sinus hyperbolicus */
{
    return log(x + sqrt(x*x + 1.0));
}


double arcosh(double x)       /* area cosinus hyperbolicus 1 < x < +INF */
{
    if (x >= 1.0) return log(x - sqrt(x*x - 1.0));
    else return 0.0;
}


/* if |x| < 1 use : T(x) = cos(n*arcos(x))
 * if  x >= 1 use : T(x) = cosh(n*arcosh(x))    because cosh(jx) = cos(x)
 * if x < -1 you get a little problem, because cosh(x+j*Pi) = - cosh(x)
 * and therfore arcosh(-x) = arcosh(|x|) + j*Pi ,
 * then use T(x) = (-1)^n * cosh(n*arcosh(|x|))
 */

double Chebyshev(double dOrder, double x)
{
    double dResult;
    if (fabs(x) < 1.0) return cos(dOrder * acos(x));

    dResult = cosh(dOrder * arcosh(fabs(x)));
    if ((x >= 1.0) || (!ODD((int)dOrder))) return dResult;

    return -dResult;
}


/* Jacobian function, also called invers function of elliptic integral F(k, y)
   y = sn(k, x) with k = modul,  x = F(k, arcsin(y))
 * with substitution w = arcsin(y) : x = F(k, w)
 * note : the name invers function isn't correct because w is the invers
          function of F(k, x)
 * since sn(k, x) is periodic with 2*F(k,Pi) reduction of argument
   into interval [0, 2*F(k,Pi)] is possible
 * use Newton method to calculate w by calculation of roots with following
   formula : F(k, w) - x = 0
 * w[i+1] = w[i] - (F(k, w[i]) - x)/F'(k, w[i])
          = w[i] - (F(k, w[i]) - x)*sqrt(1-k*k*sin(w[i])^2)
          = w[i] - (F(k, w[i]) - x)*sqrt(1-k*k/2*(1-cos(2*w[i])))
 * notes :
 * I  : first deviation of elliptic integral
        F'(k, w) = 1/sqrt(1-k*k*sin(w)^2) = 1/sqrt(1-k*k*0.5(1-cos(2y))
 * II : second deviation of elliptic integral
        F''(k, w) = k*k*sin(w)*cos(w)*[F'(k,w)]^3
                  = k*k/2*sin(2y)*[F'(k,w)]^3
 * III: sin(x)^2 = 0.5*(1 - cos(2x))
 * fixed point equation : phi(w) = w - (F(k,w) - x)/F'(k,w)
 * convergence criteria : m = |phi'(w)| < 1
                          k*k/2*sin(2y)*F'(k,w)*(F(k,w)-x) < 1
 * in general for Newton method : m = | f(x[i])*f''(x[i])/[f'(x[i])]^2 |
   and with this the abs. error estimation : |x(i+1)-root| < m/(1-m)*|x(i+1)-x(i)|
 *
 * with w also y is known : y=sin(w)
 */

double JacobiSN(double k, double x, double MaxErr)
{
    double m, y_old, Deriv_F_Inv, Jacobi_x;
    double AbsErr = FDMATHERRMAXRESULT, AbsErrOld = FDMATHERRMAXRESULT;

    double pow_k_2 = k*k*0.5;
    double y = 1.0;
    int sign_x = SIGN(x);

    x = fabs(x);               /* because floor only correct at pos. axis */
    y_old = 4.0*EllIntegr_K(k, MaxErr);        /* period of sn(k,x) is 4K */
    x -= y_old*floor(x/y_old);                   /* reduction of argument */

    do
    {
        Deriv_F_Inv = sqrt(1.0 - pow_k_2*(1.0-cos(2*y)));    /* 1/F'(k,y) */
        Jacobi_x = EllIntegr_F(k, y, MaxErr) - x;
        m = pow_k_2*fabs(Jacobi_x*sin(2*y))/Deriv_F_Inv;      /* criteria */

        if ((m >= 1.0) || (AbsErrOld < AbsErr))           /* divergence ? */
        {                             /* attention: do not check only m ! */
            y = x*rand()/RAND_MAX;  /* derivation of invers F(x) is lt. 1 */
            AbsErr = AbsErrOld = FDMATHERRMAXRESULT;
        } /* if */
        else                                  /* convergence of iteration */
        {
            y_old = y;                                 
            y -= Jacobi_x * Deriv_F_Inv;              /* next Newton step */
            AbsErrOld = AbsErr;    
            (void)ProtectedDiv(m*fabs(y-y_old), 1.0-m, &AbsErr);
        } /* else */
    } /* do ... while */
    while (IsOutOfTol(MaxErr, AbsErr, y));

    return sign_x*sin(y);
} /* JacobiSN */



double JacobiDN(double k, double x, double MaxErr)
{
    double sn = JacobiSN(k, x, MaxErr);
    return sqrt(1.0-k*k*sn*sn);
} /* JacobiDN() */


/* bessel function of first kind, n'th order I(n,x)
               INF                          k      n        2*k
 *    I(n,x) = Sum S(k)  with    S(k) = (-1) * (x/2) * (x/2)  / [k!*(n+k)!]
               k=0
                                                        2
 *                               S(k) = - S(k-1) * (x/2)  / [k*(n+k)]
                                             n
 *                               S(0) = (x/2) / n!
 
 * note that I(n,-x) = (-1)^n * I(n,x)
 
 * if an overflow occurs calculation will continued by integration :
   I(n,x) = 1/Pi * Integral(n*phi - x*sin(phi)) from phi=0 to phi=Pi
  
 * the abs. or rel. error limits the number of iteration loops
 */

static double Bessel_x;
static int BesselOrd;


static double BesselIntegrArgFunc(double phi)
{
    return cos(BesselOrd*phi - Bessel_x*sin(phi));
} /* BesselIntegrArgFunc() */


double bessel(int nOrd, double x, double MaxErr)  /* Bessel function of any order, first kind */
{
    int k;
    double x2_2 = x*x*0.25;                                     /* (x/2)^2 */
    EXTENDED Result;
    EXTENDED S = 1.0;                                  /* if order equal 0 */

    FNSIGNALHANDLER fnOldErrHandler = OpenLocalMathErr();

    for (k = 1; k <= nOrd; k++)
    {
        if (WasLocalMathErr() || (FABS(S) > BESSEL_OVERFLOW))
        {
            CloseLocalMathErr(fnOldErrHandler);
            return 0.0;
        } /* if */

        S *= (EXTENDED)x * 0.5 / k;                          /* calc S(0) */
    } /* for */

    Result = 0.0;
    k = 1;

    do
    {
        Result += S;
        S = -S * x2_2/k/(nOrd + k);
        ++k;
        if (WasLocalMathErr() ||
            (FABS(Result) >= BESSEL_OVERFLOW) ||            /* overflow ? */
            (FABS(S) >= BESSEL_OVERFLOW))               /* no convergence */
        {
            Bessel_x = x;
            BesselOrd = nOrd;
            Result = Integrate(0.0, M_PI, MaxErr, BesselIntegrArgFunc)/M_PI;
            break; /* do ... while */
        } /* if */
    } /* do ... while */
    while (IsOutOfTol(MaxErr, S, Result));

    CloseLocalMathErr(fnOldErrHandler);
    return (double)Result;
} /* bessel() */



/* elliptic integral F(k, x) = Integral(1/sqrt(1-k*k*sin(u)^2)) from
   u=0 to u=x integration argument function
 */
static double EllipticModul;                   /* global passed parameter */

static double EllipticIntegrArgFunc1(double u)
{
    double y;

    if (!ProtectedDiv(1.0,
                      sqrt(1.0 - EllipticModul*EllipticModul*0.5*(1.0-cos(2.0*u))),
                      &y))
        y = 0.0;

    return y;
} /* EllipticIntegrArgFunc1() */


/* elliptic integral of first kind with module k at position x
   F(k,x) = Integral(1/sqrt(1 - k*k * sin^2(x)))
 * note : because F(k, x+n*Pi) = F(k,x) + n*F(k,Pi) reduction of argument
   x to interval [0,Pi] is possible

 * following results in relation to MathCad 4.0 (TOL=1E-14) are calculated
   (at IBM-PC with double format 53 Bit mantisse)

                    MathCad       EllIntegr_F (nSteps)
                                          50                100               500                1000
   F(0.1,Pi)   3.149491123034712  3.149491123034712  3.149491123034712  3.149491123034710  3.149491123034714
   F(0.5,Pi)   3.371500709625192  3.371500709625191  3.371500709625192  3.371500709625191  3.371500709625190
   F(0.9,Pi)   4.561098276845521  4.561098276871062  4.561098276845539  4.561098276845544  4.561098276845533
   F(0.95,Pi)  5.180022461749018  5.180022502289042  5.180022461748998  5.180022461748997  5.180022461748998

 * at higher modulus k it's not so clear which program calculates the
   precisious results


 * by varying the number of integration steps following results in relation
   to 32764 integration steps (1000 trials) are detected
 * k in range 0.0174=sin(1°) < k < 0.9848=sin(80°) and equal distributed
 * x in range 0 < x < Pi and equal distributed

          Average-Error  Peak-Error     at F(k,x)
   Steps
     50    2.9E-8          6.1E-6          F(0.98108,2.78311)
    100    2.1E-9          4.9E-7          F(0.98359,1.49961)
    200    7.5E-11         6.4E-9          F(0.956044,1.67583)
    500    3.6E-12         8.7E-10         F(0.9847,1.6021)
   1000    1.5E-13         1.3E-11         F(0.966407,1.7463)

 * it seems, that 500 integration steps is one of the best choice
 */
double EllIntegr_F(double Modul, double x, double MaxErr)
{
    double ElliPi, RatioPi, sign_x;

    EllipticModul = Modul;
    sign_x = SIGN(x);
    x = fabs(x);                 /* because floor only correct at pos. axis */

    ElliPi = Integrate(0.0, M_PI, MaxErr, EllipticIntegrArgFunc1); /* F(k, Pi) */
    RatioPi = floor(x/M_PI);
    x -= RatioPi*M_PI;            /* decimation of argument to range [0,Pi] */
    return sign_x * (RatioPi*ElliPi + Integrate(0.0, x, MaxErr, EllipticIntegrArgFunc1));
} /* EllIntegr_F() */



/* Complete elliptic integral of the first kind K(x) = F(Pi/2; x)
 */
double EllIntegr_K(double x, double MaxErr)
{
    if (fabs(x) > 1.0) raise(SIGFPE); /* module greater than 1.0 undefined */
    return EllIntegr_F(fabs(x), M_PI_2, MaxErr);
} /* EllIntegr_K() */


/* sine integral Si(x) = integrate(sin(u)/u) from 0 to x
 *  Si(x) = x - x^3/3/3! + x^5/5/5! - x^7/7/7! + ...
 * the first deviation of argument is cos(x)/x-sin(x)/x^2 and the maximum
   in interval [0,x] is less then
 * 'MaxErr' is the iteration error limit - iteration stops if the abs. error
   (current partial sum in 'Term') or the rel. error falls below 'MaxErr'

 */
double SineIntegral(double x, double MaxErr)
{
    double Term;
    double Result = 0.0;
    double fak = 1.0;
    long i = 1L;
    double power_x = x;

    FNSIGNALHANDLER fnOldErrHandler = OpenLocalMathErr();

    do
    {
        Term = power_x/i/fak;
        Result += Term;
        power_x *= -x*x;
        fak *= ++i;
        fak *= ++i;

        if ((fabs(power_x) > SI_OVERFLOW) || (fabs(Term) > SI_OVERFLOW) ||
            (fabs(Result) > SI_OVERFLOW) || (fabs(fak) > SI_OVERFLOW) ||
            WasLocalMathErr())
        {
            Result = Integrate(0.0, x, MaxErr, si);
            break; /* do ... while */
        } /* if */
    } /* while */
    while (IsOutOfTol(MaxErr, Term, Result));

    CloseLocalMathErr(fnOldErrHandler);
    return Result;
} /* SineIntegral() */



/* all known windowing functions             *
 * all functions return with 0.0 if x<0 or x>1       */

double hamming(double x)
{
    if ((x < 0.0) || (x > 1.0)) return 0.0;
    return 0.54 - 0.46*cos(2*M_PI*x);
}


double hanning(double x)
{
    if ((x < 0.0) || (x > 1.0)) return 0.0;
    return 0.5 - 0.5*cos(2*M_PI*x);
}


double blackman(double x)
{
    if ((x < 0.0) || (x > 1.0)) return 0.0;
    return 0.42 - 0.5*cos(2*M_PI*x) + 0.08*cos(4*M_PI*x);
}



double kaiser(double alpha, double x, double MaxErr)
{
    if ((x < 0.0) || (x > 1.0)) return 0.0;
    return bessel(0, 2.0*alpha*sqrt(x-x*x), MaxErr)/bessel(0, alpha, MaxErr);
} /* kaiser() */

double triangle(double x)
{
    if ((x < 0.0) || (x > 1.0)) return 0.0;
    return 1.0 - fabs(1.0-2*x);
}


double rectangle(double x)   /* returns 1  if x between 0 and +1.0 */
{
    if ((x < 0.0) || (x > 1.0)) return 0.0;
    return 1.0;
}


/* polynomial functions */
void MovePolyCoeffs(PolyDat *dest, PolyDat *src)
{
    int i;
    for (i=0;i <= src->order; i++)
        dest->coeff[i] = src->coeff[i];
    dest->order = src->order;
}

BOOL ShiftPolyCoeffs(int nStages, PolyDat *lpPoly)
{
    int i;
    if (nStages > lpPoly->order) return FALSE;       /* is'nt possible */

    lpPoly->order -= nStages;

    for (i=0; i <= lpPoly->order; i++)
        lpPoly->coeff[i] = lpPoly->coeff[i+nStages];

    return TRUE;
}

/* n'th differentiation of poly with order nOrd
 * example : first diff :
 *         n         k
 * P(X) =  ä a[k] * X  = (...((a[n]*X + a[n-1])*X + a[n-2])*X +...+ a[1])*X + a[0]
 *        k=0
 *
 *          n             k-1
 * P'(X) =  ä k * a[k] * X   =
 *         k=1
 *
 * P'(x) =  (...((a[n]*n*X + a[n-1]*(n-1))*X + a[n-2]*(n-2))*X +...+ a[2]*2)*X + a[1]
 *  m       n                               k-m
 * P (X) =  ä k*(k-1)*...*(k-m+1) * a[k] * X
 *         k=m
 *
 */
static double DiffPoly(double x, int NumDiff, int nOrd, double coeffs[])
{
    register int k, i;
    double d;
    double dRes = 0.0;
    for (k = nOrd; k >= NumDiff; k--)
    {
        dRes *= x;
        for (i=k, d=coeffs[k]; i>k-NumDiff; i--) d *= i;
        dRes += d;
    } /* for */
    return dRes;
}


/* returns the coeffs of bessel polynomial n'th order in coeff[]
 * return value : FALSE, if not enough memory
 * B[n] = (2*n-1) * B[n-1] + s^2 * B[n-2]
 * B[0] = 1
 * B[1] = 1+s
 */
BOOL GetBesselPoly(int nOrd, double coeff[])
{
    int nDeg, i;
    double dCoeffOld;
    double *polyOld_2 = (double *)MALLOC((1+nOrd)*sizeof(coeff[0]));

    coeff[0] = 1.0;                        /* B[0] = 1 */
    if (nOrd > 0) coeff[1] = 1.0;          /* B[1] = 1+s */
    if (polyOld_2 == NULL) return FALSE;
    polyOld_2[0] = coeff[0];               /* B[n-2] (n=2) */

    for (nDeg = 2; nDeg <= nOrd; nDeg++)
    {
        coeff[nDeg] = 0.0;
        for (i = nDeg; i >= 0; i--)   /* from top to bottom for all coeffs */
        {
            dCoeffOld = coeff[i];                                  /* save */
            coeff[i] *= 2*nDeg-1;
            if (i > 1) coeff[i] += polyOld_2[i-2];   /* new coeff complete */
            polyOld_2[i] = dCoeffOld;                 /* save B[n-2] coeff */
        } /* for */
    } /* for */
    FREE(polyOld_2);
    return TRUE;
}


/* integration of function func in interval [start,stop] by Simpson
   method using n samples
 * error estimation is possible by fourth deviation of argument f(x)
   E < (stop-start)/180*max|f''''(u)|*(h/2)^4 with h = (stop-start)/n
   and start < u < stop
 */
static double IntegrSimpson(double start, double stop, int n, MATHFUNCDIM2 func)
{
    int i;
    double Sum, h = (stop-start)/n;

    n = (n/2)*2;             /* set to even steps for simpson integration */

    Sum = func(start)+func(stop);
    for (i = 1; i < n; i += 2) Sum += 4*func(start+i*h);
    for (i = 2; i < n; i += 2) Sum += 2*func(start+i*h);
    return Sum/3.0*h;
} /* IntegrSimpson() */


/* integration of function func in interval [start,stop] by trapezoidal
   (secant) method using n subintervals (n+1 samples)
 * error estimation is h^2/12*max|f''(u)|*(stop-start)
 */
static double IntegrTrapez(double start, double stop, int n, MATHFUNCDIM2 func)
{
    int i;
    double Sum;
    double h = (stop-start)/n;

    Sum = 0.5*(func(start)+func(stop));
    for (i = 1; i < n; i++) Sum += func(start+i*h);
    return Sum*h;
} /* IntegrTrapez() */



/* integration of passed function f(x) in intervall [start,stop] using
   rectangular method
 * the error estimation is E < h/4*max|f'(x)|*(stop-start) with
   h=(stop-start)/n
 */
static double IntegrRect(double start, double stop, int n, MATHFUNCDIM2 func)
{
    int i;
    double Sum = 0.0, h = (stop-start)/n;;

    for (i = 0; i < n; i ++) Sum += func(start+i*h);

    return Sum*h;
} /* IntegrRect() */


/* integration of argument func in [start,stop] interval using Romberg
   algorithm
 * iteration by calculation of multiple integrals with descending step
   width q = ITGR_DIVIDER = 2 (geometric) and extrapolation at step
   width zero (should be the correct value of integral)
 * single integration done by trapezoidal method
 * first estimate (i = 1) is given by the crosspoint of line fitting
   I[0] and I[1] through the y-axis at : (I[0]-q*I[1])/(1-q)
 * at i == 0 (single point) estimation isn't possible, set to I[0]

 * building new coefficients of numerator from equation
   numerator := c[0]*I[0] + (c[1]-q**i*c[0])*I[1] + (c[2]-q**i*c[1])*I[2] + ...
                + ( -q**i*c[i-1])*I[i])
 */

#define ITGR_DIVIDER     2             /* geometric descending step width */
#define ITGR_INTERVSTART (ITGR_DIVIDER*ITGR_DIVIDER*ITGR_DIVIDER)

double Integrate(double start, double stop, double MaxErr, MATHFUNCDIM2 func)
{
    int i;             /* index of iteration, (i+1) = number of integrals */
    int qn;           /* q**i everytime */
    int cDx;
    double Estimate[3];                       /* last Romberg estimations */
    double Integral[16];                      /* all calculated integrals */
    double Coeff[16];                             /* extrapolation coeffs */
    double Num, Den;

    i = 1;      /* set iteration index (I[0] calculated, calc I[1] first) */
    qn = ITGR_DIVIDER;
    cDx = ITGR_INTERVSTART;     /* start with this number of subintervals */
    Den = Coeff[0] = 1.0;

    Estimate[2] = (double)MAXFLOAT; /* don`t match iteration end criteria */
    Estimate[1] = -(double)MAXFLOAT;
    Estimate[0] = Integral[0] = IntegrTrapez(start, stop, cDx, func);
    cDx *= ITGR_DIVIDER;     /* increase number of subintervals geometric */

    do
    {
        int k;

        Estimate[2] = Estimate[1];         /* save last Romberg estimates */
        Estimate[1] = Estimate[0];         

        /* calculate next integral with increased number of sample points
         * multiplication with ITGR_DIVIDER supports at start of
           iteration more than one interval
         */
        Integral[i] = IntegrTrapez(start, stop, cDx, func);

        Coeff[i] = 0.0;  
         
        for (k = i; k > 0; k--)     /* from top to bottom calc new coeffs */
            Coeff[k] -= qn*Coeff[k-1];
         
        Num = skalar(++i, Coeff, Integral);
        Den *= 1.0 - qn;
        if (!ProtectedDiv(Num, Den, &Estimate[0])) return Estimate[0];

        qn *= ITGR_DIVIDER;
        cDx *= ITGR_DIVIDER;
    } /* do ... while */
    while ((cDx <= MAXINT/ITGR_DIVIDER) &&          /* too much intervals */
           IsOutOfTol(MaxErr, Estimate[0] - Estimate[1], Estimate[0]) &&
           IsOutOfTol(MaxErr, Estimate[1] - Estimate[2], Estimate[1]));

    return Estimate[0];              /* return with last Romberg estimate */
} /* Integrate() */


/* division y/x with devide by zero protection
 */
BOOL ProtectedDiv(double y, double x, double *lpRes)
{                      
    if (fabs(x) <= 1.0)        /* no overflow if calc fabs(x)*MAXDOUBLE ? */
    {
        if (fabs(y) >= fabs(x)*MAXDOUBLE)   /* double overflow expected ? */
        {                                        /* path handles also 0/0 */
            *lpRes = FDMATHERRMAXRESULT;
            if ((x<0.0) && (y>0.0) || (x > 0.0) && (y < 0.0)) *lpRes = - *lpRes;
            return FALSE;
        } /* if */
    } /* if */

    *lpRes = y/x;
    return TRUE;
} /* ProtectedDiv() */



/* matrix and vector functions */

static double skalar(int n, double v1[], double v2[])
{
    int i;
    double Result = 0.0;

    for (i = 0; i < n; i++) Result += v1[i]*v2[i];

    return Result;
} /* skalar */




/* complex algebraic calculations */

/* complex multiply */
struct complex CplxMult(struct complex z1, struct complex z2)
{
    struct complex w;
    w.x = z1.x * z2.x - z1.y * z2.y;
    w.y = z1.x * z2.y + z2.x * z1.y;
    return w;
}


/* complex div : result = z1/z2 = (a1+jb1)/(a2+jb2)
 *                      = (a1*a2 + b1*b2 +ja2*b1 -ja1*b2)/(a2*a2 + b2*b2)
 */
struct complex CplxDiv(struct complex z1, struct complex z2)
{
    struct complex w;
    double s2 = z2.x * z2.x + z2.y * z2.y;
    (void)ProtectedDiv(z1.x * z2.x + z1.y * z2.y, s2, &w.x);
    (void)ProtectedDiv(z2.x * z1.y - z1.x * z2.y, s2, &w.y);
    return w;
}


/* complex root : result = sqrt(z)
 * sqrt(z) = + sqrt(|z|) * exp(j arg(z)/2)
 *         = sqrt(sqrt(z.x^2 + z.y^2)) * exp(j * tan(z.y/z.x)/2)
           = 1/sqrt(2) * {sqrt(|z|+Re(z))+j*SIGN(Im(z))*sqrt(|z|-Re(z))}
 */
struct complex CplxRoot(struct complex z)
{
    struct complex result;
    double z_unit = hypot(z.x, z.y);
    result.x = M_SQRT_2 * sqrt(z_unit + z.x);
    result.y = M_SQRT_2 * SIGN(z.y) * sqrt(z_unit - z.x);
    return result;
}


/* mallocs memory space for polynomial coeffs and roots
 * returns FALSE, if memory error
 */
BOOL MallocPolySpace(PolyDat *p)
{
    p->root = MALLOC(max (1, p->order*sizeof(struct complex)));
    p->coeff = MALLOC((1+p->order*sizeof(double)));

    if ((p->root == NULL) || (p->coeff == NULL))
    {
        FreePolySpace(p);
        return FALSE;
    } /* if */

    return TRUE;
} /* MallocPolySpace */



/* frees memory space of polynomial 
 */
void FreePolySpace(PolyDat *p)
{
    if (p->root != NULL) FREE(p->root);
    if (p->coeff != NULL) FREE(p->coeff);
} /* FreePolySpace */





/* - calc the poly coeff from complex roots
   - calculation of complex polynomial P(i+1) in step i+1 :
     P(i+1) = P(i) * (z-z[i]) = P(i) * z - P(i)*z[i]  with P(0) = 1 */

BOOL RootsToCoeffs(PolyDat *poly)
{
    int i, k;
    struct complex CplxRes;     /* result */
    struct complex *CplxPoly =
        (struct complex *)MALLOC((1+poly->order)*sizeof(struct complex));
    if (CplxPoly == NULL) return FALSE;

    poly->coeff[0] = CplxPoly[0].x = 1.0;                /* if order == 0 */
    CplxPoly[0].y = 0.0;

    for (i = 0; i < poly->order; i++)     /* poly->order roots processing */
    {
        for (k = i+1; k > 0; k--)                /* with all coefficients */
        {
            CplxPoly[k].x = CplxPoly[k-1].x;                  /* P(i) * z */
            CplxPoly[k].y = CplxPoly[k-1].y;
        } /* for */

        CplxPoly[0].x = CplxPoly[0].y = 0.0;

        for (k = 0; k <= i; k++)                /* with all coefficients */
        {
            CplxRes = CplxMult(CplxPoly[k+1], poly->root[i]);
            CplxPoly[k].x -= CplxRes.x;                   /* -P(i)*z[i] */
            CplxPoly[k].y -= CplxRes.y;
        } /* for */
    } /* for */

    for (i = 0; i <= poly->order; i++)
        poly->coeff[i] = CplxPoly[i].x;

    FREE(CplxPoly);
    return TRUE;
}


/* returns the max abs of all polynomial roots (range estimation of roots),
   means 1 + Amax/a[n] with Amax=max{a[0],a[1],...,a[n-1]}
 */
static double GetMaxAbsOfPolyRoots(int ord, double coeffs[])
{
    double Amax = 0.0;
    double *c;
    register int i;
    for (i=0, c=coeffs; i<ord; i++, c++)
        Amax = max(fabs(*c), Amax);

    return (1.0 + (double)FLT_EPSILON*ord + Amax/fabs(coeffs[ord]));
}

/* calculation of nearest real root in [x1,x2] with start value *x
 * if no convergence a new random start value in a possible x-range
 * will be set (but this value must lie in [x1,x2]) and polling
   of break-proc will be done
 * if Break detected returns with FALSE, else returns TRUE
 * destroys coeffs[] array !
 * using Newton Method : x(i+1) = x(i) - P(x(i)) / P'(x(i))
 * convergence criteria : m = P(x(i))*P''(x(i))/[P'(x(i))]^2
                         |m| < 1.0
 * error estimation : |x(i+1)-root| < m/(1-m)*|x(i+1)-x(i)|
 */

BOOL SearchRealRootOfPoly(int degree, double coeffs[], double dEps, double *x,
                          double x1, double x2, FUNCROOTSCALLBACK BreakProc)
{
    double dFirstDev, dSecDev; /* first and second deviation of poly at x */
    double m, XMax, XMin, delta_x, XOld, dNormCoeff = 0.0;
    long trys = 0L;                                    /* number of try's */
    int i;

    if (degree < 1) return TRUE;

    for (i = 0; i <= degree; i++)        /* search absolutely max. coeffs */
        if (fabs(coeffs[i]) > dNormCoeff)
            dNormCoeff = fabs(coeffs[i]);

    for (i = 0; i <= degree; i++)
        coeffs[i] /= dNormCoeff;       /* divide all coeffs by max. coeff */

    XMax = GetMaxAbsOfPolyRoots(degree, coeffs);
    if (XMax > x2) XMax = x2;                    /* max start value is x2 */
    XMin = -XMax;
    if (XMin < x1) XMin = x1;

    while (TRUE)                                           /* do for ever */
    {
        do
        {
            dFirstDev = DiffPoly(*x, 1, degree, coeffs);
            dSecDev = DiffPoly(*x, 2, degree, coeffs);
            (void)ProtectedDiv(poly(*x, degree, coeffs), dFirstDev, &delta_x);
            (void)ProtectedDiv(dSecDev*delta_x, dFirstDev, &m);
            m = fabs(m);
            if (m >= 1.0)
            {                /* no convergence or root not in range [x1, x2],
                              * try another start val */
                m = 2.0;
                *x = (XMax-XMin)/RAND_MAX*rand() + XMin;
                if (BreakProc != NULL)
                    if (BreakProc(++trys)) return FALSE;
            } /* if */
        } /* while */
        while (m >= 1.0);

        XOld = *x;
        *x -= delta_x;

        if (m*fabs(*x-XOld) <= dEps*(1-m))        /* error estimation ok ? */
        {
            if ((*x >= XMin) && (*x <= XMax)) return TRUE; /* inside range */

            *x = (XMax-XMin)/RAND_MAX*rand() + XMin;
            if (BreakProc != NULL)
                if (BreakProc(++trys)) return FALSE;
        } /* if */
    } /* while */
}


/* Implementation of rekursiv algorithm to compute all complex roots of an
 * algebraic polynomial.
 * Using the Bairstow-Hitchcock-Algorithm don't need usage of complex
 * mathematics.
 * GetPolynomialRoots returns CALC_OK if ok
 */


/* compute the new function values to use in Newton-algorithm
 * returns FALSE if overflow or anything floating point error occurs */

static void GetCorrOfFunctions(int n, EXTENDED r, EXTENDED q, EXTENDED *a,
                               EXTENDED *b, EXTENDED *A, EXTENDED *B)
{
    switch(n)
    {
        case 0 : *A=0.0; *B=a[0]; break;     /* case 0 is never possible ! */
        case 1 : *A=a[1]; *B=a[0]; break;
        case 2 : b[0]=a[2]; *A=a[1]+a[2]*r; *B=a[0]+a[2]*q; break;

        default:
        {
            register int i;
            b[n-2] = a[n];
            b[n-3] = a[n-1]+b[n-2]*r;
            for (i=n-4; i>=0; i--)                         /* only if n>3 */
                b[i] = a[i+2]+b[i+1]*r+b[i+2]*q;

            *A = a[1]+b[0]*r+b[1]*q;
            *B = a[0]+b[0]*q;
        } /* default */
    } /*switch */
} /* GetCorrOfFunctions */


/* compute roots of polynomial of second order */
static void GetRootsPoly2(EXTENDED coeff0, EXTENDED coeff1,
                          struct complex roots[])
{
    EXTENDED dDiscrimi = coeff1*coeff1/4.0 - coeff0;
    roots[0].x = roots[1].x = -coeff1/2.0;
    roots[0].y = roots[1].y = 0.0;
    if (dDiscrimi >= 0.0)
    {
        dDiscrimi = SQRT(dDiscrimi);
        roots[0].x += dDiscrimi;
        roots[1].x -= dDiscrimi;
    }
    else
    {
        dDiscrimi = SQRT(FABS(dDiscrimi));
        roots[0].y = dDiscrimi;
        roots[1].y = -dDiscrimi;
    } /* else */
} /* GetRootsPoly2() */


/* division y/x with devide by zero protection
 * no protection against overflow, underflow etc.
 */
static BOOL ExtDiv(EXTENDED y, EXTENDED x, EXTENDED *lpRes)
{                      /* because signal() may be non effect if FPU error */
    if (FABS(x) <= 1.0)        /* no overflow if calc fabs(x)*LDBL_MAX ? */
    {
        if (FABS(y) >= FABS(x)*LDBL_MAX)   /* long double overflow expected ? */
            return FALSE;
    } /* if */

    *lpRes = y/x;
    return TRUE;
} /* ExtDiv */




int GetPolynomialRoots(PolyDat *poly, double dEps, FUNCROOTSCALLBACK BreakProc)
{
    EXTENDED *a;                           /* pointer to current polynomial */
    EXTENDED *c;            /* pointer to deviation of decimation polynomial */
    EXTENDED *b;                   /* pointer to decimation polynomial */
    EXTENDED r, q;              /* coefficients of second order polynomial */
    EXTENDED dMaxr, dMaxq;                           /* bounds for r and q */
    EXTENDED A, B;                          /* the Newton-Target functions */
    EXTENDED dAdq, dAdr, dBdq, dBdr;      /* partial deviations of A and B */
    EXTENDED detJacobi;                      /* must be greather than null */
    EXTENDED dEpsr, dEpsq;    /* recursiv improvement for Newton algorithm */
    EXTENDED SqrEps;          /* error limit */
    EXTENDED dOldEpsr, dOldEpsq;        /* old values to check convergency */
    EXTENDED *pHelp;
    EXTENDED Amax;                 /* bound of the zeros of the polynomial */
    struct complex *lpRoot;
    EXTENDED dNormCoeff = 0.0;          /* max of all polynomial coeffs */
    int i, nCurrOrder = poly->order;

    if (nCurrOrder < 1) return CALC_OK;       /* defensive programming */

    a = (EXTENDED *)MALLOC((1+nCurrOrder)*sizeof(EXTENDED));
    if (a == NULL) return CALC_NO_MEM;
    b = (EXTENDED *)MALLOC((1+nCurrOrder)*sizeof(EXTENDED));
    if (b == NULL) return CALC_NO_MEM;
    c = (EXTENDED *)MALLOC((1+nCurrOrder)*sizeof(EXTENDED));
    if (c == NULL) return CALC_NO_MEM;

    for (i = 0; i <= nCurrOrder; i++)
        if (FABS((EXTENDED)poly->coeff[i]) > dNormCoeff)
            dNormCoeff = FABS((EXTENDED)poly->coeff[i]);

    for (i = 0; i <= nCurrOrder; i++)         /* do not use original poly */
        a[i] = (EXTENDED)poly->coeff[i]/dNormCoeff;

    Amax = GetMaxAbsOfPolyRoots(nCurrOrder, poly->coeff);

    SqrEps = dEps * dEps;

    lpRoot = &poly->root[nCurrOrder-2];

    while (nCurrOrder > 2)
    {
        r = q = (EXTENDED)1.0;                          /* initial values */
        dOldEpsr = dOldEpsq = MAXFLOAT;

        while(1)
        {
            FNSIGNALHANDLER fnOldErrHandler;

            if (BreakProc != NULL)
                if (BreakProc((long)nCurrOrder))
                {
                    FREE(a);
                    FREE(b);
                    FREE(c);
                    return CALC_BREAK;
                } /* if */

            fnOldErrHandler = OpenLocalMathErr();

            GetCorrOfFunctions(nCurrOrder, r, q, a, b, &A, &B);
            if (!WasLocalMathErr())
            {
                GetCorrOfFunctions(nCurrOrder-2, r, q, b, c, &dAdq, &dBdq);
                if (!WasLocalMathErr())
                {
                    dAdr = r * dAdq + dBdq;
                    dBdr = q * dAdq;
                    detJacobi = dAdr * dBdq - dAdq * dBdr;

                    dEpsr = (dBdq * A - dAdq * B) / detJacobi;
                    dEpsq = (dAdr * B - dBdr * A) / detJacobi;

                    if (!WasLocalMathErr())
                    {

                        EXTENDED dR1, dR2, dI1, dI2;

                        r -= dEpsr;
                        q -= dEpsq;
                        GetRootsPoly2(-q, -r, lpRoot);  /* new roots estimation */
                        dR1 = A*lpRoot->x+B;
                        dI1 = A*lpRoot->y;
                        dR2 = A*(lpRoot+1)->x+B;
                        dI2 = A*(lpRoot+1)->y;
// for equal results !!!  if ((FABS(dEpsr) < dEps) && (FABS(dEpsq) < dEps)) break;

                        if ((dR1*dR1 + dI1*dI1 < SqrEps) &&  /* abs of error */
                            (dR2*dR2 + dI2*dI2 < SqrEps)) break;

                        if (WasLocalMathErr()) break;

                        if ((FABS(dEpsr) < FABS(dOldEpsr)) ||
                            (FABS(dEpsq) < FABS(dOldEpsq)))
                        {
                            dOldEpsr = dEpsr;
                            dOldEpsq = dEpsq;
                            continue;
                        } /* if */
                    } /* if */
                } /* if */
            } /* if */

            CloseLocalMathErr(fnOldErrHandler);

            dMaxr = 2.0*Amax;
            dMaxq = Amax*Amax;
            r = 2.0*dMaxr*(EXTENDED)rand()/(EXTENDED)RAND_MAX - dMaxr;
            q = 2.0*dMaxq*(EXTENDED)rand()/(EXTENDED)RAND_MAX - dMaxq;
            dOldEpsr = dOldEpsq = MAXFLOAT;
        }; /* while */

        pHelp = b;                 /* working now with reduced polynomial */
        b = a;
        a = pHelp;

        nCurrOrder -= 2;
        lpRoot -= 2;
    } /* while */

    switch (nCurrOrder)
    {
        case 2 :
        {
            EXTENDED dC0, dC1;
            if (ExtDiv(a[0], a[2], &dC0) && ExtDiv(a[1], a[2], &dC1))
            {
                GetRootsPoly2(dC0, dC1, &poly->root[0]);
                break;
            } /* if */
            else
            {
                poly->root[1].x = MAXDOUBLE;
                poly->root[1].y = 0.0;
            }
            /* and fall through (not possible at general) */
        } /* case order == 2 */

        case 1 :
            poly->root[0].y = 0.0;
            (void)ProtectedDiv(-a[0], a[1], &poly->root[0].x);
            break;
    } /* switch */

    FREE(a);
    FREE(b);
    FREE(c);
    return CALC_OK;
}

