#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <Basic/Singleton.h>
#define CHECK_D3DAPI(a)		\
{							\
	HRESULT hr = (a);		\
							\
	if (hr != S_OK)			\
		assert(!#a);		\
}

static const DWORD STATEMANAGER_MAX_RENDERSTATES = 256;
static const DWORD STATEMANAGER_MAX_TEXTURESTATES = 128;
static const DWORD STATEMANAGER_MAX_SAMPLERSTATES = 128;
static const DWORD STATEMANAGER_MAX_STAGES = 8;
static const DWORD STATEMANAGER_MAX_VCONSTANTS = 96;
static const DWORD STATEMANAGER_MAX_PCONSTANTS = 8;
static const DWORD STATEMANAGER_MAX_TRANSFORMSTATES = 300;	// World1 lives way up there...
static const DWORD STATEMANAGER_MAX_STREAMS = 16;

class CStreamData
{
public:
	CStreamData(LPDIRECT3DVERTEXBUFFER9 pStreamData = nullptr, UINT Stride = 0) : m_lpStreamData(pStreamData), m_Stride(Stride)
	{
	}

	bool operator == (const CStreamData& rhs) const
	{
		return ((m_lpStreamData == rhs.m_lpStreamData) && (m_Stride == rhs.m_Stride));
	}
	LPDIRECT3DVERTEXBUFFER9	m_lpStreamData;
	UINT m_Stride;
};

// State types managed by the class
typedef enum eStateType
{
	STATE_MATERIAL = 0,
	STATE_RENDER,
	STATE_TEXTURE,
	STATE_TEXTURESTAGE,
	STATE_VSHADER,
	STATE_PSHADER,
	STATE_TRANSFORM,
	STATE_VCONSTANT,
	STATE_PCONSTANT,
	STATE_STREAM,
	STATE_INDEX
} eStateType;

class CStateID
{
public:
	CStateID(eStateType Type, DWORD dwValue0 = 0, DWORD dwValue1 = 0)
		: m_Type(Type),
		m_dwValue0(dwValue0),
		m_dwValue1(dwValue1)
	{
	}

	CStateID(eStateType Type, DWORD dwStage, D3DTEXTURESTAGESTATETYPE StageType)
		: m_Type(Type),
		m_dwStage(dwStage),
		m_TextureStageStateType(StageType)
	{
	}

	CStateID(eStateType Type, D3DRENDERSTATETYPE RenderType)
		: m_Type(Type),
		m_RenderStateType(RenderType)
	{
	}

	eStateType m_Type;

	union
	{
		DWORD m_dwValue0;
		DWORD m_dwStage;
		D3DRENDERSTATETYPE m_RenderStateType;
		D3DTRANSFORMSTATETYPE m_TransformStateType;
	};

	union
	{
		DWORD m_dwValue1;
		D3DTEXTURESTAGESTATETYPE m_TextureStageStateType;
	};
};

typedef std::vector<CStateID> TStateID;

class CStateManagerState
{
public:
	CStateManagerState() : m_dwPixelShader(0), m_dwVertexShader(0)
	{
	}

	void ResetState()
	{
		DWORD i, y;

		for (i = 0; i < STATEMANAGER_MAX_RENDERSTATES; i++)
			m_RenderStates[i] = 0x7FFFFFFF;

		for (i = 0; i < STATEMANAGER_MAX_STAGES; i++)
		{
			for (y = 0; y < STATEMANAGER_MAX_TEXTURESTATES; y++)
				m_TextureStates[i][y] = 0x7FFFFFFF;

			for (y = 0; y < STATEMANAGER_MAX_SAMPLERSTATES; ++y)
				m_SamplerStates[i][y] = 0x7FFFFFFF;
		}
		for (i = 0; i < STATEMANAGER_MAX_STREAMS; i++)
			m_StreamData[i] = CStreamData();

		m_IndexData = nullptr;

		for (i = 0; i < STATEMANAGER_MAX_STAGES; i++)
			m_Textures[i] = nullptr;

		for (i = 0; i < STATEMANAGER_MAX_TRANSFORMSTATES; i++)
			D3DXMatrixIdentity(&m_Matrices[i]);

		for (i = 0; i < STATEMANAGER_MAX_VCONSTANTS; i++)
			m_VertexShaderConstants[i] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);

		for (i = 0; i < STATEMANAGER_MAX_PCONSTANTS; i++)
			m_PixelShaderConstants[i] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);

		m_dwPixelShader = 0;
		m_dwVertexShader = 0;
		m_dwVertexDeclaration = 0;
		m_dwFVF = D3DFVF_XYZ;
		m_bVertexProcessing = false;
		ZeroMemory(&m_Matrices, sizeof(D3DXMATRIX) * STATEMANAGER_MAX_TRANSFORMSTATES);
	}

	// Renderstates
	DWORD m_RenderStates[STATEMANAGER_MAX_RENDERSTATES];

	// Texture stage states
	DWORD m_TextureStates[STATEMANAGER_MAX_STAGES][STATEMANAGER_MAX_TEXTURESTATES];

	// Texture sampler states
	DWORD m_SamplerStates[STATEMANAGER_MAX_STAGES][STATEMANAGER_MAX_SAMPLERSTATES];

	// Vertex shader constants
	D3DXVECTOR4 m_VertexShaderConstants[STATEMANAGER_MAX_VCONSTANTS];

	// Pixel shader constants
	D3DXVECTOR4 m_PixelShaderConstants[STATEMANAGER_MAX_PCONSTANTS];

	// Textures
	LPDIRECT3DBASETEXTURE9 m_Textures[STATEMANAGER_MAX_STAGES];

	// Shaders
	LPDIRECT3DVERTEXSHADER9	m_dwVertexShader;
	LPDIRECT3DVERTEXDECLARATION9 m_dwVertexDeclaration;
	LPDIRECT3DPIXELSHADER9 m_dwPixelShader;

	DWORD m_dwFVF;

	D3DXMATRIX m_Matrices[STATEMANAGER_MAX_TRANSFORMSTATES];
	D3DMATERIAL9 m_D3DMaterial;
	CStreamData m_StreamData[STATEMANAGER_MAX_STREAMS];
	LPDIRECT3DINDEXBUFFER9 m_IndexData;
	bool m_bVertexProcessing;
	RECT m_scissorRect;
};

