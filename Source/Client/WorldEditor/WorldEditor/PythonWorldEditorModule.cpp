#include "StdAfx.h"
#include "WorldEditor.h"
#include "WorldEditorDoc.h"
#include "WorldEditorView.h"
#include "MainFrm.h"
#include "Scene/SceneMap.h"

#include "../../../Libraries/eterlib/Camera.h"
#include "DataCtrl/MapAccessorOutdoor.h"

CWorldEditorApp* gWorldEditorApp{nullptr};
void RegisterWorldEditorApp(CWorldEditorApp* refWorldEditorApp)
{
	gWorldEditorApp = refWorldEditorApp;
}

CWorldEditorApp* GetWorldEditorApp()
{
	return gWorldEditorApp;
}

/////////////////////////////////
PyObject* weIsMapReady(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	CMapManagerAccessor* pMapManagerAccessor = w->GetMapManagerAccessor();
	return Py_BuildValue("i", pMapManagerAccessor->IsMapReady());
}

PyObject* weGetMapType(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	CMapManagerAccessor* pMapManagerAccessor = w->GetMapManagerAccessor();
	CMapOutdoor& rMapOutdoor = pMapManagerAccessor->GetMapOutdoorRef();
	return Py_BuildValue("i", rMapOutdoor.GetType());
}

PyObject* weGetSceneType(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	CWorldEditorDoc* pDocument = w->GetDocument();

	if (!pDocument)
		return Py_BuildValue("i", SCENE_MAX);

	return Py_BuildValue("i", GetSceneTypeFromID(pDocument->GetActiveMode()));
}

PyObject* weGetBaseXY(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	CMapManagerAccessor* pMapManagerAccessor = w->GetMapManagerAccessor();
	CMapOutdoor& rMapOutdoor = pMapManagerAccessor->GetMapOutdoorRef();
	uint32_t dwBaseX, dwBaseY;
	rMapOutdoor.GetBaseXY(&dwBaseX, &dwBaseY);
	return Py_BuildValue("(ii)", dwBaseX, dwBaseY);
}

PyObject* weGetTerrainCount(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	CMapManagerAccessor* pMapManagerAccessor = w->GetMapManagerAccessor();
	CMapOutdoor& rMapOutdoor = pMapManagerAccessor->GetMapOutdoorRef();
	int16_t sCountX, sCountY;
	rMapOutdoor.GetTerrainCount(&sCountX, &sCountY);
	return Py_BuildValue("(ii)", sCountX, sCountY);
}

PyObject* weGetEnvironmentDataName(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	CMapManagerAccessor* pMapManagerAccessor = w->GetMapManagerAccessor();
	CMapOutdoor& rMapOutdoor = pMapManagerAccessor->GetMapOutdoorRef();
	return Py_BuildValue("s", rMapOutdoor.GetEnvironmentDataName().c_str());
}

PyObject* weGetPropertyType(PyObject* poSelf, PyObject* poArgs)
{
	int32_t ext;
	int32_t idx{};
	if (!PyTuple_GetLong(poArgs, idx++, &ext))
		return Py_BuildException();

	return Py_BuildValue("s", prt::GetPropertyExtension(ext));
}

PyObject* weGetPropertyExtension(PyObject* poSelf, PyObject* poArgs)
{
	char* type;
	int32_t idx{};
	if (!PyTuple_GetString(poArgs, idx++, &type))
		return Py_BuildException();

	return Py_BuildValue("i", prt::GetPropertyType(type));
}

