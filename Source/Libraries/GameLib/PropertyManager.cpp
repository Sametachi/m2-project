#include "StdAfx.h"
#include "PropertyManager.h"
#include "Property.h"
#include <VFE/Include/VFE.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

CPropertyManager::CPropertyManager()
{
    m_initialized = false;
}

CPropertyManager::~CPropertyManager()
{
    Clear();
}

bool CPropertyManager::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    const char* listFilename = "Globals/property.xml";

    auto vfs = CallFS().Open(listFilename);
    if (!vfs)
    {
        SysLog("Failed to load {0}", listFilename);
        return false;
    }
    const uint32_t size = vfs->GetSize();

    storm::View data(storm::GetDefaultAllocator());
    vfs->GetView(0, data, size);

    const auto pszXml = new char[size + 1];
    std::memcpy(pszXml, data.GetData(), size);
    pszXml[size] = 0;
    std::stringstream kXml;
    kXml << pszXml;
    delete[] pszXml;

    try
    {
        boost::property_tree::ptree propertyTree;
        read_xml(kXml, propertyTree);

        for (auto& v : propertyTree.get_child("PropertyList"))
        {
            if (v.first == "Property")
            {
                auto pProperty = std::make_unique<CProperty>(v.second.get<std::string>("<xmlattr>.filename").c_str());
                if (!pProperty->ReadFromXML(v.second.get<std::string>("<xmlattr>.crc").c_str()))
                {
                    SysLog("CPropertyManager::Initialize: Cannot register '{0}'!", v.second.get<std::string>("<xmlattr>.filename").c_str());
                    continue;
                }

                uint32_t crc = pProperty->GetCRC();
                CProperty* rawPtr = pProperty.get();

                auto itor = m_PropertyByCRCMap.find(crc);
                if (m_PropertyByCRCMap.end() != itor)
                {
                    ConsoleLog("SKIPPED! Property already registered {0} to {1}", itor->second->GetFileName(), v.second.get<std::string>("<xmlattr>.filename").c_str());
                    itor->second = std::move(pProperty);
                }
                else
                    m_PropertyByCRCMap.emplace(crc, std::move(pProperty));

                for (auto& s : v.second)
                {
                    if (s.first == "<xmlattr>")
                    {
                        for (auto& d : s.second)
                        {
                            CTokenVector kVec;
                            kVec.emplace_back(d.second.data());
                            rawPtr->PutVector(d.first.c_str(), kVec);
                        }
                    }
                }
            }
        }
    }
    catch (std::exception& e)
    {
        SysLog("CPropertyManager::Initialize: {0}", e.what());
        return false;
    }

    m_initialized = true;
    return true;
}

bool CPropertyManager::LoadReservedCRC(const char* c_pszFileName)
{
    auto vfs_string = CallFS().LoadFileToString(CallFS(), c_pszFileName);
    if (!vfs_string)
    {
        SysLog("Failed to load {0}", c_pszFileName);
        return false;
    }

    CMemoryTextFileLoader textFileLoader;
    textFileLoader.Bind(vfs_string.value());

    for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
    {
        std::string pszLine = textFileLoader.GetLineString(i);

        if (pszLine.empty())
            continue;

        ReserveCRC(std::stoul(pszLine));
    }

    return true;
}

void CPropertyManager::ReserveCRC(uint32_t dwCRC)
{
    m_ReservedCRCSet.emplace(dwCRC);
}

uint32_t CPropertyManager::GetUniqueCRC(const char* c_szSeed)
{
    std::string stTmp = c_szSeed;

    while (true)
    {
        uint32_t dwCRC = GetCRC32(stTmp.c_str(), stTmp.length());

        if (m_ReservedCRCSet.find(dwCRC) == m_ReservedCRCSet.end() &&
            m_PropertyByCRCMap.find(dwCRC) == m_PropertyByCRCMap.end())
            return dwCRC;

        char szAdd[2];
        _snprintf_s(szAdd, sizeof(szAdd), "%u", random() % 10);
        stTmp += szAdd;
    }
}

bool CPropertyManager::Register(const char* c_pszFileName, CProperty** ppProperty)
{
    auto vfs = CallFS().Open(c_pszFileName);
    if (!vfs)
        return false;

    const uint32_t size = vfs->GetSize();

    storm::View data(storm::GetDefaultAllocator());
    vfs->GetView(0, data, size);

    auto pProperty = std::make_unique<CProperty>(c_pszFileName);
    if (!pProperty->ReadFromMemory(data.GetData(), size, c_pszFileName))
    {
        return false;
    }

    uint32_t dwCRC = pProperty->GetCRC();

    CProperty* rawPtr = pProperty.get();

    auto itor = m_PropertyByCRCMap.find(dwCRC);

    if (m_PropertyByCRCMap.end() != itor)
    {
        SysLog("Property already registered, replace {0} to {1}", itor->second->GetFileName(), c_pszFileName);
        itor->second = std::move(pProperty);
    }
    else
        m_PropertyByCRCMap.emplace(dwCRC, std::move(pProperty));

    if (ppProperty)
        *ppProperty = rawPtr;

    return true;
}

bool CPropertyManager::Get(const char* c_pszFileName, CProperty** ppProperty)
{
    return Register(c_pszFileName, ppProperty);
}

bool CPropertyManager::Get(uint32_t dwCRC, CProperty** ppProperty)
{
    auto itor = m_PropertyByCRCMap.find(dwCRC);

    if (m_PropertyByCRCMap.end() == itor)
        return false;

    *ppProperty = itor->second.get();
    return true;
}

bool CPropertyManager::Put(CProperty* property)
{
    auto r = m_PropertyByCRCMap.emplace(std::make_pair(property->GetCRC(), property));
    if (!r.second)
    {
        SysLog("Property already registered, replace {0} to {1}", r.first->second->GetFileName(), property->GetFileName());
        r.first->second.reset(property);
    }

    return true;
}

bool CPropertyManager::Erase(const char* c_pszFileName)
{
    CProperty* property;
    if (!Get(c_pszFileName, &property))
        return false;

    m_PropertyByCRCMap.erase(property->GetCRC());
    delete property;

    ::DeleteFileA(c_pszFileName);
    return false;
}

void CPropertyManager::Clear()
{
}
