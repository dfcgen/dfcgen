#include "DFCWIN.H"
#include <ctype.h>
#include <dir.h>                     /* definition of MAXPATH, MAXEXT, ... */


#define FORMAT_DOUBLE "%.*lG"    /* any double output format */
#define FORMAT_DOUBLE_LIST "[%d] " FORMAT_DOUBLE /* "index:val" double format in list box */


/* prototypes of local functions */
static BOOL ToggleMenuCheckmark(HMENU menu, int nIdMenu);
static BOOL DisplayDirList(HWND hDlg, char *szTmpPath);
static void AssembleFullFilename(HWND hDlg, char *szDir);
static int GetDlgItemIntEx(HWND hDlg, int IdEditDlgItem, int rmin, int rmax, int *val);


void DeleteChars(char *lpSrc, char *lpDelChar) /* erase special chars */
{
    BOOL bIsInSet;
    char *lpCharInSet;
    char *lpDest = lpSrc;

    while (*lpSrc)
    {
	lpCharInSet = lpDelChar;
	bIsInSet = FALSE;
	while (*lpCharInSet && !bIsInSet)
	    bIsInSet = (*lpCharInSet++ == *lpSrc);   /* char in set ? */

	if (!bIsInSet)
	    *lpDest++ = *lpSrc;
	lpSrc++;
    } /* while */

    *lpDest = NULL;
}


/************* Resources (RC-file) access functions */

BOOL GetRCVerStringInfo(char *szInfoType, char *pBuffer, int cbSize)
{
    BOOL bOk;
    char szName[256];
    UINT dwSize;
    DWORD hVerInfoBlock;
    void *pMemVerData;
    void FAR *pData;

    HMODULE hM = (HMODULE)GetClassWord(hwndFDesk, GCW_HMODULE);
    GetModuleFileName(hM, szName, DIM(szName)); /* get startup path\filename */

    dwSize = (UINT)GetFileVersionInfoSize(szName, &hVerInfoBlock); /* from VER.H */
    if (dwSize < 1) return FALSE;
    if ((pMemVerData = MALLOC((size_t)dwSize)) == NULL) /* malloc Ver.-Info space */
        return FALSE; 

    bOk = GetFileVersionInfo(szName, hVerInfoBlock, dwSize, pMemVerData) != 0;

    if (bOk)
    {         /* get first language and char set in a long word (*pData) */
        bOk = VerQueryValue(pMemVerData, "\\VarFileInfo\\Translation",
                            &pData, &dwSize);
        if (bOk)
        {
            /* get UINT array with language and charset identifier from
               VarFileInfo information block in version ressource */
            wsprintf(szName, "\\StringFileInfo\\%04.4X%04.4X\\%s",
                     ((WORD *)pData)[0],   /* language (0x0407 for german) */
                     ((WORD *)pData)[1],   /* charset (0x04E4 for Windows) */
                     szInfoType);

            /* access string data in file version info block (pMemVerData) */
            bOk = VerQueryValue(pMemVerData, szName,
                                &pData, &dwSize);
            if (bOk)
            {
                dwSize = min(cbSize-1, dwSize);
                lstrcpy(pBuffer, pData);
                pBuffer[dwSize] = '\0';
            } /* if */
        } /* if */
    } /* if */

    if (!bOk) pBuffer[0] = '\0';
    FREE(pMemVerData);
    return bOk;
}



static HGLOBAL _hMemOpenRC;

/* loads RC-data resource from resource file into memory (do not call
   recursive)
 * returns NULL if error
 * FreeRCdata() frees and dicards the resource
 */
LPSTR LoadRCdata(HINSTANCE hInst, int rcId)
{
    HRSRC hrc = FindResource(hInst, MAKEINTRESOURCE(rcId), RT_RCDATA);
    if (hrc == (HRSRC)0) return NULL;

    _hMemOpenRC = LoadResource(hInst, hrc);
    if (_hMemOpenRC == (HGLOBAL)0) return NULL;
    return LockResource(_hMemOpenRC);
} /* LoadRCdata() */



void FreeRCdata()
{
    UnlockResource(_hMemOpenRC);
    FreeResource(_hMemOpenRC);
} /* FreeRCdata() */



int FDWinDialogBox(int nIddDlg, HWND hwndParent, DLGPROC fnDialog)
{
    DLGPROC lpitDlg;         /* instance thunk */
    BOOL bRet;
    HINSTANCE hInstThisAppl = GetWindowInstance(hwndParent);
    lpitDlg = (DLGPROC)MakeProcInstance((FARPROC)fnDialog, hInstThisAppl);
    bRet = DialogBox(hInstThisAppl,      /* Cancel ? */
                     MAKEINTRESOURCE(nIddDlg), hwndParent, lpitDlg);
    FreeProcInstance((FARPROC)lpitDlg);
    if (bRet == -1) ErrorAckUsr(hwndParent, IDSTROUTOFMEMORY);
    return bRet;
}


int FDWinDialogBoxParam(int nIddDlg, HWND hwndParent,
                        DLGPROC fnDialog, LPARAM dwInitParam)
{
    DLGPROC lpitDlg;         /* instance thunk */
    BOOL bRet;
    HINSTANCE hInstThisAppl = GetWindowInstance(hwndParent);
    lpitDlg = (DLGPROC)MakeProcInstance((FARPROC)fnDialog, hInstThisAppl);
    bRet = DialogBoxParam(hInstThisAppl,      /* Cancel ? */
                          MAKEINTRESOURCE(nIddDlg), hwndParent, lpitDlg, dwInitParam);
    FreeProcInstance((FARPROC)lpitDlg);
    if (bRet == -1) ErrorAckUsr(hwndParent, IDSTROUTOFMEMORY);
    return bRet;
} /* FDWinDialogBoxParam() */


