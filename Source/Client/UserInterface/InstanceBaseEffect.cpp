#include "StdAfx.h"
#include "InstanceBase.h"
#include "PythonTextTail.h"
#include "PythonApplication.h"
#include "PythonPlayer.h"
#include "PythonSystem.h"

#include "../../Libraries/EffectLib/EffectManager.h"
#include "../../Libraries/EffectLib/ParticleSystemData.h"
#include "../../Libraries/eterLib/Camera.h"

float CInstanceBase::ms_fDustGap;
float CInstanceBase::ms_fHorseDustGap;
uint32_t CInstanceBase::ms_adwCRCAffectEffect[CInstanceBase::EFFECT_NUM];
std::string CInstanceBase::ms_astAffectEffectAttachBone[EFFECT_NUM];

#define BYTE_COLOR_TO_D3DX_COLOR(r, g, b) D3DXCOLOR(float(r)/255.0f, float(g)/255.0f, float(b)/255.0f, 1.0f)

D3DXCOLOR g_akD3DXClrTitle[CInstanceBase::TITLE_NUM];
D3DXCOLOR g_akD3DXClrName[CInstanceBase::NAMECOLOR_NUM];

std::map<int32_t, std::string> g_TitleNameMap;
std::set<uint32_t> g_kSet_dwPVPReadyKey;
std::set<uint32_t> g_kSet_dwPVPKey;
std::set<uint32_t> g_kSet_dwGVGKey;
std::set<uint32_t> g_kSet_dwDUELKey;

bool g_isEmpireNameMode=false;

void  CInstanceBase::SetEmpireNameMode(bool isEnable)
{
	g_isEmpireNameMode=isEnable;

	if (isEnable)
	{
		g_akD3DXClrName[NAMECOLOR_MOB]=g_akD3DXClrName[NAMECOLOR_EMPIRE_MOB];
		g_akD3DXClrName[NAMECOLOR_NPC]=g_akD3DXClrName[NAMECOLOR_EMPIRE_NPC];
		g_akD3DXClrName[NAMECOLOR_PC]=g_akD3DXClrName[NAMECOLOR_NORMAL_PC];

		for (UINT uEmpire=1; uEmpire<EMPIRE_NUM; ++uEmpire)
			g_akD3DXClrName[NAMECOLOR_PC+uEmpire]=g_akD3DXClrName[NAMECOLOR_EMPIRE_PC+uEmpire];
		
	}
	else
	{
		g_akD3DXClrName[NAMECOLOR_MOB]=g_akD3DXClrName[NAMECOLOR_NORMAL_MOB];
		g_akD3DXClrName[NAMECOLOR_NPC]=g_akD3DXClrName[NAMECOLOR_NORMAL_NPC];

		for (UINT uEmpire=0; uEmpire<EMPIRE_NUM; ++uEmpire)
			g_akD3DXClrName[NAMECOLOR_PC+uEmpire]=g_akD3DXClrName[NAMECOLOR_NORMAL_PC];
	}
}

const D3DXCOLOR& CInstanceBase::GetIndexedNameColor(UINT eNameColor)
{
	if (eNameColor>=NAMECOLOR_NUM)
	{
		static D3DXCOLOR s_kD3DXClrNameDefault(0xffffffff);
		return s_kD3DXClrNameDefault;
	}

	return g_akD3DXClrName[eNameColor];
}

void CInstanceBase::AddDamageEffect(uint32_t damage,uint8_t flag,BOOL bSelf,BOOL bTarget)
{
	if(ClientConfig::GetInstance()->IsShowDamage())
	{		
		SEffectDamage sDamage;
		sDamage.bSelf = bSelf;
		sDamage.bTarget = bTarget;
		sDamage.damage = damage;
		sDamage.flag = flag;
		m_DamageQueue.push_back(sDamage);
	}
}

