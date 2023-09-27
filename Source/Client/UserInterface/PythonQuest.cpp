#include "StdAfx.h"
#include "PythonQuest.h"

void CPythonQuest::RegisterQuestInstance(const SQuestInstance& c_rQuestInstance)
{
	DeleteQuestInstance(c_rQuestInstance.dwIndex);
	m_QuestInstanceContainer.push_back(c_rQuestInstance);

	/////

	SQuestInstance& rQuestInstance = *m_QuestInstanceContainer.rbegin();
	rQuestInstance.iStartTime = int32_t(CTimer::GetInstance()->GetCurrentSecond());
}

struct FQuestInstanceCompare
{
	uint32_t dwSearchIndex;
	FQuestInstanceCompare(uint32_t dwIndex) : dwSearchIndex(dwIndex) {}
	bool operator () (const CPythonQuest::SQuestInstance& rQuestInstance)
	{
		return dwSearchIndex == rQuestInstance.dwIndex;
	}
};

void CPythonQuest::DeleteQuestInstance(uint32_t dwIndex)
{
	TQuestInstanceContainer::iterator itor = std::find_if(m_QuestInstanceContainer.begin(), m_QuestInstanceContainer.end(), FQuestInstanceCompare(dwIndex));
	if (itor == m_QuestInstanceContainer.end())
		return;

	m_QuestInstanceContainer.erase(itor);
}

bool CPythonQuest::IsQuest(uint32_t dwIndex)
{
	TQuestInstanceContainer::iterator itor = std::find_if(m_QuestInstanceContainer.begin(), m_QuestInstanceContainer.end(), FQuestInstanceCompare(dwIndex));
	return itor != m_QuestInstanceContainer.end();
}

void CPythonQuest::MakeQuest(uint32_t dwIndex)
{
	DeleteQuestInstance(dwIndex);
	m_QuestInstanceContainer.push_back(SQuestInstance());

	/////

	SQuestInstance& rQuestInstance = *m_QuestInstanceContainer.rbegin();
	rQuestInstance.dwIndex = dwIndex;
	rQuestInstance.iStartTime = int32_t(CTimer::GetInstance()->GetCurrentSecond());
}

void CPythonQuest::SetQuestTitle(uint32_t dwIndex, const char* c_szTitle)
{
	SQuestInstance* pQuestInstance;
	if (!__GetQuestInstancePtr(dwIndex, &pQuestInstance))
		return;

	pQuestInstance->strTitle = c_szTitle;
}

void CPythonQuest::SetQuestClockName(uint32_t dwIndex, const char* c_szClockName)
{
	SQuestInstance* pQuestInstance;
	if (!__GetQuestInstancePtr(dwIndex, &pQuestInstance))
		return;

	pQuestInstance->strClockName = c_szClockName;
}

void CPythonQuest::SetQuestCounterName(uint32_t dwIndex, const char* c_szCounterName)
{
	SQuestInstance* pQuestInstance;
	if (!__GetQuestInstancePtr(dwIndex, &pQuestInstance))
		return;

	pQuestInstance->strCounterName = c_szCounterName;
}

void CPythonQuest::SetQuestClockValue(uint32_t dwIndex, int32_t iClockValue)
{
	SQuestInstance* pQuestInstance;
	if (!__GetQuestInstancePtr(dwIndex, &pQuestInstance))
		return;

	pQuestInstance->iClockValue = iClockValue;
	pQuestInstance->iStartTime = int32_t(CTimer::GetInstance()->GetCurrentSecond());
}

void CPythonQuest::SetQuestCounterValue(uint32_t dwIndex, int32_t iCounterValue)
{
	SQuestInstance* pQuestInstance;
	if (!__GetQuestInstancePtr(dwIndex, &pQuestInstance))
		return;

	pQuestInstance->iCounterValue = iCounterValue;
}

void CPythonQuest::SetQuestIconFileName(uint32_t dwIndex, const char* c_szIconFileName)
{
	SQuestInstance* pQuestInstance;
	if (!__GetQuestInstancePtr(dwIndex, &pQuestInstance))
		return;

	pQuestInstance->strIconFileName = c_szIconFileName;
}

int32_t CPythonQuest::GetQuestCount()
{
	return m_QuestInstanceContainer.size();
}

bool CPythonQuest::GetQuestInstancePtr(uint32_t dwArrayIndex, SQuestInstance** ppQuestInstance)
{
	if (dwArrayIndex >= m_QuestInstanceContainer.size())
		return false;

	*ppQuestInstance = &m_QuestInstanceContainer[dwArrayIndex];

	return true;
}

