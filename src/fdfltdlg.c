/* DFCGEN Filter Definition Dialogs
 * Lin FIR, Standard IIR, Misc FIR, Misc IIR, predefined FIR/IIR
 * data window and frequency transform dialogs

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/fdfltdlg.c,v $
 * $Revision: 1.3 $
 * $Date: 2001-10-05 19:19:40 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $
   Revision 1.2  2000/08/17 12:45:19  ralf
   DFCGEN now is shareware (no registration or license dialogs).
   Directory with examples added.


 */

#include "dfcwin.h"

/* prototypes of local functions */

static void DisplayPredefDesc(HINSTANCE hInst, HWND hDlg);
static void EnableFTrDlgItems(HWND, tTransform);
static void ModifyLPCutoffLinFIR(HWND, tFilter *Filt);
static void ModifyLPCutoffStdIIR(HWND, tFilter *Filt);
static void ShowCurrFTransfInDlg(HWND, tTransform);
static void ShowLinFIR_DataWin_FTransf(HWND, tFilter *);
static BOOL CommonParametersOk(HWND, tFilter *);
static void EnableFltParamDlgItems(HWND, tStdIIR *);
static void SetMiscSysDlgItems(HWND hDlg, tVarSys *pDigSys);
static void CallFTransfDialog(HWND hDlg, tTransformData *pFTr);

BOOL CALLBACK LinFirDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK StdIIRDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK PredefFilterDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK MiscDigSystemsDlgProc(HWND, UINT, WPARAM, LPARAM);



BOOL FilterDefinitionDlg(HWND hwndParent, tFltDlg DlgType, BOOL bNew)
{
    typedef struct
    {
        tFltDlg type;
        UINT DlgId;
        DLGPROC FltDlgProc;
    } const tDlgTab;

    static tDlgTab ProcTab[] =
    {
        {LINFIRDLG, IDD_LINFIRDLG, LinFirDlgProc},
        {STDIIRDLG, IDD_STDIIRDLG, StdIIRDlgProc},
        {MISCDLG, IDD_MISCFLTDLG, MiscDigSystemsDlgProc},
        {PREDEFDLG, IDD_PREDEFDLG, PredefFilterDlgProc}
    };

    int i;
    tDlgTab *pDlg;

    for (i = 0, pDlg = ProcTab; i < DIM(ProcTab); i++, pDlg++)
        if (pDlg->type == DlgType) break;

    return FDWinDialogBoxParam(pDlg->DlgId, hwndParent,
                               pDlg->FltDlgProc, (LPARAM)bNew);
} /* FilterDefinitionDlg */



/* impuls response windowing box
 * lParam is a ptr to the tLinFIR struct
 * fills the struct with selected response windowing type and alpha if Kaiser-
 * Window selected */
BOOL CALLBACK ImpWinDlgProc(HWND hwndDlgWin,
                            UINT msg,         /* message */
                            WPARAM wParam,
                            LPARAM lParam)
{
    static tLinFIR *lpPar;

    static tRadioBtnDesc aBtnWin[] = 
    {
        {RECT_WIN,     IDD_IMPWIN_NONWIN},
        {KAISER_WIN,   IDD_IMPWIN_KAISER},
        {HAMMING_WIN,  IDD_IMPWIN_HAMMING},
        {HANNING_WIN,  IDD_IMPWIN_HANNING},
        {BLACKMAN_WIN, IDD_IMPWIN_BLACKMAN},
        {0, 0}
    };


    switch (msg)
    {
           case WM_INITDIALOG :
           {
               lpPar = (tLinFIR *)lParam;

               CheckSelRadioBtn(hwndDlgWin, lpPar->DataWin, aBtnWin);
               EnableDlgItem(hwndDlgWin, IDD_KAISER_ALPHA, lpPar->DataWin == KAISER_WIN);
               EnableDlgItem(hwndDlgWin, IDD_KAISER_ALPHA_TEXT, lpPar->DataWin == KAISER_WIN);
               if (lpPar->DataWin == KAISER_WIN)
                   SetDlgItemFloat(hwndDlgWin, IDD_KAISER_ALPHA, lpPar->dAlpha,
                                   anPrec[IOUTPRECDLG_OTHER]);
               return TRUE;
           } /* case */

           case WM_COMMAND :
           {
               switch(GET_WM_COMMAND_IDCTL(wParam, lParam))
               {
                   case IDD_IMPWIN_KAISER :
                       EnableDlgItem(hwndDlgWin, IDD_KAISER_ALPHA, TRUE);
                       EnableDlgItem(hwndDlgWin, IDD_KAISER_ALPHA_TEXT, TRUE);
                       SetDlgFocus(hwndDlgWin, IDD_KAISER_ALPHA);
                       break; /* IDD_IMPWIN_KAISER */

                   case IDD_IMPWIN_NONWIN :
                   case IDD_IMPWIN_HAMMING :
                   case IDD_IMPWIN_HANNING :
                   case IDD_IMPWIN_BLACKMAN :
                       EnableDlgItem(hwndDlgWin, IDD_KAISER_ALPHA, FALSE);
                       EnableDlgItem(hwndDlgWin, IDD_KAISER_ALPHA_TEXT, FALSE);
                       break;

                    case IDOK :
                    {
                        if (IsDlgButtonChecked(hwndDlgWin, IDD_IMPWIN_KAISER))
                        {
                            if (!CheckDlgItemFloat(hwndDlgWin,
                                                   IDD_KAISER_ALPHA,
                                                   IDD_KAISER_ALPHA_TEXT,
                                                   KAISER_ALPHA_MIN,
                                                   KAISER_ALPHA_MAX,
                                                   &lpPar->dAlpha)) break;
                         } /* KAISER_WIN */

                         lpPar->DataWin = GetSelRadioBtn(hwndDlgWin, aBtnWin);
                         EndDialog(hwndDlgWin, TRUE);
                         break;
                    } /* case IDOK */

                    case IDCANCEL :
                         EndDialog(hwndDlgWin, FALSE);
                         break;

                    case IDHELP :
                        WinHelp(hwndDlgWin, HELP_FNAME, HELP_CONTEXT, HelpWindowFuncDlg);
                        break;

                    default :
                        return FALSE;
                } /* switch */

                break;
           } /* WM_COMMAND */

           default : return FALSE;
    }
    return TRUE;
}



static void EnableFTrDlgItems(HWND hwndDlg, tTransform TransformType)
{
    char szCutoffBwText[40];
    BOOL bECutoff = TRUE;
    BOOL bECenter = TRUE;
    BOOL bEUnit = TRUE;
    BOOL bECenterFlag = TRUE;

    LoadString(GetWindowInstance(hwndDlg), STRING_FTRANSFORM_DLG_BANDWIDTH,
               szCutoffBwText, DIM(szCutoffBwText));       /* default load */

    switch(TransformType)
    {
        case NOFTR:
            bECutoff = bECenter = bEUnit = bECenterFlag = FALSE;

        case HIGHPASS :
            bECenter = bECenterFlag = FALSE;
            LoadString(GetWindowInstance(hwndDlg), STRING_FTRANSFORM_DLG_CUTOFF,
                       szCutoffBwText, DIM(szCutoffBwText));

                                      /* and fall through */
        case BANDPASS :
        case BANDSTOP :
            EnableDlgItem(hwndDlg, IDD_TRANS_CUTOFFBW, bECutoff);
            EnableDlgItem(hwndDlg, IDD_TRANS_CENTER, bECenter);
            EnableDlgItem(hwndDlg, IDD_TRANS_CUTOFF_TEXT, bECutoff);
            EnableDlgItem(hwndDlg, IDD_TRANS_CENTER_TEXT, bECenter);
            EnableDlgItem(hwndDlg, IDD_TRANS_UNITBOX, bEUnit);
            EnableDlgItem(hwndDlg, IDD_TRANS_CENTER_GEOMETRIC, bECenterFlag);
            SetDlgItemText(hwndDlg, IDD_TRANS_CUTOFF_TEXT, szCutoffBwText);
    } /* switch */
}



