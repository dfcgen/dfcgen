#include <TIME.H>
#include "DFCWIN.H"
#include "FDREG.H"


#define FORMAT_LICENSE_SERNO    "%05u-%05u"


/* license data */
static char szLicenseUsr[SIZE_LICENSE_USER+1] = {'\0'};
static char szLicenseCompany[SIZE_LICENSE_COMPANY+1] = {'\0'};
static char szLicenseSerNo[SIZE_LICENSE_SERNO+1] = {'\0'};


static BOOL ChkLicense(char *szSerNo, char *szUsrName);
static WORD CalcCrc(void *pDat, int n);
static BYTE UpdateCrc(BYTE Next, WORD *pCrc);



void GetLicenseData(char *szSerNo, char *szUsr, char *szCompany)
{
    if (IsLicenseOk())
    {
        lstrcpy(szSerNo, szLicenseSerNo);
        lstrcpy(szCompany, szLicenseCompany);
        lstrcpy(szUsr, szLicenseUsr);
    } /* if */
    else
    {
        szSerNo[0] = '\0';
        szUsr[0] = '\0';
        szCompany[0] = '\0';                 
    } /* else */

} /* GetLicenseData() */



/* sets new license data
 * szInstallDate identifies the first installation date (can be NULL)
 */
BOOL SetLicenseData(char *szSerNo, char *szUsr, char *szCompany)
{
    if ((lstrlen(szUsr) > 0) && ChkLicense(szSerNo, szUsr))
    {
        strncpy(szLicenseUsr, szUsr, SIZE_LICENSE_USER+1);
        strncpy(szLicenseCompany, szCompany, SIZE_LICENSE_COMPANY+1);
        strncpy(szLicenseSerNo, szSerNo, SIZE_LICENSE_SERNO+1);
        return TRUE;
    } /* if */

    return FALSE;
} /* SetLicenseData() */



BOOL IsLicenseOk()
{
    return ChkLicense(szLicenseSerNo, szLicenseUsr);
} /* IsLicenseOk() */


void MemCpyCrypt(void *dest, void *src, size_t n)
{
    size_t i;
    BYTE *pSrc = (BYTE *)src;
    BYTE *pDest = (BYTE *)dest;
    WORD Crc = 0xFFFF;

    for (i = 0; i < n; i++, pDest++, pSrc++)
        *pDest  = *pSrc ^ UpdateCrc(0, &Crc); /* crypt with register output */

} /* MemCpyCrypt() */


/********* Locale functions (implementation) ***********/

/* normally checks the passed license parameters
 * but in DEBUG mode it generates the license code !!!
   from (wrong) input data and always returns TRUE
 */
static BOOL ChkLicense(char *szSerNo, char *szUsrName)
{
    char License[SIZE_LICENSE_USER+5]; /* user name + EOS + serial no. + CRC */
    WORD *p;
    int n;

    strncpy(License, szUsrName, SIZE_LICENSE_USER+1);
    if ((n = lstrlen(License)) > 0)                  /* valid user name ? */
    {
        p = (WORD *)&License[n] + 1;                     /* points to CRC */
// !!! BUG: but i cannot fix it (registration code problem)
//      correct: p = (WORD *)(&License[n] + 1);

        if (sscanf(szSerNo, FORMAT_LICENSE_SERNO, p-1, p) == 2)
        {                                        /* valid serial no + CRC */

#if DEBUG /* generate correct registration number CRC */
            *p = ~CalcCrc(&License, (char *)(p) - License);
            SWAP(*(BYTE *)p, *((BYTE *)p + 1));
            sprintf(szSerNo, FORMAT_LICENSE_SERNO, *(p-1), *p);
            MessageBox(hwndFDesk, szSerNo, "Registration Number", MB_ICONINFORMATION|MB_OK);
#endif
            if (CalcCrc(&License, (char *)(++p) - License) == 0x1D0F) return TRUE;
        } /* if */
    } /* if */

    return FALSE;
} /* ChkSerNo() */



/* returns the CRC (CCITT) for the memory block starting with 'pDat'
   of length 'n'
 */
static WORD CalcCrc(void *pDat, int n)
{
    WORD Crc = 0xFFFFU;
    int i;

    for (i = 0; i < n; i++, ((BYTE *)pDat)++)
        (void)UpdateCrc(*((BYTE *)pDat), &Crc);

    return Crc;
} /* CalcCrc() */



#define ORDER_POLY      16             /* degree of generator polynomial */
#define POLYNOM         0x1021         /* CRC-CCITT = 10001000000100001 */
                                       /* CRC-CCITT: g(x) = x^16+x^12+x^5+1 */
                                       /* CRC-12: g(x) = x^12+x^11+x^3+x^2+x+1 */


/* CRC shift register
 * Returns the output from shift register (this is not the CRC !)
 */

static BYTE UpdateCrc(BYTE Next, WORD *pCrc)
{
    BYTE i, Out = 0;

    for (i = 0; i < 8; i++)       /* process all bits starting with bit 7 */
    {
        BYTE Feedback = !(Next & 0x80);             /* Feedback := 0 or 1 */
        if (!(*pCrc & 0x8000)) Feedback = !Feedback;      /* re-inversion */
        *pCrc = (*pCrc << 1) ^ Feedback*POLYNOM;

        Out <<= 1;                       /* output bits of shift register */
        if (*pCrc & 0x8000) Out |= 1;

        Next <<= 1;                     /* shift next data bit into bit 7 */
    } /* for */

    return Out;
} /* UpdateCrc() */

