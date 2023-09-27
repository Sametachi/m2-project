#pragma once
#include <string>

class CProperty;
class CPropertyManager : public Singleton<CPropertyManager>
{
public:
    CPropertyManager();
    virtual ~CPropertyManager();

    void Clear();

    bool LoadReservedCRC(const char* c_pszFileName);
    void ReserveCRC(uint32_t dwCRC);
    uint32_t GetUniqueCRC(const char* c_szSeed);

    bool Initialize();
    bool Register(const char* c_pszFileName, CProperty** ppProperty = nullptr);

    bool Get(uint32_t dwCRC, CProperty** ppProperty);
    bool Get(const char* c_pszFileName, CProperty** ppProperty);

    bool Put(CProperty* property);
    bool Erase(const char* c_pszFileName);

protected:
    using TPropertyCRCMap = std::map<uint32_t, std::unique_ptr<CProperty>>;
    using TCRCSet = std::set<uint32_t>;

    TPropertyCRCMap m_PropertyByCRCMap;
    TCRCSet m_ReservedCRCSet;
    bool m_initialized;
};
