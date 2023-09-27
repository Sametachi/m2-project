#include "stdafx.h"
#include <sstream>

#include "utils.h"
#include "config.h"
#include "vector.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "desc.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "item.h"
#include "sectree_manager.h"
#include "mob_manager.h"
#include "start_position.h"
#include "party.h"
#include "buffer_manager.h"
#include "guild.h"
#include "log.h"
#include "unique_item.h"
#include "questmanager.h"

extern bool test_server;

static const uint32_t s_adwSubSkillVnums[] =
{
	SKILL_LEADERSHIP,
	SKILL_COMBO,
	SKILL_MINING,
	SKILL_LANGUAGE1,
	SKILL_LANGUAGE2,
	SKILL_LANGUAGE3,
	SKILL_POLYMORPH,
	SKILL_HORSE,
	SKILL_HORSE_SUMMON,
	SKILL_HORSE_WILDATTACK,
	SKILL_HORSE_CHARGE,
	SKILL_HORSE_ESCAPE,
	SKILL_HORSE_WILDATTACK_RANGE,
	SKILL_ADD_HP,
	SKILL_RESIST_PENETRATE
};

time_t CHARACTER::GetSkillNextReadTime(uint32_t dwVnum) const
{
	if (dwVnum >= SKILL_MAX_NUM)
	{
		SysLog("vnum overflow (vnum: {})", dwVnum);
		return 0;
	}

	return m_pSkillLevels ? m_pSkillLevels[dwVnum].tNextRead : 0;
}

void CHARACTER::SetSkillNextReadTime(uint32_t dwVnum, time_t time)
{
	if (m_pSkillLevels && dwVnum < SKILL_MAX_NUM)
		m_pSkillLevels[dwVnum].tNextRead = time;
}

bool TSkillUseInfo::HitOnce(uint32_t dwVnum)
{
	if (!bUsed)
		return false;

	TraceLog("__HitOnce NextUse {} current {} count {} scount {}", dwNextSkillUsableTime, get_dword_time(), iHitCount, iSplashCount);

	if (dwNextSkillUsableTime && dwNextSkillUsableTime<get_dword_time() && dwVnum != SKILL_MUYEONG && dwVnum != SKILL_HORSE_WILDATTACK)
	{
		TraceLog("__HitOnce can't hit");

		return false;
	}

	if (iHitCount == -1)
	{
		TraceLog("__HitOnce OK {} {} {}", dwNextSkillUsableTime, get_dword_time(), iHitCount);
		return true;
	}

	if (iHitCount)
	{
		TraceLog("__HitOnce OK {} {} {}", dwNextSkillUsableTime, get_dword_time(), iHitCount);
		iHitCount--;
		return true;
	}
	return false;
}



bool TSkillUseInfo::UseSkill(bool isGrandMaster, uint32_t vid, uint32_t dwCooltime, int32_t splashcount, int32_t hitcount, int32_t range)
{
	this->isGrandMaster = isGrandMaster;
	uint32_t dwCur = get_dword_time();

	// The cooldown isn't over yet.
	if (bUsed && dwNextSkillUsableTime > dwCur)
	{
		PyLog("cooltime is not over delta {}", dwNextSkillUsableTime - dwCur);
		iHitCount = 0;
		return false;
	}

	bUsed = true;

	if (dwCooltime)
		dwNextSkillUsableTime = dwCur + dwCooltime;
	else
		dwNextSkillUsableTime = 0;

	iRange = range;
	iMaxHitCount = iHitCount = hitcount;

	if (test_server)
		PyLog("UseSkill NextUse {}  current {} cooltime {} hitcount {}/{}", dwNextSkillUsableTime, dwCur, dwCooltime, iHitCount, iMaxHitCount);

	dwVID = vid;
	iSplashCount = splashcount;
	return true;
}

int32_t CHARACTER::GetChainLightningMaxCount() const
{ 
	return aiChainLightningCountBySkillLevel[MIN(SKILL_MAX_LEVEL, GetSkillLevel(SKILL_CHAIN))];
}

void CHARACTER::SetAffectedEunhyung() 
{ 
	m_dwAffectedEunhyungLevel = GetSkillPower(SKILL_EUNHYUNG); 
}

void CHARACTER::SetSkillGroup(uint8_t bSkillGroup)
{
	if (bSkillGroup > 2) 
		return;

	if (GetLevel() < 5)
		return;

	m_points.skill_group = bSkillGroup; 

	TPacketGCChangeSkillGroup p;
	p.header = HEADER_GC_SKILL_GROUP;
	p.skill_group = m_points.skill_group;

	GetDesc()->Packet(&p, sizeof(TPacketGCChangeSkillGroup));
}

int32_t CHARACTER::ComputeCooltime(int32_t time)
{
	return CalculateDuration(GetPoint(POINT_CASTING_SPEED), time);
}

void CHARACTER::SkillLevelPacket()
{
	if (!GetDesc())
		return;

	TPacketGCSkillLevel pack;

	pack.bHeader = HEADER_GC_SKILL_LEVEL;
	memcpy(&pack.skills, m_pSkillLevels, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	GetDesc()->Packet(&pack, sizeof(TPacketGCSkillLevel));
}

void CHARACTER::SetSkillLevel(uint32_t dwVnum, uint8_t bLev)
{
	if (!m_pSkillLevels)
		return;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		SysLog("vnum overflow (vnum {})", dwVnum);
		return;
	}

	m_pSkillLevels[dwVnum].bLevel = MIN(40, bLev);

	if (bLev >= 40)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_PERFECT_MASTER;
	else if (bLev >= 30)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_GRAND_MASTER;
	else if (bLev >= 20)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_MASTER;
	else
		m_pSkillLevels[dwVnum].bMasterType = SKILL_NORMAL;
}

bool CHARACTER::IsLearnableSkill(uint32_t dwSkillVnum) const
{
	const CSkillProto* pSkill = CSkillManager::GetInstance()->Get(dwSkillVnum);

	if (!pSkill)
		return false;

	if (GetSkillLevel(dwSkillVnum) >= SKILL_MAX_LEVEL)
		return false;

	if (pSkill->dwType == 0)
	{
		if (GetSkillLevel(dwSkillVnum) >= pSkill->bMaxLevel)
			return false;

		return true;
	}

	if (pSkill->dwType == 5)
	{
		if (dwSkillVnum == SKILL_HORSE_WILDATTACK_RANGE && GetJob() != JOB_ASSASSIN)
			return false; 

		return true;
	}

	if (GetSkillGroup() == 0)
		return false;

	if (pSkill->dwType - 1 == GetJob())
		return true;

	if (6 == pSkill->dwType)
	{
		if (SKILL_7_A_ANTI_TANHWAN <= dwSkillVnum && dwSkillVnum <= SKILL_7_D_ANTI_YONGBI)
		{
			for (int32_t i=0 ; i < 4 ; i++)
			{
				if (unsigned(SKILL_7_A_ANTI_TANHWAN + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_7_A_ANTI_TANHWAN + i))
					{
						return false;
					}
				}
			}

			return true;
		}

		if (SKILL_8_A_ANTI_GIGONGCHAM <= dwSkillVnum && dwSkillVnum <= SKILL_8_D_ANTI_BYEURAK)
		{
			for (int32_t i=0 ; i < 4 ; i++)
			{
				if (unsigned(SKILL_8_A_ANTI_GIGONGCHAM + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_8_A_ANTI_GIGONGCHAM + i))
						return false;
				}
			}
			
			return true;
		}
	}

	return false;
}

// ADD_GRANDMASTER_SKILL
bool CHARACTER::LearnGrandMasterSkill(uint32_t dwSkillVnum)
{
	CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwSkillVnum);

	if (!pSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
		return false;
	}

	PyLog("learn grand master skill[{}] cur {}, next {}", dwSkillVnum, get_global_time(), GetSkillNextReadTime(dwSkillVnum));

	/*
	   if (get_global_time() < GetSkillNextReadTime(dwSkillVnum))
	   {
	   if (!(test_server && quest::CQuestManager::GetInstance()->GetEventFlag("no_read_delay")))
	   {
	   if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
	   {
	// Ignoring the time limit while using the magic spell
	RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have escaped the evil ghost curse with the help of an Exorcism Scroll."));
	}
	else 	    
	{
	SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
	return false;
	}
	}
	}
	 */

	// If bType is 0, you can practice from the beginning with a book.
	if (pSk->dwType == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill up to Grand Master level."));
		return false;
	}

	if (GetSkillMasterType(dwSkillVnum) != SKILL_GRAND_MASTER)
	{
		if (GetSkillMasterType(dwSkillVnum) > SKILL_GRAND_MASTER)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already are a Master of this skill. You cannot train this skill any further."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your Skill is not high enough to become a Grand Master."));
		return false;
	}

	std::string strTrainSkill;
	{
		std::ostringstream os;
		os << "training_grandmaster_skill.skill" << dwSkillVnum;
		strTrainSkill = os.str();
	}

	// Here we calculate the probability.
	uint8_t bLastLevel = GetSkillLevel(dwSkillVnum);

	int32_t idx = MIN(9, GetSkillLevel(dwSkillVnum) - 30);

	PyLog("LearnGrandMasterSkill {} table idx {} value {}", GetName(), idx, aiGrandMasterSkillBookCountForLevelUp[idx]);

	int32_t iTotalReadCount = GetQuestFlag(strTrainSkill) + 1;
	SetQuestFlag(strTrainSkill, iTotalReadCount);

	int32_t iMinReadCount = aiGrandMasterSkillBookMinCount[idx];
	int32_t iMaxReadCount = aiGrandMasterSkillBookMaxCount[idx];

	int32_t iBookCount = aiGrandMasterSkillBookCountForLevelUp[idx];

	if (FindAffect(AFFECT_SKILL_BOOK_BONUS))
	{
		if (iBookCount&1)
			iBookCount = iBookCount / 2 + 1; 
		else
			iBookCount = iBookCount / 2; 

		RemoveAffect(AFFECT_SKILL_BOOK_BONUS);
	}

	int32_t n = number(1, iBookCount);
	PyLog("Number({})", n);

	uint32_t nextTime = get_global_time() + number(28800, 43200);

	PyLog("GrandMaster SkillBookCount min {} cur {} max {} (next_time={})", iMinReadCount, iTotalReadCount, iMaxReadCount, nextTime);

	bool bSuccess = n == 2;

	if (iTotalReadCount < iMinReadCount)
		bSuccess = false;
	if (iTotalReadCount > iMaxReadCount)
		bSuccess = true;

	if (bSuccess)
	{
		SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
	}

	SetSkillNextReadTime(dwSkillVnum, nextTime);

	if (bLastLevel == GetSkillLevel(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("That did not work. Damn!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Training has failed. Please try again later."));
		LogManager::GetInstance()->CharLog(this, dwSkillVnum, "GM_READ_FAIL", "");
		return false;
	}

	ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("My body is full of energy!"));
	ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("The training seems to be working already..."));
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You successfully finished your training with the book."));
	LogManager::GetInstance()->CharLog(this, dwSkillVnum, "GM_READ_SUCCESS", "");
	return true;
}
// END_OF_ADD_GRANDMASTER_SKILL

static bool FN_should_check_exp(LPCHARACTER ch)
{
	return true;
}


bool CHARACTER::LearnSkillByBook(uint32_t dwSkillVnum, uint8_t bProb)
{
	const CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwSkillVnum);

	if (!pSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
		return false;
	}

	int32_t need_exp = 0;

	if (FN_should_check_exp(this))
	{
		need_exp = 20000;

		if (GetExp() < need_exp)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read this due to your lack of experience."));
			return false;
		}
	}

	// If bType is 0, you can practice from the beginning with a book.
	if (pSk->dwType != 0)
	{
		if (GetSkillMasterType(dwSkillVnum) != SKILL_MASTER)
		{
			if (GetSkillMasterType(dwSkillVnum) > SKILL_MASTER)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill with a Book."));
			else
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This skill's level is not high enough to be trained with a Book."));
			return false;
		}
	}

	if (get_global_time() < GetSkillNextReadTime(dwSkillVnum))
	{
		if (!(test_server && quest::CQuestManager::GetInstance()->GetEventFlag("no_read_delay")))
		{
			if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
			{
				// Ignoring the time limit while using the magic spell
				RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have escaped the evil ghost curse with the help of an Exorcism Scroll."));
			}
			else 	    
			{
				SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
				return false;
			}
		}
	}

	// Here we calculate the probability.
	uint8_t bLastLevel = GetSkillLevel(dwSkillVnum);

	if (bProb != 0)
	{
		// SKILL_BOOK_BONUS
		if (FindAffect(AFFECT_SKILL_BOOK_BONUS))
		{
			bProb += bProb / 2;
			RemoveAffect(AFFECT_SKILL_BOOK_BONUS);
		}
		// END_OF_SKILL_BOOK_BONUS

		PyLog("LearnSkillByBook Pct {} prob {}", dwSkillVnum, bProb);

		if (number(1, 100) <= bProb)
		{
			if (test_server)
				PyLog("LearnSkillByBook {} SUCC", dwSkillVnum);

			SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
		}
		else
		{
			if (test_server)
				PyLog("LearnSkillByBook {} FAIL", dwSkillVnum);
		}
	}
	else
	{
		int32_t idx = MIN(9, GetSkillLevel(dwSkillVnum) - 20);

		PyLog("LearnSkillByBook {} table idx {} value {}", GetName(), idx, aiSkillBookCountForLevelUp[idx]);

		int32_t need_bookcount = GetSkillLevel(dwSkillVnum) - 20;

		PointChange(POINT_EXP, -need_exp);

		auto q = quest::CQuestManager::GetInstance();
		quest::PC* pPC = q->GetPC(GetPlayerID());

		if (pPC)
		{
			char flag[128+1];
			memset(flag, 0, sizeof(flag));
			snprintf(flag, sizeof(flag), "traning_master_skill.%u.read_count", dwSkillVnum);

			int32_t read_count = pPC->GetFlag(flag);
			int32_t percent = 65;

			if (FindAffect(AFFECT_SKILL_BOOK_BONUS))
			{
				percent = 0;
				RemoveAffect(AFFECT_SKILL_BOOK_BONUS);
			}

			if (number(1, 100) > percent)
			{
				// success in reading
				if (read_count >= need_bookcount)
				{
					SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
					pPC->SetFlag(flag, 0);

					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have successfully finished your training with the Book."));
					LogManager::GetInstance()->CharLog(this, dwSkillVnum, "READ_SUCCESS", "");
					return true;
				}
				else
				{
					pPC->SetFlag(flag, read_count + 1);

					switch (number(1, 3))
					{
						case 1:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("I'm making progress, but I still haven't understood everything."));
							break;
											
						case 2:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("These instructions are difficult to understand. I have to carry on studying."));
							break;

						case 3:
						default:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("I understand this chapter. But I've got to carry on working hard."));
							break;
					}

					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to read %d more skill books to improve this skill."), need_bookcount - read_count);
					return true;
				}
			}
		}
		else
		{
			// Failed to load user's quest information
		}
	}

	if (bLastLevel != GetSkillLevel(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("My body is full of energy!"));
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("The training seems to be working already..."));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have successfully finished your training with the Book."));
		LogManager::GetInstance()->CharLog(this, dwSkillVnum, "READ_SUCCESS", "");
	}
	else
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("That did not work. Damn!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Training has failed. Please try again later."));
		LogManager::GetInstance()->CharLog(this, dwSkillVnum, "READ_FAIL", "");
	}

	return true;
}

