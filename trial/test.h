
#ifndef __TEST_H

#define WINVER                 0x030A
#define DEBUG                  FALSE
#define LIB   /* version management static functions (no DLL) -> see VER.H */
#define STRICT
#define GENBORSTYLE            TRUE  /* generate with Borland style dialog */
#define GENFPEOPT              FALSE    /* user defined FPE error handling */

#include <WINDOWSX.H>   /* includes #define <windows.h> + message crackers */
#include <VER.H>


#if GENBORSTYLE
#include "BWCC.H"       /* Borland DLL (BWCC.DLL) prototypes & defines */
                        /* Do not use old version from BCW/TCW 3.1 */

/* 'BWCCDefWindowProc' is the default function for Borland dialogs internal
 * uses shaded Borland dialog style background for window background
 * do use only for self defined user controls inserted in Borland dialogs
 */
#define FDWINDEFWINDOWPROC  BWCCDefWindowProc

#define FDWINMESSAGEBOX     BWCCMessageBox

#else                       /* windows standard styles */

#define FDWINDEFWINDOWPROC  DefWindowProc
#define FDWINMESSAGEBOX     MessageBox
#endif


#include <stdlib.h>     /* supports exit values (i.e. EXIT_FAILURE ... ) */
#include <stdio.h>
#include <string.h>

#include "dfctls.h"

/* Windows compatibility defines and castings */
#define MIN_WINDOWS_VERSION     0x03U           /* running with 3.1 upward */
#define MIN_WINDOWS_RELEASE     0x0AU

#define DUMMY_WPARAM            ((WPARAM) 0)
#define DUMMY_LPARAM            ((LPARAM) 0)



/* Macro's */
#define GET_WM_SCROLL_POS(wParam, lParam)       (int)LOWORD(lParam)
#define GET_WM_COMMAND_IDCTL(wParam, lParam)    (UINT)wParam
#define GET_WM_COMMAND_NOTIFY(wParam, lParam)   (UINT)HIWORD(lParam)
#define GET_WM_COMMAND_HWNDCTL(wParam, lParam)  (HWND)LOWORD(lParam)


#define DEFAULT_PRECISION      6

/* my own classes */
#define STATUS_CLASS        "Test_Stat"
#define INCDECEDIT_CLASS    "Test_IncDecEdit"
#define DIAG_CLASS          "Test_Diags"
#define MAINWINDOW_CLASS    "Test_Main"


#define DIM(array)             (sizeof(array)/sizeof(array[0]))
#define STRINGER(val)          #val                   /* stringizing macro */
#define UPDATEFLAG(Flags, SetValue, bToSet)  \
    ((bToSet) ? ((Flags) |= (SetValue)) : ((Flags) &= ~(SetValue)))
#define SWAP(var1, var2)        {var1^=var2; var2^=var1; var1^=var2;}
#define MAKEWORD(l, h)          ((unsigned)((h)<<8) | (l))


/* defines */
#define HWND_NULL    HWND_DESKTOP /* this is (HWND)0 in WINDOWS.H */
#define HDC_NULL     ((HDC) 0)

#define ODD(number)            ((((number)>>1)<<1) != number)
#define SIGN(x)                (((x) < 0) ? -1 : 1)


/* prototypes of import/export functions (public) */

extern HWND hwndFDesk, hwndDiagBox, hwndStatusBox, hwndStatus, hwndPosition;



#define APPNAME                 "DFCGEN"         /* application name */
#define HELP_FNAME              APPNAME ".HLP"   /* help file name */


/* strings should'nt contain in ressource file */
#define ERROR_NOMEMORY "Unzureichender Speicherplatz !!! Fortsetzen ?"
#define ERROR_WINDOWS_VERSION "Falsche Windows Version (kleiner 3.1) !!!"



#define IDSTRNULL        0             /* dummy */
#define IDSTROUTOFMEMORY 1             /* memory error (pseudo string id) */


#if GENBORSTYLE             /* Borland dialog style */
#define ERROR_LOAD_LIBRARY "Bibliothek BWCC.DLL konnte nicht geladen werden !!!"

/* STATIC_CLASS, BUTTON_CLASS, RADIO_CLASS are defined in BWCC.H */
#define FDWINDLG_CLASS      CLASS "BorDlg"

#else                       /* standard Windows 3.1 style */
#define STATIC_CLASS        "static"
#define FDWINDLG_CLASS
#define BUTTON_CLASS        "Button"
#define RADIO_CLASS         "Button"
#define CHECK_CLASS         "Button"
#endif



