#ifndef __FDUTIL_H

#include "FDGEN.H" /* special operating system dependencies */
#include "DIAGS.H"


#ifdef _Windows
#define MALLOC(size)            fd_malloc(size)
#define FREE(ptr)               fd_free(ptr)
#define REALLOC(ptr, size)      fd_realloc(ptr, size)

#define CLOSE(handle)           _lclose(handle)
#define OPEN(path, access)      _lopen(path, access)
#endif

#ifdef __UNIX__
#define MALLOC(size)            malloc(size)
#define FREE(ptr)               free(ptr)
#define REALLOC(ptr)            realloc(ptr)

#define CLOSE(handle)           close(handle)
#define OPEN(path, access)      open(path, access)
#endif


/* macros */
#define DIM(array)             (sizeof(array)/sizeof(array[0]))
#define STRINGER(val)          #val                   /* stringizing macro */
#define UPDATEFLAG(Flags, SetValue, bToSet)  \
    ((bToSet) ? ((Flags) |= (SetValue)) : ((Flags) &= ~(SetValue)))
#define SWAP(var1, var2)        {var1^=var2; var2^=var1; var1^=var2;}
#define MAKEWORD(l, h)          ((unsigned)((h)<<8) | (l))


/* defines */
#define HWND_NULL    HWND_DESKTOP /* this is (HWND)0 in WINDOWS.H */
#define HDC_NULL     ((HDC) 0)


/* windows colors */
#define BLACK        RGB(0, 0, 0)
#define BLUE         RGB(0, 0, 128)
#define GREEN        RGB(0, 128, 0)
#define CYAN         RGB(0, 128, 128)  /* green and blue */
#define RED          RGB(128, 0, 0)
#define MAGENTA      RGB(128, 0, 128)  /* red and blue */
#define BROWN        RGB(128, 128, 0)  /* red and green = yellow */
#define LIGHTGRAY    RGB(128, 128, 128)
#define DARKGRAY     RGB(64, 64, 64)
#define LIGHTBLUE    RGB(0, 0, 255)
#define LIGHTGREEN   RGB(0, 255, 0)
#define LIGHTCYAN    RGB(0, 255, 255)
#define LIGHTRED     RGB(255, 0, 0)
#define LIGHTMAGENTA RGB(255, 0, 255)
#define YELLOW       RGB(255, 255, 0)
#define WHITE        RGB(255, 255, 255)




/* parameter struct for MultipleSendDlgItemMessage() */
typedef struct
{ UINT   wMsg;                   /* message */
  WPARAM wParamMsg;              /* word parameter */
  LPARAM lParamMsg;              /* dword parameter */
} tSendItemMsgRec;




/* dialog support functions from FDUTIL.C */
BOOL GetRCVerStringInfo(char *, char *, int);
LPSTR LoadRCdata(HINSTANCE hInst, int rcId);
void FreeRCdata(void);

void SwapDouble(double *, double *);
int  FDWinDialogBox(int, HWND, DLGPROC);
int  FDWinDialogBoxParam(int, HWND, DLGPROC, LPARAM);
HWND FDWinCreateModelessDlg(LPCSTR lpTemplateName, HWND hwndParent,
                            DLGPROC fnDialog);
void ClientToScreenRect(HWND, RECT *);
void ScreenToClientRect(HWND hwndClient, RECT *lprect);
void SetCursorPosClient(HWND hw, int x, int y);
void DeleteChars(char *, char *);   /* erases special chars in a string */
void SetWindowTextId(HWND, int);
void MessageAckUsr(HWND, int);
BOOL UserOkQuestion(HWND, int);
void StatusMessage(int);
void WorkingMessage(int);
BOOL UserBreak(void);
void SetDlgItemFloat(HWND, int, double, int);
void SetDlgItemFloatUnit(HWND hDlg, int nIdDlgItem, double val, int nPrec,
                         int iUnit, tUnitDesc aUnit[]);
void SetDlgItemTextId(HWND hDlg, int nIddDlgItem, int nIddText);


/* return values of GetDlgItemFloat */
#define CVT_OK                 0
#define CVT_ERR_OVERFLOW       -1  /* number too big (ovf or out of range) */
#define CVT_ERR_UNDERFLOW      -2       /* number too small (out of range) */
#define CVT_ERR_EMPTY          -3                    /* no chars in buffer */
#define CVT_ERR_SYNTAX         -4    /* additional characters after number */

int GetDlgItemFloat(HWND, int, double, double, double*);


BOOL HandleDlgError(HWND hDlgParent, int iDlgItemId, int iErrNo);
BOOL HandleDlgNumInpErr(HWND hDlg, int IdEditDlgItem, int IdStaticDlgItem,
                        int CvtErrType, double MinVal, double MaxVal);
BOOL CheckDlgItemFloat(HWND hDlg, int IdEditDlgItem, int IdStaticDlgItem,
                       double MinVal, double MaxVal, double *DestVar);
BOOL ChangeIncDecInt(HWND h, WPARAM wParam, LPARAM lParam,
                     int nMin, int nMax, int nDefault);
BOOL CheckDlgItemInt(HWND hDlg, int IdEditDlgItem, int IdStaticDlgItem,
                     int rmin, int rmax, int *val);
void ShowDlgItem(HWND, int, BOOL);
int GetDlgListCurSel(HWND hDlg, int nIddComboList);
BOOL DrawListFloat(HWND, int nIDDList, int nOrder, double Vec[], int nPrec);
BOOL DrawBoxIdStrings(HWND hDlg, int nIdListItem, int iCurr, int nNo, int *IdVec);
BOOL DrawComboAxisUnits(HWND hDlg, int nIddUnitBox, int nIddUnitText,
                        int iCurrSel, tUnitDesc *pUnit);
void EnableDlgItem(HWND, int, BOOL);
void EnableMenuItems(HMENU hm, int num, int aId[], BOOL bEnable);
void EnableMainMenu(HWND hwndMain, BOOL bEnable);
void SetDlgFocus(HWND hDlg, int nIdd);

/* file I/O */
BOOL CALLBACK OpenFileDlgProc(HWND, UINT, WPARAM, LPARAM);


/* radio button management */
typedef struct
{
    int nSel;             /* selection number in application (enum types) */
    int nIDDRadio;  /* ID of button in dialog ressource (0 = end of list) */
} tRadioBtnDesc;

int GetSelRadioBtn(HWND hDlg, tRadioBtnDesc aBtnDesc[]);
void CheckSelRadioBtn(HWND hDlg, int nCurrSel, tRadioBtnDesc aBtnDesc[]);

void MultipleSendDlgItemMessage(HWND, int, int, tSendItemMsgRec *);
void *MallocErr(HWND hwndParent, size_t size);
void *fd_malloc(size_t);
void fd_free(void *);
void *fd_realloc (void *, size_t);
void InitAbortDlg(HWND, UINT);
void ChangeAbortString(LPSTR szNewAbortStr);
BOOL ChkAbort(void);
void EndAbortDlg(void);
void FDWinMsgHandler(LPMSG);



#define __FDUTIL_H
#endif
