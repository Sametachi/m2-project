#include "StdAfx.h"
#include "../eterlib/StateManager.h"
#include "ModelInstance.h"
#include "Model.h"

/*
	Now this is a bit tricky.. Granny loves Binding, but doesn't like resetting the state..
	Guess it is what it is until we don't have a better graphical processor implemented.

	Update:
	Apperantly AMD doesn't support getting rigid index from FX shaders...
*/

void CGrannyModelInstance::DeformNoSkin(const D3DXMATRIX* c_pWorldMatrix, float allowedError)
{
	if (IsEmpty())
		return;

	UpdateWorldPose(allowedError);
	UpdateWorldMatrices(c_pWorldMatrix);
}

void CGrannyModelInstance::RenderWithOneTexture()
{
	if (IsEmpty())
		return;

	const CGraphicVertexBuffer& deformVertexBuffer = __GetDeformableVertexBufferRef();
	if (!deformVertexBuffer.IsEmpty())
	{
		deformVertexBuffer.Bind(0);
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}

	const CGraphicVertexBuffer& rigidVertexBuffer = m_model->GetVertexBuffer();
	if (!rigidVertexBuffer.IsEmpty())
	{
		rigidVertexBuffer.Bind(0);
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithOneTexture()
{
	if (IsEmpty())
		return;

	const CGraphicVertexBuffer& deformVertexBuffer = __GetDeformableVertexBufferRef();
	if (!deformVertexBuffer.IsEmpty()) 
	{
		deformVertexBuffer.Bind(0);
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	const CGraphicVertexBuffer& rigidVertexBuffer = m_model->GetVertexBuffer();
	if (!rigidVertexBuffer.IsEmpty()) 
	{
		rigidVertexBuffer.Bind(0);
		RenderMeshNodeListWithOneTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

// With Two Texture
void CGrannyModelInstance::RenderWithTwoTexture()
{
	if (IsEmpty())
		return;

	const CGraphicVertexBuffer& deformVertexBuffer = __GetDeformableVertexBufferRef();
	if (!deformVertexBuffer.IsEmpty()) 
	{
		deformVertexBuffer.Bind(0);
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}

	const CGraphicVertexBuffer& rigidVertexBuffer = m_model->GetVertexBuffer();
	if (!rigidVertexBuffer.IsEmpty()) 
	{
		rigidVertexBuffer.Bind(0);
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
	}
}

void CGrannyModelInstance::BlendRenderWithTwoTexture()
{
	if (IsEmpty())
		return;

	const CGraphicVertexBuffer& deformVertexBuffer = __GetDeformableVertexBufferRef();
	if (!deformVertexBuffer.IsEmpty()) 
	{
		deformVertexBuffer.Bind(0);
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	const CGraphicVertexBuffer& rigidVertexBuffer = m_model->GetVertexBuffer();
	if (!rigidVertexBuffer.IsEmpty()) 
	{
		rigidVertexBuffer.Bind(0);
		RenderMeshNodeListWithTwoTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

void CGrannyModelInstance::RenderWithoutTexture()
{
	if (IsEmpty())
		return;

	STATEMANAGER->SetTexture(0, nullptr);
	STATEMANAGER->SetTexture(1, nullptr);

	const CGraphicVertexBuffer& deformVertexBuffer = __GetDeformableVertexBufferRef();
	if (!deformVertexBuffer.IsEmpty()) 
	{
		deformVertexBuffer.Bind(0);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_DEFORM, CGrannyMaterial::TYPE_BLEND_PNT);
	}

	const CGraphicVertexBuffer& rigidVertexBuffer = m_model->GetVertexBuffer();
	if (!rigidVertexBuffer.IsEmpty()) 
	{
		rigidVertexBuffer.Bind(0);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);
		RenderMeshNodeListWithoutTexture(CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_BLEND_PNT);
	}
}

// With One Texture
void CGrannyModelInstance::RenderMeshNodeListWithOneTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderMeshNodeListWithOneTexture **");
	assert(m_model != nullptr);

	LPDIRECT3DINDEXBUFFER9 lpd3dIdxBuf = m_model->GetD3DIndexBuffer();
	assert(lpd3dIdxBuf != nullptr);

	STATEMANAGER->SetIndices(lpd3dIdxBuf);

	const CGrannyModel::TMeshNode* pMeshNode = m_model->GetMeshNodeList(eMeshType, eMtrlType);
	while (pMeshNode)
	{
		const CGrannyMesh* pMesh = pMeshNode->pMesh;
		int32_t vtxMeshBasePos = pMesh->GetVertexBasePosition();

		STATEMANAGER->SetTransform(D3DTS_WORLD, &m_meshMatrices[pMeshNode->iMesh]);

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int32_t vtxCount = pMesh->GetVertexCount();
		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			CGrannyMaterial& rkMtrl = m_kMtrlPal.GetMaterialRef(pTriGroupNode->mtrlIndex);
			rkMtrl.ApplyRenderState();
			STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, vtxCount, pTriGroupNode->idxPos, pTriGroupNode->triCount, vtxMeshBasePos);
			rkMtrl.RestoreRenderState();

			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}
	D3DPERF_EndEvent();
}

// With Two Texture
void CGrannyModelInstance::RenderMeshNodeListWithTwoTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 100, 0), L"** RenderMeshNodeListWithTwoTexture **");
	assert(m_model != nullptr);

	LPDIRECT3DINDEXBUFFER9 lpd3dIdxBuf = m_model->GetD3DIndexBuffer();
	assert(lpd3dIdxBuf != nullptr);

	STATEMANAGER->SetIndices(lpd3dIdxBuf);

	const CGrannyModel::TMeshNode* pMeshNode = m_model->GetMeshNodeList(eMeshType, eMtrlType);
	while (pMeshNode)
	{
		const CGrannyMesh* pMesh = pMeshNode->pMesh;
		int32_t vtxMeshBasePos = pMesh->GetVertexBasePosition();

		STATEMANAGER->SetTransform(D3DTS_WORLD, &m_meshMatrices[pMeshNode->iMesh]);

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int32_t vtxCount = pMesh->GetVertexCount();
		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			const CGrannyMaterial& rkMtrl = m_kMtrlPal.GetMaterialRef(pTriGroupNode->mtrlIndex);
			STATEMANAGER->SetTexture(0, rkMtrl.GetD3DTexture(0));
			STATEMANAGER->SetTexture(1, rkMtrl.GetD3DTexture(1));
			STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, vtxCount, pTriGroupNode->idxPos, pTriGroupNode->triCount, vtxMeshBasePos);
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}
	D3DPERF_EndEvent();
}

