#include "stdafx.h"
#include "pythoncharactermanager.h"
#include "PythonBackground.h"
#include "PythonNonPlayer.h"
#include "PythonPlayer.h"
#include <EterLib/Camera.h>

// Frame Process
int32_t CHAR_STAGE_VIEW_BOUND = 200 * 100;

void CPythonCharacterManager::AdjustCollisionWithOtherObjects(CActorInstance* pInst)
{
	if (!pInst->IsPC())
		return;

	CPythonCharacterManager* rkChrMgr = CPythonCharacterManager::GetInstance();
	for (CPythonCharacterManager::CharacterIterator i = rkChrMgr->CharacterInstanceBegin(); i != rkChrMgr->CharacterInstanceEnd(); ++i)
	{
		CInstanceBase* pkInstEach = *i;
		CActorInstance* rkActorEach = pkInstEach->GetGraphicThingInstancePtr();

		if (rkActorEach == pInst)
			continue;

		if (rkActorEach->IsPC() || rkActorEach->IsNPC() || rkActorEach->IsEnemy())
			continue;

		if (pInst->TestPhysicsBlendingCollision(*rkActorEach))
		{
			TPixelPosition curPos;
			pInst->GetPixelPosition(&curPos);
			pInst->SetBlendingPosition(curPos);
			break;
		}
	}
}

void CPythonCharacterManager::InsertPVPKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	CInstanceBase::InsertPVPKey(dwVIDSrc, dwVIDDst);

	CInstanceBase* pkInstSrc = GetInstancePtr(dwVIDSrc);
	if (pkInstSrc)
		pkInstSrc->RefreshTextTail();

	CInstanceBase* pkInstDst = GetInstancePtr(dwVIDDst);
	if (pkInstDst)
		pkInstDst->RefreshTextTail();
}

void CPythonCharacterManager::RemovePVPKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	CInstanceBase::RemovePVPKey(dwVIDSrc, dwVIDDst);

	CInstanceBase* pkInstSrc = GetInstancePtr(dwVIDSrc);
	if (pkInstSrc)
		pkInstSrc->RefreshTextTail();

	CInstanceBase* pkInstDst = GetInstancePtr(dwVIDDst);
	if (pkInstDst)
		pkInstDst->RefreshTextTail();
}

void CPythonCharacterManager::ChangeGVG(uint32_t dwSrcGuildID, uint32_t dwDstGuildID)
{
	for (auto& itor : m_kAliveInstMap)
	{
		CInstanceBase* pInstance = itor.second;

		uint32_t dwInstanceGuildID = pInstance->GetGuildID();
		if (dwSrcGuildID == dwInstanceGuildID || dwDstGuildID == dwInstanceGuildID)
			pInstance->RefreshTextTail();
	}
}

void CPythonCharacterManager::ClearMainInstance()
{
	m_pkInstMain = nullptr;
}

bool CPythonCharacterManager::SetMainInstance(uint32_t dwVID)
{
	m_pkInstMain = GetInstancePtr(dwVID);

	if (!m_pkInstMain)
		return false;

	return true;
}

CInstanceBase* CPythonCharacterManager::GetMainInstancePtr()
{
	return m_pkInstMain;
}

void CPythonCharacterManager::Update()
{
	CInstanceBase::ResetPerformanceCounter();
	CInstanceBase* pkInstMain = GetMainInstancePtr();

	auto i = m_kAliveInstMap.begin();
	while (m_kAliveInstMap.end() != i)
	{
		auto c = i++;

		CInstanceBase* pkInstEach = c->second;
		pkInstEach->Update();

		if (pkInstMain)
		{
			int32_t nDistance = int32_t(pkInstEach->NEW_GetDistanceFromDestInstance(*pkInstMain));
			if (nDistance > CHAR_STAGE_VIEW_BOUND + 10)
			{
				__DeleteBlendOutInstance(pkInstEach);
				m_kAliveInstMap.erase(c);
			}
		}
	}
	UpdateTransform();
	UpdateDeleting();
	__NEW_Pick();
}

