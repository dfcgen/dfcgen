#ifndef __DFCTLS_H



void InitDfcgenDlgControls(HINSTANCE h);
LRESULT CALLBACK StatusWndProc(HWND hwndDesk, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK IncDecEditWndProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam);

/* additional notification codes for incremental edit fields
 * defines with respect to windows edit field notify codes
 */
#define EN_MOUSE_INC    0x1000
#define EN_MOUSE_DEC    0x1001

#define __DFCTLS_H
#endif
