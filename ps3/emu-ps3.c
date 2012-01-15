/******************************************************************************* 
 * emu-ps3.cpp - FCEUNext PS3
 *
 *  Created on: Aug 18, 2011
********************************************************************************/

/* system includes */
#include <sys/timer.h>
#include <sys/return_code.h>
#include <sys/process.h>
#include <cell/sysmodule.h>
#include <math.h>
#include <sysutil/sysutil_sysparam.h>
#if(CELL_SDK_VERSION > 0x340000)
#include <sysutil/sysutil_screenshot.h>
#endif
#include <sysutil/sysutil_msgdialog.h>
#include <cell/cell_fs.h>
#include <sys/spu_initialize.h>
#if(CELL_SDK_VERSION > 0x340000)
#include <sysutil/sysutil_bgmplayback.h>
#endif

/* emulator-specific includes */

#include "../src-fceumm/types.h"
#include "../src-fceumm/file.h"
#include "../src-fceumm/general.h"

/* PS3 frontend includes */

#include "emu-ps3.h"
#include "conf/settings.h"
#include "menu.h"

SYS_PROCESS_PARAM(1001, 0x10000);

/* PS3 frontend variables */

char contentInfoPath[MAX_PATH_LENGTH];
char usrDirPath[MAX_PATH_LENGTH];
char DEFAULT_PRESET_FILE[MAX_PATH_LENGTH];
char DEFAULT_BORDER_FILE[MAX_PATH_LENGTH];
char DEFAULT_MENU_BORDER_FILE[MAX_PATH_LENGTH];
char GAME_AWARE_SHADER_DIR_PATH[MAX_PATH_LENGTH];
char PRESETS_DIR_PATH[MAX_PATH_LENGTH];
char INPUT_PRESETS_DIR_PATH[MAX_PATH_LENGTH];
char BORDERS_DIR_PATH[MAX_PATH_LENGTH];
char SHADERS_DIR_PATH[MAX_PATH_LENGTH];
char DEFAULT_SHADER_FILE[MAX_PATH_LENGTH];
char DEFAULT_MENU_SHADER_FILE[MAX_PATH_LENGTH];
char SYS_CONFIG_FILE[MAX_PATH_LENGTH];
char DEFAULT_GAME_HACK[MAX_PATH_LENGTH];
char MULTIMAN_GAME_TO_BOOT[MAX_PATH_LENGTH];

cell_audio_handle_t audio_handle;
const struct cell_audio_driver *audio_driver = &cell_audio_audioport;
oskutil_params oskutil_handle;
uint32_t control_binds[MAX_PADS][BTN_DEF_MAX];

struct SSettings Settings;

int mode_switch = MODE_MENU;				/* mode the main loop is in*/
int control_style;
uint32_t is_running;					/* is the ROM currently running in the emulator?*/
static bool is_ingame_menu_running;			/* is the ingame menu currently running?*/
static bool return_to_MM = false;			/* launch multiMAN on exit if ROM is passed*/
uint32_t emulator_initialized = 0;			/* is the emulator loaded?*/
char current_rom[MAX_PATH_LENGTH];			/* filename of the current rom being emulated*/
static bool dialog_is_running;				/* is a dialog screen currently running?*/
static char special_action_msg[256];			/* message which should be overlaid on top of the screen*/
uint32_t special_action_msg_expired;			/* time at which the message no longer needs to be overlaid onscreen*/
uint64_t ingame_menu_item = 0;				/* the current ingame menu item that is selected*/

/* Emulator-specific global variables */

static uint32 JSReturn = 0;
void *InputDPR;
static uint8 *gfx=0;					/* had to be made a global because ingame_menu needs access to it too*/
static uint32_t hack_prevent_game_sram_from_being_erased = 1; /*ugly hack - is set to 0 after the hack has been applied*/

#if 0
static INPUTC *zapperdata[2];
static unsigned int myzappers[2][3];
#endif

/* PS3 frontend - save state/emulator SRAM related functions */

void set_text_message(const char * message, uint32_t speed)
{
	snprintf(special_action_msg, sizeof(special_action_msg), message);
	special_action_msg_expired = ps3graphics_set_text_message_speed(speed);
}

static void emulator_decrement_current_save_state_slot(void)
{
	char msg[512];

	if (Settings.CurrentSaveStateSlot != MIN_SAVE_STATE_SLOT)
		Settings.CurrentSaveStateSlot--;
	snprintf(msg, sizeof(msg), "Save state slot changed to: #%d", Settings.CurrentSaveStateSlot);

	set_text_message(msg, 60);
}

static void emulator_increment_current_save_state_slot(void)
{
	char msg[512];

	Settings.CurrentSaveStateSlot++;
	snprintf(msg, sizeof(msg), "Save state slot changed to: #%d", Settings.CurrentSaveStateSlot);
	
	set_text_message(msg, 60);
}

static void emulator_load_current_save_state_slot(void)
{
	/* emulator-specific */
	int slot = Settings.CurrentSaveStateSlot;
	const char * fname = FCEU_MakeFName(FCEUMKF_STATE, slot, 0);
	int ret = FCEUSS_Load(fname);
	if(ret)
		snprintf(special_action_msg, sizeof(special_action_msg), "Loaded save state slot #%d", Settings.CurrentSaveStateSlot);
	else
		snprintf(special_action_msg, sizeof(special_action_msg), "Can't load from save state slot #%d", Settings.CurrentSaveStateSlot);
	special_action_msg_expired = ps3graphics_set_text_message_speed(60);
}

static void emulator_save_current_save_state_slot(void)
{
	/* emulator-specific */
	int slot = Settings.CurrentSaveStateSlot;
	const char * fname = FCEU_MakeFName(FCEUMKF_STATE, slot, 0);
	FCEUSS_Save(fname);
	snprintf(special_action_msg, sizeof(special_action_msg), "Saved to save state slot #%d", Settings.CurrentSaveStateSlot);
	special_action_msg_expired = ps3graphics_set_text_message_speed(60);
}

/* PS3 frontend - cheat position related functions */

static void emulator_decrement_current_cheat_position(void)
{
	if(Settings.CurrentCheatPosition > 0)
		Settings.CurrentCheatPosition--;
}

static void emulator_increment_current_cheat_position(void)\
{
	Settings.CurrentCheatPosition++;
}

/* PS3 frontend - ROM-related functions */

float Emulator_GetFontSize()
{
	return Settings.PS3FontSize/100.0;
}

/* PS3 frontend callbacks */

static void cb_dialog_ok(int button_type, void *userdata)
{
	switch(button_type)
	{
		case CELL_MSGDIALOG_BUTTON_ESCAPE:
			dialog_is_running = false;
			break;
	}
}

static void sysutil_exit_callback (uint64_t status, uint64_t param, void *userdata)
{
	(void) param;
	(void) userdata;

	switch (status)
	{
		case CELL_SYSUTIL_REQUEST_EXITGAME:
			menu_is_running = 0;
			is_ingame_menu_running = 0;
			is_running = 0;
			#ifdef MULTIMAN_SUPPORT
			return_to_MM = false;
			#endif
			mode_switch = MODE_EXIT;
			break;
		case CELL_SYSUTIL_DRAWING_BEGIN:
		case CELL_SYSUTIL_DRAWING_END:
		case CELL_SYSUTIL_OSKDIALOG_LOADED:
			break;
		case CELL_SYSUTIL_OSKDIALOG_FINISHED:
			oskutil_close(&oskutil_handle);
			oskutil_finished(&oskutil_handle);
			break;
		case CELL_SYSUTIL_OSKDIALOG_UNLOADED:
			oskutil_unload(&oskutil_handle);
			break;
		default:
			break;
	}
}

static void emulator_toggle_throttle(bool enable)
{
	char msg[512];

	ps3graphics_set_vsync(enable);
	if(enable)
		snprintf(msg, sizeof(msg), "Throttle mode: ON");
	else
		snprintf(msg, sizeof(msg), "Throttle mode: OFF");

	set_text_message(msg, 60);
}

/* PS3 frontend - controls related macros */

#define init_setting_uint(charstring, setting, defaultvalue) \
	if(!(config_get_uint(currentconfig, charstring, &setting))) \
		setting = defaultvalue; 

#define init_setting_int(charstring, setting, defaultvalue) \
	if(!(config_get_int(currentconfig, charstring, &setting))) \
		setting = defaultvalue; 

#define init_setting_bool(charstring, setting, defaultvalue) \
	if(!(config_get_bool(currentconfig, charstring, &setting))) \
		setting = defaultvalue; 

#define init_setting_bool(charstring, setting, defaultvalue) \
	if(!(config_get_bool(currentconfig, charstring, &setting))) \
		setting =	defaultvalue;

#define init_setting_char(charstring, setting, defaultvalue) \
	if(!(config_get_char_array(currentconfig, charstring, setting, sizeof(setting)))) \
		strncpy(setting,defaultvalue, sizeof(setting));

void emulator_implementation_set_gameaware(const char * fname)
{
	ps3graphics_init_state_uniforms(fname);
	strcpy(Settings.PS3CurrentShader, ps3graphics_get_fragment_shader_path(0));
	strcpy(Settings.PS3CurrentShader2, ps3graphics_get_fragment_shader_path(1));
}

static void map_ps3_standard_controls(const char * config_file)
{
	char filetitle_tmp[512];
	char string_tmp[256];
	config_file_t * currentconfig = config_file_new(config_file);
	for(uint32_t i = 0; i < MAX_PADS; i++)
	{
		for(uint32_t j = 0; j < BTN_DEF_MAX; j++)
		{
			snprintf(string_tmp, sizeof(string_tmp), "PS3Player%d::%d", i, j);
			config_set_uint(currentconfig, string_tmp,control_binds[i][j]);
		}
	}
	config_set_string(currentconfig, "InputPresetTitle", filetitle_tmp);
	config_file_write(currentconfig, config_file);
}

static void get_ps3_standard_controls(const char * config_file)
{
	config_file_t * currentconfig = config_file_new(config_file);
	char string_tmp[256];

	for(uint32_t i = 0; i < MAX_PADS; i++)
	{
		for(uint32_t j = 0; j < BTN_DEF_MAX; j++)
		{
			snprintf(string_tmp, sizeof(string_tmp), "PS3Player%d::%d", i, j);
			init_setting_uint(string_tmp, control_binds[i][j], default_control_binds[j]);
		}
	}

	init_setting_char("InputPresetTitle", Settings.PS3CurrentInputPresetTitle, "Default");
}

uint32_t default_control_binds[] = {
	BTN_UP,				// CTRL_UP_DEF
	BTN_DOWN,			// CTRL_DOWN_DEF
	BTN_LEFT,			// CTRL_LEFT_DEF
	BTN_RIGHT,			// CTRL_RIGHT_DEF
	BTN_A,				// CTRL_CIRCLE_DEF
	BTN_B,				// CTRL_CROSS_DEF
	BTN_NONE,			// CTRL_TRIANGLE_DEF
	BTN_NONE,			// CTRL_SQUARE_DEF
	BTN_SELECT,			// CTRL_SELECT_DEF
	BTN_START,			// CTRL_START_DEF
	BTN_NONE,			// CTRL_L1_DEF
	BTN_NONE,			// CTRL_R1_DEF
	BTN_NONE,			// CTRL_L2_DEF
	BTN_NONE,			// CTRL_R2_DEF
	BTN_NONE,			// CTRL_L3_DEF
	BTN_INGAME_MENU,		// CTRL_R3_DEF
	BTN_NONE,			// CTRL_L2_L3_DEF
	BTN_NONE,			// CTRL_L2_R3_DEF
	BTN_INCREMENTCHEAT,		// CTRL_L2_RSTICK_RIGHT_DEF
	BTN_DECREMENTCHEAT,		// CTRL_L2_RSTICK_LEFT_DEF
	BTN_NONE,			// CTRL_L2_RSTICK_UP_DEF
	BTN_NONE,			// CTRL_L2_RSTICK_DOWN_DEF
	BTN_INCREMENTSAVE,		// CTRL_R2_RSTICK_RIGHT_DEF
	BTN_DECREMENTSAVE,		// CTRL_R2_RSTICK_LEFT_DEF
	BTN_QUICKLOAD,			// CTRL_R2_RSTICK_UP_DEF
	BTN_QUICKSAVE,			// CTRL_R2_RSTICK_DOWN_DEF
	BTN_NONE,			// CTRL_R2_R3_DEF
	BTN_EXITTOMENU,			// CTRL_R3_L3_DEF
	BTN_NONE,			// CTRL_RSTICK_UP_DEF
	BTN_FASTFORWARD,		// CTRL_RSTICK_DOWN_DEF
	BTN_DECREMENT_PALETTE,		// CTRL_RSTICK_LEFT_DEF
	BTN_INCREMENT_PALETTE		// CTRL_RSTICK_RIGHT_DEF
};

