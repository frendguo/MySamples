// ILpcTran.h: interface for the ILpcTran class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ILPCTRAN_H__3E7A6D89_B463_43FB_BE44_0067705E3A6A__INCLUDED_)
#define AFX_ILPCTRAN_H__3E7A6D89_B463_43FB_BE44_0067705E3A6A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "lpcCommon.h"

class ILpcTran  
{
public:
	ILpcTran();
	virtual ~ILpcTran();

public:

	// 发送数据立即返回 NtRequestPort()
	virtual BOOL SendWithRequestPort(IN  HANDLE PortHandle,
									 IN  PTRANSFERRED_MESSAGE RequestMessage) = 0; 
	
	// 发送数据立即返回 NtReplyPort()
	virtual BOOL SendWithReplyPort(IN  HANDLE PortHandle,
								   IN  PTRANSFERRED_MESSAGE ReplyMessage) = 0; 
	
	// 阻塞接受数据 NtReplyWaitReceivePort()
	virtual BOOL Receive(IN  HANDLE PortHandle,
						 OUT PVOID *PortContext OPTIONAL,
						 IN  PTRANSFERRED_MESSAGE ReplyMessage OPTIONAL,
						 OUT PTRANSFERRED_MESSAGE ReceiveMessage) = 0; 
	
	// 发送数据并阻塞等待回应  NtRequestWaitReplyPort()
	virtual BOOL SendWithRequestWaitReplyPort(IN  HANDLE PortHandle,
											  IN  PTRANSFERRED_MESSAGE RequestMessage,
											  OUT PTRANSFERRED_MESSAGE ReplyMessage) = 0;

};

#endif // !defined(AFX_ILPCTRAN_H__3E7A6D89_B463_43FB_BE44_0067705E3A6A__INCLUDED_)
