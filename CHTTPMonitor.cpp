#include "MediaMonitor/CHTTPMonitor.h"
#include "url/CHttpHeader.h"
#include "logger.h"
#include "sysutils.h"
#include "VariantUtilitys.h"
using namespace MoyeaBased;
#include "flvinfo_paser.h"
#include "RTMPDefines.h"
#include "CUrlCvt.h"
#include <stdlib.h>
InitBinLog;
#define WANT_BODY_BLOCK_SIZE 4096
#include <fstream>
/*Class CHTTPMonitor*/
CHTTPMonitor::CHTTPMonitor()
{
    try {
        m_Url = NULL;
        m_PartBodyData = false;
        m_state = WAIT_FOR_SENDHEAD;
        m_HasCheck = false;
        m_WantSaveBodySize = 0;
    }CatchAllAndCopy();
}
CHTTPMonitor::~CHTTPMonitor()
{
    try {
        DELETEINTERFACE(m_Url);
    }CatchAllAndCopy();
}
void STDCALL CHTTPMonitor::Free()
{
    delete this;
}
bool STDCALL CHTTPMonitor::OnSend(void* buf, int len, bool oob)
{
	std::fstream mysend("e:/onsend",std::fstream::out|std::fstream::app|std::fstream::binary);
	mysend.write((const char*)buf,len);
	mysend.close();
    if (m_Url != NULL)
    {
        return true;
    }

    try {
        CheckPointer(buf);
        char* pData = (char *)buf;
        if (m_state == WAIT_FOR_SENDHEAD)
        {
            if (strNCaseInsensitiveCompare(pData, HTTP_METHOD_GET, 3) == 0)
            {
                char* pEnd = strstr(pData, HTTP_HEADER_END);
                if(pEnd != NULL)
                {
                    pEnd += strlen(HTTP_HEADER_END);
                    m_state = WAIT_FOR_REPONSE_HEAD;
                }
                else
                {
                    m_state = WAIT_FOR_WHOLEHEAD;
                }
                m_HttpSendHeader.append(pData, pEnd - pData);
            }
            else
            {
                ThrowExp(errBadParam,"Http header is not begin with \"get\".");
            }            
        } 
        else if (m_state == WAIT_FOR_WHOLEHEAD)
        {
            char* pEnd = strstr(pData, HTTP_HEADER_END);
            if(pEnd)
            {
                pEnd += strlen(HTTP_HEADER_END);
                m_state = WAIT_FOR_REPONSE_HEAD;
            }

            m_HttpSendHeader.append(pData, pEnd - pData);
        }
        else
        {
           // assert(false);
            //ThrowUnexpected();
        }

		if(m_state==WAIT_FOR_REPONSE_HEAD && ((m_HttpSendHeader.find("googlevideo.com",0)!=std::string::npos) || m_HttpSendHeader.find("youtube.com",0)!=std::string::npos))
		{

			size_t pos_begin,pos_end,pos_got,pos_youtube;
			pos_begin=m_HttpSendHeader.find("GET",0);
			pos_end=m_HttpSendHeader.find(" ",pos_begin+4);
			if((pos_youtube=m_HttpSendHeader.find("/watch?v=",0))!=std::string::npos && pos_youtube<pos_end && pos_youtube>pos_begin)
			{
				pos_begin=pos_youtube+sizeof("/watch?v=")-1;
			}
			else
			{
				if((pos_youtube=m_HttpSendHeader.find("/embed/",0))!=std::string::npos && pos_youtube<pos_end && pos_youtube>pos_begin)
				{
					pos_begin=pos_youtube+sizeof("/embed/")-1;

				}
				else
				{
					pos_youtube=std::string::npos;
					return true;
				}
			}

			if(pos_youtube!=std::string::npos)
				{			
					std::string new_header;
					std::string::size_type src_begin,src_end;
					src_begin=pos_begin;
					src_end=pos_end;
					std::string id=m_HttpSendHeader.substr(src_begin,YOUTUBEIDLENGTH);
					CUrlCvt cvt(id);
					cvt.SetQueryUrl(YOUTUBEQUERYURL);
					cvt.SetUrlBeginFlag(YOUTUBEURLHEADER);
					cvt.SetUrlSepFlag(YOUTUBEURLSEPERATOR);
					
					cvt.ParseUrl();
					std::vector<std::string> mylist;
					mylist=cvt.GetDownloadUrl();
					std::string tmp=mylist[0];
					m_Url=ParserURL(tmp.c_str(),"http");

					src_begin=tmp.find("itag=",0)+sizeof("itag=")-1;
					src_end=tmp.find("&",src_begin);
					std::string itag_str=tmp.substr(src_begin,src_end-src_begin);
					int itag_int=atoi(itag_str.c_str());
					IVariantMap* pmap=m_Url->GetPropertyMap();
					switch(itag_int)
					{
					case 34:
					case 35:
					case 5:
					case 6:
						pmap->SetKeyValue(HTTP_URL_PROPERTY_MIME,String2Variant(MIME_FLASHVIDEO));
						break;

					case 18:
					case 22:
					case 37:
					case 38:
					case 82:
					case 84:
						pmap->SetKeyValue(HTTP_URL_PROPERTY_MIME,String2Variant(MIME_MP4));
						break;
						
					case 13:
					case 17:
					case 36:
						pmap->SetKeyValue(HTTP_URL_PROPERTY_MIME,String2Variant("video/3gpp"));
						break;
						
					case 43:
					case 45:
					case 46:
						pmap->SetKeyValue(HTTP_URL_PROPERTY_MIME,String2Variant("video/webm"));
						break;

					default:
							break;
					}
					m_state=WAIT_FOR_GET_URL;	
			}
			else
			{
				IgnoreCurSeesion();
			}
			
		}
		
		if(m_state==WAIT_FOR_REPONSE_HEAD && m_HttpSendHeader.find("Host: www.dailymotion.com",0)!=std::string::npos)
		{
			int pos_prefix_id,pos_suffix_id,pos_id,pos_id_sep,pos_url_sep;
			pos_prefix_id=m_HttpSendHeader.find("GET",0);
			pos_suffix_id=m_HttpSendHeader.find(" ",pos_prefix_id+4);
			pos_id=m_HttpSendHeader.find("/video/",0);
			pos_id_sep=m_HttpSendHeader.find("_",pos_id);
			pos_url_sep=m_HttpSendHeader.find(" ",pos_id);
			if(pos_id!=std::string::npos && pos_id>pos_prefix_id && pos_id<pos_suffix_id && pos_id_sep!=std::string::npos && pos_url_sep!=std::string::npos && pos_id_sep<pos_id+15)
			{
				CUrlCvt cvt;
				
				std::string::size_type begin,end;
				begin=m_HttpSendHeader.find("GET ",0)+4;
				end=m_HttpSendHeader.find(" ",begin);
				std::string url="www.dailymotion.com";
				url.append(m_HttpSendHeader.c_str()+pos_id,pos_url_sep-pos_id);
				cvt.SetQueryUrl(url);
				cvt.SetUrlBeginFlag("video_url%22%3A%22");
				cvt.SetUrlSepFlag("%22%2C%22");
				cvt.ParseUrl();
				std::vector<std::string> mylist;
				mylist=cvt.GetDownloadUrl();
				std::string tmp=mylist[0];
				m_Url=ParserURL(tmp.c_str(),"http");
				m_state=WAIT_FOR_GET_URL;
			}
			else
			{
				IgnoreCurSeesion();
			}
		}
		
		return true;
    }CatchAllAndCopy();
    return false;
}
bool STDCALL CHTTPMonitor::OnRecv(void* buf, int len, bool oob, bool peek)
{
	std::fstream fs("e:/onrecv",std::fstream::app|std::fstream::out|std::fstream::binary);
	fs.write((const char*)buf,len);
	fs.close();
    if (m_Url != NULL)
    {
        return true;
    }

#ifdef _DEBUG
    if (strstr((char*)buf, "video/flv") != NULL)
    {
        LogBin(buf, len, "catch flv packet.");
    }
#endif // _DEBUG
    

    try {

        ///在等待发送http请求
        if (m_state == WAIT_FOR_SENDHEAD || m_state == WAIT_FOR_WHOLEHEAD)
        {
            return true;
        }

        CheckPointer(buf);
        char* pData = (char *)buf;
		std::string mytest=m_HttpSendHeader;
         int nLeaveLen = len;
        uint32_t offset = 0;
        while (nLeaveLen > 0)
        {
            if (m_state == WAIT_FOR_REPONSE_HEAD || m_state == WAIT_FOR_REPONSE_WHOLE_HEAD)
            {
                char* pEnd = strstr(pData, HTTP_HEADER_END);
                offset = uint32_t((pEnd - pData) + strlen(HTTP_HEADER_END));               
                m_HttpRecvHeader.append(pData, offset);
                pData += offset;//jump to body
                nLeaveLen -= offset;

                if(pEnd == NULL)
                {
					m_HttpRecvHeader.append(pData,len);
                    m_state = WAIT_FOR_REPONSE_WHOLE_HEAD;
                }
                else
                {
					m_state=WAIT_FOR_REPONSE_BODY;
                    DealHeader();
                    if (m_BodyLength == 0)
                    {
                        IgnoreCurSeesion();
                        return true;///已经忽略当前Send－recv信息，等待下一次交互
                    } 
                }               
            }
            else if(m_state == WAIT_FOR_REPONSE_BODY)
            {
                ///检测第一个body是否想要的类型
                if (!m_HasCheck)
                {
					
                    CheckFirstRecvBody(pData, nLeaveLen);
                    if(m_state == WAIT_FOR_SENDHEAD){
                        return true;///已经忽略当前Send－recv信息，等待下一次交互
                    }

                }      

                //如果检测通过，就拷贝想要的大小
                if (m_PartBodyData == NULL)
                {
                    m_PartBodyData = CreateBytesBuffer(m_WantSaveBodySize);
                }
                if (m_PartBodyData->Size() < m_WantSaveBodySize)
                {
                    m_PartBodyData->Malloc(m_WantSaveBodySize);
                    
                }

                if (m_PartBodyData->GetDataLen() < m_WantSaveBodySize)
                {
					int nHasDataLen = (int) m_PartBodyData->GetDataLen();
                    int nNeedLen = m_WantSaveBodySize - nHasDataLen;
                     offset = min(nNeedLen, nLeaveLen);
                    uint8_t * pBuffer = (uint8_t *)m_PartBodyData->GetBuffer() + nHasDataLen;
                    memcpy(pBuffer, pData, offset);
                    m_PartBodyData->SetDatalen(nHasDataLen + offset);
                    nLeaveLen -= offset;

                    if (m_PartBodyData->GetDataLen() == m_WantSaveBodySize)
                    {                       
                        GenerateUrl();
                        return true;
                    }
                }  
                else
                {
                    GenerateUrl();
                    return true;
                }
            }
        }
        return true;
    }CatchAllAndCopy();
    return false;
}
bool STDCALL CHTTPMonitor::IsComplete()
{
    try {
        return m_state == WAIT_FOR_GET_URL;
    }CatchAllAndCopy();
    return NULL;
}
IURL* STDCALL CHTTPMonitor::GetURL()
{
    try {
        assert(m_state == WAIT_FOR_GET_URL);
        if (m_state == WAIT_FOR_GET_URL)
        {
            return m_Url;
        }
        else
        {
            ThrowExp(errCallingContext, "Parse url has not finish.");
        }
    }CatchAllAndCopy();
    return NULL;
}
bool STDCALL CHTTPMonitor::ContinueMonitor()
{
    try{
        IgnoreCurSeesion();
        DELETEINTERFACE(m_Url);
        return true;
    }CatchAllAndCopy();
    return false;
}

