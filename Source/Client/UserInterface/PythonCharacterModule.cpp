#include "StdAfx.h"
#include "PythonCharacterManager.h"
#include "PythonNonPlayer.h"

static void chrDismountHorse()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pkInst)
 		pkInst->DismountHorse();

}

static void chrMountHorse()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pkInst)
		pkInst->MountHorse(20030);

}

static void chrDestroy()
{
	CPythonCharacterManager::GetInstance()->Destroy();

}

static void chrUpdate()
{
	CPythonCharacterManager::GetInstance()->Update();

}

static void chrDeform()
{
	CPythonCharacterManager::GetInstance()->Deform();

}

static void chrRender()
{
	CPythonCharacterManager::GetInstance()->Render();

}

static void chrRenderCollision()
{
	CPythonCharacterManager::GetInstance()->RenderCollision();

}

static void chrCreateInstance(uint32_t iVirtualID)
{
	CPythonCharacterManager::GetInstance()->RegisterInstance(iVirtualID);
}

static void chrCreateInstance2(uint32_t iVirtualID, std::unordered_map<std::string, uint32_t> dict)
{
	CInstanceBase::SCreateData kCreateData;
	kCreateData.m_bType = CActorInstance::TYPE_PC;
	kCreateData.m_dwLevel = 0;
	kCreateData.m_dwGuildID = 0;
	kCreateData.m_dwEmpireID = 0;
	kCreateData.m_dwVID = iVirtualID;
	kCreateData.m_dwMountVnum = 0;
	kCreateData.m_dwRace = 0;
	kCreateData.m_fRot = CInstanceBase::DIR_NORTH;
	kCreateData.m_lPosX = 0;
	kCreateData.m_lPosY = 0;
	kCreateData.m_stName = "NONAME";
	kCreateData.m_dwStateFlags = 0;
	kCreateData.m_dwMovSpd = 100;
	kCreateData.m_dwAtkSpd = 100;
	kCreateData.m_sAlignment = 0;
	kCreateData.m_byPKMode = 0;
	kCreateData.m_kAffectFlags.Clear();
	kCreateData.m_dwArmor = 8;
	kCreateData.m_dwWeapon = 0;
	kCreateData.m_dwHair = 0;
	kCreateData.m_isMain = false;

	if (dict.contains("horse"))
		kCreateData.m_dwMountVnum = dict["horse"];

	if (dict.contains("x"))
		kCreateData.m_lPosX = int32_t(dict["x"]);

	if (dict.contains("y"))
		kCreateData.m_lPosY = int32_t(dict["y"]);

	CPythonCharacterManager::GetInstance()->CreateInstance(kCreateData);
}

static void chrDeleteInstance(uint32_t iVirtualID)
{

	CPythonCharacterManager::GetInstance()->DeleteInstance(iVirtualID);

}

static void chrDeleteInstanceByFade(uint32_t iVirtualID)
{

	CPythonCharacterManager::GetInstance()->DeleteInstanceByFade(iVirtualID);

}

static void chrSelectInstance(uint32_t iVirtualID)
{

	CPythonCharacterManager::GetInstance()->SelectInstance(iVirtualID);

}

static bool chrHasInstance(int32_t iVirtualID)
{

	return CPythonCharacterManager::GetInstance()->GetInstancePtr(uint32_t(iVirtualID));
}

static BOOL chrIsEnemy(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (!pInstance)
		return FALSE;

	return  pInstance->IsEnemy();
}

static BOOL chrIsNPC(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (!pInstance)
		return FALSE;

	return  pInstance->IsNPC();
}

static BOOL chrIsGameMaster(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (!pInstance)
		return  FALSE;
	
	return  pInstance->IsGameMaster();
}

static bool chrIsPartyMember(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (!pInstance)
		return  false;

	return  pInstance->IsPartyMember();
}

static void chrSelect(uint32_t iVirtualID)
{

	CPythonCharacterManager::GetInstance()->SelectInstance(iVirtualID);

}

static void chrSetAddRenderMode(uint32_t iVirtualID, float fr, float fg, float fb)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (!pkInst)
		return;

	pkInst->SetAddRenderMode();
	pkInst->SetAddColor(D3DXCOLOR(fr, fg, fb, 1.0f));


}

static void chrSetBlendRenderMode(uint32_t iVirtualID, float fAlpha)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (pkInst)
		pkInst->SetAlpha(fAlpha);
	

}

static void chrUnselect(uint32_t iVirtualID)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
		pkInst->RestoreRenderMode();

}

static void chrHide()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->Hide();
}

static void chrShow()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->Show();

}

static int64_t chrPick()
{
	uint32_t VirtualID = 0;
	if (CPythonCharacterManager::GetInstance()->OLD_GetPickedInstanceVID(&VirtualID))
		return  VirtualID;

	return  -1;
}

static int32_t chrPickAll()
{
	return CPythonCharacterManager::GetInstance()->PickAll();
}

static void chrSetArmor(uint32_t iForm)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pkInst)
		return;

	pkInst->SetArmor(iForm);

	pkInst->RegisterBoundingSphere();


}

