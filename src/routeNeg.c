/* 
 * File:   main.c
 * Author: wyy
 *
 * Created on 2014年10月3日, 上午11:54
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>

#include "SvrMain.h"
#include "PipeShell.h"
#include "wsocket.h"
#include "iw/iw.h"
#include "gwsman.h"
/*
 * 
 */
#define FLASH_BASE_CALDATA_OFFSET   0x1000
#define MAX_ROUTE_ENTRIES           5
#define MUTLICAST_PORT              14441
#define ART_PARTITION               "7"
#define ASSOC_LIST_NO_CHANGE        0x00
#define ASSOC_LIST_STA_JOIN         0x01
#define ASSOC_LIST_STA_BACK         0x02
#define ASSOC_LIST_STA_LEAV         0x04
#define ASSOC_LIST_STA_NB_OFF      0x08
#define MAX_NEG_TIMEOUT             30  //negotiation time out in secondss
#define MIN_NEG_BROADCAST_INTERVAL  60

//#define PATH_DEV_ART        "/dev/caldata"
#define PATH_DEV_ART        "/dev/mtdblock" ART_PARTITION
#define PATH_SORT_LIST      "/var/run/route.lst"
#define PATH_INSTANCE       "/var/run/routeneg.pid"
#define GWS_NET_IFACE       "br-lan"
#define CONFIG_PATH         "/etc/config/"
#define MESH_FILE           "mesh_id"
#define WDS_FILE            "ssid"

#define ASSOC_ENTRY_FLAG_CONFIRM    0x0001
#define ASSOC_ENTRY_FLAG_IS_LOCAL   0x0002
#define ASSOC_ENTRY_FLAG_ONLINE     0x0004
#define ASSOC_ENTRY_FLAG_RPT_BEGIN  0x0100
#define ASSOC_ENTRY_FLAG_RPT_END    0x0200
#define ASSOC_ENTRY_FLAG_IP_REQUEST 0x2000
#define ASSOC_ENTRY_FLAG_IP_RESPOND 0x4000
#define ASSOC_ENTRY_FLAG_NEGOCIATE  0x8000

#define MESH_ROUTE_STATE_INIT           0x00000001
#define MESH_ROUTE_STATE_INIT_IFACE     0x00000002
#define MESH_ROUTE_STATE_INIT_SOCKET    0x00000004
#define MESH_ROUTE_STATE_INIT_MESHID    0x00000008
#define MESH_ROUTE_STATE_INIT_ASSOC     0x00000010
#define MESH_ROUTE_STATE_INIT_DELAY     0x00000020
#define MESH_ROUTE_STATE_IDEL           0x00000040
#define MESH_ROUTE_STATE_CHECK_ASSOC    0x00000080
#define MESH_ROUTE_STATE_NEG_INIT       0x00000100
#define MESH_ROUTE_STATE_NEG_START      0x00000200
#define MESH_ROUTE_STATE_NEG_SURVEY     0x00000400
#define MESH_ROUTE_STATE_SURVEY_DONE    0x00000800
#define MESH_ROUTE_STATE_NEG_CONFIRM    0x00001000
#define MESH_ROUTE_STATE_NEG_CONFIRMED  0x00002000
#define MESH_ROUTE_STATE_NEG_COMMIT     0x00004000
#define MESH_ROUTE_STATE_DO_NOTHING     0x00008000
#define MESH_ROUTE_STATE_EXIT           0x80000000

typedef struct __assoc_entry {
    long    station;
    char    mac[6];
    int     ip;
    int     flag;
} ASSOC_ENTRY;

inline bool EntryFlagTst(ASSOC_ENTRY *entry,int flag) {
    return (entry->flag & flag) ? true : false;
}

inline void EntryStationSet(ASSOC_ENTRY *entry,int sn) {
    entry->station = sn;
}

inline int EntryStationGet(ASSOC_ENTRY *entry) {
    return entry->station;
}

inline void EntryFlagSet(ASSOC_ENTRY *entry,int flag) {
    entry->flag |= flag;
}

inline void EntryFlagClr(ASSOC_ENTRY *entry,int flag) {
    entry->flag &= (~flag);
}

inline void EntryFlagRef(ASSOC_ENTRY *entry,ASSOC_ENTRY *ref,int flag) {
    EntryFlagClr(entry,flag);
    entry->flag |= (ref->flag & flag);
}

typedef struct __assoc_list {
    int nEntry;
    ASSOC_ENTRY entries[MAX_ASSOC_ENTRIES];
} ASSOC_LIST;

typedef struct __route_entry {
    int sn;
    char mac[6];
    char prev[6];
    char next[6];
} ROUTE_ENTRY;

typedef struct __route_table {
    int nEntry;
    ROUTE_ENTRY entries[MAX_ROUTE_ENTRIES];
} ROUTE_TABLE;

static int state_assoc_list = ASSOC_LIST_NO_CHANGE;
static int static_confirm_cnt = 0;
static time_t neg_start_time = 0;

static char static_mac_zero[ETH_ALEN] = { 0x00,0x00,0x00,0x00,0x00,0x00 };
static struct nl80211_state static_nlstate;
static char static_ifname[] = "wlan0";
static char static_multicast[] = "224.1.1.2";
static ASSOC_ENTRY static_local_entry;
static ASSOC_LIST static_online_list;

//static ROUTE_TABLE init_route_table;
//static ROUTE_TABLE active_route_table;
static PipeShell mesh_shell;
static bool if_need_report = true;
static bool route_debug_en = false;
static int  route_distance = 26000;
static char route_workmode = 0;
static char route_interval = 2;

char* nmac_2_string(int* mac) {
    static char str_mac[32];
    
    memset (str_mac,0x00,32);
    if (mac) {
        sprintf(str_mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                0x00ff & mac[0],0x00ff & mac[1],0x00ff & mac[2],
                0x00ff & mac[3],0x00ff & mac[4],0x00ff & mac[5]);
    }
    return str_mac;
}

char* mac_2_string(char* mac) {
    static char str_mac[32];
    
    memset (str_mac,0x00,32);
    if (mac) {
        sprintf(str_mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                0x00ff & mac[0],0x00ff & mac[1],0x00ff & mac[2],
                0x00ff & mac[3],0x00ff & mac[4],0x00ff & mac[5]);
    }
    return str_mac;
}

void set_mesh_station_sn(time_t sn) {
    if (sn > 0) {
        int fd = open(PATH_DEV_ART,O_RDWR);
        if (fd >= 0) {
            lseek(fd, 12, SEEK_SET);
            write(fd, &sn,sizeof(time_t));
            lseek(fd, 0, SEEK_END);
            close(fd);
        } else printf("\nCan not open ART partition for MAC address.\n");
    }
}

#define SET_CHANNEL_BANDWIDTH(bw)    \
do {\
    if (bw != 0) {\
        char    command[128];\
        memset (command,0x00,128);\
        sprintf(command,"echo 0x%x > " DEBUG_INFO_BASE "chanbw",bw);\
        system (command);\
        sprintf(command,"uci set wireless.@wifi-device[0].chanbw=0x%x",bw);\
        system (command);\
        system("uci commit wireless");\
    }\
} while (0)

int get_wifi_bandwidth() {
    int fd;
    int rv = 0;
    char buffer[32];
    char *value = NULL;

    memset (buffer,0x00,32);
    if ((fd = open(DEBUG_INFO_BASE "chanbw", O_RDONLY)) > -1) {
        if (read(fd, buffer, sizeof(buffer)) > 0) {
            value = strstr(buffer,"0x");
            if (value >= buffer) {
                sscanf(value + 2,"%x",&rv);
            }
        }
        close(fd);
    }

    return rv;
}

void set_wifi_bandwidth(char* bw) {
    if (bw) {
        int newbw = 0;
        int oldbw = 0xffffff00 & get_wifi_bandwidth();
        sscanf(bw,"%d",&newbw);
        if (newbw > 0) {
            newbw &= 0x000000ff;
            newbw |= oldbw;
            SET_CHANNEL_BANDWIDTH(newbw);
        }
    }
}

