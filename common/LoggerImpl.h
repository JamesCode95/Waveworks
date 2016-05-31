#pragma once
#include "Logger.h"
#include <cstdarg>
#include <cstdio>

class LoggerWWSamples : public nv::ILogger
{
public:
	LoggerWWSamples();
	LoggerWWSamples(nv::LogSeverity loggingLevel);
	virtual ~LoggerWWSamples() = default;

	virtual void log(const char* text, nv::LogSeverity severity, const char* filename, int linenumber) override;
	virtual void log(const wchar_t* text, nv::LogSeverity severity, const wchar_t* filename, int linenumber) override;

	nv::LogSeverity getLoggingLevel();
	void setLoggingLevel(nv::LogSeverity newLevel);

private:
	nv::LogSeverity	LoggingLevel;
};

extern LoggerWWSamples* g_Logger;

namespace wwsamples
{
	inline void log(nv::LogSeverity severity, const char* filename, int linenumber, const char* format, ...)
	{
		if (g_Logger == nullptr)
		{
			g_Logger = new LoggerWWSamples();
		}

		if (g_Logger->getLoggingLevel() > severity)
			return;

		char buffer[1024];

		va_list args;
		va_start(args, format);
#if NV_GCC_FAMILY
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
		vsnprintf_s(buffer, sizeof(buffer), format, args);
#if NV_GCC_FAMILY
#pragma GCC diagnostic pop
#endif
		va_end(args);

		g_Logger->log(buffer, severity, filename, linenumber);
	}
}



#ifndef NV_LOG
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_LOG(...) wwsamples::log(nv::LogSeverity::kInfo, __FILE__, __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_WARN(...) wwsamples::log(nv::LogSeverity::kWarning, __FILE__, __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_ERROR(...) wwsamples::log(nv::LogSeverity::kError, __FILE__, __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_FATAL(...) wwsamples::log(nv::LogSeverity::kFatal, __FILE__, __LINE__, __VA_ARGS__)

#endif