PyObject* weGetObjectList(PyObject* poSelf, PyObject* poArgs)
{
	//int32_t wTerrainNumX, wTerrainNumY;
	//int32_t idx{};
	//if (!PyTuple_GetLong(poArgs, idx++, &wTerrainNumX))
	//	return Py_BuildException();
	//if (!PyTuple_GetLong(poArgs, idx++, &wTerrainNumY))
	//	return Py_BuildException();

	//auto w = GetWorldEditorApp();
	//CMapManagerAccessor* pMapManagerAccessor = w->GetMapManagerAccessor();
	//CMapOutdoor& rMapOutdoor = pMapManagerAccessor->GetMapOutdoorRef();

	//uint8_t ucTerrainNum;
	//if (!rMapOutdoor.GetTerrainNumFromCoord(wTerrainNumX, wTerrainNumY, &ucTerrainNum))
	//	return Py_BuildValue("()");

	//CTerrainAccessor* pTerrainAccessor = NULL;
	//if (!rMapOutdoor.GetTerrainPointer(ucTerrainNum, (CTerrain**)&pTerrainAccessor))
	//	return Py_BuildValue("()");


	//auto mapManagerAccessor = w->GetMapManagerAccessor();
	//CAreaAccessor* pAreaAccessor;
	//if (!mapManagerAccessor->GetMapAccessor()->GetAreaAccessor(4, &pAreaAccessor))
	//	return Py_BuildValue("()");


	//for (uint32_t i = 0; i < pAreaAccessor->GetObjectDataCount(); ++i)
	//{
	//	const CArea::TObjectData* c_pObjectData;
	//	if (!pAreaAccessor->GetObjectDataPointer(i, &c_pObjectData))
	//		continue;
	//	CProperty* pProperty;
	//	if (!CPropertyManager::Instance().Get(c_pObjectData->dwCRC, &pProperty))
	//		continue;
	//	printf("    %f %f %f\n", c_pObjectData->Position.x, c_pObjectData->Position.y, c_pObjectData->Position.z);
	//	printf("    %u\n", c_pObjectData->dwCRC);
	//	printf("    %f#%f#%f\n", c_pObjectData->m_fYaw, c_pObjectData->m_fPitch, c_pObjectData->m_fRoll);
	//	printf("    %f\n", c_pObjectData->m_fHeightBias);
	//}
	//printf("Total Object data count %d\n", pAreaAccessor->GetObjectDataCount());
	return Py_BuildValue("()");
}

PyObject* weGetTargetPosition(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	CWorldEditorView* pView = w->GetView();

	D3DXVECTOR3 v3Target = CCameraManager::Instance().GetCurrentCamera()->GetTarget();
	return Py_BuildValue("(ff)", v3Target.x, v3Target.y);
}

PyObject* weUpdateTargetPosition(PyObject* poSelf, PyObject* poArgs)
{
	int32_t lX, lY;
	int32_t idx{};
	if (!PyTuple_GetLong(poArgs, idx++, &lX))
		return Py_BuildException();
	if (!PyTuple_GetLong(poArgs, idx++, &lY))
		return Py_BuildException();

	auto w = GetWorldEditorApp();
	CWorldEditorView* pView = w->GetView();
	pView->UpdateTargetPosition(lX, lY);

	return Py_BuildNone();
}

PyObject* weGetHeightPixel(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	auto mapManagerAccessor = w->GetMapManagerAccessor();
	if (!mapManagerAccessor)
		return Py_BuildNone();
	auto mapOutdoorAccessor = mapManagerAccessor->GetMapOutdoorPtr();
	if (!mapOutdoorAccessor)
		return Py_BuildNone();

	int32_t lX, lY;
	int32_t idx{};
	if (!PyTuple_GetLong(poArgs, idx++, &lX))
		return Py_BuildException();
	if (!PyTuple_GetLong(poArgs, idx++, &lY))
		return Py_BuildException();

	auto terrainNumX = lX / TERRAIN_SIZE;
	auto terrainNumY = lY / TERRAIN_SIZE;
	lX = lX % TERRAIN_SIZE;
	lY = lY % TERRAIN_SIZE;

	auto lHeight = mapOutdoorAccessor->GetHeightPixel(terrainNumX, terrainNumY, lX, lY);
	return Py_BuildValue("i", lHeight);
}

