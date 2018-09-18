#include "stdafx.h"
#include <sys/time.h>
#include <syslog.h>
#include "RedisClient.hpp"
#include "MainServ.h"
#include "json.h"

using namespace std;
using namespace std::tr1;

#define USEMACEX 0
#define LOST_CONNECTION_TIMEOUT 30

BOOL OUTPUTLOGFILE = FALSE;
unsigned long nowsec = 0;
int updateflag = 0;

unsigned int GetSequenceID()
{
	static unsigned int seq = 1;
	if(seq >= 0xFFFFFFFF)
		seq = 1;
	return seq ++;
}

unsigned long long MacToInt64(const char *mac)
{
	unsigned long long mac64 = 0;
	char Mac[32] = {0};
	if(!mac || !mac[0])
		return 0;

	char *mac_ptr = (char *)mac;
	char *Mac_ptr = Mac;
	while(*mac_ptr)
	{
		if((*mac_ptr) == ':' || (*mac_ptr) == '.' || (*mac_ptr) == '-')
		{
			mac_ptr ++;
			continue;
		}
		*(Mac_ptr ++) = *(mac_ptr ++);
	}

	sscanf(Mac, "%llx", &mac64);

	return mac64;
}

void Int64ToMac(unsigned long long i64, char tokken, char *mac)
{
	unsigned char *i64_ptr = (unsigned char *)&i64;

	memset(mac, 0, 20);
	for(int i = 5; i >= 0; i --)
	{
		if(i < 7)
			mac[strlen(mac)] = tokken;
		sprintf(mac + strlen(mac), "%02x", i64_ptr[i]);
	}
}

void Int64ToString(unsigned long long i64, char *outstr, int data_length)
{
	unsigned char *i64_ptr = (unsigned char *)&i64;

	memset(outstr, 0, 20);
	for(int i = data_length; i >= 0; i --)
	{
		sprintf(outstr + strlen(outstr), "%02x", i64_ptr[i]);
	}
}

char *Int64ToMac(unsigned long long i64, char tokken)
{
	static char mac[32];

	memset(mac, 0, 32);
	Int64ToMac(i64, tokken, mac);

	return mac;
}

char *Int64ToMac(unsigned long long i64)
{
	static char mac[32];

	memset(mac, 0, 32);
	Int64ToMac(i64, ':', mac);

	return mac;
}

// 替换字符串中的指定内容
void StringReplace(char *strori, const char *strsrc, const char *strdst)
{
	std::string::size_type pos = 0;
	std::string strBig = string(strori);
	unsigned int srclen = strlen(strsrc), dstlen = strlen(strdst), orilen = strlen(strori);

	while( (pos = strBig.find(strsrc, pos)) != string::npos)
	{
		strBig.replace(pos, srclen, strdst);
		pos += dstlen;
	}

	memset(strori, 0, orilen);
	strncpy(strori, strBig.c_str(), orilen);
}

int VerCmp(char *ver1, char *ver2)
{
	int a1 = 0, a2 = 0, a3 = 0, a4 = 0;
	int b1 = 0, b2 = 0, b3 = 0, b4 = 0;
	char v1[32] = {0};
	char v2[32] = {0};

	strncpy(v1, ver1 + 1, 30);
	strncpy(v2, ver2 + 1, 30);

	{
		char *vind = v1;
		while(*vind)
		{
			if((*vind) == '.')
				(*vind) = ',';
			vind ++;
		}

		vind = v2;
		while(*vind)
		{
			if((*vind) == '.')
				(*vind) = ',';
			vind ++;
		}

		sscanf(v1, "%d,%d,%d,%d", &a1, &a2, &a3, &a4);
		sscanf(v2, "%d,%d,%d,%d", &b1, &b2, &b3, &b4);
	}

	if(a1 > b1)
		return -1;
	else if(a1 < b1)
		return 1;

	if(a2 > b2)
		return -1;
	else if(a2 < b2)
		return 1;

	if(a3 > b3)
		return -1;
	else if(a3 < b3)
		return 1;

	if(a4 > b4)
		return -1;
	else if(a4 < b4)
		return 1;

	return 0;
}

int CheckTimeSpan(int st_time, int ed_time)
{
	time_t timep;
	struct tm *p_tm;
	int now_time;

	timep = time(NULL);
	p_tm = localtime(&timep); /*获取本地时区时间*/

	now_time = p_tm->tm_hour * 100 + p_tm->tm_min;
    if(st_time <= now_time && now_time <= ed_time)
		return 1;
	else
		return 0;
}

