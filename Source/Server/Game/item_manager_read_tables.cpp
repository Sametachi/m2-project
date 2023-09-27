#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "desc_client.h"
#include "db.h"
#include "log.h"
#include "skill.h"
#include "text_file_loader.h"
#include "priv_manager.h"
#include "questmanager.h"
#include "unique_item.h"
#include "safebox.h"
#include "blend_item.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "item_manager_private_types.h"
#include "group_text_parse_tree.h"

std::vector<CItemDropInfo> g_vec_pCommonDropItem[MOB::RANK_MAX_NUM];

bool ITEM_MANAGER::ReadCommonDropItemFile(const char* c_pszFileName)
{
	FILE * fp = fopen(c_pszFileName, "r");

	if (!fp)
	{
		SysLog("Cannot open {}", c_pszFileName);
		return false;
	}

	char buf[1024];

	int32_t lines = 0;

	while (fgets(buf, 1024, fp))
	{
		++lines;

		if (!*buf || *buf == '\n')
			continue;

		TDropItem d[MOB::RANK_MAX_NUM];
		char szTemp[64];

		memset(&d, 0, sizeof(d));

		char* p = buf;
		char* p2;

		for (int32_t i = 0; i <= MOB::RANK_S_KNIGHT; ++i)
		{
			for (int32_t j = 0; j < 6; ++j)
			{
				p2 = strchr(p, '\t');

				if (!p2)
					break;

				strlcpy(szTemp, p, MIN(sizeof(szTemp), (p2 - p) + 1));
				p = p2 + 1;

				switch (j)
				{
				case 0: break;
				case 1: str_to_number(d[i].iLvStart, szTemp);	break;
				case 2: str_to_number(d[i].iLvEnd, szTemp);	break;
				case 3: d[i].fPercent = atof(szTemp);	break;
				case 4: strlcpy(d[i].szItemName, szTemp, sizeof(d[i].szItemName));	break;
				case 5: str_to_number(d[i].iCount, szTemp);	break;
				}
			}

			uint32_t dwPct = (uint32_t) (d[i].fPercent * 10000.0f);
			uint32_t dwItemVnum = 0;

			if (!ITEM_MANAGER::GetInstance()->GetVnumByOriginalName(d[i].szItemName, dwItemVnum))
			{
				str_to_number(dwItemVnum, d[i].szItemName);
				if (!ITEM_MANAGER::GetInstance()->GetTable(dwItemVnum))
				{
					SysLog("No such an item (name: {})", d[i].szItemName);
					fclose(fp);
					return false;
				}
			}

			if (d[i].iLvStart == 0)
				continue;

			g_vec_pCommonDropItem[i].push_back(CItemDropInfo(d[i].iLvStart, d[i].iLvEnd, dwPct, dwItemVnum));
		}
	}

	fclose(fp);

	for (int32_t i = 0; i < MOB::RANK_MAX_NUM; ++i)
	{
		std::vector<CItemDropInfo> & v = g_vec_pCommonDropItem[i];
		std::sort(v.begin(), v.end());

		std::vector<CItemDropInfo>::iterator it = v.begin();

		TraceLog("CommonItemDrop rank {}", i);

		while (it != v.end())
		{
			const CItemDropInfo & c = *(it++);
			TraceLog("CommonItemDrop {} {} {} {}", c.m_iLevelStart, c.m_iLevelEnd, c.m_iPercent, c.m_dwVnum);
		}
	}

	return true;
}