void CHTTPMonitor::DealHeader()
{
   // assert(m_state >= WAIT_FOR_REPONSE_BODY );
    CHTTPRespondHeader  header(m_HttpRecvHeader);
    m_BodyLength = header.GetLength();
	size_t f4m_posbegin=m_HttpSendHeader.find(".f4m");
	size_t f4m_posend=m_HttpSendHeader.find("\r\n");
    if (m_BodyLength != 0)
    {
		if(f4m_posbegin<f4m_posend && f4m_posbegin!=std::string::npos && f4m_posend!=std::string::npos)
			m_WantSaveBodySize=m_BodyLength;
		else
			m_WantSaveBodySize = min(m_BodyLength, WANT_BODY_BLOCK_SIZE);
    }
}

void CHTTPMonitor::GenerateUrl()
{ 
	assert(m_state == WAIT_FOR_REPONSE_BODY);
    CHTTPQueryHeader query(m_HttpSendHeader);
    string url = query.GetAbsoluteURL();
	//从某些包含range（分段）标签的url中提取出完整视频的下载地址
	int nPos = -1;
	int nCount = 0;
	int pos1 = url.find("range=");
	int pos2 = -1;
	if(pos1 != -1)
	{
		 pos2 = url.find("&",pos1);
		 if(pos2 == -1)
			pos2 = url.size();
		 nCount = pos2 - pos1 + 1;
		 nPos = pos1;
	}
	if(nPos != -1 && nCount != 0)
		url.erase(nPos,nCount);
    m_Url = ParserURL(STR(url), "http");
    MAPI_SUCCESS(m_Url);

    //设置url的一些属性
    IVariantMap* pMap = m_Url->GetPropertyMap();

    //MIME
    pMap->SetKeyValue(HTTP_URL_PROPERTY_MIME, String2Variant(STR(m_MIME)));

    ///HTTP请求头
    pMap->SetKeyValue(HTTP_URL_PROPERTY_QUERY_HEADER_STRING, String2Variant(STR(m_HttpSendHeader)));

    ///HTTP回应头
    pMap->SetKeyValue(HTTP_URL_PROPERTY_REPONSER_HEADER_STRING, String2Variant(STR(m_HttpRecvHeader)));
    
    ///一部分的文件体
    //IVariant * pData = CreateVariant();
	IVariant* pData=String2Variant((const char*)m_PartBodyData->GetBuffer());
    //pData->SetDataBuffer(m_PartBodyData->Clone(true));
	pMap->SetKeyValue(HTTP_URL_PROPERTY_REPONSER_BODY_PARTDATA, pData);

     //metadata
     IVariantMap* m_pMetaData = NULL;
     uint8_t* pBuffer = (uint8_t*)m_PartBodyData->GetBuffer();
     if (memcpy(pBuffer,"FLV", 3) == 0)
     {
         m_pMetaData = GetFLVMetaDataFromMem(pBuffer, m_PartBodyData->GetDataLen());
     }

     if (m_pMetaData != NULL)
     {
         ///设置url中的媒体属性       
         IVariant* pTemp = NULL;
         if( m_pMetaData->HasKey(RTMP_METADATA_PARAMS_DURATION)  )
         {
             pTemp = m_pMetaData->GetValue(RTMP_METADATA_PARAMS_DURATION);
             pMap->SetKeyValue(MEDIA_URL_DURATION, pTemp) ;
         }

         if (m_pMetaData->HasKey(RTMP_METADATA_PARAMS_HEIGHT))
         {
             pTemp = m_pMetaData->GetValue(RTMP_METADATA_PARAMS_HEIGHT);
             if (pTemp->GetType() == VAR_ENUM_VALUE_DOUBLE)
             {
                 pMap->SetKeyValue(MEDIA_URL_HEIGHT, pTemp);
             }
         }

         if (m_pMetaData->HasKey(RTMP_METADATA_PARAMS_WIDTH))
         {
             pTemp = m_pMetaData->GetValue(RTMP_METADATA_PARAMS_WIDTH);
             if (pTemp->GetType() == VAR_ENUM_VALUE_DOUBLE)
             {
                 pMap->SetKeyValue(MEDIA_URL_WIDTH, pTemp);
             }
         }

         ///将metadata做为一个整体放到url属性
         IVariant* pMetaDataVar = CreateVariant();
         InterfaceHandleKeeper<IVariant* > IKMetadata(pMetaDataVar);
         pMetaDataVar->SetMap(m_pMetaData->Clone(true));
         m_pMetaData = NULL;
         m_Url->GetPropertyMap()->SetKeyValue(RTMP_URL_PROPERTY_KEY_METEDATA, pMetaDataVar);
         IKMetadata.Detach();
     }
     
     ///refer页面
     if (query.HasProperty(HTTP_HEADER_PROPERTY_KEY_REFERER))
     {
         string referPage;
         if( !query.GetProperty(HTTP_HEADER_PROPERTY_KEY_REFERER, referPage))
         {
             assert(false);
             WARN(STR(GetLastMoyeaExpAsText()));
         }
         pMap->SetKeyValue(MEDIA_URL_REFER_PAGEURL, String2Variant(STR(referPage)));
     }

    m_state = WAIT_FOR_GET_URL;
}

