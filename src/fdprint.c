/* DFCGEN Printer Functions

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/fdprint.c,v $
 * $Revision: 1.3 $
 * $Date: 2000-11-11 18:07:08 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $
   Revision 1.2  2000/08/17 12:45:25  ralf
   DFCGEN now is shareware (no registration or license dialogs).
   Directory with examples added.


 */

#include "DFCWIN.H"
#include "FDPRINT.H"
#include <print.h>                     /* windows printer setup & control */
#include <dir.h>



/* printing defines */
#define PRNBORDER_TOP       20
#define PRNBORDER_BOTTOM    15
#define PRNBORDER_RIGHT     15
#define PRNBORDER_LEFT      25
#define DIAGSGAP            15
#define DIAGSPERPAGE        2                 /* number of diags per page */

/* macros */

/* pixel to millimeters conversion */
#define MM_TO_PIXEL_X(dc, mm)   (int)((long)(mm)*GetDeviceCaps(dc, HORZRES) \
                                 / GetDeviceCaps(dc, HORZSIZE))

#define MM_TO_PIXEL_Y(dc, mm)   (int)((long)(mm)*GetDeviceCaps(dc, VERTRES) \
                                 / GetDeviceCaps(dc, VERTSIZE))


/* locale types */
typedef struct tagPrnDev
{
    LPSTR szDevice;
    LPSTR szDriver;
    LPSTR szOutput;
} PrnDriver;


/* locale prototypes */
static BOOL GetPrinterInfo(PrnDriver *DrvInfoStruc);
static void ChkOkEnable(HWND hDlgPrint);
static int DrawInfoXY(HWND hwndOwner, HDC dc, int IdStrHead, char *szInfo, int x, int y);
static BOOL PrintHeadedInfo(HWND hwndOwner, HDC dc, RECT *rcPos, int IdStrHead, char *szInfo);
static void PrintPageNum(HWND hwndOwner, HDC dc, int nPage);
static void PrintFilename(HWND hwndOwner, HDC dc, int x, int y);
static void ShowPrintStatus(HWND hwndOwner, int nPage, PrnDriver *pPrnInfo);
static BOOL PrintDiagList(HWND hwndOwner, ABORTPROC lpitAbort, HWND *pWindows);
static int LinCoordTransf(int val, int div, int mul);
static int DsplToDevPix(HDC dc, int nDsplPixel);
static void DsplToDevRect(HWND hDiagWin, HDC dcPrn, RECT *prcDspl);




/* locale variables */

BOOL IsPrintAbort; /* must be FAR (called with DS from print manager) because
                      modified via AbortProc(), but do not write "static FAR ..."
                      since this corrupts any segment information and you
                      can not start a second instance of DFCGEN
                    */



/* implementation */


static void ChkOkEnable(HWND hDlgPrint)
{
    EnableDlgItem(hDlgPrint, IDOK,
                  0 < (int)SendDlgItemMessage(hDlgPrint, IDD_PRINTSEL, LB_GETSELCOUNT,
                                              DUMMY_WPARAM, DUMMY_LPARAM));
} /* ChkOkEnable() */



