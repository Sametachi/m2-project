#include "StdAfx.h"
#include "PythonGuild.h"
#include "MarkManager.h"
#include "PythonPlayer.h"

std::map<uint32_t, uint32_t> g_GuildSkillSlotToIndexMap;

static const int32_t GUILD_LEVEL_MAX = 20;

static uint32_t INTERNATIONAL_GUILDEXP_LIST[GUILD_LEVEL_MAX + 1] =
{
	0,			// 0
	6000UL,		// 1
	18000UL,	// 2
	36000UL,	// 3
	64000UL,	// 4
	94000UL,	// 5
	130000UL,	// 6
	172000UL,	// 7
	220000UL,	// 8
	274000UL,	// 9
	334000UL,	// 10
	400000UL,	// 11
	600000UL,	// 12
	840000UL,	// 13
	1120000UL,	// 14
	1440000UL,	// 15
	1800000UL,	// 16
	2600000UL,	// 17
	3200000UL,	// 18
	4000000UL,	// 19		
	16800000UL	// 20		
};


void CPythonGuild::EnableGuild()
{
	m_bGuildEnable = TRUE;
}

void CPythonGuild::SetGuildMoney(uint32_t dwMoney)
{
	m_GuildInfo.dwGuildMoney = dwMoney;
}

void CPythonGuild::SetGuildEXP(uint8_t byLevel, uint32_t dwEXP)
{
	m_GuildInfo.dwGuildLevel = byLevel;
	m_GuildInfo.dwCurrentExperience = dwEXP;
}

void CPythonGuild::SetGradeData(uint8_t byGradeNumber, TGuildGradeData& rGuildGradeData)
{
	m_GradeDataMap[byGradeNumber] = rGuildGradeData;
}

void CPythonGuild::SetGradeName(uint8_t byGradeNumber, const char* c_szName)
{
	if (!__IsGradeData(byGradeNumber))
		return;

	TGuildGradeData& rGradeData = m_GradeDataMap.find(byGradeNumber)->second;
	rGradeData.strName = c_szName;
}

void CPythonGuild::SetGradeAuthority(uint8_t byGradeNumber, uint8_t byAuthority)
{
	if (!__IsGradeData(byGradeNumber))
		return;

	TGuildGradeData& rGradeData = m_GradeDataMap.find(byGradeNumber)->second;
	rGradeData.byAuthorityFlag = byAuthority;
}

void CPythonGuild::ClearComment()
{
	m_GuildBoardCommentVector.clear();
}

void CPythonGuild::RegisterComment(uint32_t dwCommentID, const char* c_szName, const char* c_szComment)
{
	if (0 == strlen(c_szComment))
		return;

	TGuildBoardCommentData CommentData;
	CommentData.dwCommentID = dwCommentID;
	CommentData.strName = c_szName;
	CommentData.strComment = c_szComment;

	m_GuildBoardCommentVector.push_back(CommentData);
}

void CPythonGuild::RegisterMember(TGuildMemberData& rGuildMemberData)
{
	TGuildMemberData* pGuildMemberData;
	if (GetMemberDataPtrByPID(rGuildMemberData.dwPID, &pGuildMemberData))
	{
		pGuildMemberData->byGeneralFlag = rGuildMemberData.byGeneralFlag;
		pGuildMemberData->byGrade = rGuildMemberData.byGrade;
		pGuildMemberData->byLevel = rGuildMemberData.byLevel;
		pGuildMemberData->dwOffer = rGuildMemberData.dwOffer;
	}
	else
	{
		m_GuildMemberDataVector.push_back(rGuildMemberData);
	}

	__CalculateLevelAverage();
	__SortMember();
}

struct CPythonGuild_FFindGuildMemberByPID
{
	CPythonGuild_FFindGuildMemberByPID(uint32_t dwSearchingPID_) : dwSearchingPID(dwSearchingPID_) {}
	int32_t operator () (CPythonGuild::TGuildMemberData& rGuildMemberData)
	{
		return rGuildMemberData.dwPID == dwSearchingPID;
	}

	uint32_t dwSearchingPID;
};

struct CPythonGuild_FFindGuildMemberByName
{
	CPythonGuild_FFindGuildMemberByName(const char* c_szSearchingName) : strSearchingName(c_szSearchingName) {}
	int32_t operator () (CPythonGuild::TGuildMemberData& rGuildMemberData)
	{
		return 0 == strSearchingName.compare(rGuildMemberData.strName.c_str());
	}