void CPythonCharacterManager::ShowPointEffect(uint32_t ePoint, uint32_t dwVID)
{
	CInstanceBase* pkInstSel = (dwVID == 0xffffffff) ? GetMainInstancePtr() : GetInstancePtr(dwVID);

	if (!pkInstSel)
		return;

	switch (ePoint)
	{
	case POINT_LEVEL:
		pkInstSel->LevelUp();
		break;
	case POINT_LEVEL_STEP:
		pkInstSel->SkillUp();
		break;
	}
}

bool CPythonCharacterManager::RegisterPointEffect(uint32_t ePoint, const char* c_szFileName)
{
	if (ePoint >= POINT_MAX_NUM)
		return false;

	CEffectManager* rkEftMgr = CEffectManager::GetInstance();
	rkEftMgr->RegisterEffect(c_szFileName, &m_adwPointEffect[ePoint]);

	return true;
}

void CPythonCharacterManager::UpdateTransform()
{
	if (m_pkInstMain)
	{
		CPythonBackground* rkBG = CPythonBackground::GetInstance();
		for (auto& i : m_kAliveInstMap)
		{
			CInstanceBase* pSrcInstance = i.second;

			pSrcInstance->CheckAdvancing();
			if (pSrcInstance->IsPushing())
				rkBG->CheckAdvancing(pSrcInstance);
		}

		rkBG->CheckAdvancing(m_pkInstMain);
	}
	for (auto& itor : m_kAliveInstMap)
	{
		CInstanceBase* pInstance = itor.second;
		pInstance->Transform();
	}
}

void CPythonCharacterManager::UpdateDeleting()
{
	auto itor = m_kDeadInstList.begin();
	for (; itor != m_kDeadInstList.end();)
	{
		CInstanceBase* pInstance = *itor;

		if (pInstance->UpdateDeleting())
		{
			++itor;
		}
		else
		{
			CInstanceBase::Delete(pInstance);
			itor = m_kDeadInstList.erase(itor);
		}
	}
}

void CPythonCharacterManager::Deform()
{
	std::for_each(m_kAliveInstMap.begin(), m_kAliveInstMap.end(), [](const std::pair<uint32_t, CInstanceBase*>& cr_Pair)
		{
			cr_Pair.second->Deform();
		});

	std::for_each(m_kDeadInstList.begin(), m_kDeadInstList.end(), [](CInstanceBase* pInstance)
		{
			pInstance->Deform();
		});
}

bool CPythonCharacterManager::OLD_GetPickedInstanceVID(uint32_t* pdwPickedActorID)
{
	if (!m_pkInstPick)
		return false;

	*pdwPickedActorID = m_pkInstPick->GetVirtualID();
	return true;
}

CInstanceBase* CPythonCharacterManager::OLD_GetPickedInstancePtr()
{
	return m_pkInstPick;
}

D3DXVECTOR2& CPythonCharacterManager::OLD_GetPickedInstPosReference()
{
	return m_v2PickedInstProjPos;
}

bool CPythonCharacterManager::IsRegisteredVID(uint32_t dwVID)
{
	if (m_kAliveInstMap.end() == m_kAliveInstMap.find(dwVID))
		return false;

	return true;
}

bool CPythonCharacterManager::IsAliveVID(uint32_t dwVID)
{
	return m_kAliveInstMap.find(dwVID) != m_kAliveInstMap.end();
}

bool CPythonCharacterManager::IsDeadVID(uint32_t dwVID)
{
	for (auto& f : m_kDeadInstList)
	{
		if (f->GetVirtualID() == dwVID)
			return true;
	}

	return false;
}

void CPythonCharacterManager::__RenderSortedAliveActorList()
{
	static std::vector<CInstanceBase*> s_kVct_pkInstAliveSort;
	s_kVct_pkInstAliveSort.clear();
	s_kVct_pkInstAliveSort.reserve(100);

	TCharacterInstanceMap& rkMap_pkInstAlive = m_kAliveInstMap;
	for (auto& i : rkMap_pkInstAlive)
		s_kVct_pkInstAliveSort.emplace_back(i.second);

	std::sort(s_kVct_pkInstAliveSort.begin(), s_kVct_pkInstAliveSort.end(), [](CInstanceBase* pkLeft, CInstanceBase* pkRight)
		{
			return pkLeft->LessRenderOrder(pkRight);
		});

	std::for_each(s_kVct_pkInstAliveSort.begin(), s_kVct_pkInstAliveSort.end(), [](CInstanceBase* pInstance)
		{
			pInstance->Render();
			pInstance->RenderTrace();
		});
}

