#include "stdafx.h"
#include "TextTag.h"

bool GetTextTag(const std::string& str, TextTag& tag)
{
	if (str.length() < 2 || str[0] != '|')
		return false;

	// We want to have two
	tag.length = 2;

	switch (str[1]) 
	{
		// Color Case
		case 'c': 
		{
			if (str.length() < 10)
				return false;

			tag.type = TEXT_TAG_COLOR;
			tag.length += 8;
			tag.content = str.substr(2, 8);
			break;
		}
		case '|': 
		{
			tag.type = TEXT_TAG_TAG;
			break;
		}
		case 'r': 
		{
			tag.type = TEXT_TAG_RESTORE_COLOR;
			break;
		}
		// Emoticon Case
		case 'E': 
		{
			tag.type = TEXT_TAG_EMOTICON_START;

			const auto end = str.find("|e");
			if (end == std::string::npos)
				return false;

			tag.length = end + 2;
			tag.content = str.substr(2, end - 2);
			break;
		}
		case 'e': 
		{
			tag.type = TEXT_TAG_EMOTICON_END;
			break;
		}
		// Mighty Hyperlink case
		case 'H': 
		{
			tag.type = TEXT_TAG_HYPERLINK_START;

			const auto end = str.find("|h");
			if (end == std::string::npos)
				return false;

			tag.length = end + 2;
			tag.content = str.substr(2, end - 2);
			break;
		}
		case 'h': 
		{
			tag.type = TEXT_TAG_HYPERLINK_END;
			break;
		}
		default:
			return false;
	}

	return true;
}

uint32_t FindTextTagEnd(const std::string& str, const TextTag& in)
{
	for (std::size_t p = 0, l = str.size(); p != l; ) 
	{
		TextTag tag;
		if (!GetTextTag(str.substr(p), tag)) 
		{
			++p;
			continue;
		}

		p += tag.length;

		if (tag.type == TEXT_TAG_HYPERLINK_START || tag.type == TEXT_TAG_COLOR) 
		{
			p += FindTextTagEnd(str.substr(p), tag);
			continue;
		}

		if (in.type == TEXT_TAG_HYPERLINK_START && tag.type == TEXT_TAG_HYPERLINK_END)
			return p;

		if (in.type == TEXT_TAG_COLOR && tag.type == TEXT_TAG_RESTORE_COLOR)
			return p;
	}

	// Text tag
	return str.length();
}

std::pair<uint32_t, uint32_t> GetTextTagBoundary(const std::string& str, uint32_t pos)
{
	for (std::size_t p = 0, l = str.size(); p != l; ) 
	{
		TextTag tag;
		if (!GetTextTag(str.substr(p), tag)) 
		{
			++p;
			continue;
		}
		// Handle text-tags correctly
		if (tag.type == TEXT_TAG_HYPERLINK_START || tag.type == TEXT_TAG_COLOR) 
		{
			tag.length += FindTextTagEnd(str.substr(p + tag.length), tag);
		}

		if (pos >= p && pos < p + tag.length)
			return std::make_pair(p, p + tag.length);

		p += tag.length;
	}
	// Not inside a text-tag
	return std::make_pair(pos, pos);
}

std::string StripTextTags(const std::string& str)
{
	std::string res;
	for (std::size_t p = 0, l = str.size(); p != l; ) 
	{
		TextTag tag;
		if (GetTextTag(str.substr(p), tag)) 
		{
			p += tag.length;
			continue;
		}
		res.append(1, str[p++]);
	}
	return res;
}
