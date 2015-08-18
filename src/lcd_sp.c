//**
//* by Qige from 6Harmonics Inc. @ Beijing
//* 2014.03.13
//**

#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

#include "gwsmanlib.h"
#include "gwsman.h"
#include "lcd.h"

static struct termios _opt;
static char *sp_file = SP_FILE;
static int sp_fd = -1;
typedef struct __struct_sp_rx_cache {
    int     flag;
    int     leng;
    char    buff[SP_BUF_MAX_LEN];
} SP_RX_CACHE;
typedef struct __struct_sq_rx_queue {
    SP_RX_CACHE queue[SP_QUEUE_SIZE];
    int         first;
    bool        dirty;
} SP_RX_QUEUE;

#define SP_RX_CACHE_EMPTY   0x0000
#define SP_RX_CACHE_START   0x0001
#define SP_RX_CACHE_BODY    0x0002
#define SP_RX_CACHE_HALF    (SP_RX_CACHE_START | SP_RX_CACHE_BODY)
#define SP_RX_CACHE_RT      0x0010
#define SP_RX_CACHE_LN      0x0020
#define SP_RX_CACHE_END     (SP_RX_CACHE_RT | SP_RX_CACHE_LN)
//#define SP_RX_CACHE_READY   (SP_RX_CACHE_HALF | SP_RX_CACHE_END)

static SP_RX_QUEUE sp_rx_queue;
char sp_tx_buf[SP_BUF_MAX_LEN];
char sp_rx_buf[SP_BUF_MAX_LEN];

//#if 1
//static _debug = true;
//#else
//static _debug = false;
//#endif

bool If_cache_ready(SP_RX_CACHE* cache) {
    int flag = cache->flag;
    if ((flag & SP_RX_CACHE_HALF) == SP_RX_CACHE_HALF && 
        (flag & SP_RX_CACHE_END) && cache->leng > 0) {
        return true;
    } else return false;
}

bool If_append_done(const uint _debug,SP_RX_CACHE* cache) {
    bool ready = If_cache_ready(cache);
    
    if (ready) {
        LCD_DEBUG(_debug,"+++ <%d> [%s]", sp_rx_queue.first, cache->buff);
        sp_rx_queue.first ++;
        sp_rx_queue.first %= SP_QUEUE_SIZE;
        sp_rx_queue.dirty = true;
    }
    return ready;
}

bool Append_cache(const uint _debug,SP_RX_CACHE* cache,char c) {  //return if is READY
    if (cache->leng < SP_BUF_MAX_LEN - 1) {
        if ('+' == c) {
            if (SP_RX_CACHE_EMPTY == cache->flag) {
                cache->flag = SP_RX_CACHE_START;
                cache->buff[cache->leng] = c;
                cache->leng ++;
            } else {    // A '+' followed an other '+' without contents and '\r\n' bewteen them
                cache->leng = 0;
                cache->flag = SP_RX_CACHE_START;
            }
        } else if ('\r' == c) {
            if (SP_RX_CACHE_HALF == cache->flag) {
                cache->flag |= SP_RX_CACHE_RT;
            } else return false;   //ignore this '\r'
        } else if ('\n' == c) {
            if (SP_RX_CACHE_HALF == cache->flag) {
                cache->flag |= SP_RX_CACHE_LN;
            } else return false;   //ignore this '\n'
        } else {
            if (SP_RX_CACHE_START == cache->flag) {
                cache->flag |= SP_RX_CACHE_BODY;
                cache->buff[cache->leng] = c;
                cache->leng ++;
            } else if (SP_RX_CACHE_HALF == cache->flag) {
                cache->buff[cache->leng] = c;
                cache->leng ++;
            } else {
                cache->flag = SP_RX_CACHE_EMPTY;
                cache->leng = 0;
                return false; //ignore this charactor and reset this cache
            }
        }
        cache->buff[cache->leng] = 0;
    } else {    //exceed length limitation,drop it
        cache->flag = SP_RX_CACHE_EMPTY;
        cache->leng = 0;
    }
    return (If_append_done(_debug,cache));
}

