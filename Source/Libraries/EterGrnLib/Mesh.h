#pragma once
#include "Material.h"

class CGrannyMesh
{
public:
	enum EType
	{
		TYPE_RIGID,
		TYPE_DEFORM,
		TYPE_MAX_NUM
	};

	typedef struct STriGroupNode
	{
		STriGroupNode* pNextTriGroupNode;
		int32_t idxPos;
		int32_t triCount;
		uint32_t mtrlIndex;
	} TTriGroupNode;

public:
	CGrannyMesh();
	~CGrannyMesh();

	bool IsEmpty() const;

	bool Create(granny_mesh* pgrnMesh, int32_t vtxBasePos, int32_t idxBasePos, CGrannyMaterialPalette& matPal, const granny_data_type_definition* outputVertexType);

	void LoadIndices(void* dstBaseIndices);
	void LoadVertices(void* dstBaseVertices);
	void Destroy();

	bool CanDeformPNTVertices() const;
	void DeformPNTVertices(void* dstBaseVertices, granny_world_pose* worldPose, granny_mesh_binding* pgrnMeshBinding) const;

	bool IsTwoSide() const;

	int32_t GetVertexCount() const;

	int32_t GetVertexBasePosition() const;
	int32_t GetIndexBasePosition() const;

	const granny_mesh* GetGrannyMeshPointer() const;
	const TTriGroupNode* GetTriGroupNodeList(CGrannyMaterial::EType eMtrlType) const;

	void RebuildTriGroupNodeList();
	bool HaveBlendThing() { return m_bHaveBlendThing; }

protected:
	void Initialize();

	bool LoadMaterials(CGrannyMaterialPalette& matPal);
	bool LoadTriGroupNodeList(CGrannyMaterialPalette& matPal);

	// Source Granny mesh
	granny_mesh* m_pgrnMesh;

	// General output vertex format
	// Used by LoadVertices() and DeformPNTVertices
	const granny_data_type_definition* m_outputVertexType;

	// null when mesh is rigid
	granny_mesh_deformer* m_meshDeformer;

	// Granny Material Data
	std::vector<uint32_t> m_mtrlIndexVector;

	// TriGroups Data
	TTriGroupNode* m_triGroupNodes;
	TTriGroupNode* m_triGroupNodeLists[CGrannyMaterial::TYPE_MAX_NUM];

	int32_t m_vtxBasePos;
	int32_t m_idxBasePos;

	bool m_isTwoSide;
	bool m_bHaveBlendThing;
};