void CInstanceBase::ProcessDamage()
{
	if(m_DamageQueue.empty())
		return;

	SEffectDamage sDamage = m_DamageQueue.front();

	m_DamageQueue.pop_front();

	uint32_t damage = sDamage.damage;
	uint8_t flag = sDamage.flag;
	BOOL bSelf = sDamage.bSelf;
	BOOL bTarget = sDamage.bTarget;

	CCamera * pCamera = CCameraManager::GetInstance()->GetCurrentCamera();	
	float cameraAngle = GetDegreeFromPosition2(pCamera->GetTarget().x,pCamera->GetTarget().y,pCamera->GetEye().x,pCamera->GetEye().y);

	uint32_t FONT_WIDTH = 30;
	
	auto rkEftMgr=CEffectManager::GetInstance();

	D3DXVECTOR3 v3Pos = m_GraphicThingInstance.GetPosition();
	v3Pos.z += float(GetBaseHeight() + m_GraphicThingInstance.GetHeight());

	D3DXVECTOR3 v3Rot = D3DXVECTOR3(0.0f, 0.0f, cameraAngle);

	if ( (flag & DAMAGE_DODGE) || (flag & DAMAGE_BLOCK) )
	{
		if(bSelf)
			rkEftMgr->CreateEffect(ms_adwCRCAffectEffect[EFFECT_DAMAGE_MISS],v3Pos,v3Rot);
		else
			rkEftMgr->CreateEffect(ms_adwCRCAffectEffect[EFFECT_DAMAGE_TARGETMISS],v3Pos,v3Rot);
		//__AttachEffect(EFFECT_DAMAGE_MISS);
		return;
	}
	else if (flag & DAMAGE_CRITICAL)
	{
		//rkEftMgr.CreateEffect(ms_adwCRCAffectEffect[EFFECT_DAMAGE_CRITICAL],v3Pos,v3Rot);
		//return; ���ڵ� ǥ��.
	}

	std::string strDamageType;
	uint32_t rdwCRCEft = 0;
	/*
	if ( (flag & DAMAGE_POISON) )
	{
		strDamageType = "poison_";
		rdwCRCEft = EFFECT_DAMAGE_POISON;
	}
	else
	*/
	{
		if(bSelf)
		{
			strDamageType = "damage_";
			if(m_bDamageEffectType==0)
				rdwCRCEft = EFFECT_DAMAGE_SELFDAMAGE;
			else
				rdwCRCEft = EFFECT_DAMAGE_SELFDAMAGE2;
			m_bDamageEffectType = !m_bDamageEffectType;
		}
		else if(bTarget == false)
		{
			strDamageType = "nontarget_";
			rdwCRCEft = EFFECT_DAMAGE_NOT_TARGET;
			return;//���� ���� �ȵ�.
		}
		else
		{
			strDamageType = "target_";
			rdwCRCEft = EFFECT_DAMAGE_TARGET;
		}
	}
	
	uint32_t index = 0;
	uint32_t num = 0;
	std::vector<std::string> textures;
	while(damage>0)
	{
		if(index > 7)
		{
			TraceLog("ProcessDamage���ѷ��� ���ɼ�");
			break;
		}
		num = damage%10;
		damage /= 10;
		char numBuf[MAX_PATH];
		sprintf(numBuf,"%d.dds",num);
		textures.push_back("d:/ymir work/effect/affect/damagevalue/"+strDamageType+numBuf);
		
		rkEftMgr->SetEffectTextures(ms_adwCRCAffectEffect[rdwCRCEft],textures);
		
		D3DXMATRIX matrix,matTrans;
		D3DXMatrixIdentity(&matrix);
		matrix._41 = v3Pos.x;
		matrix._42 = v3Pos.y;
		matrix._43 = v3Pos.z;
		D3DXMatrixTranslation(&matrix,v3Pos.x,v3Pos.y,v3Pos.z);
		D3DXMatrixMultiply(&matrix,&pCamera->GetInverseViewMatrix(),&matrix);
		D3DXMatrixTranslation(&matTrans,FONT_WIDTH*index,0,0);
		matTrans._41 = -matTrans._41;
		matrix = matTrans*matrix;
		D3DXMatrixMultiply(&matrix,&pCamera->GetViewMatrix(),&matrix);
		
		rkEftMgr->CreateEffect(ms_adwCRCAffectEffect[rdwCRCEft],D3DXVECTOR3(matrix._41,matrix._42,matrix._43)
			,v3Rot);	
		
		textures.clear();

		index++;
	}	
}

void CInstanceBase::AttachSpecialEffect(uint32_t effect)
{
	__AttachEffect(effect);
}

void CInstanceBase::LevelUp()
{
	__AttachEffect(EFFECT_LEVELUP);
}

void CInstanceBase::SkillUp()
{
	__AttachEffect(EFFECT_SKILLUP);
}

void CInstanceBase::CreateSpecialEffect(uint32_t iEffectIndex)
{
	const D3DXMATRIX & c_rmatGlobal = m_GraphicThingInstance.GetTransform();

	uint32_t dwEffectIndex = CEffectManager::GetInstance()->GetEmptyIndex();
	uint32_t dwEffectCRC = ms_adwCRCAffectEffect[iEffectIndex];
	CEffectManager::GetInstance()->CreateEffectInstance(dwEffectIndex, dwEffectCRC);
	CEffectManager::GetInstance()->SelectEffectInstance(dwEffectIndex);
	CEffectManager::GetInstance()->SetEffectInstanceGlobalMatrix(c_rmatGlobal);
}

void CInstanceBase::__EffectContainer_Destroy()
{
	SEffectContainer::Dict& rkDctEftID=__EffectContainer_GetDict();

	SEffectContainer::Dict::iterator i;
	for (i=rkDctEftID.begin(); i!=rkDctEftID.end(); ++i)
		__DetachEffect(i->second);

	rkDctEftID.clear();
}