HWND FDWinCreateModelessDlg(LPCSTR lpTemplateName, HWND hwndParent,
                           DLGPROC fnDialog)
{
    HINSTANCE hInstThisAppl = GetWindowInstance(hwndFDesk);
    DLGPROC lpitDlg = (DLGPROC)MakeProcInstance((FARPROC)fnDialog, hInstThisAppl);
    return CreateDialog(hInstThisAppl, MAKEINTRESOURCE(lpTemplateName),
                        hwndParent, lpitDlg);
} /* FDWinCreateModelessDlg */



/* pops up message box with error text related to iErrNo string ressource
 * sets focus to edit dialog item and marks input text 
 * returns TRUE if no cancelling, else FALSE
 */

BOOL HandleDlgError(HWND hDlgParent, int iDlgItemId, int iErrNo)
{
    BOOL bOk;                                /* TRUE if no user cancelling */
    char acErrMsg[256];               /* Error Message from resources file */

    LoadString(GetWindowInstance(hDlgParent), iErrNo, acErrMsg,
               DIM(acErrMsg));
    bOk = (IDOK == FDWINMESSAGEBOX(hDlgParent, acErrMsg, NULL,
                                   MB_OKCANCEL|MB_APPLMODAL|MB_ICONQUESTION));

    if (bOk) SetDlgFocus(hDlgParent, iDlgItemId);         /* set focus */
    else EndDialog(hDlgParent, FALSE);
    return bOk;
} /* HandleDlgError */



BOOL HandleDlgNumInpErr(HWND hDlg, int IdEditDlgItem, int IdStaticDlgItem,
                        int CvtErrType, double MinVal, double MaxVal)
{
    BOOL bOk;
    char szErrFormat[256];
    char szError[256];
    char szName[80];

    HINSTANCE hInst = GetWindowInstance(hDlg);
    GetDlgItemText(hDlg, IdStaticDlgItem, szName, DIM(szName));
    DeleteChars(szName, "&\n");                     /* erase special chars */

    switch (CvtErrType)
    {
        case CVT_OK : return TRUE;  /* all ok */

        case CVT_ERR_OVERFLOW :
            LoadString(hInst, ERROR_CVT_OVERFLOW, szErrFormat, DIM(szErrFormat));
            sprintf(szError, szErrFormat, szName, MaxVal);
            break;

        case CVT_ERR_UNDERFLOW :
            LoadString(hInst, ERROR_CVT_UNDERFLOW, szErrFormat, DIM(szErrFormat));
            sprintf(szError, szErrFormat, szName, MinVal);
            break;

        case CVT_ERR_EMPTY :
            LoadString(hInst, ERROR_CVT_EMPTY, szErrFormat, DIM(szErrFormat));
            sprintf(szError, szErrFormat, szName);
            break;

        case CVT_ERR_SYNTAX :
            LoadString(hInst, ERROR_CVT_SYNTAX, szErrFormat, DIM(szErrFormat));
            sprintf(szError, szErrFormat, szName);
            break;
    } /* switch */


    bOk = IDOK == FDWINMESSAGEBOX(hDlg, szError, NULL,
                                  MB_OKCANCEL|MB_APPLMODAL|MB_ICONQUESTION);
    if (bOk) SetDlgFocus(hDlg, IdEditDlgItem);            /* set focus */
    else EndDialog(hDlg, FALSE);         /* if CANCEL button end of dialog */
    return bOk;
}


void SetDlgItemTextId(HWND hDlg, int nIddDlgItem, int nIddText)
{
    char szText[512];
    LoadString(GetWindowInstance(hDlg), nIddText, szText, DIM(szText));
    SetDlgItemText(hDlg, nIddDlgItem, szText);
}


int GetDlgItemFloat(HWND hDlg, int nIDDlgItem, double rmin, double rmax, double *val)
{
    char achval[40];
    char *pEndChar = achval;        /* initialize before calls strtod() ! */
    double dTmpVal;

    GetDlgItemText(hDlg, nIDDlgItem, achval, sizeof(achval));
    dTmpVal = strtod(achval, &pEndChar);

    /* check possible errors */
    if (HUGE_VAL == dTmpVal) return CVT_ERR_OVERFLOW;  /* ovf in strtod ? */
    if (-HUGE_VAL == dTmpVal) return CVT_ERR_UNDERFLOW;    /* underflow ? */

    if (isspace(*pEndChar) || (*pEndChar == '\0')) /* regulary end of num */
    {   
        if (pEndChar == achval) return CVT_ERR_EMPTY;    /* empty field ? */
        if (dTmpVal > rmax) return CVT_ERR_OVERFLOW;    /* out of range ? */
        if (dTmpVal < rmin) return CVT_ERR_UNDERFLOW;

        *val = dTmpVal;       
        return CVT_OK;
    } /* if */

    return CVT_ERR_SYNTAX;
} /* GetDlgItemFloat */


void SetDlgItemFloat(HWND hDlg, int nIdDlgItem, double val, int nPrec)
{
    char szVal[80];
    sprintf(szVal, FORMAT_DOUBLE, nPrec, val);
    SetDlgItemText(hDlg, nIdDlgItem, szVal);
} /* SetDlgItemFloat() */



void SetDlgItemFloatUnit(HWND hDlg, int nIdDlgItem, double val, int nPrec,
                         int iUnit, tUnitDesc aUnit[])
{
    char szVal[80];
    double dUnitFac = 1.0;
    if (aUnit != NULL) dUnitFac = aUnit[iUnit].dUnitFactor;
    sprintf(szVal, FORMAT_DOUBLE, nPrec, val/dUnitFac);
    SetDlgItemText(hDlg, nIdDlgItem, szVal);
}



