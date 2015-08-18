#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <bits/signum.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

#include "gwsman.h"
#include "PipeShell.h"
#include "GuiMain.h"
#include "SvrMain.h"
#include "../include/readconf.h"

#define TIME_SLOT       50000   //us
#define MSEC_TICKS      1000 / TIME_SLOT
#define SECOND_TICKS    1000000 / TIME_SLOT

#define PERIOD_SWITCH   800
#define PERIOD_READ_NF  1200
#define PERIOD_TOTAL    (PERIOD_SWITCH + PERIOD_READ_NF)

#define GWS_SECOND      1000000 //us
#define GWS_TIMEOUT     (1 * GWS_SECOND)
#define GWS_INTERVEL    (3 * GWS_SECOND)
#define GWS_SLEEP_TIMER 50000   //us
#define GWS_SLEEP_COUNT (GWS_TIMEOUT / GWS_SLEEP_TIMER)
#define GWS_SLEEP_INTVL (GWS_INTERVEL / GWS_SLEEP_TIMER)

#define TASK_IDEL           0
#define TASK_COMMAND        0x01
#define TASK_POLLING_RADIO  0x02   //waiting for rfinfo query
#define TASK_UPDATE_RADIO   0x04
#define TASK_CHANGING_CHAN  0x08
#define TASK_UPDATE_NOISE   0x10

#define SCANNING_DONE           0
#define SCANNING_ALL_INIT       0x01
#define SCANNING_ALL_READ       0x02
#define SCANNING_FIXED_INIT     0x10
#define SCANNING_FIXED_READ     0x20
#define SCANNING_FIXED_DONE     0x30

#define COM_TEST    (static_chp.m_nCommBusy <= 0)
#define COM_LOCK(t) do {    \
            LED_SET(LED_RADIO,1);   \
            static_chp.m_nCommBusy = t;    \
        } while (0)
#define COM_RELEASE do {    \
            (static_chp.m_nCommBusy = 0);   \
        } while (0)
#define CHANNEL_SCAN_ON_OFF(_switch,_on_off)    do {     \
            char command[128];\
            _switch = _on_off;\
            sprintf(command,"echo scan %s > " DEBUG_INFO_BASE "chanscan",_switch ? "enable" : "disable");\
            system(command);\
            svrIsScanDone(_on_off);\
        } while (0)


#define TIMER_DISABLE           -1
#define DisableTimer(timer)     (timer = TIMER_DISABLE)
#define EnableTimer(timer,t)    (timer = t)
#define SetTimerOut(timer)      (timer = 0)
#define IsTimerRunning(timer)   (timer > 0)
#define IsTimerTimeout(timer)   (timer == 0)
#define IsTimerDisabled(timer)  (timer == TIMER_DISABLE)

//const static unsigned long TICKS_IN_250_MSECS = 250 * MSEC_TICKS;
static GWS_BUFF local_gws_buff;
static int  timer_radio_query = TIMER_DISABLE;
static int  timer_noise_floor = TIMER_DISABLE;
static int  timer_chan_switch = TIMER_DISABLE;
static int  timer_wifi_update = TIMER_DISABLE;
static pthread_mutex_t mutex_lock_timer;

CommandHandlerPara static_chp = { NULL,NULL,NULL,NULL,NULL,0,0,0,0,0,0 };

void svrSetErrorNo(int err) {
    SetKpiErrorNo(static_chp.m_pKpi,static_chp.m_nIdentifier,err);
}

bool svrInitInstance() {
    static_chp.m_semInstance = sem_open(SEM_NAME_GWSMAN_SERVER,O_CREAT|O_RDWR,00777,1);
    if (SEM_FAILED == static_chp.m_semInstance) {   //errno == EEXIST
        printf("\nFailed on initializing instance.\n\n");
        return false;
    } else {
        if (0 == sem_trywait(static_chp.m_semInstance)) {
            printf("\n<Server init OK>\n");
            return true;
        } else {
            printf("\nAn other gwsman is running.\n\n");
            return false;
        }
    }
}

void svrExitInstance() {
    if (SEM_FAILED != static_chp.m_semInstance) {
        sem_close(static_chp.m_semInstance);
        static_chp.m_semInstance = SEM_FAILED;
        Socket_Disconnect();
        printf("\nExit gwsman\n");
    }
}

bool svrTestInstance(sem_t* sem) {
    if (sem && 0 == sem_trywait(sem)) {
        sem_post(sem);
        return false;   //unlocked, means we can exit
    } else {
        return true;    //locked, we must continue our work
    }
}

typedef struct __struct_channel_info {
    short   m_ch_no;
    short   m_ch_nf;
} SORT_CHANNEL_INFO;

void svrSortExchange(SORT_CHANNEL_INFO* c0, SORT_CHANNEL_INFO* c1) {
    SORT_CHANNEL_INFO   ct;
    ct.m_ch_no = c0->m_ch_no;
    ct.m_ch_nf = c0->m_ch_nf;
    c0->m_ch_no = c1->m_ch_no;
    c0->m_ch_nf = c1->m_ch_nf;
    c1->m_ch_no = ct.m_ch_no;
    c1->m_ch_nf = ct.m_ch_nf;
}

