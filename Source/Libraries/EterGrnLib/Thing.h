#pragma once
#include <EterLib/Ref.h>
#include <EterLib/Resource.h>

using granny_file = struct granny_file;
using granny_file_info = struct granny_file_info;
using granny_animation = struct granny_animation;

class CGrannyModel;
class CGrannyMotion;
class CGraphicThing : public CResource
{
public:
	using TRef = CRef<CGraphicThing>;
	static TType Type();

	CGraphicThing(const FilenameWrapper& filename);
	virtual ~CGraphicThing();
	virtual bool CreateDeviceObjects();
	virtual void DestroyDeviceObjects();

	bool CheckModelIndex(int32_t iModel) const;
	CGrannyModel* GetModelPointer(int32_t iModel);
	int32_t	GetModelCount() const;

	bool CheckMotionIndex(int32_t iMotion) const;
	std::shared_ptr<CGrannyMotion> GetMotionPointer(int32_t iMotion);
	int32_t GetMotionCount() const;

	int32_t GetTextureCount() const;
	const char* GetTexturePath(int32_t iTexture);

protected:
	void Initialize();
	bool LoadModels();
	bool LoadMotions();

	granny_file* m_pgrnFile;
	granny_file_info* m_pgrnFileInfo;

	granny_animation* m_pgrnAni;

	CGrannyModel* m_models;
	std::vector<std::shared_ptr<CGrannyMotion>> m_motions;

/* Reference related and Resource Manager */
	bool OnLoad(int32_t iSize, const void* c_pvBuf);
	void OnClear();
	bool OnIsEmpty() const;
	bool OnIsType(TType type);
};
