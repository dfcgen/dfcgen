/* DFCGEN dialog controls

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/dfctls.c,v $
 * $Revision: 1.2 $
 * $Date: 2000-08-17 12:45:09 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $

 */

#include "dfcwin.h"


/* defines */
#define GETHRGNINC(hwnd)        (HRGN)(LPSTR)GetWindowWord(hwnd, 0)
#define GETHRGNDEC(hwnd)        (HRGN)(LPSTR)GetWindowWord(hwnd, sizeof(DWORD))
#define GETHWNDEDIT(hwnd)       (HWND)(LPSTR)GetWindowWord(hwnd, 2*sizeof(DWORD))

#define SETHRGNINC(hwnd, hrgn)   SetWindowLong(hwnd, 0, (DWORD)(LPSTR)(hrgn))
#define SETHRGNDEC(hwnd, hrgn)   SetWindowLong(hwnd, sizeof(DWORD), (DWORD)(LPSTR)(hrgn))
#define SETHWNDEDIT(hwnd, h)     SetWindowLong(hwnd, 2*sizeof(DWORD), (DWORD)(LPSTR)(h))



/* prototypes */


static void FillGrayRect(HDC dc, int x1, int y1, int x2, int y2);
static void PaintButton(HDC dc, COLORREF colBtn, HRGN hrgnButton, BOOL bPointDown);
static void SysLineTo(HDC dc, int x, int y, int cxy, int nSysColor);



/* implementation */

void InitDfcgenDlgControls(HINSTANCE hinstance)
{
    WNDCLASS wndclass;

    wndclass.lpszClassName = STATUS_CLASS;
    wndclass.lpfnWndProc   = StatusWndProc;
    wndclass.hInstance     = hinstance;
    wndclass.hCursor       = NULL;
    wndclass.hIcon         = NULL;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.lpszMenuName  = NULL;
    wndclass.hbrBackground = (HBRUSH)0;            /* sets own background */
    wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;
    RegisterClass(&wndclass);

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = IncDecEditWndProc;
    wndclass.hCursor       = LoadCursor(0, IDC_ARROW);
    wndclass.lpszClassName = INCDECEDIT_CLASS;
    wndclass.cbWndExtra    = 2*sizeof(DWORD) + sizeof(DWORD);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wndclass);
} /* InitDfcgenDlgControls() */



static void SysLineTo(HDC dc, int x, int y, int cxy, int nSysColor)
{
    HPEN hp = SelectPen(dc, CreatePen(PS_SOLID /*PS_INSIDEFRAME*/, cxy, GetSysColor(nSysColor)));
    LineTo(dc, x, y);
    DeletePen(SelectPen(dc, hp));
} /* LineToSysCol() */


/* fills rectangle with the color 'COLOR_BTNFACE' including the border of
   the passed rectangle
 */
static void FillGrayRect(HDC dc, int x1, int y1, int x2, int y2)
{
    HBRUSH hbrOld = SelectBrush(dc, CreateSolidBrush(GetSysColor(COLOR_BTNFACE)));
    HPEN hpOld = SelectPen(dc, GetStockPen(NULL_PEN));

    /* note : rectangle fills not the right and bottom line, means fills
              only the rectangle x1, y1, x1-1, y2-1
     */
    Rectangle(dc, x1, y1, x2+1, y2+1);
    SelectPen(dc, hpOld);
    DeleteBrush(SelectBrush(dc, hbrOld));
} /* FillGrayRect */




/* window function of shadowed status windows */