bool CHARACTER::SkillLevelDown(uint32_t dwVnum)
{
	if (!m_pSkillLevels)
		return false;

	if (g_bSkillDisable)
		return false;

	if (IsPolymorphed())
		return false;

	CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwVnum);

	if (!pSk)
	{
		SysLog("There is no such skill by number {}", dwVnum);
		return false;
	}

	if (!IsLearnableSkill(dwVnum))
		return false;

	if (GetSkillMasterType(pSk->dwVnum) != SKILL_NORMAL)
		return false;

	if (!GetSkillGroup())
		return false;

	if (pSk->dwVnum >= SKILL_MAX_NUM)
		return false;

	if (m_pSkillLevels[pSk->dwVnum].bLevel == 0)
		return false;

	int32_t idx = POINT_SKILL;
	switch (pSk->dwType)
	{
		case 0:
			idx = POINT_SUB_SKILL;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 6:
			idx = POINT_SKILL;
			break;
		case 5:
			idx = POINT_HORSE_SKILL;
			break;
		default:
			SysLog("Wrong skill type {} skill vnum {}", pSk->dwType, pSk->dwVnum);
			return false;

	}

	PointChange(idx, +1);
	SetSkillLevel(pSk->dwVnum, m_pSkillLevels[pSk->dwVnum].bLevel - 1);

	PyLog("SkillDown: {} {} {} {} type {}", GetName(), pSk->dwVnum, m_pSkillLevels[pSk->dwVnum].bMasterType, m_pSkillLevels[pSk->dwVnum].bLevel, pSk->dwType);
	Save();

	ComputePoints();
	SkillLevelPacket();
	return true;
}

void CHARACTER::SkillLevelUp(uint32_t dwVnum, uint8_t bMethod)
{
	if (!m_pSkillLevels)
		return;

	if (g_bSkillDisable)
		return;

	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change your status while you are transformed."));
		return;
	}

	if (SKILL_7_A_ANTI_TANHWAN <= dwVnum && dwVnum <= SKILL_8_D_ANTI_BYEURAK)
	{
		if (0 == GetSkillLevel(dwVnum))
			return;
	}

	const CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwVnum);

	if (!pSk)
	{
		SysLog("There is no such skill by number (vnum {})", dwVnum);
		return;
	}

	if (pSk->dwVnum >= SKILL_MAX_NUM)
	{
		SysLog("Skill Vnum overflow (vnum {})", dwVnum);
		return;
	}

	if (!IsLearnableSkill(dwVnum))
		return;

	// Grand Master can only be completed as a quest
	if (pSk->dwType != 0)
	{
		switch (GetSkillMasterType(pSk->dwVnum))
		{
			case SKILL_GRAND_MASTER:
				if (bMethod != SKILL_UP_BY_QUEST)
					return;
				break;

			case SKILL_PERFECT_MASTER:
				return;
		}
	}

	if (bMethod == SKILL_UP_BY_POINT)
	{
		// Can only be trained without being a master
		if (GetSkillMasterType(pSk->dwVnum) != SKILL_NORMAL)
			return;

		if (IS_SET(pSk->dwFlag, SKILL_FLAG_DISABLE_BY_POINT_UP))
			return;
	}
	else if (bMethod == SKILL_UP_BY_BOOK)
	{
		if (pSk->dwType != 0) // Skills that do not belong to a profession or that cannot be raised as points can be learned from a book from the beginning.
			if (GetSkillMasterType(pSk->dwVnum) != SKILL_MASTER)
				return;
	}

	if (GetLevel() < pSk->bLevelLimit)
		return;

	if (pSk->preSkillVnum)
		if (GetSkillMasterType(pSk->preSkillVnum) == SKILL_NORMAL &&
			GetSkillLevel(pSk->preSkillVnum) < pSk->preSkillLevel)
			return;

	if (!GetSkillGroup())
		return;

	if (bMethod == SKILL_UP_BY_POINT)
	{
		int32_t idx;

		switch (pSk->dwType)
		{
			case 0:
				idx = POINT_SUB_SKILL;
				break;

			case 1:
			case 2:
			case 3:
			case 4:
			case 6:
				idx = POINT_SKILL;
				break;

			case 5:
				idx = POINT_HORSE_SKILL;
				break;

			default:
				SysLog("Wrong skill type {} skill vnum {}", pSk->dwType, pSk->dwVnum);
				return;
		}

		if (GetPoint(idx) < 1)
			return;

		PointChange(idx, -1);
	}

	int32_t SkillPointBefore = GetSkillLevel(pSk->dwVnum);
	SetSkillLevel(pSk->dwVnum, m_pSkillLevels[pSk->dwVnum].bLevel + 1);

	if (pSk->dwType != 0)
	{
		// Coding that suddenly upgrades
		switch (GetSkillMasterType(pSk->dwVnum))
		{
			case SKILL_NORMAL:
				// Burnsup is a random master training between 17 and 20 skill upgrades.
				if (GetSkillLevel(pSk->dwVnum) >= 17)
				{
					if (GetQuestFlag("reset_scroll.force_to_master_skill") > 0)
					{
						SetSkillLevel(pSk->dwVnum, 20);
						SetQuestFlag("reset_scroll.force_to_master_skill", 0);
					}
					else
					{
						if (number(1, 21 - MIN(20, GetSkillLevel(pSk->dwVnum))) == 1)
							SetSkillLevel(pSk->dwVnum, 20);
					}
				}
				break;

			case SKILL_MASTER:
				if (GetSkillLevel(pSk->dwVnum) >= 30)
				{
					if (number(1, 31 - MIN(30, GetSkillLevel(pSk->dwVnum))) == 1)
						SetSkillLevel(pSk->dwVnum, 30);
				}
				break;

			case SKILL_GRAND_MASTER:
				if (GetSkillLevel(pSk->dwVnum) >= 40)
				{
					SetSkillLevel(pSk->dwVnum, 40);
				}
				break;
		}
	}

	char szSkillUp[1024];

	snprintf(szSkillUp, sizeof(szSkillUp), "SkillUp: %s %u %d %d[Before:%d] type %u",
			GetName(), pSk->dwVnum, m_pSkillLevels[pSk->dwVnum].bMasterType, m_pSkillLevels[pSk->dwVnum].bLevel, SkillPointBefore, pSk->dwType);

	PyLog("{}", szSkillUp);

	LogManager::GetInstance()->CharLog(this, pSk->dwVnum, "SKILLUP", szSkillUp);
	Save();

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ComputeSkillPoints()
{
	if (g_bSkillDisable)
		return;
}

void CHARACTER::ResetSkill()
{
	if (!m_pSkillLevels)
		return;

	// Secondary skills do not reset
	std::vector<std::pair<uint32_t, TPlayerSkill> > vec;
	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		vec.push_back(std::make_pair(s_adwSubSkillVnums[i], m_pSkillLevels[s_adwSubSkillVnums[i]]));
	}

	memset(m_pSkillLevels, 0, sizeof(TPlayerSkill) * SKILL_MAX_NUM);

	std::vector<std::pair<uint32_t, TPlayerSkill> >::const_iterator iter = vec.begin();

	while (iter != vec.end())
	{
		const std::pair<uint32_t, TPlayerSkill>& pair = *(iter++);
		m_pSkillLevels[pair.first] = pair.second;
	}

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ComputePassiveSkill(uint32_t dwVnum)
{
	if (g_bSkillDisable)
		return;

	if (GetSkillLevel(dwVnum) == 0)
		return;

	CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwVnum);
	pSk->SetPointVar("k", GetSkillLevel(dwVnum));
	int32_t iAmount = (int32_t) pSk->kPointPoly.Eval();

	TraceLog("{} passive #{} on {} amount {}", GetName(), dwVnum, pSk->bPointOn, iAmount);
	PointChange(pSk->bPointOn, iAmount);
}

struct FFindNearVictim
{
	FFindNearVictim(LPCHARACTER center, LPCHARACTER attacker, const CHARACTER_SET& excepts_set = empty_set_)
		: m_pChrCenter(center),
	m_pChrNextTarget(nullptr),
	m_pChrAttacker(attacker),
	m_count(0),
	m_excepts_set(excepts_set)
	{
	}

	void operator ()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pChr = (LPCHARACTER) ent;

		if (!m_excepts_set.empty()) {
			if (m_excepts_set.find(pChr) != m_excepts_set.end())
				return;
		}

		if (m_pChrCenter == pChr)
			return;

		if (!battle_is_attackable(m_pChrAttacker, pChr))
		{
			return;
		}

		if (abs(m_pChrCenter->GetX() - pChr->GetX()) > 1000 || abs(m_pChrCenter->GetY() - pChr->GetY()) > 1000)
			return;

		float fDist = DISTANCE_APPROX(m_pChrCenter->GetX() - pChr->GetX(), m_pChrCenter->GetY() - pChr->GetY());

		if (fDist < 1000)
		{
			++m_count;

			if ((m_count == 1) || number(1, m_count) == 1)
				m_pChrNextTarget = pChr;
		}
	}

	LPCHARACTER GetVictim()
	{
		return m_pChrNextTarget;
	}

	LPCHARACTER m_pChrCenter;
	LPCHARACTER m_pChrNextTarget;
	LPCHARACTER m_pChrAttacker;
	int32_t		m_count;
	const CHARACTER_SET & m_excepts_set;
private:
	static CHARACTER_SET empty_set_;
};

CHARACTER_SET FFindNearVictim::empty_set_;

EVENTINFO(chain_lightning_event_info)
{
	uint32_t			dwVictim;
	uint32_t			dwChr;

	chain_lightning_event_info()
	: dwVictim(0)
	, dwChr(0)
	{
	}
};

EVENTFUNC(ChainLightningEvent)
{
	chain_lightning_event_info * info = dynamic_cast<chain_lightning_event_info *>(event->info);

	LPCHARACTER pChrVictim = CHARACTER_MANAGER::GetInstance()->Find(info->dwVictim);
	LPCHARACTER pChr = CHARACTER_MANAGER::GetInstance()->Find(info->dwChr);
	LPCHARACTER pTarget = nullptr;

	if (!pChr || !pChrVictim)
	{
		TraceLog("use chainlighting, but no character");
		return 0;
	}

	TraceLog("chainlighting event {}", pChr->GetName());

	if (pChrVictim->GetParty()) // party first
	{
		pTarget = pChrVictim->GetParty()->GetNextOwnership(NULL, pChrVictim->GetX(), pChrVictim->GetY());
		if (pTarget == pChrVictim || !number(0, 2) || pChr->GetChainLightingExcept().find(pTarget) != pChr->GetChainLightingExcept().end())
			pTarget = nullptr;
	}

	if (!pTarget)
	{
		// 1. Find Next victim
		FFindNearVictim f(pChrVictim, pChr, pChr->GetChainLightingExcept());

		if (pChrVictim->GetSectree())
		{
			pChrVictim->GetSectree()->ForEachAround(f);
			// 2. If exist, compute it again
			pTarget = f.GetVictim();
		}
	}

	if (pTarget)
	{
		pChrVictim->CreateFly(FLY_CHAIN_LIGHTNING, pTarget);
		pChr->ComputeSkill(SKILL_CHAIN, pTarget);
		pChr->AddChainLightningExcept(pTarget);
	}
	else
	{
		TraceLog("{} use chainlighting, but find victim failed near {}", pChr->GetName(), pChrVictim->GetName());
	}

	return 0;
}

