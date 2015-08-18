#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>

#include "GuiMain.h"
#include "SvrMain.h"
////////////////////////////////////////////////////////////////////////////////

static const GWS_PARA_INDEX gws_params[] = {
    { GWS_FIRMWARE, "Firmware"},
    { GWS_BOARDSNO, "BoardSNO"},
    { GWS_REGION, "Region"},
    { GWS_CHANNO, "ChanNo"},
    { GWS_IFOUT, "IFOUT"},
    { GWS_AGCMODE, "AGCMode"},
    { GWS_TEMP, "Temp"},
    { GWS_TXCAL, "TXCal"},
    { GWS_RXCAL, "RXCal"},
    { GWS_TX, "TX"},
    { GWS_RX, "RX"},
    { GWS_CURTXPWR, "CurTxPwr"},
    { GWS_RXGAIN, "RXGain"},
    { GWS_BRDMAXPWR, "BrdMaxPwr"},
    { GWS_RXMAXGAIN, "RxMaxGain"},
    { GWS_TXATTEN, "TxAtten"},
    { GWS_RXFATTEN, "RxFAtten"},
    { GWS_MAXATTEN, "MaxAtten"},
    { GWS_RXRATTEN, "RxRAtten"},
    { GWS_MAXTXPWR, "MaxTxPwr"},
    { GWS_MINTXPWR, "MinTxPwr"},
};

static const GWS_PARA_INDEX wifi_params[] = {
    { WIFI_CHANNEL, "Bandwidth"},
    { WIFI_FREQUENCY, "Frequency"},
    { WIFI_FREQUENCY_OFFSET, "Frequency Offset"},
    { WIFI_TXPOWER, "WifiPower"},
    { WIFI_TXPOWER_OFFSET, "TX Power Offset"},
    { WIFI_BITRATE, "Bit Rate"},
    { WIFI_SIGNAL, "Signal"},
    { WIFI_NOISE, "Noise"},
    { WIFI_QUALITY, "Quality"},
    { WIFI_QUALITY_MAX, "Quality Max"},
    { WIFI_MBESSID_SUPPORT, "MbESSID Support"},
    { WIFI_HWMODELIST, "HW Modes"},
    { WIFI_MODE, "Mode"},
    { WIFI_SSID, "SSID"},
    { WIFI_BSSID, "BSSID"},
    { WIFI_COUNTRY, "Country"},
    { WIFI_HARDWARE_ID, "HW ID"},
    { WIFI_HARDWARE_NAME, "HW Name"},
    { WIFI_ENCRYPTION, "Encryption"},
    { WIFI_ASSOCLIST, "Connected "},
    { WIFI_TXPWRLIST, "TX Power List"},
    { WIFI_SCANLIST, "Scan List"},
    { WIFI_FREQLIST, "Frequncy List"},
    { WIFI_COUNTRYLIST, "Country List"},
};

VarBind gwsVars[MAX_GWS_VAR];
VarBind wifiVars[MAX_WIFI_VAR];
bool _b_expert_mode = false;

#define COL1            1
#define COL2            35
#define COL3            60