PyObject* weDrawHeightPixel(PyObject* poSelf, PyObject* poArgs)
{
	auto w = GetWorldEditorApp();
	auto mapManagerAccessor = w->GetMapManagerAccessor();
	if (!mapManagerAccessor)
		return Py_BuildNone();
	auto mapOutdoorAccessor = mapManagerAccessor->GetMapOutdoorPtr();
	if (!mapOutdoorAccessor)
		return Py_BuildNone();

	int32_t lX, lY, lHeight;
	int32_t idx{};
	if (!PyTuple_GetLong(poArgs, idx++, &lX))
		return Py_BuildException();
	if (!PyTuple_GetLong(poArgs, idx++, &lY))
		return Py_BuildException();
	if (!PyTuple_GetLong(poArgs, idx++, &lHeight))
		return Py_BuildException();

	auto terrainNumX = lX / TERRAIN_SIZE;
	auto terrainNumY = lY / TERRAIN_SIZE;
	lX = lX % TERRAIN_SIZE;
	lY = lY % TERRAIN_SIZE;
	mapOutdoorAccessor->DrawHeightPixel(lHeight, terrainNumX, terrainNumY, lX, lY);
	return Py_BuildNone();
}

static PyMethodDef WorldEditorMethods[] =
{
	// map general
	{"IsMapReady", weIsMapReady, METH_VARARGS},
	{"GetSceneType", weGetSceneType, METH_VARARGS},
	{"GetBaseXY", weGetBaseXY, METH_VARARGS},
	{"GetTerrainCount", weGetTerrainCount, METH_VARARGS},
	{"GetMapType", weGetMapType, METH_VARARGS},
	{"GetEnvironmentDataName", weGetEnvironmentDataName, METH_VARARGS},

	// property
	{"GetPropertyType", weGetPropertyType, METH_VARARGS},
	{"GetPropertyExtension", weGetPropertyExtension, METH_VARARGS},
	{"GetObjectList", weGetObjectList, METH_VARARGS},

	// camera
	{"GetTargetPosition", weGetTargetPosition, METH_VARARGS},
	{"UpdateTargetPosition", weUpdateTargetPosition, METH_VARARGS},

	// map draw
	{"GetHeightPixel", weGetHeightPixel, METH_VARARGS},
	{"DrawHeightPixel", weDrawHeightPixel, METH_VARARGS},

	{NULL, NULL},
};
WCPY_GENERATE_PYTHON3_MODULE(WorldEditor);

void initWorldEditor()
{
	WCPY_INIT_PYTHON_MODULE(WorldEditor);

}

void defWorldEditor()
{
	auto m = wcpy::App::GetModule("WorldEditor");

	wcpy::App::AddInt(m, "SCENE_MAP", SCENE_MAP);
	wcpy::App::AddInt(m, "SCENE_OBJECT", SCENE_OBJECT);
	wcpy::App::AddInt(m, "SCENE_EFFECT", SCENE_EFFECT);
	wcpy::App::AddInt(m, "SCENE_FLY", SCENE_FLY);
	wcpy::App::AddInt(m, "SCENE_MAX", SCENE_MAX);

	wcpy::App::AddInt(m, "MAPTYPE_INVALID", CMapBase::MAPTYPE_INVALID);
	wcpy::App::AddInt(m, "MAPTYPE_INDOOR", CMapBase::MAPTYPE_INDOOR);
	wcpy::App::AddInt(m, "MAPTYPE_OUTDOOR", CMapBase::MAPTYPE_OUTDOOR);

	wcpy::App::AddInt(m, "PROPERTY_TYPE_NONE", prt::EPropertyType::PROPERTY_TYPE_NONE);
	wcpy::App::AddInt(m, "PROPERTY_TYPE_TREE", prt::EPropertyType::PROPERTY_TYPE_TREE);
	wcpy::App::AddInt(m, "PROPERTY_TYPE_BUILDING", prt::EPropertyType::PROPERTY_TYPE_BUILDING);
	wcpy::App::AddInt(m, "PROPERTY_TYPE_EFFECT", prt::EPropertyType::PROPERTY_TYPE_EFFECT);
	wcpy::App::AddInt(m, "PROPERTY_TYPE_AMBIENCE", prt::EPropertyType::PROPERTY_TYPE_AMBIENCE);
	wcpy::App::AddInt(m, "PROPERTY_TYPE_DUNGEON_BLOCK", prt::EPropertyType::PROPERTY_TYPE_DUNGEON_BLOCK);
	wcpy::App::AddInt(m, "PROPERTY_TYPE_MAX_NUM", prt::EPropertyType::PROPERTY_TYPE_MAX_NUM);
}
