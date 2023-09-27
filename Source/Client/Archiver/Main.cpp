#include <StdAfx.hpp>
#include "Main.hpp"
#include "PackageCreator.hpp"
#include <Basic/SimpleApp.hpp>
#include <Basic/Logging.hpp>
#include <FileSystem/FilenameWrapper.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <iostream>
#include <ppl.h>
#include <concurrent_vector.h>
#include <ppltasks.h>
#include <ppltaskscheduler.h>
#include "XmlUtil.hpp"
#include <storm/optionparser.h>

namespace boost { namespace program_options {} }
namespace po = boost::program_options;
using namespace concurrency;

class Main : public SimpleApp
{
public:
	Main();
	virtual ~Main() = default;

	int Run(int argc, const char** argv);
	task<void> ParseXMLFiles();
private:
	bool ParseArguments(int argc, const char** argv);
	std::vector<std::string> m_xmls;
	Logger m_log;
};

// Observes all exceptions that occurred in all tasks in the given range.
template<class T, class InIt>
void observe_all_exceptions(InIt first, InIt last)
{
	std::for_each(first, last, [](concurrency::task<T> t)
	{
		t.then([](concurrency::task<T> previousTask)
		{
			try
			{
				previousTask.get();
			}
			catch (...)
			{
				WarnLog("An exception occured");
			}
		});
	});
}

Main::Main() : m_log(ProviderType::Spdlog)
{
	m_log.UseConsole();
	m_log.UseFullLogging();
	if (!m_log.Initialize())
	{
		throw std::runtime_error("Cannot initialize logging!");
	}
}

int Main::Run(int argc, const char** argv)
{
	try 
	{
		if (!ParseArguments(argc, argv))
			return 1;
	}
	catch (std::exception& e) 
	{
		WarnLog("args: {0}", e.what());
		return 1;
	}

	ParseXMLFiles().then([](task<void> previousTask)
    {
        try
        {
            previousTask.get();
        }
        catch (const task_canceled&)
        {
        }
		WarnLog("Done");
    }).wait();


	return 0;
}

bool Main::ParseArguments(int argc, const char** argv)
{
	std::string action;

	po::options_description desc("Allowed options");
	desc.add_options()
	("xmls,X", po::value< std::vector<std::string> >(&m_xmls)->multitoken(), "XML files");

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
	po::notify(vm);

	if (!vm.count("xmls")) 
	{
		WarnLog("No xml files given");
		return false;
	}

	return true;
}

task<void> Main::ParseXMLFiles()
{
	std::vector<task<void>> tasks;
	tasks.reserve(m_xmls.size());

	for (auto& xmlFile : m_xmls)
	{
		tasks.emplace_back(create_task([&]() -> boost::property_tree::ptree
		{
			WarnLog("Parsing {0}", xmlFile);
			
			boost::property_tree::ptree tree;
			read_xml(xmlFile, tree);
			return tree;
		}).then([](boost::property_tree::ptree result)
			{
				for (auto& vt : result.get_child("ScriptFile"))
				{
					if (vt.first == "CreateEterPack") 
					{
						std::vector<task<void>> subtasks;
						const auto path = vt.second.get<std::string>("<xmlattr>.ArchivePath");
						WarnLog("Creating {0}", path);

						PackageCreator write;

						if (write.Create(path)) 
						{
							for (auto& elem : vt.second) 
							{
								if (elem.first != "File") 
								{
									continue;
								}
									
								const FilenameWrapper archivedPath(elem.second.get<std::string>("<xmlattr>.ArchivedPath"));

								write.Add(archivedPath, elem.second.data(), kFileFlagLz4);
							}
						}
						write.Save();
					}
					else if (vt.first == "CreateEterPackXml") 
					{
						WarnLog("Creating make xml");

						const auto input = vt.second.get<std::string>("<xmlattr>.Input");
						const auto separator = input.find(':');
						std::string source, prefix;
						if (separator != std::string::npos) 
						{
							source = input.substr(0, separator);
							prefix = input.substr(separator + 1, std::string::npos);
						}
						else 
						{
							source = input;
						}

						std::vector<std::string> ignores;
						ignores.reserve(vt.second.size());
						std::vector<std::string> adds;
						adds.reserve(vt.second.size());
						std::vector<std::pair<std::string, std::string>> patches;
						patches.reserve(vt.second.size());

						for (auto& j : vt.second) 
						{
							if (j.first == "Ignore") 
							{
								ignores.emplace_back(j.second.get<std::string>("<xmlattr>.Pattern")); // <Ignore Pattern="[a-zA-Z0-9]+.png" />
							}
							else if (j.first == "Add") 
							{
								adds.emplace_back(j.second.get<std::string>("<xmlattr>.Pattern")); // <Add><![CDATA[Path]]></Add>
							}
							else if (j.first == "Patch") 
							{
								patches.emplace_back(std::make_pair<std::string, std::string>(j.second.get<std::string>("<xmlattr>.Search"),j.second.get<std::string>("<xmlattr>.Replace"))); // <Add><![CDATA[Path]]></Add>
							}
						}

						XmlGenerator gen;
						gen(vt.second.get<std::string>("<xmlattr>.XmlPath"),
							source,
							prefix,
							vt.second.get<std::string>("<xmlattr>.ArchivePath"),
							ignores,
							patches,
							adds);
					}
				}
			}
		));
	}
	
	return when_all(begin(tasks), end(tasks)).then([tasks](task<void> previousTask)
    {
        task_status status = completed;
		try 
		{
			status = previousTask.wait();
		} 
		catch (const std::exception& ex) 
		{
			WarnLog(ex.what());
		}
        observe_all_exceptions<void>(begin(tasks), end(tasks));
        if (status == canceled)
        {
            cancel_current_task();
        }
    });
}
// Storm application Wrapper
SIMPLE_APPLICATION(Main)