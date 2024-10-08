
#include "stdafx.h"

#include "DragonLair.h"

#include "entity.h"
#include "sectree_manager.h"
#include "char.h"
#include "guild.h"
#include "locale_service.h"
#include "regen.h"
#include "log.h"
#include "utils.h"

extern int32_t passes_per_sec;

struct FWarpToDragronLairWithGuildMembers
{
	uint32_t dwGuildID;
	int32_t mapIndex;
	int32_t x, y;

	FWarpToDragronLairWithGuildMembers(uint32_t guildID, int32_t map, int32_t X, int32_t Y)
		: dwGuildID(guildID), mapIndex(map), x(X), y(Y)
	{
	}

	void operator()(LPENTITY ent)
	{
		if (NULL != ent && ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (pChar->IsPC())
			{
				if (NULL != pChar->GetGuild())
				{
					if (dwGuildID == pChar->GetGuild()->GetID())
					{
						pChar->WarpSet(x, y, mapIndex);
					}
				}
			}
		}
	}
};

struct FWarpToVillage
{
	void operator() (LPENTITY ent)
	{
		if (NULL != ent)
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (NULL != pChar)
			{
				if (pChar->IsPC())
				{
					pChar->GoHome();
				}
			}
		}
	}
};

EVENTINFO(tag_DragonLair_Collapse_EventInfo)
{
	int32_t step;
	CDragonLair* pLair;
	int32_t InstanceMapIndex;

	tag_DragonLair_Collapse_EventInfo()
	: step(0)
	, pLair(0)
	, InstanceMapIndex(0)
	{
	}
};

EVENTFUNC(DragonLair_Collapse_Event)
{
	tag_DragonLair_Collapse_EventInfo* pInfo = dynamic_cast<tag_DragonLair_Collapse_EventInfo*>(event->info);

	if (pInfo == nullptr)
	{
		SysLog("DragonLair_Collapse_Event> <Factor> Null pointer");
		return 0;
	}

	if (0 == pInfo->step)
	{
		char buf[512];
		snprintf(buf, 512, LC_TEXT("Dragon will die in %d seconds."), pInfo->pLair->GetEstimatedTime());
		SendNoticeMap(buf, pInfo->InstanceMapIndex, true);

		pInfo->step++;

		return PASSES_PER_SEC(30);
	}
	else if (1 == pInfo->step)
	{
		LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(pInfo->InstanceMapIndex);

		if (NULL != pMap)
		{
			FWarpToVillage f;
			pMap->for_each(f);
		}

		pInfo->step++;

		return PASSES_PER_SEC(30);
	}
	else
	{
		SECTREE_MANAGER::GetInstance()->DestroyPrivateMap(pInfo->InstanceMapIndex);
		M2_DELETE(pInfo->pLair);
	}

	return 0;
}









CDragonLair::CDragonLair(uint32_t guildID, int32_t BaseMapID, int32_t PrivateMapID)
	: GuildID_(guildID), BaseMapIndex_(BaseMapID), PrivateMapIndex_(PrivateMapID)
{
	StartTime_ = get_global_time();
}

CDragonLair::~CDragonLair()
{
}

uint32_t CDragonLair::GetEstimatedTime() const
{
	return get_global_time() - StartTime_;
}

void CDragonLair::OnDragonDead(LPCHARACTER pDragon)
{
	PyLog("DragonLair: ������� �׾��ȿ");

	LogManager::GetInstance()->DragonSlayLog(GuildID_, pDragon->GetMobTable().dwVnum, StartTime_, get_global_time());
}












CDragonLairManager::CDragonLairManager()
{
}

CDragonLairManager::~CDragonLairManager()
{
}

bool CDragonLairManager::Start(int32_t MapIndexFrom, int32_t BaseMapIndex, uint32_t GuildID)
{
	int32_t instanceMapIndex = SECTREE_MANAGER::GetInstance()->CreatePrivateMap(BaseMapIndex);
	if (instanceMapIndex == 0) {
		SysLog("CDragonLairManager::Start() : no private map index available");
		return false;
	}

	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(MapIndexFrom);

	if (NULL != pMap)
	{
		LPSECTREE_MAP pTargetMap = SECTREE_MANAGER::GetInstance()->GetMap(BaseMapIndex);

		if (!pTargetMap)
		{
			return false;
		}

		const TMapRegion* pRegionInfo = SECTREE_MANAGER::GetInstance()->GetMapRegion(pTargetMap->m_setting.iIndex);

		if (NULL != pRegionInfo)
		{
			FWarpToDragronLairWithGuildMembers f(GuildID, instanceMapIndex, 844000, 1066900);

			pMap->for_each(f);

			LairMap_.insert(std::make_pair(GuildID, M2_NEW CDragonLair(GuildID, BaseMapIndex, instanceMapIndex)));

			std::string strMapBasePath(LocaleService_GetMapPath());

			strMapBasePath += "/" + pRegionInfo->strMapName + "/instance_regen.txt";

			PyLog("{}", strMapBasePath.c_str());

			regen_do(strMapBasePath.c_str(), instanceMapIndex, pTargetMap->m_setting.iBaseX, pTargetMap->m_setting.iBaseY, NULL, true);

			return true;
		}
	}

	return false;
}

void CDragonLairManager::OnDragonDead(LPCHARACTER pDragon, uint32_t KillerGuildID)
{
	if (!pDragon)
		return;

	if (!pDragon->IsMonster())
		return;

	boost::unordered_map<uint32_t, CDragonLair*>::iterator iter = LairMap_.find(KillerGuildID);

	if (LairMap_.end() == iter)
	{
		return;
	}

	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(pDragon->GetMapIndex());

	if (!iter->second || !pMap)
	{
		LairMap_.erase(iter);
		return;
	}

	iter->second->OnDragonDead(pDragon);

	tag_DragonLair_Collapse_EventInfo* info;
	info = AllocEventInfo<tag_DragonLair_Collapse_EventInfo>();

	info->step = 0;
	info->pLair = iter->second;
	info->InstanceMapIndex = pDragon->GetMapIndex();

	event_create(DragonLair_Collapse_Event, info, PASSES_PER_SEC(10));

	LairMap_.erase(iter);
}