	std::string strSearchingName;
};

void CPythonGuild::ChangeGuildMemberGrade(uint32_t dwPID, uint8_t byGrade)
{
	TGuildMemberData* pGuildMemberData;
	if (!GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		return;

	pGuildMemberData->byGrade = byGrade;
}

void CPythonGuild::ChangeGuildMemberGeneralFlag(uint32_t dwPID, uint8_t byFlag)
{
	TGuildMemberData* pGuildMemberData;
	if (!GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		return;

	pGuildMemberData->byGeneralFlag = byFlag;
}

void CPythonGuild::RemoveMember(uint32_t dwPID)
{
	TGuildMemberDataVector::iterator itor;
	itor = std::find_if(m_GuildMemberDataVector.begin(),
		m_GuildMemberDataVector.end(),
		CPythonGuild_FFindGuildMemberByPID(dwPID));

	if (m_GuildMemberDataVector.end() == itor)
		return;

	m_GuildMemberDataVector.erase(itor);
}

void CPythonGuild::RegisterGuildName(uint32_t dwID, const char* c_szName)
{
	m_GuildNameMap.insert(make_pair(dwID, std::string(c_szName)));
}

BOOL CPythonGuild::IsMainPlayer(uint32_t dwPID)
{
	TGuildMemberData* pGuildMemberData;
	if (!GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		return FALSE;

	auto rPlayer = CPythonPlayer::GetInstance();
	return 0 == pGuildMemberData->strName.compare(rPlayer->GetName());
}

BOOL CPythonGuild::IsGuildEnable()
{
	return m_bGuildEnable;
}

CPythonGuild::TGuildInfo& CPythonGuild::GetGuildInfoRef()
{
	return m_GuildInfo;
}

BOOL CPythonGuild::GetGradeDataPtr(uint32_t dwGradeNumber, TGuildGradeData** ppData)
{
	TGradeDataMap::iterator itor = m_GradeDataMap.find(dwGradeNumber);
	if (m_GradeDataMap.end() == itor)
		return FALSE;

	*ppData = &(itor->second);

	return TRUE;
}

const CPythonGuild::TGuildBoardCommentDataVector& CPythonGuild::GetGuildBoardCommentVector()
{
	return m_GuildBoardCommentVector;
}

uint32_t CPythonGuild::GetMemberCount()
{
	return m_GuildMemberDataVector.size();
}

BOOL CPythonGuild::GetMemberDataPtr(uint32_t dwIndex, TGuildMemberData** ppData)
{
	if (dwIndex >= m_GuildMemberDataVector.size())
		return FALSE;

	*ppData = &m_GuildMemberDataVector[dwIndex];

	return TRUE;
}

BOOL CPythonGuild::GetMemberDataPtrByPID(uint32_t dwPID, TGuildMemberData** ppData)
{
	TGuildMemberDataVector::iterator itor;
	itor = std::find_if(m_GuildMemberDataVector.begin(),
		m_GuildMemberDataVector.end(),
		CPythonGuild_FFindGuildMemberByPID(dwPID));

	if (m_GuildMemberDataVector.end() == itor)
		return FALSE;

	*ppData = &(*itor);
	return TRUE;
}

BOOL CPythonGuild::GetMemberDataPtrByName(const char* c_szName, TGuildMemberData** ppData)
{
	TGuildMemberDataVector::iterator itor;
	itor = std::find_if(m_GuildMemberDataVector.begin(),
		m_GuildMemberDataVector.end(),
		CPythonGuild_FFindGuildMemberByName(c_szName));

	if (m_GuildMemberDataVector.end() == itor)
		return FALSE;

	*ppData = &(*itor);
	return TRUE;
}

uint32_t CPythonGuild::GetGuildMemberLevelSummary()
{
	return m_dwMemberLevelSummary;
}

uint32_t CPythonGuild::GetGuildMemberLevelAverage()
{
	return m_dwMemberLevelAverage;
}

uint32_t CPythonGuild::GetGuildExperienceSummary()
{
	return m_dwMemberExperienceSummary;
}

CPythonGuild::TGuildSkillData& CPythonGuild::GetGuildSkillDataRef()
{
	return m_GuildSkillData;
}

bool CPythonGuild::GetGuildName(uint32_t dwID, std::string* pstrGuildName)
{
	if (m_GuildNameMap.end() == m_GuildNameMap.find(dwID))
		return false;

	*pstrGuildName = m_GuildNameMap[dwID];

	return true;
}

uint32_t CPythonGuild::GetGuildID()
{
	return m_GuildInfo.dwGuildID;
}

BOOL CPythonGuild::HasGuildLand()
{
	return m_GuildInfo.bHasLand;
}

void CPythonGuild::StartGuildWar(uint32_t dwEnemyGuildID)
{
	int32_t i;

	for (i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (dwEnemyGuildID == m_adwEnemyGuildID[i])
			return;

	for (i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (0 == m_adwEnemyGuildID[i])
		{
			m_adwEnemyGuildID[i] = dwEnemyGuildID;
			break;
		}
}

void CPythonGuild::EndGuildWar(uint32_t dwEnemyGuildID)
{
	for (int32_t i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (dwEnemyGuildID == m_adwEnemyGuildID[i])
			m_adwEnemyGuildID[i] = 0;
}

uint32_t CPythonGuild::GetEnemyGuildID(uint32_t dwIndex)
{
	if (dwIndex >= ENEMY_GUILD_SLOT_MAX_COUNT)
		return 0;

	return m_adwEnemyGuildID[dwIndex];
}

BOOL CPythonGuild::IsDoingGuildWar()
{
	for (int32_t i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (0 != m_adwEnemyGuildID[i])
		{
			return TRUE;
		}

	return FALSE;
}

void CPythonGuild::__CalculateLevelAverage()
{
	m_dwMemberLevelSummary = 0;
	m_dwMemberLevelAverage = 0;
	m_dwMemberExperienceSummary = 0;

	if (m_GuildMemberDataVector.empty())
		return;

	TGuildMemberDataVector::iterator itor;

	// Sum Level & Experience
	itor = m_GuildMemberDataVector.begin();
	for (; itor != m_GuildMemberDataVector.end(); ++itor)
	{
		TGuildMemberData& rGuildMemberData = *itor;
		m_dwMemberLevelSummary += rGuildMemberData.byLevel;
		m_dwMemberExperienceSummary += rGuildMemberData.dwOffer;
	}

	assert(!m_GuildMemberDataVector.empty());
	m_dwMemberLevelAverage = m_dwMemberLevelSummary / m_GuildMemberDataVector.size();
}

struct CPythonGuild_SLessMemberGrade
{
	bool operator() (CPythonGuild::TGuildMemberData& rleft, CPythonGuild::TGuildMemberData& rright)
	{
		if (rleft.byGrade < rright.byGrade)
			return true;

		return false;
	}
};

void CPythonGuild::__SortMember()
{
	std::sort(m_GuildMemberDataVector.begin(), m_GuildMemberDataVector.end(), CPythonGuild_SLessMemberGrade());
}

BOOL CPythonGuild::__IsGradeData(uint8_t byGradeNumber)
{
	return m_GradeDataMap.end() != m_GradeDataMap.find(byGradeNumber);
}

void CPythonGuild::__Initialize()
{
	ZeroMemory(&m_GuildInfo, sizeof(m_GuildInfo));
	ZeroMemory(&m_GuildSkillData, sizeof(m_GuildSkillData));
	ZeroMemory(&m_adwEnemyGuildID, ENEMY_GUILD_SLOT_MAX_COUNT * sizeof(uint32_t));
	m_GradeDataMap.clear();
	m_GuildMemberDataVector.clear();
	m_dwMemberLevelSummary = 0;
	m_dwMemberLevelAverage = 0;
	m_bGuildEnable = FALSE;
	m_GuildNameMap.clear();
}

void CPythonGuild::Destroy()
{
	__Initialize();
}

CPythonGuild::CPythonGuild()
{
	__Initialize();
}
CPythonGuild::~CPythonGuild()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


static BOOL guildIsGuildEnable()
{
	return  CPythonGuild::GetInstance()->IsGuildEnable();
}

static uint32_t guildGuildIDToMarkID(uint32_t guild_id)
{

	return  CGuildMarkManager::GetInstance()->GetMarkID(guild_id);
}

static std::string guildGetMarkImageFilenameByMarkID(uint32_t markID)
{
	std::string imagePath;
	CGuildMarkManager::GetInstance()->GetMarkImageFilename(markID / CGuildMarkImage::MARK_TOTAL_COUNT, imagePath);
	return  imagePath;
}

static uint32_t guildGetMarkIndexByMarkID(uint32_t markID)
{
	return  markID % CGuildMarkImage::MARK_TOTAL_COUNT;
}

static uint32_t guildGetGuildID()
{
	return  CPythonGuild::GetInstance()->GetGuildID();
}

static BOOL guildHasGuildLand()
{
	return  CPythonGuild::GetInstance()->HasGuildLand();
}

static std::string guildGetGuildName(uint32_t iGuildID)
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::GetInstance()->GetGuildInfoRef();
	return  rGuildInfo.szGuildName;
}

static std::string guildGetGuildMasterName()
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::GetInstance()->GetGuildInfoRef();

	CPythonGuild::TGuildMemberData * pData;
	if (!CPythonGuild::GetInstance()->GetMemberDataPtrByPID(rGuildInfo.dwMasterPID, &pData))
		return  "Noname";

	return  pData->strName;
}

static std::string guildGetEnemyGuildName(uint32_t iIndex)
{

	uint32_t dwEnemyGuildID = CPythonGuild::GetInstance()->GetEnemyGuildID(iIndex);

	std::string strEnemyGuildName;
	if (!CPythonGuild::GetInstance()->GetGuildName(dwEnemyGuildID, &strEnemyGuildName))
		return  "";

	return  strEnemyGuildName;
}

static uint32_t guildGetGuildMoney()
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::GetInstance()->GetGuildInfoRef();
	return  rGuildInfo.dwGuildMoney;
}

static size_t guildGetGuildBoardCommentCount()
{
	const CPythonGuild::TGuildBoardCommentDataVector & rCommentVector = CPythonGuild::GetInstance()->GetGuildBoardCommentVector();
	return  rCommentVector.size();
}

static std::tuple<uint32_t, std::string, std::string> guildGetGuildBoardCommentData(size_t iIndex)
{

	const CPythonGuild::TGuildBoardCommentDataVector & c_rCommentVector = CPythonGuild::GetInstance()->GetGuildBoardCommentVector();
	if (iIndex >= c_rCommentVector.size())
		return std::make_tuple( 0, "Noname", "Noname");

	const CPythonGuild::TGuildBoardCommentData & c_rData = c_rCommentVector[iIndex];

	return std::make_tuple( c_rData.dwCommentID, c_rData.strName, c_rData.strComment);
}

static uint32_t guildGetGuildLevel()
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::GetInstance()->GetGuildInfoRef();
	return  rGuildInfo.dwGuildLevel;
}

static std::tuple<uint32_t, uint32_t> guildGetGuildExperience()
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::GetInstance()->GetGuildInfoRef();

	int32_t GULID_MAX_LEVEL = 20;
	if (rGuildInfo.dwGuildLevel >= GULID_MAX_LEVEL)
		return std::make_tuple(0U, 0U);

	if (rGuildInfo.dwGuildLevel < 0 && rGuildInfo.dwGuildLevel >= GUILD_LEVEL_MAX)
		return std::make_tuple(rGuildInfo.dwCurrentExperience, 0);

	return std::make_tuple( rGuildInfo.dwCurrentExperience, INTERNATIONAL_GUILDEXP_LIST[rGuildInfo.dwGuildLevel] - rGuildInfo.dwCurrentExperience);
}

static std::tuple<uint32_t, uint32_t> guildGetGuildMemberCount()
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::GetInstance()->GetGuildInfoRef();
	return std::make_tuple( rGuildInfo.dwCurrentMemberCount, rGuildInfo.dwMaxMemberCount);
}

static uint32_t guildGetGuildMemberLevelSummary()
{
	return  CPythonGuild::GetInstance()->GetGuildMemberLevelSummary();
}

static uint32_t guildGetGuildMemberLevelAverage()
{
	return  CPythonGuild::GetInstance()->GetGuildMemberLevelAverage();
}

static uint32_t guildGetGuildExperienceSummary()
{
	return  CPythonGuild::GetInstance()->GetGuildExperienceSummary();
}

static uint8_t guildGetGuildSkillPoint()
{
	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::GetInstance()->GetGuildSkillDataRef();
	return  c_rSkillData.bySkillPoint;
}

static std::tuple<uint16_t, uint16_t> guildGetDragonPowerPoint()
{
	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::GetInstance()->GetGuildSkillDataRef();
	return std::make_tuple( c_rSkillData.wGuildPoint, c_rSkillData.wMaxGuildPoint);
}

static uint8_t guildGetGuildSkillLevel(size_t iSkillIndex)
{
	assert(FALSE && !"guildGetGuildSkillLevel - ������� �ʴ� �Լ��Դϴ�.");

	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::GetInstance()->GetGuildSkillDataRef();
	return  c_rSkillData.bySkillLevel[iSkillIndex];
}

static uint8_t guildGetSkillLevel(uint32_t iSlotIndex)
{

	std::map<uint32_t, uint32_t>::iterator itor = g_GuildSkillSlotToIndexMap.find(iSlotIndex);

	if (g_GuildSkillSlotToIndexMap.end() == itor)
		return 0;

	uint32_t dwSkillIndex = itor->second;
	assert(dwSkillIndex < CPythonGuild::GUILD_SKILL_MAX_NUM);

	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::GetInstance()->GetGuildSkillDataRef();
	return  c_rSkillData.bySkillLevel[dwSkillIndex];
}

static uint8_t guildGetSkillMaxLevelNew(uint32_t iSlotIndex)
{

	std::map<uint32_t, uint32_t>::iterator itor = g_GuildSkillSlotToIndexMap.find(iSlotIndex);

	if (g_GuildSkillSlotToIndexMap.end() == itor)
		return  0;

	uint32_t dwSkillIndex = itor->second;
	assert(dwSkillIndex < CPythonGuild::GUILD_SKILL_MAX_NUM);

	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::GetInstance()->GetGuildSkillDataRef();
	return  c_rSkillData.bySkillLevel[dwSkillIndex];
}

static void guildSetSkillIndex(uint32_t iSlotIndex, uint32_t iSkillIndex)
{

	g_GuildSkillSlotToIndexMap.insert(std::make_pair(iSlotIndex, iSkillIndex));


}

static uint32_t guildGetSkillIndex(uint32_t iSlotIndex)
{

	std::map<uint32_t, uint32_t>::iterator itor = g_GuildSkillSlotToIndexMap.find(iSlotIndex);

	if (g_GuildSkillSlotToIndexMap.end() == itor)
		return  0;

	uint32_t dwSkillIndex = itor->second;
	return  dwSkillIndex;
}

static std::tuple<std::string, uint8_t> guildGetGradeData(uint32_t iGradeNumber)
{

	CPythonGuild::TGuildGradeData * pData;
	if (!CPythonGuild::GetInstance()->GetGradeDataPtr(iGradeNumber, &pData))
		return std::make_tuple( "?", 0);

	return std::make_tuple( pData->strName, pData->byAuthorityFlag);
}

static std::string guildGetGradeName(uint32_t iGradeNumber)
{

	CPythonGuild::TGuildGradeData * pData;
	if (!CPythonGuild::GetInstance()->GetGradeDataPtr(iGradeNumber, &pData))
		return  "?";

	return  pData->strName;
}

static uint32_t guildGetMemberCount()
{
	return  CPythonGuild::GetInstance()->GetMemberCount();
}

static std::tuple<int64_t,std::string,uint8_t,uint8_t,uint8_t, uint32_t,uint8_t> guildGetMemberData(uint32_t iIndex)
{

	CPythonGuild::TGuildMemberData * pData;
	if (!CPythonGuild::GetInstance()->GetMemberDataPtr(iIndex, &pData))
		return std::make_tuple( -1, "", 0U, 0U, 0U, 0U, 0U);

	return std::make_tuple( pData->dwPID, pData->strName, pData->byGrade, pData->byJob, pData->byLevel, pData->dwOffer, pData->byGeneralFlag);
}

static int64_t guildMemberIndexToPID(uint32_t iIndex)
{

	CPythonGuild::TGuildMemberData * pData;
	if (!CPythonGuild::GetInstance()->GetMemberDataPtr(iIndex, &pData))
		return  -1;

	return  pData->dwPID;
}

static BOOL guildIsMember(uint32_t iIndex)
{

	CPythonGuild::TGuildMemberData * pData;
	return CPythonGuild::GetInstance()->GetMemberDataPtr(iIndex, &pData);
}

static BOOL guildIsMemberByName(std::string szName)
{

	CPythonGuild::TGuildMemberData * pData;
	return CPythonGuild::GetInstance()->GetMemberDataPtrByName(szName.c_str(), &pData);
}

static bool guildMainPlayerHasAuthority(uint8_t iAuthority)
{

	auto rPlayer = CPythonPlayer::GetInstance();
	const char * c_szMainPlayerName = rPlayer->GetName();

	CPythonGuild::TGuildMemberData * pMemberData;
	if (!CPythonGuild::GetInstance()->GetMemberDataPtrByName(c_szMainPlayerName, &pMemberData))
		return false;

	CPythonGuild::TGuildGradeData * pGradeData;
	if (!CPythonGuild::GetInstance()->GetGradeDataPtr(pMemberData->byGrade, &pGradeData))
		return false;

	return  iAuthority == (pGradeData->byAuthorityFlag & iAuthority);
}

static void guildDestroy()
{
	CPythonGuild::GetInstance()->Destroy();
	g_GuildSkillSlotToIndexMap.clear();
}



PYBIND11_EMBEDDED_MODULE(guild, m)
{
	m.def("IsGuildEnable",	guildIsGuildEnable);
	m.def("GuildIDToMarkID",	guildGuildIDToMarkID);
	m.def("GetMarkImageFilenameByMarkID",	guildGetMarkImageFilenameByMarkID);
	m.def("GetMarkIndexByMarkID",	guildGetMarkIndexByMarkID);
	m.def("GetGuildID",	guildGetGuildID);
	m.def("HasGuildLand",	guildHasGuildLand);
	m.def("GetGuildName",	guildGetGuildName);
	m.def("GetGuildMasterName",	guildGetGuildMasterName);
	m.def("GetEnemyGuildName",	guildGetEnemyGuildName);
	m.def("GetGuildMoney",	guildGetGuildMoney);
	m.def("GetGuildBoardCommentCount",	guildGetGuildBoardCommentCount);
	m.def("GetGuildBoardCommentData",	guildGetGuildBoardCommentData);
	m.def("GetGuildLevel",	guildGetGuildLevel);
	m.def("GetGuildExperience",	guildGetGuildExperience);
	m.def("GetGuildMemberCount",	guildGetGuildMemberCount);
	m.def("GetGuildMemberLevelSummary",	guildGetGuildMemberLevelSummary);
	m.def("GetGuildMemberLevelAverage",	guildGetGuildMemberLevelAverage);
	m.def("GetGuildExperienceSummary",	guildGetGuildExperienceSummary);
	m.def("GetGuildSkillPoint",	guildGetGuildSkillPoint);
	m.def("GetDragonPowerPoint",	guildGetDragonPowerPoint);
	m.def("GetGuildSkillLevel",	guildGetGuildSkillLevel);
	m.def("GetSkillLevel",	guildGetSkillLevel);
	m.def("GetSkillMaxLevelNew",	guildGetSkillMaxLevelNew);
	m.def("SetSkillIndex",	guildSetSkillIndex);
	m.def("GetSkillIndex",	guildGetSkillIndex);
	m.def("GetGradeData",	guildGetGradeData);
	m.def("GetGradeName",	guildGetGradeName);
	m.def("GetMemberCount",	guildGetMemberCount);
	m.def("GetMemberData",	guildGetMemberData);
	m.def("MemberIndexToPID",	guildMemberIndexToPID);
	m.def("IsMember",	guildIsMember);
	m.def("IsMemberByName",	guildIsMemberByName);
	m.def("MainPlayerHasAuthority",	guildMainPlayerHasAuthority);
	m.def("Destroy",	guildDestroy);

	m.attr("AUTH_ADD_MEMBER") = int32_t(GUILD_AUTH_ADD_MEMBER);
	m.attr("AUTH_REMOVE_MEMBER") = int32_t(GUILD_AUTH_REMOVE_MEMBER);
	m.attr("AUTH_NOTICE") = int32_t(GUILD_AUTH_NOTICE);
	m.attr("AUTH_SKILL") = int32_t(GUILD_AUTH_USE_SKILL);
	m.attr("ENEMY_GUILD_SLOT_MAX_COUNT") = int32_t(CPythonGuild::ENEMY_GUILD_SLOT_MAX_COUNT);
}