void set_mesh_station_no(char* stationNo) {
    time_t sn = 0;
    if (stationNo) {
        sscanf(stationNo,"%d",&sn);
        if (sn > 0) {
            int fd = open(PATH_DEV_ART,O_RDWR);
            if (fd >= 0) {
                lseek(fd, 12, SEEK_SET);
                write(fd, &sn,sizeof(time_t));
                lseek(fd, 0, SEEK_END);
                close(fd);
            } else printf("\nCan not open ART partition for MAC address.\n");
        }
    }
}

time_t get_mesh_station_no() {
    int fd = -1;;
    time_t  sn = -1;

    if ((fd = open(PATH_DEV_ART,O_RDONLY)) >= 0) {
        lseek(fd, 12, SEEK_SET);
        read(fd, &sn,sizeof(time_t));
        close(fd);
    } else {
        printf("\nCan not open ART partition for MAC address.\n");
        return 0;
    }
    return sn;
}

void dump_mesh_id(bool mesh,char* id) {
    char* path = mesh ? CONFIG_PATH MESH_FILE : CONFIG_PATH WDS_FILE;
    if (id && strlen(id) > 6) {
        FILE* fp = fopen(path,"w");
        if (fp) {
            fprintf(fp,"%s\n",id);
            fclose(fp);
            printf("\nDone, ID (%s) saved in %s.\n",id,path);
        } else {
            printf("\nCan not open file (%s) for write\n",path);
        }
    } else {
        printf("\nInvalidate mesh id or wds id (%s)\n",id ? id : "null");
    }
}

void set_phy_distance(int distance) {
//    time_t  sn = get_mesh_station_no();
//    if (sn == 1 || sn == MAX_ROUTE_ENTRIES) {
//        system ("uci set wireless.@wifi-iface[0].mesh_fwding=0");
//    } else {
//        system ("uci set wireless.@wifi-iface[0].mesh_fwding=1");
//    }
//    system ("uci commit wireless");
    if (0 <= distance && distance <= 26000) {
        char    command[128];
        memset (command,0x00,128);        
        sprintf(command,"iw phy0 set distance %d",distance);
        system(command);
    }
}

void restore_mesh_id() {
    char* path = CONFIG_PATH MESH_FILE;
    FILE* fp = fopen(path,"r");
    if (fp) {
        char    mesh_id[64];
        memset (mesh_id,0x00,64);
        fgets(mesh_id,63,fp);
        fclose(fp);
        char*   ln = strchr(mesh_id,'\n');
        if (ln && ln >= mesh_id) {
            ln[0] = 0;
        }
        if (strlen(mesh_id) > 0) {
            char command[128];
            memset (command,0x00,128);
            sprintf(command,"uci set wireless.@wifi-iface[0].mesh_id=%s",mesh_id);
            system (command);
            system ("uci commit wireless");
            system ("wifi");
        }
    }
}

int print_assoc_list(bool dbg,char* msg,ASSOC_LIST* list) {
    if (list && dbg) {
        int i;
        time_t  current_time = time(&current_time);

        if (msg) printf("[%d]%s   [%d items in list]\n",current_time,msg,list->nEntry);
//        for (i = 0; i < list->nEntry; i ++) {
        for (i = 0; i < MAX_ASSOC_ENTRIES; i ++) {
            if (EntryStationGet(&list->entries[i]) > 0)
                printf("%c station %d,mac = %s, active = %c, confirmed = %c\n",
                    EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_IS_LOCAL) ? '*' : ' ',
                    EntryStationGet(&list->entries[i]), mac_2_string(list->entries[i].mac),
                    EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_ONLINE) ? 'O' : 'x',
                    EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_CONFIRM) ? 'O' : 'x');
        }
        printf("\n");
    }
    return 0;
}

ASSOC_ENTRY*    search_entry_by_sn(ASSOC_LIST* list,int sn) {
    if (list && sn) {
        int n;
        for (n = 0; n < list->nEntry; n ++) {
            if (EntryStationGet(&list->entries[n]) == sn) {
                return &(list->entries[n]);
            }
        }
        return NULL;
    } else return NULL;
}

ASSOC_ENTRY*    search_entry_by_mac(ASSOC_LIST* list,char mac[]) {
    if (list && mac) {
        int n;
        for (n = 0; n < list->nEntry; n ++) {
            if (memcmp(list->entries[n].mac,mac,6) == 0) {
                return &(list->entries[n]);
            }
        }
        return NULL;
    } else return NULL;
}

bool bubble_sort_list(ASSOC_LIST* list) {
    if (NULL == list) return false;
    int i,j;
    for (j = 0; j < list->nEntry; j ++) {
        if (EntryStationGet(&list->entries[j]) < 0) return false;
    }
    for (j = 0; j < list->nEntry - 1; j ++) {
        for (i = 0; i < list->nEntry - 1 - j; i ++) {
            ASSOC_ENTRY     t;
            ASSOC_ENTRY*    e0 = &list->entries[i];
            ASSOC_ENTRY*    e1 = &list->entries[i+1];
            if (memcmp(e0->mac,static_mac_zero,ETH_ALEN) == 0 || EntryStationGet(e0) == 0)
                EntryFlagClr(e0,ASSOC_ENTRY_FLAG_ONLINE);
            if (EntryStationGet(e0) > EntryStationGet(e1) || !EntryFlagTst(e0,ASSOC_ENTRY_FLAG_ONLINE)) {
                memcpy (&t,e0,sizeof(ASSOC_ENTRY));
                memcpy (e0,e1,sizeof(ASSOC_ENTRY));
                memcpy (e1,&t,sizeof(ASSOC_ENTRY));
            }
        }
    }
    for (i = 0, j = 0; j < list->nEntry; j ++) {    //remove offline nodes
        if (EntryFlagTst(&list->entries[j],ASSOC_ENTRY_FLAG_ONLINE)) i ++;
    }
    list->nEntry = i;
    
    return true;
}

//#define MESH_COMMAND_DEL    "iw dev wlan0 mpath del "
//#define MESH_COMMAND_NEW    "iw dev wlan0 mpath new "
//#define MESH_COMMAND_SET    "iw dev wlan0 mpath set "

