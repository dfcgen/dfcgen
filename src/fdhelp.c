#include "DFCWIN.H"
#include "FDREG.H"


#pragma argsused  /* disable "Parameter is never used" Warning */
BOOL CALLBACK HelpAboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG :
        {
            static char *FileInfoName[] =
            {
                "ProductName", "FileDescription",
                "FileVersion", "LegalCopyright", "E-Mail"
            };


            int i;
            LPSTR pFileInfo[DIM(FileInfoName)]; /* array of position ptr's */
            char szAboutText[1024]; /* the complete "About"-Text in window */
            char szFileInfoBuf[1024];       /* all requested version infos */
            char szFormatAbout[256]; /* format of output string (in string tab. */
            char *pBuf = szFileInfoBuf;      /* current position in buffer */

            for (i = 0; i < DIM(FileInfoName); i++)
            {
                pFileInfo[i] = pBuf;    /* save position of info in buffer */
                if (!GetRCVerStringInfo(FileInfoName[i], pBuf, /* get info */
                                        DIM(szFileInfoBuf)-
                                        (pBuf-szFileInfoBuf))) return TRUE;
                pBuf += lstrlen(pBuf)+1;               /* to next position */
            } /* for */


            if (IsLicenseOk())
            {
                LoadString(GetWindowInstance(hDlg), FORMAT_ABOUT_INFO,
                           szFormatAbout, DIM(szFormatAbout));  /* load format */

                wsprintf(szAboutText, szFormatAbout, pFileInfo[0], pFileInfo[1],
                         pFileInfo[2], pFileInfo[3], pFileInfo[4],
                         GetFreeSpace(0)/1024UL,
                         GetFreeSystemResources(GFSR_SYSTEMRESOURCES));
            } /* if */
            else
            {
                LoadString(GetWindowInstance(hDlg), FORMAT_ABOUT_INFO_NOLICENSE,
                           szFormatAbout, DIM(szFormatAbout));

                wsprintf(szAboutText, szFormatAbout, pFileInfo[0], pFileInfo[1],
                         pFileInfo[2], pFileInfo[3], pFileInfo[4]);
            
            } /* else */

            SetDlgItemText(hDlg, IDD_ABOUT_TEXT, szAboutText); 
            return TRUE;
        } /* WM_INITDIALOG */


	    case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {                             /* wait until OK button pressed */
                case IDD_ABOUT_REGISTER:
                    PostMessage(hwndFDesk, WM_COMMAND, IDM_HELP_REGISTER, MAKELONG(0, 0));
                                                      /* and fall through */
                case IDOK :               
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDCANCEL :
                    EndDialog(hDlg, FALSE);
                    return TRUE;

                case IDHELP :
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpAboutDlg);
                    return TRUE;
            } /* switch */

            break; /* WM_COMMAND */
    } /* switch */

    return FALSE;
} /* HelpAboutDlgProc */




#pragma argsused             /* disable "Parameter is never used" Warning */
BOOL CALLBACK HelpLicenseDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char szCompany[SIZE_LICENSE_COMPANY+1];
    char szSerNo[SIZE_LICENSE_SERNO+1];
    char szUsr[SIZE_LICENSE_USER+1];

    switch (msg)
    {
        case WM_INITDIALOG :
            SendDlgItemMessage(hDlg, IDD_LICENSEDLG_NAME, EM_LIMITTEXT,
                               SIZE_LICENSE_USER, DUMMY_LPARAM);
            SendDlgItemMessage(hDlg, IDD_LICENSEDLG_COMPANY, EM_LIMITTEXT,
                               SIZE_LICENSE_COMPANY, DUMMY_LPARAM);
            SendDlgItemMessage(hDlg, IDD_LICENSEDLG_SN, EM_LIMITTEXT,
                               SIZE_LICENSE_SERNO, DUMMY_LPARAM);

            GetLicenseData(szSerNo, szUsr, szCompany);
            SetDlgItemText(hDlg, IDD_LICENSEDLG_NAME, szUsr);
            SetDlgItemText(hDlg, IDD_LICENSEDLG_COMPANY, szCompany);
            SetDlgItemText(hDlg, IDD_LICENSEDLG_SN, szSerNo);
            return TRUE;

#if DEBUG
		case WM_CHAR:
            (void)SetLicenseData(szSerNo, szUsr, szCompany);
            SetDlgItemText(hDlg, IDD_LICENSEDLG_SN, szSerNo);
            return FALSE;
#endif


	    case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDOK :
                    GetDlgItemText(hDlg, IDD_LICENSEDLG_NAME, szUsr, DIM(szUsr));
                    GetDlgItemText(hDlg, IDD_LICENSEDLG_COMPANY, szCompany, DIM(szCompany));
                    GetDlgItemText(hDlg, IDD_LICENSEDLG_SN, szSerNo, DIM(szSerNo));

                    if (SetLicenseData(szSerNo, szUsr, szCompany))
                        EndDialog(hDlg, TRUE);
                    else
                        if (!HandleDlgError(hDlg, IDD_LICENSEDLG_NAME, ERROR_INPLICENSE)) return TRUE;

                    return TRUE;

                case IDCANCEL :
                    EndDialog(hDlg, FALSE);
                    return FALSE;

                case IDHELP :
                    WinHelp(hDlg, HELP_FNAME, HELP_CONTEXT, HelpLicenseDlg);
                    return TRUE;

            } /* switch */
            break;
    } /* switch */

    return FALSE;
} /* HelpLicenseDlgProc() */



#pragma argsused  /* disable "Parameter is never used" Warning */
BOOL CALLBACK NoLicenseStartupDlgProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
/* End of Dialog with FALSE if the user press the CANCEL-button */
{
    static unsigned RemainSec;

    switch (msg)
    {
        case WM_INITDIALOG :
            RemainSec = NonLicenseStartupSec;
            SetTimer(h, 1, 1000, NULL);
            SetDlgItemInt(h, IDD_STARTUPDLG_COUNTER, RemainSec, FALSE);
            return TRUE;

        case WM_TIMER:
            SetDlgItemInt(h, IDD_STARTUPDLG_COUNTER, --RemainSec, FALSE);
            SetTimer(h, 1, 1000, NULL);
            if (RemainSec == 0) EndDialog(h, TRUE+1);
            return TRUE;


        case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {
                case IDD_STARTUPDLG_REGBTN:
                    EndDialog(h, TRUE);
                    return TRUE;

                case IDD_STARTUPDLG_INFOBTN:
                    if (FDWinDialogBox(IDD_ABOUTDLG, h, HelpAboutDlgProc))
                        return TRUE;

                             /* else fall through (end of startup dialog) */
                case IDCANCEL:
                    EndDialog(h, FALSE);
                    return TRUE;
            } /* switch */
    } /* switch */
    return FALSE;
}

