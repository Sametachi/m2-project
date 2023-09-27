#include "StdAfx.h"
#include "GrpDIB.h"
#include <boost/system/error_code.hpp>
#include <Storm/UnicodeUtil.hpp>

CGraphicDib::CGraphicDib()
{
	Initialize();
}

CGraphicDib::~CGraphicDib()
{
	Destroy();
}

void CGraphicDib::Initialize()
{
	m_hDC = nullptr;
	m_hBmp = nullptr;
	m_pvBuf = nullptr;
	m_width = 0;
	m_height = 0;
}

void CGraphicDib::Destroy()
{
	if (m_hBmp) DeleteObject(m_hBmp);
	if (m_hDC)	DeleteDC(m_hDC);

	Initialize();
}

bool CGraphicDib::Create(HDC hDC, int32_t width, int32_t height)
{
	Destroy();

	m_width = width;
	m_height = height;

	ZeroMemory(&m_bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bmi.bmiHeader.biWidth = m_width;
	m_bmi.bmiHeader.biHeight = -m_height;
	m_bmi.bmiHeader.biPlanes = 1;
	m_bmi.bmiHeader.biBitCount = 32;
	m_bmi.bmiHeader.biCompression = BI_RGB;

	m_hDC = CreateCompatibleDC(hDC);
	if (!m_hDC)
	{
		assert(!"CGraphicDib::Create CreateCompatibleDC Error");
		return false;
	}

	m_hBmp = CreateDIBSection(m_hDC, &m_bmi, DIB_RGB_COLORS, &m_pvBuf, nullptr, 0);
	if (!m_hBmp)
	{
		assert(!"CGraphicDib::Create CreateDIBSection Error");
		return false;
	}

	SelectObject(m_hDC, m_hBmp);

	::SetTextColor(m_hDC, RGB(255, 255, 255));

	return true;
}

HDC	CGraphicDib::GetDCHandle()
{
	return m_hDC;
}

void CGraphicDib::SetBkMode(int32_t iBkMode)
{
	::SetBkMode(m_hDC, iBkMode);
}

void CGraphicDib::TextOuts(int32_t ix, int32_t iy, const char* c_szText)
{
	::SetBkColor(m_hDC, 0);

	std::string src(c_szText);
	wchar_t buffer[4096];

	boost::system::error_code error_code;
	auto r = storm::ConvertUtf8ToUtf16(src.data(), src.data() + src.length(), buffer, buffer + 4096, error_code);
	if (error_code)
	{
		SysLog("Failed to convert {0} to UTF-8 with {1}", src, error_code.message());
		::TextOutA(m_hDC, ix, iy, "<INVALID>", 10);
		return;
	}

	::TextOutW(m_hDC, ix, iy, buffer, r);
}

void CGraphicDib::Put(HDC hDC, int32_t x, int32_t y)
{
	SetDIBitsToDevice(
		hDC,
		x,
		y,
		m_width,
		m_height,
		0,
		0,
		0,
		m_height,
		m_pvBuf,
		&m_bmi,
		DIB_RGB_COLORS
	);
}

void* CGraphicDib::GetPointer()
{
	return m_pvBuf;
}

int32_t CGraphicDib::GetWidth()
{
	return m_width;
}

int32_t CGraphicDib::GetHeight()
{
	return m_height;
}