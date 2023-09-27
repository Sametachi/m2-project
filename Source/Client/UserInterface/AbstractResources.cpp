#include "StdAfx.h"
#include "AbstractResources.h"
#include "PythonNetworkStream.h"
#include <boost/algorithm/string.hpp>
#include <Core/Race/RaceConstans.hpp>

/* Loading Layers */
bool AbstractResources::LoadDefaultGameData()
{
	RegisterConstansData();
	RegisterAffects();
	RegisterEffects();
	RegisterRefinedEffects();
	RegisterFlyEffects();
	RegisterTitleNamesAndColors();
	RegisterEmojis();
	CPythonBackground::GetInstance()->CreateProperty();
	return true;
}

bool AbstractResources::LoadRaceData()
{
	LoadActorMotions();
	LoadNonPlayerData();
	LoadHugeRaceData();

	//Set Skills to Races
	auto net = CPythonNetworkStream::GetInstance();
	RegisterSkills(net->GetMainActorRace(), net->GetMainActorSkillGroup(), net->GetMainActorEmpire());
	LoadRaceHeights();
	return true;
}

/* Register Constans Data */
const bool AbstractResources::RegisterConstansData()
{
	CInstanceBase::SetDustGap(250);
	CInstanceBase::SetHorseDustGap(500);
	CPythonPlayer::GetInstance()->RegisterEffect(CPythonPlayer::EFFECT_PICK, "d:/ymir work/effect/etc/click/click.mse", true);

	/* Register Dungeon Names */
	for (const auto& DungeonNames : dungeonArray)
		CPythonBackground::GetInstance()->RegisterDungeonMapName(DungeonNames);

	/* Load Interface Use Sounds */
	for (const auto& [useSoundIndex, useSoundFile] : useSoundFiles)
		CPythonItem::GetInstance()->SetUseSoundFileName(useSoundIndex, useSoundFile);

	/* Load Game Drop Sounds */
	for (const auto& [dropSoundIndex, dropSoundFile] : dropSoundFiles)
		CPythonItem::GetInstance()->SetDropSoundFileName(dropSoundIndex, dropSoundFile);

	return true;
}

/* Register Title Names and Colors */
const bool AbstractResources::RegisterTitleNamesAndColors()
{
	/* Title Names */
	auto lang = py::module::import("lang");
	const static std::unordered_map<uint16_t, std::string> mapTitleName =
	{
		{0, lang.attr("Get")("PVP_LEVEL0").cast<std::string>()},
		{1, lang.attr("Get")("PVP_LEVEL1").cast<std::string>()},
		{2, lang.attr("Get")("PVP_LEVEL2").cast<std::string>()},
		{3, lang.attr("Get")("PVP_LEVEL3").cast<std::string>()},
		{4, lang.attr("Get")("PVP_LEVEL4").cast<std::string>()},
		{5, lang.attr("Get")("PVP_LEVEL5").cast<std::string>()},
		{6, lang.attr("Get")("PVP_LEVEL6").cast<std::string>()},
		{7, lang.attr("Get")("PVP_LEVEL7").cast<std::string>()},
		{8, lang.attr("Get")("PVP_LEVEL8").cast<std::string>()},
	};

	for (const auto& [titleIndex, titleName] : mapTitleName)
		CInstanceBase::RegisterTitleName(titleIndex, titleName.c_str());

	/* Title Colors */
	for (const auto& [index, rgbTuple] : titleColorMap)
	{
		const auto& [r, g, b] = rgbTuple;
		CInstanceBase::RegisterTitleColor(index, r, g, b);
	}

	/* Name Colors */
	for (const auto& [index, rgbTuple] : nameColorMap)
	{
		const auto& [r, g, b] = rgbTuple;
		CInstanceBase::RegisterNameColor(index, r, g, b);
	}

	return true;
}

/* Register Emojis and Chat Strings */
const bool AbstractResources::RegisterEmojis()
{
	CInstanceBase EmoticonRegister = CInstanceBase();
	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 0, "", "d:/ymir work/effect/etc/emoticon/sweat.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(sweat)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 1, "", "d:/ymir work/effect/etc/emoticon/money.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(money)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 2, "", "d:/ymir work/effect/etc/emoticon/happy.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(happy)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 3, "", "d:/ymir work/effect/etc/emoticon/love_s.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(love_s)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 4, "", "d:/ymir work/effect/etc/emoticon/love_l.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(love_l)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 5, "", "d:/ymir work/effect/etc/emoticon/angry.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(angry)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 6, "", "d:/ymir work/effect/etc/emoticon/aha.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(aha)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 7, "", "d:/ymir work/effect/etc/emoticon/gloom.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(gloom)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 8, "", "d:/ymir work/effect/etc/emoticon/sorry.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(sorry)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 9, "", "d:/ymir work/effect/etc/emoticon/!_mix_back.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(!)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 10, "", "d:/ymir work/effect/etc/emoticon/question.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(?)");

	EmoticonRegister.RegisterEffect(CInstanceBase::EFFECT_EMOTICON + 11, "", "d:/ymir work/effect/etc/emoticon/fish.mse", true);
	CPythonNetworkStream::GetInstance()->RegisterEmoticonString("(fish)");
	return true;
}

/* Register Game Affects */
const bool AbstractResources::RegisterAffects()
{
	CInstanceBase AffectRegistry = CInstanceBase();
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 0, "Bip01", "Globals/effect/gm.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 3, "Bip01", "d:/ymir work/effect/hit/blow_poison/poison_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 4, "", "d:/ymir work/effect/affect/slow.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 5, "Bip01 Head", "d:/ymir work/effect/etc/stun/stun_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 6, "", "d:/ymir work/effect/etc/ready/ready.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 16, "", "d:/ymir work/pc/warrior/effect/gyeokgongjang_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 17, "", "d:/ymir work/pc/assassin/effect/gyeonggong_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 19, "Bip01 R Finger2", "d:/ymir work/pc/sura/effect/gwigeom_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 20, "", "d:/ymir work/pc/sura/effect/fear_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 21, "", "d:/ymir work/pc/sura/effect/jumagap_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 22, "", "d:/ymir work/pc/shaman/effect/3hosin_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 23, "", "d:/ymir work/pc/shaman/effect/boho_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 24, "", "d:/ymir work/pc/shaman/effect/10kwaesok_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 25, "", "d:/ymir work/pc/sura/effect/heuksin_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 26, "", "d:/ymir work/pc/sura/effect/muyeong_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 28, "Bip01", "d:/ymir work/effect/hit/blow_flame/flame_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 29, "Bip01 R Hand", "d:/ymir work/pc/shaman/effect/6gicheon_hand.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 30, "Bip01 L Hand", "d:/ymir work/pc/shaman/effect/jeungryeok_hand.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 32, "Bip01 Head", "d:/ymir work/pc/sura/effect/pabeop_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 33, "", "d:/ymir work/pc/warrior/effect/gyeokgongjang_loop.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 35, "", "d:/ymir work/effect/etc/guild_war_flag/flag_red.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 36, "", "d:/ymir work/effect/etc/guild_war_flag/flag_blue.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 37, "", "d:/ymir work/effect/etc/guild_war_flag/flag_yellow.mse", true);
	AffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AFFECT + 45, "", "d:/ymir work/pc/sura/effect/muyeong_loop.mse", true);
	return true;
}

