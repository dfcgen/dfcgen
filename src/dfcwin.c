/* DFCGEN Main Module
 * supports design of various digital filters, e.g. Lin. FIR,
   standard IIR (Bessel, Butterworth, Chebyshev I and II)     
   and various special filter functions                       

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/dfcwin.c,v $
 * $Revision: 1.2 $
 * $Date: 2000-08-17 12:45:11 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $

 */


#include "dfcwin.h"
#include "fdprint.h"
#include "fdfltrsp.h"
#include <dir.h>                     /* definition of MAXPATH, MAXEXT, ... */


/* generating defs */
#define FORMAT_ROOT_POS  "[%d] %.*lG + j%.*lG" /* root list format (pos. imag) */
#define FORMAT_ROOT_NEG  "[%d] %.*lG - j%.*lG" /* root list format (neg. imag) */
#define FORMAT_ROOT_NULL "[%d] %.*lG"          /* root list format (no imag) */




/******** functions and variables to handle the coefficients window *******/
/* local variables ::::::                                                 */
static WORD nIddSelectedList;  /* selected Listbox for coefficient change */
static int iList;              /* coefficients index in selected list     */
static double dRetVal;         /* temp coefficients for communication     */
static HWND hwndStatusBox;    /* status line with multiple status windows */



/* prototypes of local functions */

static BOOL GetIniFilename(char *szBuf, int nBufSize);
static void DisplayFilterCoeffs(void);
static void ResetCoeffSelection(BOOL);
static void DisplayFilterRoots(void);
static BOOL RootsCalcStatus(long);
static int CalcRoots(PolyDat *poly);
static BOOL DrawListOfRoots(HWND, int, PolyDat *);
static void EnableCoeffActions(HWND, BOOL);
static BOOL GetPolyRoot(double *, double *, void *);
static int GetPolyRootInit(double from, double to, void **pDummy);
static BOOL GetUnitCircleTop(double *, double *, void *);
static BOOL GetUnitCircleBottom(double *, double *, void *);
static BOOL CopyToTmpFilter(tFilter *lpTmpFlt, tFilter *lpSrcFlt);
static void ShowRootsWindow(BOOL bShow);
static void ShowCoeffWindow(BOOL bShow);
static void CopyVarAxisParameters(tDiagAxis *pDestAxis, tDiagAxis *pSrcAxis);
static BOOL CopyIfEquDiag(HWND h, DIAGCREATESTRUCT *dg, tFdDiags diag_type);
static void CopyIfEquAxis(HWND h, DIAGCREATESTRUCT *dg, tFdDiags diag_type);
static int LoadFilter(BOOL bWithPrj, char *szPath);
static BOOL WriteProfile(void);




static void ShowRootsWindow(BOOL bShow)
{
    CheckMenuItem(GetMenu(hwndFDesk), IDM_SHOWROOTS, bShow ? MF_CHECKED:MF_UNCHECKED);
    if (bShow) uDeskOpt |= DOPT_SHOWROOTS;
    else uDeskOpt &= ~DOPT_SHOWROOTS;

    if (hwndFilterRoots != HWND_NULL)
        ShowWindow(hwndFilterRoots, (uDeskOpt & DOPT_SHOWROOTS) ? SW_SHOWNORMAL:SW_HIDE);
} /* ShowRootsWindow() */


static void ShowCoeffWindow(BOOL bShow)
{
    CheckMenuItem(GetMenu(hwndFDesk), IDM_SHOWCOEFF,
                  bShow ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND|MF_UNCHECKED));

    if (bShow) uDeskOpt |= DOPT_SHOWCOEFF;
    else uDeskOpt &= ~DOPT_SHOWCOEFF;

    if (hwndFilterRoots != HWND_NULL)
        ShowWindow(hwndFilterCoeff, (uDeskOpt & DOPT_SHOWCOEFF) ? SW_SHOWNORMAL:SW_HIDE);
} /* ShowCoeffWindow() */


/* implementation of functions to handle the root of polynomial window */


#define IDXZERO         2
#define IDXPOLE         3
#define IDXCIRCLETOP    0
#define IDXCIRCLEBOTTOM 1

static tDiag DiagRoot[] =
{
    {
        HRGN_NULL, 0, NULL, NULL, 1, 0, NULL,
        NULL, GetUnitCircleTop, NULL,
        {0, 0.0, 1e3, 0.0, (COORDINATE)0, (COORDINATE)639, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_REAL},
        {0, 0.0, 2.0, 0.0, (COORDINATE)0, (COORDINATE)479, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_IMAG}
    },

    {
        HRGN_NULL, DIAG_NOSCALE, NULL, NULL, 1, 0, NULL,
        NULL, GetUnitCircleBottom, NULL,
        {0, 0.0, 1e3, 0.0, 0, 639, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_REAL},
        {0, 0.0, 2.0, 0.0, 0, 479, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_IMAG}
    },

    {
        HRGN_NULL, DIAG_DISCRET|DIAG_NOSCALE, NULL,
        MAKEINTRESOURCE(IDBMP_ZERO), DEFAULT_CXYBMP_SAMPLES, 0, NULL,
        GetPolyRootInit, GetPolyRoot, NULL,
        {0, 0.0, 1e3, 0.0, 0, 639, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_REAL},
        {0, 0.0, 2.0, 0.0, 0, 479, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_IMAG}
    },

    {
        HRGN_NULL, DIAG_DISCRET | DIAG_NOSCALE, NULL,
        MAKEINTRESOURCE(IDBMP_POLE), DEFAULT_CXYBMP_SAMPLES, 0, NULL,
        GetPolyRootInit, GetPolyRoot, NULL,
        {0, 0.0, 1e3, 0.0, (COORDINATE)0, (COORDINATE)639, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_REAL},
        {0, 0.0, 2.0, 0.0, (COORDINATE)0, (COORDINATE)479, (short) DEFAULT_PRECISION, NULL, (short)0, STRING_AXIS_Z_IMAG}
    }

};

static struct complex *lpRoot;
static int nAtOrigin;                     /* number of roots at point 0,0 */


#pragma argsused  /* disable "Parameter ist never used" Warning */
static BOOL GetPolyRoot(double *x, double *y, void *pAppData)
{
    *y = 0.0;
    if (--nAtOrigin >= 0) *x = 0.0;
    else
    {
        *x = lpRoot->x;
        *y = lpRoot->y;
        ++lpRoot;
    }
    return TRUE;
}

#pragma argsused  /* disable "Parameter ist never used" Warning */
static int GetPolyRootInit(double from, double to, void **pDummy)
{
    int nRoots;

    nAtOrigin = MainFilter.b.order - MainFilter.a.order;
    if (lpRoot == MainFilter.a.root) nRoots = MainFilter.a.order;
    else
    {
        nRoots = MainFilter.b.order;
        nAtOrigin = -nAtOrigin;
    }
    if (nAtOrigin < 0) nAtOrigin = 0;
    return nRoots+nAtOrigin;                          /* summary of roots */
}


#pragma argsused  /* disable "Parameter ist never used" Warning */
static BOOL GetUnitCircleTop(double *x, double *y, void *pDummy)
{
    if (fabs(*x) > 1.0+1.0/DiagRoot[IDXCIRCLETOP].X.dDevToWorldRatio)
        return FALSE;
    *y = 1.0 - *x * *x;
    if (*y < 0.0) *y = 0.0;
    else *y = sqrt(*y);
    return TRUE;
}


#pragma argsused  /* disable "Parameter ist never used" Warning */
static BOOL GetUnitCircleBottom(double *x, double *y, void *pDummy)
{
    if (fabs(*x) > 1.0+1.0/DiagRoot[IDXCIRCLEBOTTOM].X.dDevToWorldRatio)
        return FALSE;

    *y = 1.0 - *x * *x;
    if (*y < 0.0) *y = 0.0;
    else *y = -sqrt(*y);
    return TRUE;
}


