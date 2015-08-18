/* 
 * File:   main.cc
 * Author: liveuser
 *
 * Created on 2009年2月10日, 下午9:35
 */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <signal.h>
#include <bits/signum.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "../include/c-serial-com.h"
#include "../include/kbd.h"
#include "shmem.h"
#include "gwsman.h"
#include "gwsmanlib.h"
#include "GuiMain.h"
#include "wsocket.h"

/*
 * 
 */

#define COLOR_400    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_RED) VT100_HIGHT_LIGHT)
#define COLOR_300    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_PINK) VT100_HIGHT_LIGHT)
#define COLOR_200    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_BLUE) VT100_HIGHT_LIGHT)
#define COLOR_100    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_GREEN) VT100_HIGHT_LIGHT)

#define COLOR_401    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_RED) VT100_UNDER_LINE)
#define COLOR_301    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_PINK) VT100_UNDER_LINE)
#define COLOR_201    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_BLUE) VT100_UNDER_LINE)
#define COLOR_101    (VT100_COLOR(VT100_B_BLACK  ,VT100_F_GREEN) VT100_UNDER_LINE)

#define COLOR_41    (VT100_COLOR(VT100_B_RED    ,VT100_F_WHITE) VT100_HIGHT_LIGHT)
#define COLOR_31    (VT100_COLOR(VT100_B_PINK   ,VT100_F_WHITE) VT100_HIGHT_LIGHT)
#define COLOR_21    (VT100_COLOR(VT100_B_BLUE   ,VT100_F_WHITE) VT100_HIGHT_LIGHT)
#define COLOR_11    (VT100_COLOR(VT100_B_GREEN  ,VT100_F_WHITE) VT100_HIGHT_LIGHT)
#define COLOR_CUR   VT100_COLOR(VT100_B_RED,VT100_F_YELLOW) VT100_HIGHT_LIGHT VT100_FLASH
#define VT100_STYLE_TITLE   VT100_COLOR(VT100_B_BLACK,VT100_F_YELLOW) VT100_HIGHT_LIGHT

#define MIN_NOISE_FLOOR     (MIN_CHANNEL_NOISE)

#define ENABLE_GWSMAN_LOG   0
#define PAGE_0              0
#define PAGE_1              1
#define PAGE_2              2
#define PAGE_3              3
#define PAGE_MAX            (1+PAGE_3)

#define NF_DISP_STYLE_OUTLINE   0
#define NF_DISP_STYLE_THIN_BAR  1
#define NF_DISP_STYLE_WIDE_BAR  2
#define NF_DISP_STYLE_MAX       3
#define NF_INFO_BUFF_SIZE       1024

typedef struct __struct_nf_string {
    char    ch_string[NF_INFO_BUFF_SIZE];
    char    nf_string[NF_MAX_DIVS_1][NF_INFO_BUFF_SIZE];
} NF_STRING;


static bool bReadyGo = false;
static unsigned char nCurrentPage = PAGE_0;
static unsigned char nDisplaySchema = NF_DISP_STYLE_THIN_BAR;

static char strPrompt[128];
void guiPromptSet(pthread_mutex_t* hMutex, char* prompt) {
    if (0 == pthread_mutex_trylock(hMutex)) {
        if (prompt)
            strcpy (strPrompt,prompt);
        else strPrompt[0] = 0;
        pthread_mutex_unlock(hMutex);
    }
}

void guiPromptGet(pthread_mutex_t* hMutex, char* prompt) {
    if (0 == pthread_mutex_trylock(hMutex)) {
        if (prompt)
            strcpy (prompt,strPrompt);
        pthread_mutex_unlock(hMutex);
    }
}

char* guiGetTimeString() {
    static char strTime[32];
    static bool disp_reset;
    static time_t last_seconds;
    time_t current_time = time(&current_time);
    time_t day_seconds = current_time % 86400;
    time_t hour = day_seconds / 3600;
    time_t hour_seconds = day_seconds % 3600;
    time_t minute = hour_seconds / 60;
    time_t seconds = hour_seconds % 60;
    if (last_seconds != seconds) {
        last_seconds = seconds;
        disp_reset = !disp_reset;
    }
    memset (strTime,0x00,32);
    sprintf(strTime,"%s%02d:%02d:%02d",disp_reset ? VT100_RESET VT100_UNDER_LINE :
                            VT100_STYLE_MENU VT100_UNDER_LINE, hour, minute, seconds);
    return strTime;
}

char* PickupUserInput() {
    static char buff[128];
    
    memset (buff,0x00,128);
    if (fgets(buff, 127, stdin)) {
        char* ln = strchr(buff,'\n');
        if (ln && ln >= buff) ln[0] = 0;
        return buff;
    }
    return NULL;
}

char* guiShowCommandLine(pthread_mutex_t* hMutex, bool bNeedInput) {
    char prompt[128];
    memset (prompt,0x00,128);
    guiPromptGet(hMutex,prompt);
    if (0 == pthread_mutex_lock(hMutex)) {
        char* input = NULL;
        printf(VT100_GOTO(COMMAND_LINE, COMMAND_COLUM));
        printf(VT100_RESET \
                VT100_STYLE_MENU VT100_INVERT " %s" \
                VT100_STYLE_MENU VT100_INVERT " " LOGO_PROMPT "GH0228 v" VERSION_INFO " " \
                VT100_RESET \
                VT100_STYLE_KEY " S " VT100_STYLE_MENU "NF scan "  \
                VT100_STYLE_KEY " Q " VT100_STYLE_MENU "quit " \
                VT100_STYLE_BLUE "GWS > " \
                VT100_INVERT "%s" VT100_RESET, guiGetTimeString(),prompt);
        printf(VT100_SAVE_CURSOR);
        fflush(stdout);
        if (bNeedInput)
            input = PickupUserInput();
        pthread_mutex_unlock(hMutex);
        return input;
    }
    return NULL;
}

void m_guiPrint(VarBind* vb, int colWidth) {    
    if (NULL == vb || 0 == vb->m_k) return;
    IW_LIST* l = NULL;
    printf(VT100_RESET);
    switch (BindValueType(&vb->m_value)) {
        case VAR_BOOL:
        case VAR_STRING:
            printf("%s" VT100_STYLE_KEY "%c " VT100_STYLE_MENU "%s",
                    VT100_GOTO_XY(vb->m_x + 1, vb->m_y), vb->m_k, vb->m_title);
            printf(VT100_GOTO_XY(vb->m_x + 1, vb->m_y + colWidth));
            printf(VT100_INVERT "%s% 6s", vb->m_attri, VB_AsString(&vb->m_value));
            if (ENABLE_GWSMAN_LOG) {
                FILE*   flog = fopen("/var/run/gwsman.log","a+");
                if (flog) {
                    fprintf(flog,"\n%s\t:%s",vb->m_title,VB_AsString(&vb->m_value));
                    fclose(flog);
                }
            }
            break;
        case VAR_INTEGER:
            printf("%s" VT100_STYLE_KEY "%c " VT100_STYLE_MENU "%s",
                    VT100_GOTO_XY(vb->m_x + 1, vb->m_y), vb->m_k, vb->m_title);
            printf(VT100_GOTO_XY(vb->m_x + 1, vb->m_y + colWidth));
            printf(VT100_INVERT "%s% 6d", vb->m_attri, VB_AsInt(&vb->m_value));
            if (ENABLE_GWSMAN_LOG) {
                FILE*   flog = fopen("/var/run/gwsman.log","a+");
                if (flog) {
                    fprintf(flog,"\n%s\t:%d",vb->m_title,VB_AsInt(&vb->m_value));
                    fclose(flog);
                }
            }
            break;
        case VAR_LIST:
            if (NULL != (l = VB_AsList(&vb->m_value))) {
                int j = 0;
                printf("%s" VT100_STYLE_MENU VT100_INVERT "%s" "%s",
                        VT100_GOTO_XY(vb->m_x + 1, vb->m_y), vb->m_title, l->m_header);
                for (j = vb->m_x + 2; j < LINE_NO_STATUS - 1; j ++)
                    printf("%s" VT100_CLR_EOL, VT100_GOTO_XY(j, vb->m_y));
                printf(VT100_RESET);
                for (j = 0; j < l->m_nLine && j < IW_LIST_MAXLINE; j++) {
                    printf(VT100_GOTO_XY(vb->m_x + j + 2, vb->m_y));
                    printf("%s", vb->m_attri);
                    printf("%s", l->m_value[j]);
                }
                printf(VT100_RESET);
            }
            break;
        default:;
    }
}