void InitGwsVars(bool expert_mode) {
#define GP(id)  (&gws_params[id])
#define GV      (&gwsVars[i++])
    int i = 0;
    if (expert_mode) {
        InitVarBind(GV, ' '         , 1, COL1, GP(GWS_FIRMWARE), DISP_RO);
        InitVarBind(GV, ' '         , 2, COL1, GP(GWS_BOARDSNO), DISP_RO);
        InitVarBind(GV, ' '         , 3, COL1, GP(GWS_AGCMODE), DISP_RO);
        InitVarBind(GV, SET_REGION  , 4, COL1, GP(GWS_REGION), DISP_RW);
        InitVarBind(GV, ' '         , 5, COL1, GP(GWS_IFOUT), DISP_RO);
        InitVarBind(GV, ' '         , 6, COL1, GP(GWS_MAXTXPWR), DISP_RO);
        InitVarBind(GV, ' '         , 7, COL1, GP(GWS_MINTXPWR), DISP_RO);

        InitVarBind(GV, ' '         , 1, COL2, GP(GWS_TEMP), DISP_RO);
        InitVarBind(GV, SET_TXATT   , 2, COL2, GP(GWS_TXATTEN), DISP_RW);
        InitVarBind(GV, ' '         , 3, COL2, GP(GWS_RXFATTEN), DISP_RO);
        InitVarBind(GV, ' '         , 4, COL2, GP(GWS_RXRATTEN), DISP_RO);
        InitVarBind(GV, ' '         , 5, COL2, GP(GWS_RXMAXGAIN), DISP_RO);
        InitVarBind(GV, ' '         , 6, COL2, GP(GWS_BRDMAXPWR), DISP_RO);
        InitVarBind(GV, ' '         , 7, COL2, GP(GWS_MAXATTEN), DISP_RO);

        InitVarBind(GV, SET_CHAN    , 1, COL3, GP(GWS_CHANNO), DISP_RW);
        InitVarBind(GV, SET_TXON    , 2, COL3, GP(GWS_TX), DISP_RW);
        InitVarBind(GV, SET_RXON    , 3, COL3, GP(GWS_RX), DISP_RW);
        InitVarBind(GV, SET_TXCAL   , 4, COL3, GP(GWS_TXCAL), DISP_RW);
        InitVarBind(GV, SET_RXCAL   , 5, COL3, GP(GWS_RXCAL), DISP_RW);
        InitVarBind(GV, SET_TXPWR   , 6, COL3, GP(GWS_CURTXPWR), DISP_RW);
        InitVarBind(GV, SET_GAIN    , 7, COL3, GP(GWS_RXGAIN), DISP_RW);
    } else {
        InitVarBind(GV, SET_CHAN    , 1, COL1, GP(GWS_CHANNO), DISP_RW);
        InitVarBind(GV, SET_TXPWR   , 2, COL1, GP(GWS_CURTXPWR), DISP_RW);
        InitVarBind(GV, ' '         , 1, COL2, GP(GWS_REGION), DISP_RO);
        InitVarBind(GV, ' '         , 2, COL2, GP(GWS_TEMP), DISP_RO);
        InitVarBind(GV, SET_TXON    , 1, COL3, GP(GWS_TX), DISP_RW);
        InitVarBind(GV, ' '         , 2, COL3, GP(GWS_RX), DISP_RO);
    }
#undef  GV
#undef  GP
}

void InitWifiVars(bool expert_mode) {
    int i = 0;
#define WP(id)  (&wifi_params[id])
#define GV      (&wifiVars[i++])
    if (expert_mode) {
        InitVarBind(GV, ' ',  8, COL1, WP(WIFI_SSID), DISP_RO);
        InitVarBind(GV, ' ',  9, COL1, WP(WIFI_MODE), DISP_RO);
        InitVarBind(GV, ' ', 10, COL1, WP(WIFI_BSSID), DISP_RO);
        InitVarBind(GV, ' ', 11, COL1, WP(WIFI_HARDWARE_NAME), DISP_RO);
        InitVarBind(GV, ' ',  8, COL2, WP(WIFI_HWMODELIST), DISP_RO);
        InitVarBind(GV, ' ',  9, COL2, WP(WIFI_ENCRYPTION), DISP_RO);
        InitVarBind(GV, SET_BANDWIDTH   , 10, COL2, WP(WIFI_CHANNEL), DISP_RW);
        InitVarBind(GV, WIFI_TXPW       , 11, COL2, WP(WIFI_TXPOWER), DISP_RW);
        InitVarBind(GV, ' ',  8, COL3, WP(WIFI_SIGNAL), DISP_RO);
        InitVarBind(GV, ' ',  9, COL3, WP(WIFI_QUALITY), DISP_RO);
        InitVarBind(GV, ' ', 10, COL3, WP(WIFI_NOISE), DISP_RO);
        InitVarBind(GV, ' ', 11, COL3, WP(WIFI_BITRATE), DISP_RO);
        InitVarBind(GV, ' ', START_ASSOC_LINE, COL1, WP(WIFI_ASSOCLIST), DISP_DN);
    } else {
        InitVarBind(GV, ' ', 3, COL1, WP(WIFI_SSID), DISP_RO);
        InitVarBind(GV, ' ', 4, COL1, WP(WIFI_BSSID), DISP_RO);
        InitVarBind(GV, ' ', 5, COL1, WP(WIFI_MODE), DISP_RO);
        InitVarBind(GV, ' ', 3, COL2, WP(WIFI_TXPOWER), DISP_RO);
        InitVarBind(GV, ' ', 4, COL2, WP(WIFI_SIGNAL), DISP_RO);
        InitVarBind(GV, ' ', 5, COL2, WP(WIFI_NOISE), DISP_RO);
        InitVarBind(GV, ' ', 3, COL3, WP(WIFI_QUALITY), DISP_RO);
        InitVarBind(GV, ' ', 4, COL3, WP(WIFI_BITRATE), DISP_RO);
        InitVarBind(GV, ' ', START_ASSOC_LINE, COL1, WP(WIFI_ASSOCLIST), DISP_DN);
    }
#undef  GV
#undef  WP
}

