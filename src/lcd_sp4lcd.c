//**
//* 6Harmonics Inc.
//* Prj: E.C.
//* REV: v1.0.0-1
//*
//* Used for Global White Space REV2
//*
//* Source code modified from Yu Zhou, Ottawa
//* by Qige Zhao, Beijing
//* 2014.03.18
//**

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <syslog.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "GuiMain.h"
#include "gwsmanlib.h"
#include "lcd.h"

int lcd_main(char* lcd_port,int distance,char neg_mode,bool trace_lcd,bool trace_neg) {
    int dev_tty = sp_open(lcd_port);
    if (dev_tty < 0) {
        LCD_DEBUG(trace_lcd,"Error: cannot open serial port(%s).\n",lcd_port);
        return -1;
    }
    sp_init(dev_tty);
    sp2lcd_serv(distance,neg_mode,trace_lcd,trace_neg);
    sp_close();
    return 0;
}