void CInstanceBase::__EffectContainer_Initialize()
{
	SEffectContainer::Dict& rkDctEftID=__EffectContainer_GetDict();
	rkDctEftID.clear();	
}

CInstanceBase::SEffectContainer::Dict& CInstanceBase::__EffectContainer_GetDict()
{
	return m_kEffectContainer.m_kDct_dwEftID;
}

// Return value �� boolean ���� ID �� �ٲߴϴ�
uint32_t CInstanceBase::__EffectContainer_AttachEffect(uint32_t dwEftKey)
{
	SEffectContainer::Dict& rkDctEftID=__EffectContainer_GetDict();
	SEffectContainer::Dict::iterator f=rkDctEftID.find(dwEftKey);
	if (rkDctEftID.end()!=f)
		return 0;

	uint32_t dwEftID=__AttachEffect(dwEftKey);
	rkDctEftID.insert(SEffectContainer::Dict::value_type(dwEftKey, dwEftID));
	return dwEftID;
}


void CInstanceBase::__EffectContainer_DetachEffect(uint32_t dwEftKey)
{
	SEffectContainer::Dict& rkDctEftID=__EffectContainer_GetDict();
	SEffectContainer::Dict::iterator f=rkDctEftID.find(dwEftKey);
	if (rkDctEftID.end()==f)
		return;

	__DetachEffect(f->second);

	rkDctEftID.erase(f);
}

void CInstanceBase::__AttachEmpireEffect(uint32_t eEmpire)
{
	if (!__IsExistMainInstance())
		return;	
	
	CInstanceBase* pkInstMain=__GetMainInstancePtr();

	if (IsWarp())
		return;
	if (IsObject())
		return;
	if (IsFlag())
		return;
	if (IsResource())
		return;

	if (pkInstMain->IsGameMaster())
	{
	}
	else
	{
		if (pkInstMain->IsSameEmpire(*this))
			return;

		// HIDE_OTHER_EMPIRE_EUNHYEONG_ASSASSIN
		if (IsAffect(AFFECT_EUNHYEONG))
			return;
		// END_OF_HIDE_OTHER_EMPIRE_EUNHYEONG_ASSASSIN
	}

	if (IsGameMaster())
		return;

	__EffectContainer_AttachEffect(EFFECT_EMPIRE+eEmpire);
}

void CInstanceBase::__AttachSelectEffect()
{
	__EffectContainer_AttachEffect(EFFECT_SELECT);
}

void CInstanceBase::__DetachSelectEffect()
{
	__EffectContainer_DetachEffect(EFFECT_SELECT);
}

void CInstanceBase::__AttachTargetEffect()
{
	__EffectContainer_AttachEffect(EFFECT_TARGET);
}

void CInstanceBase::__DetachTargetEffect()
{
	__EffectContainer_DetachEffect(EFFECT_TARGET);
}


void CInstanceBase::__StoneSmoke_Inialize()
{
	m_kStoneSmoke.m_dwEftID=0;
}

void CInstanceBase::__StoneSmoke_Destroy()
{
	if (!m_kStoneSmoke.m_dwEftID)
		return;

	__DetachEffect(m_kStoneSmoke.m_dwEftID);
	m_kStoneSmoke.m_dwEftID=0;
}

void CInstanceBase::__StoneSmoke_Create(uint32_t eSmoke)
{
	m_kStoneSmoke.m_dwEftID=m_GraphicThingInstance.AttachSmokeEffect(eSmoke);
}

void CInstanceBase::SetAlpha(float fAlpha)
{
	__SetBlendRenderingMode();
	__SetAlphaValue(fAlpha);
}

bool CInstanceBase::UpdateDeleting()
{
	Update();
	Transform();

	auto rApp=CPythonApplication::GetInstance();

	float fAlpha = __GetAlphaValue() - (rApp->GetGlobalElapsedTime() * 1.5f);
	__SetAlphaValue(fAlpha);

	if (fAlpha < 0.0f)
		return false;

	return true;
}

void CInstanceBase::DeleteBlendOut()
{
	__SetBlendRenderingMode();
	__SetAlphaValue(1.0f);
	DetachTextTail();

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->NotifyDeletingCharacterInstance(GetVirtualID());
}

void CInstanceBase::ClearPVPKeySystem()
{
	g_kSet_dwPVPReadyKey.clear();
	g_kSet_dwPVPKey.clear();
	g_kSet_dwGVGKey.clear();
	g_kSet_dwDUELKey.clear();
}

void CInstanceBase::InsertPVPKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	uint32_t dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwPVPKey.insert(dwPVPKey);
}

void CInstanceBase::InsertPVPReadyKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	uint32_t dwPVPReadyKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwPVPKey.insert(dwPVPReadyKey);
}

void CInstanceBase::RemovePVPKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	uint32_t dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwPVPKey.erase(dwPVPKey);
}

