#pragma once
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <fstream>
#include <streambuf>
#include <Basic/Logging.hpp>
#include <boost/utility/string_view.hpp>
/*
	BOOST GROUPTEXT PARSERS 
*/
namespace GroupTextScope
{
	enum GroupTextScope
	{
		/// Scope is a group.
		kGroup,
	
		/// Scope is a list.
		kList
	};
}

template <class Handler>
class GroupTextParser
{
public:
	~GroupTextParser();

	bool Parse(const std::string& input);

protected:
	GroupTextParser();

	uint32_t m_depth;
	uint32_t m_currentLine;
	bool m_expectParens;
	bool m_errorOccurred;
	uint32_t m_currentScopeType;

private:
	Handler& GetHandler();
	void ParseLine(boost::string_view s);
};

template <class Handler>
GroupTextParser<Handler>::~GroupTextParser()
{
}

template <class Handler>
bool GroupTextParser<Handler>::Parse(const std::string& input)
{
	typedef boost::tokenizer< boost::char_separator<char>> tokenizer;
	boost::char_separator<char> sep{ "\n" };
	tokenizer tok{ input, sep };
	for (auto& t : tok)
	{
#ifdef USE_GROUPTEXTTREE_DEBUG
		SysLog("Encountered line {0}", t.c_str());
#endif
		++m_currentLine;
		ParseLine(t);
	}

	return !m_errorOccurred;
}

template <class Handler>
GroupTextParser<Handler>::GroupTextParser(): m_depth(0), m_currentLine(0), m_expectParens(false), m_errorOccurred(false), m_currentScopeType(GroupTextScope::kGroup)
{
	// ctor
}

template <class Handler>
Handler& GroupTextParser<Handler>::GetHandler()
{
	return static_cast<Handler&>(*this);
}

template <class Handler>
void GroupTextParser<Handler>::ParseLine(boost::string_view s)
{
	std::string sb = s.to_string();
	boost::trim(sb);

	// Filter empty lines and comments early on.
	if (sb.empty() || sb.front() == '#')
		return;

	if (sb.front() == '{') 
	{
		if (m_expectParens) 
		{
			++m_depth;
			m_expectParens = false;
		}
		else 
		{
			GetHandler().OnError("unexpected '{'");
		}

		return;
	}
	else if (sb.front() == '}') 
	{
		if (m_depth != 0)
		{
			--m_depth;

			if (m_currentScopeType == GroupTextScope::kList)
				m_currentScopeType = GroupTextScope::kGroup;

			GetHandler().OnScopeClose();
		}
		else 
		{
			GetHandler().OnError("unexpected '}'");
		}

		return;
	}

	// Did we expect a '{' and didn't get one?
	if (m_expectParens) 
	{
		GetHandler().OnError("expected '{'");
		return;
	}

	//
	// List handling is fairly easy.
	// We just let our child-class do the parsing...
	//

	if (m_currentScopeType == GroupTextScope::kList) 
	{
		GetHandler().OnListItems(sb);
		return;
	}


	std::vector<std::string> line_parts;
	boost::erase_all(sb, "\"");
	boost::split(line_parts, sb, boost::is_any_of("\t "));


	const auto key = line_parts.at(0);
#ifdef USE_GROUPTEXTTREE_DEBUG
	ConsoleLog("Extracted argument {0}", key.c_str());
#endif
	if (key == "Group") 
	{
		GetHandler().OnScopeOpen(GroupTextScope::kGroup, line_parts.at(1));
		m_expectParens = true;
	}
	else if (key == "List") 
	{
		GetHandler().OnScopeOpen(GroupTextScope::kList, line_parts.at(1));

		m_expectParens = true;
		m_currentScopeType = GroupTextScope::kList;
	}
	else 
	{
		line_parts.erase(line_parts.begin());
		GetHandler().OnKeyValuePair(key, line_parts);
	}
}

template <class Parser>
bool LoadGroupTextFile(std::string filename, Parser& parser)
{
	std::ifstream t(filename);

	if (!t.good())
	{
		if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".mss")
		{
			// I don't care if the mss is not found.
		}
		else
		{
			SysLog("Failed to load '{0}'", filename.c_str());
		}
		return false;
	}
	std::string str;

	t.seekg(0, std::ios::end);
	str.reserve(size_t(t.tellg()));
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return parser.Parse(str);
}


