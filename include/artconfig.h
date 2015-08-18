/* 
 * File:   artconfig.h
 * Author: wyy
 *
 * Created on 2014年5月28日, 下午2:45
 */

#ifndef ARTCONFIG_H
#define	ARTCONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif
//===============================================ANSI控制码的说明
#define VT100_FLAG0         "\x1b"          //ESC
#define VT100_FLAG1         "["
#define VT100_FLAG          VT100_FLAG0 VT100_FLAG1
#define VT100_HEAD          VT100_FLAG
#define VT100_UP            VT100_HEAD "A"      //ESC[nA 光标上移n行
#define VT100_DOWN          VT100_HEAD "B"      //ESC[nB 光标下移n行
#define VT100_RIGHT         VT100_HEAD "C"      //ESC[nC 光标右移n行
#define VT100_LEFT          VT100_HEAD "D"      //ESC[nD 光标左移n行
#define VT100_CLR2EOF       VT100_HEAD "J"      //
#define VT100_CLR_EOL       VT100_HEAD "K"      //ESC[K 清除从光标到行尾的内容
#define VT100_CLEAR         VT100_HEAD "2J"     //ESC[2J 清屏
#define VT100_SAVE_CURSOR   VT100_HEAD "s"      //ESC[s 保存光标位置
#define VT100_LOAD_CURSOR   VT100_HEAD "u"      //ESC[u 恢复光标位置
#define VT100_HIDE_CURSOR   VT100_HEAD "?25l"   //ESC[?25l 隐藏光标
#define VT100_SHOW_CURSOR   VT100_HEAD "?25h"   //ESC[?25h 显示光标
#define VT100_RESET         VT100_HEAD "0m"     //ESC[0m 关闭所有属性
#define VT100_HIGHT_LIGHT   VT100_HEAD "1m"     //ESC[1m 设置高亮度
#define VT100_UNDER_LINE    VT100_HEAD "4m"     //ESC[4m 下划线
#define VT100_FLASH         VT100_HEAD "5m"     //ESC[5m 闪烁
#define VT100_INVERT        VT100_HEAD "7m"     //ESC[7m 反显
#define VT100_INVISIBLE     VT100_HEAD "8m"     //ESC[8m 消隐
#define VT100_GOTO(x,y)     VT100_HEAD x ";" y "H" VT100_CLR_EOL    //ESC[y;xH设置光标位置
#define VT100_COLOR(b,f)    VT100_HEAD b ";" f "m"                  //
//ESC[30m&nbsp-- ESC[37m 设置前景色
//ESC[40m&nbsp-- ESC[47m 设置背景色

//字颜色:30-----------39
#define VT100_F_BLACK       "30"    //黑
#define VT100_F_RED         "31"    //红
#define VT100_F_GREEN       "32"    //绿
#define VT100_F_YELLOW      "33"    //黄
#define VT100_F_BLUE        "34"    //蓝色
#define VT100_F_PINK        "35"    //紫色
#define VT100_F_DARK_G      "36"    //深绿
#define VT100_F_WHITE       "37"    //白色

//字背景颜色范围:40----49
#define VT100_B_BLACK       "40"    //黑
#define VT100_B_RED         "41"    //深红
#define VT100_B_GREEN       "42"    //绿
#define VT100_B_YELLOW      "43"    //黄色
#define VT100_B_BLUE        "44"    //蓝色
#define VT100_B_PINK        "45"    //紫色
#define VT100_B_DARK_G      "46"    //深绿
#define VT100_B_WHITE       "47"    //白色