/* frequency transformation dialog box
 * lParam is a ptr to the Transformdata struct
 * fills the tTransformData struct with selected transformation data
   do not change remaining filter structure
 */
BOOL CALLBACK FTransformDlgProc(HWND hwndDlg,
                                UINT msg,         /* message */
                                WPARAM wParam,
                                LPARAM lParam)
{
    static tRadioBtnDesc aBtnTr[] =
    {
        {NOFTR,     IDD_TRANS_NON},
        {HIGHPASS,  IDD_TRANS_HIGHPASS},
        {BANDPASS,  IDD_TRANS_BANDPASS},
        {BANDSTOP,  IDD_TRANS_NOTCH},
        {0, 0}
    };

    static tTransformData *lpPar;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            lpPar = (tTransformData  *)lParam;
            DrawComboAxisUnits(hwndDlg, IDD_TRANS_UNITBOX, IDD_TRANS_UNITTEXT,
                               lpPar->iDefaultUnit, FrequencyUnits);
            EnableFTrDlgItems(hwndDlg, lpPar->FTransf);
            CheckSelRadioBtn(hwndDlg, lpPar->FTransf, aBtnTr);

            switch(lpPar->FTransf)
            {
                case BANDPASS :
                case BANDSTOP :
                    CheckDlgButton(hwndDlg, IDD_TRANS_CENTER_GEOMETRIC,
                                   lpPar->uFlags & CENTER_GEOMETRIC);

                    SetDlgItemFloatUnit(hwndDlg, IDD_TRANS_CENTER,
                                        lpPar->dCenter, anPrec[IOUTPRECDLG_FREQU],
                                        lpPar->iDefaultUnit, FrequencyUnits);
                case HIGHPASS :
                    SetDlgItemFloatUnit(hwndDlg, IDD_TRANS_CUTOFFBW,
                                        lpPar->dCutFBw, anPrec[IOUTPRECDLG_FREQU],
                                        lpPar->iDefaultUnit, FrequencyUnits);
                    break;

                case NOFTR :
                    if (lpPar->dCutFBw != 0.0)           /* invalid flag */
                        SetDlgItemFloatUnit(hwndDlg, IDD_TRANS_CUTOFFBW,
                                            lpPar->dCutFBw, anPrec[IOUTPRECDLG_FREQU],
                                            lpPar->iDefaultUnit, FrequencyUnits);
                    break;
            } /* switch */

            return TRUE;
        } /* case */

        case WM_COMMAND :
        {
            switch(GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_TRANS_NON :
                case IDD_TRANS_HIGHPASS :
                case IDD_TRANS_BANDPASS :
                case IDD_TRANS_NOTCH :

                    EnableFTrDlgItems(hwndDlg, GetSelRadioBtn(hwndDlg, aBtnTr));
                    break;

                case IDOK :
                {
                    double dUnitFac;
                    double dMaxCutoffBw = MAX_F_INPUT;  /* only for highpass */
                    tTransformData TmpTr;


                    TmpTr.uFlags = 0;
                    TmpTr.FTransf = GetSelRadioBtn(hwndDlg, aBtnTr);

                    TmpTr.iDefaultUnit =
                        GetDlgListCurSel(hwndDlg, IDD_TRANS_UNITBOX);

                    dUnitFac = FrequencyUnits[TmpTr.iDefaultUnit].dUnitFactor;

                    switch(TmpTr.FTransf)
                    {
                        case BANDPASS :
                        case BANDSTOP :
                            if (!CheckDlgItemFloat(hwndDlg, IDD_TRANS_CENTER,
                                                   IDD_TRANS_CENTER_TEXT,
                                                   MIN_CENTER_INPUT/dUnitFac,
                                                   MAX_F_INPUT/dUnitFac,
                                                   &TmpTr.dCenter))
                                return TRUE;

                            UPDATEFLAG(TmpTr.uFlags, CENTER_GEOMETRIC,
                                       IsDlgButtonChecked(hwndDlg, IDD_TRANS_CENTER_GEOMETRIC));

                            TmpTr.dCenter *= dUnitFac;

                            /* if center frequency is geometric, the bandwidth B may be exceed
                               half of center frequency fc because fc^2 = fo^2 * fu^2
                               and fu = sqrt((B/2)^2 + fc^2) - B/2
                               and fo = sqrt((B/2)^2 + fc^2) + B/2, means the virtual linear
                               center frequency is sqrt((B/2)^2 + fc^2)
                             * if linear the expression fc = 1/2 (fo + fu) is valid, and with
                               B = fo - fu there is the following limitation
                             */
                            if (!(TmpTr.uFlags & CENTER_GEOMETRIC))
                                dMaxCutoffBw = TmpTr.dCenter * 2.0 - MIN_CUTOFF_INPUT;

                        case HIGHPASS :
                            if (!CheckDlgItemFloat(hwndDlg, IDD_TRANS_CUTOFFBW,
                                                   IDD_TRANS_CUTOFF_TEXT,
                                                   MIN_CUTOFF_INPUT/dUnitFac,
                                                   dMaxCutoffBw/dUnitFac,
                                                   &TmpTr.dCutFBw))
                                return TRUE;

                            TmpTr.dCutFBw *= dUnitFac;
                    } /* switch */

                    *lpPar = TmpTr;
                    EndDialog(hwndDlg, TRUE);
                    break;
                } /* case IDOK */

                case IDCANCEL :
                    EndDialog(hwndDlg, FALSE);
                    break;

                case IDHELP :
                    WinHelp(hwndDlg, HELP_FNAME, HELP_CONTEXT, HelpFrequTransfDlg);
                    break;

                default : return FALSE;
            } /* switch */

            break;
        } /* WM_COMMAND */

        default : return FALSE;
    }

    return TRUE;
} /* FTransformDlgProc() */



static void ModifyLPCutoffLinFIR(HWND hwndDlg, tFilter *Filt)
{
    tLinFIR *lpPar = &Filt->DefDlg.FIRDat; /* for faster access */

    switch(Filt->FTr.FTransf)
    {
        case BANDPASS :
            lpPar->dCutoff = Filt->FTr.dCutFBw*0.5;   /* half of bandwidth */
            break; /* BANDPASS */

        case BANDSTOP :
        case HIGHPASS :
        {
            #define M_INV (1.0-M_SQRT_2)

            switch(lpPar->SubType)
            {
                case RECT_LP:
                    lpPar->dCutoff = Filt->FTr.dCutFBw;
                    break;

                case GAUSS_LP :
                    lpPar->dCutoff = Filt->FTr.dCutFBw*M_SQRT_2*sqrt(-M_LN2/log(M_INV));
                    break;

                case COS_LP :
                    lpPar->dCutoff = M_PI_4*Filt->FTr.dCutFBw/acos(M_INV);
                    break;

                case COS2_LP :
                    lpPar->dCutoff = Filt->FTr.dCutFBw*acos(sqrt(M_SQRT_2))/acos(sqrt(M_INV));
                    break;

                case SQR_LP :
                    lpPar->dCutoff = Filt->FTr.dCutFBw*M_SQRT2*M_INV;
                    break;
            } /* switch */

            if (Filt->FTr.FTransf == BANDSTOP) lpPar->dCutoff *= 0.5;
        } /* case */
    } /* switch */

    if (Filt->FTr.FTransf != NOFTR)
        SetDlgItemFloatUnit(hwndDlg, IDD_DLG_CUTOFF,
                            lpPar->dCutoff, anPrec[IOUTPRECDLG_FREQU], Filt->iInputUnitF,
                            FrequencyUnits);
} /* ModifyLPCutoffLinFIR */