void SetPolyVarForAttack(LPCHARACTER ch, CSkillProto* pSk, LPITEM pWeapon)
{
	if (ch->IsPC())
	{
		if (pWeapon && pWeapon->GetType() == ITEM::TYPE_WEAPON)
		{
			int32_t iWep = number(pWeapon->GetValue(3), pWeapon->GetValue(4));
			iWep += pWeapon->GetValue(5);

			int32_t iMtk = number(pWeapon->GetValue(1), pWeapon->GetValue(2));
			iMtk += pWeapon->GetValue(5);

			pSk->SetPointVar("wep", iWep);
			pSk->SetPointVar("mtk", iMtk);
			pSk->SetPointVar("mwep", iMtk);
		}
		else
		{
			pSk->SetPointVar("wep", 0);
			pSk->SetPointVar("mtk", 0);
			pSk->SetPointVar("mwep", 0);
		}
	}
	else
	{
		int32_t iWep = number(ch->GetMobDamageMin(), ch->GetMobDamageMax());
		pSk->SetPointVar("wep", iWep);
		pSk->SetPointVar("mwep", iWep);
		pSk->SetPointVar("mtk", iWep);
	}
}

struct FuncSplashDamage
{
	FuncSplashDamage(int32_t x, int32_t y, CSkillProto* pSk, LPCHARACTER pChr, int32_t iAmount, int32_t iAG, int32_t iMaxHit, LPITEM pWeapon, bool bDisableCooltime, TSkillUseInfo* pInfo, uint8_t bUseSkillPower)
		:
		m_x(x), m_y(y), m_pSk(pSk), m_pChr(pChr), m_iAmount(iAmount), m_iAG(iAG), m_iCount(0), m_iMaxHit(iMaxHit), m_pWeapon(pWeapon), m_bDisableCooltime(bDisableCooltime), m_pInfo(pInfo), m_bUseSkillPower(bUseSkillPower)
		{
		}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
		{
			//if (m_pSk->dwVnum == SKILL_CHAIN) PyLog("CHAIN target not character {}", m_pChr->GetName());
			return;
		}

		LPCHARACTER pChrVictim = (LPCHARACTER) ent;

		if (DISTANCE_APPROX(m_x - pChrVictim->GetX(), m_y - pChrVictim->GetY()) > m_pSk->iSplashRange)
		{
			if(test_server)
				PyLog("XXX target too far {}", m_pChr->GetName());
			return;
		}

		if (!battle_is_attackable(m_pChr, pChrVictim))
		{
			if(test_server)
				PyLog("XXX target not attackable {}", m_pChr->GetName());
			return;
		}

		if (m_pChr->IsPC())
			// Guild skills do not deal with cooldowns.
			if (!(m_pSk->dwVnum >= GUILD_SKILL_START && m_pSk->dwVnum <= GUILD_SKILL_END))
				if (!m_bDisableCooltime && m_pInfo && !m_pInfo->HitOnce(m_pSk->dwVnum) && m_pSk->dwVnum != SKILL_MUYEONG)
				{
					if(test_server)
						PyLog("check guild skill {}", m_pChr->GetName());
					return;
				}

		++m_iCount;

		int32_t iDam;

		////////////////////////////////////////////////////////////////////////////////
		//float k = 1.0f * m_pChr->GetSkillPower(m_pSk->dwVnum) * m_pSk->bMaxLevel / 100;
		//m_pSk->kPointPoly2.SetVar("k", 1.0 * m_bUseSkillPower * m_pSk->bMaxLevel / 100);
		m_pSk->SetPointVar("k", 1.0 * m_bUseSkillPower * m_pSk->bMaxLevel / 100);
		m_pSk->SetPointVar("lv", m_pChr->GetLevel());
		m_pSk->SetPointVar("iq", m_pChr->GetPoint(POINT_IQ));
		m_pSk->SetPointVar("str", m_pChr->GetPoint(POINT_ST));
		m_pSk->SetPointVar("dex", m_pChr->GetPoint(POINT_DX));
		m_pSk->SetPointVar("con", m_pChr->GetPoint(POINT_HT));
		m_pSk->SetPointVar("def", m_pChr->GetPoint(POINT_DEF_GRADE));
		m_pSk->SetPointVar("odef", m_pChr->GetPoint(POINT_DEF_GRADE) - m_pChr->GetPoint(POINT_DEF_GRADE_BONUS));
		m_pSk->SetPointVar("horse_level", m_pChr->GetHorseLevel());

		//int32_t iPenetratePct = (int32_t)(1 + k*4);
		bool bIgnoreDefense = false;

		if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_PENETRATE))
		{
			int32_t iPenetratePct = (int32_t) m_pSk->kPointPoly2.Eval();

			if (number(1, 100) <= iPenetratePct)
				bIgnoreDefense = true;
		}

		bool bIgnoreTargetRating = false;

		if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_IGNORE_TARGET_RATING))
		{
			int32_t iPct = (int32_t) m_pSk->kPointPoly2.Eval();

			if (number(1, 100) <= iPct)
				bIgnoreTargetRating = true;
		}

		m_pSk->SetPointVar("ar", CalcAttackRating(m_pChr, pChrVictim, bIgnoreTargetRating));

		if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
			m_pSk->SetPointVar("atk", CalcMeleeDamage(m_pChr, pChrVictim, true, bIgnoreTargetRating));
		else if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
		{
			LPITEM pBow, pArrow;

			if (1 == m_pChr->GetArrowAndBow(&pBow, &pArrow, 1))
				m_pSk->SetPointVar("atk", CalcArrowDamage(m_pChr, pChrVictim, pBow, pArrow, true));
			else
				m_pSk->SetPointVar("atk", 0);
		}

		if (m_pSk->bPointOn == POINT_MOV_SPEED)
			m_pSk->kPointPoly.SetVar("maxv", pChrVictim->GetLimitPoint(POINT_MOV_SPEED));

		m_pSk->SetPointVar("maxhp", pChrVictim->GetMaxHP());
		m_pSk->SetPointVar("maxsp", pChrVictim->GetMaxSP());

		m_pSk->SetPointVar("chain", m_pChr->GetChainLightningIndex());
		m_pChr->IncChainLightningIndex();

		bool bUnderEunhyung = m_pChr->GetAffectedEunhyung(); // 이건 왜 여기서 하지??

		m_pSk->SetPointVar("ek", m_pChr->GetAffectedEunhyung()*1./100);
		//m_pChr->ClearAffectedEunhyung();
		SetPolyVarForAttack(m_pChr, m_pSk, m_pWeapon);

		int32_t iAmount = 0;

		if (m_pChr->GetUsedSkillMasterType(m_pSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			iAmount = (int32_t) m_pSk->kMasterBonusPoly.Eval();
		}
		else
		{
			iAmount = (int32_t) m_pSk->kPointPoly.Eval();
		}

		if (test_server && iAmount == 0 && m_pSk->bPointOn != POINT_NONE)
		{
			m_pChr->ChatPacket(CHAT_TYPE_INFO, "Wrong skill formula.");
		}
		////////////////////////////////////////////////////////////////////////////////
		iAmount = -iAmount;

		if (m_pSk->dwVnum == SKILL_AMSEOP)
		{
			float fDelta = GetDegreeDelta(m_pChr->GetRotation(), pChrVictim->GetRotation());
			float adjust;

			if (fDelta < 35.0f)
			{
				adjust = 1.5f;

				if (bUnderEunhyung)
					adjust += 0.5f;

				if (m_pChr->GetWear(WEAR_WEAPON) && m_pChr->GetWear(WEAR_WEAPON)->GetSubType() == ITEM::WEAPON_DAGGER)
				{
					adjust += 0.5f;
				}
			}
			else
			{
				adjust = 1.0f;

				if (bUnderEunhyung)
					adjust += 0.5f;

				if (m_pChr->GetWear(WEAR_WEAPON) && m_pChr->GetWear(WEAR_WEAPON)->GetSubType() == ITEM::WEAPON_DAGGER)
					adjust += 0.5f;
			}

			iAmount = (int32_t) (iAmount * adjust);
		}
		else if (m_pSk->dwVnum == SKILL_GUNGSIN)
		{
			float adjust = 1.0;

			if (m_pChr->GetWear(WEAR_WEAPON) && m_pChr->GetWear(WEAR_WEAPON)->GetSubType() == ITEM::WEAPON_DAGGER)
			{
				adjust = 1.35f;
			}

			iAmount = (int32_t) (iAmount * adjust);
		}
		////////////////////////////////////////////////////////////////////////////////
		//PyLog("name: {} skill: {} amount {} to {}", m_pChr->GetName(), m_pSk->szName, iAmount, pChrVictim->GetName());

		iDam = CalcBattleDamage(iAmount, m_pChr->GetLevel(), pChrVictim->GetLevel());

		if (m_pChr->IsPC() && m_pChr->m_SkillUseInfo[m_pSk->dwVnum].GetMainTargetVID() != (uint32_t) pChrVictim->GetVID())
		{
			// damage reduction
			iDam = (int32_t) (iDam * m_pSk->kSplashAroundDamageAdjustPoly.Eval());
		}

		// TODO You must record the damage type according to the skill.
		EDamageType dt = DAMAGE_TYPE_NONE;

		switch (m_pSk->bSkillAttrType)
		{
			case SKILL_ATTR_TYPE_NORMAL:
				break;

			case SKILL_ATTR_TYPE_MELEE:
				{
					dt = DAMAGE_TYPE_MELEE;

					LPITEM pWeapon = m_pChr->GetWear(WEAR_WEAPON);

					if (pWeapon)
						switch (pWeapon->GetSubType())
						{
							case ITEM::WEAPON_SWORD:
								iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_SWORD)) / 100;
								break;

							case ITEM::WEAPON_TWO_HANDED:
								iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_TWOHAND)) / 100;
								// Two-handed sword penalty 10%
								//iDam = iDam * 95 / 100;

								break;

							case ITEM::WEAPON_DAGGER:
								iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_DAGGER)) / 100;
								break;

							case ITEM::WEAPON_BELL:
								iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_BELL)) / 100;
								break;

							case ITEM::WEAPON_FAN:
								iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_FAN)) / 100;
								break;
						}

					if (!bIgnoreDefense)
						iDam -= pChrVictim->GetPoint(POINT_DEF_GRADE);
				}
				break;

			case SKILL_ATTR_TYPE_RANGE:
				dt = DAMAGE_TYPE_RANGE;
				// There is a bug that has not been applied before, so if you recalculate the defense, users will be upset
				//iDam -= pChrVictim->GetPoint(POINT_DEF_GRADE);
				iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_BOW)) / 100;
				break;

			case SKILL_ATTR_TYPE_MAGIC:
				dt = DAMAGE_TYPE_MAGIC;
				iDam = CalcAttBonus(m_pChr, pChrVictim, iDam);
				// There is a bug that has not been applied before, so if you recalculate the defense, users will be upset
				//iDam -= pChrVictim->GetPoint(POINT_MAGIC_DEF_GRADE);
				iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_MAGIC)) / 100;
				break;

			default:
				SysLog("Unknown skill attr type {} vnum {}", m_pSk->bSkillAttrType, m_pSk->dwVnum);
				break;
		}

		//
		// 20091109 German skill attribute request operation
		// Skills with SKILL_FLAG_WIND, SKILL_FLAG_ELEC, and SKILL_FLAG_FIRE in the existing skill table
		// Since there was none, the monster's RESIST_WIND, RESIST_ELEC, and RESIST_FIRE were not used.
		//
		// To separate PvP and PvE balance, we intentionally applied only NPCs, and
		// RESIST_MAGIC of mob_proto to RESIST_WIND, RESIST_ELEC, RESIST_FIRE in order not to feel
		// copied.
		//
		if (pChrVictim->IsNPC())
		{
			if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_WIND))
			{
				iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_WIND)) / 100;
			}

			if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_ELEC))
			{
				iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_ELEC)) / 100;
			}

			if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_FIRE))
			{
				iDam = iDam * (100 - pChrVictim->GetPoint(POINT_RESIST_FIRE)) / 100;
			}
		}

		if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_COMPUTE_MAGIC_DAMAGE))
			dt = DAMAGE_TYPE_MAGIC;

		if (pChrVictim->CanBeginFight())
			pChrVictim->BeginFight(m_pChr);

		if (m_pSk->dwVnum == SKILL_CHAIN)
			PyLog("{} CHAIN INDEX {} DAM {} DT {}", m_pChr->GetName(), m_pChr->GetChainLightningIndex() - 1, iDam, dt);

		{
			uint8_t AntiSkillID = 0;

			switch (m_pSk->dwVnum)
			{
				case SKILL_TANHWAN:		AntiSkillID = SKILL_7_A_ANTI_TANHWAN;		break;
				case SKILL_AMSEOP:		AntiSkillID = SKILL_7_B_ANTI_AMSEOP;		break;
				case SKILL_SWAERYUNG:	AntiSkillID = SKILL_7_C_ANTI_SWAERYUNG;		break;
				case SKILL_YONGBI:		AntiSkillID = SKILL_7_D_ANTI_YONGBI;		break;
				case SKILL_GIGONGCHAM:	AntiSkillID = SKILL_8_A_ANTI_GIGONGCHAM;	break;
				case SKILL_YEONSA:		AntiSkillID = SKILL_8_B_ANTI_YEONSA;		break;
				case SKILL_MAHWAN:		AntiSkillID = SKILL_8_C_ANTI_MAHWAN;		break;
				case SKILL_BYEURAK:		AntiSkillID = SKILL_8_D_ANTI_BYEURAK;		break;
			}

			if (0 != AntiSkillID)
			{
				uint8_t AntiSkillLevel = pChrVictim->GetSkillLevel(AntiSkillID);

				if (0 != AntiSkillLevel)
				{
					CSkillProto* pSk = CSkillManager::GetInstance()->Get(AntiSkillID);
					if (!pSk)
					{
						SysLog("There is no anti skill({}) in skill proto", AntiSkillID);
					}
					else
					{
						pSk->SetPointVar("k", 1.0f* pChrVictim->GetSkillPower(AntiSkillID)* pSk->bMaxLevel / 100);

						double ResistAmount = pSk->kPointPoly.Eval();

						PyLog("ANTI_SKILL: Resist(%lf) Orig({}) Reduce({})", ResistAmount, iDam, int32_t(iDam * (ResistAmount/100.0)));

						iDam -= iDam * (ResistAmount/100.0);
					}
				}
			}
		}

		if (!pChrVictim->Damage(m_pChr, iDam, dt) && !pChrVictim->IsStun())
		{
			if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_REMOVE_GOOD_AFFECT))
			{
				int32_t iAmount2 = (int32_t) m_pSk->kPointPoly2.Eval();
				int32_t iDur2 = (int32_t) m_pSk->kDurationPoly2.Eval();
				iDur2 += m_pChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (number(1, 100) <= iAmount2)
				{
					pChrVictim->RemoveGoodAffect();
					pChrVictim->AddAffect(m_pSk->dwVnum, POINT_NONE, 0, AFF_PABEOP, iDur2, 0, true);
				}
			}

			if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_SLOW | SKILL_FLAG_STUN | SKILL_FLAG_FIRE_CONT | SKILL_FLAG_POISON))
			{
				int32_t iPct = (int32_t) m_pSk->kPointPoly2.Eval();
				int32_t iDur = (int32_t) m_pSk->kDurationPoly2.Eval();

				iDur += m_pChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_STUN))
				{
					SkillAttackAffect(pChrVictim, iPct, ITEM::IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, iDur, m_pSk->szName);
				}
				else if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_SLOW))
				{
					SkillAttackAffect(pChrVictim, iPct, ITEM::IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, iDur, m_pSk->szName);
				}
				else if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_FIRE_CONT))
				{
					m_pSk->SetDurationVar("k", 1.0 * m_bUseSkillPower * m_pSk->bMaxLevel / 100);
					m_pSk->SetDurationVar("iq", m_pChr->GetPoint(POINT_IQ));

					iDur = (int32_t)m_pSk->kDurationPoly2.Eval();
					int32_t bonus = m_pChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

					if (bonus != 0)
					{
						iDur += bonus / 2;
					}

					if (number(1, 100) <= iDur)
					{
						pChrVictim->AttackedByFire(m_pChr, iPct, 5);
					}
				}
				else if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_POISON))
				{
					if (number(1, 100) <= iPct)
						pChrVictim->AttackedByPoison(m_pChr);
				}
			}

			if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_CRUSH | SKILL_FLAG_CRUSH_LONG) &&
				!IS_SET(pChrVictim->GetAIFlag(), MOB::AIFLAG_NOMOVE))
			{
				float fCrushSlidingLength = 200;

				if (m_pChr->IsNPC())
					fCrushSlidingLength = 400;

				if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_CRUSH_LONG))
					fCrushSlidingLength *= 2;

				float fx, fy;
				float degree = GetDegreeFromPositionXY(m_pChr->GetX(), m_pChr->GetY(), pChrVictim->GetX(), pChrVictim->GetY());

				if (m_pSk->dwVnum == SKILL_HORSE_WILDATTACK)
				{
					degree -= m_pChr->GetRotation();
					degree = fmod(degree, 360.0f) - 180.0f;

					if (degree > 0)
						degree = m_pChr->GetRotation() + 90.0f;
					else
						degree = m_pChr->GetRotation() - 90.0f;
				}

				GetDeltaByDegree(degree, fCrushSlidingLength, &fx, &fy);
				PyLog("CRUSH! {} -> {} ({} {}) -> ({} {})", m_pChr->GetName(), pChrVictim->GetName(), pChrVictim->GetX(), pChrVictim->GetY(), (int32_t)(pChrVictim->GetX()+fx), (int32_t)(pChrVictim->GetY()+fy));
				int32_t tx = (int32_t)(pChrVictim->GetX()+fx);
				int32_t ty = (int32_t)(pChrVictim->GetY()+fy);

				pChrVictim->Sync(tx, ty);
				pChrVictim->Goto(tx, ty);
				pChrVictim->CalculateMoveDuration();

				if (m_pChr->IsPC() && m_pChr->m_SkillUseInfo[m_pSk->dwVnum].GetMainTargetVID() == (uint32_t) pChrVictim->GetVID())
				{
					SkillAttackAffect(pChrVictim, 1000, ITEM::IMMUNE_STUN, m_pSk->dwVnum, POINT_NONE, 0, AFF_STUN, 4, m_pSk->szName);
				}
				else
				{
					pChrVictim->SyncPacket();
				}
			}
		}

		if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_HP_ABSORB))
		{
			int32_t iPct = (int32_t) m_pSk->kPointPoly2.Eval();
			m_pChr->PointChange(POINT_HP, iDam * iPct / 100);
		}

		if (IS_SET(m_pSk->dwFlag, SKILL_FLAG_SP_ABSORB))
		{
			int32_t iPct = (int32_t) m_pSk->kPointPoly2.Eval();
			m_pChr->PointChange(POINT_SP, iDam * iPct / 100);
		}

		if (m_pSk->dwVnum == SKILL_CHAIN && m_pChr->GetChainLightningIndex() < m_pChr->GetChainLightningMaxCount())
		{
			chain_lightning_event_info* info = AllocEventInfo<chain_lightning_event_info>();

			info->dwVictim = pChrVictim->GetVID();
			info->dwChr = m_pChr->GetVID();

			event_create(ChainLightningEvent, info, passes_per_sec / 5);
		}
		if(test_server)
			PyLog("FuncSplashDamage End :{} ", m_pChr->GetName());
	}

	int32_t		m_x;
	int32_t		m_y;
	CSkillProto * m_pSk;
	LPCHARACTER	m_pChr;
	int32_t		m_iAmount;
	int32_t		m_iAG;
	int32_t		m_iCount;
	int32_t		m_iMaxHit;
	LPITEM	m_pWeapon;
	bool m_bDisableCooltime;
	TSkillUseInfo* m_pInfo;
	uint8_t m_bUseSkillPower;
};

