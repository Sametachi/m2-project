#pragma once
#include "Mesh.h"

#include <Eterlib/GrpVertexBuffer.h>
#include <Eterlib/GrpIndexBuffer.h>

class CGrannyModel : public CReferenceObject
{
public:
	typedef struct SMeshNode
	{
		int32_t iMesh;
		const CGrannyMesh* pMesh;
		SMeshNode* pNextMeshNode;
	} TMeshNode;

public:
	CGrannyModel();
	virtual ~CGrannyModel();
	
	bool IsEmpty() const;
	bool CreateFromGrannyModelPointer(granny_model* pgrnModel);
	bool CreateDeviceObjects();
	void DestroyDeviceObjects();
	void Destroy();
	
	int32_t GetRigidVertexCount() const;
	int32_t GetDeformVertexCount() const;
	int32_t GetVertexCount() const;
	
	bool CanDeformPNTVertices() const;
	void DeformPNTVertices(void* dstBaseVertices, granny_world_pose* worldPose, const std::vector<granny_mesh_binding*>& c_rvct_pgrnMeshBinding) const;
	
	int32_t GetIdxCount();
	int32_t GetMeshCount() const;
	CGrannyMesh* GetMeshPointer(int32_t iMesh);
	granny_model* GetGrannyModelPointer();
	const CGrannyMesh* GetMeshPointer(int32_t iMesh) const;
	
	const CGraphicVertexBuffer& GetVertexBuffer() const;
	LPDIRECT3DINDEXBUFFER9 GetD3DIndexBuffer() const;
	
	const TMeshNode* GetMeshNodeList(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType) const;
	
	bool LockVertices(void** indicies, void** vertices) const;
	void UnlockVertices() const;
	
	const CGrannyMaterialPalette& GetMaterialPalette() const;
	
	void SetFromFilename(const std::string& fromFilename)
	{
		m_fromFilename = fromFilename;
	}
	
	const std::string& GetFromFilename() const
	{
		return m_fromFilename;
	}

protected:
	bool LoadMeshs();
	bool LoadVertices();
	bool LoadIndices();
	void Initialize();

	bool CheckMeshIndex(int32_t iIndex) const;
	void AppendMeshNode(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType, int32_t iMesh);

	std::string m_fromFilename;
	// Granny Data
	granny_model* m_pgrnModel;
	CGrannyMesh* m_meshs;

	CGraphicIndexBuffer m_idxBuf;

	// Rigid meshes only
	CGraphicVertexBuffer m_vertexBuffer;
	uint32_t m_fvf;

	TMeshNode* m_meshNodes;
	TMeshNode* m_meshNodeLists[CGrannyMesh::TYPE_MAX_NUM][CGrannyMaterial::TYPE_MAX_NUM];

	int32_t m_deformVtxCount;
	int32_t m_rigidVtxCount;

	int32_t m_vtxCount;
	int32_t m_idxCount;

	int32_t m_meshNodeSize;
	int32_t m_meshNodeCapacity;

	bool m_canDeformPNVertices;

	CGrannyMaterialPalette m_kMtrlPal;
	bool m_bHaveBlendThing;
public:
	bool HaveBlendThing() { return m_bHaveBlendThing; }
	uint32_t GetFvF() { return m_fvf; }
};