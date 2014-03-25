/*!
@file CHTTPMonitor.h
@author terry
@brief HTTP��Դ������
*/

#ifndef CHTTPMONITOR_H
#define CHTTPMONITOR_H

#include "MediaMonitor/Monitor.h"
#include "url/CHttpHeader.h"
#include "ByteBuffer.h"
using namespace MoyeaBased;

enum HTTP_MONITOR_STATE{
    WAIT_FOR_SENDHEAD,
    WAIT_FOR_WHOLEHEAD,//�ȴ�����������Ͱ�ͷ
    WAIT_FOR_REPONSE_HEAD,
    WAIT_FOR_REPONSE_WHOLE_HEAD,///�ȴ�����������հ�ͷ
    WAIT_FOR_REPONSE_BODY,
    WAIT_FOR_GET_URL///���״̬���ȴ�ȡ��url
};

/*!
@class CHTTPMonitor
@brief Httpý����Դ������
*/
class CHTTPMonitor: public IMediaMonitor
{
public:
    CHTTPMonitor();
    ~CHTTPMonitor();
public:
    virtual void STDCALL Free();
    bool STDCALL OnSend(void* buf, int len, bool oob);
    bool STDCALL OnRecv(void* buf, int len, bool oob, bool peek);
    bool STDCALL IsComplete() ;
    IURL* STDCALL GetURL();
    virtual bool STDCALL ContinueMonitor();
    static bool STDCALL FirstSendSuitForThisMonitor(void* buf, int len);
private:
    void CheckFirstRecvBody(void * buf, int len);

    ///���Ե�ǰsend-recv��Ϣ�ԣ��ȴ�һ�¸��ֻ�
    void IgnoreCurSeesion();
private:
    void DealHeader();    
    void GenerateUrl();
private:
    HTTP_MONITOR_STATE m_state;
    IURL* m_Url;
    string m_HttpSendHeader;///<���͵�ͷ����Ϣ
    string m_HttpRecvHeader;///<���յ�ͷ����Ϣ
    IBytesBuffer* m_PartBodyData;///<���ֵ�http��Ϣ������
    bool m_HasCheck;///<�Ƿ��Ѿ�������һ�����յİ�
    string m_MIME;
    size_t m_BodyLength;
    size_t m_WantSaveBodySize;
};


#endif