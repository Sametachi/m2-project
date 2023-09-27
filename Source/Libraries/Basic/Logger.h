/*!
    @file include/CorsacBase/Log.h
    @brief Logging functionalities
    @date 31/12/2021

    // 16/05/2022: Modified for yitsorabasic
*/
#pragma once

#include "log/Provider.h"
#include "log/G3log.h"
#include "log/Spdlog.h"

#include "Singleton.h"

#include <fmt/core.h>
#include <fmt/chrono.h>

#ifdef CORSAC_NS
namespace CORSAC_NS
{
#endif
    constexpr const char* DEFAULT_LOGDIR = "log";
    constexpr const char* DEFAULT_ERRFILE_NAME = "err";
    constexpr const char* DEFAULT_LOGFILE_NAME = "sys";
    constexpr const char* DEFAULT_LOG_EXT = ".log";

    enum class ProviderType
    {
        G3log,
        Spdlog,
    };

    /*!
        @class Logger
        Singleton application logger
    */
    class Logger final : public Singleton<Logger>
    {
    public:
        /*!
            Default construuctor
            @param provider The provider to use
        */
        Logger(ProviderType p)
        {
            m_szLog = GenerateFileName(DEFAULT_LOGFILE_NAME);
            m_szErr = GenerateFileName(DEFAULT_ERRFILE_NAME);
            m_szDir = DEFAULT_LOGDIR;

            if (p == ProviderType::G3log)
                m_pProvider = std::make_unique<Log::G3logProvider>();
            else
                m_pProvider = std::make_unique<Log::SpdlogProvider>();
        }

        /*!
            Default destructor
        */
        ~Logger()
        {
            Destroy();
        }

        /*!
            Changes the log level
            @param lvl The new log level
        */
        void SetLevel(Log::LogLevel lvl)
        {
            m_pProvider->SetLogLevel(lvl);
        }

        /*!
            Initializes the Logger
            @param bRotation Enable rotation
            @param bUseFull Set to true if you want to capture all logs to a file (aka using the general log)
            @param bUseConsole Set to true if you want to use capture logs in the native console as well (like the Windows one or the Linux terminal)
            @return true if the initialization succeeded, otherwise false
        */
        bool Initialize(bool bRotation = true, bool bUseFull = false, bool bUseConsole = false)
        {
            return m_pProvider->Create(m_szDir, m_szLog, m_szDir, m_szErr, bRotation, bUseFull, bUseConsole);
        }

        /*!
            Destroyes the Logger functionality
        */
        void Destroy()
        {
            m_pProvider->Shutdown();
        }

        /*!
            Flushes the provider
        */
        void Flush()
        {
            m_pProvider->Flush();
        }

        /*!
            Writes a message to the logger
            @param lvl The log level
            @param file The name of the file which writes this message (used for debugging)
            @param line The line of the file which writes this message (used for debugging)
            @param fmt The formatter line
            @param args The various arguments
        */
        template<typename... Args>
        void Write(Log::LogLevel lvl, const char* file, size_t line, const char* function, const char* fmt, Args&&... args) const
        {
            std::string str = fmt::format("[{}@{}:{}] >>> ", function, file, line);
            str += fmt::vformat(fmt, fmt::make_format_args(args...));
            m_pProvider->Write(lvl, str);
        }

        /*!
            Changes the name of the default log file
            @param sz The new log file name
        */
        void SetCustomLogFileName(std::string sz) { m_szLog = sz; }

        /*!
            Changes the name of the error log file
            @param sz The new log file name
        */
        void SetCustomErrFileName(std::string sz) { m_szErr = sz; }

        /*!
            Changes the logging directory
            @param sz The new log directory
        */
        void SetCustomLogDirectory(std::string sz) { m_szDir = sz; }

    private:
        /*!
            Unique instance to the log provider
        */
        std::unique_ptr<Log::ILogProvider> m_pProvider;

        /*!
            The file name of the default logging file
        */
        std::string m_szLog;
        
        /*!
            The file name of the default error file
        */
        std::string m_szErr;

        /*!
            The log directory
        */
        std::string m_szDir;

    private:
        /*!
            Generates a default filename from the source file name
        */
        inline static std::string GenerateFileName(const std::string& name)
        {
            auto t = std::time(nullptr);
            return fmt::format("{}_{:%d%m%Y_%H%M%S}", name, fmt::localtime(t));
        }
    };

#ifdef CORSAC_NS
}
#endif
