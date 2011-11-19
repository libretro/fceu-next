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

#include "fceumm/driver.h"
#include "fceumm/fceu.h"
#include "fceumm/input.h"
#include "fceumm/state.h"
#include "fceumm/ppu.h"
#include "fceumm/cart.h"
#include "fceumm/x6502.h"
#include "fceumm/git.h"
#include "fceumm/palette.h"
#include "fceumm/sound.h"
#include "fceumm/file.h"
#include "fceumm/cheat.h"
#include "fceumm/ines.h"
#include "fceumm/unif.h"
#include "fceumm/fds.h"

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

#define WRITE_CONTROLS			0
#define READ_CONTROLS			1
#define SET_ALL_CONTROLS_TO_DEFAULT	2

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
void emulator_set_controls(const char * config_file, int mapping_enum, const char * title);
void emulator_implementation_switch_control_scheme();
void emulator_implementation_set_shader_preset(const char * fname);
void emulator_implementation_set_texture(const char * fname);
void emulator_toggle_sound(uint64_t soundmode);
void Emulator_RequestLoadROM(const char * rom, uint32_t forceReload);
void Emulator_StartROMRunning(uint32_t set_is_running = 1);

/* Emulator-specific extern function prototypes */

extern int AddCheatEntry(char *name, uint32 addr, uint8 val, int compare, int status, int type);

/* PS3-frontend extern variables */

extern cell_audio_handle_t audio_handle;			// cell audio handle
extern const struct cell_audio_driver *audio_driver;		// cell audio driver opaque pointer
extern uint32_t rom_loaded;

extern char contentInfoPath[MAX_PATH_LENGTH];
extern char usrDirPath[MAX_PATH_LENGTH];
extern char DEFAULT_PRESET_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_BORDER_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_MENU_BORDER_FILE[MAX_PATH_LENGTH];
extern char GAME_AWARE_SHADER_DIR_PATH[MAX_PATH_LENGTH];
extern char PRESETS_DIR_PATH[MAX_PATH_LENGTH];
extern char INPUT_PRESETS_DIR_PATH[MAX_PATH_LENGTH];
extern char BORDERS_DIR_PATH[MAX_PATH_LENGTH];
extern char SHADERS_DIR_PATH[MAX_PATH_LENGTH];
extern char DEFAULT_SHADER_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_MENU_SHADER_FILE[MAX_PATH_LENGTH];
extern char DEFAULT_GAME_HACK[MAX_PATH_LENGTH];
extern oskutil_params oskutil_handle;
extern int mode_switch;
extern int control_style;

/* Emulator-specific extern variables */

extern FCEUGI *FCEUGameInfo;
extern CartInfo iNESCart;
extern CartInfo UNIFCart;

struct st_palettes {
    char name[32], desc[32];
    unsigned int data[64];
};

extern struct st_palettes palettes[];

#endif