/* show modified lowpass cutoff frequency if any frequency transformation
   desired */
static void ModifyLPCutoffStdIIR(HWND hwndDlg, tFilter *Filt)
{
    tStdIIR *lpPar = &Filt->DefDlg.StdIIRDat; /* for faster access */

    switch(Filt->FTr.FTransf)
    {
        case BANDSTOP :
        case BANDPASS :
            /* if center frequency is arithmetic (average of cutoff frequency),
               the geometric center frequeny should be calculate (because
               LP->BP transformation is a geometric process) using the
               following expression : sqrt(fc^2 - (B/2)^2)
             */
            if (!(Filt->FTr.uFlags & CENTER_GEOMETRIC))
            {
                if (Filt->FTr.dCenter*Filt->FTr.dCenter > 0.25*Filt->FTr.dCutFBw*Filt->FTr.dCutFBw)
                    lpPar->dCutoff =
                        sqrt(Filt->FTr.dCenter*Filt->FTr.dCenter -
                             0.25*Filt->FTr.dCutFBw*Filt->FTr.dCutFBw);

                else        /* this case isn't possible because there are */
                {         /* additional checks in F-transformation dialog */
                    SetDlgItemText(hwndDlg, IDD_DLG_CUTOFF, "");
                    return;
                } /* else */
            } /* if */
            else lpPar->dCutoff = Filt->FTr.dCenter;

            SetDlgItemFloatUnit(hwndDlg, IDD_DLG_CUTOFF,
                                lpPar->dCutoff, anPrec[IOUTPRECDLG_FREQU],
                                Filt->iInputUnitF, FrequencyUnits);
            break; /* BANDPASS, BANDSTOP */

        case HIGHPASS :
            lpPar->dCutoff = Filt->FTr.dCutFBw; /* equal cutoff */
            SetDlgItemFloatUnit(hwndDlg, IDD_DLG_CUTOFF,
                                lpPar->dCutoff, anPrec[IOUTPRECDLG_FREQU],
                                Filt->iInputUnitF, FrequencyUnits);
            break;
    } /* switch */
} /* ModifyLPCutoffStdIIR() */


static void ShowCurrFTransfInDlg(HWND hwndDlg, tTransform SelTransf)
{
    static struct
    {
        tTransform Trans;
        UINT     nIddStr;
    } const aTrans[] =
    {
        {NOFTR,     STRING_NON_TRANSF},
        {HIGHPASS,  STRING_LPHP_TRANSF},
        {BANDPASS,  STRING_LPBP_TRANSF},
        {BANDSTOP,  STRING_LPBS_TRANSF}
    };

    int i;

    /* enable/disable cutoff user input field */
    EnableDlgItem(hwndDlg, IDD_DLG_CUTOFF, SelTransf == NOFTR);
    EnableDlgItem(hwndDlg, IDD_DLG_CUTOFF_TEXT, SelTransf == NOFTR);

    /* show the current frequency transformation */
    i = 0;
    while (aTrans[i].Trans != SelTransf) i++;
    SetDlgItemTextId(hwndDlg, IDD_DLG_TRANSF_TEXT, aTrans[i].nIddStr);
} /* ShowCurrFTransfInDlg */


static void ShowLinFIR_DataWin_FTransf(HWND hwndDlg, tFilter *lpFilter)
{
    static struct
    {
        tDataWin DataW;
        UINT     nIddStr;
    } const aWin[] =
    {
        {RECT_WIN,     STRING_NON_WIN},
        {HAMMING_WIN,  STRING_HAMMING_WIN},
        {HANNING_WIN,  STRING_HANNING_WIN},
        {BLACKMAN_WIN, STRING_BLACKMAN_WIN},
        {KAISER_WIN,   STRING_KAISER_WIN}
    };

    int i;

    ShowCurrFTransfInDlg(hwndDlg, lpFilter->FTr.FTransf);   /* handle current frequency transformation */

    /* show the current impuls response data window */
    i = 0;
    while (aWin[i].DataW != lpFilter->DefDlg.FIRDat.DataWin) i++;
    SetDlgItemTextId(hwndDlg, IDD_LINFIRDLG_WIN_TEXT, aWin[i].nIddStr);
}


/* checks sampling frequency and order */
static BOOL GetSamplesAndOrder(HWND hDlg, short *pIdxUnit, double *pdFs, int *pOrder)
{
    double dUnitFac;

    *pIdxUnit = GetDlgListCurSel(hDlg, IDD_FREQUENCY_UNITBOX);
    dUnitFac = FrequencyUnits[*pIdxUnit].dUnitFactor;

    if (!CheckDlgItemInt(hDlg, IDD_FILTER_ORDER, IDD_FILTER_ORDER_TEXT,
                         1, MAXORDER, pOrder))
        return FALSE;

    if (!CheckDlgItemFloat(hDlg, IDD_SAMPLE_RATE, IDD_SAMPLE_RATE_TEXT,
                           MIN_SAMPLE_INPUT/dUnitFac,
                           MAX_F_INPUT/dUnitFac, pdFs))
        return FALSE;

    *pdFs *= dUnitFac;     /* store only in Hz */
    return TRUE;           /* no error */
} /* GetSamplesAndOrder */


/* checks common dialog items in Lin. FIR and Standard IIR dialog boxes
   (syntax and semantic, e.g. Nyquist frequency bound limits)
 * checks syntax/semantic of : design order, sampling frequency, center frequency
 * returns TRUE, if no errors found
 * returns FALSE, if an error occurs and the user does wish to correct or not
                  (if not, the call of EndDialog is automatic done)
 */

