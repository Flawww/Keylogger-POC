#ifndef MAPPER_H
#define MAPPER_H

#include "pch.h"

typedef NTSTATUS( __stdcall* QueryInformation_t )( HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG );
typedef NTSTATUS( __stdcall* LdrLoadDll_t )( PWCHAR, PULONG, PUNICODE_STRING, PHANDLE );
typedef int( __stdcall* EntryPoint_t )( PVOID, uint32_t, PVOID );

typedef struct _MMAP_DATA {
    PIMAGE_DOS_HEADER   DosHeader;
    PIMAGE_NT_HEADERS   NtHeader;
    IMAGE_FILE_HEADER   FileHeader;
    PVOID               EntryPoint;
    PVOID               ImageBuffer;
    PVOID               MapAddress;
} MMAP_DATA, *PMMAP_DATA;

//
// Gets the PID of process from process (exe file) name
// @param PCHAR Name name of the .exe file of process
// @return PID of the process, if process were not found it returns 0
//
UINT32
MapperGetPid(
    IN PCHAR Name
);

//
// Get the X64 Peb struct for the given process
// @param HANDLE Process handle to the process to read PEB from
// @return PEB struct of the process
//
PEB
MapperGetProcessPeb(
    IN HANDLE Process
);

//
// Get the address of an export in a remote module
// @param HANDLE Process handle to the process that module resides in
// @param PVOID Module base address of the module that we are gonna read the exports from
// @param PCHAR Name name of the export we are looking for
// @return the address of the exported function, if not found it returns NULL
//
PVOID
MapperGetExportOffset(
    IN HANDLE Process,
    IN PVOID Module,
    IN PCHAR Name
);

//
// Returns the base address of the wanted X64 module mapped in a process
// @param HANDLE Process the handle to the process we want to get module from
// @param PEB Peb the target process PEB struct
// @param PWCHAR Name name of the module we want to find
// @return Returns the base address of the wanted module, if not found it returns NULL
//
PVOID
MapperGetX64ModuleHandle(
    IN HANDLE Process,
    IN PEB Peb,
    IN PWCHAR Name
);

//
// Gives back ldr data for a specific module in the remote process
// @param HANDLE Process 
// @param PEB Peb - remote process peb
// @param PWCHAR Name name of module we are looking for
// @param OUT PPEB_LDR_DATA Data the ldr data returned to the caller
// @return BOOL returns TRUE if successfull
//
BOOL
MapperGetModuleInformation(
    IN HANDLE Process,
    IN PEB Peb,
    IN PWCHAR Name,
    OUT PLDR_DATA_TABLE_ENTRY Data
);

//
// Loads a dll into remote process by creating a thread that executes LoadLibraryA
// HANDLE Process handle to target process
// PCHAR DllName full path to the dll to load
//
BOOL
MapperLoadLibrary(
    IN HANDLE Process,
    IN PCHAR DllName
);

//
// Enables debug privileges of the current process to be able to connect to higher privileged processes
// --REQUIRES ELEVATION--
// @return returns true if sucess, else false
//
BOOL
MapperEnableDebugPrivilege(
    VOID
);

//
// Calculates the absolute address of a rip-relative operand
// HANDLE Process handle to process
// uintptr_t Address address of the start of the instruction
// uint32_t Offset offset from start of instruction to relative part
// uint32_t InstructionSize full size of the instruction in question
//
uintptr_t
MapperGetAbsAddress(
    IN HANDLE Process,
    IN uintptr_t Address,
    IN uint32_t Offset,
    IN uint32_t InstructionSize
);

#endif