BOOL CALLBACK RootsDlgProc(HWND hwndDlgWin,
                             UINT msg,         /* message */
                             WPARAM wParam,
                             LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG :
        {
            RECT rcList;

            EnableMenuItem(GetSystemMenu(hwndDlgWin, FALSE), SC_MINIMIZE,
                           MF_GRAYED|MF_BYCOMMAND);
            GetClientRect(GetDlgItem(hwndDlgWin, IDD_ROOT_LIST_ZERO), &rcList);
            SendDlgItemMessage(hwndDlgWin, IDD_ROOT_LIST_ZERO, LB_SETHORIZONTALEXTENT,
                               2*(rcList.right-rcList.left), DUMMY_LPARAM);
            GetClientRect(GetDlgItem(hwndDlgWin, IDD_ROOT_LIST_POLE), &rcList);
            SendDlgItemMessage(hwndDlgWin, IDD_ROOT_LIST_POLE, LB_SETHORIZONTALEXTENT,
                               2*(rcList.right-rcList.left), DUMMY_LPARAM);
            return TRUE;
        } /* WM_INITDIALOG */

        case WM_MEASUREITEM :
            if (((LPMEASUREITEMSTRUCT)lParam)->CtlID == IDD_ROOT_PN)
            {
                RECT rDiag;
                LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT)lParam;
                GetClientRect(GetDlgItem(hwndDlgWin, IDD_ROOT_PN), &rDiag);
                lpmi->itemWidth  = rDiag.right;
                lpmi->itemHeight = rDiag.bottom;
                return TRUE;   /* all buttons are icons ! */
            } /* if */
            break; /* WM_MEASUREITEM */


        case WM_DRAWITEM :
        {
            LPDRAWITEMSTRUCT lpdi = (LPDRAWITEMSTRUCT)lParam;

            if ((lpdi->CtlID == IDD_ROOT_PN) &&
                (lpdi->itemAction & ODA_DRAWENTIRE) &&
                (MainFilter.f_type != NOTDEF) &&
                (MainFilter.uFlags & FILTER_ROOTS_VALID))
            {
                int iDiag, i;
                struct complex cplxMin, cplxMax;
                RECT rDiag;
                COLORREF aColors[SIZE_COLOR_ARR];
                COLORREF ColBtnPNOld;

                EnableMainMenu(hwndFDesk, FALSE);
                WorkingMessage(STRING_WAIT_PAINT);

                /* set correct dialog colors (not user installable) */
                aColors[IDIAGCOL_SCALENUM] =              /* axis scaling */
                aColors[IDIAGCOL_AXISNAME] = GetSysColor(COLOR_WINDOWTEXT);

                aColors[IDIAGCOL_SCALELINE] =
                aColors[IDIAGCOL_FRAME] = GetSysColor(COLOR_WINDOWFRAME);

                aColors[IDIAGCOL_NOTEFRAME] = GetSysColor(COLOR_BTNFACE); /* comment frame */
                aColors[IDIAGCOL_NOTETEXT] = GetSysColor(COLOR_BTNSHADOW);    /* and text */

                DiagRoot[IDXZERO].cxySmplCurve =
                DiagRoot[IDXPOLE].cxySmplCurve = cxyBmpRoots;

                cplxMin.x = cplxMin.y = WORLD_MAX;
                cplxMax.x = cplxMax.y = WORLD_MIN;

                lpRoot = MainFilter.a.root;       /* numerator max search */

                for (i = 0; i < MainFilter.a.order; i++, lpRoot++)
                {                                 
                    cplxMin.x = min(cplxMin.x, lpRoot->x);
                    cplxMin.y = min(cplxMin.y, lpRoot->y);
                    cplxMax.x = max(cplxMax.x, lpRoot->x);
                    cplxMax.y = max(cplxMax.y, lpRoot->y);
                } /* for */


                lpRoot = MainFilter.b.root;     /* denominator max search */
                                                
                for (i = 0; i < MainFilter.b.order; i++, lpRoot++)
                {
                    cplxMin.x = min(cplxMin.x, lpRoot->x);
                    cplxMin.y = min(cplxMin.y, lpRoot->y);
                    cplxMax.x = max(cplxMax.x, lpRoot->x);
                    cplxMax.y = max(cplxMax.y, lpRoot->y);
                } /* for */


                if (MainFilter.b.order != MainFilter.a.order)
                {                               /* n-m roots at point 0,0 */
                    cplxMin.x = min(cplxMin.x, 0.0);
                    cplxMin.y = min(cplxMin.y, 0.0);
                    cplxMax.x = max(cplxMax.x, 0.0);
                    cplxMax.y = max(cplxMax.y, 0.0);
                } /* if */

                #define STRETCH 1.25
                
                if (fabs(cplxMin.x-cplxMax.x) < aMathLimits[IMATHLIMIT_NULLROOT])
                {
                    cplxMin.x -= aMathLimits[IMATHLIMIT_NULLROOT];
                    cplxMax.x += aMathLimits[IMATHLIMIT_NULLROOT];
                } /* if */

                if (fabs(cplxMin.y-cplxMax.y) < aMathLimits[IMATHLIMIT_NULLROOT])
                {
                    cplxMin.y -= aMathLimits[IMATHLIMIT_NULLROOT];
                    cplxMax.y += aMathLimits[IMATHLIMIT_NULLROOT];
                } /* if */

                if (cplxMin.x < 0.0) cplxMin.x *= STRETCH; else cplxMin.x /= STRETCH;
                if (cplxMax.x < 0.0) cplxMax.x /= STRETCH; else cplxMax.x *= STRETCH;
                if (cplxMin.y < 0.0) cplxMin.y *= STRETCH; else cplxMin.y /= STRETCH;
                if (cplxMax.y < 0.0) cplxMax.y /= STRETCH; else cplxMax.y *= STRETCH;


                GetClientRect(lpdi->hwndItem, &rDiag);

                for (iDiag = 0; iDiag < DIM(DiagRoot); iDiag++)
                {
                    ColBtnPNOld = SetBkColor(lpdi->hDC, GetSysColor(COLOR_WINDOW));
                    if ((iDiag == IDXZERO) || (iDiag == IDXPOLE))
                        aColors[IDIAGCOL_CURVE] = GetSysColor(COLOR_WINDOWTEXT);
                    else aColors[IDIAGCOL_CURVE] = GetSysColor(COLOR_BTNSHADOW);


                    DiagRoot[iDiag].X.dWorldMin = cplxMin.x;
                    DiagRoot[iDiag].X.dWorldMax = cplxMax.x;
                    DiagRoot[iDiag].Y.dWorldMin = cplxMin.y;
                    DiagRoot[iDiag].Y.dWorldMax = cplxMax.y;
                    DiagRoot[iDiag].X.nScreenMin = rDiag.left;
                    DiagRoot[iDiag].Y.nScreenMin = rDiag.top;
                    DiagRoot[iDiag].X.nScreenMax = rDiag.right;
                    DiagRoot[iDiag].Y.nScreenMax = rDiag.bottom;
                    lpRoot = (iDiag == IDXZERO) ? MainFilter.a.root : MainFilter.b.root;
                    if (!PaintDiag(GetWindowInstance(hwndFDesk), lpdi->hDC,
                                   UserBreak, &DiagRoot[iDiag],
                                   aColors))          /* don't call MessageAckUsr() */
                        StatusMessage(ERROR_DIAGRAM); /* because recurion possible */
                    SetBkColor(lpdi->hDC, ColBtnPNOld); /* restore bkgnd color */
                } /* for */

                WorkingMessage(0);
                EnableMainMenu(hwndFDesk, TRUE);
            } /* if */

            return TRUE;
        } /* WM_DRAWITEM */

        case WM_CLOSE :            /* don't destroy window, but only hide */
            ShowRootsWindow(FALSE);
            return TRUE;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_ROOT_CALCNEW:
                    MainFilter.uFlags &= ~FILTER_ROOTS_VALID;
                    DisplayFilterRoots();
                    return TRUE;

                case IDHELP:
                    WinHelp(hwndDlgWin, HELP_FNAME, HELP_CONTEXT, HelpRootsDlg);
                    return TRUE;
            } /* switch */

    } /* switch */

    return FALSE;
}



static BOOL RootsCalcStatus(long nRemainOrder)
{
    static int nOldOrder;

    if (nOldOrder != (int)nRemainOrder)
    {
        char szMsg[256], szFormat[256];

        LoadString(GetWindowInstance(hwndStatus), STRING_STATUS_ROOTS_CALC, szFormat, DIM(szFormat));
        sprintf(szMsg, szFormat, (int)nRemainOrder);
        SetWindowText(hwndStatus, szMsg);
        nOldOrder = (int)nRemainOrder;
    } /* if */
    return ChkAbort();
} /* RootsCalcStatus */


/* returns the values passed from GetPolynomialRoots
 * CALC_OK       all roots found
 * CALC_BREAK    break via callback function
 * CALC_NO_MEM   not enough memory

                       n         -i         n-1
 * algorithm :  P(z)= SUM a[i] * z   =  K * PRD (z-z0[i])
                      i=0                   i=0
  
                        n          i         n-1                 n-1
 * with w=1/z:  P(w) = SUM a[i] * w   =  K * PRD (w-w0[i]) = K * PRD (1-w0[i]*z)/z
                       i=0                   i=0                 i=0
  
 * what means : 1-w0[i]*z0[i] = 0  is the root in z
  
                  z0[i] = 1/w0[i]    (note : this is a complex op)
 */

static int CalcRoots(PolyDat *poly)
{
    int i, nRet;

                                                 /* calculation of w-roots */
    nRet = GetPolynomialRoots(poly, aMathLimits[IMATHLIMIT_ERRROOTS], RootsCalcStatus);
                                        
    if (nRet == CALC_OK)
    {
        static struct complex CplxOne = {1.0, 0.0};

        for (i=0; i < poly->order; i++)                  /* for every root */
            poly->root[i] = CplxDiv(CplxOne, poly->root[i]);  /* root in z */

        return CALC_OK;
    } /* if */

    return nRet;                                      /* return with error */
} /* CalcRoots */