void CInstanceBase::InsertGVGKey(uint32_t dwSrcGuildVID, uint32_t dwDstGuildVID)
{
	uint32_t dwGVGKey = __GetPVPKey(dwSrcGuildVID, dwDstGuildVID);
	g_kSet_dwGVGKey.insert(dwGVGKey);
}

void CInstanceBase::RemoveGVGKey(uint32_t dwSrcGuildVID, uint32_t dwDstGuildVID)
{
	uint32_t dwGVGKey = __GetPVPKey(dwSrcGuildVID, dwDstGuildVID);
	g_kSet_dwGVGKey.erase(dwGVGKey);
}

void CInstanceBase::InsertDUELKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	uint32_t dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	g_kSet_dwDUELKey.insert(dwPVPKey);
}

uint32_t CInstanceBase::__GetPVPKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	if (dwVIDSrc>dwVIDDst)
		std::swap(dwVIDSrc, dwVIDDst);

	uint32_t awSrc[2];
	awSrc[0]=dwVIDSrc;
	awSrc[1]=dwVIDDst;

    const uint8_t * s = (const uint8_t *) awSrc;
    const uint8_t * end = s + sizeof(awSrc);
    uint32_t h = 0;

    while (s < end)
    {
        h *= 16777619;
        h ^= (uint8_t) *(uint8_t *) (s++);
    }

    return h;
}

bool CInstanceBase::__FindPVPKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	uint32_t dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	if (g_kSet_dwPVPKey.end()==g_kSet_dwPVPKey.find(dwPVPKey))
		return false;

	return true;
}

bool CInstanceBase::__FindPVPReadyKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	uint32_t dwPVPKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	if (g_kSet_dwPVPReadyKey.end()==g_kSet_dwPVPReadyKey.find(dwPVPKey))
		return false;

	return true;
}
//������� ��� ������� Ȯ���Ҷ�.
bool CInstanceBase::__FindGVGKey(uint32_t dwSrcGuildID, uint32_t dwDstGuildID)
{
	uint32_t dwGVGKey=__GetPVPKey(dwSrcGuildID, dwDstGuildID);

	if (g_kSet_dwGVGKey.end()==g_kSet_dwGVGKey.find(dwGVGKey))
		return false;

	return true;
}
//��� ��忡���� ��� ��븸 ������ �� �ִ�.
bool CInstanceBase::__FindDUELKey(uint32_t dwVIDSrc, uint32_t dwVIDDst)
{
	uint32_t dwDUELKey=__GetPVPKey(dwVIDSrc, dwVIDDst);

	if (g_kSet_dwDUELKey.end()==g_kSet_dwDUELKey.find(dwDUELKey))
		return false;

	return true;
}

bool CInstanceBase::IsPVPInstance(CInstanceBase& rkInstSel)
{
	uint32_t dwVIDSrc=GetVirtualID();
	uint32_t dwVIDDst=rkInstSel.GetVirtualID();

	uint32_t dwGuildIDSrc=GetGuildID();
	uint32_t dwGuildIDDst=rkInstSel.GetGuildID();

	if (GetDuelMode())	//��� ����϶��� ~_~
		return true;	

	return __FindPVPKey(dwVIDSrc, dwVIDDst) || __FindGVGKey(dwGuildIDSrc, dwGuildIDDst);
											//__FindDUELKey(dwVIDSrc, dwVIDDst);
}

const D3DXCOLOR& CInstanceBase::GetNameColor()
{
	return GetIndexedNameColor(GetNameColorIndex());
}

UINT CInstanceBase::GetNameColorIndex()
{
	if (IsPC())
	{
		if (m_isKiller)
		{
			return NAMECOLOR_PK;
		}

		if (__IsExistMainInstance() && !__IsMainInstance())
		{			
			CInstanceBase* pkInstMain=__GetMainInstancePtr();
			if (!pkInstMain)
			{
				TraceLog("CInstanceBase::GetNameColorIndex - MainInstance is NULL");
				return NAMECOLOR_PC;
			}
			uint32_t dwVIDMain=pkInstMain->GetVirtualID();
			uint32_t dwVIDSelf=GetVirtualID();

			if (pkInstMain->GetDuelMode())
			{
				switch(pkInstMain->GetDuelMode())
				{
				case DUEL_CANNOTATTACK:
					return NAMECOLOR_PC + GetEmpireID();
				case DUEL_START:
					if(__FindDUELKey(dwVIDMain, dwVIDSelf))
						return NAMECOLOR_PVP;
					else
						return NAMECOLOR_PC + GetEmpireID();
				}
			}

			if (pkInstMain->IsSameEmpire(*this))
			{
				if (__FindPVPKey(dwVIDMain, dwVIDSelf))
				{
					return NAMECOLOR_PVP;
				}

				uint32_t dwGuildIDMain=pkInstMain->GetGuildID();
				uint32_t dwGuildIDSelf=GetGuildID();
				if (__FindGVGKey(dwGuildIDMain, dwGuildIDSelf))
				{
					return NAMECOLOR_PVP;
				}
				/*
				if (__FindDUELKey(dwVIDMain, dwVIDSelf))
				{
					return NAMECOLOR_PVP;
				}
				*/
			}
			else
			{
				return NAMECOLOR_PVP;
			}
		}

		auto rPlayer = CPythonPlayer::GetInstance();
		if (rPlayer->IsPartyMemberByVID(GetVirtualID()))
			return NAMECOLOR_PARTY;

		return NAMECOLOR_PC + GetEmpireID();
		
	}
	else if (IsNPC())
	{
		return NAMECOLOR_NPC;
	}
	else if (IsEnemy())
	{
		return NAMECOLOR_MOB;
	}
	else if (IsPoly())
	{
		return NAMECOLOR_MOB;
	}


	return D3DXCOLOR(0xffffffff);
}

