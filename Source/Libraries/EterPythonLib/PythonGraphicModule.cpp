#include "StdAfx.h"
#include "../eterlib/Camera.h"
#include "../EterLib/TextBar.h"

#include <shlobj.h>
#include <filesystem>

static BOOST_FORCEINLINE CTextBar* CapsuleGetTextBar(pybind11::capsule& c)
{
	auto ptr = c.get_pointer();
	if (!ptr)
		throw std::runtime_error("Invalid capsule pointer");

	return reinterpret_cast<CTextBar*>(ptr);
}

static void grpInitScreenEffect()
{
	CPythonGraphic::GetInstance()->InitScreenEffect();
}

static void grpCulling()
{
	CCullingManager::GetInstance()->Process();
}

static void grpClearDepthBuffer()
{
	CPythonGraphic::GetInstance()->ClearDepthBuffer();
}

static void grpIdentity()
{
	CPythonGraphic::GetInstance()->Identity();
}

static uint32_t grpGenerateColor(float r, float g, float b, float a)
{
	return uint32_t(CPythonGraphic::GetInstance()->GenerateColor(r, g, b, a));
}

static void grpPopState()
{
	CPythonGraphic::GetInstance()->PopState();
}

static void grpPushState()
{
	CPythonGraphic::GetInstance()->PushState();
}

static void grpPushMatrix()
{
	CPythonGraphic::GetInstance()->PushMatrix();
}

static void grpPopMatrix()
{
	CPythonGraphic::GetInstance()->PopMatrix();
}

static void grpTranslate(float x, float y, float z)
{
	CPythonGraphic::GetInstance()->Translate(x, y, z);
}

static void grpRotate(float Degree, float x, float y, float z)
{
	CPythonGraphic::GetInstance()->Rotate(Degree, x, y, z);
}

static void grpSetColorRenderState()
{

}

static void grpSetAroundCamera(float distance, float pitch, float roll, float lookAtZ)
{
	CPythonGraphic::GetInstance()->SetAroundCamera(distance, pitch, roll, lookAtZ);
}

static void grpSetPositionCamera(float fx, float fy, float fz, float distance, float pitch, float roll)
{
	CPythonGraphic::GetInstance()->SetPositionCamera(fx, fy, fz, distance, pitch, roll);
}

static void grpSetEyeCamera(float xEye, float yEye, float zEye, float xCenter, float yCenter, float zCenter, float xUp, float yUp, float zUp)
{
	CPythonGraphic::GetInstance()->SetEyeCamera(xEye, yEye, zEye, xCenter, yCenter, zCenter, xUp, yUp, zUp);
}

static void grpSetPerspective(float fov, float aspect, float zNear, float zFar)
{
	CPythonGraphic::GetInstance()->SetPerspective(fov, aspect, zNear, zFar);
}

static void grpSetOrtho2D(float width, float height, float length)
{
	CPythonGraphic::GetInstance()->SetOrtho2D(width, height, length);
}

static void grpSetOrtho3D(float width, float height, float zmin, float zmax)
{
	CPythonGraphic::GetInstance()->SetOrtho3D(width, height, zmin, zmax);
}

static void grpSetColor(uint32_t color)
{
	CPythonGraphic::GetInstance()->SetDiffuseColor(color);
}

static void grpSetAlpha(float Alpha)
{
	//CPythonGraphic::GetInstance()->SetAlpha(Alpha);
}

static void grpSetDiffuseColor(float r, float g, float b, float a)
{
	CPythonGraphic::GetInstance()->SetDiffuseColor(r, g, b, a);
}

static void grpSetClearColor(float fr, float fg, float fb)
{
	CPythonGraphic::GetInstance()->SetClearColor(fr, fg, fb, 1.0f);
}

static std::tuple<float, float, float> grpGetCursorPosition3d()
{
	float x, y, z;
	CPythonGraphic::GetInstance()->GetCursorPosition(&x, &y, &z);
	return std::make_tuple(x, y, z);
}

static void grpSetCursorPosition(int32_t ix, int32_t iy)
{
	CPythonGraphic::GetInstance()->SetCursorPosition(ix, iy);
}