class CStateManager : public Singleton<CStateManager>
{
public:
	CStateManager(LPDIRECT3DDEVICE9 lpDevice);
	virtual ~CStateManager();

	void	SetDefaultState();
	void	Restore();

	bool	BeginScene();
	void	EndScene();

	// Material
	void	SaveMaterial();
	void	SaveMaterial(const D3DMATERIAL9* pMaterial);
	void	SetMaterial(const D3DMATERIAL9* pMaterial);
	void	GetMaterial(D3DMATERIAL9* pMaterial);
	void	SetLight(DWORD index, CONST D3DLIGHT9* pLight);
	void	GetLight(DWORD index, D3DLIGHT9* pLight);
	void	RestoreMaterial();

	// Renderstates
	void	SaveRenderState(D3DRENDERSTATETYPE Type, DWORD dwValue);
	void	RestoreRenderState(D3DRENDERSTATETYPE Type);
	void	SetRenderState(D3DRENDERSTATETYPE Type, DWORD Value);
	void	GetRenderState(D3DRENDERSTATETYPE Type, DWORD* pdwValue);

	// Textures
	void	SaveTexture(DWORD dwStage, LPDIRECT3DBASETEXTURE9 pTexture);
	void	SetTexture(DWORD dwStage, LPDIRECT3DBASETEXTURE9 pTexture);
	void	GetTexture(DWORD dwStage, LPDIRECT3DBASETEXTURE9* ppTexture);
	void	RestoreTexture(DWORD dwStage);

