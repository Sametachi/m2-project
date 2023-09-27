#include "StdAfx.h"
#include "../eterBase/Stl.h"
#include "GrpVertexBuffer.h"
#include "StateManager.h"

CGraphicVertexBuffer::CGraphicVertexBuffer()
	: m_vb(nullptr)
	, m_usage(0)
	, m_pool(D3DPOOL_MANAGED)
	, m_vertexCount(0)
	, m_lockFlags(0)
{
	// ctor
}

CGraphicVertexBuffer::~CGraphicVertexBuffer()
{
	DestroyDeviceObjects();
}

bool CGraphicVertexBuffer::Create(uint32_t vertexCount, uint32_t fvf, uint32_t usage, D3DPOOL pool)
{
	assert(ms_lpd3dDevice != nullptr);
	assert(vertexCount > 0);

	DestroyDeviceObjects();

	m_vertexCount = vertexCount;
	m_pool = pool;
	m_usage = usage;
	m_fvf = fvf;

	if ((usage & D3DUSAGE_WRITEONLY) || (usage & D3DUSAGE_DYNAMIC))
		m_lockFlags = D3DLOCK_DISCARD;
	else
		m_lockFlags = D3DLOCK_READONLY;

	return CreateDeviceObjects();
}

void CGraphicVertexBuffer::Destroy()
{
	DestroyDeviceObjects();
}

bool CGraphicVertexBuffer::CreateDeviceObjects()
{
	assert(ms_lpd3dDevice != nullptr);
	assert(m_vb == nullptr);

	const auto size = D3DXGetFVFVertexSize(m_fvf) * m_vertexCount;
	const auto hr = ms_lpd3dDevice->CreateVertexBuffer(size,
		m_usage,
		m_fvf,
		m_pool,
		&m_vb,
		nullptr);
	if (FAILED(hr))
	{
		SysLog("CreateVertexBuffer() failed for size {0} usage {1} fvf {2} pool {3} with {4}", size, m_usage, m_fvf, m_pool, hr);
		return false;
	}

	return true;
}

void CGraphicVertexBuffer::DestroyDeviceObjects()
{
	safe_release(m_vb);
}

bool CGraphicVertexBuffer::IsEmpty() const
{
	return m_vb == nullptr;
}

bool CGraphicVertexBuffer::Copy(int bufSize, const void* srcVertices)
{
	void* dstVertices;
	if (!Lock(&dstVertices))
		return false;

	memcpy(dstVertices, srcVertices, bufSize);

	Unlock();
	return true;
}

bool CGraphicVertexBuffer::LockRange(uint32_t count, void** vertices) const
{
	if (!m_vb)
		return false;

	const auto hr = m_vb->Lock(0,
		D3DXGetFVFVertexSize(m_fvf) * count,
		vertices,
		m_lockFlags);

	if (FAILED(hr))
	{
		SysLog("VertexBuffer::Lock() failed for count {0} with {1}", count, hr);
		return false;
	}

	return true;
}

bool CGraphicVertexBuffer::Lock(void** vertices) const
{
	if (!m_vb)
		return false;

	const auto hr = m_vb->Lock(0, 0, vertices, m_lockFlags);
	if (FAILED(hr))
	{
		SysLog("VertexBuffer::Lock() failed with {0}", hr);
		return false;
	}

	return true;
}

bool CGraphicVertexBuffer::Unlock() const
{
	if (!m_vb)
		return false;

	const auto hr = m_vb->Unlock();
	if (FAILED(hr))
	{
		SysLog("VertexBuffer::Unlock() with {0}", hr);
		return false;
	}

	return true;
}

void CGraphicVertexBuffer::Bind(uint32_t index) const
{
	assert(ms_lpd3dDevice != NULL);
	STATEMANAGER->SetFVF(m_fvf);
	STATEMANAGER->SetStreamSource(index, m_vb, D3DXGetFVFVertexSize(m_fvf));
	D3DPERF_SetMarker(D3DCOLOR_ARGB(251, 50, 50, 0), L" CGraphicVertexBuffer::Bind");
}

uint32_t CGraphicVertexBuffer::GetFlexibleVertexFormat() const
{
	return m_fvf;
}

uint32_t CGraphicVertexBuffer::GetVertexCount() const
{
	return m_vertexCount;
}

IDirect3DVertexBuffer9* CGraphicVertexBuffer::GetD3DVertexBuffer() const
{
	return m_vb;
}
