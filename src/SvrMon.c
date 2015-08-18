#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gwsman.h"

#define IF_NOT_EXIST    0
#define IF_IS_ACTIVE    1
#define IF_IS_DOWN      2

static FILE* fd_eth0 = NULL;
static FILE* fd_eth1 = NULL;

void svrClearSysMon() {
    if (fd_eth0) fclose(fd_eth0);
    if (fd_eth1) fclose(fd_eth1);
    fd_eth0 = NULL;
    fd_eth1 = NULL;
}

static int IsIfUp(char* iface) {
    FILE*   fd = fopen(iface,"r");
    if (NULL != fd) {
        char* eof = NULL;
        char value[128];
        
        memset(value,0x00,128);
        fseek(fd,0,SEEK_SET);
        eof = fgets(value,127,fd);
        fclose(fd);
        if (eof != value) {
            return IF_IS_DOWN;      //led turn off
        } else if (strncmp(value,"up",2) == 0) {
            return IF_IS_ACTIVE;    //led blink
        } else if (strncmp(value,"down",4) == 0) {
            return IF_IS_DOWN;      //led turn on
        } else {
            return IF_IS_DOWN;       //led turn off
        }
    } else return IF_NOT_EXIST;
}

#ifndef bool
#define bool    unsigned int
#define true    1
#define false   0
#endif

static unsigned long nTickCounter = 0;
static int led_status_on = false;

void svrTimerSysMon(unsigned long ticks,int shmID,P_GWS_KPI pKpi) {
    if (0 == (ticks % 2)) {                 //every 100 msecs
        bool    busy = svrRouteHandler(ticks,shmID,pKpi);
        unsigned long nFast = ticks % 4;    //every 200 msecs
        unsigned long nSlow = ticks % 10;   //every 500 msecs
        unsigned long nIntv = busy ? nFast : nSlow;
        if (0 == nIntv) led_status_on = !led_status_on;
        LED_SET(LED_STATUS,led_status_on);
    }
}
