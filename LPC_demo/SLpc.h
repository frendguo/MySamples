// SLpc.h: interface for the CSLpc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SLPC_H__8DE29723_0D1F_42B0_8F7B_CBEB0133A46C__INCLUDED_)
#define AFX_SLPC_H__8DE29723_0D1F_42B0_8F7B_CBEB0133A46C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Lpc.h"
#include "LpcTran.h"

//////////////////////////////////////////////////////////////////////////
/************************************************************************/
/*
Class:		 CSLpc
Description: 服务端使用、监听、接受连接、阻塞式发送、非阻塞式发送、接受数据
			 、回应阻塞式发送
*/
/************************************************************************/
class CSLpc : public CLpc  
{
public:
	// 监听Port名、服务端共享内存区大小
	CSLpc(WCHAR *PortName, LONGLONG SectionSize);
	virtual ~CSLpc();

public:

	// 监听
	BOOL Linsten(IN ULONG MaxConnectionInfoLength,
			     IN ULONG MaxPoolUsage); 
	
	// 接受连接
	BOOL Accept(IN PVOID PortContext,
				IN BOOLEAN AcceptConnection); 

	// 发送回应消息 NtReplyPort()
	BOOL ReplySyncSend(IN ULONG Command, 
					   IN PVOID DataStartAddr, 
					   IN ULONG DataLength); 
	
	// 调用LpcReplyWaitReceivePort()接受数据
	BOOL Receive(OUT PVOID *PortContext OPTIONAL); 

	// 发送数据不等待立即返回 NtRequestPort()
	BOOL AsyncSend(IN ULONG Command, 
				   IN PVOID DataStartAddr, 
				   IN ULONG DataLength);

	// 发送数据并等待数据返回 NtRequestWaitReplyPort()
	BOOL SyncSend(IN ULONG Command, 
				  IN PVOID DataStartAddr,
				  IN ULONG DataLength);


	// 获取通过Receive接受后的数据
	PBYTE GetRecvData(ULONG *);

	ULONG GetCommand();

private:
	//Add 工作线程句柄，接受到数据后通过socket发送
	HANDLE				m_hWorkTread;
	// 工作者线程服务端使用异步读取缓冲数据
	static DWORD WINAPI WorkThread(LPVOID lparam);
	//Add end
	HANDLE				m_PortHandle;		// 通信端口句柄
	HANDLE				m_ServerHandle;		// 监听端口句柄
	HANDLE				m_SectionHandle;	// 共享内存句柄
	PORT_VIEW			m_ServerView;		// 服务端共享内存映射
	REMOTE_PORT_VIEW	m_ClientView;		// 客户端共享内存映射
	TRANSFERRED_MESSAGE m_SendMessage;		// 发送的数据
	TRANSFERRED_MESSAGE m_ReceiveMessage;   // 接受的数据
	UNICODE_STRING		m_PortName;			// 用于监听的命名端口名
	ILpcTran*			m_lplpcTran;		// 数据传输接口

	TRANSFERRED_MESSAGE m_SYNReceive;		//同步接受
	
};

#endif // !defined(AFX_SLPC_H__8DE29723_0D1F_42B0_8F7B_CBEB0133A46C__INCLUDED_)
