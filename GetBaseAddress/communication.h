#pragma once
#include <ntifs.h>


NTSTATUS IrpCreateDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS IrpCloseDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS IrpCustomDispatcher(PDEVICE_OBJECT DeviceObject, PIRP Irp);

#define IOCTL_GET_MODULE_BASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x630, METHOD_BUFFERED, FILE_READ_ACCESS)

extern ULONG TEST_PID;
