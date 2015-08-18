//**
//* by Qige from 6Harmonics Inc. @ Beijing
//* 2014.03.13
//**
#define SP_FILE         "/dev/ttyS0"
#define SP_BUF_MAX_LEN  128
#define SP2LCD_INTL     10 
#define SP2LCD_SERV     1
#define PRINT_DEBUG     _debug && printf
//#define PRINT_DEBUG     printf

static struct termios _opt;
static char *sp_file = SP_FILE;
static int sp_fd;
static char sp_tx_buf[SP_BUF_MAX_LEN];
static char sp_rx_buf[SP_BUF_MAX_LEN];

void sp_open() {
    sp_fd = open(sp_file, O_RDWR);
    if (-1 == sp_fd) {
        perror("ERR: Cannot open serial port.\n");
    }
}

void sp_init() {
    struct termios opt;
    fcntl(sp_fd, F_SETFL, FNDELAY);
    tcgetattr(sp_fd, &_opt);
    tcgetattr(sp_fd, &opt);
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
    tcsetattr(sp_fd, TCSANOW, &opt);
}

void sp_send(const uint _debug) {
    PRINT_DEBUG("- sp_tx_buf = [%s]\tbefore send\n", sp_tx_buf);
    if (strlen(sp_tx_buf) > 0) {
        write(sp_fd, sp_tx_buf, strlen(sp_tx_buf));
    }
    memset(sp_tx_buf, 0, sizeof (sp_tx_buf));
    PRINT_DEBUG("- sp_tx_buf = [%s]\tafter send\n", sp_tx_buf);
}

int sp_recv(const uint _debug) {
    uint result = 0;
    fd_set rfds;
    struct timeval tv;
    int nread;
    char _buf[SP_BUF_MAX_LEN], *p;
    while ((nread = read(sp_fd, _buf, SP_BUF_MAX_LEN)) > 0) {
        if (_buf[0] == '+') {
            //PRINT_DEBUG("- debug -: sp_recv() read = %s (%d)\n", _buf, strlen(_buf));
            while (p = strchr(_buf, '\r')) {
                *p = '\0';
            }
            while (p = strchr(_buf, '\n')) {
                *p = '\0';
            }
            //PRINT_DEBUG("- debug -: sp_recv() filter = %s (%d)\n", _buf, strlen(_buf));
            if (strlen(_buf) > 0) strcat(sp_rx_buf, _buf);
            break;
        }
        memset(_buf, 0, sizeof (_buf));

        if (strlen(sp_rx_buf) >= SP_BUF_MAX_LEN) break;
    }
    return result;
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

void sp_close() {
    tcsetattr(sp_fd, TCSANOW, &_opt);
    close(sp_fd);
}

void _tx() {
    char _buf[SP_BUF_MAX_LEN];
    memset(_buf, 0, sizeof (_buf));
    sprintf(_buf, "%s", "--- SP Tx test ---\r\nIf you can see this from serial port, that means we are good.\r\n");
    strcat(sp_tx_buf, _buf);
    //	PRINT_DEBUG("%s\n", sp_tx_buf);
    sp_send(0);
}