bool CPythonQuest::__GetQuestInstancePtr(uint32_t dwQuestIndex, SQuestInstance** ppQuestInstance)
{
	TQuestInstanceContainer::iterator itor = std::find_if(m_QuestInstanceContainer.begin(), m_QuestInstanceContainer.end(), FQuestInstanceCompare(dwQuestIndex));
	if (itor == m_QuestInstanceContainer.end())
		return false;

	*ppQuestInstance = &(*itor);

	return true;
}

void CPythonQuest::__Initialize()
{
	/*
	#ifdef _DEBUG
		for (int32_t i = 0; i < 7; ++i)
		{
			SQuestInstance test;
			test.dwIndex = i;
			test.strIconFileName = "";
			test.strTitle = _getf("test%d", i);
			test.strClockName = "���� �ð�";
			test.strCounterName = "���� ������";
			test.iClockValue = 1000;
			test.iCounterValue = 1000;
			test.iStartTime = 0;
			RegisterQuestInstance(test);
		}
	#endif
	*/
}

void CPythonQuest::Clear()
{
	m_QuestInstanceContainer.clear();
}

CPythonQuest::CPythonQuest()
{
	__Initialize();
}

CPythonQuest::~CPythonQuest()
{
	Clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

static int32_t questGetQuestCount()
{
	return  CPythonQuest::GetInstance()->GetQuestCount();
}

static std::tuple<std::string,CGraphicImage*,std::string,int32_t> questGetQuestData(uint32_t iIndex)
{

	CPythonQuest::SQuestInstance * pQuestInstance;
	if (!CPythonQuest::GetInstance()->GetQuestInstancePtr(iIndex, &pQuestInstance))
		throw std::runtime_error("Failed to find quest by index " + std::to_string(iIndex));

	CGraphicImage * pImage = NULL;
	if (!pQuestInstance->strIconFileName.empty())
	{
		std::string strIconFileName;
		strIconFileName = "d:/ymir work/ui/game/quest/questicon/";
		strIconFileName += pQuestInstance->strIconFileName.c_str();
		pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(strIconFileName.c_str());
	}
	else
	{
		{
			// ������� ��� ����Ʈ �̹����� �ִ´�.
			std::string strIconFileName = "season1/icon/scroll_open.tga";
			pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(strIconFileName.c_str());
		}
	}

	return std::make_tuple(pQuestInstance->strTitle,
		pImage,
		pQuestInstance->strCounterName,
		pQuestInstance->iCounterValue);

}

static uint32_t questGetQuestIndex(uint32_t iIndex)
{

	CPythonQuest::SQuestInstance * pQuestInstance;
	if (!CPythonQuest::GetInstance()->GetQuestInstancePtr(iIndex, &pQuestInstance))
		throw std::runtime_error("Failed to find quest by index " + std::to_string(iIndex));

	return  pQuestInstance->dwIndex;
}

static std::tuple<std::string,int32_t> questGetQuestLastTime(uint32_t iIndex)
{

	CPythonQuest::SQuestInstance * pQuestInstance;
	if (!CPythonQuest::GetInstance()->GetQuestInstancePtr(iIndex, &pQuestInstance))
		throw std::runtime_error("Failed to find quest by index " + std::to_string(iIndex));

	int32_t iLastTime = 0;

	if (pQuestInstance->iClockValue >= 0)
	{
		iLastTime = (pQuestInstance->iStartTime + pQuestInstance->iClockValue) - int32_t(CTimer::GetInstance()->GetCurrentSecond());
	}

	// �ð� ���� ó�� �ڵ�
//	else
//	{
//		iLastTime = int32_t(CTimer::GetInstance()->GetCurrentSecond()) - pQuestInstance->iStartTime;
//	}

	return std::make_tuple( pQuestInstance->strClockName, iLastTime);
}

static void questClear()
{
	CPythonQuest::GetInstance()->Clear();

}



PYBIND11_EMBEDDED_MODULE(quest, m)
{
	m.def("GetQuestCount",	questGetQuestCount);
	m.def("GetQuestData",	questGetQuestData);
	m.def("GetQuestIndex",	questGetQuestIndex);
	m.def("GetQuestLastTime",	questGetQuestLastTime);
	m.def("Clear",	questClear);

	m.attr("QUEST_MAX_NUM") =	5;
}