/* print dialog */
BOOL CALLBACK PrintDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND **pWndArrPtr;

    switch(msg)
    {
        case WM_INITDIALOG :
        {
            PrnDriver DrvInfo;

            if (GetPrinterInfo(&DrvInfo))
            {
                char szPrinterInfo[256], szFormat[64];
                HWND hwndMDIChild;
                int idx;

                pWndArrPtr = (HWND **)lParam;

                LoadString(GetWindowInstance(hDlg), STRING_PRNINFO, szFormat, DIM(szFormat));
                wsprintf(szPrinterInfo, szFormat, DrvInfo.szDevice, DrvInfo.szOutput);
                SetDlgItemText(hDlg, IDD_PRINTDEVICE, szPrinterInfo);

                hwndMDIChild = GetFirstChild(hwndDiagBox);
                while (hwndMDIChild)
                {
                    if ((GETDIAGTYPE(hwndMDIChild) == DIAG_USER_DEF) ||
                        (MainFilter.f_type != NOTDEF))
                    {
                        char szWindowTitle[128];

                        GetWindowText(hwndMDIChild, szWindowTitle, DIM(szWindowTitle));
                        idx = (int)SendDlgItemMessage(hDlg, IDD_PRINTSEL,
                                                      LB_ADDSTRING, DUMMY_WPARAM,
                                                      (LPARAM)(LPSTR)szWindowTitle);
                        if (idx >= LB_OKAY)
                            SendDlgItemMessage(hDlg, IDD_PRINTSEL,
                                               LB_SETITEMDATA, idx,
                                               (LPARAM)(LPSTR)hwndMDIChild);
                    } /* if */

                    hwndMDIChild = GetNextSibling(hwndMDIChild);
                } /* while */

                ChkOkEnable(hDlg);
            } /* if */
            else
            {
                ErrorAckUsr(hDlg, ERROR_PRINTDRVINFO);
                EndDialog(hDlg, FALSE);
            } /* else */
           
            return TRUE;
        } /* WM_INITDIALOG */

        case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_PRINTALL:
                    SendDlgItemMessage(hDlg, IDD_PRINTSEL, LB_SETSEL, TRUE, -1);
                    ChkOkEnable(hDlg);
                    return TRUE;


                case IDD_PRINTSEL:
                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == LBN_SELCHANGE)
                        ChkOkEnable(hDlg);
                    return TRUE;


                case IDD_PRINTSETUP:
                {
                    HINSTANCE hInstPrintDrv;
                    LPDEVMODE PrnDevMode;
                    LPFNDEVMODE pExtDeviceMode;
                    PrnDriver DrvInfo;
                    char szDriverPath[MAXPATH];
                    int nErr = ERROR_PRINTOPTNOTAVAIL;

                    if (GetPrinterInfo(&DrvInfo))
                    {
                        int SizeDevModeStruc;

                        strcat(strcpy(szDriverPath, DrvInfo.szDriver), ".DRV");

                        /* load driver module (library) */
                        hInstPrintDrv = LoadLibrary(szDriverPath);  
                        if (hInstPrintDrv >= HINSTANCE_ERROR) /* no error ? */
                        {
                            /* get address of exported function "EXTDEVICEMODE"
                               (earlier Windows versions as 3.0 does not
                               export this, but "DEVICEMODE")
                             * at next step call the function to get the size
                               of DEVMODE stucture
                             */

                             pExtDeviceMode = (LPFNDEVMODE)GetProcAddress(hInstPrintDrv, PROC_EXTDEVICEMODE);
                             SizeDevModeStruc =
                                 (*pExtDeviceMode)(hDlg, hInstPrintDrv, NULL,
                                                   DrvInfo.szDevice, DrvInfo.szOutput,
                                                   NULL, NULL, 0);
                            if (SizeDevModeStruc > 0)
                            {
                                PrnDevMode = MALLOC(SizeDevModeStruc);

                                if (PrnDevMode == NULL) nErr = IDSTROUTOFMEMORY;
                                else
                                {
                                    (*pExtDeviceMode)(hDlg, hInstPrintDrv, PrnDevMode,
                                                      DrvInfo.szDevice, DrvInfo.szOutput,
                                                      PrnDevMode, NULL, DM_COPY);
                                    if (IDOK == (*pExtDeviceMode)(hDlg, hInstPrintDrv, PrnDevMode,
                                                                  DrvInfo.szDevice, DrvInfo.szOutput,
                                                                  PrnDevMode, NULL, DM_PROMPT|DM_MODIFY|DM_COPY))
                                    {
                                        (*pExtDeviceMode)(hDlg, hInstPrintDrv, NULL,
                                                          DrvInfo.szDevice, DrvInfo.szOutput,
                                                          PrnDevMode, NULL, DM_MODIFY|DM_UPDATE);
                                        nErr = IDSTRNULL;
                                    } /* if */

                                    FREE(PrnDevMode);
                                } /* else */
                            } /* if */

                            FreeLibrary(hInstPrintDrv);
                        } /* if */
                    } /* if */

                    if (nErr != IDSTRNULL) ErrorAckUsr(hDlg, nErr);
                    return TRUE; 
                } /* IDD_PRINTSETUP */



                case IDOK:
                {
                    int CntSel =
                        (int)SendDlgItemMessage(hDlg, IDD_PRINTSEL, LB_GETSELCOUNT,
                                                DUMMY_WPARAM, DUMMY_LPARAM);
                    if (CntSel > 0)
                    {
                        int iSel;
                        *pWndArrPtr = MallocErr(hDlg, (1+CntSel)*sizeof(HWND));

                        if (*pWndArrPtr != NULL)
                        {
                            HWND *p = *pWndArrPtr;

                            CntSel =
                                (int)SendDlgItemMessage(hDlg, IDD_PRINTSEL,
                                                        LB_GETCOUNT, DUMMY_WPARAM, DUMMY_LPARAM);

                            for (iSel = 0; iSel < CntSel; iSel++)
                            {
                                if (SendDlgItemMessage(hDlg, IDD_PRINTSEL,
                                                       LB_GETSEL, iSel, DUMMY_LPARAM))
                                {
                                    *p = (HWND)SendDlgItemMessage(hDlg, IDD_PRINTSEL,
                                                                  LB_GETITEMDATA, iSel, DUMMY_LPARAM);
                                    ++p;
                                } /* if */
                            } /* for */

                            *p = HWND_NULL;           /* end of list marker */
                        } /* if */
                    } /* if */

                    EndDialog(hDlg, TRUE);
                    return TRUE;
                } /* IDOK */

                case IDCANCEL :                             /* Escape Key */
                    EndDialog(hDlg, FALSE);
                    return TRUE;

                case IDHELP :
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpPrintDlg);
                    return TRUE;

            } /* switch */

            break; /* WM_COMMAND */
    } /* switch */

    return FALSE;
} /* PrintDlgProc */






