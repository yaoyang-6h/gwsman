/* 
 * File:   gwsman.c
 * Author: wyy
 *
 * Created on 2014年2月27日, 下午4:03
 */

#include <stdio.h>
#include <stdlib.h>

#define USE_NL80211
#include "iwinfo.h"
#include "shmem.h"
#include "gwsman.h"
#include "gwsmanlib.h"

#define PATH_ASSOC_LIST     "/var/run/assoc.lst"

static char * format_bssid(unsigned char *mac) {
	static char buf[18];

	snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return buf;
}

static char * format_enc_ciphers(int ciphers) {
    static char str[128] = {0};
    char *pos = str;

    if (ciphers & IWINFO_CIPHER_WEP40)
        pos += sprintf(pos, "WEP-40, ");

    if (ciphers & IWINFO_CIPHER_WEP104)
        pos += sprintf(pos, "WEP-104, ");

    if (ciphers & IWINFO_CIPHER_TKIP)
        pos += sprintf(pos, "TKIP, ");

    if (ciphers & IWINFO_CIPHER_CCMP)
        pos += sprintf(pos, "CCMP, ");

    if (ciphers & IWINFO_CIPHER_WRAP)
        pos += sprintf(pos, "WRAP, ");

    if (ciphers & IWINFO_CIPHER_AESOCB)
        pos += sprintf(pos, "AES-OCB, ");

    if (ciphers & IWINFO_CIPHER_CKIP)
        pos += sprintf(pos, "CKIP, ");

    if (!ciphers || (ciphers & IWINFO_CIPHER_NONE))
        pos += sprintf(pos, "NONE, ");

    *(pos - 2) = 0;

    return str;
}

static char * format_enc_suites(int suites) {
    static char str[64] = {0};
    char *pos = str;

    if (suites & IWINFO_KMGMT_PSK)
        pos += sprintf(pos, "PSK/");

    if (suites & IWINFO_KMGMT_8021x)
        pos += sprintf(pos, "802.1X/");

    if (!suites || (suites & IWINFO_KMGMT_NONE))
        pos += sprintf(pos, "NONE/");

    *(pos - 1) = 0;

    return str;
}

static char * format_encryption(struct iwinfo_crypto_entry *c) {
    static char buf[512];

    if (!c) {
        sprintf(buf, "unknown");
    } else if (c->enabled) {
        /* WEP */
        if (c->auth_algs && !c->wpa_version) {
            if ((c->auth_algs & IWINFO_AUTH_OPEN) &&
                    (c->auth_algs & IWINFO_AUTH_SHARED)) {
                sprintf(buf, "WEP Open/Shared (%s)",
                        format_enc_ciphers(c->pair_ciphers));
            } else if (c->auth_algs & IWINFO_AUTH_OPEN) {
                sprintf(buf, "WEP Open System (%s)",
                        format_enc_ciphers(c->pair_ciphers));
            } else if (c->auth_algs & IWINFO_AUTH_SHARED) {
                sprintf(buf, "WEP Shared Auth (%s)",
                        format_enc_ciphers(c->pair_ciphers));
            }
        }
            /* WPA */
        else if (c->wpa_version) {
            switch (c->wpa_version) {
                case 3:
                    sprintf(buf, "mixed WPA/WPA2 %s (%s)",
                            format_enc_suites(c->auth_suites),
                            format_enc_ciphers(c->pair_ciphers | c->group_ciphers));
                    break;
                case 2:
                    sprintf(buf, "WPA2 %s (%s)",
                            format_enc_suites(c->auth_suites),
                            format_enc_ciphers(c->pair_ciphers | c->group_ciphers));
                    break;
                case 1:
                    sprintf(buf, "WPA %s (%s)",
                            format_enc_suites(c->auth_suites),
                            format_enc_ciphers(c->pair_ciphers | c->group_ciphers));
                    break;
            }
        } else {
            sprintf(buf, "none");
        }
    } else {
        sprintf(buf, "none");
    }

    return buf;
}

static char * format_rate(int rate) {
    static char buf[14];

    if (rate <= 0) {
        sprintf(buf, "unknown");
    } else {
        sprintf(buf, "%d.%d", rate / 4000, (rate % 4000) / 400);
    }

    return buf;
}

