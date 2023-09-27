#pragma once

#include <Core/Tables.hpp>
#include <Core/Constants/Item.hpp>

#include <iostream>
#include <map>

#include "CsvReader.h"

void putItemIntoTable();

int32_t get_Item_Type_Value(std::string inputString);
int32_t get_Item_SubType_Value(int32_t type_value, std::string inputString);
int32_t get_Item_AntiFlag_Value(std::string inputString);
int32_t get_Item_Flag_Value(std::string inputString);
int32_t get_Item_WearFlag_Value(std::string inputString);
int32_t get_Item_Immune_Value(std::string inputString);
int32_t get_Item_LimitType_Value(std::string inputString);
int32_t get_Item_ApplyType_Value(std::string inputString);

int32_t get_Mob_Rank_Value(std::string inputString);
int32_t get_Mob_Type_Value(std::string inputString);
int32_t get_Mob_BattleType_Value(std::string inputString);

int32_t get_Mob_Size_Value(std::string inputString);
int32_t get_Mob_AIFlag_Value(std::string inputString);
int32_t get_Mob_RaceFlag_Value(std::string inputString);
int32_t get_Mob_ImmuneFlag_Value(std::string inputString);

bool Set_Proto_Mob_Table(TMobTable *mobTable, cCsvTable &csvTable, std::map<int32_t,const char*> &nameMap);
bool Set_Proto_Item_Table(TItemTable *itemTable, cCsvTable &csvTable,std::map<int32_t,const char*> &nameMap);