static BOOL CommonParametersOk(HWND hDlg, tFilter *lpTmpFilter)
{
    double dUnitFac;

    if (!GetSamplesAndOrder(hDlg, &lpTmpFilter->iInputUnitF,
                            &lpTmpFilter->f0,
                            (lpTmpFilter->FltDlg == LINFIRDLG) ?
                            &lpTmpFilter->DefDlg.FIRDat.nOrder :
                            &lpTmpFilter->DefDlg.StdIIRDat.nOrder))

        return FALSE;

    dUnitFac = FrequencyUnits[lpTmpFilter->iInputUnitF].dUnitFactor;

    switch(lpTmpFilter->FTr.FTransf)  /* check frequency transf dependend parameters */
    {
        case NOFTR :
        {
            double dCutoff;
            if (!CheckDlgItemFloat(hDlg, IDD_DLG_CUTOFF, IDD_DLG_CUTOFF_TEXT,
                                   MIN_CUTOFF_INPUT/dUnitFac,
                                   lpTmpFilter->f0 / dUnitFac / 2.0, /* Nyquist frequency */
                                   &dCutoff))
                return FALSE;    /* user break */

            switch(lpTmpFilter->FltDlg)
            {
                case LINFIRDLG :
                    lpTmpFilter->DefDlg.FIRDat.dCutoff = dCutoff*dUnitFac;
                    break;

                case STDIIRDLG :
                    lpTmpFilter->DefDlg.StdIIRDat.dCutoff = dCutoff*dUnitFac;
                    break;
            } /* switch */

            break;              /* input ok */
        } /* NOFTR */

        case BANDPASS :
        case BANDSTOP :                
            if (lpTmpFilter->FTr.dCenter > lpTmpFilter->f0*0.5) 
            {                                 /* if gt. Nyquist frequency */
                if (HandleDlgError(hDlg, IDD_SAMPLE_RATE,
                                   ERROR_CENTER_F_NOT_VALID))
                    PostMessage(hDlg, WM_COMMAND,          /* open dialog */
                                IDD_F_TRANSFORM, MAKELPARAM(0, BN_CLICKED));
                return FALSE;
            } /* if */
              /* and fall through */

        case HIGHPASS :
            if (lpTmpFilter->FTr.dCutFBw > lpTmpFilter->f0*0.5) 
            {                                  /* gt. Nyquist frequency ? */
                if (HandleDlgError(hDlg, IDD_SAMPLE_RATE,
                                   ERROR_CUTOFF_BW_NOT_VALID))
                    PostMessage(hDlg, WM_COMMAND,          /* open dialog */
                                IDD_F_TRANSFORM, MAKELPARAM(0, BN_CLICKED));
                return FALSE;
            } /* if */
    } /* switch */

    return TRUE;
} /* CommonParametersOk */



/* calls the frequency transformation dialog with default values
   if available 
 */
static void CallFTransfDialog(HWND hDlg, tTransformData *pFTr)
{
    tTransformData TempFTr = *pFTr;

    if (pFTr->FTransf == NOFTR)      /* set LP->HP default transformation */
    {                                          /* if valid data available */
        if (CVT_OK == GetDlgItemFloat(hDlg, IDD_DLG_CUTOFF, MIN_CUTOFF_INPUT,
                                      WORLD_MAX, &TempFTr.dCutFBw))
        {
            short  IdxUnit = GetDlgListCurSel(hDlg, IDD_FREQUENCY_UNITBOX);
            double dUnitFac = FrequencyUnits[IdxUnit].dUnitFactor;

            TempFTr.iDefaultUnit = IdxUnit;         /* default unit index */
            TempFTr.uFlags = 0;                       /* no special flags */
            TempFTr.dCutFBw *= dUnitFac;        /* cuttoff (HP) frequency */
        } /* if */
        else TempFTr.dCutFBw = 0.0; /* invalid flag */
    } /* if */

    if (FDWinDialogBoxParam(FTRANSFORMDLG, hDlg,
                            FTransformDlgProc, (LPARAM)(LPSTR)(&TempFTr)))
    {
        *pFTr = TempFTr;                /* copy to target structure if ok */
        SetDlgFocus(hDlg, IDOK);   /* if ok set focus inside parant to ok */
    } /* if */

} /* CallFTransfDialog() */



/* Definition box for lin. FIR-Filter */
BOOL CALLBACK LinFirDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static tRadioBtnDesc aBtnLinSub[] =
    {
        {RECT_LP,  IDD_LINFIRDLG_RECTLP},
        {GAUSS_LP, IDD_LINFIRDLG_GAUSS},
        {COS_LP,   IDD_LINFIRDLG_COS},
        {COS2_LP,  IDD_LINFIRDLG_COS2},
        {SQR_LP,   IDD_LINFIRDLG_SQR},
        {0, 0}
    };

    static tFilter TmpFilter;          /* definition of current FIR filter */
    static tLinFIR *lpPar = &TmpFilter.DefDlg.FIRDat; /* for faster access */

    switch (msg)
    {
           case WM_INITDIALOG :
           {
               TmpFilter = MainFilter;        /* alloc memory later (IDOK) */
               if ((BOOL)lParam || (MainFilter.FltDlg != LINFIRDLG))
               {
                   lpPar->DataWin  = RECT_WIN;     /* window to clip impulse response */
                   lpPar->SubType  = RECT_LP;      /* response of rect win */
                   TmpFilter.iInputUnitF =
                       Diags[DIAG_F_RESPONSE].DefaultDiag.X.iCurrUnit;
                   TmpFilter.FTr.iDefaultUnit = TmpFilter.iInputUnitF;
                   TmpFilter.FTr.FTransf = NOFTR;/* no frequency transform */
                   TmpFilter.FltDlg = LINFIRDLG;   /* dialog type */
               } /* if */
               else
               {
                   SetDlgItemInt(hDlg, IDD_FILTER_ORDER,
                                 lpPar->nOrder, TRUE);
                   SetDlgItemFloatUnit(hDlg, IDD_SAMPLE_RATE, TmpFilter.f0,
                                       anPrec[IOUTPRECDLG_FREQU],
                                       TmpFilter.iInputUnitF,
                                       FrequencyUnits);

                   if (TmpFilter.FTr.FTransf == NOFTR)
                       SetDlgItemFloatUnit(hDlg, IDD_DLG_CUTOFF,
                                           lpPar->dCutoff,
                                           anPrec[IOUTPRECDLG_FREQU],
                                           TmpFilter.iInputUnitF,
                                           FrequencyUnits);
                   else ModifyLPCutoffLinFIR(hDlg, &TmpFilter);
               } /* else */

               ShowLinFIR_DataWin_FTransf(hDlg, &TmpFilter);

               DrawComboAxisUnits(hDlg, IDD_FREQUENCY_UNITBOX, IDD_FREQUENCY_UNIT_TEXT,
                                  TmpFilter.iInputUnitF, FrequencyUnits);
               CheckSelRadioBtn(hDlg, lpPar->SubType, aBtnLinSub);
               return TRUE;
           } /* case */

           case WM_COMMAND :
           {
                switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
                {
                    case IDD_FREQUENCY_UNITBOX :
                        if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == CBN_SELCHANGE)
                        {
                            TmpFilter.iInputUnitF =
                                GetDlgListCurSel(hDlg, IDD_FREQUENCY_UNITBOX);
                            if (TmpFilter.FTr.FTransf != NOFTR)
                                ModifyLPCutoffLinFIR(hDlg, &TmpFilter);
                        } /* if */
                        return TRUE;

                    case IDD_WINDOWING:
                        (void)FDWinDialogBoxParam(IMPULSWINDLG, hDlg, ImpWinDlgProc,
                                                  (LPARAM)(LPSTR)(&TmpFilter.DefDlg.FIRDat));
                        ShowLinFIR_DataWin_FTransf(hDlg, &TmpFilter);
                        SetDlgFocus(hDlg, IDOK);
                        return TRUE;

                    case IDD_F_TRANSFORM:
                        CallFTransfDialog(hDlg, &TmpFilter.FTr);
                        ShowLinFIR_DataWin_FTransf(hDlg, &TmpFilter);
                        ModifyLPCutoffLinFIR(hDlg, &TmpFilter);
                        return TRUE;

                    case IDD_LINFIRDLG_RECTLP :
                    case IDD_LINFIRDLG_GAUSS  :
                    case IDD_LINFIRDLG_COS    :
                    case IDD_LINFIRDLG_COS2   :
                    case IDD_LINFIRDLG_SQR    :
                        lpPar->SubType = GetSelRadioBtn(hDlg, aBtnLinSub);

                        ModifyLPCutoffLinFIR(hDlg, &TmpFilter);
                        return TRUE;

                    case IDOK :
                    {
                         UINT nIddDefineErr;
                         if (!CommonParametersOk(hDlg, &TmpFilter))
                             return TRUE;

                         switch(TmpFilter.FTr.FTransf)
                         {
                             case HIGHPASS :
                             case BANDSTOP :
                                 if (ODD(TmpFilter.DefDlg.FIRDat.nOrder))
                                 {
                                     (void)HandleDlgError(hDlg,
                                                          IDD_FILTER_ORDER,
                                                          ERROR_ODD_ORDER_HPBP);
                                     return FALSE;
                                 } /* if */
                         } /* switch */

                         nIddDefineErr = DefineLinFirFilter(&TmpFilter, &MainFilter);
                         switch (nIddDefineErr)
                         {
                             case IDSTRNULL : EndDialog(hDlg, TRUE);  /* all ok */
                                      break;

                             case IDSTROUTOFMEMORY :
                                 ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
                                 break;

                             default :
                                 (void)HandleDlgError(hDlg,
                                                      IDD_FILTER_ORDER,
                                                      nIddDefineErr);
                         } /* switch */

                         return TRUE;
                    } /* case IDOK */

                    case IDCANCEL :
                         EndDialog(hDlg, FALSE);
                         return TRUE;

                    case IDHELP :
                        WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpDefLinFirDlg);
                        return TRUE;
                } /* switch */

                break;
           } /* WM_COMMAND */
    }
    return FALSE;
} /* LinFirDlgProc */



