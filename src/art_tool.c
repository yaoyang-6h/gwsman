/* 
 * File:   main.c
 * Author: wyy
 *
 * Created on 2015年4月15日, 上午9:58
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include "ah.h"
#include "ah_internal.h"
#include "ah_osdep.h"
#include "ar9300eep.h"
#include "PipeShell.h"
#include "../include/wsocket.h"
#include "../include/vt100.h"
#include "../include/gwsman.h"
#include "../include/GuiMain.h"
/*
 * 
 */
//-------------------------------------------------------------------------------------------------------------------
//Rate table:
//6[0]  9[1]    12[2]   18[3]   24[4]   36[5]   48[6]   54[7]   1l[8]   2l[9]   2s[10]  5l[11]  5s[12]  11l[13] 11s[14]

//t0, mcs0[32]      t1, mcs1[33]        t2, mcs2[34]        t3, mcs3[35]        t4, mcs4[36]        t5, mcs5[37]
//t6, mcs6[38]      t7, mcs7[39]        t8, mcs8[40]        t9, mcs9[41]        t10, mcs10[42]      t11, mcs11[43]
//t12, mcs12[44]    t13, mcs13[45]      t14, mcs14[46]      t15, mcs15[47]      t16, mcs16[48]      t17, mcs17[49]
//t18, mcs18[50]    t19, mcs19[51]      t20, mcs20[52]      t21, mcs21[53]      t22, mcs22[54]      t23, mcs23[55]

//f0, mcs0/40[64]   f1, mcs1/40[65]     f2, mcs2/40[66]     f3, mcs3/40[67]     f4, mcs4/40[68]     f5, mcs5/40[69]
//f6, mcs6/40[70]   f7, mcs7/40[71]     f8, mcs8/40[72]     f9, mcs9/40[73]     f10, mcs10/40[74]   f11, mcs11/40[75]
//f12, mcs12/40[76] f13, mcs13/40[77]   f14, mcs14/40[78]   f15, mcs15/40[79]   f16, mcs16/40[80]   f17, mcs17/40[81]
//f18, mcs18/40[82] f19, mcs19/40[83]   f20, mcs20/40[84]   f21, mcs21/40[85]   f22, mcs22/40[86]   f23, mcs23/40[87]

//all[1000]   legacy[1001]    ht20[1002]  ht40[1003]
//-------------------------------------------------------------------------------------------------------------------
//Transmit parameters:tx                            f--frequency;
//ir--interleave packets from different rates(0/1); gi--use short guard interval(0/1);
//txch--chain mask used for transmit;               rxch--chain mask used for receive;
//pc--the number of packets sent(-1 = forever);     pl--the length of the packets;
//bc--if set to 1 the packets are broadcast;        retry--the number of times a packet is retransmitted;
//att--the attenuation between GOLDEN & DUT;        iss--the expected input signal strength at the dut;
//stat--statistic. from 0 to 3, 3 by default;       ifs--spacing between frames. tx100[0],tx99[1];
//dur--the maximum duration of the operation;       dump--the number of bytes of each packet displayed int the nart log;
//pro--if set to 1, all packet types are received;  bssid--the bssid used by the transmitter and receiver;
//mactx--the mac address used by the transmitter;   macrx--the mac address used by the receiver;
//deaf--disable receiver during transmission;       reset--reset device before operation;
//agg--the number of aggregated packets;            cal--calibrate transmit power;
//ht40--use 40MHz channel;                          r--the data rates used
//pcdac--the tx gain used by the transmitter;       pdgain--pdgain
//-------------------------------------------------------------------------------------------------------------------

#define NSHELL_CMD_INIT   "hello"
#define NSHELL_CMD_LOAD   "load devid=-1; caldata=auto;"
#define NSHELL_CMD_TX_BASE \
        "tx f=%d;ir=0;gi=%d;txch=%d;rxch=%d;pc=-1;pl=1000;bc=0;retry=3;"\
        "att=0;iss=0;stat=3;ifs=%d;dur=-1;dump=0;pro=1;"\
        "bssid=50.55.55.55.55.05;mactx=20.22.22.22.22.02;macrx=10.11.11.11.11.01;"\
        "deaf=0;reset=-1;agg=1;cal=1;ht40=0;r=%s;"
#define NSHELL_CMD_TX_PWR   NSHELL_CMD_TX_BASE "txp=%2.1f;"
#define NSHELL_CMD_TX_GAIN  NSHELL_CMD_TX_BASE "txgain=%d;"
#define NSHELL_CMD_RX \
        "rx f=%d;ir=0;gi=%d;txch=%d;rxch=%d;pc=-1;pl=1000;bc=0;retry=3;"\
        "att=0;iss=%d;stat=3;ifs=1;dur=-1;dump=0;pro=1;"\
        "bssid=50.55.55.55.55.05;mactx=20.22.22.22.22.02;macrx=10.11.11.11.11.01;"\
        "deaf=0;reset=-1;agg=1;cal=1;ht40=0;r=%s;tp=%2.1f;txgain=%d;"

#define NSHELL_CMD_START  "START"
#define NSHELL_CMD_STOP   "STOP"
#define NSHELL_CMD_EXIT   "unload"

#define NSHELL_CMD_LN "\r\n"
#define SLEEP_MS        1000
#define SLEEP_SEC       (1000 * SLEEP_MS)
static bool static_is_nart_connected = false;
static bool static_info_enable = false;
static int static_debug = 0;
static int static_WorkingMode = ART_MODE_UNLOAD;
static bool static_Wifi_up = true;
static PipeShell static_pipe_shell;

#define SHELL_CMD(fmt,...)  do {\
        char nart_cmd[128];\
        memset (nart_cmd,0x00,128);\
        sprintf(nart_cmd,fmt,##__VA_ARGS__);\
        system (nart_cmd);\
        WYY_LOG_INFO("%s\n",nart_cmd);\
     } while (0)

#define STATUS_LINE     "24"
#define SYSTEM_LOG_LINE "22"
#define ShowArtStatusBar(fmt,...)        do {\
            printf(VT100_GOTO(STATUS_LINE,"1"));\
            printf(VT100_RESET VT100_INVERT VT100_STYLE_BLUE fmt VT100_RESET,##__VA_ARGS__); \
            printf(VT100_LOAD_CURSOR);\
        } while (0)

#define ShowArtSystemLog(fmt,...)       do {\
        if (static_debug > 0) {\
            printf(VT100_GOTO(SYSTEM_LOG_LINE,"1"));\
            printf(VT100_RESET VT100_INVERT VT100_STYLE_MENU fmt VT100_RESET,##__VA_ARGS__); \
            printf(VT100_LOAD_CURSOR);\
        }\
    } while (0)

#define WYY_LOG_INFO(fmt,...)   do {\
        if (static_info_enable) {\
            printf(VT100_GOTO(SYSTEM_LOG_LINE,"1"));\
            printf(VT100_RESET VT100_INVERT VT100_STYLE_KEY fmt VT100_RESET,##__VA_ARGS__); \
            printf(VT100_LOAD_CURSOR);\
        }\
    } while (0)

char* art2_n2a(int value) {
    static char strValue[32];
    sprintf(strValue,"%d",value);
    return strValue;
}

char* build_tx_command(int freq,bool sgi,int txmask,int rxmask,bool tx99,char* rate,double power,int txgain) {
    static char tx_command[1024];
    
    memset (tx_command,0x00,1024);
    if (txgain >= 0)
        sprintf(tx_command,NSHELL_CMD_TX_GAIN,freq,!!sgi,txmask,rxmask,tx99,rate,txgain);
    else sprintf(tx_command,NSHELL_CMD_TX_PWR,freq,!!sgi,txmask,rxmask,tx99,rate,power);
    return tx_command;
}