/* Register Game Effects */
const bool AbstractResources::RegisterEffects()
{
	CInstanceBase EffectRegistry = CInstanceBase();
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DUST, "", "d:/ymir work/effect/etc/dust/dust.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_HORSE_DUST, "", "d:/ymir work/effect/etc/dust/running_dust.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_HIT, "", "d:/ymir work/effect/hit/blow_1/blow_1_low.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_HPUP_RED, "", "d:/ymir work/effect/etc/recuperation/drugup_red.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SPUP_BLUE, "", "d:/ymir work/effect/etc/recuperation/drugup_blue.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SPEEDUP_GREEN, "", "d:/ymir work/effect/etc/recuperation/drugup_green.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DXUP_PURPLE, "", "d:/ymir work/effect/etc/recuperation/drugup_purple.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AUTO_HPUP, "", "d:/ymir work/effect/etc/recuperation/autodrugup_red.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_AUTO_SPUP, "", "d:/ymir work/effect/etc/recuperation/autodrugup_blue.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_RAMADAN_RING_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item1.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_HALLOWEEN_CANDY_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item2.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_HAPPINESS_RING_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item3.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_LOVE_PENDANT_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item4.mse", true);
	//EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_ACCE_SUCCEDED, "", "d:/ymir work/effect/etc/buff/buff_item6.mse", true);
	//EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_ACCE_EQUIP, "", "d:/ymir work/effect/etc/buff/buff_item7.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_PENETRATE, "Bip01", "d:/ymir work/effect/hit/gwantong.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_FIRECRACKER, "", "d:/ymir work/effect/etc/firecracker/newyear_firecracker.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SPIN_TOP, "", "d:/ymir work/effect/etc/firecracker/paing_i.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SELECT, "", "d:/ymir work/effect/etc/click/click_select.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_TARGET, "", "d:/ymir work/effect/etc/click/click_glow_select.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_STUN, "Bip01 Head", "d:/ymir work/effect/etc/stun/stun.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_CRITICAL, "Bip01 R Hand", "d:/ymir work/effect/hit/critical.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_TARGET, "", "d:/ymir work/effect/affect/damagevalue/target.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_NOT_TARGET, "", "d:/ymir work/effect/affect/damagevalue/nontarget.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_SELFDAMAGE, "", "d:/ymir work/effect/affect/damagevalue/damage.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_SELFDAMAGE2, "", "d:/ymir work/effect/affect/damagevalue/damage_1.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_POISON, "", "d:/ymir work/effect/affect/damagevalue/poison.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_MISS, "", "d:/ymir work/effect/affect/damagevalue/miss.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_TARGETMISS, "", "d:/ymir work/effect/affect/damagevalue/target_miss.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_DAMAGE_CRITICAL, "", "d:/ymir work/effect/affect/damagevalue/critical.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_PERCENT_DAMAGE1, "", "d:/ymir work/effect/hit/percent_damage1.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_PERCENT_DAMAGE2, "", "d:/ymir work/effect/hit/percent_damage2.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_PERCENT_DAMAGE3, "", "d:/ymir work/effect/hit/percent_damage3.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SPAWN_APPEAR, "Bip01", "d:/ymir work/effect/etc/appear_die/monster_appear.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SPAWN_DISAPPEAR, "Bip01", "d:/ymir work/effect/etc/appear_die/monster_die.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_FLAME_ATTACK, "equip_right_hand", "d:/ymir work/effect/hit/blow_flame/flame_3_weapon.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_FLAME_HIT, "", "d:/ymir work/effect/hit/blow_flame/flame_3_blow.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_FLAME_ATTACH, "", "d:/ymir work/effect/hit/blow_flame/flame_3_body.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_ELECTRIC_ATTACK, "equip_right", "d:/ymir work/effect/hit/blow_electric/light_1_weapon.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_ELECTRIC_HIT, "", "d:/ymir work/effect/hit/blow_electric/light_1_blow.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_ELECTRIC_ATTACH, "", "d:/ymir work/effect/hit/blow_electric/light_1_body.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_LEVELUP, "", "d:/ymir work/effect/etc/levelup_1/level_up.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SKILLUP, "", "d:/ymir work/effect/etc/skillup/skillup_1.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_EMPIRE + 1, "Bip01", "d:/ymir work/effect/etc/empire/empire_A.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_EMPIRE + 2, "Bip01", "d:/ymir work/effect/etc/empire/empire_B.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_EMPIRE + 3, "Bip01", "d:/ymir work/effect/etc/empire/empire_C.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_WEAPON + 1, "equip_right_hand", "d:/ymir work/pc/warrior/effect/geom_sword_loop.mse", true);
	EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_WEAPON + 2, "equip_right_hand", "d:/ymir work/pc/warrior/effect/geom_spear_loop.mse", true);
	//EffectRegistry.RegisterEffect(CInstanceBase::EFFECT_SELECT_PRIVATE_SHOP, "", "d:/ymir work/effect/etc/direction/direction_land2.mse", true);

	return true;
}

/* Register Refined Game Effects */
const bool AbstractResources::RegisterRefinedEffects()
{
	CInstanceBase RefinedRegistry = CInstanceBase();
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 1, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_7.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 2, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_8.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 3, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_9.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 4, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_7_b.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 5, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_8_b.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 6, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_9_b.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 7, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_7_f.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 8, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_8_f.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 9, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_9_f.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 10, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_7_s.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 11, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_8_s.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 12, "PART_WEAPON", "D:/ymir work/pc/common/effect/sword/sword_9_s.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 13, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_7_s.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 14, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_8_s.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 15, "PART_WEAPON_LEFT", "D:/ymir work/pc/common/effect/sword/sword_9_s.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 16, "Bip01", "D:/ymir work/pc/common/effect/armor/armor_7.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 17, "Bip01", "D:/ymir work/pc/common/effect/armor/armor_8.mse", true);
	RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 18, "Bip01", "D:/ymir work/pc/common/effect/armor/armor_9.mse", true);
	//RefinedRegistry.RegisterEffect(CInstanceBase::EFFECT_REFINED + 19,			"Bip01",			"d:/ymir work/pc/common/effect/armor/acc_01.mse", true);
	return true;
}

