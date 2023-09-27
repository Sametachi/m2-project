/*!
    @file src/Log_G3log.cpp
    @brief G3log provider implementation
    @date 31/12/2021
*/
//#include "CorsacBasePch.h"
#include "log/G3log.h"

#include <g3log/g3log.hpp>

#include "log/g3sinks/LogRotateWithFilter.h"
#include "log/g3sinks/ColorCoutSink.h"

#ifdef CORSAC_NS
namespace CORSAC_NS
{
#endif
    namespace Log
    {
		/*!
			Funcion used to format the logs
			@param msg The message info to format
			@return The formatted log
		*/
		static std::string log_detail(const g3::LogMessage& msg) {
			std::string out;
			out.append(msg.timestamp("[%d/%m/%y %H:%M:%S] [") + msg.level() + "] ");
			return out;
		}

		/*!
			A custom level used to define an error (g3log doesn't have the difference between ERROR and FATAL)
		*/
		static const LEVELS MYERROR{ WARNING.value + 1, "ERROR" };

		/*!
			Default constructor
		*/
		G3logProvider::G3logProvider() : m_eLevel(DEFAULT_LOGLEVEL)
        {
            m_pWorker = g3::LogWorker::createLogWorker();
			g3::initializeLogging(m_pWorker.get());
		}

		/*
			Default destructor
		*/
		G3logProvider::~G3logProvider()
		{
			Shutdown();
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
        bool G3logProvider::Create(const std::string& log_dir, const std::string& log_file, const std::string& err_dir, const std::string& err_file, bool bRotation, bool bUseFull, bool bUseConsole)
        {
			// todo: disable rotation for g3log

			std::vector<LEVELS> filterOut = { WARNING, MYERROR, FATAL, g3::internal::CONTRACT, g3::internal::FATAL_SIGNAL, g3::internal::FATAL_EXCEPTION };
			auto logSyserr = LogRotateWithFilter::CreateLogRotateWithFilter(err_file, err_dir, filterOut);
			logSyserr->overrideLogDetails(&log_detail);
			logSyserr->setFlushPolicy(LOGFILE_FLUSH_POLICY);
			logSyserr->setMaxLogSize(LOGFILE_MAX_SIZE);
			m_pWorker->addSink(std::move(logSyserr), &LogRotateWithFilter::save);

			if (bUseFull)
			{
				std::vector<LEVELS> filterLog = { G3LOG_DEBUG, INFO };
				auto logSyslog = LogRotateWithFilter::CreateLogRotateWithFilter(log_file, log_dir, filterLog);
				logSyslog->overrideLogDetails(&log_detail);
				logSyslog->setFlushPolicy(LOGFILE_FLUSH_POLICY);
				logSyslog->setMaxLogSize(LOGFILE_MAX_SIZE);
				m_pWorker->addSink(std::move(logSyslog), &LogRotateWithFilter::save);
			}

			if (bUseConsole)
			{
				std::vector<LEVELS> consoleOut;

				if (!bUseFull)
					 consoleOut = { WARNING, MYERROR, FATAL };
				else
					 consoleOut = { G3LOG_DEBUG, INFO, WARNING, MYERROR, FATAL };

				auto logConsole = std::make_unique<ColorCoutSink>(consoleOut);

				logConsole->overrideLogDetails(&log_detail);
				m_pWorker->addSink(std::move(logConsole), &ColorCoutSink::ReceiveLogMessage);
			}

			return true;
        }

		/*!
			Shuts down the log provider
		*/
		void G3logProvider::Shutdown()
		{
			m_pWorker->removeAllSinks();

			g3::internal::shutDownLogging();
		}

		/*!
			Writes a new message to the log
			@param lvl The log level of the current message to write
			@param data The data of the message
		*/
		void G3logProvider::Write(LogLevel lvl, const std::string& data)
		{
			if (static_cast<uint32_t>(lvl) <= static_cast<uint32_t>(m_eLevel))
			{
				const LEVELS* level = NULL;

				switch (lvl)
				{
				default:
					return;
				case LogLevel::Debug:
					level = &DEBUG;
					break;
				case LogLevel::Info:
					level = &INFO;
					break;
				case LogLevel::Warn:
					level = &WARNING;
					break;
				case LogLevel::Err:
					level = &MYERROR;
					break;
				case LogLevel::Fatal:
					level = &FATAL;
					break;
				}

				LOG(*level) << data;
			}
		}

		void G3logProvider::Flush()
		{
			// lol
		}
    }

#ifdef CORSAC_NS
}
#endif