struct FuncSplashAffect
{
	FuncSplashAffect(LPCHARACTER ch, int32_t x, int32_t y, int32_t iDist, uint32_t dwVnum, uint8_t bPointOn, int32_t iAmount, uint32_t dwAffectFlag, int32_t iDuration, int32_t iSPCost, bool bOverride, int32_t iMaxHit)
	{
		m_x = x;
		m_y = y;
		m_iDist = iDist;
		m_dwVnum = dwVnum;
		m_bPointOn = bPointOn;
		m_iAmount = iAmount;
		m_dwAffectFlag = dwAffectFlag;
		m_iDuration = iDuration;
		m_iSPCost = iSPCost;
		m_bOverride = bOverride;
		m_pChrAttacker = ch;
		m_iMaxHit = iMaxHit;
		m_iCount = 0;
	}

	void operator () (LPENTITY ent)
	{
		if (m_iMaxHit && m_iMaxHit <= m_iCount)
			return;

		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChr = (LPCHARACTER) ent;

			if (test_server)
				PyLog("FuncSplashAffect step 1 : name:{} vnum:{} iDur:{}", pChr->GetName(), m_dwVnum, m_iDuration);
			if (DISTANCE_APPROX(m_x - pChr->GetX(), m_y - pChr->GetY()) < m_iDist)
			{
				if (test_server)
					PyLog("FuncSplashAffect step 2 : name:{} vnum:{} iDur:{}", pChr->GetName(), m_dwVnum, m_iDuration);
				if (m_dwVnum == SKILL_TUSOK)
					if (pChr->CanBeginFight())
						pChr->BeginFight(m_pChrAttacker);

				if (pChr->IsPC() && m_dwVnum == SKILL_TUSOK)
					pChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration/3, m_iSPCost, m_bOverride);
				else
					pChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration, m_iSPCost, m_bOverride);

				m_iCount ++;
			}
		}
	}

	LPCHARACTER m_pChrAttacker;
	int32_t		m_x;
	int32_t		m_y;
	int32_t		m_iDist;
	uint32_t	m_dwVnum;
	uint8_t	m_bPointOn;
	int32_t		m_iAmount;
	uint32_t	m_dwAffectFlag;
	int32_t		m_iDuration;
	int32_t		m_iSPCost;
	bool	m_bOverride;
	int32_t         m_iMaxHit;
	int32_t         m_iCount;
};

EVENTINFO(skill_gwihwan_info)
{
	uint32_t pid;
	uint8_t bsklv;

	skill_gwihwan_info()
	: pid(0)
	, bsklv(0)
	{
	}
};

EVENTFUNC(skill_gwihwan_event)
{
	skill_gwihwan_info* info = dynamic_cast<skill_gwihwan_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("skill_gwihwan_event> <Factor> Null pointer");
		return 0;
	}

	uint32_t pid = info->pid;
	uint8_t sklv= info->bsklv;
	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pid);

	if (!ch)
		return 0;

	int32_t percent = 20 * sklv - 1;

	if (number(1, 100) <= percent)
	{
		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::GetInstance()->GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
		{
			TraceLog("Recall: {} {} {} -> {} {}", ch->GetName(), ch->GetX(), ch->GetY(), pos.x, pos.y);
			ch->WarpSet(pos.x, pos.y);
		}
		else
		{
			SysLog("CHARACTER::UseItem : cannot find spawn position (name {}, {} x {})", ch->GetName(), ch->GetX(), ch->GetY());
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Teleportation has failed."));
	}
	return 0;
}