static int GetDlgItemIntEx(HWND hDlg, int IdEditDlgItem, int rmin, int rmax,
                           int *val)
{
    char achval[40], *pEndChar;
    long lVal;
    GetDlgItemText(hDlg, IdEditDlgItem, achval, sizeof(achval)); 
    lVal = strtol(achval, &pEndChar, 10);

    if (isspace(*pEndChar) || (*pEndChar == '\0')) /* regulary end of num */
    {   
        if (pEndChar == achval) return CVT_ERR_EMPTY;    /* empty field ? */
        if (lVal > rmax) return CVT_ERR_OVERFLOW;    /* out of range ? */
        if (lVal < rmin) return CVT_ERR_UNDERFLOW;

        *val = (int)lVal;       
        return CVT_OK;
    } /* if */

    return CVT_ERR_SYNTAX;
} /* GetDlgItemIntEx */



/* returns TRUE, if input is ok
 * returns FALSE, if number syntax isn't correct or number too small/big
 * IdStaticDlgItem contains the title of checked edit window
 * returns FALSE :
     1. if the user finished the dialog because he did'nt want to
        correct an input error (the function calls EndDialog) themself
     2. if the user input is'nt correct but the user wish to correct that
*/
BOOL CheckDlgItemInt(HWND hDlg, int IdEditDlgItem, int IdStaticDlgItem,
                     int rmin, int rmax, int *val)
{
    int CvtErr = GetDlgItemIntEx(hDlg, IdEditDlgItem, rmin, rmax, val);
    if (CVT_OK == CvtErr) return TRUE;

    (void)HandleDlgNumInpErr(hDlg, IdEditDlgItem, IdStaticDlgItem, CvtErr,
                             rmin, rmax);
    return FALSE;
} /* CheckDlgItemInt */



/* handles EN_MOUSE_DEC and EN_MOUSE_INC notification messages at incremental
   edit fields for integer numbers
 * returns TRUE if message processed else FALSE
 */
BOOL ChangeIncDecInt(HWND h, WPARAM wParam, LPARAM lParam,
                     int nMin, int nMax, int nDefault)
{
    if ((GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_MOUSE_DEC)
        || (GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_MOUSE_INC))
    {
        int nVal;
                       /* wParam contains the ID of dialog element */
        switch (GetDlgItemIntEx(h, wParam, nMin, nMax, &nVal))
        {
            case CVT_ERR_EMPTY : nVal = nDefault; break;

            case CVT_ERR_OVERFLOW : nVal = nMax; break;

            case CVT_ERR_UNDERFLOW : nVal = nMin; break;

            case CVT_ERR_SYNTAX : return TRUE; /* do nothing */

            case CVT_OK :

                switch (GET_WM_COMMAND_NOTIFY(wParam, lParam))
                {
                    case EN_MOUSE_DEC:
                        if (--nVal < nMin) return TRUE;
                        break;

                    case EN_MOUSE_INC:
                        if (++nVal > nMax) return TRUE;
                        break; 
                } /* switch */

                break; /* CVT_OK */
        } /* switch */

        SetDlgItemInt(h, wParam, nVal, TRUE);
        return TRUE;
    } /* if */

    return FALSE;
} /* ChangeIncDecInt() */



/* returns TRUE, if input is ok
 * returns FALSE, if number syntax isn't correct or number too small/big
 * IdStaticDlgItem contains the title of checked edit window
 */
BOOL CheckDlgItemFloat(HWND hDlg, int IdEditDlgItem, int IdStaticDlgItem,
                       double MinVal, double MaxVal, double *DestVar)
{
    int CvtErr = GetDlgItemFloat(hDlg, IdEditDlgItem, MinVal, MaxVal, DestVar);
    if (CVT_OK == CvtErr) return TRUE;

    (void)HandleDlgNumInpErr(hDlg, IdEditDlgItem, IdStaticDlgItem, CvtErr,
                             MinVal, MaxVal);
    return FALSE;
} /* CheckDlgItemFloat */




void EnableDlgItem(HWND hwndParent, int nIddItem, BOOL bEnable)
{
#if GENBORSTYLE
    char szDlgCtrlClass[128];
    (void)GetClassName(GetDlgItem(hwndParent, nIddItem), szDlgCtrlClass,
                       DIM(szDlgCtrlClass));
    if (!lstrcmpi("static", szDlgCtrlClass)) return; /* do not disable static text in Borland's dialog*/
#endif
    EnableWindow(GetDlgItem(hwndParent, nIddItem), bEnable);
}


/* returns the current selection from a list or combo box including
   the well known error codes (LB_ERR, CB_ERR etc.)
 * the passed Id of the dialog item must identify a combo or list box,
   else the proper function cannot be garantued
 */
int GetDlgListCurSel(HWND hDlg, int nIddComboList)
{
    char szClass[128];
    UINT msg = LB_GETCURSEL;

    GetClassName(GetDlgItem(hDlg, nIddComboList), szClass, DIM(szClass));
    if (!lstrcmpi("COMBOBOX", szClass)) msg = CB_GETCURSEL;

    return (int)SendDlgItemMessage(hDlg, nIddComboList, msg,
                              DUMMY_WPARAM, DUMMY_LPARAM);
} /* GetDlgListCurSel */


/* same function as Windows 'SetFocus' but only for dialog windows with
   controls like buttons, edit fields etc.
 * SetFocus don't sets the default button frame in dialog windows 
 */