SORT_CHANNEL_INFO* svrBubbleSortChannels(int region,GWS_CHAN_NF nf[]) {
    static SORT_CHANNEL_INFO    sort_channels[MAX_CH_NUM];
    const int                   MAX_CH = NUM_CHANNEL(region);
    int                         i = 0,j = 0;
    
    if (NULL == nf) return NULL;
    for (i = 0; i < MAX_CH; i ++) {
        sort_channels[i].m_ch_no = i + MIN_CHANNEL(region);
        sort_channels[i].m_ch_nf = nf[i].m_noise_avg;
    }
    for (j = 0; j < MAX_CH - 1; j ++) {
        for (i = 0; i < MAX_CH - 1 - j; i ++) {
            if (sort_channels[i].m_ch_nf > sort_channels[i+1].m_ch_nf) {
                svrSortExchange(&sort_channels[i],&sort_channels[i+1]);
            }
        }
    }
    return sort_channels;
}

bool svrUpdateChannels(bool _debug_output,int region, SORT_CHANNEL_INFO* chs) {
    FILE* fp = NULL;
    int i = 0 , group = 0;

    if (NULL != chs && (fp = fopen(SCAN_LOG_FILE,"w"))) {
        for (i = 0; i < NUM_CHANNEL(region); i ++) {
            if (0 == i) {
                group = 1;
                SVR_DEBUG(_debug_output,VT100_RESET VT100_STYLE_BLUE 
                            "\n[% 4d dBm] channel: %d ", chs[i].m_ch_nf,chs[i].m_ch_no);
            } else if (chs[i].m_ch_nf != chs[i-1].m_ch_nf) {
                group ++;
                SVR_DEBUG(_debug_output,"\n[% 4d dBm] channel: %d ",chs[i].m_ch_nf,chs[i].m_ch_no);
            } else {
                SVR_DEBUG(_debug_output,"%d ",chs[i].m_ch_no);
            }
            fprintf(fp, "%d,%d,%d\n", chs[i].m_ch_no, chs[i].m_ch_nf,group);
        }
        fclose(fp);
        return true;
    } else return false;
}

void svrParseRfInfo(char* res, P_GWS_KPI pKpi, int nId) {
    char* begin = NULL;
    char* sep = NULL;
    char* end = NULL;
    int i = 0, nParsed = 0, nRest = 0;

//    printf("\n***[%s]***\n",res);
    for (i = 0; i < MAX_GWS_VAR; i++) {
        if ((begin = strstr(res, gwsVars[i].m_title)) != NULL) {
//            printf("\n@1 Info:keyword[%s] found at %d\n",gwsVars[i].m_title,begin);
            if ((sep = strstr(begin, " #")) > begin) {
                sep = strstr(sep, "- ");
            } else sep = strstr(begin, ":");
            if (sep > begin) {
//                printf("\n@2 Info:seperator <:> found at %d\n",sep);
                if ((end = strstr(sep, "\n")) != NULL) {
//                    printf("\n@3 Info:\\n found at %d\n",end);
                    *end = 0x00;
                    SetKpiValue(pKpi, nId, gwsVars[i].m_index, sep + 2);
                    nParsed ++;
                    nRest = strlen(end + 1);
                    memmove(begin, end + 1, nRest + 1);
//                    printf("{%s = ",gwsVars[i].m_title);
//                    printf("%s}\n",VB_AsString(&gwsVars[i].m_value));
//                    printf("\n@4 After erase varbind <%s>\n",res);
                }
            }
        }
    }
    int rest = strlen(res);
    if (rest > 32) {
        memmove(res, res + rest - 32, 32);
        res[32] = 0;
    }
    if (nParsed) {
        LED_SET(LED_RADIO,0);
    }
}

static short  nSvrTimerBusy = 0;
static unsigned long nTimer_ticks = 0;
static unsigned long nRecentIwInfo = 0;
static unsigned long nRecentChanSW = 0;
static int idel_times = 0;

int svrQueryNoise(char* ifname, P_GWS_KPI pKpi, int nId) {
    int region = 0;
    short nChannel = iw_getcurrchan(pKpi, nId);
    RW_KPI_VAR(nId,region,pKpi->m_radio.m_nRegion);
    if (MIN_CHANNEL(region) <= nChannel && nChannel <= MAX_CHANNEL(region)) {
        return iw_noise(ifname, nChannel, pKpi, nId);
    }
    return MIN_CHANNEL_NOISE;
}

//#define IW_LONG_QUERY_TIMES (250 * MSEC_TICKS) = 5
#define IW_LONG_QUERY_TIMES 5

void svrQueryWifi(bool _debug_output,char* ifname, P_GWS_KPI pKpi, int nId, int nTicks) {
    static int index;
    char* sValue = NULL;

    if (0 == nTicks % IW_LONG_QUERY_TIMES) { //for some parameters,we only query once per IW_LONG_QUERY_TIMES ticks
        int idx = (nTicks / IW_LONG_QUERY_TIMES) % MAX_WIFI_VAR;
        sValue = iwinfo(ifname, wifiVars[idx].m_index, pKpi, nId);
        SVR_DEBUG(_debug_output,VT100_RESET VT100_STYLE_HOT "\n(%d)Query[%s][%s]=%s *\n", 
                                    nTicks,ifname,wifiVars[idx].m_title,sValue);
    } else {
        index ++;
        index %= MAX_WIFI_VAR;
        switch (wifiVars[index].m_index) {
//            case WIFI_TXPOWER:
//            case WIFI_BITRATE:
//            case WIFI_MODE:
            case WIFI_ASSOCLIST:
            case WIFI_SIGNAL:
            case WIFI_NOISE:
                sValue = iwinfo(ifname, wifiVars[index].m_index, pKpi, nId);
                SVR_DEBUG(_debug_output,VT100_RESET VT100_STYLE_HOT "(%d)Query[%s][%s]=%s\n", 
                                        nTicks,ifname,wifiVars[index].m_title,sValue);
                return;
            default:;
        }
    }
}