/* ressources defines (also for ressources compiler) */

/* BITMAP's */
#define IDBMP_POINT                    11
#define IDBMP_ZERO                     12
#define IDBMP_POLE                     13

/* Icons's */
#define IDICO_MAIN                     11
#define IDICO_ATT                      12
#define IDICO_DELAY                    13
#define IDICO_DIAG                     14
#define IDICO_GRP                      15
#define IDICO_IMPULS                   16
#define IDICO_MATH                     17
#define IDICO_PHASE                    18
#define IDICO_STEP                     19
#define IDICO_APPROX                   20




/* STRINGTABLE */
#define STRING_MAINWINTITLE            100
#define STRING_MESSAGE_TITLE           101
#define STRING_ENDFILTERPROGRAM        103
#define STRING_COEFF_DELETE            104
#define STRING_COEFF_GAIN1             105
#define STRING_COEFF_ROUNDING          106
#define STRING_FRESP_DIAG              107
#define STRING_ATTENUATION_DIAG        108
#define STRING_APPROXRESP_DIAG         109
#define STRING_IRESP_DIAG              110
#define STRING_STEPRESP_DIAG           111
#define STRING_PHASE_DIAG              112
#define STRING_PHASEDELAY_DIAG         113
#define STRING_GROUPDELAY_DIAG         114
#define STRING_USERDEF_DIAG            115
#define STRING_OVERWRITE_DATA          116
#define STRING_WAIT_ROOT               117
#define STRING_WAIT_PAINT              118
#define STRING_STATUS_ROOTS_CALC       119
#define STRING_ZOOM                    120
#define STRING_TRACK                   121
#define STRING_MOVE                    122
#define STRING_SAVE_FILE               123
#define STRING_READ_FILE               124
#define STRING_SAVE_INIFILE            125
#define STRING_LOAD_INIFILE            126
#define STRING_SYS_STABIL              127
#define STRING_SYS_INSTABIL            128
#define STRING_STATUS_DEF_CHANGED      129
#define STRING_STATUS_NOT_SAVED        130
#define STRING_STATUS_SAVED            131


#define STRING_AXIS_FREQUENCY          140
#define STRING_AXIS_TIME               141
#define STRING_AXIS_MAGNITUDE          142
#define STRING_AXIS_ATTENUATION        143
#define STRING_AXIS_IMPULS_RESP        144
#define STRING_AXIS_STEP_RESP          145
#define STRING_AXIS_PHASE              146
#define STRING_AXIS_PHASEDELAY         147
#define STRING_AXIS_GROUPDELAY         148
#define STRING_AXIS_APPROXCHARAC       149
#define STRING_AXIS_Y                  150
#define STRING_AXIS_X                  151
#define STRING_AXISNAME_FREQUENCY      152
#define STRING_AXISNAME_TIME           153
#define STRING_AXISNAME_MAGNITUDE      154
#define STRING_AXISNAME_X              155
#define STRING_AXISNAME_ATTENUATION    156
#define STRING_AXISNAME_IMPULS_RESP    157
#define STRING_AXISNAME_STEP_RESP      158
#define STRING_AXISNAME_PHASE          159
#define STRING_AXISNAME_PHASEDELAY     160
#define STRING_AXISNAME_GROUPDELAY     161
#define STRING_AXISNAME_APPROXCHARAC   162

#define STRING_AXISNAME_Y              163
#define STRING_AXIS_Z_REAL             164
#define STRING_AXIS_Z_IMAG             165


#define STRING_FTRANSFORM_DLG_CUTOFF   166  /* var text in f-transform dialog */
#define STRING_FTRANSFORM_DLG_BANDWIDTH        167
#define STRING_TITLE_QUESTION          168

#define STRING_NON_WIN                 169
#define STRING_HAMMING_WIN             170
#define STRING_KAISER_WIN              171
#define STRING_HANNING_WIN             172
#define STRING_BLACKMAN_WIN            173
#define STRING_NON_TRANSF              174
#define STRING_LPHP_TRANSF             175
#define STRING_LPBP_TRANSF             176
#define STRING_LPBS_TRANSF             177

#define STRING_STDIIRDLG_MINATTENUATION        178
#define STRING_STDIIRDLG_RIPPLEATTENUATION     179
#define STRING_WAIT_BESSEL_FILTER      180
#define STRING_WAIT_CAUER_FILTER       181
#define STRING_WAIT_INTEGRATOR         182

