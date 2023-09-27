#include "StdAfx.h"
#include "Engine.h"


FontManager fontManager;
FontManager& Engine::m_fontManager = fontManager;

//ClientConfig clientConfig;
//ClientConfig& Engine::m_config = clientConfig;

CGraphicDevice* Engine::m_device = nullptr;
KeyboardInput* Engine::m_keyboard = nullptr;