short svrSetChannel(SerialCom* comm,int region,short channel,int *nTimer) {
    char command[64];
    
    if (MIN_CHANNEL(region) <= channel && channel <= MAX_CHANNEL(region)) {
        EnableTimer(*nTimer,PERIOD_SWITCH * MSEC_TICKS);    //500 ms to ensure the pll done
        memset (command, 0x00, 64);
        sprintf(command, "setchan %d\n", channel);
        ScSend(comm, command, strlen(command));
        return channel;
    } else return MIN_CHANNEL(region);
}

void svrNextChannel(SerialCom* comm,CommandHandlerPara* para,int *nTimer) {
    P_GWS_KPI pKpi = para->m_pKpi;
    int nIdentifier = para->m_nIdentifier;
    int region = 0;
    if (para->m_nCommBusy <= 0) {   //need channel scan and no any other task
        RW_KPI_VAR(nIdentifier,region,pKpi->m_radio.m_nRegion);
        para->m_nCurrentChannel = iw_getcurrchan(pKpi, nIdentifier) + 1;
        if (para->m_nCurrentChannel < MIN_CHANNEL(region)) para->m_nCurrentChannel = MAX_CHANNEL(region);
        if (para->m_nCurrentChannel > MAX_CHANNEL(region)) para->m_nCurrentChannel = MIN_CHANNEL(region);
        if (para->m_nCurrentChannel >= MIN_CHANNEL(region)) {
            svrSetChannel(comm, region, para->m_nCurrentChannel,nTimer);
        }
    }// else return -1;   //channel dose not changed
}

void svrGwsRequest(bool _debug_output,SerialCom* comm,int* nTimer) {
    if (COM_TEST) {
        COM_LOCK(SECOND_TICKS);    //occupy serial port
        SVR_DEBUG(_debug_output,VT100_RESET VT100_INVERT "(%d,%03d)Radio query...\n",
                nTimer_ticks,static_chp.m_nCommBusy);
        static char* cmd = "rfinfo 2\n";
        ScSend(comm, cmd, strlen(cmd));
        EnableTimer(*nTimer,SECOND_TICKS);
    }
}

int svrGwsPolling(bool _debug_output,SerialCom* comm,int* nTimer) {
    int nData = 0;
    char sData[512];

    if (*nTimer > 0) {
        memset(sData, 0x00, 512);
        SVR_DEBUG(_debug_output,VT100_RESET VT100_INVERT "\b\b\b\b\b\b<%02d>", *nTimer);
        nData = ScRead(comm, sData, 512);
        SVR_DEBUG(_debug_output,"%d",nData);
        if (nData > 0) {
            idel_times = 0;
            BufAppend(&local_gws_buff, sData);
            return TASK_POLLING_RADIO;
        } else { //If we have read something, and after 5 times of reading nothing, 
            if (!BufIsEmpty(&local_gws_buff)) {
                idel_times ++;
            }
            if (idel_times >= 5) { //consider it was over
                BufAppend(&local_gws_buff, NULL);
                idel_times = 0;
                SetTimerOut(*nTimer);
                return TASK_UPDATE_RADIO;
            } else return TASK_POLLING_RADIO;
        }
    } else return TASK_IDEL;
}

void svrGwsParse(bool _debug_output,int* nTimer) {
    SVR_DEBUG(_debug_output,VT100_RESET VT100_INVERT "\n(%d,%03d)Radio Done", nTimer_ticks,static_chp.m_nCommBusy);
    char rfinfo[MAX_GWS_BUFF];

    memset (rfinfo, 0x00, MAX_GWS_BUFF);
    if (BufGetBuff(&local_gws_buff, rfinfo, true)) {
        svrParseRfInfo(rfinfo, static_chp.m_pKpi, static_chp.m_nIdentifier);
        if (!static_chp.m_uChanScan) {
            RW_KPI_VAR(static_chp.m_nIdentifier,static_chp.m_nOriginChannel,static_chp.m_pKpi->m_radio.m_nChanNo);
            iw_setchannel(static_chp.m_nOriginChannel,static_chp.m_pKpi,static_chp.m_nIdentifier);
        }
    }
    DisableTimer(*nTimer);
    COM_RELEASE;   //release serial port for other tasks
}

static GWS_MESSAGE latest_msg;
static int latest_rxcal = 1;
static int latest_rxgain = 12;
#define PRE_SCAN_COMMAND(comm,cmd_str)   \
    do {\
        char szCommand[64];\
        sprintf(szCommand, "%s\n", cmd_str);\
        ScSend(comm, szCommand, strlen(szCommand));\
        usleep(100000);\
    } while (0)

