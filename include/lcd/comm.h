
#include "shmem.h"

//**
//* by Qige from 6Harmonics Inc. @ Beijing
//* 2014.03.18
//**
#define NO_PROMPT 0

void sp2lcd_intl(const uint _debug) {
    char _buf[SP_BUF_MAX_LEN];
    memset(_buf, 0, sizeof (_buf));
    if (pKpiLcd) {
        int nMode = 2;
        if (strstr(pKpiLcd->m_wifi.m_sMode,"Master")) nMode = 0;        //0
        else if(strstr(pKpiLcd->m_wifi.m_sMode,"Client")) nMode = 1;    //1
        else nMode = 2;                                                 //2
        sprintf(_buf, "+stswb:%d,%d,%d,%d\r\n", 
                pKpiLcd->m_radio.m_nChanNo,
                pKpiLcd->m_wifi.m_nSignal - pKpiLcd->m_wifi.m_nNoise,
                pKpiLcd->m_radio.m_nCurTxPwr,nMode);
    } else {
        sprintf(_buf, "+stswb:%d,%d,%d,%d\r\n", 15, 20, 30, 1);
        
    }
    while (strlen(sp_tx_buf) > 0) {
        PRINT_DEBUG("- debug: sp tx is busy.\n");
    };
    strcat(sp_tx_buf, _buf);
    PRINT_DEBUG("\t** send: %s", sp_tx_buf);
    sp_send(0);
}

uint pickup_parameter(char* target,char* start,char* end) {
    char sParam[16];
    char *pStart = NULL, *pEnd = NULL;
    int nParam = 0, nStart = 0;
    if (NULL == target || NULL == start || NULL == end) return 0xFFFFFFFF;
//    printf("\n Search string : [%s] from [%s] to [%s]\n",target,start,end);

    nStart = strlen(start);
    if (pStart = strstr(target, start)) {
//        printf("\n Found %s\n",start);
        if (NULL == (pEnd = strstr(target, end)))
            pEnd = pStart + strlen(target);
        pStart += nStart;    //move to poistion right after ':'
        nParam = pEnd - pStart;
        nParam = nParam > 15 ? 15 : nParam;
        memset (sParam,0x00,16);
        if (nParam > 0) {
            memcpy (sParam,pStart,nParam);
            return atoi(sParam);
        } else {
            return 0;
        }
    } else return 0xFFFFFFFF;
}

bool open_chanlist(int resultList[MAX_CHANNEL_N]) {
    int i = 0, channo = 0;
    for (i = 0; i < MAX_CHANNEL_N; i ++) {
        if (resultList[i] != -1) return true;
    }
    FILE* fp = fopen(SCAN_LOG_FILE,"r");
    if (NULL != fp) {
        char record[256];
        char *p = NULL;
        while (!feof(fp)) {
            memset (record,0x00,256);
            fgets(record,256,fp);
            if (p = strstr(record,":")) {
                *p = '\0';  //terminate at ':'
                resultList[channo ++ ] = atoi(record);
            }
        }
        fclose(fp);
        return true;
    } else return false;
}

void sp2lcd_serv(const uint _debug) {
    int nIdentifier = 0;
    uint cmd = 0, i = 0,param = 0;
    char *tmp = NULL;
    
    PRINT_DEBUG("- SP4LCD providing service at %s ...\n", sp_file);
    if (pKpiLcd) CloseShm(pKpiLcd);
    pKpiLcd = ConnectShm(&nIdentifier);
    for (;;) {
        sp_recv(_debug);
        //sp_rx_filter();
        if (strlen(sp_rx_buf) > 0 && sp_rx_buf[0] == '+') {
            PRINT_DEBUG("\t* LCD cmd detected (%d) ...\n", i);
            PRINT_DEBUG("\t- Analyzing cmd %s from LCD ...\n", sp_rx_buf);
            if (strstr(sp_rx_buf, "get")) {
                PRINT_DEBUG("\t- Answering get cmd: %s\n", sp_rx_buf);
                if (strstr(sp_rx_buf, "getchns")) {
                    PRINT_DEBUG("\t- getchns -> exec(gws_get_chlist)\r\n");
                    _debug ? sprintf(sp_tx_buf, "\r\n%s%s\r\n", "gws_get_chlist -> ", gws_get_chlist())
                            : sprintf(sp_tx_buf, "\r\n%s\r\n", gws_get_chlist());
                }
                if (strstr(sp_rx_buf, "getsig")) {
                    PRINT_DEBUG("\t- getsig -> exec(gws_get_signal)\r\n");
                    _debug ? sprintf(sp_tx_buf, "\r\n%s%s\r\n", "gws_get_signal -> ", gws_get_signal())
                            : sprintf(sp_tx_buf, "\r\n%s\r\n", gws_get_signal());
                }
                if (strstr(sp_rx_buf, "getpwrs")) {
                    PRINT_DEBUG("\t- getpwr -> exec(gws_get_txpwr)\r\n");
                    _debug ? sprintf(sp_tx_buf, "\r\n%s%s\r\n", "gws_get_txpwr -> ", gws_get_txpwr())
                            : sprintf(sp_tx_buf, "\r\n%s\r\n", gws_get_txpwr());
                }
                sp_send(NO_PROMPT);
                memset(sp_rx_buf, 0, sizeof (sp_rx_buf));
            } else if (strstr(sp_rx_buf, "set")) {
                PRINT_DEBUG("\t- Answering set cmd: %s\n", sp_rx_buf);
                if ((param = pickup_parameter(sp_rx_buf,"+setchn:","\r\n")) != 0xFFFFFFFF) {
                    PRINT_DEBUG("\t- setchn:%d\n",param);
                    sprintf(sp_tx_buf, "\r\n%s\r\n", gws_set_ch(param));
                } else if ((param = pickup_parameter(sp_rx_buf,"+setpwr:","\r\n")) != 0xFFFFFFFF) {
                    PRINT_DEBUG("\t- setpwr:%d\n",param);
                    sprintf(sp_tx_buf, "\r\n%s\r\n", gws_set_txpwr(param));
                } else if ((param = pickup_parameter(sp_rx_buf,"+setmod:","\r\n")) != 0xFFFFFFFF) {
                    PRINT_DEBUG("\t- setmod:%d\n",param);
                    sprintf(sp_tx_buf, "\r\n%s\r\n", gws_set_mode(param));
                }
                sp_send(NO_PROMPT);
                memset(sp_rx_buf, 0, sizeof (sp_rx_buf));
            } else if (strstr(sp_rx_buf, "scanchns")) {
//                printf("\t- scanchns\n");
                PRINT_DEBUG("\t- scanchns\n");
//                sprintf(sp_tx_buf, "\r\n%s\r\n", gws_scanchns());
                sprintf(sp_tx_buf, "\r\n+scanrst:%s\r\n",gws_get_chlist());
                sp_send(NO_PROMPT);
                memset(sp_rx_buf, 0, sizeof (sp_rx_buf));
            }
            memset(sp_rx_buf, 0, sizeof (sp_rx_buf));
        } else {
//            PRINT_DEBUG("\t* No cmd detected (%d) ...\n", i);
        }
        if (strlen(sp_rx_buf) >= SP_BUF_MAX_LEN) {
            memset(sp_rx_buf, 0, sizeof (sp_rx_buf));
        }
        usleep(150000);
        i++;
        if (i % SP2LCD_INTL == 0) {
            PRINT_DEBUG("\t-- Sending stsws.\n");
            sp2lcd_intl(_debug);
            //i = 0;
        }
    }
    CloseShm(pKpiLcd);
}

