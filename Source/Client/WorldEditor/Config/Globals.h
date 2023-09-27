#ifndef __INC_GLOBALS_H__
#define __INC_GLOBALS_H__

#define USE_WE_CONFIG
#define CWE_CENTERED_WINDOW
#define CWE_MULTI_LANGUAGE

#ifdef USE_WE_CONFIG
#include <inih/inih.h>
#include <msl/msl.h>
#include "Lzo/lzo_manager.h"
#endif

namespace globals
{
namespace dft
{
extern bool VIEW_CHAR_OUTPUT_BY_DEFAULT;
extern bool VIEW_SHADOW_OUTPUT_BY_DEFAULT;
extern bool VIEW_WATER_OUTPUT_BY_DEFAULT;

extern int32_t WINDOW_HEIGHT_SIZE;
extern int32_t WINDOW_WIDTH_SIZE;
extern float WINDOW_FOV_SIZE;

extern float WASD_MINIMAL_MOVE;

extern bool NO_GOTO_AFTER_INSERT;
extern bool NOMAI_ATLAS_DUMP;
extern bool NOMINIMAP_RAWALPHA;
extern bool DETECT_MDATR_HEIGHT;
extern bool NOFOG_ONMAPLOAD;
extern bool REFRESHALL_ONUPDATEUI;

extern bool NEWMAP_TEXTURESETSAVEASMAPNAME;
extern std::string NEWMAP_TEXTURESETLOADPATH;
extern std::string NEW_MAP_MAPNAME_PREFIX;

extern bool SERVERATTR_REMOVE_WEIRD_FLAGS;
extern bool VIEW_OBJECT_LIGHTING;
extern std::string MOB_PROTO_PATH;
extern bool VIEW_MONSTER_AREA_INFO;

extern float RENDER_CURSOR_COLOR_R;
extern float RENDER_CURSOR_COLOR_G;
extern float RENDER_CURSOR_COLOR_B;

extern int32_t OBJECT_HEIGHT_SLIDER_MAX;
extern float OBJECT_HEIGHT_MAX;

extern bool ATTR_SLIDER_REMOVAL;

#ifdef CWE_CENTERED_WINDOW
extern bool CENTERED_WINDOW;
#endif

extern int32_t AUTO_SAVE_TIME;
#ifdef CWE_MULTI_LANGUAGE
extern std::string LOCALE;
#endif
}

bool readGlobalConfigs(const char* szConfigName);

}
#endif

