#pragma once

class CHARACTER;

class CBuffOnAttributes
{
public:
	CBuffOnAttributes(LPCHARACTER pOwner, uint8_t m_point_type, std::vector <uint8_t>* vec_buff_targets);
	~CBuffOnAttributes();

	// If it is an item corresponding to m_p_vec_buff_wear_targets while being equipped, the effect attached to the item is removed.
	void RemoveBuffFromItem(LPITEM pItem);
	// In the case of an item corresponding to m_p_vec_buff_wear_targets, an effect for the attribute of the item is added.
	void AddBuffFromItem(LPITEM pItem);
	// Change m_bBuffValue and change the value of the buff.
	void ChangeBuffValue(uint8_t bNewValue);
	// Since CHRACTRE::ComputePoints initializes and recalculates the attribute values,
	// Force the buff attribute values to the owner.
	void GiveAllAttributes();

	// Sum up the attributes of all items corresponding to m_p_vec_buff_wear_targets by type,
	// Gives (m_bBuffValue)% of the attributes as buffs.
	bool On(uint8_t bValue);
	// After removing the buff, reset
	void Off();

	void Initialize();
private:
	LPCHARACTER m_pBuffOwner;
	uint8_t m_bPointType;
	uint8_t m_bBuffValue;
	std::vector <uint8_t>* m_p_vec_buff_wear_targets;
	
	// apply_type, apply_value pair's map
	typedef std::map <uint8_t, int32_t> TMapAttr;
	// The attributes of all items corresponding to m_p_vec_buff_wear_targets are summed up by type and have.
	TMapAttr m_map_additional_attrs;

};
