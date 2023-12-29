#ifndef __LPCCOMMON_H__
#define __LPCCOMMON_H__
#include <windows.h>

#define LARGE_MESSAGE_SIZE 0x400000


#ifdef __cplusplus
extern "C"
{
#endif


#ifndef _NTDDK_

typedef LONG NTSTATUS;


#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#define STATUS_SUCCESS 0


#endif // _NTDDK_

#ifndef _NTDDK_

#define OBJ_INHERIT				0x00000002L
#define OBJ_PERMANENT			0x00000010L
#define OBJ_EXCLUSIVE			0x00000020L
#define OBJ_CASE_INSENSITIVE	0x00000040L
#define OBJ_OPENIF				0x00000080L
#define OBJ_OPENLINK			0x00000100L
#define OBJ_KERNEL_HANDLE		0x00000200L
#define OBJ_VALID_ATTRIBUTES	0x000003F2L

//同步
#define LPCSYN		0
//异步
#define LPCASYN     1 

#define DEF_CODE(Function, Access) (((Function) << 2)|(Access) )
//测试是否同步
#define ISSYNLPC(Function) ( (Function)&LPCASYN?FALSE:TRUE )

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

#define UNICODE_NULL ((WCHAR)0)

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService; 
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

//
// ClientId
//

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

#endif // _NTDDK_

//
// LPC 定义及结构

#define MAX_LPC_DATA 0x130		// Maximum number of bytes that can be copied through LPC

// LPC 连接类型 
typedef enum _LPC_TYPE
{
	LPC_NEW_MESSAGE,		// (0) A new message
	LPC_REQUEST,			// (1) A request message
	LPC_REPLY,				// (2) A reply to a request message
	LPC_DATAGRAM,			// (3)
	LPC_LOST_REPLY,			// (4)
	LPC_PORT_CLOSED,		// (5) Send when port is deleted
	LPC_CLIENT_DIED,		// (6) Messages to thread termination ports
	LPC_EXCEPTION,			// (7) Messages to thread exception ports
	LPC_DEBUG_EVENT,		// (8) Messages to thread debug port
	LPC_ERROR_EVENT,		// (9) Used by NtRaiseHardError
	LPC_CONNECTION_REQUEST, // (A) Used by NtConnectPort
} LPC_TYPE, *PLPC_TYPE;

//
// 为port消息定义头
//

typedef struct _PORT_MESSAGE 
{
	USHORT DataLength;				// Length of data following header (bytes)
	USHORT TotalLength;				// Length of data + sizeof(PORT_MESSAGE)
	USHORT Type;					// Type of the message (LPC_TYPE)
	USHORT VirtualRangesOffset;		// Offset of array of virtual address ranges
	CLIENT_ID ClientId;				// Client identifier of the message sender
	ULONG MessageId;				// Identifier of the particular message instance
	union 
	{
		ULONG CallbackId;			//
		ULONG ClientViewSize;		// Size, in bytes, of section created by the sender
	};
} PORT_MESSAGE, *PPORT_MESSAGE;

typedef struct _TRANSFERRED_MESSAGE
{
    PORT_MESSAGE Header;
	ULONG   Command;
	ULONG  DataLength;
    BYTE   MessageText[224];	
} TRANSFERRED_MESSAGE, *PTRANSFERRED_MESSAGE;
//
// Define structure for initializing shared memory on the caller's side of the port
//

typedef struct _PORT_VIEW {
	ULONG Length;					// Size of this structure
	HANDLE SectionHandle;			// Handle to section object with
									// SECTION_MAP_WRITE and SECTION_MAP_READ
	ULONG SectionOffset;			// The offset in the section to map a view for
									// the port data area. the offset must be aligned
									// with the allocation granularity of the system.
	ULONG ViewSize;					// The size of the view (in bytes)
	PVOID ViewBase;					// The base address of the view in the creator
									//
	PVOID ViewRemoteBase;			// The Base address of the view in the process 
									// connected to the port.
} PORT_VIEW, *PPORT_VIEW;

// 
// Define 
// 
typedef struct _REMOTE_PORT_VIEW {
	
	ULONG Length;							// Size of this structure
	ULONG ViewSize;							// The size of the view (bytes)
	PVOID ViewBase;							// Base address of the view
} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW;

#ifndef RtlInitUnicodeString
#define RtlInitUnicodeString(Dest, Str)									\
	(Dest)->Buffer			= (PWSTR) (Str);							\
	(Dest)->Length			= (USHORT) (wcslen(Str) * sizeof(WCHAR));	\
	(Dest)->MaximumLength	= (USHORT) ((Dest)->Length + 1)
#endif

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes(p, n, a, r, s) {		\
	(p)->Length = sizeof(OBJECT_ATTRIBUTES);			\
	(p)->RootDirectory = r;								\
	(p)->Attributes = a;								\
	(p)->ObjectName = n;								\
	(p)->SecurityDescriptor = s;						\
	(p)->SecurityQualityOfService = NULL;				\
}
#endif

#ifndef InitializePortHeader
#define InitializeMessageHeader(ph, l, t) {						\
	(ph)->TotalLength	= (USHORT) (l);							\
	(ph)->DataLength	= (USHORT) (l - sizeof(PORT_MESSAGE));	\
	(ph)->Type			= (USHORT) (t);							\
	(ph)->VirtualRangesOffset = 0;								\
}

