/******************************************************************************* 
 * emu-ps3.cpp - FCEUNext PS3
 *
 *  Created on: Aug 18, 2011
********************************************************************************/

#ifndef EMUPS3_H_
#define EMUPS3_H_

/* System includes */

#include <sysutil/sysutil_gamecontent.h>

#ifdef CELL_DEBUG_CONSOLE
#include <cell/control_console.h>
#endif

#include <string>

/* Emulator-specific includes */

#include "fceu/driver.h"
#include "fceu/fceu.h"
#include "fceu/input.h"
#include "fceu/state.h"
#include "fceu/ppu.h"
#include "fceu/cart.h"
#include "fceu/x6502.h"
#include "fceu/git.h"
#include "fceu/palette.h"
#include "fceu/sound.h"
#include "fceu/file.h"
#include "fceu/cheat.h"
#include "fceu/ines.h"
#include "fceu/unif.h"
#include "fceu/fds.h"

/* PS3 frontend includes */

#include "cellframework2/audio/stream.h"
#include "cellframework2/input/pad_input.h"
#include "cellframework2/utility/oskutil.h"

#include "ps3video.hpp"
#include "ps3input.h"
#include "emu-ps3-constants.h"

#include "conf/config_file.h"

/* PS3 frontend constants */

#define SOUND_MODE_NORMAL	1
#define SOUND_MODE_HEADSET	2
#define SOUND_MODE_RSOUND	3

enum {
   MENU_ITEM_LOAD_STATE = 0,
   MENU_ITEM_SAVE_STATE,
   MENU_ITEM_KEEP_ASPECT_RATIO,
   MENU_ITEM_OVERSCAN_AMOUNT,
   MENU_ITEM_RESIZE_MODE,
   MENU_ITEM_FRAME_ADVANCE,
   MENU_ITEM_SCREENSHOT_MODE,
   MENU_ITEM_RESET,
   MENU_ITEM_RESET_FORCE_NTSC_PAL,
   MENU_ITEM_RESET_FORCE_PAL_NTSC,
   MENU_ITEM_RETURN_TO_GAME,
   MENU_ITEM_RETURN_TO_MENU,
#ifdef MULTIMAN_SUPPORT
   MENU_ITEM_RETURN_TO_MULTIMAN,
#endif
   MENU_ITEM_RETURN_TO_XMB
};

#define MENU_ITEM_LAST			MENU_ITEM_RETURN_TO_XMB+1

#define AUDIO_INPUT_RATE		(48000)
#define AUDIO_BUFFER_SAMPLES		(4096)

#define MAP_BUTTONS_OPTION_SETTER	0
#define MAP_BUTTONS_OPTION_GETTER	1
#define MAP_BUTTONS_OPTION_DEFAULT	2
#define MAP_BUTTONS_OPTION_NEW		3

#define CONTROL_STYLE_ORIGINAL		0
#define CONTROL_STYLE_BETTER		1

#define MIN_SAVE_STATE_SLOT		0

/* Emulator-specific constants */

// max fs path
#define MAX_PATH 1024

// color palettes
#define MAXPAL 13

float Emulator_GetFontSize();
void emulator_save_settings(uint64_t filetosave);
void emulator_implementation_save_custom_controls(bool showdialog);
void emulator_implementation_button_mapping_settings(int map_button_option_enum);
void emulator_implementation_switch_control_scheme();
void emulator_implementation_set_shader_preset(const char * fname);
void emulator_implementation_set_texture(const char * fname);
void emulator_toggle_sound(uint64_t soundmode);
void Emulator_RequestLoadROM(const char * rom, uint32_t forceReload);
void Emulator_StartROMRunning(uint32_t set_is_running = 1);

/* Emulator-specific extern function prototypes */

extern int AddCheatEntry(char *name, uint32 addr, uint8 val, int compare, int status, int type);
extern int FCEUX_PPU_Loop(int skip);

/* PS3-frontend extern variables */

extern cell_audio_handle_t audio_handle;			// cell audio handle
extern const struct cell_audio_driver *audio_driver;		// cell audio driver opaque pointer
extern PS3Graphics* Graphics;					// object graphics class
extern uint32_t rom_loaded;

extern char contentInfoPath[MAX_PATH_LENGTH];
extern char usrDirPath[MAX_PATH_LENGTH];
extern char DEFAULT_PRESET_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_BORDER_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_MENU_BORDER_FILE[MAX_PATH_LENGTH];
extern char GAME_AWARE_SHADER_DIR_PATH[MAX_PATH_LENGTH];
extern char PRESETS_DIR_PATH[MAX_PATH_LENGTH];
extern char BORDERS_DIR_PATH[MAX_PATH_LENGTH];
extern char SHADERS_DIR_PATH[MAX_PATH_LENGTH];
extern char DEFAULT_SHADER_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_MENU_SHADER_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_GAME_HACK[MAX_PATH_LENGTH];
extern oskutil_params oskutil_handle;
extern int mode_switch;
extern int control_style;

/* Emulator-specific extern variables */

extern FCEUGI *GameInfo;
extern CartInfo iNESCart;
extern CartInfo UNIFCart;

struct st_palettes {
    char name[32], desc[32];
    unsigned int data[64];
};

extern struct st_palettes palettes[];

#endif