static void chrSetWeapon(uint32_t iForm)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetWeapon(iForm);


}

static void chrChangeShape(uint32_t iForm)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->ChangeArmor(iForm);


}

static void chrSetRace(uint32_t iRace)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetRace(iRace);


}

static void chrSetHair(uint32_t iRace)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetHair(iRace);


}

static void chrChangeHair(uint32_t iHair)
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->ChangeHair(iHair);
}

static void chrSetVirtualID(uint32_t iVID)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetVirtualID(iVID);

}

static void chrSetNameString(std::string c_szName)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetNameString(c_szName.c_str(), c_szName.size());

}

static void chrSetInstanceType(int32_t iInstanceType)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetInstanceType(iInstanceType);

}

static void chrSetPixelPosition(float iX, float iY, float iZ)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->NEW_SetPixelPosition(TPixelPosition(iX, iY, iZ));

}

static void chrSetDirection(int32_t iDirection)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	pkInst->SetDirection(iDirection);

}

static void chrRefresh()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
		return;

	// Select ȭ�鿡���� WAIT ����� �غ� ���� ���� �����̱� ������ ������ �����.
	//pkInst->Refresh(MOTION_WAIT, true);
}

static void chrRevive()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pkInst)
 	pkInst->Revive();

}

static void chrDie()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->Die();

}

static void chrAttachEffectByID(int32_t iParentPartIndex, std::string pszBoneName, int32_t iEffectId)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
		return;

	//pkInst->AttachEffectByID(iParentPartIndex, pszBoneName, iEffectID, CActorInstance::EFFECT_LIFE_INFINITE);
}

static void chrAttachEffectByID2(int32_t iParentPartIndex, std::string pszBoneName, int32_t iEffectId, int32_t iLife)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
		return;

	//pkInst->AttachEffectByID(iParentPartIndex, pszBoneName, iEffectID, iLife);
}

static void chrAttachEffectByName(int32_t iParentPartIndex, std::string pszBoneName, std::string pszEffectName)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
		return;

	//pkInst->AttachEffectByName(iParentPartIndex, pszBoneName, pszEffectName, CActorInstance::EFFECT_LIFE_INFINITE);
}

static void chrAttachEffectByName2(int32_t iParentPartIndex, std::string pszBoneName, std::string pszEffectName, int32_t iLife)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
		return;

	//pkInst->AttachEffectByName(iParentPartIndex, pszBoneName, pszEffectName, iLife);
}

static void chrLookAt(int32_t iCellX, int32_t iCellY)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pkInst)
		return;

// 	pkInst->LookAt(TPixelPosition(iCellX * c_Section_xAttributeCellSize, iCellY * c_Section_xAttributeCellSize));

}

static void chrSetMotionMode(int32_t iMotionMode)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetMotionMode(iMotionMode);
}

static void chrSetLoopMotion(uint16_t iMotionIndex)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetLoopMotion(iMotionIndex);


}

static void chrBlendLoopMotion(uint16_t iMotionIndex, float fBlendTime)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pkInst)
		pkInst->SetLoopMotion(iMotionIndex, fBlendTime);


}

static void chrPushOnceMotion(uint16_t iMotionIndex, float fBlendTime)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (pkInst)
		pkInst->PushOnceMotion(iMotionIndex, fBlendTime);
}

static void chrPushOnceMotion2(uint16_t iMotionIndex)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (pkInst)
		pkInst->PushOnceMotion(iMotionIndex, 0.1f);
}

static void chrPushLoopMotion(uint16_t iMotionIndex, float fBlendTime)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (pkInst)
		pkInst->PushLoopMotion(iMotionIndex, fBlendTime);
}

static void chrPushLoopMotion2(uint16_t iMotionIndex)
{
	CInstanceBase* pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (pkInst)
		pkInst->PushLoopMotion(iMotionIndex, 0.1f);
}

static std::tuple<float,float,float> chrGetPixelPosition(uint32_t iVirtualID)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (!pkInst)
		return std::make_tuple(0.0f, 0.0f, 0.0f);

	TPixelPosition PixelPosition;
	pkInst->NEW_GetPixelPosition(&PixelPosition);

	return std::make_tuple( PixelPosition.x, PixelPosition.y, PixelPosition.z);
}

static void chrSetRotation(float fRotation)
{

	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pCharacterInstance)
		return;

	fRotation = fmod(fRotation + 180.0f, 360.0f);

	pCharacterInstance->SetRotation(fRotation);


}

static void chrSetRotationAll(float fRotX, float fRotY, float fRotZ)
{

	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pCharacterInstance)
		return;

	pCharacterInstance->GetGraphicThingInstanceRef().SetXYRotation(fRotX, fRotY);
	pCharacterInstance->GetGraphicThingInstanceRef().SetRotation(fRotZ);

}

static void chrBlendRotation(float fRotation, float fBlendTime)
{

	fRotation = fmod(720.0f - fRotation, 360.0f);

	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pCharacterInstance)
		return;

	pCharacterInstance->BlendRotation(fRotation, fBlendTime);


}

