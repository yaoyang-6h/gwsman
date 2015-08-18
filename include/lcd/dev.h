//**
//* by Qige from 6Harmonics Inc. @ Beijing
//* 2014.03.18
//**
#include "chanscan.h"
#include "gwsmanlib.h"

#define GWS_TXPWR_MIN 0
#define GWS_TXPWR_MAX 32
#define GWS_CH_MIN 14
#define GWS_CH_MAX 51

static char lcd_message_ok[]    = "OK";
static char lcd_message_error[] = "ERR";
static P_GWS_KPI pKpiLcd = NULL;

#define LCD_OK  lcd_message_ok
#define LCD_ERR lcd_message_error

int gws_command_str(int action, char* parameter) {
    int nQid = -1;
    if (NULL == parameter) return -1;
    if ((nQid = OpenMessageQueue(0)) >= 0) {
        switch (action) {
            case SET_MODE:
                break;
            case SET_CHAN:
                break;
            case SET_TXPWR:
                break;
            case SCAN_CHAN:
                break;
            case SET_TXON:
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

int gws_command_int(int action, int para) {
    char parameter[32];

    memset(parameter, 0x00, 32);
    sprintf(parameter, "%d", para);
    return gws_command_str(action, parameter);
}

char *gws_get_chlist() {
    int i = 0;
    int resultList[MAX_CHANNEL_N];
    static char chanList[256];
    memset (chanList,0x00,256);
    
    memset (resultList,0xFF,sizeof(int) * MAX_CHANNEL_N);
    if (open_chanlist(resultList)) {
        for (i = 0; i < MAX_CHANNEL_N; i ++) {
            if (resultList[i] > 0) {
                char sChan[8];
                memset (sChan,0x00,8);
                if (0 == chanList[0]) sprintf(sChan,"%d",resultList[i]);
                else sprintf(sChan,",%d",resultList[i]);
                strcat(chanList,sChan);
            }
        }        
    } else strcpy(chanList,"14,51");
    return chanList;
}

char *gws_get_signal() {
    static char sSignal[16];

    memset (sSignal,0x00,16);
    if (pKpiLcd) {  
        if (pKpiLcd->m_wifi.m_nSignal >= 0) {   //if there is no connection, the 
            //signal should be 0 and the SNR should be (0-noisefloor) and should be
            //a positive value, that does not make sense,force it to 0
            sprintf(sSignal,"%d,%d", pKpiLcd->m_radio.m_nChanNo,0);
        } else {
            sprintf(sSignal,"%d,%d",
                    pKpiLcd->m_radio.m_nChanNo,
                    pKpiLcd->m_wifi.m_nSignal - pKpiLcd->m_wifi.m_nNoise);
        }
    } else sprintf(sSignal,"0,0");
    return sSignal;
}

char *gws_get_txpwr() {
    return "0,5,10,15,20,25,30,33";
}

char *gws_set_ch(const uint ch) {
    if (ch < GWS_CH_MIN || ch > GWS_CH_MAX) {
        return LCD_ERR;
    } else { // system(sprintf("setchan %d; setchan %s; setchan %d", ch, ch, ch));
        if (gws_command_int(SET_CHAN, ch) < 0) return LCD_ERR;
    }
    return LCD_OK;
}

char *gws_set_txpwr(const uint txpwr) {
    if (txpwr < GWS_TXPWR_MIN || txpwr > GWS_TXPWR_MAX) {
        return LCD_ERR;
    } else {
        if (GWS_TXPWR_MIN == txpwr) { // system("txoff; txoff; txoff");
            if (gws_command_int(SET_TXPWR, 0) < 0) return LCD_ERR;
            if (gws_command_str(SET_TXON, "off") < 0) return LCD_ERR;
        } else if (txpwr <= GWS_TXPWR_MAX) { // system(sprintf("txon; txon; txon; settxcal 1; settxcal 1; setrxcal 1; setrxcal 1; settxpwr %d; settxpwr %s; settxpwr %d", txpwr, txpwr, txpwr));
            if (gws_command_int(SET_TXPWR, txpwr) < 0) return LCD_ERR;
            if (gws_command_str(SET_TXON, "on") < 0) return LCD_ERR;
        } else { // system("txon; txon; txon; settxcal 0; settxcal 0; setrxcal 0; setrxcal 0; settxatten 0; settxatten 0");
            if (gws_command_str(SET_TXON, "on") < 0) return LCD_ERR;
        }
    }
    return LCD_OK;
}

char *gws_set_mode(const uint mode) {
    switch (mode) {
        case 0:
            system("config_ap");
            break;
        case 1:
            system("config_sta");
            break;
        case 2:
        default:
            system("config_mesh");
            break;
    }
    return LCD_OK;
}

char *gws_scanchns() {
    char parameter[32];
//    system("gwsman -scan d=2&");
    memset (parameter,0x00,32);
    if (gws_command_str(SCAN_CHAN, parameter) < 0) return LCD_ERR;
    return LCD_OK;
}
