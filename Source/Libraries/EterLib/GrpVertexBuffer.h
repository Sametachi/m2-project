#pragma once
#include "GrpBase.h"

class CGraphicVertexBuffer : public CGraphicBase
{
public:
	CGraphicVertexBuffer();
	~CGraphicVertexBuffer();

	bool Create(uint32_t vertexCount, uint32_t fvf, uint32_t usage, D3DPOOL pool);
	void Destroy();

	bool CreateDeviceObjects();
	void DestroyDeviceObjects();

	bool IsEmpty() const;

	bool Copy(int bufSize, const void* srcVertices);

	bool LockRange(uint32_t count, void** vertices) const;
	bool Lock(void** vertices) const;
	bool Unlock() const;

	void Bind(uint32_t index) const;

	uint32_t GetFlexibleVertexFormat() const;
	uint32_t GetVertexCount() const;
	IDirect3DVertexBuffer9* GetD3DVertexBuffer() const;

protected:
	IDirect3DVertexBuffer9* m_vb;

	uint32_t m_fvf;
	uint32_t m_usage;
	D3DPOOL m_pool;
	uint32_t m_vertexCount;
	uint32_t m_lockFlags;
};