/* check if printer is compatible for DFCGEN diags printing
 */
BOOL IsPrinterFdCompatible()
{
    PrnDriver DrvInfo;

    if (GetPrinterInfo(&DrvInfo))
    {
        HDC dc;
        BOOL IsCompatible;

        dc = CreateIC(DrvInfo.szDriver, DrvInfo.szDevice, DrvInfo.szOutput, NULL);
        if (dc == HDC_NULL) return FALSE;

        IsCompatible = (GetDeviceCaps(dc, CLIPCAPS) & (CP_RECTANGLE|CP_REGION)) &&
                       (GetDeviceCaps(dc, RASTERCAPS) & (RC_BITBLT|RC_STRETCHBLT|RC_SCALING)) &&
                       (GetDeviceCaps(dc, POLYGONALCAPS) & (PC_RECTANGLE|PC_SCANLINE));
        DeleteDC(dc);
        return IsCompatible;
    } /* if */

    return FALSE;
} /* IsPrinterFdCompatible() */



/* draws the text with header to passed position (x, y)
 * returns the height of printed line
 */
static int DrawInfoXY(HWND hwndOwner, HDC dc, int IdStrHead, char *szInfo, int x, int y)
{
    LOGFONT lfPrint;
    RECT rcOut;
    int x1, cyLine;
    char szHead[SIZE_PRJDESC];
    HFONT hfPrint, hfStd = NULL;

    memset(&lfPrint, 0, sizeof(lfPrint));         /* define standard font */
    lfPrint.lfWeight = FW_BOLD;             /* bold characters for header */

    if ((hfPrint = CreateFontIndirect(&lfPrint)) != NULL)
        hfStd = SelectFont(dc, hfPrint);               /* and get into dc */

    SetTextAlign(dc, TA_LEFT|TA_TOP);
    LoadString(GetWindowInstance(hwndOwner), IdStrHead, szHead, DIM(szHead));

    rcOut.left = x; rcOut.top = y;           /* prepare working rectangle */
    DrawText(dc, szHead, -1, &rcOut,     /* calc width of rect (no print) */
             DT_CALCRECT|DT_EXTERNALLEADING|DT_TOP|DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
    cyLine = rcOut.bottom - rcOut.top;  /* save height of bold characters */
    x1 = rcOut.right+1;                            /* save right position */
    DrawText(dc, szHead, -1, &rcOut,                      /* print header */
             DT_TOP|DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_EXTERNALLEADING);
    if (hfPrint != NULL)
	    DeleteFont(SelectFont(dc, hfStd));        /* back to default font */

    rcOut.top = y;                                  /* prepare new output */
    rcOut.left = x1;                         /* set for new left position */
    rcOut.right = GetDeviceCaps(dc, HORZRES) - MM_TO_PIXEL_X(dc, PRNBORDER_RIGHT);

    lfPrint.lfWeight = FW_NORMAL;           /* normal characters for text */
    if ((hfPrint = CreateFontIndirect(&lfPrint)) != NULL)
        hfStd = SelectFont(dc, hfPrint);               /* and get into dc */

    DrawText(dc, szInfo, -1, &rcOut,                        /* print info */
             DT_CALCRECT|DT_EXTERNALLEADING|DT_TOP|DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
    if (rcOut.bottom - rcOut.top > cyLine)    /* bold characters higher ? */
        cyLine = rcOut.bottom - rcOut.top;
    DrawText(dc, szInfo, -1, &rcOut,                        /* print info */
             DT_TOP|DT_LEFT|DT_WORDBREAK|DT_EXTERNALLEADING|DT_NOPREFIX);
    if (hfPrint != NULL)
	    DeleteFont(SelectFont(dc, hfStd));        /* back to default font */

    return cyLine;
} /* PrintHeadedInfo() */





/* returns TRUE if the string was printed into passed rectangle
 */

static BOOL PrintHeadedInfo(HWND hwndOwner, HDC dc, RECT *prcPos, int IdStrHead, char *szInfo)
{
    if (lstrlen(szInfo) == 0) return FALSE;         /* nothing to print ? */
    prcPos->top += DrawInfoXY(hwndOwner, dc, IdStrHead, szInfo, prcPos->left, prcPos->top);
    return TRUE;
} /* PrintHeadedInfo() */


/* prints page number at top of page
 */
static void PrintPageNum(HWND hwndOwner, HDC dc, int nPage)
{
    char szPageNumForm[32], szNum[40];
    UINT OldAlign;

    LoadString(GetWindowInstance(hwndOwner), STRING_PRNPAGE, szPageNumForm, DIM(szPageNumForm));
    wsprintf(szNum, szPageNumForm, nPage++);
    OldAlign = SetTextAlign(dc, TA_TOP|TA_CENTER); /* page number centered */
    TextOut(dc, GetDeviceCaps(dc, HORZRES)/2, 0, szNum, lstrlen(szNum));
    SetTextAlign(dc, OldAlign);                       /* back to old align */
} /* PrintPageNum() */



/* prints filename at bottom of page
 */
static void PrintFilename(HWND hwndOwner, HDC dc, int x, int y)
{
    if ((MainFilter.f_type != NOTDEF) && (lstrlen(szPrjFileName) > 0))
        (void)DrawInfoXY(hwndOwner, dc, STRING_PRNHEADFILENAME,
                         szPrjFileName, x, y);
} /* PrintFilename() */


/* updates the printer status window with passed information (displays
   the new page number, printer driver and output port)
 */
static void ShowPrintStatus(HWND hwndOwner, int nPage, PrnDriver *pPrnInfo)
{
    char szFormat[128], szLine[256];

    LoadString(GetWindowInstance(hwndOwner), STRING_PRNINFO, szFormat, DIM(szFormat));
    lstrcat(szFormat, "\n");
    LoadString(GetWindowInstance(hwndOwner), STRING_PRNPAGE,
               szFormat+lstrlen(szFormat), DIM(szFormat)-lstrlen(szFormat));
    wsprintf(szLine, szFormat, pPrnInfo->szDevice, pPrnInfo->szOutput, nPage);
    ChangeAbortString(szLine);            /* view driver, port, page info */
} /* ShowPrintStatus() */




/* prints all passed diags to default printer
 */
void FdPrintDiags(HWND hwndParent, HWND *pWindows)
{
    ABORTPROC lpitAbort;                /* abort procedure instance thunk */
    void *pMallocedSpace = pWindows;

    IsPrintAbort = FALSE;                             /* reset abort flag */
    lpitAbort = (ABORTPROC)MakeProcInstance((FARPROC)FdPrintAbortCallback, GetWindowInstance(hwndParent));
    InitAbortDlg(hwndParent, IDSTRNULL);

    if (!PrintDiagList(hwndParent, lpitAbort, pWindows)) /* print all diags */
        MessageAckUsr(hwndParent, ERROR_PRINTDOC);

    EndAbortDlg();
    FreeProcInstance((FARPROC)lpitAbort);
    FREE (pMallocedSpace);
} /* FdPrintDiags() */


/* the print manager calls this function on errors or testing
   job status progress
 * the function returns TRUE if it wants to preceed the print job 
 */
#pragma argsused             /* disable "Parameter is never used" Warning */
BOOL CALLBACK FdPrintAbortCallback(HDC dc, int error)
{
    if (error != PR_JOBSTATUS)                     /* callback on error ? */
    {
        IsPrintAbort = TRUE;                 /* cancel printing in DFCGEN */
        return FALSE;                /* cancel print job in print manager */
    } /* if */
    else return !IsPrintAbort; /* return the status (CANCEL pressed by user) */
} /* FdPrintAbortCallback() */



/* transformation of display pixel width into device pixel width of the same
   size in millimeters based on horizontal dimensions
 */
static int DsplToDevPix(HDC dc, int nDsplPixel)
{
    int nDevPixel = 1;
    HDC hdcDspl = CreateIC("DISPLAY", NULL, NULL, NULL);

    if (hdcDspl != HDC_NULL)
    {
        float fDevPix =
            ROUND((float)(nDsplPixel)
                  * GetDeviceCaps(hdcDspl, HORZSIZE)
                  * GetDeviceCaps(dc, HORZRES)
                  / GetDeviceCaps(hdcDspl, HORZRES)
                  / GetDeviceCaps(dc, HORZSIZE));

        if (fabs(fDevPix) <= 1.0) nDevPixel = SIGN(fDevPix);
        else
            if (fabs(fDevPix) > MAXINT-1) nDevPixel = SIGN(fDevPix)*(MAXINT-1);
            else nDevPixel = (int) fDevPix;

        DeleteDC(hdcDspl);
    } /* if */

    return nDevPixel;
} /* DsplToDevPix() */



static int LinCoordTransf(int nVal, int nDiv, int nMul)
{
    long lv = ((long)nVal)*nMul/nDiv;
    if (lv > MAXINT) return MAXINT;
    if (lv <= -MAXINT) return -(MAXINT-1);
    return lv;
} /* LinCoordTransf() */


/* transformation of display pixel into device pixel based on selected
   font metrics
 */
static void DsplToDevRect(HWND hDiagWin, HDC dcPrn, RECT *pRect)
{
    TEXTMETRIC tmDspl, tmPrn;
    HDC dcDspl;

    dcDspl = GetDC(hDiagWin);
    GetTextMetrics(dcDspl, &tmDspl);
    GetTextMetrics(dcPrn, &tmPrn);

    tmPrn.tmHeight += tmPrn.tmExternalLeading;       /* add line distance */
    tmDspl.tmHeight += tmDspl.tmExternalLeading;

    pRect->top = LinCoordTransf(pRect->top, tmDspl.tmHeight, tmPrn.tmHeight);
    pRect->bottom = LinCoordTransf(pRect->bottom, tmDspl.tmHeight, tmPrn.tmHeight);
    pRect->left = LinCoordTransf(pRect->left, tmDspl.tmAveCharWidth, tmPrn.tmAveCharWidth);
    pRect->right = LinCoordTransf(pRect->right, tmDspl.tmAveCharWidth, tmPrn.tmAveCharWidth);
    
    DeleteDC(dcDspl);
} /* DsplToDevRect() */




/* prints all passed diags to default printer
 */
static BOOL PrintDiagList(HWND hwndOwner, ABORTPROC lpitAbort, HWND *pWindows)
{
    COLORREF aColor[SIZE_COLOR_ARR] = {BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK};
    PrnDriver DrvInfo;

    if (GetPrinterInfo(&DrvInfo))
    {
        HDC dc;

        dc = CreateDC(DrvInfo.szDriver, DrvInfo.szDevice, DrvInfo.szOutput, NULL);

        if (dc != HDC_NULL)
        {
            char szLine[256];
            RECT rcPrnPage;
            TEXTMETRIC tm;
            int cyDiag, cyChar, nPage = 1;

            DOCINFO Doc = {sizeof(DOCINFO), "", NULL};

            rcPrnPage.top = MM_TO_PIXEL_Y(dc, PRNBORDER_TOP);
            rcPrnPage.bottom = GetDeviceCaps(dc, VERTRES) - MM_TO_PIXEL_Y(dc, PRNBORDER_BOTTOM);
            rcPrnPage.left = MM_TO_PIXEL_X(dc, PRNBORDER_LEFT);
            rcPrnPage.right = GetDeviceCaps(dc, HORZRES) - MM_TO_PIXEL_X(dc, PRNBORDER_RIGHT);

            SetAbortProc(dc, lpitAbort);           /* before StartDoc() ! */

            Doc.lpszDocName = szLine;
            GetWindowText(hwndOwner, szLine, DIM(szLine));
            if (StartDoc(dc, &Doc) <= SP_ERROR) return FALSE;

            GetTextMetrics(dc, &tm);
            cyChar = tm.tmHeight + tm.tmExternalLeading;
            cyDiag = (rcPrnPage.bottom - rcPrnPage.top
                      - (DIAGSPERPAGE-1)*MM_TO_PIXEL_Y(dc, DIAGSGAP))
                     / DIAGSPERPAGE - cyChar - 1;       /* title height ! */

            if (StartPage(dc) <= 0)                         /* first page */
            {
                EndDoc(dc);                /* end of document since error */
                return FALSE;
            } /* if */

            ShowPrintStatus(hwndOwner, nPage, &DrvInfo);
            PrintPageNum(hwndOwner, dc, nPage);  /* print page number top */
            PrintFilename(hwndOwner, dc, rcPrnPage.left, rcPrnPage.bottom+cyChar);

            if (MainFilter.f_type != NOTDEF)
            {
                BOOL bLineOut = PrintHeadedInfo(hwndOwner, dc, &rcPrnPage,
                                                STRING_PRNHEADPROJECT, MainFilter.szPrjName);
                bLineOut |= PrintHeadedInfo(hwndOwner, dc, &rcPrnPage,
                                            STRING_PRNHEADPRJDESC, MainFilter.szPrjDesc);
                if (bLineOut) rcPrnPage.top += MM_TO_PIXEL_Y(dc, DIAGSGAP);
            } /* if */

            while ((*pWindows != HWND_NULL) && !IsPrintAbort)
            {
                tDiag PrnDiag;
                tDiag *pWinDiag = GETPDIAG(*pWindows);
                pMarker pLastNote, pNewNote, pWinNote;


                if (rcPrnPage.top+cyDiag >= rcPrnPage.bottom)
                {                              /* if would be out of page */
                    rcPrnPage.top = MM_TO_PIXEL_Y(dc, PRNBORDER_TOP);

                    EndPage(dc);                  /* end of previous page */
                    StartPage(dc);           /* next page (ignore errors) */
                    PrintPageNum(hwndOwner, dc, ++nPage);  /* page number */
                    PrintFilename(hwndOwner, dc, rcPrnPage.left, rcPrnPage.bottom+cyChar);
                    ShowPrintStatus(hwndOwner, nPage, &DrvInfo);
                } /* if */


                GetWindowText(*pWindows, szLine, DIM(szLine));
                (void)PrintHeadedInfo(hwndOwner, dc, &rcPrnPage,
                                      STRING_PRNHEADDIAGTYPE, szLine);

                PrnDiag = *pWinDiag;                         /* copy diag */
                PrnDiag.pM = pLastNote = NULL;

                pWinNote = pWinDiag->pM;  /* prepare copy of all comments */
                while (pWinNote != NULL)
                {
                    pNewNote = MALLOC(sizeof(tMarker));  /* get mem space */
                    if (pLastNote == NULL) PrnDiag.pM = pNewNote;
                    else
                        pLastNote->pNextMarker = pNewNote;

                    if (pNewNote != NULL)                  /* malloc ok ? */
                    {
                        *pNewNote = *pWinNote;   /* copy note description */
                        pNewNote->pNextMarker = NULL;     /* no successor */
                        pNewNote->hrgnHotSpot = HRGN_NULL; /* no hot spot */
                        DsplToDevRect(*pWindows, dc, &pNewNote->rcNote);
                    } /* if */
                    else
                    {
                        ErrorAckUsr(hwndOwner, IDSTROUTOFMEMORY);
                        break; /* while loop */
                    } /* else */

                    pWinNote = pWinNote->pNextMarker;
                    pLastNote = pNewNote;     /* save ptr to last comment */
                } /* while */


                PrnDiag.hrgnCurve = NULL;           /* nothing to destroy */
                if (PrnDiag.cxySmplCurve > 1) /* do not touch if smallest */
                {
                    PrnDiag.cxySmplCurve = DsplToDevPix(dc, PrnDiag.cxySmplCurve)/2;
                    if (PrnDiag.cxySmplCurve < 1) PrnDiag.cxySmplCurve = 1;
                } /* if */

                PrnDiag.X.nScreenMin = rcPrnPage.left;
                PrnDiag.X.nScreenMax = rcPrnPage.right;
                PrnDiag.Y.nScreenMin = rcPrnPage.top;
                PrnDiag.Y.nScreenMax = rcPrnPage.top + cyDiag;

                if (PaintDiag(GetWindowInstance(hwndOwner), dc, ChkAbort, &PrnDiag, aColor))
                    IsPrintAbort = ChkAbort();     /* check CANCEL button */
                else IsPrintAbort = TRUE;

                if (PrnDiag.hrgnCurve != HRGN_NULL)    /* kill new region */
                    DeleteRgn(PrnDiag.hrgnCurve);

                pNewNote = PrnDiag.pM;       /* free copy of all comments */
                while (pNewNote != NULL)
                {
                    if (pNewNote->hrgnHotSpot != HRGN_NULL) DeleteRgn(pNewNote->hrgnHotSpot);
                    pLastNote = pNewNote;
                    pNewNote = pLastNote->pNextMarker;
                    FREE(pLastNote);            /* free note memory space */
                } /* while */

                ++pWindows;
                rcPrnPage.top += cyDiag+MM_TO_PIXEL_Y(dc, DIAGSGAP);

            } /* while */

            EndPage(dc);                                     /* last page */
            EndDoc(dc);
            DeleteDC(dc);

            return TRUE; /* all ok */
        } /* if */
    } /* if */

    return FALSE; /* error */
} /* PrintDiagList() */



static BOOL GetPrinterInfo(PrnDriver *DrvInfoStruc)
{

    static char szPrinter[128];

    GetProfileString("windows", "device", ",,,", szPrinter, DIM(szPrinter));

    #pragma option -w-pia   /* suppress warning "Possibly incorrect assignment" */
    return ((DrvInfoStruc->szDevice = strtok(szPrinter, ",")) &&
            (DrvInfoStruc->szDriver = strtok(NULL, ",")) &&
            (DrvInfoStruc->szOutput = strtok(NULL, ",")));
    #pragma option -wpia
} /* GetPrinterInfo() */