static void map_ps3_button_array(void)
{
	for(int i = 0; i < MAX_PADS; i++)
		for(uint32_t j = 0; j < BTN_DEF_MAX; j++)
			Input_MapButton(control_binds[i][j],false,default_control_binds[j]);
}

void emulator_set_controls(const char * config_file, int mapping_enum, const char * title)
{
	switch(mapping_enum)
	{
		case WRITE_CONTROLS:
		{
			map_ps3_standard_controls(config_file);
			break;
		}
		case READ_CONTROLS:
		{
			get_ps3_standard_controls(config_file);
			break;
		}
		case SET_ALL_CONTROLS_TO_DEFAULT:
		{
			map_ps3_button_array();
			break;
		}
	}
}

/* PS3 frontend - settings-related functions */

static bool file_exists(const char * filename)
{
	CellFsStat sb;
	if(cellFsStat(filename,&sb) == CELL_FS_SUCCEEDED)
		return true;
	else
		return false;
}

static void emulator_init_settings(void)
{
	bool config_file_newly_created = false;
	memset((&Settings), 0, (sizeof(Settings)));

	if(!file_exists(SYS_CONFIG_FILE))
	{
		FILE * f;
		f = fopen(SYS_CONFIG_FILE, "w");
		fclose(f);
		config_file_newly_created = true;
	}

	config_file_t * currentconfig = config_file_new(SYS_CONFIG_FILE);

	init_setting_uint("PS3General::ApplyShaderPresetOnStartup", Settings.ApplyShaderPresetOnStartup, 0);
	init_setting_uint("PS3General::KeepAspect", Settings.PS3KeepAspect, ASPECT_RATIO_4_3);
	init_setting_uint("PS3General::Smooth", Settings.PS3Smooth, 1);
	init_setting_uint("PS3General::Smooth2", Settings.PS3Smooth2, 1);
	init_setting_char("PS3General::PS3CurrentShader", Settings.PS3CurrentShader, DEFAULT_SHADER_FILE);
	init_setting_char("PS3General::PS3CurrentShader2", Settings.PS3CurrentShader2, DEFAULT_SHADER_FILE);
	init_setting_char("PS3General::Border", Settings.PS3CurrentBorder, DEFAULT_BORDER_FILE);
	init_setting_uint("PS3General::PS3TripleBuffering",Settings.TripleBuffering, 1);
	init_setting_char("PS3General::ShaderPresetPath", Settings.ShaderPresetPath, "");
	init_setting_char("PS3General::ShaderPresetTitle", Settings.ShaderPresetTitle, "None");
	init_setting_uint("PS3General::ScaleFactor", Settings.ScaleFactor, 2);
	init_setting_uint("PS3General::ViewportX", Settings.ViewportX, 0);
	init_setting_uint("PS3General::ViewportY", Settings.ViewportY, 0);
	init_setting_uint("PS3General::ViewportWidth", Settings.ViewportWidth, 0);
	init_setting_uint("PS3General::ViewportHeight", Settings.ViewportHeight, 0);
	init_setting_uint("PS3General::ScaleEnabled", Settings.ScaleEnabled, 1);
	init_setting_uint("PS3General::Orientation", Settings.Orientation, 0);
	init_setting_uint("PS3General::PS3CurrentResolution", Settings.PS3CurrentResolution, NULL);
	init_setting_uint("PS3General::OverscanEnabled", Settings.PS3OverscanEnabled, 0);
	init_setting_int("PS3General::OverscanAmount", Settings.PS3OverscanAmount, 0);
	init_setting_uint("PS3General::PS3PALTemporalMode60Hz", Settings.PS3PALTemporalMode60Hz, 0);
	init_setting_uint("Sound::SoundMode", Settings.SoundMode, SOUND_MODE_NORMAL);
	init_setting_char("RSound::RSoundServerIPAddress",  Settings.RSoundServerIPAddress, "0.0.0.0");
	init_setting_uint("PS3General::Throttled", Settings.Throttled, 1);
	init_setting_uint("PS3General::PS3FontSize", Settings.PS3FontSize, 100);
	init_setting_char("PS3Paths::PathSaveStates", Settings.PS3PathSaveStates, usrDirPath);
	init_setting_char("PS3Paths::PathSRAM", Settings.PS3PathSRAM, usrDirPath);
	init_setting_char("PS3Paths::PathROMDirectory", Settings.PS3PathROMDirectory, "/");
	init_setting_uint("PS3General::ControlScheme", Settings.ControlScheme, CONTROL_SCHEME_DEFAULT);
	init_setting_uint("PS3General::CurrentSaveStateSlot",  Settings.CurrentSaveStateSlot, 0);
	init_setting_uint("PS3General::ScreenshotsEnabled", Settings.ScreenshotsEnabled, 0);
	char tmp_str[256];
	if (config_get_char_array(currentconfig,"PS3General::GameAwareShaderPath", tmp_str, sizeof(tmp_str)))
		config_get_char_array(currentconfig, "PS3General::GameAwareShaderPath", Settings.GameAwareShaderPath, sizeof(Settings.GameAwareShaderPath));

	/* emulator-specific settings */

	init_setting_uint("FCEU::ControlStyle",Settings.FCEUControlstyle, CONTROL_STYLE_ORIGINAL);
	init_setting_uint("FCEU::FCEUPPUMode", Settings.FCEUPPUMode, 1);
	init_setting_uint("FCEU::DisableSpriteLimitation", Settings.FCEUDisableSpriteLimitation, 0);
	init_setting_uint("FCEU::GameGenie", Settings.FCEUGameGenie, 0);
	init_setting_uint("FCEU::ScanlineNTSCStart", Settings.FCEUScanlineNTSCStart, 8);
	init_setting_uint("FCEU::ScanlineNTSCEnd", Settings.FCEUScanlineNTSCEnd, 231);
	init_setting_uint("FCEU::ScanlinePALStart", Settings.FCEUScanlinePALStart, 0);
	init_setting_uint("FCEU::ScanlinePALEnd", Settings.FCEUScanlinePALEnd, 239);
	init_setting_char("PS3Paths::BaseDirectory", Settings.PS3PathBaseDirectory, usrDirPath);
	init_setting_char("PS3Paths::PathCheats", Settings.PS3PathCheats, usrDirPath);

	FCEUI_SetGameGenie(Settings.FCEUGameGenie);
	control_style = Settings.FCEUControlstyle;
	FCEUI_DisableSpriteLimitation(Settings.FCEUDisableSpriteLimitation);

	if(config_file_newly_created)
		emulator_set_controls(SYS_CONFIG_FILE, SET_ALL_CONTROLS_TO_DEFAULT, "Default");
	else
		emulator_set_controls(SYS_CONFIG_FILE, READ_CONTROLS, "Default");
}

void emulator_implementation_set_shader_preset(const char * fname)
{
	config_file_t * currentconfig = config_file_new(fname);

	init_setting_uint("ScaleFactor", Settings.ScaleFactor, Settings.ScaleFactor);
	init_setting_uint("Smooth", Settings.PS3Smooth, Settings.PS3Smooth);
	init_setting_uint("Smooth2", Settings.PS3Smooth2, Settings.PS3Smooth2);
	init_setting_uint("ScaleEnabled", Settings.ScaleEnabled, Settings.ScaleEnabled);
	init_setting_uint("Orientation", Settings.Orientation, Settings.Orientation);
	init_setting_char("PS3CurrentShader", Settings.PS3CurrentShader, DEFAULT_SHADER_FILE);
	init_setting_char("PS3CurrentShader2", Settings.PS3CurrentShader2, DEFAULT_SHADER_FILE);
	init_setting_char("Border", Settings.PS3CurrentBorder, DEFAULT_BORDER_FILE);

	strncpy(Settings.ShaderPresetPath, fname, sizeof(Settings.ShaderPresetPath));
	init_setting_char("ShaderPresetTitle", Settings.ShaderPresetTitle, "None");
	init_setting_uint("KeepAspect", Settings.PS3KeepAspect, Settings.PS3KeepAspect);
	init_setting_uint("OverscanEnabled", Settings.PS3OverscanEnabled, Settings.PS3OverscanEnabled);
	init_setting_int("OverscanAmount", Settings.PS3OverscanAmount, Settings.PS3OverscanAmount);
	init_setting_uint("ViewportX", Settings.ViewportX, Settings.ViewportX);
	init_setting_uint("ViewportY", Settings.ViewportY, Settings.ViewportY);
	init_setting_uint("ViewportWidth", Settings.ViewportWidth, Settings.ViewportWidth);
	init_setting_uint("ViewportHeight", Settings.ViewportHeight, Settings.ViewportHeight);
	ps3graphics_load_fragment_shader(Settings.PS3CurrentShader, 0);
	ps3graphics_load_fragment_shader(Settings.PS3CurrentShader2, 1);
	ps3graphics_set_fbo_scale(Settings.ScaleEnabled,Settings.ScaleFactor);
	ps3graphics_set_aspect_ratio(Settings.PS3KeepAspect, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT, 1);
	ps3graphics_set_overscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100, 1);
	ps3graphics_set_smooth(Settings.PS3Smooth, 0);
	ps3graphics_set_smooth(Settings.PS3Smooth2, 1);
}

