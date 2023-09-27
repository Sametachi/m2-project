#include "StdAfx.h"
#include "../EterLib/ResourceManager.h"

void ImageCapsuleDestroyer(void* capsule)
{
	delete capsule;
}

static CGraphicImageInstance* CapsuleGetImage(pybind11::capsule& c)
{
	const auto ptr = static_cast<CGraphicImageInstance*>(c.get_pointer());
	if (!ptr)
		throw std::runtime_error("Invalid capsule pointer");

	return ptr;
}

static CGraphicExpandedImageInstance* CapsuleGetImageEx(pybind11::capsule& c)
{
	const auto ptr = static_cast<CGraphicExpandedImageInstance*>(c.get_pointer());
	if (!ptr)
		throw std::runtime_error("Invalid capsule pointer");

	return ptr;
}

static void grpImageRender(pybind11::capsule handle)
{
	CGraphicImageInstance* pImageInstance = CapsuleGetImage(handle);
	pImageInstance->Render();
}

static void grpSetImagePosition(pybind11::capsule handle, float x, float y)
{
	CGraphicImageInstance* pImageInstance = CapsuleGetImage(handle);
	pImageInstance->SetPosition(x, y);
}

static pybind11::capsule grpImageGenerate(std::string szFileName)
{
	if (szFileName.empty())
		return pybind11::capsule();

	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(szFileName.c_str());

	if (!pResource->IsType(CGraphicImage::Type()))
		throw std::runtime_error("Resource is not an image (filename: " + szFileName + ")");

	CGraphicImageInstance* pImageInstance = CGraphicImageInstance::New();
	pImageInstance->SetImagePointer(static_cast<CGraphicImage*>(pResource));

	if (pImageInstance->IsEmpty())
		throw std::runtime_error("Cannot load image (filename: " + szFileName + ")");

	return pybind11::capsule(pImageInstance, [](void* a) {delete a; });
}

static pybind11::capsule grpImageGenerateExpanded(std::string szFileName)
{
	if (szFileName.empty())
		return pybind11::capsule();

	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(szFileName.c_str());

	if (!pResource->IsType(CGraphicImage::Type()))
		throw std::runtime_error("Resource is not an image (filename: " + szFileName + ")");

	CGraphicExpandedImageInstance* pImageInstance = new CGraphicExpandedImageInstance();
	pImageInstance->SetImagePointer(static_cast<CGraphicImage*>(pResource));

	if (pImageInstance->IsEmpty())
		throw std::runtime_error("Cannot load image (filename: " + szFileName + ")");

	return pybind11::capsule(pImageInstance, [](void* a) {delete a; });
}

static pybind11::capsule grpImageGenerateFromHandle(ptrdiff_t iHandle)
{
	CGraphicImageInstance* pImageInstance = new CGraphicExpandedImageInstance();
	pImageInstance->SetImagePointer((CGraphicImage*)iHandle);
	return pybind11::capsule(pImageInstance, [](void* a) {delete a; });
}

// capsule takes care of himself, no need to call this
static void grpImageDelete(pybind11::capsule handle) {}
static void grpImageDeleteExpanded(pybind11::capsule handle) {}

static void grpImageSetFileName(pybind11::capsule handle, std::string szFileName)
{
	CGraphicImageInstance* pImageInstance = CapsuleGetImage(handle);
	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(szFileName.c_str());

	if (!pResource->IsType(CGraphicImage::Type()))
		throw std::runtime_error("Resource is not an image (filename: " + szFileName + ")");

	pImageInstance->SetImagePointer(static_cast<CGraphicImage*>(pResource));
}

static void grpSetImageOrigin(pybind11::capsule handle, float x, float y)
{
	CGraphicExpandedImageInstance* pImageInstance = CapsuleGetImageEx(handle);
	pImageInstance->SetOrigin(x, y);
}

static void grpSetImageRotation(pybind11::capsule handle, float Degree)
{
	CGraphicExpandedImageInstance* pExpandedImageInstance = CapsuleGetImageEx(handle);
	pExpandedImageInstance->SetRotation(Degree);
}

static void grpSetImageScale(pybind11::capsule handle, float x, float y)
{
	CGraphicExpandedImageInstance* pImageInstance = CapsuleGetImageEx(handle);
	pImageInstance->SetScale(x, y);
}

static void grpSetRenderingRect(pybind11::capsule handle, float fLeft, float fTop, float fRight, float fBottom)
{
	CGraphicExpandedImageInstance* pImageInstance = CapsuleGetImageEx(handle);
	pImageInstance->SetRenderingRect(fLeft, fTop, fRight, fBottom);
}

static void grpSetImageDiffuseColor(pybind11::capsule handle, float r, float g, float b, float a)
{
	CGraphicImageInstance* pImageInstance = CapsuleGetImage(handle);
	pImageInstance->SetDiffuseColor(r, g, b, a);
}

static int grpGetWidth(pybind11::capsule handle)
{
	CGraphicImageInstance* pImageInstance = CapsuleGetImage(handle);
	if (pImageInstance->IsEmpty())
		throw std::runtime_error("Image is empty");

	return pImageInstance->GetWidth();
}

static int grpGetHeight(pybind11::capsule handle)
{
	CGraphicImageInstance* pImageInstance = CapsuleGetImage(handle);
	if (pImageInstance->IsEmpty())
		throw std::runtime_error("Image is empty");

	return pImageInstance->GetHeight();
}

PYBIND11_EMBEDDED_MODULE(grpImage, m)
{
	m.def("Render", grpImageRender);
	m.def("SetPosition", grpSetImagePosition);
	m.def("Generate", grpImageGenerate);
	m.def("GenerateExpanded", grpImageGenerateExpanded);
	m.def("GenerateFromHandle", grpImageGenerateFromHandle);
	m.def("Delete", grpImageDelete);
	m.def("DeleteExpanded", grpImageDeleteExpanded);
	m.def("SetFileName", grpImageSetFileName);
	m.def("SetOrigin", grpSetImageOrigin);
	m.def("SetRotation", grpSetImageRotation);
	m.def("SetScale", grpSetImageScale);
	m.def("SetRenderingRect", grpSetRenderingRect);
	m.def("SetDiffuseColor", grpSetImageDiffuseColor);
	m.def("GetWidth", grpGetWidth);
	m.def("GetHeight", grpGetHeight);
}