/* Register Fly Effects */
const bool AbstractResources::RegisterFlyEffects()
{
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_EXP, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/effect/etc/gathering/ga_piece_yellow_small2.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_HP_MEDIUM, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/effect/etc/gathering/ga_piece_red_small.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_HP_BIG, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/effect/etc/gathering/ga_piece_red_big.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_SP_SMALL, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/effect/etc/gathering/ga_piece_blue_warrior_small.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_SP_MEDIUM, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/effect/etc/gathering/ga_piece_blue_small.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_SP_BIG, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/effect/etc/gathering/ga_piece_blue_big.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_FIREWORK1, INDEX_FLY_TYPE_FIRE_CRACKER, "d:/ymir work/effect/etc/firecracker/firecracker_1.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_FIREWORK2, INDEX_FLY_TYPE_FIRE_CRACKER, "d:/ymir work/effect/etc/firecracker/firecracker_2.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_FIREWORK3, INDEX_FLY_TYPE_FIRE_CRACKER, "d:/ymir work/effect/etc/firecracker/firecracker_3.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_FIREWORK4, INDEX_FLY_TYPE_FIRE_CRACKER, "d:/ymir work/effect/etc/firecracker/firecracker_4.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_FIREWORK5, INDEX_FLY_TYPE_FIRE_CRACKER, "d:/ymir work/effect/etc/firecracker/firecracker_5.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_FIREWORK6, INDEX_FLY_TYPE_FIRE_CRACKER, "d:/ymir work/effect/etc/firecracker/firecracker_6.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_FIREWORK_XMAS, INDEX_FLY_TYPE_FIRE_CRACKER, "d:/ymir work/effect/etc/firecracker/firecracker_xmas.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_CHAIN_LIGHTNING, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/pc/shaman/effect/pokroe.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_HP_SMALL, INDEX_FLY_TYPE_NORMAL, "d:/ymir work/effect/etc/gathering/ga_piece_red_smallest.msf");
	CFlyingManager::GetInstance()->RegisterIndexedFlyData(FLY_SKILL_MUYEONG, INDEX_FLY_TYPE_AUTO_FIRE, "d:/ymir work/pc/sura/effect/muyeong_fly.msf");
	return true;
}

/* Register Actor Skill Data */
const bool AbstractResources::RegisterSkills(uint32_t race, uint8_t skill_group, uint8_t empire)
{
	const uint32_t playerRace = race;
	const uint32_t playerSkillGroup = skill_group;
	const uint8_t playerJob = RaceToJob(playerRace);
	const uint32_t playerEmpire = empire;

	// Player Skills
	if (generalSkillIndexes.find(playerJob) != generalSkillIndexes.end() && generalSkillIndexes.at(playerJob).find(skill_group) != generalSkillIndexes.at(playerJob).end())
	{
		std::vector<uint8_t> playerSkills = generalSkillIndexes.at(playerJob).at(playerSkillGroup);

		for (size_t slot = 0; slot < playerSkills.size(); ++slot)
		{
			CPythonPlayer::GetInstance()->SetSkill(slot + 1, playerSkills[slot]);
		}
	}

	// Support Skills
	const auto supportSkills = supportSkillIndexes;
	for (size_t slot = 0; slot < supportSkills.size(); ++slot)
	{
		CPythonPlayer::GetInstance()->SetSkill(slot + 100 + 1, supportSkills[slot]);
	}

	//Language Skills
	if (playerEmpire)
	{
		for (size_t i = 0; i < 3; ++i)
		{
			if ((i + 1) != playerEmpire)
			{
				CPythonPlayer::GetInstance()->SetSkill(109 + i, c_iSkillIndex_Language1 + i);
			}
		}
	}

	// Passive Guild Skills
	const auto& passiveSkills = guildSkillsIndexes.at("PASSIVE");
	for (size_t j = 0; j < passiveSkills.size(); ++j)
		CPythonPlayer::GetInstance()->SetSkill(200 + j, passiveSkills[j]);

	// Active Guild Skills
	const auto& activeSkills = guildSkillsIndexes.at("ACTIVE");
	for (size_t k = 0; k < activeSkills.size(); ++k)
		CPythonPlayer::GetInstance()->SetSkill(210 + k, activeSkills[k]);

	return true;
}

/* Load Guild Building List */
bool AbstractResources::RegisterGuildBuildingList()
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), GUILD_BUILDING_LIST_FILENAME);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", GUILD_BUILDING_LIST_FILENAME);
		return false;
	}

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(vfs_string.value());

	CTokenVector TokenVector;

	std::vector<py::dict> guildbuildingMap;

	for (size_t i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		std::string& strVnum = TokenVector[GUIILD_BUILDING_TOKEN_VNUM];

		if (strVnum.find_first_not_of("0123456789") != std::string::npos)
			continue;

		if (TokenVector.size() < GUIILD_BUILDING_LIMIT_TOKEN_COUNT)
		{
			SysLog("RegisterGuildBuildingList({0}) - StrangeLine in {1} TokenVector size too long: {2}", GUILD_BUILDING_LIST_FILENAME, i, TokenVector.size());
			continue;
		}

		const uint32_t& id = atoi(strVnum.c_str());
		const std::string_view type = TokenVector[GUIILD_BUILDING_TOKEN_TYPE];
		const char* name = TokenVector[GUIILD_BUILDING_TOKEN_NAME].c_str();
		const char* localName = TokenVector[GUIILD_BUILDING_TOKEN_LOCAL_NAME].c_str();
		const uint32_t& xRotLimit = atoi(TokenVector[GUIILD_BUILDING_TOKEN_X_ROT_LIMIT].c_str());
		const uint32_t& yRotLimit = atoi(TokenVector[GUIILD_BUILDING_TOKEN_Y_ROT_LIMIT].c_str());
		const uint32_t& zRotLimit = atoi(TokenVector[GUIILD_BUILDING_TOKEN_Z_ROT_LIMIT].c_str());
		const uint32_t& price = atoi(TokenVector[GUIILD_BUILDING_TOKEN_PRICE].c_str());
		const std::string_view material = TokenVector[GUIILD_BUILDING_TOKEN_MATERIAL];
		const uint32_t& enableType = atoi(TokenVector[GUIILD_BUILDING_TOKEN_ENABLE_FLAG].c_str());

		if (enableType == 0)
			continue;

		// Register material
		const std::string& folderName = m_buildingTypeToFolder[type];
		const std::string& full_path = "D:/ymir work/guild/" + folderName + ".msm";

		CRaceManager::GetInstance()->RegisterRace(id, full_path);

		// Create building material List
		std::vector<std::string> materialVector;
		boost::split(materialVector, material, boost::is_any_of("/"));

		std::vector<std::string> pyMaterialList;

		for (size_t j = 0; j < GUILD_MATERIAL_NUM; ++j)
		{
			pyMaterialList.emplace_back("");
		}

		for (const std::string& itemPair : materialVector)
		{

			std::vector<std::string> itemVector;
			boost::split(itemVector, itemPair, boost::is_any_of(","));

			if (itemVector.size() != 2)
				continue;

			const uint32_t& vnum = atoi(itemVector[0].c_str());
			const std::string& count = itemVector[1];

			const uint8_t index = getGuildMaterialIndex(vnum);

			if (index == -1)
			{
				SysLog("Strange guild material: {0}", vnum);
				continue;
			}

			pyMaterialList[index] = count;
		}

		// Create guild data dict and append to list

		py::dict dict;
		dict.attr("VNUM") = id;
		dict.attr("TYPE") = type.data();
		dict.attr("NAME") = name;
		dict.attr("LOCAL_NAME") = localName;
		dict.attr("X_ROT_LIMIT") = xRotLimit;
		dict.attr("Y_ROT_LIMIT") = yRotLimit;
		dict.attr("Z_ROT_LIMIT") = zRotLimit;
		dict.attr("PRICE") = price;
		dict.attr("MATERIAL") = pyMaterialList;
		dict.attr("SHOW") = enableType != 2;

		guildbuildingMap.emplace_back(dict);
	}


	CPythonNetworkStream::GetInstance()->SendDataToPython("BINARY_SetGuildBuildingList", guildbuildingMap);

	return true;
}

