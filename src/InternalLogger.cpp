#include "InternalLogger.h"
#include <iostream>
#include <sstream>
#include <windows.h>

std::unique_ptr<InternalLogger> g_Logger;
nv::ILogger* g_UserLogger = nullptr;

InternalLogger::InternalLogger()
{

}

InternalLogger::InternalLogger(nv::LogSeverity loggingLevel):
	LoggingLevel(loggingLevel)
{

}

nv::LogSeverity InternalLogger::getLoggingLevel()
{
	return LoggingLevel;
}

void InternalLogger::setLoggingLevel(nv::LogSeverity newLevel)
{
	LoggingLevel = newLevel;
}

void InternalLogger::log(nv::LogSeverity severity, const wchar_t* filename, int linenumber, const wchar_t* format, ...)
{
	if (getLoggingLevel() > severity)
		return;

	wchar_t buffer[1024];

	va_list args;
	va_start(args, format);
#if NV_GCC_FAMILY
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
	_vsnwprintf_s(buffer, sizeof(buffer), format, args);
#if NV_GCC_FAMILY
#pragma GCC diagnostic pop
#endif
	va_end(args);

	std::wstringstream out;

	out << filename << "(" << linenumber << "): " << "[" << nv::LogSeverityStrings[(int)severity] << "] " << buffer << std::endl;

	OutputDebugStringW(out.str().c_str());
}
