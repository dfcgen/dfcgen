/* filter designer test module
 * running with Windows 3.1
 * author : Ralf Hoppe                                        
 */


#include "test.h"
#include "dfctls.h"
#include <dir.h>                     /* definition of MAXPATH, MAXEXT, ... */



HWND hwndFDesk, hwndDiagBox, hwndStatusBox, hwndStatus, hwndPosition;


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


/* Filter Designer desktop window callback function */

LRESULT CALLBACK DeskWndProc(HWND hwndDesk, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
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

    } /* switch */

    return DefFrameProc(hwndDesk, hwndDiagBox, msg, wParam, lParam);
}




LRESULT CALLBACK DiagProc(HWND hwndDiag, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefMDIChildProc(hwndDiag, msg, wParam, lParam);
} /* GeneralDiagProc() */




/* filter designer main function */

#pragma argsused  /* disable "Parameter is never used" Warning */
int PASCAL WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int CmdShow)
{
    WNDCLASS wndclass;
    MSG msg;                    /* window manager message */
    HINSTANCE hBwcc;               /* Library Handle */
    UINT wPrevErrMode;

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
    if (hBwcc < HINSTANCE_ERROR) FatalAppExit(0, "BWCC.DLL nicht gefunden");
    #endif

    if (hPrevInstance == (HINSTANCE) 0)
    {
        wndclass.lpszClassName = MAINWINDOW_CLASS;
        wndclass.hInstance     = hinstance;
        wndclass.lpfnWndProc   = DeskWndProc;
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
        wndclass.lpfnWndProc   = DiagProc;
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



    hwndFDesk = CreateWindow(MAINWINDOW_CLASS, "Test",
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             HWND_DESKTOP, LoadMenu(hinstance, "MAINMENU"),          
                             hinstance, NULL);   /* create desktop window */


    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


#if GENBORSTYLE
    FreeLibrary(hBwcc);
#endif
    return 0;
}