void SetDlgFocus(HWND hDlg, int nIdd)
{
    PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, nIdd),TRUE);
} /* SetDlgFocus */


/* sends multiple messages from the passed array */
void MultipleSendDlgItemMessage(HWND hwndDialog, int nIDDlgItem, int nNoOfMsg,
                                tSendItemMsgRec *vec)
{
    int i;
    for (i=0; i<nNoOfMsg; i++, vec++)
        SendDlgItemMessage(hwndDialog, nIDDlgItem, vec->wMsg, vec->wParamMsg,
                           vec->lParamMsg);
} /* MultipleSendDlgItemMessage */


/*  fill the passed listbox with the values passed in the double vector 
 *  nOrder is the number of elements in the vector - 1                   
 *  returns FALSE if there are any memory problems
 */
BOOL DrawListFloat(HWND hDlg, int nIdListItem, int nOrder, double *adVec,
                   int nPrec)
{
    int i;
    char achCoeff[80];
    static tSendItemMsgRec PrepareFillDat[3] = {
        {WM_SETREDRAW, FALSE, 0L},         /* do not redraw the list */
        {LB_RESETCONTENT, 0, 0L}, /* clear the complete list */
        {WM_SETREDRAW, TRUE, 0L}           /* enable redrawing */
    };

    MultipleSendDlgItemMessage(hDlg, nIdListItem,
                               sizeof(PrepareFillDat)/sizeof(PrepareFillDat[0]),
                               PrepareFillDat);

    for(i = 0; i <= nOrder; i++, adVec++)        /* for every coefficient */
    {
        sprintf(achCoeff, FORMAT_DOUBLE_LIST, i, nPrec, *adVec);
        if (SendDlgItemMessage(hDlg, nIdListItem, LB_ADDSTRING,/* add to list */
                               0, (LPARAM)(LPSTR)achCoeff) < LB_OKAY)
        {
            ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
            return FALSE;                           /* only if memory full */
        } /* if */
    } /* for */

    return TRUE;                                    /* all ok */
} /* DrawListFloat */


BOOL DrawBoxIdStrings(HWND hDlg, int nIdListItem, int iCurr, int nNo, int *IdVec)
{
    int i;
    char szStr[256];

    /* attention : do not use WM_SETREDRAW to combo boxes, because the edit
                   field wouldn't repaint at CB_RESETCONTENT
     */
    SendDlgItemMessage(hDlg, nIdListItem, CB_RESETCONTENT, DUMMY_WPARAM, DUMMY_LPARAM);

    for(i = 0; i < nNo; i++, IdVec++)        /* for every string id */
    {
        LoadString(GetWindowInstance(hDlg), *IdVec, szStr, DIM(szStr));
        if (SendDlgItemMessage(hDlg, nIdListItem, CB_ADDSTRING,/* add to list */
                               0, (LPARAM)(LPSTR)szStr) < 0)
        {
            ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
            return FALSE;                           /* only if memory full */
        } /* if */
    } /* for */

    SendDlgItemMessage(hDlg, nIdListItem, CB_SETCURSEL, iCurr, 0L);
    return TRUE;                                    /* all ok */
}


BOOL DrawComboAxisUnits(HWND hDlg, int nIddUnitBox, int nIddUnitText,
                        int iCurrSel, tUnitDesc *pUnit)
{
    if (nIddUnitText != 0)
        EnableDlgItem(hDlg, nIddUnitText, pUnit != NULL);

    (void)SendDlgItemMessage(hDlg, nIddUnitBox, CB_RESETCONTENT, 0, 0L);

    if (pUnit != NULL)                        /* is a unit list defined ? */
    {
        EnableDlgItem(hDlg, nIddUnitBox, TRUE);

        do
        {
            if (SendDlgItemMessage(hDlg, nIddUnitBox, CB_ADDSTRING, 0,
                                   (LPARAM)pUnit->szUnit) < (LRESULT)0)
            {
                ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
                return FALSE;
            } /* if */

        } /* do */
        while ((++pUnit)->szUnit != NULL);           /* up to end of list */

        SendDlgItemMessage(hDlg, nIddUnitBox, CB_SETCURSEL, iCurrSel, DUMMY_LPARAM);
    } /* if */
    else EnableDlgItem(hDlg, nIddUnitBox, FALSE);

    return TRUE;
} /* DrawComboAxisUnits */



/* radio button management */

int GetSelRadioBtn(HWND hDlg, tRadioBtnDesc aBtnDesc[])
{
    tRadioBtnDesc *pDesc = aBtnDesc;
    int IdSelBtn = pDesc->nIDDRadio;

    do                            /* at first search checked radio button */
    {
        IdSelBtn = GetDlgCtrlID(GetNextDlgGroupItem(hDlg, GetDlgItem(hDlg, IdSelBtn),
                                                    TRUE));
        if (IsDlgButtonChecked(hDlg, IdSelBtn)) break;
    }
    while (IdSelBtn != pDesc->nIDDRadio);                   /* circular ? */


    do
    
        if (pDesc->nIDDRadio == IdSelBtn) return pDesc->nSel;
    
    while ((++pDesc)->nIDDRadio != 0);

    return aBtnDesc[0].nSel;
} /* GetSelRadioBtn */


void CheckSelRadioBtn(HWND hDlg, int nCurrSel, tRadioBtnDesc aBtnDesc[])
{
    tRadioBtnDesc *pDesc = aBtnDesc;
    tRadioBtnDesc *pChkDesc = aBtnDesc;

    do       /* search the array index of button description (id and sel) */
    
        if (pDesc->nSel == nCurrSel) pChkDesc = pDesc;   /* save if found */
    
    while ((++pDesc)->nIDDRadio != 0);

    CheckRadioButton(hDlg, aBtnDesc[0].nIDDRadio, (--pDesc)->nIDDRadio,
                     pChkDesc->nIDDRadio);
} /* CheckSelRadioBtn */