	// Texture stage states
	void	SaveTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type, DWORD dwValue);
	void	RestoreTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type);
	void	SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type, DWORD dwValue);
	void	GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pdwValue);
	void	SaveSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD dwValue);
	void	RestoreSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type);
	void	SetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD dwValue);
	void	GetSamplerState(DWORD dwStage, D3DSAMPLERSTATETYPE Type, DWORD* pdwValue);
	void	SetBestFiltering(DWORD dwStage); // if possible set anisotropy filtering, or use trilinear

	// Vertex Shader
	void	SaveVertexShader(LPDIRECT3DVERTEXSHADER9 dwShader);
	void	SetVertexShader(LPDIRECT3DVERTEXSHADER9 dwShader);
	void	GetVertexShader(LPDIRECT3DVERTEXSHADER9* pdwShader);
	void	SaveVertexDeclaration(LPDIRECT3DVERTEXDECLARATION9 dwShader);
	void	RestoreVertexDeclaration();
	void	SetVertexDeclaration(LPDIRECT3DVERTEXDECLARATION9 dwShader);
	void	GetVertexDeclaration(LPDIRECT3DVERTEXDECLARATION9* pdwShader);
	void	RestoreVertexShader();

	// Pixel Shader
	void	SavePixelShader(LPDIRECT3DPIXELSHADER9 lpShader);
	void	SetPixelShader(LPDIRECT3DPIXELSHADER9 lpShader);
	void	GetPixelShader(LPDIRECT3DPIXELSHADER9* lppShader);

	void	SaveFVF(DWORD dwFVF);
	void	RestoreFVF();
	void	SetFVF(DWORD dwFVF);
	void	GetFVF(DWORD* dwFVF);
	void	RestorePixelShader();
	// *** These states are cached, but not protected from multiple sends of the same value.
	// Transform
	void SaveTransform(D3DTRANSFORMSTATETYPE Transform, const D3DMATRIX* pMatrix);
	void RestoreTransform(D3DTRANSFORMSTATETYPE Transform);

	// Don't cache-check the transform.  To much to do
	void SetTransform(D3DTRANSFORMSTATETYPE Type, const D3DMATRIX* pMatrix);
	void GetTransform(D3DTRANSFORMSTATETYPE Type, D3DMATRIX* pMatrix);
	// SetVertexShaderConstant
	void RestoreVertexShaderConstant(UINT uiRegister, UINT uiConstantCount);
	// SetPixelShaderConstant
	void RestorePixelShaderConstant(UINT uiRegister, UINT uiConstantCount);
	void SetVertexShaderConstant(DWORD dwRegister, CONST void* pConstantData, DWORD dwConstantCount);
	void SaveVertexShaderConstant(DWORD dwRegister, CONST void* pConstantData, DWORD dwConstantCount);
	void SavePixelShaderConstant(DWORD dwRegister, CONST void* pConstantData, DWORD dwConstantCount);
	void SetPixelShaderConstant(DWORD dwRegister, CONST void* pConstantData, DWORD dwConstantCount);
	void SaveStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT Stride);
	void RestoreStreamSource(UINT StreamNumber);
	void SetStreamSource(UINT StreamNumber, LPDIRECT3DVERTEXBUFFER9 pStreamData, UINT Stride);

	void SaveIndices(LPDIRECT3DINDEXBUFFER9 pIndexData);
	void RestoreIndices();
	void SetIndices(LPDIRECT3DINDEXBUFFER9 pIndexData);

	HRESULT DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
	HRESULT DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
	HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount, INT baseVertexIndex = 0);
	HRESULT DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);

	DWORD GetRenderState(D3DRENDERSTATETYPE Type);
	inline LPDIRECT3DDEVICE9 GetDevice() { return m_lpD3DDev; }

	// Scissor Support
	void SetScissorRect(RECT scissorRect);
	void RestoreScissorRect();
	void SaveScissorRect(RECT scissorRect);

	void UpdateAnisotropy();

private:
	void SetDevice(LPDIRECT3DDEVICE9 lpDevice);

private:
	CStateManagerState	m_ChipState;
	CStateManagerState	m_CurrentState;
	CStateManagerState	m_CopyState;
	TStateID			m_DirtyStates;
	bool				m_bForce;
	bool				m_bScene;
	DWORD				m_dwBestMinFilter;
	DWORD				m_dwBestMagFilter;
	LPDIRECT3DDEVICE9	m_lpD3DDev;

#ifdef _DEBUG
	// Saving Flag
	BOOL				m_bRenderStateSavingFlag[STATEMANAGER_MAX_RENDERSTATES];
	BOOL				m_bTextureStageStateSavingFlag[STATEMANAGER_MAX_STAGES][STATEMANAGER_MAX_TEXTURESTATES];
	BOOL				m_bSamplerStateSavingFlag[STATEMANAGER_MAX_STAGES][STATEMANAGER_MAX_SAMPLERSTATES];
	BOOL				m_bTransformSavingFlag[STATEMANAGER_MAX_TRANSFORMSTATES];
#endif _DEBUG
};

#define STATEMANAGER (CStateManager::GetInstance())