static float chrGetRotation()
{
	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pCharacterInstance)
		return  0.0f;

	float fRotation = pCharacterInstance->GetRotation();

	return  360.0f - fRotation;
}

static uint32_t chrGetRace()
{
	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pCharacterInstance)
		return  0U;

	return  pCharacterInstance->GetRace();
}

static std::string chrGetName()
{
	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pCharacterInstance)
		return  "";

	return  pCharacterInstance->GetNameString();
}

static std::string chrGetNameByVID(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (!pInstance)
		return  "None";

	return  pInstance->GetNameString();
}

static uint32_t chrGetGuildID(uint32_t iVirtualID)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (!pInstance)
		return  0;

	return  pInstance->GetGuildID();
}

static std::tuple<int32_t,int32_t> chrGetProjectPosition(uint32_t iVirtualID, int iHeight)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (!pInstance)
		return std::make_tuple( -100, -100);

	TPixelPosition PixelPosition;
	pInstance->NEW_GetPixelPosition(&PixelPosition);

	auto rpyGraphic = CPythonGraphic::GetInstance();

	float fx, fy, fz;
	rpyGraphic->ProjectPosition(PixelPosition.x,
							   -PixelPosition.y,
							   PixelPosition.z + float(iHeight),
							   &fx, &fy, &fz);

	if (1 == int32_t(fz))
		return std::make_tuple( -100, -100);

	return std::make_tuple( int32_t(fx), int32_t(fy));
}

static uint32_t chrGetVirtualNumber(uint32_t iVirtualID)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (pkInst)
		return  pkInst->GetVirtualNumber();

	return uint32_t(CActorInstance::TYPE_PC);
}

static uint32_t chrGetInstanceType(uint32_t iVirtualID)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);

	if (pkInst)
		return  pkInst->GetInstanceType();
	
	return  uint32_t(CActorInstance::TYPE_PC);
}

static std::tuple<float,float,float,float> chrGetBoundBoxOnlyXY()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
	return std::make_tuple( 0.0f, 0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 v3Min, v3Max;
	pkInst->GetBoundBox(&v3Min, &v3Max);

	return std::make_tuple( v3Min.x, v3Min.y, v3Max.x, v3Max.y);
}

static int32_t chrRaceToJob(int32_t race)
{

	return  RaceToJob(race);
}

static int32_t chrRaceToSex(int32_t race)
{

	return  RaceToSex(race);
}

static std::tuple<int32_t, uint32_t> chrtestGetPKData()
{
	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();

	if (!pkInst)
		return std::make_tuple( 0, 4U);

	return std::make_tuple( pkInst->GetAlignment(), pkInst->GetAlignmentGrade());
}

static void chrFaintTest()
{
	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (pCharacterInstance)
	{
		if (pCharacterInstance->GetGraphicThingInstanceRef().IsFaint())
		{
			pCharacterInstance->GetGraphicThingInstanceRef().SetFaint(false);
		}
		else
		{
			pCharacterInstance->GetGraphicThingInstanceRef().InterceptOnceMotion(MOTION_DAMAGE_FLYING);
			pCharacterInstance->GetGraphicThingInstanceRef().PushOnceMotion(MOTION_STAND_UP);
			pCharacterInstance->GetGraphicThingInstanceRef().PushLoopMotion(MOTION_WAIT);
			pCharacterInstance->GetGraphicThingInstanceRef().SetFaint(true);
		}
	}


}

static void chrSetMoveSpeed(float iSpeed)
{

}

static void chrSetAttackSpeed(float iSpeed)
{

}

static void chrWeaponTraceSetTexture(std::string szPathName)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetMainInstancePtr();
	if (pInstance)
	{
		pInstance->GetGraphicThingInstanceRef().SetWeaponTraceTexture(szPathName.c_str());
	}

}

static void chrWeaponTraceUseAlpha()
{
	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetMainInstancePtr();
	if (pInstance)
	{
		pInstance->GetGraphicThingInstanceRef().UseAlphaWeaponTrace();
	}

}

static void chrWeaponTraceUseTexture()
{
	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetMainInstancePtr();
	if (pInstance)
	{
		pInstance->GetGraphicThingInstanceRef().UseTextureWeaponTrace();
	}

}

static void chrMoveToDestPosition(uint32_t iVID, float ix, float iy)
{

	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVID);
	if (!pCharacterInstance)

	pCharacterInstance->NEW_MoveToDestPixelPositionDirection(TPixelPosition(ix, iy, 0.0f));


}

static void chrtestSetComboType(uint16_t iComboType)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	if (!pkInst)
		return;

	pkInst->GetGraphicThingInstanceRef().SetComboType(iComboType);


}

static void chrtestSetAddRenderMode(uint32_t iVirtualID, uint32_t iColor)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
	{
		pkInst->SetAddRenderMode();
		pkInst->SetAddColor(0xff000000 | iColor);
	}


}

static void chrtestSetModulateRenderMode(uint32_t iVirtualID, uint32_t iColor)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
	{
		pkInst->SetModulateRenderMode();
		pkInst->SetAddColor(0xff000000 | iColor);
	}


}

