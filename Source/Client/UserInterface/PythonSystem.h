#pragma once

#include "../../Libraries/EterLib/VideoModeList.h"
#include "../../Libraries/MilesLib/SoundManager.h"

class ClientConfig : public Singleton<ClientConfig>
{
	public:
		ClientConfig();
		~ClientConfig() = default;
		ClientConfig(const ClientConfig& other) = delete;
		ClientConfig(ClientConfig&& other) : m_videoModes(std::move(other.m_videoModes)), m_Config(std::move(other.m_Config)) {}
		ClientConfig& operator=(const ClientConfig& other) = delete;
		ClientConfig& operator=(ClientConfig&& other);

		enum EWindowModes { FULLSCREEN = 0, WINDOWED = 1, BOARDLESS = 2, BOARDLESS_FSCREEN = 3, };
		typedef struct SResolution { uint32_t width; uint32_t height; uint32_t bpp; uint32_t frequency[20]; uint8_t frequency_count; } TResolution;

		typedef struct SConfig
		{
			std::string m_szLang;

			// Display:
			uint32_t width;
			uint32_t height;
			uint32_t width_l;
			uint32_t height_l;
			uint32_t bpp;
			uint32_t frequency;

			int32_t gamma;
			uint8_t WindowMode;

			bool forceResolution;
			bool bShowFPS;
			bool darkMode;

			// Sound:
			float music_volume;
			float soundEffect_volume;

			// Mouse:
			bool is_software_cursor;
			uint8_t left_mouse;
			uint8_t right_mouse;
			uint8_t mouse_size;

			// Chat:
			bool bViewChat;
			bool bAllowEmojis;

			// Damage:
			bool bShowDamage;

			// Shop title:
			bool bShowSalesText;

			// Fog:
			bool bFogMode;

			// TextTail:
			bool bAlwaysShowNamePlayer;
			bool bAlwaysShowNameNpc;
			bool bAlwaysShowNameMonster;
			bool bAlwaysShowNameItem;

#if 0
			bool bAlwaysShowNamePrivateShop;

#endif
			// Shadows:
			int32_t iShadowTargetLevel;
			int32_t iShadowQualityLevel;
			bool bShadowMode;

#if 0
			// Monster Info
			bool bShowMobLevel;
			bool bShowMobAIFlag;
#endif
			bool bShowPlayerLanguage;
			bool bShowChatLanguage;
			bool bShowWhisperLanguage;

			// Not revisited!:
			int32_t iDistance;
			bool bNoAudio;
		} TConfig;

#ifdef _DEBUG
		typedef struct SDbgConfig
		{
			bool bInterpreter;
			std::string szSuperPath;
			bool bRunMain;
		} TDbgConfig;
#endif

		// Display:
		int32_t GetResolutionCount() const
		{
			return m_videoModes ? m_videoModes->count() : 0;
		};
		uint32_t GetWidth() const
		{
			return m_Config.width;
		};
		uint32_t GetHeight() const
		{
			return m_Config.height;
		};
		uint32_t GetWidthRes() const
		{
			return m_Config.width_l;
		};
		uint32_t GetHeightRes() const
		{
			return m_Config.height_l;
		};
		uint32_t GetBPP() const
		{
			return m_Config.bpp;
		};
		uint32_t GetFrequency() const
		{
			return m_Config.frequency;
		};
		uint32_t GetGamma() const
		{
			return m_Config.gamma;
		};
		uint8_t GetWindowMode() const
		{
			return m_Config.WindowMode;
		};
		bool IsDarkMode() const
		{
			return m_Config.darkMode;
		}
		void SetDarkMode(bool iFlag);

#ifdef _DEBUG
		bool IsInterpreterMode() const
		{
			return m_Dbg.bInterpreter;
		}
		bool RunMain() const
		{
			return m_Dbg.bRunMain;
		}
		std::string GetSuperPath() const
		{
			return m_Dbg.szSuperPath;
		}
#endif

		std::optional<std::tuple<uint32_t, uint32_t, uint32_t>> GetResolution(int32_t index);
		bool SetResolution(int32_t index);
		bool IsResolutionByDescription(const std::string& desc);
		void CreateVideoModeList();

		std::string GetLanguage() const
		{
			return m_Config.m_szLang;
		}

		uint8_t SetWindowMode(uint8_t windowmode)
		{
			return m_Config.WindowMode = windowmode;
		};

		bool IsShowFPS() const
		{
			return m_Config.bShowFPS;
		};
		void SetShowFPS(bool iFlag);
		bool IsForceResolution() const
		{
			return m_Config.forceResolution;
		};
		void SetForceResolution(bool iFlag);