#define APP_SCAN_COMMAND(comm,channel)   \
    do {\
        char szCommand[64];\
        sprintf(szCommand, "setchan %d\n", channel);\
        ScSend(comm, szCommand, strlen(szCommand));\
        usleep(400000);\
        SVR_DEBUG(true,VT100_RESET VT100_STYLE_ALERT "\n--Channel Scan : %s",szCommand);\
        sprintf(szCommand, "setrxcal %d\n", latest_rxcal);\
        ScSend(comm, szCommand, strlen(szCommand));\
        usleep(400000);\
        SVR_DEBUG(true,VT100_RESET VT100_STYLE_ALERT "\n--Channel Scan : %s",szCommand);\
        sprintf(szCommand, "setrxgain %d\n", latest_rxgain * 2);\
        ScSend(comm, szCommand, strlen(szCommand));\
        usleep(400000);\
        SVR_DEBUG(true,VT100_RESET VT100_STYLE_ALERT "\n--Channel Scan : %s",szCommand);\
    } while (0)
        
//        SVR_DEBUG(true,VT100_RESET VT100_STYLE_ALERT "\n--Channel Scan : %s",szCommand);\
//

const int   MSG_SZ = sizeof (GWS_COMMAND);
int svrCmdHandler(SerialCom* comm,GWS_MESSAGE* gws_msg,CommandHandlerPara* para) {
    char command[64];
    char parameter[32];
    int* nTimer = &(para->m_nCommBusy);
    int region = 0;
    int nId = para->m_nIdentifier;
    P_GWS_KPI pKpi = para->m_pKpi;
    
    RW_KPI_VAR(nId,region,pKpi->m_radio.m_nRegion);
    strcpy(parameter, gws_msg->m_command.m_sPara);
    switch (gws_msg->m_command.m_nReq) {
        case SCAN_CHAN:
        case SCAN_FIXED_CHAN:
            if (MIN_CHANNEL(region) <= para->m_nOriginChannel && para->m_nOriginChannel <= MAX_CHANNEL(region)) {
                COM_LOCK(1000 * MSEC_TICKS);    //occupy serial port for 500 ms
                RW_KPI_VAR(nId,latest_rxcal,pKpi->m_radio.m_bRXCal);
                RW_KPI_VAR(nId,latest_rxgain,pKpi->m_radio.m_nRXGain);
                PRE_SCAN_COMMAND(comm,"setrxcal 0");
                PRE_SCAN_COMMAND(comm,"setrxgain 0");
                para->m_nCurrentChannel = para->m_nOriginChannel;
                CHANNEL_SCAN_ON_OFF(para->m_uChanScan,gws_msg->m_command.m_nReq == SCAN_FIXED_CHAN ?
                                                        SCANNING_FIXED_INIT : SCANNING_ALL_INIT);
                gws_msg->m_nType = 0;
            } else {
                svrSetErrorNo(GWS_ERROR_INVALIDATE_CHANNEL);
            }
            break;
        case STOP_CHAN:
            if (para->m_uChanScan == SCANNING_FIXED_READ) {
                CHANNEL_SCAN_ON_OFF(para->m_uChanScan, SCANNING_FIXED_DONE);
            } else {
                CHANNEL_SCAN_ON_OFF(para->m_uChanScan, SCANNING_DONE);
                APP_SCAN_COMMAND(comm,static_chp.m_nOriginChannel);
            }
            gws_msg->m_nType = 0;
            break;
        case SET_TXON:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "tx%s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
            break;
        case SET_RXON:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "rx%s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
            break;
        case SET_TXCAL:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "settxcal %s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
        case SET_RXCAL:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "setrxcal %s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
            break;
        case SET_REGION:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "setregion %s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
            break;
        case SET_CHAN:
            if (*nTimer <= 0) {
                int new_channel = para->m_nOriginChannel;
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                CHANNEL_SCAN_ON_OFF(para->m_uChanScan,false);
                if (IsNumber(parameter)) {
                    new_channel = atoi(parameter);
                } else if (parameter[0] == 'x') {
                    new_channel --;
                    if (new_channel < MIN_CHANNEL(region)) new_channel = MAX_CHANNEL(region);
                } else if (parameter[0] == 'v') {
                    new_channel ++;
                    if (new_channel > MAX_CHANNEL(region)) new_channel = MIN_CHANNEL(region);
                }
                para->m_nCurrentChannel = new_channel;
                para->m_nOriginChannel = svrSetChannel(comm, region,para->m_nCurrentChannel,&timer_chan_switch);
                gws_msg->m_nType = 0;
            }
            break;
        case SET_TXPWR:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "settxpwr %s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
            break;
        case SET_GAIN:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "setrxgain %s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
            break;
        case SET_MODE:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "config_%s", parameter);
                system(command);
                usleep(1000000);
                system("reboot");
                gws_msg->m_nType = 0;
            }
            break;
        case SET_TXATT:
            if (*nTimer <= 0) {
                COM_LOCK(500 * MSEC_TICKS);    //occupy serial port for 500 ms
                sprintf(command, "settxatten %s\n", parameter);
                ScSend(comm, command, strlen(command));
                gws_msg->m_nType = 0;
            }
            break;
        case SET_BANDWIDTH:
            set_wifi_bandwidth(parameter);
            gws_msg->m_nType = 0;
            break;
        case WIFI_TXPW:
            PipeShellCommand(   para->m_shell, "iw", "dev",
                                para->m_ifName, "set", "txpower", "fixed",
                                gws_msg->m_command.m_sPara, NULL);
            gws_msg->m_nType = 0;
            break;
        case 'Q':
        case 'q':
            CHANNEL_SCAN_ON_OFF(para->m_uChanScan,false);
            SetTimerOut(*nTimer);
            gws_msg->m_nType = 0;
            break;
        case SHUTDOWN:
            #ifdef  _GWS_DEBUG
            ShowStatusBar("Shutdown Server.");
            #endif
            gws_msg->m_nType = 0;
            return 0;
        default:
            gws_msg->m_nType = 0;
    }
    return 1;
}