/* draws the filter roots in the list box if valid
 * returns FALSE if no memory available
 */ 
static BOOL DrawListOfRoots(HWND hwnd, int nIDDList, PolyDat *poly)
{
    int i;
    char szComplexRoot[80];

    static tSendItemMsgRec PrepareFillDat[] = {
        {WM_SETREDRAW, FALSE, DUMMY_LPARAM},         /* do not redraw the edit win */
        {LB_RESETCONTENT, DUMMY_WPARAM, DUMMY_LPARAM}, /* clear the complete list */
        {WM_SETREDRAW, TRUE, DUMMY_LPARAM},            /* enable redrawing */
        {WM_ENABLE, FALSE, DUMMY_LPARAM}       /* disable input in listbox */
    };

    MultipleSendDlgItemMessage(hwnd, nIDDList, DIM(PrepareFillDat), PrepareFillDat);

    for (i=0; i < poly->order; i++)                   /* do for every root */
    {
        if (poly->root[i].y > aMathLimits[IMATHLIMIT_NULLROOT]) /* imag part positiv */
            sprintf(szComplexRoot, FORMAT_ROOT_POS, i, anPrec[IOUTPRECDLG_ROOTS],
                    poly->root[i].x, anPrec[IOUTPRECDLG_ROOTS], poly->root[i].y);
        else
        {
            if (poly->root[i].y < -aMathLimits[IMATHLIMIT_NULLROOT])
                sprintf(szComplexRoot, FORMAT_ROOT_NEG, i, anPrec[IOUTPRECDLG_ROOTS],
                        poly->root[i].x, anPrec[IOUTPRECDLG_ROOTS], fabs(poly->root[i].y));

            else sprintf(szComplexRoot, FORMAT_ROOT_NULL, i, anPrec[IOUTPRECDLG_ROOTS],
                         poly->root[i].x);                /* no imag part */
        } /* else */

        if (SendDlgItemMessage(hwnd, nIDDList, LB_ADDSTRING, DUMMY_WPARAM,
                               (LPARAM)(LPSTR)szComplexRoot) < LB_OKAY)
            return FALSE;
    } /* for */

    return TRUE;                                               /* all ok ! */
} /* DrawListOfRoots */



/* displays the listbox for the nominator and denominator roots
 * if the roots window doesn't exist and the user wish to see it,
   (see main menu checkmark) the function creates the window
 * displays also the system stability information
 * invalidates the root plane for repainting by the window callback
 */
static void DisplayFilterRoots(void)
{
    RECT RootRect, rWndStatus;
    int i, nIddList;
    char szNullRoot[80];
    BOOL bError = FALSE;

    if (MainFilter.f_type == NOTDEF) return;        /* transparent working */

    if (hwndFilterRoots == HWND_NULL)
    {
        hwndFilterRoots = FDWinCreateModelessDlg(MAKEINTRESOURCE(ROOTSDLG),
                                                 hwndFDesk, RootsDlgProc);

        GetWindowRect(hwndFilterRoots, &RootRect);
        GetWindowRect(hwndStatusBox, &rWndStatus);
        MoveWindow(hwndFilterRoots,              /* at right screen corner */
                   GetSystemMetrics(SM_CXSCREEN)-RootRect.right+RootRect.left,   /* X */
                   GetSystemMetrics(SM_CYSCREEN)-RootRect.bottom+RootRect.top-
                   rWndStatus.bottom + rWndStatus.top,   /* Y */
                   RootRect.right-RootRect.left,              /* Width */
                   RootRect.bottom-RootRect.top,             /* Height */
                   FALSE);
    } /* if */

    SetDlgItemText(hwndFilterRoots, IDD_STABILITY, "");
    if (MainFilter.b.order > MainFilter.a.order) nIddList = IDD_ROOT_LIST_ZERO;
    else nIddList = IDD_ROOT_LIST_POLE;

    EnableDlgItem(hwndFilterRoots, IDD_ROOT_LIST_POLE, MainFilter.b.order > 0);
    EnableDlgItem(hwndFilterRoots, IDD_ROOT_LIST_ZERO, MainFilter.a.order > 0);
    if (abs(MainFilter.a.order - MainFilter.b.order) > 0)
        EnableDlgItem(hwndFilterRoots, nIddList, TRUE);

    if (!(MainFilter.uFlags & FILTER_ROOTS_VALID) &&      /* roots valid ? */
        (uDeskOpt & DOPT_SHOWROOTS))
    {
        WorkingMessage(STRING_WAIT_ROOT);
        InitAbortDlg(hwndFDesk, STRING_ABORT_ROOTS);

        bError = CalcRoots(&MainFilter.a) != CALC_OK;
        bError |= CalcRoots(&MainFilter.b) != CALC_OK;

        if (!bError) MainFilter.uFlags |= FILTER_ROOTS_VALID;

        EndAbortDlg();
        WorkingMessage(0);
    } /* if */

    if (MainFilter.uFlags & FILTER_ROOTS_VALID)      /* roots valid ? */
    {
        SetWindowTextId(GetDlgItem(hwndFilterRoots, IDD_STABILITY),
                        SystemStabil(&MainFilter) ? STRING_SYS_STABIL : STRING_SYS_INSTABIL);

        bError |= !DrawListOfRoots(hwndFilterRoots, IDD_ROOT_LIST_ZERO,
                                   &MainFilter.a);
        bError |= !DrawListOfRoots(hwndFilterRoots, IDD_ROOT_LIST_POLE,
                                   &MainFilter.b);
        for (i = 0; (i < abs(MainFilter.a.order - MainFilter.b.order))
                    && !bError; i++)
        {
            sprintf(szNullRoot, FORMAT_ROOT_NULL,
                    (MainFilter.b.order > MainFilter.a.order) ? MainFilter.a.order+i :
                    MainFilter.b.order+i, anPrec[IOUTPRECDLG_ROOTS], 0.0);
            bError |= (SendDlgItemMessage(hwndFilterRoots, nIddList, LB_ADDSTRING,
                                              DUMMY_WPARAM, (LPARAM)(LPSTR)szNullRoot) < LB_OKAY);
        } /* for */
    } /* if valid roots */


    if (bError) ShowRootsWindow(FALSE);
    else
    {
        if (uDeskOpt & DOPT_SHOWROOTS)               /* roots output desired ? */
        {
            if (!IsWindowVisible(hwndFilterRoots))
                ShowWindow(hwndFilterRoots, SW_SHOWNOACTIVATE);
            InvalidateRect(GetDlgItem(hwndFilterRoots, IDD_ROOT_PN), NULL, TRUE);
        } /* if */
    } /* else */
} /* DisplayFilterRoots */



/****** implementation of functions to handle the coefficients window ******/

/* displays the listbox for the nominator and denominator polynomial
   coefficients
 * if the coefficients window not exist and the user wishes to see it,
   (see main menu checkmark) the function creates the window
 * displays also the gain at 0 Hz
 */

static void DisplayFilterCoeffs(void)
{
    RECT rWndCoeff, rWndStatus;
    double dGain;
    double dF0 = 0.0;


    if (MainFilter.f_type == NOTDEF) return;        /* transparent working */

    if (hwndFilterCoeff == HWND_NULL)
    {
        hwndFilterCoeff = FDWinCreateModelessDlg(MAKEINTRESOURCE(COEFFDLG),
                                                 hwndFDesk, CoeffDlgProc);

        GetWindowRect(hwndFilterCoeff, &rWndCoeff);
        GetWindowRect(hwndStatusBox, &rWndStatus);
        MoveWindow(hwndFilterCoeff,              /* at right screen corner */
                   GetSystemMetrics(SM_CXSCREEN)-rWndCoeff.right+rWndCoeff.left,   /* X */
                   GetSystemMetrics(SM_CYSCREEN)-rWndCoeff.bottom+rWndCoeff.top-
                   rWndStatus.bottom + rWndStatus.top,   /* Y */
                   rWndCoeff.right-rWndCoeff.left,              /* Width */
                   rWndCoeff.bottom-rWndCoeff.top,             /* Height */
                   FALSE);
    } /* if */

    /* fill and display the nominator and denominator polynomial listbox */

    SetDlgItemInt(hwndFilterCoeff, IDD_COEFF_NUM_ZERO, MainFilter.a.order, TRUE);
    SetDlgItemInt(hwndFilterCoeff, IDD_COEFF_NUM_POLE, MainFilter.b.order, TRUE);

    if (DrawListFloat(hwndFilterCoeff, IDD_COEFF_LIST_ZERO,
                      MainFilter.a.order, MainFilter.a.coeff,
                      anPrec[IOUTPRECDLG_COEFFS]) &&
        DrawListFloat(hwndFilterCoeff, IDD_COEFF_LIST_POLE,
                      MainFilter.b.order, MainFilter.b.coeff,
                      anPrec[IOUTPRECDLG_COEFFS]))
    {
        if (uDeskOpt & DOPT_SHOWCOEFF)
            if (!IsWindowVisible(hwndFilterCoeff))
                ShowWindow(hwndFilterCoeff, SW_SHOWNOACTIVATE);
    } /* if */

    if (FrequencyResponse(&dF0, &dGain, NULL))   /* calculate DC transfer */
        SetDlgItemFloat(hwndFilterCoeff, IDD_GAIN_VAL, dGain, anPrec[IOUTPRECDLG_GAIN]);
    else SetDlgItemText(hwndFilterCoeff, IDD_GAIN_VAL, "?");

    /* - and now reset selection after filling
       - attention : the focus moves to the coefficients window ! */

    ResetCoeffSelection(FALSE);
    SetFocus(hwndFDesk); /* to set the focus from coeff window to main window */
}


