#include "driver.h"


HANDLE TEST_PID = (HANDLE)2604;


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = DriverExit;

    PsGetProcessPeb = (PsGetProcessPeb_T)GetRuntimeFuncAddress(L"PsGetProcessPeb");
    if (!PsGetProcessPeb) return STATUS_UNSUCCESSFUL;

    NTSTATUS status;
    PEPROCESS eproc = NULL;
    status = PsLookupProcessByProcessId(TEST_PID, &eproc);
    if (!NT_SUCCESS(status)) return status;

    uintptr_t UnityPlayerBase = GetModuleBase(eproc, L"UnityPlayer.dll");

    ObDereferenceObject(eproc);        
    UNREFERENCED_PARAMETER(UnityPlayerBase);

    return STATUS_SUCCESS;
}


VOID DriverExit(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
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
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                    "FOUND %wZ @ %p (size %lu)\n",
                    &entry->BaseDllName, entry->DllBase, entry->SizeOfImage);

                base = (uintptr_t)entry->DllBase;
                break;
            }
        }
    }


    KeUnstackDetachProcess(&apc);
    return base;
}
