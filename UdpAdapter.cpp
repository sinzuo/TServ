#include "stdafx.h"
#include "UdpAdapter.h"

int recv_hb = 0;
extern int SysClosing;

UdpBroadcaster::UdpBroadcaster(unsigned int port, void *data, unsigned int len)
{
	g_ServerPort = port;

	memset(bdctData, 0, 1024);
	memcpy(bdctData, data, len);

	dataLen = len;

	MOUTPUT("UDP Broadcast Socket Inited Successfully. Using port %d\n", port);

	thread send_thread(&UdpBroadcaster::SendThread, (LPVOID)this);
    send_thread.detach();
}

UdpBroadcaster::~UdpBroadcaster()
{
	close(g_ServerSocket);
}

void UdpBroadcaster::SendThread(LPVOID WorkThreadContext)
{
	UdpBroadcaster *adapter = (UdpBroadcaster *)WorkThreadContext;
	adapter->g_ServerSocket = socket(PF_INET, SOCK_DGRAM, 0);

	BOOL fBroadcast = TRUE;
	setsockopt( adapter->g_ServerSocket, SOL_SOCKET, SO_BROADCAST, (char *)&fBroadcast, sizeof( BOOL ) );

	// ��IP���˿�
	sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl ( INADDR_BROADCAST );
	serveraddr.sin_port = htons(adapter->g_ServerPort);

	int sendret = 0;
	for(;;)
	{
		sendret = sendto( adapter->g_ServerSocket, adapter->bdctData, adapter->dataLen, 0, (struct sockaddr *)&serveraddr, sizeof( serveraddr ));

		sleep(1);

		if(SysClosing)
			return;
	}
}

// ���� UDP �ӿڡ�������ϵͳ����ҪһЩ����֧��
// pNetMsgProducer: NetMsg���ڴ��ṩ�߶���
// pUdpInMsgQueue: �������
// pUdpOutMsgQueue: �������
UdpAdapter::UdpAdapter(unsigned short listenport, MsgQueue *pUdpInMsgQueue)
{
	m_pMsgInQueue = pUdpInMsgQueue;
	ListenPort = listenport;

	// ��ʼ��MemQueue
	//MsgOutQueue;

	// ����Socket
	g_ServerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if( g_ServerSocket <= 0 )
	{
		MOUTPUT("Create UDP Socket Error.\n");
		return;
	}

	// ��IP���˿�
	sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(ListenPort);

	// ��ָ���Ķ˿�
	bind(g_ServerSocket,(sockaddr *)&serveraddr, sizeof(serveraddr));

	// ���sendto����
	//DWORD dwBytesReturned = 0;
	//BOOL bNewBehavior = FALSE;
	//DWORD status = WSAIoctl(g_ServerSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof (bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);

	MOUTPUT("UDP Socket Inited Successfully. Binding port %d\n", listenport);
}

// �ͷ� UDP �ӿ�
UdpAdapter::~UdpAdapter()
{
	close(g_ServerSocket);
	MsgOutQueue.Clear();
}

extern unsigned int SetDevAddrCmd(char *injson, void *vaddr, char *cmdstr);

// ѭ�����շ���
DWORD UdpAdapter::RecvUdp()
{
	unsigned int len = sizeof(sockaddr_in);
	int recvlen = 0;
	NetMsg msg;
	char cmdstr[1024];
	unsigned int cmdlen;

	while(1)
	{
		if(1)
		{
			memset(&msg, 0, sizeof(NetMsg));

			// ���ܲ��ܳ����յ�������Ҳ����̫С
			while((recvlen = recvfrom(g_ServerSocket, msg.Buffer, MAX_BUFF_SIZE, 0, (sockaddr *)&msg.Source.RemoteAddr, &len)) < 0)
			{
				if(SysClosing)
					return 1;
				memset(msg.Buffer, 0, MAX_BUFF_SIZE);
			}
			msg.Source.pAdapter = this;
			msg.Length = recvlen;
		}

		MOUTPUT("[%s][UDP][Recv][%s:%d][Length:%d]\n%s\n", GetNowTimeStr(), inet_ntoa(msg.Source.RemoteAddr.sin_addr), ntohs(msg.Source.RemoteAddr.sin_port), recvlen, msg.Buffer);

		m_pMsgInQueue->Push(&msg);
	}
	return 0;
}

// �Ӷ���ѭ�����ͷ���
DWORD UdpAdapter::SendUdp()
{
	int len = sizeof(sockaddr_in);
	int sendlen = 0;
	char datestr[32] = {0};
	NetMsg msg;
	while(1)
	{
		MsgOutQueue.Pop(&msg);

		if(msg.Source.RemoteAddr.sin_port == 0)
		{
			MOUTPUT("[Send to null, ignored!]");
			continue;
		}

		// ��������
		sendlen = sendto(g_ServerSocket, msg.Buffer, msg.Length, 0, (sockaddr *)&msg.Source.RemoteAddr, len);

		if(sendlen == -1)
			MOUTPUT("[Send Error: %s]\n", strerror(errno));

		MOUTPUT("[%s][UDP][Send][%s:%d][Length:%d]\n%s\n", GetNowTimeStr(), inet_ntoa(msg.Source.RemoteAddr.sin_addr), ntohs(msg.Source.RemoteAddr.sin_port), msg.Length, msg.Buffer);
	}
	return 0;
}

// ��������
BOOL UdpAdapter::SendData(NetMsg *pNetMsg)
{
	if(pNetMsg->Source.RemoteAddr.sin_port == 0 || pNetMsg->Length == 0)
		return FALSE;

	MsgOutQueue.Push(pNetMsg);
	return TRUE;
}

// ����IP�Ͷ˿ڷ�������
BOOL UdpAdapter::SendData(char *ip, unsigned short port, char *buff, unsigned int len)
{
	NetMsg msg;

	msg.Source.RemoteAddr.sin_family = AF_INET;
	msg.Source.RemoteAddr.sin_addr.s_addr = inet_addr(ip);
	msg.Source.RemoteAddr.sin_port = htons(port);

	memset(msg.Buffer, 0, MAX_BUFF_SIZE);
	memcpy(msg.Buffer, buff, len);
	msg.Length = len;
	//msg.CurrentTime = COleDateTime::GetTickCount();

	MsgOutQueue.Push(&msg);;
	return TRUE;
}

// �鿴��������
UdpAdapter::NetAdapterType UdpAdapter::GetType()
{
	return NET_ADAPTER_TYPE_UDP;
}

// ѭ�������߳�
void UdpAdapter::RecvThread(LPVOID WorkThreadContext)
{
	UdpAdapter *adapter = (UdpAdapter *)WorkThreadContext;
	adapter->RecvUdp();
}

// ѭ�������߳�
void UdpAdapter::SendThread(LPVOID WorkThreadContext)
{
	UdpAdapter *adapter = (UdpAdapter *)WorkThreadContext;
	adapter->SendUdp();
}

// ��������
void UdpAdapter::Start()
{
	thread recv_thread(&UdpAdapter::RecvThread, (LPVOID)this);
	recv_thread.detach();

	thread send_thread(&UdpAdapter::SendThread, (LPVOID)this);
	send_thread.detach();
}

// �رշ���
void UdpAdapter::Stop()
{
	//
}
