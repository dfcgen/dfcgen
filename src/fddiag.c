/* DFCGEN Screen Plot Functions

 * Copyright (c) 1994-2000 Ralf Hoppe

 * $Source: /home/cvs/dfcgen/src/fddiag.c,v $
 * $Revision: 1.2 $
 * $Date: 2000-08-17 12:45:15 $
 * $Author: ralf $
 * History:
   $Log: not supported by cvs2svn $

 */

#include "DFCWIN.H"
#include "FDPRINT.H"


/* generating defs */
#define FORMAT_COORD_CURSOR "%.*lG %s"


/* macros */
#define POWER10(val)   exp(M_LN10*(val))



/* prototypes of local functions */
static char *GetAxisUnitString(tDiagAxis *pAxis);
static char *GetCoordStr(tDiagAxis *, double );
static void SetAxisBounds(tDiagAxis *, double, double);
static int PtAtRectFrame(RECT *pRect, POINT pt);
static int PtAtCommentFrame(pMarker pNote, POINT pt, RECT *pRectNoteAbs);
static void GetPaintRect(HWND h, PRECT prc);
static BOOL IsPaintRectEmpty(HWND h, PRECT p);
static void SetPaintRectEmpty(HWND h);



/* implementation */

static char *GetAxisUnitString(tDiagAxis *pAxis)
{
    static char *ZeroStr = "";

    if (pAxis->lpUnitList == NULL) return ZeroStr;    /* definition made ? */

    return pAxis->lpUnitList[pAxis->iCurrUnit].szUnit;
}


HWND CreateDiagWin(DIAGCREATESTRUCT *pDg)
{
    HWND hwndMDIChild;
    MDICREATESTRUCT mdi;
    char szTitle[128], *szPosNum;
    int nEquDiag;

    LoadString(GetWindowInstance(hwndDiagBox), Diags[pDg->type].nIddDiagName,
               szTitle, DIM(szTitle));
    szPosNum = szTitle + lstrlen(szTitle);

    hwndMDIChild = GetFirstChild(hwndDiagBox);
    nEquDiag = 0;

    while (hwndMDIChild)           /* search all MDI childs for compatible */
    {
        if (GETDIAGTYPE(hwndMDIChild) == pDg->type) ++nEquDiag;
        hwndMDIChild = GetNextSibling(hwndMDIChild);
    } /* while */

    if (nEquDiag > 0)
    {
        hwndMDIChild = GetFirstChild(hwndDiagBox);
        nEquDiag = 0;

        while (hwndMDIChild)
        {
            if (GETDIAGTYPE(hwndMDIChild) == pDg->type)
            {
                wsprintf(szPosNum, ":%d", ++nEquDiag);
                SetWindowText(hwndMDIChild, szTitle);
            } /* if */

            hwndMDIChild = GetNextSibling(hwndMDIChild);
        } /* while */

        wsprintf(szPosNum, ":%d", ++nEquDiag);
    } /* if */

    mdi.szClass = DIAG_CLASS;
    mdi.szTitle = szTitle;
    mdi.x = mdi.y = mdi.cx = mdi.cy = CW_USEDEFAULT;
    mdi.hOwner = GetWindowInstance(hwndDiagBox);
    mdi.style = 0;
    mdi.lParam = (LPARAM)(LPSTR) pDg;
    
    return (HWND)LOWORD(SendMessage(hwndDiagBox, WM_MDICREATE, DUMMY_WPARAM,
                                    (LPARAM)(LPSTR)(&mdi)));
} /* CreateDiagWin() */



/* frees all memory allocated by passed diag (includes GDI objects) */
void FreeDiag(tDiag *pDiag)
{
    pMarker pNote;
    pMarker pFreeNote;

    pNote = pDiag->pM;
    while(pNote != NULL)
    {
        if (pNote->hrgnHotSpot != HRGN_NULL) DeleteRgn(pNote->hrgnHotSpot);
        if (pNote->szTxt != NULL) FREE(pNote->szTxt);
        pFreeNote = pNote;
        pNote = pNote->pNextMarker;
        FREE(pFreeNote);
    } /* while */

    if (pDiag->hrgnCurve != HRGN_NULL) DeleteRgn(pDiag->hrgnCurve);
    if (pDiag->pAppData != NULL) FREE(pDiag->pAppData);    /* release mem */
    FREE(pDiag);
} /* FreeDiag */