bool do_mesh_command(bool dbg,PipeShell* ps,bool needEcho,...) {
    int     i;
    char*   argv[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    static char dest[32];
    static char next[32];
    va_list ap;
    va_start(ap, needEcho);
    for (i = 0; i < 10; i ++) {
        if ( NULL == (argv[i] = va_arg(ap, char*))) break;
    }
    va_end(ap);
    if (i > 0) {
        if (strcasecmp(argv[0],"get") == 0) {
            return true;
        } else if (strcasecmp(argv[0],"new") == 0 && i == 3) {
            strcpy (dest,argv[1]);
            strcpy (next,argv[2]);
            if (0 == gws_mpath_new(&static_nlstate,static_ifname,dest,next)) return true;
        } else if (strcasecmp(argv[0],"set") == 0 && i == 3) {
            strcpy (dest,argv[1]);
            strcpy (next,argv[2]);
            if (0 == gws_mpath_set(&static_nlstate,static_ifname,dest,next)) return true;
        } else if (strcasecmp(argv[0],"del") == 0) {
            return true;
        } else {
            return false;
        }        
    }
    return false;
}
//bool do_mesh_command(bool dbg,PipeShell* ps,bool needEcho,...) {
//    char*       argv[10] = {"iw","dev","wlan0","mpath",NULL,NULL,NULL,NULL,NULL,NULL};
//    int         i = 0;
//    
//    va_list ap;
//    va_start(ap, needEcho);
//    for (i = 4; i < 10; i ++) {
//        if ( NULL == (argv[i] = va_arg(ap, char*))) break;
//    }
//    va_end(ap);
//
//    if (PipeShellCmd(ps,"iw", argv)) {
//        int     i;
//        char    echo[1024];
//        bool    ret = true;
//        memset (echo,0x00,1024);
//        for (i = 0; needEcho && i < 20; i ++, usleep(50000)) {
//            if (PipeTryRead(ps,echo,1023) > 0) {
//                if (strstr(echo,"fail") >= echo) {
//                    ret = false;
//                    if (dbg) i = 0;
//                    else break;
//                } else break;
//            }
//        }
//        if (needEcho) {
//            if (ret) SVR_DEBUG(dbg,"\tOK [ %s ]\n",echo);
//                else SVR_DEBUG(dbg,"\tERR[ %s ]\n",echo);
//        }
//        return ret;
//    }
//    return false;
//}

int del_route_entry(bool dbg,PipeShell* ps,char* target) {
    if (ps && target) {
//        char    command[128];
        char    argument[128];
        
//        memset (command,0x00,128);
        memset (argument,0x00,128);
        sprintf(argument,"%s", mac_2_string(target));
//        sprintf(command,MESH_COMMAND_DEL "%s\n", argument);
        SVR_DEBUG(dbg,"\nRemove route : %s\n",argument);
        if (do_mesh_command(dbg,ps,true,"get",argument,NULL))
            do_mesh_command(dbg,ps,false,"del",argument,NULL);
        return 0;
    }
    return -1;
}

int set_route_entry(bool dbg,PipeShell* ps,char* target,char* next_hop) {
    int i;
    
//    for (i = 0; i < curr_mpath_table.m_nEntries; i ++) {
//        if (memcmp (target  ,curr_mpath_table.m_entry[i].m_mac_dest_add,ETH_ALEN) == 0) {
//            if (memcmp (next_hop,curr_mpath_table.m_entry[i].m_mac_next_hop,ETH_ALEN) == 0) {
//                if (dbg) {
//                    char    tgt[32];
//                    char    hop[32];
//                    memset (tgt,0x00,32);
//                    memset (hop,0x00,32);
//                    strcpy (tgt,mac_2_string(target));
//                    strcpy (hop,mac_2_string(next_hop));
//                    SVR_DEBUG(dbg,"Mesh route [ %s <-- %s ] already existed\n",tgt,hop);
//                }
//                return 0;
//            }
//        }            
//    }
    if (ps && target && next_hop) {
        char    tgt[32];
        char    hop[32];
        if (0 == memcmp(next_hop,static_mac_zero,ETH_ALEN)) return -1;
        memset (tgt,0x00,32);
        memset (hop,0x00,32);
        strcpy (tgt,mac_2_string(target));
        strcpy (hop,mac_2_string(next_hop));
        SVR_DEBUG(dbg,"SET Mesh Path [ %s <-- %s ]\n", tgt, hop);
        if (do_mesh_command(dbg,ps,true,"new",tgt,hop,NULL))
            do_mesh_command(dbg,ps,false,"set",tgt,hop,NULL);
//        do_mesh_command(dbg,ps,false,"new",tgt,"next_hop",hop,NULL);
//        do_mesh_command(dbg,ps,false,"set",tgt,"next_hop",hop,NULL);
        return 0;
    }
    return -1;
}

int save_assoc_list(char* path, ASSOC_LIST* list) {
    if (path && list) {
        FILE*   fd_assoc = fopen(path,"w");

        if (fd_assoc) {
            int         i;
            char        assoc_string[128];

            for (i = 0; i < list->nEntry; i ++) {
                if (!EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_IS_LOCAL)) {
                    memset(assoc_string,0x00,128);
                    sprintf(assoc_string,"%s\n",mac_2_string(list->entries[i].mac));
                    fputs(assoc_string,fd_assoc);
                }
            }
            fclose(fd_assoc);
            return 1;
        }
    }
    return 0;
}

int read_assoc_list(char* path,ASSOC_LIST* list) {
    if (path && list) {
        FILE*   fd_assoc = fopen(path,"r");

        //set local entry (with predefined station no. and mac address) as the first one
        memset(list,0x00,sizeof(ASSOC_LIST));
        memcpy(&list->entries[0],&static_local_entry,sizeof(ASSOC_ENTRY));
        list->nEntry = 1;
        if (fd_assoc) {
            while (!feof(fd_assoc) && list->nEntry < MAX_ASSOC_ENTRIES) {
                int     mac[6];
                char    assoc_string[128];
                memset(mac,0x00,sizeof(mac));
                memset(assoc_string,0x00,128);
                if (fgets(assoc_string,127,fd_assoc)) {
                    sscanf(assoc_string,"%x:%x:%x:%x:%x:%x",
                            &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
                    if (mac[3] || mac[4] || mac[5]) {
                        int i;
                        for (i = 0; i < 6; i ++)
                            list->entries[list->nEntry].mac[i] = (char) (0x00ff&mac[i]);
                        EntryFlagSet(&list->entries[list->nEntry],ASSOC_ENTRY_FLAG_ONLINE);
                        EntryStationSet(&(list->entries[list->nEntry]),0);
                        list->nEntry ++;
                    } else break;
                } else break;
            }
            fclose(fd_assoc);
            return 1;
        }
    }
    return 0;
}

static void clear_mesh_route(bool dbg,PipeShell* ps,ASSOC_LIST* list) {
    ASSOC_LIST  old_list;
    int i;
    read_assoc_list(PATH_SORT_LIST,&old_list);
    for (i = 0; i < old_list.nEntry; i ++) {  //clear route table
        if (NULL == search_entry_by_mac(list,old_list.entries[i].mac)) {    //is not an active assoc anymore
            del_route_entry(dbg,ps,old_list.entries[i].mac);
        }
    }
}

int route_commit_link(bool dbg,PipeShell* ps,ASSOC_LIST* list) {
    int i = 0, curr = -1;
    if (ps && list) {
        if (list->nEntry > 2 ) {
            char mac_prev[ETH_ALEN];
            char mac_next[ETH_ALEN];

            memset (mac_prev,0x00,ETH_ALEN);
            memset (mac_next,0x00,ETH_ALEN);
//            clear_mesh_route();
            for (i = 0; i < list->nEntry; i ++) {
                if (EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_IS_LOCAL)) {
                    curr = i;
                    if (0 == i) {
                        memcpy (mac_next,list->entries[1].mac,ETH_ALEN);
                    } else if (list->nEntry - 1 == i) {
                        memcpy (mac_prev,list->entries[list->nEntry - 2].mac,ETH_ALEN);
                    } else {
                        memcpy (mac_prev,list->entries[i-1].mac,ETH_ALEN);
                        memcpy (mac_next,list->entries[i+1].mac,ETH_ALEN);
                    }
//                } else if (!EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_ONLINE)) {
//                    del_route_entry(dbg,ps,list->entries[i].mac);
                }
            }
            if (curr < 0) return 0;
            for (i = 0; i < list->nEntry; i ++) {
                if (i < curr) set_route_entry(dbg,ps,list->entries[i].mac,mac_prev);
                else if (i > curr) set_route_entry(dbg,ps,list->entries[i].mac,mac_next);
                else continue;
            }
            for (; i < MAX_ASSOC_ENTRIES; i ++) {
                if (0 != memcmp(list->entries[i].mac,static_mac_zero,ETH_ALEN) &&
                        !EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_ONLINE))
                    set_route_entry(dbg,ps,list->entries[i].mac,list->entries[i].mac);
            }
            save_assoc_list(PATH_SORT_LIST,list);
            return 1;
        } else {
            for (i = 0; i < list->nEntry; i ++) {
                if (0 != memcmp(list->entries[i].mac,static_mac_zero,ETH_ALEN))
                    set_route_entry(dbg,ps,list->entries[i].mac,list->entries[i].mac);
            }
        }
    }
    return 0;
}