#define STRING_ABORT_ROOTS             183
#define STRING_ABORT_BESSEL            184
#define STRING_ABORT_CAUER             185


#define STRING_DIAGITEM_SCALEVALS      200
#define STRING_DIAGITEM_SCALELINES     201
#define STRING_DIAGITEM_CURVE          202
#define STRING_DIAGITEM_FRAME          203
#define STRING_DIAGITEM_NOTES          204
#define STRING_DIAGITEM_NOTEFRAME      205
#define STRING_DIAGITEM_AXISNAME       206

#define STRING_PRNINFO                 210
#define STRING_PRNPAGE                 211
#define STRING_PRNHEADDIAGTYPE         212
#define STRING_PRNHEADPROJECT          213
#define STRING_PRNHEADFILENAME         214
#define STRING_PRNHEADPRJDESC          215



#define STRING_OKINPLICENSE            220


#define STRING_USERFN_SIN              300
#define STRING_USERFN_COS              301
#define STRING_USERFN_TAN              302
#define STRING_USERFN_COS2             303
#define STRING_USERFN_SQR              304
#define STRING_USERFN_SINH             305
#define STRING_USERFN_COSH             306
#define STRING_USERFN_TANH             307
#define STRING_USERFN_EXP              308
#define STRING_USERFN_SI               309
#define STRING_USERFN_SI2              310
#define STRING_USERFN_HAMMING          311
#define STRING_USERFN_HANNING          312
#define STRING_USERFN_BLACKMAN         313
#define STRING_USERFN_BESSEL           314
#define STRING_USERFN_TRIANGLE         315
#define STRING_USERFN_RECTANGLE        316
#define STRING_USERFN_CHEBY            317
#define STRING_USERFN_COMPL_ELLIPTIC1  318
#define STRING_USERFN_ELLIPTIC_INTEGR1 319
#define STRING_USERFN_JACOBI_SN        320
#define STRING_USERFN_JACOBI_CN        321
#define STRING_USERFN_JACOBI_DN        322
#define STRING_USERFN_INTEGRSIN        323

#define STRING_PARAM_ORDER             370
#define STRING_PARAM_ELLIPTIC_MODUL    371



/* misc digital systems names */
#define STRING_HILBERT90               400
#define STRING_DIFFERENTIATOR          401
#define STRING_INTEGRATOR              402
#define STRING_COMBFILTER              403
#define STRING_AVGFIR                  404
#define STRING_AVGIIR                  405
#define STRING_AVGEXP                  406






/* error string codes */
#define ERROR_FILTER_IMPLEMENT         500
#define ERROR_COEFF0_BAD               501
#define ERROR_COEFFN_BAD               502
#define ERROR_USR_ACK                  503
#define ERROR_FLOAT_OP                 504
#define ERROR_SIGINT                   505
#define ERROR_SIGTERM                  506
#define ERROR_SIGABRT                  507
#define ERROR_SIGSEGV                  508
#define ERROR_DIAGRAM                  509
#define ERROR_INTOVFLOW                510
#define ERROR_INTDIV0                  511
#define ERROR_INVALID87                512
#define ERROR_FLOATDIV0                513
#define ERROR_FLOATOVFLOW              514
#define ERROR_FLOATUNFLOW              515
#define ERROR_INEXACT                  516
#define ERROR_FPESUCCIGN               517
#define ERROR_ODD_ORDER_HPBP           518
#define ERROR_ODD_ORDER_FRDEF          519
#define ERROR_HILBERT_ORDER            520
#define ERROR_BESSEL_USR_BREAK         521
#define ERROR_CHANGE_PATHSPEC          522
#define ERROR_CAUER_USR_BREAK          523
#define ERROR_CAUER_RIPPLEGT3DB        524
#define ERROR_CAUER_MINATTLT3DB        525
#define ERROR_CVT_OVERFLOW             526
#define ERROR_CVT_UNDERFLOW            527
#define ERROR_CVT_EMPTY                528
#define ERROR_CVT_SYNTAX               529
#define ERROR_CENTER_F_NOT_VALID       530
#define ERROR_CUTOFF_BW_NOT_VALID      531
#define ERROR_NOTEATX_SING             532
#define ERROR_LOAD_FILTER_DATA         533
#define ERROR_FILEWRITE                534
#define ERROR_FILEREAD_CLOSE           535
#define ERROR_FILEREAD_OPEN            536
#define ERROR_FILEREAD_VERSION         537
#define ERROR_FILEREAD                 538
#define ERROR_FILEREAD_MEM             539
#define ERROR_FILEREAD_INCOMPLETE      540
#define ERROR_FILEREAD_INCMEM          541
#define ERROR_INIFILEWRITE             542
#define ERROR_APPMISMATCH              543
#define ERROR_PRINTOPTNOTAVAIL         544
#define ERROR_PRINTDRVINFO             545
#define ERROR_PRINTDOC                 546
#define ERROR_LICENSE                  547
#define ERROR_INPLICENSE               548

