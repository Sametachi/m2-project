/*
    CorsacBase Logger wrapper
*/
#pragma once

#include "Logger.h"
#include <fstream>

inline void ExceptionLogger(const std::string& stFileName, const std::string& stLogData)
{
    std::ofstream f(stFileName.c_str(), std::ofstream::out | std::ofstream::app);
    f << stLogData.c_str() << std::endl;
    f.close();
}

inline void Fail(const std::string& stFileName, const char* c_szFormat, ...)
{
    char szBuffer[8192] = { 0 };

    va_list vaArgList = nullptr;
    va_start(vaArgList, c_szFormat);
    vsprintf_s(szBuffer, c_szFormat, vaArgList);
    va_end(vaArgList);

    ExceptionLogger(stFileName.c_str(), szBuffer);
}

/*
    Setting up my log likings..
*/

#ifdef _DEBUG

#ifndef ConsoleLog
#define ConsoleLog(fmt, ...)	Logger::GetInstance()->Write(Log::LogLevel::Debug, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__);
#endif

#ifndef SysLog
#define SysLog(fmt, ...) Logger::GetInstance()->Write(Log::LogLevel::Err, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__);
#endif

#ifndef WarnLog
#define WarnLog(fmt, ...) Logger::GetInstance()->Write(Log::LogLevel::Warn, __FILE__, __LINE__, __FUNCTION__, fmt,  __VA_ARGS__);
#endif

#ifndef PyLog
#define PyLog(fmt, ...) Logger::GetInstance()->Write(Log::LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__);
#endif

#ifndef FatalLog
#define FatalLog(fmt, ...)	Logger::GetInstance()->Write(Log::LogLevel::Fatal, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__);
#endif

#ifndef TraceLog
#define TraceLog(fmt, ...)	Logger::GetInstance()->Write(Log::LogLevel::Trace, __FILE__, __LINE__, __FUNCTION__, fmt,  __VA_ARGS__);
#endif

#else

#ifndef ConsoleLog
#define ConsoleLog(fmt, ...)
#endif

#ifndef SysLog
#define SysLog(fmt, ...) Logger::GetInstance()->Write(Log::LogLevel::Err, "", 0, "", fmt, __VA_ARGS__);
#endif

#ifndef WarnLog
#define WarnLog(fmt, ...) Logger::GetInstance()->Write(Log::LogLevel::Warn, "", 0, "", fmt, __VA_ARGS__);
#endif

#ifndef PyLog
#define PyLog(fmt, ...) Logger::GetInstance()->Write(Log::LogLevel::Info, "", 0, "", fmt, __VA_ARGS__);
#endif

#ifndef FatalLog
#define FatalLog(fmt, ...)	Logger::GetInstance()->Write(Log::LogLevel::Fatal, "", 0, "", fmt, __VA_ARGS__);
#endif

#ifndef TraceLog
#define TraceLog(fmt, ...)	Logger::GetInstance()->Write(Log::LogLevel::Trace, "", 0, "", fmt, __VA_ARGS__);
#endif

#endif
