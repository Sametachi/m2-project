#include "StdAfx.h"
#include <EterLib/ResourceManager.h>
#include <EterBase/lzo.h>
#include <Core/ProtoReader.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "ItemManager.h"

static uint32_t s_adwItemProtoKey[4] =
{
	173217,
	72619434,
	408587239,
	27973291
};

bool CItemManager::SelectItemData(uint32_t dwIndex)
{
	auto f = m_ItemMap.find(dwIndex);

	if (m_ItemMap.end() == f)
	{
		int32_t n = m_vec_ItemRange.size();
		for (int32_t i = 0; i < n; i++)
		{
			CItemData* p = m_vec_ItemRange[i];
			const TItemTable* pTable = p->GetTable();
			if ((pTable->dwVnum < dwIndex) &&
				dwIndex < (pTable->dwVnum + pTable->dwVnumRange))
			{
				m_pSelectedItemData = p;
				return true;
			}
		}
		return false;
	}

	m_pSelectedItemData = f->second;

	return true;
}

CItemData* CItemManager::GetSelectedItemDataPointer()
{
	return m_pSelectedItemData;
}

bool CItemManager::GetItemDataPointer(uint32_t dwItemID, CItemData** ppItemData)
{
	if (0 == dwItemID)
		return false;

	auto f = m_ItemMap.find(dwItemID);

	if (m_ItemMap.end() == f)
	{
		int32_t n = m_vec_ItemRange.size();
		for (int32_t i = 0; i < n; i++)
		{
			CItemData* p = m_vec_ItemRange[i];
			const TItemTable* pTable = p->GetTable();
			if ((pTable->dwVnum < dwItemID) &&
				dwItemID < (pTable->dwVnum + pTable->dwVnumRange))
			{
				*ppItemData = p;
				return true;
			}
		}
		return false;
	}

	*ppItemData = f->second;

	return true;
}

bool CItemManager::GetItemDataPointer(const char* c_szInput, CItemData** ppItemData)
{
	std::string strInput = c_szInput;
	boost::algorithm::to_lower(strInput);

	for (const auto& [dwKey, pItemData] : m_ItemMap)
	{
		std::string strItemName = pItemData->GetName();
		boost::algorithm::to_lower(strItemName);

		// Ignore names that are shorter than the input
		if (strItemName.length() < strInput.length())
			continue;

		if (!strItemName.substr(0, strInput.length()).compare(strInput))
		{
			*ppItemData = pItemData;
			return true;
		}
	}

	for (const auto& pItemData : m_vec_ItemRange)
	{
		std::string strItemName = pItemData->GetName();
		boost::algorithm::to_lower(strItemName);

		// Ignore names that are shorter than the input
		if (strItemName.length() < strInput.length())
			continue;

		if (!strItemName.substr(0, strInput.length()).compare(strInput))
		{
			*ppItemData = pItemData;
			return true;
		}
	}

	ppItemData = nullptr;
	return false;
}

CItemData* CItemManager::GetProto(int32_t dwItemID)
{
	if (0 == dwItemID)
		return 0;

	auto find = m_ItemMap.find(dwItemID);

	if (m_ItemMap.end() == find)
	{
		int32_t n = m_vec_ItemRange.size();
		for (int32_t i = 0; i < n; i++)
		{
			CItemData* itemData = m_vec_ItemRange[i];
			const TItemTable* pTable = itemData->GetTable();
			if ((pTable->dwVnum < dwItemID) && dwItemID < (pTable->dwVnum + pTable->dwVnumRange))
			{
				return itemData;
			}
		}
		ConsoleLog("CItemManager::GetItemDataPointer - FIND ERROR [{0}]", dwItemID);
		return nullptr;
	}

	return find->second;
}

CItemData* CItemManager::MakeItemData(uint32_t dwIndex)
{
	auto f = m_ItemMap.find(dwIndex);

	if (m_ItemMap.end() == f)
	{
		CItemData* pItemData = CItemData::New();

		m_ItemMap.emplace(dwIndex, pItemData);

		return pItemData;
	}

	return f->second;
}

////////////////////////////////////////////////////////////////////////////////////////
// Load Item Table

bool CItemManager::LoadItemList(const char * c_szFileName)
{
	auto vfs = CallFS().Open(c_szFileName);
	if (!vfs)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}
	const uint32_t size = vfs->GetSize();

	storm::View data(storm::GetDefaultAllocator());
	vfs->GetView(0, data, size);

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(std::string_view(reinterpret_cast<const char*>(data.GetData()), size));

	CTokenVector TokenVector;
    for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLine(i, &TokenVector, "\t"))
			continue;

		if (!(TokenVector.size() == 3 || TokenVector.size() == 4))
		{
			TraceLog(" CItemManager::LoadItemList({}) - StrangeLine in {}\n", c_szFileName, i);
			continue;
		}

		const std::string & c_rstrID = TokenVector[0];
		//const std::string & c_rstrType = TokenVector[1];
		const std::string & c_rstrIcon = TokenVector[2];

		uint32_t dwItemVNum=atoi(c_rstrID.c_str());

		CItemData * pItemData = MakeItemData(dwItemVNum);
		{
			if (4 == TokenVector.size())
			{
				const std::string & c_rstrModelFileName = TokenVector[3];
				pItemData->SetDefaultItemData(c_rstrIcon.c_str(), c_rstrModelFileName.c_str());
			}
			else
			{
				pItemData->SetDefaultItemData(c_rstrIcon.c_str());
			}
		}
	}

	return true;
}

