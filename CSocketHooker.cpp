/*!
@file CSocketHooker.cpp
@author bob1002
@brief SocketHooker 实现文件
*/
#include "CSocketHooker.h"
#include "detours.h"
#include <Windows.h>
#include <assert.h>

//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500
#pragma comment(lib, "ws2_32.lib")

//Start of Addition
#if _WIN32_WINNT < 0x0502
  #define ADDRINFOT addrinfo
  #define GetAddrInfo getaddrinfo
  #define FreeAddrInfo freeaddrinfo
#endif

///获取对一个STDCALL调用约定的函数的调用者的地址
#define CALLER_ADDRESS_STDCALL(x) \
    void * x = NULL;\
    _asm mov eax,[ebp+4] _asm mov x,eax

void* __stdcall getCall()
{
    void *res;
    __asm
    {
        mov eax,[ebp]
        mov eax,[eax+4];
        mov res,eax
    }
    return res;
}



#pragma warning(disable:4996)

bool operator < (const socket_callback_t& a, const socket_callback_t& b)
{
	if (a.pParam < b.pParam) return true;
	else if (a.pParam == b.pParam) {
		return a.pCallBack < b.pCallBack;
	} else {
		return false;
	}
}

///connect跳板
static int (WINAPI *connectTrampoline)(
				  SOCKET s,
				  const struct sockaddr* name,
				  int namelen
				  ) = connect;

//WSAConnect跳板
int (WINAPI *WSAConnectTrampoline)(
			   SOCKET s,
			   const struct sockaddr* name,
			   int namelen,
			   LPWSABUF lpCallerData,
			   LPWSABUF lpCalleeData,
			   LPQOS lpSQOS,
			   LPQOS lpGQOS
			   ) = WSAConnect;



///closesocket跳板
int (WINAPI *closesocketTrampoline)(
				  SOCKET s
				  ) = closesocket;

///recv跳板
int (WINAPI *recvTrampoline)(
		 SOCKET s,
		 char* buf,
		 int len,
		 int flags
		 ) = recv;

///WSARecv跳板
int  (WINAPI *WSARecv_trampoline)(
				  SOCKET s,
				  LPWSABUF lpBuffers,
				  DWORD dwBufferCount,
				  LPDWORD lpNumberOfBytesRecvd,
				  LPDWORD lpFlags,
				  LPWSAOVERLAPPED lpOverlapped,
				  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				  ) = WSARecv;

///send跳板
int (WINAPI *sendTrampoline)(
		 SOCKET s,
		 const char* buf,
		 int len,
		 int flags
		 ) = send;

///WSASend跳板
int (WINAPI *WSASend_trampoline)(
				  SOCKET s,
				  LPWSABUF lpBuffers,
				  DWORD dwBufferCount,
				  LPDWORD lpNumberOfBytesSent,
				  DWORD dwFlags,
				  LPWSAOVERLAPPED lpOverlapped,
				  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				  ) =
				  WSASend;

///WSAGetOverlappedResult跳板
BOOL (WINAPI *WSAGetOverlappedResultTrampoline)(
	SOCKET s,
	LPWSAOVERLAPPED lpOverlapped,
	LPDWORD lpcbTransfer,
	BOOL fWait,
	LPDWORD lpdwFlags
	) = WSAGetOverlappedResult;


///全局唯一的SockHooker
CSocketHooker* CSocketHooker::m_pSockHooker = NULL;

CSocketHooker::CSocketHooker(void)
{	
	InitializeCriticalSection(&m_Critical);
    InitializeCriticalSection(&m_CriticalAdress);    
	m_LockNumber = 0;	
	enter_critical();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)WSAConnectTrampoline, WSAConnectDetour);
	DetourAttach(&(PVOID&)connectTrampoline, connectDetour);
	DetourAttach(&(PVOID&)closesocketTrampoline, closesocketDetour);
	DetourAttach(&(PVOID&)sendTrampoline, sendDetour);
	DetourAttach(&(PVOID&)recvTrampoline, recvDetour);
	DetourAttach(&(PVOID&)WSARecv_trampoline, wsarecvDetour);
	DetourAttach(&(PVOID&)WSASend_trampoline, wsasendDetour);
	DetourAttach(&(PVOID&)WSAGetOverlappedResultTrampoline, WSAGetOverlappedResultDetour);
	DetourTransactionCommit();
	leave_critical();
}

