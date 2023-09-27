#pragma once
#include <vector>

extern std::string strResult;

class NSound
{
public:
	typedef struct SSoundData
	{
		float fTime;
		std::string strSoundFileName;
	} TSoundData;

	typedef struct SSoundInstance
	{
		uint32_t dwFrame;
		std::string strSoundFileName;
	} TSoundInstance;

	using TSoundDataVector = std::vector<TSoundData>;
	using TSoundInstanceVector = std::vector<TSoundInstance>;

	static bool LoadSoundInformationPiece(const char* c_szFileName, NSound::TSoundDataVector& rSoundDataVector, const char* c_szPathHeader = nullptr);
	static void DataToInstance(const NSound::TSoundDataVector& c_rSoundDataVector, NSound::TSoundInstanceVector* pSoundInstanceVector);

	static const char* GetResultString();
	static void SetResultString(const char* c_pszStr);
};
