#include "StdAfx.h"
#include "../eterBase/Stl.h"
#include "GrpSubImage.h"
#include "ResourceManager.h"
#include <Basic/GroupTextTree.h>

char CGraphicSubImage::m_SearchPath[256] = "D:/Ymir Work/UI/";

CGraphicSubImage::TType CGraphicSubImage::Type()
{
	static TType s_type = StringToType("CGraphicSubImage");
	return s_type;
}

CGraphicSubImage::CGraphicSubImage(const FilenameWrapper& filename) : CGraphicImage(filename)
{
}

bool CGraphicSubImage::CreateDeviceObjects()
{
	m_imageTexture.CreateFromTexturePointer(m_roImage->GetTexturePointer());
	return true;
}

void CGraphicSubImage::SetImagePointer(CGraphicImage* pImage)
{
	m_roImage = pImage;
	CreateDeviceObjects();
}

bool CGraphicSubImage::SetImageFileName(const char* c_szFileName)
{
	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CResource>(c_szFileName);

	if (!pResource->IsType(CGraphicImage::Type()))
		return false;

	SetImagePointer(static_cast<CGraphicImage*>(pResource));
	return true;
}

void CGraphicSubImage::SetRectPosition(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	m_rect.left = left;
	m_rect.top = top;
	m_rect.right = right;
	m_rect.bottom = bottom;
}

void CGraphicSubImage::SetRectReference(const RECT& c_rRect)
{
	m_rect = c_rRect;
}

void CGraphicSubImage::SetSearchPath(const char* c_szFileName)
{
	strncpy_s(m_SearchPath, c_szFileName, sizeof(m_SearchPath) - 1);
}

bool CGraphicSubImage::OnLoad(int32_t iSize, const void* c_pvBuf)
{
	if (!c_pvBuf)
		return false;

	GroupTextMemory mem;
	GroupTextReader reader(&mem);

	boost::string_view data(static_cast<const char*> (c_pvBuf), iSize);

	if (!reader.LoadString(data.to_string()))
	{
		return false;
	}

	if (reader.GetProperty("title") != "subImage")
	{
#ifdef USE_GROUPTEXTTREE_DEBUG
		SysLog("Title is not subImage while loading {0}", CResource::GetFileNameString().c_str());
#endif
		return false;
	}

	std::string fileName;
	if (reader.GetProperty("version") == "3.0")
	{
		fileName = "Assets/Design/";
		fileName.append(reader.GetProperty("image").data(), reader.GetProperty("image").length());
	}
	else if (reader.GetProperty("version") == "2.0")
	{
		int nPos = CResource::GetFileNameString().find_last_of('/');
		if (nPos < 0)
		{
			fileName.append(reader.GetProperty("image").data(), reader.GetProperty("image").length());
		}
		else
		{
			fileName.append(CResource::GetFileNameString(), 0, nPos + 1);
			fileName.append(reader.GetProperty("image").data(), reader.GetProperty("image").length());
		}
	}
	else
	{
		fileName = "D:/Ymir Work/UI/";
		fileName.append(reader.GetProperty("image").data(), reader.GetProperty("image").length());
	}

	assert(fileName != CResource::GetFileNameString());

	SetImageFileName(fileName.c_str());

	if (m_roImage.IsNull())
	{
		SysLog("Failed to load {0} while loading {1}", fileName.c_str(), GetFileName());
		return false;
	}

	m_rect.left = std::stoi(reader.GetProperty("left"));
	m_rect.right = std::stoi(reader.GetProperty("right"));
	m_rect.top = std::stoi(reader.GetProperty("top"));
	m_rect.bottom = std::stoi(reader.GetProperty("bottom"));

#ifdef USE_GROUPTEXTTREE_DEBUG
	ConsoleLog("Sucessfully loaded {0}", CResource::GetFileNameString());
#endif
	return true;
}

void CGraphicSubImage::OnClear()
{
	m_roImage = nullptr;
	std::memset(&m_rect, 0, sizeof(m_rect));
}

bool CGraphicSubImage::OnIsEmpty() const
{
	if (!m_roImage.IsNull())
		if (!m_roImage->IsEmpty())
			return false;

	return true;
}

bool CGraphicSubImage::OnIsType(TType type)
{
	if (CGraphicSubImage::Type() == type)
		return true;

	return CGraphicImage::OnIsType(type);
}