int svrIsScanning() {
    return static_chp.m_uChanScan;
}

void svrExitCommandHandler() {
    struct itimerval interval;

    interval.it_value.tv_sec = 0;
    interval.it_value.tv_usec = 0;
    interval.it_interval.tv_sec = 0;
    interval.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &interval, NULL);
    if (static_chp.m_pKpi) {
        CloseShm(static_chp.m_pKpi);
        static_chp.m_pKpi = NULL;
    }
    if (static_chp.m_nIdentifier) {
        DestoryShm(static_chp.m_nIdentifier);
        static_chp.m_nIdentifier = 0;
    }
    if (static_chp.m_nQid) {
        CloseMessageQueue(static_chp.m_nQid);
        static_chp.m_nQid = 0;
    }
    pthread_mutex_destroy(&mutex_lock_timer);
    PipeShellExit(static_chp.m_shell);
    ScClose(static_chp.m_comm);
    ScExit(static_chp.m_comm);
    BufExit(&local_gws_buff);
    svrClearSysMon();
    ShutdownInstance();
}

static int num_chan_scan_done = 0;
static bool cur_chan_scan_done = true;
void svrNoiseFloorSwitchChan(bool _debug_output,SerialCom* comm,int *timerSwitch) {
    if (COM_TEST) {
        //switch channel periodically for noise floor detecting
        SVR_DEBUG(_debug_output,VT100_RESET VT100_STYLE_ALERT "\n(%d,%03d)Switch channel      ",nTimer_ticks,static_chp.m_nCommBusy);
        svrNextChannel(comm,&static_chp,timerSwitch);
        PRE_SCAN_COMMAND(comm,"setrxatten 0 0");
        PRE_SCAN_COMMAND(comm,"setrxatten 1 36");
        cur_chan_scan_done = false;
        COM_LOCK(PERIOD_SWITCH * MSEC_TICKS);    //occupy serial port for 500 ms
    }
}

void svrNoiseFloorSetAvailable(bool _debug_output,int *timerSwitch, int *timerNoise) {
    SVR_DEBUG(_debug_output,VT100_RESET VT100_STYLE_ALERT "\n(%d,%03d)to channel %d\n",
            nTimer_ticks,static_chp.m_nCommBusy,static_chp.m_nCurrentChannel);
    iw_setchannel(static_chp.m_nCurrentChannel,static_chp.m_pKpi,static_chp.m_nIdentifier);
    svrQueryNoise(NULL,static_chp.m_pKpi,static_chp.m_nIdentifier);                 //reset noise floor
    DisableTimer(*timerSwitch);
    EnableTimer(*timerNoise,PERIOD_READ_NF * MSEC_TICKS); //read noise floor in the comming 1000 msecs
}

int svrNoiseFloorRead(bool _debug_output,char* ifName,P_GWS_KPI kpi,int nIdentifier) {
    int nf = svrQueryNoise(ifName,kpi,nIdentifier);     //parse noise only
    int region = 0;
    int ch = -1;
    RW_KPI_VAR(nIdentifier,region,kpi->m_radio.m_nRegion);
    ch = static_chp.m_nCurrentChannel - MIN_CHANNEL(region);
    if (ch >= 0) {
        SVR_DEBUG(_debug_output,VT100_RESET VT100_STYLE_MENU "[%d] ", nf);
    }
    return nf;
}

void svrNoiseFloorDone(bool _debug_output,SerialCom* comm,int* nTimer,char *uScan) {
    DisableTimer(*nTimer);
    int region = 0;
    int ch = -1;
    
    RW_KPI_VAR(static_chp.m_nIdentifier,region,static_chp.m_pKpi->m_radio.m_nRegion);
    ch = static_chp.m_nCurrentChannel - MIN_CHANNEL(region);
    SVR_DEBUG(_debug_output,VT100_RESET VT100_STYLE2 \
            "\n(%d,%03d)Reading NF done.(Org chan = %d ,Curr chan = %d , Avg NF = %d dBm)",   \
            nTimer_ticks,static_chp.m_nCommBusy,
            static_chp.m_nOriginChannel,
            static_chp.m_nCurrentChannel,
            static_chp.m_pKpi->m_noise_floor[ch].m_noise_avg);
    cur_chan_scan_done = true;
    if (num_chan_scan_done >= NUM_CHANNEL(region) - 1) {
        SORT_CHANNEL_INFO*  s_ch_info = NULL;
        RW_KPI_VAR(static_chp.m_nIdentifier,static_chp.m_pKpi->m_noise_floor[ch].m_noise_cur,
                                            static_chp.m_pKpi->m_noise_floor[ch].m_noise_avg);
        num_chan_scan_done = 0;
        CHANNEL_SCAN_ON_OFF(*uScan,SCANNING_DONE);
        APP_SCAN_COMMAND(comm,static_chp.m_nOriginChannel);
        s_ch_info = svrBubbleSortChannels(region,static_chp.m_pKpi->m_noise_floor);
        svrUpdateChannels(_debug_output,region,s_ch_info);
    } else {
        num_chan_scan_done ++;
        CHANNEL_SCAN_ON_OFF(*uScan,SCANNING_ALL_INIT);
    }
}

