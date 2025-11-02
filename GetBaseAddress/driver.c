#include "driver.h"
#include "communication.h"


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\GetBaseDriver");
    UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(L"\\??\\GetBaseDriverLink");

    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status = IoCreateDevice(
        DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &deviceObject);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateDevice failed: 0x%X\n", status);
        return status;
    }

    status = IoCreateSymbolicLink(&SymbolicLink, &DeviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IoCreateSymbolicLink failed: 0x%X\n", status);
        IoDeleteDevice(deviceObject);
        return status;
    }

    PsGetProcessPeb = (PsGetProcessPeb_T)GetRuntimeFuncAddress(L"PsGetProcessPeb");
    if (!PsGetProcessPeb) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
            "MmGetSystemRoutineAddress(\"PsGetProcessPeb\") failed\n");
        IoDeleteSymbolicLink(&SymbolicLink);
        IoDeleteDevice(deviceObject);
        return STATUS_PROCEDURE_NOT_FOUND;
    }


    DriverObject->DriverUnload = DriverExit;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpCreateDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpCloseDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpCustomDispatcher;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    deviceObject->Flags |= DO_BUFFERED_IO;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Driver loaded successfully\n");
    return STATUS_SUCCESS;
}
    

VOID DriverExit(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(L"\\??\\GetBaseDriverLink");

    // Delete symbolic link first
    IoDeleteSymbolicLink(&SymbolicLink);

    // Then delete the device object
    if (DriverObject->DeviceObject)
        IoDeleteDevice(DriverObject->DeviceObject);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "BYE\n");
}


uintptr_t GetRuntimeFuncAddress(const wchar_t* functionName)
{
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, functionName);
	uintptr_t functionPointer = (uintptr_t)MmGetSystemRoutineAddress(&name);

	if (!functionPointer)
		return 0;

	return functionPointer;
}

uintptr_t GetModuleBase(PEPROCESS eprocess, const wchar_t* moduleName)
{
    KAPC_STATE apc;
    KeStackAttachProcess(eprocess, &apc);

    uintptr_t base = 0;
    PPEB peb = PsGetProcessPeb(eprocess);
    if (!peb || !peb->Ldr) {
        KeUnstackDetachProcess(&apc);
        return 0;
    }

    PPEB_LDR_DATA ldr = peb->Ldr;
    UNICODE_STRING target;
    RtlInitUnicodeString(&target, moduleName);

    for (PLIST_ENTRY list = ldr->InLoadOrderModuleList.Flink;
        list != &ldr->InLoadOrderModuleList;
        list = list->Flink)
    {
        PLDR_DATA_TABLE_ENTRY entry =
            CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (entry->BaseDllName.Length && entry->BaseDllName.Buffer)
        {
            if (RtlEqualUnicodeString(&entry->BaseDllName, &target, TRUE))
            {
                base = (uintptr_t)entry->DllBase;
                break;
            }
        }
    }

    KeUnstackDetachProcess(&apc);
    return base;
}