void guiPrintRfInfo(bool* updated, pthread_mutex_t* hMutex) {
    if (0 == pthread_mutex_trylock(hMutex)) {
        if (!(*updated)) {
            int i = 0;
            *updated = true;
            for (i = 0; i < MAX_GWS_VAR; i++) {
                m_guiPrint(&gwsVars[i], 12);
            }
        }
        pthread_mutex_unlock(hMutex);
    }
}

void guiPrintIwInfo(bool* updated, pthread_mutex_t* hMutex) {
    if (0 == pthread_mutex_trylock(hMutex)) {
        if (!(*updated)) {
            int i = 0;
            *updated = true;
            for (i = 0; i < MAX_WIFI_VAR; i++) {
                m_guiPrint(&wifiVars[i], 12);
            }
        }
        pthread_mutex_unlock(hMutex);
    }
}

#define MAX_MAP_LN   20
#define GET_STYLE_BY_NF(style,nf)   \
    do {\
        if (nf >= -65 ) {\
            strcpy(style, COLOR_41);\
        } else if (nf > -75) {\
            strcpy(style, COLOR_31);\
        } else if (nf > -85) {\
            strcpy(style, COLOR_21);\
        } else {\
            strcpy(style, COLOR_11);\
        }\
    } while (0)

#define PRINT_CHANNEL(rgn,base_freq,ch,bw,column,nf_list) \
    do {\
        char style_NF[64];\
        int chidx = ch - MIN_CHANNEL(rgn);\
        GET_STYLE_BY_NF(style_NF,nf_list[chidx].m_noise_avg);\
        printf("%s" VT100_RESET "%s" VT100_UNDER_LINE "Ch %d"\
                VT100_RESET VT100_STYLE_MENU VT100_UNDER_LINE "%s %dM % 4d dBm ",\
                VT100_GOTO_XY(START_NF_LINE + (chidx % MAX_MAP_LN) + 2,(chidx / MAX_MAP_LN) * 20 + column),\
                (rgn == region) ? style_NF : VT100_COLOR(VT100_B_GREEN,VT100_F_BLACK), ch,\
                (rgn == region && ch == cur_chan) ? VT100_INVERT : "", base_freq + chidx * bw,\
                (rgn == region) ? nf_list[chidx].m_noise_avg : 0);\
    } while (0)

void guiPrintChannelMapping(pthread_mutex_t* hMutex,GWS_CHAN_NF nf_list[],int region,int cur_chan) {
    if (0 == pthread_mutex_trylock(hMutex)) {
        int ch = MIN_CHANNEL(region);
        printf( VT100_STYLE_TITLE VT100_INVERT
                "%s|================ GWS Radio Channel/Center Frequency Mapping ==================|" VT100_RESET,
                VT100_GOTO_XY(START_NF_LINE, 1));
        printf( VT100_RESET VT100_STYLE0 VT100_INVERT
                "%schan  freq    noise|chan  freq    noise|chan  freq    noise|chan  freq    noise|" VT100_RESET,
                VT100_GOTO_XY(START_NF_LINE + 1, 1));
        
        for (ch = MIN_CHANNEL(0); ch <= MAX_CHANNEL(0); ch ++) PRINT_CHANNEL(0,473,ch,6, 1,nf_list);
        for (ch = MIN_CHANNEL(1); ch <= MAX_CHANNEL(1); ch ++) PRINT_CHANNEL(1,474,ch,8,41,nf_list);
        printf( VT100_RESET VT100_STYLE0 VT100_INVERT
                "%s              Region 0                 |              Region 1                 |" VT100_RESET,
                VT100_GOTO_XY(START_NF_LINE + MAX_MAP_LN + 2, 1));
        pthread_mutex_unlock(hMutex);
    }
}

void Fill_Nf_Header(int region,int max_lines,NF_STRING* nf_info) {
    int j;
    int nDbmPerDiv = (max_lines > 1) ? NF_DBM_PER_DIV_2 : NF_DBM_PER_DIV_1;
    int nMaxDivs = (max_lines > 1) ? NF_MAX_DIVS_2 : NF_MAX_DIVS_1;
    memset(nf_info->ch_string, 0x00, NF_INFO_BUFF_SIZE);
    if (0 == region) strcpy(nf_info->ch_string, VT100_INVERT " " VT100_STYLE_BLUE "dBm" VT100_RESET);
    for (j = 0; j < nMaxDivs; j++) {
        memset (nf_info->nf_string[j], 0x00, NF_INFO_BUFF_SIZE);
        if (0 == region) sprintf(nf_info->nf_string[j],
                VT100_INVERT "% 4d" VT100_RESET, j * nDbmPerDiv + MIN_NOISE_FLOOR + 1);
    }
}

