#include "MainDialog.h"
#include <boost/algorithm/clamp.hpp>
#include <d3d9.h>
#include <algorithm>
#include <math.h>

#define APP_NAME _T("Metin2 Config")
#define FILENAME_CONFIG "metin2.cfg"
#define LOCALE_CONFIG "locale.cfg"

static const unsigned int kVisibilityNames[CMainDialog::VISIBILITY_MAX_NUM] =
{
	IDS_FOG_NEAR, IDS_FOG_MID, IDS_FOG_FAR
};

static const unsigned int kTilingNames[CMainDialog::TILING_MAX_NUM] =
{
	IDS_TILE_AUTO, IDS_TILE_CPU, IDS_TILE_GPU
};

static const unsigned int kShadowNames[CMainDialog::SHADOW_MAX_NUM] =
{
	IDS_SHADOW_NONE, IDS_SHADOW_LV1, IDS_SHADOW_LV2, IDS_SHADOW_ALL
};

enum Language
{
	kLanguageEnglish,
	kLanguageGerman,
	kLanguageSpanish,
	kLanguagePolish,
	kLanguagePortuguese,
	kLanguageRomanian,
	kLanguageRussian,

	kLanguageMax
};

struct LanguageConfig
{
	int stringId;
	const char* path;
}

kLanguageConfig[] = 
{
	{IDS_LANGUAGE_ENGLISH, "en"},
	{IDS_LANGUAGE_GERMAN, "de"},
	{IDS_LANGUAGE_SPANISH, "es"},
	{IDS_LANGUAGE_POLISH, "pl"},
	{IDS_LANGUAGE_PORTUGUESE, "pt"},
	{IDS_LANGUAGE_ROMANIAN, "ro"},
	{IDS_LANGUAGE_RUSSIAN, "ru"},
};

CMainDialog::CMainDialog()
{
	InitDefaultConfig();
}

BOOL CMainDialog::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	CenterWindow();

	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);

	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	GetResolutionList();

	LoadConfig();

	DoDataExchange(FALSE);

	// Screen Resolution
	CString strResolutionName;
	for (int i = 0; i < m_nResolutionCount; i++)
	{
		strResolutionName.Format(_T("%dx%d %dbpp"), m_ResolutionList[i].width, m_ResolutionList[i].height, m_ResolutionList[i].bpp);
		m_comboResolution.InsertString(i, strResolutionName);
	}

	// Resolution Selection
	strResolutionName.Format(_T("%dx%d %dbpp"), m_stConfig.width, m_stConfig.height, m_stConfig.bpp);
	int iResolutionSel = m_comboResolution.FindString(0, strResolutionName);

	if (iResolutionSel < 0)
		m_comboResolution.SetCurSel(0);
	else
		m_comboResolution.SetCurSel(iResolutionSel);

	// Frequency
	UpdateFrequencyComboBox();

	CString strFrequency;
	strFrequency.Format(_T("%d"), m_stConfig.frequency);

	int iFrequencySel = m_comboFrequency.FindString(0, strFrequency);
	if (iFrequencySel < 0)
		m_comboFrequency.SetCurSel(0);
	else
		m_comboFrequency.SetCurSel(iFrequencySel);

	// Gamma
	CString strGammaName;
	for (int i = 0; i < GAMMA_MAX_NUM; i++) 
	{
		strGammaName.Format(_T("%d"), i + 1);
		m_comboGamma.InsertString(i, strGammaName);
	}

	// Gamma Selection
	if (m_stConfig.gamma > 0 && m_stConfig.gamma <= GAMMA_MAX_NUM)
		m_comboGamma.SetCurSel(m_stConfig.gamma - 1);
	else
		m_comboGamma.SetCurSel(0);

	// Visibility
	CString strVisibility;
	for (int i = 0; i < VISIBILITY_MAX_NUM; i++) 
	{
		strVisibility.LoadString(kVisibilityNames[i]);
		m_comboVisibility.InsertString(i, strVisibility);
	}

	if (m_stConfig.iDistance > 0 && m_stConfig.iDistance <= VISIBILITY_MAX_NUM)
		m_comboVisibility.SetCurSel(m_stConfig.iDistance - 1);
	else
		m_comboVisibility.SetCurSel(0);

	// Tiling
	CString strTiling;
	for (int i = 0; i < TILING_MAX_NUM; i++) 
	{
		strTiling.LoadString(kTilingNames[i]);
		m_comboTiling.InsertString(i, strTiling);
	}

	m_stConfig.iTiling = std::max<int>(m_stConfig.iTiling, 0);
	m_stConfig.iTiling = std::min<int>(m_stConfig.iTiling, TILING_MAX_NUM - 1);

	m_comboTiling.SetCurSel(m_stConfig.iTiling);

	// Shadow
	CString strShadow;
	for (int i = 0; i < SHADOW_MAX_NUM; i++) 
	{
		strShadow.LoadString(kShadowNames[i]);
		m_comboShadow.InsertString(i, strShadow);
	}

	m_stConfig.iShadowLevel = std::max<int>(m_stConfig.iShadowLevel, 0);
	m_stConfig.iShadowLevel = std::min<int>(m_stConfig.iShadowLevel, SHADOW_MAX_NUM - 1);
	m_comboShadow.SetCurSel(m_stConfig.iShadowLevel);

	// Language
	CString language;
	for (int i = 0; i < kLanguageMax; ++i) 
	{
		language.LoadString(kLanguageConfig[i].stringId);
		m_comboLanguage.InsertString(i, language);
	}

	m_stConfig.language = boost::algorithm::clamp(m_stConfig.language,
		0, kLanguageMax - 1);

	m_comboLanguage.SetCurSel(m_stConfig.language);

	// Sound
	m_sliderBGMVolume.SetRange(0, 1000, true);
	m_sliderSoundVolume.SetRange(0, 1000, true);

	if (m_stConfig.music_volume >= 0.0f &&
		m_stConfig.music_volume <= MUSIC_VOLUME_MAX_NUM)
		m_sliderBGMVolume.SetPos(int(m_stConfig.music_volume * 1000.0f));

	if (m_stConfig.voice_volume >= 0 &&
		m_stConfig.voice_volume <= SOUND_VOLUME_MAX_NUM)
		m_sliderSoundVolume.SetPos(m_stConfig.voice_volume * 200);

	return TRUE;
}