static void EnableCoeffActions(HWND hwnd, BOOL bOn)
{
    EnableDlgItem(hwnd, IDD_COEFF_MODIFY, bOn);
    EnableDlgItem(hwnd, IDD_COEFF_DELETE, bOn);
    EnableDlgItem(hwnd, IDD_COEFF_NORM, bOn);
    EnableDlgItem(hwnd, IDD_COEFF_FACTOR, bOn);
    EnableDlgItem(hwnd, IDD_COEFF_ROUNDING, bOn);
}


/* will be called if any action on coefficients is done
 * disables all buttons in coefficients window
 */
static void ResetCoeffSelection(BOOL bForceFocus)
{
    /* set all other buttons changing coefficients to inactiv state,
       because the user did'nt select any list (give a list the focus) */

    SendDlgItemMessage(hwndFilterCoeff, IDD_COEFF_LIST_ZERO, LB_SETCURSEL,
                       -1, DUMMY_LPARAM);
    SendDlgItemMessage(hwndFilterCoeff, IDD_COEFF_LIST_POLE, LB_SETCURSEL,
                       -1, DUMMY_LPARAM);
    EnableCoeffActions(hwndFilterCoeff, FALSE);
    if (bForceFocus)  SetDlgFocus(hwndFilterCoeff, IDD_COEFF_LIST_ZERO);
} /* ResetCoeffSelection */



static BOOL CopyToTmpFilter(tFilter *lpTmpFlt, tFilter *lpSrcFlt)
{
    *lpTmpFlt = *lpSrcFlt;
    if (!MallocFilter(lpTmpFlt)) return FALSE;       /* not enough memory */
    memcpy((void *)lpTmpFlt->a.coeff, (void *)lpSrcFlt->a.coeff,
           (size_t)(sizeof(lpSrcFlt->a.coeff[0]) * (lpSrcFlt->a.order+1)));
    memcpy((void *)lpTmpFlt->b.coeff, (void *)lpSrcFlt->b.coeff,
           (size_t)(sizeof(lpSrcFlt->b.coeff[0]) * (lpSrcFlt->b.order+1)));
    memcpy((void *)lpTmpFlt->a.root, (void *)lpSrcFlt->a.root,
           (size_t)(sizeof(lpSrcFlt->a.root[0]) * lpSrcFlt->a.order));
    memcpy((void *)lpTmpFlt->b.root, (void *)lpSrcFlt->b.root,
           (size_t)(sizeof(lpSrcFlt->b.root[0]) * lpSrcFlt->b.order));
    return TRUE;
}




BOOL CALLBACK CoeffDlgProc(HWND hwndDlgWin,
                             UINT msg,         /* message */
                             WPARAM wParam,
                             LPARAM lParam)
{
    switch (msg)
    {
	    case WM_INITDIALOG :
            EnableMenuItem(GetSystemMenu(hwndDlgWin, FALSE), SC_MINIMIZE,
                           MF_GRAYED|MF_BYCOMMAND);
            ResetCoeffSelection(FALSE);            /* no listbox selected */
            return TRUE;

        case WM_CLOSE :
            ShowCoeffWindow(FALSE);
            break; /* WM_CLOSE */

        case WM_VKEYTOITEM :       /* only if 'LBS_WANTKEYBOARDINPUT' set */
            if (wParam == VK_SPACE)
                PostMessage(hwndDlgWin, WM_COMMAND, IDD_COEFF_MODIFY,
                            MAKELPARAM(0, BN_CLICKED));
            return -1;  /* set standard reaction */

        case WM_COMMAND :
        {
            BOOL bOkCoeffAction = TRUE;
            BOOL bUsrAck;
            tFilter TmpFilter;              /* to save the original filter */
            PolyDat *lpPoly;                          /* prepare for using */

            switch(GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_COEFF_MODIFY   :
                case IDD_COEFF_DELETE   :
                case IDD_COEFF_NORM     :
                case IDD_COEFF_FACTOR   :
                case IDD_COEFF_ROUNDING :
                    lpPoly = (nIddSelectedList == IDD_COEFF_LIST_ZERO) ? &MainFilter.a : &MainFilter.b;
		            iList = GetDlgListCurSel(hwndDlgWin, nIddSelectedList);
                    if (iList < 0) return TRUE;

                    if (!CopyToTmpFilter(&TmpFilter, &MainFilter)) return TRUE;
            } /* switch */

            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_COEFF_MODIFY :
                {
                    bUsrAck = FDWinDialogBox(COEFFMODIFYDLG, hwndDlgWin, ModifyCoeffDlgProc);
                    if (bUsrAck) lpPoly->coeff[iList] = dRetVal;
                    break;
                } /* case IDD_COEFF_MODIFY */

                case IDD_COEFF_DELETE :
                    bUsrAck = UserOkQuestion(hwndDlgWin, STRING_COEFF_DELETE);
                    if (bUsrAck)
                        bOkCoeffAction = DeletePolyCoeff(iList, lpPoly);
                    break;

                case IDD_COEFF_NORM :
                    bUsrAck = FDWinDialogBox(COEFFNORMDLG, hwndDlgWin, NormCoeffDlgProc);
                    if (bUsrAck)
                        bOkCoeffAction = NormGainTo(lpPoly, &MainFilter,
                                                    dRetVal, 1.0);
                    break;

		        case IDD_COEFF_FACTOR :
		            bUsrAck = FDWinDialogBox(COEFFFACTORDLG, hwndDlgWin, FactorCoeffDlgProc);
                    if (bUsrAck)
		                bOkCoeffAction = MultPolynomUp(lpPoly, dRetVal, 0);
                    break;

		        case IDD_COEFF_ROUNDING :
                    bUsrAck = UserOkQuestion(hwndDlgWin, STRING_COEFF_ROUNDING);
                    if (bUsrAck) bOkCoeffAction = RoundCoefficients(lpPoly);
                    break;

                case IDHELP :
                    WinHelp(hwndDlgWin, HELP_FNAME, HELP_CONTEXT, HelpCoeffsDlg);
                    break;


		        case IDD_COEFF_LIST_POLE :          /* any Listbox-Action */
		        case IDD_COEFF_LIST_ZERO :
		            switch (GET_WM_COMMAND_NOTIFY(wParam, lParam))
	                {
                        case LBN_SELCHANGE:       /* also if new selection */
                        {
			                nIddSelectedList = GET_WM_COMMAND_IDCTL(wParam, lParam);
                            SendDlgItemMessage(      /* reset other list */
                                               hwndDlgWin,
                                (GET_WM_COMMAND_IDCTL(wParam, lParam) ==
                                 IDD_COEFF_LIST_POLE) ? IDD_COEFF_LIST_ZERO:IDD_COEFF_LIST_POLE,
                                LB_SETCURSEL, -1, DUMMY_LPARAM);

                            EnableCoeffActions(hwndDlgWin, TRUE);
			                break;
                        } /* case LBN_SELCHANGE */

			            case LBN_DBLCLK :
                            PostMessage(hwndDlgWin, WM_COMMAND, IDD_COEFF_MODIFY,
                                        MAKELPARAM(0, BN_CLICKED));
                            break;

                        default : return FALSE;
                    } /* switch */
                    break;

                default : return FALSE;
            } /* switch */

            switch(GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_COEFF_MODIFY   :
                case IDD_COEFF_DELETE   :
                case IDD_COEFF_ROUNDING :
                case IDD_COEFF_NORM     :
                case IDD_COEFF_FACTOR   :
                    if (bUsrAck)
                    {
                        if (bOkCoeffAction)
                            bOkCoeffAction = CheckImplementation(&MainFilter);

                        if (bOkCoeffAction)
                        {
                            int CntItems;

                            MainFilter.uFlags &= ~FILTER_SAVED;

                            switch(GET_WM_COMMAND_IDCTL(wParam, lParam))
                            {
                                case IDD_COEFF_MODIFY   :
                                case IDD_COEFF_DELETE   :
                                    MainFilter.uFlags |= FILTER_TYPE_INVALID;
                                    MainFilter.uFlags &= ~FILTER_ROOTS_VALID;
                            } /* switch */

                            FreeFilter(&TmpFilter);
                            DisplayNewFilterData();
                            CntItems = (int)SendDlgItemMessage(hwndDlgWin, nIddSelectedList,
                                                               LB_GETCOUNT, DUMMY_WPARAM,DUMMY_LPARAM);
                            if (iList >= CntItems) iList = CntItems-1;

                            SendDlgItemMessage(hwndDlgWin, nIddSelectedList,
                                               LB_SETCURSEL, (WPARAM)iList,
                                               DUMMY_LPARAM);
                        } /* if */
                        else       /* error handling */
                        {
                            FreeFilter(&MainFilter);
                            MainFilter = TmpFilter;
                            MessageAckUsr(hwndDlgWin, ERROR_FILTER_IMPLEMENT);
                        } /* else */
                    } /* if */
                    else FreeFilter(&TmpFilter);

		            ResetCoeffSelection(TRUE);
            } /* switch */

            break;
        } /* case WM_COMMAND */

        default : return FALSE;
    } /* switch */

    return TRUE;
}


