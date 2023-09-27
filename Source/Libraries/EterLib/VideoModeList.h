#pragma once
#include "../EterBase/VideoMode.h"
#include <stack>

class VideoModeList
{
private:
    std::vector<VideoMode> mModeList;

public:
    VideoModeList();
    ~VideoModeList();

    bool enumerate(LPDIRECT3D9 pD3D);

    VideoMode *item(size_t index);
    size_t count();

    VideoMode *item(const std::string &name);
};