int32_t CHARACTER::ComputeSkillAtPosition(uint32_t dwVnum, const PIXEL_POSITION& posTarget, uint8_t bSkillLevel)
{
	if (GetMountVnum())
		return BATTLE_NONE;

	if (IsPolymorphed())
		return BATTLE_NONE;

	if (g_bSkillDisable)
		return BATTLE_NONE;

	CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwVnum);

	if (!pSk)
		return BATTLE_NONE;

	if (test_server)
	{
		PyLog("ComputeSkillAtPosition {} vnum {} x {} y {} level {}", 
				GetName(), dwVnum, posTarget.x, posTarget.y, bSkillLevel); 
	}

	// Skills used for me use my location.
	//if (IS_SET(pSk->dwFlag, SKILL_FLAG_SELFONLY))
	// posTarget = GetXYZ();

	// Non-splash skills are weird around them
	if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
		return BATTLE_NONE;

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pSk->dwVnum)) == 0)
		{
			return BATTLE_NONE;
		}
	}

	const float k = 1.0 * GetSkillPower(pSk->dwVnum, bSkillLevel)* pSk->bMaxLevel / 100;

	pSk->SetPointVar("k", k);
	pSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	if (IS_SET(pSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pSk->SetPointVar("atk", CalcMeleeDamage(this, this, true, false));
	}
	else if (IS_SET(pSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pSk->SetPointVar("atk", CalcMagicDamage(this, this));
	}
	else if (IS_SET(pSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pBow, pArrow;
		if (1 == GetArrowAndBow(&pBow, &pArrow, 1))
		{
			pSk->SetPointVar("atk", CalcArrowDamage(this, this, pBow, pArrow, true));
		}
		else
		{
			pSk->SetPointVar("atk", 0);
		}
	}

	if (pSk->bPointOn == POINT_MOV_SPEED)
	{
		pSk->SetPointVar("maxv", this->GetLimitPoint(POINT_MOV_SPEED));
	}

	pSk->SetPointVar("lv", GetLevel());
	pSk->SetPointVar("iq", GetPoint(POINT_IQ));
	pSk->SetPointVar("str", GetPoint(POINT_ST));
	pSk->SetPointVar("dex", GetPoint(POINT_DX));
	pSk->SetPointVar("con", GetPoint(POINT_HT));
	pSk->SetPointVar("maxhp", this->GetMaxHP());
	pSk->SetPointVar("maxsp", this->GetMaxSP());
	pSk->SetPointVar("chain", 0);
	pSk->SetPointVar("ar", CalcAttackRating(this, this));
	pSk->SetPointVar("def", GetPoint(POINT_DEF_GRADE));
	pSk->SetPointVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pSk->SetPointVar("horse_level", GetHorseLevel());

	if (pSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true);

	LPITEM pWeapon = GetWear(WEAR_WEAPON);

	SetPolyVarForAttack(this, pSk, pWeapon);

	pSk->SetDurationVar("k", k/*bSkillLevel*/);

	int32_t iAmount = (int32_t) pSk->kPointPoly.Eval();
	int32_t iAmount2 = (int32_t) pSk->kPointPoly2.Eval();

	// ADD_GRANDMASTER_SKILL
	int32_t iAmount3 = (int32_t) pSk->kPointPoly3.Eval();

	if (GetUsedSkillMasterType(pSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		/*
		   if (iAmount >= 0)
		   iAmount += (int32_t) m_pSk->kMasterBonusPoly.Eval();
		   else
		   iAmount -= (int32_t) m_pSk->kMasterBonusPoly.Eval();
		 */
		iAmount = (int32_t) pSk->kMasterBonusPoly.Eval();
	}

	if (test_server && iAmount == 0 && pSk->bPointOn != POINT_NONE)
	{
		ChatPacket(CHAT_TYPE_INFO, "Wrong skill formula.");
	}

	if (IS_SET(pSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (number(1, 100) <= iAmount2)
		{
			RemoveBadAffect();
		}
	}
	// END_OF_ADD_GRANDMASTER_SKILL

	if (IS_SET(pSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		//
		// In case of attack skill
		//
		bool bAdded = false;

		if (pSk->bPointOn == POINT_HP && iAmount < 0)
		{
			int32_t iAG = 0;

			FuncSplashDamage f(posTarget.x, posTarget.y, pSk, this, iAmount, iAG, pSk->lMaxHit, pWeapon, m_bDisableCooltime, IsPC()?&m_SkillUseInfo[dwVnum]:NULL, GetSkillPower(dwVnum, bSkillLevel));

			if (IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
			{
				if (GetSectree())
					GetSectree()->ForEachAround(f);
			}
			else
			{
				//if (dwVnum == SKILL_CHAIN) PyLog("CHAIN skill call FuncSplashDamage {}", GetName());
				f(this);
			}
		}
		else
		{
			//if (dwVnum == SKILL_CHAIN) PyLog("CHAIN skill no damage {} {}", iAmount, GetName());
			int32_t iDur = (int32_t) pSk->kDurationPoly.Eval();

			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)) // Guild skills do not deal with cooldowns.
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_MUYEONG)
					{
						//if (dwVnum == SKILL_CHAIN) PyLog("CHAIN skill cannot hit {}", GetName());
						return BATTLE_NONE;
					}


			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pSk->dwVnum, pSk->bPointOn, iAmount, pSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pSk->iSplashRange, pSk->dwVnum, pSk->bPointOn, iAmount, pSk->dwAffectFlag, iDur, 0, true, pSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		if (pSk->bPointOn2 != POINT_NONE)
		{
			int32_t iDur = (int32_t) pSk->kDurationPoly2.Eval();

			TraceLog("try second {} {} {}", pSk->dwVnum, pSk->bPointOn2, iDur);

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pSk->dwVnum, pSk->bPointOn2, iAmount2, pSk->dwAffectFlag2, iDur, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pSk->iSplashRange, pSk->dwVnum, pSk->bPointOn2, iAmount2, pSk->dwAffectFlag2, iDur, 0, !bAdded, pSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
			else
			{
				PointChange(pSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (GetUsedSkillMasterType(pSk->dwVnum) >= SKILL_GRAND_MASTER && pSk->bPointOn3 != POINT_NONE)
		{
			int32_t iDur = (int32_t) pSk->kDurationPoly3.Eval();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pSk->dwVnum, pSk->bPointOn3, iAmount3, 0 /*pSk->dwAffectFlag3*/, iDur, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pSk->iSplashRange, pSk->dwVnum, pSk->bPointOn3, iAmount3, 0 /*pSk->dwAffectFlag3*/, iDur, 0, !bAdded, pSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
			}
			else
			{
				PointChange(pSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		bool bAdded = false;
		int32_t iDur = (int32_t) pSk->kDurationPoly.Eval();

		if (iDur > 0)
		{
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
			// AffectFlag Unless there is no or toggle ..
			pSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			AddAffect(pSk->dwVnum,
					  pSk->bPointOn,
					  iAmount,
					  pSk->dwAffectFlag,
					  iDur,
					  (int32_t) pSk->kDurationSPCostPoly.Eval(),
					  !bAdded);

			bAdded = true;
		}
		else
		{
			PointChange(pSk->bPointOn, iAmount);
		}

		if (pSk->bPointOn2 != POINT_NONE)
		{
			int32_t iDur = (int32_t) pSk->kDurationPoly2.Eval();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
				AddAffect(pSk->dwVnum, pSk->bPointOn2, iAmount2, pSk->dwAffectFlag2, iDur, 0, !bAdded);
				bAdded = true;
			}
			else
			{
				PointChange(pSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (GetUsedSkillMasterType(pSk->dwVnum) >= SKILL_GRAND_MASTER && pSk->bPointOn3 != POINT_NONE)
		{
			int32_t iDur = (int32_t) pSk->kDurationPoly3.Eval();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
				AddAffect(pSk->dwVnum, pSk->bPointOn3, iAmount3, 0 /*pSk->dwAffectFlag3*/, iDur, 0, !bAdded);
			}
			else
			{
				PointChange(pSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_NONE;
	}
}

// If the bSkillLevel argument is not 0, it is forced without using m_abSkillLevels.
// Calculate with bSkillLevel.
int32_t CHARACTER::ComputeSkill(uint32_t dwVnum, LPCHARACTER pVictim, uint8_t bSkillLevel)
{
	const bool bCanUseHorseSkill = CanUseHorseSkill();

	// Return if you are riding a horse but cannot use skills
	if (!bCanUseHorseSkill && IsRiding())
		return BATTLE_NONE;

	if (IsPolymorphed())
		return BATTLE_NONE;

	if (g_bSkillDisable)
		return BATTLE_NONE;

	CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwVnum);

	if (!pSk)
		return BATTLE_NONE;

	if (bCanUseHorseSkill && pSk->dwType != SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (!bCanUseHorseSkill && pSk->dwType == SKILL_TYPE_HORSE)
		return BATTLE_NONE;
	

	// If you are not writing to someone else, you should write to me.
	if (IS_SET(pSk->dwFlag, SKILL_FLAG_SELFONLY))
		pVictim = this;

	if (!pVictim)
	{
		if (test_server)
			PyLog("ComputeSkill: {} Victim == nullptr, skill {}", GetName(), dwVnum);

		return BATTLE_NONE;
	}

	if (pSk->dwTargetRange && DISTANCE_SQRT(GetX() - pVictim->GetX(), GetY() - pVictim->GetY()) >= pSk->dwTargetRange + 50)
	{
		if (test_server)
			PyLog("ComputeSkill: Victim too far, skill {} : {} to {} (distance {} limit {})", 
					dwVnum,
					GetName(),
					pVictim->GetName(),
					(int32_t)DISTANCE_SQRT(GetX() - pVictim->GetX(), GetY() - pVictim->GetY()),
					pSk->dwTargetRange);

		return BATTLE_NONE;
	}

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pSk->dwVnum)) == 0)
		{
			if (test_server)
				PyLog("ComputeSkill : name:{} vnum:{}  skillLevelBySkill : {} ", GetName(), pSk->dwVnum, bSkillLevel);
			return BATTLE_NONE;
		}
	}

	if (pVictim->IsAffectFlag(AFF_PABEOP) && pVictim->IsGoodAffect(dwVnum))
	{
		return BATTLE_NONE;
	}

	const float k = 1.0 * GetSkillPower(pSk->dwVnum, bSkillLevel)* pSk->bMaxLevel / 100;

	pSk->SetPointVar("k", k);
	pSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	if (pSk->dwType == SKILL_TYPE_HORSE)
	{
		LPITEM pBow, pArrow;
		if (1 == GetArrowAndBow(&pBow, &pArrow, 1))
		{
			pSk->SetPointVar("atk", CalcArrowDamage(this, pVictim, pBow, pArrow, true));
		}
		else
		{
			pSk->SetPointVar("atk", CalcMeleeDamage(this, pVictim, true, false));
		}
	}
	else if (IS_SET(pSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pSk->SetPointVar("atk", CalcMeleeDamage(this, pVictim, true, false));
	}
	else if (IS_SET(pSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pSk->SetPointVar("atk", CalcMagicDamage(this, pVictim));
	}
	else if (IS_SET(pSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pBow, pArrow;
		if (1 == GetArrowAndBow(&pBow, &pArrow, 1))
		{
			pSk->SetPointVar("atk", CalcArrowDamage(this, pVictim, pBow, pArrow, true));
		}
		else
		{
			pSk->SetPointVar("atk", 0);
		}
	}

	if (pSk->bPointOn == POINT_MOV_SPEED)
	{
		pSk->SetPointVar("maxv", pVictim->GetLimitPoint(POINT_MOV_SPEED));
	}

	pSk->SetPointVar("lv", GetLevel());
	pSk->SetPointVar("iq", GetPoint(POINT_IQ));
	pSk->SetPointVar("str", GetPoint(POINT_ST));
	pSk->SetPointVar("dex", GetPoint(POINT_DX));
	pSk->SetPointVar("con", GetPoint(POINT_HT));
	pSk->SetPointVar("maxhp", pVictim->GetMaxHP());
	pSk->SetPointVar("maxsp", pVictim->GetMaxSP());
	pSk->SetPointVar("chain", 0);
	pSk->SetPointVar("ar", CalcAttackRating(this, pVictim));
	pSk->SetPointVar("def", GetPoint(POINT_DEF_GRADE));
	pSk->SetPointVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pSk->SetPointVar("horse_level", GetHorseLevel());

	if (pSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true);

	LPITEM pWeapon = GetWear(WEAR_WEAPON);

	SetPolyVarForAttack(this, pSk, pWeapon);

	pSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
	pSk->kDurationPoly2.SetVar("k", k/*bSkillLevel*/);

	int32_t iAmount = (int32_t) pSk->kPointPoly.Eval();
	int32_t iAmount2 = (int32_t) pSk->kPointPoly2.Eval();
	int32_t iAmount3 = (int32_t) pSk->kPointPoly3.Eval();

	if (test_server && IsPC())
		PyLog("iAmount: {} {} {} , atk:{} skLevel:{} k:{} GetSkillPower({}) MaxLevel:{} Per:{}",
				iAmount, iAmount2, iAmount3,
				pSk->kPointPoly.GetVar("atk"),
				pSk->kPointPoly.GetVar("k"),
				k,
				GetSkillPower(pSk->dwVnum, bSkillLevel),
				pSk->bMaxLevel,
				pSk->bMaxLevel/100
				);

	// ADD_GRANDMASTER_SKILL
	if (GetUsedSkillMasterType(pSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		iAmount = (int32_t) pSk->kMasterBonusPoly.Eval();
	}

	if (test_server && iAmount == 0 && pSk->bPointOn != POINT_NONE)
	{
		ChatPacket(CHAT_TYPE_INFO, "Wrong skill formula.");
	}
	// END_OF_ADD_GRANDMASTER_SKILL

	//PyLog("XXX SKILL Calc {} Amount {}", dwVnum, iAmount);

	// REMOVE_BAD_AFFECT_BUG_FIX
	if (IS_SET(pSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (number(1, 100) <= iAmount2)
		{
			pVictim->RemoveBadAffect();
		}
	}
	// END_OF_REMOVE_BAD_AFFECT_BUG_FIX

	if (IS_SET(pSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE) &&
		!(pSk->dwVnum == SKILL_MUYEONG && pVictim == this) && !(pSk->IsChargeSkill() && pVictim == this))
	{
		bool bAdded = false;

		if (pSk->bPointOn == POINT_HP && iAmount < 0)
		{
			int32_t iAG = 0;
			

			FuncSplashDamage f(pVictim->GetX(), pVictim->GetY(), pSk, this, iAmount, iAG, pSk->lMaxHit, pWeapon, m_bDisableCooltime, IsPC()?&m_SkillUseInfo[dwVnum]:NULL, GetSkillPower(dwVnum, bSkillLevel));
			if (IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
			{
				if (pVictim->GetSectree())
					pVictim->GetSectree()->ForEachAround(f);
			}
			else
			{
				f(pVictim);
			}
		}
		else
		{
			pSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
			int32_t iDur = (int32_t) pSk->kDurationPoly.Eval();
			

			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)) // Guild skills do not process cooldown.
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_MUYEONG)
					{
						return BATTLE_NONE;
					}

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
					pVictim->AddAffect(pSk->dwVnum, pSk->bPointOn, iAmount, pSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (pVictim->GetSectree())
					{
						FuncSplashAffect f(this, pVictim->GetX(), pVictim->GetY(), pSk->iSplashRange, pSk->dwVnum, pSk->bPointOn, iAmount, pSk->dwAffectFlag, iDur, 0, true, pSk->lMaxHit);
						pVictim->GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		if (pSk->bPointOn2 != POINT_NONE && !pSk->IsChargeSkill())
		{
			pSk->kDurationPoly2.SetVar("k", k/*bSkillLevel*/);
			int32_t iDur = (int32_t) pSk->kDurationPoly2.Eval();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
					pVictim->AddAffect(pSk->dwVnum, pSk->bPointOn2, iAmount2, pSk->dwAffectFlag2, iDur, 0, !bAdded);
				else
				{
					if (pVictim->GetSectree())
					{
						FuncSplashAffect f(this, pVictim->GetX(), pVictim->GetY(), pSk->iSplashRange, pSk->dwVnum, pSk->bPointOn2, iAmount2, pSk->dwAffectFlag2, iDur, 0, !bAdded, pSk->lMaxHit);
						pVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pVictim->PointChange(pSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pSk->bPointOn3 != POINT_NONE && !pSk->IsChargeSkill() && GetUsedSkillMasterType(pSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			pSk->kDurationPoly3.SetVar("k", k/*bSkillLevel*/);
			int32_t iDur = (int32_t) pSk->kDurationPoly3.Eval();


			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
					pVictim->AddAffect(pSk->dwVnum, pSk->bPointOn3, iAmount3, /*pSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded);
				else
				{
					if (pVictim->GetSectree())
					{
						FuncSplashAffect f(this, pVictim->GetX(), pVictim->GetY(), pSk->iSplashRange, pSk->dwVnum, pSk->bPointOn3, iAmount3, /*pSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded, pSk->lMaxHit);
						pVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pVictim->PointChange(pSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		if (dwVnum == SKILL_MUYEONG)
		{
			pSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
			pSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			int32_t iDur = (int32_t) pSk->kDurationPoly.Eval();
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

			if (pVictim == this)
				AddAffect(dwVnum,
						POINT_NONE, 0,
						AFF_MUYEONG, 
						iDur,
						(int32_t) pSk->kDurationSPCostPoly.Eval(),
						true);

			return BATTLE_NONE;
		}

		bool bAdded = false;
		pSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
		int32_t iDur = (int32_t) pSk->kDurationPoly.Eval();

		if (iDur > 0)
		{
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
			// If there is no AffectFlag or if it is not toggled...
			pSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			if (pSk->bPointOn2 != POINT_NONE)
			{
				pVictim->RemoveAffect(pSk->dwVnum);

				int32_t iDur2 = (int32_t) pSk->kDurationPoly2.Eval();

				if (iDur2 > 0)
				{
					if (test_server)
						PyLog("SKILL_AFFECT: {} {} Dur:{} To:{} Amount:{}", 
								GetName(),
								pSk->szName,
								iDur2,
								pSk->bPointOn2,
								iAmount2);

					iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);
					pVictim->AddAffect(pSk->dwVnum, pSk->bPointOn2, iAmount2, pSk->dwAffectFlag2, iDur2, 0, false);
				}
				else
				{
					pVictim->PointChange(pSk->bPointOn2, iAmount2);
				}

				uint32_t affact_flag = pSk->dwAffectFlag;

				if ((pSk->dwVnum == SKILL_CHUNKEON && GetUsedSkillMasterType(pSk->dwVnum) < SKILL_GRAND_MASTER))
					affact_flag = AFF_CHEONGEUN_WITH_FALL;

				pVictim->AddAffect(pSk->dwVnum,
						pSk->bPointOn,
						iAmount,
						affact_flag,
						iDur,
						(int32_t) pSk->kDurationSPCostPoly.Eval(),
						false);
			}
			else
			{
				if (test_server)
					PyLog("SKILL_AFFECT: {} {} Dur:{} To:{} Amount:{}", 
							GetName(),
							pSk->szName,
							iDur,
							pSk->bPointOn,
							iAmount);

				pVictim->AddAffect(pSk->dwVnum,
						pSk->bPointOn,
						iAmount,
						pSk->dwAffectFlag,
						iDur,
						(int32_t) pSk->kDurationSPCostPoly.Eval(),
						// ADD_GRANDMASTER_SKILL
						!bAdded);
				// END_OF_ADD_GRANDMASTER_SKILL
			}

			bAdded = true;
		}
		else
		{
			if (!pSk->IsChargeSkill())
				pVictim->PointChange(pSk->bPointOn, iAmount);

			if (pSk->bPointOn2 != POINT_NONE)
			{
				pVictim->RemoveAffect(pSk->dwVnum);

				int32_t iDur2 = (int32_t) pSk->kDurationPoly2.Eval();

				if (iDur2 > 0)
				{
					iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);

					if (pSk->IsChargeSkill())
						pVictim->AddAffect(pSk->dwVnum, pSk->bPointOn2, iAmount2, AFF_TANHWAN_DASH, iDur2, 0, false);
					else
						pVictim->AddAffect(pSk->dwVnum, pSk->bPointOn2, iAmount2, pSk->dwAffectFlag2, iDur2, 0, false);
				}
				else
				{
					pVictim->PointChange(pSk->bPointOn2, iAmount2);
				}

			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pSk->bPointOn3 != POINT_NONE && !pSk->IsChargeSkill() && GetUsedSkillMasterType(pSk->dwVnum) >= SKILL_GRAND_MASTER)
		{

			pSk->kDurationPoly3.SetVar("k", k/*bSkillLevel*/);
			int32_t iDur = (int32_t) pSk->kDurationPoly3.Eval();

			PyLog("try third {} {} {} {} 1894", pSk->dwVnum, pSk->bPointOn3, iDur, iAmount3);

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pSk->dwFlag, SKILL_FLAG_SPLASH))
					pVictim->AddAffect(pSk->dwVnum, pSk->bPointOn3, iAmount3, /*pSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded);
				else
				{
					if (pVictim->GetSectree())
					{
						FuncSplashAffect f(this, pVictim->GetX(), pVictim->GetY(), pSk->iSplashRange, pSk->dwVnum, pSk->bPointOn3, iAmount3, /*pSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded, pSk->lMaxHit);
						pVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pVictim->PointChange(pSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_NONE;
	}
}

bool CHARACTER::UseSkill(uint32_t dwVnum, LPCHARACTER pVictim, bool bUseGrandMaster)
{
	if (!CanUseSkill(dwVnum))
		return false;

	// NO_GRANDMASTER
	if (test_server)
	{
		if (quest::CQuestManager::GetInstance()->GetEventFlag("no_grand_master"))
		{
			bUseGrandMaster = false;
		}
	}
	// END_OF_NO_GRANDMASTER

	if (g_bSkillDisable)
		return false;

	if (IsObserverMode())
		return false;

	if (!CanMove())
		return false;

	if (IsPolymorphed())
		return false;

	const bool bCanUseHorseSkill = CanUseHorseSkill();


	if (dwVnum == SKILL_HORSE_SUMMON)
	{
		if (GetSkillLevel(dwVnum) == 0)
			return false;

		if (GetHorseLevel() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No Horse here. Ask the Stable Boy."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Please use an item to call a Horse."));

		return true;
	}

	// Return false if you are riding a horse but cannot use skills
	if (!bCanUseHorseSkill && IsRiding())
		return false;

	CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwVnum);
	TraceLog("{}: USE_SKILL: {}", GetName(), dwVnum);

	if (!pSk)
		return false;

	if (bCanUseHorseSkill && pSk->dwType != SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (!bCanUseHorseSkill && pSk->dwType == SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (GetSkillLevel(dwVnum) == 0)
		return false;
	

	// NO_GRANDMASTER
	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		bUseGrandMaster = false;
	// END_OF_NO_GRANDMASTER

	// MINING
	if (GetWear(WEAR_WEAPON) && (GetWear(WEAR_WEAPON)->GetType() == ITEM::TYPE_ROD || GetWear(WEAR_WEAPON)->GetType() == ITEM::TYPE_PICK))
		return false;
	// END_OF_MINING

	m_SkillUseInfo[dwVnum].TargetVIDMap.clear();

	if (pSk->IsChargeSkill())
	{
		if (IsAffectFlag(AFF_TANHWAN_DASH) || pVictim && pVictim != this)
		{
			if (!pVictim)
				return false;

			if (!IsAffectFlag(AFF_TANHWAN_DASH))
			{
				if (!UseSkill(dwVnum, this))
					return false;
			}

			m_SkillUseInfo[dwVnum].SetMainTargetVID(pVictim->GetVID());
			// Bullets in DASH state are attack skills
			ComputeSkill(dwVnum, pVictim);
			RemoveAffect(dwVnum);
			return true;
		}
	}

	if (dwVnum == SKILL_COMBO)
	{
		if (m_bComboIndex)
			m_bComboIndex = 0;
		else
			m_bComboIndex = GetSkillLevel(SKILL_COMBO);

		ChatPacket(CHAT_TYPE_COMMAND, "combo %d", m_bComboIndex);
		return true;
	}

	// When Toggle, SP is not used (Separated by SelfOnly)
	if ((0 != pSk->dwAffectFlag || pSk->dwVnum == SKILL_MUYEONG) && (pSk->dwFlag & SKILL_FLAG_TOGGLE) && RemoveAffect(pSk->dwVnum))
	{
		return true;
	}

	if (IsAffectFlag(AFF_REVIVE_INVISIBLE))
		RemoveAffect(AFFECT_REVIVE_INVISIBLE);

	const float k = 1.0 * GetSkillPower(pSk->dwVnum)* pSk->bMaxLevel / 100;

	pSk->SetPointVar("k", k);
	pSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	// check cooldown
	pSk->kCooldownPoly.SetVar("k", k);
	int32_t iCooltime = (int32_t) pSk->kCooldownPoly.Eval();
	int32_t lMaxHit = pSk->lMaxHit ? pSk->lMaxHit : -1;

	pSk->SetSPCostVar("k", k);

	uint32_t dwCur = get_dword_time();

	if (dwVnum == SKILL_TERROR && m_SkillUseInfo[dwVnum].bUsed && m_SkillUseInfo[dwVnum].dwNextSkillUsableTime > dwCur)
	{
		PyLog(" SKILL_TERROR's Cooltime is not delta over {}", m_SkillUseInfo[dwVnum].dwNextSkillUsableTime  - dwCur);
		return false;
	}

	int32_t iNeededSP = 0;

	if (IS_SET(pSk->dwFlag, SKILL_FLAG_USE_HP_AS_COST))
	{
		pSk->SetSPCostVar("maxhp", GetMaxHP());
		pSk->SetSPCostVar("v", GetHP());
		iNeededSP = (int32_t) pSk->kSPCostPoly.Eval();

		// ADD_GRANDMASTER_SKILL
		if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
		{
			iNeededSP = (int32_t) pSk->kGrandMasterAddSPCostPoly.Eval();
		}
		// END_OF_ADD_GRANDMASTER_SKILL	

		if (GetHP() < iNeededSP)
			return false;

		PointChange(POINT_HP, -iNeededSP);
	}
	else
	{
		// SKILL_FOMULA_REFACTORING
		pSk->SetSPCostVar("maxhp", GetMaxHP());
		pSk->SetSPCostVar("maxv", GetMaxSP());
		pSk->SetSPCostVar("v", GetSP());

		iNeededSP = (int32_t) pSk->kSPCostPoly.Eval();

		if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
		{
			iNeededSP = (int32_t) pSk->kGrandMasterAddSPCostPoly.Eval();
		}
		// END_OF_SKILL_FOMULA_REFACTORING

		if (GetSP() < iNeededSP)
			return false;

		if (test_server)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s FP-Consumption: %d"), pSk->szName, iNeededSP);

		PointChange(POINT_SP, -iNeededSP);
	}

	if (IS_SET(pSk->dwFlag, SKILL_FLAG_SELFONLY))
		pVictim = this;

	if (pSk->dwVnum == SKILL_MUYEONG || pSk->IsChargeSkill() && !IsAffectFlag(AFF_TANHWAN_DASH) && !pVictim)
	{
		// Youngjin Moo, who is used for the first time, attaches an Affect to himself.
		pVictim = this;
	}

	int32_t iSplashCount = 1;

	if (!m_bDisableCooltime)
	{
		if (!
				m_SkillUseInfo[dwVnum].UseSkill(
					bUseGrandMaster,
				   	(NULL != pVictim && SKILL_HORSE_WILDATTACK != dwVnum) ? pVictim->GetVID() : NULL,
				   	ComputeCooltime(iCooltime * 1000),
				   	iSplashCount,
				   	lMaxHit))
		{
			if (test_server)
				ChatPacket(CHAT_TYPE_NOTICE, "cooltime not finished %s %d", pSk->szName, iCooltime);

			return false;
		}
	}

	if (dwVnum == SKILL_CHAIN)
	{
		ResetChainLightningIndex();
		AddChainLightningExcept(pVictim);
	}
	

	if (IS_SET(pSk->dwFlag, SKILL_FLAG_SELFONLY))
		ComputeSkill(dwVnum, this);
	else if (!IS_SET(pSk->dwFlag, SKILL_FLAG_ATTACK))
		ComputeSkill(dwVnum, pVictim);
	else if (dwVnum == SKILL_BYEURAK)
		ComputeSkill(dwVnum, pVictim);
	else if (dwVnum == SKILL_MUYEONG || pSk->IsChargeSkill())
		ComputeSkill(dwVnum, pVictim);

	m_dwLastSkillTime = get_dword_time();

	return true;
}

int32_t CHARACTER::GetUsedSkillMasterType(uint32_t dwVnum)
{
	const TSkillUseInfo& rInfo = m_SkillUseInfo[dwVnum];

	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		return GetSkillMasterType(dwVnum);

	if (rInfo.isGrandMaster)
		return GetSkillMasterType(dwVnum);

	return MIN(GetSkillMasterType(dwVnum), SKILL_MASTER);
}

int32_t CHARACTER::GetSkillMasterType(uint32_t dwVnum) const
{
	if (!IsPC())
		return 0;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		SysLog("{} skill vnum overflow {}", GetName(), dwVnum);
		return 0;
	}

	return m_pSkillLevels ? m_pSkillLevels[dwVnum].bMasterType:SKILL_NORMAL;
}

int32_t CHARACTER::GetSkillPower(uint32_t dwVnum, uint8_t bLevel) const
{
	// mermaid ring item
	if (dwVnum >= SKILL_LANGUAGE1 && dwVnum <= SKILL_LANGUAGE3 && IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
	{
		return 100;
	}

	if (dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)
	{
		if (GetGuild())
			return 100 * GetGuild()->GetSkillLevel(dwVnum) / 7 / 7;
		else
			return 0;
	}

	if (bLevel)
	{
		//SKILL_POWER_BY_LEVEL
		return GetSkillPowerByLevel(bLevel, true);
		//END_SKILL_POWER_BY_LEVEL;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		SysLog("{} skill vnum overflow {}", GetName(), dwVnum);
		return 0;
	}

	//SKILL_POWER_BY_LEVEL
	return GetSkillPowerByLevel(GetSkillLevel(dwVnum));
	//SKILL_POWER_BY_LEVEL
}

int32_t CHARACTER::GetSkillLevel(uint32_t dwVnum) const
{
	if (dwVnum >= SKILL_MAX_NUM)
	{
		SysLog("{} skill vnum overflow {}", GetName(), dwVnum);
		PyLog("{} skill vnum overflow {}", GetName(), dwVnum);
		return 0;
	}

	return MIN(SKILL_MAX_LEVEL, m_pSkillLevels ? m_pSkillLevels[dwVnum].bLevel : 0);
}

EVENTFUNC(skill_muyoung_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("skill_muyoung_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == nullptr) { // <Factor>
		return 0;
	}

	if (!ch->IsAffectFlag(AFF_MUYEONG))
	{
		ch->StopMuyeongEvent();
		return 0;
	}

	// 1. Find Victim
	FFindNearVictim f(ch, ch);
	if (ch->GetSectree())
	{
		ch->GetSectree()->ForEachAround(f);
		// 2. Shoot!
		if (f.GetVictim())
		{
			ch->CreateFly(FLY_SKILL_MUYEONG, f.GetVictim());
			ch->ComputeSkill(SKILL_MUYEONG, f.GetVictim());
		}
	}

	return PASSES_PER_SEC(3);
}

void CHARACTER::StartMuyeongEvent()
{
	if (m_pMuyeongEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pMuyeongEvent = event_create(skill_muyoung_event, info, PASSES_PER_SEC(1));
}

void CHARACTER::StopMuyeongEvent()
{
	event_cancel(&m_pMuyeongEvent);
}

void CHARACTER::SkillLearnWaitMoreTimeMessage(uint32_t ms)
{
	//const char* str = "";
	//
	if (ms < 3 * 60)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I am burning inside, but it is calming down my body. My Chi has to stabilise."));
	else if (ms < 5 * 60)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("A little slow, but steady... Without stopping!"));
	else if (ms < 10 * 60) // 10분
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Yes, that feels great. My body is full of Chi."));
	else if (ms < 30 * 60) // 30분
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I have read it! Now the Chi will spread through my body."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("The training is completed."));
	}
	else if (ms < 1 * 3600) // 1시간
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I am on the last page of the book. The training is nearly finished!"));
	else if (ms < 2 * 3600) // 2시간
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Nearly finished! Just a little bit more to go!"));
	else if (ms < 3 * 3600)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Eureka! I have nearly finished reading it!"));
	else if (ms < 6 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Only a few more pages and then I'll have read everything."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I feel refreshed."));
	}
	else if (ms < 12 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Now I understand it!"));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Okay I have to stay concentrated!"));
	}
	else if (ms < 18 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I keep reading the same line over and over again."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I do not want to learn any more."));
	}
	else //if (ms < 2 * 86400)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("It is a lot more complicated and more difficult to understand than I thought."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("It's hard for me to concentrate. I should take a break."));
	}
	/*
	   str = "30%";
	   else if (ms < 3 * 86400)
	   str = "10%";
	   else if (ms < 4 * 86400)
	   str = "5%";
	   else
	   str = "0%";*/

	//ChatPacket(CHAT_TYPE_TALKING, "%s", str);
}

void CHARACTER::DisableCooltime()
{
	m_bDisableCooltime = true;
}

bool CHARACTER::HasMobSkill() const
{
	return CountMobSkill() > 0;
}

size_t CHARACTER::CountMobSkill() const
{
	if (!m_pMobData)
		return 0;

	size_t c = 0;

	for (size_t i = 0; i < MOB::SKILL_MAX_NUM; ++i)
		if (m_pMobData->m_table.Skills[i].dwVnum)
			++c;

	return c;
}

const TMobSkillInfo* CHARACTER::GetMobSkill(uint32_t idx) const
{
	if (idx >= MOB::SKILL_MAX_NUM)
		return NULL;

	if (!m_pMobData)
		return NULL;

	if (0 == m_pMobData->m_table.Skills[idx].dwVnum)
		return NULL;

	return &m_pMobData->m_mobSkillInfo[idx];
}

bool CHARACTER::CanUseMobSkill(uint32_t idx) const
{
	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
		return false;

	if (m_adwMobSkillCooltime[idx] > get_dword_time())
		return false;

	if (number(0, 1))
		return false;

	return true;
}

EVENTINFO(mob_skill_event_info)
{
	DynamicCharacterPtr ch;
	PIXEL_POSITION pos;
	uint32_t vnum;
	int32_t index;
	uint8_t level;

	mob_skill_event_info()
	: ch()
	, pos()
	, vnum(0)
	, index(0)
	, level(0)
	{
	}
};

EVENTFUNC(mob_skill_hit_event)
{
	mob_skill_event_info * info = dynamic_cast<mob_skill_event_info *>(event->info);

	if (info == nullptr)
	{
		SysLog("mob_skill_event_info> <Factor> Null pointer");
		return 0;
	}

	// <Factor>
	LPCHARACTER ch = info->ch;
	if (ch == nullptr) {
		return 0;
	}

	ch->ComputeSkillAtPosition(info->vnum, info->pos, info->level);
	ch->m_mapMobSkillEvent.erase(info->index);

	return 0;
}

bool CHARACTER::UseMobSkill(uint32_t idx)
{
	if (IsPC())
		return false;

	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
		return false;

	uint32_t dwVnum = pInfo->dwSkillVnum;
	CSkillProto* pSk = CSkillManager::GetInstance()->Get(dwVnum);

	if (!pSk)
		return false;

	const float k = 1.0 * GetSkillPower(pSk->dwVnum, pInfo->bSkillLevel)* pSk->bMaxLevel / 100;

	pSk->kCooldownPoly.SetVar("k", k);
	int32_t iCooltime = (int32_t) (pSk->kCooldownPoly.Eval() * 1000);

	m_adwMobSkillCooltime[idx] = get_dword_time() + iCooltime;

	PyLog("USE_MOB_SKILL: {} idx {} vnum {} cooltime {}", GetName(), idx, dwVnum, iCooltime);

	if (m_pMobData->m_mobSkillInfo[idx].vecSplashAttack.empty())
	{
		SysLog("No skill hit data for mob {} index {}", GetName(), idx);
		return false;
	}

	for (size_t i = 0; i < m_pMobData->m_mobSkillInfo[idx].vecSplashAttack.size(); i++)
	{
		PIXEL_POSITION pos = GetXYZ();
		const TMobSplashAttackInfo& rInfo = m_pMobData->m_mobSkillInfo[idx].vecSplashAttack[i];

		if (rInfo.dwHitDistance)
		{
			float fx, fy;
			GetDeltaByDegree(GetRotation(), rInfo.dwHitDistance, &fx, &fy);
			pos.x += (int32_t) fx;
			pos.y += (int32_t) fy;
		}

		if (rInfo.dwTiming)
		{
			if (test_server)
				PyLog("               timing {}ms", rInfo.dwTiming);

			mob_skill_event_info* info = AllocEventInfo<mob_skill_event_info>();

			info->ch = this;
			info->pos = pos;
			info->level = pInfo->bSkillLevel;
			info->vnum = dwVnum;
			info->index = i;

			// <Factor> Cancel existing event first
			auto it = m_mapMobSkillEvent.find(i);
			if (it != m_mapMobSkillEvent.end()) {
				LPEVENT existing = it->second;
				event_cancel(&existing);
				m_mapMobSkillEvent.erase(it);
			}

			m_mapMobSkillEvent.insert(std::make_pair(i, event_create(mob_skill_hit_event, info, PASSES_PER_SEC(rInfo.dwTiming) / 1000)));
		}
		else
		{
			ComputeSkillAtPosition(dwVnum, pos, pInfo->bSkillLevel);
		}
	}

	return true;
}

void CHARACTER::ResetMobSkillCooltime()
{
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
}

bool CHARACTER::IsUsableSkillMotion(uint32_t dwMotionIndex) const
{
	uint32_t selfJobGroup = (GetJob()+1) * 10 + GetSkillGroup();

	const uint32_t SKILL_NUM = 158;
	static uint32_t s_anSkill2JobGroup[SKILL_NUM] = {
		0, // common_skill 0
		11, // job_skill 1
		11, // job_skill 2
		11, // job_skill 3
		11, // job_skill 4
		11, // job_skill 5
		11, // job_skill 6
		0, // common_skill 7
		0, // common_skill 8
		0, // common_skill 9
		0, // common_skill 10
		0, // common_skill 11
		0, // common_skill 12
		0, // common_skill 13
		0, // common_skill 14
		0, // common_skill 15
		12, // job_skill 16
		12, // job_skill 17
		12, // job_skill 18
		12, // job_skill 19
		12, // job_skill 20
		12, // job_skill 21
		0, // common_skill 22
		0, // common_skill 23
		0, // common_skill 24
		0, // common_skill 25
		0, // common_skill 26
		0, // common_skill 27
		0, // common_skill 28
		0, // common_skill 29
		0, // common_skill 30
		21, // job_skill 31
		21, // job_skill 32
		21, // job_skill 33
		21, // job_skill 34
		21, // job_skill 35
		21, // job_skill 36
		0, // common_skill 37
		0, // common_skill 38
		0, // common_skill 39
		0, // common_skill 40
		0, // common_skill 41
		0, // common_skill 42
		0, // common_skill 43
		0, // common_skill 44
		0, // common_skill 45
		22, // job_skill 46
		22, // job_skill 47
		22, // job_skill 48
		22, // job_skill 49
		22, // job_skill 50
		22, // job_skill 51
		0, // common_skill 52
		0, // common_skill 53
		0, // common_skill 54
		0, // common_skill 55
		0, // common_skill 56
		0, // common_skill 57
		0, // common_skill 58
		0, // common_skill 59
		0, // common_skill 60
		31, // job_skill 61
		31, // job_skill 62
		31, // job_skill 63
		31, // job_skill 64
		31, // job_skill 65
		31, // job_skill 66
		0, // common_skill 67
		0, // common_skill 68
		0, // common_skill 69
		0, // common_skill 70
		0, // common_skill 71
		0, // common_skill 72
		0, // common_skill 73
		0, // common_skill 74
		0, // common_skill 75
		32, // job_skill 76
		32, // job_skill 77
		32, // job_skill 78
		32, // job_skill 79
		32, // job_skill 80
		32, // job_skill 81
		0, // common_skill 82
		0, // common_skill 83
		0, // common_skill 84
		0, // common_skill 85
		0, // common_skill 86
		0, // common_skill 87
		0, // common_skill 88
		0, // common_skill 89
		0, // common_skill 90
		41, // job_skill 91
		41, // job_skill 92
		41, // job_skill 93
		41, // job_skill 94
		41, // job_skill 95
		41, // job_skill 96
		0, // common_skill 97
		0, // common_skill 98
		0, // common_skill 99
		0, // common_skill 100
		0, // common_skill 101
		0, // common_skill 102
		0, // common_skill 103
		0, // common_skill 104
		0, // common_skill 105
		42, // job_skill 106
		42, // job_skill 107
		42, // job_skill 108
		42, // job_skill 109
		42, // job_skill 110
		42, // job_skill 111
		0, // common_skill 112
		0, // common_skill 113
		0, // common_skill 114
		0, // common_skill 115
		0, // common_skill 116
		0, // common_skill 117
		0, // common_skill 118
		0, // common_skill 119
		0, // common_skill 120
		0, // common_skill 121
		0, // common_skill 122
		0, // common_skill 123
		0, // common_skill 124
		0, // common_skill 125
		0, // common_skill 126
		0, // common_skill 127
		0, // common_skill 128
		0, // common_skill 129
		0, // common_skill 130
		0, // common_skill 131
		0, // common_skill 132
		0, // common_skill 133
		0, // common_skill 134
		0, // common_skill 135
		0, // common_skill 136
		0, // job_skill 137
		0, // job_skill 138
		0, // job_skill 139
		0, // job_skill 140
		0, // common_skill 141
		0, // common_skill 142
		0, // common_skill 143
		0, // common_skill 144
		0, // common_skill 145
		0, // common_skill 146
		0, // common_skill 147
		0, // common_skill 148
		0, // common_skill 149
		0, // common_skill 150
		0, // common_skill 151
		0, // job_skill 152
		0, // job_skill 153
		0, // job_skill 154
		0, // job_skill 155
		0, // job_skill 156
		0, // job_skill 157
	}; // s_anSkill2JobGroup

	const uint32_t MOTION_MAX_NUM 	= 124;
	const uint32_t SKILL_LIST_MAX_COUNT	= 5;

	static uint32_t s_anMotion2SkillVnumList[MOTION_MAX_NUM][SKILL_LIST_MAX_COUNT] =
	{
		// Number of skills Warrior skill ID Assassin skill ID Sura skill ID Shaman skill ID
		{   0,		0,			0,			0,			0		}, //  0

		{   4,		1,			31,			61,			91		}, //  1
		{   4,		2,			32,			62,			92		}, //  2
		{   4,		3,			33,			63,			93		}, //  3
		{   4,		4,			34,			64,			94		}, //  4
		{   4,		5,			35,			65,			95		}, //  5
		{   4,		6,			36,			66,			96		}, //  6
		{   0,		0,			0,			0,			0		}, //  7
		{   0,		0,			0,			0,			0		}, //  8


		{   0,		0,			0,			0,			0		}, //  9
		{   0,		0,			0,			0,			0		}, //  10
		{   0,		0,			0,			0,			0		}, //  11
		{   0,		0,			0,			0,			0		}, //  12
		{   0,		0,			0,			0,			0		}, //  13
		{   0,		0,			0,			0,			0		}, //  14
		{   0,		0,			0,			0,			0		}, //  15

		{   4,		16,			46,			76,			106		}, //  16
		{   4,		17,			47,			77,			107		}, //  17
		{   4,		18,			48,			78,			108		}, //  18
		{   4,		19,			49,			79,			109		}, //  19
		{   4,		20,			50,			80,			110		}, //  20
		{   4,		21,			51,			81,			111		}, //  21
		{   0,		0,			0,			0,			0		}, //  22
		{   0,		0,			0,			0,			0		}, //  23

		{   0,		0,			0,			0,			0		}, //  24
		{   0,		0,			0,			0,			0		}, //  25

		{   4,		1,			31,			61,			91		}, //  26
		{   4,		2,			32,			62,			92		}, //  27
		{   4,		3,			33,			63,			93		}, //  28
		{   4,		4,			34,			64,			94		}, //  29
		{   4,		5,			35,			65,			95		}, //  30
		{   4,		6,			36,			66,			96		}, //  31
		{   0,		0,			0,			0,			0		}, //  32
		{   0,		0,			0,			0,			0		}, //  33

		{   0,		0,			0,			0,			0		}, //  34
		{   0,		0,			0,			0,			0		}, //  35
		{   0,		0,			0,			0,			0		}, //  36
		{   0,		0,			0,			0,			0		}, //  37
		{   0,		0,			0,			0,			0		}, //  38
		{   0,		0,			0,			0,			0		}, //  39
		{   0,		0,			0,			0,			0		}, //  40

		{   4,		16,			46,			76,			106		}, //  41
		{   4,		17,			47,			77,			107		}, //  42
		{   4,		18,			48,			78,			108		}, //  43
		{   4,		19,			49,			79,			109		}, //  44
		{   4,		20,			50,			80,			110		}, //  45
		{   4,		21,			51,			81,			111		}, //  46
		{   0,		0,			0,			0,			0		}, //  47
		{   0,		0,			0,			0,			0		}, //  48

		{   0,		0,			0,			0,			0		}, //  49
		{   0,		0,			0,			0,			0		}, //  50

		{   4,		1,			31,			61,			91		}, //  51
		{   4,		2,			32,			62,			92		}, //  52
		{   4,		3,			33,			63,			93		}, //  53
		{   4,		4,			34,			64,			94		}, //  54
		{   4,		5,			35,			65,			95		}, //  55
		{   4,		6,			36,			66,			96		}, //  56
		{   0,		0,			0,			0,			0		}, //  57
		{   0,		0,			0,			0,			0		}, //  58

		{   0,		0,			0,			0,			0		}, //  59
		{   0,		0,			0,			0,			0		}, //  60
		{   0,		0,			0,			0,			0		}, //  61
		{   0,		0,			0,			0,			0		}, //  62
		{   0,		0,			0,			0,			0		}, //  63
		{   0,		0,			0,			0,			0		}, //  64
		{   0,		0,			0,			0,			0		}, //  65

		{   4,		16,			46,			76,			106		}, //  66
		{   4,		17,			47,			77,			107		}, //  67
		{   4,		18,			48,			78,			108		}, //  68
		{   4,		19,			49,			79,			109		}, //  69
		{   4,		20,			50,			80,			110		}, //  70
		{   4,		21,			51,			81,			111		}, //  71
		{   0,		0,			0,			0,			0		}, //  72
		{   0,		0,			0,			0,			0		}, //  73

		{   0,		0,			0,			0,			0		}, //  74
		{   0,		0,			0,			0,			0		}, //  75

		{   4,		1,			31,			61,			91		}, //  76
		{   4,		2,			32,			62,			92		}, //  77
		{   4,		3,			33,			63,			93		}, //  78
		{   4,		4,			34,			64,			94		}, //  79
		{   4,		5,			35,			65,			95		}, //  80
		{   4,		6,			36,			66,			96		}, //  81
		{   0,		0,			0,			0,			0		}, //  82
		{   0,		0,			0,			0,			0		}, //  83

		{   0,		0,			0,			0,			0		}, //  84
		{   0,		0,			0,			0,			0		}, //  85
		{   0,		0,			0,			0,			0		}, //  86
		{   0,		0,			0,			0,			0		}, //  87
		{   0,		0,			0,			0,			0		}, //  88
		{   0,		0,			0,			0,			0		}, //  89
		{   0,		0,			0,			0,			0		}, //  90

		{   4,		16,			46,			76,			106		}, //  91
		{   4,		17,			47,			77,			107		}, //  92
		{   4,		18,			48,			78,			108		}, //  93
		{   4,		19,			49,			79,			109		}, //  94
		{   4,		20,			50,			80,			110		}, //  95
		{   4,		21,			51,			81,			111		}, //  96
		{   0,		0,			0,			0,			0		}, //  97
		{   0,		0,			0,			0,			0		}, //  98

		{   0,		0,			0,			0,			0		}, //  99
		{   0,		0,			0,			0,			0		}, //  100

		{   1,  152,    0,    0,    0}, //  101
		{   1,  153,    0,    0,    0}, //  102
		{   1,  154,    0,    0,    0}, //  103
		{   1,  155,    0,    0,    0}, //  104
		{   1,  156,    0,    0,    0}, //  105
		{   1,  157,    0,    0,    0}, //  106

		{   0,    0,    0,    0,    0}, //  107
		{   0,    0,    0,    0,    0}, //  108
		{   0,    0,    0,    0,    0}, //  109
		{   0,    0,    0,    0,    0}, //  110
		{   0,    0,    0,    0,    0}, //  111
		{   0,    0,    0,    0,    0}, //  112
		{   0,    0,    0,    0,    0}, //  113
		{   0,    0,    0,    0,    0}, //  114
		{   0,    0,    0,    0,    0}, //  115
		{   0,    0,    0,    0,    0}, //  116
		{   0,    0,    0,    0,    0}, //  117
		{   0,    0,    0,    0,    0}, //  118
		{   0,    0,    0,    0,    0}, //  119
		{   0,    0,    0,    0,    0}, //  120

		{   2,  137,  140,    0,    0}, //  121
		{   1,  138,    0,    0,    0}, //  122
		{   1,  139,    0,    0,    0}, //  123
	};

	if (dwMotionIndex >= MOTION_MAX_NUM)
	{
		SysLog("OUT_OF_MOTION_VNUM: name={}, motion={}/{}", GetName(), dwMotionIndex, MOTION_MAX_NUM);
		return false;
	}

	uint32_t* skillVNums = s_anMotion2SkillVnumList[dwMotionIndex];

	uint32_t skillCount = *skillVNums++;
	if (skillCount >= SKILL_LIST_MAX_COUNT)
	{
		SysLog("OUT_OF_SKILL_LIST: name={}, count={}/{}", GetName(), skillCount, SKILL_LIST_MAX_COUNT);
		return false;
	}

	for (uint32_t skillIndex = 0; skillIndex != skillCount; ++skillIndex)
	{
		if (skillIndex >= SKILL_MAX_NUM)
		{
			SysLog("OUT_OF_SKILL_VNUM: name={}, skill={}/{}", GetName(), skillIndex, SKILL_MAX_NUM);
			return false;
		}

		uint32_t eachSkillVNum = skillVNums[skillIndex];
		if (eachSkillVNum != 0)
		{
			uint32_t eachJobGroup = s_anSkill2JobGroup[eachSkillVNum];

			if (0 == eachJobGroup || eachJobGroup == selfJobGroup)
			{
				// GUILDSKILL_BUG_FIX
				uint32_t eachSkillLevel = 0;

				if (eachSkillVNum >= GUILD_SKILL_START && eachSkillVNum <= GUILD_SKILL_END)
				{
					if (GetGuild())
						eachSkillLevel = GetGuild()->GetSkillLevel(eachSkillVNum);
					else
						eachSkillLevel = 0;
				}
				else
				{
					eachSkillLevel = GetSkillLevel(eachSkillVNum);
				}

				if (eachSkillLevel > 0)
				{
					return true;
				}
				// END_OF_GUILDSKILL_BUG_FIX
			}
		}
	}

	return false;
}

void CHARACTER::ClearSkill()
{
	PointChange(POINT_SKILL, 4 + (GetLevel() - 5) - GetPoint(POINT_SKILL));

	ResetSkill();
}

void CHARACTER::ClearSubSkill()
{
	PointChange(POINT_SUB_SKILL, GetLevel() < 10 ? 0 : (GetLevel() - 9) - GetPoint(POINT_SUB_SKILL));

	if (m_pSkillLevels == nullptr)
	{
		SysLog("m_pSkillLevels nil (name: {})", GetName());
		return;
	}

	TPlayerSkill CleanSkill;
	memset(&CleanSkill, 0, sizeof(TPlayerSkill));

	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		m_pSkillLevels[s_adwSubSkillVnums[i]] = CleanSkill;
	}

	ComputePoints();
	SkillLevelPacket();
}

bool CHARACTER::ResetOneSkill(uint32_t dwVnum)
{
	if (!m_pSkillLevels)
	{
		SysLog("m_pSkillLevels nil (name {}, vnum {})", GetName(), dwVnum);
		return false;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		SysLog("vnum overflow (name {}, vnum {})", GetName(), dwVnum);
		return false;
	}

	uint8_t level = m_pSkillLevels[dwVnum].bLevel;

	m_pSkillLevels[dwVnum].bLevel = 0;
	m_pSkillLevels[dwVnum].bMasterType = 0;
	m_pSkillLevels[dwVnum].tNextRead = 0;

	if (level > 17)
		level = 17;

	PointChange(POINT_SKILL, level);

	LogManager::GetInstance()->CharLog(this, dwVnum, "ONE_SKILL_RESET_BY_SCROLL", "");

	ComputePoints();
	SkillLevelPacket();

	return true;
}

bool CHARACTER::CanUseSkill(uint32_t dwSkillVnum) const
{
	if (0 == dwSkillVnum) return false;

	if (0 < GetSkillGroup())
	{
		const int32_t SKILL_COUNT = 6;
		static const uint32_t SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][SKILL_COUNT] =
		{
			{ {	1,	2,	3,	4,	5,	6	}, {	16,	17,	18,	19,	20,	21	} },
			{ {	31,	32,	33,	34,	35,	36	}, {	46,	47,	48,	49,	50,	51	} },
			{ {	61,	62,	63,	64,	65,	66	}, {	76,	77,	78,	79,	80,	81	} },
			{ {	91,	92,	93,	94,	95,	96	}, {	106,107,108,109,110,111	} },
		};

		const uint32_t* pSkill = SkillList[ GetJob() ][ GetSkillGroup()-1 ];

		for (int32_t i=0 ; i < SKILL_COUNT ; ++i)
		{
			if (pSkill[i] == dwSkillVnum) return true;
		}
	}

	//if (IsHorseRiding())
	
	if (IsRiding())
	{
		// Among mount mounts, only advanced horses can use skills
		if(GetMountVnum())
		{
			if(!((GetMountVnum() >= 20209 && GetMountVnum() <= 20212)	||
				GetMountVnum() == 20215 || GetMountVnum() == 20218 || GetMountVnum() == 20225	)	)
					return false;
		}

		switch(dwSkillVnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				return true;
		}
	}

	switch(dwSkillVnum)
	{
		case 121: case 122: case 124: case 126: case 127: case 128: case 129: case 130:
		case 131:
		case 151: case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
			return true;
	}

	return false;
}

bool CHARACTER::CheckSkillHitCount(const uint8_t SkillID, const VID TargetVID)
{
	std::map<int32_t, TSkillUseInfo>::iterator iter = m_SkillUseInfo.find(SkillID);

	if (iter == m_SkillUseInfo.end())
	{
		PyLog("SkillHack: Skill({}) is not in container", SkillID);
		return false;
	}

	TSkillUseInfo& rSkillUseInfo = iter->second;

	if (!rSkillUseInfo.bUsed)
	{
		PyLog("SkillHack: not used skill({})", SkillID);
		return false;
	}

	switch (SkillID)
	{
		case SKILL_YONGKWON:
		case SKILL_HWAYEOMPOK:
		case SKILL_DAEJINGAK:
		case SKILL_PAERYONG:
			PyLog("SkillHack: cannot use attack packet for skill({})", SkillID);
			return false;
	}

	auto iterTargetMap = rSkillUseInfo.TargetVIDMap.find(TargetVID);

	if (rSkillUseInfo.TargetVIDMap.end() != iterTargetMap)
	{
		size_t MaxAttackCountPerTarget = 1;

		switch (SkillID)
		{
			case SKILL_SAMYEON:
			case SKILL_CHARYUN:
				MaxAttackCountPerTarget = 3;
				break;

			case SKILL_HORSE_WILDATTACK_RANGE:
				MaxAttackCountPerTarget = 5;
				break;

			case SKILL_YEONSA:
				MaxAttackCountPerTarget = 7;
				break;

			case SKILL_HORSE_ESCAPE:
				MaxAttackCountPerTarget = 10;
				break;
		}

		if (iterTargetMap->second >= MaxAttackCountPerTarget)
		{
			PyLog("SkillHack: Too Many Hit count from SkillID({}) count({})", SkillID, iterTargetMap->second);
			return false;
		}

		iterTargetMap->second++;
	}
	else
	{
		rSkillUseInfo.TargetVIDMap.insert(std::make_pair(TargetVID, 1));
	}

	return true;
}

