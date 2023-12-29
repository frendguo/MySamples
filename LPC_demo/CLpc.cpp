// CLpc.cpp: implementation of the CCLpc class.
//
//////////////////////////////////////////////////////////////////////

#include "CLpc.h"
#include <stdio.h>


#include "debug.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCLpc::CCLpc(WCHAR* PortName, LONGLONG SectionSize)
{
	LARGE_INTEGER LISectionSize;
	BOOL		  bRet;
	
	//
	// 初始化要连接的命名端口名
	//
	RtlInitUnicodeString(&m_PortName, PortName);
	
	//
	// 初始化客户端Section
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
		// 不成功则控制器置为FALSE
		m_bIsOK = FALSE;
	}
	else
	{
		// 成功控制器置为TRUE,并进行进一步的初始化
		m_bIsOK = TRUE;
		m_PortHandle = NULL;
		
		// 初始化用于客户端写入的PORT_VIEW
		m_ClientView.Length        = sizeof(PORT_VIEW);
		m_ClientView.SectionHandle = m_SectionHandle;
		m_ClientView.SectionOffset = 0;
		m_ClientView.ViewSize      = (ULONG)SectionSize;
		
		// 初始化用于读取服务REMOTE_PORT_VIEW
		m_ServerView.Length		   = sizeof(REMOTE_PORT_VIEW);

		// 初始化数据传输接口
		m_lplpcTran = new LpcTran();
	}
}

CCLpc::~CCLpc()
{
	if (NULL != m_PortHandle)
	{
		NtClose(m_PortHandle);
	}
	
	if (NULL != m_SectionHandle)
	{
		NtClose(m_SectionHandle);
	}

	if (NULL != m_lplpcTran)
	{
		delete m_lplpcTran;
	}
}


BOOL CCLpc::Connect(IN OUT PVOID ConnectionInformation OPTIONAL, 
					IN OUT PULONG ConnectionInformationLength OPTIONAL, 
					OUT PULONG MaxMessageLength)
{
	
	BOOL bRet;
	OBJECT_ATTRIBUTES ObjAttr;
	SECURITY_QUALITY_OF_SERVICE SecurityQos;
	
	InitializeObjectAttributes(&ObjAttr, &m_PortName, 0, NULL, NULL);
	
	SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
	SecurityQos.ImpersonationLevel = SecurityImpersonation;
	SecurityQos.EffectiveOnly = FALSE;
	SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	
	
	bRet = LpcConnectPort(&m_PortHandle,
						  &SecurityQos,
						  &m_PortName,
						  &m_ClientView,
						  &m_ServerView,
						  MaxMessageLength,
						  ConnectionInformation,
						  ConnectionInformationLength
						  );
	
	
	return bRet;
}

BOOL CCLpc::ReplySyncSend(IN ULONG Command,
						  IN PVOID DataStartAddr,
						  IN ULONG DataLength)
{
// 	m_ReceiveMessage.Command = Command;
// 	m_ReceiveMessage.DataLength = DataLength;
// 	
// 	if (DataLength < 224)
// 	{
// 		// 消息内容少于228字节则随报文一起发送
// 		CopyMemory((PVOID)&(m_ReceiveMessage.MessageText), 
// 			DataStartAddr, 
// 			DataLength);
// 	}
// 	else
// 	{
// 		// 消息内容大于228字节则通过Section发送
// 		CopyMemory(m_ClientView.ViewBase,
// 			DataStartAddr,
// 			DataLength);
// 	}
// 
// 	return LpcReplyPort(m_PortHandle, &m_ReceiveMessage.Header);
	InitializeMessageData(&m_ReceiveMessage,
						  &m_ClientView,
						  Command,
						  DataStartAddr,
						  DataLength);

	return m_lplpcTran->SendWithReplyPort(m_PortHandle, &m_ReceiveMessage);

}


BOOL CCLpc::AsyncSend(IN ULONG Command, 
					  IN PVOID DataStartAddr, 
					  IN ULONG DataLength)
{
	
	// 初始化要传输的数据
	InitializeMessageData(&m_SendMessage,
						  &m_ClientView, 
						  Command,
						  DataStartAddr,
						  DataLength);
	
// 	// 初始化报文头
// 	InitializeMessageHeader(&(m_SendMessage.Header), 
// 		256,
// 		LPC_NEW_MESSAGE);
// 	
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
// 		CopyMemory(m_ClientView.ViewBase,
// 			DataStartAddr,
// 			DataLength);
// 	}
	
	return m_lplpcTran->SendWithRequestPort(m_PortHandle, &m_SendMessage);
	
	
}

BOOL CCLpc::SyncSend(IN ULONG Command,
					 IN PVOID DataStartAddr,
					 IN ULONG DataLength)
{
	InitializeMessageData(&m_SendMessage,
						  &m_ClientView, 
						  Command,
						  DataStartAddr,
						  DataLength);

	return m_lplpcTran->SendWithRequestWaitReplyPort(m_PortHandle,
													&m_SendMessage,
													&m_ReceiveMessage);
}

/************************************************************************/
/* 
Parameter: PortContext 指示一个和端口相关的数字标示
ReplyMessage 指示发送给端口的回应消息
ReceiveMessage 指示由端口接受的消息
*/
/************************************************************************/
BOOL CCLpc::Receive(OUT PVOID *PortContext OPTIONAL)
{
	BOOL bRet;
	
	bRet = m_lplpcTran->Receive(m_PortHandle,
								PortContext,
								NULL,
								&m_ReceiveMessage);
	
// 	wprintf(L"CLIENT: The Command is %d: \n", m_ReceiveMessage.Command);
// 	if (m_ReceiveMessage.DataLength < 224)
// 	{
// 		wprintf(L"CLIENT: The Little Msg is : \"%s\" \n", m_ReceiveMessage.MessageText);
// 	}
// 	else
// 	{
// 		wprintf(L"CLIENT: *************************************************\n");
// 		wprintf(L"CLIENT: The BIG MSG is : \"%s\" \n", m_ServerView.ViewBase);
// 		wprintf(L"CLIENT: *************************************************\n");
// 	}
// 	
	
	return bRet;
	
}

DWORD WINAPI CCLpc::WorkThread(LPVOID lparam)
{
	CCLpc *lpClpc = (CCLpc *)lparam;
	static WCHAR buf[20];
	memset(buf, 0, 20);
	if (NULL == lpClpc)
	{
		return 0;
	}
	for (int i = 0; i < 19 && lpClpc->m_bIsOK; ++i)
	{
		
		_itow(i, buf, 10);
		if (lpClpc->m_PortHandle != INVALID_HANDLE_VALUE)
		{

		lpClpc->Receive(NULL);
		OutputDebugString(L"WinLogon 接收到命令");
//		if (i != 18)
		if (lpClpc->m_PortHandle != INVALID_HANDLE_VALUE)
		lpClpc->AsyncSend(i, buf, 210);
		OutputDebugString(L"WinLogon 发送数据");				
		}
	}
	return 0;
}

PBYTE CCLpc::GetRecvData(ULONG *nLen)
{
	// 得到数据的大小
	*nLen = m_ReceiveMessage.DataLength; 
	if (*nLen < 224)
	{
		return (PBYTE)&m_ReceiveMessage.MessageText;
	}
	else
	{
		return (PBYTE)m_ServerView.ViewBase;
	}
}

ULONG CCLpc::GetCommand()
{
	return m_ReceiveMessage.Command;
}
BOOL CCLpc::Initled()
{
	return m_bIsOK;
}

// BOOL CCLpc::Initled()
// {
// 
// }
