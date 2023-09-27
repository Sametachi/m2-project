#include "StdAfx.h"
#include "../../Libraries/gameLib/FlyingObjectManager.h"

static void flyUpdate()
{
	CFlyingManager::GetInstance()->Update();

}

static void flyRender()
{
	CFlyingManager::GetInstance()->Render();

}



PYBIND11_EMBEDDED_MODULE(fly, m)
{
	m.def("Update",	flyUpdate);
	m.def("Render",	flyRender);

}
