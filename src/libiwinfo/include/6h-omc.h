//**
//* REV_Alpha_01
//*
//* print_kpi(), reserved for OMC-KPI
//* 1. Print version; 2. Print Radio link info.
//*
//* Based on Yu's print_bts() & print_cpe() code;
//* Modified by Qige from 6Harmonics Inc. - 6WiLink Inc.
//* qige.zhao@6wilink.com; qigezhao@gmail.com
//* 2014.03.05
//**

typedef unsigned int 	uint;
typedef unsigned char 	uchar;


#define GWS_KPI_CMD_MAX_LEN	128
#define GWS_KPI_BUF_MAX_LEN	1024

#define GWS_KPI_KEYFILE		"/var/dk2_1"
#define GWS_KPI_RADIO_IF	"wlan0"
#define GWS_KPI_IP_IF		"br-lan"


//#include <sys/socket.h>	// inet_ntoa()
//#include <netinet/in.h>	// inet_ntoa()
#include <arpa/inet.h>		// inet_ntoa()


//* Save KPI data into share memory
struct gws_radio_kpi {

	char radio[GWS_KPI_BUF_MAX_LEN]; 		//char assoc[GWS_KPI_BUF_MAX_LEN];

};


//* READ FROM PIPE
static char *sysread(const char *cmd) {

	static char buff[GWS_KPI_BUF_MAX_LEN];
	size_t buff_len, read_len;

	buff_len = sizeof(buff);
	memset(buff, 0, buff_len);


	FILE *stream;

	stream = popen(cmd, "r");
	read_len = fread(buff, sizeof(char), buff_len, stream);

	if (read_len < buff_len) {
		fputs("-- more prompt unread\n", stderr);
	}

	pclose(stream);


	/*printf("[%s]\n", buff);

	char *p;
	while(p = strchr(buff, ' ')) {
		*p = '\n';
	}*/


	return buff;
}



//* TOUCH FILE
void keyfile_init(const char *file) {

	char cmd[GWS_KPI_CMD_MAX_LEN];
	memset(cmd, 0, sizeof(cmd));

	sprintf(cmd, "touch %s", file); //printf("- [%s]\n", cmd);
	system(cmd);

}



//* GET IP ADDRESS
static char *print_ip(const char *if_name, int fd) {

	static char ip[16];

	struct ifreq ifr;
	size_t if_name_len;
	struct sockaddr_in *ipaddr;

	if_name_len = strlen(if_name);
	if (if_name_len < sizeof(ifr.ifr_name)) {
		sprintf(ifr.ifr_name, "%s", if_name);
		ifr.ifr_name[if_name_len] = 0;
	} else {
		fputs("ERR: interface name is too long\n", stderr); exit(EXIT_FAILURE);
	}

	//int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		fputs("ERR: create socket failed\n", stderr); //exit(EXIT_FAILURE);
	}


	if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
		fputs("ERR: get ip address failed\n", stderr); //exit(EXIT_FAILURE);
	}
	ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
	//printf("IP Address is %s\n", inet_ntoa(ipaddr->sin_addr));
	sprintf(ip, "%s", inet_ntoa(ipaddr->sin_addr));

	//close(fd);


	return ip;
}



//* GET BROADCAST ADDRESS
static char *print_broadcast(const char *if_name, int fd) {

	static char ip[16];

	struct ifreq ifr;
	size_t if_name_len;
	struct sockaddr_in *ipaddr;


	if_name_len = strlen(if_name);
	if (if_name_len < sizeof(ifr.ifr_name)) {
		sprintf(ifr.ifr_name, "%s", if_name);
		ifr.ifr_name[if_name_len] = 0;
	} else {
		fputs("ERR: interface name is too long\n", stderr); exit(EXIT_FAILURE);
	}


	//int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		fputs("ERR: create socket failed\n", stderr); //exit(EXIT_FAILURE);
	}

	if (ioctl(fd, SIOCGIFBRDADDR, &ifr) == -1) {
		fputs("ERR: get broadcast address failed\n", stderr); //exit(EXIT_FAILURE);
	}
	ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
	//printf("Broadcast Address is %s\n", inet_ntoa(ipaddr->sin_addr));
	sprintf(ip, "%s", inet_ntoa(ipaddr->sin_addr));

	//close(fd);


	return ip;
}