static void grpRenderLine(float x, float y, float width, float height)
{
	CPythonGraphic::GetInstance()->RenderLine2d(x, y, x + width, y + height);
}

static void grpRenderBox(float x, float y, float width, float height)
{
	CPythonGraphic::GetInstance()->RenderBox2d(x, y, x + width, y + height);
}

static void grpRenderRoundBox(float fx, float fy, float fWidth, float fHeight)
{
	CPythonGraphic::GetInstance()->RenderLine2d(fx + 2.0f, fy, fx + 2.0f + (fWidth - 3.0f), fy);
	CPythonGraphic::GetInstance()->RenderLine2d(fx + 2.0f, fy + fHeight, fx + 2.0f + (fWidth - 3.0f), fy + fHeight);
	CPythonGraphic::GetInstance()->RenderLine2d(fx, fy + 2.0f, fx, fy + 2.0f + fHeight - 4.0f);
	CPythonGraphic::GetInstance()->RenderLine2d(fx + fWidth, fy + 1.0f, fx + fWidth, fy + 1.0f + fHeight - 3.0f);
	CPythonGraphic::GetInstance()->RenderLine2d(fx, fy + 2.0f, fx + 2.0f, fy);
	CPythonGraphic::GetInstance()->RenderLine2d(fx, fy + fHeight - 2.0f, fx + 2.0f, fy + fHeight);
	CPythonGraphic::GetInstance()->RenderLine2d(fx + fWidth - 2.0f, fy, fx + fWidth, fy + 2.0f);
	CPythonGraphic::GetInstance()->RenderLine2d(fx + fWidth - 2.0f, fy + fHeight, fx + fWidth, fy + fHeight - 2.0f);
}

static void grpRenderBox3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	CPythonGraphic::GetInstance()->RenderBox3d(sx, sy, sz, ex, ey, ez);
}

static void grpRenderBar(float x, float y, float width, float height)
{
	CPythonGraphic::GetInstance()->RenderBar2d(x, y, x + width, y + height);
}

static void grpRenderBar3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	CPythonGraphic::GetInstance()->RenderBar3d(sx, sy, sz, ex, ey, ez);
}

static void grpRenderGradationBar(float x, float y, float width, float height, uint32_t iStartColor, uint32_t iEndColor)
{
	CPythonGraphic::GetInstance()->RenderGradationBar2d(x, y, x + width, y + height, iStartColor, iEndColor);
}

static void grpRenderCube(float sx, float sy, float sz, float ex, float ey, float ez)
{
	CPythonGraphic::GetInstance()->RenderCube(sx, sy, sz, ex, ey, ez);
}

static void grpRenderDownButton(float x, float y, float width, float height)
{
	CPythonGraphic::GetInstance()->RenderDownButton(x, y, x + width, y + height);
}

static void grpRenderUpButton(float x, float y, float width, float height)
{
	CPythonGraphic::GetInstance()->RenderUpButton(x, y, x + width, y + height);
}

static uint32_t grpGetAvailableMemory()
{
	return CPythonGraphic::GetInstance()->GetAvailableMemory();
}

static std::tuple<bool, std::string> grpSaveScreenShot(std::string szBasePath)
{
	time_t ct;

	ct = time(nullptr);
	struct tm* tmNow = localtime(&ct);

	char szPath[MAX_PATH + 256];
	_snprintf_s(szPath, sizeof(szPath), "%s%02d%02d_%02d%02d%02d.jpg", szBasePath.c_str(), tmNow->tm_mon + 1, tmNow->tm_mday, tmNow->tm_hour,
		tmNow->tm_min, tmNow->tm_sec);

	bool bResult = CPythonGraphic::GetInstance()->SaveScreenShot(szPath);
	return std::make_tuple(bResult, szPath);
}