#define InitializeMessageData(msag, view, command, dataaddr, datalen) {		\
	InitializeMessageHeader(&((msag)->Header), 256, LPC_NEW_MESSAGE)		\
	(msag)->Command		= command;											\
	(msag)->DataLength  = datalen;											\
	if ((datalen) < 224)													\
	{																		\
	CopyMemory((PVOID)(msag)->MessageText,									\
			   (PVOID)(dataaddr),											\
			   (ULONG)(datalen));											\
	}																		\
	else																	\
	{																		\
	CopyMemory((PVOID)(view)->ViewBase,										\
			   (PVOID)(dataaddr),											\
			   (ULONG)(datalen));											\
	}																		\
}
#endif

//------------------------------------------------------------------------------
// LPC Functions

NTSTATUS
NTAPI
NtCreatePort(
			 OUT PHANDLE PortHandle,                     
			 IN  POBJECT_ATTRIBUTES ObjectAttributes,
			 IN  ULONG MaxConnectionInfoLength,
			 IN  ULONG MaxMessageLength,
			 IN  ULONG MaxPoolUsage
			 );


NTSTATUS
NTAPI
NtConnectPort(
			  OUT PHANDLE PortHandle,
			  IN  PUNICODE_STRING PortName,
			  IN  PSECURITY_QUALITY_OF_SERVICE SecurityQos,
			  IN  OUT PPORT_VIEW ClientView OPTIONAL,
			  OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
			  OUT PULONG MaxMessageLength OPTIONAL,
			  IN  OUT PVOID ConnectionInformation OPTIONAL,
			  IN  OUT PULONG ConnectionInformationLength OPTIONAL
			  );


NTSTATUS
NTAPI
ZwConnectPort(
			  OUT PHANDLE PortHandle,
			  IN  PUNICODE_STRING PortName,
			  IN  PSECURITY_QUALITY_OF_SERVICE SecurityQos,
			  IN  OUT PPORT_VIEW ClientView OPTIONAL,
			  OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
			  OUT PULONG MaxMessageLength OPTIONAL,
			  IN  OUT PVOID ConnectionInformation OPTIONAL,
			  IN  OUT PULONG ConnectionInformationLength OPTIONAL
    );


NTSTATUS
NTAPI
NtListenPort(
			 IN  HANDLE PortHandle,
			 OUT PPORT_MESSAGE RequestMessage
    );


NTSTATUS
NTAPI
NtAcceptConnectPort(
					OUT PHANDLE PortHandle,
					IN  PVOID PortContext OPTIONAL,
					IN  PPORT_MESSAGE ConnectionRequest,
					IN  BOOLEAN AcceptConnection,
					IN  OUT PPORT_VIEW ServerView OPTIONAL,
					OUT PREMOTE_PORT_VIEW ClientView OPTIONAL
    );


NTSTATUS
NTAPI
NtCompleteConnectPort(
					  IN  HANDLE PortHandle
					  );


NTSTATUS
NTAPI
ZwCompleteConnectPort(
					  IN  HANDLE PortHandle
    );


NTSTATUS
NTAPI
NtRequestPort (
			   IN  HANDLE PortHandle,
			   IN  PPORT_MESSAGE RequestMessage
    );


NTSTATUS
NTAPI
NtRequestWaitReplyPort(
					   IN  HANDLE PortHandle,
					   IN  PPORT_MESSAGE RequestMessage,
					   OUT PPORT_MESSAGE ReplyMessage
    );


NTSTATUS
NTAPI
NtReplyPort(
			IN  HANDLE PortHandle,
			IN  PPORT_MESSAGE ReplyMessage
    );

NTSTATUS
NTAPI
NtReplyWaitReplyPort(
					 IN  HANDLE PortHandle,
					 IN  OUT PPORT_MESSAGE ReplyMessage
					 );


NTSTATUS
NTAPI
NtReplyWaitReceivePort(
					   IN  HANDLE PortHandle,
					   OUT PVOID *PortContext OPTIONAL,
					   IN  PPORT_MESSAGE ReplyMessage OPTIONAL,
					   OUT PPORT_MESSAGE ReceiveMessage
    );


NTSTATUS
NTAPI
NtClose(
		IN  HANDLE Handle
		);

NTSTATUS
NTAPI
ZwClose(
		IN  HANDLE Handle
    );



NTSTATUS
NTAPI
NtCreateSection(
				OUT PHANDLE SectionHandle,
				IN  ACCESS_MASK DesiredAccess,
				IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
				IN  PLARGE_INTEGER MaximumSize OPTIONAL,
				IN  ULONG SectionPageProtection,
				IN  ULONG AllocationAttributes,
				IN  HANDLE FileHandle OPTIONAL
				);


NTSTATUS
NTAPI
ZwCreateSection(
				OUT PHANDLE SectionHandle,
				IN  ACCESS_MASK DesiredAccess,
				IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
				IN  PLARGE_INTEGER MaximumSize OPTIONAL,
				IN  ULONG SectionPageProtection,
				IN  ULONG AllocationAttributes,
				IN  HANDLE FileHandle OPTIONAL
				);



#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LPC_H__