char* build_rx_command(int freq,bool sgi,int txmask,int rxmask,int iss,char* rate,double power,int txgain) {
    static char rx_command[1024];
    
    memset (rx_command,0x00,1024);
    sprintf(rx_command,NSHELL_CMD_RX,freq,!!sgi,txmask,rxmask,iss,rate,power,txgain);
    return rx_command;
}

int nart_exec(char* cmd) {
    if (static_is_nart_connected) {
        char    nart_command[1024];
        memset (nart_command,0x00,1024);
        sprintf(nart_command,"%s" NSHELL_CMD_LN,cmd);
        Socket_SendData(nart_command, strlen(nart_command));
        usleep(50000);
    } else return -1;
    return 0;
}

static char* data_rates_table_legacy[] = {
"6",    "9",    "12",   "18",   "24",   "36",   "48",   "54",
"1l",   "2l",   "2s",   "5l",   "5s",   "11l",  "11s"
};
static char* data_rates_table_ht20_1[] = {
"t0",   "t1",   "t2",   "t3",   "t4",   "t5",   "t6",   "t7"
};
static char* data_rates_table_ht20_3[] = {
"t0",   "t1",   "t2",   "t3",   "t4",   "t5",   "t6",   "t7",
"t8",   "t9",   "t10",  "t11",  "t12",  "t13",  "t14",  "t15"
};
static char* data_rates_table_ht40[] = {
"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23"
};
static char* data_rates_table_all[] = {
"6",    "9",    "12",   "18",   "24",   "36",   "48",   "54",
"1l",   "2l",   "2s",   "5l",   "5s",   "11l",  "11s",
"t0",   "t1",   "t2",   "t3",   "t4",   "t5",   "t6",   "t7",
"t8",   "t9",   "t10",  "t11",  "t12",  "t13",  "t14",  "t15",
"t16",  "t17",  "t18",  "t19",  "t20",  "t21",  "t22",  "t23",
"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23"
};
static char* data_rates_table_desc[] = {
"6",    "9",    "12",   "18",   "24",   "36",   "48",   "54",
"1l",   "2l",   "2s",   "5l",   "5s",   "11l",  "11s",
"t0",   "t1",   "t2",   "t3",   "t4",   "t5",   "t6",   "t7",
"t8",   "t9",   "t10",  "t11",  "t12",  "t13",  "t14",  "t15",
"t16",  "t17",  "t18",  "t19",  "t20",  "t21",  "t22",  "t23",
"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
"all",  "legacy",   "ht20", "ht40"
};

#define SERCH_IN_RATE_TABLE(table,rate,match)\
    do {\
        int nItem = sizeof(table) / sizeof(table[0]);\
        int i = 0;\
        for (match = false; i < nItem; i ++) {\
            if (0 == strcasecmp(table[i],rate)) {\
                match = true;   break;\
            }\
        }\
    } while (0)

//#define SERCH_IN_RATE_TABLE_LEGACY(rate,match)  SERCH_IN_RATE_TABLE(data_rates_table_legacy,rate,match)
//#define SERCH_IN_RATE_TABLE_HT20_1(rate,match)  SERCH_IN_RATE_TABLE(data_rates_table_ht20_1,rate,match)
//#define SERCH_IN_RATE_TABLE_HT20_3(rate,match)  SERCH_IN_RATE_TABLE(data_rates_table_ht20_3,rate,match)
//#define SERCH_IN_RATE_TABLE_HT40(rate,match)    SERCH_IN_RATE_TABLE(data_rates_table_ht40,rate,match)
#define SERCH_IN_RATE_TABLE_ALL(rate,match)     SERCH_IN_RATE_TABLE(data_rates_table_desc,rate,match)

bool Art2PrintHelp() {
    printf("\nUssage:   gwsman [gui|expert] [parameter [value]] ...");
    printf("\n-------------------------- parameters ----------------------------------------\n"
            "    -txmask   [1|2|3|7]       #chain mask used for transmit,  default 3\n"
            "    -rxmask   [1|2|3|7]       #chain mask used for receiving, default 3\n"
            "    -freq     [frequency]     #should be 2412...2462 MHz,     default 2447\n"
            "    -bw       [bandwidth]     #should be [5,6,7,8...32] MHz,  default 5\n"
            "    -txp      [0.0 ... 31.5]  #Tx power for transmitter,      default 0\n\n"
            "    -rate     [54|t7|f1|...]  #data rates as following,       default ht20\n"
            "_ Legacy ___________________________________________________________________\n"
            "   6   9   12  18  24  36  48  54  1l  2l  2s  5l  5s  11l 11s\n"
            "_ HT20 _____________________________________________________________________\n"
            "   t0 , mcs0   t1 , mcs1   t2 , mcs2   t3 , mcs3   t4 , mcs4   t5 , mcs5\n"
            "   t6 , mcs6   t7 , mcs7   t8 , mcs8   t9 , mcs9   t10, mcs10  t11, mcs11\n"
            "   t12, mcs12  t13, mcs13  t14, mcs14  t15, mcs15  t16, mcs16  t17, mcs17\n"
            "   t18, mcs18  t19, mcs19  t20, mcs20  t21, mcs21  t22, mcs22  t23, mcs23\n"
            "_ HT40 _____________________________________________________________________\n"
            "   f0 , mcs0   f1 , mcs1   f2 , mcs2   f3 , mcs3   f4 , mcs4   f5 , mcs5\n"
            "   f6 , mcs6   f7 , mcs7   f8 , mcs8   f9 , mcs9   f10, mcs10  f11, mcs11\n"
            "   f12, mcs12  f13, mcs13  f14, mcs14  f15, mcs15  f16, mcs16  f17, mcs17\n"
            "   f18, mcs18  f19, mcs19  f20, mcs20  f21, mcs21  f22, mcs22  f23, mcs23\n"
            "____________________________________________________________________________\n"
            "   all                 legacy                  ht20                    ht40\n"
            "---------------------------------------------------------------------[wyy]--\n"
            "    Here are some examples:\n\n"
            "    # art2clt -txmask 1 -txp 3.0 -rate t7\n"
            "    # art2clt -recv -rxmask 1 -rate t7 -report\n\n"
            );
    return false;
}

#define ART2_ARG_CONSOLE    "-console"
#define ART2_ARG_INFO       "-info"
#define ART2_ARG_TRACE      "-trace"
#define ART2_ARG_DEBUG      "-debug"
#define ART2_ARG_RX_MODE    "-recv"
#define ART2_ARG_TX_MASK    "-txmask"
#define ART2_ARG_RX_MASK    "-rxmask"
#define ART2_ARG_TX_100     "-tx100"
#define ART2_ARG_FREQ       "-freq"
#define ART2_ARG_SHORT_GI   "-sgi"
#define ART2_ARG_BANDWIDTH  "-bw"
#define ART2_ARG_TX_POWER   "-txp"
#define ART2_ARG_TX_GAIN    "-txg"
#define ART2_ARG_RATE       "-rate"
#define ART2_ARG_ISS        "-rss"
#define ART2_ARG_REPORT     "-report"

