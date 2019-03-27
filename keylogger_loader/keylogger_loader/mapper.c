#include "pch.h"
#include "mapper.h"
#include "function_wrappers.h"

UINT32
MapperGetPid(
    IN PCHAR Name
)
{
    UINT32  ProcessIds[ 1024 ];
    UINT32  SizeReturned;
    UINT32  ProcessCount;
    CHAR    ProcessName[ MAX_PATH ];
    HMODULE Module;
    HANDLE  Process;
    UINT32  Pid = 0;

    //
    // Get all running processes on the system and calculate how many processes there is with the size it returns.
    //
    EnumProcesses( ProcessIds, sizeof( ProcessIds ), &SizeReturned );
    ProcessCount = SizeReturned / sizeof( UINT32 );

    //
    // Loop through all of the processes as long as we haven't found the one with the correct name
    // If process is not found Pid returns 0
    //
    for ( UINT i = 0; i < ProcessCount && !Pid; i++ )
    {
        if ( !ProcessIds[ i ] )
            continue;

        Process = WrapOpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessIds[ i ] );

        memset( ProcessName, 0, MAX_PATH );

        //
        // Get first module and get the name of it, first module of a process will always be the exe itself.
        //
        EnumProcessModules( Process, &Module, sizeof( Module ), &SizeReturned );
        GetModuleBaseNameA( Process, Module, ProcessName, MAX_PATH );

        //
        // If name matches up, set the pid so it stops looping and returns the pid.
        //
        if ( !( _strcmpi( Name, ProcessName ) ) )
            Pid = ProcessIds[ i ];

        CloseHandle( Process );
    }

    return Pid;
}

PEB
MapperGetProcessPeb(
    IN HANDLE Process
)
{
    QueryInformation_t          QueryInformation;
    PROCESS_BASIC_INFORMATION   Info;
    UINT32                      ReadBytes;
    PEB                         Peb;

    //
    // Get the QueryInformationProcess export from ntdll so we can query the process information which contains the address of Peb
    //
    QueryInformation = ( QueryInformation_t )GetProcAddress( GetModuleHandleA( "ntdll.dll" ), "NtQueryInformationProcess" );

    QueryInformation( Process, ProcessBasicInformation, &Info, sizeof( PROCESS_BASIC_INFORMATION ), &ReadBytes );

    //
    // The PROCESS_BASIC_INFORMATION struct only contains the actual address of PEB, read the memory at that address to get the actual PEB
    //
    WrapReadProcessMemory( Process, Info.PebBaseAddress, &Peb, sizeof( PEB ), NULL );

    return Peb;
}