/* invalidates all diags matching with passed type
 * so WM_PAINT messages are send to these diags
 */
void UpdateDiags(tFdDiags diag_type)
{
    char szClassName[40];
    HWND hwndMDIChild = GetFirstChild(hwndDiagBox);
    while (hwndMDIChild)
    {
        GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
        if (!lstrcmp(szClassName, DIAG_CLASS))
        {
            if (GETDIAGTYPE(hwndMDIChild) == diag_type)
               InvalidateRect(hwndMDIChild, NULL, TRUE);
        } /* if */

        hwndMDIChild = GetNextSibling(hwndMDIChild);
    } /* while */
} /* UpdateDiags() */



/* converting a real world coordinate to an string with the correct
   unit at the end
 */
static char *GetCoordStr(tDiagAxis *lpAxis, double xy)
{
    static char szCursorPos[80];

    sprintf(szCursorPos, FORMAT_COORD_CURSOR, lpAxis->sPrecision,
            xy/GetScaleFactor(lpAxis), GetAxisUnitString(lpAxis));

    return szCursorPos;
} /* GetCoordStr */


/* sets axis min. and max. coordinates to the new passed values 'dLimit1'
   and 'dLimit2'
 * there are no restrictions to the size of both values (dLimit2 can be
   less or greather than dLimit1)
 */
static void SetAxisBounds(tDiagAxis *pAxis, double dLimit1, double dLimit2)
{
    if (dLimit1 > dLimit2) SwapDouble(&dLimit1, &dLimit2);
    pAxis->dWorldMin = dLimit1;
    pAxis->dWorldMax = dLimit2;
} /* SetAxisBounds() */


void UpdateMDIMenuItems()
{
    char szClassName[40];
    WPARAM mfArrange = MF_GRAYED;
    WPARAM mfIconize = MF_GRAYED;
    WPARAM mfPrinter = MF_GRAYED;


    HWND hwndMDIChild = GetFirstChild(hwndDiagBox);
    HMENU hmMain = GetMenu(hwndFDesk);

    while (hwndMDIChild)
    {
        GetClassName(hwndMDIChild, szClassName, DIM(szClassName));
        if (!lstrcmp(szClassName, DIAG_CLASS))
        {
            if (IsIconic(hwndMDIChild)) mfIconize = MF_ENABLED;
            else mfArrange = MF_ENABLED;

        } /* if */

        if (GETDIAGTYPE(hwndMDIChild) == DIAG_USER_DEF) mfPrinter = MF_ENABLED;

        hwndMDIChild = GetNextSibling(hwndMDIChild);
    } /* while */

    EnableMenuItem(hmMain, IDM_DIAG_CASCADE, mfArrange);
    EnableMenuItem(hmMain, IDM_DIAG_TILE_VERT, mfArrange);
    EnableMenuItem(hmMain, IDM_DIAG_TILE_HORIZ, mfArrange);
    EnableMenuItem(hmMain, IDM_DIAG_REPAINT, mfArrange);
    EnableMenuItem(hmMain, IDM_ICON_ARRANGE, mfIconize);

    if (mfIconize == MF_ENABLED) mfArrange = MF_ENABLED;

    EnableMenuItem(hmMain, IDM_DIAG_CLOSE, mfArrange);
    EnableMenuItem(hmMain, IDM_OPT_AXIS_X, mfArrange);
    EnableMenuItem(hmMain, IDM_OPT_AXIS_Y, mfArrange);

    if ((MainFilter.f_type != NOTDEF) && (mfArrange == MF_ENABLED)) mfPrinter = MF_ENABLED;
    if (!IsPrinterFdCompatible()) mfPrinter = MF_GRAYED; /* master switch */
    EnableMenuItem(hmMain, IDM_PRINT, mfPrinter);

    EnableMenuItem(hmMain, IDM_EDITCOMMENT, MF_GRAYED);   /* default gray */
    if (mfArrange == MF_ENABLED)         /* exist any MDI window (diag) ? */
    {
        hwndMDIChild = (HWND)LOWORD(SendMessage(hwndDiagBox,   /* active Child */
                                                WM_MDIGETACTIVE,
                                                DUMMY_WPARAM, DUMMY_LPARAM));

        if ((GETDIAGTYPE(hwndMDIChild) == DIAG_USER_DEF) ||
            (MainFilter.f_type != NOTDEF))

            EnableMenuItem(hmMain, IDM_EDITCOMMENT, MF_ENABLED);
    } /* if */
} /* ActivateMDIMenuItems */




