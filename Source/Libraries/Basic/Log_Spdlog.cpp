/*!
    @file src/Log_Spdlog.cpp
    @brief Spdlog provider implementation
    @date 31/12/2021
*/
//#include "CorsacBasePch.h"
#include "log/Spdlog.h"
#include "Logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#ifdef CORSAC_NS
namespace CORSAC_NS
{
#endif
    namespace Log
    {

        /*!
            Spdlog format pattern
        */
        constexpr const char* LOG_PATTERN = "[%d/%m/%Y %H:%M:%S] [%^%l%$] %v";

        /*!
            Console logger
        */
        static std::shared_ptr<spdlog::async_logger> g_pConsoleLogger = nullptr;

        /*!
            Default constructor
        */
        SpdlogProvider::SpdlogProvider()
        {

        }

        /*
            Default destructor
        */
        SpdlogProvider::~SpdlogProvider()
        {
            Shutdown();
        }

        static constexpr spdlog::level::level_enum GetSpdLevelFromCorsac(LogLevel lvl)
        {
            switch (lvl)
            {
            case LogLevel::Info:
                return spdlog::level::info;
            case LogLevel::Debug:
                return spdlog::level::debug;
            case LogLevel::Trace:
                return spdlog::level::trace;
            case LogLevel::Fatal:
                return spdlog::level::critical;
            case LogLevel::Err:
                return spdlog::level::err;
            case LogLevel::Warn:
                return spdlog::level::warn;
            }

            return spdlog::level::off;
        }

        /*!
            Creates the log provider
            @param log_dir The directory to store the general log file
            @param log_file The name of the general log file
            @param err_dir the directory to store the error log file
            @param err_file The name of the error file
            @param bUseFull Set to true if you want to capture all logs to a file (aka using the general log)
            @param bUseConsole Set to true if you want to use capture logs in the native console as well (like the Windows one or the Linux terminal)
            @return True if the creation succeeded, otherwise false
        */
        bool SpdlogProvider::Create(const std::string& log_dir, const std::string& log_file, const std::string& err_dir, const std::string& err_file, bool bRotation, bool bUseFull, bool bUseConsole)
        {
            spdlog::flush_every(std::chrono::seconds(LOGFILE_FLUSH_TIME));
            spdlog::enable_backtrace(100);
            spdlog::set_level(GetSpdLevelFromCorsac(DEFAULT_LOGLEVEL));

            spdlog::init_thread_pool(8192, 1); // Init the async thread

            auto errlogComplete = err_dir + "/" + err_file + DEFAULT_LOG_EXT;

            std::vector<spdlog::sink_ptr> sinks;

            if (bRotation)
            {
                auto errLog = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(errlogComplete, LOGFILE_MAX_SIZE, LOGFILE_MAX_ROTATION);
                errLog->set_level(spdlog::level::warn);
                errLog->set_pattern(LOG_PATTERN);
                sinks.push_back(errLog);
            }
            else
            {
                auto errLog = std::make_shared<spdlog::sinks::basic_file_sink_mt>(errlogComplete);
                errLog->set_level(spdlog::level::warn);
                errLog->set_pattern(LOG_PATTERN);
                sinks.push_back(errLog);
            }

            if (bUseFull)
            {
                errlogComplete = log_dir + "/" + log_file + DEFAULT_LOG_EXT;

                if (bRotation)
                {
                    auto genLog = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(errlogComplete, LOGFILE_MAX_SIZE, LOGFILE_MAX_ROTATION);
                    genLog->set_pattern(LOG_PATTERN);
                    genLog->set_level(bUseFull ? spdlog::level::trace : spdlog::level::warn);
                    sinks.push_back(genLog);
                }
                else
                {
                    auto genLog = std::make_shared<spdlog::sinks::basic_file_sink_mt>(errlogComplete);
                    genLog->set_pattern(LOG_PATTERN);
                    genLog->set_level(bUseFull ? spdlog::level::trace : spdlog::level::warn);
                    sinks.push_back(genLog);
                }
            }

            if (bUseConsole)
            {
                auto consoleLog = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
                consoleLog->set_level(bUseFull ? spdlog::level::trace : spdlog::level::warn);
                consoleLog->set_pattern(LOG_PATTERN);

                // Do not register this logger in the sinks so we avoid flushing the console backtrace at crash (which is only usefull in files not in console)

                g_pConsoleLogger = std::make_shared<spdlog::async_logger>("CorsacLog Console", consoleLog, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
                g_pConsoleLogger->disable_backtrace();
                spdlog::register_logger(g_pConsoleLogger);
            }

            auto logger = std::make_shared<spdlog::async_logger>("CorsacLog", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger);
            return true;
        }

        /*!
            Shuts down the log provider
        */
        void SpdlogProvider::Shutdown()
        {
            spdlog::shutdown();
            g_pConsoleLogger.reset(); // free the global console ptr
        }

        /*!
            Writes a new message to the log
            @param lvl The log level of the current message to write
            @param data The data of the message
        */
        void SpdlogProvider::Write(LogLevel lvl, const std::string& data)
        {
            auto clvl = GetSpdLevelFromCorsac(lvl);
            spdlog::log(clvl, data);

            if (g_pConsoleLogger)
                g_pConsoleLogger->log(clvl, data);
        }

        /*!
            Sets the log level of the provider
            @param lvl The new log level to apply
        */
        void SpdlogProvider::SetLogLevel(LogLevel lvl)
        {
            auto clvl = GetSpdLevelFromCorsac(lvl);
            spdlog::set_level(clvl);

            if (g_pConsoleLogger)
                g_pConsoleLogger->set_level(clvl);
        }

        void SpdlogProvider::Flush()
        {
            spdlog::default_logger()->flush();

            if (g_pConsoleLogger)
                g_pConsoleLogger->flush();
        }

        /*!
            Internal function used at app shutdown to dump the syslog backtrace
        */
        void spdlog_dump_backtrace()
        {
            spdlog::dump_backtrace();
        }
    }

#ifdef CORSAC_NS
}
#endif
