#define PrioirtyBoosterCommon
#ifdef PrioirtyBoosterCommon

#pragma warning(disable: 4047 4013)

#define PRIORITY_BOOSTER_DEVICE 0x8000

#define IOCTL_PRIORITY_BOOSTER_SET_PRIORITY CTL_CODE(PRIORITY_BOOSTER_DEVICE, \
	0x800, METHOD_NEITHER, FILE_ANY_ACCESS)

struct ThreadData
{
	unsigned long ThreadId;
	int Priority;
};
#endif // PrioirtyBoosterCommon