static BOOL ToggleMenuCheckmark(HMENU menu, int nIdMenu)
/* returns TRUE if the checkmark is now visible */
{
    int nNewState = MF_UNCHECKED;
    if (!(GetMenuState(menu, nIdMenu, MF_BYCOMMAND) & MF_CHECKED))
        nNewState = MF_CHECKED;
    CheckMenuItem(menu, nIdMenu, nNewState);
    return (nNewState == MF_CHECKED);
}


void EnableMenuItems(HMENU hm, int num, int aId[], BOOL bEnable)
{
    int i;
    UINT mf = MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED);

    for (i=0; i<num; i++) EnableMenuItem(hm, aId[i], mf);
} /* EnableMenuItems() */


void EnableMainMenu(HWND hwndMain, BOOL bEnable)
{
    HMENU hm = GetMenu(hwndMain);
    int i, num = GetMenuItemCount(hm);
    UINT mf = MF_BYPOSITION | (bEnable ? MF_ENABLED : MF_GRAYED);

    for (i=0; i<num-1; i++)  /* disable main menu, except menuitem "help" */
        EnableMenuItem(hm, i, mf);
    DrawMenuBar(hwndMain);
} /* EnableMainMenu() */



/* works like ShowWindow(), but requires passing of ID but handle */
void ShowDlgItem(HWND hDlg, int nIDDlgItem, BOOL bShow)
{
    ShowWindow(GetDlgItem(hDlg, nIDDlgItem), bShow ? SW_SHOW : SW_HIDE);
} /* ShowDlgItem() */


void SetWindowTextId(HWND hwnd, int IdStr)
{
    char szText[128];
    LoadString(GetWindowInstance(hwnd), IdStr, szText, DIM(szText));
    SetWindowText(hwnd, szText);
}


BOOL UserOkQuestion(HWND hwndParent, int nMsgNo)
{
    char acMsg[256];                /* message from resources file */
    char szTitle[128];              /* title of question window */

    LoadString(GetWindowInstance(hwndParent), nMsgNo, acMsg, DIM(acMsg));
    LoadString(GetWindowInstance(hwndParent), STRING_TITLE_QUESTION, szTitle, DIM(szTitle));
    return (IDOK == FDWINMESSAGEBOX(hwndParent, acMsg, szTitle,
                                    MB_OKCANCEL|MB_APPLMODAL|MB_ICONEXCLAMATION));
} /* UserOkQuestion */



void MessageAckUsr(HWND hwndParent, int nIDDMsg)
{
    char achTitel[256], achMsg[256];
    HINSTANCE hInstThisAppl = GetWindowInstance(hwndParent);
    LoadString(hInstThisAppl, STRING_MESSAGE_TITLE, achTitel, sizeof(achTitel));
    LoadString(hInstThisAppl, nIDDMsg, achMsg, sizeof(achMsg));
    FDWINMESSAGEBOX(HWND_NULL, achMsg, achTitel, MB_OK|MB_TASKMODAL|MB_ICONINFORMATION);
}


static UINT WorkMsgIdStack[32];
static int iWaitMsg = 0;


/* displays a status message at bottom desktop line bypassing
   the working message stack
 */
void StatusMessage(int nIddStr)
{
    if (nIddStr != 0)
    {
        char szMsg[256] = {'\0'};

        LoadString(GetWindowInstance(hwndStatus), nIddStr, szMsg, DIM(szMsg));
        SetWindowText(hwndStatus, szMsg);
    } /* if */
    else
        if (iWaitMsg > 0)
            StatusMessage(WorkMsgIdStack[iWaitMsg-1]);     /* reentrant ! */
        else SetWindowText(hwndStatus, "");    /* nothing on stack, clear */

} /* StatusMessage */



/* if nIddStr == 0 mean that's the end of the working time */
void WorkingMessage(int nIddStr)
{
    static HCURSOR hCursor = (HCURSOR)0;

    if (nIddStr == 0)                                   /* end of waiting */
    {
        if (iWaitMsg > 0)
        {
            if (--iWaitMsg == 0)                         /* stack empty ? */
            {
                SetWindowText(hwndStatus, "");
                if (hCursor != (HCURSOR)0) SetCursor(hCursor);
            } /* if */
            else
                StatusMessage(WorkMsgIdStack[iWaitMsg-1]); /* pop last msg */
        } /* if */
    } /* if */
    else                                      /* start of waiting/working */
    {
        if (iWaitMsg < DIM(WorkMsgIdStack))      /* too many recursions ? */
        {
            if (iWaitMsg == 0)
                hCursor = SetCursor(LoadCursor((HCURSOR)0, IDC_WAIT));

            WorkMsgIdStack[iWaitMsg++] = nIddStr;       /* save new ident */
            StatusMessage(nIddStr);
        } /* if */
    } /* else */
} /* WorkingMessage() */



void FDWinMsgHandler(LPMSG pMsg)
{
    if (TranslateAccelerator(hwndFDesk, haccMainMenu, pMsg)) return;

    if (hwndFilterCoeff != HWND_NULL)    /* if 0 than window is'nt open */
       if (IsDialogMessage(hwndFilterCoeff, pMsg)) return;
    if (hwndFilterRoots != HWND_NULL)    /* if 0 than window is'nt open */
       if (IsDialogMessage(hwndFilterRoots, pMsg)) return;
    if (!TranslateMDISysAccel(hwndDiagBox, pMsg))
    {
        TranslateMessage(pMsg);
        DispatchMessage(pMsg);
    } /* if */
}


