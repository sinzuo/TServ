#include <stdlib.h>
#include <unistd.h>
#include "stdafx.h"
#include "RedisClient.hpp"
#include "ClientService.h"

extern int SysClosing;

char MYSQL_HOST[64];
char MYSQL_USER[32];
char MYSQL_PASSWD[32];
char MYSQL_DB[32];

char REDIS_HOST[64];
unsigned short REDIS_PORT = 6379;

unsigned short UDP_SERV_PORT = 8880;

int UPDATE_START_TIME = 0300;
int UPDATE_END_TIME = 0530;

// ���ݹؼ��ַֽ� string������vector����
void StringSplit(string content, string token, vector <string> &StringArray)
{
	StringArray.clear();
	int pointer = 0, offset = 0;
	unsigned int tlen = token.length();

	if((int)content.find(token) < 0)
	{
		StringArray.push_back(content);
		return;
	}

	while((pointer = (int)content.find(token, offset)) >= 0)
	{
		StringArray.push_back(content.substr(offset, pointer - offset));
		offset = pointer + tlen;
	}
	StringArray.push_back(content.substr(offset, content.length() - offset));
}

// ��ȡ��ǰ·��
size_t GetCurrentPath(char *pathbuff)
{
	getcwd(pathbuff, 256);
	return strlen(pathbuff);
}

size_t ReadFromFile(char *filename, char *cont)
{
	int fd, rdlen;
	char buffer[1024] = {0};
	char *content = cont;

	if(access(filename, F_OK))
		return 0;

	fd = open(filename, O_RDONLY);
	while((rdlen = read(fd, buffer, 1024)) > 0)
	{
		memcpy(content, buffer, rdlen);
		content += rdlen;
	}
	close(fd);

	return strlen(cont);
}

void ReadConfigFile()
{
	char cfgbuff[2048] = {0};
	char cfgpath[256] = {0};
	size_t len = 0;

	// Set default
	memset(MYSQL_HOST, 0, 64);
	memset(MYSQL_USER, 0, 32);
	memset(MYSQL_PASSWD, 0, 32);
	memset(MYSQL_DB, 0, 32);
	memset(REDIS_HOST, 0, 64);

	strcpy(MYSQL_HOST, "192.168.51.8");
	strcpy(MYSQL_USER, "root");
	strcpy(MYSQL_PASSWD, "root");
	strcpy(MYSQL_DB, "gx_wifi_home");
	strcpy(REDIS_HOST, "192.168.51.6");

	REDIS_PORT = 6379;
	UDP_SERV_PORT = 8880;

	UPDATE_START_TIME = 0300;
	UPDATE_END_TIME = 0530;

	GetCurrentPath(cfgpath);
	strncpy(cfgpath + strlen(cfgpath), "/TServ.ini", 32);

	if(!ReadFromFile(cfgpath, cfgbuff))
	{
		printf("Config file not found.\n");
		return;
	}

	vector <string> cfglines;
	vector <string> cfgparas;
	StringSplit(cfgbuff, "\n", cfglines);

	for(int i = 0; i < cfglines.size(); i ++)
	{
		if(cfglines[i][0] == '#')
			continue;

		cfgparas.clear();
		StringSplit(cfglines[i], "=", cfgparas);

		if(cfgparas.size() != 2)
			continue;

		if(cfgparas[0] == "mysql_host")
		{
			memset(MYSQL_HOST, 0, 64);
			strncpy(MYSQL_HOST, cfgparas[1].c_str(), 60);
		}
		else if(cfgparas[0] == "mysql_user")
		{
			memset(MYSQL_USER, 0, 32);
			strncpy(MYSQL_USER, cfgparas[1].c_str(), 30);
		}
		else if(cfgparas[0] == "mysql_passwd")
		{
			memset(MYSQL_PASSWD, 0, 32);
			strncpy(MYSQL_PASSWD, cfgparas[1].c_str(), 30);
		}
		else if(cfgparas[0] == "mysql_db")
		{
			memset(MYSQL_DB, 0, 32);
			strncpy(MYSQL_DB, cfgparas[1].c_str(), 30);
		}
		else if(cfgparas[0] == "udp_serv_port")
		{
			UDP_SERV_PORT = atoi(cfgparas[1].c_str());
		}
		else if(cfgparas[0] == "redis_host")
		{
			memset(REDIS_HOST, 0, 64);
			strncpy(REDIS_HOST, cfgparas[1].c_str(), 60);
		}
		else if(cfgparas[0] == "redis_port")
		{
			REDIS_PORT = atoi(cfgparas[1].c_str());
		}
		else if(cfgparas[0] == "update_start_time")
		{
			UPDATE_START_TIME = atoi(cfgparas[1].c_str());
		}
		else if(cfgparas[0] == "update_end_time")
		{
			UPDATE_END_TIME = atoi(cfgparas[1].c_str());
		}

		printf("Config %s  is  %s\n", cfgparas[0].c_str(), cfgparas[1].c_str());
	}

	cfgparas.clear();
	cfglines.clear();
}