//#define do_assert(b) if (!b) {char msg[256]; sprintf("Error:%s, Line: %d, File: %d", #b, __LINE__, __FILE__);MessageBox(NULL, msg, "assert", MB_OK | MB_ICONERROR);}
#define do_assert(b) assert(b)


CSocketHooker::~CSocketHooker(void)
{	
	enter_critical();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)WSAConnectTrampoline, WSAConnectDetour);
	DetourDetach(&(PVOID&)connectTrampoline, connectDetour);
	DetourDetach(&(PVOID&)closesocketTrampoline, closesocketDetour);
	DetourDetach(&(PVOID&)sendTrampoline, sendDetour);
	DetourDetach(&(PVOID&)recvTrampoline, recvDetour);
	DetourDetach(&(PVOID&)WSARecv_trampoline, wsarecvDetour);
	DetourDetach(&(PVOID&)WSASend_trampoline, wsasendDetour);
	DetourDetach(&(PVOID&)WSAGetOverlappedResultTrampoline, WSAGetOverlappedResultDetour);
	DetourTransactionCommit();
	leave_critical();
	
	while(IsLocked())
	{
		Sleep(200);
	}
    DeleteCriticalSection(&m_Critical);
    DeleteCriticalSection(&m_CriticalAdress);
}



bool CSocketHooker::AddConnectHook(void* pParam, socket_connect_hook_proc_t onConnect)
{
	socket_callback_t scb={pParam, onConnect};
	bool bOK = AddHook(m_ConnectCallBacks, scb);
	return bOK;
}

bool CSocketHooker::RemoveConnectHook(void* pParam, socket_connect_hook_proc_t onConnect)
{
	socket_callback_t scb={pParam, onConnect};
	bool bOK = RemoveHook(m_ConnectCallBacks, scb);
	return bOK;
}

bool CSocketHooker::AddRecvHook(void* pParam, socket_recv_hook_proc_t onRecv)
{
	socket_callback_t scb={pParam, onRecv};
	bool bOK = AddHook(m_RecvCallBacks, scb);
	return bOK;
}

bool CSocketHooker::RemoveRecvHook(void* pParam, socket_recv_hook_proc_t onRecv)
{
	socket_callback_t scb={pParam, onRecv};
	bool bOK = RemoveHook(m_RecvCallBacks, scb);
	return bOK;
}

bool CSocketHooker::AddSendHook(void* pParam, socket_send_hook_proc_t onSend)
{
	socket_callback_t scb={pParam, onSend};
	bool bOK = AddHook(m_SendCallBacks, scb);
	return bOK;
}

bool CSocketHooker::RemoveSendHook(void* pParam, socket_send_hook_proc_t onSend)
{
	socket_callback_t scb={pParam, onSend};
	bool bOK = RemoveHook(m_SendCallBacks, scb);
	return bOK;
}

bool CSocketHooker::AddCloseHook(void* pParam, socket_close_hook_proc_t onClose)
{
	socket_callback_t scb={pParam, onClose};
	bool bOK = AddHook(m_CloseCallBacks, scb);
	return bOK;
}

bool CSocketHooker::RemoveCloseHook(void* pParam, socket_close_hook_proc_t onClose)
{
	socket_callback_t scb={pParam, onClose};
	bool bOK = RemoveHook(m_CloseCallBacks, scb);
	return bOK;
}


bool CSocketHooker::AddHook(socket_callback_set& cbs, socket_callback_t& cb)
{
	bool bOK=false;
	enter_critical();
	if (cbs.find(cb) == cbs.end()) {
		cbs.insert(cb);
		bOK = true;
	}
	leave_critical();
	return bOK;
}

bool CSocketHooker::RemoveHook(socket_callback_set& cbs, socket_callback_t& cb)
{
	bool bOK=false;
	socket_callback_set::iterator iter;
	enter_critical();
	iter = cbs.find(cb);
	if (iter != cbs.end()) {
		cbs.erase(cb);
		bOK = true;
	}
	leave_critical();
	return bOK;
}

void CSocketHooker::enter_critical()
{
	EnterCriticalSection(&m_Critical);
}

