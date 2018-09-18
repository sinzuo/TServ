#include "stdafx.h"
#include "NetAdapter.h"

char time_string_buff_a[64];
char time_string_buff_b[64];
char *time_string_buff;

BOOL NetAdapter::SendData(NetMsg *pNetMsg)
{
	return TRUE;
}

NetAdapter::NetAdapterType NetAdapter::GetType()
{
	return NET_ADAPTER_TYPE_UNKNOWN;
}

void NetAdapter::Start()
{
	return;
}

void NetAdapter::Stop()
{
	return;
}





MsgQueue::MsgQueue()
{
	sem_init(&waitHandleMQ, 0, 0);

	ExitSymbol = FALSE;
}

MsgQueue::~MsgQueue()
{
	ExitSymbol = TRUE;
}

void MsgQueue::Push(NetMsg *lpmsg)
{
	NetMsg msg;
	memcpy(&msg, lpmsg, sizeof(NetMsg));
	m_csMQ.lock();
	MessageQueue.push(msg);
	// 不管是否为空，清掉阻塞，有数据可读了
	sem_post(&waitHandleMQ);
	m_csMQ.unlock();
}

void MsgQueue::Pop(NetMsg *lpmsg)
{
	while(1)
	{
		// 如果没有新数据就等待
		sem_wait( &waitHandleMQ );
		if(ExitSymbol) return;

		m_csMQ.lock();
		if(MessageQueue.empty())
		{
			m_csMQ.unlock();
			continue;
		}
		memcpy(lpmsg, &MessageQueue.front(), sizeof(NetMsg));
		MessageQueue.pop();
		m_csMQ.unlock();
		break;
	}
}

void MsgQueue::Clear()
{
	while(!MessageQueue.empty())
		MessageQueue.pop();

	sem_init(&waitHandleMQ, 0, 0);
}


void MsgQueue::Dispose()
{
	while(!MessageQueue.empty())
		MessageQueue.pop();

	sem_destroy(&waitHandleMQ);
}


char *SetNowTimeStr()
{
	time_t timep;
	struct tm *p_tm;
	char *tsb = time_string_buff;
	timep = time(NULL);
	p_tm = localtime(&timep); /*获取本地时区时间*/

	if(tsb == time_string_buff_a)
		tsb = time_string_buff_b;
	else
		tsb = time_string_buff_a;

	memset(tsb, 0, 64);
    sprintf(tsb, "%04d%02d%02d%02d%02d%02d", (p_tm->tm_year+1900), (p_tm->tm_mon+1), p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
	time_string_buff = tsb;
	
    return time_string_buff;
}

char *GetNowTimeStr()
{
    return time_string_buff;
}