void emulator_save_settings(uint64_t filetosave)
{
	config_file_t * currentconfig = config_file_new(SYS_CONFIG_FILE);
	char filepath[MAX_PATH_LENGTH];

	switch(filetosave)
	{
		case CONFIG_FILE:
			strncpy(filepath, SYS_CONFIG_FILE, sizeof(filepath));

			config_set_uint(currentconfig, "PS3General::ControlScheme",Settings.ControlScheme);
			config_set_uint(currentconfig, "PS3General::CurrentSaveStateSlot",Settings.CurrentSaveStateSlot);
			config_set_uint(currentconfig, "PS3General::KeepAspect",Settings.PS3KeepAspect);
			config_set_uint(currentconfig, "PS3General::ApplyShaderPresetOnStartup", Settings.ApplyShaderPresetOnStartup);
			config_set_uint(currentconfig, "PS3General::ViewportX", ps3graphics_get_viewport_x());
			config_set_uint(currentconfig, "PS3General::ViewportY", ps3graphics_get_viewport_y());
			config_set_uint(currentconfig, "PS3General::ViewportWidth", ps3graphics_get_viewport_width());
			config_set_uint(currentconfig, "PS3General::ViewportHeight", ps3graphics_get_viewport_height());
			config_set_uint(currentconfig, "PS3General::Smooth", Settings.PS3Smooth);
			config_set_uint(currentconfig, "PS3General::Smooth2", Settings.PS3Smooth2);
			config_set_uint(currentconfig, "PS3General::ScaleFactor", Settings.ScaleFactor);
			config_set_uint(currentconfig, "PS3General::OverscanEnabled", Settings.PS3OverscanEnabled);
			config_set_uint(currentconfig, "PS3General::OverscanAmount",Settings.PS3OverscanAmount);
			config_set_uint(currentconfig, "PS3General::PS3FontSize",Settings.PS3FontSize);
			config_set_uint(currentconfig, "PS3General::Throttled",Settings.Throttled);
			config_set_uint(currentconfig, "PS3General::PS3PALTemporalMode60Hz",Settings.PS3PALTemporalMode60Hz);
			config_set_uint(currentconfig, "PS3General::PS3TripleBuffering",Settings.TripleBuffering);
			config_set_uint(currentconfig, "PS3General::ScreenshotsEnabled",Settings.ScreenshotsEnabled);
			config_set_uint(currentconfig, "Sound::SoundMode",Settings.SoundMode);
			config_set_uint(currentconfig, "PS3General::PS3CurrentResolution",ps3graphics_get_current_resolution());
			config_set_string(currentconfig, "PS3General::ShaderPresetPath", Settings.ShaderPresetPath);
			config_set_string(currentconfig, "PS3General::ShaderPresetTitle", Settings.ShaderPresetTitle);
			config_set_string(currentconfig, "PS3General::PS3CurrentShader",ps3graphics_get_fragment_shader_path(0));
			config_set_string(currentconfig, "PS3General::PS3CurrentShader2", ps3graphics_get_fragment_shader_path(1));
			config_set_string(currentconfig, "PS3General::Border", Settings.PS3CurrentBorder);
			config_set_string(currentconfig, "PS3General::GameAwareShaderPath", Settings.GameAwareShaderPath);
			config_set_string(currentconfig, "PS3Paths::PathSaveStates", Settings.PS3PathSaveStates);
			config_set_string(currentconfig, "PS3Paths::PathSRAM", Settings.PS3PathSRAM);
			config_set_string(currentconfig, "PS3Paths::PathROMDirectory", Settings.PS3PathROMDirectory);
			config_set_string(currentconfig, "RSound::RSoundServerIPAddress", Settings.RSoundServerIPAddress);
			config_set_uint(currentconfig, "PS3General::ScaleEnabled", Settings.ScaleEnabled);
			config_set_uint(currentconfig, "PS3General::Orientation", Settings.Orientation);

			//Emulator-specific settings
			config_set_string(currentconfig, "PS3Paths::PathCheats",Settings.PS3PathCheats);
			config_set_string(currentconfig, "PS3Paths::PathBaseDirectory",Settings.PS3PathBaseDirectory);
			config_set_uint(currentconfig, "FCEU::Controlstyle",Settings.FCEUControlstyle);
			config_set_uint(currentconfig, "FCEU::DisableSpriteLimitation",Settings.FCEUDisableSpriteLimitation);
			config_set_uint(currentconfig, "FCEU::FCEUPPUMode",Settings.FCEUPPUMode);
			config_set_uint(currentconfig, "FCEU::ScanlineNTSCStart",Settings.FCEUScanlineNTSCStart);
			config_set_int(currentconfig, "FCEU::ScanlineNTSCEnd",Settings.FCEUScanlineNTSCEnd);
			config_set_int(currentconfig, "FCEU::ScanlinePALStart",Settings.FCEUScanlinePALStart);
			config_set_int(currentconfig, "FCEU::ScanlinePALEnd",Settings.FCEUScanlinePALEnd);
			config_set_int(currentconfig, "FCEU::GameGenie",Settings.FCEUGameGenie);

			config_file_write(currentconfig, filepath);
			emulator_set_controls(filepath, WRITE_CONTROLS, "Default");
			break;
		case SHADER_PRESET_FILE:
			{
				bool filename_entered = false;
				char filename_tmp[256], filetitle_tmp[256];
				oskutil_write_initial_message(&oskutil_handle, L"example");
				oskutil_write_message(&oskutil_handle, L"Enter filename for preset (with no file extension)");
				oskutil_start(&oskutil_handle);

				while(OSK_IS_RUNNING(oskutil_handle))
				{
					/* OSK Util gets updated */
					glClear(GL_COLOR_BUFFER_BIT);
					ps3graphics_draw_menu();
					_jsPlatformSwapBuffers(psgl_device);
					cell_console_poll();
					cellSysutilCheckCallback();
				}

				if(oskutil_handle.text_can_be_fetched)
				{
					strncpy(filename_tmp, OUTPUT_TEXT_STRING(oskutil_handle), sizeof(filename_tmp));
					snprintf(filepath, sizeof(filepath), "%s/%s.conf", PRESETS_DIR_PATH, filename_tmp);
					filename_entered = true;
				}

				if(filename_entered)
				{

					oskutil_write_initial_message(&oskutil_handle, L"Example file title");
					oskutil_write_message(&oskutil_handle, L"Enter title for preset");
					oskutil_start(&oskutil_handle);

					while(OSK_IS_RUNNING(oskutil_handle))
					{
						/* OSK Util gets updated */
						glClear(GL_COLOR_BUFFER_BIT);
						ps3graphics_draw_menu();
						_jsPlatformSwapBuffers(psgl_device);
						cell_console_poll();
						cellSysutilCheckCallback();
					}

					if(oskutil_handle.text_can_be_fetched)
						snprintf(filetitle_tmp, sizeof(filetitle_tmp), "%s", OUTPUT_TEXT_STRING(oskutil_handle));
					else
						snprintf(filetitle_tmp, sizeof(filetitle_tmp), "%s", filename_tmp);

					if(!file_exists(filepath))
					{
						FILE * f = fopen(filepath, "w");
						fclose(f);
					}

					currentconfig = config_file_new(filepath);

					config_set_string(currentconfig, "PS3CurrentShader", Settings.PS3CurrentShader);
					config_set_string(currentconfig, "PS3CurrentShader2", Settings.PS3CurrentShader2);
					config_set_string(currentconfig, "Border", Settings.PS3CurrentBorder);
					config_set_uint(currentconfig, "Smooth", Settings.PS3Smooth);
					config_set_uint(currentconfig, "Smooth2", Settings.PS3Smooth2);
					config_set_string(currentconfig, "ShaderPresetTitle", filetitle_tmp);
					config_set_uint(currentconfig, "ViewportX", ps3graphics_get_viewport_x());
					config_set_uint(currentconfig, "ViewportY", ps3graphics_get_viewport_y());
					config_set_uint(currentconfig, "ViewportWidth", ps3graphics_get_viewport_width());
					config_set_uint(currentconfig, "ViewportHeight", ps3graphics_get_viewport_height());
					config_set_uint(currentconfig, "ScaleFactor", Settings.ScaleFactor);
					config_set_uint(currentconfig, "ScaleEnabled", Settings.ScaleEnabled);
					config_set_uint(currentconfig, "Orientation", Settings.Orientation);
					config_set_uint(currentconfig, "OverscanEnabled", Settings.PS3OverscanEnabled);
					config_set_uint(currentconfig, "OverscanAmount", Settings.PS3OverscanAmount);
					config_file_write(currentconfig, filepath);
				}
			}
			break;
		case INPUT_PRESET_FILE:
			{
				bool filename_entered = false;
				char filename_tmp[256];
				oskutil_write_initial_message(&oskutil_handle, L"example");
				oskutil_write_message(&oskutil_handle, L"Enter filename for preset (with no file extension)");
				oskutil_start(&oskutil_handle);

				while(OSK_IS_RUNNING(oskutil_handle))
				{
					/* OSK Util gets updated */
					glClear(GL_COLOR_BUFFER_BIT);
					ps3graphics_draw_menu();
					_jsPlatformSwapBuffers(psgl_device);
					cell_console_poll();
					cellSysutilCheckCallback();
				}

				if(oskutil_handle.text_can_be_fetched)
				{
					strncpy(filename_tmp, OUTPUT_TEXT_STRING(oskutil_handle), sizeof(filename_tmp));
					snprintf(filepath, sizeof(filepath), "%s/%s.conf", INPUT_PRESETS_DIR_PATH, filename_tmp);
					filename_entered = true;
				}

				if(filename_entered)
				{
					char filetitle_tmp[512];
					oskutil_write_initial_message(&oskutil_handle, L"Example file title");
					oskutil_write_message(&oskutil_handle, L"Enter title for preset");
					oskutil_start(&oskutil_handle);

					while(OSK_IS_RUNNING(oskutil_handle))
					{
						/* OSK Util gets updated */
						glClear(GL_COLOR_BUFFER_BIT);
						ps3graphics_draw_menu();
						_jsPlatformSwapBuffers(psgl_device);
						cell_console_poll();
						cellSysutilCheckCallback();
					}

					if(oskutil_handle.text_can_be_fetched)
						snprintf(filetitle_tmp, sizeof(filetitle_tmp), "%s", OUTPUT_TEXT_STRING(oskutil_handle));
					else
						snprintf(filetitle_tmp, sizeof(filetitle_tmp), "%s", "Custom");


					if(!file_exists(filepath))
					{
						FILE * f = fopen(filepath, "w");
						fclose(f);
					}

					emulator_set_controls(filepath, WRITE_CONTROLS, filetitle_tmp);
				}
			}
			break;
	}
}

/* PS3 frontend - sound-related settings */

void emulator_toggle_sound(uint64_t soundmode)
{
	struct cell_audio_params params;
	memset(&params, 0, sizeof(params));
	params.channels = 2;
	params.samplerate = 48000;
	params.buffer_size = 8192;
	params.sample_cb = NULL;
	params.userdata = NULL;

	switch(soundmode)
	{
		case SOUND_MODE_RSOUND:
			params.device = Settings.RSoundServerIPAddress;
			params.headset = 0;
			break;
		case SOUND_MODE_HEADSET:
			params.device = NULL;
			params.headset = 1;
			break;
		case SOUND_MODE_NORMAL: 
			params.device = NULL;
			params.headset = 0;
			break;
	}

	if(audio_handle)
	{
		audio_driver->free(audio_handle);
		audio_handle = NULL;
	}

	if(soundmode == SOUND_MODE_RSOUND)
	{
		audio_driver = &cell_audio_rsound;
		audio_handle =  audio_driver->init(&params);

		if(!audio_handle || !(strlen(Settings.RSoundServerIPAddress) > 0))
		{
			audio_driver = &cell_audio_audioport;
			audio_handle = audio_driver->init(&params);
			Settings.SoundMode = SOUND_MODE_NORMAL;
			dialog_is_running = true;
			cellMsgDialogOpen2(CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR|CELL_MSGDIALOG_TYPE_BG_VISIBLE|\
			CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE|CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_OFF|\
			CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_OK,\
			"Couldn't connect to RSound server at specified IP address. Falling back to\
			regular audio.",cb_dialog_ok,NULL,NULL);

			while(dialog_is_running && is_running)
			{
				glClear(GL_COLOR_BUFFER_BIT);
				_jsPlatformSwapBuffers(psgl_device);
				cell_console_poll();
				cellSysutilCheckCallback();	
			}
		}
      }
      else
      {
         audio_driver = &cell_audio_audioport;
         audio_handle =  audio_driver->init(&params);
      }
}

/* Emulator-specific functions - implementations needed by emulator API */

void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
	// Make PC compatible copy
	r >>= 3;
	g >>= 3;
	b >>= 3;
	ps3graphics_palette[index] = (r << 10) | (g << 5) | (b << 0);
}

void FCEUD_VideoChanged()
{
	if (ps3graphics_get_current_resolution() == CELL_VIDEO_OUT_RESOLUTION_576)
	{
		if(ps3graphics_check_resolution(CELL_VIDEO_OUT_RESOLUTION_576))
		{
			switch(ps3graphics_get_pal60hz())
			{
				case 0:
					//PAL60 is OFF
					if(FCEUGameInfo->vidsys == 0)
					{
						Settings.PS3PALTemporalMode60Hz = true;
						ps3graphics_set_pal60hz(Settings.PS3PALTemporalMode60Hz);
						ps3graphics_switch_resolution(ps3graphics_get_current_resolution(), Settings.PS3PALTemporalMode60Hz, Settings.TripleBuffering, Settings.ScaleEnabled, Settings.ScaleFactor);
					}
					break;
				case 1:
					//PAL60 is ON
					if(FCEUGameInfo->vidsys == 1)
					{
						Settings.PS3PALTemporalMode60Hz = false;
						ps3graphics_set_pal60hz(Settings.PS3PALTemporalMode60Hz);
						ps3graphics_switch_resolution(ps3graphics_get_current_resolution(), Settings.PS3PALTemporalMode60Hz, Settings.TripleBuffering, Settings.ScaleEnabled, Settings.ScaleFactor);
					}
					break;
			}
		}
	}
}

