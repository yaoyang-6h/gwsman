/* 
 * File:   gwsman.h
 * Author: wyy
 *
 * Created on 2014年2月27日, 下午4:56
 */

#ifndef GWSMAN_H
#define	GWSMAN_H

#include "shmem.h"
#ifdef	__cplusplus
extern "C" {
#endif
    
#define LOGO_PROMPT         "6H-" //6-Harmonics "
#define VERSION_INFO        "1.14-6"
#define MAX_CHANNEL_NOISE   -55
#define MIN_CHANNEL_NOISE   -96
#define CONFIG_FILE     "/etc/config/gwsman.conf"
#define DEBUG_INFO_BASE "/sys/kernel/debug/ieee80211/phy0/ath9k/"
#define DEBUG_INFO_QUEUES  "queues"
#define DEBUG_INFO_QLEN_KEY "pending:"

#define GWS_ERROR_INVALIDATE_CHANNEL    -1

#define ART_MODE_UNLOAD     0
#define ART_MODE_STANDBY    1
#define ART_MODE_TX         2
#define ART_MODE_RX         3

//#define IS_SERV_AR9344
    
#ifdef  IS_SERV_AR9344
#define PLATFORM        "/sys/devices/platform/"
#define DEV_NAME_OPS    "/operstate"
#define DEV_NAME_BRN    "/brightness"

#define IF_ETH0     PLATFORM "ag71xx.1/net/eth0"        DEV_NAME_OPS
#define IF_ETH1     PLATFORM "ag71xx.1/net/eth1"        DEV_NAME_OPS
#define IF_WLAN0    PLATFORM "ar934x_wmac/net/wlan0"    DEV_NAME_OPS
#define IF_LED      PLATFORM "leds-gpio/leds/gh0228:"

#define LED_SYSTEM  IF_LED "system"  DEV_NAME_BRN
#define LED_STATUS  IF_LED "status"  DEV_NAME_BRN
#define LED_ETH0    IF_LED "eth0"    DEV_NAME_BRN
#define LED_WLAN0   IF_LED "wifi"    DEV_NAME_BRN
#define LED_RADIO   IF_LED "radio"   DEV_NAME_BRN
#define LED_LCD     IF_LED "lcd"     DEV_NAME_BRN

#define LED_SET(name,val)      do {    \
        char    cmd[1024];      \
        memset (cmd,0x00,1024); \
        sprintf(cmd,"echo %d > %s",val,name);   \
        system(cmd);    \
} while(0)
#else   //NO GWS_LEDS
#define LED_SET(name,val)      do {} while(0)
#endif  //IS_SERV_AR9344

void iw_setchannel(int nChannel, P_GWS_KPI pKpi, int nId);
short iw_getcurrchan(P_GWS_KPI pKpi, int nId);
int iw_noise(const char* ifname, int nChannel, P_GWS_KPI pKpi, int nId);
char* iwinfo(const char* ifname, int key, P_GWS_KPI pKpi, int nId);
int chanscan_main(int argc, char** argv);

#endif	/* GWSMAN_H */

