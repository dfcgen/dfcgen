// ObjectWindows - (C) Copyright 1991,1992 by Borland International

//
// BWCC.H
//

// Purpose:    Borland Windows Custom Controls  (BWCC)


#if !defined(__BWCC_H)
#define __BWCC_H

#if !defined(WORKSHOP_INVOKED)

#if !defined(__WINDOWS_H)
#include <windows.h>

#endif

#endif

#define BWCCVERSION 0x0103   // version 1.03

// from version 1.02 onward BWCCGetversion returns a DWORD
// The low-order word contains the version number
// and the high-order word contains the locale

#define BWCC_LOCALE_US     1
#define BWCC_LOCALE_JAPAN  2


#define BORDLGCLASS "BorDlg"  // Our Custom Dialog class
#define BORDLGPROP  "FB"      // Borland dialog window uses
                              // this property for instance data
                              // users should not use a property
                              // with this name!

#define IDHELP    998         // Id of help button

// button style definitions:

// the Borland buttons use Windows button styles for button
// type: i.e. BS_PUSHBUTTON/BS_DEFPUSHBUTTON


#define BUTTON_CLASS  "BorBtn"  // Our Bitmap Buttons
#define RADIO_CLASS "BorRadio"  // Our Radio Buttons
#define CHECK_CLASS "BorCheck"  // Our Check Boxes

// styles

#define BBS_BITMAP       0x8000L  // this is a bitmap static
#define BBS_PARENTNOTIFY 0x2000L  // Notify parent of TAB keys and focus
#define BBS_OWNERDRAW    0x1000L  // let parent paint via WM_DRAWITEM

// messages

#define BBM_SETBITS       ( BM_SETSTYLE + 10)

// notifications

#define BBN_SETFOCUS      ( BN_DOUBLECLICKED + 10)
#define BBN_SETFOCUSMOUSE ( BN_DOUBLECLICKED + 11)
#define BBN_GOTATAB       ( BN_DOUBLECLICKED + 12)
#define BBN_GOTABTAB      ( BN_DOUBLECLICKED + 13)

#define SHADE_CLASS "BorShade"

// The following is the name of the window message passed to
// RegisterWindowMessage for CtlColor processing for group box shades:
#define BWCC_CtlColor_Shade "BWCC_CtlColor_Shade"

#define BSS_GROUP     1L  // recessed group box
#define BSS_HDIP      2L  // horizontal border
#define BSS_VDIP      3L  // vertical border
#define BSS_HBUMP     4L  // horizontal speed bump
#define BSS_VBUMP     5L  // vertical speed bump
#define BSS_RGROUP    6L  // raised group box

#define BSS_CAPTION   0x8000L // Set off the caption
#define BSS_CTLCOLOR  0x4000L // Send WM_CTLCOLOR messages to parent of control
#define BSS_NOPREFIX  0x2000L // & in caption does not underline following letter
#define BSS_LEFT      0x0000L // Caption is left-justified
#define BSS_CENTER    0x0100L // Caption is centered
#define BSS_RIGHT     0x0200L // Caption is right-justified
#define BSS_ALIGNMASK 0x0300L

#define STATIC_CLASS  "BorStatic" // Our statics

#if !defined(EXPORT)
#define EXPORT _export
#endif

#if defined( __cplusplus )
extern "C" {
#endif  /* __cplusplus */

extern HGLOBAL FAR EXPORT PASCAL SpecialLoadDialog
(

  HINSTANCE   hResMod,
  LPCSTR      lpResName,
  DLGPROC     fpDlgProc
);

extern HGLOBAL FAR EXPORT PASCAL MangleDialog
(
  HGLOBAL     hDlg,
  HINSTANCE   hResources,
  DLGPROC     fpDialogProc
);

extern LRESULT FAR EXPORT PASCAL BWCCDefDlgProc
(
  HWND        hWnd,
  UINT        message,
  WPARAM      wParam,
  LPARAM      lParam
);
extern LRESULT FAR EXPORT PASCAL BWCCDefWindowProc
(
  HWND        hWnd,
  UINT        message,
  WPARAM      wParam,
  LPARAM      lParam
);

extern LRESULT FAR EXPORT PASCAL BWCCDefMDIChildProc
(
  HWND        hWnd,
  UINT        message,
  WPARAM      wParam,
  LPARAM      lParam
);

extern int FAR EXPORT PASCAL BWCCMessageBox
(
  HWND        hWndParent,
  LPCSTR      lpText,
  LPCSTR      lpCaption,
  UINT        wType
);

extern HBRUSH FAR EXPORT PASCAL BWCCGetPattern( void );
extern DWORD FAR EXPORT PASCAL BWCCGetVersion( void);

#if defined( __cplusplus )
}
#endif  /* __cplusplus */

#endif  /* __BWCC_H */

