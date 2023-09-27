#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "PetSystem.h"
#include <Common/VnumHelper.h>
#include "packet.h"
#include "item_manager.h"
#include "item.h"


extern int32_t passes_per_sec;
EVENTINFO(petsystem_event_info)
{
	CPetSystem* pPetSystem;
};

EVENTFUNC(petsystem_update_event)
{
	petsystem_event_info* info = dynamic_cast<petsystem_event_info*>(event->info);
	if (info == nullptr)
	{
		SysLog("check_speedhack_event> <Factor> Null pointer");
		return 0;
	}

	CPetSystem*	pPetSystem = info->pPetSystem;

	if (!pPetSystem)
		return 0;

	
	pPetSystem->Update(0);
	return PASSES_PER_SEC(1) / 4;
}

const float PET_COUNT_LIMIT = 3;

///////////////////////////////////////////////////////////////////////////////////////
//  CPetActor
///////////////////////////////////////////////////////////////////////////////////////

CPetActor::CPetActor(LPCHARACTER owner, uint32_t vnum, uint32_t options)
{
	m_dwVnum = vnum;
	m_dwVID = 0;
	m_dwOptions = options;
	m_dwLastActionTime = 0;

	m_pChar = 0;
	m_pOwner = owner;

	m_originalMoveSpeed = 0;
	
	m_dwSummonItemVID = 0;
	m_dwSummonItemVnum = 0;
}

CPetActor::~CPetActor()
{
	this->Unsummon();

	m_pOwner = 0;
}

void CPetActor::SetName(const char* name)
{
	std::string petName = m_pOwner->GetName();

	if (0 != m_pOwner && 
		0 == name && 
		0 != m_pOwner->GetName())
	{
		petName += "'s Pet";
	}
	else
		petName += name;

	if (IsSummoned())
		m_pChar->SetName(petName);

	m_name = petName;
}

bool CPetActor::Mount()
{
	if (0 == m_pOwner)
		return false;

	if (HasOption(EPetOption_Mountable))
		m_pOwner->MountVnum(m_dwVnum);

	return m_pOwner->GetMountVnum() == m_dwVnum;;
}

void CPetActor::Unmount()
{
	if (0 == m_pOwner)
		return;

	if (m_pOwner->IsHorseRiding())
		m_pOwner->StopRiding();
}

void CPetActor::Unsummon()
{
	if (this->IsSummoned())
	{
		this->ClearBuff();
		this->SetSummonItem(nullptr);
		if (NULL != m_pOwner)
			m_pOwner->ComputePoints();

		if (NULL != m_pChar)
			M2_DESTROY_CHARACTER(m_pChar);

		m_pChar = 0;
		m_dwVID = 0;
	}
}

uint32_t CPetActor::Summon(const char* petName, LPITEM pSummonItem, bool bSpawnFar)
{
	int32_t x = m_pOwner->GetX();
	int32_t y = m_pOwner->GetY();
	int32_t z = m_pOwner->GetZ();

	if (bSpawnFar)
	{
		x += (number(0, 1) * 2 - 1) * number(2000, 2500);
		y += (number(0, 1) * 2 - 1) * number(2000, 2500);
	}
	else
	{
		x += number(-100, 100);
		y += number(-100, 100);
	}

	if (0 != m_pChar)
	{
		m_pChar->Show (m_pOwner->GetMapIndex(), x, y);
		m_dwVID = m_pChar->GetVID();

		return m_dwVID;
	}
	
	m_pChar = CHARACTER_MANAGER::GetInstance()->SpawnMob(
				m_dwVnum, 
				m_pOwner->GetMapIndex(), 
				x, y, z,
				false, (int32_t)(m_pOwner->GetRotation()+180), false);

	if (0 == m_pChar)
	{
		SysLog("[CPetSystem::Summon] Failed to summon the pet. (vnum: {})", m_dwVnum);
		return 0;
	}

	m_pChar->SetPet();

	m_pChar->SetEmpire(m_pOwner->GetEmpire());

	m_dwVID = m_pChar->GetVID();

	this->SetName(petName);

	this->SetSummonItem(pSummonItem);
	m_pOwner->ComputePoints();
	m_pChar->Show(m_pOwner->GetMapIndex(), x, y, z);

	return m_dwVID;
}

bool CPetActor::_UpdatAloneActionAI(float fMinDist, float fMaxDist)
{
	float fDist = number(fMinDist, fMaxDist);
	float r = (float)number (0, 359);
	float dest_x = GetOwner()->GetX() + fDist * cos(r);
	float dest_y = GetOwner()->GetY() + fDist * sin(r);

	m_pChar->SetNowWalking(true);

	if (!m_pChar->IsStateMove() && m_pChar->Goto(dest_x, dest_y))
		m_pChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	m_dwLastActionTime = get_dword_time();

	return true;
}