void Fill_Channel_Info(short curr_ch,short active_ch,GWS_CHAN_NF *nf,int* last_DC,int max_lines,NF_STRING* nf_info) {
    char sCh[32];
    char sNf[32];
    char sC1[16];
    char sC0[16];
    int i;
    int nDbmPerDiv = (max_lines > 1) ? NF_DBM_PER_DIV_2 : NF_DBM_PER_DIV_1;
    int nMaxRows =  (max_lines > 1) ? NF_MAX_ROWS_2 : NF_MAX_ROWS_1;
    int nLc = *last_DC;
//    int nNf_max = (0 == nf->m_noise_max) ? -1 : nf->m_noise_max - MIN_NOISE_FLOOR;
    int nNf_avg = (nf->m_noise_cur < MIN_NOISE_FLOOR) ? 0 : (nf->m_noise_cur - MIN_NOISE_FLOOR);
    int nNf_min = (nf->m_noise_min < MIN_NOISE_FLOOR) ? 0 : (nf->m_noise_min < MIN_NOISE_FLOOR);

    nNf_avg = (0 == nf->m_noise_cur) ? -1 : nNf_avg;
    nNf_min = (0 == nf->m_noise_min) ? -1 : nNf_min;
//    int nDcMax = nNf_max / nDbmPerDiv;
//    int nRsMax = nNf_max % nDbmPerDiv;
//    int nRsMin = nNf_min % nDbmPerDiv;
    int nDcAvg = nNf_avg / nDbmPerDiv;
    int nDcMin = nNf_min / nDbmPerDiv;
    int nRsAvg = nNf_avg % nDbmPerDiv;

    memset(sCh, 0x00, 32);
    memset(sNf, 0x00, 32);
    memset(sC1, 0x00, 16);
    memset(sC0, 0x00, 16);

    if (nNf_avg > 31) {
        strcpy(sC1, nDisplaySchema ? COLOR_41 : VT100_RESET);
        strcpy(sC0, (curr_ch % 2) ? COLOR_401 : COLOR_400);
    } else if (nNf_avg > 21) {
        strcpy(sC1, nDisplaySchema ? COLOR_31 : VT100_RESET);
        strcpy(sC0, (curr_ch % 2) ? COLOR_301 : COLOR_300);
    } else if (nNf_avg > 11) {
        strcpy(sC1, nDisplaySchema ? COLOR_21 : VT100_RESET);
        strcpy(sC0, (curr_ch % 2) ? COLOR_201 : COLOR_200);
    } else {
        strcpy(sC1, nDisplaySchema ? COLOR_11 : VT100_RESET);
        strcpy(sC0, (curr_ch % 2) ? COLOR_101 : COLOR_100);
    }
    if (active_ch == curr_ch) {
        switch (nDisplaySchema) {
            case NF_DISP_STYLE_OUTLINE:
            case NF_DISP_STYLE_WIDE_BAR:
                sprintf(sCh, VT100_RESET COLOR_CUR " %d" VT100_RESET, curr_ch);
                break;
            case NF_DISP_STYLE_THIN_BAR:
                sprintf(sCh, VT100_RESET COLOR_CUR "%d" VT100_RESET, curr_ch);
                break;
            default:;
        }
    } else {
        switch (nDisplaySchema) {
            case NF_DISP_STYLE_OUTLINE:
            case NF_DISP_STYLE_WIDE_BAR:
                sprintf(sCh, VT100_RESET " " VT100_INVERT "%d", curr_ch);
                break;
            case NF_DISP_STYLE_THIN_BAR:
                sprintf(sCh, "%s%d", (curr_ch % 2) ? VT100_RESET : VT100_INVERT,curr_ch);
                break;
            default:;
        }
    }
    strcat(nf_info->ch_string, sCh);
    if (NF_DISP_STYLE_OUTLINE == nDisplaySchema) {  //  _/^\_
        if (nLc > nDcAvg + 1) {
            int len = strlen(nf_info->nf_string[nLc]);
            nf_info->nf_string[nLc][len-1] = ' ';
            len = strlen(nf_info->nf_string[nLc-1]);
            nf_info->nf_string[nLc-1][len-1] = '\\';
        } else if (nLc < nDcAvg - 1) {
            int len = strlen(nf_info->nf_string[nLc]);
            nf_info->nf_string[nLc][len-1] = '/';
        }
    }
    for (i = 0; i < nMaxRows - 1; i++) {
        if (i < nDcAvg) {
            switch (nDisplaySchema) {
                case NF_DISP_STYLE_OUTLINE:
                    if (i > nLc) {
                        sprintf(sNf, VT100_RESET "%c%s  ", (i < nDcAvg - 1) ? '|' : '/', sC1);
                    } else if (i == nLc) {
                        sprintf(sNf, VT100_RESET "%c%s  ", (i < nDcAvg - 1) ? ' ' : '/', sC1);
                    } else sprintf(sNf, VT100_RESET " %s  ",sC1);
                    break;
                case NF_DISP_STYLE_WIDE_BAR:
                    if (i >= nDcMin || i >= nMaxRows - 1)
                        sprintf(sNf, VT100_RESET "%s| " VT100_RESET " ", sC1);
                    else sprintf(sNf, VT100_RESET "%s  "  VT100_RESET " ", sC1);
                    break;
                case NF_DISP_STYLE_THIN_BAR:
                    if (i >= nDcMin || i >= nMaxRows - 1)
                        sprintf(sNf, VT100_RESET "%s|" VT100_RESET " ", sC1);
                    else sprintf(sNf, VT100_RESET "%s " VT100_RESET " ", sC1);
                default:;
            }
        } else if (i == nDcAvg) {
            switch (nDisplaySchema) {
                case NF_DISP_STYLE_OUTLINE:
                case NF_DISP_STYLE_WIDE_BAR:
                    if (nf->m_noise_cur < -99) sprintf(sNf,VT100_RESET "%s   ",sC0);
                        else sprintf(sNf,VT100_RESET "%s%3d",sC0,nf->m_noise_cur);
                    break;
                case NF_DISP_STYLE_THIN_BAR:
                    if (nf->m_noise_cur < -99) sprintf(sNf,VT100_RESET "%s  ",sC0);
                        else sprintf(sNf,VT100_RESET "%s%2d",sC0,-nf->m_noise_cur);
                    break;
                default:;
            }
            nRsAvg = -1;
//        } else if (i == nDcMax) {
//            switch (nDisplaySchema) {
//                case NF_DISP_STYLE_OUTLINE:
//                case NF_DISP_STYLE_WIDE_BAR:    sprintf(sNf,VT100_RESET "%s__" VT100_RESET " ",sC0);    break;
//                case NF_DISP_STYLE_THIN_BAR:    sprintf(sNf,VT100_RESET "%s_" VT100_RESET " ",sC0);     break;
//                default:;
//            }
        } else {
            switch (nDisplaySchema) {
                case NF_DISP_STYLE_OUTLINE:
                case NF_DISP_STYLE_WIDE_BAR:    strcpy(sNf, VT100_RESET"   ");  break;
                case NF_DISP_STYLE_THIN_BAR:    strcpy(sNf, VT100_RESET"  ");   break;
                default:;
            }
        }
        strcat(nf_info->nf_string[i], sNf);
    }
    *last_DC = nDcAvg;
}

void PrintChannelInfo(int max_lines,NF_STRING* nf_info) {
    int i,j;
    printf(VT100_STYLE_TITLE VT100_INVERT "%s====== Channel Noise Floor =========" 
            VT100_RESET " %s%d ~ %d" VT100_RESET " %s%d ~ %d" VT100_RESET " %s%d ~ %d"
            VT100_RESET " %s >= %d dBm" VT100_RESET,
            VT100_GOTO_XY(START_NF_LINE, 1),
            COLOR_11    ,MIN_NOISE_FLOOR     ,MIN_NOISE_FLOOR + 10,
            COLOR_21    ,MIN_NOISE_FLOOR + 11,MIN_NOISE_FLOOR + 20,
            COLOR_31    ,MIN_NOISE_FLOOR + 21,MIN_NOISE_FLOOR + 30,
            COLOR_41    ,MIN_NOISE_FLOOR + 31);
    int Max_Divs = (max_lines == 1) ? NF_MAX_DIVS_1 : NF_MAX_DIVS_2;
    int Max_Rows = (max_lines == 1) ? NF_MAX_ROWS_1 : NF_MAX_ROWS_2;
    for (i = 0; i < max_lines; i ++) {
        for (j = 0; j < Max_Divs; j++) {
            printf(VT100_RESET "%s%s" VT100_RESET, VT100_GOTO_XY( START_NF_LINE + 
                Max_Rows * i + Max_Divs - j, 1), nf_info[i].nf_string[j]);
        }
        printf(VT100_RESET "%s%s" VT100_RESET, VT100_GOTO_XY( START_NF_LINE + 
                Max_Rows * (i +1), 1), nf_info[i].ch_string);
    }
    printf( VT100_STYLE_BLUE  "%s========================"
            "=======================================================" VT100_RESET,
            VT100_GOTO_XY(START_NF_LINE + max_lines * Max_Rows + 1, 1));
                    
}

void guiPrintNoiseFloor(pthread_mutex_t* hMutex, GWS_CHAN_NF nf_list[], int region, short nCurrChannel) {
    
    if (0 == pthread_mutex_trylock(hMutex)) {
        NF_STRING nf_info[2];
        int i = 0, j = 0, ch = 0;
        int last_nf = 0,max_lines = 2;
        switch (nDisplaySchema) {
            case NF_DISP_STYLE_OUTLINE:     max_lines = 2;  break;
            case NF_DISP_STYLE_WIDE_BAR:    max_lines = 2;  break;
            case NF_DISP_STYLE_THIN_BAR:    max_lines = 1;  break;
            default:;
        }
        for (i = 0; i < 2; i ++) {
            Fill_Nf_Header(region,max_lines,&nf_info[i]);
        }
        for (ch = MIN_CHANNEL(region); ch <= MAX_CHANNEL(region); ch++) {
            int ch_idx = ch - MIN_CHANNEL(region);
            i = ch_idx / NF_COL_PER_LINE(max_lines,region);

            Fill_Channel_Info(ch,nCurrChannel,&nf_list[ch_idx],&last_nf,max_lines,&nf_info[i]);
        }
        PrintChannelInfo(max_lines,nf_info);
        pthread_mutex_unlock(hMutex);
    }
}

#define GWS_VB_LOAD(index,type,address)     \
        if (NULL != (pVar = GetGwsVB(index)))     \
            VB_Load(&pVar->m_value,type,address);
//        else printf("\nGWS Varbind not found(index=%d)\n",index)

#define IW_VB_LOAD(index,type,address)     \
        if (NULL != (pVar = GetIwVB(index)))     \
            VB_Load(&pVar->m_value,type,address);
//        else printf("\nWifi Varbind not found(index=%d)\n",index)

