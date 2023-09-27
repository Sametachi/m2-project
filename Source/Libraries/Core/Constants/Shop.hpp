#pragma once
#include <Core/Constants/Item.hpp>
#include <Common/tables.h>

typedef struct packet_shop_item
{
    uint32_t     			vnum;
    uint32_t     			price;
    uint8_t     			count;
	uint8_t					display_pos;
	int32_t					alSockets[ITEM::SOCKET_MAX_NUM];
    TPlayerItemAttribute	aAttr[ITEM::ATTRIBUTE_MAX_NUM];
} TShopItemData;