typedef struct __struct_menu_index {
    int x;
    int y;
    char title[16];
    bool upper;
    char key;
    char val_str[32];
} structMenuIndex;
#define ART_MENU_ITEM_TXMASK    0
#define ART_MENU_ITEM_RXMASK    1
#define ART_MENU_ITEM_TX_99     2
#define ART_MENU_ITEM_FREQ      3
#define ART_MENU_ITEM_TX_PWR    4
#define ART_MENU_ITEM_TX_RATE   5
#define ART_MENU_ITEM_TX_GAIN   6
#define ART_MENU_ITEM_SGI       7
#define ART_MENU_ITEM_BW        8
#define ART_MENU_MAX_ITEMS      9
static structMenuIndex  static_art_menu[ART_MENU_MAX_ITEMS] = {
        {2, 15    ," Tx Mask    ", true,    ART2_TX_MASK , ""},
        {3, 15    ," Rx Mask    ", true,    ART2_RX_MASK , ""},
        {4, 15    ," Tx 99/100  ", false,   ART2_TX100   , ""},
        {2, 40    ," Frequency  ", true,    ART2_FREQ    , ""},
        {3, 40    ," Tx power   ", false,   ART2_TX_PWR  , ""},
        {4, 40    ," Tx Rate    ", false,   ART2_TX_RATE , ""},
        {2, 65    ," Tx gain    ", false,   ART2_TX_GAIN , ""},
        {3, 65    ," Short GI   ", true,    ART2_TX_SGI  , ""},
        {4, 65    ," Band width ", true,    ART2_TX_BW   , ""},
};

static bool bRunning = true;
static int  static_cal_partition = 7;
static bool static_driver_on = false;
static bool static_console = false;
static bool static_rx_mode = false;
static bool static_rx_report = true;
static bool static_sgi = true;
static int static_tx_mask = 0x01;
static int static_rx_mask = 0x01;
static int static_tx99 = true;
static int static_freq = 2447;
static int static_bandwidth = 5;
static int static_iss = 0;
static int static_tx_gain = 30;
static double static_tx_pwr = 6.0f;
static char static_rate[8] = "ht20";
//static int8_t static_tx_ref_pwr = 0;

char art2_move_menu_cursor(char key) {
    int i;
    for (i = 0; i < ART_MENU_MAX_ITEMS; i ++) {
        if (static_art_menu[i].key == key) {
            printf(VT100_GOTO_XY(static_art_menu[i].x,static_art_menu[i].y + 12));
            printf(VT100_INVERT VT100_STYLE_HOT "%s" VT100_RESET,static_art_menu[i].val_str);
            fflush(stdout);
            return key;
        }
    }
    return key;
}
//int8_t art2_get_caldata_pierdata_2g_refpower(int ch) {
//    int fd = -1;
//    if((fd = open("/dev/caldata", O_RDONLY)) >= 0) {
//        ar9300_eeprom_t flash;
//        int8_t tx_correction;
//
//        lseek(fd, FLASH_BASE_CALDATA_OFFSET, SEEK_SET);
//        read(fd, &flash, sizeof(ar9300_eeprom_t));
//        if (static_freq <= 2412) {
//            tx_correction = flash.calPierData2G[ch][0].refPower;
//        } else if (static_freq <= 2447) {
//            tx_correction = flash.calPierData2G[ch][1].refPower;
//        } else {
//            tx_correction = flash.calPierData2G[ch][2].refPower;
//        }
//        close(fd);
//        static_tx_ref_pwr = (-1) * tx_correction;
//        return static_tx_ref_pwr;
//    }
//    ShowArtStatusBar("\nCan not read art partition.\n");
//    return 0;
//}

//static bool art2_set_caldata_pierdata_2g_refpower(int ch,int8_t refpower) {
//    int fd = -1;
//    if((fd = open("/dev/caldata", O_RDWR)) >= 0) {
//        ar9300_eeprom_t flash;
//        int8_t tx_correction = (-1) * refpower;
//
//        static_tx_ref_pwr = refpower;
//        lseek(fd, FLASH_BASE_CALDATA_OFFSET, SEEK_SET);
//        read(fd, &flash, sizeof(ar9300_eeprom_t));
//        if (static_freq <= 2412) {
//            flash.calPierData2G[ch][0].refPower = tx_correction;
//        } else if (static_freq <= 2447) {
//            flash.calPierData2G[ch][1].refPower = tx_correction;
//        } else {
//            flash.calPierData2G[ch][2].refPower = tx_correction;
//        }
//        lseek(fd, FLASH_BASE_CALDATA_OFFSET, SEEK_SET);
//        write(fd, &flash, sizeof(ar9300_eeprom_t));
//        close(fd);
//        return true;
//    }
//    ShowArtStatusBar("\nCan not modify art partition.\n");
//    return false;
//}
#define ART2_RFSILENT_START     0x00
#define ART2_RFSILENT_STOP      0x03

static bool art2_set_caldata_rfsilent(u_int8_t rfsilent) {
    static u_int8_t static_rfsilent;
    int fd = -1;
    if((fd = open("/dev/caldata", O_RDWR)) >= 0) {
        ar9300_eeprom_t flash;
        lseek(fd, FLASH_BASE_CALDATA_OFFSET, SEEK_SET);
        read(fd, &flash, sizeof(ar9300_eeprom_t));
        if (rfsilent == ART2_RFSILENT_STOP) { //restore the caldata rfsilent
            flash.baseEepHeader.rfSilent = static_rfsilent;
            lseek(fd, FLASH_BASE_CALDATA_OFFSET, SEEK_SET);
            write(fd, &flash, sizeof(ar9300_eeprom_t));
            WYY_LOG_INFO("\n\tRestore rfSilent = 0x%x to flash.\n",rfsilent);
        } else if (rfsilent == ART2_RFSILENT_START){
            static_rfsilent = flash.baseEepHeader.rfSilent;
            flash.baseEepHeader.rfSilent = 0;
            lseek(fd, FLASH_BASE_CALDATA_OFFSET, SEEK_SET);
            write(fd, &flash, sizeof(ar9300_eeprom_t));
        }
        close(fd);
        return true;
    }
    ShowArtStatusBar("\nCan not access art partition for rfsilent.\n");
    return false;
}

static void art_sig(int signo) {
    if ((signo == SIGTERM) || (signo == SIGINT) || (signo == SIGSEGV)) {
        printf("\n");
        switch (signo) {
            case SIGSEGV:
            {
                ShowArtStatusBar("Segmentation fault, aborting.\n");
                exit(1);
            }
            case SIGTERM:
            case SIGINT:
            {
                if (bRunning) {
                    bRunning = false;
                    printf("Exit ...\n");
                }
            }
        }
    }
}

void art_init_signals()
{
    signal (SIGTERM, art_sig);
    signal (SIGINT, art_sig);
    signal (SIGSEGV, art_sig);
}	

#define MSEC                1000
#define SECOND              1000000
#define TX_LOOP_INTERVAL    (50 * MSEC)
#define RX_LOOP_INTERVAL    (50 * MSEC)

typedef struct _struct_Report_Row {
    unsigned long   m_nSN;
    char            m_sRate[16];
    unsigned long   m_nTPut;
} Art_Report_Row;

#define RX_STATS_DUMP_FILE  "/var/run/art_rx_stats"
static char* VT100_move_right(int n) {
    static char vt100_str[8];
    sprintf(vt100_str,VT100_HEAD "%dC", n);
    return vt100_str;
}

