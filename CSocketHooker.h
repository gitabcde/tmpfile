#pragma once

#include "sockhook.h"
#include <set>
#include <map>
#include <vector>
#include <Windows.h>

using namespace std;

///回调节点
typedef struct {
	void* pParam; ///< 回调参数
	void* pCallBack; ///< 回调函数地址
} socket_callback_t;

///回调集合
typedef set<socket_callback_t> socket_callback_set;


///WSARecv的缓冲区
typedef struct  
{
	SOCKET   socket;
	LPWSABUF lpBuffers;
	DWORD dwBufferCount;
} OverlappedBuffers;

///Overlapped 和 WSARecv缓冲区的映射
typedef map<LPWSAOVERLAPPED, OverlappedBuffers> overlapped_buffers_map;

/*!
注册和反注册回调
*/
class CSocketHooker
{
private:
	CSocketHooker(void);
private:
	virtual ~CSocketHooker(void);
public:
	bool AddConnectHook(void* pParam, socket_connect_hook_proc_t onConnect);
	bool RemoveConnectHook(void* pParam, socket_connect_hook_proc_t onConnect);
	bool AddRecvHook(void* pParam, socket_recv_hook_proc_t onRecv);
	bool RemoveRecvHook(void* pParam, socket_recv_hook_proc_t onRecv);
	bool AddSendHook(void* pParam, socket_send_hook_proc_t onSend);
	bool RemoveSendHook(void* pParam, socket_send_hook_proc_t onSend);
	bool AddCloseHook(void* pParam, socket_close_hook_proc_t onClose);
	bool RemoveCloseHook(void* pParam, socket_close_hook_proc_t onClose);
	void enter_critical();
	void leave_critical();
	///获取唯一的SockHooker对象
	static CSocketHooker* GetSockHooker();
	///删除全局唯一的SockHooker对象
	static void DestroySockHooker();

	///对类实例的锁定，在解锁之前不允许退出，用来确定调用系统的Recv等函数返回
	void LockSockHooker();
	void UnLockSockHooker();
	///查询是否锁定
	bool IsLocked();

    //添加和移除回调函数
    bool AddConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);
    bool RemoveConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);
    bool AddRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);
    bool RemoveRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);
    bool AddSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);
    bool RemoveSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);
    bool AddCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);
    bool RemoveCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);

    void GetOrgSocketFunctions(SocketFunctions_t &funcs);

    ///添加要忽视的地址
    void AddIgnoreRange(void * beginAdress, void * endAdress);
    ///清除所有忽视的地址
    void ClearIgnoreRange();
    ///检测当前指定地址是否被忽视
    bool IsIgnoredCaller(void * testAdresss);
private:
    //代表地址范围的
    typedef struct ADDRESS_RANGE
    {
        void * BEGIN_ADRESS;
        void * END_ADRESS;
    };
    vector<ADDRESS_RANGE> m_IgnoreAddressList;///<地址列表
    CRITICAL_SECTION m_CriticalAdress;

	socket_callback_set m_ConnectCallBacks;
	socket_callback_set m_RecvCallBacks;
	socket_callback_set m_SendCallBacks;
	socket_callback_set m_CloseCallBacks;
	overlapped_buffers_map m_Overlappeds;
	CRITICAL_SECTION m_Critical;	
	static CSocketHooker* m_pSockHooker;
	volatile long m_LockNumber;

    socket_callback_set m_ConnectInterceptCallBacks;
    socket_callback_set m_RecvInterceptCallBacks;
    socket_callback_set m_SendInterceptCallBacks;
    socket_callback_set m_CloseInterceptCallBacks;
private:
	bool AddHook(socket_callback_set& cbs, socket_callback_t& cb);
	bool RemoveHook(socket_callback_set& cbs, socket_callback_t& cb);
private:
	///////////////////////////////detours/////////////////////////////////////////
	static int WINAPI connectDetour(
		SOCKET s,
		const struct sockaddr* name,
		int namelen
		);	
	static int WINAPI WSAConnectDetour(
		SOCKET s,
		const struct sockaddr* name,
		int namelen,
		LPWSABUF lpCallerData,
		LPWSABUF lpCalleeData,
		LPQOS lpSQOS,
		LPQOS lpGQOS
		);
	static int WINAPI closesocketDetour(
		SOCKET s
		);

	static int WINAPI recvDetour(
		SOCKET s,
		char* buf,
		int len,
		int flags
		);

	static int WINAPI sendDetour(
		SOCKET s,
		const char* buf,
		int len,
		int flags
		);
	static int WINAPI wsarecvDetour(
		SOCKET s,
		LPWSABUF lpBuffers,
		DWORD dwBufferCount,
		LPDWORD lpNumberOfBytesRecvd,
		LPDWORD lpFlags,
		LPWSAOVERLAPPED lpOverlapped,
		LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
		);
	static int WINAPI wsasendDetour(
		SOCKET s,
		LPWSABUF lpBuffers,
		DWORD dwBufferCount,
		LPDWORD lpNumberOfBytesSent,
		DWORD dwFlags,
		LPWSAOVERLAPPED lpOverlapped,
		LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
		);
	static BOOL WINAPI WSAGetOverlappedResultDetour(
		SOCKET s,
		LPWSAOVERLAPPED lpOverlapped,
		LPDWORD lpcbTransfer,
		BOOL fWait,
		LPDWORD lpdwFlags
		);
private:
	int DoConnect(
		SOCKET s,
		const struct sockaddr* name,
		int namelen
		);	
	
	int DoWSAConnect(
		SOCKET s,
		const struct sockaddr* name,
		int namelen,
		LPWSABUF lpCallerData,
		LPWSABUF lpCalleeData,
		LPQOS lpSQOS,
		LPQOS lpGQOS
		);

	void CallOnConnect(SOCKET s, const struct sockaddr* name);

	int DoClose(
		SOCKET s
		);

	int DoRecv(
		SOCKET s,
		char* buf,
		int len,
		int flags
		);

	int DoSend(
		SOCKET s,
		const char* buf,
		int len,
		int flags
		);	

	int DoWsaRecv( SOCKET s,
		LPWSABUF lpBuffers,
		DWORD dwBufferCount,
		LPDWORD lpNumberOfBytesRecvd,
		LPDWORD lpFlags,
		LPWSAOVERLAPPED lpOverlapped,
		LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

	void CallOnWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, DWORD dwRecvCount, DWORD flags);

	///添加Overlapped
	void AddOverlapped(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPWSABUF lpBuffers, DWORD dwBufferCount);

	///删除单个Overlapped
	void RemoveOverlapped(LPWSAOVERLAPPED lpOverlapped);

	///删除属于s的所有Overlapped
	void RemoveSocketOverlappeds(SOCKET s);

	int DoWsaSend(
		SOCKET s,
		LPWSABUF lpBuffers,
		DWORD dwBufferCount,
		LPDWORD lpNumberOfBytesSent,
		DWORD dwFlags,
		LPWSAOVERLAPPED lpOverlapped,
		LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
		);

	BOOL DoWSAGetOverlappedResult(
		SOCKET s,
		LPWSAOVERLAPPED lpOverlapped,
		LPDWORD lpcbTransfer,
		BOOL fWait,
		LPDWORD lpdwFlags
		);

};
