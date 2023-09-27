#include "stdafx.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "sectree_manager.h"
#include "config.h"
#include "char.h"
#include "wedding.h"
#include "regen.h"
#include "locale_service.h"

namespace marriage
{
	EVENTINFO(wedding_map_info)
	{
		WeddingMap* pWeddingMap;
		int32_t iStep;

		wedding_map_info()
		: pWeddingMap(0)
		, iStep(0)
		{
		}
	};

	EVENTFUNC(wedding_end_event)
	{
		wedding_map_info* info = dynamic_cast<wedding_map_info*>(event->info);

		if (info == nullptr)
		{
			SysLog("wedding_end_event> <Factor> Null pointer");
			return 0;
		}
		
		WeddingMap* pMap = info->pWeddingMap;

		if (info->iStep == 0)
		{
			++info->iStep;
			pMap->WarpAll(); 
			return PASSES_PER_SEC(15);
		}
		WeddingManager::GetInstance()->DestroyWeddingMap(pMap);
		return 0;
	}

	// Map instance
	WeddingMap::WeddingMap(uint32_t dwMapIndex, uint32_t dwPID1, uint32_t dwPID2) :
		m_dwMapIndex(dwMapIndex),
		m_pEndEvent(nullptr),
		m_isDark(false),
		m_isSnow(false),
		m_isMusic(false),
		dwPID1(dwPID1),
		dwPID2(dwPID2)
	{
	}

	WeddingMap::~WeddingMap()
	{
		event_cancel(&m_pEndEvent);
	}

	void WeddingMap::SetEnded()
	{
		if (m_pEndEvent)
		{
			SysLog("WeddingMap::SetEnded - ALREADY EndEvent");
			return;
		}

		wedding_map_info* info = AllocEventInfo<wedding_map_info>();

		info->pWeddingMap = this;

		m_pEndEvent = event_create(wedding_end_event, info, PASSES_PER_SEC(5));

		Notice(LC_TEXT("The wedding is finishing soon."));
		Notice(LC_TEXT("Will be left automatically."));

		for (auto it = m_set_pChr.begin(); it != m_set_pChr.end(); ++it)
		{
			LPCHARACTER ch = *it;
			if (ch->GetPlayerID() == dwPID1 || ch->GetPlayerID() == dwPID2)
				continue;

			if (ch->GetLevel() < 10) // Level 10 and below are not allowed.
				continue;

			//ch->AutoGiveItem(27003, 5);
			ch->AutoGiveItem(27002, 5);
		}
	}

	struct FNotice
	{
		FNotice(const char* psz) :
			m_psz(psz)
		{
		}

		void operator() (LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
		}

		const char* m_psz;
	};

	void WeddingMap::Notice(const char* psz)
	{
		FNotice f(psz);
		std::for_each(m_set_pChr.begin(), m_set_pChr.end(), f);
	}

	struct FWarpEveryone
	{
		void operator() (LPCHARACTER ch)
		{
			if (ch->IsPC())
			{
				ch->ExitToSavedLocation();
			}
		}
	};

	void WeddingMap::WarpAll()
	{
		FWarpEveryone f;
		std::for_each(m_set_pChr.begin(), m_set_pChr.end(), f);
	}

	struct FDestroyEveryone
	{
		void operator() (LPCHARACTER ch)
		{
			PyLog("WeddingMap::DestroyAll: {}", ch->GetName());

			if (ch->GetDesc())
				DESC_MANAGER::GetInstance()->DestroyDesc(ch->GetDesc());
			else
				M2_DESTROY_CHARACTER(ch);
		}
	};

	void WeddingMap::DestroyAll()
	{
		PyLog("WeddingMap::DestroyAll: m_set_pChr size %zu", m_set_pChr.size());
		
		FDestroyEveryone f;

		for (charset_t::iterator it = m_set_pChr.begin(); it != m_set_pChr.end(); it = m_set_pChr.begin())
			f(*it);
	}

