/* DFCGEN Help Dialogs

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/fdhelp.c,v $
 * $Revision: 1.3 $
 * $Date: 2000-08-17 12:45:22 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $

 */

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


            LoadString(GetWindowInstance(hDlg), FORMAT_ABOUT_INFO,
                       szFormatAbout, DIM(szFormatAbout));  /* load format */

            wsprintf(szAboutText, szFormatAbout, pFileInfo[0], pFileInfo[1],
                     pFileInfo[2], pFileInfo[3], pFileInfo[4],
                     GetFreeSpace(0)/1024UL,
                     GetFreeSystemResources(GFSR_SYSTEMRESOURCES));

            SetDlgItemText(hDlg, IDD_ABOUT_TEXT, szAboutText);
            return TRUE;
        } /* WM_INITDIALOG */


	    case WM_COMMAND :
            switch (GET_WM_COMMAND_IDCTL(wParam, lParam))
            {                             /* wait until OK button pressed */
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




