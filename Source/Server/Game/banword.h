#pragma once

#include <boost/unordered_map.hpp>

class CBanwordManager : public Singleton<CBanwordManager>
{
	public:
		CBanwordManager();
		virtual ~CBanwordManager();

		bool Initialize(TBanwordTable* p, uint16_t wSize);
		bool Find(const char* c_pszString);
		bool CheckString(const char* c_pszString, size_t _len);
		void ConvertString(char* c_pszString, size_t _len);

	protected:
		typedef boost::unordered_map<std::string, bool> TBanwordHashmap;
		TBanwordHashmap m_hashmap_words;
};
