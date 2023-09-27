#include "StdAfx.h"
#include "Mesh.h"

#include "Granny3D.h"
#include "Model.h"
#include "Material.h"

CGrannyMesh::CGrannyMesh()
{
	Initialize();
}

CGrannyMesh::~CGrannyMesh()
{
	Destroy();
}

void CGrannyMesh::Initialize()
{
	for (int32_t r = 0; r < CGrannyMaterial::TYPE_MAX_NUM; ++r)
		m_triGroupNodeLists[r] = nullptr;

	m_outputVertexType = GrannyPNT332VertexType;
	m_pgrnMesh = nullptr;

	m_meshDeformer = nullptr;

	m_triGroupNodes = nullptr;

	m_vtxBasePos = 0;
	m_idxBasePos = 0;

	m_isTwoSide = false;
	m_bHaveBlendThing = false;
}

void CGrannyMesh::Destroy()
{
	if (m_triGroupNodes)
		delete[] m_triGroupNodes;

	m_mtrlIndexVector.clear();

	m_meshDeformer = nullptr;

	Initialize();
}

void CGrannyMesh::LoadIndices(void* dstBaseIndices)
{
	TIndex* dstIndices = ((TIndex*)dstBaseIndices) + m_idxBasePos;
	GrannyCopyMeshIndices(m_pgrnMesh, sizeof(TIndex), dstIndices);
}

void CGrannyMesh::LoadVertices(void* dstBaseVertices)
{
	assert(GrannyMeshIsRigid(m_pgrnMesh) && "Deforming meshes have no raw data access");

	uint8_t* byteVertices = static_cast<uint8_t*>(dstBaseVertices);
	byteVertices += m_vtxBasePos * GrannyGetTotalObjectSize(m_outputVertexType);

	GrannyCopyMeshVertices(m_pgrnMesh, m_outputVertexType, byteVertices);
}

bool CGrannyMesh::CanDeformPNTVertices() const
{
	return !GrannyMeshIsRigid(m_pgrnMesh);
}

void CGrannyMesh::DeformPNTVertices(void* dstBaseVertices, granny_world_pose* worldPose, granny_mesh_binding* pgrnMeshBinding) const
{
	assert(m_meshDeformer != nullptr);
	assert(dstBaseVertices != nullptr);
	assert(worldPose != nullptr);

	uint8_t* byteVertices = static_cast<uint8_t*>(dstBaseVertices);
	byteVertices += m_vtxBasePos * GrannyGetTotalObjectSize(m_outputVertexType);

	granny_int32x BoneCount = GrannyGetMeshBindingBoneCount(pgrnMeshBinding);
	granny_real32* TransformBuffer = (granny_real32*)_aligned_malloc(GrannyGetMeshBinding4x4ArraySize(pgrnMeshBinding, BoneCount), 16);

	if (GrannyMeshBindingIsTransferred(pgrnMeshBinding)) 
	{
		GrannyBuildMeshBinding4x4Array(pgrnMeshBinding, worldPose, 0, BoneCount, TransformBuffer);
		GrannyDeformVertices(m_meshDeformer, 0, TransformBuffer, GrannyGetMeshVertexCount(m_pgrnMesh), GrannyGetMeshVertices(m_pgrnMesh), byteVertices);
	}
	else 
	{
		GrannyDeformVertices(m_meshDeformer,
			GrannyGetMeshBindingToBoneIndices(pgrnMeshBinding),
			(float*)GrannyGetWorldPoseComposite4x4Array(worldPose),
			GrannyGetMeshVertexCount(m_pgrnMesh),
			GrannyGetMeshVertices(m_pgrnMesh),
			byteVertices);
	}

	_aligned_free(TransformBuffer);

}

const granny_mesh* CGrannyMesh::GetGrannyMeshPointer() const
{
	return m_pgrnMesh;
}

const CGrannyMesh::TTriGroupNode* CGrannyMesh::GetTriGroupNodeList(CGrannyMaterial::EType eMtrlType) const
{
	return m_triGroupNodeLists[eMtrlType];
}

int32_t CGrannyMesh::GetVertexCount() const
{
	assert(m_pgrnMesh != nullptr);
	return GrannyGetMeshVertexCount(m_pgrnMesh);
}

int32_t CGrannyMesh::GetVertexBasePosition() const
{
	return m_vtxBasePos;
}

int32_t CGrannyMesh::GetIndexBasePosition() const
{
	return m_idxBasePos;
}

bool CGrannyMesh::IsEmpty() const
{
	if (m_pgrnMesh)
		return false;

	return true;
}