static void EnableFltParamDlgItems(HWND hDlg, tStdIIR *lpIIRDat)
{
    static int const DlgItem[] =
    {
        IDD_STDIIRDLG_MINATT,
        IDD_STDIIRDLG_MINATT_TEXT,
        IDD_STDIIRDLG_MINATT_DBTXT,
        IDD_STDIIRDLG_RIPPLEATT,
        IDD_STDIIRDLG_RIPATT_TEXT,
        IDD_STDIIRDLG_RIPATT_DBTXT,
        IDD_STDIIRDLG_PARAMTXT,
        IDD_STDIIRDLG_MODULE_ANGLE,
        IDD_STDIIR_MODULE_TEXT,
        IDD_STDIIR_MODDEG_TEXT,
    };

    static BOOL const Enable[BESSEL+1][DIM(DlgItem)] =
    {
        {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, /* BUTTERWORTH */
        {FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE},     /* CHEBY 1 */
        {TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE},     /* CHEBY 2 */
        {FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE},        /* CAUER 1 */
        {TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE},        /* CAUER 2 */
        {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}  /* BESSEL */
    };

    int i;

    for (i = 0; i < DIM(DlgItem); i++)
        EnableDlgItem(hDlg, DlgItem[i], Enable[lpIIRDat->SubType][i]);
} /* EnableFltParamDlgItems() */



/* Definition box for standard IIR-Filter */
BOOL CALLBACK StdIIRDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                            LPARAM lParam)
{
    static tRadioBtnDesc aBtnStdIIRSub[] =
    {
        {BUTTERWORTH, IDD_STDIIRDLG_BTWLP},
        {CHEBY1,      IDD_STDIIRDLG_CHEBY1},
        {CHEBY2,      IDD_STDIIRDLG_CHEBY2},
        {CAUER1,      IDD_STDIIRDLG_CAUER1},
        {CAUER2,      IDD_STDIIRDLG_CAUER2},
        {BESSEL,      IDD_STDIIRDLG_BESSEL},
        {0, 0}
    };

    static tFilter TmpFilter;          /* definition of current FIR filter */
    static tStdIIR *lpPar = &TmpFilter.DefDlg.StdIIRDat; /* for faster access */

    switch (msg)
    {
           case WM_INITDIALOG :
           {
               TmpFilter = MainFilter;        /* alloc memory later (IDOK) */

               if ((BOOL)lParam || (MainFilter.FltDlg != STDIIRDLG))
               {
                   lpPar->SToZTransf = BILINEAR;
                   lpPar->SubType  = BUTTERWORTH;       /* standard filter */
                   TmpFilter.FTr.FTransf = NOFTR;/* no frequency transform */
                   TmpFilter.FltDlg = STDIIRDLG;   /* dialog type */
                   TmpFilter.iInputUnitF =
                       Diags[DIAG_F_RESPONSE].DefaultDiag.X.iCurrUnit;
                   TmpFilter.FTr.iDefaultUnit = TmpFilter.iInputUnitF;
               }
               else
               {
                   SetDlgItemInt(hDlg, IDD_FILTER_ORDER,
                                 lpPar->nOrder, TRUE);
                   SetDlgItemFloatUnit(hDlg, IDD_SAMPLE_RATE,
                                       TmpFilter.f0, anPrec[IOUTPRECDLG_FREQU],
                                       TmpFilter.iInputUnitF, FrequencyUnits);

                   if (TmpFilter.FTr.FTransf == NOFTR)
                       SetDlgItemFloatUnit(hDlg, IDD_DLG_CUTOFF,
                                           lpPar->dCutoff, anPrec[IOUTPRECDLG_FREQU],
                                           TmpFilter.iInputUnitF, FrequencyUnits);
                   else ModifyLPCutoffStdIIR(hDlg, &TmpFilter);

                   switch (lpPar->SubType)
                   {
                       case CAUER1 :
                           SetDlgItemFloat(hDlg, IDD_STDIIRDLG_MODULE_ANGLE,
                                           lpPar->dModuleAngle,
                                           anPrec[IOUTPRECDLG_OTHER]);
                                                       /* and fall through */
                       case CHEBY1 :
                           SetDlgItemFloat(hDlg, IDD_STDIIRDLG_RIPPLEATT,
                                           lpPar->dRippleAtt,
                                           anPrec[IOUTPRECDLG_ATTENUATION]);
                           break;

                       case CAUER2 :
                           SetDlgItemFloat(hDlg, IDD_STDIIRDLG_MODULE_ANGLE,
                                           lpPar->dModuleAngle,
                                           anPrec[IOUTPRECDLG_OTHER]);
                                                       /* and fall through */
                       case CHEBY2 :
                           SetDlgItemFloat(hDlg, IDD_STDIIRDLG_MINATT,
                                           lpPar->dMinAtt,
                                           anPrec[IOUTPRECDLG_ATTENUATION]);
                           break;
                   } /* switch */
               } /* else */

               ShowCurrFTransfInDlg(hDlg, TmpFilter.FTr.FTransf);
               EnableFltParamDlgItems(hDlg, lpPar);

               DrawComboAxisUnits(hDlg, IDD_FREQUENCY_UNITBOX, IDD_FREQUENCY_UNIT_TEXT,
                                  TmpFilter.iInputUnitF, FrequencyUnits);
               CheckSelRadioBtn(hDlg, lpPar->SubType, aBtnStdIIRSub);
               return TRUE;
           } /* WM_INITDIALOG */


           case WM_COMMAND :
           {
                switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
                {
                    case IDD_FREQUENCY_UNITBOX :
                        if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == CBN_SELCHANGE)
                        {
                            TmpFilter.iInputUnitF =
                                GetDlgListCurSel(hDlg, IDD_FREQUENCY_UNITBOX);

                            ModifyLPCutoffStdIIR(hDlg, &TmpFilter);
                        } /* if */
                        return TRUE;

                    case IDD_F_TRANSFORM:
                        CallFTransfDialog(hDlg, &TmpFilter.FTr);
                        ShowCurrFTransfInDlg(hDlg, TmpFilter.FTr.FTransf);
                        ModifyLPCutoffStdIIR(hDlg, &TmpFilter);
                        return TRUE;

                    case IDD_STDIIRDLG_BTWLP  :
                    case IDD_STDIIRDLG_CHEBY1 :
                    case IDD_STDIIRDLG_CHEBY2 :
                    case IDD_STDIIRDLG_CAUER1 :
                    case IDD_STDIIRDLG_CAUER2 :
                    case IDD_STDIIRDLG_BESSEL :

                        lpPar->SubType = GetSelRadioBtn(hDlg, aBtnStdIIRSub);

                        EnableFltParamDlgItems(hDlg, lpPar);
                        return TRUE;


                    case IDD_STDIIRDLG_MODULE_ANGLE :
                        if ((GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_MOUSE_DEC)
                            || (GET_WM_COMMAND_NOTIFY(wParam, lParam) == EN_MOUSE_INC))
                        {
                            double dAngle;
                            int CvtErr;

                            CvtErr = GetDlgItemFloat(hDlg,
                                                     IDD_STDIIRDLG_MODULE_ANGLE,
                                                     MIN_MODANGLE_INPUT,
                                                     MAX_MODANGLE_INPUT,
                                                     &dAngle);

                            switch (CvtErr)
                            {
                                case CVT_ERR_EMPTY :
                                    dAngle = DEFAULT_MODANGLE; break;

                                case CVT_OK :
                                    dAngle = ROUND(dAngle); break;

                                case CVT_ERR_OVERFLOW :
                                    dAngle = MAX_MODANGLE_INPUT-1; break;

                                case CVT_ERR_UNDERFLOW :
                                    dAngle = MIN_MODANGLE_INPUT+1; break;
                            } /* switch */

                            switch (GET_WM_COMMAND_NOTIFY(wParam, lParam))
                            {
                                case EN_MOUSE_DEC:
                                    if (--dAngle < MIN_MODANGLE_INPUT) return TRUE;
                                    break;

                                case EN_MOUSE_INC:
                                    if (++dAngle > MAX_MODANGLE_INPUT) return TRUE;
                                    break;
                            } /* switch */

                            SetDlgItemFloat(hDlg, IDD_STDIIRDLG_MODULE_ANGLE,
                                            dAngle, anPrec[IOUTPRECDLG_OTHER]);
                            return TRUE;
                        } /* if */
                        break;


                    case IDOK :
                    {
                         UINT nIddDefineErr;

                         if(!CommonParametersOk(hDlg, &TmpFilter))
                             return TRUE;

                         switch(TmpFilter.FTr.FTransf)
                         {
                             case BANDPASS :
                             case BANDSTOP :
                                 if (ODD(TmpFilter.DefDlg.StdIIRDat.nOrder))
                                 {
                                     (void)HandleDlgError(hDlg,
                                                          IDD_FILTER_ORDER,
                                                          ERROR_ODD_ORDER_HPBP);
                                     return FALSE;
                                 } /* if */
                         } /* switch */

                         switch(lpPar->SubType)
                         {
                             case CAUER1 :
                                 if (!CheckDlgItemFloat(hDlg,
                                                        IDD_STDIIRDLG_MODULE_ANGLE,
                                                        IDD_STDIIR_MODULE_TEXT,
                                                        MIN_MODANGLE_INPUT,
                                                        MAX_MODANGLE_INPUT,
                                                        &lpPar->dModuleAngle))
                                     return FALSE;
                             /* if non error fall through */

                             case CHEBY1 :
                                 if (!CheckDlgItemFloat(hDlg, IDD_STDIIRDLG_RIPPLEATT,
                                                        IDD_STDIIRDLG_RIPATT_TEXT,
                                                        MIN_RIPPLE_INPUT,
                                                        MAX_RIPPLE_INPUT,
                                                        &lpPar->dRippleAtt))
                                     return FALSE;    /* user break */
                                 break;

                             case CAUER2 :
                                 if (!CheckDlgItemFloat(hDlg,
                                                        IDD_STDIIRDLG_MODULE_ANGLE,
                                                        IDD_STDIIR_MODULE_TEXT,
                                                        MIN_MODANGLE_INPUT,
                                                        MAX_MODANGLE_INPUT,
                                                        &lpPar->dModuleAngle))
                                     return FALSE;
                             /* if non error fall through */

                             case CHEBY2 :
                                 if (!CheckDlgItemFloat(hDlg, IDD_STDIIRDLG_MINATT,
                                                        IDD_STDIIRDLG_MINATT_TEXT,
                                                        MIN_STOPBAND_ATT_INPUT,
                                                        MAX_STOPBAND_ATT_INPUT,
                                                        &lpPar->dMinAtt))
                                     return FALSE;    /* user break */
                         } /* switch */

                         nIddDefineErr = DefineStdIIRFilter(hDlg, &TmpFilter, &MainFilter);
                         switch (nIddDefineErr)
                         {
                             case IDSTRNULL :
                                 EndDialog(hDlg, TRUE);
                                 break;                         /* all ok */

                             case IDSTROUTOFMEMORY :
                                 ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
                                 break; /* IDSTROUTOFMEMORY */

                             default :                          /* errors */
                                 (void)HandleDlgError(hDlg, IDD_FILTER_ORDER, nIddDefineErr);
                         } /* switch */

                         return TRUE;
                    } /* case IDOK */

                    case IDCANCEL :
                         EndDialog(hDlg, FALSE);
                         return TRUE;

                    case IDHELP :
                        WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpDefStdIirDlg);
                        return TRUE;

                } /* switch */

                break;
           } /* WM_COMMAND */
    }
    return FALSE;
} /* StdIIRDlgProc */