//int route_commit_circle(bool dbg,PipeShell* ps,ASSOC_LIST* list) {
//    if (ps && list && list->nEntry >= 4) {
//        int i;
//        for (i = 0; i < list->nEntry; i ++) {
//            if (EntryFlagTst(&list->entries[i],ASSOC_ENTRY_FLAG_IS_LOCAL)) {
//                int a = (i + list->nEntry - 2) % list->nEntry;
//                int b = (i + list->nEntry - 1) % list->nEntry;
//                int c = (i + 1) % list->nEntry;
//                int d = (i + 2) % list->nEntry;
//                SVR_DEBUG(dbg,"\ncircle a = %d,b=%d,c=%d,d=%d\n",a+1,b+1,c+1,d+1);
//                set_route_entry(dbg,ps,list->entries[a].mac,list->entries[b].mac);
//                set_route_entry(dbg,ps,list->entries[b].mac,list->entries[b].mac);
//                set_route_entry(dbg,ps,list->entries[c].mac,list->entries[c].mac);
//                set_route_entry(dbg,ps,list->entries[d].mac,list->entries[c].mac);
//                return 1;
//            }
//        }
//    }
//    return 0;
//}

int append_assoc_list(ASSOC_LIST* list,ASSOC_ENTRY* entry) {
    if (list && entry) {
        int i,nOnline = 0;
        bool append_done = false;
        for (i = 0; i < MAX_ASSOC_ENTRIES; i ++) {
            if (!append_done) {
                if (memcmp (list->entries[i].mac,static_mac_zero,ETH_ALEN) == 0 ||  //free space here
                    memcmp (list->entries[i].mac,entry->mac     ,ETH_ALEN) == 0) {  //alread existed
                    memcpy (&list->entries[i],entry,sizeof(ASSOC_ENTRY));       //just update
                    append_done = true;
                }
            } else {
                if (memcmp (list->entries[i].mac,static_mac_zero,ETH_ALEN) == 0 ||  //free space here
                    memcmp (list->entries[i].mac,entry->mac     ,ETH_ALEN) == 0) {  //alread existed
                    memset (list->entries[i].mac,0x00           ,ETH_ALEN);
                    list->nEntry = i;
                    return i;
                }
            }
        }
        return (MAX_ASSOC_ENTRIES-1);
    }
    return 0;
}

int route_commit(bool dbg,PipeShell* ps,bool fixed_route) {
    if (fixed_route) {
        int i;

        if (NULL == ps ) return 0;
//        for (i = 0; i < list->nEntry; i ++) {    //prepare new route table
//            ASSOC_ENTRY*    current = &list->entries[i];
//            ASSOC_ENTRY*    entry = search_entry_by_mac(&static_online_list,current->mac);
//
//            if (entry) {
//                entry->station = current->station;
//            }
//        }
        SVR_DEBUG(dbg,"Insert local STA to online list [flag = %x, station = %d, IP = %s, mac = %s]\n",
                static_local_entry.flag,EntryStationGet(&static_local_entry),
                IPconvertN2A(static_local_entry.ip),mac_2_string(static_local_entry.mac));
        EntryFlagSet(&static_local_entry,ASSOC_ENTRY_FLAG_ONLINE);
        append_assoc_list(&static_online_list,&static_local_entry);
        if (bubble_sort_list(&static_online_list)) {
            print_assoc_list(dbg,"COMMIT route...",&static_online_list);
            return route_commit_link(dbg,ps,&static_online_list);
        } else {
            SVR_DEBUG(dbg,"Online list not confirmed, can not change route.\n");
            return 0;
        }
    //    return enable ? route_commit_circle(dbg,ps,list) :
    //                    route_commit_link(dbg,ps,list);
    }
    return 0;
}

int check_assoc_list(ASSOC_LIST* list,int shmID,P_GWS_KPI pKpi) {
    if (list && pKpi) {
        int i,j,nAssoc = 0;
        //set local entry (with predefined station no. and mac address) as the first one
        RW_KPI_VAR(shmID,nAssoc,pKpi->m_assoc_list.m_nLine);
        memset(list,0x00,sizeof(ASSOC_LIST));
        memcpy(&list->entries[0],&static_local_entry,sizeof(ASSOC_ENTRY));
        for (list->nEntry = 1, i = 0; i < nAssoc && list->nEntry < MAX_ASSOC_ENTRIES; i ++) {
            int     tmp[ETH_ALEN];
            char    mac[ETH_ALEN];
            char    assoc_string[IW_LIST_MAXSIZE];
            memset(tmp,0x00,ETH_ALEN * sizeof(int));
            memset(mac,0x00,ETH_ALEN * sizeof(char));
            memset(assoc_string,0x00,IW_LIST_MAXSIZE);

            RW_KPI_STR(shmID,assoc_string,pKpi->m_assoc_list.m_value[i]);
            sscanf(assoc_string,"%x:%x:%x:%x:%x:%x", &tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]);
            
            if (tmp[0] && tmp[1] && tmp[2]) {   // [ac:ee:3b:01:xx:yy]
                for (j = 0; j < ETH_ALEN; j ++) list->entries[list->nEntry].mac[j] = (char) (0x00ff&tmp[j]);
                EntryFlagSet(&list->entries[list->nEntry],ASSOC_ENTRY_FLAG_ONLINE);
                EntryStationSet(&(list->entries[list->nEntry]),0);
                list->nEntry ++;
            }
        }
        return 1;
    }
    return 0;
}

void send_response(bool dbg,int flag_set,int flag_clr) {
    ASSOC_ENTRY res;
    memcpy (&res,&static_local_entry,sizeof(ASSOC_ENTRY));
    res.ip = Socket_GetIP(GWS_NET_IFACE);
    if (flag_set) EntryFlagSet(&res,flag_set);
    if (flag_clr) EntryFlagClr(&res,flag_clr);
    EntryFlagClr(&res,ASSOC_ENTRY_FLAG_IS_LOCAL);
    SVR_DEBUG(dbg,"-->> [station -- %d, mac -- %s, IP -- %s]\n",
                    EntryStationGet(&res),mac_2_string(res.mac),IPconvertN2A(res.ip));
    Socket_SendTo(static_multicast,MUTLICAST_PORT,(char*)(&res),sizeof(ASSOC_ENTRY));
}

char* parse_assoc_entry(char* s,char c,char* sub) {
    if (sub) {
        char* p = strchr(s,c);
        if (p > s) {
            *p = 0;
            strcpy (sub,s);
            return (p+1);
        } else {
            strcpy (sub,s);
            return NULL;
        }
    }
    return NULL;
}

int update_assoc_route_table(MPATH_INFO* entry) {
    int i;
    if (entry) {
        for (i = 0; i < curr_mpath_table.m_nEntries; i ++) {
            if (memcmp (entry->m_mac_dest_add,curr_mpath_table.m_entry[i].m_mac_dest_add,ETH_ALEN) == 0) {
                curr_mpath_table.m_entry[i].m_signal = entry->m_signal;
                curr_mpath_table.m_entry[i].m_noise = entry->m_noise;
                curr_mpath_table.m_entry[i].m_mcs_rx = entry->m_mcs_rx;
                curr_mpath_table.m_entry[i].m_mcs_tx = entry->m_mcs_tx;
                return curr_mpath_table.m_nEntries;
            }
        }
        return mpath_table_append(entry);
    }
    return 0;
}