void svrIsScanDone(int state) {
    FILE*   fd_scan = fopen("/var/run/chanscan.sig","w");
    time_t  current = time(&current);
    if (fd_scan) {
        fprintf(fd_scan,"%d\n",state);
        fclose(fd_scan);
    }
}

void svrIamAlive() {
    FILE*   fd_sig = fopen("/var/run/gwsman.sig","w");
    time_t  current = time(&current);
    if (fd_sig) {
        fprintf(fd_sig,"%d\n",current);
        fclose(fd_sig);
    }
}

static bool static_debug_gws = false;

bool svrIncreaseTicks(int sig) {
    if (SIGALRM == sig) {
        nTimer_ticks ++;
        if (IsTimerRunning(static_chp.m_nCommBusy))   static_chp.m_nCommBusy --;
        if (IsTimerRunning(timer_noise_floor))  timer_noise_floor --;
        if (IsTimerRunning(timer_radio_query))  timer_radio_query --;
        if (IsTimerRunning(timer_chan_switch))  timer_chan_switch --;
        if (IsTimerRunning(timer_wifi_update))  timer_wifi_update --;
        return true;
    } else return false;
}

void svrTimer(int sig) {
    if (0 == pthread_mutex_trylock(&mutex_lock_timer)) {
        SerialCom*  comm = static_chp.m_comm;
        int         nQid = static_chp.m_nQid;
        bool        dbg = static_debug_gws;

        if (!svrIncreaseTicks(sig)) {
            pthread_mutex_unlock(&mutex_lock_timer);
            return;
        }
        if (CheckPeriod(nTimer_ticks,2 * SECOND_TICKS)) {
            svrIamAlive(-1);
        }
        const int nNoiseScanPeriod = PERIOD_TOTAL * MSEC_TICKS;
        switch (svrIsScanning()) {
            case SCANNING_FIXED_INIT:           //fixed channel scanning...
                svrQueryNoise(NULL,static_chp.m_pKpi,static_chp.m_nIdentifier);                 //reset noise floor
                PRE_SCAN_COMMAND(comm,"setrxatten 0 0");
                PRE_SCAN_COMMAND(comm,"setrxatten 1 36");
                EnableTimer(timer_chan_switch,2000 * MSEC_TICKS);    //500 ms to ensure the pll done
                static_chp.m_uChanScan = SCANNING_FIXED_READ;
            break;
            case SCANNING_FIXED_READ:           //channel scanning...
                if (IsTimerTimeout(timer_chan_switch))
                    svrNoiseFloorRead(dbg,static_chp.m_ifName, static_chp.m_pKpi, static_chp.m_nIdentifier);
            break;
            case SCANNING_FIXED_DONE:   do {    //channel scan done
                int region = 0;
                int ch = -1;
                RW_KPI_VAR(static_chp.m_nIdentifier,region,static_chp.m_pKpi->m_radio.m_nRegion);
                ch = static_chp.m_nCurrentChannel - MIN_CHANNEL(region);
                RW_KPI_VAR(static_chp.m_nIdentifier,static_chp.m_pKpi->m_noise_floor[ch].m_noise_cur,
                                                    static_chp.m_pKpi->m_noise_floor[ch].m_noise_avg);
                CHANNEL_SCAN_ON_OFF(static_chp.m_uChanScan,SCANNING_DONE);
                APP_SCAN_COMMAND(comm,static_chp.m_nOriginChannel);
            } while (0);
            break;
            case SCANNING_ALL_INIT:             //channel scanning...
                if (TimeDiff(nTimer_ticks, &nRecentChanSW, nNoiseScanPeriod)) { //handler for noise scan
                    svrNoiseFloorSwitchChan(dbg,comm,&timer_chan_switch);
                } else if (IsTimerTimeout(timer_chan_switch)) {    //switch done, notify that noise available
                    svrNoiseFloorSetAvailable(dbg,&timer_chan_switch,&timer_noise_floor);
                    static_chp.m_uChanScan = SCANNING_ALL_READ;
                }
            break;
            case SCANNING_ALL_READ:             //channel scanning...
                if (IsTimerDisabled(timer_chan_switch) && IsTimerRunning(timer_noise_floor)) {
                    svrNoiseFloorRead(dbg,static_chp.m_ifName, static_chp.m_pKpi, static_chp.m_nIdentifier);
                } else if (IsTimerTimeout(timer_noise_floor)) {
                    svrNoiseFloorDone(dbg,comm,&timer_noise_floor,&static_chp.m_uChanScan);
                }
            break;
            default:                            //routine...
                if (CheckPeriod(nTimer_ticks,3 * SECOND_TICKS)) {   //radio polling.....
                    svrGwsRequest(dbg,comm,&timer_radio_query);
                } else if (IsTimerRunning(timer_radio_query)) {    //within radio query timeout
                    svrGwsPolling(dbg,comm,&timer_radio_query);
                } else if (IsTimerTimeout(timer_radio_query)) {   //radio query done or timeout
                    svrGwsParse(dbg,&timer_radio_query);
                }
                if (IsTimerDisabled(timer_noise_floor)) { // ensure noise is not reading currently
                    svrQueryWifi(dbg,static_chp.m_ifName, static_chp.m_pKpi, static_chp.m_nIdentifier,nTimer_ticks);
                }
        }
        if (TASK_COMMAND == latest_msg.m_nType) {
            if (0 == svrCmdHandler(comm,&latest_msg,&static_chp)) {
                SVR_DEBUG(dbg,VT100_RESET VT100_STYLE_HOT "\nDone : idel = %d\n",nSvrTimerBusy);
                pthread_mutex_unlock(&mutex_lock_timer);
                svrExitCommandHandler();
                return;
            }
        }
        if (COM_TEST) { //latest message has already done
            if (msgrcv(nQid, &latest_msg, MSG_SZ, latest_msg.m_nType, IPC_NOWAIT) > 0) {
                //no task currently, try to detect new task
            } else {    //no new message, we can handle the channel scan things
            }
        }   //else, consider there is still a task not handled yet
        pthread_mutex_unlock(&mutex_lock_timer);
    }
    return;
}
#undef  COM_TEST
#undef  COM_LOCK
#undef  COM_RELEASE

