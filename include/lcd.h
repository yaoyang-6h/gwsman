/* 
 * File:   lcd.h
 * Author: wyy
 *
 * Created on 2014年6月20日, 上午11:22
 */

#ifndef LCD_H
#define	LCD_H

#ifdef	__cplusplus
extern "C" {
#endif

#define GWS_TXPWR_MIN 0
#define GWS_TXPWR_MAX 33

#define SP_FILE         "/dev/ttyS0"
#define SP_BUF_MAX_LEN  128
#define SP_QUEUE_SIZE   32
#define LCD_TIME_SLOT   50000           //50ms
#define TICKS_SECOND    (1000000/LCD_TIME_SLOT)
#define MESH_CHK_INTL   (5*TICKS_SECOND)    //5 seconds 
#define SP2LCD_INTL_L(i)   (i*TICKS_SECOND)    //4 seconds 
#define SP2LCD_INTL_S   (2*TICKS_SECOND)    //2 seconds 
#define SP2LCD_SERV     1
#define LCD_DEBUG(dbg,fmt,...)    do {    \
            if (dbg) { \
                printf(fmt,##__VA_ARGS__); \
            }   \
        } while (0)
            
//#define LCD_DEBUG     printf
#define REPORT_NONE         0
#define REPORT_STATUS       1
#define REPORT_MULTI_SNR    2
#define REPORT_TYPE_MAX     2

#define BUILD_DBG_RESPONSE(t,s) sprintf(t, "\r\n%s +wb:%s\r\n", "gws_get_signal ->", s)
#define BUILD_RESPONSE(t,s)     sprintf(t, "\r\n+wb:%s\r\n", s)
#define SEND_RESPONSE           do {    \
                                    sp_send(NO_PROMPT); \
                                } while (0)

extern char sp_tx_buf[SP_BUF_MAX_LEN];
extern char sp_rx_buf[SP_BUF_MAX_LEN];

char*       gws_get_chlist(int region);
int lcd_main(char* lcd_port,int distance,char neg_mode,bool trace_lcd,bool trace_neg);

#ifdef	__cplusplus
}
#endif

#endif	/* LCD_H */