#define ERROR_TITLE                    550

#define HINT_NOLICENSE                 700



#define FORMAT_ABOUT_INFO              800
#define FORMAT_ABOUT_INFO_NOLICENSE    801


/* menu items of main menu */
#define IDM_LOADFILE           101
#define IDM_LOADPROJECT        102
#define IDM_SAVEFILE           103
#define IDM_SAVEFILE_AS        104
#define IDM_PRINT              105
#define IDM_EXIT               106

#define IDM_NEWLINFIR          120
#define IDM_NEWSTDIIR          121
#define IDM_NEWFIRIIR          122
#define IDM_NEWPREDEF          123
#define IDM_DEFCHANGE          124
#define IDM_EDITCOEFF          125
#define IDM_EDITPROJECT        126
#define IDM_EDITCOMMENT        127

#define IDM_SHOWCOEFF          140
#define IDM_SHOWROOTS          141

/* options menu */
#define IDM_OPT_TECH	       160
#define IDM_OPT_DESK	       161
#define IDM_OPT_AXIS_X	       162
#define IDM_OPT_AXIS_Y	       163
#define IDM_OPT_USERFN         164

#define IDM_DIAG_TILE_VERT             400
#define IDM_DIAG_TILE_HORIZ            401
#define IDM_DIAG_CASCADE               402
#define IDM_DIAG_CLOSE                 403
#define IDM_DIAG_REPAINT               404
#define IDM_ICON_ARRANGE               405
#define IDM_NEW_DIAG_FRESP             406
#define IDM_NEW_DIAG_ATTENUAT          407
#define IDM_NEW_DIAG_APPROXRESP        408
#define IDM_NEW_DIAG_IMPULSRESP	       409
#define IDM_NEW_DIAG_STEPRESP          410
#define IDM_NEW_DIAG_PHASE             411
#define IDM_NEW_DIAG_PHASEDELAY	       412
#define IDM_NEW_DIAG_GROUPDELAY	       413
#define IDM_NEW_DIAG_USERDEF           414

#define IDM_FIRST_CHILD                415


#define IDM_HELP_HELP           600
#define IDM_HELP_ABOUT          601
#define IDM_HELP_INDEX          602
#define IDM_HELP_REGISTER       603


/* dialog ressources */

/* standard buttons (IDOK .. IDNO normally defined in windows.h) */
#ifndef IDOK
#define IDOK                   1
#endif

#ifndef IDCANCEL
#define IDCANCEL               2
#endif

#ifndef IDABORT
#define IDABORT                3
#endif

#ifndef IDRETRY
#define IDRETRY                4
#endif

#ifndef IDIGNORE
#define IDIGNORE               5
#endif

#ifndef IDYES
#define IDYES                  6
#endif

#ifndef IDNO
#define IDNO                   7
#endif



#ifndef IDHELP                  /* normally in BWCC.H */
#define IDHELP                 998
#endif


                               /* common used dialog buttons */
#define IDCLEAR                995
#define IDNEW                  996
#define IDCLOSE                997



/* common usage in various dlg-boxes are 200- id's */
#define IDD_DLG_CUTOFF           201
#define IDD_DLG_CUTOFF_TEXT      202
#define IDD_DLG_TRANSF_TEXT      203
#define IDD_FREQUENCY_UNITBOX    204
#define IDD_FREQUENCY_UNIT_TEXT  205
#define IDD_SAMPLE_RATE          206
#define IDD_SAMPLE_RATE_TEXT     207
#define IDD_FILTER_ORDER	     208  /* filter order edit window */
#define IDD_FILTER_ORDER_TEXT    209
#define IDD_F_TRANSFORM	         210  /* frequency transfomation button */