static unsigned long static_linkStatsSerial = 0;
static void LinkReceiveStatsReportRow(char* stamp,char* rate,char* tput,char* error,char* snr,char* sig_nf0) {
//    printf("[%s] Rate = %s, TPUT = %d kbps, err = %.2f%%, SNR = %.1f, RSSI = %.1f, NF = %.1f\n",
//            stamp,sRate,nTput,fErr,fSNR,fSig,fNoise);
    printf(VT100_INVERT VT100_STYLE1 "%s" VT100_RESET , stamp);
    printf(VT100_INVERT VT100_STYLE_MENU "Rate["    VT100_RESET VT100_STYLE_VALUE "%s" VT100_RESET, rate);
    printf(VT100_INVERT VT100_STYLE_MENU "]TPUT["   VT100_RESET);// VT100_STYLE_VALUE "%s" VT100_RESET, tput);
    do {
#define SPACE_TPT   15
        char sTPT1[SPACE_TPT + 1],sTPT2[SPACE_TPT + 1];
        long nThroughput = 0;
        long i = 0, j = 0, l = strlen(tput), k = SPACE_TPT - l, nTPT = 0;
        
        memset (sTPT1, 0x00, SPACE_TPT + 1);
        memset (sTPT2, 0x00, SPACE_TPT + 1);
        sscanf(tput,"%u",&nThroughput);
        nTPT = nThroughput / 512;
        nTPT = (nTPT > SPACE_TPT) ? SPACE_TPT : nTPT;
        for (i = 0; i < SPACE_TPT && i < nTPT; i ++) { //the yellow part
            sTPT1[i] = (i < k) ? ' ' : tput[i - k];
        }
        for (j = i; j < SPACE_TPT; j ++) {
            sTPT2[j-i] = (j < k) ? ' '  : tput[j - k];
        }
        printf( VT100_RESET "%s%s" VT100_RESET VT100_STYLE_DARK "%s",
                VT100_STYLE_LED_A(nThroughput < 512), sTPT1, sTPT2);
    } while (0);
    printf(VT100_INVERT VT100_STYLE_MENU "]PER["    VT100_RESET);
    do {
#define SPACE_PER   15
        char sPER1[SPACE_PER + 1],sPER2[SPACE_PER + 1];
        double  fPER = 0.0;
        long i = 0, j = 0, l = strlen(error), k = SPACE_PER - l, nPER = 0;
        
        memset (sPER1  , 0x00, SPACE_PER + 1);
        memset (sPER2  , 0x00, SPACE_PER + 1);
        sscanf(error,"%lf",&fPER);
        if (fPER < 0.5) nPER = 0;
        else if (fPER < 1.0) nPER = 1;
        else if (fPER < 2.0) nPER = 2;
        else if (fPER < 4.0) nPER = 3;
        else if (fPER < 8.0) nPER = 4;
        else if (fPER < 10.0) nPER = 5;
        else if (fPER < 20.0) nPER = 6;
        else if (fPER < 30.0) nPER = 7;
        else if (fPER < 40.0) nPER = 8;
        else if (fPER < 50.0) nPER = 9;
        else if (fPER < 60.0) nPER = 10;
        else if (fPER < 70.0) nPER = 11;
        else if (fPER < 80.0) nPER = 12;
        else if (fPER < 90.0) nPER = 13;
        else nPER = 14;        
        for (i = 0; i < SPACE_PER && i < nPER; i ++) { //the red part
            sPER1[i] = (i < k) ? ' ' : error[i - k];
        }
        for (j = i; j < SPACE_PER; j ++) {
            sPER2[j-i] = (j < k) ? ' ' : error[j - k];
        }
        printf( VT100_RESET "%s%s" VT100_RESET "%s%s",
                VT100_STYLE_LED_B(true), sPER1, VT100_STYLE_LED_B(false), sPER2);
    } while (0);
    printf(VT100_INVERT VT100_STYLE_MENU "]SNR["     VT100_RESET);
    do {
#define SPACE_SNR   15
        char sSNR1[16],sSNR2[16];
        double  fSNR = 0.0;
        long i = 0, j = 0, l = strlen(snr), k = SPACE_SNR - l, nSNR = 0;
        
        memset (sSNR1, 0x00, 16);
        memset (sSNR2, 0x00, 16);
        sscanf(snr,"%lf",&fSNR);
        nSNR = fSNR / 2;
        nSNR = (nSNR > SPACE_SNR) ? SPACE_SNR : nSNR;
        for (i = 0; i < SPACE_SNR && i < nSNR - 3; i ++) { //the yellow part
            sSNR1[i] = (i < k) ? ' ' : snr[i - k];
        }
        for (j = i; j < SPACE_SNR; j ++) {
            sSNR2[j-i] = (j < k) ? ' ' : snr[j - k];
        }
        printf( VT100_RESET "%s%s" VT100_RESET VT100_STYLE_DARK "%s",
                VT100_STYLE_LED_A(fSNR < 10.0), sSNR1, sSNR2);
    } while (0);
    do {
        char sSigNoise[32];
        char* pNf = NULL;
        memset (sSigNoise, 0x00, 32);
        strncpy(sSigNoise, sig_nf0, 31);
        pNf = strstr(sSigNoise,"/");
        if (pNf) {
            *pNf = 0;
            printf( VT100_INVERT VT100_STYLE_MENU "]RSSI/NF[" 
                    VT100_RESET VT100_STYLE_VALUE "%s"
                    VT100_RESET VT100_STYLE1 "/%s"
                    VT100_INVERT VT100_STYLE_MENU "]", sSigNoise, pNf + 1);
        } else {
            printf( VT100_INVERT VT100_STYLE_MENU "]RSSI/NF[" 
                    VT100_RESET VT100_STYLE_VALUE "%s"
                    VT100_INVERT VT100_STYLE_MENU "]", sig_nf0);
        }
    } while (0);
    printf(VT100_RESET "\n");
}

#define COPY_STRING_FIELD(dst,src,key)  do {\
                if (src) {\
                    src += strlen(key);\
                    memset (dst, 0x00, 32);\
                    strncpy (dst, src, (strstr(src,"|") - src));\
                }\
            } while (0)

#define WYY_FREQ_2312       2312
#define WYY_FREQ_2372       2370
static void LinkReceiveStatsReport(int freq) {
    if ((WYY_FREQ_2312 <= freq && freq <= WYY_FREQ_2372) || (2412 <= freq && freq <= 2472)) {
        char dump_file_name[128];
        sprintf(dump_file_name,"%s_%d",RX_STATS_DUMP_FILE,freq);
        FILE*   fp = fopen(dump_file_name,"r");
        char    report[1024];
        char    stamp[256];
        time_t  moment = time(&moment);
        if (fp) {
            while (fgets(report,1023,fp)) {
                char*   pSN = strstr(report,"sn=");
                unsigned long nSN = 0;
                if (pSN) {
                    pSN += strlen("sn=");
                    sscanf(pSN,"%d",&nSN);
                    if (static_linkStatsSerial != nSN) {
                        char*   pRate = strstr(report,"rate=");
                        char*   pTPut = strstr(report,"tput=");
                        char*   pErr = strstr(report,"err=");
                        char*   pSNR = strstr(report,"SNR=");
                        char*   pSigNf0 = strstr(report,"Sig0/NF0=");
                        char    sRate[32];
                        char    sTput[32];
                        char    sPER[32];
                        char    sSNR[32];
                        char    sRSSI_NF[32];
                        static_linkStatsSerial = nSN;
                        strftime(stamp,sizeof(stamp),"%H::%M:%S",localtime(&moment));
                        COPY_STRING_FIELD(sRate,pRate,"rate=");
                        COPY_STRING_FIELD(sTput,pTPut,"tput=");
                        COPY_STRING_FIELD(sPER ,pErr ,"err=");
                        COPY_STRING_FIELD(sSNR ,pSNR ,"SNR=");
                        COPY_STRING_FIELD(sRSSI_NF ,pSigNf0 ,"Sig0/NF0=");
                        LinkReceiveStatsReportRow(stamp,sRate,sTput,sPER,sSNR,sRSSI_NF);
                    }
                }
            }
            fclose(fp);
        }
    }
}
//wangyaoyang

