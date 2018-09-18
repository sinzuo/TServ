#include <iostream>
#include "MainServ.h"
#include <mysql/mysql.h>
#include <sys/time.h>
#include <syslog.h>
#include "json.h"
#include "RedisClient.hpp"

using namespace std;

int SysClosing = 0;

extern unsigned long long MacToInt64(const char *mac);
extern void Int64ToMac(unsigned long long i64, char tokken, char *mac);
extern void StringReplace(char *strori, const char *strsrc, const char *strdst);
extern void ReadConfigFile();

CRedisClient *lp_redis_client;

int main()
{
	if(0){
	MYSQL conn;
	int res;
	unsigned long long affect_rows;
	mysql_init(&conn);
	char sqlstr[2048];
	if(mysql_real_connect(&conn,"192.168.3.180","root","root","gx_wifi_home",0,NULL,CLIENT_FOUND_ROWS)) //"root":数据库管理员 "":root密码 "test":数据库的名字
	{
		struct timeval tstart, tend;
		printf("connect success!\n");
		gettimeofday( &tstart, NULL );

		for(int i = 0; i < 1000; i ++)
		{
			unsigned long long mac64 = 0x666666666666;
			memset(sqlstr, 0, 2048);
			sprintf(sqlstr, "UPDATE gx_wifi_home.bp_device_m SET IP = '10.0.68.30', ROUTER_VERSION = 'V1.1.0-20170612', BITSTATE = 0, ONLINE_NUM = 66, REPORT_DATE = NOW(), MAC64 = %llu WHERE MAC = '%llX'", mac64 + i, mac64 + i);
			res = mysql_query(&conn,sqlstr);
			affect_rows = mysql_affected_rows(&conn);
			if(res)
				printf("%llX update error!\n");
		}

		gettimeofday( &tend, NULL );

		mysql_close(&conn);
		printf("Time: %d.%06d - %d.%06d\n", tstart.tv_sec, tstart.tv_usec, tend.tv_sec, tend.tv_usec);
	}
	return 0;
	}

	if(0){
	CRedisClient redisCli;
	char *jsonstr = "{\"name\": \"update_smartdevs\", \"serialnumber\": \"D8:D8:66:02:89:7C\", \"devs\":[{\"id\":\"60:01:94:24:59:29\",\"devtype\":\"1\",\"statstr\":\"1\"},{\"id\":\"60:01:94:24:59:30\",\"devtype\":\"1\",\"statstr\":\"1\"}]}";
	JsonItem jitem, *lp_subparas, *lp_endparas;
	jitem.LoadFromStringF(jsonstr);
	int devlen = 0;

	char MAC[32] = {0}, SubId[32] = {0}, KEYID[64] = {0};
	jitem.GetAttrByName("serialnumber", MAC, 30);

	StringReplace(MAC, ":", "");
	snprintf(KEYID, 62, "udp.smart.devs.%s", MAC);

	std::string strKey = KEYID;
	std::string strField = "";

	if(!jitem.GetSubItemByName("devs", &lp_subparas))
		return 0;

	if (!redisCli.Initialize("192.168.3.68", 6379, 2, 10))
	{
		printf("\nconnect to redis failed\n");
		return -1;
	}
	redisCli.Del(strKey);
	devlen = lp_subparas->SubItemCount;
	for(int i = 0; i < devlen; i ++)
	{
		lp_subparas->SubItems[i].GetAttrByName("id", SubId, 30);
		if(SubId[0])
		{
			printf("\n Dev %d: %s %s\n%s\n", i+1, KEYID, SubId, lp_subparas->SubItems[i].JsonString.c_str());
			strField = SubId;

			if(redisCli.Hset(strKey, strField, lp_subparas->SubItems[i].JsonString) == RC_SUCCESS)
				printf("\n Redis Hset OK!\n");
		}
	}
	return 0;
	}

	unsigned int SequenceId = 1;

	openlog("tservlog", LOG_CONS | LOG_PID, 0);
	SetNowTimeStr();
	MainServ mainserv;

	while(1)
	{
		char cc = getchar();
		if(cc == 'q')
			break;
		else if(cc == 'i')
		{
			char *jsonstr = "{\"name\": \"inform\", \"serialnumber\": \"11:22:33:44:55:66\", \"packet\": { \"SoftwareVersion\": \"v1.1.0-20170612\", \"wan_current_ip_addr\": \"10.0.68.30\", \"WiFiClient\": [ { \"mac\": \"60:6D:C7:35:C5:A7\", \"type\": \"0\", \"host\": \"\", \"ip\": \"\" }, { \"mac\": \"74:51:BA:E1:E3:26\", \"type\": \"0\", \"host\": \"\", \"ip\": \"\" }]}}";
			NetMsg msg;
			memset(&msg, 0, sizeof(NetMsg));
			strcpy(msg.Buffer, jsonstr);
			msg.Length = strlen(msg.Buffer);
			msg.Source.pAdapter = mainserv.HdlUdpService->m_pNetAdapter;
			mainserv.HdlUdpService->InMsgQueue.Push(&msg);
		}
		else if(cc == 'l')
		{
			char *jsonstr = "{\"name\": \"inform\", \"serialnumber\": \"%s\", \"packet\": { \"SoftwareVersion\": \"v1.1.0-20170612\", \"wan_current_ip_addr\": \"10.0.68.30\", \"WiFiClient\": [ { \"mac\": \"60:6D:C7:35:C5:A7\", \"type\": \"0\", \"host\": \"\", \"ip\": \"\" }, { \"mac\": \"74:51:BA:E1:E3:26\", \"type\": \"0\", \"host\": \"\", \"ip\": \"\" }]}}";
			unsigned long long mac64 = 0x666666666666;
			char mac[32] = {0};
			for(int i = 0; i < 100000; i ++)
			{
				NetMsg msg;
				memset(&msg, 0, sizeof(NetMsg));
				memset(mac, 0, 32);
				Int64ToMac(mac64 + i, ':', mac);
				snprintf(msg.Buffer, 1024, jsonstr, mac);
				msg.Length = strlen(msg.Buffer);
				msg.Source.pAdapter = mainserv.HdlUdpService->m_pNetAdapter;
				mainserv.HdlUdpService->InMsgQueue.Push(&msg);
			}
		}
		else if(cc == 'b')
		{
			char sendbuf[256] = {0};
			snprintf(sendbuf, 200, "{\"sid\":\"%d\",\"id\":\"21:3F:7C:2E:1F:07\",\"ver\":\"16\",\"devtype\":\"1\",\"cmdtype\":\"4097\",\"statstr\":\"\"}", (SequenceId ++));
			((UdpAdapter *)mainserv.HdlUdpService->m_pNetAdapter)->SendData("192.168.3.242", 8880, sendbuf, strlen(sendbuf));
		}
		else if(cc == 'n')
		{
			char sendbuf[256] = {0};
			snprintf(sendbuf, 200, "{\"sid\":\"%d\",\"id\":\"21:3F:7C:2E:1F:09\",\"ver\":\"16\",\"devtype\":\"2\",\"cmdtype\":\"4097\",\"statstr\":\"\"}", (SequenceId ++));
			((UdpAdapter *)mainserv.HdlUdpService->m_pNetAdapter)->SendData("192.168.3.242", 8880, sendbuf, strlen(sendbuf));
		}
		else if(cc == 'm')
		{
			char *jsonstr = "{\"name\":\"update_smartdevs\",\"serialnumber\":\"11:22:33:44:55:66\",\"devs\":[{\"sid\":\"26\",\"id\":\"21:3F:7C:2E:1F:07\",\"ver\":\"16\",\"devtype\":\"1\",\"cmdtype\":\"4097\",\"statstr\":\"\"}]}";
			NetMsg msg;
			memset(&msg, 0, sizeof(NetMsg));
			strcpy(msg.Buffer, jsonstr);
			msg.Length = strlen(msg.Buffer);
			msg.Source.pAdapter = mainserv.HdlUdpService->m_pNetAdapter;
			mainserv.HdlUdpService->InMsgQueue.Push(&msg);
		}
	}

	SysClosing = 1;
	closelog();
    return 0;
}