/* Checks Strg+Break key status (independent from active task or focus) */

BOOL UserBreak(void)
{
    MSG msg;


    if (GetAsyncKeyState(VK_CANCEL))                 /* focus independent */
    {                                          /* kill all own key events */
        while (PeekMessage(&msg, HWND_NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE|PM_NOYIELD)) ;
        return TRUE;
    } /* if */


    if (PeekMessage(&msg, HWND_NULL, 0, 0, PM_REMOVE))
        if (msg.message != WM_QUIT) FDWinMsgHandler(&msg);

    return FALSE; /* no user break */
} /* UserBreak() */



/* - abort dialog functions
 * - next functions build the abort dialog sandwich :
 *    ====== InitAbortDlg
 *    ------ ChkAbort  or ChangeAbortString
 *    ------ ChkAbort
 *    ====== EndAbortDlg
 */

static BOOL bAborted = FALSE;
static DLGPROC lpitAbortDlg;                            /* instance thunk */
static HWND hwndAbortDlg;
static UINT IdComment;




void ChangeAbortString(LPSTR szNewAbortStr)
{
    SetDlgItemText(hwndAbortDlg, IDD_ABORT_COMMENT, szNewAbortStr);
} /* ChangeAbortString(() */



#pragma argsused  /* disable "Parameter is never used" Warning */
BOOL CALLBACK AbortDlgProc(HWND hwndBreakDlg, UINT msg, WPARAM wParam,
                           LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG :
            if (IdComment != 0)
            {
                char szComment[256];
                LoadString(GetWindowInstance(hwndBreakDlg), IdComment, szComment, DIM(szComment));
                SetDlgItemText(hwndBreakDlg, IDD_ABORT_COMMENT, szComment);
            } /* if */
            return TRUE;

        case WM_COMMAND :
            if (wParam == IDCANCEL)
            {
                bAborted = TRUE;
                return TRUE;
            } /* if */
    } /* switch */

    return FALSE;
}


void InitAbortDlg(HWND hwnd, UINT IdStr)
{
    HINSTANCE hInstThisAppl = GetWindowInstance(hwnd);

    EnableMainMenu(hwndFDesk, FALSE); /* not neccessary in modeless dialog */

    IdComment = IdStr;
    bAborted = FALSE;
    lpitAbortDlg = (DLGPROC)MakeProcInstance((FARPROC)AbortDlgProc, hInstThisAppl);
    hwndAbortDlg = CreateDialog(hInstThisAppl, MAKEINTRESOURCE(ABORTDLG),
                                hwnd, lpitAbortDlg);
    EnableWindow(hwnd, FALSE);             /* disable all mouse/key events */
    if (hwndFilterCoeff != HWND_NULL)        /* if 0 than window is'nt open */
        EnableWindow(hwndFilterCoeff, FALSE);
    if (hwndFilterRoots != HWND_NULL)        /* if 0 than window is'nt open */
        EnableWindow(hwndFilterRoots, FALSE);
}



/* returns TRUE if user has aborted the action */
BOOL ChkAbort()
{
    MSG msg;

    if (PeekMessage(&msg, HWND_NULL, 0, 0, PM_REMOVE))
        if (!IsDialogMessage(hwndAbortDlg, &msg)) FDWinMsgHandler(&msg);

    return bAborted;
} /* ChkAbort() */



void EndAbortDlg()
{
    if (hwndFilterCoeff != HWND_NULL)    /* if 0 than window is'nt open */
        EnableWindow(hwndFilterCoeff, TRUE);
    if (hwndFilterRoots != HWND_NULL)    /* if 0 than window is'nt open */
        EnableWindow(hwndFilterRoots, TRUE);
    EnableWindow(GetParent(hwndAbortDlg), TRUE);     /* before destroy */
    DestroyWindow(hwndAbortDlg);
    FreeProcInstance((FARPROC)lpitAbortDlg);
    EnableMainMenu(hwndFDesk, TRUE);
}

/* end of abort dialog functions */



void SwapDouble(double *lpPar1, double *lpPar2)
{
    double dHelp = *lpPar1;
    *lpPar1 = *lpPar2;
    *lpPar2 = dHelp;
}



/* coordinate transformation */
void SetCursorPosClient(HWND hw, int x, int y)
{
    POINT pt;

    pt.x = x; pt.y = y;
    ClientToScreen(hw, &pt);
    SetCursorPos(pt.x, pt.y);
} /* SetCursorPosClient */


void ClientToScreenRect(HWND hwndClient, RECT *lprect)
{
    POINT pt1, pt2;
    pt1.x = lprect->left; pt1.y = lprect->top;
    pt2.x = lprect->right; pt2.y = lprect->bottom;
    ClientToScreen(hwndClient, &pt1);
    ClientToScreen(hwndClient, &pt2);
    lprect->left = pt1.x; lprect->top = pt1.y;
    lprect->right = pt2.x; lprect->bottom = pt2.y;
}


void ScreenToClientRect(HWND hwndClient, RECT *lprect)
{
    POINT pt1, pt2;
    pt1.x = lprect->left; pt1.y = lprect->top;
    pt2.x = lprect->right; pt2.y = lprect->bottom;
    ScreenToClient(hwndClient, &pt1);
    ScreenToClient(hwndClient, &pt2);
    lprect->left = pt1.x; lprect->top = pt1.y;
    lprect->right = pt2.x; lprect->bottom = pt2.y;
}



/**************** file I/O dialog functions */

/* change directory/path to passed parameter if possible and display
   new directory and file list
 * returns FALSE if error
 */
