#include "StdAfx.h"
#include "Thing.h"
#include "ThingInstance.h"
#include "Model.h"
#include "Motion.h"
#include "Granny3D.h"

CGraphicThing::CGraphicThing(const FilenameWrapper& filename) : CResource(filename)
{
	Initialize();	
}

CGraphicThing::~CGraphicThing()
{
	Clear();
}

void CGraphicThing::Initialize()
{
	m_pgrnFile = nullptr;
	m_pgrnFileInfo = nullptr;
	m_pgrnAni = nullptr;

	m_models = nullptr;
}

void CGraphicThing::OnClear()
{
	if (!m_motions.empty())
		m_motions.clear();

	if (m_models)
		delete[] m_models;

	if (m_pgrnFile)
		GrannyFreeFile(m_pgrnFile);

	Initialize();
}

CGraphicThing::TType CGraphicThing::Type()
{
	static TType s_type = StringToType("CGraphicThing");
	return s_type;
}

bool CGraphicThing::OnIsEmpty() const
{
	return m_pgrnFile ? false : true;
}

bool CGraphicThing::OnIsType(TType type)
{
	if (Type() == type)
		return true;

	return CResource::OnIsType(type);
}

bool CGraphicThing::CreateDeviceObjects()
{
	if (!m_pgrnFileInfo)
		return true;
	
	for (int32_t m = 0; m < m_pgrnFileInfo->ModelCount; ++m)
	{
		CGrannyModel & rModel = m_models[m];
		rModel.CreateDeviceObjects();
	}

	return true;
}

void CGraphicThing::DestroyDeviceObjects()
{
	if (!m_pgrnFileInfo)
		return;

	for (int32_t m = 0; m < m_pgrnFileInfo->ModelCount; ++m)
	{
		CGrannyModel & rModel = m_models[m];
		rModel.DestroyDeviceObjects();
	}
}

bool CGraphicThing::CheckModelIndex(int32_t iModel) const
{
	if (!m_pgrnFileInfo)
	{
		ConsoleLog("m_pgrnFileInfo == nullptr: {0}\n", GetFileNameString());
		return false;
	}

	assert(m_pgrnFileInfo != nullptr);

	if (iModel < 0)
		return false;

	if (iModel >= m_pgrnFileInfo->ModelCount)
		return false;

	return true;
}

bool CGraphicThing::CheckMotionIndex(int32_t iMotion) const
{
	if (!m_pgrnFileInfo)
		return false;

	assert(m_pgrnFileInfo != nullptr);

	if (iMotion < 0)
		return false;
	
	if (iMotion >= m_pgrnFileInfo->AnimationCount)
		return false;

	return true;
}

CGrannyModel * CGraphicThing::GetModelPointer(int32_t iModel)
{	
	assert(CheckModelIndex(iModel));
	assert(m_models != nullptr);
	return m_models + iModel;
}

std::shared_ptr<CGrannyMotion> CGraphicThing::GetMotionPointer(int32_t iMotion)
{
	if (iMotion >= m_pgrnFileInfo->AnimationCount)
		return nullptr;

	if (m_motions.empty())
		return nullptr;

	return m_motions.at(iMotion);
}

int32_t CGraphicThing::GetTextureCount() const
{
	if (!m_pgrnFileInfo)
		return 0;

	if (m_pgrnFileInfo->TextureCount <= 0)
		return 0;

	return (m_pgrnFileInfo->TextureCount);
}

const char* CGraphicThing::GetTexturePath(int32_t iTexture)
{
	if (iTexture >= GetTextureCount())
		return "";

	return m_pgrnFileInfo->Textures[iTexture]->FromFileName;
}

int32_t CGraphicThing::GetModelCount() const
{
	if (!m_pgrnFileInfo)
		return 0;

	return (m_pgrnFileInfo->ModelCount);
}

int32_t CGraphicThing::GetMotionCount() const
{
	if (!m_pgrnFileInfo)
		return 0;

	return (m_pgrnFileInfo->AnimationCount);
}

bool CGraphicThing::OnLoad(int32_t iSize, const void * c_pvBuf)
{
	if (!c_pvBuf)
		return false;

	m_pgrnFile = GrannyReadEntireFileFromMemory(iSize, (void*)c_pvBuf);

	if (!m_pgrnFile)
		return false;

	auto info = Granny3D::GetInstance()->GetMyFileInfo(m_pgrnFile);
	if (!info)
		return false;

	if (!info->FileInfo || info->Sentinel != Granny3D::GetInstance()->kSentinel)
	{
		// If the gr2 file's format is old, we need to convert again. Since we can hold only one conversion buffer
		// (and GrannyGetFileInfo just gives us the already existing buffer back if one already exists), we need to free our current buffer here.
		if (m_pgrnFile->ConversionBuffer) 
		{
			GrannyFreeBuilderResult(m_pgrnFile->ConversionBuffer);
			m_pgrnFile->ConversionBuffer = nullptr;
		}

		m_pgrnFileInfo = GrannyGetFileInfo(m_pgrnFile);
		if (!m_pgrnFileInfo)
			return false;
	}
	else 
	{
		m_pgrnFileInfo = info->FileInfo;
	}

	//m_pgrnFileInfo = GrannyGetFileInfo(m_pgrnFile);

	if (!m_pgrnFileInfo)
	{
		SysLog("Failed to read granny file info");
		return false;
	}

	LoadModels();
	LoadMotions();
	return true;
}

static std::string gs_modelLocalPath;
const std::string& GetModelLocalPath()
{
	return gs_modelLocalPath;
}

bool CGraphicThing::LoadModels()
{
	assert(m_pgrnFile != nullptr);
	assert(m_models == nullptr);
	
	if (m_pgrnFileInfo->ModelCount <= 0)
		return false;	

	const std::string fileName(GetFileNameString().data(), GetFileNameString().length());	
	int32_t sepPos = fileName.rfind('\\');
	gs_modelLocalPath.assign(fileName, 0, static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(sepPos) + 1);

	int32_t modelCount = m_pgrnFileInfo->ModelCount;

	m_models = new CGrannyModel[modelCount];

	for (int32_t m = 0; m < modelCount; ++m)
	{
		CGrannyModel & rModel = m_models[m];
		granny_model * pgrnModel = m_pgrnFileInfo->Models[m];

		if (!rModel.CreateFromGrannyModelPointer(pgrnModel))
			return false;

		rModel.SetFromFilename(GetFileName());
	}

	GrannyFreeFileSection(m_pgrnFile, GrannyStandardRigidVertexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardRigidIndexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardDeformableIndexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardTextureSection);
	return true;
}

bool CGraphicThing::LoadMotions()
{
	assert(m_pgrnFile != nullptr);

	if (m_pgrnFileInfo->AnimationCount <= 0)
		return false;

	int32_t motionCount = m_pgrnFileInfo->AnimationCount;

	for (int32_t m = 0; m < motionCount; ++m)
	{
		std::shared_ptr<CGrannyMotion> motion = std::make_shared<CGrannyMotion>();

		if (!motion->BindGrannyAnimation(m_pgrnFileInfo->Animations[m]))
			return false;

		m_motions.emplace_back(motion);
	}

	return true;
}