void CPythonCharacterManager::__RenderSortedDeadActorList()
{
	static std::vector<CInstanceBase*> s_kVct_pkInstDeadSort;
	s_kVct_pkInstDeadSort.clear();
	s_kVct_pkInstDeadSort.reserve(50);

	TCharacterInstanceList& rkLst_pkInstDead = m_kDeadInstList;
	for (auto& i : rkLst_pkInstDead)
		s_kVct_pkInstDeadSort.emplace_back(i);

	std::sort(s_kVct_pkInstDeadSort.begin(), s_kVct_pkInstDeadSort.end(), [](CInstanceBase* pkLeft, CInstanceBase* pkRight)
		{
			return pkLeft->LessRenderOrder(pkRight);
		});

	std::for_each(s_kVct_pkInstDeadSort.begin(), s_kVct_pkInstDeadSort.end(), [](CInstanceBase* pInstance)
		{
			pInstance->Render();
		});
}

void CPythonCharacterManager::Render()
{
	STATEMANAGER->SetTexture(0, nullptr);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTexture(1, nullptr);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);


	__RenderSortedAliveActorList();
	__RenderSortedDeadActorList();

	CInstanceBase* pkPickedInst = OLD_GetPickedInstancePtr();
	if (pkPickedInst)
	{
		const D3DXVECTOR3& c_rv3Position = pkPickedInst->GetGraphicThingInstanceRef().GetPosition();
		CPythonGraphic::GetInstance()->ProjectPosition(c_rv3Position.x, c_rv3Position.y, c_rv3Position.z, &m_v2PickedInstProjPos.x, &m_v2PickedInstProjPos.y);
	}
}

void CPythonCharacterManager::RenderShadowMainInstance()
{
	CInstanceBase* pkInstMain = GetMainInstancePtr();
	if (pkInstMain)
		pkInstMain->RenderToShadowMap();
}

void CPythonCharacterManager::RenderShadowAllInstances()
{
	std::for_each(m_kAliveInstMap.begin(), m_kAliveInstMap.end(), [](const std::pair<uint32_t, CInstanceBase*>& cr_Pair)
		{
			cr_Pair.second->RenderToShadowMap();
		});
}

