#pragma once

#define PRIORITY_BOOSTER_DEVICE 0x8000

#define IOCTL_ZERO_GET_STATE CTL_CODE(PRIORITY_BOOSTER_DEVICE, \
	0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct ZeroStates
{
	long long TotalRead;
	long long TotalWrite;
};