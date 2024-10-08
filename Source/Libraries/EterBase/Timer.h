#pragma once

#include <windows.h>
#include <Basic/Singleton.h>

class CTimer : public Singleton<CTimer>
{
	public:
		CTimer();
		virtual ~CTimer();

		void	Advance();
		void	Adjust(int32_t iTimeGap);
		void	SetBaseTime();

		float	GetCurrentSecond();
		uint32_t	GetCurrentMillisecond();

		float	GetElapsedSecond();
		uint32_t	GetElapsedMilliecond();

		void	UseCustomTime();

	protected:
		bool	m_bUseRealTime;
		uint32_t	m_dwBaseTime;
		uint32_t	m_dwCurrentTime;
		float	m_fCurrentTime;
		uint32_t	m_dwElapsedTime;
		int32_t		m_index;
};

BOOL	ELTimer_Init();

uint32_t	ELTimer_GetMSec();

VOID	ELTimer_SetServerMSec(uint32_t dwServerTime);
uint32_t	ELTimer_GetServerMSec();

VOID	ELTimer_SetFrameMSec();
uint32_t	ELTimer_GetFrameMSec();