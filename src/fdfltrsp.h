#ifndef __FDFLTRSP_H

/* discret time response utility structure */
typedef struct tagTimeRespStruc
{
    double dCurrTime;
    double *pLastOut;
    double *pCurrIn;
    double *pInputBuffer;   /* input buffer (per malloc) */
    double *pOutputBuffer;  /* output buffer (per malloc) */
    tFilterInput TimeSigType; /* step, dirac pulse or user defined */
} TimeRespStruc;




/* filter response functions (diagram values getting functions) */
double Magnitude(double f, PolyDat *p);
BOOL FrequencyResponse(double *, double *, void *);
BOOL Attenuation(double *, double *, void *);
BOOL PhaseResponse(double *, double *, void *);
BOOL ApproxCharacResponse(double *frequ, double *D, void *pAppData);
BOOL GroupDelay(double *, double *, void *);
BOOL PhaseDelay(double *, double *, void *);
int PhaseRespInit(double dStart, double dEnd, void **pDummy);
void PhaseRespEnd(void *pPhase0);
int PhaseDelayInit(double dStart, double dEnd, void **pDummy);
void PhaseDelayEnd(void *pPhase0);
int ImpulseRespInit(double, double, void **pTimeRespStrucPtr);
int StepRespInit(double, double, void **pTimeRespStrucPtr);
BOOL TimeResponse(double *, double *, void *pTimeRespStruc);
void TimeRespEnd(void *pTimeRespStruc);

#define __FDFLTRSP_H
#endif
