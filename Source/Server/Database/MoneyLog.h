#pragma once

#include <map>

class CMoneyLog : public Singleton<CMoneyLog>
{
public:
	CMoneyLog();
	virtual ~CMoneyLog();

	void Save();
	void AddLog(uint8_t bType, uint32_t dwVnum, int32_t iGold);

private:
	std::map<uint32_t, int32_t> m_MoneyLogContainer[MONEY_LOG_TYPE_MAX_NUM];
};