static void chrtestSetAddRenderModeRGB(uint32_t iVirtualID, float fr, float fg, float fb)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
	{
		pkInst->SetAddRenderMode();
		pkInst->SetAddColor(D3DXCOLOR(fr, fg, fb, 1.0f));
	}


}

static void chrtestSetModulateRenderModeRGB(uint32_t iVirtualID, float fr, float fg, float fb)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
	{
		pkInst->SetModulateRenderMode();
		pkInst->SetAddColor(D3DXCOLOR(fr, fg, fb, 1.0f));
	}


}

static void chrtestSetSpecularRenderMode(uint32_t iVirtualID, int32_t iPart, float fAlpha)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
	{
		pkInst->GetGraphicThingInstanceRef().SetSpecularInfo(TRUE, iPart, fAlpha);
	}


}

static void chrtestSetSpecularRenderMode2(uint32_t iVirtualID, int32_t iPart, float fAlpha)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
	{
		pkInst->GetGraphicThingInstanceRef().SetSpecularInfoForce(TRUE, iPart, fAlpha);
	}


}

static void chrtestRestoreRenderMode(uint32_t iVirtualID)
{

	CInstanceBase * pkInst = CPythonCharacterManager::GetInstance()->GetInstancePtr(iVirtualID);
	if (pkInst)
	{
		pkInst->RestoreRenderMode();
	}


}

static void chrtestSetRideMan(int32_t ix, int32_t iy, uint32_t imount)
{
	CInstanceBase * pCharacterInstance = CPythonCharacterManager::GetInstance()->RegisterInstance(1);
	CInstanceBase::SCreateData kCreateData;
	ZeroMemory(&kCreateData, sizeof(kCreateData));
	kCreateData.m_bType = CActorInstance::TYPE_PC;
	kCreateData.m_dwRace = 0;
	kCreateData.m_dwArmor = 0;
	kCreateData.m_dwHair = 100;
	kCreateData.m_dwMovSpd = 100;
	kCreateData.m_dwAtkSpd = 100;
	kCreateData.m_dwMountVnum = imount;
	kCreateData.m_lPosX = ix;
	kCreateData.m_lPosY = iy;
	pCharacterInstance->Create(kCreateData);


}



