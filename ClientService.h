#ifndef _CLIENT_SERVICE
#define _CLIENT_SERVICE

#include "UdpAdapter.h"
#include <mysql/mysql.h>
#include "RedisClient.hpp"

class ClientService;

// ������ķ����ص������ӿ�
typedef void (* pParseFunction)(NetMsg *pNetMsg, void *LPCONN, void *LPCONN2, void *PARAM1, void *PARAM2);
typedef void (* pOnTcpDisconnect)(int s, void *LRARAM);

extern unsigned short UDP_SERV_PORT;
extern int UPDATE_START_TIME;
extern int UPDATE_END_TIME;

// �ն˷���ӿ�
class ClientService
{
public:
	// ��ʼ������
	// taskCount Ϊͬʱ����Ĵ����߳�������Adapter������Խ����صĴ�������ʱ��Խ����������ֶ������������ߡ�
	ClientService(NetAdapter::NetAdapterType dwNetAdapterType, unsigned short port, pParseFunction pParseCallBack, void *PARAM1, void *PARAM2, pOnTcpDisconnect pDisConnCallBack, void *DisconnParam, unsigned short taskCount = 8);

	// �ͷ���Դ
	~ClientService();

	// ���� NetMsg ������  ParseThreadParam *pParam
	static void ParseMsgThread(LPVOID WorkThreadContext);

	// ����Ƿ����ߵ��߳�
	static void CheckingOnlineThread(LPVOID WorkThreadContext);

	// ��ʼ����
	void Start();

	// �������ӽӿ�
	NetAdapter *m_pNetAdapter;

	// ��� NetMsg �Ľ��ն���
	MsgQueue InMsgQueue;

protected:

	// ����ʵ�����ݵĹ������߳�����
	unsigned short ThreadCount;

	// �ص������ӿ�
	pParseFunction m_pParseFunction;

	// TCP���߻ص������ӿ�
	pOnTcpDisconnect m_pOnTcpDisconnect;

	// �����û��Զ�������������ָ�룩
	void *Param1;
	void *Param2;

	void *DisconnPARAM;

	// ��ӦTCP���Ӷο��¼��Ļص�����
	static void OnTcpDisconnect(int s, void *lparam);
};

class SockAddr
{
public:
	unsigned long addr;
	unsigned short port;
	int activeSocket;

	SockAddr();
	~SockAddr();
	SockAddr(sockaddr_in x);
	SockAddr(int s);

	SockAddr &operator = (SockAddr &x);

	SockAddr &operator = (sockaddr_in x);

	SockAddr &operator = (SOCKET s);

	bool operator == (SockAddr &x);

	bool operator != (SockAddr &x);

	bool operator < (const SockAddr &x) const;
};

void InitSock(void);

void ReleaseSock(void);

void ReadConfigFile();

int ConnectDB(MYSQL *lpconn);

int ReconnectDB(MYSQL *lpconn);

signed long long QueryDB(MYSQL *lpconn, const char *sqlstr);

#endif