static char * format_mcs(struct iwinfo_rate_entry *r,struct iwinfo_rate_entry *t) {
    static char buf[64];
    char *p = buf;
    int l = sizeof (buf);

    if (r->mcs >= 0) {
        p += snprintf(p, l, "%d", r->mcs);
        l = sizeof (buf) - (p - buf);

        if (r->is_short_gi)
            p += snprintf(p, l, "s");
    } else {
        p += snprintf(p, l, "%s", "NA");
        l = sizeof (buf) - (p - buf);
    }
    p += snprintf(p, l, "%s", "/");
    l = sizeof (buf) - (p - buf);
    if (t->mcs >= 0) {
        p += snprintf(p, l, "%d", t->mcs);
        l = sizeof (buf) - (p - buf);

        if (t->is_short_gi)
            p += snprintf(p, l, "s");
    } else {
        p += snprintf(p, l, "%s", "NA");
        l = sizeof (buf) - (p - buf);
    }
    return buf;
}

static char * format_bps(struct iwinfo_rate_entry *r,struct iwinfo_rate_entry *t) {
    static char buf[64];
    char *p = buf;
    int l = sizeof (buf);

    if (r->rate > 0) {
        p += snprintf(p, l, "%s", format_rate(r->rate));
        l = sizeof (buf) - (p - buf);
    } else {
        p += snprintf(p, l, "%s", "NA");
        l = sizeof (buf) - (p - buf);
    }
    p += snprintf(p, l, "%s", "/");
    l = sizeof (buf) - (p - buf);
    if (t->rate > 0) {
        p += snprintf(p, l, "%s", format_rate(t->rate));
        l = sizeof (buf) - (p - buf);
    } else {
        p += snprintf(p, l, "%s", "NA");
        l = sizeof (buf) - (p - buf);
    }

    return buf;
}

static char * format_throughput_rx(int kb) {
    static char buf[32];
    
    memset (buf,0x00,32);
    if (kb < 1000) sprintf(buf,"%dKb",kb);
    else sprintf(buf,"%2.1fMb",(float)kb/1000);
    return buf;
}

static char * format_throughput_tx(int kb) {
    static char buf[32];
    
    memset (buf,0x00,32);
    if (kb < 1000) sprintf(buf,"%dKb",kb);
    else sprintf(buf,"%2.1fMb",(float)kb/1000);
    return buf;
}

static char * format_online_time(unsigned int online) {
    static char buf[32];
    
    memset (buf,0x00,32);
    int hour = (online / 3600);
    int min = (online % 3600) / 60;
    int sec = online % 60;
    sprintf(buf,"%02d:%02d:%02d",hour,min,sec);
    return buf;
}

static char * format_loss_packets(unsigned int delta_tx_failed,unsigned int delta_tx_packets) {
    static char buf[32];
    
    memset (buf,0x00,32);
        
    if (0 <= delta_tx_failed && delta_tx_failed < delta_tx_packets) {
        sprintf(buf,"%2.1f%%",100.0 * delta_tx_failed / delta_tx_packets);
    } else sprintf(buf,"%d",delta_tx_failed);

    return buf;
}

static char * format_snr(int sig,int noise) {
    static char buf[32];

    memset (buf,0x00,32);
    sprintf (buf, "%d/%d/%d", sig,noise,sig-noise);
    return buf;
}

#define VT100_FLAG0             "\x1b"          //ESC
#define VT100_FLAG1             "["
#define VT100_FLAG              VT100_FLAG0 VT100_FLAG1
#define VT100_HEAD              VT100_FLAG
#define VT100_COLOR(b,f)        VT100_HEAD b ";" f "m"                  //
#define VT100_B_BLACK           "40"    //黑
#define VT100_F_RED             "31"    //红
#define DISP_ATTENTION          VT100_COLOR(VT100_B_BLACK,VT100_F_RED)

#define MAX_INACTIVE_MS         10000   //10 s
#define MID_INACTIVE_MS         5000    //5 s

static char * format_timer(int timer) {
    static char buf[32];

    memset (buf,0x00,32);
    if (timer < MID_INACTIVE_MS) sprintf (buf, "% 2d", timer/1000);
    else sprintf (buf, DISP_ATTENTION "% 2d", timer/1000);
    return buf;
}