static char *print_mac(const char *if_name, int fd) {

	static char mac_addr[18] = { 0 };

	struct ifreq ifr;
	size_t if_name_len;
	struct sockaddr_in *ipaddr;

	if_name_len = strlen(if_name);
	if (if_name_len < sizeof(ifr.ifr_name)) {
		sprintf(ifr.ifr_name, "%s", if_name);
		ifr.ifr_name[if_name_len] = 0;
	} else {
		fputs("ERR: interface name is too long\n", stderr); exit(EXIT_FAILURE);
	}


	// Create an ifreq structure for passing data in and out of ioctl
	// Provide an open socket descriptor with the address family AF_INET
	//int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		//perror(strerror(errno));
		fputs("create socket failed", stderr);
	}

	// Invoke ioctl
	// Get MAC
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
		fputs("cannot get mac address", stderr);
	}

	ipaddr = (struct ifreq*)&ifr;
	sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
			(uchar) ifr.ifr_hwaddr.sa_data[0],
			(uchar) ifr.ifr_hwaddr.sa_data[1],
			(uchar) ifr.ifr_hwaddr.sa_data[2],
			(uchar) ifr.ifr_hwaddr.sa_data[3],
			(uchar) ifr.ifr_hwaddr.sa_data[4],
			(uchar) ifr.ifr_hwaddr.sa_data[5]);
	//printf("MAC address: %s\n", mac_addr);

	//close(fd);


	return mac_addr;
}








//**
//* print KPI in shm in .ini format
//* 2014.03.05
//**

static void print_kpi_shm_r2_0(const struct iwinfo_ops *iw, const char *ifname) {

	//* SHARE MEMORY
	key_t key;
	int shmid;
	struct gws_radio_kpi *shm;

	// init keyfile
	keyfile_init(GWS_KPI_KEYFILE);
	key = ftok(GWS_KPI_KEYFILE, 1);

	// connect shm
	if ((shmid = shmget(key, sizeof(shm), 0666 | IPC_CREAT)) < 0) {
		perror("KPI shmget error.\n");
		exit(EXIT_FAILURE);
	}
	if ((shm = shmat(shmid, (void *)0, 0)) == (char *) -1) {
		perror("KPI shmat error.\n");
		exit(EXIT_FAILURE);
	}

	printf("Memory attached at %X\n", (int) shm);
	printf("-------- -------- -------- --------\n");


	// WRITE SHARED DATA
	int i, len, qty = 0; char *q;
	struct iwinfo_assoclist_entry *e;
	struct iwinfo_rate_entry *t, *r;


	char radio_mac[18], br_mac[18], br_ip[16];
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	sprintf(radio_mac, "%s", print_mac(ifname, fd));
	sprintf(br_mac, "%s", print_mac(GWS_KPI_IP_IF, fd));
	sprintf(br_ip, "%s", print_ip(GWS_KPI_IP_IF, fd));

	close(fd);


	char buffer[GWS_KPI_BUF_MAX_LEN], block[GWS_KPI_BUF_MAX_LEN], tmp[GWS_KPI_BUF_MAX_LEN];
	time_t timep; 	//* LOG SEND TIME STAMP
	struct tm *p;
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

	for(;;) {

		memset(buffer, 0, strlen(buffer));
		memset(block, 0, strlen(block));
		memset(tmp, 0, strlen(tmp));

		sprintf(block, "[version]\nv=1.0.2-1\nr=140305\n");

		strcat(buffer, block);


		//* PRINT LOCAL RADIO #0
		memset(block, 0x0, sizeof(block));
		memset(tmp, 0x0, sizeof(tmp));


		snprintf(block, sizeof(block), "[radio0]\nmac=%s\nbrmac=%s\nip=%s\nmode=%s\nbssid=%s\nnetwork=%s\nlinkq=%s/%s\nsignal=%s\nnoise=%s\nbitrate=%s\n", 
			radio_mac, br_mac, br_ip, print_mode(iw, ifname),
			print_bssid(iw, ifname), print_ssid(iw, ifname),
			print_quality(iw, ifname), print_quality_max(iw, ifname),
			print_signal(iw, ifname), print_noise(iw, ifname),
			print_rate(iw, ifname)
		);

		strcat(buffer, block);


		//* PEERS OF RADIO #0
		if (iw->assoclist(ifname, tmp, &len)) {
			fputs("NOTE: No peer info available\n", stderr);
		} else if (len <= 0) {
			fputs("NOTE: No peer connected\n", stderr);
		} else {
			for (i = 0; i < len; i += sizeof(struct iwinfo_assoclist_entry)) {

				memset(block, 0, sizeof(block));

				e = (struct iwinfo_assoclist_entry *) &tmp[i];
				t = &e->tx_rate;
				r = &e->rx_rate;

				snprintf(block, sizeof(block), "[peer%d]\nbssid=%s\nsignal=%s\nnoise=%s\ninactive=%d\ntx=%d.%d,%d,%d\nrx=%d.%d,%d,%d\n", 
					i, 
					format_bssid(e->mac),
					format_signal(e->signal), format_noise(e->noise),
					e->inactive,
					t->rate / 1024 / 4,		// format_rate for a quarter freq
					t->rate % 1024 / 100 / 4,
					t->mcs,
					e->tx_packets,
					r->rate / 1024 / 4,		// format_rate for a quarter freq
					r->rate % 1024 / 100 / 4,
					r->mcs,
					e->rx_packets
				);

				strcat(buffer, block);
			}
		}


		//* COPY TO SHARE MEMORY	//buffer[strlen(buffer)] = '\0'; //buffer[strlen(buffer)] = NULL;
		memset(shm->radio, 0, strlen(shm->radio));
		strncpy(shm->radio, buffer, strlen(buffer));

		printf("%s\n", buffer);
		printf("------------------------------------------------------------------\n");

		sleep(1);
	}

	if (shmdt(shm) == -1) {
		perror("KPI shmdt failed.\n"); exit(EXIT_FAILURE);
	}
	
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
		perror("KPI shmctl(IPC_RMID) failed.\n"); exit(EXIT_FAILURE);
	}


	return;
	
} //* END OF print_kpi_shm_r2_0 //*




