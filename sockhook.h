/*!
@file sockhook.h
@author bob1002
@brief SocketHooker �ӿ��ļ�
@details SockHook ���ṩ����socket API�ĺ�����ʹ��connect, send, recv, closesocket��API������֮�����һ���ص��������ص������ֳ����࣬�ֱ��ǣ�
- 1. connect��: ���ӷ���ʱ�Ļص�
- 2. close��: ���������ر�ʱ�Ļص�
- 3. recv��: ��ȡ����ʱ�Ļص�
- 4. send��: ��������ʱ�Ļص�


����ص���ԭ�;���ͬ�����ǵ�һ�����������Զ���Ļص�������

����ص����������ɾ������������Ӷ���ص�������ͬһ�ص���

@note Ŀǰ֧��connect, WSAConnect, send, WSASend, recv, WSARecv, closesocket�����Ļص�
@note ʹ�÷�ʽ: 
- ��һ��: �ȵ���SKH_Init\n
- �ڶ���: ʹ��SKH_AddXXXXע�����Ȥ�Ļص�
- ������: �ڻص��ﴦ�����ص�����
- ���Ĳ�: ���Ҫ��������socket��ָ���ԭ��״̬�������SKH_Deinit
@note �ڻص�����������ʱ�����ڵ������̣߳�������һЩ���ƣ�
- ��Ҫ�������C���п⺯������Ϊ��ʱC���п�û�г�ʼ�����ܶ�C���п⺯��������⣩
- ��������, �ڻص�����������ļ�(����C���п��WIN32 API)�����socket�������������� (ԭ��δ��), ����Ϊ��ԭ��CSHKTest��ȡ�˻ص�Ͷ�ݣ������߳�д��ķ�ʽ
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
connect�Ļص�ԭ��
@param pParan �ص�����
@param s SOCKET
@param port �˿�
@param ip IP��
*/
typedef void socket_connect_hook_proc_t(void* pParam, SOCKET s, unsigned int port, const char* ip);

/*!
recv�Ļص�ԭ��
@param pParam �ص�����
@param s SOCKET
@param buf ���ջ�����
@param len ���յ����ֽ���
@param oob �Ƿ���OOB����
@param peek �Ƿ��Ƕ�ȡ�����Ƴ�ģʽ
*/
typedef void socket_recv_hook_proc_t(void* pParam, SOCKET s, void* buf, int len, bool oob, bool peek);

/*!
send�Ļص�ԭ��
@param pParam �ص�����
@param s SOCKET
@param buf ���ͻ�����
@param len ���ͳ�ȥ���ֽ���
@param oob �Ƿ���OOB����
@param 
*/
typedef void socket_send_hook_proc_t(void* pParam, SOCKET s, void* buf, int len, bool oob);


/*!
closesocket�Ļص�ԭ��
@param pParam �ص�����
@param s SOCKET
*/
typedef void socket_close_hook_proc_t(void* pParam, SOCKET s);


/*!
connect���غ����Ļص�ԭ��
@param pParan �ص�����
@param s ԭʼSOCKET
@param name ԭʼ��ַ
@param ip IP�� ԭʼ��ַ����
@param newS ������SOCKET
@param newName �����µ�ַ
@param newNamelen �����µ�ַ�ĳ���
*/
typedef void socket_connect_intercept_proc_t(void* pParam, IN SOCKET s, IN const struct sockaddr name, IN int namelen,
                                             OUT SOCKET* newS,  OUT struct sockaddr* newName,OUT int *newNamelen);

/*!
recv���غ����Ļص�ԭ��
@param pParam �ص�����
@param s SOCKET
@param buf ���ջ�����
@param len ���յ����ֽ���
@param oob �Ƿ���OOB����
@param peek �Ƿ��Ƕ�ȡ�����Ƴ�ģʽ
@param newS ������SOCKET
@param newBuf �����µĽ��ջ�����, �˻�����Ҫ�������ά��������һ�ε��ô˻ص����ǻص�ʧЧǰҪ�󻺳�����Ч
@param newLen ���ؽ��յ����ֽ���
@param newOob �����Ƿ���OOB����
@param newPeek �����Ƿ��Ƕ�ȡ�����Ƴ�ģʽ
@note ���Ҫ��newlen����Ϊ-1��һ��Ҫ���صĿ��Ǻ��
*/
typedef void socket_recv_intercept_proc_t(void* pParam, IN SOCKET s, IN void* buf, IN int len, IN bool oob, IN bool peek
                                           ,OUT SOCKET *newS, OUT void** newBuf, OUT int* newlen, OUT bool* newOob, OUT bool* newPeek);

/*!
send�����غ����ص�ԭ��
@param pParam �ص�����
@param s SOCKET
@param buf ���ͻ�����
@param len ���ͳ�ȥ���ֽ���
@param oob �Ƿ���OOB����
@param newS ������SOCKET
@param newBuf �����µĽ��ջ��������˻�����Ҫ�������ά��������һ�ε��ô˻ص����ǻص�ʧЧǰҪ�󻺳�����Ч
@param newLen ���ؽ��յ����ֽ���
@param newOob �����Ƿ���OOB����
@param 
*/
typedef void socket_send_intercept_proc_t(void* pParam, IN SOCKET s, IN void* buf, IN int len, IN bool oob
                                          ,OUT SOCKET *newS, OUT void** newBuf, OUT int* newlen, OUT bool* newOob);


