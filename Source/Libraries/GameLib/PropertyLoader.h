#pragma once
#include <VFE/Include/VFE.hpp>

class CPropertyManager;
class CPropertyLoader : public CDir
{
public:
    CPropertyLoader();
    virtual ~CPropertyLoader() = default;

    void SetPropertyManager(CPropertyManager* pPropertyManager);
    uint32_t RegisterFile(const char* c_szPathName, const char* c_szFileName);

    virtual bool OnFolder(const char* c_szFilter, const char* c_szPathName, const char* c_szFileName);
    virtual bool OnFile(const char* c_szPathName, const char* c_szFileName);

protected:
    CPropertyManager* m_pPropertyManager;
};
