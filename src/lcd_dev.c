//**
//* by Qige from 6Harmonics Inc. @ Beijing
//* 2014.03.18
//**
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#include "gwsmanlib.h"
#include "GuiMain.h"
#include "lcd.h"
#include "iw/iw.h"

typedef struct __struct_gws_cache {
    time_t m_timer;
    int m_nChannel;
    int m_nTxPower;
    int m_nMode;
    int m_nScanStartChan;
} GWS_LCD_CACHE;

static GWS_LCD_CACHE lcd_cache = {0,0,0,0,0};
static char lcd_message_ok[]    = "OK";
static char lcd_message_error[] = "ERR";

#define LCD_OK  lcd_message_ok
#define LCD_ERR lcd_message_error

#define _KPI_HOLD_    HoldShm(nKpiLcdId);
#define _KPI_FREE_    ReleaseShm(nKpiLcdId);

int gws_command_str(P_GWS_KPI pKpiLcd,int nKpiLcdId,int action, char* parameter) {
    int nQid = -1;
    if (NULL == parameter) return -1;
    if ((nQid = OpenMessageQueue(0)) >= 0) {
        int para = atoi(parameter);

        lcd_cache.m_timer = time(&lcd_cache.m_timer);
        switch (action) {
            case SET_MODE:  lcd_cache.m_nMode = para;       break;
            case SET_CHAN:  lcd_cache.m_nChannel = para;    break;
            case SET_TXPWR: lcd_cache.m_nTxPower = para;    break;
            case SET_TXON:  break;
            case SCAN_CHAN:
                if (pKpiLcd != NULL && !svrIsScanning()) {
                    lcd_cache.m_nScanStartChan = iw_getcurrchan(pKpiLcd,nKpiLcdId,nKpiLcdId);
                } else return -4;
                break;
            default: return -2;
        }
        CommandGo(action, parameter, nQid, false);
    } else {
        printf("\nError can not open message queue\n");
        return -3;
    }
    return 0;
}

int gws_command_int(P_GWS_KPI pKpiLcd,int nKpiLcdId,int action, int para) {
    char parameter[32];

    memset(parameter, 0x00, 32);
    sprintf(parameter, "%d", para);
    return gws_command_str(pKpiLcd,nKpiLcdId,action, parameter);
}


int gws_check_cache(int* cache_value,int real_value,time_t current_time) {
#if 0
    return real_value;
#else
    if (*cache_value >= 0) {    //setmode request recently, if request not meet yet,
        if (real_value != *cache_value && (current_time - lcd_cache.m_timer) < 10) {
            return *cache_value;    //report the fake state in 10 seconds
        } else {    //after 10s, we will report real value and clear the cache value
            *cache_value = -1;   //request done, from now, will report the real state
            return real_value;
        }
    } else return real_value;
#endif
}

int gws_cal_snr(int sig,int noise) {
    int snr = sig - noise;
    if (sig >= 0) {   //if there is no connection, the 
        //signal should be 0 and the SNR should be (0-noisefloor) and should be
        //a positive value, that does not make sense,force it to 0
        return 0;
    } else if (snr < 0) {
        return 0;
    } else {
        return snr;
    }
}

int parse_assoc_list_string(char* str) {
    if (str) {
        int i,j;
        unsigned char mac[6];
        char    assoc_string[IW_LIST_MAXSIZE];
        char    assoc_params[8][32];
        char    assoc_signal[3][32];
        char*   s = NULL;
        memset(mac,0x00,sizeof(mac));
        memset(assoc_string,0x00,IW_LIST_MAXSIZE);

        strcpy(assoc_string,str);
//            "00:03:7F:04:00:01|  -49/-101/52|    0| 5.4 MCS2 s| 5.4 MCS2 s|    9542|    2821"
        for (s = assoc_string, j = 0; j < 8 && s; j ++) {
            memset (assoc_params[j],0x00,32);
            s = parse_assoc_entry(s,'|',assoc_params[j]);
//                printf("[%d]    %s\n",j,assoc_params[j]);
        }
        if (j >= 6) {
            MPATH_INFO      info;
            mac_addr_a2n(info.m_mac_dest_add,assoc_params[0]);
            for (s = assoc_params[1], j = 0; j < 3 && s; j ++) {
                memset (assoc_signal[j],0x00,32);
                s = parse_assoc_entry(s,'/',assoc_signal[j]);
            }
            if (j > 1) {
                info.m_signal = atoi(assoc_signal[0]);
                info.m_noise = atoi(assoc_signal[1]);
                int snr = info.m_signal - info.m_noise;
                if (snr < 0) return 0;
                if (snr > 100) return 100;
                return snr;
            }
//            info.m_mcs_rx = (unsigned short) (10 * atof(assoc_params[3]));
//            info.m_mcs_tx = (unsigned short) (10 * atof(assoc_params[4]));
            return 0;
        }
        return 0;
    }
    return 0;
}