/* callback function for the smart "modify coefficient" dialog window */
#pragma argsused  /* disable "Parameter is never used" Warning */
BOOL CALLBACK ModifyCoeffDlgProc(HWND hwndModify, UINT msg, WPARAM wParam, LPARAM lParam)
/* End of Dialog with FALSE if the user press the CANCEL-button */
{
    switch (msg)
    {
        case WM_INITDIALOG :
            SetDlgItemFloat(hwndModify, IDD_CHANGE_VAL,
                            (nIddSelectedList == IDD_COEFF_LIST_ZERO) ?
                            MainFilter.a.coeff[iList] : MainFilter.b.coeff[iList],
                            anPrec[IOUTPRECDLG_COEFFS]);

            SetDlgItemInt(hwndModify,   /* display current index */
                          IDD_CHANGE_INDEX, iList, TRUE);
            return TRUE;

        case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDOK :
                    if (CheckDlgItemFloat(hwndModify,
                                          IDD_CHANGE_VAL,
                                          IDD_CHANGE_VAL_TEXT,
                                          MIN_COEFF_INPUT,
                                          MAX_COEFF_INPUT,
                                          &dRetVal))
                        EndDialog(hwndModify, TRUE);
                    return TRUE;

                case IDCANCEL : EndDialog(hwndModify, FALSE);
                                return TRUE;

            } /* switch */
    } /* switch */
    return FALSE;
}


#pragma argsused  /* disable "Parameter is never used" Warning */
BOOL CALLBACK FactorCoeffDlgProc(HWND hwndFactor,
                                   UINT msg,         /* message */
                                   WPARAM wParam,
                                   LPARAM lParam)
/* End of Dialog with FALSE if the user press the CANCEL-button */
{
    switch (msg)
    {
        case WM_INITDIALOG :
            SetDlgItemFloat(hwndFactor, IDD_FACTOR_VAL, 1.0, anPrec[IOUTPRECDLG_OTHER]);
            return TRUE;

        case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDOK :
                    if (CheckDlgItemFloat(hwndFactor,
                                          IDD_FACTOR_VAL,
                                          IDD_FACTOR_TEXT,
                                          MIN_COEFF_INPUT,
                                          MAX_COEFF_INPUT,
                                          &dRetVal))
                        EndDialog(hwndFactor, TRUE);
                    return TRUE;

                case IDCANCEL : EndDialog(hwndFactor, FALSE);
                                  return TRUE;
            } /* switch */
    } /* switch */
    return FALSE;
}



#pragma argsused  /* disable "Parameter is never used" Warning */
BOOL CALLBACK NormCoeffDlgProc(HWND hDlg,
                               UINT msg,         /* message */
                               WPARAM wParam,
                               LPARAM lParam)
/* End of Dialog with FALSE if the user press the CANCEL-button */
{
    switch (msg)
    {
        case WM_INITDIALOG :
        {
            double f = 0.0;

            switch (MainFilter.FTr.FTransf)
            {
                case BANDPASS : f = MainFilter.FTr.dCenter; break;

                case BANDSTOP :
                case HIGHPASS : f = MainFilter.f0*0.5;
            } /* switch */

            DrawComboAxisUnits(hDlg, IDD_NORM_UNITBOX, 0,
                               MainFilter.iInputUnitF, FrequencyUnits);

            SetDlgItemFloat(hDlg, IDD_FREQUENCY_VAL,
                            f/FrequencyUnits[MainFilter.iInputUnitF].dUnitFactor,
                            anPrec[IOUTPRECDLG_FREQU]);
            return TRUE;
        } /* WM_INITDIALOG */

        case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDOK :
                {
                    double dUnitFac;
                    dUnitFac =
                        FrequencyUnits[GetDlgListCurSel(hDlg, IDD_NORM_UNITBOX)].dUnitFactor;

                    if (CheckDlgItemFloat(hDlg, IDD_FREQUENCY_VAL,
                                          IDD_FREQUENCY_TEXT,
                                          MIN_F_INPUT/dUnitFac,
                                          MAX_F_INPUT/dUnitFac,
                                          &dRetVal))
                    {
                        dRetVal *= dUnitFac;
                        EndDialog(hDlg, TRUE);
                    } /* if */
                    return TRUE;
                } /* IDOK */

                case IDCANCEL : EndDialog(hDlg, FALSE);
                                return TRUE;
            } /* switch */
    } /* switch */
    return FALSE;
}


/****** implementation of general functions for digital filter design ******/

void DisplayNewFilterData(void)
{
    if (MainFilter.f_type != NOTDEF)
    {
        EnableMenuItem(GetMenu(hwndFDesk), IDM_SAVEFILE, MF_ENABLED);
        EnableMenuItem(GetMenu(hwndFDesk), IDM_SAVEFILE_AS, MF_ENABLED);
        EnableMenuItem(GetMenu(hwndFDesk), IDM_EDITCOEFF, MF_ENABLED);
        EnableMenuItem(GetMenu(hwndFDesk), IDM_EDITPROJECT, MF_ENABLED);
        EnableMenuItem(GetMenu(hwndFDesk), IDM_DEFCHANGE, MF_ENABLED);
        DisplayFilterRoots();
        DisplayFilterCoeffs();
        UpdateDiags(DIAG_F_RESPONSE);
        UpdateDiags(DIAG_ATTENUATION);
        UpdateDiags(DIAG_APPROXCHARAC_RESP);
        UpdateDiags(DIAG_PHASE);
        UpdateDiags(DIAG_PHASE_DELAY);
        UpdateDiags(DIAG_GROUP_DELAY);
        UpdateDiags(DIAG_I_RESPONSE);
        UpdateDiags(DIAG_STEP_RESP);
    } /* if */
} /* DisplayNewFilterData() */


static void CopyVarAxisParameters(tDiagAxis *pDestAxis, tDiagAxis *pSrcAxis)
{
    pDestAxis->AxisFlags = pSrcAxis->AxisFlags;
    pDestAxis->dWorldMin = pSrcAxis->dWorldMin;
    pDestAxis->dWorldMax = pSrcAxis->dWorldMax;
    pDestAxis->sPrecision = pSrcAxis->sPrecision;
    pDestAxis->iCurrUnit = pSrcAxis->iCurrUnit;
}


static BOOL CopyIfEquDiag(HWND h, DIAGCREATESTRUCT *dg, tFdDiags diag_type)
{
    char szClassName[40];
    tDiag *pDiag;

    GetClassName(h, szClassName, DIM(szClassName));
    if (!lstrcmp(szClassName, DIAG_CLASS))
    {
        if (GETDIAGTYPE(h) == diag_type) /* same diag type ? */
        {
            pDiag = GETPDIAG(h);  /* ptr existing diag */
            CopyVarAxisParameters(&dg->lpDiag->X, &pDiag->X);
            CopyVarAxisParameters(&dg->lpDiag->Y, &pDiag->Y);
            if (!(dg->lpDiag->nDiagOpt & DIAG_DISCRET))  /* num samples */
                dg->lpDiag->nConstSmpl = pDiag->nConstSmpl;
                dg->lpDiag->nDiagOpt = pDiag->nDiagOpt;
            return TRUE;
        } /* if */
    } /* if */

    return FALSE;
} /* CheckEquDiagType() */



static void CopyIfEquAxis(HWND h, DIAGCREATESTRUCT *dg, tFdDiags diag_type)
{
    char szClassName[40];
    tDiag *pDiag;

    GetClassName(h, szClassName, DIM(szClassName));
    if (!lstrcmp(szClassName, DIAG_CLASS))
    {
        pDiag = GETPDIAG(h);  /* ptr existing diag */
                                                    /* same x axis ? */
        if (Diags[GETDIAGTYPE(h)].nIdAxisX == Diags[diag_type].nIdAxisX)
            CopyVarAxisParameters(&dg->lpDiag->X, &pDiag->X);

        if (Diags[GETDIAGTYPE(h)].nIdAxisY == Diags[diag_type].nIdAxisY)            
            CopyVarAxisParameters(&dg->lpDiag->Y, &pDiag->Y);
    } /* if */
} /* CheckEquAxis() */



