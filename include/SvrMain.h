/* 
 * File:   GuiMain.h
 * Author: wyy
 *
 * Created on 2014年3月5日, 下午12:03
 */

#ifndef SVRMAIN_H
#define	SVRMAIN_H

#include "gwsmanlib.h"

#define MAX_GWS_BUFF    4096
#define SVR_DEBUG(enable, fmt,...)  \
        do {    \
            if (enable) { \
                printf(fmt,##__VA_ARGS__);  \
                fflush(stdout); \
            }   \
        } while (0)

typedef struct __gws_buffer {
    bool                m_bEmpty;
    bool                m_bComplete;
    pthread_mutex_t	m_hAccess;
    char*               m_pBuffer;
    char                m_sBuffer[MAX_GWS_BUFF];
} GWS_BUFF;

void BufInit(GWS_BUFF* gb);
void BufExit(GWS_BUFF* gb);
void BufClear(GWS_BUFF* gb);
bool BufIsEmpty(GWS_BUFF* gb);
bool BufGetBuff(GWS_BUFF* gb,char* buf, bool clear);
bool BufAppend(GWS_BUFF* gb,char* data);

void svrTimerSysMon(unsigned long ticks,int shmID,P_GWS_KPI pKpi);
void svrClearSysMon();

#define NEG_MODE_IS_MESH        0x80
#define NEG_MODE_BROADCAST      0x01
#define NEG_MODE_FIXED_PATH     0x02
#endif	/* SVRMAIN_H */