// dummy functions

bool FCEUD_ShouldDrawInputAids() { return 1; }
void FCEUD_Message(const char *s) { printf("MESSAGE: %s\n", s); }
void FCEUD_PrintError(const char *s) { printf("ERROR: %s\n", s); }
const char * GetKeyboard(void) { return ""; }

struct st_palettes palettes[] = {
	{ "asqrealc", "AspiringSquire's Real palette",
		{ 0x6c6c6c, 0x00268e, 0x0000a8, 0x400094,
			0x700070, 0x780040, 0x700000, 0x621600,
			0x442400, 0x343400, 0x005000, 0x004444,
			0x004060, 0x000000, 0x101010, 0x101010,
			0xbababa, 0x205cdc, 0x3838ff, 0x8020f0,
			0xc000c0, 0xd01474, 0xd02020, 0xac4014,
			0x7c5400, 0x586400, 0x008800, 0x007468,
			0x00749c, 0x202020, 0x101010, 0x101010,
			0xffffff, 0x4ca0ff, 0x8888ff, 0xc06cff,
			0xff50ff, 0xff64b8, 0xff7878, 0xff9638,
			0xdbab00, 0xa2ca20, 0x4adc4a, 0x2ccca4,
			0x1cc2ea, 0x585858, 0x101010, 0x101010,
			0xffffff, 0xb0d4ff, 0xc4c4ff, 0xe8b8ff,
			0xffb0ff, 0xffb8e8, 0xffc4c4, 0xffd4a8,
			0xffe890, 0xf0f4a4, 0xc0ffc0, 0xacf4f0,
			0xa0e8ff, 0xc2c2c2, 0x202020, 0x101010 }
	},
	{ "loopy", "Loopy's palette",
		{ 0x757575, 0x271b8f, 0x0000ab, 0x47009f,
			0x8f0077, 0xab0013, 0xa70000, 0x7f0b00,
			0x432f00, 0x004700, 0x005100, 0x003f17,
			0x1b3f5f, 0x000000, 0x000000, 0x000000,
			0xbcbcbc, 0x0073ef, 0x233bef, 0x8300f3,
			0xbf00bf, 0xe7005b, 0xdb2b00, 0xcb4f0f,
			0x8b7300, 0x009700, 0x00ab00, 0x00933b,
			0x00838b, 0x000000, 0x000000, 0x000000,
			0xffffff, 0x3fbfff, 0x5f97ff, 0xa78bfd,
			0xf77bff, 0xff77b7, 0xff7763, 0xff9b3b,
			0xf3bf3f, 0x83d313, 0x4fdf4b, 0x58f898,
			0x00ebdb, 0x000000, 0x000000, 0x000000,
			0xffffff, 0xabe7ff, 0xc7d7ff, 0xd7cbff,
			0xffc7ff, 0xffc7db, 0xffbfb3, 0xffdbab,
			0xffe7a3, 0xe3ffa3, 0xabf3bf, 0xb3ffcf,
			0x9ffff3, 0x000000, 0x000000, 0x000000 }
	},
	{ "quor", "Quor's palette",
		{ 0x3f3f3f, 0x001f3f, 0x00003f, 0x1f003f,
			0x3f003f, 0x3f0020, 0x3f0000, 0x3f2000,
			0x3f3f00, 0x203f00, 0x003f00, 0x003f20,
			0x003f3f, 0x000000, 0x000000, 0x000000,
			0x7f7f7f, 0x405f7f, 0x40407f, 0x5f407f,
			0x7f407f, 0x7f4060, 0x7f4040, 0x7f6040,
			0x7f7f40, 0x607f40, 0x407f40, 0x407f60,
			0x407f7f, 0x000000, 0x000000, 0x000000,
			0xbfbfbf, 0x809fbf, 0x8080bf, 0x9f80bf,
			0xbf80bf, 0xbf80a0, 0xbf8080, 0xbfa080,
			0xbfbf80, 0xa0bf80, 0x80bf80, 0x80bfa0,
			0x80bfbf, 0x000000, 0x000000, 0x000000,
			0xffffff, 0xc0dfff, 0xc0c0ff, 0xdfc0ff,
			0xffc0ff, 0xffc0e0, 0xffc0c0, 0xffe0c0,
			0xffffc0, 0xe0ffc0, 0xc0ffc0, 0xc0ffe0,
			0xc0ffff, 0x000000, 0x000000, 0x000000 }
	},
	{ "chris", "Chris Covell's palette",
		{ 0x808080, 0x003DA6, 0x0012B0, 0x440096,
			0xA1005E, 0xC70028, 0xBA0600, 0x8C1700,
			0x5C2F00, 0x104500, 0x054A00, 0x00472E,
			0x004166, 0x000000, 0x050505, 0x050505,
			0xC7C7C7, 0x0077FF, 0x2155FF, 0x8237FA,
			0xEB2FB5, 0xFF2950, 0xFF2200, 0xD63200,
			0xC46200, 0x358000, 0x058F00, 0x008A55,
			0x0099CC, 0x212121, 0x090909, 0x090909,
			0xFFFFFF, 0x0FD7FF, 0x69A2FF, 0xD480FF,
			0xFF45F3, 0xFF618B, 0xFF8833, 0xFF9C12,
			0xFABC20, 0x9FE30E, 0x2BF035, 0x0CF0A4,
			0x05FBFF, 0x5E5E5E, 0x0D0D0D, 0x0D0D0D,
			0xFFFFFF, 0xA6FCFF, 0xB3ECFF, 0xDAABEB,
			0xFFA8F9, 0xFFABB3, 0xFFD2B0, 0xFFEFA6,
			0xFFF79C, 0xD7E895, 0xA6EDAF, 0xA2F2DA,
			0x99FFFC, 0xDDDDDD, 0x111111, 0x111111 }
	},
	{ "matt", "Matthew Conte's palette",
		{ 0x808080, 0x0000bb, 0x3700bf, 0x8400a6,
			0xbb006a, 0xb7001e, 0xb30000, 0x912600,
			0x7b2b00, 0x003e00, 0x00480d, 0x003c22,
			0x002f66, 0x000000, 0x050505, 0x050505,
			0xc8c8c8, 0x0059ff, 0x443cff, 0xb733cc,
			0xff33aa, 0xff375e, 0xff371a, 0xd54b00,
			0xc46200, 0x3c7b00, 0x1e8415, 0x009566,
			0x0084c4, 0x111111, 0x090909, 0x090909,
			0xffffff, 0x0095ff, 0x6f84ff, 0xd56fff,
			0xff77cc, 0xff6f99, 0xff7b59, 0xff915f,
			0xffa233, 0xa6bf00, 0x51d96a, 0x4dd5ae,
			0x00d9ff, 0x666666, 0x0d0d0d, 0x0d0d0d,
			0xffffff, 0x84bfff, 0xbbbbff, 0xd0bbff,
			0xffbfea, 0xffbfcc, 0xffc4b7, 0xffccae,
			0xffd9a2, 0xcce199, 0xaeeeb7, 0xaaf7ee,
			0xb3eeff, 0xdddddd, 0x111111, 0x111111 }
	},
	{ "pasofami", "PasoFami/99 palette",
		{ 0x7f7f7f, 0x0000ff, 0x0000bf, 0x472bbf,
			0x970087, 0xab0023, 0xab1300, 0x8b1700,
			0x533000, 0x007800, 0x006b00, 0x005b00,
			0x004358, 0x000000, 0x000000, 0x000000,
			0xbfbfbf, 0x0078f8, 0x0058f8, 0x6b47ff,
			0xdb00cd, 0xe7005b, 0xf83800, 0xe75f13,
			0xaf7f00, 0x00b800, 0x00ab00, 0x00ab47,
			0x008b8b, 0x000000, 0x000000, 0x000000,
			0xf8f8f8, 0x3fbfff, 0x6b88ff, 0x9878f8,
			0xf878f8, 0xf85898, 0xf87858, 0xffa347,
			0xf8b800, 0xb8f818, 0x5bdb57, 0x58f898,
			0x00ebdb, 0x787878, 0x000000, 0x000000,
			0xffffff, 0xa7e7ff, 0xb8b8f8, 0xd8b8f8,
			0xf8b8f8, 0xfba7c3, 0xf0d0b0, 0xffe3ab,
			0xfbdb7b, 0xd8f878, 0xb8f8b8, 0xb8f8d8,
			0x00ffff, 0xf8d8f8, 0x000000, 0x000000 }
	},
	{ "crashman", "CrashMan's palette",
		{ 0x585858, 0x001173, 0x000062, 0x472bbf,
			0x970087, 0x910009, 0x6f1100, 0x4c1008,
			0x371e00, 0x002f00, 0x005500, 0x004d15,
			0x002840, 0x000000, 0x000000, 0x000000,
			0xa0a0a0, 0x004499, 0x2c2cc8, 0x590daa,
			0xae006a, 0xb00040, 0xb83418, 0x983010,
			0x704000, 0x308000, 0x207808, 0x007b33,
			0x1c6888, 0x000000, 0x000000, 0x000000,
			0xf8f8f8, 0x267be1, 0x5870f0, 0x9878f8,
			0xff73c8, 0xf060a8, 0xd07b37, 0xe09040,
			0xf8b300, 0x8cbc00, 0x40a858, 0x58f898,
			0x00b7bf, 0x787878, 0x000000, 0x000000,
			0xffffff, 0xa7e7ff, 0xb8b8f8, 0xd8b8f8,
			0xe6a6ff, 0xf29dc4, 0xf0c0b0, 0xfce4b0,
			0xe0e01e, 0xd8f878, 0xc0e890, 0x95f7c8,
			0x98e0e8, 0xf8d8f8, 0x000000, 0x000000 }
	},
	{ "mess", "MESS palette",
		{ 0x747474, 0x24188c, 0x0000a8, 0x44009c,
			0x8c0074, 0xa80010, 0xa40000, 0x7c0800,
			0x402c00, 0x004400, 0x005000, 0x003c14,
			0x183c5c, 0x000000, 0x000000, 0x000000,
			0xbcbcbc, 0x0070ec, 0x2038ec, 0x8000f0,
			0xbc00bc, 0xe40058, 0xd82800, 0xc84c0c,
			0x887000, 0x009400, 0x00a800, 0x009038,
			0x008088, 0x000000, 0x000000, 0x000000,
			0xfcfcfc, 0x3cbcfc, 0x5c94fc, 0x4088fc,
			0xf478fc, 0xfc74b4, 0xfc7460, 0xfc9838,
			0xf0bc3c, 0x80d010, 0x4cdc48, 0x58f898,
			0x00e8d8, 0x000000, 0x000000, 0x000000,
			0xfcfcfc, 0xa8e4fc, 0xc4d4fc, 0xd4c8fc,
			0xfcc4fc, 0xfcc4d8, 0xfcbcb0, 0xfcd8a8,
			0xfce4a0, 0xe0fca0, 0xa8f0bc, 0xb0fccc,
			0x9cfcf0, 0x000000, 0x000000, 0x000000 }
	},
	{ "zaphod-cv", "Zaphod's VS Castlevania palette",
		{ 0x7f7f7f, 0xffa347, 0x0000bf, 0x472bbf,
			0x970087, 0xf85898, 0xab1300, 0xf8b8f8,
			0xbf0000, 0x007800, 0x006b00, 0x005b00,
			0xffffff, 0x9878f8, 0x000000, 0x000000,
			0xbfbfbf, 0x0078f8, 0xab1300, 0x6b47ff,
			0x00ae00, 0xe7005b, 0xf83800, 0x7777ff,
			0xaf7f00, 0x00b800, 0x00ab00, 0x00ab47,
			0x008b8b, 0x000000, 0x000000, 0x472bbf,
			0xf8f8f8, 0xffe3ab, 0xf87858, 0x9878f8,
			0x0078f8, 0xf85898, 0xbfbfbf, 0xffa347,
			0xc800c8, 0xb8f818, 0x7f7f7f, 0x007800,
			0x00ebdb, 0x000000, 0x000000, 0xffffff,
			0xffffff, 0xa7e7ff, 0x5bdb57, 0xe75f13,
			0x004358, 0x0000ff, 0xe7005b, 0x00b800,
			0xfbdb7b, 0xd8f878, 0x8b1700, 0xffe3ab,
			0x00ffff, 0xab0023, 0x000000, 0x000000 }
	},
	{ "zaphod-smb", "Zaphod's VS SMB palette",
		{ 0x626a00, 0x0000ff, 0x006a77, 0x472bbf,
			0x970087, 0xab0023, 0xab1300, 0xb74800,
			0xa2a2a2, 0x007800, 0x006b00, 0x005b00,
			0xffd599, 0xffff00, 0x009900, 0x000000,
			0xff66ff, 0x0078f8, 0x0058f8, 0x6b47ff,
			0x000000, 0xe7005b, 0xf83800, 0xe75f13,
			0xaf7f00, 0x00b800, 0x5173ff, 0x00ab47,
			0x008b8b, 0x000000, 0x91ff88, 0x000088,
			0xf8f8f8, 0x3fbfff, 0x6b0000, 0x4855f8,
			0xf878f8, 0xf85898, 0x595958, 0xff009d,
			0x002f2f, 0xb8f818, 0x5bdb57, 0x58f898,
			0x00ebdb, 0x787878, 0x000000, 0x000000,
			0xffffff, 0xa7e7ff, 0x590400, 0xbb0000,
			0xf8b8f8, 0xfba7c3, 0xffffff, 0x00e3e1,
			0xfbdb7b, 0xffae00, 0xb8f8b8, 0xb8f8d8,
			0x00ff00, 0xf8d8f8, 0xffaaaa, 0x004000 }
	},
	{ "vs-drmar", "VS Dr. Mario palette",
		{ 0x5f97ff, 0x000000, 0x000000, 0x47009f,
			0x00ab00, 0xffffff, 0xabe7ff, 0x000000,
			0x000000, 0x000000, 0x000000, 0x000000,
			0xe7005b, 0x000000, 0x000000, 0x000000,
			0x5f97ff, 0x000000, 0x000000, 0x000000,
			0x000000, 0x8b7300, 0xcb4f0f, 0x000000,
			0xbcbcbc, 0x000000, 0x000000, 0x000000,
			0x000000, 0x000000, 0x000000, 0x000000,
			0x00ebdb, 0x000000, 0x000000, 0x000000,
			0x000000, 0xff9b3b, 0x000000, 0x000000,
			0x83d313, 0x000000, 0x3fbfff, 0x000000,
			0x0073ef, 0x000000, 0x000000, 0x000000,
			0x00ebdb, 0x000000, 0x000000, 0x000000,
			0x000000, 0x000000, 0xf3bf3f, 0x000000,
			0x005100, 0x000000, 0xc7d7ff, 0xffdbab,
			0x000000, 0x000000, 0x000000, 0x000000 }
	},
	{ "vs-cv", "VS Castlevania palette",
		{ 0xaf7f00, 0xffa347, 0x008b8b, 0x472bbf,
			0x970087, 0xf85898, 0xab1300, 0xf8b8f8,
			0xf83800, 0x007800, 0x006b00, 0x005b00,
			0xffffff, 0x9878f8, 0x00ab00, 0x000000,
			0xbfbfbf, 0x0078f8, 0xab1300, 0x6b47ff,
			0x000000, 0xe7005b, 0xf83800, 0x6b88ff,
			0xaf7f00, 0x00b800, 0x6b88ff, 0x00ab47,
			0x008b8b, 0x000000, 0x000000, 0x472bbf,
			0xf8f8f8, 0xffe3ab, 0xf87858, 0x9878f8,
			0x0078f8, 0xf85898, 0xbfbfbf, 0xffa347,
			0x004358, 0xb8f818, 0x7f7f7f, 0x007800,
			0x00ebdb, 0x000000, 0x000000, 0xffffff,
			0xffffff, 0xa7e7ff, 0x5bdb57, 0x6b88ff,
			0x004358, 0x0000ff, 0xe7005b, 0x00b800,
			0xfbdb7b, 0xffa347, 0x8b1700, 0xffe3ab,
			0xb8f818, 0xab0023, 0x000000, 0x007800 }
	},
	{ "vs-smb", "VS SMB/VS Ice Climber palette",
		{ 0xaf7f00, 0x0000ff, 0x008b8b, 0x472bbf,
			0x970087, 0xab0023, 0x0000ff, 0xe75f13,
			0xbfbfbf, 0x007800, 0x5bdb57, 0x005b00,
			0xf0d0b0, 0xffe3ab, 0x00ab00, 0x000000,
			0xbfbfbf, 0x0078f8, 0x0058f8, 0x6b47ff,
			0x000000, 0xe7005b, 0xf83800, 0xf87858,
			0xaf7f00, 0x00b800, 0x6b88ff, 0x00ab47,
			0x008b8b, 0x000000, 0x000000, 0x3fbfff,
			0xf8f8f8, 0x006b00, 0x8b1700, 0x9878f8,
			0x6b47ff, 0xf85898, 0x7f7f7f, 0xe7005b,
			0x004358, 0xb8f818, 0x0078f8, 0x58f898,
			0x00ebdb, 0xfbdb7b, 0x000000, 0x000000,
			0xffffff, 0xa7e7ff, 0xb8b8f8, 0xf83800,
			0xf8b8f8, 0xfba7c3, 0xffffff, 0x00ffff,
			0xfbdb7b, 0xffa347, 0xb8f8b8, 0xb8f8d8,
			0xb8f818, 0xf8d8f8, 0x000000, 0x007800 }
	}
};