const D3DXCOLOR& CInstanceBase::GetTitleColor()
{
	UINT uGrade = GetAlignmentGrade();
	if ( uGrade >= TITLE_NUM)
	{
		static D3DXCOLOR s_kD3DXClrTitleDefault(0xffffffff);
		return s_kD3DXClrTitleDefault;
	}

	return g_akD3DXClrTitle[uGrade];
}

void CInstanceBase::AttachTextTail()
{
	if (m_isTextTail)
	{
		SysLog("CInstanceBase::AttachTextTail - VID [{0}] ALREADY EXIST", GetVirtualID());
		return;
	}

	if (strcmp(GetNameString(), "<noname>") == 0)
	{
		ConsoleLog("Should not attach text tail to {0}, due to <noname> name.", GetVirtualID());
		return;
	}

	m_isTextTail = true;

	uint32_t dwVID = GetVirtualID();

	float fTextTailHeight = GetBaseHeight() + 10.0f;

	static D3DXCOLOR s_kD3DXClrTextTail = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	CPythonTextTail::GetInstance()->RegisterCharacterTextTail(m_dwGuildID, dwVID, s_kD3DXClrTextTail, fTextTailHeight);

	// CHARACTER_LEVEL
	if (m_dwLevel)
		UpdateTextTailLevel(m_dwLevel);
}

void CInstanceBase::DetachTextTail()
{
	if (!m_isTextTail)
		return;

	m_isTextTail = false;
	CPythonTextTail::GetInstance()->DeleteCharacterTextTail(GetVirtualID());
}

void CInstanceBase::UpdateTextTailLevel(uint32_t level)
{
	static uint32_t LevelColor1 = 0xFF1C9726;
	static uint32_t LevelColor2 = 0xFF65FC26;

	std::string szLevel = fmt::format("Lv {}", level);

	CPythonTextTail::GetInstance()->AttachLevel(GetVirtualID(), szLevel.c_str(), LevelColor1, LevelColor2);
}

void CInstanceBase::RefreshTextTail()
{
	CPythonTextTail::GetInstance()->SetCharacterTextTailColor(GetVirtualID(), GetNameColor());

	int32_t iAlignmentGrade = GetAlignmentGrade();
	if (TITLE_NONE == iAlignmentGrade)
	{
		CPythonTextTail::GetInstance()->DetachTitle(GetVirtualID());
	}
	else
	{
		auto itor = g_TitleNameMap.find(iAlignmentGrade);
		if (g_TitleNameMap.end() != itor)
		{
			const std::string& c_rstrTitleName = itor->second;
			CPythonTextTail::GetInstance()->AttachTitle(GetVirtualID(), c_rstrTitleName.c_str(), GetTitleColor());
		}
	}
}

void CInstanceBase::RefreshTextTailTitle()
{
	RefreshTextTail();
}

// 2004.07.25.myevan.����Ʈ �� �ٴ� ���� �ذ�
/////////////////////////////////////////////////
void CInstanceBase::__ClearAffectFlagContainer()
{
	m_kAffectFlagContainer.Clear();
}

void CInstanceBase::__ClearAffects()
{
	if (IsStone())
	{
		__StoneSmoke_Destroy();
	}
	else
	{
		for (int32_t iAffect=0; iAffect<AFFECT_NUM; ++iAffect)
		{
			__DetachEffect(m_adwCRCAffectEffect[iAffect]);
			m_adwCRCAffectEffect[iAffect]=0;
		}

		__ClearAffectFlagContainer();
	}

	m_GraphicThingInstance.__OnClearAffects();
}

/////////////////////////////////////////////////

void CInstanceBase::__SetNormalAffectFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer)
{
	for (int32_t i=0; i<CAffectFlagContainer::BIT_SIZE; ++i)
	{
		bool isOldSet=m_kAffectFlagContainer.IsSet(i);
		bool isNewSet=c_rkAffectFlagContainer.IsSet(i);

		if (isOldSet != isNewSet)
		{
			__SetAffect(i, isNewSet);

			if (isNewSet)
				m_GraphicThingInstance.__OnSetAffect(i);
			else
				m_GraphicThingInstance.__OnResetAffect(i);
		}
	}

	m_kAffectFlagContainer.CopyInstance(c_rkAffectFlagContainer);
}