/*
(VO):  qnum: 0 qdepth:  0 ampdu-depth:  0 pending:   0 stopped: 0
(VI):  qnum: 1 qdepth:  0 ampdu-depth:  0 pending:   0 stopped: 0
(BE):  qnum: 2 qdepth:  0 ampdu-depth:  0 pending:   0 stopped: 0
(BK):  qnum: 3 qdepth:  0 ampdu-depth:  0 pending:   0 stopped: 0
(CAB): qnum: 8 qdepth:  0 ampdu-depth:  0 pending:   0 stopped: 0
*/
static unsigned char get_queue_length() {
    FILE* fd = fopen(DEBUG_INFO_BASE DEBUG_INFO_QUEUES,"r");
    char    buf[128];
    int     qlen = 0;
    
    if (fd) {
        memset (buf,0x00,128);
        while (fgets(buf,127,fd)) {
            char* pLine = strstr(buf,"(BE)");
            if (pLine >= buf) {
                char* pValue = strstr(pLine,DEBUG_INFO_QLEN_KEY);
                if (pValue > pLine) {
                    pValue += strlen(DEBUG_INFO_QLEN_KEY);
                    sscanf(pValue,"%d",&qlen);
                    break;
                }
            }
        }
        fclose(fd);
    }
    return qlen;
}

#define MAX_QUEUE_LEN   123     //512 / 4 - 5
static uint32_t last_tx_packets = 0;
static uint32_t last_tx_failed = 0;
static uint32_t last_tx_bytes = 0;
static uint32_t last_rx_bytes = 0;
static struct timeval last_count_time = {0,0};
static char * format_monitor(struct iwinfo_assoclist_entry *e,bool *isAlive) {
    static char buf[256];
    
    *isAlive = false;
    memset (buf,0x00,256);
    if (NULL == e) {
        sprintf(buf,"Station|Rssi/Nf/Snr|na|MCS r/t|Mbps r/t |load| online |R rate|T rate");
    } else {
//        unsigned char   qlen = 0x000000ff & e->drop_qlen;
//        unsigned int    drop = 0x00fffff & (e->drop_qlen >> 24);
        unsigned int    drop = 0;
        unsigned char   qlen = get_queue_length();
        uint32_t delta_tx_packets = e->tx_packets - last_tx_packets;
        uint32_t delta_tx_failed = (drop + e->tx_failed) - last_tx_failed;
        int     tx_KBps = 0;
        int     rx_KBps = 0;
        struct timeval current_time;
        
        gettimeofday (&current_time,NULL);
        time_t diff_in_ms = (current_time.tv_sec - last_count_time.tv_sec) * 1000 +
                            (current_time.tv_usec - last_count_time.tv_usec) / 1000;
        last_count_time.tv_sec = current_time.tv_sec;
        last_count_time.tv_usec = current_time.tv_usec;
        
        last_tx_packets = e->tx_packets;
        last_tx_failed = drop + e->tx_failed;
        if (diff_in_ms > 0) {
            if (last_rx_bytes < e->rx_bytes) rx_KBps = (e->rx_bytes - last_rx_bytes) / diff_in_ms;
            if (last_tx_bytes < e->tx_bytes) tx_KBps = (e->tx_bytes - last_tx_bytes) / diff_in_ms;
            last_rx_bytes = e->rx_bytes;
            last_tx_bytes = e->tx_bytes;
        }
        *isAlive = (e->inactive <= MAX_INACTIVE_MS) ? true : false;
        sprintf(buf,"%s|% 11s|%s|% 7s|% 9s|%3d%%|%8s|% 6s|% 6s",
                format_bssid(e->mac),
                format_snr(e->signal,e->noise),
                format_timer(e->inactive),
                format_mcs(&e->rx_rate,&e->tx_rate),
                format_bps(&e->rx_rate,&e->tx_rate),
                100 * qlen / MAX_QUEUE_LEN,
                format_online_time(e->t_connect),
                format_throughput_rx(8 * rx_KBps),
                format_throughput_tx(8 * tx_KBps)
                );
    }
    return buf;
}

void iw_setchannel(int nChannel, P_GWS_KPI pKpi, int nId) {
    int region = 0;
    short chan = nChannel;
    RW_KPI_VAR(nId,region,pKpi->m_radio.m_nRegion);
    if (MIN_CHANNEL(region) <= chan && chan <= MAX_CHANNEL(region)) {
        RW_KPI_VAR(nId,pKpi->m_currchannel,chan);
    } else if (-1 == chan) {
        RW_KPI_VAR(nId,pKpi->m_currchannel,chan);
    } else {
        RW_KPI_VAR(nId,pKpi->m_currchannel,MIN_CHANNEL(region));
    }
}