/* multiple use axis options dialog elements */
#define IDD_OPTAXISDLG_FROM      220
#define IDD_OPTAXISDLG_TO        221
#define IDD_OPTAXISDLG_UNITBOX   222
#define IDD_OPTAXISDLG_PRECISION 223
#define IDD_OPTAXISDLG_PREC_TEXT 224
#define IDD_OPTAXISDLG_LOG       225
#define IDD_OPTAXISDLG_TO_TEXT   226
#define IDD_OPTAXISDLG_FROM_TEXT 227
#define IDD_OPTAXISDLG_UNIT_TEXT 228
#define IDD_OPTAXISDLG_SETALL    229
#define IDD_OPTAXISDLG_DOTLINES  230


/* filter definition dialog window for linear FIR-Filter */
#define IDD_LINFIRDLG          10

#define IDD_WINDOWING          104
#define IDD_RADIO_RESPONSE     105
#define IDD_LINFIRDLG_RECTLP   106
#define IDD_LINFIRDLG_GAUSS    107
#define IDD_LINFIRDLG_COS      108
#define IDD_LINFIRDLG_COS2     109
#define IDD_LINFIRDLG_SQR      110
#define IDD_LINFIRDLG_WIN_TEXT 112

/* digital filter coefficients dialog window */
#define COEFFDLG               11
#define IDD_COEFF_LIST_ZERO    101
#define IDD_COEFF_LIST_POLE    102
#define IDD_COEFF_ROUNDING     103
#define IDD_COEFF_DELETE       105
#define IDD_COEFF_NORM         106
#define IDD_COEFF_FACTOR       107
#define IDD_COEFF_MODIFY       108
#define IDD_GAIN_VAL           109
#define IDD_COEFF_NUM_ZERO     110
#define IDD_COEFF_NUM_POLE     111


#define COEFFMODIFYDLG         12
#define IDD_CHANGE_VAL         101
#define IDD_CHANGE_VAL_TEXT    102
#define IDD_CHANGE_INDEX       103

#define COEFFFACTORDLG         13
#define IDD_FACTOR_VAL         101
#define IDD_FACTOR_TEXT        102

#define COEFFNORMDLG           14
#define IDD_FREQUENCY_VAL      101
#define IDD_FREQUENCY_TEXT     102
#define IDD_NORM_UNITBOX       103


#define IDD_AXISNOTESDLG       15
#define IDD_NOTEBOX_POSX       100
#define IDD_NOTEPOSX_UNITBOX   101
#define IDD_NOTEPOSX_UNIT_TEXT 102
#define IDD_NOTE_POSY_TEXT     103
#define IDD_NOTE               104
#define IDD_NOTE_AXISNAME_X    105
#define IDD_NOTE_AXISNAME_Y    106
#define IDD_NOTE_HPOS_LEFT     107
#define IDD_NOTE_HPOS_RIGHT    108
#define IDD_NOTE_HPOS_CENTER   109
#define IDD_NOTE_VPOS_TOP      110
#define IDD_NOTE_VPOS_BOTTOM   111
#define IDD_NOTE_VPOS_CENTER   112
#define IDD_NOTE_OPT_OPAQUE    113
#define IDD_NOTE_OPT_HIDE      114
#define IDD_NOTE_OPT_FRAME     115
#define IDD_NOTE_OPT_POSAUTO   116
#define IDD_NOTE_VPOS_TEXT     117
#define IDD_NOTE_HPOS_TEXT     118


#define ROOTSDLG               16
#define IDD_ROOT_LIST_ZERO     101
#define IDD_ROOT_LIST_POLE     102
#define IDD_STABILITY          103
#define IDD_ROOT_PN            104
#define IDD_ROOT_CALCNEW       105


#define IDD_OPTTECHDLG	               17
#define IDD_OPTTECH_NULLCOEFF_TEXT     100
#define IDD_OPTTECH_NULLCOEFF          101
#define IDD_OPTTECH_NULLROOT_TEXT      102
#define IDD_OPTTECH_NULLROOT           103
#define IDD_OPTTECH_ERRPHASE	       104
#define IDD_OPTTECH_ERRPHASE_TEXT      105
#define IDD_OPTTECH_ERRROOTS           106
#define IDD_OPTTECH_ERRROOTS_TEXT      107
#define IDD_OPTTECH_ERRSI              108
#define IDD_OPTTECH_ERRSI_TEXT         109
#define IDD_OPTTECH_ERRBESSEL          110
#define IDD_OPTTECH_ERRBESSEL_TEXT     111
#define IDD_OPTTECH_ERRELLIPTIC        112
#define IDD_OPTTECH_ERRELLIPTIC_TEXT   113
#define IDD_OPTTECH_ERRJACOBISN        114
#define IDD_OPTTECH_ERRJACOBISN_TEXT   115
#define IDD_OPTTECH_ERRKAISER	       116
#define IDD_OPTTECH_ERRKAISER_TEXT     117
#define IDD_OPTTECH_SETSTD             118
#define IDD_OPTTECH_IGNFPE             119


