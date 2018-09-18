#ifndef _NET_COMMON_ADAPTER
#define _NET_COMMON_ADAPTER

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <thread>
#include <semaphore.h>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <iostream>
#include <queue>
#include <map>
#include <list>

// 最大缓存
#define MAX_BUFF_SIZE			4096
#define BOOL int
#define DWORD unsigned int
#define LPVOID void*
#define SOCKET int
#define TRUE 1
#define FALSE 0

using namespace std;

class NetAdapter;

// 网络来源
struct NetSource
{
	int			ActiveSocket;				// TCP使用的Socket
	sockaddr_in		RemoteAddr;					// UDP使用的客户端地址
	char			SimNumber[20];				// Sim卡号码
	char			Para1[64];
	char			Para2[64];
	char			Para3[64];
	char			Para4[64];
	NetAdapter		*pAdapter;					// 对应的网络适配器
};

// TCP网络上传输的短消息（收发都用）
struct NetMsg{
	char			Buffer[MAX_BUFF_SIZE];		// 缓冲区
	unsigned int	Length;						// 缓冲区内部数据长度
	NetSource		Source;						// 来源网络适配器及对应属性
	//COleDateTime	CurrentTime;				// 当前时间
};

class MsgQueue
{
public:
	void Push(NetMsg *lpmsg);
	void Pop(NetMsg *lpmsg);
	void Clear();
	void Dispose();

	MsgQueue();
	~MsgQueue();

private:
	queue <NetMsg> MessageQueue;
	sem_t waitHandleMQ;
	mutex m_csMQ;

	BOOL ExitSymbol;
};

// 网络适配器基类
class NetAdapter
{
public:
	enum NetAdapterType
	{
		// 未知或者未定义类型（没有实际效果）
		NET_ADAPTER_TYPE_UNKNOWN	= 1000,

		// TCP适配器
		NET_ADAPTER_TYPE_TCP		= 1001,

		// UDP 适配器
		NET_ADAPTER_TYPE_UDP		= 1002,

		// 短信适配器（通过Modem）
		NET_ADAPTER_TYPE_SMS		= 1003,

		// 短信适配器（通过 SMS Gateway）
		NET_ADAPTER_TYPE_SMS_GW		= 1004
	};

	// 发送数据
	virtual BOOL SendData(NetMsg *pNetMsg);

	// 取得类型
	virtual NetAdapterType GetType();

	// 启动服务
	virtual void Start();

	// 关闭服务
	virtual void Stop();

	// 操作的原始消息队列
	MsgQueue *m_pMsgInQueue;

	// TCP侦听端口
	unsigned short ListenPort;
};

char *SetNowTimeStr();
char *GetNowTimeStr();

#endif