/* the Hit-Codes are defined in WINDOWS.H
 * but returns only a subset */
static int PtAtRectFrame(RECT *pRect, POINT pt)
{
    if (!PtInRect(pRect, pt)) return HTNOWHERE;

    if (abs(pt.y-pRect->top) <= NOTE_BORDER_Y) return HTTOP;
    if (abs(pt.y-pRect->bottom) <= NOTE_BORDER_Y) return HTBOTTOM;
    if (abs(pt.x-pRect->left) <= NOTE_BORDER_X) return HTLEFT;
    if (abs(pt.x-pRect->right) <= NOTE_BORDER_X) return HTRIGHT;

    return HTCLIENT; /* inside rectangle */
} /* PtAtRectFrame */


/* returns Hit-Codes (note rectangle to point relation) */
static int PtAtCommentFrame(pMarker pNote, POINT pt, RECT *pRectNoteAbs)
{
    RECT rcHotSpot, rcComment;

    if ((GetRgnBox(pNote->hrgnHotSpot, &rcHotSpot) == ERROR) ||
        (pNote->uOpt & MARKER_HIDE) ||
        !(pNote->uOpt & MARKER_FRAME)) return HTNOWHERE;

    CopyRect(&rcComment, &pNote->rcNote);
    OffsetRect(&rcComment, (rcHotSpot.left+rcHotSpot.right)/2,
               (rcHotSpot.top+rcHotSpot.bottom)/2);

    if (pRectNoteAbs != NULL) CopyRect(pRectNoteAbs, &rcComment);

    return PtAtRectFrame(&rcComment, pt);
} /* PtAtCommentFrame */


/* returns the current paint rect of MDI client window */
static void GetPaintRect(HWND h, PRECT prc)
{
    if (IsWindow(h))
    {
        prc->left = GETPAINTRECT(h, 0);
        prc->top = GETPAINTRECT(h, 1);
        prc->right = GETPAINTRECT(h, 2);
        prc->bottom = GETPAINTRECT(h, 3);
    }
    else SetRectEmpty(prc);
} /* GetPaintRect() */


static BOOL IsPaintRectEmpty(HWND h, PRECT p)
{
    RECT rcPaint;

    if (IsWindow(h)) GetPaintRect(h, &rcPaint);
    else SetRectEmpty(&rcPaint);

    if (p != NULL) CopyRect(p, &rcPaint);
    return (IsRectEmpty(&rcPaint));
} /* IsPaintRectEmpty() */



/* sets "no paint in progress" flag for the passed window */
static void SetPaintRectEmpty(HWND h)
{
    if (IsWindow(h))
    {
        SETPAINTRECT(h, 0, 0);
        SETPAINTRECT(h, 1, 0); 
        SETPAINTRECT(h, 2, 0);
        SETPAINTRECT(h, 3, 0);
    } /* if */
} /* SetPaintRectEmpty() */