#define IDD_OPTDESKDLG                 18
#define IDD_OPTDESK_PHASE180           100
#define IDD_OPTDESK_PHASE360           101
#define IDD_OPTDESK_PHASE_EXT          102
#define IDD_OPTDESK_COLORRECT          103
#define IDD_OPTDESK_COLORMONITOR       104
#define IDD_OPTDESK_COLORNAMES         105
#define IDD_OPTDESK_GETSTDCOLORS       106
#define IDD_OPTDESK_PIXMAP_SIZE        107
#define IDD_OPTDESK_PIXMAP_TEXT        108
#define IDD_OPTDESK_ROOTS_BMPSIZE      109
#define IDD_OPTDESK_ROOTS_BMPSIZE_TEXT 110
#define IDD_OPTDESK_CURVE_TEXT         111
#define IDD_OPTDESK_CURVE_SIZE         112
#define IDD_OPTDESK_OUTPREC_ROOT       113
#define IDD_OPTDESK_OUTPREC_ROOT_TEXT  114
#define IDD_OPTDESK_OUTPREC_COEFF      115
#define IDD_OPTDESK_OUTPREC_COEFF_TEXT 116
#define IDD_OPTDESK_OUTPREC_FREQU      117
#define IDD_OPTDESK_OUTPREC_FREQU_TEXT 118
#define IDD_OPTDESK_OUTPREC_GAIN       119
#define IDD_OPTDESK_OUTPREC_GAIN_TEXT  120
#define IDD_OPTDESK_OUTPREC_ATT        121
#define IDD_OPTDESK_OUTPREC_ATT_TEXT   122
#define IDD_OPTDESK_OUTPREC_OTHER      123
#define IDD_OPTDESK_OUTPREC_OTHER_TEXT 124



#define IMPULSWINDLG                   19
#define IDD_RADIO_WINDOWING            100
#define IDD_IMPWIN_NONWIN              101
#define IDD_IMPWIN_KAISER              102
#define IDD_IMPWIN_HAMMING             103
#define IDD_IMPWIN_HANNING             104
#define IDD_IMPWIN_BLACKMAN            105
#define IDD_KAISER_ALPHA               106
#define IDD_KAISER_ALPHA_TEXT          107


#define FTRANSFORMDLG                  20
#define IDD_RADIO_TRANSFORM            100
#define IDD_TRANS_NON                  101
#define IDD_TRANS_HIGHPASS             102
#define IDD_TRANS_BANDPASS             103
#define IDD_TRANS_NOTCH                104
#define IDD_TRANS_CUTOFFBW             105
#define IDD_TRANS_CENTER               106
#define IDD_TRANS_CUTOFF_TEXT          107
#define IDD_TRANS_CENTER_TEXT          108
#define IDD_TRANS_UNITBOX              109
#define IDD_TRANS_UNITTEXT             110
#define IDD_TRANS_CENTER_GEOMETRIC	   111



/* filter definition dialog window for Standard IIR-Filter */
#define IDD_STDIIRDLG              21   /* standard IIR dialog window */
#define IDD_STDIIRDLG_MINATT       101  /* min. attenuation edit window */
#define IDD_STDIIRDLG_RIPPLEATT    102  /* min. attenuation edit window */
#define IDD_STDIIR_LPTYPE	       106  /* radio box */
#define IDD_STDIIRDLG_BTWLP	       107  /* Butterworth button */
#define IDD_STDIIRDLG_CHEBY1	   108  /* Chebyshev I button */
#define IDD_STDIIRDLG_CHEBY2	   109  /* Chebyshev II button */
#define IDD_STDIIRDLG_CAUER1       110
#define IDD_STDIIRDLG_CAUER2       111
#define IDD_STDIIRDLG_BESSEL	   112  /* Bessel button */
#define IDD_STDIIRDLG_MINATT_TEXT  113
#define IDD_STDIIRDLG_RIPATT_TEXT  114
#define IDD_STDIIRDLG_MINATT_DBTXT 115
#define IDD_STDIIRDLG_RIPATT_DBTXT 116
#define IDD_STDIIRDLG_PARAMTXT     117
#define IDD_STDIIRDLG_MODULE_ANGLE 118
#define IDD_STDIIR_MODULE_TEXT     119
#define IDD_STDIIR_MODDEG_TEXT     120


