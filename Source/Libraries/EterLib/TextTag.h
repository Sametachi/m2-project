#pragma once
/*
	TextTag reimplementation
*/
enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
	TEXT_TAG_EMOTICON_START,
	TEXT_TAG_EMOTICON_END,
};

struct TextTag
{
	uint32_t type;
	uint32_t length;
	// Color equals: Hex value, Hyperlink equals item:12344566768678765675765
	std::string content;
};

bool GetTextTag(const std::string& str, TextTag& tag);
uint32_t FindTextTagEnd(const std::string& str, const TextTag& in);

// If |pos| is inside a text-tag, the result pair contains the text-tag's boundary positions, that is [first, second).
// Otherwise this function should return |pos| twice.
std::pair<uint32_t, uint32_t> GetTextTagBoundary(const std::string& str, uint32_t pos);
// Returns a string without text tags
std::string StripTextTags(const std::string& str);
