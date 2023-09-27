#pragma once
#include "StdAfx.h"
#include "GrpFontManager.h"

class CGraphicDevice;
class KeyboardInput;
class Engine
{
public:
	//inline static ClientConfig& GetSettings()
	//{
	//	return m_config;
	//}

	inline static FontManager& GetFontManager()
	{
		return m_fontManager;
	}

    inline static CGraphicDevice& GetDevice()
    {
        _ASSERT(m_device != nullptr);
        return *m_device;
    }

    inline static CGraphicDevice* GetDevicePtr()
    {
        return m_device;
    }

    static void RegisterDevice(CGraphicDevice* pDevice)
    {
        m_device = pDevice;
    }

    static void SetKeyboardInput(KeyboardInput* keyboard)
    {
        m_keyboard = keyboard;
    }

    inline static KeyboardInput& GetKeyboardInput()
    {
        assert(nullptr != m_keyboard && "KeyboardInput not set");
        return *m_keyboard;
    }

private:
	//static ClientConfig& m_config;
	static FontManager& m_fontManager;
    static CGraphicDevice* m_device;
    static KeyboardInput* m_keyboard;
};