bool CPetActor::_UpdateFollowAI()
{
	if (0 == m_pChar->m_pMobData)
	{
		return false;
	}

	if (0 == m_originalMoveSpeed)
	{
		const CMob* mobData = CMobManager::GetInstance()->Get(m_dwVnum);

		if (0 != mobData)
			m_originalMoveSpeed = mobData->m_table.sMovingSpeed;
	}
	float	START_FOLLOW_DISTANCE = 300.0f;		
	float	START_RUN_DISTANCE = 900.0f;		

	float	RESPAWN_DISTANCE = 4500.f;			
	int32_t		APPROACH = 200;						

	bool bDoMoveAlone = true;					
	bool bRun = false;							

	uint32_t currentTime = get_dword_time();

	int32_t ownerX = m_pOwner->GetX();		int32_t ownerY = m_pOwner->GetY();
	int32_t charX = m_pChar->GetX();			int32_t charY = m_pChar->GetY();

	float fDist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (fDist >= RESPAWN_DISTANCE)
	{
		float fOwnerRot = m_pOwner->GetRotation() * 3.141592f / 180.f;
		float fx = -APPROACH * cos(fOwnerRot);
		float fy = -APPROACH * sin(fOwnerRot);
		if (m_pChar->Show(m_pOwner->GetMapIndex(), ownerX + fx, ownerY + fy))
		{
			return true;
		}
	}
	
	
	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if(fDist >= START_RUN_DISTANCE)
		{
			bRun = true;
		}

		m_pChar->SetNowWalking(!bRun);
		
		Follow(APPROACH);

		m_pChar->SetLastAttacked(currentTime);
		m_dwLastActionTime = currentTime;
	}

	else 
		m_pChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	return true;
}

bool CPetActor::Update(uint32_t deltaTime)
{
	bool bResult = true;

	if (m_pOwner->IsDead() || (IsSummoned() && m_pChar->IsDead()) 
		|| !ITEM_MANAGER::GetInstance()->FindByVID(this->GetSummonItemVID())
		|| ITEM_MANAGER::GetInstance()->FindByVID(this->GetSummonItemVID())->GetOwner() != this->GetOwner()
		)
	{
		this->Unsummon();
		return true;
	}

	if (this->IsSummoned() && HasOption(EPetOption_Followable))
		bResult = bResult && this->_UpdateFollowAI();

	return bResult;
}

bool CPetActor::Follow(float fMinDistance)
{
	if(!m_pOwner || !m_pChar) 
		return false;

	float fOwnerX = m_pOwner->GetX();
	float fOwnerY = m_pOwner->GetY();

	float fPetX = m_pChar->GetX();
	float fPetY = m_pChar->GetY();

	float fDist = DISTANCE_SQRT(fOwnerX - fPetX, fOwnerY - fPetY);
	if (fDist <= fMinDistance)
		return false;

	m_pChar->SetRotationToXY(fOwnerX, fOwnerY);

	float fx, fy;

	float fDistToGo = fDist - fMinDistance;
	GetDeltaByDegree(m_pChar->GetRotation(), fDistToGo, &fx, &fy);
	
	if (!m_pChar->Goto((int32_t)(fPetX+fx+0.5f), (int32_t)(fPetY+fy+0.5f)))
		return false;

	m_pChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);

	return true;
}

void CPetActor::SetSummonItem (LPITEM pItem)
{
	if (!pItem)
	{
		m_dwSummonItemVID = 0;
		m_dwSummonItemVnum = 0;
		return;
	}

	m_dwSummonItemVID = pItem->GetVID();
	m_dwSummonItemVnum = pItem->GetVnum();
}

void CPetActor::GiveBuff()
{
	if (34004 == m_dwVnum || 34009 == m_dwVnum)
	{
		if (!m_pOwner->GetDungeon())
		{
			return;
		}
	}
	LPITEM item = ITEM_MANAGER::GetInstance()->FindByVID(m_dwSummonItemVID);
	if (NULL != item)
		item->ModifyPoints(true);
	return ;
}

void CPetActor::ClearBuff()
{
	if (!m_pOwner)
		return ;
	TItemTable* item_proto = ITEM_MANAGER::GetInstance()->GetTable(m_dwSummonItemVnum);
	if (!item_proto)
		return;
	for (int32_t i = 0; i < ITEM::APPLY_MAX_NUM; i++)
	{
		if (item_proto->aApplies[i].bType == ITEM::APPLY_NONE)
			continue;
		m_pOwner->ApplyPoint(item_proto->aApplies[i].bType, -item_proto->aApplies[i].lValue);
	}

	return ;
}

///////////////////////////////////////////////////////////////////////////////////////
//  CPetSystem
///////////////////////////////////////////////////////////////////////////////////////

