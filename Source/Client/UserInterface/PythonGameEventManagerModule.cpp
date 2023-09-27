#include "StdAfx.h"
#include "../../Libraries/gameLib/GameEventManager.h"

static void eventmgrUpdate(float fx, float fy, float fz)
{

	CGameEventManager::GetInstance()->SetCenterPosition(fx, fy, fz);
	CGameEventManager::GetInstance()->Update();

}



PYBIND11_EMBEDDED_MODULE(eventMgr, m)
{
	m.def("Update",	eventmgrUpdate);

}
