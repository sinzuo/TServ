#ifndef _UDP_ADAPTER
#define _UDP_ADAPTER

#include "NetAdapter.h"

// UDP���ӽӿ�
class UdpAdapter : public NetAdapter
{
public:
	// �շ���Socket
	int g_ServerSocket;

	// ���� UDP �ӿڡ�������ϵͳ����ҪһЩ����֧��
	UdpAdapter(unsigned short listenport, MsgQueue *pUdpInMsgQueue);

	// �ͷ�Socket����
	~UdpAdapter();

	// ѭ�����շ���
	DWORD RecvUdp();

	// ѭ�������߳�
	static void RecvThread(LPVOID WorkThreadContext);

	// �Ӷ���ѭ�����ͷ���
	DWORD SendUdp();

	// ѭ�������߳�
	static void SendThread(LPVOID WorkThreadContext);

	// ��������
	BOOL SendData(NetMsg *pNetMsg);

	BOOL SendData(char *ip, unsigned short port, char *buff, unsigned int len);

	// �鿴��������
	NetAdapterType GetType();

	// ��������
	void Start();

	// �رշ���
	void Stop();

private:

	// ���������ԭʼ��Ϣ����
	MsgQueue MsgOutQueue;
};

class UdpBroadcaster
{
	public:
	// �շ���Socket
	int g_ServerSocket;

	unsigned int g_ServerPort;

	char bdctData[1024];

	unsigned int dataLen;

	UdpBroadcaster(unsigned int port, void *data, unsigned int len);

	~UdpBroadcaster();

	// ѭ�������߳�
	static void SendThread(LPVOID WorkThreadContext);
};

#endif
