#include "StdAfx.h"
//#include <Basic/_DebugControl_.h>

#ifdef USE_SCRIPTLIB_DEBUG
extern bool isDebugMode = true;
extern bool isDebugFilesMode = true;
#else
extern bool isDebugMode = false;
extern bool isDebugFilesMode = false;
#endif

PYBIND11_EMBEDDED_MODULE(log, m)
{
    m.def("LogBox", [](std::string msg) {
        MessageBoxA(nullptr, msg.c_str(), "Debug", MB_OK);
    });

    m.def("LogBox", [](std::string msg, std::string caption) {
        MessageBoxA(nullptr, msg.c_str(), caption.c_str(), MB_OK);
        });

    m.def("DebugMode", []() -> bool { return isDebugMode;});
    m.def("Debug", []() -> bool { return isDebugFilesMode;});
    m.def("Trace", [](std::string szMsg) { PyLog("{0}", szMsg); });
    m.def("Trace", [](std::string szMsg, std::string szArgs) { PyLog("{0} {1}", szMsg, szArgs); });
    m.def("Fatal", [](std::string what, int32_t level, std::string file, size_t line, std::string fn) { Logger::GetInstance()->Write(Log::LogLevel(level), file.c_str(), line, fn.c_str(), what.c_str()); });


    m.attr("SEVERITY_INFO") = int32_t(Log::LogLevel::Info); // Python
    m.attr("SEVERITY_WARNING") = int32_t(Log::LogLevel::Warn); // Warning
    m.attr("SEVERITY_ERROR") = int32_t(Log::LogLevel::Err); // Syserr
    m.attr("SEVERITY_CRITICAL") = int32_t(Log::LogLevel::Fatal); // Fatal
}

PYBIND11_EMBEDDED_MODULE(dbg, m)
{
    // STUB!
    m.def("LogBox", [](std::string msg) {});
    m.def("TraceError", [](std::string msg) {});
}
