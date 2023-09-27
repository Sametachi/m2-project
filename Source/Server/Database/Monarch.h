#pragma once

#include <LibThecore/include/stdafx.h>
#include <map>
#include <vector>
#include <Basic/Singleton.h>
#include <Common/tables.h>

class CMonarch : public Singleton<CMonarch>
{
public:
		typedef std::map<uint32_t, MonarchElectionInfo*> MAP_MONARCHELECTION;
		typedef std::vector<MonarchCandidacy> VEC_MONARCHCANDIDACY;

		CMonarch();
		virtual ~CMonarch();

		bool VoteMonarch(uint32_t pid, uint32_t selectedpid);
		void ElectMonarch();

		bool IsCandidacy(uint32_t pid);
		bool AddCandidacy(uint32_t pid, const char* name);
		bool DelCandidacy(const char* name);

		bool LoadMonarch();
		bool SetMonarch(const char* name);
		bool DelMonarch(int32_t Empire);
		bool DelMonarch(const char* name);

		bool IsMonarch(int32_t Empire, uint32_t pid);
		bool AddMoney(int32_t Empire, int64_t Money);
		bool DecMoney(int32_t Empire, int64_t Money);
		bool TakeMoney(int32_t Empire, uint32_t pid, int64_t Money);

		TMonarchInfo* GetMonarch()
		{
			return &m_MonarchInfo;
		}

		VEC_MONARCHCANDIDACY& GetVecMonarchCandidacy()
		{
			return m_vec_MonarchCandidacy;
		}

		size_t MonarchCandidacySize()
		{
			return m_vec_MonarchCandidacy.size();
		}

private:
		int32_t GetCandidacyIndex(uint32_t pid);

		MAP_MONARCHELECTION m_map_MonarchElection;
		VEC_MONARCHCANDIDACY m_vec_MonarchCandidacy;
		TMonarchInfo m_MonarchInfo;

		MonarchElectionInfo* GetMonarchElection(uint32_t pid)
		{
			auto it = m_map_MonarchElection.find(pid);

			if (it != m_map_MonarchElection.end())
				return it->second;

			return NULL;
		}
};
