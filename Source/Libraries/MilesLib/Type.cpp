#include "StdAfx.h"
#include "Type.h"
#include "../EterLib/TextFileLoader.h"

std::string strResult;

const char* NSound::GetResultString()
{
	return strResult.c_str();
}

void NSound::SetResultString(const char* c_pszStr)
{
	strResult.assign(c_pszStr);
}

bool NSound::LoadSoundInformationPiece(const char* c_szFileName, NSound::TSoundDataVector& rSoundDataVector, const char* c_szPathHeader)
{
	std::string strResult;
	strResult = c_szFileName;

	CTextFileLoader* pkTextFileLoader = CTextFileLoader::Cache(c_szFileName);
	if (!pkTextFileLoader)
		return false;

	CTextFileLoader& rkTextFileLoader = *pkTextFileLoader;
	if (rkTextFileLoader.IsEmpty())
	{
		SetResultString((strResult + " Unable to open file for reading").c_str());
		return false;
	}

	rkTextFileLoader.SetTop();

	int32_t iCount;
	if (!rkTextFileLoader.GetTokenInteger("sounddatacount", &iCount))
	{
		SetResultString((strResult + " File Format Error, SoundDataCount Not Found").c_str());
		return false;
	}

	rSoundDataVector.clear();
	rSoundDataVector.resize(iCount);

	char szSoundDataHeader[32 + 1];
	for (uint32_t i = 0; i < rSoundDataVector.size(); ++i)
	{
		_snprintf_s(szSoundDataHeader, sizeof(szSoundDataHeader), "sounddata%02d", i);
		CTokenVector* pTokenVector;
		if (!rkTextFileLoader.GetTokenVector(szSoundDataHeader, &pTokenVector))
		{
			SetResultString((strResult + "File format error: " + szSoundDataHeader + " not found").c_str());
			return false;
		}

		if (2 != pTokenVector->size())
		{
			SetResultString((strResult + "File format error: vector size not equal to 2").c_str());
			return false;
		}

		//storm::ParseNumber(pTokenVector->at(0), rSoundDataVector[i].fTime);
		rSoundDataVector[i].fTime = (float)atof(pTokenVector->at(0).c_str());
		if (c_szPathHeader)
		{
			rSoundDataVector[i].strSoundFileName = c_szPathHeader;
			rSoundDataVector[i].strSoundFileName += pTokenVector->at(1).c_str();
		}
		else
		{
			rSoundDataVector[i].strSoundFileName = pTokenVector->at(1).c_str();
		}
	}

	SetResultString((strResult + "Call up").c_str());
	return true;
}

void NSound::DataToInstance(const NSound::TSoundDataVector& c_rSoundDataVector, NSound::TSoundInstanceVector* pSoundInstanceVector)
{
	if (c_rSoundDataVector.empty())
		return;

	uint32_t dwFPS = 60;
	const float c_fFrameTime = 1.0f / float(dwFPS);

	pSoundInstanceVector->clear();
	pSoundInstanceVector->resize(c_rSoundDataVector.size());
	for (uint32_t i = 0; i < c_rSoundDataVector.size(); ++i)
	{
		const NSound::TSoundData& c_rSoundData = c_rSoundDataVector[i];
		NSound::TSoundInstance& rSoundInstance = pSoundInstanceVector->at(i);

		rSoundInstance.dwFrame = (uint32_t)(c_rSoundData.fTime / c_fFrameTime);
		rSoundInstance.strSoundFileName = c_rSoundData.strSoundFileName;
	}
}