/*!
@file sockhook.h
@author bob1002
@brief SocketHooker 接口文件
@details SockHook 库提供拦截socket API的函数，使在connect, send, recv, closesocket等API被调用之后调用一个回调函数，回调函数分成四类，分别是：
- 1. connect类: 连接发生时的回调
- 2. close类: 连接主动关闭时的回调
- 3. recv类: 读取数据时的回调
- 4. send类: 发送数据时的回调


四类回调的原型均不同，但是第一个参数都是自定义的回调参数。

四类回调均可以添加删除，即可以添加多个回调函数到同一回调。

@note 目前支持connect, WSAConnect, send, WSASend, recv, WSARecv, closesocket函数的回调
@note 使用方式: 
- 第一步: 先调用SKH_Init\n
- 第二步: 使用SKH_AddXXXX注册感兴趣的回调
- 第三步: 在回调里处理拦截的数据
- 第四步: 如果要结束并把socket库恢复到原来状态，请调用SKH_Deinit
@note 在回调函数被调用时，是在调用者线程，所有有一些限制：
- 不要随意调用C运行库函数（因为这时C运行库没有初始化，很多C运行库函数会出问题）
- 经过试验, 在回调函数里操作文件(包括C运行库和WIN32 API)会干扰socket函数的正常调用 (原因未明), 就因为该原因CSHKTest采取了回调投递，工作线程写入的方式
*/

#ifndef __SOCK_HOOK
#define __SOCK_HOOK

#include <WinSock2.h>
#ifndef IN
#define IN
#endif 

#ifndef OUT
#define OUT
#endif


/*!
connect的回调原型
@param pParan 回调参数
@param s SOCKET
@param port 端口
@param ip IP号
*/
typedef void socket_connect_hook_proc_t(void* pParam, SOCKET s, unsigned int port, const char* ip);

/*!
recv的回调原型
@param pParam 回调参数
@param s SOCKET
@param buf 接收缓冲区
@param len 接收到的字节数
@param oob 是否是OOB数据
@param peek 是否是读取但不移除模式
*/
typedef void socket_recv_hook_proc_t(void* pParam, SOCKET s, void* buf, int len, bool oob, bool peek);

/*!
send的回调原型
@param pParam 回调参数
@param s SOCKET
@param buf 发送缓冲区
@param len 发送出去的字节数
@param oob 是否是OOB数据
@param 
*/
typedef void socket_send_hook_proc_t(void* pParam, SOCKET s, void* buf, int len, bool oob);


/*!
closesocket的回调原型
@param pParam 回调参数
@param s SOCKET
*/
typedef void socket_close_hook_proc_t(void* pParam, SOCKET s);


/*!
connect拦截函数的回调原型
@param pParan 回调参数
@param s 原始SOCKET
@param name 原始地址
@param ip IP号 原始地址长度
@param newS 返回新SOCKET
@param newName 返回新地址
@param newNamelen 返回新地址的长度
*/
typedef void socket_connect_intercept_proc_t(void* pParam, IN SOCKET s, IN const struct sockaddr name, IN int namelen,
                                             OUT SOCKET* newS,  OUT struct sockaddr* newName,OUT int *newNamelen);

/*!
recv拦截函数的回调原型
@param pParam 回调参数
@param s SOCKET
@param buf 接收缓冲区
@param len 接收到的字节数
@param oob 是否是OOB数据
@param peek 是否是读取但不移除模式
@param newS 返回新SOCKET
@param newBuf 返回新的接收缓冲区, 此缓冲区要求调用者维护，在下一次调用此回调或是回调失效前要求缓冲区有效
@param newLen 返回接收到的字节数
@param newOob 返回是否是OOB数据
@param newPeek 返回是否是读取但不移除模式
@note 如果要将newlen设置为-1，一定要慎重的考虑后果
*/
typedef void socket_recv_intercept_proc_t(void* pParam, IN SOCKET s, IN void* buf, IN int len, IN bool oob, IN bool peek
                                           ,OUT SOCKET *newS, OUT void** newBuf, OUT int* newlen, OUT bool* newOob, OUT bool* newPeek);

/*!
send的拦截函数回调原型
@param pParam 回调参数
@param s SOCKET
@param buf 发送缓冲区
@param len 发送出去的字节数
@param oob 是否是OOB数据
@param newS 返回新SOCKET
@param newBuf 返回新的接收缓冲区，此缓冲区要求调用者维护，在下一次调用此回调或是回调失效前要求缓冲区有效
@param newLen 返回接收到的字节数
@param newOob 返回是否是OOB数据
@param 
*/
typedef void socket_send_intercept_proc_t(void* pParam, IN SOCKET s, IN void* buf, IN int len, IN bool oob
                                          ,OUT SOCKET *newS, OUT void** newBuf, OUT int* newlen, OUT bool* newOob);


