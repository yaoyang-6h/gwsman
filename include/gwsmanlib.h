/* 
 * File:   gwsmanlib.h
 * Author: wyy
 *
 * Created on 2014年6月19日, 上午10:37
 */

#ifndef GWSMANLIB_H
#define	GWSMANLIB_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include "shmem.h"
#include "PipeShell.h"
#include "c-serial-com.h"

#ifndef bool
#define bool    int
#endif
#ifndef BYTE
#define BYTE    unsigned char
#endif
#ifndef true
#define true    1
#endif
#ifndef false
#define false   0
#endif
#ifndef NULL    
#define NULL    0
#endif

#define LANG_CHINESE    936
#define LANG_ENGLISH    437

#ifdef RELEASE_ON_ARM
#define LANGUAGE        LANG_ENGLISH
#else
#define LANGUAGE        LANG_CHINESE
#endif

#define DISP_NORMAL     VT100_RESET
#define DISP_RO         VT100_COLOR(VT100_B_BLACK,VT100_F_WHITE)
#define DISP_RW         VT100_COLOR(VT100_B_RED,VT100_F_WHITE) VT100_UNDER_LINE
#define DISP_DN         VT100_COLOR(VT100_B_WHITE,VT100_F_BLACK) //VT100_HIGHT_LIGHT

#define SEM_NAME_GWSMAN_SERVER  "SEM_NAME_GWSMAN_SERVER"

enum radio_param_key {
    GWS_FIRMWARE = 0,
    GWS_BOARDSNO,
    GWS_REGION,
    GWS_CHANNO,
    GWS_IFOUT,
    GWS_AGCMODE,
    GWS_TEMP,
    GWS_TXCAL,
    GWS_RXCAL,
    GWS_TX,
    GWS_RX,
    GWS_CURTXPWR,
    GWS_RXGAIN,
    GWS_BRDMAXPWR,
    GWS_RXMAXGAIN,
    GWS_TXATTEN,
    GWS_RXFATTEN,
    GWS_MAXATTEN,
    GWS_RXRATTEN,
    GWS_MAXTXPWR,
    GWS_MINTXPWR,
};

enum wifi_param_key {
    WIFI_CHANNEL = 0,
    WIFI_FREQUENCY,
    WIFI_FREQUENCY_OFFSET,
    WIFI_TXPOWER,
    WIFI_TXPOWER_OFFSET,
    WIFI_BITRATE,
    WIFI_SIGNAL,
    WIFI_NOISE,
    WIFI_QUALITY,
    WIFI_QUALITY_MAX,
    WIFI_MBESSID_SUPPORT,
    WIFI_HWMODELIST,
    WIFI_MODE,
    WIFI_SSID,
    WIFI_BSSID,
    WIFI_COUNTRY,
    WIFI_HARDWARE_ID,
    WIFI_HARDWARE_NAME,
    WIFI_ENCRYPTION,
    WIFI_ASSOCLIST,
    WIFI_TXPWRLIST,
    WIFI_SCANLIST,
    WIFI_FREQLIST,
    WIFI_COUNTRYLIST,
};

#define MAX_GWS_VAR         21
#define MAX_WIFI_VAR        13

typedef struct __gws_index {
    int         m_param_key;
    char        m_param_title[32];
} GWS_PARA_INDEX;

enum CValueType {
    VAR_BOOL = 0,
    VAR_INTEGER,
    VAR_STRING,
    VAR_LIST,
};

typedef struct __binding_value {
    int         m_nType;
    void*       m_pValue;
} BindValue;

typedef struct __var_binding {
    char        m_k;
    int         m_x;
    int         m_y;
    int         m_index;
    char        m_title[32];
    char        m_attri[32];
    BindValue   m_value;
} VarBind;

typedef struct __CommandHandlerParameter {
    sem_t*      m_semInstance;
    SerialCom*  m_comm;
    PipeShell*  m_shell;
    char*       m_ifName;
    P_GWS_KPI   m_pKpi;
    int         m_nIdentifier;
    int         m_nQid;
    int         m_nCommBusy;
    short       m_nOriginChannel;
    short       m_nCurrentChannel;
    char        m_uChanScan;
} CommandHandlerPara;

extern VarBind gwsVars[MAX_GWS_VAR];
extern VarBind wifiVars[MAX_WIFI_VAR];
extern bool _b_expert_mode;
extern CommandHandlerPara static_chp;

#define NF_DBM_PER_DIV_1    2
#define NF_DBM_PER_DIV_2    5
#define NF_MAX_DIVS_1       (40/NF_DBM_PER_DIV_1)
#define NF_MAX_DIVS_2       (45/NF_DBM_PER_DIV_2)
#define NF_MAX_ROWS_1       (NF_MAX_DIVS_1 + 1)
#define NF_MAX_ROWS_2       (NF_MAX_DIVS_2 + 1)
#define NF_ROW_START            3
//#define NF_MAX_ROWS         16
//#define NF_COLUMN_START     50
//#define NF_COLUMN_WIDTH     10
#define START_NF_LINE       2
#define START_ASSOC_LINE    (_b_expert_mode ? 12 : 6)

VarBind* SearchVB(VarBind* vars, int maxItems, int index);
VarBind* GetGwsVB(int index);
VarBind* GetIwVB(int index);

void ClearBindValue(BindValue* bv);
void InitBindValue(BindValue* bv, const char type, void* pValue);
void VB_Load(BindValue* bv, int type, void* pValue);
char* VB_AsString(BindValue* bv);
int VB_AsInt(BindValue* bv);
IW_LIST* VB_AsList(BindValue* bv);
int BindValueType(BindValue* bv);
bool IsNumber(char* str);

#ifdef	__cplusplus
}
#endif

#endif	/* GWSMANLIB_H */

