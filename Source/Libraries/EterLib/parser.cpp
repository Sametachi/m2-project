#include "StdAfx.h"
#include "parser.h"
#include <boost/locale/utf.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>

namespace //Extract as Utf-8
{
	typedef boost::locale::utf::utf_traits<char> Utf8;

	std::size_t ExtractBox(std::string& source)
	{
		assert(source[0] == '[' && "Requirement");

		auto boxEnd = source.find(']');
		if (boxEnd == std::string::npos)
		{
			SysLog("Syntax error: EOF while looking for ']' in {0}", source);
			return {};
		}
		source = source.substr(1, boxEnd - 1);
		boost::trim(source);
		// TODO(ipx): Is plus one really that important!?
		return boxEnd + 1;
	}
}

// Let's Create the source in UTF8, we don't want mismatch for each decoder
uint32_t ScriptGroup::Create(const std::string& source)
{
	m_cmdList.clear();
	int32_t lineCount = 1;

	if (source.empty())
		return 0;

	for (std::size_t pos = 0, size = source.size(); pos != size; )
	{
		SCmd cmd;

		if (source[pos] == '[')
		{
			std::string box = source.substr(pos);
			pos += ExtractBox(box);

			size_t space = box.find(' ');
			if (space == std::string::npos)
			{
				cmd.name = box;
				m_cmdList.emplace_back(std::move(cmd));
				continue;
			}

			cmd.name = box.substr(0, space);
			if (!ParseArguments(box.substr(space), cmd.argList))
				return 0;
		}
		else if (source[pos] == '\r' || source[pos] == '\n')
		{
			if (source[pos] == '\n')
				++lineCount;
			++pos;
			continue;
		}
		else
		{
			auto it = source.begin() + pos;

			auto code = Utf8::decode(it, source.end());
			if (code == boost::locale::utf::illegal || code == boost::locale::utf::incomplete)
			{
				SysLog("Invalid UTF-8 in {0}", source);
				return -1;
			}

			const std::string val = source.substr(pos, (it - source.begin()) - pos);

			cmd.name = "LETTER";
			cmd.argList.emplace_back(SArgument("value", val));
			pos += val.size();
		}

		m_cmdList.emplace_back(std::move(cmd));
	}

	return lineCount;
}

// Just as before..
bool ScriptGroup::GetCmd(SCmd& cmd)
{
	if (m_cmdList.empty())
		return false;

	cmd = m_cmdList.front();
	m_cmdList.pop_front();
	return true;
}

bool ScriptGroup::ReadCmd(SCmd& cmd)
{
	if (m_cmdList.empty())
		return false;

	cmd = m_cmdList.front();
	return true;
}

// Let's get the Args and set them to List
bool ScriptGroup::ParseArguments(const std::string& source, TArgList& argList)
{
	using Tokenizer = boost::tokenizer< boost::char_separator<char>>;
	const boost::char_separator<char> separator{ "|" };
	Tokenizer tok{ source, separator };
	for (auto& t : tok)
	{
		std::string name, value, cur = t;

		size_t nameValueSep = cur.find(';');
		if (nameValueSep == std::string::npos)
		{
			name = cur;
		}
		else
		{
			name = cur.substr(0, nameValueSep);
			value = cur.substr(nameValueSep + 1);
		}

		boost::trim(name);
		boost::trim(value);

		argList.emplace_back(SArgument(name, value));
	}

	return true;
}