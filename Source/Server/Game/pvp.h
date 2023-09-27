#pragma once

class CHARACTER;

class CPVP
{
	public:
		friend class CPVPManager;

		typedef struct _player
		{
			uint32_t	dwPID;
			uint32_t	dwVID;
			bool	bAgree;
			bool	bCanRevenge;

			_player() : dwPID(0), dwVID(0), bAgree(false), bCanRevenge(false)
			{
			}
		} TPlayer;

		CPVP(uint32_t dwID1, uint32_t dwID2);
		CPVP(CPVP & v);
		~CPVP();

		void	Win(uint32_t dwPID);
		bool	CanRevenge(uint32_t dwPID);
		bool	IsFight();
		bool	Agree(uint32_t dwPID);

		void	SetVID(uint32_t dwPID, uint32_t dwVID);
		void	Packet(bool bDelete = false);

		void	SetLastFightTime();
		uint32_t	GetLastFightTime();

		uint32_t 	GetCRC() { return m_dwCRC; }

	protected:
		TPlayer	m_players[2];
		uint32_t	m_dwCRC;
		bool	m_bRevenge;

		uint32_t   m_dwLastFightTime;
};

class CPVPManager : public Singleton<CPVPManager>
{
	typedef std::map<uint32_t, TR1_NS::unordered_set<CPVP*> > CPVPSetMap;

	public:
	CPVPManager();
	virtual ~CPVPManager();

	void			Insert(LPCHARACTER pChr, LPCHARACTER pVictim);
	bool			CanAttack(LPCHARACTER pChr, LPCHARACTER pVictim);
	bool			Dead(LPCHARACTER pChr, uint32_t dwKillerPID);
	void			GiveUp(LPCHARACTER pChr, uint32_t dwKillerPID);
	void			Connect(LPCHARACTER pChr);
	void			Disconnect(LPCHARACTER pChr);

	void			SendList(LPDESC d);
	void			Delete(CPVP* pPVP);

	void			Process();

	public:
	CPVP *			Find(uint32_t dwCRC);
	protected:
	void			ConnectEx(LPCHARACTER pChr, bool bDisconnect);

	std::map<uint32_t, CPVP *>	m_map_pPVP;
	CPVPSetMap		m_map_pPVPSetByID;
};
