#pragma once

#include "Logger.h"
#include <cstdarg>
#include <cstdio>
#include <wtypes.h>
#include <stdio.h>
#include <wchar.h>
#include <memory>

class InternalLogger : public nv::ILogger
{
public:
	InternalLogger();
	InternalLogger(nv::LogSeverity loggingLevel);
	virtual ~InternalLogger() = default;

	virtual void log(const char* text, nv::LogSeverity severity, const char* filename, int linenumber) override;
	virtual void log(const wchar_t* text, nv::LogSeverity severity, const wchar_t* filename, int linenumber) override;

	nv::LogSeverity getLoggingLevel();
	void setLoggingLevel(nv::LogSeverity newLevel);

	static InternalLogger* GetInstance()
	{
		static InternalLogger Instance;
		return &Instance;
	}

private:
	nv::LogSeverity	LoggingLevel;

};

extern nv::ILogger* g_UserLogger;

namespace WaveworksInternal
{
	inline void log(nv::LogSeverity severity, const wchar_t* filename, int linenumber, const wchar_t* format, ...)
	{
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


		// Only output to one logger at a time. May not be the desired behaviour.
		if (g_UserLogger)
		{
			g_UserLogger->log(buffer, severity, filename, linenumber);
		}
		else
		{
			InternalLogger::GetInstance()->log(buffer, severity, filename, linenumber);
		}

	}

	inline void log(nv::LogSeverity severity, const char* filename, int linenumber, const char* format, ...)
	{
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

		// Only output to one logger at a time. May not be the desired behaviour.
		if (g_UserLogger)
		{
			g_UserLogger->log(buffer, severity, filename, linenumber);
		}
		else
		{
			InternalLogger::GetInstance()->log(buffer, severity, filename, linenumber);
		}
	}
}



#ifndef NV_LOG
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_LOG(...) WaveworksInternal::log(nv::LogSeverity::kInfo, __DEF_FILE__, __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_WARN(...) WaveworksInternal::log(nv::LogSeverity::kWarning, __DEF_FILE__, __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_ERROR(...) WaveworksInternal::log(nv::LogSeverity::kError, __DEF_FILE__, __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_FATAL(...) WaveworksInternal::log(nv::LogSeverity::kFatal, __DEF_FILE__, __LINE__, __VA_ARGS__)

#endif