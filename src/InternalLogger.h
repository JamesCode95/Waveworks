#pragma once

#include "GFSDK_Logger.h"
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

//	virtual void log(nv::LogSeverity severity, const char* filename, int linenumber, const char* text, ...) override;
	virtual void log(nv::LogSeverity severity, const wchar_t* filename, int linenumber, const wchar_t* text, ...) override;

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


#ifndef NV_LOG
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_LOG(...) if (g_UserLogger) { g_UserLogger->log(nv::LogSeverity::kInfo, __DEF_FILE__, __LINE__, __VA_ARGS__); } else { InternalLogger::GetInstance()->log(nv::LogSeverity::kInfo, __DEF_FILE__, __LINE__, __VA_ARGS__); }
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_WARN(...) if (g_UserLogger) { g_UserLogger->log(nv::LogSeverity::kWarning, __DEF_FILE__, __LINE__, __VA_ARGS__); } else { InternalLogger::GetInstance()->log(nv::LogSeverity::kWarning, __DEF_FILE__, __LINE__, __VA_ARGS__); }
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_ERROR(...) if (g_UserLogger) { g_UserLogger->log(nv::LogSeverity::kError, __DEF_FILE__, __LINE__, __VA_ARGS__); } else { InternalLogger::GetInstance()->log(nv::LogSeverity::kError, __DEF_FILE__, __LINE__, __VA_ARGS__); }
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_FATAL(...) if (g_UserLogger) { g_UserLogger->log(nv::LogSeverity::kFatal, __DEF_FILE__, __LINE__, __VA_ARGS__); } else { InternalLogger::GetInstance()->log(nv::LogSeverity::kFatal, __DEF_FILE__, __LINE__, __VA_ARGS__); }

#endif