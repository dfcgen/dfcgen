#ifndef __FDPRINT_H

#include "FDGEN.H"

BOOL CALLBACK PrintDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL IsPrinterFdCompatible(void);
void FdPrintDiags(HWND hwndParent, HWND *pWindows);
BOOL CALLBACK FdPrintAbortCallback(HDC dc, int error);


#define __FDPRINT_H
#endif