int Pop_queue_head(const uint _debug,char* rx_buf) {
    int i,head = sp_rx_queue.first;
    if (NULL == rx_buf || !sp_rx_queue.dirty) return 0;
//    LCD_DEBUG(_debug," head = %d ",head);
    for (i = 0; i < SP_QUEUE_SIZE; i ++) {
        if (If_cache_ready(&sp_rx_queue.queue[head])) {
            int len = sp_rx_queue.queue[head].leng;
            memcpy(rx_buf,sp_rx_queue.queue[head].buff,sp_rx_queue.queue[head].leng);
            rx_buf[sp_rx_queue.queue[head].leng] = 0;
            LCD_DEBUG(_debug," --- <%d> [%s]",head, rx_buf);
            sp_rx_queue.queue[head].leng = 0;
            sp_rx_queue.queue[head].flag = SP_RX_CACHE_EMPTY;
            LED_SET(LED_LCD,0);
            return len;
        } else {
//            LCD_DEBUG(_debug,"[%d,f=%x,l=%d]",head,sp_rx_queue.queue[head].flag,sp_rx_queue.queue[head].leng);
            head ++;
            head %= SP_QUEUE_SIZE;
        }
    }
//    LCD_DEBUG(_debug,"none\n");
    sp_rx_queue.dirty = false;
    return 0;
}

SP_RX_CACHE* Get_queue_first_free() {
    return &sp_rx_queue.queue[sp_rx_queue.first];
}

char* sp_filename() {
    return sp_file;
}

void sp_close() {
    if (sp_fd >= 0) {
        tcsetattr(sp_fd, TCSANOW, &_opt);
        close(sp_fd);
        sp_fd = -1;
    }
}

int sp_open(char* dev_name) {
    sp_close();
    if (NULL == dev_name)
        sp_fd = open(sp_file, O_RDWR);
    else sp_fd = open(dev_name, O_RDWR);
    return sp_fd;
}

void sp_init(int dev_tty) {
    if (dev_tty >= 0) {
        struct termios opt;
        fcntl(dev_tty, F_SETFL, FNDELAY);
        tcgetattr(dev_tty, &_opt);
        tcgetattr(dev_tty, &opt);
        cfsetispeed(&opt, B9600);
        cfsetospeed(&opt, B9600);
        opt.c_cflag |= (CLOCAL | CREAD);
        opt.c_cflag &= ~PARENB;
        opt.c_cflag &= ~CSTOPB;
        opt.c_cflag &= ~CSIZE;
        opt.c_cflag |= CS8;
        opt.c_cflag &= ~CRTSCTS;
        opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        opt.c_oflag &= ~OPOST;
        tcsetattr(dev_tty, TCSANOW, &opt);
        memset(sp_tx_buf, 0, SP_BUF_MAX_LEN);
        memset(sp_rx_buf, 0, SP_BUF_MAX_LEN);
        memset(&sp_rx_queue, 0, sizeof(sp_rx_queue));
    }
}

void sp_send(const uint _debug) {
    if (sp_fd >= 0) {
        if (strlen(sp_tx_buf) > 0) {
            LCD_DEBUG(_debug,"- sp_tx_buf = [%s]\tbefore send\n", sp_tx_buf);
            LED_SET(LED_LCD,1);
            write(sp_fd, sp_tx_buf, strlen(sp_tx_buf));
        }
        memset(sp_tx_buf, 0, sizeof (sp_tx_buf));
        LCD_DEBUG(_debug,"- sp_tx_buf = [%s]\tafter send\n", sp_tx_buf);
    }
}

int sp_recv(const uint _debug) {
    if (sp_fd >= 0) {
        int i = 0, nread = 0;
        char _buf[SP_BUF_MAX_LEN ];
        memset(_buf, 0, sizeof (_buf));
        while ((nread = read(sp_fd, _buf, SP_BUF_MAX_LEN - 1)) > 0) {
            for (i = 0; i < nread; i ++) {
                Append_cache(_debug,Get_queue_first_free(),_buf[i]);
            }
        }
        return Pop_queue_head(_debug,sp_rx_buf);
    }
    return 0;
}

void sp_rx_filter() {
    char *p;
    while (p = strchr(sp_rx_buf, '\r')) {
        *p = '\n';
    }
    while (p = strstr(sp_rx_buf, "\n\n\n")) {
        *p = '\n';
        *(p + 1) = ' ';
        *(p + 2) = '\t';
    }
}