int ConnectDB(MYSQL *lpconn)
{
	if(!mysql_real_connect(lpconn,MYSQL_HOST,MYSQL_USER,MYSQL_PASSWD,MYSQL_DB,0,NULL,CLIENT_FOUND_ROWS))
	{
		MOUTPUT("Can not connect to mysql : %s.", (mysql_error(lpconn)))
		return 0;
	}
	else
	{
		MOUTPUT("MySQL database connected.");
		return -1;
	}
}

int ReconnectDB(MYSQL *lpconn)
{
	mysql_close(lpconn);
	return ConnectDB(lpconn);
}

signed long long QueryDB(MYSQL *lpconn, const char *sqlstr)
{
	int res = mysql_query(lpconn, sqlstr);
	if(res)
	{
		ReconnectDB(lpconn);
		res = mysql_query(lpconn, sqlstr);
		if(res)
			return -1;
	}
	return mysql_affected_rows(lpconn);
}

// ��ʼ������
// taskCount Ϊͬʱ����Ĵ����߳�������Adapter������Խ����صĴ�������ʱ��Խ����������ֶ������������ߡ�
// pParseCallBack Ҫ�������� void (*)(NetMsg *, void *, void *)�Ϳ��ԣ����ڲ������þ�̬��������������PARAM1/2������ȥ��
ClientService::ClientService(NetAdapter::NetAdapterType dwNetAdapterType, unsigned short port, pParseFunction pParseCallBack, void *PARAM1, void *PARAM2, pOnTcpDisconnect pDisConnCallBack, void *DisconnParam, unsigned short taskCount)
{
	ThreadCount = taskCount;

	//InMsgQueue

	switch(dwNetAdapterType)
	{
	case NetAdapter::NET_ADAPTER_TYPE_TCP:
		{
			//m_pNetAdapter = new IocpAdapter(port, &InMsgQueue, &OnTcpDisconnect, this);
			break;
		}
	case NetAdapter::NET_ADAPTER_TYPE_UDP:
		{
			m_pNetAdapter = new UdpAdapter(port, &InMsgQueue);
			break;
		}
	case NetAdapter::NET_ADAPTER_TYPE_SMS:
		{
			m_pNetAdapter = NULL;
			break;
		}
	case NetAdapter::NET_ADAPTER_TYPE_SMS_GW:
		{
			m_pNetAdapter = NULL;
			break;
		}
	case NetAdapter::NET_ADAPTER_TYPE_UNKNOWN:
		{
			m_pNetAdapter = NULL;
			break;
		}
	}

	m_pParseFunction = pParseCallBack;
	m_pOnTcpDisconnect = pDisConnCallBack;
	//m_pDBA = NULL;
	Param1 = PARAM1;
	Param2 = PARAM2;
	DisconnPARAM = DisconnParam;
}

// �ͷ���Դ
ClientService::~ClientService()
{
	InMsgQueue.Dispose();
}