void ServSysCheckTh(LPVOID WorkThreadContext)
{
	MainServ *server = (MainServ *)WorkThreadContext;

	int counter = 0;
	struct timeval tnow;
	for(;;)
	{
		counter ++;

		if(counter > 20)
		{
			counter = 0;
			
			if(CheckTimeSpan(UPDATE_START_TIME, UPDATE_END_TIME))
				updateflag = 3;
		}

		gettimeofday( &tnow, NULL );
		nowsec = tnow.tv_sec;
		SetNowTimeStr();
		usleep(500000);
	}
}

int CheckModel(char *model_id)
{
	static char Models[1024] = {0};

	if(model_id[0] == '_')
		return 1;

	if(strstr(Models, model_id))
		return 1;
	else
	{
		snprintf(Models + strlen(Models), 64, ",%s", model_id);
		MOUTPUT("Add model - %s .", model_id);
		return 0;
	}
}


void ReadModels()
{
	MYSQL_RES *res_ptr;
	MYSQL_ROW sqlrow;

	MYSQL conn;
	mysql_init(&conn);
	ConnectDB(&conn);

	unsigned long long res = QueryDB(&conn, "SELECT DM_ID FROM gx_wifi_home.bp_device_model");
	if(res > 0)
	{
		res_ptr = mysql_use_result(&conn);

		if (res_ptr) {
			int first = 1;
			while ((sqlrow = mysql_fetch_row(res_ptr)))
			{
                CheckModel(sqlrow[0]);
            }
			if (mysql_errno(&conn))
			{
				MOUTPUT("Retrive error: %s", mysql_error(&conn));
			}
			mysql_free_result(res_ptr);
		}
	}

	mysql_close(&conn);
	MOUTPUT("Mysql connection closed!");
}


// Main Serv
MainServ::MainServ()
{
	ReadConfigFile();

	ReadModels();

	HdlUdpService = new ClientService(NetAdapter::NET_ADAPTER_TYPE_UDP, UDP_SERV_PORT, &ParseUdpMsg, this, NULL, NULL, this);
	HdlUdpService->Start();

	thread serv_thread(&ServSysCheckTh, (LPVOID)this);
	serv_thread.detach();
}

MainServ::~MainServ()
{

	SAFEDELETE(HdlUdpService)
}

BOOL MainServ::FindRemoteDevice(const char *pid, LP_REMOTE_DEVICE lp_dev)
{
	if(!lp_dev)
		return FALSE;

	m_csDevices.lock();
	unordered_map <Uint64, RemoteDevice>::iterator vi = Devices.find(MacToInt64(pid));
	if(vi != Devices.end())
	{
		memcpy(lp_dev, &(vi->second), sizeof(REMOTE_DEVICE));
		m_csDevices.unlock();
		return TRUE;
	}
	m_csDevices.unlock();

	return FALSE;
}

BOOL MainServ::FindRemoteDevice(SockAddr *addr, LP_REMOTE_DEVICE lp_dev)
{
	if(!lp_dev)
		return FALSE;

	m_csDevices.lock();
	unordered_map <Uint64, RemoteDevice>::iterator _end = Devices.end();
	for (unordered_map <Uint64, RemoteDevice>::iterator itr = Devices.begin(); itr != _end; itr++)
	{
		if(itr->second.addr == (* addr))
		{
			memcpy(lp_dev, &(itr->second), sizeof(REMOTE_DEVICE));
			m_csDevices.unlock();
			return TRUE;
		}
	}
	m_csDevices.unlock();

	return FALSE;
}

void MainServ::SetRemoteDevice(LP_REMOTE_DEVICE lpdev)
{
	m_csDevices.lock();
	BOOL found = FALSE;

	unordered_map <Uint64, RemoteDevice>::iterator vi = Devices.find(MacToInt64(lpdev->MAC));
	if(vi != Devices.end())
	{
		vi->second.addr = lpdev->addr;
		// 设置时间
		vi->second.LastTime = nowsec;

		// 检查指令缓存
		if(vi->second.CMD[0])
		{
			memcpy(lpdev->CMD, vi->second.CMD, 2048);
			memset(vi->second.CMD, 0, 2048);
		}

		found = TRUE;
	}
	if(!found) // 无则加之
	{
		REMOTE_DEVICE rd;
		memcpy(&rd, lpdev, sizeof(REMOTE_DEVICE));
		// 设置时间
		rd.LastTime = nowsec;

		Devices.insert(make_pair(MacToInt64(lpdev->MAC), rd));
	}
	m_csDevices.unlock();
}