void broadcast_curr_route_table(bool dbg,char* ifname,int shmID,P_GWS_KPI pKpi) {
    if (pKpi) {
        int i,j,nAssoc = 0;
        //set local entry (with predefined station no. and mac address) as the first one
        RW_KPI_VAR(shmID,nAssoc,pKpi->m_assoc_list.m_nLine);
        for (i = 0; i < nAssoc && i < MAX_ASSOC_ENTRIES; i ++) {
            unsigned char mac[6];
            char    assoc_string[IW_LIST_MAXSIZE];
            char    assoc_params[8][32];
            char    assoc_signal[3][32];
            char*   s = NULL;
            memset(mac,0x00,sizeof(mac));
            memset(assoc_string,0x00,IW_LIST_MAXSIZE);

            RW_KPI_STR(shmID,assoc_string,pKpi->m_assoc_list.m_value[i]);
//            "00:03:7F:04:00:01|  -49/-101/52|    0| 5.4 MCS2 s| 5.4 MCS2 s|    9542|    2821"
            for (s = assoc_string, j = 0; j < 8 && s; j ++) {
                memset (assoc_params[j],0x00,32);
                s = parse_assoc_entry(s,'|',assoc_params[j]);
//                printf("[%d]    %s\n",j,assoc_params[j]);
            }
            if (j >= 6) {
                MPATH_INFO      info;
                ASSOC_ENTRY*    entry = NULL;
                mac_addr_a2n(info.m_mac_dest_add,assoc_params[0]);
                for (s = assoc_params[1], j = 0; j < 3 && s; j ++) {
                    memset (assoc_signal[j],0x00,32);
                    s = parse_assoc_entry(s,'/',assoc_signal[j]);
                }
                if (j > 1) {
                    info.m_signal = atoi(assoc_signal[0]);
                    info.m_noise = atoi(assoc_signal[1]);
                }
                info.m_mcs_rx = (unsigned short) (10 * atof(assoc_params[3]));
                info.m_mcs_tx = (unsigned short) (10 * atof(assoc_params[4]));
                update_assoc_route_table(&info);
            }
        }
        if (curr_mpath_table.m_nEntries > 0) {
            curr_mpath_table.m_station_no = EntryStationGet(&static_local_entry);
            curr_mpath_table.m_ip_address = static_local_entry.ip;
            memcpy (curr_mpath_table.m_mac_local,static_local_entry.mac,ETH_ALEN);
//            SVR_DEBUG(dbg,"Broadcast [%02x:%02x:%02x:%02x:%02x:%02x] station = %d\n",
//                        0x00ff & static_local_entry.mac[0],0x00ff & static_local_entry.mac[1],0x00ff & static_local_entry.mac[2],
//                        0x00ff & static_local_entry.mac[3],0x00ff & static_local_entry.mac[4],0x00ff & static_local_entry.mac[5],static_local_entry.station);
            RW_KPI_VAR(shmID,curr_mpath_table.m_channel,pKpi->m_radio.m_nChanNo);
            RW_KPI_VAR(shmID,curr_mpath_table.m_txpower,pKpi->m_radio.m_nCurTxPwr);
            RW_KPI_VAR(shmID,curr_mpath_table.m_temperature,pKpi->m_radio.m_nTemp);
            Socket_SendTo(static_multicast,MUTLICAST_PORT,(char*)(&curr_mpath_table),sizeof(MPATH_TABLE));
        }
        gws_mpath_dump(&static_nlstate,ifname);
    }
}

bool update_assoc_list_with_station_no(bool dbg,unsigned char sn,unsigned int ip,unsigned char* mac,int shmID,P_GWS_KPI pKpi,ASSOC_LIST* online_list) {
    bool need_negotiation = false;
    if (pKpi) {
        int             i,j,nAssoc = 0;
        ASSOC_ENTRY*    current = NULL;
        if (current = search_entry_by_mac(online_list,mac)) {
            EntryStationSet(current,sn);
            current->ip = ip;
            EntryFlagSet(current,ASSOC_ENTRY_FLAG_ONLINE);
        } else {
            ASSOC_ENTRY entry;
            EntryFlagSet(&entry,ASSOC_ENTRY_FLAG_ONLINE);
            EntryStationSet(&entry,sn);
            entry.ip = ip;
            memcpy (entry.mac, mac, ETH_ALEN);
            append_assoc_list(online_list,&entry);    //can not see this node, but it's exist
            need_negotiation = true;
        }
        //set local entry (with predefined station no. and mac address) as the first one
        RW_KPI_VAR(shmID,nAssoc,pKpi->m_assoc_list.m_nLine);
        for (i = 0; i < nAssoc; i ++) {
            int     mac_int[6];
            char    assoc_string[IW_LIST_MAXSIZE];
            memset(mac_int,0x00,sizeof(mac_int));
            memset(assoc_string,0x00,IW_LIST_MAXSIZE);

            RW_KPI_STR(shmID,assoc_string,pKpi->m_assoc_list.m_value[i]);
            sscanf(assoc_string, "%x:%x:%x:%x:%x:%x",   &mac_int[0],&mac_int[1],&mac_int[2],
                                                        &mac_int[3],&mac_int[4],&mac_int[5]);
            if (mac[0] == (unsigned char)mac_int[0] && mac[1] == (unsigned char)mac_int[1] &&
                mac[2] == (unsigned char)mac_int[2] && mac[3] == (unsigned char)mac_int[3] &&
                mac[4] == (unsigned char)mac_int[4] && mac[5] == (unsigned char)mac_int[5] && sn) {
                unsigned char sn_old = 0;
                RW_KPI_VAR(shmID,sn_old,pKpi->m_assoc_list.m_index[i]);
                if (sn_old != sn) {
                    RW_KPI_VAR(shmID,pKpi->m_assoc_list.m_index[i],sn);
                    SVR_DEBUG(dbg,"Update [%02x:%02x:%02x:%02x:%02x:%02x] station [%d -> %d]\n",
                                        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],sn_old,sn);
                    need_negotiation = true;
                    break;
                }
            }
        }
    }
    return need_negotiation;
}

#define SVR_DEBUG_INCOME(dbg,entry,path)   do {\
    SVR_DEBUG(dbg,"<<-- [flag = %x, station = %d, IP = %s, mac = %s]",\
        entry->flag,EntryStationGet(entry),IPconvertN2A(entry->ip),mac_2_string(entry->mac));\
    if (11 == path) {\
        SVR_DEBUG(dbg,"\tSET SN = %d",EntryStationGet(entry));\
    } else if (12 == path) {\
        SVR_DEBUG(dbg,"\tSET IP = %s",IPconvertN2A(entry->ip));\
    } else if (13 == path) {\
        SVR_DEBUG(dbg,"\t[IP request] from host\n");\
    } else if (14 == path) {\
        SVR_DEBUG(dbg,"\t[IP request]\n");\
    } else if (21 == path) {\
        SVR_DEBUG(dbg,"\t[NEG request]\n");\
    } else if (22 == path) {\
        SVR_DEBUG(dbg,"\t[NEG confirm]\n");\
    } else if (23 == path) {\
        SVR_DEBUG(dbg,"\t[NEG message] (hidden node)\n");\
    } else if (31 == path) {\
        SVR_DEBUG(dbg,"[report begin]\n");\
    } else if (32 == path) {\
        SVR_DEBUG(dbg,"[report end]\n");\
    } else if (33 == path) {\
        SVR_DEBUG(dbg,"[ip respond]\n");\
    } else if (34 == path) {\
        SVR_DEBUG(dbg,"[unknown request]\n");\
    } else if (41 == path) {\
        SVR_DEBUG(dbg,"[Append to online list]\n");\
    }\
} while (0)
    
