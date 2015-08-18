#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "shmem.h"
#include "gwsmanlib.h"
#include "SvrMain.h"

void BufInit(GWS_BUFF* gb) {
    gb->m_bComplete = false;
    gb->m_bEmpty = true;
    gb->m_pBuffer = gb->m_sBuffer;
    memset (gb->m_sBuffer,0x00,4096);
    pthread_mutex_init(&gb->m_hAccess,NULL);
    }

void BufExit(GWS_BUFF* gb) {
    pthread_mutex_unlock(&gb->m_hAccess);
    pthread_mutex_destroy(&gb->m_hAccess);
}


void BufClear(GWS_BUFF* gb) {
    pthread_mutex_lock(&gb->m_hAccess);
    gb->m_bComplete = false;
    gb->m_bEmpty = true;
    memset (gb->m_sBuffer,0x00,MAX_GWS_BUFF);
    gb->m_pBuffer = gb->m_sBuffer;
    pthread_mutex_unlock(&gb->m_hAccess);     
}

bool BufIsEmpty(GWS_BUFF* gb) {
    return gb->m_bEmpty;
}

bool BufGetBuff(GWS_BUFF* gb,char* buf, bool clear) {
    bool ret = false;
    if (0 == pthread_mutex_trylock(&gb->m_hAccess)) {
        if (gb->m_bComplete) {
            strcpy(buf,gb->m_sBuffer);
            ret = true;
            if (clear) {
                memset (gb->m_sBuffer,0x00,MAX_GWS_BUFF);
                gb->m_pBuffer = gb->m_sBuffer;
                gb->m_bComplete = false;
                gb->m_bEmpty = true;
            }
        } else ret = false;
        pthread_mutex_unlock(&gb->m_hAccess);
        return ret;
    } else return false;
}

bool BufAppend(GWS_BUFF* gb,char* data) {
    struct timespec timeout;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = 1000000;     //1 ms
    if (0 == pthread_mutex_timedlock(&gb->m_hAccess,&timeout)) {
        if (NULL != data) {
            int len = strlen(data);
            if (gb->m_pBuffer + len < gb->m_sBuffer + MAX_GWS_BUFF) {
                strcpy(gb->m_pBuffer,data);
                gb->m_pBuffer = gb->m_sBuffer + strlen(gb->m_sBuffer);
            }
            gb->m_bEmpty = false;
        } else {
            gb->m_bComplete = true;
        }
        pthread_mutex_unlock(&gb->m_hAccess);
        return true;
    } else return false;
}

void ClearBindValue(BindValue* bv) {
    if (bv) {
        bv->m_nType = 0;
        bv->m_pValue = NULL;
    }
}

void InitBindValue(BindValue* bv, const char type, void* pValue) {
    if (bv) {
        bv->m_nType = type;
        bv->m_pValue = pValue;
    }
}

void VB_Load(BindValue* bv, int type, void* pValue) {
    if (bv) {
        bv->m_nType = type;
        bv->m_pValue = pValue;
    }
}

char* VB_AsString(BindValue* bv) {
    static char sValue[128];
    
    memset (sValue,0x00,128);
    if (NULL == bv || NULL == bv->m_pValue) return NULL;
    switch (bv->m_nType) {
        case VAR_BOOL:
            if (0 == *(int*) bv->m_pValue) strcpy(sValue,"OFF");
            else strcpy(sValue,"ON");
            return sValue;
        case VAR_INTEGER:
            sprintf(sValue, "%d", *(int*) bv->m_pValue);
            return sValue;
        case VAR_STRING: return ((char*) bv->m_pValue);
        case VAR_LIST: return sValue;
        default: return NULL;
    }
    return NULL;
}

int VB_AsInt(BindValue* bv) {
    if (NULL == bv || NULL == bv->m_pValue) return -1;
    if (VAR_INTEGER == bv->m_nType) {
        return *(int*) bv->m_pValue;
    } else if (VAR_BOOL == bv->m_nType) {
        return (0 != *(int*) bv->m_pValue);
    } else return 0;
}

IW_LIST* VB_AsList(BindValue* bv) {
    if (NULL == bv || NULL == bv->m_pValue) return NULL;
    if (VAR_LIST == bv->m_nType) {
        return (IW_LIST*) bv->m_pValue;
    } else return NULL;
}

int BindValueType(BindValue* bv) {
    if (bv) return bv->m_nType; else return -1;
}

void InitVarBind0(VarBind* vb,char k,int x,int y,int idx, char* title, char* attri) {
    if (vb) {
        vb->m_k = k;
        vb->m_x = x;
        vb->m_y = y;
        vb->m_index = idx;
        memset(vb->m_title,0x00,32);
        memset(vb->m_attri,0x00,32);
        if (title) strncpy(vb->m_title,title,31);
        if (attri) strncpy(vb->m_attri,attri,31);
    }
}

void InitVarBind(VarBind* vb,char k,int x,int y,const GWS_PARA_INDEX* p,char* attri) {
    if (vb) {
        vb->m_k = k;
        vb->m_x = x;
        vb->m_y = y;
        memset(vb->m_title,0x00,32);
        memset(vb->m_attri,0x00,32);
        if (p) {
            vb->m_index = p->m_param_key;
            strncpy(vb->m_title,p->m_param_title,31);
        }
        if (attri) strncpy(vb->m_attri,attri,31);
    }
}