LRESULT CALLBACK StatusWndProc(HWND hwndStat, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_ERASEBKGND:
        {
            RECT rcWin;
            int cy;

            GetClientRect(hwndStat, &rcWin);        /* size of client area */
            cy = GetSystemMetrics(SM_CYBORDER);
            FillGrayRect((HDC)wParam, rcWin.left, rcWin.top, rcWin.right,
                         rcWin.bottom);         /* erase whole client area */

            rcWin.right -= cy; rcWin.bottom -= cy;   /* subtract pen width */
            MoveTo((HDC)wParam, rcWin.left, rcWin.bottom);
            SysLineTo((HDC)wParam, rcWin.right, rcWin.bottom, cy, COLOR_BTNHIGHLIGHT);
            SysLineTo((HDC)wParam, rcWin.right, rcWin.top, cy, COLOR_BTNHIGHLIGHT);
            SysLineTo((HDC)wParam, rcWin.left, rcWin.top, cy, COLOR_BTNSHADOW);
            SysLineTo((HDC)wParam, rcWin.left, rcWin.bottom, cy, COLOR_BTNSHADOW);
            return TRUE;
        } /* WM_ERASEBKGND */

        case WM_SETTEXT:
        {
            char szWndTxt[256];

            GetWindowText(hwndStat, szWndTxt, DIM(szWndTxt)-1);

            if (lstrcmp(szWndTxt, (LPSTR)lParam))
            {
                /* pass window text to DefWindowProc, means WND struct
                   by the help of DefWindowProc */
                FDWINDEFWINDOWPROC(hwndStat, msg, wParam, lParam);

                /* and update window direct (recursive call to StatusWndProc)
                 * note : only the invalidation isn't sufficiently if
                   a function processes WM_PAINT and don't give back the
                   control to windows, which would pass WM_PAINT to
                   StatusWndProc
                 */
                InvalidateRect(hwndStat, NULL, TRUE);
                UpdateWindow(hwndStat);     /* call wnd proc with WM_PAINT */
                return 0L;
            } /* if */

            break;  
        } /* WM_SETTEXT */
            

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rcWin;
            char szWndTxt[256];
            int cy  = GetSystemMetrics(SM_CYBORDER);

            GetClientRect(hwndStat, &rcWin);
            InflateRect(&rcWin, -cy, -cy);          /* do not erase border */

            /* note : 'BeginPaint' forces automatic WM_ERASEBKGND if the
                      field ps.fErase is TRUE
             */
            BeginPaint(hwndStat, &ps);
             
            rcWin.left += GetSystemMetrics(SM_CXFRAME);

            if ((rcWin.right > rcWin.left) && (rcWin.bottom > rcWin.top))
            {
                GetWindowText(hwndStat, szWndTxt, DIM(szWndTxt)-1);
                SetBkMode(ps.hdc, TRANSPARENT);
                DrawText(ps.hdc, szWndTxt, -1, &rcWin,
                         DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            } /* if */
            EndPaint(hwndStat, &ps);
            return 0L;
        } /* WM_PAINT */

        case WM_MOUSEMOVE :
        case WM_NCMOUSEMOVE :
            SetWindowText(hwndPosition, "");       /* outside diag region */
            break;
    } /* switch */

    /* call 'BWCCDefWindowProc' if generating for Borland dialog style, because
       fills also the backround with correct pattern (gray shaded Borland dialogs)
     */
    return FDWINDEFWINDOWPROC(hwndStat, msg, wParam, lParam);
} /* StatusWndProc() */



/******************* incremental button class support *********************/



static void PaintButton(HDC dc, COLORREF colBtn, HRGN hrgnButton, BOOL bPointDown)
{
    RECT rcButton;
    HBRUSH hbr;
    HPEN hp;
    POINT pt[3];
    int xdist;

    GetRgnBox(hrgnButton, &rcButton);

    xdist = GetSystemMetrics(SM_CXBORDER);
    
    /* paint rectangular button border
       note : rectangle fills not the right and bottom line, means fills
              only the rectangle x1, y1, x1-1, y2-1
     */


    hp = SelectPen(dc, CreatePen(PS_SOLID, GetSystemMetrics(SM_CXBORDER),
                                 GetSysColor(COLOR_WINDOWFRAME)));
    hbr = SelectBrush(dc, CreateSolidBrush(GetSysColor(COLOR_BTNFACE)));
    Rectangle(dc, rcButton.left, rcButton.top, rcButton.right+1, rcButton.bottom+1);
    DeleteBrush(SelectBrush(dc, hbr));
    DeletePen(SelectPen(dc, hp));

    InflateRect(&rcButton, -xdist, -xdist);              /* button shadow */
    MoveTo(dc, rcButton.left, rcButton.bottom);
    SysLineTo(dc, rcButton.left, rcButton.top, xdist, COLOR_BTNHIGHLIGHT);
    SysLineTo(dc, rcButton.right, rcButton.top, xdist, COLOR_BTNHIGHLIGHT);
    SysLineTo(dc, rcButton.right, rcButton.bottom, xdist, COLOR_BTNSHADOW);
    SysLineTo(dc, rcButton.left, rcButton.bottom, xdist, COLOR_BTNSHADOW);

    InflateRect(&rcButton, -2*xdist, -2*xdist);
    pt[0].x = (rcButton.right+rcButton.left)/2;      /* center, top point */
    pt[0].y = rcButton.top;
    pt[1].x = rcButton.left;                        /* left, bottom point */
    pt[1].y = rcButton.bottom;     
    pt[2].x = rcButton.right;                      /* right, bottom point */
    if (bPointDown) SWAP(pt[0].y, pt[1].y);
    pt[2].y = pt[1].y ;

    hbr = SelectBrush(dc, CreateSolidBrush(colBtn));
    Polygon(dc, pt, DIM(pt));
    DeleteBrush(SelectBrush(dc, hbr));
} /* PaintButton() */


static LRESULT NotifyMsgIncDec(HWND hCtl, LPARAM NotifyCode)
{
    HWND hDlg = GetParent(hCtl);

    SetFocus(GETHWNDEDIT(hCtl));

    return SendMessage(hDlg, WM_COMMAND, GetDlgCtrlID(hCtl),
                       MAKELPARAM((WPARAM)GETHWNDEDIT(hCtl), NotifyCode));
} /* NotifyMsgIncDec() */



/* window function of edit fields with Inc/Dec Buttons */

