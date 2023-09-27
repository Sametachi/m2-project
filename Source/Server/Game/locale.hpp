#pragma once

extern "C"
{
	void locale_init(const char* filename); 
	const char* locale_find(const char* string);

	extern int32_t g_iUseLocale;
};