/* Emulator-specific - palette  */

static void emulator_set_custom_palette()
{
	if ( Settings.FCEUPalette == 0 )
		FCEU_ResetPalette();	// Do palette reset
	else
	{
		// Now setup this palette
		uint8 i,r,g,b;

		for ( i = 0; i < 64; i++ )
		{
			r = palettes[Settings.FCEUPalette-1].data[i] >> 16;
			g = ( palettes[Settings.FCEUPalette-1].data[i] & 0xff00 ) >> 8;
			b = ( palettes[Settings.FCEUPalette-1].data[i] & 0xff );
			FCEUD_SetPalette( i, r, g, b);
			FCEUD_SetPalette( i+64, r, g, b);
			FCEUD_SetPalette( i+128, r, g, b);
			FCEUD_SetPalette( i+192, r, g, b);
		}
	}
}


/* PS3 Frontend input code */

static void emulator_set_input(void)
{
	InputDPR = &JSReturn;
	//commented out Zapper code
	#if 0
	zapperdata[0] = NULL;
	zapperdata[1] = NULL;
	myzappers[0][0] = myzappers[1][0] = 128;
	myzappers[0][1] = myzappers[1][1] = 120;
	myzappers[0][2] = myzappers[1][2] = 0;
	FCEUI_SetInput(1, SI_ZAPPER, myzappers[1], 0);
	#endif

	FCEUI_SetInput(0, SI_GAMEPAD, InputDPR, 0);
	FCEUI_SetInput(1, SI_GAMEPAD, InputDPR, 0);
}

static void ingame_menu_enable (int enable)
{
	is_running = 0;
	is_ingame_menu_running = enable;
}

static void special_actions(int specialbuttonmap)
{
	if(specialbuttonmap & BTN_CHEATENABLE)
	{
		if (FCEUI_ToggleCheat(Settings.CurrentCheatPosition))
			snprintf(special_action_msg, sizeof(special_action_msg), "Activated cheat: %d", Settings.CurrentCheatPosition);
		else
			snprintf(special_action_msg, sizeof(special_action_msg), "Disabled cheat: %d", Settings.CurrentCheatPosition);
		special_action_msg_expired = ps3graphics_set_text_message_speed(60);
	}

	if(frame_count < special_action_msg_expired)
	{
	}
	else
	{
		if(specialbuttonmap & BTN_EXITTOMENU)
		{
			is_running = 0;
			mode_switch = MODE_MENU;
			set_text_message("", 15);
		}
	}

	if(specialbuttonmap & BTN_DECREMENTSAVE)
	{
		emulator_decrement_current_save_state_slot();
	}
	if(specialbuttonmap & BTN_INCREMENTSAVE)
	{
		emulator_increment_current_save_state_slot();
	}
	if(specialbuttonmap & BTN_QUICKSAVE)
	{
		emulator_save_current_save_state_slot();
	}
	if(specialbuttonmap & BTN_QUICKLOAD)
	{
		emulator_load_current_save_state_slot();
	}
	if(specialbuttonmap & BTN_DECREMENT_PALETTE)
	{
		if(Settings.FCEUPalette != 0)
		{
			Settings.FCEUPalette--;
			emulator_set_custom_palette();
			snprintf(special_action_msg, sizeof(special_action_msg), "Palette #%d (%s)", Settings.FCEUPalette, palettes[Settings.FCEUPalette].desc);
			special_action_msg_expired = ps3graphics_set_text_message_speed(60);
		}
	}
	if(specialbuttonmap & BTN_INCREMENT_PALETTE)
	{
		Settings.FCEUPalette++;
		emulator_set_custom_palette();
		snprintf(special_action_msg, sizeof(special_action_msg), "Palette #%d (%s)", Settings.FCEUPalette, palettes[Settings.FCEUPalette].desc);
		special_action_msg_expired = ps3graphics_set_text_message_speed(60);
	}

	if(frame_count < special_action_msg_expired)
	{
	}
	else
	{
		if(specialbuttonmap & BTN_INGAME_MENU)
		{
			ingame_menu_enable(1);
		}
	}

	if(specialbuttonmap & BTN_FASTFORWARD)
	{
		if(frame_count < special_action_msg_expired)
		{
		}
		else
		{
			Settings.Throttled = !Settings.Throttled;
			emulator_toggle_throttle(Settings.Throttled);
		}
	}
}

#define special_button_mappings(controllerno, specialbuttonmap, condition) \
	if(condition) \
	{ \
		if(specialbuttonmap <= BTN_LASTGAMEBUTTON) \
			pad[controllerno] |=  specialbuttonmap; \
		else \
			special_action_to_execute = specialbuttonmap; \
	}

// emulator-specific - commented out Zapper code
#if 0
static int pos_x = 0;
static int pos_y = 0;

#define ZAPPERPADCAL 20

#define UpdateZapperCursor() \
        if (CTRL_AXIS_LSTICK_X(state) > ZAPPERPADCAL) \
        { \
                pos_x += (CTRL_AXIS_LSTICK_X(state)*1.0)/ZAPPERPADCAL; \
                if (pos_x > 256) pos_x = 256; \
        } \
        if (CTRL_AXIS_LSTICK_X(state) < -ZAPPERPADCAL) \
        { \
                pos_x -= (CTRL_AXIS_LSTICK_X(state)*-1.0)/ZAPPERPADCAL; \
                if (pos_x < 0) pos_x = 0; \
        } \
	\
        if (CRL_AXIS_LSTICK_Y(state) < -ZAPPERPADCAL) \
        { \
                pos_y += (CTRL_AXIS_LSTICK_Y(state)*-1.0)/ZAPPERPADCAL; \
                if (pos_y > 224) pos_y = 224; \
        } \
        if (CTRL_AXIS_LSTICK_Y(state) > ZAPPERPADCAL) \
        { \
                pos_y -= (CTRL_AXIS_LSTICK_Y(state)*1.0)/ZAPPERPADCAL; \
                if (pos_y < 0) pos_y = 0; \
        }
#endif
	
#if 0
static bool first = true;
static int _x = 0;
static int _y = 0;

#define ZAPPERCALC 12
#endif

//broken zapper code right now - crosshair position is correct, but appears to be out of sync with actual position on screen
#if 0
		if (first)
		{
			pos_x = 0;
			pos_y = 0;
			first = false;
		}
		else
		{
			_x += (CTRL_AXIS_LSTICK_X(state) - 128) / ZAPPERCALC;
			_y += (CTRL_AXIS_LSTICK_Y(state) - 128) / ZAPPERCALC;

			if(_x > 256)
				_x = 256;
			if(_x < 0)
				_x = 0;

			if(_y > 240)
				_y = 240;
			if(_y < 0)
				_y = 0;

			pos_x = _x;
			pos_y = _y;
		}


		myzappers[1][0] = pos_x;
		myzappers[1][1] = pos_y;

		if(CTRL_SQUARE(state))
			myzappers[1][2] |= 1;
		else
			myzappers[1][2] = 0;
