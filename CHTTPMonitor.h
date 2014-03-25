/*!
@file CHTTPMonitor.h
@author terry
@brief HTTP资源监视器
*/

#ifndef CHTTPMONITOR_H
#define CHTTPMONITOR_H

#include "MediaMonitor/Monitor.h"
#include "url/CHttpHeader.h"
#include "ByteBuffer.h"
using namespace MoyeaBased;

enum HTTP_MONITOR_STATE{
    WAIT_FOR_SENDHEAD,
    WAIT_FOR_WHOLEHEAD,//等待完成整个发送包头
    WAIT_FOR_REPONSE_HEAD,
    WAIT_FOR_REPONSE_WHOLE_HEAD,///等待完成整个接收包头
    WAIT_FOR_REPONSE_BODY,
    WAIT_FOR_GET_URL///完成状态，等待取走url
};

/*!
@class CHTTPMonitor
@brief Http媒体资源监视器
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

    ///忽略当前send-recv消息对，等待一下个轮回
    void IgnoreCurSeesion();
private:
    void DealHeader();    
    void GenerateUrl();
private:
    HTTP_MONITOR_STATE m_state;
    IURL* m_Url;
    string m_HttpSendHeader;///<发送的头部信息
    string m_HttpRecvHeader;///<接收的头部信息
    IBytesBuffer* m_PartBodyData;///<部分的http消息体数据
    bool m_HasCheck;///<是否已经检查过第一个接收的包
    string m_MIME;
    size_t m_BodyLength;
    size_t m_WantSaveBodySize;
};


#endif