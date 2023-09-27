#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "sectree_manager.h"
#include "battle.h"
#include "affect.h"
#include "shop_manager.h"

int32_t	OnClickShop(TRIGGERPARAM);
int32_t	OnClickTalk(TRIGGERPARAM);

int32_t	OnIdleDefault(TRIGGERPARAM);
int32_t	OnAttackDefault(TRIGGERPARAM);

typedef struct STriggerFunction
{
	int32_t (*func) (TRIGGERPARAM);
} TTriggerFunction;

TTriggerFunction OnClickTriggers[ON_CLICK_MAX_NUM] =
{
	{ NULL,          	},	// ON_CLICK_NONE,
	{ OnClickShop,	},	// ON_CLICK_SHOP,
};

void CHARACTER::AssignTriggers(const TMobTable * table)
{
	if (table->bOnClickType >= ON_CLICK_MAX_NUM)
	{
		SysLog("{} has invalid OnClick value {}", GetName(), table->bOnClickType);
		abort();
	}

	m_triggerOnClick.bType = table->bOnClickType;
	m_triggerOnClick.pFunc = OnClickTriggers[table->bOnClickType].func;
}

/*
 * ON_CLICK
 */
int32_t OnClickShop(TRIGGERPARAM)
{
	CShopManager::GetInstance()->StartShopping(causer, ch);
	return 1;
}

int32_t OnIdleDefault(TRIGGERPARAM)
{
	if (ch->OnIdle())
		return PASSES_PER_SEC(1);

	return PASSES_PER_SEC(1);
}

class FuncFindMobVictim
{
	public:
		FuncFindMobVictim(LPCHARACTER pChr, int32_t iMaxDistance) :
			m_pChr(pChr),
			m_iMinDistance(~(1L << 31)),
			m_iMaxDistance(iMaxDistance),
			m_lx(pChr->GetX()),
			m_ly(pChr->GetY()),
			m_pChrVictim(nullptr),
			m_pChrBuilding(nullptr)
	{
	};

		bool operator () (LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_CHARACTER))
				return false;

			LPCHARACTER pChr = (LPCHARACTER) ent;

			if (pChr->IsBuilding() && 
				(pChr->IsAffectFlag(AFF_BUILDING_CONSTRUCTION_SMALL) ||
				 pChr->IsAffectFlag(AFF_BUILDING_CONSTRUCTION_LARGE) ||
				 pChr->IsAffectFlag(AFF_BUILDING_UPGRADE)))
			{
				m_pChrBuilding = pChr;
			}

			if (pChr->IsNPC())
			{
				if (!pChr->IsMonster() || !m_pChr->IsAttackMob() || m_pChr->IsAggressive())
					return false;
					
			}

			if (pChr->IsDead())
				return false;

			if (pChr->IsAffectFlag(AFF_EUNHYUNG) || 
					pChr->IsAffectFlag(AFF_INVISIBILITY) ||
					pChr->IsAffectFlag(AFF_REVIVE_INVISIBLE))
				return false;

			if (pChr->IsAffectFlag(AFF_TERROR) && m_pChr->IsImmune(ITEM::IMMUNE_TERROR) == false)
			{
				if (pChr->GetLevel() >= m_pChr->GetLevel())
					return false;
			}

		 	if (m_pChr->IsNoAttackShinsu())
			{
				if (pChr->GetEmpire() == 1)
					return false;
			}

			if (m_pChr->IsNoAttackChunjo())
			{
				if (pChr->GetEmpire() == 2)
					return false;
			}
			

			if (m_pChr->IsNoAttackJinno())
			{
				if (pChr->GetEmpire() == 3)
					return false;
			}

			int32_t iDistance = DISTANCE_APPROX(m_lx - pChr->GetX(), m_ly - pChr->GetY());

			if (iDistance < m_iMinDistance && iDistance <= m_iMaxDistance)
			{
				m_pChrVictim = pChr;
				m_iMinDistance = iDistance;
			}
			return true;
		}

		LPCHARACTER GetVictim()
		{
			if (m_pChrBuilding && m_pChr->GetHP() * 2 > m_pChr->GetMaxHP() || !m_pChrVictim)
			{
				return m_pChrBuilding;
			}

			return (m_pChrVictim);
		}

	private:
		LPCHARACTER	m_pChr;

		int32_t		m_iMinDistance;
		int32_t		m_iMaxDistance;
		int32_t		m_lx;
		int32_t		m_ly;

		LPCHARACTER	m_pChrVictim;
		LPCHARACTER	m_pChrBuilding;
};

LPCHARACTER FindVictim(LPCHARACTER pChr, int32_t iMaxDistance)
{
	FuncFindMobVictim f(pChr, iMaxDistance);
	if (pChr->GetSectree() != nullptr) {
		pChr->GetSectree()->ForEachAround(f);	
	}
	return f.GetVictim();
}

