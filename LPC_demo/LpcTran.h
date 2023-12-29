// LpcTran.h: interface for the LpcTran class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LPCTRAN_H__CF07ABDA_EBE6_4084_B777_978FBAC16EA1__INCLUDED_)
#define AFX_LPCTRAN_H__CF07ABDA_EBE6_4084_B777_978FBAC16EA1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ILpcTran.h"
#include "Lpc.h"

class LpcTran : 
	public CLpc, 
	public ILpcTran  
{
public:
	LpcTran();
	virtual ~LpcTran();

public:
	// 发送数据立即返回 NtRequestPort()
	virtual BOOL SendWithRequestPort(IN  HANDLE PortHandle,
									 IN  PTRANSFERRED_MESSAGE RequestMessage); 
	
	// 发送数据立即返回 NtReplyPort()
	virtual BOOL SendWithReplyPort(IN  HANDLE PortHandle,
								   IN  PTRANSFERRED_MESSAGE ReplyMessage); 
	
	// 阻塞接受数据 NtReplyWaitReceivePort()
	virtual BOOL Receive(IN  HANDLE PortHandle,
						 OUT PVOID *PortContext OPTIONAL,
						 IN  PTRANSFERRED_MESSAGE ReplyMessage OPTIONAL,
						 OUT PTRANSFERRED_MESSAGE ReceiveMessage); 
	
	// 发送数据并阻塞等待回应  NtRequestWaitReplyPort()
	virtual BOOL SendWithRequestWaitReplyPort(IN  HANDLE PortHandle,
											  IN  PTRANSFERRED_MESSAGE RequestMessage,
											  OUT PTRANSFERRED_MESSAGE ReplyMessage);

};

#endif // !defined(AFX_LPCTRAN_H__CF07ABDA_EBE6_4084_B777_978FBAC16EA1__INCLUDED_)