PVOID
MapperGetExportOffset(
    IN HANDLE Process,
    IN PVOID Module,
    IN PCHAR Name
)
{
    IMAGE_DOS_HEADER        DosHeader;
    IMAGE_NT_HEADERS        NtHeader;
    IMAGE_EXPORT_DIRECTORY  ExportDirectory;
    UINT32                  NumberOfFunctions;
    PUINT32                 FunctionAddresses;
    PUINT32                 FunctionNames;
    PUSHORT                 FunctionOrdinals;
    CHAR                    CurrentFunctionName[ 4096 ]; // 4096 is the maximum length an export can have
    PVOID                   FunctionAddress = NULL;

    //
    // Read module base address to get the dos headers
    // Then use the DosHeader->e_flanew + baseaddr to get address of NT headers and read those too
    //
    WrapReadProcessMemory( Process, ( PBYTE )Module, &DosHeader, sizeof( IMAGE_DOS_HEADER ), NULL );
    WrapReadProcessMemory( Process, ( PBYTE )Module + DosHeader.e_lfanew, &NtHeader, sizeof( IMAGE_NT_HEADERS ), NULL );

    if ( DosHeader.e_magic != IMAGE_DOS_SIGNATURE || NtHeader.Signature != IMAGE_NT_SIGNATURE )
    {
        //printf( "Invalid PE binary" );
        return NULL;
    }

    //
    // Get the export directory to read all function addresses, names and ordinals
    // So we then can iterate through these and find the export we are looking for
    //
    WrapReadProcessMemory( Process, ( PBYTE )Module + NtHeader.OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress, &ExportDirectory, sizeof( IMAGE_EXPORT_DIRECTORY ), NULL );

    //
    // Read the number of functions and allocate the space that we need to store all of the exports
    // And then RPM to get the addresses, names and ordinals
    //
    NumberOfFunctions = ExportDirectory.NumberOfFunctions;
    FunctionAddresses = malloc( NumberOfFunctions * sizeof( UINT32 ) );
    FunctionNames = malloc( NumberOfFunctions * sizeof( UINT32 ) );
    FunctionOrdinals = malloc( NumberOfFunctions * sizeof( USHORT ) );

    if ( !FunctionAddresses || !FunctionNames || !FunctionOrdinals )
    {
        //printf( "Couldn't allocate memory to read exports\n" );
        return NULL;
    }

    WrapReadProcessMemory( Process, ( PBYTE )Module + ExportDirectory.AddressOfFunctions, FunctionAddresses, NumberOfFunctions * sizeof( UINT32 ), NULL );
    WrapReadProcessMemory( Process, ( PBYTE )Module + ExportDirectory.AddressOfNames, FunctionNames, NumberOfFunctions * sizeof( UINT32 ), NULL );
    WrapReadProcessMemory( Process, ( PBYTE )Module + ExportDirectory.AddressOfNameOrdinals, FunctionOrdinals, NumberOfFunctions * sizeof( USHORT ), NULL );
    
    for ( UINT32 i = 0; i < NumberOfFunctions; i++ )
    {
        memset( CurrentFunctionName, 0, sizeof( CurrentFunctionName ) );

        //
        // Read name of the current function and then compare it with the name we got, if they are the same calculate the address the function
        // is at and break out of the loop
        //
        WrapReadProcessMemory( Process, ( PBYTE )Module + FunctionNames[ i ], CurrentFunctionName, sizeof( CurrentFunctionName ), NULL );

        if ( !_strcmpi( Name, CurrentFunctionName ) )
        {
            FunctionAddress = FunctionAddresses[ FunctionOrdinals[ i ] ];
            break;
        }
    }

    //
    // Free up the space we allocated
    //
    free( FunctionAddresses );
    free( FunctionNames );
    free( FunctionOrdinals );

    return FunctionAddress;
}

PVOID
MapperGetX64ModuleHandle(
    IN HANDLE Process,
    IN PEB Peb,
    IN PWCHAR Name
    )
{
    LDR_DATA_TABLE_ENTRY TableEntry;

    if ( MapperGetModuleInformation( Process, Peb, Name, &TableEntry ) )
        return TableEntry.DllBase;

    return NULL;
}