static BOOL DisplayDirList(HWND hDlg, char *szPath)
{
    while (!DlgDirList(hDlg, szPath, IDD_FILEDLG_DIRS, IDD_FILEDLG_PATH,
                       DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE))
    {          /* if drive not ready or path not found */
        char szErrStr[256];

        LoadString(GetWindowInstance(hDlg), ERROR_CHANGE_PATHSPEC, szErrStr,
                   DIM(szErrStr));
        if (IDCANCEL == FDWINMESSAGEBOX(hDlg, szErrStr, NULL,
                                        MB_RETRYCANCEL|MB_APPLMODAL|MB_ICONSTOP))
            return FALSE;  /* list not displayed */
    }; /* while */ /* next try */

    SetDlgItemText(hDlg, IDD_FILEDLG_FNAME, szPath); /* show extracted file name */
    (void)DlgDirList(hDlg, szPath, IDD_FILEDLG_FILES, IDD_FILEDLG_PATH,
                     DDL_READWRITE);
    return TRUE;
} /* DisplayDirList */


/* szDir contains the dev\dir specification
 * the function gets the current filename from edit field 'IDD_FILEDLG_FNAME'
   and cats it in a correct manner to the path spec in 'szDir'
 */
static void AssembleFullFilename(HWND hDlg, char *szDir)
{
    char *pLast = &szDir[lstrlen(szDir)];

    if (pLast != szDir)                          /* means string length 0 */
        if (*AnsiPrev(szDir, pLast) != ':')
            if (*AnsiPrev(szDir, pLast) != '\\')
            {
                *pLast++ = '\\'; *pLast = '\0';          /* cat backslash */
            } /* if */ 

    GetDlgItemText(hDlg, IDD_FILEDLG_FNAME,/* assemble full path/filename */
                   pLast, MAXFILE+MAXEXT);
} /* AssembleFullFilename */


/* returns selected file name (including path) to the passed OFSTRUCT
   variable (pointed out by lParam)
 * the component nErrCode should contain the OpenFile style code
 * returns with FALSE if user cancels operation
 */

BOOL CALLBACK OpenFileDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static char szDefaultDir[MAXPATH] = "";/* only dir/path (no filename) */
    static UINT nPrevErrMode;
    static LPOFSTRUCT pOf;
    static UINT OpenStyle;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            char szPath[MAXPATH];

            nPrevErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

            (void)SendDlgItemMessage(hDlg, IDD_FILEDLG_FNAME, EM_LIMITTEXT,
                                     MAXPATH, DUMMY_LPARAM);
            pOf = (LPOFSTRUCT)lParam;
            OpenStyle = pOf->nErrCode;        /* save static */

            SetDlgItemText(hDlg, IDD_FILEDLG_FNAME, "*." FILTER_FILE_EXT);
            lstrcpy(szPath, szDefaultDir); 
            AssembleFullFilename(hDlg, szPath); /* cat extension wildcard */

            if (!DisplayDirList(hDlg, szPath))  /* change to default path */
            {
                GetWindowsDirectory(szPath, MAXPATH);
                (void)DisplayDirList(hDlg, szPath);
            } /* if */

            return TRUE;
        } /* WM_INITDIALOG */


	    case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_FILEDLG_DIRS :                /* change directory */
                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == LBN_DBLCLK)
                    {
                        char szSelDir[MAXFILE+MAXEXT];
                        char szPath[MAXPATH];

                        GetDlgItemText(hDlg, IDD_FILEDLG_PATH, szPath, MAXPATH);

                        szSelDir[0] = '\0';
                        DlgDirSelect(hDlg, szSelDir, IDD_FILEDLG_DIRS);    /* get current dir */
                        if (*AnsiPrev(szSelDir, &szSelDir[lstrlen(szSelDir)]) == ':')
                        {                                                      /* drive ? yes */
                            (void)lstrcpy(szPath, szSelDir);  /* copy drive at first */
                            szSelDir[0] = '\0'; /* and set to current directory at this drive */
                        } /* if */

                        if (*AnsiPrev(szPath, &szPath[lstrlen(szPath)]) != '\\')
                            (void)lstrcat(szPath, "\\");

                        (void)lstrcat(szPath, szSelDir);     /* cat the selected dir */
                        AssembleFullFilename(hDlg, szPath);
                        (void)DisplayDirList(hDlg, szPath);/* if dir access possible */
                        return TRUE;
                    } /* if */

                    break; /* IDD_FILEDLG_DIRS */


                case IDD_FILEDLG_FILES :
                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == LBN_SELCHANGE)
                    {
                        char szFname[MAXFILE+MAXEXT];
                        szFname[0] = '\0';
                        DlgDirSelect(hDlg, szFname, IDD_FILEDLG_FILES);
                        if (lstrlen(szFname) > 0)
                            SetDlgItemText(hDlg, IDD_FILEDLG_FNAME, szFname);
                        return TRUE;
                    } /* if */

                    break; /* IDD_FILEDLG_FILES */


                case IDOK :
                {
                    int hf;  /* file handle */

                    GetDlgItemText(hDlg, IDD_FILEDLG_PATH, pOf->szPathName,   
                                   MAXPATH);                   /* get dir */
                    AssembleFullFilename(hDlg, pOf->szPathName); /* dir\file */

                    if ((hf=OpenFile(pOf->szPathName, pOf, OpenStyle)) != HFILE_ERROR)
                    {                             /* open file possible ? */
                        GetDlgItemText(hDlg, IDD_FILEDLG_PATH, szDefaultDir,       /* save dir */
                                       MAXPATH);
                        CLOSE(hf);
                        SetErrorMode(nPrevErrMode);
                        EndDialog(hDlg, TRUE); /* file name in pOf->szPathName */
                        return TRUE;
                    } /* if */

                    if (GetFocus() == GetDlgItem(hDlg, IDD_FILEDLG_DIRS)) /* ENTER in file tree */
                    {
                        pOf->szPathName[0] = '\0';
                        if (!DlgDirSelect(hDlg, pOf->szPathName, IDD_FILEDLG_DIRS)) /* no selection ? */
                        {
                            SendDlgItemMessage(hDlg, IDD_FILEDLG_DIRS, LB_SETCURSEL, 0, DUMMY_LPARAM);
                            (void)DlgDirSelect(hDlg, pOf->szPathName, IDD_FILEDLG_DIRS);
                        } /* if */

                        PostMessage(hDlg, WM_COMMAND, IDD_FILEDLG_DIRS,
                                    MAKELPARAM(0, LBN_DBLCLK));
                        return TRUE;      /* change dir via return key */
                    } /* if */

                    GetDlgItemText(hDlg, IDD_FILEDLG_FNAME, pOf->szPathName, MAXPATH);
                    (void)DisplayDirList(hDlg, pOf->szPathName);
                    return TRUE;
                } /* IDOK */

                case IDCANCEL :
                    SetErrorMode(nPrevErrMode);
                    EndDialog(hDlg, FALSE);
                    return TRUE;

                case IDHELP :
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpFileDlg);
                    break;
            } /* switch */
    } /* switch */

    return FALSE;
} /* OpenFileDlgProc() */