LRESULT CALLBACK GeneralDiagProc(HWND hwndDiag, UINT msg, WPARAM wParam, LPARAM lParam)
{
    typedef enum {STD = 0, ZOOM = 1, MOVE = 2, TRACK_LEFT = 3,
                  TRACK_RIGHT = 4, TRACK_TOP = 5, TRACK_BOTTOM = 6} TRACKMODE;

    static HDC hdcTrack = HDC_NULL;
    #define ISTRACKING()  (hdcTrack != HDC_NULL)

    static TRACKMODE iCur = STD;      /* index of current cursor in array */
    static HCURSOR aCursor[TRACK_BOTTOM+1] = {0, 0, 0, 0, 0, 0, 0};
    static RECT rcTrack;                /* painting rectangle coordinates */
    static pMarker pNote = NULL;            /* if note text move/tracking */

    static HPEN hpenTrackRect;
    static tDiag *pDiagTrack;     /* only for tracking of comment or zoom */

    static BOOL bPaintRuns = FALSE;


    switch(msg)
    {
        case WM_CREATE:  /* Create the child window to display mouse pos. */
        {
            DIAGCREATESTRUCT *lpDiagCreate =
                (DIAGCREATESTRUCT *)(LPSTR)
                (
                    ((LPMDICREATESTRUCT)
                    (
                        ((LPCREATESTRUCT)(LPSTR)lParam)->lpCreateParams
                    ))->lParam
                );

            SETPDIAG(hwndDiag, lpDiagCreate->lpDiag);
            SETDIAGTYPE(hwndDiag, lpDiagCreate->type);
            SetPaintRectEmpty(hwndDiag);    /* set "no paint in progress" */

            /* load all used cursors */
            if (aCursor[STD] == (HCURSOR)0)
            {
                aCursor[TRACK_TOP] = LoadCursor(GetWindowInstance(hwndDiag), "TOPCUR");
                aCursor[TRACK_BOTTOM] = LoadCursor(GetWindowInstance(hwndDiag), "BOTTCUR");
                aCursor[TRACK_RIGHT] = LoadCursor(GetWindowInstance(hwndDiag), "RIGHTCUR");
                aCursor[TRACK_LEFT] = LoadCursor(GetWindowInstance(hwndDiag), "LEFTCUR");
                aCursor[MOVE] = LoadCursor(GetWindowInstance(hwndDiag), "MOVECUR");
                aCursor[ZOOM] = LoadCursor(GetWindowInstance(hwndDiag), "ZOOMCUR");
                aCursor[STD] = LoadCursor(0, IDC_ARROW);
            } /* if */

            UPDATE_MENU();
            return 0L;
        } /* WM_CREATE */

        case WM_RBUTTONDOWN :
        {
            CREATEAXISNOTEDLGSTRUCT CreateNoteDlg;
            POINT ptClick;
            double YWorld;
            pMarker pSelNote;
                              
            SendMessage(hwndDiag, WM_MBUTTONDOWN, 0, 0); /* force end of zoom/tracking */
            pDiagTrack = GETPDIAG(hwndDiag);
            if (IsIconic(hwndDiag) || (pDiagTrack->hrgnCurve == HRGN_NULL)) break;

            ptClick = MAKEPOINT(lParam);
            if (!PtInRegion(pDiagTrack->hrgnCurve, ptClick.x, ptClick.y)) break;

            CreateNoteDlg.hwndDiag = hwndDiag; /* param to AxisNotedlgProc */
            pSelNote = pDiagTrack->pM;

            while (pSelNote != NULL)
            {
                if (pSelNote->hrgnHotSpot != HRGN_NULL)
                {
                    if (PtInRegion(pSelNote->hrgnHotSpot, ptClick.x, ptClick.y))
                    {
                        CreateNoteDlg.PosX = GetWorldXY(&pDiagTrack->X, ptClick.x);

                        (void)FDWinDialogBoxParam(IDD_AXISNOTESDLG,
                                                  hwndDiag, AxisNoteDlgProc,
						  (LPARAM)(LPSTR)&CreateNoteDlg);
                        return 0L;
                    } /* if */
                } /* if */

                pSelNote = pSelNote->pNextMarker;
            } /* while */

            /* if click point not in hot spot region of any comment, create
               new comment if possible */

            CreateNoteDlg.PosX = GetWorldXY(&pDiagTrack->X, ptClick.x);

            if (!GetWorldYByX(pDiagTrack, &CreateNoteDlg.PosX,
                              fabs(GetWorldXY(&pDiagTrack->X, ptClick.x-CX_HOTSPOT(pDiagTrack)/2) -
                                   GetWorldXY(&pDiagTrack->X, ptClick.x+CX_HOTSPOT(pDiagTrack)/2)),
                              &YWorld))

                break; /* no valid y at point XWorld */

            if ((YWorld < pDiagTrack->Y.dWorldMin) ||
                (YWorld > pDiagTrack->Y.dWorldMax)) break;

            if (abs(GetDevXY(&pDiagTrack->Y, YWorld) - ptClick.y) > CY_HOTSPOT(pDiagTrack)/2)
                break;

            (void)FDWinDialogBoxParam(IDD_AXISNOTESDLG, hwndDiag,
                                      AxisNoteDlgProc, (LPARAM)(LPSTR)&CreateNoteDlg);
            return 0L;
        } /* WM_RBUTTONDOWN */



        case WM_LBUTTONDOWN :
        {
            RECT rectDiag;
            POINT pt;

            SendMessage(hwndDiag, WM_MBUTTONDOWN, 0, 0); /* force end of zoom */
            pDiagTrack = GETPDIAG(hwndDiag);

            if (IsIconic(hwndDiag) || (pDiagTrack->hrgnCurve == HRGN_NULL)) break;

            pt = MAKEPOINT(lParam);

            if (PtInRegion(pDiagTrack->hrgnCurve, pt.x, pt.y))
            {
                pMarker p = pDiagTrack->pM;

                while(p != NULL)         /* hit test with all note frames */
                {
                    if (!(p->uOpt & MARKER_HIDE) && (p->uOpt & MARKER_FRAME))
                    {
                        if ((PtAtCommentFrame(p, pt, &rcTrack) != HTNOWHERE) &&
                            (iCur >= TRACK_LEFT) && (iCur <= TRACK_BOTTOM))
                        {
                            
                            pNote = p;
                            StatusMessage(STRING_TRACK);
                            break; /* while loop */
                        } /* if */

                        if (PtInRect(&rcTrack, pt) && (iCur == MOVE))
                        {
                            pNote = p;   /* save pointer to selected note */
                            iCur = MOVE;      /* set correct cursor index */
                            StatusMessage(STRING_MOVE);
                            break; /* while loop */
                        } /* if */
                    } /* if */
                    p = p->pNextMarker;
                } /* while */

                GetRgnBox(pDiagTrack->hrgnCurve, &rectDiag);

                if (iCur == STD)        /* marker hit test not successful */
                {
                    StatusMessage(STRING_ZOOM);   /* zoom mode invoke */
                    rcTrack.top = rcTrack.bottom = pt.y;
                    rcTrack.left = rcTrack.right = pt.x;
                    if (pDiagTrack->nDiagOpt & DIAG_YAUTORANGE)
                    {
                        rcTrack.top = rectDiag.top;
                        rcTrack.bottom = rectDiag.bottom;
                    } /* if */

                    iCur = ZOOM;
                } /* if */

                SetCursor(aCursor[iCur]);
                ClientToScreenRect(hwndDiag, &rectDiag);
                ClipCursor(&rectDiag);
                hdcTrack = GetDC(hwndDiag);       /* define tracking flag */
                hpenTrackRect = CreatePen(PS_DOT, 1, BLACK);
                SelectPen(hdcTrack, hpenTrackRect);
                SetBkMode(hdcTrack, TRANSPARENT);
                SelectBrush(hdcTrack, GetStockBrush(NULL_BRUSH)); /* don't fill rect */
                SetROP2(hdcTrack, R2_NOTXORPEN);
                Rectangle(hdcTrack, rcTrack.left, rcTrack.top,
                          rcTrack.right, rcTrack.bottom);
            } /* if */
            break;
        } /* WM_LBUTTONDOWN */

        case WM_KEYDOWN :                     
            if (wParam != VK_ESCAPE) break;      /* goto default handling */
                                                     /* else fall through */

        case WM_SYSKEYDOWN:                   /* termination of zoom mode */
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:

        case WM_SYSCOMMAND:                       /* task list, zoom etc. */

            if (ISTRACKING())
            {
                iCur = STD;
                SetCursor(aCursor[STD]);
                Rectangle(hdcTrack, rcTrack.left, rcTrack.top,
                          rcTrack.right, rcTrack.bottom);
                ClipCursor(NULL);
                DeletePen(hpenTrackRect);
                ReleaseDC(hwndDiag, hdcTrack);
                hdcTrack = HDC_NULL;                 /* reset tracking flag */
                StatusMessage(0);
            } /* if */

            break;  /* goto default message handling */


        case WM_LBUTTONUP :               /* possibly correct end of zoom */
            if (ISTRACKING())
            {
                TRACKMODE iOldCursor = iCur;         /* save current mode */

                SendMessage(hwndDiag, WM_MBUTTONDOWN, 0, 0);/* reset mode */
                                                       /* erase rectangle */
                if ((abs(rcTrack.left-rcTrack.right) >=
                     GetSystemMetrics(SM_CXDOUBLECLK)) &&  /* too small ? */
                    (abs(rcTrack.bottom-rcTrack.top) >=
                     GetSystemMetrics(SM_CYDOUBLECLK)))
                {
                    POINT ptUp = MAKEPOINT(lParam);
                                                     
                    switch (iOldCursor)     /* end zoom or note tracking */
                    {
                        case ZOOM :
                            if (!(pDiagTrack->nDiagOpt & DIAG_YAUTORANGE) &&
                                (abs(rcTrack.top-ptUp.y) < GetSystemMetrics(SM_CYDOUBLECLK)))
                                break; /* ZOOM */

                            SetAxisBounds(&pDiagTrack->X,
                                          GetWorldXY(&pDiagTrack->X, rcTrack.left),
                                          GetWorldXY(&pDiagTrack->X, ptUp.x));

                            if (!(pDiagTrack->nDiagOpt & DIAG_YAUTORANGE))
                                SetAxisBounds(&pDiagTrack->Y,
                                              GetWorldXY(&pDiagTrack->Y, rcTrack.top),
                                              GetWorldXY(&pDiagTrack->Y, ptUp.y));
                            break; /* ZOOM */

                        case MOVE :
                        {
                            double YWorld;

                            pNote->uOpt &= ~MARKER_POSAUTO;
                            (void)GetWorldYByX(pDiagTrack, &pNote->x, 0, &YWorld);
                            OffsetRect(&rcTrack, -GetDevXY(&pDiagTrack->X, pNote->x),
                                       -GetDevXY(&pDiagTrack->Y, YWorld));
                            CopyRect(&pNote->rcNote, &rcTrack);
                            break;
                        } /* MOVE */

                        case TRACK_LEFT :
                            pNote->rcNote.left = pNote->rcNote.right -
                                                 (rcTrack.right-rcTrack.left);
                            break; /* TRACK_LEFT */

                        case TRACK_RIGHT :
                            pNote->rcNote.right = pNote->rcNote.left +
                                                 (rcTrack.right-rcTrack.left);
                            break; /* TRACK_RIGHT */

                        case TRACK_TOP :
                            pNote->rcNote.top = pNote->rcNote.bottom -
                                                (rcTrack.bottom-rcTrack.top);
                            break; /* TRACK_TOP */

                        case TRACK_BOTTOM :
                            pNote->rcNote.bottom = pNote->rcNote.top +
                                                   (rcTrack.bottom-rcTrack.top);
                            break; /* TRACK_BOTTOM */
                    } /* switch iCursor */

                    InvalidateRect(hwndDiag, NULL, TRUE);
                } /* if not too small */ 
            } /* if tracking in progress */

            return 0L; /* WM_LBUTTONUP */

        case WM_NCMOUSEMOVE :
            SetWindowText(hwndPosition, "");       /* outside diag region */
            break; /* and call DefMDIChildProc() */


        case WM_MOUSEMOVE :
        {
            tDiag *pDiag = GETPDIAG(hwndDiag);

            if (!IsIconic(hwndDiag) && (pDiag->hrgnCurve != HRGN_NULL))
            {
                POINT ptMouse = MAKEPOINT(lParam);

                if (PtInRegion(pDiag->hrgnCurve, ptMouse.x, ptMouse.y))
                {
                    char szXYPos[128], szXPos[128];
                    tDiagAxis *lpAxisX = &pDiag->X;
                    tDiagAxis *lpAxisY = &pDiag->Y;

                    if (ISTRACKING())   /* zoom/note tracking in progress */
                    {                                   /* erase old rect */
                        Rectangle(hdcTrack, rcTrack.left, rcTrack.top,
                                  rcTrack.right, rcTrack.bottom);
                        SetCursor(aCursor[iCur]);  /* don't change cursor */

                        switch(iCur)
                        {
                            case ZOOM :
                                if (!(pDiagTrack->nDiagOpt & DIAG_YAUTORANGE))
                                    rcTrack.bottom = ptMouse.y;

                                rcTrack.right = ptMouse.x;
                                break; /* ZOOM */

                            case TRACK_RIGHT :
                                if (ptMouse.x >= rcTrack.left+NOTE_BORDER_X)
                                    rcTrack.right = ptMouse.x;
                                else
                                    SetCursorPosClient(hwndDiag, rcTrack.left+NOTE_BORDER_X, ptMouse.y);
                                break; /* TRACK_RIGHT */

                            case TRACK_LEFT :
                                if (ptMouse.x <= rcTrack.right-NOTE_BORDER_X)
                                    rcTrack.left = ptMouse.x;
                                else
                                    SetCursorPosClient(hwndDiag, rcTrack.right-NOTE_BORDER_X, ptMouse.y);
                                break; /* TRACK_LEFT */

                            case TRACK_TOP :
                                if (ptMouse.y <= rcTrack.bottom-NOTE_BORDER_Y)
                                    rcTrack.top = ptMouse.y;
                                else
                                    SetCursorPosClient(hwndDiag, ptMouse.x, rcTrack.bottom-NOTE_BORDER_Y);
                                break; /* TRACK_TOP */

                            case TRACK_BOTTOM :
                                if (ptMouse.y >= rcTrack.top+NOTE_BORDER_Y)
                                    rcTrack.bottom = ptMouse.y;
                                else
                                    SetCursorPosClient(hwndDiag, ptMouse.x, rcTrack.top+NOTE_BORDER_Y);
                                break; /* TRACK_BOTTOM */

                            case MOVE :
                                OffsetRect(&rcTrack,
                                           ptMouse.x - (rcTrack.right+rcTrack.left)/2,
                                           ptMouse.y - (rcTrack.bottom+rcTrack.top)/2);
                                break; /* MOVE */

                        } /* switch */

                        Rectangle(hdcTrack, rcTrack.left, rcTrack.top,
                                  rcTrack.right, rcTrack.bottom);
                    } /* if */
                    else  /* no tracking in progress, mouse inside region */
                    {                   /* change cursur if on note frame */
                        iCur = STD;
                        pNote = pDiag->pM;

                        while((pNote != NULL) && (iCur == STD))
                        {                              /* check all notes */
                            switch (PtAtCommentFrame(pNote, ptMouse, NULL))
                            {

                                case HTLEFT :   iCur = TRACK_LEFT; break;
                                case HTRIGHT :  iCur = TRACK_RIGHT; break;
                                case HTTOP :    iCur = TRACK_TOP; break;
                                case HTBOTTOM : iCur = TRACK_BOTTOM; break;
                                case HTCLIENT : iCur = MOVE; break;
                            } /* switch */

                            pNote = pNote->pNextMarker;
                        } /* while */

                        if (iCur == STD) SetCursor(aCursor[STD]);
                        else SetCursor(aCursor[iCur]);
                    } /* else (no input tracking mode) */

                    lstrcpy(szXPos,
                            GetCoordStr(lpAxisX,
                                        GetWorldXY(lpAxisX, ptMouse.x)));

                    sprintf(szXYPos, "%s, %s", szXPos,
                            GetCoordStr(lpAxisY,
                                        GetWorldXY(lpAxisY, ptMouse.y)));

                    SetWindowText(hwndPosition, szXYPos);
                    return 0L;
                } /* if (point in curve region) */
            } /* if */

            SetWindowText(hwndPosition, ""); /* iconic | region not exist */
            iCur = STD;                            /* set standard cursor */
            SetCursor(aCursor[STD]);
            return 0L;
        } /* WM_MOUSEMOVE */


        case WM_QUERYDRAGICON :
            return MAKELRESULT(LoadIcon(GetWindowInstance(hwndDiag),
                                        Diags[GETDIAGTYPE(hwndDiag)].szIconName),
                               0);

        case WM_PAINT :                                  /* paint diagram */
        {
            RECT rectClient;
            PAINTSTRUCT ps;
            tDiag *pDiag;
            BOOL bOkPaint;

            if (IsIconic(hwndDiag))
            {
                SetPaintRectEmpty(hwndDiag);     /* set update rect empty */
                BeginPaint(hwndDiag, &ps);
                DrawIcon(ps.hdc, 0, 0,
                         LoadIcon(GetWindowInstance(hwndDiag),
                                  Diags[GETDIAGTYPE(hwndDiag)].szIconName));
                EndPaint(hwndDiag, &ps);
                SetWindowText(hwndPosition, "");
                return 0L;
            } /* if */

            if ((MainFilter.f_type == NOTDEF) &&
                (GETDIAGTYPE(hwndDiag) != DIAG_USER_DEF)) break;

            if (!bPaintRuns)       /* painting in progress ? -> recursion */
            {
                HWND hSibling;

                pDiag = GETPDIAG(hwndDiag);
                BeginPaint(hwndDiag, &ps);

                GetClientRect(hwndDiag, &rectClient);   /* get whole rect */
                pDiag->X.nScreenMin = rectClient.left;
                pDiag->X.nScreenMax = rectClient.right;
                pDiag->Y.nScreenMax = rectClient.bottom;
                pDiag->Y.nScreenMin = rectClient.top;

                if (pDiag->nDiagOpt & DIAG_DISCRET)
                    pDiag->cxySmplCurve = cxyBmpSamples;
                else pDiag->cxySmplCurve = nWidthCurvePen;

                WorkingMessage(STRING_WAIT_PAINT);

                EnableMainMenu(hwndFDesk, FALSE);         /* disable menu */
                EnableMenuItem(GetSystemMenu(hwndFDesk, FALSE), SC_CLOSE,
                               MF_GRAYED|MF_BYCOMMAND);

                aCursor[STD] = LoadCursor(0, IDC_WAIT);

                hSibling = GetFirstChild(GetParent(hwndDiag));
                while (hSibling)    /* empty all MDI childs invalid rects */
                {
                    SetPaintRectEmpty(hSibling);   /* set rectangle empty */
                    hSibling = GetNextSibling(hSibling);
                } /* while */

                bPaintRuns = TRUE;        /* set "paint in progress" flag */

                bOkPaint = PaintDiag(GetWindowInstance(hwndDiag), ps.hdc,
                                     UserBreak, pDiag, alInstColor);

                EndPaint(hwndDiag, &ps);

                hSibling = GetFirstChild(GetParent(hwndDiag));

                while (hSibling)            /* search for invalid regions */
                {
                    RECT rcRepaint;

                    if (!IsPaintRectEmpty(hSibling, &rcRepaint))
                    {
                        InvalidateRect(hSibling, &rcRepaint, TRUE);
                        SetPaintRectEmpty(hSibling);   /* set rectangle empty */
                    } /* if */

                    hSibling = GetNextSibling(hSibling);
                } /* while */

                bPaintRuns = FALSE;

                EnableMainMenu(hwndFDesk, TRUE);
                EnableMenuItem(GetSystemMenu(hwndFDesk, TRUE), SC_CLOSE,
                               MF_ENABLED|MF_BYCOMMAND);

                WorkingMessage(0);
                aCursor[STD] = LoadCursor(0, IDC_ARROW);

                if (!bOkPaint)                    /* don't call MessageAckUsr() */
                    StatusMessage(ERROR_DIAGRAM); /* because recursion possible */
            } /* if */
            else    
            {        /* maybe a recursion, but PaintDiag is not reeentrant */
                RECT rc1, rc2;
                PAINTSTRUCT psRecur;

                BeginPaint(hwndDiag, &psRecur);
                GetPaintRect(hwndDiag, &rc1); /* accumul. invalid regions */
                UnionRect(&rc2, &psRecur.rcPaint, &rc1);
                EndPaint(hwndDiag, &psRecur); /* validate clipping region */

                SETPAINTRECT(hwndDiag, 0, rc2.left);    /* update invalid */
                SETPAINTRECT(hwndDiag, 1, rc2.top); /* region (rectangle) */
                SETPAINTRECT(hwndDiag, 2, rc2.right);
                SETPAINTRECT(hwndDiag, 3, rc2.bottom);
            } /* else */

            return 0L;
        } /* WM_PAINT */

        case WM_MDIACTIVATE :
            if (GETDIAGTYPE(hwndDiag) == DIAG_USER_DEF)
                EnableMenuItem(GetMenu(hwndFDesk), IDM_OPT_USERFN,
                               wParam ? MF_ENABLED : MF_GRAYED);
            return 0L; /* WM_MDIACTIVATE */

        case WM_SIZE :
            UPDATE_MENU();
            break; /* WM_SIZE */


        case WM_CLOSE:
            if (bPaintRuns) return 0L;       /* no close during WM_PAINT! */
            break; /* WM_CLOSE */


        case WM_DESTROY :
            FreeDiag(GETPDIAG(hwndDiag));
            UPDATE_MENU();
            return 0L; /* WM_DESTROY */

    } /* switch */

    return DefMDIChildProc(hwndDiag, msg, wParam, lParam);
} /* GeneralDiagProc() */