void InitParameters(bool expert_mode) {
    _b_expert_mode = expert_mode;
    InitGwsVars(_b_expert_mode);
    InitWifiVars(_b_expert_mode);
}

void ShutdownInstance() {
    sem_t*  sem = sem_open(SEM_NAME_GWSMAN_SERVER,O_RDWR,00777,1);
    if (SEM_FAILED != sem) {
        sem_post(sem);
        sem_unlink(SEM_NAME_GWSMAN_SERVER);
        sem_close(sem);
    }
}

VarBind* SearchVB(VarBind* vars, int maxItems, int index) {
    int i = 0;
    for (i = 0; i < maxItems; i++) {
        if (vars[i].m_index == index)
            return &vars[i];
    }
    return NULL;
}

VarBind* GetGwsVB(int index) {
    return SearchVB(gwsVars, MAX_GWS_VAR, index);
}

VarBind* GetIwVB(int index) {
    return SearchVB(wifiVars, MAX_WIFI_VAR, index);
}

void PrintHelp() {
    printf("\nUssage:   gwsman [-COMMAND] [parameter]");
    printf("\n-------------------------- COMMAND --------------------------------\n"
            "\tserver [port options] &      #work in server mode\n"
            "\tgui                          #work as user interface\n"
            "\t" CMD_SERVER_DOWN "                         #shutdown background server process\n"
            "\t" CMD_RADIO_TX " [on|off]                  #turn on/off radio Tx\n"
            "\t" CMD_RADIO_RX " [on|off]                  #turn on/off radio Rx\n"
            "\t" CMD_RADIO_TX_CAL " [on|off]            #turn on/off Tx calibration\n"
            "\t" CMD_RADIO_RX_CAL " [on|off]            #turn on/off Rx calibration\n"
            "\t" CMD_RADIO_CHANNEL " [channel_num(14-51)]\n"
            "\t" CMD_RADIO_POWER " [Tx power(14-33) in dBm]\n"
            "\t" CMD_RADIO_GAIN " [Rx gain(0-18) in dBm]\n"
            "\t" CMD_RADIO_ATTEN " [Rx attenuator]\n"
            "\t" CMD_WIFI_POWER " [dBm]\n"
            "\t" CMD_SET_MESHID " [id str]\n"
            "\t" CMD_GET_MESHID "\n"
            "\t" CMD_SET_SSID " [id str]\n"
            "\t" CMD_GET_SSID "\n"
            "\t" CMD_SET_KEY " [encryption key]\n"
            "\t" CMD_SET_STA_NO " [station no.]\n"
            "\t" CMD_GET_STA_NO "\n"
            "\t" CMD_GET_LOCAL_IP
            );
    printf("\n---------------- port options in server mode: ---------------------\n");
    printf("\n[o=COM Port] [r=Baud Rate] [b=Byte Size] [s=Stop Bits] [p=Parity Bit]");
    printf("\n\to = 1..8					(default=1)");
    printf("\n\tr = 1200/2400/4800/9600/19200/38400/57600/115200(default=9600)");
    printf("\n\tb = 7..8					(default=8)");
    printf("\n\ts = 1..2					(default=1)");
    printf("\n\tp = N|O|E					(default=N)");
    printf("\n-------------------------------------------------------------------\n");
    printf("\n\th ussage\n\n");
}