/* Load RaceHeights */
const bool AbstractResources::LoadRaceHeights()
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), RACE_HEIGHT_FILENAME);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", RACE_HEIGHT_FILENAME);
		return false;
	}

	CMemoryTextFileLoader fileLoader;
	fileLoader.Bind(vfs_string.value());

	CTokenVector kTokenVector;
	for (uint32_t i = 0; i < fileLoader.GetLineCount(); ++i)
	{
		if (!fileLoader.SplitLineByTab(i, &kTokenVector))
			continue;

		for (auto& token : kTokenVector)
			token.erase(std::remove_if(token.begin(), token.end(), isspace), token.end());

		while (kTokenVector.size() < 3)
			kTokenVector.push_back("");

		const uint32_t dwRaceVnum = std::stoul(kTokenVector[0]);
		const float height_data = std::stof(kTokenVector.at(1));

		if (dwRaceVnum)
		{
			CRaceManager::GetInstance()->SetRaceHeight(dwRaceVnum, height_data);
		}
		else
		{
			SysLog("Register Race Heights: {0} Strage instruction at ID: {1}, Line: #{2}", RACE_HEIGHT_FILENAME, dwRaceVnum, i);
		}
	}
	return true;
}

/* Load NonPlayer List */
const bool AbstractResources::LoadNonPlayerData()
{
	auto data = CallFS().LoadFileToString(CallFS(), NPC_LIST_FILENAME);
	if (!data)
		return false;

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(data.value());

	CTokenVector args;
	for (uint32_t i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &args))
			continue;

		if (args.size() < 2)
		{
			SysLog("Npc race list '{0}' line {1} has only {2} tokens", NPC_LIST_FILENAME, i, args.size());
			return false;
		}

		try
		{
			uint32_t vnum = std::stoul(args[0]);
			CRaceManager::GetInstance()->RegisterRace(vnum, args[1]);
		}
		catch (const std::invalid_argument& ia)
		{
			SysLog("Npc race list '{0}' line {1} has invalid token {2}", NPC_LIST_FILENAME, i, args[0]);
			return false;
		}
		catch (const std::out_of_range& oor)
		{
			SysLog("Npc race list '{0}' line {1} has out of range token {2}", NPC_LIST_FILENAME, i, args[0]);
			return false;
		}
	}

	return true;
}

/* Load Huge Race data*/
const bool AbstractResources::LoadHugeRaceData()
{
	auto data = CallFS().LoadFileToString(CallFS(), HUGE_RACE_LIST_FILENAME);
	if (!data)
		return false;

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(data.value());

	CTokenVector args;
	for (uint32_t i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &args))
			continue;

		if (args.size() < 1)
		{
			SysLog("Huge race list '{0}' line {1} has only {2} tokens", HUGE_RACE_LIST_FILENAME, i, args.size());
			return false;
		}

		try
		{
			uint32_t vnum = std::stoul(args[0]);
			CRaceManager::GetInstance()->SetHugeRace(vnum);
		}
		catch (const std::invalid_argument& ia)
		{
			SysLog("Npc race list '{0}' line {1} has invalid token {2}", HUGE_RACE_LIST_FILENAME, i, args[0]);
			return false;
		}
		catch (const std::out_of_range& oor)
		{
			SysLog("Npc race list '{0}' line {1} has out of range token {2}", HUGE_RACE_LIST_FILENAME, i, args[0]);
			return false;
		}
	}

	return true;
}

/* Start of Loading Actors */
void AbstractResources::LoadRaceMotions(uint32_t race)
{
	CRaceManager::GetInstance()->PreloadRace(race);

	auto pRaceData = CRaceManager::GetInstance()->GetRaceDataPointer(race);
	if (pRaceData)
		pRaceData.value()->LoadMotions();
}

void AbstractResources::RegisterRace(uint8_t race, const std::string& msmPath)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();
	chrmgr->CreateRace(race);
	chrmgr->SelectRace(race);

	auto pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	pRaceData->LoadRaceData(msmPath.c_str());
}

void AbstractResources::SetIntroMotions(uint8_t mode, const std::string& folder)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();
	chrmgr->SetPathName(folder);

	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	pRaceData->RegisterMotionMode(mode);
	chrmgr->RegisterCacheMotionData(mode, MOTION_INTRO_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_INTRO_SELECTED, "selected.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_INTRO_NOT_SELECTED, "not_selected.msa");
}

