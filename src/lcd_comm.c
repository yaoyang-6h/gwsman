
//**
//* by Qige from 6Harmonics Inc. @ Beijing
//* 2014.03.18
//**

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "gwsmanlib.h"
#include "lcd.h"
#include "GuiMain.h"
#include "SvrMain.h"

#define NO_PROMPT 0


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

bool open_chanlist(int region,int resultList[]) {
    int i = 0, channo = 0;
    for (i = 0; i < NUM_CHANNEL(region); i ++) {
        if (resultList[i] != -1) return true;
    }
    FILE* fp = fopen(SCAN_LOG_FILE,"r");
    if (NULL != fp) {
        char record[256];
        char *p = NULL;
        while (!feof(fp) && channo < NUM_CHANNEL(region)) {
            memset (record,0x00,256);
            fgets(record,256,fp);
            if (p = strstr(record,",")) {
                *p = '\0';  //terminate at ':'
                resultList[channo ++ ] = atoi(record);
            }
        }
        fclose(fp);
        return true;
    } else return false;
}

void lcd_listen(bool trace_lcd,P_GWS_KPI pKpiLcd, int nKpiLcd) {
    int param = 0;
    int region = 0;
    if (sp_recv(trace_lcd) > 0) {
        RW_KPI_VAR(nKpiLcd,region,pKpiLcd);
        if (strstr(sp_rx_buf, "wbget")) {
            LCD_DEBUG(trace_lcd,"\t- Answering get cmd: %s\n", sp_rx_buf);
            if (strstr(sp_rx_buf, "wbgetchns")) {
                LCD_DEBUG(trace_lcd,"\t- getchns -> exec(gws_get_chlist)\r\n");
                trace_lcd ? BUILD_DBG_RESPONSE(sp_tx_buf, gws_get_chlist(region))
                        : BUILD_RESPONSE(sp_tx_buf, gws_get_chlist(region));
            } else if (strstr(sp_rx_buf, "wbgetchn")) {
                LCD_DEBUG(trace_lcd,"\t- getchn -> exec(gws_get_ch)\r\n");
                trace_lcd ? BUILD_DBG_RESPONSE(sp_tx_buf, gws_get_ch(pKpiLcd,nKpiLcd))
                        : BUILD_RESPONSE(sp_tx_buf, gws_get_ch(pKpiLcd,nKpiLcd));
            } else if (strstr(sp_rx_buf, "wbgetsig")) {
                LCD_DEBUG(trace_lcd,"\t- getsig -> exec(gws_get_signal)\r\n");
                trace_lcd ? BUILD_DBG_RESPONSE(sp_tx_buf, gws_get_signal(pKpiLcd,nKpiLcd))
                        : BUILD_RESPONSE(sp_tx_buf, gws_get_signal(pKpiLcd,nKpiLcd));
            }
            if (strstr(sp_rx_buf, "wbgetpwrs")) {
                LCD_DEBUG(trace_lcd,"\t- getpwr -> exec(gws_get_txpwr)\r\n");
                trace_lcd ? BUILD_DBG_RESPONSE(sp_tx_buf, gws_get_txpwr())
                        : BUILD_RESPONSE(sp_tx_buf, gws_get_txpwr());
            }
            SEND_RESPONSE;
        } else if (strstr(sp_rx_buf, "wbset")) {
            LCD_DEBUG(trace_lcd,"\t- Answering set cmd: %s\n", sp_rx_buf);
            if ((param = pickup_parameter(sp_rx_buf,"wbsetchn:","\r\n")) != 0xFFFFFFFF) {
                LCD_DEBUG(trace_lcd,"\t- setchn:%d\n",param);
                BUILD_RESPONSE(sp_tx_buf, gws_set_ch(pKpiLcd,nKpiLcd,param));
            } else if ((param = pickup_parameter(sp_rx_buf,"wbsetpwr:","\r\n")) != 0xFFFFFFFF) {
                LCD_DEBUG(trace_lcd,"\t- setpwr:%d\n",param);
                BUILD_RESPONSE(sp_tx_buf, gws_set_txpwr(pKpiLcd,nKpiLcd,param));
            } else if ((param = pickup_parameter(sp_rx_buf,"wbsetmod:","\r\n")) != 0xFFFFFFFF) {
                LCD_DEBUG(trace_lcd,"\t- setmod:%d\n",param);
                BUILD_RESPONSE(sp_tx_buf, gws_set_mode(pKpiLcd,nKpiLcd,param));
            }
            SEND_RESPONSE;
        } else if (strstr(sp_rx_buf, "wbscan")) {
//                printf("\t- scanchns\n");
            LCD_DEBUG(trace_lcd,"\t- scanchns\n");
//                sprintf(sp_tx_buf, "\r\n%s\r\n", gws_scanchns());
//                sprintf(sp_tx_buf, "\r\n+scanrst:%s\r\n",gws_scanchns());
//                BUILD_RESPONSE(sp_tx_buf, gws_get_chlist());
            gws_scanchns(pKpiLcd,nKpiLcd);
//                SEND_RESPONSE;
        }
        memset(sp_rx_buf, 0, SP_BUF_MAX_LEN);
    } else {
//            LCD_DEBUG(trace_lcd,"\t* No cmd detected (%d) ...\n", i);
    }
    if (strlen(sp_rx_buf) >= SP_BUF_MAX_LEN) {
        memset(sp_rx_buf, 0, SP_BUF_MAX_LEN);
    }
}

