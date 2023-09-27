#pragma once
#include <minwinbase.h>
#include <d3d9types.h>
#include <stack>

static uint32_t modeCount = 0;

class VideoMode
{
private:
    D3DDISPLAYMODE mDisplayMode;
    uint32_t modeNumber;

public:
    VideoMode()
    {
        modeNumber = ++modeCount;
        ZeroMemory(&mDisplayMode, sizeof(D3DDISPLAYMODE));
    }

    VideoMode(const VideoMode& ob)
    {
        modeNumber = ++modeCount;
        mDisplayMode = ob.mDisplayMode;
    }

    VideoMode(D3DDISPLAYMODE d3ddm)
    {
        modeNumber = ++modeCount;
        mDisplayMode = d3ddm;
    }

    ~VideoMode()
    {
        modeCount--;
    }

    uint32_t getWidth() const
    {
        return mDisplayMode.Width;
    }

    uint32_t getHeight() const
    {
        return mDisplayMode.Height;
    }

    D3DFORMAT getFormat() const
    {
        return mDisplayMode.Format;
    }

    uint32_t getRefreshRate() const
    {
        return mDisplayMode.RefreshRate;
    }

    uint32_t getColourDepth() const;

    D3DDISPLAYMODE getDisplayMode() const
    {
        return mDisplayMode;
    }

    void increaseRefreshRate(uint32_t rr)
    {
        mDisplayMode.RefreshRate = rr;
    }

    std::string getDescription() const;
};