BOOL MainServ::SetRemoteDeviceB(LP_REMOTE_DEVICE lpdev)
{
	m_csDevices.lock();
	BOOL found = FALSE;

	unordered_map <Uint64, RemoteDevice>::iterator vi = Devices.find(MacToInt64(lpdev->MAC));
	if(vi != Devices.end())
	{
		vi->second.addr = lpdev->addr;
		vi->second.LastTime = nowsec;

		memcpy(vi->second.STAT, lpdev->STAT, 2048);

		if(vi->second.CMD[0])
		{
			memcpy(lpdev->CMD, vi->second.CMD, 2048);
			memset(vi->second.CMD, 0, 2048);
		}

		found = TRUE;
	}
	if(!found) 
	{
		REMOTE_DEVICE rd;
		memcpy(&rd, lpdev, sizeof(REMOTE_DEVICE));

		rd.LastTime = nowsec;

		Devices.insert(make_pair(MacToInt64(lpdev->MAC), rd));
	}
	m_csDevices.unlock();

	return found;
}

void MainServ::SetRemoteDeviceAddr(char *pid, SockAddr addr)
{
	m_csDevices.lock();

	unordered_map <Uint64, RemoteDevice>::iterator vi = Devices.find(MacToInt64(pid));
	if(vi != Devices.end())
	{
		vi->second.addr = addr;
		// 设置时间
		vi->second.LastTime = nowsec;

	}
	else // 无则加之
	{
		REMOTE_DEVICE rd;
		memset(&rd, 0, sizeof(REMOTE_DEVICE));
		strncpy(rd.MAC, pid, 30);
		rd.addr = addr;
		// 设置时间
		rd.LastTime = nowsec;

		Devices.insert(make_pair(MacToInt64(pid), rd));
	}
	m_csDevices.unlock();
}

void MainServ::SetRemoteDeviceCmd(char *pid, char *cmd)
{
	m_csDevices.lock();

	unordered_map <Uint64, RemoteDevice>::iterator vi = Devices.find(MacToInt64(pid));
	if(vi != Devices.end())
	{
		memset(vi->second.CMD, 0, 2048);
		strncpy(vi->second.CMD, cmd, 2046);
	}
	else // 无则加之
	{
		REMOTE_DEVICE rd;
		memset(&rd, 0, sizeof(REMOTE_DEVICE));
		strncpy(rd.MAC, pid, 30);
		strncpy(rd.CMD, cmd, 2046);

		Devices.insert(make_pair(MacToInt64(pid), rd));
	}
	m_csDevices.unlock();
}

BOOL MainServ::SetRemoteDeviceSmartDevs(char *pid, char *smart_devs)
{
	BOOL found = FALSE;
	m_csDevices.lock();

	unordered_map <Uint64, RemoteDevice>::iterator vi = Devices.find(MacToInt64(pid));
	if(vi != Devices.end())
	{
		memset(vi->second.SDEVS, 0, 2048);
		strncpy(vi->second.SDEVS, smart_devs, 2046);
		found = TRUE;
	}
	else // 无则加之
	{
		REMOTE_DEVICE rd;
		memset(&rd, 0, sizeof(REMOTE_DEVICE));
		strncpy(rd.MAC, pid, 30);
		strncpy(rd.SDEVS, smart_devs, 2046);

		Devices.insert(make_pair(MacToInt64(pid), rd));
	}
	m_csDevices.unlock();
	return found;
}

void MainServ::RemoveDevice(char *pid)
{
	m_csDevices.lock();
	unordered_map <Uint64, RemoteDevice>::iterator vi = Devices.find(MacToInt64(pid));
	if(vi != Devices.end())
	{
		Devices.erase(vi);
		m_csDevices.unlock();
		return;
	}
	m_csDevices.unlock();
}

void MainServ::RemoveDevice(SockAddr *addr)
{
	m_csDevices.lock();
	unordered_map <Uint64, RemoteDevice>::iterator _end = Devices.end();
	for (unordered_map <Uint64, RemoteDevice>::iterator itr = Devices.begin(); itr != _end; itr++)
	{
		if(itr->second.addr == (*addr))
		{
			Devices.erase(itr);
			m_csDevices.unlock();
			return;
		}
	}
	m_csDevices.unlock();
}