bool ITEM_MANAGER::ReadSpecialDropItemFile(const char* c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (uint32_t i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int32_t iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			SysLog("ReadSpecialDropItemFile : Syntax error {} : no vnum, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		PyLog("DROP_ITEM_GROUP {} {}", stName.c_str(), iVnum);

		CTokenVector* pTok;

		std::string stType;
		int32_t type = CSpecialItemGroup::NORMAL;
		if (loader.GetTokenString("type", &stType))
		{
			stl_lowers(stType);
			if (stType == "pct")
			{
				type = CSpecialItemGroup::PCT;
			}
			else if (stType == "quest")
			{
				type = CSpecialItemGroup::QUEST;
				quest::CQuestManager::GetInstance()->RegisterNPCVnum(iVnum);
			}
			else if (stType == "special")
			{
				type = CSpecialItemGroup::SPECIAL;
			}
		}

		if ("attr" == stType)
		{
			CSpecialAttrGroup* pGroup = M2_NEW CSpecialAttrGroup(iVnum);
			for (int32_t k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					uint32_t apply_type = 0;
					int32_t	apply_value = 0;
					str_to_number(apply_type, pTok->at(0).c_str());
					if (0 == apply_type)
					{
						apply_type = FN_get_apply_type(pTok->at(0).c_str());
						if (0 == apply_type)
						{
							SysLog("Invalid APPLY_TYPE {} in Special Item Group Vnum {}", pTok->at(0).c_str(), iVnum);
							return false;
						}
					}
					str_to_number(apply_value, pTok->at(1).c_str());
					if (apply_type > ITEM::MAX_APPLY_NUM)
					{
						SysLog("Invalid APPLY_TYPE {} in Special Item Group Vnum {}", apply_type, iVnum);
						M2_DELETE(pGroup);
						return false;
					}
					pGroup->m_vecAttrs.push_back(CSpecialAttrGroup::CSpecialAttrInfo(apply_type, apply_value));
				}
				else
				{
					break;
				}
			}
			if (loader.GetTokenVector("effect", &pTok))
			{
				pGroup->m_stEffectFileName = pTok->at(0);
			}
			loader.SetParentNode();
			m_map_pSpecialAttrGroup.insert(std::make_pair(iVnum, pGroup));
		}
		else
		{
			CSpecialItemGroup* pGroup = M2_NEW CSpecialItemGroup(iVnum, type);
			for (int32_t k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					const std::string& name = pTok->at(0);
					uint32_t dwVnum = 0;

					if (!GetVnumByOriginalName(name.c_str(), dwVnum))
					{
						if (name == "경험치" || name == "exp")
						{
							dwVnum = CSpecialItemGroup::EXP;
						}
						else if (name == "mob")
						{
							dwVnum = CSpecialItemGroup::MOB;
						}
						else if (name == "slow")
						{
							dwVnum = CSpecialItemGroup::SLOW;
						}
						else if (name == "drain_hp")
						{
							dwVnum = CSpecialItemGroup::DRAIN_HP;
						}
						else if (name == "poison")
						{
							dwVnum = CSpecialItemGroup::POISON;
						}
						else if (name == "group")
						{
							dwVnum = CSpecialItemGroup::MOB_GROUP;
						}
						else
						{
							str_to_number(dwVnum, name.c_str());
							if (!ITEM_MANAGER::GetInstance()->GetTable(dwVnum))
							{
								SysLog("ReadSpecialDropItemFile : there is no item {} : node {}", name.c_str(), stName.c_str());
								M2_DELETE(pGroup);

								return false;
							}
						}
					}

					int32_t iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());
					int32_t iProb = 0;
					str_to_number(iProb, pTok->at(2).c_str());

					int32_t iRarePct = 0;
					if (pTok->size() > 3)
					{
						str_to_number(iRarePct, pTok->at(3).c_str());
					}

					PyLog("        name {} count {} prob {} rare {}", name.c_str(), iCount, iProb, iRarePct);
					pGroup->AddItem(dwVnum, iCount, iProb, iRarePct);

					if (iVnum < 30000)
					{
						m_ItemToSpecialGroup[dwVnum] = iVnum;
					}
					
					continue;
				}

				break;
			}
			loader.SetParentNode();
			if (CSpecialItemGroup::QUEST == type)
			{
				m_map_pQuestItemGroup.insert(std::make_pair(iVnum, pGroup));
			}
			else
			{
				m_map_pSpecialItemGroup.insert(std::make_pair(iVnum, pGroup));
			}
		}
	}

	return true;
}


