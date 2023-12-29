// CLpc.h: interface for the CCLpc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLPC_H__1200A0CA_80F4_4BDA_B551_9C33424F7362__INCLUDED_)
#define AFX_CLPC_H__1200A0CA_80F4_4BDA_B551_9C33424F7362__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Lpc.h"
#include "LpcTran.h"

//////////////////////////////////////////////////////////////////////////
/************************************************************************/
/* 
Class:		 CCLpc
Description: 客户端使用,阻塞式、非阻塞式发送数据、接受数据、接受连接
*/
/************************************************************************/
class CCLpc : public CLpc  
{
public:
	// 连接的Port端口名,客户端共享内存区大小
	CCLpc(WCHAR* PortName, LONGLONG SectionSize);
	virtual ~CCLpc();

public:
	BOOL Initled();
	// 发起连接
	BOOL Connect(IN OUT PVOID ConnectionInformation OPTIONAL,
				 IN OUT PULONG ConnectionInformationLength OPTIONAL,
				 OUT PULONG MaxMessageLength); 

	
	// 调用LpcReplyWaitReceivePort()接受数据
	BOOL Receive(OUT PVOID *PortContext OPTIONAL); 

	// 发送回应消息 最终调用NtReplyPort()
	BOOL ReplySyncSend(IN ULONG Command, 
					   IN PVOID DataStartAddr, 
					   IN ULONG DataLength);

	// 发送数据立即返回不等待 最终调用NtRequestPort()
	BOOL AsyncSend(IN ULONG Command, 
				   IN PVOID DataStartAddr, 
				   IN ULONG DataLength);

	// 发送数据并等待返回 最终调用NtRequestWaitReceivePort()
	BOOL SyncSend(IN ULONG Command, 
				  IN PVOID DataStartAddr, 
				  IN ULONG DataLength);

	// 获取通过Receive后接受的数据
	PBYTE GetRecvData(ULONG *nLen);

	// 得到命令 
	ULONG GetCommand();

	//工作线程,负责读出数据
	static DWORD WINAPI WorkThread(LPVOID lparam);

private: // 监时测试用PUBLIC
	HANDLE		m_PortHandle;				// 通信端口句柄 
	TRANSFERRED_MESSAGE m_SendMessage;		// 发送的短报文
	TRANSFERRED_MESSAGE m_ReceiveMessage;	// 接受的短报文
	PORT_VIEW	m_ClientView;				// 客户端共享内存区
	REMOTE_PORT_VIEW m_ServerView;			// 服务端共享内存区
	UNICODE_STRING m_PortName;				// 连接的端口名
	HANDLE m_SectionHandle;					// 共享内存区句柄
	ILpcTran* m_lplpcTran;					// 数据发送接口
};

#endif // !defined(AFX_CLPC_H__1200A0CA_80F4_4BDA_B551_9C33424F7362__INCLUDED_)