		// Sound:
		float GetMusicVolume() const
		{
			return m_Config.music_volume;
		};
		float GetSoundVolume() const
		{
			return m_Config.soundEffect_volume;
		};
		void SetMusicVolume(float fVolume);
		void SetSoundVolumef(float fVolume);

		// Mouse:
		int GetMouseRight() const
		{
			return m_Config.right_mouse;
		};
		int GetMouseLeft() const
		{
			return m_Config.left_mouse;
		};
		bool IsSoftwareCursor() const
		{
			return m_Config.is_software_cursor;
		};
		void SetMouseRight(uint8_t RightMouse);
		void SetMouseLeft(uint8_t LeftMouse);
		void SetMouseSize(uint8_t m_size);
		int GetMouseSize() const
		{
			return m_Config.mouse_size;
		};

		// Chat:
		bool IsViewChat() const
		{
			return m_Config.bViewChat;
		};
		bool IsAllowEmojis() const
		{
			return m_Config.bAllowEmojis;
		};
		void SetViewChatFlag(bool iFlag);
		void SetAllowEmojis(bool iFlag);


		// Damage -> Not reviewed yet
		bool IsShowDamage();
		void SetShowDamageFlag(int32_t iFlag);

		// Shop title: -> Not reviewed yet
		bool IsShowSalesText();
		void SetShowSalesTextFlag(int32_t iFlag);

		// Fog:
		void SetFogMode(bool iFlag);
		bool IsFogMode() const
		{
			return m_Config.bFogMode;
		};

		// TextTail:
		bool IsAlwaysShowPlayerName() const
		{
			return m_Config.bAlwaysShowNamePlayer;
		};
		bool IsAlwaysShowMonsterName() const
		{
			return m_Config.bAlwaysShowNameMonster;
		};

		bool IsAlwaysShowNPCName() const
		{
			return m_Config.bAlwaysShowNameNpc;
		};

		bool IsAlwaysShowItemName() const
		{
			return m_Config.bAlwaysShowNameItem;
		};
#if 0

		bool IsAlwaysShowPrivateShopName() const
		{
			return m_Config.bAlwaysShowNamePrivateShop;
		};
#endif

		void SetAlwaysShowPlayerNameFlag(bool iFlag);
		void SetAlwaysShowNPCNameFlag(bool iFlag);
		void SetAlwaysShowMonsterNameFlag(bool iFlag);
		void SetAlwaysShowItemNameFlag(bool iFlag);

#if 0
		void SetAlwaysShowPrivateShopNameFlag(bool iFlag);
#endif
		// Shadows:
		int32_t	GetShadowQualityLevel() const
		{
			return m_Config.iShadowQualityLevel;
		};

		int32_t	GetShadowTargetLevel() const
		{
			return m_Config.iShadowTargetLevel;
		};

		void SetShadowTargetLevel(uint32_t level);
		bool IsDynamicShadow() const
		{
			return m_Config.bShadowMode;
		};
		
		void SetShadowQualityLevel(uint32_t level);
		void SetShadowMode(bool iFlag);

#if 0
		// Monster Info
		void SetShowMobAIFlag(bool iFlag);
		bool IsShowMobAIFlag() const
		{
			return m_Config.bShowMobAIFlag;
		};
		void SetShowMobLevel(bool iFlag);
		bool IsShowMobLevel() const
		{
			return m_Config.bShowMobLevel;
		};
#endif

		void SetShowPlayerLanguage(bool iFlag) { m_Config.bShowPlayerLanguage = iFlag; };
		bool IsShowPlayerLanguage() const  { return m_Config.bShowPlayerLanguage; };

		void SetShowChatLanguage(bool iFlag) { m_Config.bShowChatLanguage = iFlag; };
		bool IsShowChatLanguage() const  { return m_Config.bShowChatLanguage; };

		void SetShowWhisperLanguage(bool iFlag) { m_Config.bShowWhisperLanguage = iFlag; };
		bool IsShowWhisperLanguage() const { return m_Config.bShowWhisperLanguage; };

		// Not revisited!	
		int32_t GetDistance();

		bool HaveAudio() const { return m_Config.bNoAudio; }

		// Config Related
		void SetDefaultConfig();
		bool LoadConfig();
		bool SaveConfig();
		void SetConfig(TConfig* set_config);
		void AdjustPlayArea();
		TConfig* GetConfig();
protected:
	std::unique_ptr<VideoModeList> m_videoModes;
	TConfig m_Config;
#ifdef _DEBUG
	TDbgConfig m_Dbg;
#endif
};
