#include "StdAfx.h"
#include "../../Libraries/EffectLib/EffectManager.h"
#include "PythonCharacterManager.h"

static void effectRegisterEffect(std::string szFileName)
{

	CEffectManager::GetInstance()->RegisterEffect(szFileName.c_str());

}

static int32_t effectCreateEffect(std::string szEffectName)
{

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetSelectedInstancePtr();
	TPixelPosition PixelPosition;
	pInstance->NEW_GetPixelPosition(&PixelPosition);
	return CEffectManager::GetInstance()->CreateEffect(szEffectName.c_str(), PixelPosition, D3DXVECTOR3(0.0f, 0.0f, 0.0f));
}

static void effectDeleteEffect(uint32_t iIndex)
{

	CEffectManager::GetInstance()->DestroyEffectInstance(iIndex);


}

static void effectSetPosition(uint32_t iIndex, float fx, float fy, float fz)
{

	CEffectManager::GetInstance()->SelectEffectInstance(iIndex);
	CEffectManager::GetInstance()->SetEffectInstancePosition(D3DXVECTOR3(fx, fy, fz));


}

static void effectRegisterIndexedFlyData(uint32_t iIndex, uint8_t iType, std::string szFlyDataName)
{

	CFlyingManager::GetInstance()->RegisterIndexedFlyData(iIndex, iType, szFlyDataName.c_str());


}

static void effectUpdate()
{
	CEffectManager::GetInstance()->Update();

}

static void effectRender()
{
	CEffectManager::GetInstance()->Render();

}

PYBIND11_EMBEDDED_MODULE(effect, m)
{
	m.def("RegisterEffect",	effectRegisterEffect);
	m.def("CreateEffect",	effectCreateEffect);
	m.def("DeleteEffect",	effectDeleteEffect);
	m.def("SetPosition",	effectSetPosition);
	m.def("RegisterIndexedFlyData",	effectRegisterIndexedFlyData);
	m.def("Update",	effectUpdate);
	m.def("Render",	effectRender);

	m.attr("INDEX_FLY_TYPE_NORMAL") = int32_t(INDEX_FLY_TYPE_NORMAL);
	m.attr("INDEX_FLY_TYPE_FIRE_CRACKER") = int32_t(INDEX_FLY_TYPE_FIRE_CRACKER);
	m.attr("INDEX_FLY_TYPE_AUTO_FIRE") = int32_t(INDEX_FLY_TYPE_AUTO_FIRE);
	m.attr("FLY_NONE") = int32_t(FLY_NONE);
	m.attr("FLY_EXP") = int32_t(FLY_EXP);
	m.attr("FLY_HP_MEDIUM") = int32_t(FLY_HP_MEDIUM);
	m.attr("FLY_HP_BIG") = int32_t(FLY_HP_BIG);
	m.attr("FLY_SP_SMALL") = int32_t(FLY_SP_SMALL);
	m.attr("FLY_SP_MEDIUM") = int32_t(FLY_SP_MEDIUM);
	m.attr("FLY_SP_BIG") = int32_t(FLY_SP_BIG);
	m.attr("FLY_FIREWORK1") = int32_t(FLY_FIREWORK1);
	m.attr("FLY_FIREWORK2") = int32_t(FLY_FIREWORK2);
	m.attr("FLY_FIREWORK3") = int32_t(FLY_FIREWORK3);
	m.attr("FLY_FIREWORK4") = int32_t(FLY_FIREWORK4);
	m.attr("FLY_FIREWORK5") = int32_t(FLY_FIREWORK5);
	m.attr("FLY_FIREWORK6") = int32_t(FLY_FIREWORK6);
	m.attr("FLY_FIREWORK_XMAS") = int32_t(FLY_FIREWORK_XMAS);
	m.attr("FLY_CHAIN_LIGHTNING") = int32_t(FLY_CHAIN_LIGHTNING);
	m.attr("FLY_HP_SMALL") = int32_t(FLY_HP_SMALL);
	m.attr("FLY_SKILL_MUYEONG") = int32_t(FLY_SKILL_MUYEONG);
}