void CInstanceBase::__SetStoneSmokeFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer)
{
	m_kAffectFlagContainer.CopyInstance(c_rkAffectFlagContainer);

	uint32_t eSmoke;
	if (m_kAffectFlagContainer.IsSet(STONE_SMOKE8))
		eSmoke=3;
	else if (m_kAffectFlagContainer.IsSet(STONE_SMOKE5)|m_kAffectFlagContainer.IsSet(STONE_SMOKE6)|m_kAffectFlagContainer.IsSet(STONE_SMOKE7))
		eSmoke=2;
	else if (m_kAffectFlagContainer.IsSet(STONE_SMOKE2)|m_kAffectFlagContainer.IsSet(STONE_SMOKE3)|m_kAffectFlagContainer.IsSet(STONE_SMOKE4))
		eSmoke=1;
	else
		eSmoke=0;

	__StoneSmoke_Destroy();
	__StoneSmoke_Create(eSmoke);
}

void CInstanceBase::SetAffectFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer)
{
	if (IsBuilding())
	{
		return;		
	}
	else if (IsStone())
	{
		__SetStoneSmokeFlagContainer(c_rkAffectFlagContainer);
	}
	else
	{
		__SetNormalAffectFlagContainer(c_rkAffectFlagContainer);
	}
}


void CInstanceBase::SCRIPT_SetAffect(UINT eAffect, bool isVisible)
{
	__SetAffect(eAffect, isVisible);
}

void CInstanceBase::__SetReviveInvisibilityAffect(bool isVisible)
{
	if (isVisible)
	{
		// NOTE : Dress �� �԰� ������ Alpha �� ���� �ʴ´�.
		if (IsWearingDress())
			return;

		m_GraphicThingInstance.BlendAlphaValue(0.5f, 1.0f);
	}
	else
	{
		m_GraphicThingInstance.BlendAlphaValue(1.0f, 1.0f);	
	}
}

void CInstanceBase::__Assassin_SetEunhyeongAffect(bool isVisible)
{
	if (isVisible)
	{
		// NOTE : Dress �� �԰� ������ Alpha �� ���� �ʴ´�.
		if (IsWearingDress())
			return;

		if (__IsMainInstance() || __MainCanSeeHiddenThing())
		{
			m_GraphicThingInstance.BlendAlphaValue(0.5f, 1.0f);
		}
		else
		{
			// 2004.10.16.myevan.������ ���� ����
			m_GraphicThingInstance.BlendAlphaValue(0.0f, 1.0f);
			m_GraphicThingInstance.HideAllAttachingEffect();
		}
	}
	else
	{
		m_GraphicThingInstance.BlendAlphaValue(1.0f, 1.0f);	
		m_GraphicThingInstance.ShowAllAttachingEffect();
	}
}

void CInstanceBase::__Shaman_SetParalysis(bool isParalysis)
{
	m_GraphicThingInstance.SetParalysis(isParalysis);
}



void CInstanceBase::__Warrior_SetGeomgyeongAffect(bool isVisible)
{
	if (isVisible)
	{
		if (IsWearingDress())
			return;

		if (m_kWarrior.m_dwGeomgyeongEffect)
			__DetachEffect(m_kWarrior.m_dwGeomgyeongEffect);

		m_GraphicThingInstance.SetReachScale(1.5f);
		if (m_GraphicThingInstance.IsTwoHandMode())
			m_kWarrior.m_dwGeomgyeongEffect=__AttachEffect(EFFECT_WEAPON+WEAPON_TWOHAND);
		else
			m_kWarrior.m_dwGeomgyeongEffect=__AttachEffect(EFFECT_WEAPON+WEAPON_ONEHAND);
	}
	else
	{
		m_GraphicThingInstance.SetReachScale(1.0f);

		__DetachEffect(m_kWarrior.m_dwGeomgyeongEffect);
		m_kWarrior.m_dwGeomgyeongEffect=0;
	}
}