// With Three Texture
void CGrannyModelInstance::RenderMeshNodeListWithThreeTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 100, 0), L"** RenderMeshNodeListWithTwoTexture **");
	assert(m_model != nullptr);

	LPDIRECT3DINDEXBUFFER9 lpd3dIdxBuf = m_model->GetD3DIndexBuffer();
	assert(lpd3dIdxBuf != nullptr);

	STATEMANAGER->SetIndices(lpd3dIdxBuf);

	const CGrannyModel::TMeshNode* pMeshNode = m_model->GetMeshNodeList(eMeshType, eMtrlType);
	while (pMeshNode)
	{
		const CGrannyMesh* pMesh = pMeshNode->pMesh;
		int32_t vtxMeshBasePos = pMesh->GetVertexBasePosition();

		STATEMANAGER->SetTransform(D3DTS_WORLD, &m_meshMatrices[pMeshNode->iMesh]);

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int32_t vtxCount = pMesh->GetVertexCount();
		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			const CGrannyMaterial& rkMtrl = m_kMtrlPal.GetMaterialRef(pTriGroupNode->mtrlIndex);
			STATEMANAGER->SetTexture(0, rkMtrl.GetD3DTexture(0));
			STATEMANAGER->SetTexture(1, rkMtrl.GetD3DTexture(1));
			STATEMANAGER->SetTexture(1, rkMtrl.GetD3DTexture(2));

			STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, vtxCount, pTriGroupNode->idxPos, pTriGroupNode->triCount, vtxMeshBasePos);
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}
	D3DPERF_EndEvent();
}

// Without Texture
void CGrannyModelInstance::RenderMeshNodeListWithoutTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType)
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 140, 140, 0), L"** RenderMeshNodeListWithoutTexture **");
	assert(m_model != nullptr);

	LPDIRECT3DINDEXBUFFER9 lpd3dIdxBuf = m_model->GetD3DIndexBuffer();
	assert(lpd3dIdxBuf != nullptr);

	STATEMANAGER->SetIndices(lpd3dIdxBuf);

	const CGrannyModel::TMeshNode* pMeshNode = m_model->GetMeshNodeList(eMeshType, eMtrlType);
	while (pMeshNode)
	{
		const CGrannyMesh* pMesh = pMeshNode->pMesh;
		int32_t vtxMeshBasePos = pMesh->GetVertexBasePosition();

		STATEMANAGER->SetTransform(D3DTS_WORLD, &m_meshMatrices[pMeshNode->iMesh]);

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList(eMtrlType);
		int32_t vtxCount = pMesh->GetVertexCount();

		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;
			STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, vtxCount, pTriGroupNode->idxPos, pTriGroupNode->triCount, vtxMeshBasePos);
			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}

	D3DPERF_EndEvent();
}