void AbstractResources::SetGeneralMotions(uint8_t mode, const std::string& folder)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();
	chrmgr->SetPathName(folder);

	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	pRaceData->RegisterMotionMode(mode);
	chrmgr->RegisterCacheMotionData(mode, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(mode, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(mode, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(mode, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(mode, MOTION_DAMAGE_FLYING, "damage_flying.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_STAND_UP, "falling_stand.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_DAMAGE_FLYING_BACK, "back_damage_flying.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_STAND_UP_BACK, "back_falling_stand.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_DEAD, "dead.msa");
	chrmgr->RegisterCacheMotionData(mode, MOTION_DIG, "dig.msa");
}

void AbstractResources::RegisterSharedEmotionAnis(uint32_t mode, const std::string path)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();
	chrmgr->SetPathName(path);

	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();

	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	pRaceData->RegisterMotionMode(mode);

	for (const auto& elem : aniMap)
	{
		chrmgr->RegisterCacheMotionData(mode, elem.first, elem.second);
	}
}

void AbstractResources::RegisterEmotionAnis(std::string path)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();

	std::string actionPath = path + "action/";
	std::string weddingPath = path + "wedding/";

	RegisterSharedEmotionAnis(MOTION_MODE_GENERAL, actionPath);
	RegisterSharedEmotionAnis(MOTION_MODE_WEDDING_DRESS, actionPath);

	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();

	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	chrmgr->SetPathName(weddingPath);
	pRaceData->RegisterMotionMode(MOTION_MODE_WEDDING_DRESS);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_WEDDING_DRESS, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_WEDDING_DRESS, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_WEDDING_DRESS, MOTION_RUN, "walk.msa");
}

const bool AbstractResources::LoadActorMotions()
{
	AbstractResources::RegisterRace(MAIN_RACE_WARRIOR_M, "Globals/MSM/warrior_m.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc/warrior/intro/");

	AbstractResources::RegisterRace(MAIN_RACE_WARRIOR_W, "Globals/MSM/warrior_w.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc2/warrior/intro/");

	AbstractResources::RegisterRace(MAIN_RACE_ASSASSIN_W, "Globals/MSM/assassin_w.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc/assassin/intro/");

	AbstractResources::RegisterRace(MAIN_RACE_ASSASSIN_M, "Globals/MSM/assassin_m.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc2/assassin/intro/");

	AbstractResources::RegisterRace(MAIN_RACE_SURA_M, "Globals/MSM/sura_m.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc/sura/intro/");

	AbstractResources::RegisterRace(MAIN_RACE_SURA_W, "Globals/MSM/sura_w.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc2/sura/intro/");

	AbstractResources::RegisterRace(MAIN_RACE_SHAMAN_W, "Globals/MSM/shaman_w.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc/shaman/intro/");

	AbstractResources::RegisterRace(MAIN_RACE_SHAMAN_M, "Globals/MSM/shaman_m.msm");
	AbstractResources::SetIntroMotions(MOTION_MODE_GENERAL, "d:/ymir work/pc2/shaman/intro/");

	AbstractResources::LoadWarrior(MAIN_RACE_WARRIOR_M, "d:/ymir work/pc/warrior/");
	AbstractResources::LoadWarrior(MAIN_RACE_WARRIOR_W, "d:/ymir work/pc2/warrior/");

	AbstractResources::LoadAssassin(MAIN_RACE_ASSASSIN_W, "d:/ymir work/pc/assassin/");
	AbstractResources::LoadAssassin(MAIN_RACE_ASSASSIN_M, "d:/ymir work/pc2/assassin/");

	AbstractResources::LoadSura(MAIN_RACE_SURA_M, "d:/ymir work/pc/sura/");
	AbstractResources::LoadSura(MAIN_RACE_SURA_W, "d:/ymir work/pc2/sura/");

	AbstractResources::LoadShaman(MAIN_RACE_SHAMAN_M, "d:/ymir work/pc2/shaman/");
	AbstractResources::LoadShaman(MAIN_RACE_SHAMAN_W, "d:/ymir work/pc/shaman/");

	for (uint32_t i = 0; i < MAIN_RACE_MAX_NUM; ++i)
	{
		AbstractResources::LoadRaceMotions(i);
	}

	return true;
}