// ���� NetMsg ������  ParseThreadParam *pParam
void ClientService::ParseMsgThread(LPVOID WorkThreadContext)
{
	ClientService *lp_service = (ClientService *)WorkThreadContext;
	NetMsg msg;

	MYSQL conn;
	char val = 1;
	mysql_init(&conn);
	if(!mysql_real_connect(&conn,MYSQL_HOST,MYSQL_USER,MYSQL_PASSWD,MYSQL_DB,0,NULL,CLIENT_FOUND_ROWS))
	{
		MOUTPUT("Can not connect to mysql : %s.", (mysql_error(&conn)))
	}
	else
	{
		MOUTPUT("MySQL database connected.");
	}
	mysql_options(&conn, MYSQL_OPT_RECONNECT, &val);

	CRedisClient redis_client;
	if(! redis_client.Initialize(REDIS_HOST, REDIS_PORT, 2, 10) )
	{
		printf("\nConnect to redis %s %d failed!\n", REDIS_HOST, REDIS_PORT);
		return;
	}
	else
	{
		printf("\nConnected to redis %s %d!\n", REDIS_HOST, REDIS_PORT);
	}

	while(1)
	{
		// ���û�������ݾ͵ȴ�
		lp_service->InMsgQueue.Pop(&msg);

		if(SysClosing)
			break;

		// �������
		try
		{
			if(lp_service->m_pParseFunction)
				(*lp_service->m_pParseFunction)(&msg, &conn, &redis_client, lp_service->Param1, lp_service->Param2);
		}
		catch(...)
		{
			MOUTPUT("�������ݴ�����������������\n������룺%s\r\n", strerror(errno));
		}
	}

	mysql_close(&conn);
	MOUTPUT("MySQL database closed.");
}

// ��ʼ����
void ClientService::Start()
{
	for(int i = 0; i < ThreadCount; i ++)
	{
		// ��ʼ��������Ϣ�����߳�
		thread work_thread(&ClientService::ParseMsgThread, (LPVOID)this);
		work_thread.detach();
	}

	// ��������ӿ�
	if(m_pNetAdapter)
		m_pNetAdapter->Start();
}

// ��ӦTCP���ӶϿ�
void ClientService::OnTcpDisconnect(int s, void *lparam)
{
	if(!lparam)
		return;
	ClientService *pClientService = (ClientService *)lparam;
	if(pClientService->m_pOnTcpDisconnect)
		(*pClientService->m_pOnTcpDisconnect)(s, pClientService->DisconnPARAM);
}

// �����ʼ��
void InitSock()
{
	return;
}

// ����socket
void ReleaseSock()
{
	return;
}

/************************************* SockAddr ************************************/

SockAddr::SockAddr()
{
	addr = NULL;
	port = NULL;
	activeSocket = NULL;
}

SockAddr::~SockAddr()
{
	//
}

SockAddr::SockAddr(sockaddr_in x)
{
	addr = x.sin_addr.s_addr;
	port = x.sin_port;
	activeSocket = NULL;
}

SockAddr::SockAddr(SOCKET s)
{
	addr = NULL;
	port = NULL;
	activeSocket = s;
}

SockAddr &SockAddr::operator =(SockAddr &x)
{
	addr = x.addr;
	port = x.port;
	activeSocket = x.activeSocket;

	return *this;
}

SockAddr &SockAddr::operator =(sockaddr_in x)
{
	addr = x.sin_addr.s_addr;
	port = x.sin_port;
	activeSocket = NULL;

	return *this;
}

bool SockAddr::operator ==(SockAddr &x)
{
	if(addr == x.addr && port == x.port && activeSocket == x.activeSocket)
		return true;
	else
		return false;
}

bool SockAddr::operator !=(SockAddr &x)
{
	if(addr != x.addr || port != x.port || activeSocket != x.activeSocket)
		return true;
	else
		return false;
}

bool SockAddr::operator <(const SockAddr &x) const
{
	if(addr < x.addr)
		return true;
	else if(addr == x.addr && port < x.port)
		return true;
	else if(addr == x.addr && port == x.port && activeSocket < x.activeSocket)
		return true;
	else
		return false;
}
