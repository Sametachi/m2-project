#pragma once

typedef std::map<std::string, std::string> TValueMap;

class CConfig : public Singleton<CConfig>
{
public:
		CConfig();
		~CConfig();

		bool LoadFile(const char* filename);
		bool GetValue(const char* key, int32_t* dest);
		bool GetValue(const char* key, float* dest);
		bool GetValue(const char* key, uint32_t* dest);
		bool GetValue(const char* key, uint8_t* dest);
		bool GetValue(const char* key, char* dest, size_t destSize);
		bool GetWord(FILE* fp, char* dest);
		bool GetLine(FILE* fp, char* dest);
		bool GetTwoValue(const char* key, uint32_t* dest1, uint32_t*dest2);
		void NextLine(FILE* fp);

private:
		void Destroy();
		bool GetParam(const char*key,int32_t index, uint32_t*Param);

		const char*	Get(const char* key);
		std::string *	Search(const char* key);

private:
		TValueMap	m_valueMap;
};