void SetKpiErrorNo(P_GWS_KPI pKpi, int nId,int err) {
    RW_KPI_VAR(nId,pKpi->m_error_no,err);
}

void SetKpiValue(P_GWS_KPI pKpi, int nId, int index, char* value) {
    int nV = 0;
    if (strstr(value, "ON") != NULL) nV = 1;
    else if (strstr(value, "OFF") != NULL) nV = 0;
    else nV = atoi(value);
    switch (index) {
        case GWS_FIRMWARE: RW_KPI_STR(nId, pKpi->m_radio.m_sFirmware, value);
            break;
        case GWS_BOARDSNO: RW_KPI_VAR(nId, pKpi->m_radio.m_nBoardSNO, nV);
            break;
        case GWS_REGION: RW_KPI_VAR(nId, pKpi->m_radio.m_nRegion, nV);
            break;
        case GWS_CHANNO: RW_KPI_VAR(nId, pKpi->m_radio.m_nChanNo, nV);
            break;
        case GWS_IFOUT: RW_KPI_VAR(nId, pKpi->m_radio.m_nIFOUT, nV);
            break;
        case GWS_AGCMODE: RW_KPI_VAR(nId, pKpi->m_radio.m_nAGCMode, nV);
            break;
        case GWS_TEMP: RW_KPI_VAR(nId, pKpi->m_radio.m_nTemp, nV);
            break;
        case GWS_TXCAL: RW_KPI_VAR(nId, pKpi->m_radio.m_bTXCal, nV);
            break;
        case GWS_RXCAL: RW_KPI_VAR(nId, pKpi->m_radio.m_bRXCal, nV);
            break;
        case GWS_TX: RW_KPI_VAR(nId, pKpi->m_radio.m_bTX, nV);
            break;
        case GWS_RX: RW_KPI_VAR(nId, pKpi->m_radio.m_bRX, nV);
            break;
        case GWS_CURTXPWR: RW_KPI_VAR(nId, pKpi->m_radio.m_nCurTxPwr, nV);
            break;
        case GWS_RXGAIN: RW_KPI_VAR(nId, pKpi->m_radio.m_nRXGain, nV);
            break;
        case GWS_BRDMAXPWR: RW_KPI_VAR(nId, pKpi->m_radio.m_nBrdMaxPwr, nV);
            break;
        case GWS_RXMAXGAIN: RW_KPI_VAR(nId, pKpi->m_radio.m_nRxMaxGain, nV);
            break;
        case GWS_TXATTEN: RW_KPI_VAR(nId, pKpi->m_radio.m_nTxAtten, nV);
            break;
        case GWS_RXFATTEN: RW_KPI_VAR(nId, pKpi->m_radio.m_nRxFAtten, nV);
            break;
        case GWS_MAXATTEN: RW_KPI_VAR(nId, pKpi->m_radio.m_nMaxAtten, nV);
            break;
        case GWS_RXRATTEN: RW_KPI_VAR(nId, pKpi->m_radio.m_nRxRAtten, nV);
            break;
        case GWS_MAXTXPWR: RW_KPI_VAR(nId, pKpi->m_radio.m_nMaxTxPwr, nV);
            break;
        case GWS_MINTXPWR: RW_KPI_VAR(nId, pKpi->m_radio.m_nMinTxPwr, nV);
            break;
        default:;
    }
}

bool CheckPeriod(int ticks, int period) {
    return (0 == (ticks % period));
}

bool TimeDiff(unsigned long nCurrTicks, unsigned long *recent, const unsigned long nTimeout) {
    if (*recent > nCurrTicks)
        *recent = nCurrTicks;
    if (nCurrTicks - *recent >= nTimeout) {
        *recent = nCurrTicks;
        return true;
    } else return false;
}
