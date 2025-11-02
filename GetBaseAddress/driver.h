#pragma once
#include <ntdef.h>       // Defines BYTE, WORD, ULONG, etc.
#include <ntifs.h>       // Kernel-mode headers (includes ntddk.h + wdm.h)
#include <ntstatus.h>    // Must come *after* ntifs.h

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverExit(PDRIVER_OBJECT DriverObject);
uintptr_t GetRuntimeFuncAddress(const wchar_t* functionName);
uintptr_t GetModuleBase(PEPROCESS eprocess, const wchar_t* moduleName);


typedef struct _PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _PEB {
    BYTE InheritedAddressSpace;
    BYTE ReadImageFileExecOptions;
    BYTE BeingDebugged;
    BYTE Spare;
    PVOID Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
    PVOID ProcessParameters;
} PEB, * PPEB;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    PVOID SectionPointer;
    ULONG CheckSum;
    PVOID LoadedImports;
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;


typedef PPEB(*PsGetProcessPeb_T)(PEPROCESS process); 
PsGetProcessPeb_T PsGetProcessPeb;