void* guiThreadPrintData(void* pParam) {
    if (pParam) {
        pthread_mutex_t* hMutex = (pthread_mutex_t*) pParam;
        bool rf_updated = false;
        bool iw_updated = false;
        int nIdentifier = 0;
        int nTimer = 0;
        P_GWS_KPI pKpi = ConnectShm(&nIdentifier);

        if (NULL != pKpi) {
            VarBind* pVar = NULL;

            GWS_VB_LOAD(GWS_FIRMWARE, VAR_STRING, pKpi->m_radio.m_sFirmware);
            GWS_VB_LOAD(GWS_BOARDSNO, VAR_INTEGER, &pKpi->m_radio.m_nBoardSNO);
            GWS_VB_LOAD(GWS_REGION, VAR_INTEGER, &pKpi->m_radio.m_nRegion);
            GWS_VB_LOAD(GWS_CHANNO, VAR_INTEGER, &pKpi->m_radio.m_nChanNo);
            GWS_VB_LOAD(GWS_IFOUT, VAR_INTEGER, &pKpi->m_radio.m_nIFOUT);
            GWS_VB_LOAD(GWS_AGCMODE, VAR_INTEGER, &pKpi->m_radio.m_nAGCMode);
            GWS_VB_LOAD(GWS_TEMP, VAR_INTEGER, &pKpi->m_radio.m_nTemp);
            GWS_VB_LOAD(GWS_TXCAL, VAR_BOOL, &pKpi->m_radio.m_bTXCal);
            GWS_VB_LOAD(GWS_RXCAL, VAR_BOOL, &pKpi->m_radio.m_bRXCal);
            GWS_VB_LOAD(GWS_TX, VAR_BOOL, &pKpi->m_radio.m_bTX);
            GWS_VB_LOAD(GWS_RX, VAR_BOOL, &pKpi->m_radio.m_bRX);
            GWS_VB_LOAD(GWS_CURTXPWR, VAR_INTEGER, &pKpi->m_radio.m_nCurTxPwr);
            GWS_VB_LOAD(GWS_RXGAIN, VAR_INTEGER, &pKpi->m_radio.m_nRXGain);
            GWS_VB_LOAD(GWS_BRDMAXPWR, VAR_INTEGER, &pKpi->m_radio.m_nBrdMaxPwr);
            GWS_VB_LOAD(GWS_RXMAXGAIN, VAR_INTEGER, &pKpi->m_radio.m_nRxMaxGain);
            GWS_VB_LOAD(GWS_TXATTEN, VAR_INTEGER, &pKpi->m_radio.m_nTxAtten);
            GWS_VB_LOAD(GWS_RXFATTEN, VAR_INTEGER, &pKpi->m_radio.m_nRxFAtten);
            GWS_VB_LOAD(GWS_MAXATTEN, VAR_INTEGER, &pKpi->m_radio.m_nMaxAtten);
            GWS_VB_LOAD(GWS_RXRATTEN, VAR_INTEGER, &pKpi->m_radio.m_nRxRAtten);
            GWS_VB_LOAD(GWS_MAXTXPWR, VAR_INTEGER, &pKpi->m_radio.m_nMaxTxPwr);
            GWS_VB_LOAD(GWS_MINTXPWR, VAR_INTEGER, &pKpi->m_radio.m_nMinTxPwr);

            IW_VB_LOAD(WIFI_SSID, VAR_STRING, pKpi->m_wifi.m_sSSID);
            IW_VB_LOAD(WIFI_MODE, VAR_STRING, pKpi->m_wifi.m_sMode);
            IW_VB_LOAD(WIFI_CHANNEL, VAR_INTEGER, &pKpi->m_wifi.m_nChannel);
            IW_VB_LOAD(WIFI_BSSID, VAR_STRING, pKpi->m_wifi.m_sBSSID);
            IW_VB_LOAD(WIFI_ENCRYPTION, VAR_STRING, pKpi->m_wifi.m_sEncryption);
            IW_VB_LOAD(WIFI_TXPOWER, VAR_INTEGER, &pKpi->m_wifi.m_nTxPower);
            IW_VB_LOAD(WIFI_HARDWARE_NAME, VAR_STRING, pKpi->m_wifi.m_sHardwareName);
            IW_VB_LOAD(WIFI_HWMODELIST, VAR_STRING, pKpi->m_wifi.m_sHwModeList);
            IW_VB_LOAD(WIFI_QUALITY, VAR_STRING, pKpi->m_wifi.m_sQuality);
            IW_VB_LOAD(WIFI_SIGNAL, VAR_INTEGER, &pKpi->m_wifi.m_nSignal);
            IW_VB_LOAD(WIFI_NOISE, VAR_INTEGER, &pKpi->m_wifi.m_nNoise);
            IW_VB_LOAD(WIFI_BITRATE, VAR_INTEGER, &pKpi->m_wifi.m_nBitRate);
            IW_VB_LOAD(WIFI_ASSOCLIST, VAR_LIST, &pKpi->m_assoc_list);

            for ( ; true; usleep(100000)) { //sleep 50 ms
                if ((nTimer++) > 20) {
                    nTimer = 0;
                    rf_updated = false;
                    iw_updated = false;
                } else if ((nTimer % 2) == 0) {
                    GWS_CHAN_NF* noise = NULL;
                    int region = 0, channel = 0;
                    bool art_occupied = false;
                    RW_KPI_VAR(nIdentifier,noise,pKpi->m_noise_floor);
                    RW_KPI_VAR(nIdentifier,region,pKpi->m_radio.m_nRegion);
                    RW_KPI_VAR(nIdentifier,channel,pKpi->m_currchannel);
                    switch (nCurrentPage) {
                        case PAGE_0:
                            guiPrintRfInfo(&rf_updated, hMutex);
                            guiPrintIwInfo(&iw_updated, hMutex);
                        break;
                        case PAGE_1:
                            if (noise)
                                guiPrintNoiseFloor(hMutex, noise, region, channel);
                        break;
                        case PAGE_2:
                            if (noise && (nTimer % 10) == 0)
                                guiPrintChannelMapping(hMutex, noise, region, channel);
                        break;
                        case PAGE_3:
                            art_occupied = art2_guiPrintMenu(hMutex);
                        break;
                        default:;
                    }
                    if (!art_occupied) guiShowCommandLine(hMutex, false);
                }
            }
            CloseShm(pKpi);
        } else return NULL;
    }
    return NULL;
}

bool IsYes(char* str) {
    if (NULL == str) return false;
    if (strlen(str) <= 0) return false;
    if (str[0] == 'y' || str[0] == 'Y') return true;
    return false;
}

bool IsNumber(char* str) {
    int i = 0;
    if (NULL == str) return false;
    if (strlen(str) <= 0) return false;
    for (i = 0; str[i]; i ++) {
        if (!isdigit(str[i]) && str[i] != '-' && str[i] != '.' && str[i] != '\n') {
            ShowStatusBar("%s is not a number...",str);
            return false;
        }
    }
    return true;
}