static std::tuple<bool, std::string> grpSaveScreenShotToPath(std::string szBasePath)
{
	time_t ct;

	ct = time(nullptr);
	struct tm* tmNow = localtime(&ct);

	char szPath[MAX_PATH + 256];
	SHGetSpecialFolderPath(nullptr, szPath, CSIDL_PERSONAL, true);
	strcat(szPath, "\\Screenshots\\");

	if (!std::filesystem::exists(szPath))
	{
		if (!CreateDirectory(szPath, nullptr))
		{
			SysLog("Failed to create directory [{0}]\n", szPath);
			return std::make_tuple(false, "");
		}
	}

	auto len = strlen(szPath);

	sprintf_s(szPath + len, sizeof(szPath) - len, "%02d%02d_%02d%02d%02d.jpg",
		tmNow->tm_mon + 1,
		tmNow->tm_mday,
		tmNow->tm_hour,
		tmNow->tm_min,
		tmNow->tm_sec);

	bool bResult = CPythonGraphic::GetInstance()->SaveScreenShot(szPath);
	return std::make_tuple(bResult, szPath);
}

static void grpSetGamma(float gamma)
{
	CPythonGraphic::GetInstance()->SetGamma(gamma);
}

static void grpSetInterfaceRenderState()
{
	CPythonGraphic::GetInstance()->SetInterfaceRenderState();
}

static void grpSetGameRenderState()
{
	CPythonGraphic::GetInstance()->SetGameRenderState();
}

static void grpSetViewport(float fx, float fy, float fWidth, float fHeight)
{
	uint32_t uWidth;
	uint32_t uHeight;
	CPythonGraphic::GetInstance()->GetBackBufferSize(&uWidth, &uHeight);
	CPythonGraphic::GetInstance()->SetViewport(fx * uWidth, fy * uHeight, fWidth * uWidth, fHeight * uHeight);
}

static void grpRestoreViewport()
{
	CPythonGraphic::GetInstance()->RestoreViewport();
}

static void grpSetOmniLight()
{
	CPythonGraphic::GetInstance()->SetOmniLight();
}

static std::tuple<float, float, float> grpGetCameraPosition()
{
	D3DXVECTOR3 v3Eye = CCameraManager::GetInstance()->GetCurrentCamera()->GetEye();
	return std::make_tuple(v3Eye.x, v3Eye.y, v3Eye.z);
}

static std::tuple<float, float, float> grpGetTargetPosition()
{
	D3DXVECTOR3 v3Target = CCameraManager::GetInstance()->GetCurrentCamera()->GetTarget();
	return std::make_tuple(v3Target.x, v3Target.y, v3Target.z);
}

static void CapsuleDestroyer(void* capsule)
{
	CTextBar* winPointer = static_cast<CTextBar*>(capsule);
	delete winPointer;
}

static py::capsule grpCreateTextBar(uint32_t iWidth, uint32_t iHeight)
{
	CTextBar* pTextBar = new CTextBar(12, false);
	if (!pTextBar->Create(nullptr, iWidth, iHeight))
	{
		delete pTextBar;
		return py::capsule();
	}

	auto capsule = pybind11::capsule(pTextBar, CapsuleDestroyer);
	return capsule;
}

static py::capsule grpCreateBigTextBar(uint32_t iWidth, uint32_t iHeight, int32_t iFontSize)
{

	CTextBar* pTextBar = new CTextBar(iFontSize, true);
	if (!pTextBar->Create(nullptr, iWidth, iHeight))
	{
		delete pTextBar;
		return py::capsule();
	}

	auto capsule = pybind11::capsule(pTextBar, CapsuleDestroyer);
	return capsule;
}

static void grpDestroyTextBar(py::capsule c)
{

}

static void grpRenderTextBar(py::capsule c, int32_t ix, int32_t iy)
{
	CTextBar* pTextBar = CapsuleGetTextBar(c);

	if (pTextBar)
		pTextBar->Render(ix, iy);
}

static void grpTextBarTextOut(py::capsule c, int32_t ix, int32_t iy, std::string szText)
{
	CTextBar* pTextBar = CapsuleGetTextBar(c);

	if (pTextBar)
		pTextBar->TextOut(ix, iy, szText.c_str());
}

static void grpTextBarSetTextColor(py::capsule c, int32_t r, int32_t g, int32_t b)
{
	CTextBar* pTextBar = CapsuleGetTextBar(c);

	if (pTextBar)
		pTextBar->SetTextColor(r, g, b);
}