int is_negotiation_request(bool dbg,int shmID,P_GWS_KPI pKpi,ASSOC_LIST* online_list) {
    DWORD       size = MAX_BUFF_SIZE;
    char*       ip = NULL;
    
    if (ip = Socket_RecvFrom(static_multicast,MUTLICAST_PORT,&size)) {
        char* buff = Socket_GetData(&size);
        if (size == sizeof(ASSOC_ENTRY)) {
            ASSOC_ENTRY* recv_entry = (ASSOC_ENTRY*) buff;
            
            if (EntryFlagTst(recv_entry,ASSOC_ENTRY_FLAG_IP_REQUEST)) {
                ASSOC_ENTRY* entry = search_entry_by_mac(online_list,recv_entry->mac);
                if (entry) {
                    if (EntryFlagTst(entry,ASSOC_ENTRY_FLAG_IS_LOCAL)) {
                        if (EntryStationGet(recv_entry) > 0) {
                            EntryStationSet(&static_local_entry,EntryStationGet(recv_entry));
                            set_mesh_station_sn(EntryStationGet(recv_entry));
                            SVR_DEBUG_INCOME(dbg,recv_entry,11);
                        }
                        if (recv_entry->ip > 0) {
                            char    mesh_id[128];
                            memset (mesh_id,0x00,128);
                            sprintf(mesh_id,"gwsmesh%u",(unsigned int)recv_entry->ip);
                            dump_mesh_id(true,mesh_id);
                            sprintf(mesh_id,"uci set wireless.@wifi-iface[0].mesh_id=gwsmesh%u",(unsigned int)recv_entry->ip);
                            system(mesh_id);
                            SVR_DEBUG_INCOME(dbg,recv_entry,12);
                            SVR_DEBUG(dbg,"\tSet MESH ID as %s",mesh_id);\
                            system("uci commit wireless");
                            system("wifi");
                        }
                    }
                } else {
//                    EntryFlagSet(recv_entry,ASSOC_ENTRY_FLAG_ONLINE);
//                    append_assoc_list(online_list,recv_entry);    //can not see this node, but it's exist
                    SVR_DEBUG_INCOME(dbg,recv_entry,13);
                    return 1;
                }
                SVR_DEBUG_INCOME(dbg,recv_entry,14);
                send_response(dbg,ASSOC_ENTRY_FLAG_IP_RESPOND,0);
            } else if (EntryFlagTst(recv_entry,ASSOC_ENTRY_FLAG_NEGOCIATE)) {// && recv_entry->station > 0) {
                ASSOC_ENTRY* assoc_entry = search_entry_by_mac(online_list,recv_entry->mac);
                if (assoc_entry) {
                    EntryStationSet(assoc_entry,EntryStationGet(recv_entry));          //give station no. with that mac
                    EntryFlagRef(assoc_entry,recv_entry,ASSOC_ENTRY_FLAG_CONFIRM);
                    if (!EntryFlagTst(assoc_entry,ASSOC_ENTRY_FLAG_CONFIRM)) {
                        SVR_DEBUG_INCOME(dbg,recv_entry,21);
                        return 1;
                    } else {
                        SVR_DEBUG_INCOME(dbg,recv_entry,22);
                    }
                } else {
                    EntryFlagSet(recv_entry,ASSOC_ENTRY_FLAG_ONLINE);
                    append_assoc_list(online_list,recv_entry);    //can not see this node, but it's exist
                    SVR_DEBUG_INCOME(dbg,recv_entry,23);
                    return 1;
                }
            } else if (EntryFlagTst(recv_entry,ASSOC_ENTRY_FLAG_RPT_BEGIN)) {
                if_need_report = true;
                SVR_DEBUG_INCOME(dbg,recv_entry,31);
            } else if (EntryFlagTst(recv_entry,ASSOC_ENTRY_FLAG_RPT_END)) {
                if_need_report = false;
                SVR_DEBUG_INCOME(dbg,recv_entry,32);
            } else if (EntryFlagTst(recv_entry,ASSOC_ENTRY_FLAG_IP_RESPOND)) {
                SVR_DEBUG_INCOME(dbg,recv_entry,33);
            } else SVR_DEBUG_INCOME(dbg,recv_entry,34);
        } else if (size == sizeof(MPATH_TABLE)) {
            MPATH_TABLE* mesh_table = (MPATH_TABLE*) buff;
            return update_assoc_list_with_station_no(dbg,   mesh_table->m_station_no, mesh_table->m_ip_address,
                                                            mesh_table->m_mac_local, shmID,pKpi,online_list);
        } else {    // something wrong, just ignore it
        }
    }
    return 0;
}

#define ASSOC_LIST_STATUS_UNCHANGED     0x00
#define ASSOC_LIST_STATUS_LOST_ASSOC    0x01
#define ASSOC_LIST_STATUS_NEW_ASSOC     0x02
#define ASSOC_LIST_STATUS_NEW_ONLINE    0x04