static bool CommandGoGws(int currCommand, char* parameter, int nQid, bool needConfirm,pthread_mutex_t* hMutex) {
    GWS_MESSAGE msg;
    int nSend = 0;
    char prompt[128];

    memset(&msg, 0x00, sizeof (GWS_MESSAGE));
    memset(prompt, 0x00, 128);
    switch (currCommand) {
        case STOP_CHAN:
            if (needConfirm) {
                if (IsYes(guiShowCommandLine(hMutex,true))) {
                    if (SCAN_CHAN == currCommand || SCAN_FIXED_CHAN == currCommand) {
                        ShowStatusBar("Noise floor scanning...");
                    } else if (STOP_CHAN == currCommand) {
                        ShowStatusBar("Stop noise scanning.");
                    }
                } else {
                    ShowStatusBar("Command cancelled");
                    break;
                }
            }
            msg.m_nType = 1;
            msg.m_command.m_nReq = currCommand;
            strcpy(msg.m_command.m_sPara, parameter);
            if (msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0) < 0) {
                ShowStatusBar("Error on Send command(%d)", errno);
            }
            break;
        case SCAN_CHAN:
        case SCAN_FIXED_CHAN:
            if (needConfirm) {
                if (IsYes(guiShowCommandLine(hMutex,true))) {
                    guiPromptGet(hMutex,prompt);
                    ShowStatusBar("%s", prompt);
                } else {
                    ShowStatusBar("Command cancelled");
                    break;
                }
            }
            nCurrentPage = PAGE_1;
            msg.m_nType = 1;
            msg.m_command.m_nReq = currCommand;
            strcpy(msg.m_command.m_sPara, parameter);
            if (msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0) < 0) {
                ShowStatusBar("Error on Send command(%d)", errno);
            }
            break;
        case SET_TXON:
        case SET_RXON:
        case SET_TXCAL:
        case SET_RXCAL:
        case SET_REGION:
            if (0 != parameter[0]) {
                if (needConfirm) {
                    if (IsYes(guiShowCommandLine(hMutex,true))) {
                        guiPromptGet(hMutex,prompt);
                        ShowStatusBar("%s", prompt);
                    } else {
                        ShowStatusBar("Command cancelled");
                        break;
                    }
                }
                msg.m_nType = 1;
                msg.m_command.m_nReq = currCommand;
                strcpy(msg.m_command.m_sPara, parameter);
                if (msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0) < 0) {
                    ShowStatusBar("Error on Send command(%d)", errno);
                }
            }
            break;
        case WIFI_TXPW:
            if (needConfirm) {
                char* cmdline = guiShowCommandLine(hMutex,true);
                if (cmdline) {
                    strcpy(parameter, cmdline);
                } else break;
            }
            if (IsNumber(parameter)) {
                int num = atoi(parameter);
                if ((currCommand == WIFI_TXPW && 0 < num && num <= 23)) {
                    guiPromptGet(hMutex,prompt);
                    sprintf(prompt, "%s%s", prompt, parameter);
                    msg.m_nType = 1;
                    msg.m_command.m_nReq = currCommand;
                    sprintf(msg.m_command.m_sPara, "%d", num * 100);
                    if (nSend = msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0) < 0) {
                        ShowStatusBar("Error on send message,error = %d", errno);
                    } else ShowStatusBar("%s", prompt);
                } else {
                    ShowStatusBar("WiFi txpower,should be 0 ~ 23.");
                    break;
                }
            } else {
                ShowStatusBar("Need parameter!");
            }
            break;
        case SET_BANDWIDTH:
            if (needConfirm) {
                char* cmdline = guiShowCommandLine(hMutex,true);
                if (cmdline) {
                    strcpy(parameter, cmdline);
                } else break;
            }
            if (IsNumber(parameter)) {
                int num = atoi(parameter);
                if (currCommand == SET_BANDWIDTH && (1 <= num && num <= 8) ||
                        num == 10 || num == 12 || num == 14 || num == 16 ||
                        num == 20 || num == 24 || num == 28 || num == 32) {
                    guiPromptGet(hMutex,prompt);
                    sprintf(prompt, "%s%s", prompt, parameter);
                    msg.m_nType = 1;
                    msg.m_command.m_nReq = currCommand;
                    sprintf(msg.m_command.m_sPara, "%d", num);
                    if (nSend = msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0) < 0) {
                        ShowStatusBar("Error on send message,error = %d", errno);
                    } else ShowStatusBar("%s", prompt);
                } else {
                    ShowStatusBar("Supported bandwidth : 1-8, 10,12,14,16,20,24,28,32 MHz.");
                    break;
                }
            } else {
                ShowStatusBar("Need parameter!");
            }
            break;
        case SET_CHAN:
        case SET_TXPWR:
        case SET_GAIN:
        case SET_TXATT:
            if (needConfirm) {
                char* cmdline = guiShowCommandLine(hMutex,true);
                if (cmdline) {
                    strcpy(parameter, cmdline);
                } else break;
            }
            guiPromptGet(hMutex,prompt);
            if (IsNumber(parameter)) {
                sprintf(prompt, "%s%s", prompt, parameter);
                int num = atoi(parameter);
                if ((currCommand == SET_CHAN && 14 <= num && num <= 60) || //channel (14-60)
                        (currCommand == SET_TXATT && 0 <= num && num <= 51)) { //txatten (0-25.5dB)
                    msg.m_nType = 1;
                    msg.m_command.m_nReq = currCommand;
                    strcpy(msg.m_command.m_sPara, parameter);
                    msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0);
                    ShowStatusBar("%s", prompt);
                } else if ((currCommand == SET_TXPWR && 0 <= num && num <= 33) || //txpower (0-23dB)
                        (currCommand == SET_GAIN && 0 <= num && num <= 26)) { //rxgain (0-25.5dB)
                    sprintf(parameter, "%d", num * 2);
                    msg.m_nType = 1;
                    msg.m_command.m_nReq = currCommand;
                    strcpy(msg.m_command.m_sPara, parameter);
                    msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0);
                    ShowStatusBar("%s", prompt);
                } else {
                    switch (currCommand) {
                        case SET_CHAN: guiPromptSet(hMutex,"Channel should be 14 ~ 51 (region 0) or 21 ~ 60 (region 1.");
                            break;
                        case SET_TXPWR: guiPromptSet(hMutex,"Txpower should be 0 ~ 33.");
                            break;
                        case SET_GAIN: guiPromptSet(hMutex,"Rx gain should be 0 ~ 26.");
                            break;
                        case SET_TXATT: guiPromptSet(hMutex,"Rx atten should be 0~ 51.");
                            break;
                        default: ShowStatusBar("Command Unknown...");
                    }
                    break;
                }
            } else if (currCommand == SET_CHAN && (parameter[0] == 'x' || parameter[0] == 'v')) {
                msg.m_nType = 1;
                msg.m_command.m_nReq = currCommand;
                strcpy(msg.m_command.m_sPara, parameter);
                msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0);
                ShowStatusBar("%s", prompt);
            } else {
                ShowStatusBar("Need parameter!");
            }
            break;
        case SET_MODE:
            if (needConfirm) {
                char* cmdline = guiShowCommandLine(hMutex,true);
                if (cmdline) {
                    strcpy(parameter, cmdline);
                } else break;
            }
            if (0 != parameter[0]) {
                if (strcasecmp(parameter, "ap") == 0) {
                    strcpy(msg.m_command.m_sPara, "car");
                } else if (strcasecmp(parameter, "sta") == 0) {
                    strcpy(msg.m_command.m_sPara, "ear");
                } else if (strcasecmp(parameter, "mesh") == 0) {
                    strcpy(msg.m_command.m_sPara, "mesh");
                } else {
                    msg.m_command.m_sPara[0] = 0;
                }
                if (msg.m_command.m_sPara[0]) {
                    msg.m_nType = 1;
                    msg.m_command.m_nReq = currCommand;
                    if (nSend = msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0) < 0) {
                        ShowStatusBar("Error on send message,error = %d", errno);
                    } else {
                        ShowStatusBar("Set Mode to : %s, Rebooting...",parameter);
                        msg.m_nType = 1; //Notify thread Handler to stop
                        msg.m_command.m_nReq = 'q';
                        msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0);
                        return false;
                    }
                } else {
                    ShowStatusBar("Only AP, STA or Mesh mode are supported currently.");
                    break;
                }
            } else {
                ShowStatusBar("Need parameter!");
            }
            break;
        case ART2_LOAD:
            if (needConfirm) {
                if (IsYes(guiShowCommandLine(hMutex,true))) {
                    if (art2_GetMode() == ART_MODE_UNLOAD) {    //load driver
                        art2_Load(true);
                        nCurrentPage = PAGE_3;
                    } else {    //remove driver
                        art2_Load(false);
                        nCurrentPage = PAGE_0;
                    }
                    printf(VT100_CLEAR);
                } else {
                    ShowStatusBar("Command cancelled");
                    break;
                }
            }
            break;
        case CMD_QUIT: case CMD_QUIT_:
            art2_Load(false);
            msg.m_nType = 1; //Notify thread Handler to stop
            msg.m_command.m_nReq = 'q';
            msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0);
            return false;
        default:;
    }
    guiPromptSet(hMutex,NULL);
    return true;
}