static unsigned long static_nTxTimer = 0;
static bool static_bTransmitting = false;
static int static_nRateIdx = 0;
static void TxLoop(bool display, int freq,bool sgi,int txmask,int rxmask,bool tx99,char* rate,double power,int txgain) {
#define TX_RATES_TABLE_LOOP(table, timer) \
    do {\
        int nRates = sizeof(table) / sizeof(table[0]);\
        WYY_LOG_INFO("Rate table size = %d\n",nRates);\
        int nRecv = Socket_RecvData(MAX_BUFF_SIZE);\
        if (nRecv > 0) {\
            BYTE* pRecv = Socket_GetData(&nRecv);\
            WYY_LOG_INFO("[%s]",pRecv);\
        }\
        if (0 == timer % 100) {\
            if (static_nRateIdx == nRates) {\
                static_nRateIdx = 0;\
                timer = 0;\
            }\
            if (static_bTransmitting) {\
                nart_exec(NSHELL_CMD_STOP);\
                usleep(TX_LOOP_INTERVAL);\
            }\
            if (display) ShowArtStatusBar("[% 6d]Transmitting [freq = %d,SGI = %d,Tx/Rx Mask = 0x%x/0x%x,Rate = %s,TXP = %2.1f,TXG = %d,%s]...\n",\
                    timer,freq,sgi,txmask,rxmask,table[static_nRateIdx],power,txgain,tx99 ? "Tx 99" : "Tx 100");\
            nart_exec(build_tx_command(freq,sgi,txmask,rxmask,tx99,table[static_nRateIdx],power,txgain));\
            nart_exec(NSHELL_CMD_START);\
            static_bTransmitting = true;\
            static_nRateIdx ++;\
        }\
        timer ++;\
    } while (0)
//#define TX_RATES_TABLE_LOOP(table, timer)
    if (0 == strcasecmp(rate,"all")) {
        WYY_LOG_INFO("\n------- Using rates table [ all ],\t");
        TX_RATES_TABLE_LOOP(data_rates_table_all, static_nTxTimer);
    } else if (0 == strcasecmp(rate,"legacy")) {
        WYY_LOG_INFO("\n------- Using rates table [ legacy ],\t");
        TX_RATES_TABLE_LOOP(data_rates_table_legacy, static_nTxTimer);
    } else if (0 == strcasecmp(rate,"ht20")) {
        if (0x03 == (txmask & 0x03)) {
            WYY_LOG_INFO("\n------- Using rates table [ ht20_3 ],\t");
            TX_RATES_TABLE_LOOP(data_rates_table_ht20_3, static_nTxTimer);
        } else if (txmask & 0x03) {
            WYY_LOG_INFO("\n------- Using rates table [ ht20_1 ],\t");
            TX_RATES_TABLE_LOOP(data_rates_table_ht20_1, static_nTxTimer);
        }
    } else if (0 == strcasecmp(rate,"ht40")) {
        WYY_LOG_INFO("\n------- Using rates table [ ht40 ],\t");
        TX_RATES_TABLE_LOOP(data_rates_table_ht40, static_nTxTimer);
    } else if (0 == static_nTxTimer) {
        ShowArtStatusBar("freq = %d,sig = %d,txmask = 0x%x,rxmask = 0x%x,rate = %s, wifi pwr = %2.1f\n",
                freq,sgi,txmask,rxmask,rate,power);
        nart_exec(build_tx_command(freq,sgi,txmask,rxmask,tx99,rate,power,txgain));
        nart_exec(NSHELL_CMD_START);
        static_nTxTimer = 1;
    }
}

static char static_rx_counter = 0;
static bool static_rx_start = false;
static bool static_rx_record = false;
static void RxLoop(bool display, char* rx_command,int freq) {
    int nRecv = Socket_RecvData(MAX_BUFF_SIZE);
    if (nRecv > 0) {
        BYTE* pRecv = Socket_GetData(&nRecv);
        WYY_LOG_INFO("[%s]",pRecv);
    }
    if (!static_rx_start) {
        if (static_rx_record && display) {
            LinkReceiveStatsReport(freq);
            static_rx_record = false;
            static_rx_counter = 0;
        } else if (0 == static_rx_counter % 4) {   //START receiving after reporting
            nart_exec(rx_command);
            nart_exec(NSHELL_CMD_START);
            static_rx_counter = 0;
            static_rx_start = true;
        }
    } else if (0 == static_rx_counter % 4) { //after 1 second, STOP receiving
        nart_exec(NSHELL_CMD_STOP);
        static_rx_start = false;
        static_rx_record = true;
    }
    static_rx_counter ++;
}

int art2_get_kernel_var(char* name, char* format, void* value) {
    static char* prefix = "/sys/kernel/debug/ieee80211/phy0/ath9k";
    char path[128];
    char buff[256];
    if (name) {
        memset (path, 0x00, 128);
        sprintf(path,"%s/%s",prefix,name);
        FILE*   fp = fopen(path,"r");
        if (fp) {
            memset (buff, 0x00, 256);
            fgets (buff, 255, fp);
            fclose(fp);
            sscanf(buff, format, value);
            return 0;
        }
        return -1;
    }
    return -1;
}

