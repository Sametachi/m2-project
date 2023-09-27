#pragma once
#ifndef DIRECT3D_VERSION
#define DIRECT3D_VERSION 0x900
#endif
#include <d3d9.h>
#include <cstddef>
#include <cstdint>

namespace DirectX
{
#ifndef WIC_LOADER_FLAGS_DEFINED
#define WIC_LOADER_FLAGS_DEFINED
    enum WIC_LOADER_FLAGS : uint32_t
    {
        WIC_LOADER_DEFAULT      = 0,
        WIC_LOADER_MIP_AUTOGEN  = 0x8,
        WIC_LOADER_FIT_POW2     = 0x20,
        WIC_LOADER_MAKE_SQUARE  = 0x40,
        WIC_LOADER_FORCE_RGBA32 = 0x80,
    };
#endif

    // Standard version
    HRESULT CreateWICTextureFromMemory(
        _In_ LPDIRECT3DDEVICE9 d3dDevice,
        _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
        _In_ size_t wicDataSize,
        _Outptr_ LPDIRECT3DTEXTURE9* texture,
        _In_ size_t maxsize = 0,
        _In_ unsigned int loadFlags = 0) noexcept;

    HRESULT CreateWICTextureFromFile(
        _In_ LPDIRECT3DDEVICE9 d3dDevice,
        _In_z_ const wchar_t* fileName,
        _Outptr_ LPDIRECT3DTEXTURE9* texture,
        _In_ size_t maxsize = 0,
        _In_ unsigned int loadFlags = 0) noexcept;

    // Extended version
    HRESULT CreateWICTextureFromMemoryEx(
        _In_ LPDIRECT3DDEVICE9 d3dDevice,
        _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
        _In_ size_t wicDataSize,
        _In_ size_t maxsize,
        _In_ DWORD usage,
        _In_ D3DPOOL pool,
        _In_ unsigned int loadFlags,
        _Outptr_ LPDIRECT3DTEXTURE9* texture) noexcept;

    HRESULT CreateWICTextureFromFileEx(
        _In_ LPDIRECT3DDEVICE9 d3dDevice,
        _In_z_ const wchar_t* fileName,
        _In_ size_t maxsize,
        _In_ DWORD usage,
        _In_ D3DPOOL pool,
        _In_ unsigned int loadFlags,
        _Outptr_ LPDIRECT3DTEXTURE9* texture) noexcept;
}