BOOL
MapperGetModuleInformation(
    IN HANDLE Process,
    IN PEB Peb,
    IN PWCHAR Name,
    OUT PLDR_DATA_TABLE_ENTRY Data
)
{
    PEB_LDR_DATA            Ldr;
    WCHAR                   DllName[ MAX_PATH ];
    PVOID                   FirstEntryAddr;
    PVOID                   Module = NULL;

    //
    // Read the address of the PEB_LDR from Peb, which then contains the address to the table entires linked list
    // Also save the "blink" value, which will be the address that the first entry is stored at for knowing when to stop iterating.
    //
    WrapReadProcessMemory( Process, Peb.Ldr, &Ldr, sizeof( PEB_LDR_DATA ), NULL );
    WrapReadProcessMemory( Process, Ldr.InLoadOrderModuleList.Flink, Data, sizeof( LDR_DATA_TABLE_ENTRY ), NULL );
    FirstEntryAddr = Data->InLoadOrderLinks.Blink;

    while ( TRUE )
    {
        memset( DllName, 0, MAX_PATH * sizeof( WCHAR ) );

        //
        // Read the name of the dll from PEB ( WCHAR ), if name is the one we are looking for break out of the loop
        //
        WrapReadProcessMemory( Process, Data->BaseDllName.Buffer, DllName, Data->BaseDllName.Length * sizeof( WCHAR ), NULL );

        wprintf( "%ws\n", DllName );

        if ( !lstrcmpiW( Name, DllName ) )
            return TRUE;

        //
        // Check the next entry of the LIST_ENTRY to see if it is the address of the first entry
        // If it is, we know we reached the end and it's time to break out of the loop
        // If it is NOT, we read the memory of the next entry of the LIST_ENTRY
        //
        if ( Data->InLoadOrderLinks.Flink == FirstEntryAddr )
            break;

        WrapReadProcessMemory( Process, Data->InLoadOrderLinks.Flink, Data, sizeof( LDR_DATA_TABLE_ENTRY ), NULL );
    }


    return FALSE;
}
BOOL
MapperLoadLibrary(
    IN HANDLE Process,
    IN PCHAR DllName
)
{
    PVOID RemoteBuffer;
    PVOID KernelBase;
    PVOID LoadLibraryExport;
    PEB Peb;

    Peb = MapperGetProcessPeb( Process );

    RemoteBuffer = VirtualAllocEx( Process, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    if ( !RemoteBuffer )
        return FALSE;

    // get the exported "LoadLibraryA" function from KernelBase.dll
    //KernelBase = MapperGetX64ModuleHandle( Process, Peb, L"KernelBase.dll" );
    //LoadLibraryExport = ( PBYTE )KernelBase + ( UINT_PTR )MapperGetExportOffset( Process, KernelBase, "LoadLibraryA" );
    LoadLibraryExport = LoadLibraryA;

    // write the dll path into remote process and create a thread at LoadLibraryA, with the string as paramater.
    WrapWriteProcessMemory( Process, RemoteBuffer, DllName, MAX_PATH, NULL );
    WrapCreateRemoteThreadEx( Process, NULL, 0, ( LPTHREAD_START_ROUTINE )LoadLibraryExport, RemoteBuffer, 0, NULL, NULL );

    // let it initialize
    Sleep( 1000 );

    // free the memory we allocated.
    VirtualFreeEx( Process, RemoteBuffer, 0, MEM_RELEASE );
}


//BOOL
//MapperEnableDebugPrivilege(
//    VOID
//)
//{
//    HANDLE              Token;
//    LUID                Luid;
//    TOKEN_PRIVILEGES    Privileges;
//    BOOL                Ret = FALSE;
//
//    //
//    // Open the current process privilege token with ADJUST privilieges
//    //
//    WrapOpenProcessToken( GetCurrentProcess( ), TOKEN_ADJUST_PRIVILEGES, &Token );
//
//    //
//    // Now create the Privileges object so we can pass it to the AdjustTokenPrivileges function to actually change the privileges
//    //
//    LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &Luid );
//    Privileges.PrivilegeCount = 1;
//    Privileges.Privileges[ 0 ].Luid = Luid;
//    Privileges.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;
//
//    AdjustTokenPrivileges( Token, FALSE, &Privileges, 0, NULL, NULL );
//
//    Ret = ( GetLastError( ) == ERROR_SUCCESS );
//
//    CloseHandle( Token );
//
//    return Ret;
//}

uintptr_t
MapperGetAbsAddress(
    IN HANDLE Process,
    IN uintptr_t Address,
    IN uint32_t Offset,
    IN uint32_t InstructionSize
)
{
    uint32_t Relative;
    WrapReadProcessMemory( Process, ( LPVOID )( Address + Offset ), &Relative, sizeof( uint32_t ), NULL );

    return Address + Relative + InstructionSize;
}