//* WRAPPER OF KPI PRINT FOR DIFFERENT VERSION
static void print_kpi_sm(const struct iwinfo_ops *iw, const char *ifname) {

	print_kpi_shm_r2_0(iw, ifname);		// iOMC_r2.x
	//print_kpi_sm(iw, ifname);		// iOMC_r1.x

}




//* TURN ON/DOWN LED
void sw_flash_led(const unsigned int led_num, const unsigned int status) {

	char cmd[GWS_KPI_BUF_MAX_LEN], led[GWS_KPI_BUF_MAX_LEN];

	sprintf(led, "/sys/class/leds/rb4xx:green:led%d/brightness", led_num);
	sprintf(cmd, "echo %d >%s", status, led);

	system(cmd);
}


//**
//* Signal mapping to LEDs status;
//*
//* LED5 LED4 LED3 LED2 LED1 VALUE    STATUS
//*  0    b    1    1    1    >60      Too strong signal;
//*  0    1    1    1    1    >45      Strong signal;
//*  0    0    1    1    1    >30      Normal signal;
//*  0    0    0    1    1    >15      Low signal;
//*  0    0    0    0    1    >5       Poor signal;
//*  0    0    0    0    b    <5 >0    Losing signal;
//*  0    b    b    b    b    <1       No signal;
//*  b    0    0    0    0    >-60     Strong noise;
//*
//* by Qige, from 6Harmonics Inc. - 6WiLink Inc.
//* qige.zhao@6wilink.com, qigezhao@gmail.com
//* 2013.02.20
//**

void set_signal_led(const struct iwinfo_ops *iw, const char *ifname) {

	int sig, noise, snr = 0;

	for(;;) {
		if (iw->signal(ifname, &sig)) sig = -95;
		if (iw->noise(ifname, &noise)) noise = -95;

		snr = sig - noise;
		printf("SNR = %d, signal = %d dBm, noise = %d dBm\n", snr, sig, noise);
		printf("------------------------------------------------------------------\n");

		// Strong
		if (snr >= 45) { sw_flash_led(4, 1); } 
		else { sw_flash_led(4, 0); }

		// Normal
		if (snr >= 30) { sw_flash_led(3, 1); } 
		else { sw_flash_led(3, 0); }

		// Low
		if (snr >= 15) { sw_flash_led(2, 1); }
		else { sw_flash_led(2, 0); }

		// Poor
		if (snr >= 5) { sw_flash_led(1, 1); }

		// None: slow blink
		if (snr <= 0) {
			sw_flash_led(4, 1); sw_flash_led(3, 1); sw_flash_led(2, 1); sw_flash_led(1, 1);
			usleep(LED_BSLOW_I);
			sw_flash_led(4, 0); sw_flash_led(3, 0); sw_flash_led(2, 0); sw_flash_led(1, 0);
		}

		// Missing signal: fast blink
		if (snr < 5 && snr > 0) {
			sw_flash_led(1, 1);
			usleep(LED_BFAST_I);
			sw_flash_led(1, 0);
		}

		// Signal too strong
		if (snr >= 60 && snr < 75) {
			sw_flash_led(4, 1);
			usleep(LED_BSLOW_II);
			sw_flash_led(4, 0);
		}

		// Signal too strong
		if (snr >= 75) {
			sw_flash_led(4, 1);
			usleep(LED_BFAST_II);
			sw_flash_led(4, 0);
		}

		// Noise too strong
		if (noise >= -60) { sw_flash_led(5, 1); }
		else { sw_flash_led(5, 0); }

		usleep(250000);
	}
}




