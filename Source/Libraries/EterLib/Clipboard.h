#pragma once
#include <boost/system/error_code.hpp>
#include <utility>
#include <memory>

typedef unsigned int ClipboardType;
extern ClipboardType YITSORA_CF;

std::string GetClipboardText();
std::pair<std::unique_ptr<uint8_t>, size_t> GetClipboardContent(ClipboardType type);

bool ClearClipboard();
bool SetClipboardContent(ClipboardType type, const void* data, size_t len);
bool SetClipboardText(const std::string& str);
bool ClipboardInit();