/************* memory management functions */


/* malloc with error handling */
void *MallocErr(HWND hwndParent, size_t size)
{
    void *p = MALLOC(size);
    if (p == NULL) ErrorAckUsr(hwndParent, IDSTROUTOFMEMORY);
    return p;
} /* MallocErr() */




/* low level */
static BOOL AllocMem(GLOBALHANDLE *, void **, size_t);
static BOOL ReAllocMem(GLOBALHANDLE *, void **, size_t);


typedef struct {
                  void *pMem;    /* if NULL this field is empty */
                  GLOBALHANDLE hMem;
               } tAllocStruct;

static tAllocStruct *AllocTab;   /* pointer to alloc tab */
static int nDimAllocTab = 0;    /* current number of fields in alloc tab */

#define MEM_FLAGS       GMEM_FIXED //GMEM_MOVEABLE


static BOOL ReAllocMem(GLOBALHANDLE *hMem, void **pMem, size_t size)
{
    *hMem = GlobalReAlloc(*hMem, size, MEM_FLAGS);
    if (*hMem != (GLOBALHANDLE)0)
    {
        *pMem = (void *)GlobalLock(*hMem);        /* lock the memory block */
        if (*pMem == NULL)                             /* is'nt possible ? */
        {
            *hMem = (GLOBALHANDLE)0;
            (void)GlobalFree(*hMem);
        } /* if */
    } /* if */

    if (*hMem == (GLOBALHANDLE)0)                                  /* error */
    {
        *pMem = NULL;
        return FALSE;
    } /* if */
    return TRUE;
}


static BOOL AllocMem(GLOBALHANDLE *hMem, void **pMem, size_t size)
{
    *hMem = GlobalAlloc(MEM_FLAGS, size);
    if (*hMem != (GLOBALHANDLE)0)
    {
        *pMem = (void *)GlobalLock(*hMem);       /* lock the memory block */
        if (*pMem == NULL)                            /* is'nt possible ? */
        {
            *hMem = (GLOBALHANDLE)0;
            (void)GlobalFree(*hMem);
        } /* if */
    } /* if */

    if (*hMem == (GLOBALHANDLE)0)                                /* error */
    {
        *pMem = NULL;
        return FALSE;
    } /* if */
    return TRUE;
}


/* The DFCGEN malloc function
 */
void *fd_malloc(size_t size)
{
    static GLOBALHANDLE hAllocTab;
    register int i;
    for (i=0; i<nDimAllocTab; i++)
        if (AllocTab[i].pMem == NULL) break;              /* empty field ? */

    if (i >= nDimAllocTab)              /* if no empty field in alloc tab */
    {
        if (nDimAllocTab == 0)                       /* alloc first field */
        {
            if (!AllocMem(&hAllocTab, (void *)&AllocTab, sizeof(tAllocStruct)))
                return NULL;
        } /* if */
        else
            if (!ReAllocMem(&hAllocTab, (void *)&AllocTab, (nDimAllocTab+1) *
                            sizeof(tAllocStruct))) return NULL;
        i = nDimAllocTab++;
    } /* if */

    if (!AllocMem(&AllocTab[i].hMem, &AllocTab[i].pMem, size)) return NULL;
    return AllocTab[i].pMem;                                    /* pointer */
}


void fd_free(void *block)
{
    register int i;

    for (i=0; i<nDimAllocTab; i++)
        if (AllocTab[i].pMem == block) break;

    if (i < nDimAllocTab)
    {
        (void) GlobalUnlock(AllocTab[i].hMem);
        (void) GlobalFree(AllocTab[i].hMem);
        AllocTab[i].pMem = NULL;                  /* set to free state */
    } /* if */
}


void *fd_realloc (void *block, size_t size)
{
    register int i;

    for (i=0; i<nDimAllocTab; i++)
        if (AllocTab[i].pMem == block) break;

    if (i >= nDimAllocTab) return NULL;         /* memory block not found */

    if (!ReAllocMem(&AllocTab[i].hMem, &AllocTab[i].pMem, size)) return NULL;
    return AllocTab[i].pMem;
} /* fd_realloc() */