	void WeddingMap::IncMember(LPCHARACTER ch)
	{
		if (IsMember(ch))
			return;

		m_set_pChr.insert(ch);

		SendLocalEvent(ch);

		if (ch->GetLevel() < 10)
		{
			ch->SetObserverMode(true);
		}
	}

	void WeddingMap::DecMember(LPCHARACTER ch)
	{
		if (IsMember(ch) == false)
			return;

		//PyLog("WeddingMap: DecMember {}", ch->GetName());
		m_set_pChr.erase(ch);

		if (ch->GetLevel() < 10)
		{
			ch->SetObserverMode(false);
		}
	}

	bool WeddingMap::IsMember(LPCHARACTER ch)
	{
		if (m_set_pChr.size() <= 0)
			return false;

		return m_set_pChr.find(ch) != m_set_pChr.end();
	}

	void WeddingMap::ShoutInMap(uint8_t type, const char* msg)
	{
		for (auto it = m_set_pChr.begin(); it != m_set_pChr.end(); ++it)
		{
			LPCHARACTER ch = *it;
			ch->ChatPacket(CHAT_TYPE_COMMAND, msg);
		}
	}

	void WeddingMap::SetMusic(bool bSet, const char* musicFileName)
	{
		if (m_isMusic != bSet)
		{
			m_isMusic = bSet;
			m_stMusicFileName = musicFileName;

			char szCommand[256];
			if (m_isMusic)
			{
				ShoutInMap(CHAT_TYPE_COMMAND, __BuildCommandPlayMusic(szCommand, sizeof(szCommand), 1, m_stMusicFileName.c_str()));
			}
			else
			{
				ShoutInMap(CHAT_TYPE_COMMAND, __BuildCommandPlayMusic(szCommand, sizeof(szCommand), 0, "default"));
			}
		} 
	}

	void WeddingMap::SetDark(bool bSet)
	{
		if (m_isDark != bSet)
		{
			m_isDark = bSet;

			if (m_isDark)
				ShoutInMap(CHAT_TYPE_COMMAND, "DayMode dark");
			else
				ShoutInMap(CHAT_TYPE_COMMAND, "DayMode light");
		}
	}

	void WeddingMap::SetSnow(bool bSet)
	{
		if (m_isSnow != bSet)
		{
			m_isSnow = bSet;

			if (m_isSnow)
				ShoutInMap(CHAT_TYPE_COMMAND, "xmas_snow 1");
			else
				ShoutInMap(CHAT_TYPE_COMMAND, "xmas_snow 0");
		}
	}

	bool WeddingMap::IsPlayingMusic()
	{
		return m_isMusic;
	}

	void WeddingMap::SendLocalEvent(LPCHARACTER ch)
	{
		char szCommand[256];

		if (m_isDark)
			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
		if (m_isSnow)
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_snow 1");
		if (m_isMusic)
			ch->ChatPacket(CHAT_TYPE_COMMAND, __BuildCommandPlayMusic(szCommand, sizeof(szCommand), 1, m_stMusicFileName.c_str()));	
	}

	const char* WeddingMap::__BuildCommandPlayMusic(char* szCommand, size_t nCmdLen, uint8_t bSet, const char* c_szMusicFileName)
	{
		if (nCmdLen < 1)
		{
			szCommand[0] = '\0';
			return "PlayMusic 0 CommandLengthError";
		}

		snprintf(szCommand, nCmdLen, "PlayMusic %d %s", bSet, c_szMusicFileName);
		return szCommand;
	}
	// Manager

	WeddingManager::WeddingManager()
	{
	}

	WeddingManager::~WeddingManager()
	{
	}

	bool WeddingManager::IsWeddingMap(uint32_t dwMapIndex)
	{
		return (dwMapIndex == WEDDING_MAP_INDEX || dwMapIndex / 10000 == WEDDING_MAP_INDEX);
	}

