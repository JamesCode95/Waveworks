#pragma once

namespace nv
{
	enum struct LogSeverity
	{
		kInfo = 0, // Message contains information about normal and expected behavior
		kWarning = 1, // Message contains information about a potentially problematic situation
		kError = 2, // Message contains information about a problem
		kFatal = 3 // Message contains information about a fatal problem; program should be aborted
	};

	static const char * LogSeverityStrings[] = { "INFO", "WARNING", "ERROR", "FATAL" };


	// Note: Implementation of this interface must be thread-safe
	class ILogger
	{
	public:
		// ‘filename’ is NULL and ‘linenumber’ is 0 in release builds of GameWorks	
		virtual void log(const char* text, LogSeverity severity, const char* filename, int linenumber) = 0;
		virtual void log(const wchar_t* text, LogSeverity severity, const wchar_t* filename, int linenumber) = 0;
	};
}