#define ABORTDLG                   22   /* generic modeless abort dialog */
#define IDD_ABORT_COMMENT          101


#define IDD_PROJECTDLG             23
#define IDD_PROJECT_DESC           101
#define IDD_PROJECT_NAME           102
#define IDD_PROJECT_STATUS         103
#define IDD_PROJECT_FILE           104
#define IDD_PROJECT_DELETE         105


#define IDD_PREDEFDLG              24
#define IDD_PREDEF_TYPEBOX         101
#define IDD_PREDEF_DESC            102


#define IDD_MISCFLTDLG              25
#define IDD_PREDEF_PAR1_NAME       101
#define IDD_PREDEF_PAR1_EDIT       102
#define IDD_PREDEF_PAR1_UNIT       103
#define IDD_PREDEF_PAR2_NAME       104
#define IDD_PREDEF_PAR2_EDIT       105
#define IDD_PREDEF_PAR2_UNIT       106
#define IDD_MISC_TYPEBOX           107


/* definition of x axis dialog window */
#define IDD_OPTAXISXDLG	            26
#define IDD_OPTAXISDLG_CONST_BTN   111
#define IDD_OPTAXISDLG_CONST_SAMPLES 112

/* definition of y axis dialog window */
#define IDD_OPTAXISYDLG	            27
#define IDD_OPTAXISDLG_AUTOSCALE   105

#define IDD_MATHFUNCDLG             28
#define IDD_USERFN_BOX             100
#define IDD_USERFN_PARA            101
#define IDD_USERFN_PARA_TEXT       102

#define IDD_FILEDLG                 29
#define IDD_FILEDLG_FNAME          100
#define IDD_FILEDLG_FILES          101
#define IDD_FILEDLG_PATH           102
#define IDD_FILEDLG_DIRS           103


#define IDD_PRINTDLG                30
#define IDD_PRINTDEVICE            100
#define IDD_PRINTSEL               101
#define IDD_PRINTALL               102
#define IDD_PRINTSETUP             103


#define IDD_ABOUTDLG                31
#define IDD_ABOUT_TEXT             100
#define IDD_ABOUT_REGISTER         101


#define IDD_LICENSEDLG             32
#define IDD_LICENSEDLG_NAME        100
#define IDD_LICENSEDLG_COMPANY     101
#define IDD_LICENSEDLG_SN          102


#define IDD_STARTUPDLG             33
#define IDD_STARTUPDLG_COUNTER     100
#define IDD_STARTUPDLG_REGBTN      101
#define IDD_STARTUPDLG_INFOBTN	   102



/* RC-DATA Ressources */
#define IDRC_NUMPREDEF             1024 /* ID of ressource containing the */
                                        /* number of predefined filter in */
                                        /* RC-file */

#define COEFF_INTEGR_RECT          1025   /* integrator (rectangular rule) */
#define COEFF_INTEGR_TRAPEZ        1026   /* integrator (trapezoidal rule) */
#define COEFF_INTEGR_SIMPSON       1027   /* integrator (Simpson rule) */
#define COEFF_INTEGR_38            1028   /* integrator (3/8 rule) */
#define COEFF_DIFF_SIMPLE          1029   /* diferentiator 1. order */
#define COEFF_DIFF2                1030   /* diferentiator 2. order */
#define COEFF_FITT4                1031   /* fitting filter 4. order */
#define COEFF_FITT6_POLY2          1032   /* fitting filter 6. order type 1 */
#define COEFF_FITT6_POLY4          1033   /* fitting filter 6. order type 2 */
#define COEFF_FITT8_POLY2          1034   /* fitting filter 8. order type 1 */
#define COEFF_FITT8_POLY4          1035   /* fitting filter 8. order type 2 */
#define COEFF_FITT14               1036   /* fitting filter 14. order type 1 */
#define COEFF_FITT20               1037   /* fitting filter 20. order type 1 */


#define __TEST_H
#endif /* ifdef __TEST_H */

