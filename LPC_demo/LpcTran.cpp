// LpcTran.cpp: implementation of the LpcTran class.
//
//////////////////////////////////////////////////////////////////////

#include "LpcTran.h"

#ifdef _DEBUG
#include "Debug.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LpcTran::LpcTran()
{

}

LpcTran::~LpcTran()
{

}

BOOL LpcTran::SendWithReplyPort(IN HANDLE PortHandle, 
								IN PTRANSFERRED_MESSAGE ReplyMessage)
{	
	BOOL bRet;


	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("===> LpcTran::SendWithReplyPort()\n"));
#endif
	//---------------------------------------------------------


	bRet =  LpcReplyPort(PortHandle, &ReplyMessage->Header);


	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("<=== LpcTran::SendWithReplyPort()\n"));
#endif
	//---------------------------------------------------------

	return bRet;
}

BOOL LpcTran::SendWithRequestPort(IN HANDLE PortHandle,
								  IN PTRANSFERRED_MESSAGE RequestMessage)
{
	BOOL bRet;


	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("===> LpcTran::SendWithRequestPort()\n"));
#endif
	//---------------------------------------------------------


	bRet = LpcRequestPort(PortHandle, &RequestMessage->Header);


	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("<=== LpcTran::SendWithRequestPort\n"));
#endif
	//---------------------------------------------------------


	return bRet;
}

BOOL LpcTran::SendWithRequestWaitReplyPort(IN HANDLE PortHandle,
										   IN PTRANSFERRED_MESSAGE RequestMessage,
										   OUT PTRANSFERRED_MESSAGE ReplyMessage)
{
	BOOL bRet;


	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("===> LpcTran::SendWithRequestWaitReplyPort()\n"));
#endif
	//---------------------------------------------------------


	bRet = LpcRequestWaitReplyPort(PortHandle,
								   &RequestMessage->Header,
								   &ReplyMessage->Header);


	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("<=== LpcTran::SendWithRequestWaitReplyPort()\n"));
#endif
	//---------------------------------------------------------


	return bRet;
}

BOOL LpcTran::Receive(IN HANDLE PortHandle,
					  OUT PVOID *PortContext OPTIONAL,
					  IN PTRANSFERRED_MESSAGE ReplyMessage OPTIONAL,
					  OUT PTRANSFERRED_MESSAGE ReceiveMessage)
{
	BOOL bRet;

	
	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("===> LpcTran::Receive()\n"));
#endif
	//---------------------------------------------------------



	bRet = LpcReplyWaitReceivePort(PortHandle,
								   PortContext,
								  &ReplyMessage->Header,
								  &ReceiveMessage->Header);


	//---------------------------------------------------------
#ifdef _DEBUG
	ShowMyMsg(_T("<=== LpcTran::Receive()\n"));
#endif
	//---------------------------------------------------------


	return bRet;
}