bool ITEM_MANAGER::ConvSpecialDropItemFile()
{
	char szSpecialItemGroupFileName[256];
	snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
		"%s/special_item_group.txt", LocaleService_GetBasePath().c_str());

	FILE *fp = fopen("special_item_group_vnum.txt", "w");
	if (!fp)
	{
		SysLog("could not open file ({})", "special_item_group_vnum.txt");
		return false;
	}

	CTextFileLoader loader;

	if (!loader.Load(szSpecialItemGroupFileName))
	{
		fclose(fp);
		return false;
	}

	std::string stName;

	for (uint32_t i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int32_t iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			SysLog("ConvSpecialDropItemFile : Syntax error {} : no vnum, node {}", szSpecialItemGroupFileName, stName.c_str());
			loader.SetParentNode();
			fclose(fp);
			return false;
		}

		std::string str;
		int32_t type = 0;
		if (loader.GetTokenString("type", &str))
		{
			stl_lowers(str);
			if (str == "pct")
			{
				type = 1;
			}
		}

		CTokenVector* pTok;

		fprintf(fp, "Group	%s\n", stName.c_str());
		fprintf(fp, "{\n");
		fprintf(fp, "	Vnum	%i\n", iVnum);
		if (type)
			fprintf(fp, "	Type	Pct");

		for (int32_t k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				const std::string& name = pTok->at(0);
				uint32_t dwVnum = 0;

				if (!GetVnumByOriginalName(name.c_str(), dwVnum))
				{
					if (	name == "경험치" ||
						name == "mob" ||
						name == "slow" ||
						name == "drain_hp" ||
						name == "poison" ||
						name == "group")
					{
						dwVnum = 0;
					}
					else
					{
						str_to_number(dwVnum, name.c_str());
						if (!ITEM_MANAGER::GetInstance()->GetTable(dwVnum))
						{
							SysLog("ReadSpecialDropItemFile : there is no item {} : node {}", name.c_str(), stName.c_str());
							fclose(fp);

							return false;
						}
					}
				}

				int32_t iCount = 0;
				str_to_number(iCount, pTok->at(1).c_str());
				int32_t iProb = 0;
				str_to_number(iProb, pTok->at(2).c_str());

				int32_t iRarePct = 0;
				if (pTok->size() > 3)
				{
					str_to_number(iRarePct, pTok->at(3).c_str());
				}

				if (0 == dwVnum)
					fprintf(fp, "	%d	%s	%d	%d\n", k, name.c_str(), iCount, iProb);
				else
					fprintf(fp, "	%d	%u	%d	%d\n", k, dwVnum, iCount, iProb);

				continue;
			}

			break;
		}
		fprintf(fp, "}\n");
		fprintf(fp, "\n");

		loader.SetParentNode();
	}

	fclose(fp);
	return true;
}

bool ITEM_MANAGER::ReadEtcDropItemFile(const char* c_pszFileName)
{
	FILE * fp = fopen(c_pszFileName, "r");

	if (!fp)
	{
		SysLog("Cannot open {}", c_pszFileName);
		return false;
	}

	char buf[512];

	int32_t lines = 0;

	while (fgets(buf, 512, fp))
	{
		++lines;

		if (!*buf || *buf == '\n')
			continue;

		char szItemName[256];
		float fProb = 0.0f;

		strlcpy(szItemName, buf, sizeof(szItemName));
		char*  cpTab = strrchr(szItemName, '\t');

		if (!cpTab)
			continue;

		*cpTab = '\0';
		fProb = atof(cpTab + 1);

		if (!*szItemName || fProb == 0.0f)
			continue;

		uint32_t dwItemVnum;

		if (!ITEM_MANAGER::GetInstance()->GetVnumByOriginalName(szItemName, dwItemVnum))
		{
			SysLog("No such an item (name: {})", szItemName);
			fclose(fp);
			return false;
		}

		m_map_dwEtcItemDropProb[dwItemVnum] = (uint32_t) (fProb * 10000.0f);
		PyLog("ETC_DROP_ITEM: {} prob {}", szItemName, fProb);
	}

	fclose(fp);
	return true;
}