void CMainDialog::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (DoDataExchange(TRUE) == FALSE) 
	{
		//MessageBox();
	}

	ApplyConfig();
	SaveConfig();

	EndDialog(nID);
}

void CMainDialog::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(nID);
}

// Utility functions
void CMainDialog::InitDefaultConfig()
{
	memset(&m_stConfig, 0, sizeof(m_stConfig));

	m_stConfig.width = 800;
	m_stConfig.height = 600;
	m_stConfig.frequency = 60;
	m_stConfig.bpp = 16;

	m_stConfig.bUseSoftwareCursor = false;
	m_stConfig.iDistance = 3;
	m_stConfig.iTiling = 0;
	m_stConfig.iShadowLevel = 3;

	m_stConfig.gamma = 3;
	m_stConfig.music_volume = 1.0f;
	m_stConfig.voice_volume = 5;

	m_stConfig.bFullscreen = true;

	auto lang = PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID()));
	switch (lang) {
	case LANG_GERMAN:
		m_stConfig.language = kLanguageGerman;
		break;

	default:
		m_stConfig.language = kLanguageEnglish;
		break;
	}
}

void CMainDialog::LoadConfig()
{
	char buf[256];
	char command[256];
	char value[256];

	FILE* fp = fopen(FILENAME_CONFIG, "rt");
	if (fp == NULL)
		return;

	while (fgets(buf, 256, fp)) 
	{
		if (sscanf(buf, " %s %s \n", command, value) == EOF)
			break;

		if (!_stricmp(command, "WIDTH"))
			m_stConfig.width = atoi(value);
		else if (!_stricmp(command, "HEIGHT"))
			m_stConfig.height = atoi(value);
		else if (!_stricmp(command, "BPP"))
			m_stConfig.bpp = atoi(value);
		else if (!_stricmp(command, "FREQUENCY"))
			m_stConfig.frequency = atoi(value);
		else if (!_stricmp(command, "SOFTWARE_CURSOR"))
			m_stConfig.bUseSoftwareCursor = atoi(value) ? true : false;
		else if (!_stricmp(command, "VISIBILITY"))
			m_stConfig.iDistance = atoi(value);
		else if (!_stricmp(command, "SOFTWARE_TILING"))
			m_stConfig.iTiling = atoi(value);
		else if (!_stricmp(command, "SHADOW_LEVEL"))
			m_stConfig.iShadowLevel = atoi(value);
		else if (!_stricmp(command, "MUSIC_VOLUME"))
			m_stConfig.music_volume = (float)atof(value);
		else if (!_stricmp(command, "VOICE_VOLUME"))
			m_stConfig.voice_volume = (char)atoi(value);
		else if (!_stricmp(command, "GAMMA"))
			m_stConfig.gamma = std::max<int>(1, std::min<int>(GAMMA_MAX_NUM, atoi(value)));
		else if (!_stricmp(command, "WINDOWED"))
			m_stConfig.bFullscreen = atoi(value) == 1 ? false : true;
	}

	fclose(fp);

	fp = fopen(LOCALE_CONFIG, "rt");
	if (!fp)
		return;

	fgets(buf, 256, fp);
	sscanf(buf, " %s ", command);

	for (int i = 0; i < kLanguageMax; ++i) 
	{
		const auto& cfg = kLanguageConfig[i];
		if (0 == std::strcmp(command, cfg.path))
			m_stConfig.language = i;
	}

	fclose(fp);
}