short iw_getcurrchan(P_GWS_KPI pKpi, int nId) {
    short chan = 0;
    RW_KPI_VAR(nId, chan, pKpi->m_currchannel);
    return chan;
}

#define MAX_AVG     1   //0 -- MAX, 1 -- AVG
#define MAX_NOISE(x,y)  (x > y ? x : y)
#define MIN_NOISE(x,y)  (x < y ? x : y)
#define AVG_NOISE(_avg,_new,_n) (((_n - 1) * _avg + _new) / _n)
#define NF_EXCHANGE(x,y)    do {    \
            int temp = x;           \
            x = y;                  \
            y = temp;               \
        } while (0)
#define NF_CHECK_MAX(max,cur)  do {                 \
            if (cur > max) NF_EXCHANGE(max,cur);    \
        } while (0)
#define NF_CHECK_MIN(min,cur)  do {                 \
            if (cur < min) NF_EXCHANGE(min,cur);    \
        } while (0)

typedef struct __MaxMinNoiseFloor {
    short max;
    short min;
} MAX_MIN_NOISE;

#define MAX_NOISE_FLOOR     -80 //dBm
//according to old avg, make curr_nf to new avg, then save the new avg
#define NF_COUNT_AVG(max,curr_nf,min,nSamples)  \
do {\
    double avg = 0.0f;\
    nSamples ++;\
    NF_CHECK_MAX(max,curr_nf);\
    NF_CHECK_MIN(min,curr_nf);\
    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_max,max);\
    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_min,min);\
    RW_KPI_VAR(nId,avg,pKpi->m_noise_floor[chan_idx].m_noise_avg);\
    curr_nf = AVG_NOISE(avg,curr_nf,nSamples);\
    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_avg,curr_nf);\
} while (0)
//the max and min noise will not counted
int iw_noise(const char* ifname, int nChannel, P_GWS_KPI pKpi, int nId) {   //ifname == NULL means reset
    const struct iwinfo_ops* iw = &nl80211_ops;
    int region = 0;
    
    RW_KPI_VAR(nId,region,pKpi->m_radio.m_nRegion);
    if (MIN_CHANNEL(region) <= nChannel && nChannel <= MAX_CHANNEL(region)) {
        static int nSamples;
        int chan_idx = nChannel-MIN_CHANNEL(region);    // 0 based channel index
        if (ifname) {
            int curr = 0;
            iw->noise(ifname, &curr);
            RW_KPI_VAR(nId,pKpi->m_wifi.m_nNoise,curr);
#if (MAX_AVG == 1)
            do {
                int curr_nf = curr;
                char max = 0, min = 0;
                RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_cur,curr);
                RW_KPI_VAR(nId,max,pKpi->m_noise_floor[chan_idx].m_noise_max);
                RW_KPI_VAR(nId,min,pKpi->m_noise_floor[chan_idx].m_noise_min);
                if (0 == max) { //no max value at first time
                    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_max,curr_nf);
                    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_avg,curr_nf);
                } else if (0 == min) { //no min value yet
                    NF_CHECK_MAX(max,curr_nf);
                    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_max,max);
                    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_avg,curr_nf);
                    RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_min,curr_nf);
//                } else if (max > MAX_NOISE_FLOOR) { //interference exsit
//                    if (curr_nf > MAX_NOISE_FLOOR) {    //only noise above max NF will counted
//                        NF_COUNT_AVG(max,curr_nf,min,nSamples);
//                    }
                } else {    //already have a max value AND a min value
                    NF_COUNT_AVG(max,curr_nf,min,nSamples);
                }
            } while (0);
            return curr;    //give the max value and return
