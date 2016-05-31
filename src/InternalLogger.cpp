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

void InternalLogger::log(const char* text, nv::LogSeverity severity, const char* filename, int linenumber)
{
	if (getLoggingLevel() > severity)
		return;

	std::ostringstream out;

	out << filename << "(" << linenumber << "): " << "[" << nv::LogSeverityStrings[(int)severity] << "] " << text << std::endl;

	OutputDebugStringA(out.str().c_str());
}

void InternalLogger::log(const wchar_t* text, nv::LogSeverity severity, const wchar_t* filename, int linenumber)
{
	if (getLoggingLevel() > severity)
		return;

	std::wstringstream out;

	out << filename << "(" << linenumber << "): " << "[" << nv::LogSeverityStrings[(int)severity] << "] " << text << std::endl;

	OutputDebugStringW(out.str().c_str());
}