void CMainDialog::SaveConfig()
{
	FILE* fp = fopen(FILENAME_CONFIG, "wt");
	if (fp == NULL)
		return;

	fprintf(fp, "WIDTH					%d\n", m_stConfig.width);
	fprintf(fp, "HEIGHT				%d\n", m_stConfig.height);
	fprintf(fp, "BPP					%d\n", m_stConfig.bpp);
	fprintf(fp, "FREQUENCY				%d\n", m_stConfig.frequency);
	fprintf(fp, "SOFTWARE_CURSOR		%d\n", m_stConfig.bUseSoftwareCursor);
	fprintf(fp, "VISIBILITY			%d\n", m_stConfig.iDistance);
	fprintf(fp, "SOFTWARE_TILING		%d\n", m_stConfig.iTiling);
	fprintf(fp, "SHADOW_LEVEL			%d\n", m_stConfig.iShadowLevel);
	fprintf(fp, "MUSIC_VOLUME			%.3f\n", m_stConfig.music_volume);
	fprintf(fp, "VOICE_VOLUME			%d\n", m_stConfig.voice_volume);
	fprintf(fp, "GAMMA					%d\n", m_stConfig.gamma);
	fprintf(fp, "WINDOWED				%d\n", !m_stConfig.bFullscreen);
	fprintf(fp, "\n");
	fclose(fp);

	fp = fopen(LOCALE_CONFIG, "wt");
	if (!fp)
		return;

	fprintf(fp, "%s", kLanguageConfig[m_stConfig.language].path);
	fclose(fp);
}

void CMainDialog::UpdateFrequencyComboBox()
{
	int iScreenCurrentSelection = m_comboResolution.GetCurSel();

	m_comboFrequency.ResetContent();

	CString strFrequencyName;
	for (int i = 0; i < m_ResolutionList[iScreenCurrentSelection].frequency_count; i++) 
	{
		strFrequencyName.Format(_T("%d"), m_ResolutionList[iScreenCurrentSelection].frequency[i]);
		m_comboFrequency.InsertString(i, strFrequencyName);
	}

	m_comboFrequency.SetCurSel(m_ResolutionList[iScreenCurrentSelection].frequency_count - 1);
}

