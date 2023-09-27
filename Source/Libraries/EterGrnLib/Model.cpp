#include "StdAfx.h"
#include "Model.h"
#include "Mesh.h"

const CGrannyMaterialPalette& CGrannyModel::GetMaterialPalette() const
{
	return m_kMtrlPal;
}

const CGrannyModel::TMeshNode* CGrannyModel::GetMeshNodeList(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType) const
{
	return m_meshNodeLists[eMeshType][eMtrlType];
}

CGrannyMesh* CGrannyModel::GetMeshPointer(int32_t iMesh)
{
	assert(m_meshs != nullptr);
	assert(CheckMeshIndex(iMesh));
	return m_meshs + iMesh;
}

const CGrannyMesh* CGrannyModel::GetMeshPointer(int32_t iMesh) const
{
	assert(m_meshs != nullptr);
	assert(CheckMeshIndex(iMesh));
	return m_meshs + iMesh;
}

bool CGrannyModel::CanDeformPNTVertices() const
{
	return m_canDeformPNVertices;
}

void CGrannyModel::DeformPNTVertices(void* dstBaseVertices, granny_world_pose* worldPose, const std::vector<granny_mesh_binding*>& c_rvct_pgrnMeshBinding) const
{
	int32_t meshCount = GetMeshCount();

	for (int32_t iMesh = 0; iMesh < meshCount; ++iMesh)
	{
		assert(iMesh < c_rvct_pgrnMeshBinding.size());

		CGrannyMesh& rMesh = m_meshs[iMesh];
		if (rMesh.CanDeformPNTVertices())
			rMesh.DeformPNTVertices(dstBaseVertices, worldPose, c_rvct_pgrnMeshBinding[iMesh]);
	}
}

int32_t CGrannyModel::GetRigidVertexCount() const
{
	return m_rigidVtxCount;
}

int32_t CGrannyModel::GetDeformVertexCount() const
{
	return m_deformVtxCount;
}

int32_t CGrannyModel::GetVertexCount() const
{
	return m_vtxCount;
}

int32_t CGrannyModel::GetMeshCount() const
{
	return m_pgrnModel ? m_pgrnModel->MeshBindingCount : 0;
}

granny_model* CGrannyModel::GetGrannyModelPointer()
{
	return m_pgrnModel;
}

const CGraphicVertexBuffer& CGrannyModel::GetVertexBuffer() const
{
	return m_vertexBuffer;
}

LPDIRECT3DINDEXBUFFER9 CGrannyModel::GetD3DIndexBuffer() const
{
	return m_idxBuf.GetD3DIndexBuffer();
}

bool CGrannyModel::LockVertices(void** indicies, void** vertices) const
{
	if (!m_idxBuf.Lock(indicies))
		return false;

	if (!m_vertexBuffer.Lock(vertices))
	{
		m_idxBuf.Unlock();
		return false;
	}

	return true;
}

void CGrannyModel::UnlockVertices() const
{
	m_idxBuf.Unlock();
	m_vertexBuffer.Unlock();
}

bool CGrannyModel::LoadVertices()
{
	if (m_rigidVtxCount <= 0)
		return true;

	assert(m_meshs != nullptr);

	if (!m_vertexBuffer.Create(m_rigidVtxCount, m_fvf, D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED))
		return false;

	void* vertices;
	if (!m_vertexBuffer.Lock(&vertices))
		return false;

	for (int32_t m = 0; m < m_pgrnModel->MeshBindingCount; ++m)
	{
		CGrannyMesh& rMesh = m_meshs[m];
		if (!rMesh.CanDeformPNTVertices())
			rMesh.LoadVertices(vertices);
	}

	m_vertexBuffer.Unlock();
	return true;
}

bool CGrannyModel::LoadIndices()
{
	if (m_idxCount <= 0)
		return true;

	if (!m_idxBuf.Create(m_idxCount, D3DFMT_INDEX16))
		return false;

	void* indices = nullptr;

	if (!m_idxBuf.Lock((void**)&indices))
		return false;

	for (int32_t m = 0; m < m_pgrnModel->MeshBindingCount; ++m)
	{
		CGrannyMesh& rMesh = m_meshs[m];
		rMesh.LoadIndices(indices);
	}

	m_idxBuf.Unlock();
	return true;
}