#endif

static void emulator_input_loop() 
{
	static uint64_t old_state[MAX_PADS];
	unsigned char pad[2] = {0, 0};
	uint32_t pads_connected = cell_pad_input_pads_connected();

	for (uint8_t i = 0; i < pads_connected; i++)
	{
		uint32_t special_action_to_execute = 0;
		const uint64_t state = cell_pad_input_poll_device(i);
		const uint64_t button_was_pressed = old_state[i] & (old_state[i] ^ state);
		const uint64_t button_was_not_held = ~(old_state[i] & state);
		special_button_mappings(i, control_binds[i][CTRL_UP_DEF], (CTRL_UP(state) || CTRL_LSTICK_UP(state)));
		special_button_mappings(i, control_binds[i][CTRL_DOWN_DEF], (CTRL_DOWN(state) || CTRL_LSTICK_DOWN(state)));
		special_button_mappings(i, control_binds[i][CTRL_LEFT_DEF], (CTRL_LEFT(state) || CTRL_LSTICK_LEFT(state)));
		special_button_mappings(i, control_binds[i][CTRL_RIGHT_DEF], (CTRL_RIGHT(state) || CTRL_LSTICK_RIGHT(state)));
		special_button_mappings(i, control_binds[i][CTRL_SQUARE_DEF], (CTRL_SQUARE(state)));
		special_button_mappings(i, control_binds[i][CTRL_CROSS_DEF], (CTRL_CROSS(state)));
		special_button_mappings(i, control_binds[i][CTRL_CIRCLE_DEF], (CTRL_CIRCLE(state)));
		special_button_mappings(i, control_binds[i][CTRL_TRIANGLE_DEF], (CTRL_TRIANGLE(state)));
		special_button_mappings(i, control_binds[i][CTRL_START_DEF], (CTRL_START(state)));
		special_button_mappings(i, control_binds[i][CTRL_SELECT_DEF], (CTRL_SELECT(state)));
		special_button_mappings(i, control_binds[i][CTRL_L1_DEF], (CTRL_L1(state)));
		special_button_mappings(i, control_binds[i][CTRL_L2_DEF], (CTRL_L2(state)));
		special_button_mappings(i, control_binds[i][CTRL_L3_DEF], (CTRL_L3(state) && CTRL_R3(button_was_not_held)));
		special_button_mappings(i, control_binds[i][CTRL_R1_DEF], (CTRL_R1(state)));
		special_button_mappings(i, control_binds[i][CTRL_R2_DEF], (CTRL_R2(state)));
		special_button_mappings(i, control_binds[i][CTRL_R3_DEF], (CTRL_R3(state) && CTRL_L3(button_was_not_held)));
		special_button_mappings(i, control_binds[i][CTRL_R3_L3_DEF], (CTRL_R3(state) && CTRL_L3(state)));
		special_button_mappings(i, control_binds[i][CTRL_R2_R3_DEF], (CTRL_R2(state) && CTRL_R3(state)));
		special_button_mappings(i, control_binds[i][CTRL_L2_R3_DEF], (CTRL_R3(state) && CTRL_L2(state)));
		special_button_mappings(i, control_binds[i][CTRL_L2_L3_DEF], (CTRL_L2(state) && CTRL_L3(state)));
		special_button_mappings(i, control_binds[i][CTRL_L2_RSTICK_RIGHT_DEF], (CTRL_L2(state) && CTRL_RSTICK_RIGHT(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_L2_RSTICK_LEFT_DEF], (CTRL_L2(state) && CTRL_RSTICK_LEFT(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_L2_RSTICK_UP_DEF], (CTRL_L2(state) && CTRL_RSTICK_UP(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_L2_RSTICK_DOWN_DEF], (CTRL_L2(state) && CTRL_RSTICK_DOWN(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_R2_RSTICK_RIGHT_DEF], (CTRL_R2(state) && CTRL_RSTICK_RIGHT(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_R2_RSTICK_LEFT_DEF], (CTRL_R2(state) && CTRL_RSTICK_LEFT(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_R2_RSTICK_UP_DEF], (CTRL_R2(state) && CTRL_RSTICK_UP(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_R2_RSTICK_DOWN_DEF], (CTRL_R2(state) && CTRL_RSTICK_DOWN(button_was_pressed)));
		special_button_mappings(i, control_binds[i][CTRL_RSTICK_DOWN_DEF], CTRL_RSTICK_DOWN(button_was_pressed) && CTRL_R2(button_was_not_held) && CTRL_L2(button_was_not_held));
		special_button_mappings(i, control_binds[i][CTRL_RSTICK_UP_DEF], CTRL_RSTICK_UP(button_was_pressed) && CTRL_R2(button_was_not_held) && CTRL_L2(button_was_not_held));
		special_button_mappings(i, control_binds[i][CTRL_RSTICK_LEFT_DEF], CTRL_RSTICK_LEFT(button_was_pressed) && CTRL_R2(button_was_not_held) && CTRL_L2(button_was_not_held));
		special_button_mappings(i, control_binds[i][CTRL_RSTICK_RIGHT_DEF], CTRL_RSTICK_RIGHT(button_was_pressed) && CTRL_R2(button_was_not_held) && CTRL_L2(button_was_not_held));
		if(special_action_to_execute)
		{
			special_actions(special_action_to_execute);
		}
		old_state[i] = state;
	}
	JSReturn = pad[0] | pad[1] << 8;
}


/* PS3 frontend - ingame menu-related functions */


//FIXME: Turn GREEN into WHITE and RED into LIGHTBLUE once the overlay is in
#define ingame_menu_reset_entry_colors(ingame_menu_item) \
{ \
	for(int i = 0; i < MENU_ITEM_LAST; i++) \
		menuitem_colors[i] = GREEN; \
	menuitem_colors[ingame_menu_item] = RED; \
}

static  void ingame_menu(void)
{
	uint32_t menuitem_colors[MENU_ITEM_LAST];
	char comment[256], msg_temp[256];

	do
	{
		uint64_t state = cell_pad_input_poll_device(0);
		static uint64_t old_state = 0;
		uint64_t stuck_in_loop = 1;
		const uint64_t button_was_pressed = old_state & (old_state ^ state);
		const uint64_t button_was_held = old_state & state;
		static uint64_t blocking = 0;

		ps3graphics_draw(gfx, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT);

		if(frame_count < special_action_msg_expired && blocking)
		{
		}
		else
		{
			if(CTRL_CIRCLE(state))
			{
				is_running = 1;
				ingame_menu_item = 0;
				is_ingame_menu_running = 0;
				mode_switch = MODE_EMULATION;
			}

			switch(ingame_menu_item)
			{
				case MENU_ITEM_LOAD_STATE:
					if(CTRL_CROSS(button_was_pressed))
					{
						emulator_load_current_save_state_slot();
						is_running = 1;
						ingame_menu_item = 0;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EMULATION;
					}

					if(CTRL_LEFT(button_was_pressed) || CTRL_LSTICK_LEFT(button_was_pressed))
					{
						emulator_decrement_current_save_state_slot();
						blocking = 0;
					}

					if(CTRL_RIGHT(button_was_pressed) || CTRL_LSTICK_RIGHT(button_was_pressed))
					{
						emulator_increment_current_save_state_slot();
						blocking = 0;
					}

					ingame_menu_reset_entry_colors(ingame_menu_item);
					strcpy(comment,"Press LEFT or RIGHT to change the current save state slot.\nPress CROSS to load the state from the currently selected save state slot.");
					break;
				case MENU_ITEM_SAVE_STATE:
					if(CTRL_CROSS(button_was_pressed))
					{
						emulator_save_current_save_state_slot();
						is_running = 1;
						ingame_menu_item = 0;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EMULATION;
					}

					if(CTRL_LEFT(button_was_pressed) || CTRL_LSTICK_LEFT(button_was_pressed))
					{
						emulator_decrement_current_save_state_slot();
						blocking = 0;
					}

					if(CTRL_RIGHT(button_was_pressed) || CTRL_LSTICK_RIGHT(button_was_pressed))
					{
						emulator_increment_current_save_state_slot();
						blocking = 0;
					}

					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press LEFT or RIGHT to change the current save state slot.\nPress CROSS to save the state to the currently selected save state slot.");
					break;
				case MENU_ITEM_KEEP_ASPECT_RATIO:
					if(CTRL_LEFT(button_was_pressed) || CTRL_LSTICK_LEFT(button_was_pressed))
					{
						if(Settings.PS3KeepAspect > 0)
						{
							Settings.PS3KeepAspect--;
							ps3graphics_set_aspect_ratio(Settings.PS3KeepAspect, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT, 1);
						}
					}
					if(CTRL_RIGHT(button_was_pressed)  || CTRL_LSTICK_RIGHT(button_was_pressed))
					{
						if(Settings.PS3KeepAspect < LAST_ASPECT_RATIO)
						{
							Settings.PS3KeepAspect++;
							ps3graphics_set_aspect_ratio(Settings.PS3KeepAspect, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT, 1);
						}
					}
					if(CTRL_START(button_was_pressed))
					{
						Settings.PS3KeepAspect = ASPECT_RATIO_4_3;
						ps3graphics_set_aspect_ratio(Settings.PS3KeepAspect, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT, 1);
					}
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press LEFT or RIGHT to change the [Aspect Ratio].\nPress START to reset back to default values.");
					break;
				case MENU_ITEM_OVERSCAN_AMOUNT:
					if(CTRL_LEFT(button_was_pressed)  ||  CTRL_LSTICK_LEFT(button_was_pressed) || CTRL_CROSS(button_was_pressed) || CTRL_LSTICK_LEFT(button_was_held))
					{
						Settings.PS3OverscanAmount--;
						Settings.PS3OverscanEnabled = 1;

						if(Settings.PS3OverscanAmount == 0)
							Settings.PS3OverscanEnabled = 0;
						ps3graphics_set_overscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100, 1);
					}

					if(CTRL_RIGHT(button_was_pressed) || CTRL_LSTICK_RIGHT(button_was_pressed) || CTRL_CROSS(button_was_pressed) || CTRL_LSTICK_RIGHT(button_was_held))
					{
						Settings.PS3OverscanAmount++;
						Settings.PS3OverscanEnabled = 1;

						if(Settings.PS3OverscanAmount == 0)
							Settings.PS3OverscanEnabled = 0;
						ps3graphics_set_overscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100, 1);
					}

					if(CTRL_START(button_was_pressed))
					{
						Settings.PS3OverscanAmount = 0;
						Settings.PS3OverscanEnabled = 0;
						ps3graphics_set_overscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100, 1);
					}
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press LEFT or RIGHT to change the [Overscan] settings.\nPress START to reset back to default values.");
					break;
				case MENU_ITEM_ORIENTATION:
					if(CTRL_LEFT(button_was_pressed)  ||  CTRL_LSTICK_LEFT(button_was_pressed) || CTRL_CROSS(button_was_pressed) || CTRL_LSTICK_LEFT(button_was_held))
					{
						if(Settings.Orientation > 0)
						{
							Settings.Orientation--;
							ps3graphics_set_orientation(Settings.Orientation);
						}
					}

					if(CTRL_RIGHT(button_was_pressed) || CTRL_LSTICK_RIGHT(button_was_pressed) || CTRL_CROSS(button_was_pressed) || CTRL_LSTICK_RIGHT(button_was_held))
					{
						if(Settings.Orientation != MAX_ORIENTATION)
						{
							Settings.Orientation++;
							ps3graphics_set_orientation(Settings.Orientation);
						}
					}

					if(CTRL_START(button_was_pressed))
					{
						Settings.Orientation = NORMAL;
						ps3graphics_set_orientation(Settings.Orientation);
					}
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment, "Press LEFT or RIGHT to change the [Orientation] settings.\nPress START to reset back to default values.");
					break;
				case MENU_ITEM_FRAME_ADVANCE:
					if(CTRL_CROSS(state) || CTRL_R2(state) || CTRL_L2(state))
					{
						is_running = 0;
						ingame_menu_item = MENU_ITEM_FRAME_ADVANCE;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EMULATION;
					}
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press 'CROSS', 'L2' or 'R2' button to step one frame.\nNOTE: Pressing the button rapidly will advance the frame more slowly\nand prevent buttons from being input.");
					break;
				case MENU_ITEM_RESIZE_MODE:
					ingame_menu_reset_entry_colors (ingame_menu_item);
					if(CTRL_CROSS(state))
					{
						ps3graphics_set_aspect_ratio(ASPECT_RATIO_CUSTOM, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT, 1);
						do
						{
							ps3graphics_draw(gfx, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT);
							state = cell_pad_input_poll_device(0);
							ps3graphics_resize_aspect_mode_input_loop(state);
							if(CTRL_CIRCLE(state))
							{
								set_text_message("", 7);
								blocking = 1;
								stuck_in_loop = 0;
							}

							_jsPlatformSwapBuffers(psgl_device);
							cellSysutilCheckCallback();
							old_state = state;
						}while(stuck_in_loop && is_ingame_menu_running);
					}
					strcpy(comment, "Allows you to resize the screen by moving around the two analog sticks.\nPress TRIANGLE to reset to default values, and CIRCLE to go back to the\nin-game menu.");
					break;
				case MENU_ITEM_SCREENSHOT_MODE:
					if(CTRL_CROSS(state))
					{
						while(stuck_in_loop && is_ingame_menu_running)
						{
							state = cell_pad_input_poll_device(0);
							if(CTRL_CIRCLE(state))
							{
								blocking = 0;
								stuck_in_loop = 0;
							}

							ps3graphics_draw(gfx, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT);
							_jsPlatformSwapBuffers(psgl_device);
							cellSysutilCheckCallback();
							old_state = state;
						}
					}

					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment, "Allows you to take a screenshot without any text clutter.\nPress CIRCLE to go back to the in-game menu while in 'Screenshot Mode'.");
					break;
				case MENU_ITEM_RETURN_TO_GAME:
					if(CTRL_CROSS(button_was_pressed))
					{
						is_running = 1;
						ingame_menu_item = 0;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EMULATION;
					} 
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press 'CROSS' to return back to the game.");
					break;
				case MENU_ITEM_RESET:
					if(CTRL_CROSS(button_was_pressed))
					{
						ResetNES();
						is_running = 1;
						ingame_menu_item = 0;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EMULATION;
					} 
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press 'CROSS' to reset the game.");
					break;
				case MENU_ITEM_RESET_FORCE_NTSC_PAL:
					if(CTRL_CROSS(button_was_pressed))
					{
						//need_load_rom = 1;
						is_running = 1;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EMULATION;
						ingame_menu_item = 0;
					} 
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press 'CROSS' to reset the game and force it into PAL videomode.");
					break;
				case MENU_ITEM_RESET_FORCE_PAL_NTSC:
					if(CTRL_CROSS(button_was_pressed))
					{
						//need_load_rom = 1;
						is_running = 1;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EMULATION;
						ingame_menu_item = 0;
					} 
					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press 'CROSS' to reset the game and force it into NTSC videomode.");
					break;
				case MENU_ITEM_RETURN_TO_MENU:
					if(CTRL_CROSS(button_was_pressed))
					{
						is_running = 1;
						ingame_menu_item = 0;
						is_ingame_menu_running = 0;
						mode_switch = MODE_MENU;
					}

					ingame_menu_reset_entry_colors (ingame_menu_item);
					strcpy(comment,"Press 'CROSS' to return to the ROM Browser menu.");
					break;
#ifdef MULTIMAN_SUPPORT
				case MENU_ITEM_RETURN_TO_MULTIMAN:
					if(CTRL_CROSS(button_was_pressed))
					{
						is_running = 0;
						is_ingame_menu_running = 0;
						mode_switch = MODE_EXIT;
					}

					ingame_menu_reset_entry_colors (ingame_menu_item);

					strcpy(comment,"Press 'CROSS' to quit the emulator and return to multiMAN.");
					break;
#endif
				case MENU_ITEM_RETURN_TO_XMB:
					if(CTRL_CROSS(button_was_pressed))
					{
						is_running = 0;
						is_ingame_menu_running = 0;
#ifdef MULTIMAN_SUPPORT
						return_to_MM = false;
#endif
						mode_switch = MODE_EXIT;
					}

					ingame_menu_reset_entry_colors (ingame_menu_item);

					strcpy(comment,"Press 'CROSS' to quit the emulator and return to the XMB.");
					break;
			}

			if(CTRL_UP(button_was_pressed) || CTRL_LSTICK_UP(button_was_pressed))
			{
				if(ingame_menu_item != 0)
					ingame_menu_item--;
			}

			if(CTRL_DOWN(button_was_pressed) || CTRL_LSTICK_DOWN(button_was_pressed))
			{
				if(ingame_menu_item < MENU_ITEM_LAST)
					ingame_menu_item++;
			}
		}

		float x_position = 0.3f;
		float font_size = 1.1f;
		float ypos = 0.19f;
		float ypos_increment = 0.04f;
		cellDbgFontPrintf	(x_position,	0.10f,	1.4f+0.01f,	BLUE,               "Quick Menu");
		cellDbgFontPrintf	(x_position,	0.10f,	1.4f,	WHITE,               "Quick Menu");

		cellDbgFontPrintf	(x_position,	ypos,	font_size+0.01f,	BLUE,	"Load State #%d", Settings.CurrentSaveStateSlot);
		cellDbgFontPrintf	(x_position,	ypos,	font_size,	menuitem_colors[MENU_ITEM_LOAD_STATE],	"Load State #%d", Settings.CurrentSaveStateSlot);

		cellDbgFontPrintf	(x_position,	ypos+(ypos_increment*MENU_ITEM_SAVE_STATE),	font_size+0.01f,	BLUE,	"Save State #%d", Settings.CurrentSaveStateSlot);
		cellDbgFontPrintf	(x_position,	ypos+(ypos_increment*MENU_ITEM_SAVE_STATE),	font_size,	menuitem_colors[MENU_ITEM_SAVE_STATE],	"Save State #%d", Settings.CurrentSaveStateSlot);

		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_KEEP_ASPECT_RATIO)),	font_size+0.01f,	BLUE,	"Aspect Ratio: %s %s %d:%d", ps3graphics_calculate_aspect_ratio_before_game_load() ?"(Auto)" : "", Settings.PS3KeepAspect == LAST_ASPECT_RATIO ? "Custom" : "", (int)ps3graphics_get_aspect_ratio_int(0), (int)ps3graphics_get_aspect_ratio_int(1));
		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_KEEP_ASPECT_RATIO)),	font_size,	menuitem_colors[MENU_ITEM_KEEP_ASPECT_RATIO],	"Aspect Ratio: %s %s %d:%d", ps3graphics_calculate_aspect_ratio_before_game_load() ?"(Auto)" : "", Settings.PS3KeepAspect == LAST_ASPECT_RATIO ? "Custom" : "", (int)ps3graphics_get_aspect_ratio_int(0), (int)ps3graphics_get_aspect_ratio_int(1));

		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_OVERSCAN_AMOUNT)),	font_size+0.01f,	BLUE,	"Overscan: %f", (float)Settings.PS3OverscanAmount/100);
		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_OVERSCAN_AMOUNT)),	font_size,	menuitem_colors[MENU_ITEM_OVERSCAN_AMOUNT],	"Overscan: %f", (float)Settings.PS3OverscanAmount/100);

		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_OVERSCAN_AMOUNT)),	font_size+0.01f,	BLUE,	"Overscan: %f", (float)Settings.PS3OverscanAmount/100);
		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_OVERSCAN_AMOUNT)),	font_size,	menuitem_colors[MENU_ITEM_OVERSCAN_AMOUNT],	"Overscan: %f", (float)Settings.PS3OverscanAmount/100);

		switch(ps3graphics_get_orientation_name())
		{
			case NORMAL:
				snprintf(msg_temp, sizeof(msg_temp), "Normal");
				break;
			case VERTICAL:
				snprintf(msg_temp, sizeof(msg_temp), "Vertical");
				break;
			case FLIPPED:
				snprintf(msg_temp, sizeof(msg_temp), "Flipped");
				break;
			case FLIPPED_ROTATED:
				snprintf(msg_temp, sizeof(msg_temp), "Flipped Rotated");
				break;
		}


		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_ORIENTATION)),	font_size+0.01f,	BLUE,	"Orientation: %s", msg_temp);
		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_ORIENTATION)),	font_size,	menuitem_colors[MENU_ITEM_ORIENTATION],	"Orientation: %s", msg_temp);

		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESIZE_MODE)),	font_size+0.01f,	BLUE,	"Resize Mode");
		cellDbgFontPrintf	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESIZE_MODE)),	font_size,	menuitem_colors[MENU_ITEM_RESIZE_MODE],	"Resize Mode");

		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_FRAME_ADVANCE)),	font_size+0.01f,	BLUE,	"Frame Advance");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_FRAME_ADVANCE)),	font_size,	menuitem_colors[MENU_ITEM_FRAME_ADVANCE],	"Frame Advance");

		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_SCREENSHOT_MODE)),	font_size+0.01f,	BLUE,	"Screenshot Mode");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_SCREENSHOT_MODE)),	font_size,	menuitem_colors[MENU_ITEM_SCREENSHOT_MODE],	"Screenshot Mode");

		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESET)),	font_size+0.01f,	BLUE,	"Reset");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESET)),	font_size,	menuitem_colors[MENU_ITEM_RESET],	"Reset");

		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESET_FORCE_NTSC_PAL)),	font_size+0.01f,	BLUE,	"Reset - Force NTSC to PAL");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESET_FORCE_NTSC_PAL)),	font_size,	menuitem_colors[MENU_ITEM_RESET_FORCE_NTSC_PAL],	"Reset - Force NTSC to PAL");
		cellDbgFontDraw();

		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESET_FORCE_PAL_NTSC)),	font_size+0.01f,	BLUE,	"Reset - Force PAL to NTSC");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RESET_FORCE_PAL_NTSC)),	font_size,	menuitem_colors[MENU_ITEM_RESET_FORCE_PAL_NTSC],	"Reset - Force PAL to NTSC");

		cellDbgFontPuts   (x_position,   (ypos+(ypos_increment*MENU_ITEM_RETURN_TO_GAME)),   font_size+0.01f,  BLUE,  "Return to Game");
		cellDbgFontPuts   (x_position,   (ypos+(ypos_increment*MENU_ITEM_RETURN_TO_GAME)),   font_size,  menuitem_colors[MENU_ITEM_RETURN_TO_GAME],  "Return to Game");

		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RETURN_TO_MENU)),	font_size+0.01f,	BLUE,	"Return to Menu");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RETURN_TO_MENU)),	font_size,	menuitem_colors[MENU_ITEM_RETURN_TO_MENU],	"Return to Menu");
