#include "pch.h"
#include "inputsystem.h"

int main( )
{
    c_inputsystem* input = new c_inputsystem( );

    // vector of wstrings holding the buttons you pressed
    std::vector<std::wstring> gathered_input;

    while ( true ) {

        input->get_keystates( );

        for ( int i = 0; i < 0xFF; i++ ) {
            if ( input->get_state( i, PRESSED ) ) {
                auto str = input->get_character( i );
                if ( str.length( ) ) {
                    gathered_input.push_back( str );
                    wprintf( L"%ls", str.c_str( ) );
                }
            }
        }

        // break out of the loop when END is pressed, and write the input to a text document
        if ( input->get_state( VK_END, PRESSED ) )
            break;

        Sleep( 10 );
    }

    // vector of bytes to hold the new UTF-8 encoded bytes
    std::vector<BYTE> output_bytes;

    // just make a stack-allocation for this and we'll use the same temporary buffer for all of the inputs
    char output[ 128 ];
    for ( auto it : gathered_input ) {
        // zero out the output to not keep any invalid information
        memset( output, 0, 128 );
        
        // convert WCHAR to MultiByte UTF-8 format
        int written = WideCharToMultiByte( CP_UTF8, 0, it.c_str( ), -1, output, 128, nullptr, nullptr );

        // if it couldnt write anything, or if it only wrote null termination dont do anything further
        if ( written && --written ) {
            // get each byte written (except for null-termination) into the vector of bytes
            for ( int i = 0; i < written; i++ )
                output_bytes.push_back( output[ i ] );
        }
    }

    // create a file called output.txt in the current directory
    FILE* file;
    fopen_s( &file, "output.txt", "wb" );

    // write all of UTF-8 bytes that we gathered
    fwrite( &output_bytes[0], 1, output_bytes.size(), file );
    
    // close the file
    fclose( file );

    return 0;
}