// UDP收包处理方法
void MainServ::ParseUdpMsg(NetMsg *pNetMsg, void *LPCONN, void *LPCONN2, void *PARAM1, void *PARAM2)
{
	MainServ *server = (MainServ *)PARAM1;
	MYSQL *lpconn = (MYSQL *)LPCONN;
	CRedisClient *lp_redis_client = (CRedisClient *)LPCONN2;

	signed long long res;
	char sqlstr[2048];

	char jsonname[32] = {0};
	char proclass[32] = {0};
	unsigned long long mac64 = 0;

	RemoteDevice c_dev;
	memset(&c_dev, 0, sizeof(REMOTE_DEVICE));
	c_dev.addr = pNetMsg->Source.RemoteAddr;

	JsonItem jitem, *lp_subparas, *lp_endparas;
	jitem.LoadFromStringF(pNetMsg->Buffer);
	if (jitem.SubItemCount == 0)
	{
		MOUTPUT("Not protocol data, ignored!");
		return;
	}

	jitem.GetAttrByName("serialnumber", c_dev.MAC, 30);
	jitem.GetAttrByName("name", jsonname, 30);
	jitem.GetAttrByName("ProductClass", proclass, 30);
	if(!proclass[0])
		jitem.GetAttrByName("productClass", proclass, 30);

	if (!c_dev.MAC[0])
	{
		MOUTPUT("MAC not found, ignored!");
		return;
	}

	if(USEMACEX && proclass[0])
	{
		char mac_ex[32] = {0};
		if(strstr(proclass, "410"))
			strncpy(mac_ex, "01", 30);
		else if(strstr(proclass, "420"))
			strncpy(mac_ex, "02", 30);

		strncpy(mac_ex + strlen(mac_ex), c_dev.MAC, 30);
		strncpy(c_dev.MAC, mac_ex, 30);
	}

	mac64 = MacToInt64(c_dev.MAC);

	if(!strcmp(jsonname, "inform"))
	{
		char version[32] = {0};
		char wanip[32] = {0};
		int wifi_client_count = 0;
		char Mac[32] = {0};

		char Manufacturer[32] = {0};
		char ProductClass[32] = {0};
		char models_id[64] = {0};

		strncpy(c_dev.STAT, pNetMsg->Buffer, 2000);

		if(!jitem.GetSubItemByName("packet", &lp_subparas))
			return;

		lp_subparas->GetAttrByName("SoftwareVersion", version, 30);
		lp_subparas->GetAttrByName("wan_current_ip_addr", wanip, 30);

		lp_subparas->GetAttrByName("Manufacturer", Manufacturer, 30);
		lp_subparas->GetAttrByName("ProductClass", ProductClass, 30);
		snprintf(models_id, 62, "%s_%s", Manufacturer, ProductClass);

		if(updateflag > 0 && strstr(ProductClass, "410") && VerCmp(version, "v2.1.29") > 0)
		{
			updateflag --;
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			snprintf(pNetMsg->Buffer, 2000, "{\"jtime\":%llu265,\"keyname\":\"download\",\"name\":\"set\",\"packet\":{\"FileSize\":\"6029316\",\"md5\":\"112254442425251\",\"url\":\"http://113.98.195.201:9091/wifi_home_gx//upload/file/backpage/D12_7628n_8m_IJLY410_v2.1.29_180403.bin\"},\"serialnumber\":\"%s\",\"version\":\"1.0\"}", nowsec, c_dev.MAC);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);
			return;
		}

		if(USEMACEX)
		{
			char mac_ex[32] = {0};
			if(strstr(ProductClass, "410"))
				strncpy(mac_ex, "01", 30);
			else if(strstr(ProductClass, "420"))
				strncpy(mac_ex, "02", 30);

			strncpy(mac_ex + strlen(mac_ex), c_dev.MAC, 30);
			strncpy(c_dev.MAC, mac_ex, 30);

			mac64 = MacToInt64(c_dev.MAC);
		}

		if(!server->SetRemoteDeviceB(&c_dev))
		{
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			snprintf(pNetMsg->Buffer, 2000, "{\"jtime\":%llu265,\"keyname\":\"config\",\"name\":\"set\",\"packet\":{\"InternetGatewayDevice.DeviceInfo.debug_command\":\"f=0;if@[@88@==@$(uci@get@wireless.ra0.KickStaRssiLow)@]@;then@uci@set@wireless.ra0.AssocReqRssiThres=;uci@set@wireless.ra0.KickStaRssiLow=;f=1;fi;var=$(uci@-q@get@wireless.ra0.ht);if@[@20+40@!=@$var@];then@uci@set@wireless.ra0.ht=20+40;f=1;fi;if@[@$f@==@1@];then@uci@commit@wireless;/sbin/wifi;fi\",\"parameter_name_multi\":\"1\"},\"serialnumber\":\"%s\",\"version\":\"1.0\"}", nowsec, c_dev.MAC);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);
		}

		if(!lp_subparas->GetSubItemByName("WiFiClient", &lp_endparas))
			return;

		wifi_client_count = lp_endparas->SubItemCount;

		// Cached commands found ?
		if(c_dev.CMD[0])
		{
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			strncpy(pNetMsg->Buffer, c_dev.CMD, 2048);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);

			server->SetRemoteDeviceCmd(c_dev.MAC, "");
		}
		else
		{
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			snprintf(pNetMsg->Buffer, 2048, "{\"name\":\"informResponse\",\"version\":\"1.0.0\",\"serialnumber\":\"%s\",\"keyname\":\"inform\",\"packet\":{\"serialnumber\":\"%s\"}}", c_dev.MAC, c_dev.MAC);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);
		}

		if(!CheckModel(models_id))
		{
			memset(sqlstr, 0, 2048);
			snprintf(sqlstr, 2046, "INSERT INTO gx_wifi_home.bp_device_model  ( DM_ID, PROVIDER, MODEL, MODEL_MARK ) VALUES ( '%s', '%s', '%s', '%s' )",
				models_id, Manufacturer, ProductClass, Manufacturer);
			res = QueryDB(lpconn, sqlstr);
			if(res < 0)
			{
				MOUTPUTA("Models %s insert error! %s\n%s", models_id, (mysql_error(lpconn)), sqlstr);
				mysql_ping(lpconn);
				return;
			}
		}

		// write to db
		memset(sqlstr, 0, 2048);
		snprintf(sqlstr, 2046, "UPDATE gx_wifi_home.bp_device_m SET IP = '%s', ROUTER_VERSION = '%s', ONLINE_NUM = %d, REPORT_DATE = NOW() WHERE MAC64 = %llu",
			wanip, version, wifi_client_count, mac64);

		res = QueryDB(lpconn, sqlstr);
		if(res < 0)
		{
			MOUTPUTA("%llx update error! %s\n%s", mac64, (mysql_error(lpconn)), sqlstr);
			mysql_ping(lpconn);
			return;
		}
		else if(res)
		{
			MOUTPUT("%llx updated.", mac64)
			return;
		}

		strncpy(Mac, c_dev.MAC, 30);
		StringReplace(Mac, ":", "");

		memset(sqlstr, 0, 2048);
		snprintf(sqlstr, 2046, "INSERT INTO gx_wifi_home.bp_device_m  ( MAC, REPORT_DATE, IP, ROUTER_VERSION, DEV_MODEL_ID, BITSTATE, ONLINE_NUM, MAC64 ) VALUES ( '%s', NOW(), '%s', '%s', '%s', 0, %d, %llu )",
			Mac, wanip, version, models_id, wifi_client_count, mac64);
		res = QueryDB(lpconn, sqlstr);
		if(res < 0)
		{
			MOUTPUTA("%llX insert error! %s\n%s", mac64, (mysql_error(lpconn)), sqlstr);
			mysql_ping(lpconn);
			return;
		}
		else if(res)
		{
			MOUTPUT("%llX inserted.", mac64)
		}
	}
	else if(!strcmp(jsonname, "update_smartdevs"))
	{
		JsonItem *lp_jitem = NULL;
		int devlen = 0;
		int hRet = 0;
		char MAC[32] = {0}, SubId[32] = {0};
		std::string strKey = "";
		std::string strField = "";

		if(!jitem.GetSubItemByName("devs", &lp_jitem))
		{
			MOUTPUT("No smart device!");
		}

		snprintf(MAC, 64, "udp.smart.devs.%s", c_dev.MAC);
		StringReplace(MAC, ":", "");
		strKey = MAC;

		if(!server->SetRemoteDeviceSmartDevs(c_dev.MAC, pNetMsg->Buffer))
		{
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			snprintf(pNetMsg->Buffer, 2000, "{\"jtime\":%llu265,\"keyname\":\"config\",\"name\":\"set\",\"packet\":{\"InternetGatewayDevice.DeviceInfo.debug_command\":\"f=0;if@[@88@==@$(uci@get@wireless.ra0.KickStaRssiLow)@]@;then@uci@set@wireless.ra0.AssocReqRssiThres=;uci@set@wireless.ra0.KickStaRssiLow=;f=1;fi;var=$(uci@-q@get@wireless.ra0.ht);if@[@20+40@!=@$var@];then@uci@set@wireless.ra0.ht=20+40;f=1;fi;if@[@$f@==@1@];then@uci@commit@wireless;/sbin/wifi;fi\",\"parameter_name_multi\":\"1\"},\"serialnumber\":\"%s\",\"version\":\"1.0\"}", nowsec, c_dev.MAC);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);
		}

		if((hRet = lp_redis_client->Del(strKey)) != RC_SUCCESS)
			MOUTPUTA("\n Redis delete key %s failed! [%d]", MAC, hRet);
		devlen = lp_jitem->SubItemCount;
		for(int i = 0; i < devlen; i ++)
		{
			lp_jitem->SubItems[i].GetAttrByName("id", SubId, 30);
			if(SubId[0])
			{
				printf("\n Dev %d: %s %s\n%s\n", i+1, MAC, SubId, lp_jitem->SubItems[i].JsonString.c_str());
				strField = SubId;

				if((hRet = lp_redis_client->Hset(strKey, strField, lp_jitem->SubItems[i].JsonString)) != RC_SUCCESS)
					MOUTPUTA("\n Redis Hset %s %s failed! [%d]", MAC, SubId, hRet);
			}
		}

		// Cached commands found ?
		if(c_dev.CMD[0])
		{
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			strncpy(pNetMsg->Buffer, c_dev.CMD, 2048);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);

			server->SetRemoteDeviceCmd(c_dev.MAC, "");
		}
		else
		{
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			snprintf(pNetMsg->Buffer, 2048, "{\"name\":\"smartdevsResponse\",\"version\":\"1.0.0\",\"serialnumber\":\"%s\",\"keyname\":\"update_smartdevs\",\"packet\":{\"serialnumber\":\"%s\"}}", c_dev.MAC, c_dev.MAC);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);
		}
	}
	else if(!strcmp(jsonname, "get_smartdevs"))
	{
		//
	}
	else if(!strcmp(jsonname, "set") || !strcmp(jsonname, "get") || !strcmp(jsonname, "down") || !strcmp(jsonname, "homesmart"))
	{
		RemoteDevice rd;
		if(!server->FindRemoteDevice(c_dev.MAC, &rd))
		{
			MOUTPUT("Device %s not found. Command transfered failed.", c_dev.MAC);
			server->SetRemoteDeviceCmd(c_dev.MAC, pNetMsg->Buffer);
			return;
		}

		pNetMsg->Source.RemoteAddr.sin_addr.s_addr = rd.addr.addr;
		pNetMsg->Source.RemoteAddr.sin_port = rd.addr.port;
		pNetMsg->Source.pAdapter->SendData(pNetMsg);
	}
	else if(!strcmp(jsonname, "get_status"))
	{
		RemoteDevice rd;
		if(!server->FindRemoteDevice(c_dev.MAC, &rd))
		{
			MOUTPUT("Device %s not found. Command transfered failed.", c_dev.MAC);
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			snprintf(pNetMsg->Buffer, 2048, "{\"name\":\"getstatusResponse\",\"version\":\"1.0.0\",\"serialnumber\":\"%s\",\"keyname\":\"get_status\",\"packet\":{\"error\":\"%s device not found.\"}}", c_dev.MAC, c_dev.MAC);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);
		}
		else
		{
			memset(pNetMsg->Buffer, 0, MAX_BUFF_SIZE);
			memcpy(pNetMsg->Buffer, rd.STAT, 2048);
			pNetMsg->Length = strlen(pNetMsg->Buffer);
			pNetMsg->Source.pAdapter->SendData(pNetMsg);
		}

		return;
	}
}