static bool CommandGoArt(int currCommand, char* parameter, int nQid, bool needConfirm,pthread_mutex_t* hMutex) {
    GWS_MESSAGE msg;
    int nSend = 0;
    char prompt[128];

    memset(&msg, 0x00, sizeof (GWS_MESSAGE));
    memset(prompt, 0x00, 128);
    switch (currCommand) {
        case ART2_LOAD:
            if (needConfirm) {
                if (IsYes(guiShowCommandLine(hMutex,true))) {
                    if (art2_GetMode() == ART_MODE_UNLOAD) {    //load driver
                        art2_Load(true);
                        nCurrentPage = PAGE_3;
                    } else {    //remove driver
                        art2_Load(false);
                        nCurrentPage = PAGE_0;
                    }
                    printf(VT100_CLEAR);
                } else {
                    ShowStatusBar("Command cancelled");
                    break;
                }
            }
            break;
        case ART2_STANDBY:  art2_SetMode(ART_MODE_STANDBY); break;
        case ART2_TX:       art2_SetMode(ART_MODE_TX);      break;
        case ART2_RX:       art2_SetMode(ART_MODE_RX);      break;
        case ART2_TX100:
        case ART2_TX_SGI:   if (ART2_TX100 == currCommand) art2_SetTx99();
                            else if (ART2_TX_SGI == currCommand) art2_SetSGI();
            break;
        case ART2_TX_MASK:
        case ART2_RX_MASK:
        case ART2_FREQ:
        case ART2_TX_BW:
        case ART2_TX_PWR:
        case ART2_TX_GAIN:
            if (needConfirm) {
                char* cmdline = guiShowCommandLine(hMutex,true);
                if (cmdline) {
                    strcpy(parameter, cmdline);
                } else break;
            }
            if (IsNumber(parameter)) {
                int num = atoi(parameter);
                guiPromptGet(hMutex,prompt);
                sprintf(prompt, "%s%s", prompt, parameter);
                if (currCommand == ART2_TX_MASK) {
                    if (0x01 <= num && num <= 0x03) {
                        art2_SetTxMask(num);
                    } else ShowStatusBar("Tx mask should be 1,2,3.");
                } else if (currCommand == ART2_RX_MASK) {
                    if (0x01 <= num && num <= 0x03) {
                        art2_SetRxMask(num);
                    } else ShowStatusBar("Rx mask should be 1,2,3.");
                } else if (currCommand == ART2_FREQ) {
                    if (!art2_SetFrequency(num))
                        ShowStatusBar("Invalidate frequency %d!",num);
                } else if (currCommand == ART2_TX_PWR) {
                    double txp = atof(parameter);
                    if (-100 <= txp && txp <= 31.5) {
                        art2_SetTxPower(txp);
                    } else ShowStatusBar("Tx power should be -100 ~ 31.5");
                } else if (currCommand == ART2_TX_GAIN) {
                    if (-1 <= num && num <= 100) {
                        art2_SetTxGain(num);
                    } else ShowStatusBar("Tx gain should be 0 ~ 100 (-1 means use TxPower instead of TxGain)");
                } else if (currCommand == ART2_TX_BW) {
                    if (!art2_SetBandWdith(num))
                        ShowStatusBar("Bandwidth should be 3,4,5,6,7,8,10,12,14,16,20,24,28,32");
                }
            }
            break;
        case ART2_TX_RATE:
            if (needConfirm) {
                char* cmdline = guiShowCommandLine(hMutex,true);
                if (cmdline) {
                    strcpy(parameter, cmdline);
                } else break;
            }
            if (art2_CheckRate(parameter)) {
                art2_SetRate(parameter);
            } else ShowStatusBar("Invalidate Rate %s",parameter);
            break;
        case CMD_QUIT: case CMD_QUIT_:
            art2_Load(false);
            msg.m_nType = 1; //Notify thread Handler to stop
            msg.m_command.m_nReq = 'q';
            msgsnd(nQid, &msg, sizeof (GWS_COMMAND), 0);
            return false;
        default:;
    }
    guiPromptSet(hMutex,NULL);
    return true;
}

bool CommandGo(int currCommand, char* parameter, int nQid, bool needConfirm,pthread_mutex_t* hMutex) {
    return (art2_GetMode() == ART_MODE_UNLOAD) ?
        CommandGoGws(currCommand, parameter, nQid, needConfirm, hMutex) :
        CommandGoArt(currCommand, parameter, nQid, needConfirm, hMutex);
}

int HandleMenuKeyGws(int nKey, char* command, pthread_mutex_t* hMutex) { //return false to exit Application
    VarBind* vb = NULL;
    switch ((char) nKey) {
        case SCAN_CHAN:
        case SCAN_FIXED_CHAN:
            guiPromptSet(hMutex,"Start noise scan(y/n)?");
            bReadyGo = true;
            return (char) nKey;
        case STOP_CHAN:
            ShowStatusBar("RF transmission will stop!");
            guiPromptSet(hMutex,"Stop noise scan(y/n)?");
            bReadyGo = true;
            return (char) nKey;
        case NEXT_PAGE:
            nCurrentPage ++;
            nCurrentPage %= (art2_GetMode() == ART_MODE_UNLOAD ? PAGE_3 : PAGE_MAX);
            printf(VT100_CLEAR);
            bReadyGo = true;
            return (char) nKey;
        case DISP_STYLE:
            nDisplaySchema ++;
            nDisplaySchema %= NF_DISP_STYLE_MAX;
            printf(VT100_CLEAR);
            bReadyGo = true;
            return (char) nKey;
        case SET_CHAN:
            if (vb = GetGwsVB(GWS_CHANNO)) {
                guiPromptSet(hMutex,"set GWS channel = ");
                bReadyGo = true;
                return (char) nKey;
            }
            break;
        case SET_TXPWR:
            if (vb = GetGwsVB(GWS_CURTXPWR)) {
                guiPromptSet(hMutex,"GWS Tx power = ");
                bReadyGo = true;
                return (char) nKey;
            }
            break;
        case SET_GAIN:
            if (vb = GetGwsVB(GWS_RXGAIN)) {
                guiPromptSet(hMutex,"GWS Rx gain = ");
                bReadyGo = true;
                return (char) nKey;
            }
            break;
        case SET_TXATT:
            if (vb = GetGwsVB(GWS_RXGAIN)) {
                guiPromptSet(hMutex,"GWS Tx atten = ");
                bReadyGo = true;
                return (char) nKey;
            }
            break;
        case SET_TXON:
            if (vb = GetGwsVB(GWS_TX)) {
                if (VB_AsInt(&vb->m_value) == 1) {
                    guiPromptSet(hMutex,"GWS Tx OFF (y/n)?");
                    strcpy(command, "off");
                } else {
                    guiPromptSet(hMutex,"GWS Tx ON (y/n)?");
                    strcpy(command, "on");
                }
                bReadyGo = true;
                return SET_TXON;
            }
            break;
        case SET_RXON:
            if (vb = GetGwsVB(GWS_RX)) {
                if (VB_AsInt(&vb->m_value) == 1) {
                    guiPromptSet(hMutex,"GWS RxOFF (y/n)?");
                    strcpy(command, "off");
                } else {
                    guiPromptSet(hMutex,"GWS Rx ON (y/n)?");
                    strcpy(command, "on");
                }
                bReadyGo = true;
                return SET_RXON;
            }
            break;
        case SET_TXCAL:
            if (vb = GetGwsVB(GWS_TXCAL)) {
                if (VB_AsInt(&vb->m_value) == 1) {
                    guiPromptSet(hMutex,"GWS Tx Cal OFF (y/n)?");
                    strcpy(command, "0");
                } else {
                    guiPromptSet(hMutex,"GWS Tx Cal ON (y/n)?");
                    strcpy(command, "1");
                }
                bReadyGo = true;
                return SET_TXCAL;
            }
            break;
        case SET_RXCAL:
            if (vb = GetGwsVB(GWS_RXCAL)) {
                if (VB_AsInt(&vb->m_value) == 1) {
                    guiPromptSet(hMutex,"GWS Rx Cal OFF?");
                    strcpy(command, "0");
                } else {
                    guiPromptSet(hMutex,"GWS Rx Cal ON?");
                    strcpy(command, "1");
                }
                bReadyGo = true;
                return SET_RXCAL;
            }
            break;
        case SET_REGION:
            if (vb = GetGwsVB(GWS_REGION)) {
                if (VB_AsInt(&vb->m_value) == 1) {
                    guiPromptSet(hMutex,"Set to region 0(14-51) ? (y/n)");
                    strcpy(command, "0");
                } else {
                    guiPromptSet(hMutex,"Set to region 1(21-60) ? (y/n)");
                    strcpy(command, "1");
                }
                bReadyGo = true;
                return SET_REGION;
            }
            break;
        case WIFI_TXPW:
            if (vb = GetIwVB(WIFI_TXPOWER)) {
                if (strcmp(vb->m_attri,DISP_RW) == 0) {
                    guiPromptSet(hMutex,"WiFi Tx Power(3-23) = ");
                    bReadyGo = true;
                    return WIFI_TXPW;
                }
            }
            break;
        case SET_BANDWIDTH:
            if (vb = GetIwVB(WIFI_CHANNEL)) {
                if (strcmp(vb->m_attri,DISP_RW) == 0) {
                    guiPromptSet(hMutex,"Channel bandwidth(1-40) = ");
                    bReadyGo = true;
                    return SET_BANDWIDTH;
                }
            }
            break;
        case SET_MODE:      guiPromptSet(hMutex,"Set mode to :");                   bReadyGo = true;    return (char) nKey;
        case ART2_LOAD:
            if (art2_GetMode() == ART_MODE_UNLOAD) guiPromptSet(hMutex,"Load ART2 driver (y/n)?");
            else guiPromptSet(hMutex,"Remove ART2 driver (y/n)?");
            bReadyGo = true;
            return (char) nKey;
        case CMD_QUIT: case CMD_QUIT_:  bReadyGo = true;    return CMD_QUIT_;
        default:; //do nothing
    }
    return nKey;
}

