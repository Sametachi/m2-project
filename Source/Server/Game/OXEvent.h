#pragma once

#define OXEVENT_MAP_INDEX 113

struct tag_Quiz
{
	char level;
	char Quiz[256];
	bool answer;
};

enum OXEventStatus
{
	OXEVENT_FINISH = 0, 
	OXEVENT_OPEN = 1,
	OXEVENT_CLOSE = 2,
	OXEVENT_QUIZ = 3,

	OXEVENT_ERR = 0xff
};

class COXEventManager : public Singleton<COXEventManager>
{
	private :
		std::map<uint32_t, uint32_t> m_map_char;
		std::map<uint32_t, uint32_t> m_map_attender;
		std::map<uint32_t, uint32_t> m_map_miss;

		std::vector<std::vector<tag_Quiz> > m_vec_quiz;

		LPEVENT m_timedEvent;

	protected :
		bool CheckAnswer();

		bool EnterAudience(LPCHARACTER pChar);
		bool EnterAttender(LPCHARACTER pChar);

	public :
		bool Initialize();
		void Destroy();

		OXEventStatus GetStatus();
		void SetStatus(OXEventStatus status);

		bool LoadQuizScript(const char* szFileName);

		bool Enter(LPCHARACTER pChar);
		
		bool CloseEvent();

		void ClearQuiz();
		bool AddQuiz(uint8_t level, const char* pszQuestion, bool answer);
		bool ShowQuizList(LPCHARACTER pChar);

		bool Quiz(uint8_t level, int32_t timelimit);
		bool GiveItemToAttender(uint32_t dwItemVnum, uint8_t count);

		bool CheckAnswer(bool answer);
		void WarpToAudience();

		bool LogWinner();

		uint32_t GetAttenderCount() { return m_map_attender.size(); }
};