LRESULT CALLBACK IncDecEditWndProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            HWND hEditChild;

            LPCREATESTRUCT p = (LPCREATESTRUCT)(LPSTR)lParam;

            (void)SetWindowLong(hEdit, GWL_STYLE,
                                GetWindowStyle(hEdit) | WS_CLIPCHILDREN);

            hEditChild = CreateWindow("EDIT", p->lpszName,
                                      p->style|WS_BORDER, 0, 0, p->cx, p->cy,
                                      hEdit, NULL, p->hInstance, DUMMY_LPARAM);
            SETHWNDEDIT(hEdit, hEditChild);
            break;
        } /* WM_CREATE */


        case WM_DESTROY:
            DeleteRgn(GETHRGNINC(hEdit));
            DeleteRgn(GETHRGNDEC(hEdit));
            DestroyWindow(GETHWNDEDIT(hEdit));
            break; /* WM_DESTROY */

        case WM_COMMAND:
        /* at first check possibility source is EDIT window (child), means
           this is a notification message (EN_xxxx codes)
         * if so set the correct dialog control ID in wParam (because wParam
           is zero) and pass to popup dialog window (parent)
         */ 
            if (GET_WM_COMMAND_HWNDCTL(wParam, lParam)   
                == GETHWNDEDIT(hEdit))                /* from EDIT window */
                return SendMessage(GetParent(hEdit), msg,
                                   GetDlgCtrlID(hEdit), lParam);

         /* all other messages reveiving from popup dialog (parent) are
            EM_xxxx control codes (fall through) */

        case WM_NEXTDLGCTL:
        case WM_SETTEXT:
        case WM_GETTEXT:
        case WM_GETTEXTLENGTH:
        case WM_COPY:
        case WM_PASTE:
        case WM_CUT:
            return SendMessage(GETHWNDEDIT(hEdit), msg, wParam, lParam);

//        case WM_GETDLGCODE: return DLGC_HASSETSEL|DLGC_WANTARROWS;


        case WM_ENABLE:
            InvalidateRgn(hEdit, GETHRGNDEC(hEdit), FALSE);
            InvalidateRgn(hEdit, GETHRGNINC(hEdit), FALSE);
            return SendMessage(GETHWNDEDIT(hEdit), msg, wParam, lParam);


        case WM_SETFOCUS:                        /* pass focus to child window */
            SetFocus(GETHWNDEDIT(hEdit));   /* invert input (see ES_NOHIDESEL) */
            PostMessage(GETHWNDEDIT(hEdit), EM_SETSEL, DUMMY_WPARAM, MAKELPARAM(0, 32767));
            return 0L;


      case WM_LBUTTONDOWN:
       {
            POINT ptMouse = MAKEPOINT(lParam);

            if (PtInRegion(GETHRGNDEC(hEdit), ptMouse.x, ptMouse.y))
                return NotifyMsgIncDec(hEdit, EN_MOUSE_DEC);

            if (PtInRegion(GETHRGNINC(hEdit), ptMouse.x, ptMouse.y))
                return NotifyMsgIncDec(hEdit, EN_MOUSE_INC);
            break;
        } /* WM_LBUTTONDOWN */


        case WM_SIZE:
        {
            RECT rcEdit;
            int cx, cy;

            DeleteRgn(GETHRGNINC(hEdit));
            DeleteRgn(GETHRGNDEC(hEdit));
            GetWindowRect(hEdit, &rcEdit);
            ScreenToClientRect(hEdit, &rcEdit);
            OffsetRect(&rcEdit, -rcEdit.left, -rcEdit.top); /* width/height */

            cy = rcEdit.bottom - 6*GetSystemMetrics(SM_CXBORDER);
            cx = cy*GetSystemMetrics(SM_CXVSCROLL)/GetSystemMetrics(SM_CYVSCROLL)/2;
            if (!ODD(cx)) ++cx;    /* because symmetric triangle required */
            cx += 6*GetSystemMetrics(SM_CXBORDER);

            MoveWindow(GETHWNDEDIT(hEdit), 0, 0,
                       rcEdit.right-cx, rcEdit.bottom, TRUE);

            SETHRGNINC(hEdit, CreateRectRgn(rcEdit.right-cx, 0,
                                            rcEdit.right-1, rcEdit.bottom/2));
            SETHRGNDEC(hEdit, CreateRectRgn(rcEdit.right-cx, (rcEdit.bottom+1)/2,
                                            rcEdit.right-1, rcEdit.bottom-1));
            return 0L;
        } /* WM_SIZE */


        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            COLORREF colBtn = GetSysColor(COLOR_BTNTEXT);

            if (!IsWindowEnabled(hEdit)) colBtn = GetSysColor(COLOR_BTNSHADOW);

            /* note : 'BeginPaint' forces automatic WM_ERASEBKGND if the
                      field ps.fErase is TRUE
             */
            BeginPaint(hEdit, &ps);
            PaintButton(ps.hdc, colBtn, GETHRGNINC(hEdit), FALSE);
            PaintButton(ps.hdc, colBtn, GETHRGNDEC(hEdit), TRUE);
            EndPaint(hEdit, &ps);
            return 0L;
        } /* WM_PAINT */
    }

    return FDWINDEFWINDOWPROC(hEdit, msg, wParam, lParam);
} /* IncDecEditWndProc() */

