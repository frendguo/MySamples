// SLpc.cpp: implementation of the CSLpc class.
//
//////////////////////////////////////////////////////////////////////

#include "SLpc.h"
#include <stdio.h>


#include "debug.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSLpc::CSLpc(WCHAR *PortName, LONGLONG SectionSize)
{
	LARGE_INTEGER LISectionSize;
	BOOL		  bRet;
	//Init member
	m_hWorkTread = m_PortHandle = m_ServerHandle = m_SectionHandle = NULL;
	// 
	// 初始化命名端口名
	//
	RtlInitUnicodeString(&m_PortName, PortName);
	
	// 
	// 创建服务端Section
	//
	LISectionSize.QuadPart = SectionSize;
	bRet = LpcCreateSection(&m_SectionHandle,
		SECTION_MAP_READ | SECTION_MAP_WRITE,
		NULL,
		&LISectionSize,
		PAGE_READWRITE,
		SEC_COMMIT,
		NULL);
	
	if (!bRet)
	{
		// 不成功则把控制器置为FALSE
		m_bIsOK = FALSE;
	}
	else
	{
		// 成功则把控制器置为TRUE,进行进一步初始化
		m_bIsOK = TRUE;
		m_PortHandle = NULL;
		m_ServerHandle = NULL;
		
		// 初始化用于服务端写入的PORT_VIEW
		m_ServerView.Length        = sizeof(PORT_VIEW);
		m_ServerView.SectionHandle = m_SectionHandle;
		m_ServerView.SectionOffset = 0;
		m_ServerView.ViewSize      = (ULONG)SectionSize;
		
		// 初始化用于读取客户端REMOTE_PORT_VIEW
		m_ClientView.Length		   = sizeof(REMOTE_PORT_VIEW);

		// 初始化传输接口
		m_lplpcTran = new LpcTran();
	}
	OBJECT_ATTRIBUTES ObjAttr;
    SECURITY_DESCRIPTOR sd;
	DWORD nError;
	
	if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
	{
		nError = GetLastError();
	}
	
	//
	// Set the empty DACL to the security descriptor
	//
	
	if(!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
	{
		nError = GetLastError();
	}
	
	InitializeObjectAttributes(&ObjAttr, &m_PortName, 0, NULL, &sd);
	
	// 
	// 创建一个命名端口对象,名字等相关属性在参数ObjectAttributes中
	//
	bRet = LpcCreatePort(&m_ServerHandle,
		&ObjAttr,
		256,
		256,
		0);
	
}

CSLpc::~CSLpc()
{
	
	if (NULL != m_PortHandle)
	{
		NtClose(m_PortHandle);
	}
	
	if (NULL != m_ServerHandle)
	{
		NtClose(m_ServerHandle);
	}
	
	if (NULL != m_SectionHandle)
	{
		NtClose(m_SectionHandle);
	}
	if(m_hWorkTread)
	{
		CloseHandle(m_hWorkTread);
	}
	if (NULL != m_lplpcTran)
	{
		delete m_lplpcTran;
	}

}

/************************************************************************/
/*
Parameter:   -------------------------------------------------------------
用于创建端口的参数
MaxConnectionInfoLength 指定通过通信端口发送的数据的最大字节数
MaxMessageLength 指定报文Message的最大字节数
MaxPoolUsage 指定非分页池的最大数量,0表示使用默认值
Return:      BOOL
Description: 调用后m_ReceiveMessage将得到连接时请求报文信息,成功返回TRUE,
否则返回FALSE			 
*/
/************************************************************************/
BOOL CSLpc::Linsten(IN ULONG MaxConnectionInfoLength,
					   IN ULONG MaxPoolUsage)
{
	
	BOOL bRet = TRUE;

	
	//
	// 创建命名端口成功,则在该端口上监听
	//
	if (bRet)
	{
		bRet = LpcListenPort(m_ServerHandle, &m_ReceiveMessage.Header);
	}
	
	
	return bRet;
}

/************************************************************************/
/*
Parameter:   PortContext 和端口相关联的数字标示
AcceptConnection 指定是否接受连接
-------------------------------------------------------------
Return:	     BOOL
Description: m_ReceiveMessage中指定要返回给连接请求者的连接数据,成功返回TRUE
否则返回FALSE
*/
/************************************************************************/
BOOL CSLpc::Accept(IN PVOID PortContext, 
				   IN BOOLEAN AcceptConnection)
{
	BOOL bRet= TRUE;
	
	bRet = LpcAcceptConnectPort(&m_PortHandle,
								PortContext,
								&m_ReceiveMessage.Header,
								AcceptConnection,
								&m_ServerView,
								&m_ClientView
								);
	
	if (bRet)
	{
		bRet = LpcCompleteConnectPort(m_PortHandle);
	}
// 	if (bRet)
// 	{
// 		m_hWorkTread = CreateThread(NULL,0,WorkThread,this,0,NULL);
// 	}	
	return bRet;
	
}

BOOL CSLpc::ReplySyncSend(IN ULONG Command, 
						  IN PVOID DataStartAddr, 
						  IN ULONG DataLength)
{
	BOOL bRet;

	InitializeMessageData(&m_ReceiveMessage,
						  &m_ServerView,
						  Command,
						  DataStartAddr,
						  DataLength);

	bRet = m_lplpcTran->SendWithReplyPort(m_PortHandle, &m_ReceiveMessage);
	
	return bRet;
}

/************************************************************************/
/* 
Parameter:	 PortContext 指示一个和端口相关的数字标示
Return:      BOOL
Description: m_ReceiveMessage将得到报文的信息,成功返回TRUE,否则返回FALSE
*/
/************************************************************************/
BOOL CSLpc::Receive(OUT PVOID *PortContext OPTIONAL)
{
	BOOL bRet;
	
	bRet = m_lplpcTran->Receive(m_PortHandle,
								PortContext, 
								NULL, 
								&m_ReceiveMessage);
	
	
// 	wprintf(L"SERVER: The Command is %d: \n", m_ReceiveMessage.Command);
// 	if (m_ReceiveMessage.DataLength < 224)
// 	{
// 		wprintf(L"SERVER: The Little Msg is : \"%s\" \n", m_ReceiveMessage.MessageText);
// 	}
// 	else
// 	{
// 		wprintf(L"SERVER: *************************************************\n");
// 		wprintf(L"SERVER: The BIG MSG is : \"%s\" \n", m_ClientView.ViewBase);
// 		wprintf(L"SERVER: *************************************************\n");
// 	}
	
	return bRet;
	
}


BOOL CSLpc::AsyncSend(IN ULONG Command, 
					 IN PVOID DataStartAddr, 
					 IN ULONG DataLength)
{
	
	
	// 初始化发送的数据 
	InitializeMessageData(&m_SendMessage, 
						  &m_ServerView, 
						  Command, 
						  DataStartAddr, 
						  DataLength);
	
// 	m_SendMessage.Command = Command;
// 	m_SendMessage.DataLength = DataLength;
// 	
// 	if (DataLength < 224)
// 	{
// 		// 消息内容少于228字节则随报文一起发送
// 		CopyMemory((PVOID)&(m_SendMessage.MessageText), 
// 			DataStartAddr, 
// 			DataLength);
// 	}
// 	else
// 	{
// 		// 消息内容大于228字节则通过Section发送
// 		CopyMemory(m_ServerView.ViewBase,
// 			DataStartAddr,
// 			DataLength);
// 	}

	return m_lplpcTran->SendWithRequestPort(m_PortHandle, &m_SendMessage);	
}

BOOL CSLpc::SyncSend(IN ULONG Command,
					 IN PVOID DataStartAddr, 
					 IN ULONG DataLength)
{
	BOOL bRet;

#ifdef _DEBUG
	ShowMyMsg(_T("===> CSLpc::SyncSend()\n"));
#endif


	// 初始化发送的数据
	InitializeMessageData(&m_SendMessage, 
						  &m_ServerView, 
						  Command,
						  DataStartAddr,
						  DataLength);
	
// 	m_SendMessage.Command = Command;
// 	m_SendMessage.DataLength = DataLength;
// 	
// 	if (DataLength < 224)
// 	{
// 		// 消息内容少于228字节则随报文一起发送
// 		CopyMemory((PVOID)&(m_SendMessage.MessageText), 
// 			DataStartAddr, 
// 			DataLength);
// 	}
// 	else
// 	{
// 		// 消息内容大于228字节则通过Section发送
// 		CopyMemory(m_ServerView.ViewBase,
// 			DataStartAddr,
// 			DataLength);
// 	}

	bRet = m_lplpcTran->SendWithRequestWaitReplyPort(m_PortHandle,
													&m_SendMessage,
													&m_ReceiveMessage);



#ifdef _DEBUG
	wprintf(L"SERVER SEND: The Command is %d: \n", m_SendMessage.Command);
	if (m_SendMessage.DataLength < 224)
	{
		wprintf(L"SERVER SEND: The Little Msg is : \"%s\" \n", m_ReceiveMessage.MessageText);
	}
	else
	{
		wprintf(L"SERVER SEND: *************************************************\n");
		wprintf(L"SERVER SEND: The BIG MSG is : \"%s\" \n", m_ServerView.ViewBase);
		wprintf(L"SERVER SEND: *************************************************\n");
	}

	wprintf(L"SERVER RECV: The Command is %d: \n", m_ReceiveMessage.Command);
	if (m_ReceiveMessage.DataLength < 224)
	{
		wprintf(L"SERVER RECV: The Little Msg is : \"%s\" \n", m_ReceiveMessage.MessageText);
	}
	else
	{
		wprintf(L"SERVER RECV: *************************************************\n");
		wprintf(L"SERVER RECV: The BIG MSG is : \"%s\" \n", m_ClientView.ViewBase);
		wprintf(L"SERVER RECV: *************************************************\n");
	}

	ShowMyMsg(_T("<=== CSLpc::SyncSend()\n"));
#endif

	return bRet;


}

// 工作者线程服务端使用异步读取缓冲数据
DWORD WINAPI CSLpc::WorkThread(LPVOID lparam)
{
	CSLpc *lpClpc = (CSLpc *)lparam;
//	static WCHAR buf[20];
//	memset(buf, 0, 20);
	if (NULL == lpClpc)
	{
		return 0;
	}
// 	for (int i = 0; i < 19 && lpClpc->m_bIsOK; ++i)
// 	{
// 		
// 		_itow(i, buf, 10);
// 		
// 		lpClpc->LpcReceive(NULL);
// 		
// 		lpClpc->LpcSendWithRequestPort(i, buf, 210);
// 		
// 	}
	while(lpClpc->m_PortHandle != INVALID_HANDLE_VALUE)
	{
		lpClpc->Receive(NULL);
		OutputDebugString(L"CSLpc::WorkThread lpClpc->LpcReceive 返回");
	}
	return 0;
}



PBYTE CSLpc::GetRecvData(ULONG *nLen)
{
	// 得到接收的数据大小
	*nLen = m_ReceiveMessage.DataLength;
	if (*nLen < 224)
	{
		return (PBYTE)&m_ReceiveMessage.MessageText;
	}
	else
	{
		return (PBYTE)m_ClientView.ViewBase;
	}
}


ULONG CSLpc::GetCommand()
{
	return m_ReceiveMessage.Command;
}