#elif (MAX_AVG == 2)
            do { //ignore the max value and the min value, get the avg value of the rest
                int curr_nf = curr;
                RW_KPI_VAR(nId,avg,pKpi->m_noise_list[chan_idx]);   //pick up average noise
                if (0 == static_max_min_nf[chan_idx].max) { //no max value first time
                    static_max_min_nf[chan_idx].max = curr;
                    RW_KPI_VAR(nId,pKpi->m_noise_list[chan_idx],curr_nf);
                    return curr_nf;    //give the max value and return
                } else {    //already have a max value, update it if needed and keep the smaller one in curr
                    NF_CHECK_MAX(static_max_min_nf[chan_idx].max,curr);
                    if (0 == static_max_min_nf[chan_idx].min) { //no min value yet
                        static_max_min_nf[chan_idx].min = curr;
                        RW_KPI_VAR(nId,pKpi->m_noise_list[chan_idx],curr_nf);
                        return curr_nf;    //give the min value and return
                    } else {    //already have a max value AND a min value
                        nSamples ++;    //update min value if needed and keep the bigger one in curr
                        NF_CHECK_MIN(static_max_min_nf[chan_idx].min,curr);
                        avg = AVG_NOISE(avg,curr,nSamples); //now curr is between max & min, add it to avg
                        //save the avg value without the max and the min counted
                        RW_KPI_VAR(nId,pKpi->m_noise_list[chan_idx],(int)avg);
                        return curr_nf;    //return current nf
                    }
                }
            } while (0);
#else
            do {
                curr = MAX_NOISE(pKpi->m_noise_list[chan_idx],curr);
                RW_KPI_VAR(nId,pKpi->m_noise_list[chan_idx],curr);
                return curr;
            } while (0);
#endif
        } else {
            nSamples = 0;
            RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_max,0);
            RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_min,0);
            RW_KPI_VAR(nId,pKpi->m_noise_floor[chan_idx].m_noise_avg,MIN_CHANNEL_NOISE);
            return MIN_CHANNEL_NOISE;
        }
    }
    return 0;
}

char* iwinfo(const char* ifname, int key, P_GWS_KPI pKpi,int nId) {
    const struct iwinfo_ops* iw = &nl80211_ops;
    int nValue = -1;
    static char sValue[IW_LIST_MAXSIZE];

    if (NULL == pKpi) return sValue;
    if (NULL == ifname) return sValue;
    memset (sValue, 0x00, IW_LIST_MAXSIZE);
    switch (key) {
        case WIFI_CHANNEL:
//            if (iw->channel(ifname, &nValue) >= 0) {
            if ((nValue = get_wifi_bandwidth())) {
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nChannel,nValue & 0x000000ff);
                sprintf(sValue,"%d",nValue);
            }
            break;
        case WIFI_FREQUENCY:
            if (iw->frequency(ifname, &nValue) >= 0) {
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nFrequency,nValue);
                sprintf(sValue,"%d",nValue);
            }
            break;
        case WIFI_FREQUENCY_OFFSET:
            if (iw->frequency_offset(ifname, &nValue) >= 0) {
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nFrequencyOffset,nValue);
                sprintf(sValue,"%d",nValue);
            }
            break;
        case WIFI_TXPOWER:
            if (iw->txpower(ifname, &nValue) >= 0) {
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nTxPower,nValue);
                sprintf(sValue,"%d",nValue);
            }
            break;
        case WIFI_TXPOWER_OFFSET:
            if (iw->txpower_offset(ifname, &nValue) >= 0) {
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nTxPowerOffset,nValue);
                sprintf(sValue,"%d",nValue);
            }
            break;
        case WIFI_BITRATE:
            if (iw->bitrate(ifname, &nValue) >= 0) {
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nBitRate,nValue);
                sprintf(sValue,"%d",nValue);
            }
            break;
        case WIFI_SIGNAL:
            do {
                int nAssoc = 0;
                RW_KPI_VAR(nId,nAssoc,pKpi->m_assoc_list.m_nLine);
                if (nAssoc > 0 && iw->signal(ifname, &nValue) == 0) {
                    RW_KPI_VAR(nId,pKpi->m_wifi.m_nSignal,nValue);
                    sprintf(sValue,"%d",nValue);
                }
            } while (0);
            break;
        case WIFI_NOISE:
            if (iw->noise(ifname, &nValue) >= 0) {
                int chan = 0;
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nNoise,nValue);
                RW_KPI_VAR(nId,chan, pKpi->m_currchannel);
                sprintf(sValue,"%d",nValue);