/**************** Access to predefined filters from RC-file ***************/


static void DisplayPredefDesc(HINSTANCE hInst, HWND hDlg)
{
    int idx = GetDlgListCurSel(hDlg, IDD_PREDEF_TYPEBOX);

    if (idx >= 0)
    {
        LPSTR p;

        /* get ID of ressource from selected entry for loading */
        idx = (int)SendDlgItemMessage(hDlg, IDD_PREDEF_TYPEBOX,
                                      CB_GETITEMDATA, idx, 0L);

        if ((p = LoadRCdata(hInst, idx)) != NULL)
        {
            SetDlgItemText(hDlg, IDD_PREDEF_DESC, p);
            FreeRCdata();
        } /* if */
    } /* if */
} /* DisplayPredefDesc() */



/* dialog procedure for various predefined FIR/IIR-Filters with constant
   order */
BOOL CALLBACK PredefFilterDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                  LPARAM lParam)
{
    static tFilter TmpFilter;         /* definition of current FIR filter */

    switch (msg)
    {
           case WM_INITDIALOG :
           {
               LPSTR pData;
               HRSRC hrc;
               HGLOBAL hMemIdRcFlt;
               int nFltNum;
               int iDefaultSel = 0;

               HINSTANCE hInst = GetWindowInstance(hwndFDesk);

               TmpFilter = MainFilter;        /* alloc memory later (IDOK) */

               hrc = FindResource(hInst, MAKEINTRESOURCE(IDRC_NUMPREDEF), RT_RCDATA);
               if (hrc != (HRSRC)0)
               {
                   hMemIdRcFlt = LoadResource(hInst, hrc);
                   if (hMemIdRcFlt != (HGLOBAL)0)
                   {
                       pData = LockResource(hMemIdRcFlt); /* lock into mem */
                       if (pData != NULL)
                       {
                           int *anIdRcFlt;          /* points to ID-array */
                           int i;

                           nFltNum = *((int *)pData);   /* first entry is */
                           pData += sizeof(int);     /* number of filters */
                           anIdRcFlt = (int *)pData;

                           for (i=0;i < nFltNum; i++)    /* fill the list */
                           {
                               int idx;
                               LPSTR p = LoadRCdata(hInst, anIdRcFlt[i]);

                               if (p != NULL)
                               {
                                   idx = (int)SendDlgItemMessage
                                         (
                                             hDlg, IDD_PREDEF_TYPEBOX,
                                             CB_ADDSTRING, DUMMY_WPARAM,
                                             (LPARAM)(p+lstrlen(p)+1)
                                         );

                                   if (idx >= 0)
                                   {
                                       SendDlgItemMessage
                                       (
                                           hDlg, IDD_PREDEF_TYPEBOX,
                                           CB_SETITEMDATA, idx, anIdRcFlt[i]
                                       );

                                       if (anIdRcFlt[i] ==
                                           TmpFilter.DefDlg.PredefSub)

                                           iDefaultSel = idx;
                                   } /* if */

                                   FreeRCdata();
                               } /* if */
                           } /* for */

                           SendDlgItemMessage(hDlg, IDD_PREDEF_TYPEBOX,
                                              CB_SETCURSEL, 0, 0L);

                           UnlockResource(hMemIdRcFlt);
                       } /* if */

                       FreeResource(hMemIdRcFlt);
                   } /* if */
               } /* if */


               if ((BOOL)lParam || (MainFilter.FltDlg != PREDEFDLG))
               {
                   iDefaultSel = 0;
                   TmpFilter.FTr.FTransf = NOFTR;/* no frequency transform */
                   TmpFilter.FltDlg = PREDEFDLG;   /* dialog type */
                   TmpFilter.iInputUnitF =
                       Diags[DIAG_F_RESPONSE].DefaultDiag.X.iCurrUnit;
                   TmpFilter.FTr.iDefaultUnit = TmpFilter.iInputUnitF;
               }
               else
                   SetDlgItemFloatUnit(hDlg, IDD_SAMPLE_RATE,
                                       TmpFilter.f0, anPrec[IOUTPRECDLG_FREQU],
                                       TmpFilter.iInputUnitF, FrequencyUnits);

               SendMessage(GetDlgItem(hDlg, IDD_PREDEF_TYPEBOX),
                           CB_SETCURSEL, iDefaultSel, DUMMY_LPARAM);

               DisplayPredefDesc(hInst, hDlg);
               DrawComboAxisUnits(hDlg, IDD_FREQUENCY_UNITBOX, IDD_FREQUENCY_UNIT_TEXT,
                                  TmpFilter.iInputUnitF, FrequencyUnits);
               return TRUE;
           } /* WM_INITDIALOG */

           case WM_COMMAND :
                switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
                {
                    case IDD_PREDEF_TYPEBOX :
                        if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == CBN_SELCHANGE)
                        {
                            DisplayPredefDesc(GetWindowInstance(hwndFDesk), hDlg);
                            return TRUE;
                        } /* if */

                        break; /* WM_COMMAND */


                    case IDOK :
                    {
                        int nIddDefineErr, idx;
                        double dUnitFac;


                        TmpFilter.iInputUnitF =
                            GetDlgListCurSel(hDlg, IDD_FREQUENCY_UNITBOX);

                        dUnitFac = FrequencyUnits[TmpFilter.iInputUnitF].dUnitFactor;
                        if (!CheckDlgItemFloat(hDlg, IDD_SAMPLE_RATE,
                                               IDD_SAMPLE_RATE_TEXT,
                                               MIN_SAMPLE_INPUT/dUnitFac,
                                               MAX_F_INPUT/dUnitFac,
                                               &TmpFilter.f0))
                            return TRUE;


                        idx = GetDlgListCurSel(hDlg, IDD_PREDEF_TYPEBOX);
                        if (idx >= 0)
                        {
                            TmpFilter.DefDlg.PredefSub =
                                (int)SendDlgItemMessage(hDlg, IDD_PREDEF_TYPEBOX,
                                                       CB_GETITEMDATA, idx, 0L);

                            TmpFilter.f0 *= dUnitFac;     /* define in Hz */
                            nIddDefineErr = SetPredefFilter (
                                                GetWindowInstance(hDlg),
                                                &TmpFilter, &MainFilter);
                            switch (nIddDefineErr)
                            {
                                case IDSTRNULL :
                                    EndDialog(hDlg, TRUE);
                                    break; /* ok ! */

                                case IDSTROUTOFMEMORY :
                                    ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
                                    break;

                                default :
                                    (void)HandleDlgError(hDlg, IDD_FILTER_ORDER,
                                                         nIddDefineErr);
                            } /* switch */
                        } /* if */

                        return TRUE;
                    } /* IDOK */

                    case IDCANCEL :
                         EndDialog(hDlg, FALSE);
                         return TRUE;

                    case IDHELP :
                        WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpPredefSysDlg);
                        return TRUE;
                } /* switch */

                break;
    } /* switch */
    return FALSE;
}