void CSocketHooker::leave_critical()
{
	LeaveCriticalSection(&m_Critical);
}

void CSocketHooker::LockSockHooker()
{	
	InterlockedIncrement(&m_LockNumber);	
}
void CSocketHooker::UnLockSockHooker()
{
	InterlockedDecrement(&m_LockNumber);
}

void debug_print(char *szMsg, ...)
{
	va_list arglist;
	char buffer[256];

	va_start(arglist, szMsg);
	vsprintf(buffer, szMsg, arglist);
	va_end(arglist);

	OutputDebugString(buffer);
	OutputDebugString("\n");
}
bool CSocketHooker::IsLocked()
{
	//不等于就是正在被使用
	int retVar = InterlockedExchangeAdd(&m_LockNumber, 0) ;
	debug_print("\nm_LockNumber:\t%d", retVar);
	return retVar != 0;
}

bool CSocketHooker::AddConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect)
{
    socket_callback_t scb={pParam, onConnect};
    bool bOK = AddHook(m_ConnectInterceptCallBacks, scb);
    return bOK;
}

bool CSocketHooker::RemoveConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect)
{
    socket_callback_t scb={pParam, onConnect};
    bool bOK = RemoveHook(m_ConnectInterceptCallBacks, scb);
    return bOK;
}

bool CSocketHooker::AddRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv)
{
    socket_callback_t scb={pParam, onRecv};
    bool bOK = AddHook(m_RecvInterceptCallBacks, scb);
    return bOK;
}

bool CSocketHooker::RemoveRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv)
{
    socket_callback_t scb={pParam, onRecv};
    bool bOK = RemoveHook(m_RecvInterceptCallBacks, scb);
    return bOK;
}

bool CSocketHooker::AddSendIntercept(void* pParam, socket_send_intercept_proc_t onSend)
{
    socket_callback_t scb={pParam, onSend};
    bool bOK = AddHook(m_SendInterceptCallBacks, scb);
    return bOK;
}

bool CSocketHooker::RemoveSendIntercept(void* pParam, socket_send_intercept_proc_t onSend)
{
    socket_callback_t scb={pParam, onSend};
    bool bOK = RemoveHook(m_SendInterceptCallBacks, scb);
    return bOK;
}

bool CSocketHooker::AddCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose)
{
    socket_callback_t scb={pParam, onClose};
    bool bOK = AddHook(m_CloseInterceptCallBacks, scb);
    return bOK;
}

bool CSocketHooker::RemoveCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose)
{
    socket_callback_t scb={pParam, onClose};
    bool bOK = RemoveHook(m_CloseInterceptCallBacks, scb);
    return bOK;
}


CSocketHooker* CSocketHooker::GetSockHooker()
{
	if (CSocketHooker::m_pSockHooker == NULL) {
		CSocketHooker::m_pSockHooker = new CSocketHooker();
	}
	return CSocketHooker::m_pSockHooker;
}

void CSocketHooker::DestroySockHooker()
{
	if (CSocketHooker::m_pSockHooker != NULL) {
		delete CSocketHooker::m_pSockHooker;
		CSocketHooker::m_pSockHooker = NULL;
	}
}
void CSocketHooker::GetOrgSocketFunctions(SocketFunctions_t &funcs)
{
    funcs.close = closesocketTrampoline;
    funcs.connect = connectTrampoline;
    funcs.recv = recvTrampoline;
    funcs.send = sendTrampoline;
}


void CSocketHooker::AddIgnoreRange(void * beginAdress, void * endAdress)
{
    assert( (intptr_t)(beginAdress) <= (intptr_t)(endAdress));///开始地址小于等于结束地址
    if ((intptr_t)(beginAdress) <= (intptr_t)(endAdress))
    {
        ADDRESS_RANGE range = {beginAdress,endAdress};
        EnterCriticalSection(&m_CriticalAdress);
        m_IgnoreAddressList.push_back(range);
        LeaveCriticalSection(&m_CriticalAdress);
    }
}
void CSocketHooker::ClearIgnoreRange()
{
    EnterCriticalSection(&m_CriticalAdress);
    m_IgnoreAddressList.clear();
    LeaveCriticalSection(&m_CriticalAdress);
}
bool CSocketHooker::IsIgnoredCaller(void * testAdresss)
{
    EnterCriticalSection(&m_CriticalAdress);
    for (vector<ADDRESS_RANGE>::iterator it = m_IgnoreAddressList.begin(); it != m_IgnoreAddressList.end(); it++)
    {
        intptr_t testPtr = (intptr_t)(testAdresss);
        if( (intptr_t)(*it).BEGIN_ADRESS <=  testPtr  && testPtr <= (intptr_t)(*it).END_ADRESS)
        {
            LeaveCriticalSection(&m_CriticalAdress);
            return true;
        }
    }

    LeaveCriticalSection(&m_CriticalAdress);
    return false;
}

