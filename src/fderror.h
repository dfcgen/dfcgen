#ifndef __FDERROR_H

/* error handler FDERROR.C */


/* limitations */
#define FDMATHERRMAXRESULT      (MAXDOUBLE/100.0) /* math. error max. return val */

typedef void   (*FNSIGNALHANDLER)(int sig, int subtype);

void InitErrorHandler(void);
void ErrorAckUsr(HWND hwndParent, int nIDDMsg);

#define __FDERROR_H
#endif
