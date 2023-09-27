#include "StdAfx.h"
#include "VideoModeList.h"
#include "../EterPythonLib/PythonGraphic.h"

VideoModeList::VideoModeList()
{

}

VideoModeList::~VideoModeList()
{
    mModeList.clear();
}

bool VideoModeList::enumerate(LPDIRECT3D9 pD3D)
{
    uint32_t iMode;

    for (iMode = 0; iMode < pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_R5G6B5); iMode++)
    {
        D3DDISPLAYMODE displayMode;
        pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_R5G6B5, iMode, &displayMode);

        // Filter out low-resolutions
        if (displayMode.Width < 640 || displayMode.Height < 400)
            continue;

        // Check to see if it is already in the list (to filter out refresh rates)
        bool found = false;
        for (auto it = mModeList.begin(); it != mModeList.end(); it++)
        {
            D3DDISPLAYMODE oldDisp = it->getDisplayMode();
            if (oldDisp.Width == displayMode.Width &&
                oldDisp.Height == displayMode.Height &&
                oldDisp.Format == displayMode.Format)
            {
                // Check refresh rate and favour higher if poss
                if (oldDisp.RefreshRate < displayMode.RefreshRate)
                    it->increaseRefreshRate(displayMode.RefreshRate);
                found = true;
                break;
            }
        }

        if (!found)
            mModeList.emplace_back(VideoMode(displayMode));
    }

    for (iMode = 0; iMode < pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8); iMode++)
    {
        D3DDISPLAYMODE displayMode;
        pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, iMode, &displayMode);

        // Filter out low-resolutions
        if (displayMode.Width < 1024 || displayMode.Height < 768)
            continue;

        // Check to see if it is already in the list (to filter out refresh rates)
        bool found = false;;
        for (auto it = mModeList.begin(); it != mModeList.end(); it++)
        {
            D3DDISPLAYMODE oldDisp = it->getDisplayMode();
            if (oldDisp.Width == displayMode.Width &&
                oldDisp.Height == displayMode.Height &&
                oldDisp.Format == displayMode.Format)
            {
                // Check refresh rate and favour higher if poss
                if (oldDisp.RefreshRate < displayMode.RefreshRate)
                    it->increaseRefreshRate(displayMode.RefreshRate);
                found = true;
                break;
            }
        }

        if (!found)
            mModeList.emplace_back(displayMode);
    }

    return true;
}

size_t VideoModeList::count()
{
    return mModeList.size();
}

VideoMode* VideoModeList::item(size_t index)
{
    auto p = mModeList.begin();

    return &p[index];
}

VideoMode* VideoModeList::item(const std::string& name)
{
    auto it = mModeList.begin();
    if (it == mModeList.end())
        return nullptr;

    for (; it != mModeList.end(); ++it)
    {
        if (it->getDescription() == name)
            return &(*it);
    }

    return nullptr;
}