//////////////////////////////////////////////////////////////////////////
// Detours
//////////////////////////////////////////////////////////////////////////
int WINAPI CSocketHooker::connectDetour(
						 SOCKET s,
						 const struct sockaddr* name,
						 int namelen
						 )
{
    CALLER_ADDRESS_STDCALL(callerAdress);

    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return connectTrampoline(s, name, namelen);
    }
    else
    {
        return GetSockHooker()->DoConnect(s,name, namelen);
    }
}

int WINAPI CSocketHooker::WSAConnectDetour(
							SOCKET s,
							const struct sockaddr* name,
							int namelen,
							LPWSABUF lpCallerData,
							LPWSABUF lpCalleeData,
							LPQOS lpSQOS,
							LPQOS lpGQOS
							)
{
    CALLER_ADDRESS_STDCALL(callerAdress);
    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return WSAConnectTrampoline(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
    }
    else
    {
        return GetSockHooker()->DoWSAConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
    }
}

int WINAPI CSocketHooker::closesocketDetour(
					   SOCKET s
					   )
{
    CALLER_ADDRESS_STDCALL(callerAdress);
    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return closesocketTrampoline(s);
    }
    else
    {
	return GetSockHooker()->DoClose(s);
    }
}


int WINAPI CSocketHooker::wsarecvDetour(SOCKET s,
										LPWSABUF lpBuffers,
										DWORD dwBufferCount,
										LPDWORD lpNumberOfBytesRecvd,
										LPDWORD lpFlags,
										LPWSAOVERLAPPED lpOverlapped,
										LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    CALLER_ADDRESS_STDCALL(callerAdress);
    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return wsarecvDetour(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
    }
    else
    {
		return GetSockHooker()->DoWsaRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
    }
}

int WINAPI CSocketHooker::wsasendDetour(
								  SOCKET s,
								  LPWSABUF lpBuffers,
								  DWORD dwBufferCount,
								  LPDWORD lpNumberOfBytesSent,
								  DWORD dwFlags,
								  LPWSAOVERLAPPED lpOverlapped,
								  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
								  )
{
    CALLER_ADDRESS_STDCALL(callerAdress);
    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return WSASend_trampoline(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
    }
    else
    {
        return GetSockHooker()->DoWsaSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
    }
}

int WINAPI CSocketHooker::recvDetour(
                                     SOCKET s,
                                     char* buf,
                                     int len,
                                     int flags
                                     )
{
    CALLER_ADDRESS_STDCALL(callerAdress);
    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return recvTrampoline(s, buf, len, flags);
    }
    else
    {
        return GetSockHooker()->DoRecv(s, buf, len, flags);
    }
}


int WINAPI CSocketHooker::sendDetour(
					  SOCKET s,
					  const char* buf,
					  int len,
					  int flags
                      )
{
    CALLER_ADDRESS_STDCALL(callerAdress);
    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return sendTrampoline(s, buf, len, flags);
    }
    else
    {
        return GetSockHooker()->DoSend(s, buf, len, flags);
    }
}