/*!
closesocket的拦截函数回调原型
@param pParam 回调参数
@param s SOCKET
@param newS 返回新的SOCKET
*/
typedef void socket_close_intercept_proc_t(void* pParam, IN SOCKET s, OUT SOCKET *newS);


///SocketHooker初始化
void SKH_Init();

///SocketHooker反初始化
void SKH_Deinit();

///进入临界区，与回调互斥
void SKH_EnterCritical();

///退出临界区，与回调互斥
void SKH_LeaveCritical();

///添加Connect HOOK
bool SKH_AddConnectHook(void* pParam, socket_connect_hook_proc_t onConnect);
///移除Connect HOOK
bool SKH_RemoveConnectHook(void* pParam, socket_connect_hook_proc_t onConnect);

///添加Recv HOOK
bool SKH_AddRecvHook(void* pParam, socket_recv_hook_proc_t onRecv);
///移除Recv HOOK
bool SKH_RemoveRecvHook(void* pParam, socket_recv_hook_proc_t onRecv);

///添加Send HOOK
bool SKH_AddSendHook(void* pParam, socket_send_hook_proc_t onSend);
///移除Send HOOK
bool SKH_RemoveSendHook(void* pParam, socket_send_hook_proc_t onSend);

///添加Close HOOK
bool SKH_AddCloseHook(void* pParam, socket_close_hook_proc_t onClose);
///删除Close HOOK
bool SKH_RemoveCloseHook(void* pParam, socket_close_hook_proc_t onClose);


///添加Connect INTERCEPT
bool SKH_AddConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);
///移除Connect INTERCEPT
bool SKH_RemoveConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);

///添加Recv INTERCEPT
bool SKH_AddRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);
///移除Recv INTERCEPT
bool SKH_RemoveRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);

///添加Send INTERCEPT
bool SKH_AddSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);
///移除Send INTERCEPT
bool SKH_RemoveSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);

///添加Close INTERCEPT
bool SKH_AddCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);
///删除Close INTERCEPT
bool SKH_RemoveCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);

///connect跳板
typedef int (WINAPI *connectFunc_t)(
                                    SOCKET s,
                                    const struct sockaddr* name,
                                    int namelen
                                    ) ;
///closesocket跳板
typedef int (WINAPI *closesocketFunc_t)(
                                        SOCKET s
                                        );

///recv跳板
typedef int (WINAPI *recvFunc_t)(
                                 SOCKET s,
                                 char* buf,
                                 int len,
                                 int flags
                                 ) ;
///send跳板
typedef int (WINAPI *sendFunc_t)(
                                 SOCKET s,
                                 const char* buf,
                                 int len,
                                 int flags
                                 );

//WSAConnect跳板
typedef int (WINAPI *WSAConnectFunc_t)(
                                       SOCKET s,
                                       const struct sockaddr* name,
                                       int namelen,
                                       LPWSABUF lpCallerData,
                                       LPWSABUF lpCalleeData,
                                       LPQOS lpSQOS,
                                       LPQOS lpGQOS
                                       );
///WSARecv跳板
typedef int  (WINAPI *WSARecv_Func_t)(
                                      SOCKET s,
                                      LPWSABUF lpBuffers,
                                      DWORD dwBufferCount,
                                      LPDWORD lpNumberOfBytesRecvd,
                                      LPDWORD lpFlags,
                                      LPWSAOVERLAPPED lpOverlapped,
                                      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
                                      );
///WSASend跳板
typedef int (WINAPI *WSASend_Func_t)(
                                     SOCKET s,
                                     LPWSABUF lpBuffers,
                                     DWORD dwBufferCount,
                                     LPDWORD lpNumberOfBytesSent,
                                     DWORD dwFlags,
                                     LPWSAOVERLAPPED lpOverlapped,
                                     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
                                     );

///WSAGetOverlappedResult跳板
typedef BOOL (WINAPI *WSAGetOverlappedResultFunc_t)(
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD lpcbTransfer,
    BOOL fWait,
    LPDWORD lpdwFlags
    );

///包含SocketFunction的一个结构体
typedef struct SocketFunctions{
    connectFunc_t connect;
    closesocketFunc_t close;
    recvFunc_t recv;
    sendFunc_t send;
}SocketFunctions_t,*PSocketFunctions;

///在SKH_INIT之后还能调用此方法去获得原始socket系列函数的地址
void SKH_GetSocketFunctions(SocketFunctions *func);


void SKH_AddIgnoreRange(void * beginAdress, void * endAdress);


#endif