#define SWITCH_REPORT_TYPE(report) ((report %= REPORT_TYPE_MAX) + 1)
void sp2lcd_serv(int distance,char neg_mode,bool trace_lcd,bool trace_neg) {
    uint        counter = 0;
    char        report = 0;
    int         nKpiLcd = 0;
    P_GWS_KPI   pKpiLcd = NULL;
    bool        is_mesh = false;
    sem_t*      semInstance = sem_open(SEM_NAME_GWSMAN_SERVER,O_RDWR);
    int         local_sta = get_mesh_station_no();
    
    LCD_DEBUG(trace_lcd,"- SP4LCD providing service at %s ...\n", sp_filename());
    
    if (SEM_FAILED == semInstance) return;
    if ((pKpiLcd = ConnectShm(&nKpiLcd)) == NULL) return;
    LCD_DEBUG(trace_lcd,"\nLCD service initialization OK!\n");

    svrSetDistance(distance);
    svrSetDebug(trace_neg);
    svrSetWorkMode(trace_neg, neg_mode);
    char interval = 0x00ff & (neg_mode >> 8);
    interval = interval < 2 ? 2 : interval;
    while (svrTestInstance(semInstance)) {
        usleep(LCD_TIME_SLOT);
        counter ++;
        
        if (0 == (counter % MESH_CHK_INTL)) {
            char    sMode[32];
            memset (sMode,0x00,32);
            RW_KPI_STR(nKpiLcd,sMode,pKpiLcd->m_wifi.m_sMode);
            if (strstr(sMode,"Mesh Point") >= sMode) {
                is_mesh = true;
            } else is_mesh = false;
        }
        if (0 == (counter % SP2LCD_INTL_L(interval))) {
            report ++;
            sp2lcd_intl(trace_lcd,pKpiLcd,nKpiLcd,SWITCH_REPORT_TYPE(report),local_sta);
        } else if (0 == (counter % SP2LCD_INTL_S)) {
            sp2lcd_intl(trace_lcd,pKpiLcd,nKpiLcd,REPORT_NONE,local_sta);
        } else {
            lcd_listen(trace_lcd,pKpiLcd,nKpiLcd);
            if (is_mesh) {
                svrSetWorkMode(trace_neg, NEG_MODE_IS_MESH | neg_mode);
                svrTimerSysMon(counter,nKpiLcd,pKpiLcd);
            } else if (neg_mode & NEG_MODE_BROADCAST) {
                svrSetWorkMode(trace_neg, NEG_MODE_BROADCAST);
                svrTimerSysMon(counter,nKpiLcd,pKpiLcd);
            } else {
                svrSetWorkMode(trace_neg, 0);
                svrTimerSysMon(counter,nKpiLcd,pKpiLcd);
            }
        }
//        counter %= SP2LCD_INTL_L;
    }
    if (semInstance) sem_close(semInstance);
    CloseShm(pKpiLcd);
    LCD_DEBUG(trace_lcd,"\t- Exit from lcd server\n");
}