//                if (MIN_CHANNEL <= chan && chan <= MAX_CHANNEL) {
//                    nValue = MAX_NOISE(pKpi->m_noise_list[chan-MIN_CHANNEL],nValue);
//                    RW_KPI_VAR(nId,pKpi->m_noise_list[chan-MIN_CHANNEL],nValue);
//                }
            }
            break;
        case WIFI_QUALITY:
        case WIFI_QUALITY_MAX:
            if (iw->quality(ifname, &nValue) >= 0) {
                int vmax = 0;
                iw->quality_max(ifname, &vmax);
                sprintf(sValue, "%d/%d", nValue, vmax);
                RW_KPI_STR(nId,pKpi->m_wifi.m_sQuality,sValue);
            }
            break;
        case WIFI_MBESSID_SUPPORT:
            if (iw->mbssid_support(ifname, &nValue) >= 0) {
                sprintf(sValue, "%d", nValue);
                RW_KPI_VAR(nId,pKpi->m_wifi.m_nMbssid_support,nValue);
            }
            break;
        case WIFI_MODE:
            if (iw->mode(ifname, &nValue) >= 0) {
                sprintf(sValue, "%s", IWINFO_OPMODE_NAMES[nValue]);
                RW_KPI_STR(nId,pKpi->m_wifi.m_sMode,sValue);
            }
            break;
        case WIFI_SSID:
            if (0 == iw->ssid(ifname, sValue)) {
                RW_KPI_STR(nId,pKpi->m_wifi.m_sSSID,sValue);
            }
            break;
        case WIFI_BSSID:
            if (iw->bssid(ifname, sValue)) {
                sprintf(sValue, "00:00:00:00:00:00");
            }
            RW_KPI_STR(nId,pKpi->m_wifi.m_sBSSID,sValue);
            break;
            //        case WIFI_COUNTRY:      iw->country(ifname,result);             break;
        case WIFI_HARDWARE_ID:
            do {
                struct iwinfo_hardware_id ids;

                if (!iw->hardware_id(ifname, (char *) &ids)) {
                    sprintf(sValue, "%04X:%04X %04X:%04X",
                            ids.vendor_id, ids.device_id,
                            ids.subsystem_vendor_id, ids.subsystem_device_id);
                } else {
                    sprintf(sValue, "unknown");
                }
                RW_KPI_STR(nId,pKpi->m_wifi.m_sHardwareID,sValue);
            } while (0);
            break;
        case WIFI_HARDWARE_NAME:
            if (iw->hardware_name(ifname, sValue)) {
                sprintf(sValue, "unknown");
            }
            RW_KPI_STR(nId,pKpi->m_wifi.m_sHardwareName,sValue);
            break;
        case WIFI_ENCRYPTION:
            do {
                struct iwinfo_crypto_entry c = {0};
                if (iw->encryption(ifname, (char *) &c)) { //error
                    strcpy(sValue, "unknown");
                } else {
                    strcpy(sValue, format_encryption(&c));
                }
                RW_KPI_STR(nId,pKpi->m_wifi.m_sEncryption,sValue);
            } while (0);
            break;
        case WIFI_HWMODELIST:
            if (iw->hwmodelist(ifname, &nValue)) {
                sprintf(sValue, "unknown");
            } else {
                sprintf(sValue, "802.11%s%s%s%s",
                        (nValue & IWINFO_80211_A) ? "a" : "",
                        (nValue & IWINFO_80211_B) ? "b" : "",
                        (nValue & IWINFO_80211_G) ? "g" : "",
                        (nValue & IWINFO_80211_N) ? "n" : "");
            }
            RW_KPI_STR(nId,pKpi->m_wifi.m_sHwModeList,sValue);
            break;
        case WIFI_ASSOCLIST:
            do {
                struct iwinfo_assoclist_entry *e;
                char    buff[IWINFO_BUFSIZE];
                const int ENTRY_SIZE = sizeof (struct iwinfo_assoclist_entry);
                int i,j,len = 0, connections = 0;
                iw->assoclist(ifname, buff, &len);
                len = len / ENTRY_SIZE;
                len = (len < IW_LIST_MAXLINE) ? len : IW_LIST_MAXLINE - 1;
                
                FILE* fd_assoc = fopen(PATH_ASSOC_LIST,"w");
                if (len <= 0) { //no connection, set signal to 0
                    RW_KPI_VAR(nId,pKpi->m_wifi.m_nSignal,0);
                    RW_KPI_VAR(nId,pKpi->m_assoc_list.m_nLine,0);
                    for (i = 0; i < IW_LIST_MAXLINE; i ++) {
//                        RW_KPI_VAR(nId,pKpi->m_assoc_list.m_index[i],0);
                        RW_KPI_VAR(nId,pKpi->m_assoc_list.m_value[i][0],0);
                    }
                } else {
                    bool isAlive = false;
                    sprintf(sValue,"%s",format_monitor(NULL,&isAlive));  //sValue is reserved for title
                    RW_KPI_STR(nId,pKpi->m_assoc_list.m_header,sValue);
                    for (i = 0; i < IW_LIST_MAXLINE; i ++) {
//                        RW_KPI_VAR(nId,pKpi->m_assoc_list.m_index[i],0);
                        RW_KPI_VAR(nId,pKpi->m_assoc_list.m_value[i][0],0);
                    }
                    for (i = 0,j = 0; i < len; i ++) {
                        e = (struct iwinfo_assoclist_entry *) (buff + i * ENTRY_SIZE);
                        memset (sValue, 0x00, IW_LIST_MAXSIZE);
                        sprintf(sValue,"%s",format_monitor(e,&isAlive));
                        if (isAlive) {
                            RW_KPI_STR(nId,pKpi->m_assoc_list.m_value[j],sValue);
                            if (fd_assoc) fprintf(fd_assoc,"%s\n",sValue);
                            connections ++;
                            j ++;
                        }
                    }
                    if (0 == connections) RW_KPI_VAR(nId,pKpi->m_wifi.m_nSignal,0);
                    RW_KPI_VAR(nId,pKpi->m_assoc_list.m_nLine,connections);
                }
                if (fd_assoc) fclose(fd_assoc);
            } while (0);
            break;
        case WIFI_TXPWRLIST:
            do {
                struct iwinfo_txpwrlist_entry *e;
                char    buff[IWINFO_BUFSIZE];
                const int ENTRY_SIZE = sizeof (struct iwinfo_txpwrlist_entry);
                int i,len = 0;
                iw->txpwrlist(ifname, buff, &len);
                len = len / ENTRY_SIZE;
                len = (len < IW_LIST_MAXLINE) ? len : IW_LIST_MAXLINE - 1;
                for (i = 0; i < len; i ++ ) {
                    e = (struct iwinfo_txpwrlist_entry *) (buff + i * ENTRY_SIZE);
//                    sprintf(result->m_value[i],"%s",format_monitor(e));
                }
            } while (0);
            break;
        case WIFI_SCANLIST:
            do {
                struct iwinfo_scanlist_entry *e;
                char    buff[IWINFO_BUFSIZE];
                const int ENTRY_SIZE = sizeof (struct iwinfo_scanlist_entry);
                int i,len = 0;
                iw->scanlist(ifname, buff, &len);
                len = len / ENTRY_SIZE;
                len = (len < IW_LIST_MAXLINE) ? len : IW_LIST_MAXLINE - 1;
                for (i = 0; i < len; i ++ ) {
                    e = (struct iwinfo_scanlist_entry *) (buff + i * ENTRY_SIZE);
//                    sprintf(result->m_value[i],"%s",format_monitor(e));
                }
            } while (0);
            break;
        case WIFI_FREQLIST:
            do {
                struct iwinfo_freqlist_entry *e;
                char    buff[IWINFO_BUFSIZE];
                const int ENTRY_SIZE = sizeof (struct iwinfo_freqlist_entry);
                int i,len = 0;
                iw->freqlist(ifname, buff, &len);
                len = len / ENTRY_SIZE;
                len = (len < IW_LIST_MAXLINE) ? len : IW_LIST_MAXLINE - 1;
                for (i = 0; i < len; i ++ ) {
                    e = (struct iwinfo_freqlist_entry *) (buff + i * ENTRY_SIZE);
//                    sprintf(result->m_value[i],"%s",format_monitor(e));
                }
            } while (0);
            break;
        case WIFI_COUNTRYLIST:
            do {
                struct iwinfo_country_entry *e;
                char    buff[IWINFO_BUFSIZE];
                const int ENTRY_SIZE = sizeof (struct iwinfo_country_entry);
                int i,len = 0;
                iw->countrylist(ifname, buff, &len);
                len = len / ENTRY_SIZE;
                len = (len < IW_LIST_MAXLINE) ? len : IW_LIST_MAXLINE - 1;
                for (i = 0; i < len; i ++ ) {
                    e = (struct iwinfo_country_entry *) (buff + i * ENTRY_SIZE);
//                    sprintf(result->m_value[i],"%s",format_monitor(e));
                }
            } while (0);
            break;
        default: return sValue;
    }
    return sValue;
}
