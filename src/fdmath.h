#ifndef __FDMATH_H

/* using fdmath.c you must define the following macros before compiling
   fdmath.c : MALLOC(), FREE(), REALLOC()
 * in most cases these are the same lower letter functions malloc(), free(),
   realloc()
 */

#include "FDUTIL.H"  /* memory management (MALLOC, FREE) */
#include <MATH.H>




#ifndef BOOL
#define BOOL                   int
#endif

#ifndef TRUE
#define TRUE                   1
#endif

#ifndef FALSE
#define FALSE                  0
#endif

#define ODD(number)            ((((number)>>1)<<1) != number)
#define SIGN(x)                (((x) < 0) ? -1 : 1)


/* polynomial struct */
typedef struct
{
    int order;                                    /* true order of polynom */
    double *coeff;        /* pointer to polynomial coefficients (z-Domain) */
    struct complex *root;                /* pointer to roots of polynomial */
} PolyDat;                              /* size is (MAXORDER+1)*8*3+2 Byte */


/* two dimensional math. function y=f(x) */
typedef double (* MATHFUNCDIM2)(double);



/*  mathematic support functions */
/* generic math functions */
double si(double);          /* si(x) = sin(x)/x */
double si2(double);         /* si^2(x) */
double arsinh(double);      /* area sinus hyperbolicus */
double arcosh(double);      /* area cosinus hyperbolicus */
double Chebyshev(double, double);                       /* cos(n*arcos(x)) */
#define ChebyshevInv(dOrder, x) Chebyshev(1.0/(dOrder), x)
BOOL ProtectedDiv(double, double, double *);

/* complex arithmetic */
struct complex CplxMult(struct complex, struct complex);
struct complex CplxDiv(struct complex, struct complex);
struct complex CplxRoot(struct complex);


/* windowing functions for digital signal processing :  0<=x<=1 */
double rectangle(double);   /* = 1 if 0<=x<=1 */
double hamming(double);
double hanning(double);
double blackman(double);
double kaiser(double alpha, double x, double MaxErr);
double triangle(double);
double bessel(int n, double x, double MaxErr);  /* Bessel function of n'th order, first kind */
double SineIntegral(double x, double MaxErr);
double EllIntegr_F(double Modul, double phi, double MaxErr);
double EllIntegr_K(double x, double MaxErr);
double JacobiSN(double k, double x, double MaxErr);
double JacobiCN(double k, double x, double MaxErr);
double JacobiDN(double k, double x, double MaxErr);

/* operations with polynomials */
BOOL GetBesselPoly(int, double *);
void MovePolyCoeffs(PolyDat *, PolyDat *);
BOOL DeletePolyCoeff(int, PolyDat *);


typedef BOOL (*FUNCROOTSCALLBACK)(long);

BOOL SearchRealRootOfPoly(int, double *, double, double *, double, double,
                          FUNCROOTSCALLBACK);

/* return values for GetPolynomialRoots */
#define CALC_OK        0   /* all roots found */
#define CALC_BREAK    -1   /* break via callback function */
#define CALC_NO_MEM   -2   /* not enough memory */
int GetPolynomialRoots(PolyDat *p, double, FUNCROOTSCALLBACK);

BOOL MallocPolySpace(PolyDat *p);
void FreePolySpace(PolyDat *p);

BOOL RootsToCoeffs(PolyDat *p);

/* integration */
double Integrate(double start, double stop, double MaxErr, MATHFUNCDIM2 func);

#define __FDMATH_H

#endif