//#define VT100_STYLE0        VT100_RESET
#define VT100_STYLE0        VT100_COLOR(VT100_B_BLACK,VT100_F_GREEN)
#define VT100_STYLE1        VT100_COLOR(VT100_B_BLACK,VT100_F_WHITE) VT100_HIGHT_LIGHT
#define VT100_STYLE2        VT100_COLOR(VT100_B_DARK_G,VT100_F_WHITE) VT100_HIGHT_LIGHT
#define VT100_STYLE_BLUE    VT100_COLOR(VT100_B_BLUE,VT100_F_WHITE) VT100_HIGHT_LIGHT
#define VT100_STYLE_ALERT   VT100_COLOR(VT100_B_RED,VT100_F_YELLOW) VT100_HIGHT_LIGHT
#define VT100_STYLE_NORMAL  VT100_COLOR(VT100_B_GREEN,VT100_F_WHITE) VT100_HIGHT_LIGHT
#define VT100_STYLE_MENU    VT100_RESET VT100_COLOR(VT100_B_BLACK,VT100_F_GREEN) VT100_HIGHT_LIGHT
#define VT100_STYLE_KEY     VT100_COLOR(VT100_B_BLACK,VT100_F_YELLOW) VT100_HIGHT_LIGHT VT100_UNDER_LINE
#define VT100_STYLE_HOT     VT100_RESET VT100_COLOR(VT100_B_BLACK,VT100_F_RED) VT100_HIGHT_LIGHT
#define DISP_NORMAL     VT100_RESET
#define DISP_READONLY   VT100_COLOR(VT100_B_BLACK,VT100_F_WHITE)
#define DISP_READWRITE  VT100_COLOR(VT100_B_RED,VT100_F_WHITE) VT100_UNDER_LINE
#define DISP_DYNAMIC    VT100_COLOR(VT100_B_BLACK,VT100_F_WHITE) VT100_HIGHT_LIGHT

#define     KEY_ESC     0x1B
#define     KEY_ENTER   0x0D
#define     KEY_UP      0x41
#define     KEY_DOWN    0X42
#define     KEY_RIGHT   0x43
#define     KEY_LEFT    0x44
#define     KEY_K_UP    'k'
#define     KEY_J_DOWN  'j'
#define     KEY_L_RIGHT 'l'
#define     KEY_H_LEFT  'h'
#define     KEY_C_HIDE  'm'
#define     TITLE_WIDTH 18
#define     COL1        1
#define     COL2        33
#define     COL3        57
#define     LN_STATUS   22
#define     LN_PROMPT   23
    
typedef struct __art_index {
    int         m_param_key;
    char        m_param_title[32];
} ART_PARA_INDEX;

enum CValueType {
    VAR_BYTE = 0,
    VAR_DBM,
    VAR_UCHAR,
    VAR_CHAR,
    VAR_REF_PWR,
    VAR_WORD,
    VAR_DWORD,
    VAR_BINARY,
    VAR_STRING,
};

typedef struct __binding_value {
    int             m_nType;
    int             m_nSize;
    unsigned char*  m_pValue;
} BindValue;

typedef struct __val_position_struct {
    int             m_curr;
    int             m_max;
} QueuePosition;

inline void print_value(BindValue* val,bool highlight) {
    if (val) {
        int i = 0;
        if (highlight) {
            printf(VT100_RESET VT100_STYLE_ALERT);
        }
        switch (val->m_nType) {
            case VAR_WORD:
            case VAR_DWORD:
                for (i = val->m_nSize - 1; i >= 0 ; i --)
                    printf("%02X", 0x00ff & val->m_pValue[i]);
                break;
            case VAR_BYTE:
                printf("%02X", 0x00ff & val->m_pValue[0]);
                break;
            case VAR_DBM:
                printf("%2.1f", (float) (0x00ff & val->m_pValue[0]) / 2.0);
                break;
            case VAR_UCHAR:
                i = (unsigned char)(0x00ff & val->m_pValue[0]);
                printf("%+d", -i);
                break;
            case VAR_CHAR:
                i = (char)(0x00ff & val->m_pValue[0]);
                printf("%+d", -i);
                break;
            case VAR_REF_PWR:
                i = (char)(0x00ff & val->m_pValue[0]);
                printf("%+d", -i);
                break;
            case VAR_BINARY:
                for (i = 0; i < val->m_nSize; i ++)
                    printf("%02x", 0x00ff & val->m_pValue[i]);
                break;
            case VAR_STRING:
                for (i = 0; i < val->m_nSize; i ++)
                    printf("%c", 0x00ff & val->m_pValue[i]);
                break;
            default:;
        }
    }
}

static QueuePosition cachePosition = {0,0};
static QueuePosition paramPosition = {0,0};

typedef struct __var_binding {
    char        m_k;
    int         m_x;
    int         m_y;
    char        m_title[32];
    char        m_attri[32];
    BindValue   m_value;
} VarBind;

inline char* VT100_GOTO_XY(int x,int y) {
    static char sVT100_FROMAT_STRING[32];
    memset( sVT100_FROMAT_STRING,0x00,32 );
    sprintf( sVT100_FROMAT_STRING,VT100_HEAD"%d;%dH"VT100_CLR_EOL,x,y);
    return sVT100_FROMAT_STRING;
}

