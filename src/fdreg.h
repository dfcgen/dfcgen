#ifndef __FDREG_H

#include <TIME.H>
#include "FDGEN.H"


#define SIZE_LICENSE_COMPANY    64
#define SIZE_LICENSE_USER       32
#define SIZE_LICENSE_SERNO      11  /* see also FORMAT_LICENSE_SERNO */


void GetLicenseData(char *szSerNo, char *szUsr, char *szCompany);
BOOL SetLicenseData(char *szSerNo, char *szUsr, char *szCompany);
BOOL IsLicenseOk(void);
void MemCpyCrypt(void *dest, void *src, size_t n);

#define __FDREG_H
#endif
