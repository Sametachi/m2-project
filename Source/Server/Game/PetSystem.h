#pragma once


class CHARACTER;

// TODO: Abilities as a pet? Or intimacy, hunger, etc... Numerical
struct SPetAbility
{
};

class CPetActor //: public CHARACTER
{
public:
	enum EPetOptions
	{
		EPetOption_Followable		= 1 << 0,
		EPetOption_Mountable		= 1 << 1,
		EPetOption_Summonable		= 1 << 2,
		EPetOption_Combatable		= 1 << 3,		
	};


protected:
	friend class CPetSystem;

	CPetActor(LPCHARACTER owner, uint32_t vnum, uint32_t options = EPetOption_Followable | EPetOption_Summonable);
//	CPetActor(LPCHARACTER owner, uint32_t vnum, const SPetAbility& petAbility, uint32_t options = EPetOption_Followable | EPetOption_Summonable);

	virtual ~CPetActor();

	virtual bool	Update(uint32_t deltaTime);

protected:
	virtual bool	_UpdateFollowAI();				///< AI processing that follows the owner
	virtual bool	_UpdatAloneActionAI(float fMinDist, float fMaxDist);			///< AI processing playing alone near the owner

	/// @TODO
	//virtual bool	_UpdateCombatAI();

private:
	bool Follow(float fMinDistance = 50.f);

public:
	LPCHARACTER		GetCharacter()	const					{ return m_pChar; }
	LPCHARACTER		GetOwner()	const						{ return m_pOwner; }
	uint32_t			GetVID() const							{ return m_dwVID; }
	uint32_t			GetVnum() const							{ return m_dwVnum; }

	bool			HasOption(EPetOptions option) const		{ return m_dwOptions & option; }

	void			SetName(const char* petName);

	bool			Mount();
	void			Unmount();

	uint32_t			Summon(const char* petName, LPITEM pSummonItem, bool bSpawnFar = false);
	void			Unsummon();

	bool			IsSummoned() const			{ return 0 != m_pChar; }
	void			SetSummonItem (LPITEM pItem);
	uint32_t			GetSummonItemVID () { return m_dwSummonItemVID; }

	void			GiveBuff();
	void			ClearBuff();

private:
	uint32_t			m_dwVnum;
	uint32_t			m_dwVID;
	uint32_t			m_dwOptions;
	uint32_t			m_dwLastActionTime;
	uint32_t			m_dwSummonItemVID;
	uint32_t			m_dwSummonItemVnum;

	int16_t			m_originalMoveSpeed;

	std::string		m_name;

	LPCHARACTER		m_pChar;					// Instance of pet(CHARACTER)
	LPCHARACTER		m_pOwner;

};

/**
*/
class CPetSystem
{
public:
	typedef	boost::unordered_map<uint32_t,	CPetActor*>		TPetActorMap;		/// <VNUM, PetActor> map. (Is it possible for a character to have multiple pets with the same vnum..??)

public:
	CPetSystem(LPCHARACTER owner);
	virtual ~CPetSystem();

	CPetActor*	GetByVID(uint32_t vid) const;
	CPetActor*	GetByVnum(uint32_t vnum) const;

	bool		Update(uint32_t deltaTime);
	void		Destroy();

	size_t		CountSummoned() const;			///< Number of pets currently summoned (with actualized characters)

public:
	void		SetUpdatePeriod(uint32_t ms);

	CPetActor*	Summon(uint32_t mobVnum, LPITEM pSummonItem, const char* petName, bool bSpawnFar, uint32_t options = CPetActor::EPetOption_Followable | CPetActor::EPetOption_Summonable);

	void		Unsummon(uint32_t mobVnum, bool bDeleteFromList = false);
	void		Unsummon(CPetActor* petActor, bool bDeleteFromList = false);

	// TODO: Implemented when the real pet system enters. (For example, when adding information about a character's pet...)
	CPetActor*	AddPet(uint32_t mobVnum, const char* petName, const SPetAbility& ability, uint32_t options = CPetActor::EPetOption_Followable | CPetActor::EPetOption_Summonable | CPetActor::EPetOption_Combatable);

	void		DeletePet(uint32_t mobVnum);
	void		DeletePet(CPetActor* petActor);
	void		RefreshBuff();

private:
	TPetActorMap	m_petActorMap;
	LPCHARACTER		m_pOwner;					///< Owner of the pet system
	uint32_t			m_dwUpdatePeriod;			///< Update cycle (in ms)
	uint32_t			m_dwLastUpdateTime;
	LPEVENT			m_pPetSystemUpdateEvent;
};
