#include "StdAfx.h"
#include "PythonExchange.h"

static void exchangeInitTrading()
{
	CPythonExchange::GetInstance()->End();

}

static bool exchangeisTrading()
{
	return  CPythonExchange::GetInstance()->isTrading();
}

static uint32_t exchangeGetElkFromSelf()
{
	return  CPythonExchange::GetInstance()->GetElkFromSelf();
}

static uint32_t exchangeGetElkFromTarget()
{
	return  CPythonExchange::GetInstance()->GetElkFromTarget();
}

static uint32_t exchangeGetItemVnumFromSelf(uint8_t pos)
{
	return  CPythonExchange::GetInstance()->GetItemVnumFromSelf(pos);
}

static uint32_t exchangeGetItemVnumFromTarget(uint8_t pos)
{
	return  CPythonExchange::GetInstance()->GetItemVnumFromTarget(pos);
}

static uint32_t exchangeGetItemCountFromSelf(uint8_t pos)
{
	return  CPythonExchange::GetInstance()->GetItemCountFromSelf(pos);
}

static uint32_t exchangeGetItemCountFromTarget(uint8_t pos)
{
	return  CPythonExchange::GetInstance()->GetItemCountFromTarget(pos);
}

static bool exchangeGetAcceptFromSelf()
{
	return  CPythonExchange::GetInstance()->GetAcceptFromSelf();
}

static bool exchangeGetAcceptFromTarget()
{
	return  CPythonExchange::GetInstance()->GetAcceptFromTarget();
}

static std::string exchangeGetNameFromSelf()
{
	return  CPythonExchange::GetInstance()->GetNameFromSelf();
}

static std::string exchangeGetNameFromTarget()
{
	return  CPythonExchange::GetInstance()->GetNameFromTarget();
}

static uint32_t exchangeGetItemMetinSocketFromTarget(uint8_t pos, int32_t iMetinSocketPos)
{
	return CPythonExchange::GetInstance()->GetItemMetinSocketFromTarget(pos, iMetinSocketPos);
}

static uint32_t exchangeGetItemMetinSocketFromSelf(uint8_t pos, int32_t iMetinSocketPos)
{
	return CPythonExchange::GetInstance()->GetItemMetinSocketFromSelf(pos, iMetinSocketPos);
}

static std::tuple<uint8_t, int16_t> exchangeGetItemAttributeFromTarget(uint8_t pos, int iAttrSlotPos)
{

	uint8_t byType;
	int16_t sValue;
	CPythonExchange::GetInstance()->GetItemAttributeFromTarget(pos, iAttrSlotPos, &byType, &sValue);

	return std::make_tuple( byType, sValue);
}

static std::tuple<uint8_t, int16_t> exchangeGetItemAttributeFromSelf(uint8_t pos, int32_t iAttrSlotPos)
{

	uint8_t byType;
	int16_t sValue;
	CPythonExchange::GetInstance()->GetItemAttributeFromSelf(pos, iAttrSlotPos, &byType, &sValue);

	return std::make_tuple( byType, sValue);
}

static bool exchangeGetElkMode()
{
	return  CPythonExchange::GetInstance()->GetElkMode();
}

static void exchangeSetElkMode(bool elk_mode)
{
	CPythonExchange::GetInstance()->SetElkMode(elk_mode);

}



PYBIND11_EMBEDDED_MODULE(exchange, m)
{
	m.def("InitTrading",	exchangeInitTrading);
	m.def("isTrading",	exchangeisTrading);
	m.def("GetElkFromSelf",	exchangeGetElkFromSelf);
	m.def("GetElkFromTarget",	exchangeGetElkFromTarget);
	m.def("GetItemVnumFromSelf",	exchangeGetItemVnumFromSelf);
	m.def("GetItemVnumFromTarget",	exchangeGetItemVnumFromTarget);
	m.def("GetItemCountFromSelf",	exchangeGetItemCountFromSelf);
	m.def("GetItemCountFromTarget",	exchangeGetItemCountFromTarget);
	m.def("GetAcceptFromSelf",	exchangeGetAcceptFromSelf);
	m.def("GetAcceptFromTarget",	exchangeGetAcceptFromTarget);
	m.def("GetNameFromSelf",	exchangeGetNameFromSelf);
	m.def("GetNameFromTarget",	exchangeGetNameFromTarget);
	m.def("GetItemMetinSocketFromTarget",	exchangeGetItemMetinSocketFromTarget);
	m.def("GetItemMetinSocketFromSelf",	exchangeGetItemMetinSocketFromSelf);
	m.def("GetItemAttributeFromTarget",	exchangeGetItemAttributeFromTarget);
	m.def("GetItemAttributeFromSelf",	exchangeGetItemAttributeFromSelf);
	m.def("GetElkMode",	exchangeGetElkMode);
	m.def("SetElkMode",	exchangeSetElkMode);

	m.attr("EXCHANGE_ITEM_MAX_NUM") = int32_t(CPythonExchange::EXCHANGE_ITEM_MAX_NUM);
}
