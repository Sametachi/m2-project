#include "StdAfx.h"
#include "VideoMode.h"
#include <string>
#include <fmt/format.h>

std::string VideoMode::getDescription() const
{
    unsigned int colourDepth = 16;
    if (mDisplayMode.Format == D3DFMT_X8R8G8B8 || mDisplayMode.Format == D3DFMT_A8R8G8B8 || mDisplayMode.Format == D3DFMT_R8G8B8)
        colourDepth = 32;

    return fmt::format("{}x{} {}bpp", mDisplayMode.Width, mDisplayMode.Height, colourDepth);
}

uint32_t VideoMode::getColourDepth() const
{
    uint32_t colourDepth = 16;
    if (mDisplayMode.Format == D3DFMT_X8R8G8B8 || mDisplayMode.Format == D3DFMT_A8R8G8B8 || mDisplayMode.Format == D3DFMT_R8G8B8)
        colourDepth = 32;

    return colourDepth;
}
