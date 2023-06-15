#pragma once

#pragma warning(disable: 4047 4013)

#define RESUME_PROCESS_DEVICE 0x8000

// DeviceType: 一个常数。一般来自WDK头文件中定义的 FILE_DEVICE_xxx 常数。三方的值必须从 0x8000 开始
// Function：一个往上增加的数字。这个驱动中，不同的控制代码必须不同。三方驱动必须从 0x800 开始
// Method：标识输入/输出缓冲区是怎样传递给驱动的。包含 METHOD_BUFFERED、METHOD_IN_DIRECT、METHOD_OUT_DIRECT、METHOD_NEITHER
// Access：指明这个操作是到达驱动程序（FILE_WRITE_ACCESS），来自驱动程序（FILE_READ_ACCESS），还是二者兼有（FILE_ANY_ACCESS）
#define IOCTL_UNINSTALL_RESUME_PROCESS CTL_CODE(RESUME_PROCESS_DEVICE, \
	0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