static std::tuple<float, float> grpTextBarGetTextExtent(py::capsule c, std::string szText)
{
	SIZE size = { 0, 0 };
	CTextBar* pTextBar = CapsuleGetTextBar(c);

	if (pTextBar)
		pTextBar->GetTextExtent(szText.c_str(), &size);

	return std::make_tuple(size.cx, size.cy);
}

static void grpClearTextBar(py::capsule c)
{
	CTextBar* pTextBar = CapsuleGetTextBar(c);

	if (pTextBar)
		pTextBar->ClearBar();
}

static void grpSetTextBarClipRect(py::capsule c, LONG isx, LONG isy, LONG iex, LONG iey)
{
	CTextBar* pTextBar = CapsuleGetTextBar(c);

	if (pTextBar)
	{
		RECT rect{};
		rect.left = isx;
		rect.top = isy;
		rect.right = iex;
		rect.bottom = iey;

		pTextBar->SetClipRect(rect);
	}
}

PYBIND11_EMBEDDED_MODULE(grp, m)
{
	m.def("InitScreenEffect", grpInitScreenEffect);
	m.def("Culling", grpCulling);
	m.def("ClearDepthBuffer", grpClearDepthBuffer);
	m.def("Identity", grpIdentity);
	m.def("GenerateColor", grpGenerateColor);
	m.def("PopState", grpPopState);
	m.def("PushState", grpPushState);
	m.def("PushMatrix", grpPushMatrix);
	m.def("PopMatrix", grpPopMatrix);
	m.def("Translate", grpTranslate);
	m.def("Rotate", grpRotate);
	m.def("SetColorRenderState", grpSetColorRenderState);
	m.def("SetAroundCamera", grpSetAroundCamera);
	m.def("SetPositionCamera", grpSetPositionCamera);
	m.def("SetEyeCamera", grpSetEyeCamera);
	m.def("SetPerspective", grpSetPerspective);
	m.def("SetOrtho2d", grpSetOrtho2D);
	m.def("SetOrtho3d", grpSetOrtho3D);
	m.def("SetColor", grpSetColor);
	m.def("SetAlpha", grpSetAlpha);
	m.def("SetDiffuseColor", grpSetDiffuseColor);
	m.def("SetClearColor", grpSetClearColor);
	m.def("GetCursorPosition3d", grpGetCursorPosition3d);
	m.def("SetCursorPosition", grpSetCursorPosition);
	m.def("RenderLine", grpRenderLine);
	m.def("RenderBox", grpRenderBox);
	m.def("RenderRoundBox", grpRenderRoundBox);
	m.def("RenderBox3d", grpRenderBox3d);
	m.def("RenderBar", grpRenderBar);
	m.def("RenderBar3d", grpRenderBar3d);
	m.def("RenderGradationBar", grpRenderGradationBar);
	m.def("RenderCube", grpRenderCube);
	m.def("RenderDownButton", grpRenderDownButton);
	m.def("RenderUpButton", grpRenderUpButton);
	m.def("GetAvailableMemory", grpGetAvailableMemory);
	m.def("SaveScreenShot", grpSaveScreenShot);
	m.def("SaveScreenShotToPath", grpSaveScreenShotToPath);
	m.def("SetGamma", grpSetGamma);
	m.def("SetInterfaceRenderState", grpSetInterfaceRenderState);
	m.def("SetGameRenderState", grpSetGameRenderState);
	m.def("SetViewport", grpSetViewport);
	m.def("RestoreViewport", grpRestoreViewport);
	m.def("SetOmniLight", grpSetOmniLight);
	m.def("GetCameraPosition", grpGetCameraPosition);
	m.def("GetTargetPosition", grpGetTargetPosition);
	m.def("CreateTextBar", grpCreateTextBar);
	m.def("CreateBigTextBar", grpCreateBigTextBar);
	m.def("DestroyTextBar", grpDestroyTextBar);
	m.def("RenderTextBar", grpRenderTextBar);
	m.def("TextBarTextOut", grpTextBarTextOut);
	m.def("TextBarSetTextColor", grpTextBarSetTextColor);
	m.def("TextBarGetTextExtent", grpTextBarGetTextExtent);
	m.def("ClearTextBar", grpClearTextBar);
	m.def("SetTextBarClipRect", grpSetTextBarClipRect);
}
