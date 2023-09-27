#pragma once
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlcrack.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlddx.h>
#include "resource.h"

#define MUSIC_VOLUME_MAX_NUM		1.0f

class CMainDialog : public CDialogImpl<CMainDialog>, public CWinDataExchange<CMainDialog>
{
	public:
		enum { IDD = IDR_MAINFRAME };

		typedef struct SResolution
		{
			DWORD	width;
			DWORD	height;
			DWORD	bpp;		// bits per pixel (high-color = 16bpp, true-color = 32bpp)

			DWORD	frequency[20];
			BYTE	frequency_count;
		} TResolution;

		typedef struct SConfig
		{
			DWORD			width;
			DWORD			height;
			DWORD			bpp;
			DWORD			frequency;

			bool			is_object_culling;
			int				iDistance;
			int				iTiling;
			int				iShadowLevel;

			FLOAT			music_volume;
			int				voice_volume;

			int				gamma;

			BOOL			bUseSoftwareCursor;
			BOOL			bFullscreen;

			unsigned int language;
		} TConfig;

		enum
		{
			FREQUENCY_MAX_NUM  = 30,
			RESOLUTION_MAX_NUM = 64,

			SOUND_VOLUME_MAX_NUM = 5,

			GAMMA_MAX_NUM = 5,

			VISIBILITY_MAX_NUM = 3,
			TILING_MAX_NUM = 3,
			SHADOW_MAX_NUM = 4,
		};

		CMainDialog();

		BEGIN_MSG_MAP(CMainDialog)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDC_VIDEO_RESOLUTION, CBN_SELCHANGE, OnResolutionSelChange)
			COMMAND_HANDLER_EX(IDC_OPTIMIZE_TILING, CBN_SELCHANGE, OnTilingSelChange)
			COMMAND_ID_HANDLER_EX(IDOK, OnOK)
			COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		END_MSG_MAP()

		BEGIN_DDX_MAP(CMainDialog)
			DDX_CONTROL_HANDLE(IDC_VIDEO_RESOLUTION, m_comboResolution)
			DDX_CONTROL_HANDLE(IDC_VIDEO_FREQUENCY, m_comboFrequency)
			DDX_CONTROL_HANDLE(IDC_VIDEO_GAMMA, m_comboGamma)
			DDX_CONTROL_HANDLE(IDC_SOUND_BGM, m_sliderBGMVolume)
			DDX_CONTROL_HANDLE(IDC_SOUND_SOUND, m_sliderSoundVolume)
			DDX_CONTROL_HANDLE(IDC_OPTIMIZE_VISIBILITY, m_comboVisibility)
			DDX_CONTROL_HANDLE(IDC_OPTIMIZE_TILING, m_comboTiling)
			DDX_CONTROL_HANDLE(IDC_OPTIMIZE_SHADOW, m_comboShadow)
			DDX_CONTROL_HANDLE(IDC_LANGUAGE_COMBO, m_comboLanguage)

			DDX_RADIO(IDC_GRAPHIC_WINDOW_MODE, m_stConfig.bFullscreen)
			DDX_CHECK(IDC_GRAPHIC_SOFTWARE_CURSOR, m_stConfig.bUseSoftwareCursor)
		END_DDX_MAP()

		BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);

		void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
		void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
		void OnResolutionSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);
		void OnTilingSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);

		void GetResolutionList();
		void InitDefaultConfig();
		void LoadConfig();
		void SaveConfig();
		void ApplyConfig();
		void UpdateFrequencyComboBox();

	protected:
		typedef ATL::CSimpleArray<CString> TStringVector;

		WTL::CComboBox m_comboResolution;
		WTL::CComboBox m_comboFrequency;
		WTL::CComboBox m_comboGamma;

		WTL::CTrackBarCtrl m_sliderBGMVolume;
		WTL::CTrackBarCtrl m_sliderSoundVolume;

		WTL::CComboBox m_comboVisibility;
		WTL::CComboBox m_comboTiling;
		WTL::CComboBox m_comboShadow;
		WTL::CComboBox m_comboLanguage;

		TStringVector m_ShadowNameVector;

	protected:
		TConfig m_stConfig;
		TResolution m_ResolutionList[RESOLUTION_MAX_NUM];
		int m_nResolutionCount;
};
