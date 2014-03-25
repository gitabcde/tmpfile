#pragma once

#include "sockhook.h"
#include <set>
#include <map>
#include <vector>
#include <Windows.h>

using namespace std;

///�ص��ڵ�
typedef struct {
	void* pParam; ///< �ص�����
	void* pCallBack; ///< �ص�������ַ
} socket_callback_t;

///�ص�����
typedef set<socket_callback_t> socket_callback_set;


///WSARecv�Ļ�����
typedef struct  
{
	SOCKET   socket;
	LPWSABUF lpBuffers;
	DWORD dwBufferCount;
} OverlappedBuffers;

///Overlapped �� WSARecv��������ӳ��
typedef map<LPWSAOVERLAPPED, OverlappedBuffers> overlapped_buffers_map;

/*!
ע��ͷ�ע��ص�
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
	///��ȡΨһ��SockHooker����
	static CSocketHooker* GetSockHooker();
	///ɾ��ȫ��Ψһ��SockHooker����
	static void DestroySockHooker();

	///����ʵ�����������ڽ���֮ǰ�������˳�������ȷ������ϵͳ��Recv�Ⱥ�������
	void LockSockHooker();
	void UnLockSockHooker();
	///��ѯ�Ƿ�����
	bool IsLocked();

    //��Ӻ��Ƴ��ص�����
    bool AddConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);
    bool RemoveConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);
    bool AddRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);
    bool RemoveRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);
    bool AddSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);
    bool RemoveSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);
    bool AddCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);
    bool RemoveCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);

    void GetOrgSocketFunctions(SocketFunctions_t &funcs);

    ///���Ҫ���ӵĵ�ַ
    void AddIgnoreRange(void * beginAdress, void * endAdress);
    ///������к��ӵĵ�ַ
    void ClearIgnoreRange();
    ///��⵱ǰָ����ַ�Ƿ񱻺���
    bool IsIgnoredCaller(void * testAdresss);
private:
    //�����ַ��Χ��
    typedef struct ADDRESS_RANGE
    {
        void * BEGIN_ADRESS;
        void * END_ADRESS;
    };
    vector<ADDRESS_RANGE> m_IgnoreAddressList;///<��ַ�б�
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

	///���Overlapped
	void AddOverlapped(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPWSABUF lpBuffers, DWORD dwBufferCount);

	///ɾ������Overlapped
	void RemoveOverlapped(LPWSAOVERLAPPED lpOverlapped);

	///ɾ������s������Overlapped
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
