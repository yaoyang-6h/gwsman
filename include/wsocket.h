#ifndef		__WSOCKET_H__
#define		__WSOCKET_H__

#ifndef		__WYY_SOCKET_H__
#define		__WYY_SOCKET_H__

#define	MAX_BUFF_SIZE			4096
#define	MAX_CONNECTIONS			128

#define	MODE_TCPSERVER			1
#define	MODE_TCPCLIENT			2
#define	MODE_UDPIP			3

#define	SOCKET_SYNCHRONOUS		0	//	blocking mode,		accept() µÈŽýœá¹ûºó²Å·µ»Ø
#define	SOCKET_ASYNCHRONOUS             1	//	nonblocking mode£¬	accept() Á¢ŒŽ·µ»Ø
#define bool                            int
#define true                            1
#define false                           0
#define	SOCKET				int
#define string                          char*
#define	LPSTR				char*
#define	LPVOID				void*
#define	BYTE				unsigned char
#define	WORD				unsigned short
#define	DWORD				unsigned int
#define	INVALID_SOCKET			-1
#define	INVALID_THREAD			0xFFFFFFFF
#define	SOCKET_ERROR			-1
#define	SOCKADDR			struct sockaddr
#define	PSOCKADDR			SOCKADDR*
#define	SOCKADDR_IN			struct sockaddr_in
#define	IN_ADDR				struct in_addr
#define	ZeroMemory			bzero
#define	HANDLE				int

#define	WSAEWOULDBLOCK			EWOULDBLOCK	//EAGAIN
#define	WSAECONNRESET			ECONNRESET
#define	WSAENOTSOCK			ENOTSOCK
#define	COLOR_RED_BLACK			0x01

typedef struct __SocketBase {
    int m_nRecv;
    BYTE m_buffer[MAX_BUFF_SIZE];
    SOCKET m_socket;
    DWORD m_dwRemoteAdd;
    WORD m_wRemotePort;
    BYTE m_host_ip[4];
    short m_PortNo;
} SOCKET_BASE;

void Socket_Init(short PortNo);
bool Socket_Bind(int type, short port, LPSTR multicastIP); //multicastIP= NULL
DWORD Socket_GetIP(const char *if_name);
DWORD Socket_Accept(SOCKET listener);
bool Socket_Connect(int b1, int b2, int b3, int b4, int port);
int Socket_SendData(LPSTR buffer, DWORD size); /* TCP/IP */
int Socket_RecvData(DWORD size); /* TCP/IP */
int Socket_SendTo(string ip, int port, LPSTR buffer, DWORD size); /* UDP/IP */
char* Socket_RecvFrom(string ip, int port, DWORD* size); /* UDP/IP */
int Socket_RecvFrom_dw(DWORD* ip, int port, DWORD size); /* UDP/IP */
void Socket_Disconnect();
BYTE* Socket_GetData(int* nSize);
string IPconvertN2A(DWORD dwIP);
DWORD IPconvertA2N(const char* ip);
/*
class CSocketServer;
class CSocketConn : public CSocketBase {
    friend class CSocketServer;
private:
    int m_pipe_er[2];
    int m_pipe_up[2];
    int m_pipe_dn[2];
private:
    char m_multicastIP[16];
    pthread_cond_t m_cond;
    pthread_mutex_t m_lock;
    pthread_mutex_t m_receiveEvent;
    pthread_t m_receiveThread;
private:
    bool m_ReceiveState();
    void m_ReceivePolling(BYTE cMode);
    static LPVOID m_ReceiveServer(LPVOID lp);
    static LPVOID m_ReceiveClient(LPVOID lp);
    static LPVOID m_ReceiveUdpip(LPVOID lp);
protected:
    bool m_Bind(SOCKET* listener); //	for TCP/IP (stream)
    DWORD m_Accept(SOCKET listener);
    bool m_IsUpdate();
    void m_Release();
public:
    CSocketConn(short PortNo);
    CSocketConn();
    ~CSocketConn();
public:
    bool m_Bind(LPSTR multicastIP, short port = 0); //	for UDP/IP (datagram)
    bool m_Connect(int b1, int b2, int b3, int b4, short port);
    bool m_IsConnected();
    void m_DisConnect();
    void m_CreatePipe();
    int* m_GetRecvPipe();       //reading end of the recv pipe
    int* m_GetSendPipe();       //writing end of the send pipe
    void m_RedirectStdIO();
public:
    int m_Multicast(WORD port, LPSTR buffer, DWORD size);
    int m_TryRead(int fd,BYTE* buff,int nMax);
    int m_Send(string ip, WORD port, LPSTR buffer, DWORD size);
    int m_Send(DWORD ip, WORD port, LPSTR buffer, DWORD size);
    int m_Send(LPSTR buffer, DWORD size);
    int m_Recv(string ip, WORD* port, DWORD* size);
    int m_Recv(DWORD* ip, WORD* port, DWORD* size);
    int m_Recv(DWORD size);
public:
    virtual int OnSend(BYTE* buf_send);
    virtual void OnRecv(int nRecv,BYTE* buf_recv);
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
*/
#endif		//__WYY_SOCKET_H__
/*
class CSocketServer {
private:
    pthread_t   m_listenThread;
    pthread_mutex_t m_listenEvent;
    SOCKET m_listener;
protected:
    CSocketConn m_connection[MAX_CONNECTIONS];
private:
    void m_ListenPolling();
    bool m_ListenState();
    DWORD m_Accept(SOCKET socket);
    void m_CleanUp();
private:
    static LPVOID m_ListenConnections(LPVOID lp);
    ////////////////////////////////////////////////////////////////////////////
public:
    CSocketServer(short port);
    ~CSocketServer();
    bool m_StartListenThread();
    void m_StopListenThread();
public:
    int m_TryRead(int fd,BYTE* buff,int nMax);
    int m_Broadcast(char* buffer, DWORD size);
    int m_Browse(int* index); //index=EOF或返回数据长度值
    int m_Recv(int index); //为0，均表示无数据
    int m_Scan(int* index); //扫描所有端口
    int m_Send(int index, char* buffer, DWORD size);
    int m_GetConnectionCount(); //返回目前已经被占用的通道数
    bool m_DisConnect(int index); //断开索引为index的通道
public:
    virtual void OnRecv(int index, int nData, BYTE* pData);
};
*/
#endif		//__WSOCKET_H__
