#include "DFCWIN.H"
#include <signal.h>
#include <float.h>
#include <stdlib.h>

/*****************************************************************
 * filter designer error handler                                 *
 *****************************************************************/

/* 80x87 control word settings via _control87()
 * EM_INVALID    = unsupported format or 0*INF, 0/0, (+INF) + (-INF),
                   stack overflow or underflow (don't mask !)
 * EM_DENORMAL   = operand has smallest exponent but a nonzero significand
                   (normal processing continues)
 * EM_ZERODIVIDE = divisor is zero, dividend is nonzero finite (FPU sets
                   result to +/-INF)
 * EM_OVERFLOW   = result too large (FPU sets result to INF or MAXDOUBLE
                   or LDBL_MAX) 
 * EM_UNDERFLOW  = nonzero result but too small (if masked denormalization
                   causes loss of accuracy else result is set to zero)
 * EM_INEXACT    = true result not exactly representable (e.g. 1/3 -> normal
                   processing continues)

 * don't mask INVALID exception of FPU, because this are dramatic errors
 * the following should everytime turned off : EM_DENORMAL, EM_UNDERFLOW,
   EM_INEXACT because FPU handling is the best
 */
#define CTL87_EXCP_OFF  (EM_DENORMAL|EM_ZERODIVIDE|EM_OVERFLOW|EM_UNDERFLOW| \
                         EM_INEXACT)
#define CTL87_EXCP_ON   (EM_DENORMAL|EM_UNDERFLOW|EM_INEXACT)



static void ValidateAll(void);
static void ClearFPU(void);
static void ErrHandler(int sig, int type);



/* validate all MDI childs when message box closed, because recursive
   paint call possible
 */
static void ValidateAll()
{
    HWND hwndMDIChild = GetFirstChild(hwndDiagBox);
    while (hwndMDIChild)
    {
        char szClassName[40];
        GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
        if (!lstrcmp(szClassName, DIAG_CLASS)) ValidateRect(hwndMDIChild, NULL);

        hwndMDIChild = GetNextSibling(hwndMDIChild);              /* next */
    } /* while */
} /* ValidateAll */



static void ClearFPU()
{
    if (GetWinFlags() & WF_80x87)              /* coprocessor available ? */
    {
        _clear87();                                        /* see float.h */
        _control87((uDeskOpt & DOPT_IGNFPE) ? CTL87_EXCP_OFF:CTL87_EXCP_ON,
                   MCW_EM);
    } /* if */
} /* ClearFPU() */


/* set my own signal handler */
void InitErrorHandler()
{
    signal(SIGFPE, ErrHandler);   /* Floating point trap  */
    signal(SIGINT, ErrHandler);   /* break by ^C */
    signal(SIGTERM, ErrHandler);  /* program end */
    signal(SIGABRT, ErrHandler);  /* break with error (not used by MS-DOS/Borland C) */
    signal(SIGSEGV, ErrHandler);  /* Memory access violation */

    ClearFPU();
} /* InitErrorHandler */



/* signal error handling function */

static void ErrHandler(int sig, int type)
{
    int nIddErr;
    char szMsg[256];                /* Message from resources file */
    char szStandard[256];
    HINSTANCE hInst = GetWindowInstance(hwndFDesk);

    MessageBeep(0);  
    signal(sig, ErrHandler);

    switch (sig)
    {
        case SIGFPE :    /* coprocessor FPEs or raised signals */
            switch (type)
            {
                /* at first two integer errors */
                case FPE_INTOVFLOW  : nIddErr = ERROR_INTOVFLOW; break; /* INT4 */
                case FPE_INTDIV0    : nIddErr = ERROR_INTDIV0; break;   /* INT0 */

                /* next are true floating point errors */
                case FPE_INVALID    : nIddErr = ERROR_INVALID87; break;
                case FPE_ZERODIVIDE : nIddErr = ERROR_FLOATDIV0; break;
                case FPE_OVERFLOW   : nIddErr = ERROR_FLOATOVFLOW; break;
                case FPE_UNDERFLOW  : nIddErr = ERROR_FLOATUNFLOW; break;
                case FPE_INEXACT    : nIddErr = ERROR_INEXACT; break;
                case FPE_STACKFAULT : nIddErr = ERROR_INVALID87; break;
                case FPE_EXPLICITGEN : nIddErr = ERROR_FLOAT_OP; break;

                /* CPU FPE's : Borland 8087 error extensions (type is a
                   pointer to CPU register (BP, DI, SI, DS, ES, DX, CX,
                   BX, AX, IP, CS, FLAGS) for modify) */
                default :
                    nIddErr = ERROR_FLOAT_OP;

                    if (GetWinFlags() & WF_80x87)      /* FPU available ? */
                    {
                        unsigned int stat;

                        stat = _clear87(); /* clear (FCLEX) and get FPU exception */

                        if (stat & SW_INVALID)
                        {
                            nIddErr = ERROR_INVALID87;
                            break;   /* cannot handle such critical error */
                        } /* if */

                        if (stat & SW_DENORMAL) nIddErr = ERROR_INEXACT;
                        if (stat & SW_INEXACT) nIddErr = ERROR_INEXACT;
                        if (stat & SW_ZERODIVIDE) nIddErr = ERROR_FLOATDIV0;
                        if (stat & SW_OVERFLOW) nIddErr = ERROR_FLOATOVFLOW;
                        if (stat & SW_UNDERFLOW) nIddErr = ERROR_FLOATUNFLOW;

                        LoadString(hInst, nIddErr, szMsg, sizeof(szMsg));
                        stat = lstrlen(szMsg);

                        #if GENFPEOPT
                        LoadString(hInst, ERROR_FPESUCCIGN, &szMsg[stat], DIM(szMsg)-stat);
                        #endif

                        ValidateAll();      /* before message box display */

                        switch (FDWINMESSAGEBOX(HWND_NULL, szMsg, NULL,
                                                #if GENFPEOPT
                                                MB_YESNOCANCEL
                                                #else
                                                MB_OK
                                                #endif
                                                | MB_ICONHAND | MB_TASKMODAL))
                        {
                            case IDYES :
                                UPDATEFLAG(uDeskOpt, DOPT_IGNFPE, FALSE);
                                break; /* IDYES */

                            case IDNO :        /* ignore error successors */
                            case IDOK:
                                UPDATEFLAG(uDeskOpt, DOPT_IGNFPE, TRUE);
                                break; /* IDNO */

                            case IDCANCEL :
                                exit(EXIT_FAILURE);    /* the last chance */
                        } /* switch */

                        ClearFPU();
                        ValidateAll();       /* after message box display */
                        return;
                    } /* if */

                    break; /* default */
            } /* switch */

            break; /* SIGFPE */


        case SIGINT  : nIddErr = ERROR_SIGINT; break;
        case SIGTERM : nIddErr = ERROR_SIGTERM; break;
        case SIGABRT : nIddErr = ERROR_SIGABRT; break;
        case SIGSEGV: nIddErr = ERROR_SIGSEGV; break;
    } /* switch */

    LoadString(hInst, nIddErr, szMsg, sizeof(szMsg));
    LoadString(hInst, ERROR_USR_ACK, szStandard, sizeof(szStandard));

    ValidateAll();                          /* before message box display */

    if (IDOK != FDWINMESSAGEBOX(HWND_NULL, lstrcat(szMsg, szStandard), NULL,
                                MB_OKCANCEL | MB_ICONHAND | MB_TASKMODAL))
        exit(EXIT_FAILURE);                              /* the last hook */

    ValidateAll();
} /* ErrHandler */