inline void print_varBind(VarBind* vb,int colWidth,bool highlight) {
    if (vb && vb->m_x > 0 && vb->m_y > 0) {
        printf(VT100_RESET);
        if (colWidth > 0) {
            printf("%s" VT100_STYLE_KEY "%02d " VT100_STYLE_MENU "%s",
                    VT100_GOTO_XY(vb->m_x + 1, vb->m_y), vb->m_k, vb->m_title);
            printf("%s",VT100_GOTO_XY(vb->m_x + 1, vb->m_y + colWidth));
            printf(VT100_RESET VT100_INVERT "%s", vb->m_attri);
            print_value(&vb->m_value,highlight);
        }
    }
}

inline void RefresPrompt(char* msg) {
    printf(VT100_RESET);
    printf("%s ["   VT100_STYLE_HOT "ARROW " VT100_RESET "Move] ["
                    VT100_STYLE_HOT "m " VT100_RESET "Show/Hide Power Table] ["
                    VT100_STYLE_HOT "ENTER " VT100_RESET "Modify] ["
                    VT100_STYLE_HOT "W " VT100_RESET "Save] ["
                    VT100_STYLE_HOT "Q " VT100_RESET "Quit] ",
                    VT100_GOTO_XY(LN_PROMPT, 1));
    if (msg) printf("%s",msg);
    fflush(stdout);
}

inline void print_varForEdit(VarBind* vb,int curr) {
    int i = 0;
    
    if (vb && vb->m_x > 0 && vb->m_y > 0) {
        RefresPrompt(NULL);
        printf(VT100_RESET);
        printf("%s" VT100_STYLE_KEY  "%s", VT100_GOTO_XY(LN_STATUS, 1), vb->m_title);
        printf("%s",VT100_GOTO_XY(LN_STATUS, TITLE_WIDTH));
        printf(VT100_RESET VT100_INVERT "%s", vb->m_attri);

        if (vb->m_value.m_nSize > 0 && vb->m_value.m_pValue) {
            switch (vb->m_value.m_nType) {
            case VAR_WORD:
            case VAR_DWORD:
                for (i = vb->m_value.m_nSize - 1; i >= 0 ; i --) {
                    if (curr == i) {
                        printf(VT100_SAVE_CURSOR);
                        printf(VT100_INVERT);
                    } else {
                        printf(VT100_RESET);
                    }
                    printf(" %02X", 0x00ff & vb->m_value.m_pValue[i]);
                }
                break;
            case VAR_BYTE:
                printf("%02X", 0x00ff & vb->m_value.m_pValue[0]);
                break;
            case VAR_UCHAR:
                i = (unsigned char)(0x00ff & vb->m_value.m_pValue[0]);
                printf("%+d", -i);
                break;
            case VAR_CHAR:
                i = (char)(0x00ff & vb->m_value.m_pValue[0]);
                printf("%+d", -i);
                break;
            case VAR_REF_PWR:
                i = (char)(0x00ff & vb->m_value.m_pValue[0]);
                printf("%+d", -i);
                break;
            case VAR_DBM:
                printf("%2.1f", (float) (0x00ff & vb->m_value.m_pValue[0]) / 2.0);
                break;
            case VAR_BINARY:
                for (i = 0; i < vb->m_value.m_nSize; i ++) {
                    if (curr == i) {
                        printf(VT100_SAVE_CURSOR);
                        printf(VT100_INVERT);
                    } else {
                        printf(VT100_RESET);
                    }
                    printf(" %02X", 0x00ff & vb->m_value.m_pValue[i]);
                }
                break;
            case VAR_STRING:
                for (i = 0; i < vb->m_value.m_nSize; i ++) {
                    if (curr == i) {
                        printf(VT100_SAVE_CURSOR);
                        printf(VT100_INVERT);
                    } else {
                        printf(VT100_RESET);
                    }
                    printf(" %c", 0x00ff & vb->m_value.m_pValue[i]);
                }
                break;
            default:;
            }
        printf(VT100_LOAD_CURSOR);
//            printf("%s",VT100_GOTO_XY(LN_STATUS, TITLE_WIDTH + 3 * cachePosition.m_curr));

        }
    } else {
        printf("%s" VT100_CLR_EOL,VT100_GOTO_XY(LN_STATUS, 1));
        printf("%s" VT100_CLR_EOL,VT100_GOTO_XY(LN_PROMPT, 1));
    }
}


#ifdef	__cplusplus
}
#endif

#endif	/* ARTCONFIG_H */

