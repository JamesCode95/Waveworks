#pragma once
#include "GFSDK_Logger.h"
#include <cstdarg>
#include <cstdio>

class LoggerWWSamples : public nv::ILogger
{
public:
	LoggerWWSamples();
	LoggerWWSamples(nv::LogSeverity loggingLevel);
	virtual ~LoggerWWSamples() = default;

//	virtual void log(const char* text, nv::LogSeverity severity, const char* filename, int linenumber) override;
	virtual void log(nv::LogSeverity severity, const wchar_t* filename, int linenumber, const wchar_t* format, ...) override;

	nv::LogSeverity getLoggingLevel();
	void setLoggingLevel(nv::LogSeverity newLevel);

	static LoggerWWSamples* GetInstance()
	{
		static LoggerWWSamples Instance;
		return &Instance;
	}
private:
	nv::LogSeverity	LoggingLevel;
};

#ifndef NV_LOG

// NOTE: This assumes we're using wide strings!
#define WIDEWRAP2(s) L##s
#define WIDEWRAP1(s) WIDEWRAP2(s)

/// <param name="__VA_ARGS__">format, ...</param>
#define NV_LOG(...) LoggerWWSamples::GetInstance()->log(nv::LogSeverity::kInfo, WIDEWRAP1(__FILE__), __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_WARN(...) LoggerWWSamples::GetInstance()->log(nv::LogSeverity::kWarning, WIDEWRAP1(__FILE__), __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_ERROR(...) LoggerWWSamples::GetInstance()->log(nv::LogSeverity::kError, WIDEWRAP1(__FILE__), __LINE__, __VA_ARGS__)
/// <param name="__VA_ARGS__">format, ...</param>
#define NV_FATAL(...) LoggerWWSamples::GetInstance()->log(nv::LogSeverity::kFatal, WIDEWRAP1(__FILE__), __LINE__, __VA_ARGS__)

#endif