int convert_nf(int noise) {
    int nf = 101 + noise;
    if (nf < 0) nf = 0;
    if (nf > 100) nf = 100;
    return nf;
}

#define BUILD_NF_ECHO(buff,ch,noise,sn,over)  do {    \
    sprintf(buff,"+wbecho:%d,%d,%d,%d\r\n",ch, convert_nf(noise),sn,over);    \
} while (0)

void sp2lcd_intl(const uint _debug,P_GWS_KPI pKpiLcd,int nKpiLcdId,char report,int local_sta) {
    static char nf_report_counter[MAX_CH_NUM];
    char        _buf[SP_BUF_MAX_LEN];
    
    memset(_buf, 0, sizeof (_buf));
    
    if (pKpiLcd) {
        int region = 0;
        RW_KPI_VAR(nKpiLcdId,region,pKpiLcd->m_radio.m_nRegion);
        if (MIN_CHANNEL(region) <= lcd_cache.m_nScanStartChan && lcd_cache.m_nScanStartChan <= MAX_CHANNEL(region)) {
            time_t  current_time = time(&current_time);                 //scanning demanded by LCD
            time_t  time_escaped = current_time - lcd_cache.m_timer;
            if (time_escaped > 180) {  //time out
                lcd_cache.m_nScanStartChan = 0;
                lcd_cache.m_timer = 0;
            } else if (time_escaped < 1) { //first channel not done yet,do nothing
                memset( nf_report_counter,0x00,MAX_CH_NUM);
            } else if (svrIsScanning()) {
                short ch = iw_getcurrchan(pKpiLcd,nKpiLcdId,nKpiLcdId) - 1;
                if (ch < MIN_CHANNEL(region)) ch = MAX_CHANNEL(region);
                if (MIN_CHANNEL(region) <= ch && ch <= MAX_CHANNEL(region)) {
                    int sn = ch - lcd_cache.m_nScanStartChan;
                    
                    if (sn < 0) sn += NUM_CHANNEL(region);
                    if (nf_report_counter[sn] < 3) {
                        nf_report_counter[sn] ++;
                        _KPI_HOLD_
                        BUILD_NF_ECHO(_buf,ch,pKpiLcd->m_noise_floor[ch-MIN_CHANNEL(region)].m_noise_avg,sn,0);
                        _KPI_FREE_
                        LCD_DEBUG(_debug,"channel scan  = %d [%d]\n",ch,lcd_cache.m_nScanStartChan);
                    } else return;
                } else LCD_DEBUG(_debug,"channel scan  = %d *\n",ch);
            } else {    //scan has done,(chp.m_nOriginChannel == chp.m_nCurrentChannel)
                short ch = lcd_cache.m_nScanStartChan;
                _KPI_HOLD_
                BUILD_NF_ECHO(_buf,ch,pKpiLcd->m_noise_floor[ch-MIN_CHANNEL(region)].m_noise_avg,NUM_CHANNEL(region),1);
                _KPI_FREE_
                LCD_DEBUG(_debug,"channel scan  = %d\n",lcd_cache.m_nScanStartChan);
                lcd_cache.m_nScanStartChan = 3; //act as counter for sending scan result 3 times
                lcd_cache.m_timer = 0;
            }
        } else if (0 < lcd_cache.m_nScanStartChan && lcd_cache.m_nScanStartChan <= 3) {
            sprintf(_buf, "+wbrch:%s\r\n", gws_get_chlist(region));
            lcd_cache.m_nScanStartChan --;
        } else {
            if (REPORT_STATUS == report) {  //common status report
                int nChannel = MIN_CHANNEL(region),nTxPower = 0,nMode = 2,nSNR = 0;
                time_t  current_time = time(&current_time);

                _KPI_HOLD_
                if (strstr(pKpiLcd->m_wifi.m_sMode,"Master")) nMode = 2;        //AP
                else if(strstr(pKpiLcd->m_wifi.m_sMode,"Client")) nMode = 1;    //STA
                else nMode = 0;                                                 //MESH NODE
                nChannel = pKpiLcd->m_radio.m_nChanNo;
                nTxPower = pKpiLcd->m_radio.m_bTX ? pKpiLcd->m_radio.m_nCurTxPwr : 0;
                nSNR = gws_cal_snr(pKpiLcd->m_wifi.m_nSignal, pKpiLcd->m_wifi.m_nNoise);
                _KPI_FREE_
                if (current_time - lcd_cache.m_timer < 10) {
                    if (lcd_cache.m_nMode >= 0) nMode = lcd_cache.m_nMode;
                    if (lcd_cache.m_nChannel >= 0) nChannel = lcd_cache.m_nChannel;
                    if (lcd_cache.m_nTxPower >= 0) nTxPower = lcd_cache.m_nTxPower;
                } else {
                    lcd_cache.m_nMode = nMode;
                    lcd_cache.m_nChannel = nChannel;
                    lcd_cache.m_nTxPower = nTxPower;
                }
                sprintf(_buf, "\r\n+wbsts:%d,%d,%d,%d\r\n", nChannel,nSNR, nTxPower, nMode);
                LCD_DEBUG(_debug,"\t[%02d:%02d:%02d] <+wbsts:%d,%d,%d,%d>\n",
                            (current_time % 86400) / 3600,
                            ((current_time % 86400) % 3600) / 60,
                            ((current_time % 86400) % 3600) % 60,
                            nChannel,nSNR, nTxPower, nMode);
            } else if (REPORT_MULTI_SNR == report) {   //multi SNR report
                _KPI_HOLD_
                int snrs[5] = {0,0,0,0,0};  //max 5 mesh points
                int i,nAssoc = pKpiLcd->m_assoc_list.m_nLine;
                for (i = 0; i < nAssoc; i ++) {
                    int     sta = pKpiLcd->m_assoc_list.m_index[i];
                    int     snr = parse_assoc_list_string(pKpiLcd->m_assoc_list.m_value[i]);
                    if (1 <= sta && sta <= 5) {
                        snrs[sta-1] = snr;
                    }
                }
                _KPI_FREE_

                if (1 <= local_sta && local_sta <= 5)
                    snrs[local_sta-1] = 99;
//                if (nAssoc > 0) {
                {
                    time_t  current_time = time(&current_time);
                    sprintf(_buf, "\r\n+wbsigs:%d,%d,%d,%d,%d\r\n",
                            snrs[0],snrs[1],snrs[2],snrs[3],snrs[4]);
                    LCD_DEBUG(_debug,"\t[%02d:%02d:%02d] <+wbsigs:%d,%d,%d,%d,%d>\n",
                                (current_time % 86400) / 3600,
                                ((current_time % 86400) % 3600) / 60,
                                ((current_time % 86400) % 3600) % 60,
                                snrs[0],snrs[1],snrs[2],snrs[3],snrs[4]);
                }
            } else return;  //REPORT_NONE
        }
    } else {    //share memory open failed, just report 0
        sprintf(_buf, "\r\n+wbsts:%d,%d,%d,%d\r\n", 0, 0, 0, 0);
    }
//    while (strlen(sp_tx_buf) > 0) {
//        LCD_DEBUG(_debug,"- debug: sp tx is busy.\n");
//    }
//    strcat(sp_tx_buf, _buf);
    strcpy(sp_tx_buf, _buf);
//    LCD_DEBUG(_debug,"\t** send: %s", sp_tx_buf);
    sp_send(0);
}