int HandleMenuKeyArt(int nKey, char* command, pthread_mutex_t* hMutex) { //return false to exit Application
#define SET_ART_PROMPT(art_prompt)  \
    do {\
        if (art2_GetMode() > ART_MODE_UNLOAD) {\
            guiPromptSet(hMutex,art_prompt);\
            bReadyGo = true;\
            return (char) nKey;\
        }\
    } while (0)
//define SET_ART_PROMPT

    VarBind* vb = NULL;
    switch (art2_move_menu_cursor((char) nKey)) {
        case ART2_TX:       SET_ART_PROMPT("Start ART2 Tx");                    break;
        case ART2_RX:       SET_ART_PROMPT("Start ART2 Rx");                    break;
        case ART2_STANDBY:  SET_ART_PROMPT("Press 0 to stop Tx/Rx");            break;
        case ART2_TX_MASK:  SET_ART_PROMPT("Set ART2 Tx/Rx mask (1,2,3) :");    break;
        case ART2_RX_MASK:  SET_ART_PROMPT("Set ART2 Tx/Rx mask (1,2,3) :");    break;
        case ART2_FREQ:     SET_ART_PROMPT("Set ART2 Tx frequency :");          break;
        case ART2_TX_BW:    SET_ART_PROMPT("Set ART2 Band width :");            break;
        case ART2_TX_PWR:   SET_ART_PROMPT("Set ART2 Tx Power :");              break;
        case ART2_TX_RATE:  SET_ART_PROMPT("Set ART2 Tx Rate :");               break;
        case ART2_TX_GAIN:  SET_ART_PROMPT("Set ART2 Tx Gain :");               break;
        case ART2_LOAD:
            if (art2_GetMode() == ART_MODE_UNLOAD) guiPromptSet(hMutex,"Load ART2 driver (y/n)?");
            else guiPromptSet(hMutex,"Remove ART2 driver (y/n)?");
            bReadyGo = true;
            return (char) nKey;
        case ART2_TX100:
            if (art2_GetMode() > ART_MODE_UNLOAD) {
                if (art2_GetTx99()) guiPromptSet(hMutex,"Switch to ART2 Tx 100 mode(y/n)?");
                    else guiPromptSet(hMutex,"Switch to ART2 Tx 99 mode(y/n)?");
                bReadyGo = true;
                return (char) nKey;
            }
            break;
        case ART2_TX_SGI:
            if (art2_GetMode() > ART_MODE_UNLOAD) {
                if (art2_GetSGI()) guiPromptSet(hMutex,"Turn OFF ART2 Short GI(y/n)?");
                    else guiPromptSet(hMutex,"Turn ON ART2 Short GI(y/n)?");
                bReadyGo = true;
                return (char) nKey;
            }
            break;
        case CMD_QUIT: case CMD_QUIT_:  bReadyGo = true;    return CMD_QUIT_;
        default:; //do nothing
    }
    return nKey;
}

int HandleMenuKey(int nKey, char* command, pthread_mutex_t* hMutex) { //return false to exit Application
    return (art2_GetMode() == ART_MODE_UNLOAD) ?
        HandleMenuKeyGws(nKey, command, hMutex) :
        HandleMenuKeyArt(nKey, command, hMutex);
}

void ReadCmdLineAction(int argc, char* argv[], int* action, char* parameter) {
    int i = 0;
    *action = 0;
    for (i = 1; i < argc; i++) {
        char* szArgv = argv[i];
        if (!szArgv) continue;
        if (strstr(szArgv, CMD_SERVER_DOWN) != NULL) {
            *action = SHUTDOWN;
            return;
        } else if (strstr(szArgv, CMD_RADIO_TX_CAL) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_TXCAL;
                if (strstr(szArgv, "on") != NULL) strcpy(parameter, "1");
                else strcpy(parameter, "0");
            }
            return;
        } else if (strstr(szArgv, CMD_RADIO_RX_CAL) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_RXCAL;
                if (strstr(szArgv, "on") != NULL) strcpy(parameter, "1");
                else strcpy(parameter, "0");
            }
            return;
        } else if (strstr(szArgv, CMD_RADIO_CHANNEL) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_CHAN;
                strcpy(parameter, szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_RADIO_POWER) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_TXPWR;
                strcpy(parameter, szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_RADIO_GAIN) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_GAIN;
                strcpy(parameter, szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_RADIO_ATTEN) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_TXATT;
                strcpy(parameter, szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_WIFI_BANDWIDTH) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_BANDWIDTH;
                strcpy(parameter, szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_WIFI_POWER) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = WIFI_TXPW;
                strcpy(parameter, szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_SET_MESHID) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_MESH_ID;
                dump_mesh_id(true,szArgv);
                sprintf(parameter, "uci set wireless.@wifi-iface[0].mesh_id=%s",szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_GET_MESHID) != NULL) {
            *action = GET_MESH_ID;
            strcpy(parameter, "uci get wireless.@wifi-iface[0].mesh_id");
            return;
        } else if (strstr(szArgv, CMD_SET_SSID) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_WDS_ID;
                dump_mesh_id(false,szArgv);
                sprintf(parameter, "uci set wireless.@wifi-iface[0].ssid=%s",szArgv);
            }
            return;
        } else if (strstr(szArgv, CMD_GET_SSID) != NULL) {
            *action = GET_WDS_ID;
            strcpy(parameter, "uci get wireless.@wifi-iface[0].ssid");
            return;
        } else if (strstr(szArgv, CMD_SET_KEY) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_ENCRYPTION;
                if (strcmp(szArgv,"none") != 0) {
                    sprintf(parameter, "uci set wireless.@wifi-iface[0].key=%s",szArgv);
                }
            }
            return;
        } else if (strstr(szArgv, CMD_SET_STA_NO) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_STATION_NO;
                strcpy(parameter, szArgv);
            }
        } else if (strstr(szArgv, CMD_GET_STA_NO) != NULL) {
            *action = GET_STATION_NO;
            sprintf(parameter,"%d",get_mesh_station_no());
            return;
        } else if (strstr(szArgv, CMD_GET_LOCAL_IP) != NULL) {
            char*   ifname = "br-lan";

            *action = GET_IP_ADDRESS;
            if (argv[i + 1]) {
                ifname = argv[i + 1];
            }
            DWORD   nIP = Socket_GetIP(ifname);
            sprintf(parameter,"IP address of interface(%s) is : %s\n",ifname,IPconvertN2A(nIP));
            return;
        } else if (strstr(szArgv, CMD_RADIO_TX) != NULL) {
            if (argv[i + 1]) {
                printf("\nWYY:  TX ON/OFF\n");
                szArgv = argv[i + 1];
                *action = SET_TXON;
                if (strstr(szArgv, "on") != NULL) strcpy(parameter, "on");
                else strcpy(parameter, "off");
            }
            return;
        } else if (strstr(szArgv, CMD_RADIO_RX) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SET_RXON;
                if (strstr(szArgv, "on") != NULL) strcpy(parameter, "on");
                else strcpy(parameter, "off");
            }
            return;
        } else if (strstr(szArgv, CMD_CHAN_SCAN) != NULL) {
            *action = SCAN_CHAN;
            return;
        } else if (strstr(szArgv, CMD_STOP_SCAN) != NULL) {
            *action = STOP_CHAN;
            return;
        } else if (strstr(szArgv, CMD_FIXED_SCAN) != NULL) {
            if (argv[i + 1]) {
                szArgv = argv[i + 1];
                *action = SCAN_FIXED_CHAN;
                strcpy(parameter, szArgv);
            }
            return;
        } else return;
    }
}