bool svrInitCommandHandler(SerialCom* comm,PipeShell* shell,char* sIfName,bool dbg_gws) {
    struct itimerval interval;

    ShowStatusBar("%s", "Handler Init....\n");
    if (NULL == comm || NULL == shell || NULL == sIfName) return false;
    static_chp.m_comm = comm;
    static_chp.m_shell = shell;
    static_chp.m_ifName = sIfName;
    static_chp.m_nQid = OpenMessageQueue(1);
    static_chp.m_pKpi = CreateShm(&static_chp.m_nIdentifier);

    ShowStatusBar("%s", "MSQ & SHM OK\n");
    if (static_chp.m_nQid < 0 || NULL == static_chp.m_pKpi) {
        if (static_chp.m_nQid < 0) {
            ShowStatusBar("Error on create message queue,error=%d", errno);
            svrSetErrorNo(errno);
            return false;
        } else if (NULL == static_chp.m_pKpi) {
            ShowStatusBar("Error on open share memory,error=%d", errno);
            svrSetErrorNo(errno);
            return false;
        }
    }
    ShowStatusBar("%s", "Turnning on GWS Tx....\n");
    ScSend(static_chp.m_comm, "txon\n", 5);
    ShowStatusBar("%s", "Adjusting GWS channel....\n");
    iw_setchannel(-1,static_chp.m_pKpi,static_chp.m_nIdentifier);
    
    pthread_mutex_init(&mutex_lock_timer,NULL);
    interval.it_value.tv_sec = 0;
    interval.it_value.tv_usec = TIME_SLOT;
    interval.it_interval.tv_sec = 0;
    interval.it_interval.tv_usec = TIME_SLOT;
    static_debug_gws = dbg_gws;
    signal(SIGALRM, svrTimer);
    setitimer(ITIMER_REAL, &interval, NULL);
    ShowStatusBar("%s", "Initialize Handler OK!");
    return true;
}

void ReadCmdLine(int argc, char* argv[], int* nPort, int* nBaud, int* nBits, int* nStop, char* cParity,
                        bool* debug_lcd, bool* debug_neg) {
    int i = 0;
    for (i = 1; i < argc; i++) {
        char* szArgv = argv[i];
        char* p = NULL;
        if ((p = strstr(szArgv, "port=")) != NULL) {
            sscanf(p + 5, "%d", nPort);
            if (*nPort < 1 || *nPort > 8) {
                printf("\nInvalidate serial port:%d\n", *nPort);
                PrintHelp();
                return;
            }
        } else if ((p = strstr(szArgv, "baud=")) != NULL) {
            sscanf(p + 5, "%d", nBaud);
            if (*nBaud != 1200 && *nBaud != 2400 && *nBaud != 4800 &&
                    *nBaud != 9600 && *nBaud != 19200 && *nBaud != 38400 &&
                    *nBaud != 57600 && *nBaud != 115200) {
                printf("\nInvalidate bard rate:%d\n", *nBaud);
                PrintHelp();
                return;
            }
        } else if ((p = strstr(szArgv, "bits=")) != NULL) {
            sscanf(p + 5, "%d", nBits);
            if (*nBits != 7 && *nBits != 8) {
                printf("\nInvalidate data bits:%d\n", *nBits);
                PrintHelp();
                return;
            }
        } else if ((p = strstr(szArgv, "stop=")) != NULL) {
            sscanf(p + 5, "%d", nStop);
            if (*nStop != 1 && *nStop != 2) {
                printf("\nInvalidate stop bits:%d\n", *nStop);
                PrintHelp();
                return;
            }
        } else if ((p = strstr(szArgv, "parity=")) != NULL) {
            sscanf(p + 7, "%c", cParity);
            if (*cParity != 'N' && *cParity != 'n' && *cParity != 'O' &&
                    *cParity != 'o' && *cParity != 'E' && *cParity != 'e') {
                printf("\nInvalidate parity:%c\n", *cParity);
                PrintHelp();
                return;
            }
        } else if ((p = strstr(szArgv, "tracelcd")) != NULL) {
            if (debug_lcd) *debug_lcd = true;
        } else if ((p = strstr(szArgv, "traceneg")) != NULL) {
            if (debug_neg) *debug_neg = true;
        }
    }
}

