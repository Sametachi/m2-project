#pragma once
#include <Core/Race/RaceMotionConstants.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <vector>

#pragma pack(push, 1)
union MotionId
{
	struct 
	{
		uint8_t mode;
		uint16_t index;
		uint8_t subIndex;
	};

	uint32_t key;
};
#pragma pack(pop)

BOOST_FORCEINLINE MotionId MakeMotionId(uint8_t mode, uint16_t index, uint8_t subIndex = 0)
{
	MotionId id{};
	id.mode = mode;
	id.index = index;
	id.subIndex = subIndex;
	return id;
}

BOOST_FORCEINLINE MotionId MakeMotionId(uint32_t key)
{
	MotionId id{};
	id.key = key;
	return id;
}

BOOST_FORCEINLINE uint32_t MakeMotionKey(uint8_t mode, uint16_t index, uint8_t subIndex = 0)
{
	MotionId id{};
	id.mode = mode;
	id.index = index;
	id.subIndex = subIndex;
	return id.key;
}