char *gws_get_ch(P_GWS_KPI pKpiLcd,int nKpiLcdId) {
    static char sChannel[32];

    memset (sChannel,0x00,32);
    if (pKpiLcd) {  
        int ch = 0;
        RW_KPI_VAR(nKpiLcdId,ch,pKpiLcd->m_radio.m_nChanNo);
        sprintf(sChannel,"%d",ch);
    } else sprintf(sChannel,"51");
    return sChannel;
}

char *gws_get_chlist(int region) {
    int i = 0;
    int resultList[MAX_CH_NUM];
    static char chanList[256];
    
    memset (chanList,0x00,256);    
    memset (resultList,0xFF,sizeof(int) * MAX_CH_NUM);
    if (open_chanlist(region,resultList)) {
        for (i = 0; i < 6; i ++) {  //just first 6 channels
            if (resultList[i] > 0) {
                char sChan[8];
                memset (sChan,0x00,8);
                if (0 == chanList[0]) sprintf(sChan,"%d",resultList[i]);
                else sprintf(sChan,",%d",resultList[i]);
                strcat(chanList,sChan);
            }
        }        
    } else strcpy(chanList,"14,20,29,30,49,51");
    return chanList;
}

char *gws_get_signal(P_GWS_KPI pKpiLcd,int nKpiLcdId) {
    static char sSignal[32];

    memset (sSignal,0x00,32);
    if (pKpiLcd) {  
        int sig = 0;
        int noise = 0;
        RW_KPI_VAR(nKpiLcdId,sig,pKpiLcd->m_wifi.m_nSignal);
        RW_KPI_VAR(nKpiLcdId,noise,pKpiLcd->m_wifi.m_nNoise);
        sprintf(sSignal,"%d",gws_cal_snr(sig, noise));
    } else sprintf(sSignal,"0");
    return sSignal;
}

