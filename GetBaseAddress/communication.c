#include "communication.h"
#include "driver.h"

ULONG TEST_PID = 0;
PEPROCESS eproc = NULL;

NTSTATUS IrpCreateDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Device handle opened\n");
    return STATUS_SUCCESS;
}

NTSTATUS IrpCloseDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Device handle closed\n");
    return STATUS_SUCCESS;
}

NTSTATUS IrpCustomDispatcher(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    switch (stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_GET_MODULE_BASE:
    {
        if (stack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(ULONG))
        {
            TEST_PID = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Received PID: %lu\n", TEST_PID);

            status = PsLookupProcessByProcessId((HANDLE)TEST_PID, &eproc);
            if (!NT_SUCCESS(status))
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "PsLookupProcessByProcessId failed: 0x%X\n", status);
                break;
            }

            uintptr_t base = GetModuleBase(eproc, L"UnityPlayer.dll");
            if (base)
            {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Module found @ 0x%llX\n", base);
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GOM found @ 0x%llX\n", base+ 0x1CF93E0);
            }
            else
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Module not found\n");

            ObDereferenceObject(eproc);
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
