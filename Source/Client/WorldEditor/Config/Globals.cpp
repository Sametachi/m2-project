#include "stdafx.h"
#include "Globals.h"

namespace globals
{
namespace dft
{
bool VIEW_CHAR_OUTPUT_BY_DEFAULT		= false;
bool VIEW_SHADOW_OUTPUT_BY_DEFAULT		= false;
bool VIEW_WATER_OUTPUT_BY_DEFAULT		= false;

int32_t WINDOW_HEIGHT_SIZE			= 768;
int32_t WINDOW_WIDTH_SIZE			= 1024;
float WINDOW_FOV_SIZE			= 40.0f;

float WASD_MINIMAL_MOVE				= 100.0f;

bool NO_GOTO_AFTER_INSERT			= false;
bool NOMAI_ATLAS_DUMP			= false;
bool NOMINIMAP_RAWALPHA		= false;
bool DETECT_MDATR_HEIGHT	= false;
bool NOFOG_ONMAPLOAD		= false;
bool REFRESHALL_ONUPDATEUI	= false;

bool NEWMAP_TEXTURESETSAVEASMAPNAME = false;
std::string NEWMAP_TEXTURESETLOADPATH = "";
std::string NEW_MAP_MAPNAME_PREFIX = "";

bool SERVERATTR_REMOVE_WEIRD_FLAGS = true;
bool VIEW_OBJECT_LIGHTING = true;
std::string MOB_PROTO_PATH(R"(locale/ymir/mob_proto)");
bool VIEW_MONSTER_AREA_INFO = false;

float RENDER_CURSOR_COLOR_R = 0.f;
float RENDER_CURSOR_COLOR_G = 1.f;
float RENDER_CURSOR_COLOR_B = 0.f;

int32_t OBJECT_HEIGHT_SLIDER_MAX = 3000;
float OBJECT_HEIGHT_MAX = 5000.f;

bool ATTR_SLIDER_REMOVAL = true;
#ifdef CWE_CENTERED_WINDOW
bool CENTERED_WINDOW = true;
#endif

int32_t AUTO_SAVE_TIME = 300;
#ifdef CWE_MULTI_LANGUAGE
std::string LOCALE = "EN";
#endif
}

bool readGlobalConfigs(const char* szConfigName)
{
#ifdef USE_WE_CONFIG
	INIReader reader(szConfigName);
	if (reader.ParseError() < 0)
	{
		Tracef("WorldEditorConfig: %s not found or broken\n", szConfigName);
		return false;
	}
	else
	{
		if (reader.HasValue("config", "VIEW_CHAR_OUTPUT_BY_DEFAULT"))
		{
			globals::dft::VIEW_CHAR_OUTPUT_BY_DEFAULT = reader.GetBoolean("config", "VIEW_CHAR_OUTPUT_BY_DEFAULT");
			Tracef("WorldEditorConfig: VIEW_CHAR_OUTPUT_BY_DEFAULT %d\n", globals::dft::VIEW_CHAR_OUTPUT_BY_DEFAULT);
		}
		if (reader.HasValue("config", "VIEW_SHADOW_OUTPUT_BY_DEFAULT"))
		{
			globals::dft::VIEW_SHADOW_OUTPUT_BY_DEFAULT = reader.GetBoolean("config", "VIEW_SHADOW_OUTPUT_BY_DEFAULT");
			Tracef("WorldEditorConfig: VIEW_SHADOW_OUTPUT_BY_DEFAULT %d\n", globals::dft::VIEW_SHADOW_OUTPUT_BY_DEFAULT);
		}
		if (reader.HasValue("config", "VIEW_WATER_OUTPUT_BY_DEFAULT"))
		{
			globals::dft::VIEW_WATER_OUTPUT_BY_DEFAULT = reader.GetBoolean("config", "VIEW_WATER_OUTPUT_BY_DEFAULT");
			Tracef("WorldEditorConfig: VIEW_WATER_OUTPUT_BY_DEFAULT %d\n", globals::dft::VIEW_WATER_OUTPUT_BY_DEFAULT);
		}

		if (reader.HasValue("config", "WINDOW_HEIGHT_SIZE"))
		{
			globals::dft::WINDOW_HEIGHT_SIZE = reader.GetInteger("config", "WINDOW_HEIGHT_SIZE");
			Tracef("WorldEditorConfig: WINDOW_HEIGHT_SIZE %u\n", globals::dft::WINDOW_HEIGHT_SIZE);
		}
		if (reader.HasValue("config", "WINDOW_WIDTH_SIZE"))
		{
			globals::dft::WINDOW_WIDTH_SIZE = reader.GetInteger("config", "WINDOW_WIDTH_SIZE");
			Tracef("WorldEditorConfig: WINDOW_WIDTH_SIZE %u\n", globals::dft::WINDOW_WIDTH_SIZE);
		}
		if (reader.HasValue("config", "WINDOW_FOV_SIZE"))
		{
			globals::dft::WINDOW_FOV_SIZE = reader.GetFloat("config", "WINDOW_FOV_SIZE");
			Tracef("WorldEditorConfig: WINDOW_FOV_SIZE %.f\n", globals::dft::WINDOW_FOV_SIZE);
		}
		if (reader.HasValue("config", "WASD_MINIMAL_MOVE"))
		{
			globals::dft::WASD_MINIMAL_MOVE = reader.GetFloat("config", "WASD_MINIMAL_MOVE");
			Tracef("WorldEditorConfig: WASD_MINIMAL_MOVE %.f\n", globals::dft::WASD_MINIMAL_MOVE);
		}
		if (reader.HasValue("config", "NO_GOTO_AFTER_INSERT"))
		{
			globals::dft::NO_GOTO_AFTER_INSERT = reader.GetBoolean("config", "NO_GOTO_AFTER_INSERT");
			Tracef("WorldEditorConfig: NO_GOTO_AFTER_INSERT %u\n", globals::dft::NO_GOTO_AFTER_INSERT);
		}
		if (reader.HasValue("config", "NOMAI_ATLAS_DUMP"))
		{
			globals::dft::NOMAI_ATLAS_DUMP = reader.GetBoolean("config", "NOMAI_ATLAS_DUMP");
			Tracef("WorldEditorConfig: NOMAI_ATLAS_DUMP %u\n", globals::dft::NOMAI_ATLAS_DUMP);
		}
		if (reader.HasValue("config", "NOMINIMAP_RAWALPHA"))
		{
			globals::dft::NOMINIMAP_RAWALPHA = reader.GetBoolean("config", "NOMINIMAP_RAWALPHA");
			Tracef("WorldEditorConfig: NOMINIMAP_RAWALPHA %u\n", globals::dft::NOMINIMAP_RAWALPHA);
		}
		if (reader.HasValue("config", "DETECT_MDATR_HEIGHT"))
		{
			globals::dft::DETECT_MDATR_HEIGHT = reader.GetBoolean("config", "DETECT_MDATR_HEIGHT");
			Tracef("WorldEditorConfig: DETECT_MDATR_HEIGHT %u\n", globals::dft::DETECT_MDATR_HEIGHT);
		}
		if (reader.HasValue("config", "NOFOG_ONMAPLOAD"))
		{
			globals::dft::NOFOG_ONMAPLOAD = reader.GetBoolean("config", "NOFOG_ONMAPLOAD");
			Tracef("WorldEditorConfig: NOFOG_ONMAPLOAD %u\n", globals::dft::NOFOG_ONMAPLOAD);
		}
		if (reader.HasValue("config", "REFRESHALL_ONUPDATEUI"))
		{
			globals::dft::REFRESHALL_ONUPDATEUI = reader.GetBoolean("config", "REFRESHALL_ONUPDATEUI");
			Tracef("WorldEditorConfig: REFRESHALL_ONUPDATEUI %u\n", globals::dft::REFRESHALL_ONUPDATEUI);
		}

		if (reader.HasValue("config", "NEWMAP_TEXTURESETSAVEASMAPNAME"))
		{
			globals::dft::NEWMAP_TEXTURESETSAVEASMAPNAME = reader.GetBoolean("config", "NEWMAP_TEXTURESETSAVEASMAPNAME");
			Tracef("WorldEditorConfig: NEWMAP_TEXTURESETSAVEASMAPNAME %u\n", globals::dft::NEWMAP_TEXTURESETSAVEASMAPNAME);
		}
		if (reader.HasValue("config", "NEWMAP_TEXTURESETLOADPATH"))
		{
			globals::dft::NEWMAP_TEXTURESETLOADPATH = reader.Get("config", "NEWMAP_TEXTURESETLOADPATH");
			msl::string_replace_in_place(globals::dft::NEWMAP_TEXTURESETLOADPATH, "\"", ""); // remove all the trailing ""
			Tracef("WorldEditorConfig: NEWMAP_TEXTURESETLOADPATH %s\n", globals::dft::NEWMAP_TEXTURESETLOADPATH.c_str());
		}
		if (reader.HasValue("config", "NEW_MAP_MAPNAME_PREFIX"))
		{
			globals::dft::NEW_MAP_MAPNAME_PREFIX = reader.Get("config", "NEW_MAP_MAPNAME_PREFIX");
			msl::string_replace_in_place(globals::dft::NEW_MAP_MAPNAME_PREFIX, "\"", ""); // remove all the trailing ""
			Tracef("WorldEditorConfig: NEW_MAP_MAPNAME_PREFIX %s\n", globals::dft::NEW_MAP_MAPNAME_PREFIX.c_str());
		}

		if (reader.HasValue("config", "SERVERATTR_REMOVE_WEIRD_FLAGS"))
		{
			globals::dft::SERVERATTR_REMOVE_WEIRD_FLAGS = reader.GetBoolean("config", "SERVERATTR_REMOVE_WEIRD_FLAGS");
			Tracef("WorldEditorConfig: SERVERATTR_REMOVE_WEIRD_FLAGS %d\n", globals::dft::SERVERATTR_REMOVE_WEIRD_FLAGS);
		}
		if (reader.HasValue("config", "VIEW_OBJECT_LIGHTING"))
		{
			globals::dft::VIEW_OBJECT_LIGHTING = reader.GetBoolean("config", "VIEW_OBJECT_LIGHTING");
			Tracef("WorldEditorConfig: VIEW_OBJECT_LIGHTING %d\n", globals::dft::VIEW_OBJECT_LIGHTING);
		}
		if (reader.HasValue("config", "MOB_PROTO_PATH"))
		{
			globals::dft::MOB_PROTO_PATH = reader.Get("config", "MOB_PROTO_PATH");
			msl::string_replace_in_place(globals::dft::MOB_PROTO_PATH, "\"", ""); // remove all the trailing ""
			Tracef("WorldEditorConfig: MOB_PROTO_PATH %s\n", globals::dft::MOB_PROTO_PATH.c_str());
		}
		if (reader.HasValue("config", "VIEW_MONSTER_AREA_INFO"))
		{
			globals::dft::VIEW_MONSTER_AREA_INFO = reader.GetBoolean("config", "VIEW_MONSTER_AREA_INFO");
			Tracef("WorldEditorConfig: VIEW_MONSTER_AREA_INFO %d\n", globals::dft::VIEW_MONSTER_AREA_INFO);
		}

		if (reader.HasValue("config", "RENDER_CURSOR_COLOR_R"))
		{
			globals::dft::RENDER_CURSOR_COLOR_R = reader.GetFloat("config", "RENDER_CURSOR_COLOR_R");
			Tracef("WorldEditorConfig: RENDER_CURSOR_COLOR_R %.f\n", globals::dft::RENDER_CURSOR_COLOR_R);
		}
		if (reader.HasValue("config", "RENDER_CURSOR_COLOR_G"))
		{
			globals::dft::RENDER_CURSOR_COLOR_G = reader.GetFloat("config", "RENDER_CURSOR_COLOR_G");
			Tracef("WorldEditorConfig: RENDER_CURSOR_COLOR_G %.f\n", globals::dft::RENDER_CURSOR_COLOR_G);
		}
		if (reader.HasValue("config", "RENDER_CURSOR_COLOR_B"))
		{
			globals::dft::RENDER_CURSOR_COLOR_B = reader.GetFloat("config", "RENDER_CURSOR_COLOR_B");
			Tracef("WorldEditorConfig: RENDER_CURSOR_COLOR_B %.f\n", globals::dft::RENDER_CURSOR_COLOR_B);
		}

		if (reader.HasValue("config", "OBJECT_HEIGHT_SLIDER_MAX"))
		{
			globals::dft::OBJECT_HEIGHT_SLIDER_MAX = reader.GetInteger("config", "OBJECT_HEIGHT_SLIDER_MAX");
			Tracef("WorldEditorConfig: OBJECT_HEIGHT_SLIDER_MAX %u\n", globals::dft::OBJECT_HEIGHT_SLIDER_MAX);
		}
		if (reader.HasValue("config", "OBJECT_HEIGHT_MAX"))
		{
			globals::dft::OBJECT_HEIGHT_MAX = reader.GetFloat("config", "OBJECT_HEIGHT_MAX");
			Tracef("WorldEditorConfig: OBJECT_HEIGHT_MAX %.f\n", globals::dft::OBJECT_HEIGHT_MAX);
		}

		if (reader.HasValue("config", "ATTR_SLIDER_REMOVAL"))
		{
			globals::dft::ATTR_SLIDER_REMOVAL = reader.GetBoolean("config", "ATTR_SLIDER_REMOVAL");
			Tracef("WorldEditorConfig: ATTR_SLIDER_REMOVAL %d\n", globals::dft::ATTR_SLIDER_REMOVAL);
		}

		#ifdef CWE_CENTERED_WINDOW
		if (reader.HasValue("config", "CENTERED_WINDOW"))
		{
			globals::dft::CENTERED_WINDOW = reader.GetBoolean("config", "CENTERED_WINDOW");
			Tracef("WorldEditorConfig: CENTERED_WINDOW %d\n", globals::dft::CENTERED_WINDOW);
		}
		#endif

		if (reader.HasValue("config", "AUTO_SAVE_TIME"))
		{
			globals::dft::AUTO_SAVE_TIME = std::max<int32_t>(reader.GetInteger("config", "AUTO_SAVE_TIME"), 5);
			Tracef("WorldEditorConfig: AUTO_SAVE_TIME %d\n", globals::dft::AUTO_SAVE_TIME);
		}
		#ifdef CWE_MULTI_LANGUAGE
		if (reader.HasValue("config", "LOCALE"))
		{
			globals::dft::LOCALE = reader.Get("config", "LOCALE");
			// remove all the trailing ""
			msl::string_replace_in_place(globals::dft::LOCALE, "\"", "");
			// convert locale name to uppercase
			std::for_each(globals::dft::LOCALE.begin(), globals::dft::LOCALE.end(), [](char& c) { c = ::toupper(c); });
			Tracef("WorldEditorConfig: LOCALE %s\n", globals::dft::LOCALE.c_str());
		}
		#endif
	}
#endif
	return true;
}

}