BOOL WINAPI CSocketHooker::WSAGetOverlappedResultDetour(
	SOCKET s,
	LPWSAOVERLAPPED lpOverlapped,
	LPDWORD lpcbTransfer,
	BOOL fWait,
	LPDWORD lpdwFlags
    )
{
    CALLER_ADDRESS_STDCALL(callerAdress);
    if (GetSockHooker()->IsIgnoredCaller(callerAdress) )
    {
        return WSAGetOverlappedResultTrampoline(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
    }
    else
    {
        return GetSockHooker()->DoWSAGetOverlappedResult(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
// 派发回调
//////////////////////////////////////////////////////////////////////////
int CSocketHooker::DoConnect(
			  SOCKET s,
			  const struct sockaddr* name,
			  int namelen
			  )
{
	LockSockHooker();

    enter_critical();
    socket_callback_set::iterator iter;
    sockaddr newName = *name;
    int newLen = namelen;
    SOCKET newS = s;
    for(iter = m_ConnectInterceptCallBacks.begin(); iter != m_ConnectInterceptCallBacks.end(); ++iter) {
        ((socket_connect_intercept_proc_t*)((*iter).pCallBack))((*iter).pParam, newS, newName, newLen, &newS,&newName, &newLen);
    }
    leave_critical();

    int nCode = connectTrampoline(newS, (const struct sockaddr *)&newName, newLen);
#ifdef _DEBUG
    if (nCode < 0) {
        char msg[64];
        sprintf(msg, "connect error code:%d", WSAGetLastError());
        OutputDebugString(msg);       
    }	
#endif

	CallOnConnect(s, name);		
	UnLockSockHooker();
	return nCode;
}

void CSocketHooker::CallOnConnect(SOCKET s, const struct sockaddr* name)
{
	const struct sockaddr_in* ps = (const struct sockaddr_in*)name;
	unsigned int port = ntohs(ps->sin_port);
	char ip[64];
	sprintf(ip, "%d.%d.%d.%d", ps->sin_addr.s_addr&0xFF, (ps->sin_addr.s_addr>>8)&0xFF, (ps->sin_addr.s_addr>>16)&0xFF, (ps->sin_addr.s_addr>>24));

	enter_critical();
	socket_callback_set::iterator iter;
	for(iter = m_ConnectCallBacks.begin(); iter != m_ConnectCallBacks.end(); ++iter) {
		((socket_connect_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, s, port, ip);
	}
	leave_critical();
}

int CSocketHooker::DoWSAConnect(
				 SOCKET s,
				 const struct sockaddr* name,
				 int namelen,
				 LPWSABUF lpCallerData,
				 LPWSABUF lpCalleeData,
				 LPQOS lpSQOS,
				 LPQOS lpGQOS
				 )
{
	LockSockHooker();
	int nCode = WSAConnectTrampoline(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
	CallOnConnect(s, name);
	UnLockSockHooker();
	return nCode;
}

int CSocketHooker::DoWsaRecv( SOCKET s,
							 LPWSABUF lpBuffers,
							 DWORD dwBufferCount,
							 LPDWORD lpNumberOfBytesRecvd,
							 LPDWORD lpFlags,
							 LPWSAOVERLAPPED lpOverlapped,
							 LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	LockSockHooker();
	int nCode = WSARecv_trampoline(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
	if (nCode == 0) {		
		CallOnWSARecv(s, lpBuffers, dwBufferCount, *lpNumberOfBytesRecvd, *lpFlags);		
	} else if (lpOverlapped != NULL && lpCompletionRoutine == NULL) { //FIXME We don't support lpCompletionRoutine now
		if (WSAGetLastError() ==  WSA_IO_PENDING) {
			AddOverlapped(s, lpOverlapped, lpBuffers, dwBufferCount);
		}
	}

	UnLockSockHooker();
	return nCode;
}

void CSocketHooker::CallOnWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, DWORD dwRecvCount, DWORD flags)
{	
	if (dwRecvCount == 0) return;
	bool bFinished = false;
	for(int i=0; (i<dwBufferCount) && !bFinished; ++i)
	{
		enter_critical();
		if (m_RecvCallBacks.empty()) 
		{
			leave_critical();
			return;
		}
		if(lpBuffers[i].buf==0x00000000)
		{
			leave_critical();
			return;
		}
		socket_callback_set::iterator iter;
		for(iter = m_RecvCallBacks.begin(); iter != m_RecvCallBacks.end(); ++iter) 
		{
			if(dwRecvCount > lpBuffers[i].len)
			{
				((socket_recv_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, s, lpBuffers[i].buf, lpBuffers[i].len, (flags&MSG_OOB) != 0, (flags&MSG_PEEK) != 0);				
				bFinished = false;
			}
			else
			{
				((socket_recv_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, s, lpBuffers[i].buf, dwRecvCount, (flags&MSG_OOB) != 0, (flags&MSG_PEEK) != 0);				
				bFinished = true;
			}
		} 
		leave_critical();
		if (!bFinished) {
			assert(dwRecvCount > lpBuffers[i].len);
			dwRecvCount -= lpBuffers[i].len;
		}
	}		
}

void CSocketHooker::AddOverlapped(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPWSABUF lpBuffers, DWORD dwBufferCount)
{
	assert(lpOverlapped != NULL);
	enter_critical();
	overlapped_buffers_map::iterator it = m_Overlappeds.find(lpOverlapped);
	if (it != m_Overlappeds.end()) {
		OverlappedBuffers& bufs = (*it).second;
		assert(bufs.socket == s);
		bufs.socket = s;
		bufs.lpBuffers = lpBuffers;
		bufs.dwBufferCount = dwBufferCount;
	} else {
		OverlappedBuffers bufs;
		bufs.socket = s;
		bufs.dwBufferCount = dwBufferCount;
		bufs.lpBuffers = lpBuffers;
		m_Overlappeds.insert(std::make_pair(lpOverlapped, bufs));
	}
	leave_critical();
}


void CSocketHooker::RemoveOverlapped(LPWSAOVERLAPPED lpOverlapped)
{
	enter_critical();
	overlapped_buffers_map::iterator it = m_Overlappeds.find(lpOverlapped);
	if (it != m_Overlappeds.end()) {
		m_Overlappeds.erase(it);
	}
	leave_critical();
}

void CSocketHooker::RemoveSocketOverlappeds(SOCKET s)
{
	enter_critical();
	overlapped_buffers_map::iterator it = m_Overlappeds.begin();
	while (it != m_Overlappeds.end()) {
		if ((*it).second.socket == s) {
			overlapped_buffers_map::iterator tmp = it++;
			m_Overlappeds.erase(tmp);
		} else ++it;
	}
	leave_critical();
}

int CSocketHooker::DoWsaSend(SOCKET s,
							 LPWSABUF lpBuffers,
							 DWORD dwBufferCount,
							 LPDWORD lpNumberOfBytesSent,
							 DWORD dwFlags,
							 LPWSAOVERLAPPED lpOverlapped,
							 LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	LockSockHooker();
	int nCode = WSASend_trampoline(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
	if (nCode >= 0) {
		unsigned long dwCount = 0;
		unsigned long dwSendBytes = *lpNumberOfBytesSent;

		
		while(dwBufferCount > dwCount)
		{
			enter_critical();
			socket_callback_set::iterator iter;
			for(iter = m_SendCallBacks.begin(); iter != m_SendCallBacks.end(); ++iter)
			{
				if(dwSendBytes > lpBuffers[dwCount].len)
				{
					((socket_send_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, s, lpBuffers[dwCount].buf, lpBuffers[dwCount].len, (dwFlags&MSG_OOB) != 0);
					dwSendBytes -= lpBuffers[dwCount].len;
				}
				else
				{
					((socket_send_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, s, lpBuffers[dwCount].buf, dwSendBytes, (dwFlags&MSG_OOB) != 0);
					break;
				}
			}
			leave_critical();
			dwCount++;
		}

	}

	UnLockSockHooker();
	return nCode;
}

BOOL CSocketHooker::DoWSAGetOverlappedResult(
							  SOCKET s,
							  LPWSAOVERLAPPED lpOverlapped,
							  LPDWORD lpcbTransfer,
							  BOOL fWait,
							  LPDWORD lpdwFlags
							  )
{
	LockSockHooker();
	BOOL bRet = WSAGetOverlappedResultTrampoline(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);	
	if (bRet) {
		enter_critical();
		overlapped_buffers_map::iterator it = m_Overlappeds.find(lpOverlapped);
		if (it != m_Overlappeds.end()) {
			OverlappedBuffers& bufs = (*it).second;
			CallOnWSARecv(s, bufs.lpBuffers, bufs.dwBufferCount, *lpcbTransfer, *lpdwFlags);
			m_Overlappeds.erase(it);
		}
		leave_critical();
	} 

	UnLockSockHooker();
	return bRet;
}

int  CSocketHooker::DoClose(
			SOCKET s
			)
{
	LockSockHooker();

    enter_critical();
    SOCKET newSocket = s;
    socket_callback_set::iterator iter;
    for(iter = m_CloseInterceptCallBacks.begin(); iter != m_CloseInterceptCallBacks.end(); ++iter) {
        ((socket_close_intercept_proc_t*)((*iter).pCallBack))((*iter).pParam, newSocket, &newSocket);
    }
    leave_critical();

	int nCode = closesocketTrampoline(newSocket);
	enter_critical();
	for(iter = m_CloseCallBacks.begin(); iter != m_CloseCallBacks.end(); ++iter) {
		((socket_close_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, newSocket);
	}
	leave_critical();
	RemoveSocketOverlappeds(s);

	UnLockSockHooker();
	return nCode;
}

int CSocketHooker::DoRecv(
		   SOCKET s,
		   char* buf,
		   int len,
		   int flags
		   )
{
	LockSockHooker();
	int nCode = recvTrampoline(s, buf, len, flags);	
    SOCKET newS = s;
    void* newBuf = buf;
    int newFlags = flags;
    bool oob = (flags&MSG_OOB) != 0; 
    bool peek = (flags&MSG_PEEK) != 0;
    bool needchange = false;
    
	if (nCode >= 0) {
        enter_critical();
         socket_callback_set::iterator iter;
        for(iter = m_RecvInterceptCallBacks.begin(); iter != m_RecvInterceptCallBacks.end(); ++iter) {
            ((socket_recv_intercept_proc_t*)((*iter).pCallBack))((*iter).pParam, newS, newBuf, nCode, oob, peek
                ,&newS, &newBuf, &nCode, &oob, &peek);
            needchange = true;
        }        
        leave_critical();      
	}
    
    if (needchange)//FIXME 
    {
        assert(nCode <= len);
        int nCopy =  nCode < len? nCode:len;
        if (nCopy != -1 )
        {
            memcpy(buf, newBuf, nCopy);
        }
        nCode = nCopy;
    }

    if (nCode >= 0) {
        enter_critical();
        socket_callback_set::iterator iter;
        for(iter = m_RecvCallBacks.begin(); iter != m_RecvCallBacks.end(); ++iter) {
            ((socket_recv_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, newS, newBuf, nCode, (flags&MSG_OOB) != 0, (flags&MSG_PEEK) != 0);
        }
        leave_critical();       
    }
	UnLockSockHooker();
	return nCode;
}

int CSocketHooker::DoSend(
		   SOCKET s,
		   const char* buf,
		   int len,
		   int flags
		   )
{
	LockSockHooker();

    //拦截数据给回调，再将回调改变后的数据发送出去
    enter_critical();
    SOCKET newS = s;
    void* newBuf = (void*)buf;       
    int newlen = len;
    int newFlags = flags;
    bool oob = (flags&MSG_OOB) != 0; 
    if (len >=0)
    {        
        socket_callback_set::iterator iter;
        for(iter = m_SendInterceptCallBacks.begin(); iter != m_SendInterceptCallBacks.end(); ++iter) {
            ((socket_send_intercept_proc_t*)((*iter).pCallBack))((*iter).pParam, newS, newBuf, newlen, (newFlags&MSG_OOB) != 0
                ,&newS, &newBuf, &newlen, &oob);
        }        
    }    
    if (oob)
    {
        newFlags |= MSG_OOB;
    }
    else
    {
        newFlags = newFlags | (!MSG_OOB);//将OOB标志位清0
    }
    leave_critical();

	int nCode = sendTrampoline(newS, (const char *)newBuf, newlen, newFlags);
	if (nCode >= 0) {
		enter_critical();
		socket_callback_set::iterator iter;
		for(iter = m_SendCallBacks.begin(); iter != m_SendCallBacks.end(); ++iter) {
			((socket_send_hook_proc_t*)((*iter).pCallBack))((*iter).pParam, s, (void*)buf, nCode, (flags&MSG_OOB) != 0);
		}
		leave_critical();        
	}
	UnLockSockHooker();
	return nCode;
}

//////////////////////////////////////////////////////////////////////////
//sockhook API
//////////////////////////////////////////////////////////////////////////

///SocketHooker初始化
void SKH_Init()
{
	CSocketHooker::GetSockHooker();
}

///SocketHooker反初始化
void SKH_Deinit()
{
	CSocketHooker::DestroySockHooker();
}

///进入临界区，与回调互斥
void SKH_EnterCritical()
{
	CSocketHooker::GetSockHooker()->enter_critical();
}

///退出临界区，与回调互斥
void SKH_LeaveCritical()
{

	CSocketHooker::GetSockHooker()->leave_critical();
}

///添加Connect HOOK
bool SKH_AddConnectHook(void* pParam, socket_connect_hook_proc_t onConnect)
{
	return CSocketHooker::GetSockHooker()->AddConnectHook(pParam, onConnect);
}
///移除Connect HOOK
bool SKH_RemoveConnectHook(void* pParam, socket_connect_hook_proc_t onConnect)
{
	return CSocketHooker::GetSockHooker()->RemoveConnectHook(pParam, onConnect);
}

///添加Recv HOOK
bool SKH_AddRecvHook(void* pParam, socket_recv_hook_proc_t onRecv)
{
	return CSocketHooker::GetSockHooker()->AddRecvHook(pParam, onRecv);
}
///移除Recv HOOK
bool SKH_RemoveRecvHook(void* pParam, socket_recv_hook_proc_t onRecv)
{
	return CSocketHooker::GetSockHooker()->RemoveRecvHook(pParam, onRecv);
}

///添加Send HOOK
bool SKH_AddSendHook(void* pParam, socket_send_hook_proc_t onSend)
{
	return CSocketHooker::GetSockHooker()->AddSendHook(pParam, onSend);
}
///移除Send HOOK
bool SKH_RemoveSendHook(void* pParam, socket_send_hook_proc_t onSend)
{
	return CSocketHooker::GetSockHooker()->RemoveSendHook(pParam, onSend);
}

///添加Close HOOK
bool SKH_AddCloseHook(void* pParam, socket_close_hook_proc_t onClose)
{
	return CSocketHooker::GetSockHooker()->AddCloseHook(pParam, onClose);
}
///删除Close HOOK
bool SKH_RemoveCloseHook(void* pParam, socket_close_hook_proc_t onClose)
{
	return CSocketHooker::GetSockHooker()->RemoveCloseHook(pParam, onClose);
}

///添加Connect INTERCEPT
bool SKH_AddConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect)
{
    return CSocketHooker::GetSockHooker()->AddConnectIntercept(pParam, onConnect);
}
///移除Connect INTERCEPT
bool SKH_RemoveConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect)
{
    return CSocketHooker::GetSockHooker()->RemoveConnectIntercept(pParam, onConnect);
}

///添加Recv INTERCEPT
bool SKH_AddRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv)
{
    return CSocketHooker::GetSockHooker()->AddRecvIntercept(pParam, onRecv);
}
///移除Recv INTERCEPT
bool SKH_RemoveRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv)
{
    return CSocketHooker::GetSockHooker()->RemoveRecvIntercept(pParam, onRecv);
}

///添加Send INTERCEPT
bool SKH_AddSendIntercept(void* pParam, socket_send_intercept_proc_t onSend)
{
    return CSocketHooker::GetSockHooker()->AddSendIntercept(pParam, onSend);
}
///移除Send INTERCEPT
bool SKH_RemoveSendIntercept(void* pParam, socket_send_intercept_proc_t onSend)
{
    return CSocketHooker::GetSockHooker()->RemoveSendIntercept(pParam, onSend);
}

///添加Close INTERCEPT
bool SKH_AddCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose)
{
    return CSocketHooker::GetSockHooker()->AddCloseIntercept(pParam, onClose);
}
///删除Close INTERCEPT
bool SKH_RemoveCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose)
{
    return CSocketHooker::GetSockHooker()->RemoveCloseIntercept(pParam, onClose);
}
void SKH_GetSocketFunctions(SocketFunctions *func)
{
    assert(func != NULL);
    if (func != NULL)
    {
        CSocketHooker::GetSockHooker()->GetOrgSocketFunctions(*func);
    }
}

void SKH_AddIgnoreRange(void * beginAdress, void * endAdress)
{
    CSocketHooker::GetSockHooker()->AddIgnoreRange(beginAdress, endAdress);
}