char *gws_get_txpwr() {
    return "0,5,10,15,20,25,30,33";
}

char *gws_set_ch(P_GWS_KPI pKpiLcd,int nKpiLcdId,const uint ch) {
    int region = 0;
    RW_KPI_VAR(nKpiLcdId,region,pKpiLcd->m_radio.m_nRegion);
    if (ch < MIN_CHANNEL(region) || ch > MAX_CHANNEL(region)) {
        return LCD_ERR;
    } else { // system(sprintf("setchan %d; setchan %s; setchan %d", ch, ch, ch));
        if (gws_command_int(pKpiLcd,nKpiLcdId,SET_CHAN, ch) < 0) return LCD_ERR;
    }
    return LCD_OK;
}

char *gws_set_txpwr(P_GWS_KPI pKpiLcd,int nKpiLcdId,const uint txpwr) {
    if (txpwr < GWS_TXPWR_MIN || txpwr > GWS_TXPWR_MAX) {
        return LCD_ERR;
    } else {
        if (GWS_TXPWR_MIN == txpwr) { // system("txoff; txoff; txoff");
            if (gws_command_int(pKpiLcd,nKpiLcdId,SET_TXPWR, 0) < 0) return LCD_ERR;
            if (gws_command_str(pKpiLcd,nKpiLcdId,SET_TXON, "off") < 0) return LCD_ERR;
        } else if (txpwr <= GWS_TXPWR_MAX) { // system(sprintf("txon; txon; txon; settxcal 1; settxcal 1; setrxcal 1; setrxcal 1; settxpwr %d; settxpwr %s; settxpwr %d", txpwr, txpwr, txpwr));
            if (gws_command_int(pKpiLcd,nKpiLcdId,SET_TXPWR, txpwr) < 0) return LCD_ERR;
            if (gws_command_str(pKpiLcd,nKpiLcdId,SET_TXON, "on") < 0) return LCD_ERR;
        } else { // system("txon; txon; txon; settxcal 0; settxcal 0; setrxcal 0; setrxcal 0; settxatten 0; settxatten 0");
            if (gws_command_str(pKpiLcd,nKpiLcdId,SET_TXON, "on") < 0) return LCD_ERR;
        }
    }
    return LCD_OK;
}

char *gws_set_mode(P_GWS_KPI pKpiLcd,int nKpiLcdId,const uint mode) {
    char parameter[32];

    memset (parameter,0x00,32);
    switch (mode) {
        case 0: strcpy (parameter,"mesh");  break;
        case 1: strcpy (parameter,"sta");   break;
        case 2: strcpy (parameter,"ap");    break;
        default: return LCD_ERR;
    }
    if (gws_command_str(pKpiLcd,nKpiLcdId,SET_MODE, parameter) < 0) return LCD_ERR;
    return LCD_OK;
}

char *gws_scanchns(P_GWS_KPI pKpiLcd,int nKpiLcdId) {
    char parameter[32];

    memset (parameter,0x00,32);
    if (gws_command_str(pKpiLcd,nKpiLcdId,SCAN_CHAN, parameter) < 0) return LCD_ERR;
    return LCD_OK;
}