	WeddingMap* WeddingManager::Find(uint32_t dwMapIndex)
	{
		auto it = m_mapWedding.find(dwMapIndex);

		if (it == m_mapWedding.end())
			return NULL;

		return it->second;
	}

	uint32_t WeddingManager::__CreateWeddingMap(uint32_t dwPID1, uint32_t dwPID2)
	{
		auto rkSecTreeMgr = SECTREE_MANAGER::GetInstance();

		uint32_t dwMapIndex = rkSecTreeMgr->CreatePrivateMap(WEDDING_MAP_INDEX);

		if (!dwMapIndex)
		{
			SysLog("CreateWeddingMap(pid1={}, pid2={}) / CreatePrivateMap({}) FAILED", dwPID1, dwPID2, WEDDING_MAP_INDEX);
			return 0;
		}

		m_mapWedding.insert(std::make_pair(dwMapIndex, M2_NEW WeddingMap(dwMapIndex, dwPID1, dwPID2)));


		// LOCALE_SERVICE
		LPSECTREE_MAP pSectreeMap = rkSecTreeMgr->GetMap(dwMapIndex);
		if (pSectreeMap == nullptr) {
			return 0;
		}
		std::string st_weddingMapRegenFileName;
		st_weddingMapRegenFileName.reserve(64);
		st_weddingMapRegenFileName  = LocaleService_GetMapPath();
		st_weddingMapRegenFileName += "/metin2_map_wedding_01/npc.txt";

		if (!regen_do(st_weddingMapRegenFileName.c_str(), dwMapIndex, pSectreeMap->m_setting.iBaseX, pSectreeMap->m_setting.iBaseY, NULL, true))
		{
			SysLog("CreateWeddingMap(pid1={}, pid2={}) / regen_do(fileName={}, mapIndex={}, basePos=({}, {})) FAILED", dwPID1, dwPID2, st_weddingMapRegenFileName.c_str(), dwMapIndex, pSectreeMap->m_setting.iBaseX, pSectreeMap->m_setting.iBaseY);
		}
		else
		{
			PyLog("CreateWeddingMap(pid1={}, pid2={}) / regen_do(fileName={}, mapIndex={}, basePos=({}, {})) ok", dwPID1, dwPID2, st_weddingMapRegenFileName.c_str(), dwMapIndex, pSectreeMap->m_setting.iBaseX, pSectreeMap->m_setting.iBaseY);
		}
		// END_OF_LOCALE_SERVICE

		return dwMapIndex;
	}
	
	void WeddingManager::DestroyWeddingMap(WeddingMap* pMap)
	{
		PyLog("DestroyWeddingMap(index={})", pMap->GetMapIndex());
		pMap->DestroyAll();
		m_mapWedding.erase(pMap->GetMapIndex());
		SECTREE_MANAGER::GetInstance()->DestroyPrivateMap(pMap->GetMapIndex());
		M2_DELETE(pMap);
	}

	bool WeddingManager::End(uint32_t dwMapIndex)
	{
		auto it = m_mapWedding.find(dwMapIndex);

		if (it == m_mapWedding.end())
			return false;

		it->second->SetEnded();
		return true;
	}

	void WeddingManager::Request(uint32_t dwPID1, uint32_t dwPID2)
	{
		if (map_allow_find(WEDDING_MAP_INDEX))
		{
			uint32_t dwMapIndex = __CreateWeddingMap(dwPID1, dwPID2);

			if (!dwMapIndex)
			{
				SysLog("cannot create wedding map for {}, {}", dwPID1, dwPID2);
				return;
			}

			TPacketWeddingReady p;
			p.dwPID1 = dwPID1;
			p.dwPID2 = dwPID2;
			p.dwMapIndex = dwMapIndex;

			db_clientdesc->DBPacket(HEADER_GD_WEDDING_READY, 0, &p, sizeof(p));
		}
	}

}