bool CGrannyModel::LoadMeshs()
{
	assert(m_meshs == nullptr);
	assert(m_pgrnModel != nullptr);

	if (m_pgrnModel->MeshBindingCount <= 0)
		return true;

	const granny_data_type_definition* rigidType;
	if (!FindBestRigidVertexFormat(m_pgrnModel, rigidType, m_fvf))
		return false;

	int32_t vtxRigidPos = 0;
	int32_t vtxDeformPos = 0;
	int32_t idxPos = 0;

	int32_t diffusePNTMeshNodeCount = 0;
	int32_t blendPNTMeshNodeCount = 0;

	m_meshs = new CGrannyMesh[m_pgrnModel->MeshBindingCount];

	for (int32_t m = 0; m < m_pgrnModel->MeshBindingCount; ++m)
	{
		CGrannyMesh& rMesh = m_meshs[m];
		granny_mesh* pgrnMesh = m_pgrnModel->MeshBindings[m].Mesh;

		if (GrannyMeshIsRigid(pgrnMesh)) 
		{
			if (!rMesh.Create(pgrnMesh, vtxRigidPos, idxPos, m_kMtrlPal, rigidType))
				return false;

			vtxRigidPos += GrannyGetMeshVertexCount(pgrnMesh);
		}
		else 
		{
			// This uses GrannyPNT332VertexType
			if (!rMesh.Create(pgrnMesh, vtxDeformPos, idxPos, m_kMtrlPal, GrannyPNT332VertexType))
				return false;

			vtxDeformPos += GrannyGetMeshVertexCount(pgrnMesh);
			m_canDeformPNVertices = true;
		}

		m_bHaveBlendThing |= rMesh.HaveBlendThing();

		idxPos += GrannyGetMeshIndexCount(pgrnMesh);

		if (rMesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_DIFFUSE_PNT))
			++diffusePNTMeshNodeCount;

		if (rMesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_BLEND_PNT))
			++blendPNTMeshNodeCount;
	}

	m_meshNodeCapacity = diffusePNTMeshNodeCount + blendPNTMeshNodeCount;
	m_meshNodes = new TMeshNode[m_meshNodeCapacity];

	for (int32_t n = 0; n < m_pgrnModel->MeshBindingCount; ++n)
	{
		CGrannyMesh& rMesh = m_meshs[n];
		granny_mesh* pgrnMesh = m_pgrnModel->MeshBindings[n].Mesh;

		CGrannyMesh::EType eMeshType = GrannyMeshIsRigid(pgrnMesh) ? CGrannyMesh::TYPE_RIGID : CGrannyMesh::TYPE_DEFORM;

		if (rMesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_DIFFUSE_PNT))
			AppendMeshNode(eMeshType, CGrannyMaterial::TYPE_DIFFUSE_PNT, n);

		if (rMesh.GetTriGroupNodeList(CGrannyMaterial::TYPE_BLEND_PNT))
			AppendMeshNode(eMeshType, CGrannyMaterial::TYPE_BLEND_PNT, n);
	}

	m_rigidVtxCount = vtxRigidPos;
	m_deformVtxCount = vtxDeformPos;

	m_vtxCount = vtxRigidPos + vtxDeformPos;
	m_idxCount = idxPos;
	return true;
}

bool CGrannyModel::CheckMeshIndex(int32_t iIndex) const
{
	if (iIndex >= 0 && iIndex < m_pgrnModel->MeshBindingCount)
		return true;

	SysLog("CGrannyModel::CheckMeshIndex> index {0} MeshBindingCount {1} name {2}", iIndex, m_pgrnModel->MeshBindingCount, m_pgrnModel->Name);

	return false;
}

void CGrannyModel::AppendMeshNode(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType, int32_t iMesh)
{
	assert(m_meshNodeSize < m_meshNodeCapacity);

	TMeshNode& rMeshNode = m_meshNodes[m_meshNodeSize++];
	rMeshNode.iMesh = iMesh;
	rMeshNode.pMesh = m_meshs + iMesh;
	rMeshNode.pNextMeshNode = m_meshNodeLists[eMeshType][eMtrlType];

	m_meshNodeLists[eMeshType][eMtrlType] = &rMeshNode;
}

bool CGrannyModel::CreateFromGrannyModelPointer(granny_model* pgrnModel)
{
	assert(IsEmpty());

	m_pgrnModel = pgrnModel;

	if (!LoadMeshs())
		return false;

	if (!LoadVertices())
		return false;

	if (!LoadIndices())
		return false;

	AddReference();

	return true;
}

int32_t CGrannyModel::GetIdxCount()
{
	return m_idxCount;
}

bool CGrannyModel::CreateDeviceObjects()
{
	if (m_rigidVtxCount > 0)
		if (!m_vertexBuffer.CreateDeviceObjects())
			return false;

	if (m_idxCount > 0)
		if (!m_idxBuf.CreateDeviceObjects())
			return false;

	int32_t meshCount = GetMeshCount();

	for (int32_t i = 0; i < meshCount; ++i)
	{
		CGrannyMesh& rMesh = m_meshs[i];
		rMesh.RebuildTriGroupNodeList();
	}

	return true;
}

void CGrannyModel::DestroyDeviceObjects()
{
	m_vertexBuffer.DestroyDeviceObjects();
	m_idxBuf.DestroyDeviceObjects();
}

bool CGrannyModel::IsEmpty() const
{
	if (m_pgrnModel)
		return false;

	return true;
}

void CGrannyModel::Destroy()
{
	m_kMtrlPal.Clear();

	if (m_meshNodes)
		delete[] m_meshNodes;

	if (m_meshs)
		delete[] m_meshs;

	m_vertexBuffer.Destroy();
	m_idxBuf.Destroy();

	Initialize();
}

void CGrannyModel::Initialize()
{
	memset(m_meshNodeLists, 0, sizeof(m_meshNodeLists));

	m_pgrnModel = nullptr;
	m_meshs = nullptr;
	m_meshNodes = nullptr;

	m_meshNodeSize = 0;
	m_meshNodeCapacity = 0;

	m_rigidVtxCount = 0;
	m_deformVtxCount = 0;
	m_vtxCount = 0;
	m_idxCount = 0;

	m_canDeformPNVertices = false;

	m_fvf = 0;
	m_bHaveBlendThing = false;
}

CGrannyModel::CGrannyModel()
{
	Initialize();
}

CGrannyModel::~CGrannyModel()
{
	Destroy();
}