static bool art2_read_cmdline(int argc, char* argv[], bool* console, int* debug, bool* rx_mode,bool *rx_report,
        int* txmask, int* rxmask, bool* tx99, int* freq, bool* sgi,int* bandwidth, double* power, int* txgain, char* rate,int* iss) {
    if (argc > 1) {
        int i = 0,counter = 0;
        for (i = 1; i < argc; i++) {
            char* szArgv = argv[i];
            char* p = NULL;
            if ((p = strstr(szArgv, ART2_ARG_CONSOLE)) != NULL) {
                counter ++;
                *console = true;
            } else if ((p = strstr(szArgv, ART2_ARG_TRACE)) != NULL) {
                counter ++;
                if (*debug < 1) *debug = 1;
            } else if ((p = strstr(szArgv, ART2_ARG_DEBUG)) != NULL) {
                counter ++;
                if (*debug < 2) *debug = 2;
            } else if ((p = strstr(szArgv, ART2_ARG_INFO)) != NULL) {
                counter ++;
                static_info_enable = true;
            } else if ((p = strstr(szArgv, ART2_ARG_RX_MODE)) != NULL) {
                counter ++;
                *rx_mode = true;
            } else if ((p = strstr(szArgv, ART2_ARG_REPORT)) != NULL) {
                counter ++;
                *rx_report = true;
            } else if ((p = strstr(szArgv, ART2_ARG_SHORT_GI)) != NULL) {
                counter ++;
                *sgi = true;
            } else if ((p = strstr(szArgv, ART2_ARG_TX_100)) != NULL) {
                counter ++;
                *tx99 = false;
            } else if ((p = strstr(szArgv, ART2_ARG_TX_MASK)) != NULL) {
                counter ++;
                sscanf(p + strlen(ART2_ARG_TX_MASK) + 1, "%x", txmask);
                if (*txmask != 0x01 && *txmask != 0x02 &&
                    *txmask != 0x03 && *txmask != 0x07) {
                    ShowArtStatusBar("\nInvalidate txmask:%x\n", *txmask);
                    return Art2PrintHelp();
                }
            } else if ((p = strstr(szArgv, ART2_ARG_RX_MASK)) != NULL) {
                counter ++;
                sscanf(p + strlen(ART2_ARG_RX_MASK) + 1, "%x", rxmask);
                if (*rxmask != 0x01 && *rxmask != 0x02 &&
                    *rxmask != 0x03 && *rxmask != 0x07) {
                    ShowArtStatusBar("\nInvalidate rxmask:%x\n", *rxmask);
                    return Art2PrintHelp();
                }
            } else if ((p = strstr(szArgv, ART2_ARG_FREQ)) != NULL) {
                counter ++;
                sscanf(p + strlen(ART2_ARG_FREQ) + 1, "%d", freq);
                if (*freq != 2412 && *freq != 2417 && *freq != 2422 && *freq != 2427 && *freq != 2432 &&
                    *freq != 2437 && *freq != 2442 && *freq != 2447 && *freq != 2452 && *freq != 2457 &&
                    *freq != 2462 && *freq != 2467 && *freq != 2472 &&
                    *freq != 2310 && *freq != 2315 && *freq != 2320 && *freq != 2325 && *freq != 2330 &&
                    *freq != 2335 && *freq != 2340 && *freq != 2345 && *freq != 2350 && *freq != 2355 &&
                    *freq != 2360 && *freq != 2365 && *freq != 2370) {
                    ShowArtStatusBar("\nInvalidate frequency:%x\n", *freq);
                    return Art2PrintHelp();
                }
            } else if ((p = strstr(szArgv, ART2_ARG_RATE)) != NULL) {
                bool    match = false;
                counter ++;
                p = p + strlen(ART2_ARG_FREQ) + 1;
                SERCH_IN_RATE_TABLE_ALL(p,match);
                if (!match) {
                    ShowArtStatusBar("\nInvalidate rate:%s\n", p);
                    return Art2PrintHelp();
                }
                strcpy (rate,p);
            } else if ((p = strstr(szArgv, ART2_ARG_BANDWIDTH)) != NULL) {
                counter ++;
                sscanf(p + strlen(ART2_ARG_BANDWIDTH) + 1, "%d", bandwidth);
                if (*bandwidth != 4 && *bandwidth != 5 && *bandwidth != 6 && *bandwidth != 7 &&
                    *bandwidth != 8 && *bandwidth != 10 && *bandwidth != 16 && *bandwidth != 20) {
                    ShowArtStatusBar("\nInvalidate bandwidth:%d\n", *bandwidth);
                    return Art2PrintHelp();
                } else ShowArtStatusBar("\nSpeicified bandwidth % MHz",*bandwidth);
            } else if ((p = strstr(szArgv, ART2_ARG_ISS)) != NULL) {
                counter ++;
                sscanf(p + strlen(ART2_ARG_ISS) + 1, "%d", iss);
            } else if ((p = strstr(szArgv, ART2_ARG_TX_POWER)) != NULL) {
                counter ++;
                sscanf(p + strlen(ART2_ARG_TX_POWER) + 1, "%lf", power);
                if (*power < 0.0 || *power > 31.5) {
                    ShowArtStatusBar("\nInvalidate Tx power : %f\n", *power);
                    return Art2PrintHelp();
                } else ShowArtStatusBar("\n\tSet Tx power to %2.1f dBm\n", *power);
            } else if ((p = strstr(szArgv, ART2_ARG_TX_GAIN)) != NULL) {
                counter ++;
                sscanf(p + strlen(ART2_ARG_TX_GAIN) + 1, "%lf", txgain);
                if (*txgain < -1 || *power > 100) {
                    ShowArtStatusBar("\nInvalidate Tx gain (should be 0 ~ 100): %d\n", *txgain);
                    return Art2PrintHelp();
                } else ShowArtStatusBar("\n\tSet Tx gain to %df\n", *txgain);
            }
        }
        if (!counter) return Art2PrintHelp();
    } else return Art2PrintHelp();
    return true;
}

void art2_SetSGI()                  { static_sgi = !static_sgi; }
bool art2_GetSGI()                  { return static_sgi; }
void art2_SetTx99()                 { static_tx99 = !static_tx99; }
bool art2_GetTx99()                 { return static_tx99; }
void art2_SetTxPower(double txp)    { static_tx_pwr = txp; }
void art2_SetTxGain(int txg)        { static_tx_gain = txg; }
void art2_SetTxMask(int mask)       { static_tx_mask = mask; }
void art2_SetRxMask(int mask)       { static_rx_mask = mask; }

//bool art2_SetTxGain(int8_t gain)    {
//    if (art2_set_caldata_pierdata_2g_refpower(0,gain)) {
//        ShowArtStatusBar("Waiting for reload ART driver...\n");
//        art2_Load(false);
//        return art2_Load(true);
//    } else ShowArtStatusBar("Failed, can not write ART partition...\n");
//    return false;
//}
//

bool art2_SetBandWdith(int bw) {
    if (2 == bw || 3 == bw || 4 == bw || 5 == bw || 6 == bw || 7 == bw || 8 == bw || 10 == bw ||
        12 == bw || 14 == bw || 16 == bw || 20 == bw || 24 == bw || 28 == bw || 32 == bw) {
        static_bandwidth = bw;
        ShowArtStatusBar("Need to Restart ART driver...\n");
        return art2_ReLoad();
    }
    return false;
}

bool art2_SetFrequency(int freq) {
    if (freq != 2412 && freq != 2417 && freq != 2422 && freq != 2427 && freq != 2432 &&
        freq != 2437 && freq != 2442 && freq != 2447 && freq != 2452 && freq != 2457 &&
        freq != 2462 && freq != 2467 && freq != 2472 &&
        freq != 2310 && freq != 2315 && freq != 2320 && freq != 2325 && freq != 2330 &&
        freq != 2335 && freq != 2340 && freq != 2345 && freq != 2350 && freq != 2355 &&
        freq != 2360 && freq != 2365 && freq != 2370) return false;
    static_freq = freq;
    return true;
}

bool art2_CheckRate(char* rate) {
    bool    match = false;
    SERCH_IN_RATE_TABLE_ALL(rate,match);
    return match;
}

void art2_SetRate(char* rate) {
    if (rate)
        strncpy (static_rate,rate,7);
}


void art2_read_config(int* cal_partion, bool *rx_report, int* txmask, int* rxmask, bool* tx99,
        int* freq, bool* sgi, int* bandwidth, double* power, int* txgain, char* rate,int* iss) {
    char sParamValue[256];
    memset(sParamValue, 0x00, 256);
    if (0 == read_cfg(CONFIG_FILE, "ArtPartition" , sParamValue)) sscanf(sParamValue, "%d", cal_partion);
    if (0 == read_cfg(CONFIG_FILE, "ArtReport"  , sParamValue)) sscanf(sParamValue, "%d", rx_report);
    if (0 == read_cfg(CONFIG_FILE, "ArtTxMask"  , sParamValue)) sscanf(sParamValue, "%d", txmask);
    if (0 == read_cfg(CONFIG_FILE, "ArtRxMask"  , sParamValue)) sscanf(sParamValue, "%d", rxmask);
    if (0 == read_cfg(CONFIG_FILE, "ArtTx99"    , sParamValue)) sscanf(sParamValue, "%d", tx99);
    if (0 == read_cfg(CONFIG_FILE, "ArtFreq"    , sParamValue)) sscanf(sParamValue, "%d", freq);
    if (0 == read_cfg(CONFIG_FILE, "ArtSGI"     , sParamValue)) sscanf(sParamValue, "%d", sgi);
    if (0 == read_cfg(CONFIG_FILE, "ArtBW"      , sParamValue)) sscanf(sParamValue, "%d", bandwidth);
    if (0 == read_cfg(CONFIG_FILE, "ArtTXP"     , sParamValue)) sscanf(sParamValue, "%lf", power);
    if (0 == read_cfg(CONFIG_FILE, "ArtTXG"     , sParamValue)) sscanf(sParamValue, "%lf", txgain);
    if (0 == read_cfg(CONFIG_FILE, "ArtRate"    , sParamValue)) sscanf(sParamValue, "%s", rate);
    if (0 == read_cfg(CONFIG_FILE, "ArtISS"     , sParamValue)) sscanf(sParamValue, "%d", iss);
}

