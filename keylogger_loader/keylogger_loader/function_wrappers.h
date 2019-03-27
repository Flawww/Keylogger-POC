#pragma once
#include "pch.h"

typedef BOOL(__stdcall* ReadWriteProcess_t)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
typedef HANDLE(__stdcall* OpenProcess_t)(DWORD, BOOL, DWORD);
typedef HANDLE(__stdcall* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE(__stdcall* CreateRemoteThreadEx_t)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPPROC_THREAD_ATTRIBUTE_LIST, LPDWORD);
typedef BOOL(__stdcall* ReadFile_t)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef HRESULT(__stdcall* SHGetKnownFolderPath_t)(GUID, DWORD, HANDLE, PWSTR*);


static BOOL __stdcall WrapWriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesWritten) {
    ReadWriteProcess_t fn = (ReadWriteProcess_t)GetProcAddress(LoadLibraryA("kernelbase.dll"), "WriteProcessMemory");
    return fn(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

static BOOL __stdcall WrapReadProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesRead) {
    ReadWriteProcess_t fn = (ReadWriteProcess_t)GetProcAddress(LoadLibraryA("kernelbase.dll"), "ReadProcessMemory");
    return fn(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

static HANDLE __stdcall WrapOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId) {
    OpenProcess_t fn = (OpenProcess_t)GetProcAddress(LoadLibraryA("kernelbase.dll"), "OpenProcess");
    return fn(dwDesiredAccess, bInheritHandle, dwProcessId);
}

static HANDLE __stdcall WrapCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    CreateFileW_t fn = (CreateFileW_t)GetProcAddress(LoadLibraryA("kernelbase.dll"), "CreateFileW");
    return fn(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

static HANDLE __stdcall WrapCreateRemoteThreadEx(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList, LPDWORD lpThreadId) {
    CreateRemoteThreadEx_t fn = (CreateRemoteThreadEx_t)GetProcAddress(LoadLibraryA("kernelbase.dll"), "CreateRemoteThreadEx");
    return fn(hProcess, lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpAttributeList, lpThreadId);
}

static BOOL __stdcall WrapReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    ReadFile_t fn = (ReadFile_t)GetProcAddress(LoadLibraryA("kernelbase.dll"), "ReadFile");
    return fn(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

static HRESULT __stdcall WrapSHGetKnownFolderPath(GUID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath) {
    SHGetKnownFolderPath_t fn = (SHGetKnownFolderPath_t)GetProcAddress(LoadLibraryA("Shell32.dll"), "SHGetKnownFolderPath");
    return fn(rfid, dwFlags, hToken, ppszPath);
}