#ifdef MULTIMAN_SUPPORT
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RETURN_TO_MULTIMAN)),	font_size+0.01f,	BLUE,	"Return to multiMAN");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RETURN_TO_MULTIMAN)),	font_size,	menuitem_colors[MENU_ITEM_RETURN_TO_MULTIMAN],	"Return to multiMAN");
#endif
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RETURN_TO_XMB)),	font_size+0.01f,	BLUE,	"Return to XMB");
		cellDbgFontPuts	(x_position,	(ypos+(ypos_increment*MENU_ITEM_RETURN_TO_XMB)),	font_size,	menuitem_colors[MENU_ITEM_RETURN_TO_XMB],	"Return to XMB");

		if(frame_count < special_action_msg_expired)
		{
			cellDbgFontPrintf (0.09f, 0.90f, 1.51f, BLUE,	special_action_msg);
			cellDbgFontPrintf (0.09f, 0.90f, 1.50f, WHITE,	special_action_msg);
		}
		else
		{
			special_action_msg_expired = 0;
			cellDbgFontPrintf (0.09f,   0.90f,   0.98f+0.01f,      BLUE,           comment);
			cellDbgFontPrintf (0.09f,   0.90f,   0.98f,      LIGHTBLUE,           comment);
		}
		cellDbgFontDraw();
		_jsPlatformSwapBuffers(psgl_device);
		old_state = state;
		cellSysutilCheckCallback();
	}while(is_ingame_menu_running);
}

