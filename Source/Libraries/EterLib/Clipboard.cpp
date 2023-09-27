#include "stdafx.h"
#include "Clipboard.h"
#include <boost/system/error_code.hpp>
#include <vector>
#include <Storm/UnicodeUtil.hpp>

namespace
{
	struct ScopedClipboard
	{
		ScopedClipboard(): CanClip(OpenClipboard(GetActiveWindow()))
		{
			if (!CanClip)
				SysLog("OpenClipboard() failed with {0}", boost::system::system_category().message(GetLastError()));
		}
	
		~ScopedClipboard()
		{
			if (CanClip)
				CloseClipboard();
		}
	
		bool CanClip;
	};

	template <typename T>
	struct ScopedGlobal
	{
		ScopedGlobal(HGLOBAL global): global(global), ptr(static_cast<T*>(GlobalLock(global)))
		{
			if (!ptr) 
			{
				SysLog("GlobalLock({0}) failed with {1}", global, boost::system::system_category().message(GetLastError()));
			}
		}
	
		~ScopedGlobal()
		{
			if (ptr && !GlobalUnlock(global)) 
			{
				const auto error = GetLastError();
				if (error != NO_ERROR) 
				{
					SysLog("GlobalUnlock({0}) failed with {1}", global, boost::system::system_category().message(error));
				}
			}
		}
	
		HGLOBAL global;
		T* ptr;
	};
}

ClipboardType YITSORA_CF;

std::string GetClipboardText()
{
	ScopedClipboard clipboard;
	if (!clipboard.CanClip)
		return std::string();

	HANDLE data = GetClipboardData(CF_UNICODETEXT);
	if (data == nullptr)
		return std::string();

	ScopedGlobal<wchar_t> global(data);
	if (!global.ptr)
		return std::string();

	size_t len = wcslen(global.ptr);
	std::string str;
	str.resize(len * 2); // UTF-16 -> UTF-8

	boost::system::error_code ec;
	len = storm::ConvertUtf16ToUtf8(global.ptr, global.ptr + len, &str[0], &str[0] + str.length(), ec);

	str.resize(len);
	return str;
}

std::pair<std::unique_ptr<uint8_t>, size_t> GetClipboardContent(ClipboardType type)
{
	ScopedClipboard clipboard;
	if (!clipboard.CanClip)
		return std::make_pair(nullptr, 0);

	HANDLE data = GetClipboardData(type);
	if (data) 
	{
		ScopedGlobal<void> global(data);
		if (global.ptr) 
		{
			size_t size = GlobalSize(data);

			if (size == 0) 
			{
				SysLog("GlobalSize() failed with {0}", boost::system::system_category().message(GetLastError()));
				return std::make_pair(nullptr,0);
			}

			std::unique_ptr<uint8_t> dst(new uint8_t[size]);
			memcpy(dst.get(), global.ptr, size);
			return std::make_pair(std::move(dst), size);
		}
	}

	return std::make_pair(nullptr, 0);
}

bool ClearClipboard()
{
	ScopedClipboard clipboard;
	if (!clipboard.CanClip)
		return false;

	if (!EmptyClipboard()) 
	{
		SysLog("EmptyClipboard() failed with {0}", boost::system::system_category().message(GetLastError()));
		return false;
	}

	return true;
}

bool SetClipboardContent(ClipboardType type, const void* srcdata, size_t len)
{
	ScopedClipboard clipboard;
	if (!clipboard.CanClip)
		return false;

	HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, len);

	if (!data) 
	{
		SysLog("GlobalAlloc() failed with {0}", boost::system::system_category().message(GetLastError()));
		return false;
	}

	ScopedGlobal<void> global(data);

	if (!global.ptr)
		return false;

	memcpy(global.ptr, srcdata, len);

	if (!SetClipboardData(type, data)) 
	{
		SysLog("SetClipboardData({0}) failed with {1}", data, boost::system::system_category().message(GetLastError()));
		return false;
	}

	return true;
}

bool SetClipboardText(const std::string& str)
{
	ScopedClipboard clipboard;
	if (!clipboard.CanClip)
		return false;

	size_t len = str.length();

	// The output string has to be smaller than |len|,
	// since our input is UTF-8 encoded and our output UTF-16,
	// but we can save a temporary buffer / an additional conversion
	// by simply using the string length here...
	HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (len + 1));

	if (!data) 
	{
		SysLog("GlobalAlloc() failed with {0}", boost::system::system_category().message(GetLastError()));
		return false;
	}

	{
		ScopedGlobal<wchar_t> global(data);
		if (!global.ptr)
			return false;

		boost::system::error_code ec;
		len = storm::ConvertUtf8ToUtf16(str.data(), str.data() + len, global.ptr, global.ptr + len, ec);

		if (ec) 
		{
			SysLog("Failed to encode clipboard data '{0}' with {1}", str, ec.message());
			return false;
		}

		// NUL terminator - required
		global.ptr[len] = 0;
	}

	if (!SetClipboardData(CF_UNICODETEXT, data)) 
	{
		SysLog("SetClipboardData({0}) failed with {1}", data, boost::system::system_category().message(GetLastError()));
		return false;
	}

	return true;
}

bool ClipboardInit()
{
	YITSORA_CF = RegisterClipboardFormat("YITSORA_ONLINE");

	if (YITSORA_CF == 0)
	{
		SysLog("RegisterClipboardFormat failed with {0}", boost::system::system_category().message(GetLastError()));
		return false;
	}

	return true;
}