/* error handling for math library functions (not on math expression errors),
   e.g. division by zero ..., because handling of such errors is possible
   only via "signal()")
 * type _mexcep enumerate all possible errors
 */

int matherr (struct exception *e)
{
    ClearFPU();

    switch(e->type)
    {
        case UNDERFLOW :
            e->retval = 0.0;                    /* set underflow to 0 */
            return TRUE;  /* correction, i.e. no set errno or message */

        case OVERFLOW :                    /* function result too big */
            e->retval = FDMATHERRMAXRESULT;  /* neg. ovfl set to pos. */
            return TRUE;

        case DOMAIN :              /* no result defined for this func */
            e->retval = 0.0;                        /* standard value */
            if (!strcmpi(e->name,"sqrt")) return TRUE;  /* x < 0 => 0 */
            if (!strcmpi(e->name,"hypot")) return TRUE;
            if (!strcmpi(e->name,"atan2")) return TRUE; /* x=y=0 => 0 */
            if (!strcmpi(e->name,"acos")) return TRUE;  /* x > 1 => 0 */
            if (!strcmpi(e->name,"atan")) return TRUE;
            if (!strcmpi(e->name,"pow")) return TRUE; /* (x = 0) and (y <= 0)
                                                         or (x < 0) and (y not an integer) */

            e->retval = -FDMATHERRMAXRESULT; /* translate x < 0 to x = 0, means y = -INF */
            if (!strcmpi(e->name,"log10")) return TRUE;
            if (!strcmpi(e->name,"log")) return TRUE;
            if (!strcmpi(e->name,"asin"))
            {
                e->retval = M_PI; /* if x > 1 */
                return TRUE;
            } /* if */

            break; /* DOMAIN */

        case SING :                            /* argument singularity */
        #if DEBUG
        {
            char szNum[80];
            sprintf(szNum, "%s : %g", e->name, e->arg1);
            ValidateAll();
            MessageBox(HWND_NULL, szNum, "SINGULARITY ERROR", MB_TASKMODAL);
            ValidateAll();
        } /* SING */
        #endif
            e->retval = -FDMATHERRMAXRESULT;
            if (!strcmpi(e->name,"log10")) return TRUE;   /* log10(0) */
            if (!strcmpi(e->name,"log")) return TRUE;       /* log(0) */
            e->retval = FDMATHERRMAXRESULT;
            if (!strcmpi(e->name,"exp")) return TRUE;     /* exp(300) */
            break; /* SING */

        case TLOSS :
            /* only for sin, cos, tan if x too big (no convergence)
             * total loss of precision, but ignore the problem
             */
            e->retval = 1.0;        /* set sin(INF) = 1, cos(INF) = 1 */
            return TRUE;
    } /* switch */

    /* all other errors are fatal */
    return FALSE;              /* no correction implemented */
} /* matherr () */




/* application error display */
void ErrorAckUsr(HWND hwndParent, int nIdstrMsg)
{
    if (nIdstrMsg == IDSTROUTOFMEMORY)  /* critical error: loss of memory */
    {
        MessageBeep(0);
        if (IDCANCEL == FDWINMESSAGEBOX(hwndParent, ERROR_NOMEMORY, NULL,
                                        MB_OKCANCEL | MB_ICONHAND | MB_SYSTEMMODAL))
            exit(EXIT_FAILURE);
    } /* if */
    else
    {
        char szTitel[256], szMsg[256];
        HINSTANCE hInstThisAppl = GetWindowInstance(hwndParent);
        LoadString(hInstThisAppl, ERROR_TITLE, szTitel, sizeof(szTitel));
        LoadString(hInstThisAppl, nIdstrMsg, szMsg, sizeof(szMsg));
        FDWINMESSAGEBOX(hwndParent, szMsg, szTitel, MB_OK|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);
    } /* else */
} /* ErrorAckUsr() */

