#ifndef __FDSYS_H

#include "FDTYPES.H"

/* prototypes of functions from FDSYS.C */
void UpdateMainWinTitle(void);
BOOL WriteProjectDataFile(char *szFileName);
int ReadProjectDataFile(char *szFileName, BOOL bLoadPrj);
BOOL WriteIniFile(char *szFileName);
int ReadIniFile(char *szFileName);

#define __FDSYS_H
#endif
