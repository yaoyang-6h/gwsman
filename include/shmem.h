/* 
 * File:   shmem.h
 * Author: wyy
 *
 * Created on 2014年3月4日, 上午9:28
 */

#ifndef SHMEM_H
#define	SHMEM_H

#ifdef	__cplusplus
extern "C" {
#endif

#define GWS_SHMEM_ID    0x20140303
#define GWS_MSG_Q_ID    0x20140304

typedef struct structGwsCommand {
    int         m_nReq;
    char        m_sPara[32];
} GWS_COMMAND;

typedef struct structGwsMsgBuf {
    long        m_nType;
    GWS_COMMAND m_command;
} GWS_MESSAGE;

typedef struct structGwsParams {
    char        m_sFirmware[32];
    int         m_nBoardSNO;
    int         m_nRegion;
    int         m_nChanNo;
    int         m_nIFOUT;
    int         m_nAGCMode;
    int         m_nTemp;
    int         m_bTXCal;
    int         m_bRXCal;
    int         m_bTX;
    int         m_bRX;
    int         m_nCurTxPwr;
    int         m_nRXGain;
    int         m_nBrdMaxPwr;
    int         m_nRxMaxGain;
    int         m_nTxAtten;
    int         m_nRxFAtten;
    int         m_nMaxAtten;
    int         m_nRxRAtten;
    int         m_nMaxTxPwr;
    int         m_nMinTxPwr;
} GWS_RADIO;

typedef struct structIwParams {
    char        m_sSSID[64];
    char        m_sMode[16];
    int         m_nChannel;
    int         m_nFrequency;
    int         m_nFrequencyOffset;
    char        m_sBSSID[18];    //Mac address
    char        m_sEncryption[32];
    int         m_nTxPower;
    int         m_nTxPowerOffset;
    char        m_sHardwareID[32];
    char        m_sHardwareName[32];
    char        m_sHwModeList[32];
    char        m_sQuality[32];
    int         m_nMbssid_support;
    int         m_nSignal;
    int         m_nNoise;
    int         m_nBitRate;
} GWS_WIFI;

#define IW_LIST_MAXLINE	16
#define IW_LIST_MAXSIZE	128
#define MIN_CHANNEL(region)     (region == 0 ? 14 : 21)
#define MAX_CHANNEL(region)     (region == 0 ? 51 : 60)
#define NUM_CHANNEL(region)     (MAX_CHANNEL(region)-MIN_CHANNEL(region)+1)
#define MAX_CH_NUM              NUM_CHANNEL(1)
#define NF_COL_PER_LINE(lines,region)  (lines > 1 ? 24 : NUM_CHANNEL(region))

typedef struct strucResult {
    int m_nLine;
    unsigned char m_header[IW_LIST_MAXSIZE];
    unsigned char m_index[IW_LIST_MAXLINE];
    unsigned char m_value[IW_LIST_MAXLINE][IW_LIST_MAXSIZE];
} IW_LIST;

typedef struct structNoiseFloor {
    char        m_noise_max;
    char        m_noise_avg;
    char        m_noise_cur;
    char        m_noise_min;
} GWS_CHAN_NF;

typedef struct structKPI {
    GWS_RADIO   m_radio;
    GWS_WIFI    m_wifi;
    IW_LIST     m_assoc_list;
    GWS_CHAN_NF m_noise_floor[MAX_CH_NUM];
    short       m_currchannel;
    int         m_error_no;
} GWS_KPI,*P_GWS_KPI;

P_GWS_KPI CreateShm(int* identifier);
P_GWS_KPI ConnectShm(int* identifier);
int CloseShm(P_GWS_KPI pKpi);
int DestoryShm(int identifier);
int HoldShm(int identifier);
int ReleaseShm(int identifier);
int OpenMessageQueue(int isServer);
void CloseMessageQueue(int queueID);

#define RW_KPI_VAR(id,dst,src)   \
    do {                        \
        if (HoldShm(id)) {      \
            dst = src;              \
            ReleaseShm(id);     \
        }                       \
    } while (0)

#define RW_KPI_STR(id,dst,src)       \
    do {                                \
        if (HoldShm(id)) {              \
            strncpy(dst,src,sizeof(dst)-1);     \
            ReleaseShm(id);             \
        }                               \
    } while (0)

#ifdef	__cplusplus
}
#endif

#endif	/* SHMEM_H */