CPetSystem::CPetSystem(LPCHARACTER owner)
{
	m_pOwner = owner;
	m_dwUpdatePeriod = 400;

	m_dwLastUpdateTime = 0;
}

CPetSystem::~CPetSystem()
{
	Destroy();
}

void CPetSystem::Destroy()
{
	for (TPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			delete petActor;
		}
	}
	event_cancel(&m_pPetSystemUpdateEvent);
	m_petActorMap.clear();
}

bool CPetSystem::Update(uint32_t deltaTime)
{
	bool bResult = true;

	uint32_t currentTime = get_dword_time();
	
	if (m_dwUpdatePeriod > currentTime - m_dwLastUpdateTime)
		return true;
	
	std::vector <CPetActor*> v_garbageActor;

	for (TPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor && petActor->IsSummoned())
		{
			LPCHARACTER pPet = petActor->GetCharacter();
			
			if (!CHARACTER_MANAGER::GetInstance()->Find(pPet->GetVID()))
			{
				v_garbageActor.push_back(petActor);
			}
			else
			{
				bResult = bResult && petActor->Update(deltaTime);
			}
		}
	}
	for (std::vector<CPetActor*>::iterator it = v_garbageActor.begin(); it != v_garbageActor.end(); it++)
		DeletePet(*it);

	m_dwLastUpdateTime = currentTime;

	return bResult;
}

void CPetSystem::DeletePet(uint32_t mobVnum)
{
	TPetActorMap::iterator iter = m_petActorMap.find(mobVnum);

	if (m_petActorMap.end() == iter)
	{
		SysLog("[CPetSystem::DeletePet] Can't find pet on my list (VNUM: {})", mobVnum);
		return;
	}

	CPetActor* petActor = iter->second;

	if (0 == petActor)
	{
		SysLog("[CPetSystem::DeletePet] Null Pointer (petActor)");
	}
	else
		delete petActor;

	m_petActorMap.erase(iter);	
}

void CPetSystem::DeletePet(CPetActor* petActor)
{
	for (TPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		if (iter->second == petActor)
		{
			delete petActor;
			m_petActorMap.erase(iter);

			return;
		}
	}
}

void CPetSystem::Unsummon(uint32_t vnum, bool bDeleteFromList)
{
	CPetActor* actor = this->GetByVnum(vnum);

	if (0 == actor)
	{
		SysLog("[CPetSystem::GetByVnum({})] Null Pointer (petActor)", vnum);
		return;
	}
	actor->Unsummon();

	if (bDeleteFromList)
		this->DeletePet(actor);

	bool bActive = false;
	for (TPetActorMap::iterator it = m_petActorMap.begin(); it != m_petActorMap.end(); it++)
	{
		bActive |= it->second->IsSummoned();
	}
	if (!bActive)
	{
		event_cancel(&m_pPetSystemUpdateEvent);
		m_pPetSystemUpdateEvent = nullptr;
	}
}


CPetActor* CPetSystem::Summon(uint32_t mobVnum, LPITEM pSummonItem, const char* petName, bool bSpawnFar, uint32_t options)
{
	CPetActor* petActor = this->GetByVnum(mobVnum);

	if (0 == petActor)
	{
		petActor = M2_NEW CPetActor(m_pOwner, mobVnum, options);
		m_petActorMap.insert(std::make_pair(mobVnum, petActor));
	}

	uint32_t petVID = petActor->Summon(petName, pSummonItem, bSpawnFar);

	if (!m_pPetSystemUpdateEvent)
	{
		petsystem_event_info* info = AllocEventInfo<petsystem_event_info>();

		info->pPetSystem = this;

		m_pPetSystemUpdateEvent = event_create(petsystem_update_event, info, PASSES_PER_SEC(1) / 4);	
	}

	return petActor;
}


CPetActor* CPetSystem::GetByVID(uint32_t vid) const
{
	CPetActor* petActor = 0;

	bool bFound = false;

	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		petActor = iter->second;

		if (0 == petActor)
		{
			SysLog("[CPetSystem::GetByVID({})] Null Pointer (petActor)", vid);
			continue;
		}

		bFound = petActor->GetVID() == vid;

		if (bFound)
			break;
	}

	return bFound ? petActor : 0;
}

CPetActor* CPetSystem::GetByVnum(uint32_t vnum) const
{
	CPetActor* petActor = 0;

	TPetActorMap::const_iterator iter = m_petActorMap.find(vnum);

	if (m_petActorMap.end() != iter)
		petActor = iter->second;

	return petActor;
}

size_t CPetSystem::CountSummoned() const
{
	size_t count = 0;

	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
				++count;
		}
	}

	return count;
}

void CPetSystem::RefreshBuff()
{
	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
			{
				petActor->GiveBuff();
			}
		}
	}
}