bool is_mesh_neighbour(ASSOC_ENTRY* entry) {
    if (entry) {
        int i;
        for (i = 0; i < curr_mpath_table.m_nEntries; i ++) {
            if (memcmp (entry->mac,curr_mpath_table.m_entry[i].m_mac_next_hop,ETH_ALEN) == 0) {
                if (memcmp (entry->mac,curr_mpath_table.m_entry[i].m_mac_dest_add,ETH_ALEN) != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

int is_assoc_list_changed(bool dbg,PipeShell* ps,int shmID,P_GWS_KPI pKpi) {
    static ASSOC_LIST static_assoc_list;
    ASSOC_LIST  latest_assoc_list;
    int         m;

    if (ps == NULL) {
        memset(&static_assoc_list,0x00,sizeof(ASSOC_LIST));
        return 0;
    }
    if (check_assoc_list(&latest_assoc_list,shmID,pKpi)) {
        int assoc_state = ASSOC_LIST_STATUS_UNCHANGED;
        for (m = 0; m < latest_assoc_list.nEntry; m ++) { //for every entry, search in static_assoc_list
            ASSOC_ENTRY*    entry = &latest_assoc_list.entries[m];
            ASSOC_ENTRY*    assoc = search_entry_by_mac(&static_assoc_list,entry->mac);
            ASSOC_ENTRY*    online = NULL;
            
            if (assoc) {   //alread exist, just pick up station no.
                EntryStationSet(entry,EntryStationGet(assoc));
                EntryStationSet(assoc,-1);
            } else {   //not in static_assoc_list, search in static_online_list
                assoc_state |= ASSOC_LIST_STATUS_NEW_ASSOC;
            }
            online = search_entry_by_mac(&static_online_list,entry->mac);
            if (online) {  //pick up station no. from static_online_list
                EntryStationSet(entry,EntryStationGet(online));
                EntryFlagSet(online,ASSOC_ENTRY_FLAG_ONLINE);
            } else {    //not in online_list, append as a new STA to online_list
                assoc_state |= ASSOC_LIST_STATUS_NEW_ONLINE;
                EntryFlagSet(entry,ASSOC_ENTRY_FLAG_ONLINE);
                append_assoc_list(&static_online_list,entry);
            }
        }
        if (assoc_state & ASSOC_LIST_STATUS_NEW_ONLINE) {   //new assoc STA join online
            memcpy (&static_assoc_list,&latest_assoc_list,sizeof(ASSOC_LIST));
            print_assoc_list(dbg,"[INFO] New STA just join...",&static_assoc_list);
            return ASSOC_LIST_STA_JOIN; //need to negociate
        } else {    //there is no any new STA added to online_list
            if (assoc_state & ASSOC_LIST_STATUS_NEW_ASSOC) { //new assoc STA, but already in online_list
                memcpy (&static_assoc_list,&latest_assoc_list,sizeof(ASSOC_LIST));
                print_assoc_list(dbg,"[INFO] Missed STA back...",&static_assoc_list);
                return ASSOC_LIST_STA_BACK;
            } else {    //no new STA added to online_list
                ASSOC_ENTRY* lost = NULL;
                ASSOC_ENTRY* offline = NULL;
                for (m = 0; m < static_assoc_list.nEntry; m ++) { //search for which STA has gone
                    if (EntryStationGet(&static_assoc_list.entries[m]) > 0) {
                        lost = &static_assoc_list.entries[m];
                        break;
                    }
                }
                if (latest_assoc_list.nEntry < static_assoc_list.nEntry) { //one ore more STA lost from online_list
                    if (is_mesh_neighbour(lost)) {  // If the losted STA is our neighbour then we need to modify  route right now
                        if (offline = search_entry_by_mac(&static_online_list,lost->mac)) {
                            EntryFlagClr(offline,ASSOC_ENTRY_FLAG_ONLINE);
                        }
                        memcpy (&static_assoc_list,&latest_assoc_list,sizeof(ASSOC_LIST));      // 
                        print_assoc_list(dbg,"[INFO] Associated STA leaving...",&static_assoc_list);// mesh
                        return ASSOC_LIST_STA_NB_OFF;
                    } else {
                        memcpy (&static_assoc_list,&latest_assoc_list,sizeof(ASSOC_LIST));
                        print_assoc_list(dbg,"[INFO] Associated STA leaving...",&static_assoc_list);
                        return ASSOC_LIST_STA_LEAV;
                    }
                } else { //new assoc online_list is same as current assoc online_list
                    memcpy (&static_assoc_list,&latest_assoc_list,sizeof(ASSOC_LIST));
                    return ASSOC_LIST_NO_CHANGE;
                }
            }
        }
//        if (0 == (assoc_state & ASSOC_LIST_STATUS_NEW_ONLINE)) { //all entries in latest_assoc_list are existed in online_list
//            if (latest_assoc_list.nEntry < online_list->nEntry) { //one ore more assoc lost from current STA
//                memcpy (online_list,&latest_assoc_list,sizeof(ASSOC_LIST));
//                print_assoc_list(dbg,"[INFO] Associated STA leaving...",online_list);
////                route_commit(dbg,ps,online_list,(pKpi == NULL) ? false : true);
//                return ASSOC_LIST_KNOWN_STA;
//            }   //else new assoc online_list is same as current assoc online_list
//            return ASSOC_LIST_NO_CHANGE;
//        } else if (0 == (assoc_state & ASSOC_LIST_STATUS_NEW_ASSOC)) {
//            memcpy (online_list,&latest_assoc_list,sizeof(ASSOC_LIST));
//            print_assoc_list(dbg,"[INFO] Missed STA back...",online_list);
////            route_commit(dbg,ps,online_list,(pKpi == NULL) ? false : true);
//            return ASSOC_LIST_KNOWN_STA;
//        } else {    
//            memcpy (online_list,&latest_assoc_list,sizeof(ASSOC_LIST));
//            print_assoc_list(dbg,"New associated node found...",online_list);
////            route_commit(dbg,ps,online_list,(pKpi == NULL) ? false : true);
//            return ASSOC_LIST_NEW_STA; //need to negociate
//        }
    }
    return ASSOC_LIST_NO_CHANGE;
}

int init_assoc_list() {
    int fd = -1;;
    char mac_ge[6];
    char mac_fe[6];
    char version[2];
    
    memset(&static_local_entry,0x00,sizeof(ASSOC_ENTRY));
    memset(&static_online_list,0x00,sizeof(ASSOC_LIST));
    is_assoc_list_changed(false,NULL,0,NULL);
    if ((fd = open(PATH_DEV_ART,O_RDONLY)) < 0) {
        printf("\nCan not open ART partition for MAC address.\n");
        return 0;
    }
    lseek(fd, 0, SEEK_SET);
    memset (mac_ge,0x00,6);
    memset (mac_fe,0x00,6);
    read(fd, mac_ge, 6);
    read(fd, mac_fe, 6);
    read(fd, &static_local_entry.station,sizeof(time_t));
    lseek(fd, FLASH_BASE_CALDATA_OFFSET, SEEK_SET);
    read(fd, version, 2);
    read(fd, static_local_entry.mac, 6);
    close(fd);
    static_local_entry.ip = Socket_GetIP("br-lan");
    EntryFlagSet(&static_local_entry,ASSOC_ENTRY_FLAG_IS_LOCAL);
    PipeShellInit(&mesh_shell);
    gws_mpath_init(&static_nlstate);

    return 1;
}

bool listen_response(bool dbg,ASSOC_LIST* online_list,time_t start_time) {
    char* ip = NULL;
    DWORD size = MAX_BUFF_SIZE;
    time_t current_time = time(&current_time);
    
    if (current_time - start_time > MAX_NEG_TIMEOUT) return false;  //time out
    if (ip = Socket_RecvFrom(static_multicast,MUTLICAST_PORT,&size)) {
        char* buff = Socket_GetData(&size);
        if (size == sizeof(ASSOC_ENTRY)) {
            ASSOC_ENTRY* recv_entry = (ASSOC_ENTRY*) buff;
            ASSOC_ENTRY* entry = search_entry_by_mac(online_list,recv_entry->mac);
            if (entry) {
//                EntryStationSet(entry,EntryStationGet(recv_entry));          //give station no. with that mac
                EntryFlagRef(entry,recv_entry,ASSOC_ENTRY_FLAG_CONFIRM);
                EntryFlagSet(entry,ASSOC_ENTRY_FLAG_ONLINE);
//                if (EntryFlagTst(entry,ASSOC_ENTRY_FLAG_CONFIRM)) {
//                    SVR_DEBUG(dbg, "Recv sta %d, mac = %s, confirm = %s\n",
//                            recv_entry->station,mac_2_string(recv_entry->mac),
//                            EntryFlagTst(recv_entry,ASSOC_ENTRY_FLAG_CONFIRM) ? "yes" : "no");
//                }
            } else {
                EntryFlagSet(recv_entry,ASSOC_ENTRY_FLAG_ONLINE);
                append_assoc_list(online_list,recv_entry);
                SVR_DEBUG_INCOME(dbg,recv_entry,41);
            }
        }   // something wrong, just ignore it
    }
    return true;
}

unsigned long negociation_survey(bool dbg,unsigned long ticks) {
    if (listen_response(dbg,&static_online_list,neg_start_time)) {
        if (ticks % 10 == 0) {
            int n;
            for (n = 0; n < static_online_list.nEntry; n ++) {
                if (memcmp(static_online_list.entries[n].mac,static_mac_zero,ETH_ALEN) == 0) continue;
                if (EntryFlagTst(&static_online_list.entries[n], ASSOC_ENTRY_FLAG_IS_LOCAL)) continue;
                if (EntryStationGet(&static_online_list.entries[n]) > 0) {
                    if (EntryFlagTst(&static_online_list.entries[n], ASSOC_ENTRY_FLAG_ONLINE)) continue;
                    else if (!is_mesh_neighbour(&static_online_list.entries[n])) continue;
                }
                send_response(dbg,ASSOC_ENTRY_FLAG_NEGOCIATE,ASSOC_ENTRY_FLAG_CONFIRM);
                SVR_DEBUG(dbg,"\nOnline list[%d] [station = %d, mac = %s] not confirmed yet\n",
                            n,EntryStationGet(&static_online_list.entries[n]),mac_2_string(static_online_list.entries[n].mac));
                return MESH_ROUTE_STATE_NEG_SURVEY;
            }
            send_response(dbg,ASSOC_ENTRY_FLAG_NEGOCIATE | ASSOC_ENTRY_FLAG_CONFIRM,0);
            return MESH_ROUTE_STATE_SURVEY_DONE;
        }
    } else return MESH_ROUTE_STATE_IDEL; //survey time out, without all the sations, we can only give up
    return MESH_ROUTE_STATE_NEG_SURVEY;
}

unsigned long negociation_survey_done(bool dbg,unsigned long ticks) {
    if (listen_response(dbg,&static_online_list,neg_start_time)) {
        if (ticks % 10 == 0) {   //every 5 times, send local info to multicast
            int n;
            for (n = 0; n < static_online_list.nEntry; n ++) {    // has it's confirmation.
                if (memcmp (static_online_list.entries[n].mac,static_mac_zero,ETH_ALEN) == 0) continue;
                if (EntryFlagTst(&static_online_list.entries[n],ASSOC_ENTRY_FLAG_IS_LOCAL)) {  //our own mac, confirm
                    EntryFlagSet(&static_online_list.entries[n],ASSOC_ENTRY_FLAG_CONFIRM);
                    EntryFlagSet(&static_local_entry,ASSOC_ENTRY_FLAG_NEGOCIATE | ASSOC_ENTRY_FLAG_CONFIRM);
                } else {    //if any other is not confirm yet, we need to wait and send confirmation repeataly
                    if (!EntryFlagTst(&static_online_list.entries[n],ASSOC_ENTRY_FLAG_ONLINE|ASSOC_ENTRY_FLAG_CONFIRM)) { //not every mac confirmed, no neg_done
                        send_response(dbg,ASSOC_ENTRY_FLAG_NEGOCIATE | ASSOC_ENTRY_FLAG_CONFIRM,0);
                        return MESH_ROUTE_STATE_SURVEY_DONE;
                    }
                }
            }
            return MESH_ROUTE_STATE_NEG_COMMIT;
        } else return MESH_ROUTE_STATE_SURVEY_DONE;
    }
    return MESH_ROUTE_STATE_NEG_COMMIT; //commit anyway
}

//input : current working state, return next working state
unsigned long state_machine(unsigned long state,
                            unsigned long ticks,
                            int shmID, P_GWS_KPI pKpi) {
    static time_t latest_stamp;
    static time_t latest_negociation;
    time_t current = time(&current);
    int ticker = 0;

//        printf("Jump to %08x\n",l);   \

#define JUMP_TO(l)    do {  \
        latest_stamp = current; \
        return l;    } while (0)
#define DELAY_TIMEOUT   1
    
    switch (state) {
        case MESH_ROUTE_STATE_INIT:
            Socket_Init(MUTLICAST_PORT);    
            JUMP_TO(MESH_ROUTE_STATE_INIT_IFACE);
        case MESH_ROUTE_STATE_INIT_IFACE:
            if (Socket_GetIP(GWS_NET_IFACE) <= DELAY_TIMEOUT )
                JUMP_TO(MESH_ROUTE_STATE_INIT_IFACE);
            JUMP_TO(MESH_ROUTE_STATE_INIT_DELAY);
        case MESH_ROUTE_STATE_INIT_DELAY:
            if (current - latest_stamp <= 10)           // without updating
                 return MESH_ROUTE_STATE_INIT_DELAY;    // latest_stamp
            JUMP_TO(MESH_ROUTE_STATE_INIT_MESHID);
        case MESH_ROUTE_STATE_INIT_MESHID:
            restore_mesh_id();
            JUMP_TO(MESH_ROUTE_STATE_INIT_ASSOC);
        case MESH_ROUTE_STATE_INIT_ASSOC:
            if (!init_assoc_list())
                JUMP_TO(MESH_ROUTE_STATE_DO_NOTHING);
            JUMP_TO(MESH_ROUTE_STATE_INIT_SOCKET);
        case MESH_ROUTE_STATE_INIT_SOCKET:
            if (!Socket_Bind(SOCK_DGRAM, MUTLICAST_PORT, static_multicast)) {
                printf("Can not bind multicast ip %s\n", static_multicast);
                JUMP_TO(MESH_ROUTE_STATE_EXIT);
            }
            set_phy_distance(route_distance);   //wlan0 is ready at this point
            JUMP_TO(MESH_ROUTE_STATE_IDEL);
        case MESH_ROUTE_STATE_IDEL:
            ticker = ticks % (route_interval * 20);
//            printf("ticks = % 6d, ticker = %d\n",ticks, ticker);
            if (ticker == 2) {    //every 1 second
                if (route_workmode & NEG_MODE_FIXED_PATH) {
                    state_assoc_list = is_assoc_list_changed(route_debug_en,&mesh_shell,shmID,pKpi);
                    JUMP_TO(MESH_ROUTE_STATE_CHECK_ASSOC);
                }
            } else if (ticker == 20) {
                if (route_workmode & NEG_MODE_BROADCAST || if_need_report)
                    broadcast_curr_route_table(route_debug_en,static_ifname,shmID,pKpi);
            } else if (route_workmode & NEG_MODE_FIXED_PATH) {
                if (is_negotiation_request(route_debug_en,shmID,pKpi,&static_online_list) ||
                    latest_negociation + MIN_NEG_BROADCAST_INTERVAL < current)
                    JUMP_TO(MESH_ROUTE_STATE_NEG_START);
            }
            JUMP_TO(MESH_ROUTE_STATE_IDEL);
        case MESH_ROUTE_STATE_CHECK_ASSOC:
            static_confirm_cnt = 0;
            switch (state_assoc_list) {
                case ASSOC_LIST_NO_CHANGE:  JUMP_TO(MESH_ROUTE_STATE_IDEL);
                case ASSOC_LIST_STA_JOIN:   JUMP_TO(MESH_ROUTE_STATE_NEG_COMMIT);
                case ASSOC_LIST_STA_NB_OFF: JUMP_TO(MESH_ROUTE_STATE_NEG_COMMIT);
                case ASSOC_LIST_STA_BACK:   JUMP_TO(MESH_ROUTE_STATE_NEG_START);
                case ASSOC_LIST_STA_LEAV:   JUMP_TO(MESH_ROUTE_STATE_NEG_INIT);
                default:;
            }
            JUMP_TO(MESH_ROUTE_STATE_IDEL);
        case MESH_ROUTE_STATE_NEG_INIT:
            do {
                int n = 0;
                for (n = 0; n < MAX_ASSOC_ENTRIES; n ++) {
                    EntryFlagClr(&static_online_list.entries[n],ASSOC_ENTRY_FLAG_ONLINE);
                }
            } while (0);
        case MESH_ROUTE_STATE_NEG_START:
            static_confirm_cnt = 5;
            neg_start_time = current;
            JUMP_TO(MESH_ROUTE_STATE_NEG_SURVEY);
        case MESH_ROUTE_STATE_NEG_SURVEY:
            JUMP_TO(negociation_survey(route_debug_en,ticks));
        case MESH_ROUTE_STATE_SURVEY_DONE:  //so far, ervery mac has it's SN, send our confirmation and check if every mac
            JUMP_TO(negociation_survey_done(route_debug_en,ticks));
        case MESH_ROUTE_STATE_NEG_COMMIT:
            if (route_commit(route_debug_en,&mesh_shell,(pKpi == NULL) ? false : true)) {
                latest_negociation = current;
                JUMP_TO(MESH_ROUTE_STATE_NEG_CONFIRM);
            } else JUMP_TO(MESH_ROUTE_STATE_NEG_SURVEY);
        case MESH_ROUTE_STATE_NEG_CONFIRM:
            if (static_confirm_cnt > 0) {  //send confirmation for more 10 seconds after negociation done
                send_response(route_debug_en,ASSOC_ENTRY_FLAG_NEGOCIATE | ASSOC_ENTRY_FLAG_CONFIRM,0);
                static_confirm_cnt --;
                JUMP_TO(MESH_ROUTE_STATE_NEG_CONFIRM);
            } else EntryFlagClr(&static_local_entry,ASSOC_ENTRY_FLAG_NEGOCIATE | ASSOC_ENTRY_FLAG_CONFIRM);
            JUMP_TO(MESH_ROUTE_STATE_NEG_CONFIRMED);
        case MESH_ROUTE_STATE_NEG_CONFIRMED:
            JUMP_TO(MESH_ROUTE_STATE_IDEL);   //negociation done !
        case MESH_ROUTE_STATE_EXIT:
            Socket_Disconnect();
            gws_mpath_exit(&static_nlstate);
        case MESH_ROUTE_STATE_DO_NOTHING:
            JUMP_TO(MESH_ROUTE_STATE_DO_NOTHING);
        default:;
    }
    return MESH_ROUTE_STATE_DO_NOTHING;
}

bool svrSetDistance(int distance) {
    route_distance = distance;
}

bool svrSetWorkMode(bool dbg, char mode) {
    char interval = 0x000000ff & (mode >> 8);
    char workmode = 0x000000ff & mode;
    if (route_interval != interval) route_interval = interval; 
    if (route_workmode != workmode) {
        route_workmode = workmode;
        SVR_DEBUG(dbg,"\nSet Working mode [%X]\n", 0x00ff & workmode);
    }
    if (route_interval < 1)
        route_interval = 2;
}

bool svrSetDebug(bool dbg) {
    route_debug_en = dbg;
}

static unsigned long static_task_state = MESH_ROUTE_STATE_INIT;
bool svrRouteHandler(unsigned long ticks,int shmID,P_GWS_KPI pKpi) {
    static_task_state = state_machine(static_task_state,ticks,shmID,pKpi);
    switch (static_task_state) {
        case MESH_ROUTE_STATE_DO_NOTHING:
        case MESH_ROUTE_STATE_IDEL:         return false;
        default:;
    }
    return true;
}