void ReadCmdLineMode(int argc, char* argv[], int* mode, bool* expert) {
    int i = 0;

    *expert = false;
    for (i = 1; i < argc; i++) {
        char* szArgv = argv[i];
        if (strstr(szArgv, "-h") != NULL) {
            PrintHelp();
            return;
        } else if (strstr(szArgv, "server") != NULL) {
            *mode = MODE_SERVER;
            for (i++; i < argc; i++) {
                szArgv = argv[i];
                if (strstr(szArgv, "debug") != NULL) {
                    *mode = MODE_SVR_DEBUG;
                    return;
                }
            }
            return;
        } else if (strstr(szArgv, "gui") != NULL) {
            *mode = MODE_GUI;
            return;
        } else if (strstr(szArgv, "expert") != NULL) {
            *mode = MODE_GUI;
            *expert = true;
            return;
        } else {
            *mode = MODE_COMMAND_LINE;
        }
    }
}

void ReadExpertConf(bool* expert) {
    char sParamValue[256];
    memset(sParamValue, 0x00, 256);
    if (0 == read_cfg(CONFIG_FILE, "OperationMode", sParamValue)) {
        if (0 == strcmp("expert", sParamValue)) *expert = true;
    }
}

int ActingAsGui(int argc, char* argv[], bool expert) {
    CKeyboard keyb;
    pthread_t printer = 0;
    bool    isExpert = expert;

    if (!isExpert) ReadExpertConf(&isExpert);
    InitParameters(isExpert);
    art2_init(argc, argv);
    if (KeyOpen(&keyb, false)) {
        pthread_mutex_t hHoldScreen;
        char command[32];
        char cKeys[32];
        int nKeys = 0;
        bool bRun = true;
        int nTimer = 0;
        int currCommand = 0;
        int nQid = OpenMessageQueue(0);

        if (nQid < 0) {
            ShowStatusBar("GUI mode Error on open queue(%d),error = %d\n", nQid, errno);
            KeyClose(&keyb);
            return 0;
        }
        memset(cKeys, 0x00, 32);
        memset(command, 0x00, 32);
        printf(VT100_GOTO("2", "1") VT100_SAVE_CURSOR VT100_CLEAR);
        pthread_mutex_init(&hHoldScreen, NULL);
        pthread_create(&printer, NULL, guiThreadPrintData, &hHoldScreen);
        usleep(100000);

        while (bRun) {
            if (bReadyGo) {
                KeySetEcho(&keyb, true);
                bRun = CommandGo(currCommand, command, nQid, true, &hHoldScreen);
                bReadyGo = false;
                KeySetEcho(&keyb, false);
            } else if ((nKeys = KeyGetKeyBuff(&keyb, cKeys, 1)) > 0) { //Key triger mode
//                switch (nKeys) {
//                    case 1: ShowStatusBar("(%d)%c [%02x]", nKeys,cKeys[0],cKeys[0]);          break;
//                    case 2: ShowStatusBar("(%d)%c %c [%02x-%02x]", nKeys,cKeys[0],cKeys[1]);  break;
//                    default:;
//                }
                currCommand = HandleMenuKey(cKeys[0], command, &hHoldScreen);
            } else {
                usleep(100000);
            }
        }
        ShowStatusBar("%s", "Quit ...");
        pthread_cancel(printer);
        pthread_join(printer, NULL);
        pthread_mutex_unlock(&hHoldScreen);
        pthread_mutex_destroy(&hHoldScreen);
        KeyClose(&keyb);
    } else {
        ShowStatusBar("%s", "Can not open radio COM port.");
        return 0;
    }
    return 1;
}

int ActingAsCmdLine(int argc, char* argv[]) {
    char parameter[128];
    int nQid = -1;
    int action = 0;

    memset(parameter, 0x00, 128);
    ReadCmdLineAction(argc, argv, &action, parameter);

    //    if (nQid < 0) {
    //        printf("\nCan not connect to gwsman server. Please run gwsman -server& first.\n");
    //    }
    switch (action) {
        case SCAN_CHAN:
        case SCAN_FIXED_CHAN:
        case STOP_CHAN:
        case SET_CHAN:
        case SET_TXCAL:
        case SET_RXCAL:
        case SET_REGION:
        case SET_TXON:
        case SET_RXON:
        case SET_TXPWR:
        case SET_GAIN:
        case SET_TXATT:
        case WIFI_TXPW:
            if ((nQid = OpenMessageQueue(0)) >= 0) {
                pthread_mutex_t hMutex;
                pthread_mutex_init(&hMutex, NULL);
                printf("\n\tCommand=%c Parameter=%s\n", action, parameter);
                CommandGo(action, parameter, nQid, false, &hMutex);
                pthread_mutex_unlock(&hMutex);
                pthread_mutex_destroy(&hMutex);
            } else goto Error_Message;
            break;
        case SET_BANDWIDTH:
            if (strlen(parameter) > 0)
                set_wifi_bandwidth(parameter);
            break;
        case SET_WDS_ID:
            if (strlen(parameter) > 40 && strstr(parameter,"uci set wireless") > 0) {
                system(parameter);
                system("uci commit wireless");
                system("wifi");
            } else {
                printf("\nInvalidate commond (%s)\n",parameter);
            }
            break;
        case SET_MESH_ID:
            if (strlen(parameter) > 40 && strstr(parameter,"uci set wireless") > 0) {
                system(parameter);
                system("uci commit wireless");
                system("wifi");
            } else {
                printf("\nInvalidate commond (%s)\n",parameter);
            }
            break;
        case GET_WDS_ID:
            if (strlen(parameter) > 0) {
                printf("\nSSID = ");
                system(parameter);
                fflush(stdout);
            }
            break;
        case GET_MESH_ID:
            if (strlen(parameter) > 0) {
                printf("\nMesh ID = ");
                system(parameter);
                fflush(stdout);
            }
            break;
        case SET_ENCRYPTION:
            if (strlen(parameter) <= 0) {
                system("uci set wireless.@wifi-iface[0].encryption=none");
                system("uci delete wireless.@wifi-iface[0].key");
            } else {
                system("uci set wireless.@wifi-iface[0].encryption=psk2");
                system(parameter);
            }
            system("uci commit wireless");
            break;
        case SET_STATION_NO:
            set_mesh_station_no(parameter);
            break;
        case GET_STATION_NO:
            printf("\nStation No. = %s\n",parameter);
            break;
        case GET_IP_ADDRESS:
            printf(parameter);
            return EXIT_SUCCESS;
        case SHUTDOWN:
            printf("Shutdown Server...\n");
            ShutdownInstance();
//            if ((nQid = OpenMessageQueue(0)) >= 0) {
//                printf("Shutdown Server...\n");
//                CommandGo(action, parameter, nQid, false);
//            } else goto Error_Message;
            return 0;
        default: PrintHelp();
    }

    usleep(1000000);
    return 0;
Error_Message:
    ShowStatusBar("GUI mode Error on open queue(%d),error = %d\n", nQid, errno);
    return -1;
}

int main(int argc, char* argv[]) {
    int mode = 0;
    bool expert = false;

    printf(VT100_RESET VT100_CLEAR VT100_GOTO("1", "1"));
    fflush(stdout);
    if (argc < 2) {
        PrintHelp();
        return 0;
    }
    ReadCmdLineMode(argc, argv, &mode, &expert);
    switch (mode) {
        case MODE_SERVER:
            ActingAsServer(argc, argv, false);
            break;
        case MODE_SVR_DEBUG:
            ActingAsServer(argc, argv, true);
            break;
        case MODE_COMMAND_LINE:
            ActingAsCmdLine(argc, argv);
            break;
        case MODE_GUI:
            ActingAsGui(argc, argv, expert);
            break;
        default: PrintHelp();
    }
COMMATE_EXIT:
    //    printf(VT100_RESET VT100_CLEAR VT100_GOTO("23","1"));
    //    fflush(stdout);
    printf("\n\n");
    return 0;
}