void ReadFileConf(SerialCom* comm,
        int* nPort, int* nBaud, int* nBits, int* nStop, char* cParity,
        char* ifname, char* lcddev, int* distance, int* enable_route,
        bool* debug_gws, bool* debug_lcd, bool* debug_neg) {
    int i = 0;
    int oPort = *nPort, oBaud = *nBaud, oBits = *nBits, oStop = *nStop;
    char oParity = *cParity;
    char sParamValue[MAX_VALUE_SIZE];
    memset(sParamValue, 0x00, MAX_VALUE_SIZE);
    for (i = 1; i <= 16; i++) {
        char sKey[13];
        memset(sKey, 0x00, 13);
        sprintf(sKey, "NameOfPort%d", i);
        if (0 == read_cfg(CONFIG_FILE, sKey, sParamValue)) {
            printf("\nmanage GWS by port %d(device name : %s)\n", i, sParamValue);
            ScSetDevName(comm, i - 1, sParamValue);
        }
    }
    if (0 == read_cfg(CONFIG_FILE, "IfName", sParamValue)) strcpy(ifname, sParamValue);
    if (0 == read_cfg(CONFIG_FILE, "LcdDev", sParamValue)) strcpy(lcddev, sParamValue);
    if (0 == read_cfg(CONFIG_FILE, "Port", sParamValue)) sscanf(sParamValue, "%d", nPort);
    if (0 == read_cfg(CONFIG_FILE, "Baud", sParamValue)) sscanf(sParamValue, "%d", nBaud);
    if (0 == read_cfg(CONFIG_FILE, "Byte", sParamValue)) sscanf(sParamValue, "%d", nBits);
    if (0 == read_cfg(CONFIG_FILE, "Stop", sParamValue)) sscanf(sParamValue, "%d", nStop);
    if (0 == read_cfg(CONFIG_FILE, "Parity", sParamValue)) sscanf(sParamValue, "%c", cParity);
    if (0 == read_cfg(CONFIG_FILE, "Distance", sParamValue)) sscanf(sParamValue, "%d", distance);
    if (0 == read_cfg(CONFIG_FILE, "MeshRoute", sParamValue)) sscanf(sParamValue, "%x", enable_route);
    if (0 == read_cfg(CONFIG_FILE, "TraceGws", sParamValue)) sscanf(sParamValue, "%d", debug_gws);
    if (0 == read_cfg(CONFIG_FILE, "TraceLcd", sParamValue)) sscanf(sParamValue, "%d", debug_lcd);
    if (0 == read_cfg(CONFIG_FILE, "TraceNeg", sParamValue)) sscanf(sParamValue, "%d", debug_neg);
    if (*nBaud != 1200 && *nBaud != 2400 && *nBaud != 4800 &&
            *nBaud != 9600 && *nBaud != 19200 && *nBaud != 38400 &&
            *nBaud != 57600 && *nBaud != 115200) *nBaud = oBaud;
    if (*nBits != 7 && *nBits != 8) *nBits = oBits;
    if (*nStop != 1 && *nStop != 2) *nStop = oStop;
    if (*cParity != 'N' && *cParity != 'n' && *cParity != 'O' &&
            *cParity != 'o' && *cParity != 'E' && *cParity != 'e') *cParity = oParity;
}

int ActingAsServer(int argc, char* argv[], bool dbg) {
    SerialCom comm;
    PipeShell shell;
    int nPort = 1;
    int nBaud = 9600;
    int nBits = 8;
    int nStop = 1;
    char cParity = 'N';
    char sIfName[32];
    char sLcdDev[32];
    int distance = 26000;
    int neg_mode = 0;
    bool trace_gws = dbg;
    bool trace_lcd = false;
    bool trace_neg = false;

    memset(sIfName, 0x00, 32);
    memset(sLcdDev, 0x00, 32);
    strcpy(sIfName, "wlan0");
    strcpy(sLcdDev, "/dev/ttyS0");
    
    if (svrInitInstance()) {
        InitParameters(true);
        BufInit(&local_gws_buff);
        ScInit(&comm, 0); //nonblock mode
        ReadFileConf(&comm, &nPort, &nBaud, &nBits, &nStop, &cParity,
                        sIfName,sLcdDev,&distance,&neg_mode,
                        &trace_gws,&trace_lcd,&trace_neg);
        ReadCmdLine(argc, argv, &nPort, &nBaud, &nBits, &nStop, &cParity,
                        &trace_lcd,&trace_neg);
        if (false == ScOpen(&comm, nPort, nBaud, nBits, nStop, cParity)) {
            ShowStatusBar("%s", "Can not open GWS radio port.\n");
        } else {
            ShowStatusBar("%s", "GWS port OK.\n");
        }
        PipeShellInit(&shell);
        svrInitCommandHandler(&comm,&shell,sIfName,trace_gws);
        ShowStatusBar("%s" VT100_RESET "\n", "gwsman server ready.\n");
        if (lcd_main(sLcdDev,distance,neg_mode,trace_lcd,trace_neg) < 0) {
            printf("\n\n");   fflush(stdout);
            while (svrTestInstance(static_chp.m_semInstance)) usleep(100000);
        } else fflush(stdout);
        ScExit(&comm);
        BufExit(&local_gws_buff);
        svrExitCommandHandler();
    }
    svrExitInstance();
    return 1;
}