void CPythonCharacterManager::RenderCollision()
{
	std::for_each(m_kAliveInstMap.begin(), m_kAliveInstMap.end(), [](const std::pair<uint32_t, CInstanceBase*>& cr_Pair)
		{
			cr_Pair.second->RenderCollision();
		});
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Managing Process

CInstanceBase* CPythonCharacterManager::CreateInstance(const CInstanceBase::SCreateData& c_rkCreateData)
{
	CInstanceBase* pCharacterInstance = RegisterInstance(c_rkCreateData.m_dwVID);
	if (!pCharacterInstance)
	{
		SysLog("CPythonCharacterManager::CreateInstance: VID[{0}] - ALREADY EXIST", c_rkCreateData.m_dwVID);
		return nullptr;
	}

	if (!pCharacterInstance->Create(c_rkCreateData))
	{
		SysLog("CPythonCharacterManager::CreateInstance VID[{0}] Race[{1}]", c_rkCreateData.m_dwVID, c_rkCreateData.m_dwRace);
		DeleteInstance(c_rkCreateData.m_dwVID);
		return nullptr;
	}

	if (c_rkCreateData.m_isMain)
		SelectInstance(c_rkCreateData.m_dwVID);

	return (pCharacterInstance);
}

CInstanceBase* CPythonCharacterManager::RegisterInstance(uint32_t VirtualID)
{
	const auto itor = m_kAliveInstMap.find(VirtualID);
	if (m_kAliveInstMap.end() != itor)
		return NULL;

	CInstanceBase* pCharacterInstance = CInstanceBase::New();
	m_kAliveInstMap.emplace(VirtualID, pCharacterInstance);
	return pCharacterInstance;
}

void CPythonCharacterManager::DeleteInstance(uint32_t dwDelVID)
{
	auto itor = m_kAliveInstMap.find(dwDelVID);

	if (m_kAliveInstMap.end() == itor)
	{
		ConsoleLog("DeleteCharacterInstance: no vid by {0}\n", dwDelVID);
		return;
	}

	CInstanceBase* pkInstDel = itor->second;

	if (pkInstDel == m_pkInstBind)
		m_pkInstBind = nullptr;

	if (pkInstDel == m_pkInstMain)
		m_pkInstMain = nullptr;

	if (pkInstDel == m_pkInstPick)
		m_pkInstPick = nullptr;

	CInstanceBase::Delete(pkInstDel);

	m_kAliveInstMap.erase(itor);
}

void CPythonCharacterManager::__DeleteBlendOutInstance(CInstanceBase* pkInstDel)
{
	pkInstDel->DeleteBlendOut();
	m_kDeadInstList.emplace_back(pkInstDel);

	auto rkPlayer = CPythonPlayer::GetInstance();
	rkPlayer->NotifyCharacterDead(pkInstDel->GetVirtualID());
}

void CPythonCharacterManager::DeleteInstanceByFade(uint32_t dwVID)
{
	auto f = m_kAliveInstMap.find(dwVID);
	if (m_kAliveInstMap.end() == f)
		return;

	__DeleteBlendOutInstance(f->second);
	m_kAliveInstMap.erase(f);
}

void CPythonCharacterManager::SelectInstance(uint32_t VirtualID)
{
	auto itor = m_kAliveInstMap.find(VirtualID);

	if (m_kAliveInstMap.end() == itor)
	{
		ConsoleLog("SelectCharacterInstance: no vid by {0}\n", VirtualID);
		return;
	}

	m_pkInstBind = itor->second;
}

CInstanceBase* CPythonCharacterManager::GetInstancePtr(uint32_t VirtualID)
{
	auto itor = m_kAliveInstMap.find(VirtualID);

	if (m_kAliveInstMap.end() == itor)
		return nullptr;

	return itor->second;
}

CInstanceBase* CPythonCharacterManager::GetInstancePtrByName(const char* name)
{
	for (auto& itor : m_kAliveInstMap)
	{
		CInstanceBase* pInstance = itor.second;

		if (!strcmp(pInstance->GetNameString(), name))
			return pInstance;
	}

	return nullptr;
}

CInstanceBase* CPythonCharacterManager::GetSelectedInstancePtr()
{
	return m_pkInstBind;
}

CInstanceBase* CPythonCharacterManager::FindClickableInstancePtr()
{
	return nullptr;
}

void CPythonCharacterManager::__UpdateSortPickedActorList()
{
	__UpdatePickedActorList();
	__SortPickedActorList();
}

void CPythonCharacterManager::__UpdatePickedActorList()
{
	m_kVct_pkInstPicked.clear();

	for (auto& i : m_kAliveInstMap)
	{
		CInstanceBase* pkInstEach = i.second;
		if (pkInstEach->CanPickInstance())
		{
			if (pkInstEach->IsDead())
			{
				if (pkInstEach->IntersectBoundingBox())
					m_kVct_pkInstPicked.emplace_back(pkInstEach);
			}
			else
			{
				if (pkInstEach->IntersectDefendingSphere())
					m_kVct_pkInstPicked.emplace_back(pkInstEach);
			}
		}
	}
}

struct CInstanceBase_SLessCameraDistance
{
	TPixelPosition m_kPPosEye;

	bool operator() (CInstanceBase* pkInstLeft, CInstanceBase* pkInstRight)
	{
		int32_t nLeftDeadPoint = pkInstLeft->IsDead();
		int32_t nRightDeadPoint = pkInstRight->IsDead();

		if (nLeftDeadPoint < nRightDeadPoint)
			return true;

		if (pkInstLeft->CalculateDistanceSq3d(m_kPPosEye) < pkInstRight->CalculateDistanceSq3d(m_kPPosEye))
			return true;

		return false;
	}
};

void CPythonCharacterManager::__SortPickedActorList()
{
	CCamera* pCamera = CCameraManager::GetInstance()->GetCurrentCamera();
	const D3DXVECTOR3& c_rv3EyePos = pCamera->GetEye();

	CInstanceBase_SLessCameraDistance kLess;
	kLess.m_kPPosEye = TPixelPosition(+c_rv3EyePos.x, -c_rv3EyePos.y, +c_rv3EyePos.z);

	std::sort(m_kVct_pkInstPicked.begin(), m_kVct_pkInstPicked.end(), kLess);
}

void CPythonCharacterManager::__NEW_Pick()
{
	__UpdateSortPickedActorList();

	CInstanceBase* pkInstMain = GetMainInstancePtr();

	// Precise check
	for (auto pkInstEach : m_kVct_pkInstPicked)
	{
		if (pkInstEach != pkInstMain && pkInstEach->IntersectBoundingBox())
		{
			if (m_pkInstPick)
			{
				if (m_pkInstPick != pkInstEach)
					m_pkInstPick->OnUnselected();
			}

			if (pkInstEach->CanPickInstance())
			{
				m_pkInstPick = pkInstEach;
				m_pkInstPick->OnSelected();
				return;
			}
		}
	}

	// If you can't find it, just
	for (auto pkInstEach : m_kVct_pkInstPicked)
	{
		if (pkInstEach != pkInstMain)
		{
			if (m_pkInstPick)
			{
				if (m_pkInstPick != pkInstEach)
					m_pkInstPick->OnUnselected();
			}

			if (pkInstEach->CanPickInstance())
			{
				m_pkInstPick = pkInstEach;
				m_pkInstPick->OnSelected();
				return;
			}
		}
	}

	if (pkInstMain)
	{
		if (pkInstMain->CanPickInstance())
		{
			if (m_kVct_pkInstPicked.end() != std::find(m_kVct_pkInstPicked.begin(), m_kVct_pkInstPicked.end(), pkInstMain))
			{
				if (m_pkInstPick)
					if (m_pkInstPick != pkInstMain)
						m_pkInstPick->OnUnselected();

				m_pkInstPick = pkInstMain;
				m_pkInstPick->OnSelected();
				return;
			}
		}
	}

	if (m_pkInstPick)
	{
		m_pkInstPick->OnUnselected();
		m_pkInstPick = nullptr;
	}
}

int32_t CPythonCharacterManager::PickAll()
{
	for (auto& itor : m_kAliveInstMap)
	{
		CInstanceBase* pInstance = itor.second;

		if (pInstance->IntersectDefendingSphere())
			return pInstance->GetVirtualID();
	}

	return -1;
}

void CPythonCharacterManager::SCRIPT_SetAffect(uint32_t dwVID, uint32_t eState, BOOL isVisible)
{
	CInstanceBase* pkInstSel = (dwVID == 0xffffffff) ? GetSelectedInstancePtr() : GetInstancePtr(dwVID);
	if (!pkInstSel)
		return;

	pkInstSel->SCRIPT_SetAffect(eState, isVisible ? true : false);
}

void CPythonCharacterManager::SetEmoticon(uint32_t dwVID, uint32_t eState)
{
	CInstanceBase* pkInstSel = (dwVID == 0xffffffff) ? GetSelectedInstancePtr() : GetInstancePtr(dwVID);
	if (!pkInstSel)
		return;

	pkInstSel->SetEmoticon(eState);
}

bool CPythonCharacterManager::IsPossibleEmoticon(uint32_t dwVID)
{
	CInstanceBase* pkInstSel = (dwVID == 0xffffffff) ? GetSelectedInstancePtr() : GetInstancePtr(dwVID);
	if (!pkInstSel)
		return false;

	return pkInstSel->IsPossibleEmoticon();
}

CInstanceBase* CPythonCharacterManager::GetTabNextTargetPointer(CInstanceBase* pkInstMain)
{
	if (!pkInstMain)
	{
		ResetTabNextTargetVectorIndex();
		return nullptr;
	}

	struct FCharacterManagerInstanceTarget
	{
		CInstanceBase* pkInstMain;
		FCharacterManagerInstanceTarget(CInstanceBase* pInstance) : pkInstMain(pInstance) {}

		inline void operator () (const std::pair<uint32_t, CInstanceBase*>& itor)
		{
			const auto pkInstTarget = itor.second;
			if (!pkInstTarget || pkInstTarget == pkInstMain || !pkInstTarget->IsEnemy())
				return;

			const auto fRadiusDistance = pkInstMain->GetDistance(pkInstTarget);
			if (fRadiusDistance < 1500.0f)
				m_vecTargetInstance.emplace_back(pkInstTarget);
		}

		std::vector<CInstanceBase*> m_vecTargetInstance;
	};

	FCharacterManagerInstanceTarget f(pkInstMain);
	f = std::for_each(m_kAliveInstMap.begin(), m_kAliveInstMap.end(), f);

	const auto kTargetCount = f.m_vecTargetInstance.size();
	if (kTargetCount == 0)
	{
		ResetTabNextTargetVectorIndex();
		return nullptr;
	}

	if (GetTabNextTargetVectorIndex() >= kTargetCount - 1)
		ResetTabNextTargetVectorIndex();

	return f.m_vecTargetInstance.at(++m_adwVectorIndexTabNextTarget);
}

void CPythonCharacterManager::RefreshAllPCTextTail()
{
	CharacterIterator itor = CharacterInstanceBegin();
	CharacterIterator itorEnd = CharacterInstanceEnd();
	for (; itor != itorEnd; ++itor)
	{
		CInstanceBase* pInstance = *itor;
		if (!pInstance->IsPC())
			continue;

		pInstance->RefreshTextTail();
	}
}

void CPythonCharacterManager::RefreshAllGuildMark()
{
	CharacterIterator itor = CharacterInstanceBegin();
	CharacterIterator itorEnd = CharacterInstanceEnd();
	for (; itor != itorEnd; ++itor)
	{
		CInstanceBase* pInstance = *itor;
		if (!pInstance->IsPC())
			continue;

		pInstance->ChangeGuild(pInstance->GetGuildID());
		pInstance->RefreshTextTail();
	}
}

void CPythonCharacterManager::RefreshGuildSymbols(std::set<uint32_t> guildIDSet)
{
	CPythonCharacterManager::CharacterIterator itor = CharacterInstanceBegin();
	CPythonCharacterManager::CharacterIterator itorEnd = CharacterInstanceEnd();
	for (; itor != itorEnd; ++itor)
	{
		CInstanceBase* pInstance = *itor;

		// Guild Symbol
		if (pInstance->GetRace() != 14200)
			continue;

		uint32_t gid = pInstance->GetGuildID();
		bool isInList = (guildIDSet.find(gid) != guildIDSet.end());

		// We found a symbol but it was not part of this update.
		if (!isInList)
			continue;

		std::string strFileName = GetGuildSymbolFileName(gid);
		CActorInstance* guildSymbolModel = pInstance->GetGraphicThingInstancePtr();
		if (!guildSymbolModel)
			continue;

		if (IsFile(strFileName.c_str()))
			guildSymbolModel->ChangeMaterial(strFileName.c_str());
		else
			guildSymbolModel->ChangeMaterial("d:/ymir work/guild/object/guild_symbol/guild_symbol01.dds"); // This makes it possible to delete symbols.
	}
}

void CPythonCharacterManager::DeleteAllInstances()
{
	DestroyAliveInstanceMap();
	DestroyDeadInstanceList();
}

void CPythonCharacterManager::DestroyAliveInstanceMap()
{
	for (auto& i : m_kAliveInstMap)
		CInstanceBase::Delete(i.second);

	m_kAliveInstMap.clear();
}

void CPythonCharacterManager::DestroyDeadInstanceList()
{
	for (auto& i : m_kDeadInstList)
		CInstanceBase::Delete(i);

	m_kDeadInstList.clear();
}

void CPythonCharacterManager::Destroy()
{
	DeleteAllInstances();
	CInstanceBase::DestroySystem();
	__Initialize();
}

void CPythonCharacterManager::__Initialize()
{
	std::memset(m_adwPointEffect, 0, sizeof(m_adwPointEffect));
	m_pkInstMain = nullptr;
	m_pkInstBind = nullptr;
	m_pkInstPick = nullptr;
	m_v2PickedInstProjPos = D3DXVECTOR2(0.0f, 0.0f);
}

CPythonCharacterManager::CPythonCharacterManager()
{
	__Initialize();
}

CPythonCharacterManager::~CPythonCharacterManager()
{
	Destroy();
}