PYBIND11_EMBEDDED_MODULE(chr, m)
{
	m.def("DismountHorse",	chrDismountHorse);
	m.def("MountHorse",	chrMountHorse);
	m.def("Destroy",	chrDestroy);
	m.def("Update",	chrUpdate);
	m.def("Deform",	chrDeform);
	m.def("Render",	chrRender);
	m.def("RenderCollision",	chrRenderCollision);
	m.def("CreateInstance",	chrCreateInstance);
	m.def("CreateInstance", chrCreateInstance2);
	m.def("DeleteInstance",	chrDeleteInstance);
	m.def("DeleteInstanceByFade",	chrDeleteInstanceByFade);
	m.def("SelectInstance",	chrSelectInstance);
	m.def("HasInstance",	chrHasInstance);
	m.def("IsEnemy",	chrIsEnemy);
	m.def("IsNPC",	chrIsNPC);
	m.def("IsGameMaster",	chrIsGameMaster);
	m.def("IsPartyMember",	chrIsPartyMember);
	m.def("Select",	chrSelect);
	m.def("SetAddRenderMode",	chrSetAddRenderMode);
	m.def("SetBlendRenderMode",	chrSetBlendRenderMode);
	m.def("Unselect",	chrUnselect);
	m.def("Hide",	chrHide);
	m.def("Show",	chrShow);
	m.def("Pick",	chrPick);
	m.def("PickAll",	chrPickAll);
	m.def("SetArmor",	chrSetArmor);
	m.def("SetWeapon",	chrSetWeapon);
	m.def("ChangeShape",	chrChangeShape);
	m.def("SetRace",	chrSetRace);
	m.def("SetHair",	chrSetHair);
	m.def("ChangeHair",	chrChangeHair);
	m.def("SetVirtualID",	chrSetVirtualID);
	m.def("SetNameString",	chrSetNameString);
	m.def("SetInstanceType",	chrSetInstanceType);
	m.def("SetPixelPosition",	chrSetPixelPosition);
	m.def("SetDirection",	chrSetDirection);
	m.def("Refresh",	chrRefresh);
	m.def("Revive",	chrRevive);
	m.def("Die",	chrDie);
	m.def("AttachEffectByID",	chrAttachEffectByID);
	m.def("AttachEffectByID", chrAttachEffectByID2);
	m.def("AttachEffectByName",	chrAttachEffectByName);
	m.def("AttachEffectByName", chrAttachEffectByName2);
	m.def("LookAt",	chrLookAt);
	m.def("SetMotionMode",	chrSetMotionMode);
	m.def("SetLoopMotion",	chrSetLoopMotion);
	m.def("BlendLoopMotion",	chrBlendLoopMotion);
	m.def("PushOnceMotion",	chrPushOnceMotion);
	m.def("PushOnceMotion", chrPushOnceMotion2);
	m.def("PushLoopMotion",	chrPushLoopMotion);
	m.def("PushLoopMotion", chrPushLoopMotion2);
	m.def("GetPixelPosition",	chrGetPixelPosition);
	m.def("SetRotation",	chrSetRotation);
	m.def("SetRotationAll",	chrSetRotationAll);
	m.def("BlendRotation",	chrBlendRotation);
	m.def("GetRotation",	chrGetRotation);
	m.def("GetRace",	chrGetRace);
	m.def("GetName",	chrGetName);
	m.def("GetNameByVID",	chrGetNameByVID);
	m.def("GetGuildID",	chrGetGuildID);
	m.def("GetProjectPosition",	chrGetProjectPosition);
	m.def("GetVirtualNumber",	chrGetVirtualNumber);
	m.def("GetInstanceType",	chrGetInstanceType);
	m.def("GetBoundBoxOnlyXY",	chrGetBoundBoxOnlyXY);
	m.def("RaceToJob",	chrRaceToJob);
	m.def("RaceToSex",	chrRaceToSex);
	m.def("testGetPKData",	chrtestGetPKData);
	m.def("FaintTest",	chrFaintTest);
	m.def("SetMoveSpeed",	chrSetMoveSpeed);
	m.def("SetAttackSpeed",	chrSetAttackSpeed);
	m.def("WeaponTraceSetTexture",	chrWeaponTraceSetTexture);
	m.def("WeaponTraceUseAlpha",	chrWeaponTraceUseAlpha);
	m.def("WeaponTraceUseTexture",	chrWeaponTraceUseTexture);
	m.def("MoveToDestPosition",	chrMoveToDestPosition);
	m.def("testSetComboType",	chrtestSetComboType);
	m.def("testSetAddRenderMode",	chrtestSetAddRenderMode);
	m.def("testSetModulateRenderMode",	chrtestSetModulateRenderMode);
	m.def("testSetAddRenderModeRGB",	chrtestSetAddRenderModeRGB);
	m.def("testSetModulateRenderModeRGB",	chrtestSetModulateRenderModeRGB);
	m.def("testSetSpecularRenderMode",	chrtestSetSpecularRenderMode);
	m.def("testSetSpecularRenderMode2",	chrtestSetSpecularRenderMode2);
	m.def("testRestoreRenderMode",	chrtestRestoreRenderMode);
	m.def("testSetRideMan",	chrtestSetRideMan);

	m.attr("PLAYER_NAME_MAX_LEN") = int32_t(PLAYER_NAME_MAX_LEN);
	m.attr("MOTION_NONE") = int32_t(MOTION_NONE);
	m.attr("MOTION_SPAWN") = int32_t(MOTION_SPAWN);
	m.attr("MOTION_WAIT") = int32_t(MOTION_WAIT);
	m.attr("MOTION_WALK") = int32_t(MOTION_WALK);
	m.attr("MOTION_RUN") = int32_t(MOTION_RUN);
	m.attr("MOTION_CHANGE_WEAPON") = int32_t(MOTION_CHANGE_WEAPON);
	m.attr("MOTION_DAMAGE") = int32_t(MOTION_DAMAGE);
	m.attr("MOTION_DAMAGE_FLYING") = int32_t(MOTION_DAMAGE_FLYING);
	m.attr("MOTION_STAND_UP") = int32_t(MOTION_STAND_UP);
	m.attr("MOTION_DAMAGE_BACK") = int32_t(MOTION_DAMAGE_BACK);
	m.attr("MOTION_DAMAGE_FLYING_BACK") = int32_t(MOTION_DAMAGE_FLYING_BACK);
	m.attr("MOTION_STAND_UP_BACK") = int32_t(MOTION_STAND_UP_BACK);
	m.attr("MOTION_DEAD") = int32_t(MOTION_DEAD);
	m.attr("MOTION_DEAD_BACK") = int32_t(MOTION_DEAD_BACK);
	m.attr("MOTION_NORMAL_ATTACK") = int32_t(MOTION_NORMAL_ATTACK);
	m.attr("MOTION_COMBO_ATTACK_1") = int32_t(MOTION_COMBO_ATTACK_1);
	m.attr("MOTION_COMBO_ATTACK_2") = int32_t(MOTION_COMBO_ATTACK_2);
	m.attr("MOTION_COMBO_ATTACK_3") = int32_t(MOTION_COMBO_ATTACK_3);
	m.attr("MOTION_COMBO_ATTACK_4") = int32_t(MOTION_COMBO_ATTACK_4);
	m.attr("MOTION_COMBO_ATTACK_5") = int32_t(MOTION_COMBO_ATTACK_5);
	m.attr("MOTION_COMBO_ATTACK_6") = int32_t(MOTION_COMBO_ATTACK_6);
	m.attr("MOTION_COMBO_ATTACK_7") = int32_t(MOTION_COMBO_ATTACK_7);
	m.attr("MOTION_COMBO_ATTACK_8") = int32_t(MOTION_COMBO_ATTACK_8);
	m.attr("MOTION_INTRO_WAIT") = int32_t(MOTION_INTRO_WAIT);
	m.attr("MOTION_INTRO_SELECTED") = int32_t(MOTION_INTRO_SELECTED);
	m.attr("MOTION_INTRO_NOT_SELECTED") = int32_t(MOTION_INTRO_NOT_SELECTED);
	m.attr("MOTION_FISHING_THROW") = int32_t(MOTION_FISHING_THROW);
	m.attr("MOTION_FISHING_WAIT") = int32_t(MOTION_FISHING_WAIT);
	m.attr("MOTION_FISHING_STOP") = int32_t(MOTION_FISHING_STOP);
	m.attr("MOTION_FISHING_REACT") = int32_t(MOTION_FISHING_REACT);
	m.attr("MOTION_FISHING_CATCH") = int32_t(MOTION_FISHING_CATCH);
	m.attr("MOTION_FISHING_FAIL") = int32_t(MOTION_FISHING_FAIL);
	m.attr("MOTION_STOP") = int32_t(MOTION_STOP);
	m.attr("MOTION_SKILL") = int32_t(MOTION_SKILL);
	m.attr("MOTION_CLAP") = int32_t(MOTION_CLAP);
	m.attr("MOTION_DANCE_1") = int32_t(MOTION_DANCE_1);
	m.attr("MOTION_DANCE_2") = int32_t(MOTION_DANCE_2);
	m.attr("MOTION_DANCE_3") = int32_t(MOTION_DANCE_3);
	m.attr("MOTION_DANCE_4") = int32_t(MOTION_DANCE_4);
	m.attr("MOTION_DANCE_5") = int32_t(MOTION_DANCE_5);
	m.attr("MOTION_DANCE_6") = int32_t(MOTION_DANCE_6);
	m.attr("MOTION_CONGRATULATION") = int32_t(MOTION_CONGRATULATION);
	m.attr("MOTION_FORGIVE") = int32_t(MOTION_FORGIVE);
	m.attr("MOTION_ANGRY") = int32_t(MOTION_ANGRY);
	m.attr("MOTION_ATTRACTIVE") = int32_t(MOTION_ATTRACTIVE);
	m.attr("MOTION_SAD") = int32_t(MOTION_SAD);
	m.attr("MOTION_SHY") = int32_t(MOTION_SHY);
	m.attr("MOTION_CHEERUP") = int32_t(MOTION_CHEERUP);
	m.attr("MOTION_BANTER") = int32_t(MOTION_BANTER);
	m.attr("MOTION_JOY") = int32_t(MOTION_JOY);
	m.attr("MOTION_CHEERS_1") = int32_t(MOTION_CHEERS_1);
	m.attr("MOTION_CHEERS_2") = int32_t(MOTION_CHEERS_2);
	m.attr("MOTION_KISS_WITH_WARRIOR") = int32_t(MOTION_KISS_WITH_WARRIOR);
	m.attr("MOTION_KISS_WITH_ASSASSIN") = int32_t(MOTION_KISS_WITH_ASSASSIN);
	m.attr("MOTION_KISS_WITH_SURA") = int32_t(MOTION_KISS_WITH_SURA);
	m.attr("MOTION_KISS_WITH_SHAMAN") = int32_t(MOTION_KISS_WITH_SHAMAN);
	m.attr("MOTION_FRENCH_KISS_WITH_WARRIOR") = int32_t(MOTION_FRENCH_KISS_WITH_WARRIOR);
	m.attr("MOTION_FRENCH_KISS_WITH_ASSASSIN") = int32_t(MOTION_FRENCH_KISS_WITH_ASSASSIN);
	m.attr("MOTION_FRENCH_KISS_WITH_SURA") = int32_t(MOTION_FRENCH_KISS_WITH_SURA);
	m.attr("MOTION_FRENCH_KISS_WITH_SHAMAN") = int32_t(MOTION_FRENCH_KISS_WITH_SHAMAN);
	m.attr("MOTION_SLAP_HIT_WITH_WARRIOR") = int32_t(MOTION_SLAP_HIT_WITH_WARRIOR);
	m.attr("MOTION_SLAP_HIT_WITH_ASSASSIN") = int32_t(MOTION_SLAP_HIT_WITH_ASSASSIN);
	m.attr("MOTION_SLAP_HIT_WITH_SURA") = int32_t(MOTION_SLAP_HIT_WITH_SURA);
	m.attr("MOTION_SLAP_HIT_WITH_SHAMAN") = int32_t(MOTION_SLAP_HIT_WITH_SHAMAN);
	m.attr("MOTION_SLAP_HURT_WITH_WARRIOR") = int32_t(MOTION_SLAP_HURT_WITH_WARRIOR);
	m.attr("MOTION_SLAP_HURT_WITH_ASSASSIN") = int32_t(MOTION_SLAP_HURT_WITH_ASSASSIN);
	m.attr("MOTION_SLAP_HURT_WITH_SURA") = int32_t(MOTION_SLAP_HURT_WITH_SURA);
	m.attr("MOTION_SLAP_HURT_WITH_SHAMAN") = int32_t(MOTION_SLAP_HURT_WITH_SHAMAN);
	m.attr("MOTION_DIG") = int32_t(MOTION_DIG);
	m.attr("MOTION_MODE_RESERVED") = int32_t(MOTION_MODE_RESERVED);
	m.attr("MOTION_MODE_GENERAL") = int32_t(MOTION_MODE_GENERAL);
	m.attr("MOTION_MODE_ONEHAND_SWORD") = int32_t(MOTION_MODE_ONEHAND_SWORD);
	m.attr("MOTION_MODE_TWOHAND_SWORD") = int32_t(MOTION_MODE_TWOHAND_SWORD);
	m.attr("MOTION_MODE_DUALHAND_SWORD") = int32_t(MOTION_MODE_DUALHAND_SWORD);
	m.attr("MOTION_MODE_BOW") = int32_t(MOTION_MODE_BOW);
	m.attr("MOTION_MODE_FAN") = int32_t(MOTION_MODE_FAN);
	m.attr("MOTION_MODE_BELL") = int32_t(MOTION_MODE_BELL);
	m.attr("MOTION_MODE_FISHING") = int32_t(MOTION_MODE_FISHING);
	m.attr("MOTION_MODE_HORSE") = int32_t(MOTION_MODE_HORSE);
	m.attr("MOTION_MODE_HORSE_ONEHAND_SWORD") = int32_t(MOTION_MODE_HORSE_ONEHAND_SWORD);
	m.attr("MOTION_MODE_HORSE_TWOHAND_SWORD") = int32_t(MOTION_MODE_HORSE_TWOHAND_SWORD);
	m.attr("MOTION_MODE_HORSE_DUALHAND_SWORD") = int32_t(MOTION_MODE_HORSE_DUALHAND_SWORD);
	m.attr("MOTION_MODE_HORSE_BOW") = int32_t(MOTION_MODE_HORSE_BOW);
	m.attr("MOTION_MODE_HORSE_FAN") = int32_t(MOTION_MODE_HORSE_FAN);
	m.attr("MOTION_MODE_HORSE_BELL") = int32_t(MOTION_MODE_HORSE_BELL);
	m.attr("MOTION_MODE_WEDDING_DRESS") = int32_t(MOTION_MODE_WEDDING_DRESS);
	m.attr("DIR_NORTH") = int32_t(CInstanceBase::DIR_NORTH);
	m.attr("DIR_NORTHEAST") = int32_t(CInstanceBase::DIR_NORTHEAST);
	m.attr("DIR_EAST") = int32_t(CInstanceBase::DIR_EAST);
	m.attr("DIR_SOUTHEAST") = int32_t(CInstanceBase::DIR_SOUTHEAST);
	m.attr("DIR_SOUTH") = int32_t(CInstanceBase::DIR_SOUTH);
	m.attr("DIR_SOUTHWEST") = int32_t(CInstanceBase::DIR_SOUTHWEST);
	m.attr("DIR_WEST") = int32_t(CInstanceBase::DIR_WEST);
	m.attr("DIR_NORTHWEST") = int32_t(CInstanceBase::DIR_NORTHWEST);
	m.attr("INSTANCE_TYPE_PLAYER") = int32_t(CActorInstance::TYPE_PC);
	m.attr("INSTANCE_TYPE_NPC") = int32_t(CActorInstance::TYPE_NPC);
	m.attr("INSTANCE_TYPE_ENEMY") = int32_t(CActorInstance::TYPE_ENEMY);
	m.attr("INSTANCE_TYPE_BUILDING") = int32_t(CActorInstance::TYPE_BUILDING);
	m.attr("INSTANCE_TYPE_OBJECT") = int32_t(CActorInstance::TYPE_OBJECT);
	m.attr("PART_WEAPON") = int32_t(PART_WEAPON);
	m.attr("PART_HEAD") = int32_t(PART_HEAD);
	m.attr("PART_WEAPON_LEFT") = int32_t(PART_WEAPON_LEFT);
	m.attr("AFFECT_POISON") = int32_t(CInstanceBase::AFFECT_POISON);
	m.attr("AFFECT_SLOW") = int32_t(CInstanceBase::AFFECT_SLOW);
	m.attr("AFFECT_STUN") = int32_t(CInstanceBase::AFFECT_STUN);
	m.attr("AFFECT_MOV_SPEED_POTION") = int32_t(CInstanceBase::AFFECT_MOV_SPEED_POTION);
	m.attr("AFFECT_ATT_SPEED_POTION") = int32_t(CInstanceBase::AFFECT_ATT_SPEED_POTION);
	m.attr("AFFECT_FISH_MIND") = int32_t(CInstanceBase::AFFECT_FISH_MIND);
	m.attr("AFFECT_JEONGWI") = int32_t(CInstanceBase::AFFECT_JEONGWI);
	m.attr("AFFECT_GEOMGYEONG") = int32_t(CInstanceBase::AFFECT_GEOMGYEONG);
	m.attr("AFFECT_CHEONGEUN") = int32_t(CInstanceBase::AFFECT_CHEONGEUN);
	m.attr("AFFECT_GYEONGGONG") = int32_t(CInstanceBase::AFFECT_GYEONGGONG);
	m.attr("AFFECT_EUNHYEONG") = int32_t(CInstanceBase::AFFECT_EUNHYEONG);
	m.attr("AFFECT_GWIGEOM") = int32_t(CInstanceBase::AFFECT_GWIGEOM);
	m.attr("AFFECT_GONGPO") = int32_t(CInstanceBase::AFFECT_GONGPO);
	m.attr("AFFECT_JUMAGAP") = int32_t(CInstanceBase::AFFECT_JUMAGAP);
	m.attr("AFFECT_HOSIN") = int32_t(CInstanceBase::AFFECT_HOSIN);
	m.attr("AFFECT_BOHO") = int32_t(CInstanceBase::AFFECT_BOHO);
	m.attr("AFFECT_KWAESOK") = int32_t(CInstanceBase::AFFECT_KWAESOK);
	m.attr("AFFECT_HEUKSIN") = int32_t(CInstanceBase::AFFECT_HEUKSIN);
	m.attr("AFFECT_MUYEONG") = int32_t(CInstanceBase::AFFECT_MUYEONG);
	m.attr("AFFECT_GICHEON") = int32_t(CInstanceBase::AFFECT_GICHEON);
	m.attr("AFFECT_JEUNGRYEOK") = int32_t(CInstanceBase::AFFECT_JEUNGRYEOK);
	m.attr("AFFECT_PABEOP") = int32_t(CInstanceBase::AFFECT_PABEOP);
	m.attr("AFFECT_FALLEN_CHEONGEUN") = int32_t(CInstanceBase::AFFECT_FALLEN_CHEONGEUN);
	m.attr("AFFECT_CHINA_FIREWORK") = int32_t(CInstanceBase::AFFECT_CHINA_FIREWORK);
	m.attr("NEW_AFFECT_MALL") = int32_t(CInstanceBase::NEW_AFFECT_MALL);
	m.attr("NEW_AFFECT_NO_DEATH_PENALTY") = int32_t(CInstanceBase::NEW_AFFECT_NO_DEATH_PENALTY);
	m.attr("NEW_AFFECT_SKILL_BOOK_BONUS") = int32_t(CInstanceBase::NEW_AFFECT_SKILL_BOOK_BONUS);
	m.attr("NEW_AFFECT_SKILL_BOOK_NO_DELAY") = int32_t(CInstanceBase::NEW_AFFECT_SKILL_BOOK_NO_DELAY);
	m.attr("NEW_AFFECT_EXP_BONUS") = int32_t(CInstanceBase::NEW_AFFECT_EXP_BONUS);
	m.attr("NEW_AFFECT_EXP_BONUS_EURO_FREE") = int32_t(CInstanceBase::NEW_AFFECT_EXP_BONUS_EURO_FREE);
	m.attr("NEW_AFFECT_EXP_BONUS_EURO_FREE_UNDER_15") = int32_t(CInstanceBase::NEW_AFFECT_EXP_BONUS_EURO_FREE_UNDER_15);
	m.attr("NEW_AFFECT_ITEM_BONUS") = int32_t(CInstanceBase::NEW_AFFECT_ITEM_BONUS);
	m.attr("NEW_AFFECT_SAFEBOX") = int32_t(CInstanceBase::NEW_AFFECT_SAFEBOX);
	m.attr("NEW_AFFECT_AUTOLOOT") = int32_t(CInstanceBase::NEW_AFFECT_AUTOLOOT);
	m.attr("NEW_AFFECT_FISH_MIND") = int32_t(CInstanceBase::NEW_AFFECT_FISH_MIND);
	m.attr("NEW_AFFECT_MARRIAGE_FAST") = int32_t(CInstanceBase::NEW_AFFECT_MARRIAGE_FAST);
	m.attr("NEW_AFFECT_GOLD_BONUS") = int32_t(CInstanceBase::NEW_AFFECT_GOLD_BONUS);
	m.attr("NEW_AFFECT_AUTO_HP_RECOVERY") = int32_t(CInstanceBase::NEW_AFFECT_AUTO_HP_RECOVERY);
	m.attr("NEW_AFFECT_AUTO_SP_RECOVERY") = int32_t(CInstanceBase::NEW_AFFECT_AUTO_SP_RECOVERY);
	m.attr("NEW_AFFECT_DRAGON_SOUL_QUALIFIED") = int32_t(CInstanceBase::NEW_AFFECT_DRAGON_SOUL_QUALIFIED);
	m.attr("NEW_AFFECT_DRAGON_SOUL_DECK1") = int32_t(CInstanceBase::NEW_AFFECT_DRAGON_SOUL_DECK1);
	m.attr("NEW_AFFECT_DRAGON_SOUL_DECK2") = int32_t(CInstanceBase::NEW_AFFECT_DRAGON_SOUL_DECK2);
}