bool STDCALL CHTTPMonitor::FirstSendSuitForThisMonitor(void* buf, int len)
{
    if (len < 3 )
    {
        return false;
    }

    if (strNCaseInsensitiveCompare((char * )buf, HTTP_METHOD_GET, 3) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CHTTPMonitor::IgnoreCurSeesion()
{
    m_state = WAIT_FOR_SENDHEAD;
    m_HttpRecvHeader = "";
    m_HttpSendHeader = "";
    m_PartBodyData = false;
    m_HasCheck = false;
    m_WantSaveBodySize = 0;
    DELETEINTERFACE(m_PartBodyData);
}

void  CHTTPMonitor::CheckFirstRecvBody(void * buf, int len)
{
    //FIXME 对Content-Encoding: gzip的flv的支持  
	//增加对某些FLV文件无FLV头的支持
    if((m_MIME == MIME_FLASHVIDEO) || (m_MIME == MIME_MP4) )
	{
        IgnoreCurSeesion();
        return;
	}
	CHTTPRespondHeader  header(m_HttpRecvHeader);
	string strContentType("");
	header.GetProperty(HTTP_HEADER_PROPERTY_KEY_CONTENT_TYPE,strContentType);

	m_HasCheck = true;	
    char * pBuffer = (char *)buf;  
    if (memcmp(pBuffer, "FLV", 3) == 0 || strContentType == MIME_FLASHVIDEO)
    {
        m_MIME=MIME_FLASHVIDEO;
		return;
    }
	if (memcmp(pBuffer+4, "ftyp", 4) == 0 || strContentType == MIME_MP4)
    {
		m_MIME=MIME_MP4;
		return;
	}
	if(strContentType==MIME_F4M ||((m_HttpSendHeader.find(".f4m",0)<m_HttpSendHeader.find("\r\n",0)) && m_HttpSendHeader.find(".f4m",0) !=std::string::npos && (m_HttpSendHeader.rfind("http",m_HttpSendHeader.find(".f4m"))==std::string::npos || (m_HttpSendHeader.rfind(" ",m_HttpSendHeader.find(".f4m"))+1)==m_HttpSendHeader.rfind("http",m_HttpSendHeader.find(".f4m")) )))
	{
		m_MIME=MIME_F4M;
		return;
	}
	//不是想要的类型就重置当前状态到未初使化状态，等待此socket的下一次send
	IgnoreCurSeesion();
	LogBinStr(Format("It's not a flv file or mp4 file.file header:(%s).", (char*)(buf)));
	return;
}