/* misc digital systems with parameters */

typedef struct
{
    int nIdParamEdit;   /* ID of edit field */
    int nIdParamName;   /* ID of name field */
    int nIdStr;         /* string id of paramter name */
    tUnitDesc *pUnits;  /* pointer to units description (NULL if not exist) */
    int nIdUnitBox;     /* ID of units combo box */
    double dMinInp;     /* min and max possible value */
    double dMaxInp;
} const tDescParam;


static tDescParam DescParam[] =     /* no description up today */
{                                      /* these are only samples ! */
    {IDD_PREDEF_PAR1_EDIT, IDD_PREDEF_PAR1_NAME, STRING_PARAM_ORDER, NULL,
     IDD_PREDEF_PAR1_UNIT, 1, MAXORDER},
    {IDD_PREDEF_PAR2_EDIT, IDD_PREDEF_PAR2_NAME, STRING_PARAM_ORDER, FrequencyUnits,
     IDD_PREDEF_PAR2_UNIT, 1, MAXORDER}
};


static tDescParam *apSysDesc[][MAXMISCSYSPARAM] =
{                 /* no additional param systems exist up today */                           
    {NULL, NULL}, /* HILBERT_TRANSF90 */
    {NULL, NULL}, /* INTEGR_FSDEV */
    {NULL, NULL}, /* DIFF_FSDEV */
    {NULL, NULL}, /* COMB */
    {NULL, NULL}, /* AVG_FIR */
    {NULL, NULL}, /* AVG_IIR */
    {NULL, NULL} /* AVG_EXP */
}; /* pDesc */



static int aSysNames[] =
{                                            
    STRING_HILBERT90,     /* HILBERT_TRANSF90 */
    STRING_INTEGRATOR,
    STRING_DIFFERENTIATOR,
    STRING_COMBFILTER,
    STRING_AVGFIR,
    STRING_AVGIIR,
    STRING_AVGEXP
}; /* pDesc */




/* enables dialog items dependent on selected system */

