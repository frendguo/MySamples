#pragma once

#pragma warning(disable: 4047 4013)

#define RESUME_PROCESS_DEVICE 0x8000

// DeviceType: һ��������һ������WDKͷ�ļ��ж���� FILE_DEVICE_xxx ������������ֵ����� 0x8000 ��ʼ
// Function��һ���������ӵ����֡���������У���ͬ�Ŀ��ƴ�����벻ͬ��������������� 0x800 ��ʼ
// Method����ʶ����/������������������ݸ������ġ����� METHOD_BUFFERED��METHOD_IN_DIRECT��METHOD_OUT_DIRECT��METHOD_NEITHER
// Access��ָ����������ǵ�����������FILE_WRITE_ACCESS����������������FILE_READ_ACCESS�������Ƕ��߼��У�FILE_ANY_ACCESS��
#define IOCTL_UNINSTALL_RESUME_PROCESS CTL_CODE(RESUME_PROCESS_DEVICE, \
	0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
