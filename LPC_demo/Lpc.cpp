// Lpc.cpp: implementation of the CLpc class.
//
//////////////////////////////////////////////////////////////////////

#include "Lpc.h"

#include "Debug.h"

void ShowSysError(NTSTATUS Status);


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLpc::CLpc()
{
	m_bIsOK = TRUE;
}

CLpc::~CLpc()
{
}

BOOL CLpc::LpcCreatePort(OUT PHANDLE PortHandle,
						 IN POBJECT_ATTRIBUTES ObjectAttributes, 
						 IN ULONG MaxConnectionInfoLength,
						 IN ULONG MaxMessageLength,
						 IN ULONG MaxPoolUsage)
{
	NTSTATUS Status = -1;
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	ShowMyMsg(_T("===> CLpc::LpcCreatePort()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtCreatePort(PortHandle,
			ObjectAttributes,
			MaxConnectionInfoLength,
			MaxMessageLength,
			MaxPoolUsage);
		
		if (!NT_SUCCESS(Status))
		{
			m_bIsOK = FALSE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcCreatePort()\n"));
#endif
	//---------------------------------------------------------
	
	
	return m_bIsOK;
}

BOOL CLpc::LpcListenPort(IN HANDLE PortHandle, OUT PPORT_MESSAGE RequestMessage)
{
	NTSTATUS Status = -1;
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	ShowMyMsg(_T("===> CLpc::LpcListenPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		
		Status = NtListenPort(PortHandle, RequestMessage);
		
		if (!NT_SUCCESS(Status))
		{
			m_bIsOK = FALSE;
		}
		else
		{
			m_bIsOK = TRUE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	if (!NTSTATUS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcListenPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	return m_bIsOK;
}

BOOL CLpc::LpcAcceptConnectPort(OUT PHANDLE pPortHandle,
								IN PVOID PortContext OPTIONAL, 
								IN PPORT_MESSAGE ConnectionRequest, 
								IN BOOLEAN AcceptConnection, 
								IN OUT PPORT_VIEW ServerView OPTIONAL,
								OUT PREMOTE_PORT_VIEW ClientView OPTIONAL)
{
	NTSTATUS Status = -1;
	m_bIsOK = TRUE;
	
	
#ifdef ENDEBUG
	ShowMyMsg(_T("===> CLpc::LpcAcceptConnectPort()\n"));
#endif
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtAcceptConnectPort(pPortHandle,
			PortContext,
			ConnectionRequest,
			AcceptConnection,
			ServerView,
			ClientView);
		if (!NT_SUCCESS(Status))
		{
			m_bIsOK = FALSE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcAcceptConnectPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	return m_bIsOK;
}

BOOL CLpc::LpcCompleteConnectPort(IN HANDLE PortHandle)
{
	NTSTATUS Status = -1;
	m_bIsOK = TRUE;
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	ShowMyMsg(_T("===> CLpc::LpcCompleteConnectPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtCompleteConnectPort(PortHandle);
		
		if (!NT_SUCCESS(Status))
		{
			m_bIsOK = FALSE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<===  CLpc::LpcCompleteConnectPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	
	return m_bIsOK;
}



BOOL CLpc::LpcConnectPort(OUT PHANDLE pPortHandle,
						  IN PSECURITY_QUALITY_OF_SERVICE SecurityQos, 
						  IN PUNICODE_STRING PortName,
						  IN OUT PPORT_VIEW ClientView OPTIONAL, 
						  OUT PREMOTE_PORT_VIEW ServerView OPTIONAL, 
						  OUT PULONG MaxMessageLength OPTIONAL, 
						  IN OUT PVOID ConnectionInformation OPTIONAL,
						  IN OUT PULONG ConnectionInformationLength OPTIONAL)
{
	NTSTATUS Status = -1;
	BOOL bRet = TRUE;
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	ShowMyMsg(_T("===> CLpc::LpcConnectPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtConnectPort(pPortHandle,
			PortName,
			SecurityQos,
			ClientView,
			ServerView,
			MaxMessageLength,
			ConnectionInformation,
			ConnectionInformationLength);
		
		if (!NT_SUCCESS(Status))
		{
// 			m_bIsOK = FALSE;
			bRet = FALSE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcConnectPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	return bRet;
}

BOOL CLpc::LpcRequestPort(IN HANDLE PortHandle, 
						  IN PPORT_MESSAGE RequestMessage)
{
	NTSTATUS Status = -1;
	
	
	//---------------------------------------------------------
#ifdef ENDEBUGCOM
	ShowMyMsg(_T("===> CLpc::LpcRequestPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtRequestPort(PortHandle, RequestMessage);
		
		if (!NT_SUCCESS(Status))
		{
			m_bIsOK = FALSE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUGCOM
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcRequestPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	return m_bIsOK;
}

BOOL CLpc::LpcReplyWaitReceivePort(IN HANDLE PortHandle,
								   OUT PVOID *PortContext OPTIONAL,
								   IN PPORT_MESSAGE ReplyMessage OPTIONAL, 
								   OUT PPORT_MESSAGE ReceiveMessage)
{
	NTSTATUS Status = -1;
	BOOL bRet = TRUE;
	
	
	//---------------------------------------------------------
#ifdef ENDEBUGCOM
	ShowMyMsg(_T("===> CLpc::LpcReplyWaitReceivePort()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtReplyWaitReceivePort(PortHandle, PortContext, ReplyMessage, 
			ReceiveMessage);
		
		if (!NT_SUCCESS(Status))
		{
			bRet = FALSE;
		}
		
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
//	ShowMyMsg(_T("<=== CLpc::LpcReplyWaitReceivePort()\n"));
#endif
	//---------------------------------------------------------
	
	
	return bRet;
}

BOOL CLpc::LpcReplyPort(IN HANDLE PortHandle, IN PPORT_MESSAGE ReplyMessage)
{
	NTSTATUS Status = -1;
	BOOL bRet = TRUE;
	
	
	//---------------------------------------------------------
#ifdef ENDEBUGCOM
	ShowMyMsg(_T("===> CLpc::LpcReplyPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtReplyPort(PortHandle, ReplyMessage);
		
		if (!NT_SUCCESS(Status))
		{
			bRet = FALSE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUGCOM
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcReplyPort()\n"));
#endif
	//---------------------------------------------------------
	
	
	return bRet;
}

BOOL CLpc::LpcCreateSection(OUT PHANDLE SectionHandle,
							IN ACCESS_MASK DesiredAccess,
							IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
							IN PLARGE_INTEGER MaximumSize OPTIONAL,
							IN ULONG SectionPageProtection,
							IN ULONG AllocationAttributes,
							IN HANDLE FileHandle OPTIONAL)
{
	NTSTATUS Status = -1;
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	ShowMyMsg(_T("===> CLpc::LpcCreateSection()\n"));
#endif
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtCreateSection(SectionHandle,
			DesiredAccess,
			ObjectAttributes,
			MaximumSize,
			SectionPageProtection,
			AllocationAttributes,
			FileHandle);
		
		if (!NT_SUCCESS(Status))
		{
			m_bIsOK = FALSE;
		}
	}
	//---------------------------------------------------------
	
	
	//---------------------------------------------------------
#ifdef ENDEBUG
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcCreateSection()\n"));
#endif
	//---------------------------------------------------------
	
	
	return m_bIsOK;
}

BOOL CLpc::LpcRequestWaitReplyPort(IN HANDLE PortHandle,
								   IN PPORT_MESSAGE RequestMessage, 
								   OUT PPORT_MESSAGE ReplyMessage)
{
	NTSTATUS Status = -1;
	BOOL bRet = TRUE;


	//---------------------------------------------------------
#ifdef ENDEBUGCOM
	ShowMyMsg(_T("===> CLpc::LpcRequestWaitReplyPort()\n"));
#endif
	//---------------------------------------------------------


	//---------------------------------------------------------
	if (m_bIsOK)
	{
		Status = NtRequestWaitReplyPort(PortHandle,
										RequestMessage,
										ReplyMessage);

		if (!NT_SUCCESS(Status))
		{
			bRet = FALSE;
		}
	}
	//---------------------------------------------------------



	//---------------------------------------------------------
#ifdef ENDEBUGCOM
	if (!NT_SUCCESS(Status))
	{
		ShowSysError(Status);
	}
	ShowMyMsg(_T("<=== CLpc::LpcRequestWaitReplyPort()\n"));
#endif

	return bRet;
}