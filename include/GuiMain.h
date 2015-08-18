/* 
 * File:   GuiMain.h
 * Author: wyy
 *
 * Created on 2014年3月5日, 下午12:03
 */

#ifndef GUIMAIN_H
#define	GUIMAIN_H

#ifdef  RELEASE_ON_ARM
#define IS_ENTER_KEY(nKey)  true
#else   //RELEASE_ON_ARM
#define IS_ENTER_KEY(nKey)  (KEY_ENTER == nKey || KEY_LF == nKey)
#endif  //RELEASE_ON_ARM

#define KEY_LF          ((short)10)
#define KEY_ENTER       ((short)13)
#define KEY_ESC         ((short)27)
#define KEY_CTRL_A      ((short)1)
#define KEY_CTRL_Q      ((short)17)
#define KEY_CTRL_Z      ((short)26)
#define KEY_BACKSPACE   ((short)127)

#define MAX_KEYS_BUFF   20

enum WorkingMode {
    MODE_SERVER = 1,
    MODE_GUI,
    MODE_SCAN,
    MODE_SVR_DEBUG,
    MODE_COMMAND_LINE,
};

#define _GWS_DEBUG

#define SET_MESH_ID     1
#define GET_WDS_ID      2
#define GET_MESH_ID     3
#define SET_STATION_NO  4
#define GET_STATION_NO  5
#define GET_IP_ADDRESS  6
#define SET_ENCRYPTION  7
#define SET_WDS_ID      8
#define SHUTDOWN        200

#define SET_TXATT       'a'
#define SET_BANDWIDTH   'b'
#define SET_CHAN        'c'
#define SET_FREQ        'f'
#define SET_GAIN        'g'
#define DISP_STYLE      'l'
#define SET_MODE        'm'
#define NEXT_PAGE       'n'
#define SET_TXPWR       'p'
#define CMD_QUIT        'q'
#define CMD_QUIT_       'Q'
#define SET_RXON        'r'
#define SCAN_CHAN       'S'
#define STOP_CHAN       's'
#define SET_TXON        't'
#define SET_REGION      'u'
#define SET_RXCAL       'v'
#define WIFI_TXPW       'w'
#define SET_TXCAL       'x'
#define SCAN_FIXED_CHAN 'z'

#define ART2_LOAD       'o'
#define ART2_STANDBY    '0'
#define ART2_TX         '1'
#define ART2_RX         '2'
#define ART2_TX_MASK    SET_TXON
#define ART2_RX_MASK    SET_RXON
#define ART2_FREQ       SET_FREQ
#define ART2_TX_GAIN    SET_GAIN
#define ART2_TX100      SET_TXCAL
#define ART2_TX_PWR     WIFI_TXPW
#define ART2_TX_RATE    'e'
#define ART2_TX_SGI     STOP_CHAN
#define ART2_TX_BW      SET_BANDWIDTH

#define CMD_SERVER_DOWN     "down"
#define CMD_RADIO_TX        "tx"
#define CMD_RADIO_RX        "rx"
#define CMD_RADIO_TX_CAL    "settxcal"
#define CMD_RADIO_RX_CAL    "setrxcal"
#define CMD_RADIO_CHANNEL   "setchan"
#define CMD_RADIO_POWER     "settxpwr"
#define CMD_RADIO_GAIN      "setrxgain"
#define CMD_RADIO_ATTEN     "settxatten"
#define CMD_WIFI_POWER      "setwifipwr"
#define CMD_WIFI_BANDWIDTH  "setbw"
#define CMD_SET_MESHID      "setmeshid"
#define CMD_GET_MESHID      "getmeshid"
#define CMD_SET_SSID        "setssid"
#define CMD_GET_SSID        "getssid"
#define CMD_SET_KEY         "setencryptionkey"
#define CMD_SET_STA_NO      "setsn"
#define CMD_GET_STA_NO      "getsn"
#define CMD_GET_LOCAL_IP    "getip"
#define CMD_CHAN_SCAN       "scan"
#define CMD_FIXED_SCAN      "fixedscan"
#define CMD_STOP_SCAN       "stop"

#define COMMAND_LINE    "1"
#define COMMAND_COLUM   "1"
#define STATUS_LINE     "24"
#define BOTTOM_LINE     "25"

#define LINE_NO_STATUS  24

#define SCAN_LOG_FILE   "/var/run/scanf.lst"

#define ShowStatusBar(fmt,...)        do {              \
            printf(VT100_GOTO(STATUS_LINE,"1"));               \
            printf(VT100_RESET VT100_INVERT fmt VT100_RESET,##__VA_ARGS__); \
            printf(VT100_LOAD_CURSOR);\
        } while (0)


#endif	/* GUIMAIN_H */