static void CreateNewDiag(tFdDiags diag_type)
{
    DIAGCREATESTRUCT dg;
    tDiag * pD;
    HWND hwndMDIChild;

    if ((pD = MallocErr(hwndFDesk, sizeof(tDiag))) == NULL) return;

    dg.lpDiag = pD;              /* assign pointer to creation struct ptr */
    *pD = Diags[diag_type].DefaultDiag;       /* init current diag struct */
    if (dg.lpDiag->nDiagOpt & DIAG_DISCRET)
        pD->cxySmplCurve = cxyBmpSamples;           /* set line width and */
    else pD->cxySmplCurve = nWidthCurvePen;       /* bitmap size defaults */

    hwndMDIChild = GetFirstChild(hwndDiagBox);

    while (hwndMDIChild)          /* search all MDI childs for compatible */
    {
        if (CopyIfEquDiag(hwndMDIChild, &dg, diag_type)) break;
        hwndMDIChild = GetNextSibling(hwndMDIChild);
    } /* while */

    hwndMDIChild = (HWND)LOWORD(SendMessage(hwndDiagBox, WM_MDIGETACTIVE,
                                            DUMMY_WPARAM, DUMMY_LPARAM));
    if (hwndMDIChild != HWND_NULL)          /* overwrite with active diag */
        if (!CopyIfEquDiag(hwndMDIChild, &dg, diag_type)) hwndMDIChild = HWND_NULL;

    if (hwndMDIChild == HWND_NULL)               /* no equal diag found ? */
    {
        if (MainFilter.f_type != NOTDEF)    /* modify default axis limits */
            switch (pD->X.nIdAxisName)               /* if filter defined */
            {                              
                case STRING_AXIS_FREQUENCY:   /* set max to Nyquist frequ */
                    pD->X.dWorldMax = MainFilter.f0/2.0;
                    pD->X.iCurrUnit = MainFilter.iInputUnitF;
                    break; /* STRING_AXIS_FREQUENCY */

                case STRING_AXIS_TIME : /* set number of taps * sampl time */
                {
                    int i = 0;

                    pD->X.dWorldMax = 1.0/MainFilter.f0;
                    pD->X.dWorldMax *= 1.0+2*max(MainFilter.a.order, MainFilter.b.order);

                    while (FrequencyUnits[i].szUnit != NULL)
                    {
                        if ((int)ROUND(log10(TimeUnits[i].dUnitFactor)) ==
                            -(int)ROUND(log10(FrequencyUnits[MainFilter.iInputUnitF].dUnitFactor)))
                        
                            pD->X.iCurrUnit = i;

                        ++i;
                    } /* while */
                        
                    break;
                } /* STRING_AXIS_TIME */
            } /* switch */


        hwndMDIChild = GetFirstChild(hwndDiagBox);    /* search equal axis */
        while (hwndMDIChild)
        {
            CopyIfEquAxis(hwndMDIChild, &dg, diag_type);
            hwndMDIChild = GetNextSibling(hwndMDIChild);
        } /* while */

        hwndMDIChild = (HWND)LOWORD(SendMessage(hwndDiagBox, WM_MDIGETACTIVE,
                                                DUMMY_WPARAM, DUMMY_LPARAM));
        if (hwndMDIChild != HWND_NULL)    /* overwrite with active diag */
            CopyIfEquAxis(hwndMDIChild, &dg, diag_type);
    } /* if */

    dg.type = diag_type;
    hwndMDIChild = CreateDiagWin(&dg);

    if (diag_type == DIAG_USER_DEF)
        (void)FDWinDialogBoxParam(IDD_MATHFUNCDLG, hwndFDesk,
                                  MathFuncSelectProc, (LPARAM)(LPSTR)hwndMDIChild);
} /* CreateNewDiag */




/* Filter Designer desktop window callback function */