void CInstanceBase::__SetAffect(UINT eAffect, bool isVisible)
{
	switch (eAffect)
	{
		case AFFECT_YMIR:
			if (IsAffect(AFFECT_INVISIBILITY))
				return;
			break;
/*
		case AFFECT_GWIGEOM: // ���� �Ӽ� �������� �ٲ� ����
			if (isVisible)
			{
				m_GraphicThingInstance.SetBattleHitEffect(ms_adwCRCAffectEffect[EFFECT_ELECTRIC_HIT]);
				m_GraphicThingInstance.SetBattleAttachEffect(ms_adwCRCAffectEffect[EFFECT_ELECTRIC_ATTACH]);
			}
			else
			{
				m_GraphicThingInstance.SetBattleHitEffect(ms_adwCRCAffectEffect[EFFECT_HIT]);
				m_GraphicThingInstance.SetBattleAttachEffect(0);
			}
			return;
			break;
		case AFFECT_HWAYEOM: // ȭ�� �Ӽ� �������� �ٲ� ����
			if (isVisible)
			{
				m_GraphicThingInstance.SetBattleHitEffect(ms_adwCRCAffectEffect[EFFECT_FLAME_HIT]);
				m_GraphicThingInstance.SetBattleAttachEffect(ms_adwCRCAffectEffect[EFFECT_FLAME_ATTACH]);
			}
			else
			{
				m_GraphicThingInstance.SetBattleHitEffect(ms_adwCRCAffectEffect[EFFECT_HIT]);
				m_GraphicThingInstance.SetBattleAttachEffect(0);
			}
			// ȭ������ ������ ���� �Ͻ������� Visible �մϴ�.
			return;
			break;
*/
		case AFFECT_CHEONGEUN:
			m_GraphicThingInstance.SetResistFallen(isVisible);
			break;
		case AFFECT_GEOMGYEONG:
			__Warrior_SetGeomgyeongAffect(isVisible);
			return;
			break;
		case AFFECT_REVIVE_INVISIBILITY:
			__Assassin_SetEunhyeongAffect(isVisible);
			break;
		case AFFECT_EUNHYEONG:
			__Assassin_SetEunhyeongAffect(isVisible);
			break;
		case AFFECT_GYEONGGONG:
		case AFFECT_KWAESOK:
			// �����, ����� �۶��� Attaching ��ŵ�ϴ�. - [levites]
			if (isVisible)
				if (!IsWalking())
					return;
			break;
		case AFFECT_INVISIBILITY:
			// 2004.07.17.levites.isShow�� ViewFrustumCheck�� ����
			if (isVisible)
			{
				m_GraphicThingInstance.ClearAttachingEffect();
				__EffectContainer_Destroy();
				DetachTextTail();
			}
			else
			{
				m_GraphicThingInstance.BlendAlphaValue(1.0f, 1.0f);
				AttachTextTail();
				RefreshTextTail();
			}
			return;
			break;
//		case AFFECT_FAINT:
//			m_GraphicThingInstance.SetFaint(isVisible);
//			break;
//		case AFFECT_SLEEP:
//			m_GraphicThingInstance.SetSleep(isVisible);
//			break;
		case AFFECT_STUN:
			m_GraphicThingInstance.SetSleep(isVisible);
			break;
	}

	if (eAffect>=AFFECT_NUM)
	{
		TraceLog("CInstanceBase[VID:{}]::SetAffect(eAffect:%d<AFFECT_NUM:{}, isVisible={})", GetVirtualID(), eAffect, isVisible);
		return;
	}

	if (isVisible)
	{
		if (!m_adwCRCAffectEffect[eAffect])
		{
			m_adwCRCAffectEffect[eAffect]=__AttachEffect(EFFECT_AFFECT+eAffect);
		}
	}
	else
	{
		if (m_adwCRCAffectEffect[eAffect])
		{
			__DetachEffect(m_adwCRCAffectEffect[eAffect]);
			m_adwCRCAffectEffect[eAffect]=0;
		}
	}
}

bool CInstanceBase::IsPossibleEmoticon()
{
	auto rkEftMgr=CEffectManager::GetInstance();
	for(uint32_t eEmoticon = 0; eEmoticon < EMOTICON_NUM; eEmoticon++)
	{
		uint32_t effectID = ms_adwCRCAffectEffect[EFFECT_EMOTICON+eEmoticon];
		if( effectID &&	rkEftMgr->IsAliveEffect(effectID) )
			return false;
	}

	if(ELTimer_GetMSec() - m_dwEmoticonTime < 1000)
	{
		TraceLog("ELTimer_GetMSec() - m_dwEmoticonTime");
		return false;
	}

	return true;
}

void CInstanceBase::SetFishEmoticon()
{
	SetEmoticon(EMOTICON_FISH);
}

void CInstanceBase::SetEmoticon(UINT eEmoticon)
{
	if (eEmoticon == 99)
	{
		m_dwEmoticonTime = ELTimer_GetMSec();
		return;
	}

	if (eEmoticon >= EMOTICON_NUM)
	{
		SysLog("CInstanceBase[VID:{0}]::SetEmoticon(eEmoticon:{1}<EMOTICON_NUM:{2}, isVisible={3})", GetVirtualID(), eEmoticon);
		return;
	}
	if (IsPossibleEmoticon())
	{
		D3DXVECTOR3 v3Pos = D3DXVECTOR3(0, 0, float(GetBaseHeight() + m_GraphicThingInstance.GetHeight()));
		m_GraphicThingInstance.AttachEffectByID(0, nullptr, ms_adwCRCAffectEffect[EFFECT_EMOTICON + eEmoticon], &v3Pos);
		m_dwEmoticonTime = ELTimer_GetMSec();
	}
}