const std::string& __SnapString(const std::string& c_rstSrc, std::string& rstTemp)
{
	uint32_t uSrcLen = c_rstSrc.length();
	if (uSrcLen < 2)
		return c_rstSrc;

	if (c_rstSrc[0] != '"')
		return c_rstSrc;

	uint32_t uLeftCut = 1;

	uint32_t uRightCut = uSrcLen;
	if (c_rstSrc[static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(uSrcLen) - 1] == '"')
		uRightCut = uSrcLen - 1;

	rstTemp = c_rstSrc.substr(uLeftCut, uRightCut - uLeftCut);
	return rstTemp;
}

bool CItemManager::LoadItemDesc(const char* c_szFileName)
{
	auto vfs = CallFS().Open(c_szFileName);
	if (!vfs)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}
	const uint32_t size = vfs->GetSize();

	storm::View data(storm::GetDefaultAllocator());
	vfs->GetView(0, data, size);

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(std::string_view(reinterpret_cast<const char*>(data.GetData()), size));

	CTokenVector args;
	for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLineByTab(i, &args))
			continue;

		if (args.size() != 2 && args.size() != 3 && args.size() != 4)
		{
			SysLog("{0}:{1}: expected 2/3/4 tokens, got {2}", c_szFileName, i, args.size());
			continue;
		}

		try
		{
			uint32_t vnum = std::stoul(args[0]);
			const auto it = m_ItemMap.find(vnum);
			if (m_ItemMap.end() == it)
			{
				WarnLog("{0}:{1}: vnum {2} doesn't exist", c_szFileName, i, vnum);
				continue;
			}

			it->second->SetName(args[1]);

			if (args.size() > 2)
				it->second->SetDescription(args[2]);

			if (args.size() > 3)
				it->second->SetSummary(args[3]);
		}
		catch (const std::invalid_argument& ia)
		{
			SysLog("{0}:{1}: has invalid token {2}", c_szFileName, i, args[0]);
			return false;
		}
		catch (const std::out_of_range& oor)
		{
			SysLog("{0}:{1}: has out of range token {2}", c_szFileName, i, args[0]);
			return false;
		}
	}
	return true;
}

uint32_t GetHashCode(const char* pString)
{
	uint32_t i, len;
	uint32_t ch;
	uint32_t result;

	len = strlen(pString);
	result = 5381;
	for (i = 0; i < len; i++)
	{
		ch = (uint32_t)pString[i];
		result = ((result << 5) + result) + ch; // hash * 33 + ch
	}

	return result;
}

bool CItemManager::LoadItemTable(const char* c_szFileName)
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), c_szFileName);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(vfs_string.value());

	CTokenVector TokenVector;

	char szName[64 + 1];
	std::map<uint32_t, uint32_t> itemNameMap;

	for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		// Ignore comments
		if (textFileLoader.GetLineString(i).find("#") == 0 ||
			textFileLoader.GetLineString(i).find("//") == 0 ||
			textFileLoader.GetLineString(i).find("--") == 0)
			continue;

		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		TItemTable table{};
		if (!ReadItemProto(TokenVector, table))
		{
			SysLog("Failed to load item proto on line {0}", i + 1);
			continue;
		}

		CItemData* pItemData;
		uint32_t dwVnum = table.dwVnum;

		auto f = m_ItemMap.find(dwVnum);
		if (m_ItemMap.end() == f)
		{
			_snprintf_s(szName, sizeof(szName), "icon/item/%05u.tga", dwVnum);

			if (CResourceManager::GetInstance()->IsFileExist(szName) == false)
			{
				auto itVnum = itemNameMap.find(GetHashCode(table.szName));

				if (itVnum != itemNameMap.end())
					_snprintf_s(szName, sizeof(szName), "icon/item/%05u.tga", itVnum->second);
				else
					_snprintf_s(szName, sizeof(szName), "icon/item/%05u.tga", dwVnum - (dwVnum % 10));

				if (CResourceManager::GetInstance()->IsFileExist(szName) == false)
				{
#ifdef _DEBUG
					SysLog("{0}, {1} cannot find icon file. setting to default.", table.szLocaleName, dwVnum);
#endif
					const uint32_t EmptyBowl = 27995;
					_snprintf_s(szName, sizeof(szName), "icon/item/%05d.tga", EmptyBowl);
				}
			}

			pItemData = CItemData::New();

			pItemData->SetDefaultItemData(szName);
			m_ItemMap.emplace(dwVnum, pItemData);
		}
		else
		{
			pItemData = f->second;
		}
		if (itemNameMap.find(GetHashCode(table.szName)) == itemNameMap.end())
			itemNameMap.emplace(GetHashCode(table.szName), table.dwVnum);
		pItemData->SetItemTableData(&table);
		if (0 != table.dwVnumRange)
			m_vec_ItemRange.emplace_back(pItemData);
	}

	return true;
}

void CItemManager::Destroy()
{
	for (auto& i : m_ItemMap)
		CItemData::Delete(i.second);

	m_ItemMap.clear();
}

CItemManager::CItemManager() : m_pSelectedItemData(nullptr)
{
}

CItemManager::~CItemManager()
{
	Destroy();
}
