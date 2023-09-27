#include "StdAfx.h"
#include <VFE/Include/VFE.hpp>

static std::string GetVfsPythonFile(std::string filename)
{
    auto vfs = CallFS().Open(filename);
    if (!vfs)
        return "";

    const uint32_t size = vfs->GetSize();

    std::string buf;
    buf.resize(size);
    vfs->Read(0, buf.data(), size);
    return buf;
}

PYBIND11_EMBEDDED_MODULE(pack, m)
{
    m.def("GetVfsFile", GetVfsPythonFile);
}