void AbstractResources::LoadWarrior(uint8_t race, const std::string& path)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();

	chrmgr->SelectRace(race);

	AbstractResources::SetGeneralMotions(MOTION_MODE_GENERAL, path + "general/");

	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	if (!pRaceData->SetMotionRandomWeight(MOTION_MODE_GENERAL, MOTION_WAIT, 0, 70))
	{
		ConsoleLog("Failed to SetMotionRandomWeight");
	}

	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_WAIT, "wait_1.msa", 30);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack.msa", 30);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack_1.msa", 50);

	chrmgr->SetPathName(path + "skill/");

	for (int32_t i = 0; i < CPythonSkill::SKILL_EFFECT_COUNT; ++i)
	{
		std::string endString = "";
		if (i != 0)
			endString = fmt::format("_{0}", i + 1);

		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 1, "samyeon" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 2, "palbang" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 3, "jeongwi" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 4, "geomgyeong" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 5, "tanhwan" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 6, "gihyeol" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 16, "gigongcham" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 17, "gyeoksan" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 18, "daejin" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 19, "cheongeun" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 20, "geompung" + endString + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 21, "noegeom" + endString + ".msa");
	}

	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLOOD, "guild_yongsinuipi.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLESS, "guild_yongsinuichukbok.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_BLESSARMOR, "guild_seonghwigap.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_SPPEDUP, "guild_gasokhwa.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONWRATH, "guild_yongsinuibunno.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_MAGICUP, "guild_jumunsul.msa");

	pRaceData->ReserveComboAttack(MOTION_MODE_GENERAL, 0, 1);
	pRaceData->RegisterComboAttack(MOTION_MODE_GENERAL, 0, 0, MOTION_COMBO_ATTACK_1);

	chrmgr->SetPathName(path + "onehand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_ONEHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WAIT, "wait.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WAIT, "wait_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_4, "combo_04.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_5, "combo_05.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_6, "combo_06.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_7, "combo_07.msa");

	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, 0, 4);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 0, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 0, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 0, 2, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 0, 3, MOTION_COMBO_ATTACK_4);
	// Combo Type 2
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, 1, 5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 1, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 1, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 1, 2, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 1, 3, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 1, 4, MOTION_COMBO_ATTACK_7);
	// Combo Type 3
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, 2, 6);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 2, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 2, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 2, 2, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 2, 3, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 2, 4, MOTION_COMBO_ATTACK_6);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, 2, 5, MOTION_COMBO_ATTACK_4);

	// TWOHAND_SWORD BATTLE
	chrmgr->SetPathName(path + "twohand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_TWOHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_WAIT, "wait.msa", 70);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_WAIT, "wait_1.msa", 30);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_4, "combo_04.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_5, "combo_05.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_6, "combo_06.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_7, "combo_07.msa");

	// Combo Type 1
	pRaceData->ReserveComboAttack(MOTION_MODE_TWOHAND_SWORD, 0, 4);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 0, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 0, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 0, 2, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 0, 3, MOTION_COMBO_ATTACK_4);
	// Combo Type 2
	pRaceData->ReserveComboAttack(MOTION_MODE_TWOHAND_SWORD, 1, 5);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 1, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 1, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 1, 2, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 1, 3, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 1, 4, MOTION_COMBO_ATTACK_7);
	// Combo Type 3
	pRaceData->ReserveComboAttack(MOTION_MODE_TWOHAND_SWORD, 2, 6);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 2, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 2, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 2, 2, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 2, 3, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 2, 4, MOTION_COMBO_ATTACK_6);
	pRaceData->RegisterComboAttack(MOTION_MODE_TWOHAND_SWORD, 2, 5, MOTION_COMBO_ATTACK_4);

	// FISHING
	chrmgr->SetPathName(path + "fishing/");
	pRaceData->RegisterMotionMode(MOTION_MODE_FISHING);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_THROW, "throw.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_WAIT, "fishing_wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_STOP, "fishing_cancel.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_REACT, "fishing_react.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_CATCH, "fishing_catch.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_FAIL, "fishing_fail.msa");

	// HORSE
	chrmgr->SetPathName(path + "horse/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait.msa", 90);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_1.msa", 9);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_2.msa", 1);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE_BACK, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DEAD, "dead.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, HORSE_SKILL_CHARGE, "skill_charge.msa");

	// HORSE_ONEHAND_SWORD
	chrmgr->SetPathName(path + "horse_onehand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_ONEHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, 0, 3);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, 0, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, 0, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, 0, 2, MOTION_COMBO_ATTACK_3);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, HORSE_SKILL_SPLASH, "skill_splash.msa");

	chrmgr->SetPathName(path + "horse_twohand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_TWOHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_TWOHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_TWOHAND_SWORD, 0, 3);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_TWOHAND_SWORD, 0, 0, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_TWOHAND_SWORD, 0, 1, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_TWOHAND_SWORD, 0, 2, MOTION_COMBO_ATTACK_3);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_TWOHAND_SWORD, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_TWOHAND_SWORD, HORSE_SKILL_SPLASH, "skill_splash.msa");

	// Bone
	pRaceData->RegisterAttachingBoneName(PART_WEAPON, "equip_right_hand");

	AbstractResources::RegisterEmotionAnis(path);
}

void AbstractResources::LoadAssassin(uint8_t race, const std::string& path)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();
	chrmgr->SelectRace(race);

	// GENERAL MOTION MODE
	AbstractResources::SetGeneralMotions(MOTION_MODE_GENERAL, path + "general/");

	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	if (!pRaceData->SetMotionRandomWeight(MOTION_MODE_GENERAL, MOTION_WAIT, 0, 70))
	{
		ConsoleLog("Failed to SetMotionRandomWeight");
	}

	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_WAIT, "wait_1.msa", 30);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack_1.msa", 50);

	pRaceData->ReserveComboAttack(MOTION_MODE_GENERAL, COMBO_TYPE_1, 1);
	pRaceData->RegisterComboAttack(MOTION_MODE_GENERAL, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);

	// SKILL
	chrmgr->SetPathName(path + "skill/");
	for (int32_t i = 0; i < CPythonSkill::SKILL_EFFECT_COUNT; ++i)
	{
		std::string END_STRING = "";
		if (i != 0)
			END_STRING = fmt::format("_{0}", i + 1);

		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 1, "amseup" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 2, "gungsin" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 3, "charyun" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 4, "eunhyeong" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 5, "sangong" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 6, "seomjeon" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 16, "yeonsa" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 17, "gwangyeok" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 18, "hwajo" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 19, "gyeonggong" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 20, "dokgigung" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 21, "seomgwang" + END_STRING + ".msa");
	}

	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLOOD, "guild_yongsinuipi.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLESS, "guild_yongsinuichukbok.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_BLESSARMOR, "guild_seonghwigap.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_SPPEDUP, "guild_gasokhwa.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONWRATH, "guild_yongsinuibunno.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_MAGICUP, "guild_jumunsul.msa");

	// ONEHAND_SWORD BATTLE
	chrmgr->SetPathName(path + "onehand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_ONEHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WAIT, "wait.msa", 70);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WAIT, "wait_1.msa", 30);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_4, "combo_04.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_5, "combo_05.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_6, "combo_06.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_7, "combo_07.msa");

	// Combo Type 1
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, 4);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_4, MOTION_COMBO_ATTACK_4);
	// Combo Type 2
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, 5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_5, MOTION_COMBO_ATTACK_7);
	// Combo Type 3
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, 6);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_5, MOTION_COMBO_ATTACK_6);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_6, MOTION_COMBO_ATTACK_4);

	// DUALHAND_SWORD BATTLE
	chrmgr->SetPathName(path + "dualhand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_DUALHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_WAIT, "wait.msa", 70);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_WAIT, "wait_1.msa", 30);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_4, "combo_04.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_5, "combo_05.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_6, "combo_06.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_7, "combo_07.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_8, "combo_08.msa");

	// Combo Type 1
	pRaceData->ReserveComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_1, 4);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_4, MOTION_COMBO_ATTACK_4);
	// Combo Type 2
	pRaceData->ReserveComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_2, 5);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_5, MOTION_COMBO_ATTACK_7);
	// Combo Type 3
	pRaceData->ReserveComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_3, 6);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_5, MOTION_COMBO_ATTACK_6);
	pRaceData->RegisterComboAttack(MOTION_MODE_DUALHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_6, MOTION_COMBO_ATTACK_8);

	// BOW BATTLE
	chrmgr->SetPathName(path + "bow/");
	pRaceData->RegisterMotionMode(MOTION_MODE_BOW);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_WAIT, "wait.msa", 70);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_WAIT, "wait_1.msa", 30);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BOW, MOTION_COMBO_ATTACK_1, "attack.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_BOW, COMBO_TYPE_1, 1);
	pRaceData->RegisterComboAttack(MOTION_MODE_BOW, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);

	// FISHING
	chrmgr->SetPathName(path + "fishing/");
	pRaceData->RegisterMotionMode(MOTION_MODE_FISHING);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_THROW, "throw.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_WAIT, "fishing_wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_STOP, "fishing_cancel.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_REACT, "fishing_react.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_CATCH, "fishing_catch.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_FAIL, "fishing_fail.msa");

	// HORSE
	chrmgr->SetPathName(path + "horse/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait.msa", 90);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_1.msa", 9);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_2.msa", 1);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE_BACK, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DEAD, "dead.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, HORSE_SKILL_CHARGE, "skill_charge.msa");

	// HORSE_ONEHAND_SWORD
	chrmgr->SetPathName(path + "horse_onehand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_ONEHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, HORSE_SKILL_SPLASH, "skill_splash.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, 3);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);

	// HORSE_DUALHAND_SWORD
	chrmgr->SetPathName(path + "horse_dualhand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_DUALHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_DUALHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_DUALHAND_SWORD, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_DUALHAND_SWORD, HORSE_SKILL_SPLASH, "skill_splash.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_DUALHAND_SWORD, COMBO_TYPE_1, 3);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_DUALHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_DUALHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_DUALHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);

	// HORSE_BOW
	chrmgr->SetPathName(path + "horse_bow/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_BOW);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, MOTION_WAIT, "wait.msa", 90);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, MOTION_WAIT, "wait_1.msa", 9);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, MOTION_WAIT, "wait_2.msa", 1);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, MOTION_DAMAGE, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, MOTION_DEAD, "dead.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, MOTION_COMBO_ATTACK_1, "attack.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BOW, HORSE_SKILL_SPLASH, "skill_splash.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_BOW, COMBO_TYPE_1, 1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_BOW, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);

	pRaceData->RegisterAttachingBoneName(PART_WEAPON, "equip_right");
	pRaceData->RegisterAttachingBoneName(PART_WEAPON_LEFT, "equip_left");

	AbstractResources::RegisterEmotionAnis(path);
}