//* History codes();

/*

//**
//* print_kpi(), reserved for OMC-KPI
//* 1. Print version; 2. Print BTS's Wifi info; 3. Print GWS BTS info; 4. Print GWS CPEs info;
//*
//* Based on Yu's print_bts() & print_cpe() code;
//* Modified by Qige from 6Harmonics Inc. - 6WiLink Inc.
//* qige.zhao@6wilink.com; qigezhao@gmail.com
//* 2013.01.18
//**
static void print_kpi(const struct iwinfo_ops *iw, const char *ifname) {

	printf("[version]\n");
	printf("v=1.0.0-2r\n");

	//* PRINT WIFI //*
	printf("[wifi]\n");
	printf("channel=%s\n", print_channel(iw, ifname));
	printf("freq=%s\n", print_frequency(iw, ifname));
	printf("txpower=%s\n", print_txpower(iw, ifname));
	printf("linkq=%s/%s\n", print_quality(iw, ifname), print_quality_max(iw, ifname));
	printf("signal=%s\n", print_signal(iw, ifname));
	printf("noise=%s\n", print_noise(iw, ifname));
	printf("bitrate=%s\n", print_rate(iw, ifname));
	printf("encryption=%s\n", print_encryption(iw, ifname));
	printf("vap=%s\n", print_mbssid_supp(iw, ifname));


	//* PRINT BTS //*
	printf("[bts]\n");
	printf("mac=%s\n", print_bssid(iw, ifname));
	printf("network=%s\n", print_ssid(iw, ifname));
	printf("linkq=%s/%s\n", print_quality(iw, ifname), print_quality_max(iw, ifname));
	printf("signal=%s\n", print_signal(iw, ifname));
	printf("noise=%s\n", print_noise(iw, ifname));
	printf("bitrate=%s\n", print_rate(iw, ifname));


	//* PRINT CPEs //*
	int i, len, qty = 0;
	char buf[IWINFO_BUFSIZE];
	struct iwinfo_assoclist_entry *e;

	if (iw->assoclist(ifname, buf, &len) || len <= 0) {
		printf("cpe0=no\n");
		return;
	}

	//printf("[cpe]\n");
	for (i = 0; i < len; i += sizeof(struct iwinfo_assoclist_entry)) {

		e = (struct iwinfo_assoclist_entry *) &buf[i];

		printf("[cpe%d]\n", qty);
		printf("mac=%s\n", format_bssid(e->mac));
		printf("linkq=%s/%s\n", print_quality(iw, ifname), print_quality_max(iw, ifname));
		printf("signal=%s\n", format_signal(e->signal));
		printf("noise=%s\n", format_noise(e->noise));
		printf("bitrate=%s\n", print_rate(iw, ifname));

		qty ++;
	}

	return;
} //* END OF print_kpi() //*



//**
//* print_kpi(), reserved for OMC-KPI
//* 1. Print version; 2. Print GWS BTS info; 3. Print GWS CPEs info;
//*
//* Based on Yu's print_bts() & print_cpe() code;
//* Modified by Qige from 6Harmonics Inc. - 6WiLink Inc.
//* qige.zhao@6wilink.com; qigezhao@gmail.com
//* 2013.01.28
//**
static void print_kpi_sm(const struct iwinfo_ops *iw, const char *ifname) {

	int shmid;
	key_t key;
	char *shm;
	struct gwsdk_report *report;
	char buffer[GWSDK_BUF_MAX_SIZE], block[GWS_KPI_BUF_MAX_LEN], tmp[GWS_KPI_BUF_MAX_LEN];

	key = 0x20130128;

	//* LOG SEND TIME STAMP //*
	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	time_t timep;
	struct tm *p;


	// SHARED MEMORY
	if ((shmid = shmget(key, GWS_KPI_BUF_MAX_LEN, 0666 | IPC_CREAT)) < 0) {
		perror("KPI shmget error.\n");
		exit(EXIT_FAILURE);
	}
	if ((shm = shmat(shmid, (void *)0, 0)) == (char *) -1) {
		perror("KPI shmat error.\n");
		exit(EXIT_FAILURE);
	}
	printf("Memory attached at %X\n", (int) shm);

	// SHARED DATA
	report = (struct gwsdk_report *) shm;


	//char buf[IWINFO_BUFSIZE];
	//struct iwinfo_assoclist_entry *e;
	int i, len, qty = 0;
	char buf[IWINFO_BUFSIZE], cpes[GWS_KPI_BUF_MAX_LEN];
	struct iwinfo_assoclist_entry *e;

	while(1) {

		memset(buffer, 0x0, sizeof(buffer));
		memset(block, 0x0, sizeof(block));
		memset(tmp, 0x0, sizeof(tmp));

		sprintf(tmp, "[version]\n");
		strcat(block, tmp);
		sprintf(tmp, "v=1.0.0-2r\n");
		strcat(block, tmp);

		strcat(buffer, block);

		//* PRINT WIFI //*
		memset(block, 0x0, sizeof(block));
		memset(tmp, 0x0, sizeof(tmp));

		/*sprintf(tmp, "[wifi]\n");
		strcat(block, tmp);
		sprintf(tmp, "channel=%s\n", print_channel(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "freq=%s\n", print_frequency(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "txpower=%s\n", print_txpower(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "linkq=%s/%s\n", print_quality(iw, ifname), print_quality_max(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "signal=%s\n", print_signal(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "noise=%s\n", print_noise(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "bitrate=%s\n", print_rate(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "encryption=%s\n", print_encryption(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "vap=%s\n", print_mbssid_supp(iw, ifname));
		strcat(block, tmp);

		strcat(buffer, block);//*


		//* PRINT BTS //*
		memset(block, 0x0, sizeof(block));
		memset(tmp, 0x0, sizeof(tmp));

		sprintf(tmp, "[bts]\n");
		strcat(block, tmp);
		sprintf(tmp, "mac=%s\n", print_bssid(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "network=%s\n", print_ssid(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "linkq=%s/%s\n", print_quality(iw, ifname), print_quality_max(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "signal=%s\n", print_signal(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "noise=%s\n", print_noise(iw, ifname));
		strcat(block, tmp);
		sprintf(tmp, "bitrate=%s\n", print_rate(iw, ifname));
		strcat(block, tmp);

		strcat(buffer, block);


		//* PRINT CPEs //*
		memset(block, 0x0, sizeof(block));
		memset(tmp, 0x0, sizeof(tmp));

		if (iw->assoclist(ifname, buf, &len) || len <= 0) {
			sprintf(tmp, "cpe0=no\n");
			strcat(buffer, tmp);
		}
		//printf("---------------\n%s\n-------------", buffer);

		qty = 0;
		memset(cpes, 0x0, sizeof(cpes));
		for (i = 0; i < len; i += sizeof(struct iwinfo_assoclist_entry)) {

			e = (struct iwinfo_assoclist_entry *) &buf[i];

			memset(block, 0x0, sizeof(block));
			memset(tmp, 0x0, sizeof(tmp));

			sprintf(tmp, "[cpe%d]\n", qty);
			strcat(block, tmp);
			sprintf(tmp, "mac=%s\n", format_bssid(e->mac));
			strcat(block, tmp);
			sprintf(tmp, "linkq=%s/%s\n", print_quality(iw, ifname), print_quality_max(iw, ifname));
			strcat(block, tmp);
			sprintf(tmp, "signal=%s\n", format_signal(e->signal));
			strcat(block, tmp);
			sprintf(tmp, "noise=%s\n", format_noise(e->noise));
			strcat(block, tmp);
			sprintf(tmp, "bitrate=%s\n", print_rate(iw, ifname));
			strcat(block, tmp);

			strcat(cpes, block);

			qty ++;
		}
		sprintf(tmp, "cpeqty=%d\n", qty);
		strcat(buffer, tmp);

		strcat(buffer, cpes);


		//* MARK SEND TIME STAMP //*
		time(&timep);
		p = gmtime(&timep);

		memset(block, 0x0, sizeof(block));
		sprintf(block, "[stamp]\nstamp=%d-%d-%d %d:%d:%d\n", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

		strcat(buffer, block);


		strncpy(report->report, buffer, sizeof(buffer));


		printf("%s\n", report->report);
		printf("------------------------------------------------------------------\n");
		//sleep(10);

		sleep(1);
	}

	if (shmdt(shm) == -1) {
		perror("KPI shmdt failed.\n");
		exit(EXIT_FAILURE);
	}
	
	if (shmctl(shmid, IPC_RMID, 0) == -1) {
		perror("KPI shmctl(IPC_RMID) failed.\n");
		exit(EXIT_FAILURE);
	}

	return;
	
} //* END OF print_kpi_sh() //*

*/

