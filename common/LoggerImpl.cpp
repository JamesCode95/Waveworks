#include "LoggerImpl.h"
#include <iostream>
#include <sstream>
#include <windows.h>

LoggerWWSamples* g_Logger = nullptr;

LoggerWWSamples::LoggerWWSamples():
LoggingLevel(nv::LogSeverity::kInfo)
{

}

LoggerWWSamples::LoggerWWSamples(nv::LogSeverity loggingLevel) :
	LoggingLevel(loggingLevel)
{

}

nv::LogSeverity LoggerWWSamples::getLoggingLevel()
{
	return LoggingLevel;
}

void LoggerWWSamples::setLoggingLevel(nv::LogSeverity newLevel)
{
	LoggingLevel = newLevel;
}

void LoggerWWSamples::log(const char* text, nv::LogSeverity severity, const char* filename, int linenumber)
{
	std::ostringstream out;

	out << filename << "(" << linenumber << "): " << "[" << nv::LogSeverityStrings[(int) severity] << "] " << text << std::endl;

	OutputDebugStringA(out.str().c_str());
}

void LoggerWWSamples::log(const wchar_t* text, nv::LogSeverity severity, const wchar_t* filename, int linenumber)
{
	std::wstringstream out;

	out << filename << "(" << linenumber << "): " << "[" << nv::LogSeverityStrings[(int)severity] << "] " << text << std::endl;

	OutputDebugStringW(out.str().c_str());
}