void CMainDialog::GetResolutionList()
{
	m_nResolutionCount = 0;

	auto lpd3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!lpd3d) 
	{
		m_ResolutionList[m_nResolutionCount].width = 800;
		m_ResolutionList[m_nResolutionCount].height = 600;
		m_ResolutionList[m_nResolutionCount].bpp = 16;
		m_ResolutionList[m_nResolutionCount].frequency[0] = 60;
		m_ResolutionList[m_nResolutionCount].frequency_count = 1;

		++m_nResolutionCount;
		return;
	}

	const D3DFORMAT allowedFormats[] = 
	{
		D3DFMT_X8R8G8B8,
		D3DFMT_R5G6B5,
	};

	for (int i = 0; i < sizeof(allowedFormats) / sizeof(D3DFORMAT); ++i) 
	{
		DWORD dwNumAdapterModes = lpd3d->GetAdapterModeCount(0, allowedFormats[i]);
		if (dwNumAdapterModes == 0) 
		{
			m_ResolutionList[m_nResolutionCount].width = 800;
			m_ResolutionList[m_nResolutionCount].height = 600;
			m_ResolutionList[m_nResolutionCount].bpp = 16;
			m_ResolutionList[m_nResolutionCount].frequency[0] = 60;
			m_ResolutionList[m_nResolutionCount].frequency_count = 1;

			++m_nResolutionCount;
			lpd3d->Release();
			return;
		}

		for (UINT iMode = 0; iMode < dwNumAdapterModes; iMode++) 
		{
			D3DDISPLAYMODE displayMode;
			lpd3d->EnumAdapterModes(0, allowedFormats[i], iMode, &displayMode);

			if (displayMode.Width < 800 || displayMode.Height < 600)
				continue;

			int bpp = 0;
			if (allowedFormats[i] == D3DFMT_R5G6B5)
				bpp = 16;
			else if (allowedFormats[i] == D3DFMT_X8R8G8B8)
				bpp = 32;

			int check_res = false;
			for (int i = 0; !check_res && i < m_nResolutionCount; ++i) 
			{
				if (m_ResolutionList[i].bpp != bpp ||
					m_ResolutionList[i].width != displayMode.Width ||
					m_ResolutionList[i].height != displayMode.Height)
					continue;

				int check_fre = false;

				for (int j = 0; j < m_ResolutionList[i].frequency_count; ++j) 
				{
					if (m_ResolutionList[i].frequency[j] == displayMode.RefreshRate) 
					{
						check_fre = true;
						break;
					}
				}

				if (!check_fre)
					if (m_ResolutionList[i].frequency_count < FREQUENCY_MAX_NUM)
						m_ResolutionList[i].frequency[m_ResolutionList[i].frequency_count++] = displayMode.RefreshRate;

				check_res = true;
			}

			if (!check_res) 
			{
				if (m_nResolutionCount < RESOLUTION_MAX_NUM) 
				{
					m_ResolutionList[m_nResolutionCount].width = displayMode.Width;
					m_ResolutionList[m_nResolutionCount].height = displayMode.Height;
					m_ResolutionList[m_nResolutionCount].bpp = bpp;
					m_ResolutionList[m_nResolutionCount].frequency[0] = displayMode.RefreshRate;
					m_ResolutionList[m_nResolutionCount].frequency_count = 1;

					++m_nResolutionCount;
				}
			}
		}
	}

	lpd3d->Release();
}

void CMainDialog::ApplyConfig()
{
	int iResolutionSel = m_comboResolution.GetCurSel();
	if ((iResolutionSel >= 0) && (iResolutionSel < m_nResolutionCount)) 
	{
		m_stConfig.width = m_ResolutionList[iResolutionSel].width;
		m_stConfig.height = m_ResolutionList[iResolutionSel].height;
		m_stConfig.bpp = m_ResolutionList[iResolutionSel].bpp;

		int iFrequencySel = m_comboFrequency.GetCurSel();
		if ((iFrequencySel >= 0) && (iFrequencySel < m_ResolutionList[iResolutionSel].frequency_count))
			m_stConfig.frequency = m_ResolutionList[iResolutionSel].frequency[iFrequencySel];
		else
			m_stConfig.frequency = 60;
	}
	else 
	{
		m_stConfig.width = 800;
		m_stConfig.height = 600;
		m_stConfig.bpp = 16;
		m_stConfig.frequency = 60;
	}

	m_stConfig.gamma = m_comboGamma.GetCurSel() + 1;
	m_stConfig.music_volume = float(m_sliderBGMVolume.GetPos()) / 1000.0f;
	m_stConfig.voice_volume = m_sliderSoundVolume.GetPos() / 200;

	m_stConfig.iDistance = m_comboVisibility.GetCurSel() + 1;
	m_stConfig.iTiling = m_comboTiling.GetCurSel();
	m_stConfig.iShadowLevel = m_comboShadow.GetCurSel();
	m_stConfig.language = m_comboLanguage.GetCurSel();
}

void CMainDialog::OnResolutionSelChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	UpdateFrequencyComboBox();
}

void CMainDialog::OnTilingSelChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	switch (m_comboTiling.GetCurSel()) 
	{
	case 0:
		break;

	case 1: 
	{
		CString strCaption((LPCTSTR)IDS_NOTIFY);
		CString strDescription((LPCTSTR)IDS_TILING_CPU);
		MessageBox(strDescription, strCaption);
		break;
	}

	case 2: 
	{
		CString strCaption((LPCTSTR)IDS_NOTIFY);
		CString strDescription((LPCTSTR)IDS_TILING_GPU);
		MessageBox(strDescription, strCaption);
		break;
	}
	}
}
