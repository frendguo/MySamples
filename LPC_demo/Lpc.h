// Lpc.h: interface for the CLpc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LPC_H__7747A08A_11FE_4880_91C5_7521C7080F96__INCLUDED_)
#define AFX_LPC_H__7747A08A_11FE_4880_91C5_7521C7080F96__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "lpcCommon.h"


class CLpc  
{
public:
	CLpc();
	virtual ~CLpc();

protected:
	// LPCAPI封装
	// NtCreatePort()
    BOOL LpcCreatePort(OUT  PHANDLE PortHandle,
					   IN POBJECT_ATTRIBUTES ObjectAttributes,
					   IN ULONG MaxConnectionInfoLength,
					   IN ULONG MaxMessageLength,
					   IN ULONG MaxPoolUsage);

	// NtConnectPort()
    BOOL LpcConnectPort(OUT PHANDLE pPortHandle,
						IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
						IN PUNICODE_STRING PortName,
						IN OUT PPORT_VIEW ClientView OPTIONAL,
						OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
						OUT PULONG MaxMessageLength OPTIONAL,	
						IN OUT PVOID ConnectionInformation OPTIONAL,
						IN OUT PULONG ConnectionInformationLength OPTIONAL);

	// NtListenPort()
    BOOL LpcListenPort(IN HANDLE PortHandle,
					   OUT PPORT_MESSAGE RequestMessage);

	// NtAcceptConnectPort()
    BOOL LpcAcceptConnectPort(OUT PHANDLE pPortHandle,
							  IN PVOID PortContext OPTIONAL,
                              IN PPORT_MESSAGE ConnectionRequest,
							  IN BOOLEAN AcceptConnection,
							  IN OUT PPORT_VIEW ServerView OPTIONAL,
							  OUT PREMOTE_PORT_VIEW ClientView OPTIONAL);
	
	// NtCompleteConnectPort()
    BOOL LpcCompleteConnectPort(IN HANDLE PortHandle);

	// NtRequestPort()
    BOOL LpcRequestPort(IN HANDLE PortHandle, IN PPORT_MESSAGE RequestMessage);

	// NtRequestWaitReplyPort()
    BOOL LpcRequestWaitReplyPort(IN HANDLE PortHandle,
								 IN PPORT_MESSAGE RequestMessage,
								 OUT PPORT_MESSAGE ReplyMessage);

	// NtReplyPort()
    BOOL LpcReplyPort(IN HANDLE PortHandle,
					  IN PPORT_MESSAGE ReplyMessage);

	// NtReplyWaitReplyPort()
    BOOL LpcReplyWaitReplyPort(IN HANDLE PortHandle,
							   IN OUT PPORT_MESSAGE ReplyMessage);
    
	// NtReplyWaitReceivePort()
	BOOL LpcReplyWaitReceivePort(IN HANDLE PortHandle,
								 OUT PVOID *PortContext OPTIONAL,
								 IN PPORT_MESSAGE ReplyMessage OPTIONAL,
								 OUT PPORT_MESSAGE ReceiveMessage);


	// 封装NtCreateSection()
	BOOL LpcCreateSection(OUT PHANDLE SectionHandle,
						  IN  ACCESS_MASK DesiredAccess,
						  IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
						  IN  PLARGE_INTEGER MaximumSize OPTIONAL,
						  IN  ULONG SectionPageProtection,
						  IN  ULONG AllocationAttributes,
						  IN  HANDLE FileHandle OPTIONAL);

protected:
	BOOL m_bIsOK;
};

#endif // !defined(AFX_LPC_H__7747A08A_11FE_4880_91C5_7521C7080F96__INCLUDED_)
