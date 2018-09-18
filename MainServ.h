#ifndef _MAIN_SERVICE
#define _MAIN_SERVICE

#define Uint64 unsigned long long
#include "ClientService.h"
#include <tr1/unordered_map>

typedef struct RemoteDevice
{
	SockAddr addr;
	unsigned long LastTime;
	char MAC[32];
	char CMD[2048];
	char SDEVS[2048];
	char STAT[2048];
} REMOTE_DEVICE, *LP_REMOTE_DEVICE;

class MainServ
{
public:
	MainServ();
	~MainServ();

	// 定义网络接口服务
	ClientService *HdlUdpService;

	// 对设备，App Map集合的操作
	BOOL FindRemoteDevice(const char *pid, LP_REMOTE_DEVICE lp_dev);
	BOOL FindRemoteDevice(SockAddr *addr, LP_REMOTE_DEVICE lp_dev);
	void SetRemoteDevice(LP_REMOTE_DEVICE lpdev);
	BOOL SetRemoteDeviceB(LP_REMOTE_DEVICE lpdev);
	void SetRemoteDeviceAddr(char *pid, SockAddr addr);
	void SetRemoteDeviceCmd(char *pid, char *cmd);
	BOOL SetRemoteDeviceSmartDevs(char *pid, char *smart_devs);
	void RemoveDevice(char *pid);
	void RemoveDevice(SockAddr *addr);

private:
	// 定义回调函数
	static void ParseUdpMsg(NetMsg *pNetMsg, void *LPCONN, void *LPCONN2, void *PARAM1, void *PARAM2);

	// 访问临界区
	std::mutex m_csDevices;

	// 设备的MAP集合
	std::tr1::unordered_map <Uint64, RemoteDevice> Devices;

};
#endif