LRESULT CALLBACK FDDeskWndProc(HWND hwndDesk, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_UPDATEMENU :                      /* user defined message */
            UpdateMDIMenuItems();
            return 0L; /* WM_UPDATEMENU */


        case WM_CREATE:
        {
            LPCREATESTRUCT p = (LPCREATESTRUCT)(LPSTR)(lParam);
            CLIENTCREATESTRUCT Client;

            Client.idFirstChild = IDM_FIRST_CHILD;
            Client.hWindowMenu = GetSubMenu(p->hMenu, /* last popup is HELP */
                                            GetMenuItemCount(p->hMenu)-2);

            hwndDiagBox =
                CreateWindow("MDICLIENT", "",
                             WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             0, 0, 0, 0, hwndDesk, 0, p->hInstance, &Client);

            hwndStatusBox =
                CreateWindow(STATIC_CLASS, "",
                             WS_CHILD | WS_BORDER | WS_CLIPCHILDREN | SS_GRAYRECT,
                             0, 0, 0, 0, hwndDesk, 0, p->hInstance, NULL);

            hwndStatus =
                CreateWindow(STATUS_CLASS, "", WS_CHILD | WS_VISIBLE,
                             0, 0, 0, 0, hwndStatusBox, 0, p->hInstance, NULL);

            hwndPosition =
                CreateWindow(STATUS_CLASS, "", WS_CHILD | WS_VISIBLE,
                             0, 0, 0, 0, hwndStatusBox, 0, p->hInstance, NULL);
            return 0L;
        } /* WM_CREATE */



        case WM_NCMOUSEMOVE :
            SetWindowText(hwndPosition, "");  /* clear coordinate display */
            break; /* WM_NCMOUSEMOVE (pass to DefFrameProc) */

        case WM_SYSCOMMAND:
            /* block ALT+F4 independent of SC_CLOSE menuitem state because
               DefFrameProc() handles ALT+F4 not correct under that condition
             */
            if ((GetMenuState(GetSystemMenu(hwndDesk, FALSE), SC_CLOSE, MF_BYCOMMAND) & MF_GRAYED) &&
                ((wParam & 0xFFF0) == SC_CLOSE)) return 0L;
            break; /* WM_SYSCOMMAND (pass to DefFrameProc) */


        case WM_QUERYENDSESSION :                   /* Windows finished ? */
        case WM_CLOSE :                             /* ALT-F4 or IDM_EXIT */
            if (MainFilter.f_type != NOTDEF)
                if (!(MainFilter.uFlags & FILTER_SAVED))
                    if (!UserOkQuestion(hwndDesk, STRING_ENDFILTERPROGRAM))
                        return 0L;

            if (!WriteProfile())
                ErrorAckUsr(hwndDesk, ERROR_INIFILEWRITE);

            DestroyWindow(hwndDesk);
            return TRUE; /* WM_CLOSE, WM_QUERYENDSESSION */


        case WM_DESTROY :
            DestroyWindow(hwndStatus);
            DestroyWindow(hwndPosition);
            DestroyWindow(hwndStatusBox);
            DestroyWindow(hwndDiagBox);
            (void)WinHelp(hwndDesk, NULL, HELP_QUIT, 0);
            PostQuitMessage(0);
            return 0L; /* WM_DESTROY */

        case WM_SIZE:
            if (wParam == SIZEFULLSCREEN || wParam == SIZENORMAL)
            {
                TEXTMETRIC tm;
                HDC dc;
                int cy;
                RECT rcStatBox;
                POINT SizeWin = MAKEPOINT(lParam);
                dc = GetDC(hwndStatus);
                GetTextMetrics(dc, &tm);
                ReleaseDC(hwndStatus, dc);
                SetRectEmpty(&rcStatBox);
                cy = tm.tmHeight + tm.tmExternalLeading;

                /* height of status box window */
                rcStatBox.bottom = cy + 6*GetSystemMetrics(SM_CYBORDER) - 1;

                /* width of cursor window */
                rcStatBox.right = 2*(tm.tmAveCharWidth*(8+DEFAULT_PRECISION));

                /* calc size of window by size of client area */
                AdjustWindowRect(&rcStatBox, GetWindowStyle(hwndStatusBox), FALSE);

                /* calc correct width, height in right, bottom */
                OffsetRect(&rcStatBox, 1-rcStatBox.left, 1-rcStatBox.top);

                MoveWindow(hwndDiagBox, 0, 0, SizeWin.x,
                           SizeWin.y - rcStatBox.bottom, TRUE);
                MoveWindow(hwndStatusBox, 0, SizeWin.y - rcStatBox.bottom,
                           SizeWin.x, rcStatBox.bottom, TRUE);
                cy += 2*GetSystemMetrics(SM_CYBORDER);

                /* move all childs of status box window */
                MoveWindow(hwndPosition, GetSystemMetrics(SM_CXFRAME),
                           (rcStatBox.bottom-cy)/2 - 1,
                           rcStatBox.right, cy, TRUE);

                /* x position of status window */
                rcStatBox.right += 2*GetSystemMetrics(SM_CXFRAME);

                MoveWindow(hwndStatus, rcStatBox.right,
                           (rcStatBox.bottom-cy)/2 - 1,
                           SizeWin.x - rcStatBox.right -
                               2*GetSystemMetrics(SM_CXFRAME)-1,
                           cy, TRUE);
                ShowWindow(hwndDiagBox, SW_SHOW);
                ShowWindow(hwndStatusBox, SW_SHOW);
                return 0L;        /* because MDI client directly moved */
            } /* if */
            break;

        case WM_DEVMODECHANGE:
            UPDATE_MENU();
            break;


        case WM_COMMAND :
        {
             OFSTRUCT ofFile;
             HWND hwndActiveChild =
                 (HWND)LOWORD(SendMessage(hwndDiagBox, WM_MDIGETACTIVE,
                                          DUMMY_WPARAM, DUMMY_LPARAM));

             switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
             {
                  case IDM_SAVEFILE_AS :
                      ofFile.nErrCode = OF_CREATE;
                      if (FDWinDialogBoxParam(IDD_FILEDLG, hwndDesk,
                                              OpenFileDlgProc, (LPARAM)(LPSTR)&ofFile))

                          lstrcpy(szPrjFileName, ofFile.szPathName);
                      else break; /* stop if cancel */

                  case IDM_SAVEFILE :
                      ofFile.nErrCode = OF_CREATE;
                      if (lstrlen(szPrjFileName) == 0)   /* first save ? */
                      {
                          if (!FDWinDialogBoxParam(IDD_FILEDLG,
                                                   hwndDesk, OpenFileDlgProc,
                                                   (LPARAM)(LPSTR)&ofFile))
                              break; /* stop if cancel */
                      } /* if */
                      else lstrcpy(ofFile.szPathName, szPrjFileName);

                      WorkingMessage(STRING_SAVE_FILE);
                      lstrcpy(szPrjFileName, ofFile.szPathName);

                      if (WriteProjectDataFile(szPrjFileName))
                      {
                          MainFilter.uFlags |= FILTER_SAVED;
                          UpdateMainWinTitle();
                      } /* if */
                      else ErrorAckUsr(hwndDesk, ERROR_FILEWRITE);

                      WorkingMessage(0);
                      break; /* IDM_SAVEFILE, IDM_SAVEFILE_AS */


                  case IDM_LOADFILE :
                  case IDM_LOADPROJECT :
                      if (MainFilter.f_type != NOTDEF)
                          if (!(MainFilter.uFlags & FILTER_SAVED))
                              if (!UserOkQuestion(hwndDesk, STRING_OVERWRITE_DATA))
                                  return 0L;

                      ofFile.nErrCode = OF_EXIST;
                      if (FDWinDialogBoxParam(IDD_FILEDLG, hwndDesk,
                                              OpenFileDlgProc, (LPARAM)(LPSTR)&ofFile))
                      {
                          int nErrStr = LoadFilter(GET_WM_COMMAND_IDCTL(wParam, lParam) == IDM_LOADPROJECT,
                                                   ofFile.szPathName);
                          if (nErrStr != IDSTRNULL) ErrorAckUsr(hwndDesk, nErrStr);
                      } /* if */

                      break; /* IDM_LOADFILE */


                  case IDM_PRINT:
                  {
                      HWND *SelArr;

                      if (FDWinDialogBoxParam(IDD_PRINTDLG, hwndDesk, PrintDlgProc, (LPARAM)(LPSTR)&SelArr))
                          FdPrintDiags(hwndDesk, SelArr);
                      break; /* IDM_PRINT */
                  }


                  case IDM_NEWPREDEF :
                  case IDM_NEWSTDIIR :
                  case IDM_NEWLINFIR :
                  case IDM_NEWFIRIIR :
                  {
                      tFilter SavedFilter;
                      tFltDlg FltDlgType;

                      SavedFilter.f_type = NOTDEF;
                      if (MainFilter.f_type != NOTDEF)
                          if (!(MainFilter.uFlags & FILTER_SAVED))
                          {
                              if (!UserOkQuestion(hwndDesk, STRING_OVERWRITE_DATA))
                                  return 0L;
                              SavedFilter = MainFilter;    /* save old */
                          } /* if */

                      switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
                      {
                          case IDM_NEWSTDIIR : FltDlgType = STDIIRDLG; break;
                          case IDM_NEWLINFIR : FltDlgType = LINFIRDLG; break;
                          case IDM_NEWPREDEF : FltDlgType = PREDEFDLG; break;
                          case IDM_NEWFIRIIR : FltDlgType = MISCDLG; break;
                      } /* switch */

                      if (FilterDefinitionDlg(hwndDesk, FltDlgType, TRUE))  
                      {                     /* new definition is made (ok) */
                          if (SavedFilter.f_type != NOTDEF)
                              FreeFilter(&SavedFilter); /* release old mem */
                          DisplayNewFilterData();   /* if ok show new data */
                      } /* if */
                      return 0L;
                  } /* IDM_NEWLINFIR */

                  case IDM_DEFCHANGE :    /* change filter definitions */
                  {
                      if (FilterDefinitionDlg(hwndDesk, MainFilter.FltDlg, FALSE))
                          DisplayNewFilterData();
                      return 0L;
                  } /* IDM_DEFCHANGE */

                  case IDM_NEW_DIAG_FRESP : CreateNewDiag(DIAG_F_RESPONSE); return 0L;
                  case IDM_NEW_DIAG_ATTENUAT : CreateNewDiag(DIAG_ATTENUATION); return 0L;
                  case IDM_NEW_DIAG_PHASE : CreateNewDiag(DIAG_PHASE); return 0L;
                  case IDM_NEW_DIAG_PHASEDELAY : CreateNewDiag(DIAG_PHASE_DELAY); return 0L;
                  case IDM_NEW_DIAG_GROUPDELAY : CreateNewDiag(DIAG_GROUP_DELAY); return 0L;
                  case IDM_NEW_DIAG_IMPULSRESP : CreateNewDiag(DIAG_I_RESPONSE); return 0L;
                  case IDM_NEW_DIAG_STEPRESP : CreateNewDiag(DIAG_STEP_RESP); return 0L;
                  case IDM_NEW_DIAG_APPROXRESP : CreateNewDiag(DIAG_APPROXCHARAC_RESP); return 0L;

                  case IDM_NEW_DIAG_USERDEF : CreateNewDiag(DIAG_USER_DEF); return 0L;

                  case IDM_EDITPROJECT :
                      (void)FDWinDialogBox(IDD_PROJECTDLG, hwndDesk, ProjectDlgProc);
                      return 0L;

                  case IDM_EDITCOMMENT :
                  {
                      CREATEAXISNOTEDLGSTRUCT CreateNoteStruct;
                      pMarker p;

                      CreateNoteStruct.hwndDiag = hwndActiveChild; /* active MDI Child */
                      p = GETPDIAG(CreateNoteStruct.hwndDiag)->pM;

                      if (p == NULL) CreateNoteStruct.PosX = 0.0;
                      else CreateNoteStruct.PosX = p->x;     /* first note */

                      FDWinDialogBoxParam(IDD_AXISNOTESDLG, hwndDesk, AxisNoteDlgProc,
                                          (LPARAM)(LPSTR) &CreateNoteStruct);
                      return 0L;
                  } /* IDM_EDITCOMMENT */

                  case IDM_ICON_ARRANGE :
                      SendMessage(hwndDiagBox, WM_MDIICONARRANGE, DUMMY_WPARAM, DUMMY_LPARAM);
                      return 0L;

                  case IDM_DIAG_TILE_HORIZ :
                      SendMessage(hwndDiagBox, WM_MDITILE, MDITILE_HORIZONTAL, DUMMY_LPARAM);
                      return 0L;

                  case IDM_DIAG_TILE_VERT :
                      SendMessage(hwndDiagBox, WM_MDITILE, MDITILE_VERTICAL, DUMMY_LPARAM);
                      return 0L;

                  case IDM_DIAG_CASCADE :
                      SendMessage(hwndDiagBox, WM_MDICASCADE, DUMMY_WPARAM, DUMMY_LPARAM);
                      return 0L;

                  case IDM_DIAG_CLOSE :
                      SendMessage(hwndDiagBox, WM_MDIDESTROY,
                                  (WPARAM)hwndActiveChild, DUMMY_LPARAM);
                      return 0L; /* IDM_DIAG_CLOSE */

                  case IDM_DIAG_REPAINT :
                      InvalidateRect(hwndActiveChild, NULL, TRUE);
                      return 0L;

                  case IDM_EDITCOEFF :
                      ShowCoeffWindow(TRUE);
                      BringWindowToTop(hwndFilterCoeff);
                      return 0L;

                  case IDM_SHOWCOEFF :
                      uDeskOpt ^= DOPT_SHOWCOEFF;
                      ShowCoeffWindow(uDeskOpt & DOPT_SHOWCOEFF);
                      return 0L;

                  case IDM_SHOWROOTS :
                      uDeskOpt ^= DOPT_SHOWROOTS;
                      ShowRootsWindow(uDeskOpt & DOPT_SHOWROOTS);
                      DisplayFilterRoots();
                      return 0L;

                  case IDM_OPT_AXIS_X	  :
                      FDWinDialogBoxParam(IDD_OPTAXISXDLG,
                                          hwndDesk, OptAxisXDlgProc,
                                          (LPARAM)hwndActiveChild);
                      return 0L;

                  case IDM_OPT_AXIS_Y	  :
                      FDWinDialogBoxParam(IDD_OPTAXISYDLG,
                                          hwndDesk, OptAxisYDlgProc,
                                          (LPARAM)hwndActiveChild);
                      return 0L;

                  case IDM_OPT_USERFN :
                      if (GETDIAGTYPE(hwndActiveChild) == DIAG_USER_DEF)
                          FDWinDialogBoxParam(IDD_MATHFUNCDLG,
                                              hwndDesk, MathFuncSelectProc,
                                              (LPARAM)hwndActiveChild);
                      return 0L;

                case IDM_OPT_TECH :
                    if (FDWinDialogBox(IDD_OPTTECHDLG, hwndDesk, OptMathDlgProc))
                    {
                        DisplayNewFilterData();
                        UpdateDiags(DIAG_USER_DEF);
                    } /* if */

                    return 0L;


                case IDM_OPT_DESK :
                    if (FDWinDialogBox(IDD_OPTDESKDLG, hwndDesk, OptDeskDlgProc))
                    {
                        HWND hMDIChild = GetFirstChild(hwndDiagBox);

                        DisplayFilterRoots();        /* refill list boxes */
                        DisplayFilterCoeffs();     /* (use new precision) */ 

                        /* set size of discret sample points in diags */
                        while (hMDIChild)
                        {
                            char szClassName[40];
                               
                            GetClassName(hMDIChild, szClassName, DIM(szClassName));
                            if (!lstrcmp(szClassName, DIAG_CLASS))
                            {
                                tDiag *pDiag = GETPDIAG(hMDIChild);

                                if (pDiag->nDiagOpt & DIAG_DISCRET)
                                    pDiag->cxySmplCurve = cxyBmpSamples;
                                else
                                    pDiag->cxySmplCurve = nWidthCurvePen;

                                InvalidateRect(hMDIChild, NULL, TRUE);
                            } /* if */

                            hMDIChild = GetNextSibling(hMDIChild); /* get next MDI */
                        } /* while */


                        /* flag DOPT_PHASE... is important only in phase
                           responses
                         */
                        UpdateDiags(DIAG_PHASE);
                    } /* if */

                    return 0L;


                case IDM_HELP_ABOUT :
                    (void)FDWinDialogBox(IDD_ABOUTDLG, hwndDesk, HelpAboutDlgProc);
                    return 0L;


                case IDM_HELP_INDEX:
                    (void)WinHelp(hwndDesk, HELP_FNAME, HELP_INDEX, 0);
                    return 0L;


                case IDM_HELP_HELP :
                    (void)WinHelp(hwndDesk, NULL, HELP_HELPONHELP, 0);
                    return 0L;


                case IDM_EXIT :
                    SendMessage(hwndDesk, WM_CLOSE, DUMMY_WPARAM, DUMMY_LPARAM);
                    return 0L;
             } /* switch */
             break;
        } /* case WM_COMMAND */
    } /* switch */

    return DefFrameProc(hwndDesk, hwndDiagBox, msg, wParam, lParam);
}