void AbstractResources::LoadSura(uint8_t race, const std::string& path)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();

	chrmgr->SelectRace(race);

	// GENERAL MOTION MODE
	AbstractResources::SetGeneralMotions(MOTION_MODE_GENERAL, path + "general/");

	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	if (!pRaceData->SetMotionRandomWeight(MOTION_MODE_GENERAL, MOTION_WAIT, 0, 70))
	{
		ConsoleLog("Failed to SetMotionRandomWeight");
	}

	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack_1.msa", 50);

	pRaceData->ReserveComboAttack(MOTION_MODE_GENERAL, COMBO_TYPE_1, 1);
	pRaceData->RegisterComboAttack(MOTION_MODE_GENERAL, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);

	// SKILL
	chrmgr->SetPathName(path + "skill/");
	for (int32_t i = 0; i < CPythonSkill::SKILL_EFFECT_COUNT; ++i)
	{
		std::string END_STRING = "";
		if (i != 0)
			END_STRING = fmt::format("_{0}", i + 1);
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 1, "swaeryeong" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 2, "yonggwon" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 3, "gwigeom" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 4, "gongpo" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 5, "jumagap" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 6, "pabeop" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 16, "maryeong" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 17, "hwayeom" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 18, "muyeong" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 19, "heuksin" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 20, "tusok" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 21, "mahwan" + END_STRING + ".msa");
	}

	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLOOD, "guild_yongsinuipi.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLESS, "guild_yongsinuichukbok.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_BLESSARMOR, "guild_seonghwigap.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_SPPEDUP, "guild_gasokhwa.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONWRATH, "guild_yongsinuibunno.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_MAGICUP, "guild_jumunsul.msa");

	// ONEHAND_SWORD BATTLE
	chrmgr->SetPathName(path + "onehand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_ONEHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_4, "combo_04.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_5, "combo_05.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_6, "combo_06.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_7, "combo_07.msa");

	// Combo Type 1
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, 4);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_4, MOTION_COMBO_ATTACK_4);
	// Combo Type 2
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, 5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_2, COMBO_INDEX_5, MOTION_COMBO_ATTACK_7);
	// Combo Type 3
	pRaceData->ReserveComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, 6);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_5, MOTION_COMBO_ATTACK_6);
	pRaceData->RegisterComboAttack(MOTION_MODE_ONEHAND_SWORD, COMBO_TYPE_3, COMBO_INDEX_6, MOTION_COMBO_ATTACK_4);

	// FISHING
	chrmgr->SetPathName(path + "fishing/");
	pRaceData->RegisterMotionMode(MOTION_MODE_FISHING);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_THROW, "throw.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_WAIT, "fishing_wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_STOP, "fishing_cancel.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_REACT, "fishing_react.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_CATCH, "fishing_catch.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_FAIL, "fishing_fail.msa");

	// HORSE
	chrmgr->SetPathName(path + "horse/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait.msa", 90);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_1.msa", 9);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_2.msa", 1);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE_BACK, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DEAD, "dead.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, HORSE_SKILL_CHARGE, "skill_charge.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, HORSE_SKILL_SPLASH, "skill_splash.msa");

	// HORSE_ONEHAND_SWORD
	chrmgr->SetPathName(path + "horse_onehand_sword/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_ONEHAND_SWORD);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, 3);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_ONEHAND_SWORD, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_ONEHAND_SWORD, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");

	pRaceData->RegisterAttachingBoneName(PART_WEAPON, "equip_right");
	AbstractResources::RegisterEmotionAnis(path);
}

