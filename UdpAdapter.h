#ifndef _UDP_ADAPTER
#define _UDP_ADAPTER

#include "NetAdapter.h"

// UDP连接接口
class UdpAdapter : public NetAdapter
{
public:
	// 收发用Socket
	int g_ServerSocket;

	// 构建 UDP 接口。并且在系统上需要一些其他支持
	UdpAdapter(unsigned short listenport, MsgQueue *pUdpInMsgQueue);

	// 释放Socket环境
	~UdpAdapter();

	// 循环接收方法
	DWORD RecvUdp();

	// 循环接收线程
	static void RecvThread(LPVOID WorkThreadContext);

	// 从队列循环发送方法
	DWORD SendUdp();

	// 循环发送线程
	static void SendThread(LPVOID WorkThreadContext);

	// 发送数据
	BOOL SendData(NetMsg *pNetMsg);

	BOOL SendData(char *ip, unsigned short port, char *buff, unsigned int len);

	// 查看连接类型
	NetAdapterType GetType();

	// 启动服务
	void Start();

	// 关闭服务
	void Stop();

private:

	// 输出操作的原始消息队列
	MsgQueue MsgOutQueue;
};

class UdpBroadcaster
{
	public:
	// 收发用Socket
	int g_ServerSocket;

	unsigned int g_ServerPort;

	char bdctData[1024];

	unsigned int dataLen;

	UdpBroadcaster(unsigned int port, void *data, unsigned int len);

	~UdpBroadcaster();

	// 循环发送线程
	static void SendThread(LPVOID WorkThreadContext);
};

#endif