bool ITEM_MANAGER::ReadMonsterDropItemGroup(const char* c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	for (uint32_t i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		std::string stName("");

		loader.GetCurrentNodeName(&stName);

		if (strncmp (stName.c_str(), "kr_", 3) == 0)
		{
				continue;
		}

		loader.SetChildNode(i);

		int32_t iMobVnum = 0;
		int32_t iKillDrop = 0;
		int32_t iLevelLimit = 0;

		std::string strType("");

		if (!loader.GetTokenString("type", &strType))
		{
			SysLog("ReadMonsterDropItemGroup : Syntax error {} : no type (kill|drop), node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		if (!loader.GetTokenInteger("mob", &iMobVnum))
		{
			SysLog("ReadMonsterDropItemGroup : Syntax error {} : no mob vnum, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		if (strType == "kill")
		{
			if (!loader.GetTokenInteger("kill_drop", &iKillDrop))
			{
				SysLog("ReadMonsterDropItemGroup : Syntax error {} : no kill drop count, node {}", c_pszFileName, stName.c_str());
				loader.SetParentNode();
				return false;
			}
		}
		else
		{
			iKillDrop = 1;
		}

		if (strType == "limit")
		{
			if (!loader.GetTokenInteger("level_limit", &iLevelLimit))
			{
				SysLog("ReadmonsterDropItemGroup : Syntax error {} : no level_limit, node {}", c_pszFileName, stName.c_str());
				loader.SetParentNode();
				return false;
			}
		}
		else
		{
			iLevelLimit = 0;
		}

		PyLog("MOB_ITEM_GROUP {} [{}] {} {}", stName.c_str(), strType.c_str(), iMobVnum, iKillDrop);

		if (iKillDrop == 0)
		{
			loader.SetParentNode();
			continue;
		}

		CTokenVector* pTok = nullptr;

		if (strType == "kill")
		{
			CMobItemGroup* pGroup = M2_NEW CMobItemGroup(iMobVnum, iKillDrop, stName);

			for (int32_t k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					std::string& name = pTok->at(0);
					uint32_t dwVnum = 0;

					if (!GetVnumByOriginalName(name.c_str(), dwVnum))
					{
						str_to_number(dwVnum, name.c_str());
						if (!ITEM_MANAGER::GetInstance()->GetTable(dwVnum))
						{
							SysLog("ReadMonsterDropItemGroup : there is no item {} : node {} : vnum {}", name.c_str(), stName.c_str(), dwVnum);
							return false;
						}
					}

					int32_t iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());

					if (iCount<1)
					{
						SysLog("ReadMonsterDropItemGroup : there is no count for item {} : node {} : vnum {}, count {}", name.c_str(), stName.c_str(), dwVnum, iCount);
						return false;
					}

					int32_t iPartPct = 0;
					str_to_number(iPartPct, pTok->at(2).c_str());

					if (iPartPct == 0)
					{
						SysLog("ReadMonsterDropItemGroup : there is no drop percent for item {} : node {} : vnum {}, count {}, pct {}", name.c_str(), stName.c_str(), iPartPct);
						return false;
					}

					int32_t iRarePct = 0;
					str_to_number(iRarePct, pTok->at(3).c_str());
					iRarePct = MINMAX(0, iRarePct, 100);

					PyLog("        {} count {} rare {}", name.c_str(), iCount, iRarePct);
					pGroup->AddItem(dwVnum, iCount, iPartPct, iRarePct);
					continue;
				}

				break;
			}
			m_map_pMobItemGroup.insert(std::map<uint32_t, CMobItemGroup*>::value_type(iMobVnum, pGroup));

		}
		else if (strType == "drop")
		{
			CDropItemGroup* pGroup = nullptr;
			bool bNew = true;
			auto it = m_map_pDropItemGroup.find (iMobVnum);
			if (it == m_map_pDropItemGroup.end())
			{
				pGroup = M2_NEW CDropItemGroup(0, iMobVnum, stName);
			}
			else
			{
				bNew = false;
				CDropItemGroup* pGroup = it->second;
			}

			for (int32_t k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					std::string& name = pTok->at(0);
					uint32_t dwVnum = 0;

					if (!GetVnumByOriginalName(name.c_str(), dwVnum))
					{
						str_to_number(dwVnum, name.c_str());
						if (!ITEM_MANAGER::GetInstance()->GetTable(dwVnum))
						{
							SysLog("ReadDropItemGroup : there is no item {} : node {}", name.c_str(), stName.c_str());
							M2_DELETE(pGroup);

							return false;
						}
					}

					int32_t iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());

					if (iCount < 1)
					{
						SysLog("ReadMonsterDropItemGroup : there is no count for item {} : node {}", name.c_str(), stName.c_str());
						M2_DELETE(pGroup);

						return false;
					}

					float fPercent = atof(pTok->at(2).c_str());

					uint32_t dwPct = (uint32_t)(10000.0f * fPercent);

					PyLog("        name {} pct {} count {}", name.c_str(), dwPct, iCount);
					pGroup->AddItem(dwVnum, dwPct, iCount);

					continue;
				}

				break;
			}
			if (bNew)
				m_map_pDropItemGroup.insert(std::map<uint32_t, CDropItemGroup*>::value_type(iMobVnum, pGroup));

		}
		else if (strType == "limit")
		{
			CLevelItemGroup* pLevelItemGroup = M2_NEW CLevelItemGroup(iLevelLimit);

			for (int32_t k=1; k < 256; k++)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					std::string& name = pTok->at(0);
					uint32_t dwItemVnum = 0;

					if (!GetVnumByOriginalName(name.c_str(), dwItemVnum))
					{
						str_to_number(dwItemVnum, name.c_str());
						if (!ITEM_MANAGER::GetInstance()->GetTable(dwItemVnum))
						{
							M2_DELETE(pLevelItemGroup);
							return false;
						}
					}

					int32_t iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());

					if (iCount < 1)
					{
						M2_DELETE(pLevelItemGroup);
						return false;
					}

					float fPct = atof(pTok->at(2).c_str());
					uint32_t dwPct = (uint32_t)(10000.0f * fPct);

					pLevelItemGroup->AddItem(dwItemVnum, dwPct, iCount);

					continue;
				}

				break;
			}

			m_map_pLevelItemGroup.insert(std::map<uint32_t, CLevelItemGroup*>::value_type(iMobVnum, pLevelItemGroup));
		}
		else if (strType == "thiefgloves")
		{
			CBuyerThiefGlovesItemGroup* pGroup = M2_NEW CBuyerThiefGlovesItemGroup(0, iMobVnum, stName);

			for (int32_t k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					std::string& name = pTok->at(0);
					uint32_t dwVnum = 0;

					if (!GetVnumByOriginalName(name.c_str(), dwVnum))
					{
						str_to_number(dwVnum, name.c_str());
						if (!ITEM_MANAGER::GetInstance()->GetTable(dwVnum))
						{
							SysLog("ReadDropItemGroup : there is no item {} : node {}", name.c_str(), stName.c_str());
							M2_DELETE(pGroup);

							return false;
						}
					}

					int32_t iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());

					if (iCount < 1)
					{
						SysLog("ReadMonsterDropItemGroup : there is no count for item {} : node {}", name.c_str(), stName.c_str());
						M2_DELETE(pGroup);

						return false;
					}

					float fPercent = atof(pTok->at(2).c_str());

					uint32_t dwPct = (uint32_t)(10000.0f * fPercent);

					PyLog("        name {} pct {} count {}", name.c_str(), dwPct, iCount);
					pGroup->AddItem(dwVnum, dwPct, iCount);

					continue;
				}

				break;
			}

			m_map_pGloveItemGroup.insert(std::map<uint32_t, CBuyerThiefGlovesItemGroup*>::value_type(iMobVnum, pGroup));
		}
		else
		{
			SysLog("ReadMonsterDropItemGroup : Syntax error {} : invalid type {} (kill|drop), node {}", c_pszFileName, strType.c_str(), stName.c_str());
			loader.SetParentNode();
			return false;
		}

		loader.SetParentNode();
	}

	return true;
}