void CInstanceBase::SetDustGap(float fDustGap)
{
	ms_fDustGap=fDustGap;
}

void CInstanceBase::SetHorseDustGap(float fDustGap)
{
	ms_fHorseDustGap=fDustGap;
}

void CInstanceBase::__DetachEffect(uint32_t dwEID)
{
	m_GraphicThingInstance.DettachEffect(dwEID);
}

uint32_t CInstanceBase::__AttachEffect(UINT eEftType)
{
	// 2004.07.17.levites.isShow�� ViewFrustumCheck�� ����
	if (IsAffect(AFFECT_INVISIBILITY))
		return 0;

	if (eEftType>=EFFECT_NUM)
		return 0;

	if (ms_astAffectEffectAttachBone[eEftType].empty())
	{
		return m_GraphicThingInstance.AttachEffectByID(0, NULL, ms_adwCRCAffectEffect[eEftType]);
	}
	else
	{
		std::string & rstrBoneName = ms_astAffectEffectAttachBone[eEftType];
		const char * c_szBoneName;
		// ��տ� ���� �� ����Ѵ�.
		// �̷� ���� ���� ó���� �س��� ���� ĳ���� ���� Equip �� Bone Name �� �ٸ��� ����.
		if (0 == rstrBoneName.compare("PART_WEAPON"))
		{
			if (m_GraphicThingInstance.GetAttachingBoneName(PART_WEAPON, &c_szBoneName))
			{
				return m_GraphicThingInstance.AttachEffectByID(0, c_szBoneName, ms_adwCRCAffectEffect[eEftType]);
			}
		}
		else if (0 == rstrBoneName.compare("PART_WEAPON_LEFT"))
		{
			if (m_GraphicThingInstance.GetAttachingBoneName(PART_WEAPON_LEFT, &c_szBoneName))
			{
				return m_GraphicThingInstance.AttachEffectByID(0, c_szBoneName, ms_adwCRCAffectEffect[eEftType]);
			}
		}
		else
		{
			return m_GraphicThingInstance.AttachEffectByID(0, rstrBoneName.c_str(), ms_adwCRCAffectEffect[eEftType]);
		}
	}

	return 0;
}

void CInstanceBase::__ComboProcess()
{
	/*
	uint32_t dwcurComboIndex = m_GraphicThingInstance.GetComboIndex();

	if (0 != dwcurComboIndex)
	{
		if (m_dwLastComboIndex != m_GraphicThingInstance.GetComboIndex())
		{
			if (!m_GraphicThingInstance.IsHandMode() & IsAffect(AFFECT_HWAYEOM))
			{
				__AttachEffect(EFFECT_FLAME_ATTACK);
			}
		}
	}

	m_dwLastComboIndex = dwcurComboIndex;
	*/
}

bool CInstanceBase::RegisterEffect(UINT eEftType, const char* c_szEftAttachBone, const char* c_szEftName, bool isCache)
{
	if (eEftType>=EFFECT_NUM)
		return false;

	ms_astAffectEffectAttachBone[eEftType]=c_szEftAttachBone;

	uint32_t& rdwCRCEft=ms_adwCRCAffectEffect[eEftType];
	if (!CEffectManager::GetInstance()->RegisterEffect2(c_szEftName, &rdwCRCEft, isCache))
	{
		TraceLog("CInstanceBase::RegisterEffect(eEftType={}, c_szEftAttachBone={}, c_szEftName={}, isCache={}) - Error", eEftType, c_szEftAttachBone, c_szEftName, isCache);
		rdwCRCEft=0;
		return false;
	}

	return true;
}

void CInstanceBase::RegisterTitleName(int32_t iIndex, const char * c_szTitleName)
{
	g_TitleNameMap.insert(std::make_pair(iIndex, c_szTitleName));
}

D3DXCOLOR __RGBToD3DXColoru(UINT r, UINT g, UINT b)
{
	uint32_t dwColor=0xff;dwColor<<=8;
	dwColor|=r;dwColor<<=8;
	dwColor|=g;dwColor<<=8;
	dwColor|=b;

	return D3DXCOLOR(dwColor);
}

bool CInstanceBase::RegisterNameColor(UINT uIndex, UINT r, UINT g, UINT b)
{
	if (uIndex>=NAMECOLOR_NUM)
		return false;

	g_akD3DXClrName[uIndex]=__RGBToD3DXColoru(r, g, b);
	return true;
}

bool CInstanceBase::RegisterTitleColor(UINT uIndex, UINT r, UINT g, UINT b)
{
	if (uIndex>=TITLE_NUM)
		return false;

	g_akD3DXClrTitle[uIndex]=__RGBToD3DXColoru(r, g, b);
	return true;	
}