static BOOL GetIniFilename(char *szBuf, int nBufSize)
{
    (void)GetWindowsDirectory(szBuf, nBufSize);
    if (*AnsiPrev(szBuf, &szBuf[lstrlen(szBuf)]) != '\\') lstrcat(szBuf, "\\");

    if (GetRCVerStringInfo("InternalName", &szBuf[lstrlen(szBuf)], nBufSize))
    {
        lstrcat(szBuf, "." INI_FILE_EXT);
        return TRUE;
    } /* if */

    return FALSE;
} /* GetIniFilename() */



static BOOL WriteProfile()
{
    char szIniPath[MAXPATH];
    BOOL bOk = FALSE;

    WorkingMessage(STRING_SAVE_INIFILE);
    if (GetIniFilename(szIniPath, DIM(szIniPath)))
        if (WriteIniFile(szIniPath)) bOk = TRUE;
    WorkingMessage(0);

    return bOk;
} /* WriteProfile() */





static int LoadFilter(BOOL bWithPrj, char *szPath)
{
    int idRetStr;

    WorkingMessage(STRING_READ_FILE);
    idRetStr = ReadProjectDataFile(szPath, bWithPrj);
    if (idRetStr == IDSTRNULL)
    {
        lstrcpy(szPrjFileName, szPath);
        MainFilter.uFlags |= FILTER_SAVED;

        DisplayNewFilterData();
        UpdateMainWinTitle();
    } /* if */

    WorkingMessage(0);
    return idRetStr;
} /* LoadFilter() */




/* filter designer main function */

#pragma argsused  /* disable "Parameter is never used" Warning */
int PASCAL WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int CmdShow)
{
    WNDCLASS wndclass;
    MSG msg;                    /* window manager message */
    HINSTANCE hBwcc;               /* Library Handle */
    UINT wPrevErrMode;
    int nErr = IDSTRNULL;
    char szIniPath[MAXPATH] = {'\0'};

    /* check the Windows version in low word return by GetVersion()
     * the high word contains the MS-DOS version !!!
     * WfW 3.11 on MS-DOS 6.22 returns 06160A03, means Windows V 3.10
     * Hex:3.0B       Hex:6.16
     */

    if (LOBYTE(LOWORD(GetVersion())) < MIN_WINDOWS_VERSION)
        FatalAppExit(0, ERROR_WINDOWS_VERSION);

    if ((LOBYTE(LOWORD(GetVersion())) == MIN_WINDOWS_VERSION) &&
        (HIBYTE(LOWORD(GetVersion())) < MIN_WINDOWS_RELEASE))
        FatalAppExit(0, ERROR_WINDOWS_VERSION);

    #if GENBORSTYLE                 /* Borland dialog style (else standard) */
    wPrevErrMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);  /* bug in init bwcc.dll */
    hBwcc = LoadLibrary("bwcc.dll");       /* Load Borland library */
    SetErrorMode(wPrevErrMode);
    if (hBwcc < HINSTANCE_ERROR) FatalAppExit(0, ERROR_LOAD_LIBRARY);
    #endif

    if (hPrevInstance == (HINSTANCE) 0)
    {
        wndclass.lpszClassName = MAINWINDOW_CLASS;
        wndclass.hInstance     = hinstance;
        wndclass.lpfnWndProc   = FDDeskWndProc;
        wndclass.hCursor       = LoadCursor(0, IDC_ARROW);
        wndclass.hIcon         = LoadIcon(hinstance, MAKEINTRESOURCE(IDICO_MAIN));
        wndclass.lpszMenuName  = NULL;
        wndclass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;

        RegisterClass(&wndclass);


        wndclass.lpszClassName = DIAG_CLASS;
        wndclass.hInstance     = hinstance;
        wndclass.lpfnWndProc   = GeneralDiagProc;
        wndclass.hCursor       = (HCURSOR)0;     /* set in window function */
        wndclass.hIcon         = (HICON)0;       /* set in window function */
        wndclass.lpszMenuName  = NULL;
        wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wndclass.style         = CS_HREDRAW | CS_VREDRAW | MDIS_ALLCHILDSTYLES;
        wndclass.cbWndExtra    = sizeof(LONG)+sizeof(WORD)+4*sizeof(int);
                                 /* diag type, diag data pointer, WM_PAINT
                                    update rectangle of MDI window */

        RegisterClass(&wndclass);

        InitDfcgenDlgControls(hinstance);

    }; /* if */

    hwndFDesk = CreateWindow(MAINWINDOW_CLASS, APPNAME,
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             HWND_DESKTOP, LoadMenu(hinstance, "MAINMENU"),
                             hinstance, NULL);   /* create desktop window */

    haccMainMenu = LoadAccelerators(hinstance, "MAINMENUACCEL");

    InitErrorHandler();

    WorkingMessage(STRING_LOAD_INIFILE);
    if (GetIniFilename(szIniPath, DIM(szIniPath)))
    {
        nErr = ReadIniFile(szIniPath);
        if (nErr == IDSTRNULL)
            CmdShow = SW_SHOW;                /* restore with saved flags */
    } /* if */

    UpdateMainWinTitle();
    ShowWindow(hwndFDesk, CmdShow);              /* zoom/show main window */
    WorkingMessage(0);

    if (lstrlen(lpszCmdLine) > 0) (void)LoadFilter(TRUE, lpszCmdLine);
    UpdateMainWinTitle();

    while (GetMessage(&msg, 0, 0, 0)) FDWinMsgHandler(&msg);

    if (MainFilter.f_type != NOTDEF) FreeFilter(&MainFilter); /* free mem */
#if GENBORSTYLE
    FreeLibrary(hBwcc);
#endif
    return 0;
}