static void SetMiscSysDlgItems(HWND hDlg, tVarSys *pDigSys)
{
    int i;
    tMiscDigSys Type = pDigSys->SubType;

    ShowDlgItem(hDlg, IDD_PREDEF_PAR1_EDIT, FALSE);
    ShowDlgItem(hDlg, IDD_PREDEF_PAR2_EDIT, FALSE);
    ShowDlgItem(hDlg, IDD_PREDEF_PAR1_NAME, FALSE);
    ShowDlgItem(hDlg, IDD_PREDEF_PAR2_NAME, FALSE);
    ShowDlgItem(hDlg, IDD_PREDEF_PAR1_UNIT, FALSE);
    ShowDlgItem(hDlg, IDD_PREDEF_PAR2_UNIT, FALSE);

    for (i = 0; i < MAXMISCSYSPARAM; i++)
    {
        tDescParam *pP = apSysDesc[Type][i];

        if (pP != NULL)                              /* parameter exist ? */
        {
            ShowDlgItem(hDlg, pP->nIdParamEdit, TRUE);
            ShowDlgItem(hDlg, pP->nIdParamName, TRUE);
            SetDlgItemTextId(hDlg, pP->nIdParamName, pP->nIdStr);

            if (pP->pUnits != NULL)
            {
                ShowDlgItem(hDlg, pP->nIdUnitBox, TRUE);
                (void)DrawComboAxisUnits(hDlg, pP->nIdUnitBox, 0,
                                         pDigSys->iUnit[i], pP->pUnits);
            } /* if */
        } /* if */
    } /* for */
} /* SetMiscSysDlgItems */




BOOL CALLBACK MiscDigSystemsDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{

    static tFilter TmpFilter;     /* definition of current digital system */
    static tVarSys *lpPar = &TmpFilter.DefDlg.MiscFltDat;  /* fast access */

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            int i;

            TmpFilter = MainFilter;                 /* alloc memory later */

            if ((BOOL)lParam || (MainFilter.FltDlg != MISCDLG))
            {                        /* new (first) misc system dialog */
                lpPar->SubType  = (tMiscDigSys)0;  /* set first system */

                for (i = 0; i < MAXMISCSYSPARAM; i++)
                    if (apSysDesc[lpPar->SubType][i] != NULL)
                        lpPar->iUnit[i] = 0;  /* independent (exist or not exist) */

                TmpFilter.iInputUnitF = Diags[DIAG_F_RESPONSE].DefaultDiag.X.iCurrUnit;
                TmpFilter.FTr.iDefaultUnit = TmpFilter.iInputUnitF;
                TmpFilter.FltDlg = MISCDLG;             /* dialog type */
                TmpFilter.FTr.FTransf = NOFTR;    /* no transformation */
            } /* if */
            else
            {
                SetDlgItemInt(hDlg, IDD_FILTER_ORDER, lpPar->nOrder, TRUE);

                SetDlgItemFloatUnit(hDlg, IDD_SAMPLE_RATE,
                                    TmpFilter.f0, anPrec[IOUTPRECDLG_FREQU],
                                    TmpFilter.iInputUnitF, FrequencyUnits);
                for (i = 0; i < MAXMISCSYSPARAM; i++)
                {
                    if (apSysDesc[lpPar->SubType][i] != NULL)
                        SetDlgItemFloatUnit(hDlg,
                                            apSysDesc[lpPar->SubType][i]->nIdParamEdit,
                                            lpPar->adParam[i], MAX_PRECISION,
                                            lpPar->iUnit[i],
                                            apSysDesc[lpPar->SubType][i]->pUnits
                                           );
                } /* for */
            } /* else */

            DrawBoxIdStrings(hDlg, IDD_MISC_TYPEBOX, (int)lpPar->SubType,
                             DIM(aSysNames), aSysNames);

            DrawComboAxisUnits(hDlg, IDD_FREQUENCY_UNITBOX, IDD_FREQUENCY_UNIT_TEXT,
                               TmpFilter.iInputUnitF, FrequencyUnits);

            SetMiscSysDlgItems(hDlg, lpPar);        /* visibility etc. */

            return TRUE;
        } /* WM_INITDIALOG */

        case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_MISC_TYPEBOX :
                    if (GET_WM_COMMAND_NOTIFY(wParam, lParam) == CBN_SELCHANGE)
                    {
                        int i;

                        lpPar->SubType = (tMiscDigSys)GetDlgListCurSel(hDlg, IDD_MISC_TYPEBOX);
                        SetMiscSysDlgItems(hDlg, lpPar);

                        for (i = 0; (i < MAXMISCSYSPARAM) &&
                                    (apSysDesc[lpPar->SubType][i] == NULL);
                             i++)
                        {
                            if (apSysDesc[lpPar->SubType][i] != NULL)
                                SetDlgFocus(hDlg, apSysDesc[lpPar->SubType][i]->nIdParamEdit);
                        } /* for */
                        return TRUE;
                    } /* if */
                    break; /* IDD_MISC_TYPEBOX */

                case IDOK :
                {

                    int i;
                    UINT nIddDefineErr;

                    if (!GetSamplesAndOrder(hDlg, &TmpFilter.iInputUnitF,
                                            &TmpFilter.f0,
                                            &lpPar->nOrder))
                        return TRUE;

                    /* check special subtype dependent restrictions */
                    switch(lpPar->SubType)
                    {
                        case HILBERT_TRANSF90 : /* Hilbert transformer */
                        case INTEGR_FSDEV :              /* integrator */
                        case DIFF_FSDEV:             /* differantiator */

                            if (ODD(lpPar->nOrder))
                            {
                                (void)HandleDlgError(hDlg, IDD_FILTER_ORDER,
                                                     ERROR_ODD_ORDER_FRDEF);
                                return TRUE;
                            } /* if */
                    } /* switch */

                    if ((lpPar->SubType == HILBERT_TRANSF90) &&
                        !ODD(lpPar->nOrder/2))
                    {
                        (void)HandleDlgError(hDlg, IDD_FILTER_ORDER,
                                             ERROR_HILBERT_ORDER);
                        return TRUE;
                    } /* if */

                    for (i = 0; i < MAXMISCSYSPARAM; i++)
                    {
                        tDescParam *pP = apSysDesc[lpPar->SubType][i];

                        if (pP != NULL)
                        {
                            double dFactor = 1.0;

                            if (pP->pUnits != NULL)
                            {
                                lpPar->iUnit[i] =  /* get index in box */
                                    GetDlgListCurSel(hDlg, pP->nIdUnitBox);

                                dFactor = pP->pUnits[lpPar->iUnit[i]].dUnitFactor;
                            } /* if */

                            if (!CheckDlgItemFloat(hDlg, pP->nIdParamEdit,
                                                   pP->nIdParamName,
                                                   pP->dMinInp/dFactor,
                                                   pP->dMaxInp/dFactor,
                                                   &lpPar->adParam[i]))
                                return TRUE;

                            lpPar->adParam[i] *= dFactor;
                        } /* if */
                    } /* for */

                    nIddDefineErr = DefineMiscDigSys(&TmpFilter, &MainFilter);
                    switch (nIddDefineErr)
                    {
                         case IDSTRNULL :
                             EndDialog(hDlg, TRUE);
                             break; /* IDSTRNULL */

                         case IDSTROUTOFMEMORY :
                             ErrorAckUsr(hDlg, IDSTROUTOFMEMORY);
                             break;

                         default : (void)HandleDlgError(hDlg, IDD_FILTER_ORDER,
                                                        nIddDefineErr);
                    } /* switch */

                    return TRUE;
                } /* IDOK */

                case IDCANCEL :
                    EndDialog(hDlg, FALSE);
                    return TRUE;

                case IDHELP :
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpDefSpecialDlg);
                    return TRUE;
            } /* switch */

            break; /* WM_COMMAND */
    } /* switch (msg) */

    return FALSE;
} /* MiscFilterDlgProc() */