bool ITEM_MANAGER::ReadDropItemGroup(const char* c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (uint32_t i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int32_t iVnum;
		int32_t iMobVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			SysLog("ReadDropItemGroup : Syntax error {} : no vnum, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		if (!loader.GetTokenInteger("mob", &iMobVnum))
		{
			SysLog("ReadDropItemGroup : Syntax error {} : no mob vnum, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		PyLog("DROP_ITEM_GROUP {} {}", stName.c_str(), iMobVnum);

		CTokenVector* pTok;

		auto it = m_map_pDropItemGroup.find(iMobVnum);

		CDropItemGroup* pGroup;

		if (it == m_map_pDropItemGroup.end())
			pGroup = M2_NEW CDropItemGroup(iVnum, iMobVnum, stName);
		else
			pGroup = it->second;

		for (int32_t k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				std::string& name = pTok->at(0);
				uint32_t dwVnum = 0;

				if (!GetVnumByOriginalName(name.c_str(), dwVnum))
				{
					str_to_number(dwVnum, name.c_str());
					if (!ITEM_MANAGER::GetInstance()->GetTable(dwVnum))
					{
						SysLog("ReadDropItemGroup : there is no item {} : node {}", name.c_str(), stName.c_str());

						if (it == m_map_pDropItemGroup.end())
							M2_DELETE(pGroup);

						return false;
					}
				}

				float fPercent = atof(pTok->at(1).c_str());

				uint32_t dwPct = (uint32_t)(10000.0f * fPercent);

				int32_t iCount = 1;
				if (pTok->size() > 2)
					str_to_number(iCount, pTok->at(2).c_str());

				if (iCount < 1)
				{
					SysLog("ReadDropItemGroup : there is no count for item {} : node {}", name.c_str(), stName.c_str());

					if (it == m_map_pDropItemGroup.end())
						M2_DELETE(pGroup);

					return false;
				}

				PyLog("        {} {} {}", name.c_str(), dwPct, iCount);
				pGroup->AddItem(dwVnum, dwPct, iCount);
				continue;
			}

			break;
		}

		if (it == m_map_pDropItemGroup.end())
			m_map_pDropItemGroup.insert(std::map<uint32_t, CDropItemGroup*>::value_type(iMobVnum, pGroup));

		loader.SetParentNode();
	}

	return true;
}

bool ITEM_MANAGER::ReadItemVnumMaskTable(const char* c_pszFileName)
{
	FILE *fp = fopen(c_pszFileName, "r");
	if (!fp)
	{
		return false;
	}

	int32_t ori_vnum, new_vnum;
	while (fscanf(fp, "%u %u", &ori_vnum, &new_vnum) != EOF)
	{
		m_map_new_to_ori.insert (TMapDW2DW::value_type (new_vnum, ori_vnum));
	}
	fclose(fp);
	return true;
}