void AbstractResources::LoadShaman(uint8_t race, const std::string& path)
{
	CRaceManager* chrmgr = CRaceManager::GetInstance();

	chrmgr->SelectRace(race);
	CRaceData* pRaceData = CRaceManager::GetInstance()->GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	if (!pRaceData->SetMotionRandomWeight(MOTION_MODE_GENERAL, MOTION_WAIT, 0, 70))
	{
		ConsoleLog("Failed to SetMotionRandomWeight");
	}

	// GENERAL MOTION MODE
	AbstractResources::SetGeneralMotions(MOTION_MODE_GENERAL, path + "general/");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_COMBO_ATTACK_1, "attack_1.msa", 50);

	pRaceData->ReserveComboAttack(MOTION_MODE_GENERAL, COMBO_TYPE_1, 1);
	pRaceData->RegisterComboAttack(MOTION_MODE_GENERAL, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);

	// Fan
	chrmgr->SetPathName(path + "fan/");
	pRaceData->RegisterMotionMode(MOTION_MODE_FAN);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_COMBO_ATTACK_4, "combo_04.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_COMBO_ATTACK_5, "combo_05.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_COMBO_ATTACK_6, "combo_06.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FAN, MOTION_COMBO_ATTACK_7, "combo_07.msa");

	// Combo Type 1
	pRaceData->ReserveComboAttack(MOTION_MODE_FAN, COMBO_TYPE_1, 4);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_1, COMBO_INDEX_4, MOTION_COMBO_ATTACK_4);
	// Combo Type 2
	pRaceData->ReserveComboAttack(MOTION_MODE_FAN, COMBO_TYPE_2, 5);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_2, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_2, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_2, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_2, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_2, COMBO_INDEX_5, MOTION_COMBO_ATTACK_7);
	// Combo Type 3
	pRaceData->ReserveComboAttack(MOTION_MODE_FAN, COMBO_TYPE_3, 6);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_3, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_3, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_3, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_3, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_3, COMBO_INDEX_5, MOTION_COMBO_ATTACK_6);
	pRaceData->RegisterComboAttack(MOTION_MODE_FAN, COMBO_TYPE_3, COMBO_INDEX_6, MOTION_COMBO_ATTACK_4);

	// Bell
	chrmgr->SetPathName(path + "Bell/");
	pRaceData->RegisterMotionMode(MOTION_MODE_BELL);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_DAMAGE, "damage.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_DAMAGE, "damage_1.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_DAMAGE_BACK, "damage_2.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_DAMAGE_BACK, "damage_3.msa", 50);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_COMBO_ATTACK_4, "combo_04.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_COMBO_ATTACK_5, "combo_05.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_COMBO_ATTACK_6, "combo_06.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_BELL, MOTION_COMBO_ATTACK_7, "combo_07.msa");

	// Combo Type 1
	pRaceData->ReserveComboAttack(MOTION_MODE_BELL, COMBO_TYPE_1, 4);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_1, COMBO_INDEX_4, MOTION_COMBO_ATTACK_4);
	// Combo Type 2
	pRaceData->ReserveComboAttack(MOTION_MODE_BELL, COMBO_TYPE_2, 5);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_2, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_2, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_2, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_2, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_2, COMBO_INDEX_5, MOTION_COMBO_ATTACK_7);
	// Combo Type 3
	pRaceData->ReserveComboAttack(MOTION_MODE_BELL, COMBO_TYPE_3, 6);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_3, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_3, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_3, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_3, COMBO_INDEX_4, MOTION_COMBO_ATTACK_5);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_3, COMBO_INDEX_5, MOTION_COMBO_ATTACK_6);
	pRaceData->RegisterComboAttack(MOTION_MODE_BELL, COMBO_TYPE_3, COMBO_INDEX_6, MOTION_COMBO_ATTACK_4);

	// SKILL
	chrmgr->SetPathName(path + "skill/");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 1, "bipabu.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 2, "yongpa.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 3, "paeryong.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 4, "hosin_target.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 5, "boho_target.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 6, "gicheon_target.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 16, "noejeon.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 17, "byeorak.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 18, "pokroe.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 19, "jeongeop_target.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 20, "kwaesok_target.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + 21, "jeungryeok_target.msa");

	for (int32_t i = 0; i < CPythonSkill::SKILL_EFFECT_COUNT; ++i)
	{
		std::string END_STRING = "";
		if (i != 0)
			END_STRING = fmt::format("_{0}", i + 1);

		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 1, "bipabu" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 2, "yongpa" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 3, "paeryong" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 4, "hosin" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 5, "boho" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 6, "gicheon" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 16, "noejeon" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 17, "byeorak" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 18, "pokroe" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 19, "jeongeop" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 20, "kwaesok" + END_STRING + ".msa");
		chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, MOTION_SKILL + (i * CPythonSkill::SKILL_GRADEGAP) + 21, "jeungryeok" + END_STRING + ".msa");
	}

	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLOOD, "guild_yongsinuipi.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONBLESS, "guild_yongsinuichukbok.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_BLESSARMOR, "guild_seonghwigap.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_SPPEDUP, "guild_gasokhwa.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_DRAGONWRATH, "guild_yongsinuibunno.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_GENERAL, GUILD_SKILL_MAGICUP, "guild_jumunsul.msa");

	// FISHING
	chrmgr->SetPathName(path + "fishing/");
	pRaceData->RegisterMotionMode(MOTION_MODE_FISHING);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WAIT, "wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_THROW, "throw.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_WAIT, "fishing_wait.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_STOP, "fishing_cancel.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_REACT, "fishing_react.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_CATCH, "fishing_catch.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_FISHING, MOTION_FISHING_FAIL, "fishing_fail.msa");

	// HORSE
	chrmgr->SetPathName(path + "horse/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait.msa", 90);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_1.msa", 9);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WAIT, "wait_2.msa", 1);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_WALK, "walk.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_RUN, "run.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DAMAGE_BACK, "damage.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, MOTION_DEAD, "dead.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, HORSE_SKILL_CHARGE, "skill_charge.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE, HORSE_SKILL_SPLASH, "skill_splash.msa");

	// HORSE_FAN
	chrmgr->SetPathName(path + "horse_fan/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_FAN);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_FAN, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_FAN, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_FAN, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_FAN, COMBO_TYPE_1, 3);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_FAN, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_FAN, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_FAN, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_FAN, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");

	// HORSE_BELL
	chrmgr->SetPathName(path + "horse_bell/");
	pRaceData->RegisterMotionMode(MOTION_MODE_HORSE_BELL);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BELL, MOTION_COMBO_ATTACK_1, "combo_01.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BELL, MOTION_COMBO_ATTACK_2, "combo_02.msa");
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BELL, MOTION_COMBO_ATTACK_3, "combo_03.msa");
	pRaceData->ReserveComboAttack(MOTION_MODE_HORSE_BELL, COMBO_TYPE_1, 3);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_BELL, COMBO_TYPE_1, COMBO_INDEX_1, MOTION_COMBO_ATTACK_1);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_BELL, COMBO_TYPE_1, COMBO_INDEX_2, MOTION_COMBO_ATTACK_2);
	pRaceData->RegisterComboAttack(MOTION_MODE_HORSE_BELL, COMBO_TYPE_1, COMBO_INDEX_3, MOTION_COMBO_ATTACK_3);
	chrmgr->RegisterCacheMotionData(MOTION_MODE_HORSE_BELL, HORSE_SKILL_WILDATTACK, "skill_wildattack.msa");

	pRaceData->RegisterAttachingBoneName(PART_WEAPON, "equip_right");
	pRaceData->RegisterAttachingBoneName(PART_WEAPON_LEFT, "equip_left");
	AbstractResources::RegisterEmotionAnis(path);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
static void resRegisterSkills(int32_t race, int32_t skill_group, int32_t empire)
{
	AbstractResources::GetInstance()->RegisterSkills(race, skill_group, empire);
}

static bool resGetCurrentLoadingState()
{
	return AbstractResources::GetInstance()->GetLoadingState();
}

static void resSetPhaseToLoading(uint8_t bCharacterSlot, int32_t iPlayerX, int32_t iPlayerY)
{
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->Warp(iPlayerX, iPlayerY);
	rkNetStream->SendSelectCharacterPacket(bCharacterSlot);
}

static void resSetPhaseToWarping(int32_t playerX, int32_t playerY)
{
	auto rkNetStream = CPythonNetworkStream::GetInstance();
	rkNetStream->Warp(playerX, playerY);
}

PYBIND11_EMBEDDED_MODULE(resource, m)
{
	m.def("RegisterSkills", resRegisterSkills);
	m.def("GetCurrentLoadingState", resGetCurrentLoadingState);
	m.def("SetPhaseToLoading", resSetPhaseToLoading);
	m.def("SetPhaseToWarping", resSetPhaseToWarping);
}
