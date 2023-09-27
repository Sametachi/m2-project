#pragma once
#include <list>
#include <string>

class ScriptGroup
{
public:
	struct SArgument
	{
		SArgument(std::string name, std::string value) : strName(std::move(name)), strValue(std::move(value))
		{
		}

		std::string strName;
		std::string strValue;
	};

	typedef std::list<SArgument> TArgList;

	struct SCmd
	{
		std::string name;
		TArgList argList;
	};

	uint32_t Create(const std::string& source);
	bool GetCmd(SCmd& cmd);
	bool ReadCmd(SCmd& cmd);

private:
	bool ParseArguments(const std::string& source, TArgList& argList);
	std::list<SCmd> m_cmdList;
};