bool CGrannyMesh::Create(granny_mesh* pgrnMesh, int32_t vtxBasePos, int32_t idxBasePos, CGrannyMaterialPalette& matPal, const granny_data_type_definition* outputVertexType)
{
	assert(IsEmpty());

	m_pgrnMesh = pgrnMesh;
	m_outputVertexType = outputVertexType;
	m_vtxBasePos = vtxBasePos;
	m_idxBasePos = idxBasePos;

	if (m_pgrnMesh->BoneBindingCount < 0)
		return true;

	if (!GrannyMeshIsRigid(m_pgrnMesh))
	{
		if (!GrannyDataTypesAreEqual(GrannyPNT332VertexType, m_outputVertexType)) 
		{
			SysLog("Mesh {0}: Deforming meshes must be of type PNT332", pgrnMesh->Name ? pgrnMesh->Name : "<no name>");
			return false;
		}

		m_meshDeformer = Granny3D::GetInstance()->GetMeshDeformerByVertexType(GrannyGetMeshVertexType(m_pgrnMesh));
		if (!m_meshDeformer) 
		{
			SysLog("Failed to create mesh deformer");
			return false;
		}
	}

	// Two Side Mesh
	if (!strncmp(m_pgrnMesh->Name, "2x", 2))
		m_isTwoSide = true;

	if (!LoadMaterials(matPal))
		return false;

	if (!LoadTriGroupNodeList(matPal))
		return false;

	return true;
}

bool compareByMaterial(const CGrannyMesh::TTriGroupNode* a, const CGrannyMesh::TTriGroupNode* b)
{
	return (a && b) && a->mtrlIndex < b->mtrlIndex;
}

bool CGrannyMesh::LoadTriGroupNodeList(CGrannyMaterialPalette& matPal)
{
	assert(m_pgrnMesh != nullptr);
	assert(m_triGroupNodes == nullptr);

	int32_t mtrlCount = m_pgrnMesh->MaterialBindingCount;
	if (mtrlCount <= 0)
		return true;

	int32_t GroupNodeCount = GrannyGetMeshTriangleGroupCount(m_pgrnMesh);
	if (GroupNodeCount <= 0)
		return true;

	m_triGroupNodes = new TTriGroupNode[GroupNodeCount];

	const granny_tri_material_group* grnTriGroups = GrannyGetMeshTriangleGroups(m_pgrnMesh);

	for (int32_t g = 0; g < GroupNodeCount; ++g)
	{
		const granny_tri_material_group& grnTriGroup = grnTriGroups[g];
		TTriGroupNode& triGroupNode = m_triGroupNodes[g];

		triGroupNode.idxPos = m_idxBasePos + grnTriGroup.TriFirst * 3;
		triGroupNode.triCount = grnTriGroup.TriCount;

		int32_t iMtrl = grnTriGroup.MaterialIndex;
		if (iMtrl < 0 || iMtrl >= mtrlCount)
			triGroupNode.mtrlIndex = 0;
		else
			triGroupNode.mtrlIndex = m_mtrlIndexVector[iMtrl];

		const CGrannyMaterial& mat = matPal.GetMaterialRef(triGroupNode.mtrlIndex);

		triGroupNode.pNextTriGroupNode = m_triGroupNodeLists[mat.GetType()];
		m_triGroupNodeLists[mat.GetType()] = &triGroupNode;
	}

	std::sort(std::begin(m_triGroupNodeLists), std::end(m_triGroupNodeLists), compareByMaterial);
	return true;
}

void CGrannyMesh::RebuildTriGroupNodeList()
{
}

bool CGrannyMesh::LoadMaterials(CGrannyMaterialPalette& matPal)
{
	assert(m_pgrnMesh != nullptr);

	if (m_pgrnMesh->MaterialBindingCount <= 0)
		return true;

	bool bHaveBlendThing = false;

	int32_t mtrlCount = m_pgrnMesh->MaterialBindingCount;
	for (int32_t m = 0; m < mtrlCount; ++m)
	{
		granny_material* pgrnMaterial = m_pgrnMesh->MaterialBindings[m].Material;

		uint32_t index = matPal.RegisterMaterial(pgrnMaterial);
		m_mtrlIndexVector.emplace_back(index);

		if (matPal.GetMaterialRef(index).GetType() == CGrannyMaterial::TYPE_BLEND_PNT)
			bHaveBlendThing = true;
	}

	m_bHaveBlendThing = bHaveBlendThing;
	return true;
}

bool CGrannyMesh::IsTwoSide() const
{
	return m_isTwoSide;
}
