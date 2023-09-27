#pragma once

#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <utility>
#include "utils.h"

class SpamManager : public Singleton<SpamManager>
{
	public:
		inline const char* GetSpamScore(const char* src, size_t len, uint32_t & score)
		{
			const char* uint16_t = nullptr;
			score = 0;

			std::string strOrig(src);
			strOrig.erase(remove_if(strOrig.begin(), strOrig.end(), isspace), strOrig.end());

			for (size_t i = 0; i < m_vec_word.size(); ++i)
			{
				std::pair<std::string, uint32_t>& r = m_vec_word[i];

				if (WildCaseCmp(r.first.c_str(), strOrig.c_str()))
				{
					uint16_t = r.first.c_str();
					score += r.second;
				}
			}

			return uint16_t;
		}

		inline void Clear()
		{
			m_vec_word.clear();
		}

		inline void Insert(const char* str, uint32_t score = 10)
		{
			m_vec_word.push_back(std::make_pair(str, score));
			PyLog("SPAM: %2d {}", score, str);
		}

	private:
		std::vector< std::pair<std::string, uint32_t> > m_vec_word;
};