int art2_init(int argc, char** argv) {
    art2_get_kernel_var("chanbw","%x",&static_bandwidth);
    art2_read_config(   &static_cal_partition,      &static_rx_report,  &static_tx_mask,    &static_rx_mask,
                        &static_tx99,               &static_freq,       &static_sgi,        &static_bandwidth,
                        &static_tx_pwr,             &static_tx_gain,    static_rate,        &static_iss);
    if (!art2_read_cmdline( argc,   argv,       &static_console,    &static_debug,      &static_rx_mode,    &static_rx_report, 
                            &static_tx_mask,    &static_rx_mask,    &static_tx99,       &static_freq,       &static_sgi,
                            &static_bandwidth,  &static_tx_pwr,     &static_tx_gain,    static_rate,        &static_iss)) {
        return (EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

static void art2_standby() {
    if (static_WorkingMode > ART_MODE_UNLOAD) {
        static_nTxTimer = 0;
        static_nRateIdx = 0;
        static_bTransmitting = false;
        if (static_is_nart_connected) {
            nart_exec(NSHELL_CMD_STOP);
        }
        static_WorkingMode = ART_MODE_STANDBY;
    }
}

char* art2_system(char* comment,char* expected,unsigned int wait_ms,...) {
    static char echo[256];
    int     i = 0, j = 0;
    char*   param = NULL;
    char*   argv[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

    va_list ap;
    va_start(ap, wait_ms);
    while (i < 10) {
        if (NULL == (param = va_arg(ap, char*))) break;
        if (strlen(param) > 0 && strcmp(param," ") != 0) {
            argv[i] = param;
            i ++;
        }
    }
    va_end(ap);
    if (i > 0 && PipeShellCmd(&static_pipe_shell, argv[0], argv)) {
        memset (echo,0x00,256);
        if (comment) ShowArtStatusBar("%s\n",comment);
        for (i = 0; expected && i < 50; i ++, usleep(50000)) {
            if (PipeTryRead(&static_pipe_shell,echo,255) > 0) {
                if (strstr(echo,expected) > echo) return echo;
                else WYY_LOG_INFO("%s\n",echo);
            }
        }
        usleep(wait_ms);
    }
    return NULL;
}

void art2_system_stdout(bool init) {
    static char    static_log[1024];
    
    if (init) memset (static_log,0x00,1024);
    if (static_info_enable && PipeTryRead(&static_pipe_shell,static_log,1023) > 0) {
        ShowArtSystemLog("%s\n",static_log);
    }
}

static void art2_driver_clear() {
    static_nTxTimer = 0;
    static_nRateIdx = 0;
    static_bTransmitting = false;
    if (static_is_nart_connected) {
        nart_exec(NSHELL_CMD_STOP);
        nart_exec(NSHELL_CMD_EXIT);
        Socket_Disconnect();
        static_is_nart_connected = false;
        usleep(1000000);
    }
    art2_system("Remove NART...",NULL,100 * SLEEP_MS, "killall","nart.out",NULL);
    art2_system("Remove ART..." ,NULL,100 * SLEEP_MS, "rmmod","art.ko",NULL);
    usleep(300 * SLEEP_MS);
}

void art2_wifi(bool up) {
    if (static_Wifi_up != up) {
        static_Wifi_up = up;
        art2_system(NULL,NULL, 500 * SLEEP_MS, "wifi", up ? "up" : "down",NULL);
        usleep(SLEEP_SEC);
    }
}

bool art2_driver_init() {
    int i;

    art2_wifi(false);
    art2_driver_clear();
    art2_system_stdout(true);
    art2_system(NULL,NULL,100 * SLEEP_MS, "mknod","/dev/dk0","c","63","0",NULL);
    art2_system(NULL,NULL,100 * SLEEP_MS, "mknod","/dev/caldata","b","31",art2_n2a(static_cal_partition),NULL);
    if (art2_set_caldata_rfsilent(ART2_RFSILENT_START)) {
        static char* debug_level[3] = { "", "-trace", "-debug" };
        
        if (static_debug < 0) static_debug = 0;
        if (static_debug > 2) static_debug = 2;
        art2_system("Load ART driver...",NULL,500 * SLEEP_MS, "insmod","/usr/sbin/art.ko",NULL);   usleep(SLEEP_SEC);
        for (i = 0; i < 3; i ++) {
            if (NULL == art2_system(NULL,"art.ko",0 * SLEEP_MS, "lsmod","|","grep","art",NULL)) break;
        }
        if (i < 3) {
            art2_system("Load NART...",NULL,500 * SLEEP_MS, "/usr/sbin/nart.out","-bw", art2_n2a(static_bandwidth),
                                    static_console ? "-console" : "",debug_level[static_debug],"&",NULL);
            Socket_Init(6780);
            if (static_is_nart_connected = Socket_Connect(127,0,0,1,2390)) {
                nart_exec(NSHELL_CMD_INIT);
                nart_exec(NSHELL_CMD_LOAD);
                ShowArtStatusBar("Ready!\n");
                usleep(500 * SLEEP_MS);
                return true;
            } else {
                art2_driver_clear();
                art2_set_caldata_rfsilent(ART2_RFSILENT_STOP);
                ShowArtStatusBar("\nCan not connect to art2\n");
            }
        }
    } else ShowArtStatusBar("\nError, Can not access ART partition!\n");
    art2_wifi(true);
    return false;
}

bool art2_driver_exit() {
    art2_driver_clear();
    art2_set_caldata_rfsilent(ART2_RFSILENT_STOP);
    art2_wifi(true);
    return true;
}

bool art2_ReLoad() {
    static_nTxTimer = 0;
    static_nRateIdx = 0;
    static_bTransmitting = false;
    if (static_is_nart_connected) {
        nart_exec(NSHELL_CMD_STOP);
        nart_exec(NSHELL_CMD_EXIT);
        Socket_Disconnect();
        static_is_nart_connected = false;
        usleep(SLEEP_SEC);
    }
    art2_system(NULL,NULL,500 * SLEEP_MS, "killall","nart.out",NULL);   usleep(SLEEP_SEC);
    static_WorkingMode = ART_MODE_UNLOAD;
    
    if (art2_set_caldata_rfsilent(ART2_RFSILENT_START)) {
        static char* debug_level[3] = { " ", "-trace", "-debug" };
        
        if (static_debug < 0) static_debug = 0;
        if (static_debug > 2) static_debug = 2;
        
        art2_system("Load NART...",NULL,500 * SLEEP_MS, "/usr/sbin/nart.out","-bw",art2_n2a(static_bandwidth),
                                static_console ? "-console" : " ",debug_level[static_debug],"&",NULL);
        Socket_Init(6780);
        if (static_is_nart_connected = Socket_Connect(127,0,0,1,2390)) {
            nart_exec(NSHELL_CMD_INIT);
            nart_exec(NSHELL_CMD_LOAD);
            static_WorkingMode = ART_MODE_STANDBY;
            ShowArtStatusBar("Ready!\n");
            return true;
        } else {
            art2_driver_clear();
            art2_set_caldata_rfsilent(ART2_RFSILENT_STOP);
            ShowArtStatusBar("\nCan not connect to art2\n");
        }
    } else ShowArtStatusBar("\nError, Can not open ART partition!\n");
    return false;
}

bool art2_Load(bool load) {
    if (load) {
        PipeShellInit(&static_pipe_shell);
        if (art2_driver_init()) {
            static_WorkingMode = ART_MODE_STANDBY;
            return true;
        }
    } else {
        art2_driver_exit();
        static_WorkingMode = ART_MODE_UNLOAD;
        PipeShellExit(&static_pipe_shell);
        printf(VT100_CLEAR);
        return true;
    }
    return false;
}

int art2_GetMode() {
    return static_WorkingMode;
}

int art2_SetMode(int mode) {
    switch (mode) {
        case ART_MODE_TX:
            if (static_WorkingMode > ART_MODE_UNLOAD) {
                art2_standby();
                static_WorkingMode = ART_MODE_TX;
                ShowArtStatusBar(   VT100_STYLE_HOT "[TX ...] "\
                                    VT100_STYLE_BLUE "Tx/Rx Mask = %x/%x, freq = %d MHz, BW = %d MHz, rate = %s, Tx pwr = %2.1f Tx gain = %d, %s\n",
                                    static_tx_mask,static_rx_mask,static_freq,static_bandwidth,static_rate,static_tx_pwr,
                                    static_tx_gain,static_tx99 ? "Tx99" : "Tx100");
            } else ShowArtStatusBar(VT100_CLEAR VT100_STYLE_BLUE "Please load ART driver first\n");
            break;
        case ART_MODE_RX:
            if (static_WorkingMode > ART_MODE_UNLOAD) {
                art2_standby();
                static_WorkingMode = ART_MODE_RX;
                ShowArtStatusBar(   VT100_STYLE_HOT "[RX ...] "\
                                    VT100_STYLE_BLUE "Tx/Rx Mask = %x/%x, freq = %d MHz, BW = %d MHz, rate = %s, %s....\n",
                                    static_tx_mask,static_rx_mask,static_freq,static_bandwidth,static_rate,static_tx99 ? "Tx99" : "Tx100");
            } else ShowArtStatusBar(VT100_CLEAR VT100_STYLE_BLUE "Please load ART driver first\n");
            break;
        case ART_MODE_STANDBY:
            if (static_WorkingMode > ART_MODE_UNLOAD) {
                art2_standby();
                static_WorkingMode = ART_MODE_STANDBY;
            }
            break;
        default:
            static_WorkingMode = mode;
    }
    return static_WorkingMode;
}

static char art2_lookup_key(char* var,bool upper,char key,char** p1,char** p2) {
    static char str[32];
    char *k = NULL, c = upper ? toupper(key) : tolower(key);
    memset (str,0x00,32);
    strcpy (str,var);
    if ((k = strchr(str,c)) >= str) {
        *k = 0;
        *p1 = str;
        *p2 = k + 1;
        return c;
    }
    return 0;
}

static void art2_guiPrintMenuCmd(int x,int y,char* var,bool upper,char key) {
    char    *p1 = NULL, *p2 = NULL, c = 0;
    if (c = art2_lookup_key(var,upper,key,&p1,&p2)) {
        printf("%s" VT100_RESET VT100_INVERT VT100_STYLE_TITLE "%s" VT100_RESET VT100_STYLE_LIGHT "%c" 
                    VT100_RESET VT100_INVERT VT100_STYLE_TITLE "%s" VT100_RESET, VT100_GOTO_XY(x,y),p1,c,p2);
    }
}

static void art2_guiPrintMenuItem(int x,int y,char* var,bool upper,char key,char* value) {
    char    *p1 = NULL, *p2 = NULL, c = 0;
    if (c = art2_lookup_key(var,upper,key,&p1,&p2)) {
        printf("%s" VT100_RESET VT100_STYLE_TITLE "%s" VT100_STYLE_LIGHT "%c"
                    VT100_RESET VT100_STYLE_TITLE "%s" VT100_STYLE_BLUE "%8s" VT100_RESET, VT100_GOTO_XY(x,y),p1,c,p2,value);
    }
}

static void art2_guiShowMenu(pthread_mutex_t* hMutex) {
#define PRINT_ART_MENU_ITEM(item,format,value) \
do {\
    memset (item.val_str,0x00,32);\
    sprintf(item.val_str,format,value);\
    art2_guiPrintMenuItem(item.x, item.y, item.title, item.upper, item.key, item.val_str);\
} while (0)

    if (0 == pthread_mutex_trylock(hMutex)) {
        art2_guiPrintMenuCmd(2, 1     ," 1 Start Tx   ", false,  ART2_TX);
        art2_guiPrintMenuCmd(3, 1     ," 2 Start Rx   ", false,  ART2_RX);
        art2_guiPrintMenuCmd(4, 1     ," 0 Stop Tx/Rx ", false,  ART2_STANDBY);
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_TXMASK]  , "%x"    ,static_tx_mask);
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_RXMASK]  , "%x"    ,static_rx_mask);
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_TX_99]   , "%s"    ,static_tx99 ? "99%" : "100%");
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_FREQ]    , "%dMHz" ,static_freq);
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_TX_PWR]  , "%2.1f" ,static_tx_pwr);
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_TX_RATE] , "%s"    ,static_rate);
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_TX_GAIN] , "%d"    ,static_tx_gain);
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_SGI]     , "%s"    ,static_sgi ? "yes" : "no");
        PRINT_ART_MENU_ITEM(static_art_menu[ART_MENU_ITEM_BW]      , "%dMHz" ,static_bandwidth);
        printf(VT100_LOAD_CURSOR);
        pthread_mutex_unlock(hMutex);
    }