/*!
closesocket�����غ����ص�ԭ��
@param pParam �ص�����
@param s SOCKET
@param newS �����µ�SOCKET
*/
typedef void socket_close_intercept_proc_t(void* pParam, IN SOCKET s, OUT SOCKET *newS);


///SocketHooker��ʼ��
void SKH_Init();

///SocketHooker����ʼ��
void SKH_Deinit();

///�����ٽ�������ص�����
void SKH_EnterCritical();

///�˳��ٽ�������ص�����
void SKH_LeaveCritical();

///���Connect HOOK
bool SKH_AddConnectHook(void* pParam, socket_connect_hook_proc_t onConnect);
///�Ƴ�Connect HOOK
bool SKH_RemoveConnectHook(void* pParam, socket_connect_hook_proc_t onConnect);

///���Recv HOOK
bool SKH_AddRecvHook(void* pParam, socket_recv_hook_proc_t onRecv);
///�Ƴ�Recv HOOK
bool SKH_RemoveRecvHook(void* pParam, socket_recv_hook_proc_t onRecv);

///���Send HOOK
bool SKH_AddSendHook(void* pParam, socket_send_hook_proc_t onSend);
///�Ƴ�Send HOOK
bool SKH_RemoveSendHook(void* pParam, socket_send_hook_proc_t onSend);

///���Close HOOK
bool SKH_AddCloseHook(void* pParam, socket_close_hook_proc_t onClose);
///ɾ��Close HOOK
bool SKH_RemoveCloseHook(void* pParam, socket_close_hook_proc_t onClose);


///���Connect INTERCEPT
bool SKH_AddConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);
///�Ƴ�Connect INTERCEPT
bool SKH_RemoveConnectIntercept(void* pParam, socket_connect_intercept_proc_t onConnect);

///���Recv INTERCEPT
bool SKH_AddRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);
///�Ƴ�Recv INTERCEPT
bool SKH_RemoveRecvIntercept(void* pParam, socket_recv_intercept_proc_t onRecv);

///���Send INTERCEPT
bool SKH_AddSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);
///�Ƴ�Send INTERCEPT
bool SKH_RemoveSendIntercept(void* pParam, socket_send_intercept_proc_t onSend);

///���Close INTERCEPT
bool SKH_AddCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);
///ɾ��Close INTERCEPT
bool SKH_RemoveCloseIntercept(void* pParam, socket_close_intercept_proc_t onClose);

///connect����
typedef int (WINAPI *connectFunc_t)(
                                    SOCKET s,
                                    const struct sockaddr* name,
                                    int namelen
                                    ) ;
///closesocket����
typedef int (WINAPI *closesocketFunc_t)(
                                        SOCKET s
                                        );

///recv����
typedef int (WINAPI *recvFunc_t)(
                                 SOCKET s,
                                 char* buf,
                                 int len,
                                 int flags
                                 ) ;
///send����
typedef int (WINAPI *sendFunc_t)(
                                 SOCKET s,
                                 const char* buf,
                                 int len,
                                 int flags
                                 );

//WSAConnect����
typedef int (WINAPI *WSAConnectFunc_t)(
                                       SOCKET s,
                                       const struct sockaddr* name,
                                       int namelen,
                                       LPWSABUF lpCallerData,
                                       LPWSABUF lpCalleeData,
                                       LPQOS lpSQOS,
                                       LPQOS lpGQOS
                                       );
///WSARecv����
typedef int  (WINAPI *WSARecv_Func_t)(
                                      SOCKET s,
                                      LPWSABUF lpBuffers,
                                      DWORD dwBufferCount,
                                      LPDWORD lpNumberOfBytesRecvd,
                                      LPDWORD lpFlags,
                                      LPWSAOVERLAPPED lpOverlapped,
                                      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
                                      );
///WSASend����
typedef int (WINAPI *WSASend_Func_t)(
                                     SOCKET s,
                                     LPWSABUF lpBuffers,
                                     DWORD dwBufferCount,
                                     LPDWORD lpNumberOfBytesSent,
                                     DWORD dwFlags,
                                     LPWSAOVERLAPPED lpOverlapped,
                                     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
                                     );

///WSAGetOverlappedResult����
typedef BOOL (WINAPI *WSAGetOverlappedResultFunc_t)(
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD lpcbTransfer,
    BOOL fWait,
    LPDWORD lpdwFlags
    );

///����SocketFunction��һ���ṹ��
typedef struct SocketFunctions{
    connectFunc_t connect;
    closesocketFunc_t close;
    recvFunc_t recv;
    sendFunc_t send;
}SocketFunctions_t,*PSocketFunctions;

///��SKH_INIT֮���ܵ��ô˷���ȥ���ԭʼsocketϵ�к����ĵ�ַ
void SKH_GetSocketFunctions(SocketFunctions *func);


void SKH_AddIgnoreRange(void * beginAdress, void * endAdress);


#endif