/* PS3 Frontend - main functions */
extern uint8 *XBuf;

#define emulation_loop() \

static void emulator_set_paths()
{
	//emulator-specific
	FCEUI_SetBaseDirectory(usrDirPath);
	FCEUI_SetDirOverride(FCEUIOD_STATE, Settings.PS3PathSaveStates);
	FCEUI_SetDirOverride(FCEUIOD_NV, Settings.PS3PathSRAM);
	FCEUI_SetDirOverride(FCEUIOD_CHEATS, Settings.PS3PathCheats);
}

static void emulator_shutdown()
{
	emulator_save_settings(CONFIG_FILE);

#ifdef PS3_PROFILING
	// shutdown everything
	ps3graphics_DeinitDbgFont();
	ps3graphics_Deinit();

	if (Graphics)
		delete Graphics;

	cellSysmoduleUnloadModule(CELL_SYSMODULE_FS);
	cellSysmoduleUnloadModule(CELL_SYSMODULE_IO);
	cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_SCREENSHOT);
	cellSysutilUnregisterCallback(0);

	exit(0);		// force exit
#else
#ifdef MULTIMAN_SUPPORT
	if(return_to_MM)
	{
		if(audio_handle)
		{
			audio_driver->free(audio_handle);
			audio_handle = NULL;
		}
		sys_spu_initialize(6, 0);
		char multiMAN[512];
		snprintf(multiMAN, sizeof(multiMAN), "%s", "/dev_hdd0/game/BLES80608/USRDIR/RELOAD.SELF");
		sys_game_process_exitspawn2((char*) multiMAN, NULL, NULL, NULL, 0, 2048, SYS_PROCESS_PRIMARY_STACK_SIZE_64K);
		sys_process_exit(0);
	}
	else
#endif
		sys_process_exit(0);
#endif
}

static void emulator_close_game(void)
{
	if(emulator_initialized)
	{
		FCEUI_CloseGame();
		FCEUGameInfo = 0;
	}
}

static void emulator_start()
{
	ps3graphics_set_orientation(Settings.Orientation);

	int32 *sound=0;
	int32 ssize=0;

	emulator_set_paths();

	emulator_set_input();

	emulator_set_custom_palette();

	if(ps3graphics_calculate_aspect_ratio_before_game_load())
		ps3graphics_set_aspect_ratio(Settings.PS3KeepAspect, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT, 1);

	if(Settings.Throttled)
		audio_driver->unpause(audio_handle);
	else
		FCEUI_SetSoundVolume(0);
	
	if(hack_prevent_game_sram_from_being_erased)
	{
		hack_prevent_game_sram_from_being_erased = 0;
		Emulator_RequestLoadROM();
		return;
	}

	do
	{
		FCEUI_Emulate(&gfx, &sound, &ssize);
		ps3graphics_draw(gfx, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT);
		if(frame_count < special_action_msg_expired)
		{
			cellDbgFontPrintf (0.09f, 0.90f, 1.51f, BLUE,	special_action_msg);
			cellDbgFontPrintf (0.09f, 0.90f, 1.50f, WHITE,	special_action_msg);
			cellDbgFontDraw();
		}
		else
			special_action_msg_expired = 0;
		_jsPlatformSwapBuffers(psgl_device);

		if(Settings.Throttled)
			audio_driver->write(audio_handle, (int16_t*)sound, ssize << 1);
		emulator_input_loop();
		cell_console_poll();
		cellSysutilCheckCallback();
	}while (is_running);
}

/* PS3 Frontend - Main ROM loading function */

void Emulator_RequestLoadROM (void)
{
	//emulator-specific
	if (emulator_initialized)
		emulator_close_game();

	if(hack_prevent_game_sram_from_being_erased)
		FCEUGameInfo = FCEUI_LoadGame(DEFAULT_GAME_HACK);
	else
		FCEUGameInfo = FCEUI_LoadGame(current_rom);
}

void emulator_implementation_set_texture(const char * fname)
{
	strcpy(Settings.PS3CurrentBorder, fname);
	ps3graphics_load_menu_texture(TEXTURE_BACKDROP, fname);
	ps3graphics_load_menu_texture(TEXTURE_MENU, DEFAULT_MENU_BORDER_FILE);
}

static void get_path_settings(bool multiman_support)
{
	unsigned int get_type;
	unsigned int get_attributes;
	CellGameContentSize size;
	char dirName[CELL_GAME_DIRNAME_SIZE];

	memset(&size, 0x00, sizeof(CellGameContentSize));

	int ret = cellGameBootCheck(&get_type, &get_attributes, &size, dirName); 
	if(ret < 0)
	{
		printf("cellGameBootCheck() Error: 0x%x\n", ret);
	}
	else
	{
		printf("cellGameBootCheck() OK\n");
		printf("  get_type = [%d] get_attributes = [0x%08x] dirName = [%s]\n", get_type, get_attributes, dirName);
		printf("  hddFreeSizeKB = [%d] sizeKB = [%d] sysSizeKB = [%d]\n", size.hddFreeSizeKB, size.sizeKB, size.sysSizeKB);

		ret = cellGameContentPermit(contentInfoPath, usrDirPath);

		if(multiman_support)
		{
			snprintf(contentInfoPath, sizeof(contentInfoPath), "/dev_hdd0/game/%s", EMULATOR_CONTENT_DIR);
			snprintf(usrDirPath, sizeof(usrDirPath), "/dev_hdd0/game/%s/USRDIR", EMULATOR_CONTENT_DIR);
		}

		if(ret < 0)
		{
			printf("cellGameContentPermit() Error: 0x%x\n", ret);
		}
		else
		{
			printf("cellGameContentPermit() OK\n");
			printf("contentInfoPath:[%s]\n", contentInfoPath);
			printf("usrDirPath:[%s]\n",  usrDirPath);
		}

		/* now we fill in all the variables */
		snprintf(DEFAULT_PRESET_FILE, sizeof(DEFAULT_PRESET_FILE), "%s/presets/stock.conf", usrDirPath);
		snprintf(DEFAULT_BORDER_FILE, sizeof(DEFAULT_BORDER_FILE), "%s/borders/Centered-1080p/mega-man-2.png", usrDirPath);
		snprintf(DEFAULT_MENU_BORDER_FILE, sizeof(DEFAULT_MENU_BORDER_FILE), "%s/borders/Menu/main-menu.png", usrDirPath);
		snprintf(GAME_AWARE_SHADER_DIR_PATH, sizeof(GAME_AWARE_SHADER_DIR_PATH), "%s/gameaware", usrDirPath);
		snprintf(PRESETS_DIR_PATH, sizeof(PRESETS_DIR_PATH), "%s/presets", usrDirPath); 
		snprintf(INPUT_PRESETS_DIR_PATH, sizeof(INPUT_PRESETS_DIR_PATH), "%s/input-presets", usrDirPath); 
		snprintf(BORDERS_DIR_PATH, sizeof(BORDERS_DIR_PATH), "%s/borders", usrDirPath); 
		snprintf(SHADERS_DIR_PATH, sizeof(SHADERS_DIR_PATH), "%s/shaders", usrDirPath);
		snprintf(DEFAULT_SHADER_FILE, sizeof(DEFAULT_SHADER_FILE), "%s/shaders/stock.cg", usrDirPath);
		snprintf(DEFAULT_MENU_SHADER_FILE, sizeof(DEFAULT_MENU_SHADER_FILE), "%s/shaders/Borders/Menu/border-only.cg", usrDirPath);
		snprintf(DEFAULT_GAME_HACK, sizeof(DEFAULT_GAME_HACK), "%s/roms/D-Pad Hero II (USA).nes", usrDirPath);
		snprintf(SYS_CONFIG_FILE, sizeof(SYS_CONFIG_FILE), "%s/fceu.conf", usrDirPath);
	}
}

int main (int argc, char **argv)
{
	// Initialize 6 SPUs but reserve 1 SPU as a raw SPU for PSGL
	sys_spu_initialize(6, 1);

	cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	cellSysmoduleLoadModule(CELL_SYSMODULE_IO);
	cellSysmoduleLoadModule(CELL_SYSMODULE_PNGDEC);
	cellSysmoduleLoadModule(CELL_SYSMODULE_JPGDEC);
	cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_GAME);

	cellSysutilRegisterCallback(0, sysutil_exit_callback, NULL);

#ifdef CELL_DEBUG_CONSOLE
	cellConsoleInit();
	cellConsoleNetworkInitialize();
	cellConsoleNetworkServerInit(-1);
	cellConsoleScreenShotPluginInit();
#endif

#ifdef MULTIMAN_SUPPORT
	return_to_MM = true;

	if(argc > 1)
	{
		mode_switch = MODE_MULTIMAN_STARTUP;
		strncpy(MULTIMAN_GAME_TO_BOOT, argv[1], sizeof(MULTIMAN_GAME_TO_BOOT));
	}
#endif

	get_path_settings(return_to_MM);

	cell_pad_input_init();

	oskutil_init(&oskutil_handle, 0);

	emulator_init_settings();

#if(CELL_SDK_VERSION > 0x340000)
	if (Settings.ScreenshotsEnabled)
	{
		cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_SCREENSHOT);
		CellScreenShotSetParam  screenshot_param = {0, 0, 0, 0};

		screenshot_param.photo_title = EMULATOR_NAME;
		screenshot_param.game_title = EMULATOR_NAME;
		cellScreenShotSetParameter (&screenshot_param);
		cellScreenShotEnable();
	}
#endif

	ps3graphics_new((uint32_t)Settings.PS3CurrentResolution, Settings.PS3KeepAspect, Settings.PS3Smooth, Settings.PS3Smooth2, Settings.PS3CurrentShader, Settings.PS3CurrentShader2, DEFAULT_MENU_SHADER_FILE, Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100, Settings.PS3PALTemporalMode60Hz, Settings.Throttled, Settings.TripleBuffering, Settings.ViewportX, Settings.ViewportY, Settings.ViewportWidth, Settings.ViewportHeight, Settings.ScaleEnabled, Settings.ScaleFactor);

	if(Settings.ApplyShaderPresetOnStartup)
		emulator_implementation_set_shader_preset(Settings.ShaderPresetPath); 

	frame_count = 0;
	ps3graphics_init_dbgfont();

	emulator_initialized = FCEUI_Initialize();
	FCEUI_SetSoundVolume(256);
	FCEUI_Sound(48200);

	emulator_toggle_sound(Settings.SoundMode);

	ps3graphics_load_fragment_shader(DEFAULT_MENU_SHADER_FILE, 2);
	emulator_implementation_set_texture(Settings.PS3CurrentBorder);

#if(CELL_SDK_VERSION > 0x340000)
	cellSysutilEnableBgmPlayback();
#endif
	menu_init();

	do
	{
		switch(mode_switch)
		{
			case MODE_MENU:
				ps3graphics_set_orientation(NORMAL);
				menu_loop();
				break;
			case MODE_EMULATION:
				if(ingame_menu_item != 0)
					is_ingame_menu_running = 1;

				emulator_start();

				if(Settings.Throttled)
					audio_driver->pause(audio_handle);

				if(is_ingame_menu_running)
					ingame_menu();
				break;
			case MODE_MULTIMAN_STARTUP:
				is_running = 1;
				mode_switch = MODE_EMULATION;
				snprintf(current_rom, sizeof(current_rom), MULTIMAN_GAME_TO_BOOT);
				Emulator_RequestLoadROM();
				break;
			case MODE_EXIT:
				emulator_shutdown();
		}
	}while(1);
}