#undef  PRINT_ART_MENU_ITEM
}

bool art2_guiPrintMenu(pthread_mutex_t* hMutex) {
    switch (static_WorkingMode) {
        case ART_MODE_TX:
            if (0 == static_tx_mask & 0x03 || 0 == static_rx_mask & 0x03) {
                ShowArtStatusBar("\nTxMask (%02x), RxMask (%02x) should be (1,2,3)\n",static_tx_mask,static_rx_mask);
                return false;
            }
            art2_guiShowMenu(hMutex);
            if (0 == pthread_mutex_trylock(hMutex)) {
                art2_system_stdout(false);
                TxLoop(true,static_freq,static_sgi,static_tx_mask,static_rx_mask,
                                static_tx99,static_rate,static_tx_pwr,static_tx_gain);
                pthread_mutex_unlock(hMutex);
                return false ;
            } else return false;
        case ART_MODE_RX:
            if (0 == static_tx_mask & 0x03 || 0 == static_rx_mask & 0x03) {
                ShowArtStatusBar("\nTxMask (%02x), RxMask (%02x) should be (1,2,3)\n",static_tx_mask,static_rx_mask);
                return false;
            } else if (0 == pthread_mutex_trylock(hMutex)) {
                RxLoop(true,build_rx_command(static_freq,static_sgi,static_tx_mask,static_rx_mask,
                            static_iss,static_rate,static_tx_pwr,static_tx_gain),static_rx_report ? static_freq : 0);
                pthread_mutex_unlock(hMutex);
                return true;
            } else return false;
        case ART_MODE_STANDBY:
            art2_guiShowMenu(hMutex);
            return false;
        default: return false;
    }
    return false;
}